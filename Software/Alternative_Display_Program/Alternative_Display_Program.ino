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
const int stopwatchX = 0;
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

const int engineMinRPM = 0;
const int engineMaxRPM = 4000;

// Pins for four status LEDs
const int cvtTempMax = 150;
const int cvtOffTemp = 140;

// Pins for four status LEDs (power status is tied to VCC)
const int cvtLed = 2;
const int lowBatteryLed = 15;
const int dasLed = 27;

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
int ledBrightness = 175;  // 0 is max, 255 is min

// Definitions for stopwatch
bool stopwatchActive = false;
unsigned long stopwatchStartTime = 0;
unsigned long stopwatchElapsedTime = 0;  // milliseconds
int stopwatchHour = 0;
int stopwatchMinute = 0;
int stopwatchSecond = 0;
int stopwatchMillisecond = 0;
int lastStopwatchSecond = 0;       // only update the stopwatch when the value of the second changes (to prevent display flicker)
int stopwatchUpdateInterval = 10;  //update every x milliseconds
unsigned long lastStopwatchUpdateTime = 0;
String lastStopwatchString = "";  // we write this in backgroudn color before writing hte next digits to prevent flickering. better than drawing whole rectangle over area


int screenSelect = 0;  //done by modulus. 0 is brightness, 1 is error code, 2 is mark
bool screenSelectStop = false;
int brightnessUpDown = 0;
int ABSTractionLightOscillator = 0;
int Osc = 6;


int gpsVelocity = 95;  // vehicle speed numbers
int batteryPercentage = 100;

GFXcanvas16 canvas(200, 40);  // Create a small buffer for text rendering

struct RPMAnimation {
  int minRPM;
  int maxRPM;
  int durationMs;
  unsigned long startTime;
  bool animating;

  void start(int minRPM, int maxRPM, int durationMs) {
    this->minRPM = minRPM;
    this->maxRPM = maxRPM;
    this->durationMs = durationMs;
    this->startTime = millis();
    this->animating = true;
  }

  void update() {
    if (!animating) return;

    unsigned long elapsed = millis() - startTime;
    if (elapsed >= durationMs) {
      updateRPMGauge(maxRPM);
      animating = false;
      return;
    }

    float progress = elapsed / (float)durationMs;
    float angle = progress * PI;  // Half sine wave (0 to PI)
    int currentRPM = minRPM + (maxRPM - minRPM) * (0.5 * (1 - cos(angle)));
    updateRPMGauge(currentRPM);
  }
};

RPMAnimation rpmAnim;


void setup() {

  Serial.begin(460800);

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
  tftL.fillScreen(GC9A01A_BLACK);

  //This should be a few seconds of splash screen displaying Highlander Racing logos/stuff on boot
  bootScreen();




  // PUSH STATIC STUFF TO DISPLAYS

  tftL.setFont(&FreeMonoBold18pt7b);
  tftL.setTextColor(TFT_WHITE);
  tftL.fillRect(0, stopwatchY, 240, -(cvtRatioTextY - stopwatchY), TFT_BLACK);


  analogWrite(lowBatteryLed, 150);
  analogWrite(dasLed, 100);
  analogWrite(cvtLed, 150);

  // Clear old ratio value
  tftL.fillRect(0, cvtRatioTextY, 240, 50, TFT_BLACK);

  // Display "CVT Text"
  tftL.setTextColor(TFT_WHITE);
  tftL.setCursor(cvtRatioTextX, cvtRatioTextY);  // Adjust coordinates as needed
  tftL.setFont(&FreeMono12pt7b);
  tftL.println("CVT");

  // Display Ratio Number
  tftL.setFont(&FreeMonoBold24pt7b);
  tftL.setCursor(cvtRatioDataX, cvtRatioDataY);  //cvtRatioTextX, cvtRatioTextY
  tftL.println(0.72, 1);

  batteryLevel = 9;
  batteryPercentage = 100;


  tftL.setFont(&FreeMonoBold18pt7b);
  tftL.setTextColor(TFT_WHITE);
  tftL.fillRect(0, timeY, 240, -(cvtRatioTextY - timeY - 95), TFT_BLACK);
  tftL.setCursor(timeX, timeY);  // Adjust coordinates as needed


  tftL.println("12:34:56");  // push to the display

  // This takes the GPS speed (mph) and displays it on the seven segments
  // It also updates the battery level LED Bar
  gpsVelocity = 95;
  updateLedDisplays();
}



void loop() {


  rpmAnim.update();  // Call this frequently in the main loop

  if ((millis() - lastStopwatchUpdateTime) > stopwatchUpdateInterval) {
    lastStopwatchUpdateTime = millis();

    stopwatchHour = (millis() - stopwatchStartTime) / 3600000;
    stopwatchMinute = (millis() - stopwatchStartTime) / 60000 % 60;
    stopwatchSecond = (millis() - stopwatchStartTime) / 1000 % 60;
    stopwatchMillisecond = (millis() - stopwatchStartTime) % 1000;


    String stopwatchString = "";

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

    stopwatchString += ".";  // append a dot

    stopwatchString += stopwatchMillisecond;
    canvas.fillScreen(GC9A01A_BLACK);  // Clear only the buffer, not the display
    canvas.setTextColor(GC9A01A_WHITE);
    canvas.setTextSize(3);  // Adjust text size as needed
    canvas.setCursor(0, 0);
    canvas.print(stopwatchString);

    tftL.drawRGBBitmap(18, 100, canvas.getBuffer(), 200, 40);  // Push buffer to display

    lastStopwatchString = stopwatchString;
  }
}
