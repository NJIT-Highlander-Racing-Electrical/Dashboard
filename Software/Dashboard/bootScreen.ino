//Boot screen for the vehicle that displays the Highlander Racing logo, Lightning McQueen "95", and a fuel LEDs animation
#include "Highlander_Racing_Image.h"

void bootScreen() {
  tft.fillScreen(TFT_BLACK);
  TJpgDec.drawJpg(0, 0, Highlander_Racing_Image, sizeof(Highlander_Racing_Image));

  for (int i = 0; i < 5; i++) {

    for (int i = 0; i < 10; i++) {
      fuelLevel = i;
      updateSevenSegments(95);
      delay(30);
    }
    for (int i = 9; i > 0; i--) {
      fuelLevel = i;
      updateSevenSegments(95);
      delay(30);
    }
  }

  fuelLevel = 0;
}




/*  Old fuel READY-SET-GO Animation

  fuelLevel = 1;
  updateSevenSegments(95);
  delay(750);

  fuelLevel = 3;
  updateSevenSegments(95);
  delay(750);

  fuelLevel = 9;
  updateSevenSegments(95);
  delay(1500);

*/