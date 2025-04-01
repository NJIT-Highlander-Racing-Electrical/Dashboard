//Boot screen for the vehicle that displays the Highlander Racing logo, Lightning McQueen "95", and a fuel LEDs animation
#include "src/Highlander_Racing_Image.h"

void bootScreen() {
  tft.fillScreen(TFT_BLACK);
  TJpgDec.drawJpg(0, 0, Highlander_Racing_Image, sizeof(Highlander_Racing_Image));

  for (int i = 0; i < 5; i++) {

    for (int i = 1; i < 10; i++) {
      batteryLevel = i;

      gpsVelocity = 95;
      updateLedDisplays();
      delay(30);
    }
    for (int i = 9; i > 0; i--) {
      batteryLevel = i;
      gpsVelocity = 95;
      updateLedDisplays();
      delay(30);
    }
  }

  gpsVelocity = 0;

  batteryLevel = 0;
}