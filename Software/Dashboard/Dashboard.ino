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


TLC591x myLED(3, 32, 33, 21, 22); //num_chips, SDI_pin, CLK_pin, LE_pin, OE_pin

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
float cvtMinRatio = 0.9;
float cvtMaxRatio = 3.9;
float cvtRatio = 0;
float lastCvtRatio = 0;
int lastRPM = 0;
const float gearboxRatio = 8.25;

//Pins for four status LEDs
const int cvtTempMax = 150;
const int cvtOffTemp = 140;

//Pins for four status LEDs (power status is tied to VCC)
const int cvtLed = 2;
const int lowBatteryLed = 15;
const int dasLed = 27;
 
//Pins for three non-green LEDs (brightness was uneven with them on the LED driver :( )
const int redLed = 12;
const int yellowLed1 = 13;
const int yellowLed2 = 14;

//Variable for battery level, which represents number of LEDs illuminated based on battery percentage 
int batteryLevel;

//Definitions for GPS time
int lastTimeH = 0;
int lastTimeM = 0;
int lastTimeS = 0;

int rightButton=19; //selects screen
int leftButton=34;//changes options
int screenSelect=0; //done by modulus. 0 is brightness, 1 is error code, 2 is mark
bool screenSelectStop=false;
int brightnessUpDown=0;
int MPHBrightness=255;// 0 is max, 255 is min
bool BrightStop=false;
bool ClearDriverData=false;
int ABSTractionLightOscillator=0;
int Osc=6;
int timeEla=0; //seconds
int LastGPSSec=gpsTimeSecond;

void setup() {

  Serial.begin(460800);
  setupCAN(DASHBOARD);

  pinMode(cvtLed, OUTPUT);
  pinMode(dasLed, OUTPUT);
  pinMode(lowBatteryLed, OUTPUT);
  pinMode(redLed, OUTPUT);
  pinMode(yellowLed1, OUTPUT);
  pinMode(yellowLed2, OUTPUT);
  pinMode(rightButton, INPUT);
  pinMode(leftButton, INPUT);


  myLED.displayEnable();       // This command has no effect if you aren't using OE pin
  myLED.displayBrightness(MPHBrightness);  
  updateSevenSegments(95);     // Part of boot screen sequence

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
  totalMultiplier = (1.0 / gearboxRatio) * wheelCircumference * 3.1415926 * 0.00001578282828 * 60.0;  // 00001578282828 is miles in an inch
  updateCvtRatio();
}



void loop() {


  batteryLevel=map(batteryPercentage, 20, 100, 0, 9);

  ABSTractionLightOscillator++;
  if(gpsTimeSecond!=LastGPSSec){
    timeEla++;
  }LastGPSSec=gpsTimeSecond;
  
  cvtRatio = (secondaryRPM / (primaryRPM + 1.0));

  // This takes the GPS speed (mph) and displays it on the seven segments
  updateSevenSegments(gpsVelocity);
 
  //This takes the CVT rpm and plots the dial accordingly
  mappedRPMAngle = map(primaryRPM, cvtMinRPM, cvtMaxRPM, angleMin, angleMax);
  //primaryRPM will be replaced with the wheel spinning the slowest 
  updateRPMGauge(mappedRPMAngle, slowestWheel, Osc, ABSTractionLightOscillator);
  updateTime();

  //This updates the digital CVT ratio on left display
  if (abs(cvtRatio - lastCvtRatio) > 0.1) {
    lastCvtRatio = cvtRatio;
    updateCvtRatio();
  }


  // These lines toggle the status LEDs
  if (batteryPercentage < 20) digitalWrite(lowBatteryLed, HIGH);
  else digitalWrite(lowBatteryLed, LOW);

  if (sdLoggingActive) {
    digitalWrite(dasLed, HIGH);
  } else digitalWrite(dasLed, LOW);
  if (primaryTemperature > cvtTempMax || secondaryTemperature > cvtTempMax) digitalWrite(cvtLed, HIGH);
  if (primaryTemperature < cvtOffTemp && secondaryTemperature < cvtOffTemp) digitalWrite(cvtLed, LOW);
  //Pins for three non-green LEDs (brightness was uneven with them on the LED driver :( )
  //options are brightness, error code screen, mark, and ....
  if(digitalRead(leftButton)==LOW && screenSelectStop==false){
    screenSelect+=1;
    screenSelectStop=true;
  }
  if(digitalRead(leftButton)==HIGH && screenSelectStop==true){
    screenSelectStop=false;
  }
  //this is screen brightness screen
  if(screenSelect%3==0 && digitalRead(rightButton)==LOW && BrightStop==false){
    brightnessUpDown+=1;
    BrightStop=true;
  }
  if(screenSelect%3==0 && digitalRead(rightButton)==HIGH && BrightStop==true){
    BrightStop=false;
  }
  if(digitalRead(leftButton)==LOW && ClearDriverData==false){
    tftL.fillRect(timeHourX-40, timeY+5, 300, 95, TFT_WHITE);
    ClearDriverData==true;
  }if(digitalRead(rightButton)==LOW && ClearDriverData==false && screenSelect%3!=2){
    tftL.fillRect(timeHourX-40, timeY+5, 300, 95, TFT_WHITE);
    ClearDriverData==true;
  }
  if(digitalRead(leftButton)==HIGH && digitalRead(rightButton)==HIGH && ClearDriverData==true){
    ClearDriverData==true;
  }
  
  if(screenSelect%4==0 && brightnessUpDown%2==0 && digitalRead(rightButton)==LOW){//0 mod 3 is 0, 1 mod 3 is 1, 2 mod 3 is 2, 3 mod 3 is 0....0 mod 2 is 0, 1 mod 2 is 1, 2 mod 2 is 0
    MPHBrightness+=1; //lower brightness
    tftL.setFont(&FreeMonoBold18pt7b);
    tftL.setTextColor(TFT_BLACK);
    tftL.setCursor(timeHourX-20, timeY+50);
    tftL.print("brightness");
    tftL.setCursor(timeHourX+55, timeY+90);
    tftL.print(MPHBrightness);
  }else if(screenSelect%4==0 && brightnessUpDown%2==1 && digitalRead(rightButton)==LOW){
    MPHBrightness-=1; //increase brightness
    tftL.setFont(&FreeMonoBold18pt7b);
    tftL.setTextColor(TFT_BLACK);
    tftL.setCursor(timeHourX-20, timeY+50);
    tftL.print("brightness");
    tftL.setCursor(timeHourX+55, timeY+90);
    tftL.print(MPHBrightness);
  }else if(screenSelect%4==0){
    tftL.setFont(&FreeMonoBold18pt7b);
    tftL.setTextColor(TFT_BLACK);
    tftL.setCursor(timeHourX-20, timeY+50);
    tftL.print("brightness");
    tftL.setCursor(timeHourX+55, timeY+90);
    tftL.print(MPHBrightness);
  }else if(screenSelect%4==2 && digitalRead(rightButton)==LOW){//write to SD card
    sdLoggingActive=0; //0x3A
    tftL.setFont(&FreeMono12pt7b);
    tftL.setTextColor(TFT_BLACK);
    tftL.setCursor(timeHourX-5, timeY+50);
    tftL.print("Shits fucked?");
    tftL.setCursor(timeHourX-20, timeY+80);
    tftL.print("PressLeftButton");
  }else if(screenSelect%4==2 && digitalRead(rightButton)==HIGH){//write to SD card
    sdLoggingActive=1; //0x3A
    tftL.setFont(&FreeMono12pt7b);
    tftL.setTextColor(TFT_BLACK);
    tftL.setCursor(timeHourX-5, timeY+50);
    tftL.print("Shits fucked?");
    tftL.setCursor(timeHourX-20, timeY+80);
    tftL.print("PressLeftButton");
  }else if(screenSelect%4==1 && digitalRead(leftButton)==LOW){//error screen
    if (primaryTemperature > cvtTempMax || secondaryTemperature > cvtTempMax){
      tftL.setFont(&FreeMono12pt7b);
      tftL.setTextColor(TFT_BLACK);
      tftL.setCursor(timeHourX+40, timeY+30);
      tftL.print("Error");
      tftL.setCursor(timeHourX+50, timeY+60);
      tftL.print("CVTH"); //whose fucked
      tftL.setCursor(timeHourX+35, timeY+90);
      tftL.print(primaryTemperature);
      tftL.setCursor(timeHourX+75, timeY+90);
      tftL.print(secondaryTemperature);
    }else{
      tftL.setFont(&FreeMonoBold18pt7b);
      tftL.setTextColor(TFT_BLACK);
      tftL.setCursor(timeHourX, timeY+30);
      tftL.print("No Error");
    }
  }else if(screenSelect%4==3){
    //hours
    tftL.fillRect(timeHourX, timeY+30, (timeMinuteX - timeHourX-20), -(cvtRatioTextY - timeY-95), TFT_WHITE);
    tftL.setCursor(timeHourX, timeY+30);  // Adjust coordinates as needed
    if (timeEla/3600 < 10) tftL.print("0");
    tftL.print(timeEla/3600); 
    tftL.print(":");
    //mins
    tftL.fillRect(timeMinuteX, timeY+30, (timeSecondX - timeMinuteX)-20, -(cvtRatioTextY - timeY-95), TFT_WHITE);
    tftL.setCursor(timeMinuteX, timeY+30);  // Adjust coordinates as needed
    if (timeEla/60 < 10) tftL.print("0");
    tftL.print(timeEla/60);
    tftL.print(":");
    tftL.fillRect(timeMinuteX, timeY+30, (timeSecondX - timeMinuteX)-20, -(cvtRatioTextY - timeY-95), TFT_WHITE);
    tftL.setCursor(timeMinuteX, timeY+30);  // Adjust coordinates as needed
    if (timeEla/60 < 10) tftL.print("0");
    tftL.print(timeEla/60);
    tftL.print(":");
    //seconds
    tftL.fillRect(timeSecondX, timeY+30, (240 - timeSecondX-3), -(cvtRatioTextY - timeY-95), TFT_WHITE);
    tftL.setCursor(timeSecondX, timeY+30);  // Adjust coordinates as needed
    if (timeEla < 10) tftL.print("0");
    tftL.print(timeEla%60);
    tftL.fillRect(timeSecondX, timeY+30, (240 - timeSecondX-3), -(cvtRatioTextY - timeY-95), TFT_WHITE);
    tftL.setCursor(timeSecondX, timeY+30);  // Adjust coordinates as needed
    if (timeEla%60 < 10) tftL.print("0");
    tftL.print(timeEla%60);
  }else{

  }

}


void updateTime() {


 /* tftL.setFont(&FreeMonoBold18pt7b);
  tftL.setTextColor(TFT_BLACK);
  tftL.setCursor(timeHourX, timeY);
  tftL.print("04:55:55");*/
  tftL.setFont(&FreeMonoBold18pt7b);
 // if (gpsTimeHour != lastTimeH) {
    //hasBeenPinged = true;
    //lastTimeH = gpsTimeHour;
    tftL.fillRect(timeHourX, timeY, (timeMinuteX - timeHourX-15), -(cvtRatioTextY - timeY-95), TFT_WHITE);
    tftL.setCursor(timeHourX, timeY);  // Adjust coordinates as needed
    if (gpsTimeHour < 10) tftL.print("0");
    tftL.print(gpsTimeHour); 
    tftL.print(":");
  //}
  //if (gpsTimeMinute != lastTimeM) {
    //lastTimePingM = millis();
    //lastTimeM = gpsTimeMinute;
    tftL.fillRect(timeMinuteX, timeY, (timeSecondX - timeMinuteX)-20, -(cvtRatioTextY - timeY-95), TFT_WHITE);
    tftL.setCursor(timeMinuteX, timeY);  // Adjust coordinates as needed
    if (gpsTimeMinute < 10) tftL.print("0");
    tftL.print(gpsTimeMinute);
    tftL.print(":");
  //}
  //if (((millis() - lastTimePingM) > 60000) && hasBeenPinged) {
    //lastTimePingM = millis();
    //gpsTimeMinute = gpsTimeMinute + 1;
    //lastTimeM = gpsTimeMinute;
    tftL.fillRect(timeMinuteX, timeY, (timeSecondX - timeMinuteX)-20, -(cvtRatioTextY - timeY-95), TFT_WHITE);
    tftL.setCursor(timeMinuteX, timeY);  // Adjust coordinates as needed
    if (gpsTimeMinute < 10) tftL.print("0");
    tftL.print(gpsTimeMinute);
    tftL.print(":");
  //}

 // if (gpsTimeSecond != lastTimeS) {
    //lastTimePingS = millis();
    //lastTimeS = gpsTimeSecond;
    tftL.fillRect(timeSecondX, timeY, (240 - timeSecondX-3), -(cvtRatioTextY - timeY-95), TFT_WHITE);
    tftL.setCursor(timeSecondX, timeY);  // Adjust coordinates as needed
    if (gpsTimeSecond < 10) tftL.print("0");
    tftL.print(gpsTimeSecond);
 // }
  //if (((millis() - lastTimePingS) > 1000) && hasBeenPinged) {
    //lastTimePingS = millis();
    //if (gpsTimeSecond > 59) {
     // gpsTimeMinute++;
     // gpsTimeSecond = 0;
    //} else gpsTimeSecond++;
    //lastTimeS = gpsTimeSecond;
    tftL.fillRect(timeSecondX, timeY, (240 - timeSecondX-3), -(cvtRatioTextY - timeY-95), TFT_WHITE);
    tftL.setCursor(timeSecondX, timeY);  // Adjust coordinates as needed
    if (gpsTimeSecond < 10) tftL.print("0");
    tftL.print(gpsTimeSecond);
  //}
}

void updateCvtRatio() {
  tftL.fillRect(0, cvtRatioTextY, 240, 50, TFT_WHITE);
  tftL.setTextColor(TFT_BLACK);
  tftL.setCursor(cvtRatioTextX, cvtRatioTextY);  // Adjust coordinates as needed
  tftL.setFont(&FreeMono12pt7b);
  tftL.println("CVT");
  tftL.setFont(&FreeMonoBold24pt7b);
  tftL.setCursor(cvtRatioDataX, cvtRatioDataY); //cvtRatioTextX, cvtRatioTextY
  tftL.println(cvtRatio, 1);
  //checks for wheel slippage or locking
  if(frontLeftWheelRPM==0 && gpsVelocity>0 && ABSTractionLightOscillator%Osc==0){
    tftL.fillRect(0,0,300,125,TFT_RED);
  }
  if(frontLeftWheelRPM>0 && gpsVelocity==0 && ABSTractionLightOscillator%Osc==0){
    tftL.fillRect(0,0,300,125,TFT_ORANGE);
  }
  if(rearLeftWheelRPM==0 && gpsVelocity>0 && ABSTractionLightOscillator%Osc==0){
    tftL.fillRect(0,125,300,125,TFT_RED);
  }
  if(rearLeftWheelRPM>0 && gpsVelocity==0 &&ABSTractionLightOscillator%Osc==0){
    tftL.fillRect(0,125,300,125,TFT_ORANGE);
  }
}

//This is necessary to get a decimal number for the CVT ratio (standard MAP function will only return an int)
float mapfloat(long x, long in_min, long in_max, long out_min, long out_max) {
  return (float)(x - in_min) * (out_max - out_min) / (float)(in_max - in_min) + out_min;
}
