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

// Boring needle functions that were copy and pasted
void plotNeedle(int16_t angle, uint16_t ms_delay) {
  digitalWrite(TFT_CS_L, HIGH); // Note: These two lines are here (and at the end of the plot function) to ensure that SPI chip select pins are properly set. Ran into some issues where pins weren't going back HIGH
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