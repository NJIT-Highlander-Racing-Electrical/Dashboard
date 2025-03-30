//RPM dial with digital readout
//Main graphic adapted from this project: https://www.youtube.com/watch?v=cBtsLxZ13hQ

#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);
#include "src/NotoSansBold36.h"
#include "src/NotoSansBold15.h"
#define AA_FONT_LARGE NotoSansBold36
#define AA_FONT_SMALL NotoSansBold15


// Parameters to control the RPM dial
int rpmAngle = 0;
const int cvtMinRPM = 0;
const int cvtMaxRPM = 4000;
const int angleMin = 0;
const int angleMax = 240;

//......................................colors
#define backColor 0xFFFF  //I really like 0x03EF, 0x7800,
#define gaugeColor 0x055D
#define dataColor 0x0311  //0x0311,
#define red 0xF00C
#define needleColor 0xF811

int cx = 120;
int cy = 110;
int r = 115;
int ir = 113;
int angle = 0;

float x[360];  //outer points of speed gauges
float y[360];
float px[360];  //inner point of speed gauges
float py[360];
float lx[360];  //text of speed gauges
float ly[360];
float nx[360];  //needle low of speed gauges
float ny[360];

double rad = 0.01745;
unsigned short color1;
unsigned short color2;
float sA;






void rpmGaugeSetup() {


  tft.init();

  tft.setRotation(0);
  tft.fillScreen(backColor);
  sprite.createSprite(235, 235);
  sprite.setSwapBytes(true);
  sprite.loadFont(AA_FONT_SMALL);
  sprite.setTextDatum(4);
  sprite.setTextColor(TFT_RED, backColor);

  int a = 120;
  for (int i = 0; i < 360; i++) {
    x[i] = ((r - 10) * cos(rad * a)) + cx;
    y[i] = ((r - 10) * sin(rad * a)) + cy;
    px[i] = ((r - 14) * cos(rad * a)) + cx;
    py[i] = ((r - 14) * sin(rad * a)) + cy;
    lx[i] = ((r - 24) * cos(rad * a)) + cx;
    ly[i] = ((r - 24) * sin(rad * a)) + cy;
    nx[i] = ((r - 45) * cos(rad * a)) + cx;
    ny[i] = ((r - 45) * sin(rad * a)) + cy;

    a++;
    if (a == 360)
      a = 0;
  }
}



void updateRPMGauge(int primaryRPM) {

  rpmAngle = map(primaryRPM, cvtMinRPM, cvtMaxRPM, angleMin, angleMax);

  if (rpmAngle < 0) rpmAngle = 0;

  sprite.fillSprite(backColor);

  //Outermost Arc
  sprite.drawSmoothArc(cx, cy, r, ir, 30, 330, gaugeColor, backColor);
  //Inner-Outer Arc
  sprite.drawSmoothArc(cx, cy, r - 5, r - 6, 30, 330, TFT_BLACK, backColor);
  //Red Region Arc
  sprite.drawSmoothArc(cx, cy, r - 9, r - 8, 270, 330, red, backColor);
  //Innermost Arc
  sprite.drawSmoothArc(cx, cy, r - 50, ir - 49, 10, 350, gaugeColor, backColor);

  // DRAW GAUGE
  // SET COLORS OF GAUGE LINE MARKINGS AND TEXT LABELS
  for (int i = 0; i < 26; i++) {
    if (i < 20) {
      color1 = gaugeColor;
      color2 = TFT_BLACK;
    } else {
      color1 = red;
      color2 = red;
    }

    // DRAW GAUGE LINE MARKINGS AND TEXT LABELS
    if (i % 2 == 0) {
      int labelValue = i * 1.8;
      sprite.drawWedgeLine(x[i * 12], y[i * 12], px[i * 12], py[i * 12], 2, 1, color1);
      sprite.setTextColor(color2, backColor);
      sprite.drawString(String(labelValue), lx[i * 12], ly[i * 12]);
    } else
      sprite.drawWedgeLine(x[i * 12], y[i * 12], px[i * 12], py[i * 12], 1, 1, color2);
  }

  color1 = gaugeColor;
  color2 = TFT_WHITE;

  // DRAW NEEDLE
  sA = rpmAngle * 1.2;
  sprite.drawWedgeLine(px[(int)sA], py[(int)sA], nx[(int)sA], ny[(int)sA], 4, 4, needleColor);


  // DRAW TEXT
  sprite.unloadFont();
  sprite.loadFont(AA_FONT_LARGE);
  sprite.setTextColor(TFT_BLACK, backColor);
  sprite.drawString(String((int)primaryRPM), cx, cy, 4);
  sprite.unloadFont();

  sprite.loadFont(AA_FONT_SMALL);
  sprite.setTextColor(0x03E0, backColor);
  sprite.drawString("x100", cx, 210);
  sprite.drawString("RPM", cx, 195);

  // //braakes are locked
  //sprite.fillRect(0,125,300,125,TFT_ORANGE); //lose traction
  // PUSH SPRITE TO SCREEN#0x FA51C
  sprite.pushSprite(0, 10);
  //fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color); red  is losing traction, orange is brake lock up
  //
}