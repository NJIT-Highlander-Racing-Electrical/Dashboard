#include "src/libraries/BajaCAN.h" // https://arduino.github.io/arduino-cli/0.35/sketch-specification/#src-subfolder

#define DEBUG true
#define DEBUG_SERIAL \
  if (DEBUG) Serial

#include "SPI.h"
#include "src/libraries/Adafruit_BusIO/Adafruit_I2CDevice.h"
#include "src/libraries/Adafruit_GFX_Library/Adafruit_GFX.h"
#include "src/libraries/Adafruit_GC9A01A/Adafruit_GC9A01A.h"
#include "src/libraries/TJpg_Decoder/src/TJpg_Decoder.h"
#include "src/libraries/Adafruit_GFX_Library/Fonts/FreeMono12pt7b.h"
#include "src/libraries/Adafruit_GFX_Library/Fonts/FreeMonoBold18pt7b.h"
#include "src/libraries/Adafruit_GFX_Library/Fonts/FreeMonoBold24pt7b.h"
#include "src/libraries/TLC591x/src/TLC591x.h"
#include "src/NotoSansBold36.h"
#include "src/NotoSansBold15.h"

#include "src/libraries/TFT_eSPI/TFT_eSPI.h"

#if defined(ENERGIA_ARCH_MSP432R)
#include <stdio.h>
#endif

// Constructor parameters: # of chips, SDI in, CLK pin, LE pin, OE pin (optional parameter)
TLC591x myLED(3, 32, 33, 21, 22);  // Uncomment if using OE pin


// Setup for right circular display
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);

// Setup for left circular display
#define TFT_DC_L 16
#define TFT_CS_L 17
Adafruit_GC9A01A tftL(TFT_CS_L, TFT_DC_L);

//X and Y positions on data on left circular display
const int timeHourX = 33;
const int timeMinuteX = 90;
const int timeSecondX = 150;
const int timeY = 60;
const int cvtRatioTextX = 100;
const int cvtRatioTextY = 180;
const int cvtRatioDataX = 80;
const int cvtRatioDataY = 220;
const int fourWheelX = 23;
const int fourWheelY = 130;
const int twoWheelX = 133;
const int twoWheelY = 130;

//Parameters to control the RPM dial on right circular display
const int cvtMinRPM = 0;
const int cvtMaxRPM = 4000;
const int angleMin = 0;
const int angleMax = 240;
int mappedRPMAngle = 0;


// Function for boot sequence Highlander Racing Image
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (y >= tft.height()) return 0;
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}

//Defintions for CVT data (CVT RPM Sensor -> Dashboard Displays)
float cvtMinRatio = 0.9;
float cvtMaxRatio = 3.9;
float cvtRatio = 0;
float lastCvtRatio = 0;
int lastRPM = 0;
const float gearboxRatio = 8.25;
const float wheelCircumference = 23.0;  // Wheel circumference in inches; distance traveled per revolution; should be 23 inches
float totalMultiplier = 0;
uint8_t wheelSpeed = 0;

//Number of fuel level LEDs to display (0-9)
int fuelLevel = 0;

//Definitions for four status LEDs
const int cvtTempMax = 150;
const int cvtOffTemp = 140;
bool daqOn = false;
bool daqError = false;
bool batteryLow = false;

//Pins for four status LEDs (power status is tied to VCC)
const int cvtLed = 2;
const int daqLed = 27;
const int batteryLed = 15;

//Pins for three non-green fuel LEDs (brightness was uneven with them on the LED driver :( )
const int redFuelLED = 12;
const int yellowFuelLED1 = 13;
const int yellowFuelLED2 = 14;

//Definitions for GPS time
int lastTimeH = 0;
int lastTimeM = 0;
int lastTimeS = 0;

unsigned long lastTimePingM = 0;
unsigned long lastTimePingS = 0;
bool hasBeenPinged = false;

void setup() {

  DEBUG_SERIAL.begin(460800);

  setupCAN(DASHBOARD, 100, 22, 21);

  pinMode(cvtLed, OUTPUT);
  pinMode(daqLed, OUTPUT);
  pinMode(batteryLed, OUTPUT);
  pinMode(redFuelLED, OUTPUT);
  pinMode(yellowFuelLED1, OUTPUT);
  pinMode(yellowFuelLED2, OUTPUT);

  myLED.displayEnable();       // This command has no effect if you aren't using OE pin
  myLED.displayBrightness(0);  // 0 is max, 255 is min
  updateSevenSegments(95);     // Part of boot screen sequence

  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tft_output);
  rpmGaugeSetup();

  //Left display configurations
  tftL.begin();
  tftL.setFont(&FreeMonoBold24pt7b);
  tftL.fillScreen(GC9A01A_BLACK);

  //This should be a few seconds of splash screen displaying Highlander Racing logos/stuff on boot
  bootScreen();

  updateTime();
  totalMultiplier = (1.0 / gearboxRatio) * wheelCircumference * 3.1415926 * 0.00001578282828 * 60.0;  // 00001578282828 is miles in an inch
  updateCvtRatio();
}



void loop() {

  cvtRatio = (secondaryRPM / (primaryRPM + 1.0));

  // 0.000015782828283 inches in a mile and 60 minutes in one hour
  wheelSpeed = (secondaryRPM * totalMultiplier);

  // This takes the calculated speed and displays it on the seven segments
  DEBUG_SERIAL.print("WHEEL SPEED: ");
  DEBUG_SERIAL.println(wheelSpeed);
  updateSevenSegments(wheelSpeed);

  //This takes the CVT rpm and plots the dial accordingly
  mappedRPMAngle = map(primaryRPM, cvtMinRPM, cvtMaxRPM, angleMin, angleMax);
  updateRPMGauge(mappedRPMAngle, primaryRPM);

  updateTime();

  //This updates the digital CVT ratio on left display
  if (abs(cvtRatio - lastCvtRatio) > 0.1) {
    lastCvtRatio = cvtRatio;
    updateCvtRatio();
  }


  // These lines toggle the status LEDs
  if (batteryLow) digitalWrite(batteryLed, HIGH);
  else digitalWrite(batteryLed, LOW);

  if (daqOn && !daqError) {
    digitalWrite(daqLed, HIGH);
  } else if (daqError) {
    analogWrite(daqLed, 25);
  } else digitalWrite(daqLed, LOW);

  if (primaryTemperature > cvtTempMax || secondaryTemperature > cvtTempMax) digitalWrite(cvtLed, HIGH);
  if (primaryTemperature < cvtTempMax && secondaryTemperature < cvtTempMax) digitalWrite(cvtLed, LOW);
}


void updateTime() {


  tftL.setFont(&FreeMonoBold18pt7b);
  tftL.setTextColor(TFT_WHITE);

  if (gpsTimeHour != lastTimeH) {
    hasBeenPinged = true;
    lastTimeH = gpsTimeHour;
    tftL.fillRect(timeHourX, timeY, (timeMinuteX - timeHourX), -(cvtRatioTextY - timeY), TFT_BLACK);
    tftL.setCursor(timeHourX, timeY);  // Adjust coordinates as needed
    if (gpsTimeHour < 10) tftL.print("0");
    tftL.print(gpsTimeHour);
    tftL.print(":");
  }

  if (gpsTimeMinute != lastTimeM) {
    lastTimePingM = millis();
    lastTimeM = gpsTimeMinute;
    tftL.fillRect(timeMinuteX, timeY, (timeSecondX - timeMinuteX), -(cvtRatioTextY - timeY), TFT_BLACK);
    tftL.setCursor(timeMinuteX, timeY);  // Adjust coordinates as needed
    if (gpsTimeMinute < 10) tftL.print("0");
    tftL.print(gpsTimeMinute);
    tftL.print(":");
  }
  if (((millis() - lastTimePingM) > 60000) && hasBeenPinged) {
    lastTimePingM = millis();
    gpsTimeMinute = gpsTimeMinute + 1;
    lastTimeM = gpsTimeMinute;
    tftL.fillRect(timeMinuteX, timeY, (240 - timeMinuteX), -(cvtRatioTextY - timeY), TFT_BLACK);
    tftL.setCursor(timeMinuteX, timeY);  // Adjust coordinates as needed
    if (gpsTimeMinute < 10) tftL.print("0");
    tftL.println(gpsTimeMinute);
  }

  if (gpsTimeSecond != lastTimeS) {
    lastTimePingS = millis();
    lastTimeS = gpsTimeSecond;
    tftL.fillRect(timeSecondX, timeY, (240 - timeSecondX), -(cvtRatioTextY - timeY), TFT_BLACK);
    tftL.setCursor(timeSecondX, timeY);  // Adjust coordinates as needed
    if (gpsTimeSecond < 10) tftL.print("0");
    tftL.println(gpsTimeSecond);
  }
  if (((millis() - lastTimePingS) > 1000) && hasBeenPinged) {
    lastTimePingS = millis();
    if (gpsTimeSecond > 59) {
      gpsTimeMinute++;
      gpsTimeSecond = 0;
    } else gpsTimeSecond++;
    lastTimeS = gpsTimeSecond;
    tftL.fillRect(timeSecondX, timeY, (240 - timeSecondX), -(cvtRatioTextY - timeY), TFT_BLACK);
    tftL.setCursor(timeSecondX, timeY);  // Adjust coordinates as needed
    if (gpsTimeSecond < 10) tftL.print("0");
    tftL.println(gpsTimeSecond);
  }
}

void updateCvtRatio() {

  tftL.fillRect(0, cvtRatioTextY, 240, 50, TFT_BLACK);
  tftL.setTextColor(TFT_WHITE);
  tftL.setCursor(cvtRatioTextX, cvtRatioTextY);  // Adjust coordinates as needed
  tftL.setFont(&FreeMono12pt7b);
  tftL.println("CVT");
  tftL.setFont(&FreeMonoBold24pt7b);
  tftL.setCursor(cvtRatioDataX, cvtRatioDataY);
  tftL.println(cvtRatio, 1);
}

//This is necessary to get a decimal number for the CVT ratio (standard MAP function will only return an int)
float mapfloat(long x, long in_min, long in_max, long out_min, long out_max) {
  return (float)(x - in_min) * (out_max - out_min) / (float)(in_max - in_min) + out_min;
}
