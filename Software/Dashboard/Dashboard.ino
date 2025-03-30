#include "src/libraries/BajaCAN.h"  // https://arduino.github.io/arduino-cli/0.35/sketch-specification/#src-subfolder

#define DEBUG true
#define DEBUG_SERIAL \
  if (DEBUG) Serial


// Including all libraries
#include "SPI.h"
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include <TJpg_Decoder.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <TLC591x.h>
#if defined(ENERGIA_ARCH_MSP432R)
#include <stdio.h>
#endif


TLC591x myLED(3, 32, 33, 21, 22);  //num_chips, SDI_pin, CLK_pin, LE_pin, OE_pin

#include "rpmGauge.h"

// Setup for left circular display
#define TFT_DC_L 16
#define TFT_CS_L 17
Adafruit_GC9A01A tftL(TFT_CS_L, TFT_DC_L);

// X and Y positions on data on left circular display
const int timeX = 33;
const int timeY = 60;
const int stopwatchX = 33;
const int stopwatchY = 120;
const int cvtRatioTextX = 100;
const int cvtRatioTextY = 180;
const int cvtRatioDataX = 80;
const int cvtRatioDataY = 220;

// Function for boot sequence Highlander Racing Image
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (y >= tft.height()) return 0;
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}

// Defintions for CVT data (CVT RPM Sensor -> Dashboard Displays)
float cvtMinRatio = 0.9;
float cvtMaxRatio = 3.9;
float cvtRatio = 0;
float lastCvtRatio = 0;
int lastRPM = 0;
const float gearboxRatio = 8.25;

// Pins for four status LEDs
const int cvtTempMax = 150;
const int cvtOffTemp = 140;

// Pins for four status LEDs (power status is tied to VCC)
const int cvtLed = 2;
const int lowBatteryLed = 15;
const int dasLed = 27;

// Brightness parameters for status LEDs
const int lowBatteryLedBrightness = 75;
const int dasLedBrightness = 120;
const int cvtLedBrightness = 75;

// Pins for three non-green LEDs (brightness was uneven with them on the LED driver :( )
const int redLed = 12;
const int yellowLed1 = 13;
const int yellowLed2 = 14;

// Pins for rear buttons
int rightButton = 19;  // Controls DAQ logging
int leftButton = 34;   // Controls Stopwatch

// Flags to check if buttons were released (to prevent issues with holding buttons)
bool rightButtonWasReleased = true;
bool leftButtonWasReleased = true;

// Variable for battery level, which represents number of LEDs illuminated based on battery percentage
int batteryLevel;

// Allows clock to only update when a second changes
int lastUpdatedSecond = 0;

// For LED Brightness (seven segments AND led bar or just one?)
int ledBrightness = 0;  // 0 is max, 255 is min

// Definitions for stopwatch
bool stopwatchActive = false;
unsigned long stopwatchStartTime = 0;
unsigned long stopwatchElapsedTime = 0;  // milliseconds
int stopwatchHour = 0;
int stopwatchMinute = 0;
int stopwatchSecond = 0;
int lastStopwatchSecond = 0;  // only update the stopwatch when the value of the second changes (to prevent display flicker)


int screenSelect = 0;  //done by modulus. 0 is brightness, 1 is error code, 2 is mark
bool screenSelectStop = false;
int brightnessUpDown = 0;
int ABSTractionLightOscillator = 0;
int Osc = 6;

void setup() {

  Serial.begin(460800);
  setupCAN(DASHBOARD);

  pinMode(cvtLed, OUTPUT);
  pinMode(dasLed, OUTPUT);
  pinMode(lowBatteryLed, OUTPUT);
  pinMode(redLed, OUTPUT);
  pinMode(yellowLed1, OUTPUT);
  pinMode(yellowLed2, OUTPUT);
  pinMode(rightButton, INPUT_PULLUP);
  pinMode(leftButton, INPUT_PULLUP);


  myLED.displayEnable();  // This command has no effect if you aren't using OE pin
  myLED.displayBrightness(ledBrightness);

  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tft_output);
  rpmGaugeSetup();

  //Left display configurations
  tftL.begin();
  tftL.setFont(&FreeMonoBold24pt7b);
  tftL.fillScreen(GC9A01A_WHITE);

  //This should be a few seconds of splash screen displaying Highlander Racing logos/stuff on boot
  bootScreen();

  updateTime();
  updateCvtRatio();
}



void loop() {


  ABSTractionLightOscillator++;

  checkButtons();

  // This takes the GPS speed (mph) and displays it on the seven segments
  // It also updates the battery level LED Bar
  updateLedDisplays();

  // Update the gauge based on the current primaryRPM
  updateRPMGauge(primaryRPM, Osc, ABSTractionLightOscillator);

  // Update the left display with the most recent time
  updateTime();

  // Update the digital CVT ratio on left display
  updateCvtRatio();

  updateStatusLEDs();

  checkWheelSlipSkid();
}



void checkButtons() {

  // Check left button

  if (!digitalRead(leftButton) && leftButtonWasReleased) {  // if left button is pressed
    sdLoggingActive = !sdLoggingActive;                     // flip logging active state
    delay(10);                                              // delay for debouncing
    leftButtonWasReleased = false;                          // to prevent continuous toggling while held down
  }
  if (digitalRead(leftButton)) {
    leftButtonWasReleased = true;  // if the button was released we can reset this flag
  }

  // Check right button

  if (!digitalRead(rightButton) && rightButtonWasReleased) {  // if left button is pressed

    stopwatchActive = !stopwatchActive;

    if (stopwatchActive) {
      stopwatchStartTime = millis();  // Set the start time to the current millis counter
    } else {
      tftL.fillRect(0, stopwatchY, 240, -(cvtRatioTextY - stopwatchY), TFT_WHITE);  // Clear the stopwatch
    }

    delay(10);                       // delay for debouncing
    rightButtonWasReleased = false;  // to prevent continuous toggling while held down
  }

  if (digitalRead(rightButton)) {
    rightButtonWasReleased = true;  // if the button was released we can reset this flag
  }

  if (stopwatchActive) updateStopwatch();  // if the stopwatch is active, continue to update the counter and display
}

void checkWheelSlipSkid() {
  //checks for wheel slippage or locking
  if (frontLeftWheelRPM == 0 && gpsVelocity > 0 && ABSTractionLightOscillator % Osc == 0) {
    tftL.fillRect(0, 0, 300, 125, TFT_RED);
  }
  if (frontLeftWheelRPM > 0 && gpsVelocity == 0 && ABSTractionLightOscillator % Osc == 0) {
    tftL.fillRect(0, 0, 300, 125, TFT_ORANGE);
  }
  if (rearLeftWheelRPM == 0 && gpsVelocity > 0 && ABSTractionLightOscillator % Osc == 0) {
    tftL.fillRect(0, 125, 300, 125, TFT_RED);
  }
  if (rearLeftWheelRPM > 0 && gpsVelocity == 0 && ABSTractionLightOscillator % Osc == 0) {
    tftL.fillRect(0, 125, 300, 125, TFT_ORANGE);
  }
}

void updateStatusLEDs() {
  Serial.print("batteryPercentage: ");
  Serial.println(batteryPercentage);

  // update BAT LED
  if (batteryPercentage < 20) analogWrite(lowBatteryLed, lowBatteryLedBrightness);
  else digitalWrite(lowBatteryLed, LOW);


  // update DAQ LED
  if (sdLoggingActive) {
    analogWrite(dasLed, dasLedBrightness);
  } else {
    digitalWrite(dasLed, LOW);
  }

  // update CVT LED
  if (primaryTemperature > cvtTempMax || secondaryTemperature > cvtTempMax) analogWrite(cvtLed, cvtLedBrightness);
  if (primaryTemperature < cvtOffTemp && secondaryTemperature < cvtOffTemp) digitalWrite(cvtLed, LOW);
}

void updateStopwatch() {


  stopwatchHour = (millis() - stopwatchStartTime) / 3600000;
  stopwatchMinute = (millis() - stopwatchStartTime) / 60000 % 60;
  stopwatchSecond = (millis() - stopwatchStartTime) / 1000 % 60;


  if (stopwatchSecond != lastStopwatchSecond) {

    lastStopwatchSecond = stopwatchSecond;

    tftL.setFont(&FreeMonoBold18pt7b);
    tftL.setTextColor(TFT_BLACK);
    tftL.fillRect(0, stopwatchY, 240, -(cvtRatioTextY - stopwatchY), TFT_WHITE);
    tftL.setCursor(stopwatchX, stopwatchY);  // Adjust coordinates as needed


    String stopwatchString = "";

    if (stopwatchHour < 10) {
      stopwatchString += 0;  // add a leading 0 for formatting purposes
    }

    stopwatchString += stopwatchHour;  // append the hours

    stopwatchString += ":";  // append a colon

    if (stopwatchMinute < 10) {
      stopwatchString += 0;  // add a leading 0 for formatting purposes
    }
    stopwatchString += stopwatchMinute;  // append the minutes

    stopwatchString += ":";  // append a colon

    if (stopwatchSecond < 10) {
      stopwatchString += 0;  // add a leading 0 for formatting purposes
    }
    stopwatchString += stopwatchSecond;  // append the seconds

    Serial.print("Current Time String is: ");
    Serial.println(stopwatchString);

    tftL.println(stopwatchString);  // push to the display
  }
}

void updateTime() {

  // If we discover that the time has changed since the last time updating the display, then push the latest time

  Serial.print("GPS HH:MM:SS   ");
  Serial.print(gpsTimeHour);
  Serial.print(":");
  Serial.print(gpsTimeMinute);
  Serial.print(":");
  Serial.println(gpsTimeSecond);

  Serial.print("lastUpdatedSecond: ");
  Serial.println(lastUpdatedSecond);

  if (gpsTimeSecond != lastUpdatedSecond) {

    lastUpdatedSecond = gpsTimeSecond;

    tftL.setFont(&FreeMonoBold18pt7b);
    tftL.setTextColor(TFT_BLACK);
    tftL.fillRect(0, timeY, 240, -(cvtRatioTextY - timeY - 95), TFT_WHITE);
    tftL.setCursor(timeX, timeY);  // Adjust coordinates as needed


    String timeString = "";

    if (gpsTimeHour < 10) {
      timeString += 0;  // add a leading 0 for formatting purposes
    }

    timeString += gpsTimeHour;  // append the hours

    timeString += ":";  // append a colon

    if (gpsTimeMinute < 10) {
      timeString += 0;  // add a leading 0 for formatting purposes
    }
    timeString += gpsTimeMinute;  // append the minutes

    timeString += ":";  // append a colon

    if (gpsTimeSecond < 10) {
      timeString += 0;  // add a leading 0 for formatting purposes
    }
    timeString += gpsTimeSecond;  // append the seconds

    Serial.print("Current Time String is: ");
    Serial.println(timeString);

    tftL.println(timeString);  // push to the display
  }
}

void updateCvtRatio() {

  cvtRatio = (secondaryRPM / (primaryRPM + 1.0));

  if (abs(cvtRatio - lastCvtRatio) > 0.1) {
    lastCvtRatio = cvtRatio;

    // Clear old ratio value
    tftL.fillRect(0, cvtRatioTextY, 240, 50, TFT_WHITE);

    // Display "CVT Text"
    tftL.setTextColor(TFT_BLACK);
    tftL.setCursor(cvtRatioTextX, cvtRatioTextY);  // Adjust coordinates as needed
    tftL.setFont(&FreeMono12pt7b);
    tftL.println("CVT");

    // Display Ratio Number
    tftL.setFont(&FreeMonoBold24pt7b);
    tftL.setCursor(cvtRatioDataX, cvtRatioDataY);  //cvtRatioTextX, cvtRatioTextY
    tftL.println(cvtRatio, 1);
  }
}

//This is necessary to get a decimal number for the CVT ratio (standard MAP function will only return an int)
float mapfloat(long x, long in_min, long in_max, long out_min, long out_max) {
  return (float)(x - in_min) * (out_max - out_min) / (float)(in_max - in_min) + out_min;
}
