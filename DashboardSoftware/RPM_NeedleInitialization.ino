#include "RPM_Dial_Image.h"

void initializeNeedle() {

  //Boring copy+pasted stuff for drawing the dial
  TJpgDec.drawJpg(0, 0, RPM_Dial_Image, sizeof(RPM_Dial_Image));
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
}