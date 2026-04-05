#include <M5Unified.h>

void setup(void) {
  M5.begin();

  M5.Display.setEpdMode(epd_mode_t::epd_quality);
  M5.Display.clear(TFT_WHITE);
  M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
  M5.Display.setTextSize(2);
  M5.Display.setCursor(20, 80);
  M5.Display.println("Hello World!");
}

void loop(void) {
  M5.update();
  M5.delay(100);
}
