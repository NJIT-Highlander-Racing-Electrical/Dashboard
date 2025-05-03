//A function that accepts an integer number (less than 99MPH) and configures each individual segment
//Once the proper segments are set, the displayData is written
//This also updated the battery LEDs using the global battery level variable

// Store the last battery level
int lastBatteryLevel = -1;  // Set an invalid initial value

// Define a hysteresis range (e.g., within 5 percentage points) (to eliminate flicker)
int hysteresisBuffer = 8;

void updateLedDisplays() {

  float slowestWheelSpeed = frontLeftWheelSpeed;
  if (frontRightWheelSpeed < frontLeftWheelSpeed) slowestWheelSpeed = frontRightWheelSpeed;
  if (rearLeftWheelSpeed < slowestWheelSpeed) slowestWheelSpeed = rearLeftWheelSpeed;
  if (rearRightWheelSpeed < slowestWheelSpeed) slowestWheelSpeed = rearRightWheelSpeed;


  uint8_t tens = slowestWheelSpeed / 10;   // Extract tens digit
  uint8_t units = (int)slowestWheelSpeed % 10;  // Extract units digit

  // Segment patterns for digits 0-9 on common-cathode displays
  const uint8_t segmentPatterns[] = {
    // abcdefg
    0b01111011,  // 0
    0b00110000,  // 1
    0b01101101,  // 2
    0b01110101,  // 3
    0b00110110,  // 4
    0b01010111,  // 5
    0b01011111,  // 6
    0b01110000,  // 7
    0b01111111,  // 8
    0b01110111   // 9
  };

  const uint8_t ledBarPatterns[] = {
    //Note: this function only controls the green LEDs
    0b00000000,  // 0
    0b00000000,  // 1
    0b00000000,  // 2
    0b00000000,  // 3
    0b10000000,  // 4
    0b11000000,  // 5
    0b11100000,  // 6
    0b11110000,  // 7
    0b11111000,  // 8
    0b11111100   // 9

  };

  // Update the number of LEDs based on the batteryPercentage
  updateBatteryLevel(batteryPercentage);

  // Calculate segment patterns for each digit
  uint8_t segmentsTens = segmentPatterns[tens];
  uint8_t segmentsUnits = segmentPatterns[units];
  uint8_t segmentsLedBar = ledBarPatterns[batteryLevel];


  // Display the speed and the green fuel LEDs using printDirect function
  uint8_t displayData[3] = { segmentsUnits, segmentsTens, segmentsLedBar };
  myLED.printDirect(displayData);

  //Update the other red and yellow fuel leds

  if (batteryLevel == 0) {
    digitalWrite(redLed, LOW);
    digitalWrite(yellowLed1, LOW);
    digitalWrite(yellowLed2, LOW);
  } else if (batteryLevel == 1) {
    digitalWrite(redLed, HIGH);
    digitalWrite(yellowLed1, LOW);
    digitalWrite(yellowLed2, LOW);
  } else if (batteryLevel == 2) {
    digitalWrite(redLed, HIGH);
    digitalWrite(yellowLed1, HIGH);
    digitalWrite(yellowLed2, LOW);
  } else {
    digitalWrite(redLed, HIGH);
    digitalWrite(yellowLed1, HIGH);
    digitalWrite(yellowLed2, HIGH);
  }
}


void updateBatteryLevel(int batteryPercentage) {
  int newBatteryLevel = 0;

  if (batteryPercentage < 20) newBatteryLevel = 1;
  else if (batteryPercentage < 30) newBatteryLevel = 2;
  else if (batteryPercentage < 40) newBatteryLevel = 3;
  else if (batteryPercentage < 50) newBatteryLevel = 4;
  else if (batteryPercentage < 60) newBatteryLevel = 5;
  else if (batteryPercentage < 70) newBatteryLevel = 6;
  else if (batteryPercentage < 80) newBatteryLevel = 7;
  else if (batteryPercentage < 90) newBatteryLevel = 8;
  else newBatteryLevel = 9;

  // Only update the battery level if it's sufficiently different from the last level
  if (newBatteryLevel != lastBatteryLevel) {
    // Update only if the battery level has changed by a significant amount
    lastBatteryLevel = newBatteryLevel;
  }
}