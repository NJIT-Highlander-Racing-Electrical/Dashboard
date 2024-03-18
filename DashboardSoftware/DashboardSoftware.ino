// EXTREMELY IMPORTANT NOTE: THE FREQUENCY FOR THE CANBUS RATE IS DOUBLED ON THIS BECAUSE THE BOARDS ARE MESSED UP (WIDESPREAD ESP32 ISSUE)

#include <CAN.h>

#define TX_GPIO_NUM 21  // Connects to CTX
#define RX_GPIO_NUM 22  // Connects to CRX

//#include <gpio_viewer.h>
//GPIOViewer gpio_viewer;

#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_GC9A01A.h"
#include "NotoSansBold36.h"
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <TLC591x.h>
#if defined(ENERGIA_ARCH_MSP432R)
#include <stdio.h>
#endif
TLC591x myLED(3, 32, 33, 25, 26);  // Uncomment if using OE pin



// Setup for left circular display
#define TFT_DC_L 16
#define TFT_CS_L 17
Adafruit_GC9A01A tftL(TFT_CS_L, TFT_DC_L);

//X and Y positions on data on left circular display
const int timeX = 33;
const int timeY = 60;
const int cvtRatioTextX = 100;
const int cvtRatioTextY = 180;
const int cvtRatioDataX = 80;
const int cvtRatioDataY = 220;
const int twoWheelX = 23;
const int twoWheelY = 130;
const int fourWheelX = 133;
const int fourWheelY = 130;

//Parameters to control the RPM dial on right circular display
const int cvtMinRPM = 0;
const int cvtMaxRPM = 4000;
const int angleMin = 0;
const int angleMax = 240;

#define NEEDLE_LENGTH 45       // Visible length
#define NEEDLE_WIDTH 11        // Width of needle - make it an odd number
#define NEEDLE_RADIUS 95       // Radius at tip
#define NEEDLE_COLOR1 TFT_RED  // Needle periphery colour
#define NEEDLE_COLOR2 TFT_RED  // Needle centre colour
#define DIAL_CENTRE_X 120
#define DIAL_CENTRE_Y 120

#define AA_FONT_LARGE NotoSansBold36
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite needle = TFT_eSprite(&tft);  // Sprite object for needle
TFT_eSprite spr = TFT_eSprite(&tft);     // Sprite for meter reading
uint16_t* tft_buffer;
bool buffer_loaded = false;
uint16_t spr_width = 0;
uint16_t bg_color = 0;

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (y >= tft.height()) return 0;
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}


//Defintions for CVT data (CVT RPM Sensor -> Dashboard Displays)
int primaryRPM = 0;
int secondaryRPM = 0;
float cvtMinRatio = 0.9;
float cvtMaxRatio = 3.9;
float cvtRatio = 0;
float lastCvtRatio = 0;
int lastRPM = 0;
const float gearboxRatio = 8.25;
const int wheelCircumference = 100;  //Wheel circumference in inches; distance traveled per revolution; should be 23 inches
uint8_t wheelSpeed = 0;

//Number of fuel level LEDs to display (0-9)
int fuelLevel = 0;

//Definitions for four status LEDs
int cvtTemp = 0;
const int cvtTempMax = 100;
bool daqOn = false;
bool batteryLow = false;

//Pins for four status LEDs (power status is tied to VCC)
const int cvtLed = 2;
const int daqLed = 13;
const int batteryLed = 15;

//Pins for three non-green fuel LEDs (brightness was uneven with them on the LED driver :( )
const int redFuelLED = 12;
const int yellowFuelLED1 = 14;
const int yellowFuelLED2 = 27;

//Definitions for 2WD/4WD state
bool fourWheelsEngaged = false;
bool lastFourWheelsState = false;


void setup() {

  Serial.begin(115200);
  //ESP32 MAC can be found using this: https://randomnerdtutorials.com/get-change-esp32-esp8266-mac-address-arduino/#:~:text=Get%20ESP32%20or%20ESP8266%20MAC%20Address
  //This one's is: D4:8A:FC:A8:59:7C
  //gpio_viewer.connectToWifi("SSID", "PASS");
  //gpio_viewer.begin();



  CAN.setPins(RX_GPIO_NUM, TX_GPIO_NUM);

  if (!CAN.begin(1000E3)) {
    Serial.println("Starting CAN failed!");
    while (1)
      ;
  } else {
    Serial.println("CAN Initialized");
  }

  pinMode(cvtLed, OUTPUT);
  pinMode(daqLed, OUTPUT);
  pinMode(batteryLed, OUTPUT);
  pinMode(redFuelLED, OUTPUT);
  pinMode(yellowFuelLED1, OUTPUT);
  pinMode(yellowFuelLED2, OUTPUT);

  myLED.displayEnable();         // This command has no effect if you aren't using OE pin
  myLED.displayBrightness(125);  // 0 is max, 255 is min
  updateSevenSegments(95);       // Part of boot screen sequence

  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tft_output);
  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);



  //Left display configurations
  tftL.begin();
  tftL.setFont(&FreeMonoBold24pt7b);
  tftL.fillScreen(GC9A01A_BLACK);

  //This should be a few seconds of splash screen displaying Highlander Racing logos/stuff on boot
  bootScreen();

  initializeNeedle();

  updateTime();
  updateCvtRatio();
  update2wd4wdState();
}



void loop() {

  updateCanbus();
  cvtRatio = (secondaryRPM / (primaryRPM + 1.0));

  // 0.000015782828283 inches in a mile and 60 minutes in one hour
  wheelSpeed = (secondaryRPM / gearboxRatio) * wheelCircumference * 0.00001578282828 * 60;

  //Serial.print("wheelSpeed: ");
  //Serial.println(wheelSpeed);

  // This takes the calculated speed and displays it on the seven segments
  updateSevenSegments(wheelSpeed);



  // THE FOLLOWING LINES ARE JUST TO SIMULATE CAN BUS INPUTS AND CAN BE DELETED AFTER CAN BUS IMPLEMENTATION
  daqOn = true;
  batteryLow = true;
  fuelLevel = 9;
  // END TEST CODE HERE



  //This takes the CVT rpm and plots the dial accordingly
  int mappedAngle = map(primaryRPM, cvtMinRPM, cvtMaxRPM, angleMin, angleMax);
  if ((abs(primaryRPM - lastRPM)) > 50) {
    plotNeedle(mappedAngle, 1);
    lastRPM = primaryRPM;
  }

  // This updates the time when GPS time is found; always false until GPS is implemented over CAN
  if (false) {
    updateTime();
  }

  //This updates the digital CVT ratio on left display
  if (abs(cvtRatio - lastCvtRatio) > 0.1) {
    lastCvtRatio = cvtRatio;
    updateCvtRatio();
  }

  //This updates the 2WD/4WD State on left display
  if (fourWheelsEngaged != lastFourWheelsState) {
    lastFourWheelsState = fourWheelsEngaged;
    update2wd4wdState();
  }


  // These lines toggle the status LEDs
  if (batteryLow) digitalWrite(batteryLed, HIGH);
  else digitalWrite(batteryLed, LOW);

  if (daqOn) digitalWrite(daqLed, HIGH);
  else digitalWrite(daqLed, LOW);

  if (cvtTemp > cvtTempMax) digitalWrite(cvtLed, HIGH);
  else digitalWrite(cvtLed, LOW);

  delay(5);
}


void updateTime() {

  tftL.fillRect(timeX, timeY, 240, -(cvtRatioTextY - timeY), TFT_BLACK);
  tftL.setFont(&FreeMonoBold18pt7b);
  tftL.setTextColor(TFT_WHITE);
  tftL.setCursor(timeX, timeY);  // Adjust coordinates as needed
  tftL.println("12:34:56");
}

void updateCvtRatio() {

  //The following line was added to prevent a divide by zero error, not sure if this will be needed in the final implementation
  //cvtRatio = (primaryRPM / (secondaryRPM + 1));)
  tftL.fillRect(0, cvtRatioTextY, 240, 100, TFT_BLACK);
  tftL.setTextColor(TFT_WHITE);
  tftL.setCursor(cvtRatioTextX, cvtRatioTextY);  // Adjust coordinates as needed
  tftL.setFont(&FreeMono12pt7b);
  tftL.println("CVT");
  tftL.setFont(&FreeMonoBold24pt7b);
  tftL.setCursor(cvtRatioDataX, cvtRatioDataY);
  tftL.println(cvtRatio, 1);
}


void update2wd4wdState() {

  if (fourWheelsEngaged) {

    tftL.fillRect(twoWheelX, twoWheelY, 240, -50, TFT_BLACK);
    tftL.setFont(&FreeMonoBold24pt7b);
    tftL.setTextColor(0x8410);
    tftL.setCursor(twoWheelX, twoWheelY);  // Adjust coordinates as needed
    tftL.println("2WD");
    tftL.setTextColor(TFT_RED);
    tftL.setCursor(fourWheelX, fourWheelY);  // Adjust coordinates as needed
    tftL.println("4WD");

  }

  else {

    tftL.fillRect(twoWheelX, twoWheelY, 240, -50, TFT_BLACK);
    tftL.setFont(&FreeMonoBold24pt7b);
    tftL.setTextColor(TFT_RED);
    tftL.setCursor(twoWheelX, twoWheelY);  // Adjust coordinates as needed
    tftL.println("2WD");
    tftL.setTextColor(0x8410);
    tftL.setCursor(fourWheelX, fourWheelY);  // Adjust coordinates as needed
    tftL.println("4WD");
  }
}


//This is necessary to get a decimal number for the CVT ratio (standard MAP function will only return an int)
float mapfloat(long x, long in_min, long in_max, long out_min, long out_max) {
  return (float)(x - in_min) * (out_max - out_min) / (float)(in_max - in_min) + out_min;
}
