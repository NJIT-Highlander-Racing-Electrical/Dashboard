#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_GC9A01A.h"

#define TFT_DC_L 16
#define TFT_CS_L 17
Adafruit_GC9A01A tftL(TFT_CS_L, TFT_DC_L);

#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>

const int timeX = 30;
const int timeY = 60;
const int cvtRatioX = 85;
const int cvtRatioY = 150;
const int twoWheelX = 50;
const int twoWheelY = 220;
const int fourWheelX = 125;
const int fourWheelY = 220;



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
#include "NotoSansBold36.h"
#define AA_FONT_LARGE NotoSansBold36
#include "dial.h"
#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite needle = TFT_eSprite(&tft);  // Sprite object for needle
TFT_eSprite spr = TFT_eSprite(&tft);     // Sprite for meter reading
#include <TJpg_Decoder.h>
uint16_t* tft_buffer;
bool buffer_loaded = false;
uint16_t spr_width = 0;
uint16_t bg_color = 0;

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (y >= tft.height()) return 0;
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}






// QUESTIONS FOR THIS PROJECT

// What voltage/current do the 7-segments need?
// Would this 8-bit shift register work? They can be daisy chained https://www.digikey.com/en/products/detail/texas-instruments/CD74HC597E/1507407
// How to interface TJA1051T with ESP32 TWAI Controller
// Keep a log of all libraries I use to upload with this code to GitHub!



int primaryRPM = 3000;
int secondaryRPM = 0;
float cvtMinRatio = 0.9;
float cvtMaxRatio = 3.9;
float cvtRatio = 0;
float lastCvtRatio = 0;
int lastRPM = 0;
const int gearboxRatio = 3;
int wheelSpeed = 0;

int fuelLevel = 0;

int cvtTemp = 0;
const int cvtTempMax = 100;
bool daqOn = false;
bool batteryLow = false;

const int cvtLed = 32;
const int daqLed = 33;
const int batteryLed = 25;

bool fourWheelsEngaged = false;
bool lastFourWheelsState = false;

int frontLeftWheel = 0;
int frontRightWheel = 0;
int rearLeftWheel = 0;
int rearRightWheel = 0;


//THESE CAN BE DELETED; JUST FOR TESTING PURPOSES
int switchTime = 2000;
int lastSwitchTime = 0;
const int CVT_TEST_POT = 15;

void setup() {

  Serial.begin(115200);

  pinMode(cvtLed, OUTPUT);
  pinMode(daqLed, OUTPUT);
  pinMode(batteryLed, OUTPUT);
  pinMode(CVT_TEST_POT, INPUT);


  TJpgDec.setSwapBytes(true);

  // The jpeg decoder must be given the exact name of the rendering function above
  TJpgDec.setCallback(tft_output);

  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  // Draw the dial
  TJpgDec.drawJpg(0, 0, dial, sizeof(dial));
  //tft.drawCircle(DIAL_CENTRE_X, DIAL_CENTRE_Y, NEEDLE_RADIUS-NEEDLE_LENGTH, TFT_DARKGREY);

  // Load the font and create the Sprite for reporting the value
  spr.loadFont(AA_FONT_LARGE);
  spr_width = spr.textWidth("7777");  // 7 is widest numeral in this font
  spr.createSprite(spr_width, spr.fontHeight());
  bg_color = tft.readPixel(120, 120);  // Get colour from dial centre
  spr.fillSprite(bg_color);
  spr.setTextColor(TFT_WHITE, bg_color, true);
  spr.setTextDatum(MC_DATUM);
  spr.setTextPadding(spr_width);
  spr.drawNumber(0, spr_width / 2, spr.fontHeight() / 2);
  spr.pushSprite(DIAL_CENTRE_X - spr_width / 2, DIAL_CENTRE_Y - spr.fontHeight() / 2);
  tft.setTextColor(TFT_WHITE, bg_color);
  tft.setTextDatum(MC_DATUM);
  tft.setPivot(DIAL_CENTRE_X, DIAL_CENTRE_Y);
  createNeedle();
  plotNeedle(0, 0);
  delay(500);



  tftL.begin();
  tftL.setFont(&FreeMonoBold18pt7b);
  tftL.setFont(&FreeMonoBold24pt7b);


  tftL.fillScreen(GC9A01A_BLACK);

  bootScreen();

  updateTime();
  updateCvtRatio();
  update2wd4wdState();

  //drawFourWheels();
}



void loop() {



  // Get CANBUS Data

  /*
*     FUEL GAUGE: 0-9 VALUE
*     2WD/4WD INPUT FROM DAQ
*     LOW BAT WARNING FROM DAQ
*     CVT PRIMARY/SECONDARY RPM FROM CVT
*     CVT RATIO FROM CVT
*     CVT OVER TEMP WARNING FROM CVT
*     
*/

  //

  daqOn = true;
  batteryLow = true;
  cvtTemp = 101;

  cvtRatio = mapfloat(analogRead(CVT_TEST_POT), 0.0, 4095.0, cvtMinRatio, cvtMaxRatio);
  if (analogRead(CVT_TEST_POT) > 2000) fourWheelsEngaged = true;
  else fourWheelsEngaged = false;
  primaryRPM = map(analogRead(CVT_TEST_POT), 0, 4095, cvtMinRPM, cvtMaxRPM);
  //

  int mappedAngle = map(primaryRPM, cvtMinRPM, cvtMaxRPM, angleMin, angleMax);
  if ((abs(primaryRPM - lastRPM)) > 50) {
    plotNeedle(mappedAngle, 1);  // if this doesnt work, add ms delay back to dial plot function
    lastRPM = primaryRPM;
  }

  if (false) {
    updateTime();
  }

  if (abs(cvtRatio - lastCvtRatio) > 0.1) {
    lastCvtRatio = cvtRatio;
    updateCvtRatio();
  }

  if (fourWheelsEngaged != lastFourWheelsState) {
    lastFourWheelsState = fourWheelsEngaged;
    update2wd4wdState();
  }





  if (batteryLow) digitalWrite(batteryLed, HIGH);
  else digitalWrite(batteryLed, LOW);

  if (daqOn) digitalWrite(daqLed, HIGH);
  else digitalWrite(daqLed, LOW);

  if (cvtTemp > cvtTempMax) digitalWrite(cvtLed, HIGH);
  else digitalWrite(cvtLed, LOW);
}


void updateTime() {

  tftL.fillRect(timeX, timeY, 240, -(cvtRatioY - timeY), TFT_BLACK);
  tftL.setFont(&FreeMonoBold18pt7b);
  tftL.setTextColor(TFT_WHITE);
  tftL.setCursor(timeX, timeY);  // Adjust coordinates as needed
  tftL.println("12:34:56");
}

void updateCvtRatio() {

  //cvtRatio = (primaryRPM / (secondaryRPM + 1));)
  tftL.fillRect(cvtRatioX, cvtRatioY, 240, -(twoWheelY - cvtRatioY), TFT_BLACK);
  tftL.setFont(&FreeMonoBold24pt7b);
  tftL.setTextColor(TFT_WHITE);
  tftL.setCursor(cvtRatioX, cvtRatioY);  // Adjust coordinates as needed
  tftL.println(cvtRatio, 1);
}


void update2wd4wdState() {

  if (fourWheelsEngaged) {

    tftL.fillRect(twoWheelX, twoWheelY, 240, -50, TFT_BLACK);
    tftL.setFont(&FreeMonoBold18pt7b);
    tftL.setTextColor(0x8410);
    tftL.setCursor(twoWheelX, twoWheelY);  // Adjust coordinates as needed
    tftL.println("2WD");
    tftL.setTextColor(TFT_RED);
    tftL.setCursor(fourWheelX, fourWheelY);  // Adjust coordinates as needed
    tftL.println("4WD");

  }

  else {

    tftL.fillRect(twoWheelX, twoWheelY, 240, -50, TFT_BLACK);
    tftL.setFont(&FreeMonoBold18pt7b);
    tftL.setTextColor(TFT_RED);
    tftL.setCursor(twoWheelX, twoWheelY);  // Adjust coordinates as needed
    tftL.println("2WD");
    tftL.setTextColor(0x8410);
    tftL.setCursor(fourWheelX, fourWheelY);  // Adjust coordinates as needed
    tftL.println("4WD");
  }
}

/*
void drawFourWheels() {

  tftL.fillRect(54, 27, 30, 55, GC9A01A_RED);
  tftL.fillRect(156, 27, 30, 55, GC9A01A_RED);
  tftL.fillRect(156, 156, 30, 55, GC9A01A_RED);
  tftL.fillRect(54, 156, 30, 55, GC9A01A_RED);

  tftL.fillRect(78, 49, 82, 6, GC9A01A_RED);
  tftL.fillRect(78, 180, 82, 6, GC9A01A_RED);
  tftL.fillRect(124, 53, 6, 133, GC9A01A_RED);
}

void updateFourWheels() {

  digitalWrite(TFT_CS_L, LOW);
  digitalWrite(5, HIGH);

  if (fourWheelsEngaged) {
    tftL.fillRect(60, 33, 19, 44, GC9A01A_RED);
    tftL.fillRect(162, 33, 19, 44, GC9A01A_RED);
  }

  else {
    tftL.fillRect(60, 33, 19, 44, GC9A01A_BLACK);
    tftL.fillRect(162, 33, 19, 44, GC9A01A_BLACK);
  }

  digitalWrite(TFT_CS_L, HIGH);
  digitalWrite(5, HIGH);
}
*/

void bootScreen() {
}





void createNeedle(void) {
  needle.setColorDepth(16);
  needle.createSprite(NEEDLE_WIDTH, NEEDLE_LENGTH);  // create the needle Sprite

  needle.fillSprite(TFT_BLACK);  // Fill with black

  uint16_t piv_x = NEEDLE_WIDTH / 2;  // pivot x in Sprite (middle)
  uint16_t piv_y = NEEDLE_RADIUS;     // pivot y in Sprite
  needle.setPivot(piv_x, piv_y);      // Set pivot point in this Sprite

  needle.fillRect(0, 0, NEEDLE_WIDTH, NEEDLE_LENGTH, TFT_RED);
  needle.fillRect(1, 1, NEEDLE_WIDTH - 2, NEEDLE_LENGTH - 2, TFT_RED);

  int16_t min_x;
  int16_t min_y;
  int16_t max_x;
  int16_t max_y;

  needle.getRotatedBounds(45, &min_x, &min_y, &max_x, &max_y);
  tft_buffer = (uint16_t*)malloc(((max_x - min_x) + 2) * ((max_y - min_y) + 2) * 2);
}

void plotNeedle(int16_t angle, uint16_t ms_delay) {
  digitalWrite(TFT_CS_L, HIGH);
  digitalWrite(5, LOW);
  static int16_t old_angle = -120;

  // Bounding box parameters
  static int16_t min_x;
  static int16_t min_y;
  static int16_t max_x;
  static int16_t max_y;

  if (angle < 0) angle = 0;
  if (angle > 240) angle = 240;

  angle -= 120;

  while (angle != old_angle || !buffer_loaded) {

    if (old_angle < angle) old_angle++;
    else old_angle--;

    if ((old_angle & 1) == 0) {
      if (buffer_loaded) {
        tft.pushRect(min_x, min_y, 1 + max_x - min_x, 1 + max_y - min_y, tft_buffer);
      }

      if (needle.getRotatedBounds(old_angle, &min_x, &min_y, &max_x, &max_y)) {
        tft.readRect(min_x, min_y, 1 + max_x - min_x, 1 + max_y - min_y, tft_buffer);
        buffer_loaded = true;
      }

      needle.pushRotated(old_angle, TFT_BLACK);
    }

    // Update the number at the centre of the dial
    spr.setTextColor(TFT_WHITE, bg_color, true);
    spr.drawNumber(primaryRPM, spr_width / 2, spr.fontHeight() / 2);
    spr.pushSprite(120 - spr_width / 2, 120 - spr.fontHeight() / 2);



    // Slow needle down slightly as it approaches the new position
    if (abs(old_angle - angle) < 10) ms_delay += ms_delay / 5;
  }

  digitalWrite(TFT_CS_L, HIGH);
  digitalWrite(5, HIGH);
}


float mapfloat(long x, long in_min, long in_max, long out_min, long out_max) {
  return (float)(x - in_min) * (out_max - out_min) / (float)(in_max - in_min) + out_min;
}
