void updateSevenSegments(uint8_t number) {
  uint8_t tens = number / 10;   // Extract tens digit
  uint8_t units = number % 10;  // Extract units digit

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

  const uint8_t fuelPatterns[] = {
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

  // Calculate segment patterns for each digit
  uint8_t segmentsTens = segmentPatterns[tens];
  uint8_t segmentsUnits = segmentPatterns[units];
  uint8_t segmentsFuel = fuelPatterns[fuelLevel];
/*
  Serial.print(segmentsTens);
  Serial.print(",");
  Serial.print(segmentsUnits);
  Serial.print(",");
  Serial.println(segmentsFuel);
*/

  // Display the speed and the green fuel LEDs using printDirect function
  uint8_t displayData[3] = { segmentsUnits, segmentsTens, segmentsFuel };
  myLED.printDirect(displayData);

  //Update the other red and yellow fuel leds
  if (fuelLevel == 0) {
    digitalWrite(redFuelLED, LOW);
    digitalWrite(yellowFuelLED1, LOW);
    digitalWrite(yellowFuelLED2, LOW);
  } else if (fuelLevel == 1) {
    digitalWrite(redFuelLED, HIGH);
    digitalWrite(yellowFuelLED1, LOW);
    digitalWrite(yellowFuelLED2, LOW);
  } else if (fuelLevel == 2) {
    digitalWrite(redFuelLED, HIGH);
    digitalWrite(yellowFuelLED1, HIGH);
    digitalWrite(yellowFuelLED2, LOW);
  } else {
    digitalWrite(redFuelLED, HIGH);
    digitalWrite(yellowFuelLED1, HIGH);
    digitalWrite(yellowFuelLED2, HIGH);
  }
}