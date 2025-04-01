//A function that accepts an integer number (less than 99MPH) and configures each individual segment
//Once the proper segments are set, the displayData is written
//This also updated the battery LEDs using the global battery level variable

void updateLedDisplays() {

  uint8_t tens = gpsVelocity / 10;   // Extract tens digit
  uint8_t units = gpsVelocity % 10;  // Extract units digit

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
  if (batteryPercentage < 20) batteryLevel = 1;
  else if (batteryPercentage < 30) batteryLevel = 2;
  else if (batteryPercentage < 40) batteryLevel = 3;
  else if (batteryPersentage < 50) batteryLevel = 4;
  else if (batteryPercentage < 60) batteryLevel = 5;
  else if (batteryPercentage < 70) batteryLevel = 6;
  else if (batteryPercentage < 80) batteryLevel = 7;
  else if (batteryPercentage < 90) batteryLevel = 8;
  else batteryLevel = 9;

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