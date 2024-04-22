#define VERSION 1.3


#define DEBUG 0


#include <Arduino.h>
#include <SensirionI2CSen5x.h>
#include <RTC.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <SPI.h>       // this is needed for display and SD card
#include <SD.h>
#include <Adafruit_ILI9341.h>
#include <Wire.h>      // this is needed for FT6206 and SEN55
#include <Adafruit_FT6206.h>

// Touchscreen stuff
Adafruit_FT6206 capTouch = Adafruit_FT6206(); // creates the touchscreen object

#define screen_CS 10
#define screen_DC 9

Adafruit_ILI9341 screen = Adafruit_ILI9341(screen_CS, screen_DC); // creates the screen object

#define SD_CS 4

// SEN55 stuff
SensirionI2CSen5x sen5x;

const int16_t SEN55_ADDRESS = 0x69;

#define tempOffset 0.0

// sda (green) -> A4
// scl (yellow) -> A5

#define debounce 100 // default delay to prevent debounce presses

#define width 320
#define height 240
#define BOXSIZE 40

#define sampleInterval 1000 
#define samplesBeforeWrite 1

RTCTime now;

String fname;

bool settingDateTime = 1; // flag to prevent the program moving on before time is set

int DD = 0; // global variables for date and time as they are used in many functions
int MM = 0;
int YY = 0;
int hh = 0;
int mm = 0;

bool datalogging = 1; // flag for wether data should be writted to the SD card

int sampleCount = 0; // adjust to change how often data is written to the SD card
int loopCount = 0;   // adjust to change how many measurments are made before taking min/max values

int mode = 0; // current selected mode variable
/*
Home = 0
Temp = 1
Humidity = 2
PM = 3
VOC = 4
NOX = 5
Data = 6
*/

int currentMillis = millis(); // timers for measurment frequency
int previousMillis = millis();

// min/max value variables
float maxTemp = 0;
float minTemp = 1023;
float maxHumidity = 0;
float minHumidity = 1023;
float maxPm1p0 = 0;
float minPm1p0 = 1023;
float maxPm2p5 = 0;
float minPm2p5 = 1023;
float maxPm4p0 = 0;
float minPm4p0 = 1023;
float maxPm10p0 = 0;
float minPm10p0 = 1023;
float maxVoc = 0;
float minVoc = 1023;
float maxNox = 0;
float minNox = 1023;
// raw min/max variables
uint16_t maxRawVoc = 0;
uint16_t minRawVoc = 1023;
uint16_t maxRawNox = 0;
uint16_t minRawNox = 1023;

// create log file metadata of the time and date
void metaDateTime(uint16_t* date, uint16_t* time) {
  RTCTime now;
  RTC.getTime(now); // gets the current date and time
  *date = FAT_DATE(now.getYear(), Month2int(now.getMonth()), now.getDayOfMonth()); // creates a date string from the current date
  *time = FAT_TIME(now.getHour(), now.getMinutes(), now.getSeconds()); // creates a time string from the current time
}

// updates each field depending on keypresses
void updateDateTime(String type, int value) {
  if (type == "DD") {
    screen.setCursor(30, 105);
    screen.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    screen.print("  ");
    screen.setCursor(30, 105);
    screen.print(value);
  } else if (type == "MM") {
    screen.setCursor(90, 105);
    screen.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    screen.print("  ");
    screen.setCursor(90, 105);
    screen.print(value);
  } else if (type == "YY") {
    screen.setCursor(150, 105);
    screen.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    screen.print("  ");
    screen.setCursor(150, 105);
    screen.print(value);
  } else if (type == "hh") {
    screen.setCursor(210, 105);
    screen.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    screen.print("  ");
    screen.setCursor(210, 105);
    screen.print(value);
  } else if (type == "mm") {
    screen.setCursor(270, 105);
    screen.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    screen.print("  ");
    screen.setCursor(270, 105);
    screen.print(value);
  }
  delay(debounce);
}

// reads the current date and time and returns it as a string
String getDateTime() {

  RTC.getTime(now);

  String dateTime;
  dateTime += String(now.getYear());
  dateTime += "-";
  dateTime += String(Month2int(now.getMonth()));
  dateTime += "-";
  dateTime += String(now.getDayOfMonth());
  dateTime += " ";
  dateTime += String(now.getHour());
  dateTime += ":";
  dateTime += String(now.getMinutes());
  dateTime += ":";
  dateTime += String(now.getSeconds());
  return dateTime;
}

// called whenever the screen detects touch in loop(),
// handles all touch events depending on mode and position
void isTouched() {
  TS_Point p = capTouch.getPoint();

  if (settingDateTime) { // section for touches when setting date and time
    if (p.y > width - BOXSIZE && p.x < BOXSIZE){
      settingDateTime = 0;

    } else if (p.x < height-49 && p.x > height-89) {
      switch (p.y) {
        case 20 ... 60:
        if (DD < 31) {
          DD += 1;
        } else {DD = 1;}
        updateDateTime("DD", DD);
        break;

        case 80 ... 120:
        if (MM < 12) {
          MM += 1;
        } else {MM = 1;}
        updateDateTime("MM", MM);
        break;

        case 140 ... 180:
        if (YY < 99) {
          YY += 1;
        } else {YY = 0;}
        updateDateTime("YY", YY);
        break;

        case 200 ... 240:
        if (hh < 23) {
          hh += 1;
        } else {hh = 0;}
        updateDateTime("hh", hh);
        break;

        case 260 ... 300:
        if (mm < 59) {
          mm += 1;
        } else {mm = 0;}
        updateDateTime("mm", mm);
        break;

        default:
        break;
      }

    } else if (p.x < height-138 && p.x > height-178) {
      switch (p.y) {
        case 20 ... 60:
        if (DD > 1) {
          DD --;
        } else {DD = 31;}
        updateDateTime("DD", DD);
        break;

        case 80 ... 120:
        if (MM > 1) {
          MM --;
        } else {MM = 12;}
        updateDateTime("MM", MM);
        break;

        case 140 ... 180:
        if (YY > 0) {
          YY --;
        } else {YY = 99;}
        updateDateTime("YY", YY);
        break;

        case 200 ... 240:
        if (hh > 0) {
          hh --;
        } else {hh = 23;}
        updateDateTime("hh", hh);
        break;

        case 260 ... 300:
        if (mm > 0) {
          mm --;
        } else {mm = 59;}
        updateDateTime("mm", mm);
        break;

        default:
        break;
      }
    }
  
    return;
  }

  if (p.x != 0 || p.y != 0) { // ignores (0, 0) coordinates which pop up on screen release sometimes

    if (mode > 0 && p.y < BOXSIZE && p.x < BOXSIZE) { // if its not home mode, checks if back button pressed
                                                              // and sends it back to the homescreen
      mode = 0;
      delay(debounce);
      homeScreen();
      return;
    } else if (mode == 0 && p.y < BOXSIZE) { // checks if its in home mode (menu panel is active)
                                      // and calls menuPanel() if pressed within the box
      delay(debounce);
      menuPanel(p.x);
    } else if (mode == 6) {
      if (p.x > height - 109 && p.x < height - 69 && p.y > width - 80 && p.y < width - 40) {
        if (datalogging) {
          datalogging = 0;
        } else {datalogging = 1;}
        dataloggingMenu();
        delay(debounce);
      } else if (p.x > height - 169 && p.x < height - 129 && p.y > width - 80 && p.y < width - 40) {
        maxTemp = 0;
        minTemp = 1023;
        maxHumidity = 0;
        minHumidity = 1023;
        maxPm1p0 = 0;
        minPm1p0 = 1023;
        maxPm2p5 = 0;
        minPm2p5 = 1023;
        maxPm4p0 = 0;
        minPm4p0 = 1023;
        maxPm10p0 = 0;
        minPm10p0 = 1023;
        maxVoc = 0;
        minVoc = 1023;
        maxNox = 0;
        minNox = 1023;
        maxRawVoc = 0;
        minRawVoc = 1023;
        maxRawNox = 0;
        minRawNox = 1023;
      }
    }
  }
}

// draws square (BOXSIZE x BOXSIZE) buttons at the specified position with the specified label
void drawButton(int x, int y, String label) {
  screen.fillRect(x, y, BOXSIZE, BOXSIZE, ILI9341_WHITE);
  screen.setTextColor(ILI9341_BLACK);
  int len = label.length();
  switch (len) {
    case 1:
    screen.setCursor(BOXSIZE/2+x-7, BOXSIZE/2+y-10);
    screen.setTextSize(3);
    screen.print(label);
    return;

    case 2:
    screen.setCursor(BOXSIZE/2+x-11, BOXSIZE/2+y-8);
    screen.setTextSize(2);
    screen.print(label);
    return;

    case 3:
    screen.setCursor(BOXSIZE/2+x-17, BOXSIZE/2+y-8);
    screen.setTextSize(2);
    screen.print(label);
    return;
  }  
}

// draws the time setting GUI, checks for keypresses and then sets the RTC time once OK is pressed
void setDateTime() {

  screen.fillScreen(ILI9341_BLACK);

  screen.setTextColor(ILI9341_WHITE);
  screen.setTextSize(2);
  screen.setCursor(30, 13);
  screen.print("DD / MM / YY   hh : mm");

  screen.setCursor(30, 105);
  screen.print("00 / 00 / 00   00 : 00");

  // + buttons
  drawButton(20, 49, "+");
  drawButton(80, 49, "+");
  drawButton(140, 49, "+");
  drawButton(200, 49, "+");
  drawButton(260, 49, "+");

  // - buttons
  drawButton(20, 138, "-");
  drawButton(80, 138, "-");
  drawButton(140, 138, "-");
  drawButton(200, 138, "-");
  drawButton(260, 138, "-");

  // ok button
  drawButton(width-BOXSIZE, height-BOXSIZE, "OK");

  do {
    isTouched();
  } while (settingDateTime);

  RTC.begin();
  RTCTime startTime(DD, (Month) (MM-1), YY+2000, hh, mm, 0, (DayOfWeek) 1, (SaveLight) 0);
  RTC.setTime(startTime);
} 

// calls metaDateTime() and initialises the SD card, returns 1 on success, 0 on failure
int SDSetup() { // SD card setup function
  Serial.print("Initializing SD card...");
  SdFile::dateTimeCallback(metaDateTime); // calls the dateTime function to create SD card metadata
  // see if the card is present and can be initialized:
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  if (!SD.begin(SD_CS)) {
    Serial.println("SD card error");
    return 0;
  } else {
    Serial.println("card initialized."); // confirmation of SD card initialisation
    return 1;
  }
}

// initialises SEN55 over I2C, sets the temperature offset and starts measuring
void sensorSetup() {
  Wire.begin();
  sen5x.begin(Wire);

  // sensor error checking
  uint16_t error;
  char errorMessage[256];
  error = sen5x.deviceReset();
  if (error) {
    Serial.print("Error trying to execute deviceReset(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }

  // setting sensor temperature offset
  error = sen5x.setTemperatureOffsetSimple(tempOffset);
  if (error) {
    Serial.print("Error trying to execute setTemperatureOffsetSimple(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else {
    Serial.print("Temperature Offset set to ");
    Serial.print(tempOffset);
    Serial.println(" deg. Celsius");
  }

  // setting sensor VOC algorithm parameters
  error = sen5x.setVocAlgorithmTuningParameters(100, 6, 6, 15, 50, 200);
  if (error) {
    Serial.print("Error trying to execute setVocAlgorithmTuningParameters(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else {
    Serial.println("VOC Algoritm Parameters set to:");
    Serial.print("100, 6, 6, 15, 50, 200");
  }

  // Start Measurement
  error = sen5x.startMeasurement();
  if (error) {
    Serial.print("Error trying to execute startMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }
}

// adds the timestamp followed by the current air quality reading to the dataString and returns it
String createDataString(float &massConcentrationPm1p0, float &massConcentrationPm2p5,
                      float &massConcentrationPm4p0, float &massConcentrationPm10p0,
                      float &ambientHumidity, float &ambientTemperature, float &vocIndex,
                      float &noxIndex, uint16_t &rawVoc, uint16_t &rawNox, String &dateTime) {

  String dataString;
  // 13:00:00, 
  // adds current time to the dataString
  dataString += dateTime;
  dataString += ", ";

  // adding data to the dataString
  dataString += String(massConcentrationPm1p0);
  dataString += ", ";
  dataString += String(massConcentrationPm2p5);
  dataString += ", ";
  dataString += String(massConcentrationPm4p0);
  dataString += ", ";
  dataString += String(massConcentrationPm10p0);
  dataString += ", ";
  dataString += String(ambientHumidity);
  dataString += ", ";
  dataString += String(ambientTemperature);
  dataString += ", ";
  dataString += String(vocIndex);
  dataString += ", ";
  dataString += String(noxIndex);
  dataString += ", ";
  dataString += String(rawVoc);
  dataString += ", ";
  dataString += String(rawNox);


  return dataString;
}

// opens the data file on the SD card and writes the dataString to it
// throws an error if the file can't be opened
void writeToSD(String &dataString) {
  File dataFile = SD.open(fname, FILE_WRITE); // opens the SD card file ready to be written to

  if (dataFile) { // checks if the file is open and writes the data string to it
    dataFile.println(dataString);
    dataFile.close();
    Serial.println(dataString); // print to the serial port too:
  } else { // if it isn't open, shows an error
    Serial.print("Error opening ");
    Serial.println(fname);
  }
}

// updates the min/max values after a measurement is taken
void updateMinMax(float &massConcentrationPm1p0, float &massConcentrationPm2p5,
                  float &massConcentrationPm4p0, float &massConcentrationPm10p0,
                  float &ambientHumidity, float &ambientTemperature, float &vocIndex,
                  float &noxIndex, uint16_t &rawVoc, uint16_t &rawNox) {
  if (loopCount > 3) {
      if (massConcentrationPm1p0 < minPm1p0) {
        minPm1p0 = massConcentrationPm1p0;
      } else if (massConcentrationPm1p0 > maxPm1p0) {
        maxPm1p0 = massConcentrationPm1p0;
      }
      if (massConcentrationPm2p5 < minPm2p5) {
        minPm2p5 = massConcentrationPm2p5;
      } else if (massConcentrationPm2p5 > maxPm2p5) {
        maxPm2p5 = massConcentrationPm2p5;
      }
      if (massConcentrationPm4p0 < minPm4p0) {
        minPm4p0 = massConcentrationPm4p0;
      } else if (massConcentrationPm4p0 > maxPm4p0) {
        maxPm4p0 = massConcentrationPm4p0;
      }
      if (massConcentrationPm10p0 < minPm10p0) {
        minPm10p0 = massConcentrationPm10p0;
      } else if (massConcentrationPm10p0 > maxPm10p0) {
        maxPm10p0 = massConcentrationPm10p0;
      }
      if (ambientTemperature < minTemp) {
        minTemp = ambientTemperature;
      } else if (ambientTemperature > maxTemp) {
        maxTemp = ambientTemperature;
      }
      if (ambientHumidity < minHumidity) {
        minHumidity = ambientHumidity;
      } else if (ambientHumidity > maxHumidity) {
        maxHumidity = ambientHumidity;
      }
      if (vocIndex < minVoc) {
        minVoc = vocIndex;
      } else if (vocIndex > maxVoc) {
        maxVoc = vocIndex;
      }
      if (noxIndex < minNox) {
        minNox = noxIndex;
      } else if (noxIndex > maxNox) {
        maxNox = noxIndex;
      }
      if (rawVoc < minRawVoc) {
        minRawVoc = rawVoc;
      } else if (rawVoc > maxRawVoc) {
        maxRawVoc = rawVoc;
      }
      if (rawNox < minRawNox) {
        minRawNox = rawNox;
      } else if (rawNox > maxRawNox) {
        maxRawNox = rawNox;
      }

    } else {loopCount++;}
}

// takes readings from the SEN55 and calls other functions to update the min/max values,
// print them to the screen and write to the SD card
void doMeasurement() {

  uint16_t error;
  char errorMessage[256];

  uint16_t rawError;
  char rawErrorMessage[256];

  // Read Measurement
  float massConcentrationPm1p0;
  float massConcentrationPm2p5;
  float massConcentrationPm4p0;
  float massConcentrationPm10p0;
  float ambientHumidity;
  float ambientTemperature;
  float vocIndex;
  float noxIndex;

  int16_t rawHumidity;
  int16_t rawTemperature;
  uint16_t rawVoc;
  uint16_t rawNox;

  error = sen5x.readMeasuredValues(massConcentrationPm1p0, massConcentrationPm2p5,
                                  massConcentrationPm4p0, massConcentrationPm10p0,
                                  ambientHumidity, ambientTemperature, vocIndex, noxIndex);

  
  rawError = sen5x.readMeasuredRawValues(rawHumidity, rawTemperature, rawVoc, rawNox);

  updateMinMax(massConcentrationPm1p0, massConcentrationPm2p5, massConcentrationPm4p0,
              massConcentrationPm10p0, ambientHumidity, ambientTemperature, vocIndex, noxIndex,
              rawVoc, rawNox);

  #if DEBUG
    if (error) {
      Serial.print("Error trying to execute readMeasuredValues(): ");
      errorToString(error, errorMessage, 256);
      Serial.println(errorMessage);
    } else if (rawError) {
      Serial.print("Error trying to execute readMeasuredRawValues(): ");
      errorToString(rawError, rawErrorMessage, 256);
      Serial.println(rawErrorMessage);
    } else {
      Serial.print("Pm1p0:");     Serial.print(massConcentrationPm1p0);   Serial.print("\t");
      Serial.print("Pm2p5:");     Serial.print(massConcentrationPm2p5);   Serial.print("\t");
      Serial.print("Pm4p0:");     Serial.print(massConcentrationPm4p0);   Serial.print("\t");
      Serial.print("Pm10p0:");    Serial.print(massConcentrationPm10p0);  Serial.print("\t");
      Serial.print("RH:");        Serial.print(ambientHumidity);          Serial.print("\t");
      Serial.print("Temp:");      Serial.print(ambientTemperature);       Serial.print("\t");
      Serial.print("VOC Index:"); Serial.print(vocIndex);                 Serial.print("\t");
      Serial.print("NOx Index:"); Serial.print(noxIndex);                 Serial.print("\t");
      Serial.print("Raw VOC:");   Serial.print(rawVoc);                   Serial.print("\t");
      Serial.print("Raw NOx:");   Serial.println(rawNox);

      // keeps track of the max/min values after the third measurement
      if (loopCount > 3) {
        Serial.print(minPm1p0);     Serial.print(" / "); Serial.print(maxPm1p0);      Serial.print("                     ");
        Serial.print(minPm2p5);     Serial.print(" / "); Serial.print(maxPm2p5);      Serial.print("                    ");
        Serial.print(minPm4p0);     Serial.print(" / "); Serial.print(maxPm4p0);      Serial.print("                    ");
        Serial.print(minPm10p0);    Serial.print(" / "); Serial.print(maxPm10p0);     Serial.print("                    ");
        Serial.print(minHumidity);  Serial.print(" / "); Serial.print(maxHumidity);   Serial.print("           ");
        Serial.print(minTemp);      Serial.print(" / "); Serial.print(maxTemp);       Serial.print("                   ");
        Serial.print(minVoc);       Serial.print(" / "); Serial.print(maxVoc);        Serial.print("      ");
        Serial.print(minNox);       Serial.print(" / "); Serial.println(maxNox);      Serial.print("      ");
        Serial.print(minRawVoc);    Serial.print(" / "); Serial.print(maxRawVoc);     Serial.print("      ");
        Serial.print(minRawNox);    Serial.print(" / "); Serial.println(maxRawNox);

      }
    }
  #endif

  String dateTime = getDateTime();

  printMeasurementsToScreen(massConcentrationPm1p0, massConcentrationPm2p5,
                            massConcentrationPm4p0, massConcentrationPm10p0,
                            ambientHumidity, ambientTemperature, vocIndex,
                            noxIndex, dateTime);

  // SD card writing code
  sampleCount++;
  if (sampleCount >= samplesBeforeWrite && datalogging) {
    sampleCount = 0;
    String dataString = createDataString(massConcentrationPm1p0, massConcentrationPm2p5,
                                        massConcentrationPm4p0, massConcentrationPm10p0, 
                                        ambientHumidity, ambientTemperature, vocIndex,
                                        noxIndex, rawVoc, rawNox, dateTime);
    writeToSD(dataString);
  }
}

// prints the releveant measurments to the screen depending on the current mode
void printMeasurementsToScreen(float &massConcentrationPm1p0, float &massConcentrationPm2p5,
                              float &massConcentrationPm4p0, float &massConcentrationPm10p0,
                              float &ambientHumidity, float &ambientTemperature, float &vocIndex,
                              float &noxIndex, String &dateTime) {

  // clears any remaining digits from previous dates/times
  if (!mode) {
    screen.setCursor(273, 13);
    screen.print("   ");
  } else if (mode > 0) {
    screen.setCursor(273, 213);
    screen.print("   ");
  }

  switch (mode) { // prints the relevant data to the screen for each mode
    case 0:    
    // home screen data printing
    screen.setCursor(93, 13);
    screen.setTextSize(2);
    screen.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    screen.print(dateTime);
    screen.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
    
    // NOX summary
    screen.setCursor(57, 53); screen.print(" ");
    screen.setCursor(45, 53); screen.print(int(noxIndex));

    // VOC summary
    screen.setCursor(69, 93); screen.print(" ");
    screen.setCursor(45, 93); screen.print(int(vocIndex));

    // PM summary
    screen.setCursor(92, 133); screen.print("   ");
    screen.setCursor(45, 133); screen.print(massConcentrationPm10p0);
    screen.setCursor(130, 133); screen.print("ug/m^3");
    screen.setCursor(230, 133); screen.print("(Total)");
    
    // Humidity summary
    screen.setCursor(92, 173); screen.print("  ");
    screen.setCursor(45, 173); screen.print(ambientHumidity);
    screen.setCursor(130, 173); screen.print("%");

    // Temperature summary
    screen.setCursor(92, 213); screen.print("  ");
    screen.setCursor(45, 213); screen.print(ambientTemperature);
    screen.drawCircle(130, 215, 2, ILI9341_YELLOW);
    screen.setCursor(134, 213); screen.print("C");
    return;

    case 1: // temperature data printing
    screen.setCursor(93, 213);
    screen.setTextSize(2);
    screen.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    screen.print(dateTime);

    screen.setTextSize(2);
    screen.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);

    screen.setCursor(45, 53); screen.print("Current:");
    screen.setCursor(202, 53); screen.print(" ");
    screen.setCursor(155, 53); screen.print(ambientTemperature);
    screen.drawCircle(235, 55, 2, ILI9341_YELLOW);
    screen.setCursor(239, 53); screen.print("C");

    screen.setCursor(45, 113); screen.print("Min:");
    screen.setCursor(202, 113); screen.print(" ");
    screen.setCursor(155, 113); screen.print(minTemp);
    screen.drawCircle(235, 115, 2, ILI9341_YELLOW);
    screen.setCursor(239, 113); screen.print("C");

    screen.setCursor(45, 173); screen.print("Max:");
    screen.setCursor(202, 173); screen.print(" ");
    screen.setCursor(155, 173); screen.print(maxTemp);
    screen.drawCircle(235, 175, 2, ILI9341_YELLOW);
    screen.setCursor(239, 173); screen.print("C");
    return;

    case 2: // humidity data printing
    screen.setCursor(93, 213);
    screen.setTextSize(2);
    screen.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    screen.print(dateTime);

    screen.setTextSize(2);
    screen.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);

    screen.setCursor(45, 53); screen.print("Current:");
    screen.setCursor(202, 53); screen.print(" ");
    screen.setCursor(155, 53); screen.print(ambientHumidity);
    screen.setCursor(235, 53); screen.print("%");

    screen.setCursor(45, 113); screen.print("Min:");
    screen.setCursor(202, 113); screen.print(" ");
    screen.setCursor(155, 113); screen.print(minHumidity);
    screen.setCursor(235, 113); screen.print("%");

    screen.setCursor(45, 173); screen.print("Max:");
    screen.setCursor(202, 173); screen.print(" ");
    screen.setCursor(155, 173); screen.print(maxHumidity);
    screen.setCursor(235, 173); screen.print("%");
    return;

    case 3: // PM data printing
    screen.setCursor(93, 213);
    screen.setTextSize(2);
    screen.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    screen.print(dateTime);

    screen.setTextSize(2);
    screen.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);

    screen.setCursor(5, 40); screen.print("Size");
    screen.setCursor(5, 70); screen.print("1.0");
    screen.setCursor(5, 100); screen.print("2.5");
    screen.setCursor(5, 130); screen.print("4.0");
    screen.setCursor(5, 160); screen.print("10.0");

    screen.setCursor(61, 40); screen.print("Current");
    screen.setCursor(109, 70); screen.print("   ");
    screen.setCursor(61, 70); screen.print(massConcentrationPm1p0);
    screen.setCursor(109, 100); screen.print("   ");
    screen.setCursor(61, 100); screen.print(massConcentrationPm2p5);
    screen.setCursor(109, 130); screen.print("   ");
    screen.setCursor(61, 130); screen.print(massConcentrationPm4p0);
    screen.setCursor(109, 160); screen.print("   ");
    screen.setCursor(61, 160); screen.print(massConcentrationPm10p0);

    screen.setCursor(157, 40); screen.print("Min");
    screen.setCursor(197, 70); screen.print("   ");
    screen.setCursor(151, 70); screen.print(minPm1p0);
    screen.setCursor(197, 100); screen.print("   ");
    screen.setCursor(151, 100); screen.print(minPm2p5);
    screen.setCursor(197, 130); screen.print("   ");
    screen.setCursor(151, 130); screen.print(minPm4p0);
    screen.setCursor(197, 160); screen.print("   ");
    screen.setCursor(151, 160); screen.print(minPm10p0);

    screen.setCursor(250, 40); screen.print("Max");
    screen.setCursor(281, 70); screen.print("  ");
    screen.setCursor(235, 70); screen.print(maxPm1p0);
    screen.setCursor(281, 100); screen.print("  ");
    screen.setCursor(235, 100); screen.print(maxPm2p5);
    screen.setCursor(281, 130); screen.print("  ");
    screen.setCursor(235, 130); screen.print(maxPm4p0);
    screen.setCursor(281, 160); screen.print("  ");
    screen.setCursor(235, 160); screen.print(maxPm10p0);
    return;

    case 4: // VOC data printing
    screen.setCursor(93, 213);
    screen.setTextSize(2);
    screen.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    screen.print(dateTime);

    screen.setTextSize(2);
    screen.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);

    screen.setCursor(45, 53); screen.print("Current:");
    screen.setCursor(202, 53); screen.print(" ");
    screen.setCursor(155, 53); screen.print(vocIndex);

    screen.setCursor(45, 113); screen.print("Min:");
    screen.setCursor(202, 113); screen.print(" ");
    screen.setCursor(155, 113); screen.print(minVoc);

    screen.setCursor(45, 173); screen.print("Max:");
    screen.setCursor(202, 173); screen.print(" ");
    screen.setCursor(155, 173); screen.print(maxVoc);
    return;

    case 5: // NOX data printing
    screen.setCursor(93, 213);
    screen.setTextSize(2);
    screen.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    screen.print(dateTime);

    screen.setTextSize(2);
    screen.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);

    screen.setCursor(45, 53); screen.print("Current:");
    screen.setCursor(202, 53); screen.print(" ");
    screen.setCursor(155, 53); screen.print(noxIndex);

    screen.setCursor(45, 113); screen.print("Min:");
    screen.setCursor(1202, 113); screen.print(" ");
    screen.setCursor(155, 113); screen.print(minNox);

    screen.setCursor(45, 173); screen.print("Max:");
    screen.setCursor(202, 173); screen.print(" ");
    screen.setCursor(155, 173); screen.print(maxNox);
    return;

    case 6:
    screen.setCursor(93, 213);
    screen.setTextSize(2);
    screen.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    screen.print(dateTime);
    
    return;

    default:
    return;
  }
}

// draws the GUI for the data menu, including the buttons
void dataloggingMenu() {
  screen.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  screen.setTextSize(2);
  screen.setCursor(45, 80);
  screen.print("Recording");
  screen.setCursor(45, 140);
  screen.print("Reset min/max");
  drawButton(240, 129, "OK");
  if (datalogging) {
    drawButton(240, 69, "On");
  } else {
    drawButton(240, 69, "Off");
  }
}

// handles changing modes depending on keypresses and calls their drawing function
void menuPanel(int x) { // changes mode according to whats selected
  switch (x) {
    case 1 ... BOXSIZE:
    mode = 1;
    drawModeScreen(mode);
    return;

    case BOXSIZE+1 ... 2*BOXSIZE:
    mode = 2;
    drawModeScreen(mode);
    return;

    case 2*BOXSIZE+1 ... 3*BOXSIZE:
    mode = 3;
    drawModeScreen(mode);
    return;

    case 3*BOXSIZE+1 ... 4*BOXSIZE:
    mode = 4;
    drawModeScreen(mode);
    return;

    case 4*BOXSIZE+1 ... 5*BOXSIZE:
    mode = 5;
    drawModeScreen(mode);
    return;

    case 5*BOXSIZE+1 ... 6*BOXSIZE:
    mode = 6;
    drawModeScreen(mode);
    dataloggingMenu();
    return;

    default:
    return;
  }
}

// draws the home screen GUI, separate function due to different layout
void homeScreen() { // draws the home screen
  screen.fillScreen(ILI9341_BLACK);

  screen.setTextColor(ILI9341_WHITE);

  screen.setTextColor(ILI9341_BLACK);

  screen.setTextSize(1);

  screen.fillRect(0, 0, BOXSIZE, BOXSIZE, ILI9341_RED);

  screen.setCursor(9, 16);
  screen.print("Data");

  screen.fillRect(0, BOXSIZE, BOXSIZE, BOXSIZE, ILI9341_ORANGE);

  screen.setCursor(12, 56);
  screen.print("NOx");

  screen.fillRect(0, BOXSIZE*2, BOXSIZE, BOXSIZE, ILI9341_YELLOW);

  screen.setCursor(12, 96);
  screen.print("VOC");

  screen.fillRect(0, BOXSIZE*3, BOXSIZE, BOXSIZE, ILI9341_GREEN);

  screen.setCursor(15, 136);
  screen.print("PM");

  screen.fillRect(0, BOXSIZE*4, BOXSIZE, BOXSIZE, ILI9341_CYAN);

  screen.setCursor(15, 176);
  screen.print("RH");

  screen.fillRect(0, BOXSIZE*5, BOXSIZE, BOXSIZE, ILI9341_PURPLE);

  screen.setCursor(9, 216);
  screen.print("Temp");
}

// draws the generic parts of each mode screen; the title and back button
void drawModeScreen(int &mode) { // draws the default parts of the mode screens
  screen.fillScreen(ILI9341_BLACK);    // once to prevent flickering

  // draws back button
  screen.setTextColor(ILI9341_WHITE);
  screen.fillRect(0, BOXSIZE*5, BOXSIZE, BOXSIZE, ILI9341_NAVY);
  screen.setCursor(9, 216);
  screen.setTextSize(1);
  screen.print("Back");

  // draws the mode screen title

  String modeName;
  switch (mode) {
    case 1:
    screen.setTextColor(ILI9341_PURPLE);
    modeName = "Temperature";
    break;
    
    case 2:
    screen.setTextColor(ILI9341_CYAN);
    modeName = "Humidity";
    break;
    
    case 3:
    screen.setTextColor(ILI9341_GREEN);
    modeName = "Particulate Matter";
    break;

    case 4:
    screen.setTextColor(ILI9341_YELLOW);
    modeName = "VOC Index";
    break;

    case 5:
    screen.setTextColor(ILI9341_ORANGE);
    modeName = "NOx Index";
    break;

    case 6:
    screen.setTextColor(ILI9341_RED);
    modeName = "Datalogging";
    break;

    default:
    break;
  }

  screen.setTextSize(2);
  screen.setCursor(45, 10);
  screen.print(modeName);
}

// draws the boot screen showing the name and version
void drawBootScreen() { // draws the startup screen with version information
  screen.setRotation(3);
  screen.fillScreen(ILI9341_BLACK);

  screen.setTextSize(3);
  screen.setTextColor(ILI9341_WHITE);
  screen.setCursor(64, 70);
  screen.print("Air Quality");
  screen.setCursor(10, 100);
  screen.print("Measurement Probe");

  screen.setTextSize(2);
  screen.setCursor(110, 200);
  screen.print("ver "); screen.print(VERSION);
}

void setup() {
  Serial.begin(115200);
  delay(1000);     // pause until Serial port active

  RTC.begin();

  // SD Card setup
  int isSDSetup = SDSetup();

  // Touchscreen setup
  screen.begin();
  // draw the boot screen
  drawBootScreen();

  if (! capTouch.begin(40, &Wire)) {  // pass in 'sensitivity' coefficient and I2C bus
    Serial.println("Couldn't start FT6206 touchscreen controller");
    while (1) delay(10); // waits until the touchscreen is active
  }

  delay(2000);

  if (isSDSetup) {
    screen.fillScreen(ILI9341_BLACK);
    screen.setTextSize(3);
    screen.setCursor(60, 80);
    screen.print("SD Card OK");
    screen.setTextSize(2);
    screen.setCursor(60, 110);
    screen.print("Please Set Time");
    delay(1000);
  } else {
    screen.fillScreen(ILI9341_BLACK);
    screen.setTextSize(3);
    screen.setCursor(40, 80);
    screen.print("SD Card Error");
    screen.setTextSize(2);
    screen.setCursor(90, 110);
    screen.print("Check Card");
    while (1) {}
  }

  // SEN55 setup
  sensorSetup();

  //sen5x.getVocAlgorithmTuningParameters(int16_t &indexOffset, int16_t &learningTimeOffsetHours, int16_t &learningTimeGainHours, int16_t &gatingMaxDurationMinutes, int16_t &stdInitial, int16_t &gainFactor)

  setDateTime();

  RTCTime now;
  RTC.getTime(now);

  fname += String(now.getDayOfMonth());
  //fname += "-";
  fname += String(Month2int(now.getMonth()));
  //fname += "-";
  //fname += String(now.getYear()-2000);
  fname += "_";
  fname += String(now.getHour());
  //fname += "-";
  fname += String(now.getMinutes());
  fname += ".txt";

  File dataFile = SD.open(fname, FILE_WRITE);
  if (dataFile) {
    dataFile.println("date, pm1, pm25, pm4, pm10, rh, temp, VOC, NOx, raw_VOC, raw_NOx");
    Serial.println("date, pm1, pm25, pm4, pm10, rh, temp, VOC, NOx, raw_VOC, raw_NOx");
    dataFile.close();
  } else {Serial.println("error opening file");}
  
  homeScreen(); // draws the home screen gui
}

void loop() {
  if (capTouch.touched()) { // checks if the screen's been touched
    isTouched();
  }

  // sensor measurements
  currentMillis = millis();
  if (currentMillis - previousMillis >= sampleInterval) {
    previousMillis = currentMillis;
    doMeasurement();
  }
}