// EXTREMELY IMPORTANT NOTE: THE FREQUENCY FOR THE CANBUS RATE IS DOUBLED ON THIS BECAUSE THE BOARDS ARE MESSED UP (WIDESPREAD ESP32 ISSUE)

#include <CAN.h>

TaskHandle_t CAN_Task;

#define DEBUG true
#define DEBUG_SERIAL \
  if (DEBUG) Serial

#define TX_GPIO_NUM 21  // Connects to CTX
#define RX_GPIO_NUM 22  // Connects to CRX

//#include <gpio_viewer.h>
//GPIOViewer gpio_viewer;

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
int secondaryRPM = 0;
float cvtMinRatio = 0.9;
float cvtMaxRatio = 3.9;
float cvtRatio = 0;
float lastCvtRatio = 0;
int lastRPM = 0;
const float gearboxRatio = 8.25;
const float wheelCircumference = 23.0;  //Wheel circumference in inches; distance traveled per revolution; should be 23 inches
float totalMultiplier = 0;
uint8_t wheelSpeed = 0;

//Number of fuel level LEDs to display (0-9)
int fuelLevel = 0;

//Definitions for four status LEDs
int cvtTemp = 0;
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

//Definitions for 2WD/4WD state
bool fourWheelsState = false;
bool lastFourWheelsState = false;
bool fourWheelsUnknown = true;
bool fourWheelsStartFlag = true;

int canbusDataPeriod = 50;
unsigned long canbusDataStart = 0;


//Definitions for GPS time
int timeH = 0;
int timeM = 0;
int timeS = 0;

int lastTimeH = 0;
int lastTimeM = 0;
int lastTimeS = 0;

unsigned long lastTimePingM = 0;
unsigned long lastTimePingS = 0;
bool hasBeenPinged = false;

//char formattedTime[9];

void setup() {

  DEBUG_SERIAL.begin(460800);
  //ESP32 MAC can be found using this: https://randomnerdtutorials.com/get-change-esp32-esp8266-mac-address-arduino/#:~:text=Get%20ESP32%20or%20ESP8266%20MAC%20Address
  //This one's is: D4:8A:FC:A8:59:7C
  //gpio_viewer.connectToWifi("SSID", "PASSWORD");
  //gpio_viewer.begin();



  CAN.setPins(RX_GPIO_NUM, TX_GPIO_NUM);

  if (!CAN.begin(1000E3)) {
    DEBUG_SERIAL.println("Starting CAN failed!");
    while (1)
      ;
  } else {
    DEBUG_SERIAL.println("CAN Initialized");
  }

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


  xTaskCreatePinnedToCore(
    CAN_Task_Code, /* Task function. */
    "CAN_Task",   /* name of task. */
    10000,     /* Stack size of task */
    NULL,      /* parameter of the task */
    1,         /* priority of the task */
    &CAN_Task,    /* Task handle to keep track of created task */
    0);        /* pin task to core 0 */
  delay(250);

  //Left display configurations
  tftL.begin();
  tftL.setFont(&FreeMonoBold24pt7b);
  tftL.fillScreen(GC9A01A_BLACK);

  //This should be a few seconds of splash screen displaying Highlander Racing logos/stuff on boot
  bootScreen();

  updateTime();
  totalMultiplier = (1.0 / gearboxRatio) * wheelCircumference * 3.1415926 * 0.00001578282828 * 60.0;  // 00001578282828 is miles in an inch
  updateCvtRatio();
  update2wd4wdState();
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

  //This updates the 2WD/4WD State on left display

  if (!fourWheelsUnknown && fourWheelsStartFlag) {
    fourWheelsStartFlag = false;
    lastFourWheelsState = fourWheelsState;
    update2wd4wdState();
  }

  if (fourWheelsState != lastFourWheelsState) {
    lastFourWheelsState = fourWheelsState;
    update2wd4wdState();
  }



  // These lines toggle the status LEDs
  if (batteryLow) digitalWrite(batteryLed, HIGH);
  else digitalWrite(batteryLed, LOW);

  if (daqOn && !daqError) {
    digitalWrite(daqLed, HIGH);
  } else if (daqError) {
    analogWrite(daqLed, 25);
  } else digitalWrite(daqLed, LOW);

  if (cvtTemp > cvtTempMax) digitalWrite(cvtLed, HIGH);
  if (cvtTemp < cvtOffTemp) digitalWrite(cvtLed, LOW);
}


void updateTime() {


  tftL.setFont(&FreeMonoBold18pt7b);
  tftL.setTextColor(TFT_WHITE);

  if (timeH != lastTimeH) {
    hasBeenPinged = true;
    lastTimeH = timeH;
    tftL.fillRect(timeHourX, timeY, (timeMinuteX - timeHourX), -(cvtRatioTextY - timeY), TFT_BLACK);
    tftL.setCursor(timeHourX, timeY);  // Adjust coordinates as needed
    if (timeH < 10) tftL.print("0");
    tftL.print(timeH);
    tftL.print(":");
  }

  if (timeM != lastTimeM) {
    lastTimePingM = millis();
    lastTimeM = timeM;
    tftL.fillRect(timeMinuteX, timeY, (timeSecondX - timeMinuteX), -(cvtRatioTextY - timeY), TFT_BLACK);
    tftL.setCursor(timeMinuteX, timeY);  // Adjust coordinates as needed
    if (timeM < 10) tftL.print("0");
    tftL.print(timeM);
    tftL.print(":");
  }
  if (((millis() - lastTimePingM) > 60000) && hasBeenPinged) {
    lastTimePingM = millis();
    timeM = timeM + 1;
    lastTimeM = timeM;
    tftL.fillRect(timeMinuteX, timeY, (240 - timeMinuteX), -(cvtRatioTextY - timeY), TFT_BLACK);
    tftL.setCursor(timeMinuteX, timeY);  // Adjust coordinates as needed
    if (timeM < 10) tftL.print("0");
    tftL.println(timeM);
  }

  if (timeS != lastTimeS) {
    lastTimePingS = millis();
    lastTimeS = timeS;
    tftL.fillRect(timeSecondX, timeY, (240 - timeSecondX), -(cvtRatioTextY - timeY), TFT_BLACK);
    tftL.setCursor(timeSecondX, timeY);  // Adjust coordinates as needed
    if (timeS < 10) tftL.print("0");
    tftL.println(timeS);
  }
  if (((millis() - lastTimePingS) > 1000) && hasBeenPinged) {
    lastTimePingS = millis();
    if (timeS > 59) {
      timeM++;
      timeS = 0;
    } else timeS++;
    lastTimeS = timeS;
    tftL.fillRect(timeSecondX, timeY, (240 - timeSecondX), -(cvtRatioTextY - timeY), TFT_BLACK);
    tftL.setCursor(timeSecondX, timeY);  // Adjust coordinates as needed
    if (timeS < 10) tftL.print("0");
    tftL.println(timeS);
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


//This is necessary to get a decimal number for the CVT ratio (standard MAP function will only return an int)
float mapfloat(long x, long in_min, long in_max, long out_min, long out_max) {
  return (float)(x - in_min) * (out_max - out_min) / (float)(in_max - in_min) + out_min;
}



void CAN_Task_Code( void * pvParameters ){
  Serial.print("CAN Task running on core ");
  Serial.println(xPortGetCoreID());

  for(;;){
 
 updateCanbus();  // dedicate 100ms to just canbus sampling to not miss messages
    delay(1);

  } 
}
