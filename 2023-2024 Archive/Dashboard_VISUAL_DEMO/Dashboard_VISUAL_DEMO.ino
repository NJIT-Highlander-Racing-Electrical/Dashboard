/*
*
*  -----------------PROGRAM NOTE----------------
*
*  This program is solely for visual purposes.
*  It illuminates most of the displays and LEDs to make it look pretty.
*  There is absolutely no function while using this program.
*  The code is messy.
*  
*  This program was developed to use the 2023-2024 dashboard as a display 
*  item at the 2024 "Reverse Career Fair"
*
*
*/

#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_GC9A01A.h"
#include <TJpg_Decoder.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <TLC591x.h>
#if defined(ENERGIA_ARCH_MSP432R)
#include <stdio.h>
#endif
TLC591x myLED(3, 32, 33, 25, 26);  // Uncomment if using OE pin

#include "rpmGauge.h"

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
int primaryRPM = 0;
float cvtRatio = 1.4;

//Number of fuel level LEDs to display (0-9)
int fuelLevel = 9;

//Pins for four status LEDs (power status is tied to VCC)
const int cvtLed = 2;
const int daqLed = 27;
const int batteryLed = 15;

//Pins for three non-green fuel LEDs (brightness was uneven with them on the LED driver :( )
const int redFuelLED = 12;
const int yellowFuelLED1 = 13;
const int yellowFuelLED2 = 14;

//Definitions for 2WD/4WD state
bool fourWheelsState = false;
bool fourWheelsUnknown = true;

unsigned long lastClockFlashMillis = 0;
bool colonsOn = false;

unsigned long lastRpmDialUpdate = 0;
int rpmDialDelayMS = 20;
int rpmDialStepVal = 50;
unsigned long endpointPauseStartTime = 0;
int endpointDelay = 500;
bool endpointPaused = false;
bool stepUp = 1;  // 1 = step up direction, 0 = step down direction

void setup() {

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
  updateCvtRatio();

  fourWheelsState = true;
  fourWheelsUnknown = false;
  update2wd4wdState();

  digitalWrite(daqLed, HIGH);
  digitalWrite(cvtLed, LOW);
  digitalWrite(batteryLed, HIGH);


  digitalWrite(redFuelLED, LOW);
  digitalWrite(yellowFuelLED1, LOW);
  digitalWrite(yellowFuelLED2, LOW);

  updateSevenSegments(88);
}



void loop() {


 digitalWrite(redFuelLED, LOW);
  digitalWrite(yellowFuelLED1, LOW);
  digitalWrite(yellowFuelLED2, LOW);


  if ((millis() - lastRpmDialUpdate) > rpmDialDelayMS) {

    // Handle endpoint pause logic
    if (endpointPaused) {
      // Check if endpoint delay has passed
      if ((millis() - endpointPauseStartTime) > endpointDelay) {
        endpointPaused = false;  // End the pause
      }
    } else {
      // Adjust RPM values only when not paused
      if (stepUp) {
        if (primaryRPM > cvtMaxRPM) {
          stepUp = false;
          endpointPauseStartTime = millis();  // Start pause at max RPM
          endpointPaused = true;              // Enter paused state
        } else {
          primaryRPM += rpmDialStepVal;  // Increase RPM
        }
      } else {
        if (primaryRPM < cvtMinRPM) {
          stepUp = true;
          endpointPauseStartTime = millis();  // Start pause at min RPM
          endpointPaused = true;              // Enter paused state
        } else {
          primaryRPM -= rpmDialStepVal;  // Decrease RPM
        }
      }

      // Update RPM gauge when not paused
      if (!endpointPaused) {
        mappedRPMAngle = map(primaryRPM, cvtMinRPM, cvtMaxRPM, angleMin, angleMax);
        updateRPMGauge(mappedRPMAngle, primaryRPM);
        lastRpmDialUpdate = millis();  // Reset timer for next update
      }
    }
  }



  if ((millis() - lastClockFlashMillis) > 1000) {
    if (colonsOn) {
      colonsOn = false;
    } else {
      colonsOn = true;
    }
    updateTime();
    lastClockFlashMillis = millis();
  }
}


void updateTime() {

  tftL.setFont(&FreeMonoBold18pt7b);
  tftL.setTextColor(TFT_WHITE);

  if (colonsOn) {
    tftL.fillRect(timeHourX, timeY, 200, -(cvtRatioTextY - timeY), TFT_BLACK);
    tftL.setCursor(timeHourX, timeY);  // Adjust coordinates as needed
    tftL.print(12);
    tftL.print(":");
    tftL.setCursor(timeMinuteX, timeY);  // Adjust coordinates as needed
    tftL.print(34);
    tftL.print(":");
    tftL.setCursor(timeSecondX, timeY);  // Adjust coordinates as needed
    tftL.println(56);
  }

  else {
    tftL.fillRect(timeHourX, timeY, 200, -(cvtRatioTextY - timeY), TFT_BLACK);
    tftL.setCursor(timeHourX, timeY);  // Adjust coordinates as needed
    tftL.print(12);
    tftL.print(" ");
    tftL.setCursor(timeMinuteX, timeY);  // Adjust coordinates as needed
    tftL.print(34);
    tftL.print(" ");
    tftL.setCursor(timeSecondX, timeY);  // Adjust coordinates as needed
    tftL.println(56);
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


void update2wd4wdState() {

  if (fourWheelsState && !fourWheelsUnknown) {

    tftL.fillRect(twoWheelX, twoWheelY, 240, 35, TFT_BLACK);
    tftL.setFont(&FreeMonoBold24pt7b);
    tftL.setTextColor(0x8410);
    tftL.setCursor(twoWheelX, twoWheelY);  // Adjust coordinates as needed
    tftL.println("2WD");
    tftL.setTextColor(TFT_RED);
    tftL.setCursor(fourWheelX, fourWheelY);  // Adjust coordinates as needed
    tftL.println("4WD");

  }

  else if (!fourWheelsState && !fourWheelsUnknown) {

    tftL.fillRect(twoWheelX, twoWheelY, 240, 35, TFT_BLACK);
    tftL.setFont(&FreeMonoBold24pt7b);
    tftL.setTextColor(TFT_RED);
    tftL.setCursor(twoWheelX, twoWheelY);  // Adjust coordinates as needed
    tftL.println("2WD");
    tftL.setTextColor(0x8410);
    tftL.setCursor(fourWheelX, fourWheelY);  // Adjust coordinates as needed
    tftL.println("4WD");
  }

  else {

    tftL.fillRect(twoWheelX, twoWheelY, 240, 35, TFT_BLACK);
    tftL.setFont(&FreeMonoBold24pt7b);
    tftL.setTextColor(0x8410);
    tftL.setCursor(twoWheelX, twoWheelY);  // Adjust coordinates as needed
    tftL.println("2WD");
    tftL.setTextColor(0x8410);
    tftL.setCursor(fourWheelX, fourWheelY);  // Adjust coordinates as needed
    tftL.println("4WD");
  }
}
