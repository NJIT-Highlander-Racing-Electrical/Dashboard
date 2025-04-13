#include "src/libraries/BajaCAN.h"  // https://arduino.github.io/arduino-cli/0.35/sketch-specification/#src-subfolder

// Set to TRUE for debugging, set to FALSE for final release
// Disabling Serial on final release allows for better performance
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
#include "rpmGauge.h"
#if defined(ENERGIA_ARCH_MSP432R)
#include <stdio.h>
#endif

// Configuration for TLC5917 LED drivers
TLC591x myLED(3, 32, 33, 21, 22);  //num_chips, SDI_pin, CLK_pin, LE_pin, OE_pin

// Setup for left circular display
#define TFT_DC_L 16
#define TFT_CS_L 17
Adafruit_GC9A01A tftL(TFT_CS_L, TFT_DC_L);

// Function for boot sequence Highlander Racing Image
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (y >= tft.height()) return 0;
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}



//
// LEFT DISPLAY CURSOR POSITIONS
//

const int timeX = 33;
const int timeY = 60;
const int stopwatchX = 33;
const int stopwatchY = 120;
const int cvtRatioTextX = 100;
const int cvtRatioTextY = 180;
const int cvtRatioDataX = 80;
const int cvtRatioDataY = 220;



//
// PIN DEFINITIONS
//

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
const int rightButton = 19;  // Controls DAQ logging
const int leftButton = 34;   // Controls Stopwatch



//
// CONFIGURABLE VARIABLES/SETTINGS
//

const int dataScreenshotFlagDuration = 1000;        // Time for screenshot flag to be active. This way we are sure it gets saved in the DAS logs for several entries
const int spinSkidLightOscillationFrequency = 250;  // milliseconds that the spin/skid light is on or off
const int ledBrightness = 0;                        // Seven segments brightness (since LED bar OE pin is non-functional); 0 is max, 255 is min
const int cvtTempMax = 150;                         // Threshold which is considered too hot for CVT (F)
const int engineMinRPM = 0;                         // Minimum RPM reading we should see on primary of CVT (engine RPM)
const int engineMaxRPM = 4000;                      // Minimum RPM reading we should see on primary of CVT (engine RPM)


//
// BUTTON VARIABLES
//

// Flags to check if buttons were released (to prevent issues with holding buttons)
bool rightButtonWasReleased = true;
bool leftButtonWasReleased = true;
// Flag to check if both buttons were pressed (for "screenshot")
bool bothButtonsPressed = false;
// Variables to track the screenshot bit duration
unsigned long dataScreenshotStart = 0;  // Time at which the screenshot flag was set



//
// STOPWATCH VARIABLES
//

bool stopwatchActive = false;            // Whether or not stopwatch is actively counting up
unsigned long stopwatchStartTime = 0;    // start time (from CPU boot) of stopwatch
unsigned long stopwatchElapsedTime = 0;  // milliseconds elapsed since start
int stopwatchHour = 0;
int stopwatchMinute = 0;
int stopwatchSecond = 0;
int lastStopwatchSecond = 0;  // only update the stopwatch when the value of the second changes (to prevent display flicker)



//
// WHEEL SPIN/SKID VARIABLES
//

bool spinSkidLightOn = false;           // State of light (on/off) for blinking
unsigned long lastLightSwitchTime = 0;  // Last millis() reading when spin/skid light was turned on/off
bool leftLcdSpinSkidActive = false;     // Prevents anything besides spin/skid code from updating displays
bool rightLcdSpinSkidActive = false;    // Prevents anyhting besides spin/skid code from updating displays

enum WheelState {  // CAN sends 0, 1, 2 for wheel state; enum is easier to understand
  GOOD,
  SPIN,
  SKID
};



//
// SCREEN SELECT VARIABLES
//

int screenSelect = 0;  //done by modulus. 0 is brightness, 1 is error code, 2 is mark
bool screenSelectStop = false;



//
// MISCELLANEOUS VARIABLES
//

int batteryLevel;                        // battery % is sent over CAN, but we want to calculate level 0-9 for LED Bar
int lastUpdatedSecond = 0;               // Allows GPS clock to only update when a second changes
float cvtRatio = 0;                      // Current CVT Ratio
float lastCvtRatio = 0;                  // Last ratio that we displayed on screen (to prevent flickering)
const int cvtOffTemp = cvtTempMax - 10;  // Prevents flickering when temp is right around cvtTempMax



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


  myLED.displayEnable();                   // This command has no effect if you aren't using OE pin
  myLED.displayBrightness(ledBrightness);  // Set brightness for all TLC5917 modules

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

  checkButtons();  // Check whether the left, right, or both rear buttons are pressed

  updateLedDisplays();  // Displays GPS sped (mph) on seven segments and updates battery level LED Bar

  updateRPMGauge();  // Update the gauge based on the current primaryRPM

  updateTime();  // Update the left display with the most recent time

  updateCvtRatio();  // Update the digital CVT ratio on left display

  updateStatusLEDs();  // Turns on/off the three configurable status LEDs

  checkWheelSpinSkid();  // Sets displays to certain colors if wheel spin/skid is active

  /*
  Note: Make sure after wheel spin/skid stuff is displayed, it clears and everything else resumes as normal
  */

  checkStatus();  // Checks health of system for Base Station's status requests
}



void checkButtons() {

  // Update left button state
  if (!digitalRead(leftButton) && leftButtonWasReleased) {  // if left button is pressed
    leftButtonWasReleased = false;                          // to prevent continuous toggling while held down
    delay(10);                                              // delay for debouncing
    sdLoggingActive = !sdLoggingActive;                     // Toggle logging active state
    Serial.print("sdLoggingActive: ");
Serial.println(sdLoggingActive);

  }
  if (digitalRead(leftButton)) {
    leftButtonWasReleased = true;  // if the button was released we can set this flag
    delay(10);                     // delay for debouncing
  }

  // Update right button state
  if (!digitalRead(rightButton) && rightButtonWasReleased) {  // if right button is pressed
    rightButtonWasReleased = false;                           // to prevent continuous toggling while held down
    delay(10);                                                // delay for debouncing
    stopwatchActive = !stopwatchActive;
    if (stopwatchActive) {
      stopwatchStartTime = millis();  // Set the start time to the current millis counter
    } else {
      tftL.fillRect(0, stopwatchY, 240, -(cvtRatioTextY - stopwatchY), TFT_WHITE);  // Clear the stopwatch
    }
  }
  if (digitalRead(rightButton)) {
    rightButtonWasReleased = true;  // if the button was released we can set this flag
    delay(10);                      // delay for debouncing
  }

  // Determine if both buttons were pressed simultaneously
  if (!digitalRead(leftButton) && !digitalRead(rightButton)) {
    bothButtonsPressed = true;
  }

  // Handle button actions
  if (bothButtonsPressed && (leftButtonWasReleased || rightButtonWasReleased)) {  // if both buttons were pressed before release
    dataScreenshotFlag = true;                                                    // Set screenshot flag
    dataScreenshotStart = millis();                                               // Record the current time
    bothButtonsPressed = false;                                                   // Reset flag

    // Not ideally programmed; this can be improved in the future
    // Fill the screen with WHITE for a second to represent a screenshot
    tftL.fillScreen(GC9A01A_WHITE);
    delay(1000);
  }

  if (stopwatchActive) updateStopwatch();  // if the stopwatch is active, continue to update the counter and display

  if (dataScreenshotFlag && ((millis() - dataScreenshotStart) > dataScreenshotFlagDuration)) {
    dataScreenshotFlag = false;  // If the flag has been set for a second, reset it
  }
}


void checkWheelSpinSkid() {

  // Porsche 911 Color Coding:
  // Front Lockup (skid): Purple
  // Rear Lockup (skid): Yellow
  // Front/Rear Traction Control Activation (spin): Blue

  // Toggle light on/off based on elapsed time
  if ((millis() - lastLightSwitchTime) > spinSkidLightOscillationFrequency) {
    lastLightSwitchTime = millis();      // reset the toggle timer
    spinSkidLightOn = !spinSkidLightOn;  // invert the state
  }

  // Block left LCD if any wheels on left display are spinning/skidding
  if (frontLeftWheelState == SKID || frontLeftWheelState == SPIN || rearLeftWheelState == SKID || rearLeftWheelState == SPIN) {
    leftLcdSpinSkidActive = true;
  } else {
    leftLcdSpinSkidActive = false;
  }

  // Block right LCD if any wheels on right display are spinning/skidding
  if (frontRightWheelState == SKID || frontRightWheelState == SPIN || rearRightWheelState == SKID || rearRightWheelState == SPIN) {
    rightLcdSpinSkidActive = true;
  } else {
    rightLcdSpinSkidActive = false;
  }

  // Front Left Wheel Skidding
  if (frontLeftWheelState == SKID && spinSkidLightOn) {
    tftL.fillRect(0, 0, 300, 125, TFT_PURPLE);
  }

  // Rear Left Wheel Skidding
  if (rearLeftWheelState == SKID && spinSkidLightOn) {
    tftL.fillRect(0, 125, 300, 125, TFT_YELLOW);
  }

  // Front Right Wheel Skidding
  if (frontRightWheelState == SKID && spinSkidLightOn) {
    sprite.fillRect(0, 0, 300, 125, TFT_PURPLE);
  }

  // Rear Right Wheel Skidding
  if (rearRightWheelState == SKID && spinSkidLightOn) {
    sprite.fillRect(0, 125, 300, 125, TFT_YELLOW);
  }


  // Front Left Wheel Spinning
  if (frontLeftWheelState == SPIN && spinSkidLightOn) {
    tftL.fillRect(0, 0, 300, 125, TFT_ORANGE);
  }

  // Rear Left Wheel Spinning
  if (rearLeftWheelState == SPIN && spinSkidLightOn) {
    tftL.fillRect(0, 125, 300, 125, TFT_ORANGE);
  }

  // Front Right Wheel Spinning
  if (frontRightWheelState == SPIN && spinSkidLightOn) {
    sprite.fillRect(0, 0, 300, 125, TFT_ORANGE);
  }

  // Rear Right Wheel Spinning
  if (rearRightWheelState == SPIN && spinSkidLightOn) {
    sprite.fillRect(0, 125, 300, 125, TFT_ORANGE);
  }

  // Push to right display -- will this affect the rpmGauge if none of the right display states are active?
  sprite.pushSprite(0, 0);
}

void updateStatusLEDs() {
  DEBUG_SERIAL.print("batteryPercentage: ");
  DEBUG_SERIAL.println(batteryPercentage);

  // update BAT LED
  if (batteryPercentage < 20) analogWrite(lowBatteryLed, lowBatteryLedBrightness);
  else analogWrite(lowBatteryLed, 0);


  // update DAQ LED
  if (sdLoggingActive) {
    analogWrite(dasLed, dasLedBrightness);
  } else {
    analogWrite(dasLed, 0);
  }

  // update CVT LED
  if (primaryTemperature > cvtTempMax || secondaryTemperature > cvtTempMax) analogWrite(cvtLed, cvtLedBrightness);
  if (primaryTemperature < cvtOffTemp && secondaryTemperature < cvtOffTemp) analogWrite(cvtLed, 0);
}

void updateStopwatch() {

  if (leftLcdSpinSkidActive) return;  // Do not update left screen while spin/skid is active

  stopwatchHour = (millis() - stopwatchStartTime) / 3600000;
  stopwatchMinute = (millis() - stopwatchStartTime) / 60000 % 60;
  stopwatchSecond = (millis() - stopwatchStartTime) / 1000 % 60;


  if (stopwatchSecond != lastStopwatchSecond) {  // Only refresh display if stopwatch time has changed

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

    DEBUG_SERIAL.print("Current Time String is: ");
    DEBUG_SERIAL.println(stopwatchString);

    tftL.println(stopwatchString);  // push to the display
  }
}

void updateTime() {
  /*
  DEBUG_SERIAL.print("GPS HH:MM:SS   ");
  DEBUG_SERIAL.print(gpsTimeHour);
  DEBUG_SERIAL.print(":");
  DEBUG_SERIAL.print(gpsTimeMinute);
  DEBUG_SERIAL.print(":");
  DEBUG_SERIAL.println(gpsTimeSecond);

  DEBUG_SERIAL.print("lastUpdatedSecond: ");
  DEBUG_SERIAL.println(lastUpdatedSecond);
  */

  if (leftLcdSpinSkidActive) return;  // Do not update left screen while spin/skid is active

  if (gpsTimeSecond != lastUpdatedSecond) {  //  Only refresh display if GPS time has changed

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

    DEBUG_SERIAL.print("Current Time String is: ");
    DEBUG_SERIAL.println(timeString);

    tftL.println(timeString);  // push to the display
  }
}

void updateCvtRatio() {

  if (leftLcdSpinSkidActive) return;  // Do not update left screen while spin/skid is active


  cvtRatio = ((float)secondaryRPM / (float)(primaryRPM + 1.0));

  if (abs(cvtRatio - lastCvtRatio) > 0.1) {  // Only refresh display if CVT ratio has changed
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

void checkStatus() {

  // At this point, no significant checks are done to the dashboard to report health
  // Perhaps in the future, some code can check if it is receiving CVT data, GPS time data, etc
  // However, for now, a simple "alive" check requested by the Base Station will suffice

  // Set status to something non-zero (1) to show that we are alive
  statusDashboard = 1;
}

//This is necessary to get a decimal number for the CVT ratio (standard MAP function will only return an int)
float mapfloat(long x, long in_min, long in_max, long out_min, long out_max) {
  return (float)(x - in_min) * (out_max - out_min) / (float)(in_max - in_min) + out_min;
}