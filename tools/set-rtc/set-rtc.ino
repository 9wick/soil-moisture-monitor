#include <M5Unified.h>

// __DATE__ = "Mar 10 2026", __TIME__ = "12:20:00"
void parseCompileTime(m5::rtc_datetime_t &dt) {
  const char *date = __DATE__;  // "Mmm DD YYYY"
  const char *time = __TIME__;  // "HH:MM:SS"

  const char *months[] = {
    "Jan","Feb","Mar","Apr","May","Jun",
    "Jul","Aug","Sep","Oct","Nov","Dec"
  };

  char mon[4];
  int day, year;
  sscanf(date, "%3s %d %d", mon, &day, &year);

  dt.date.month = 1;
  for (int i = 0; i < 12; i++) {
    if (strcmp(mon, months[i]) == 0) {
      dt.date.month = i + 1;
      break;
    }
  }
  dt.date.date = day;
  dt.date.year = year;

  int h, m, s;
  sscanf(time, "%d:%d:%d", &h, &m, &s);
  dt.time.hours = h;
  dt.time.minutes = m;
  dt.time.seconds = s;
}

void setup(void) {
  auto cfg = M5.config();
  cfg.internal_rtc = true;
  cfg.clear_display = false;
  cfg.output_power = false;
  cfg.internal_imu = false;
  cfg.internal_mic = false;
  cfg.internal_spk = false;
  cfg.external_imu = false;
  cfg.external_rtc = false;
  cfg.led_brightness = 0;
  M5.begin(cfg);

  m5::rtc_datetime_t dt;
  parseCompileTime(dt);
  M5.Rtc.setDateTime(dt);

  // 設定した時刻を表示
  M5.Display.startWrite();
  M5.Display.setEpdMode(epd_mode_t::epd_quality);
  M5.Display.setRotation(1);
  M5.Display.clear(TFT_WHITE);
  M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);

  M5.Display.setTextSize(1.5);
  M5.Display.setCursor(10, 40);
  M5.Display.print("RTC Set OK");

  M5.Display.setTextSize(1);
  M5.Display.setCursor(10, 70);
  M5.Display.printf("%04d/%02d/%02d", dt.date.year, dt.date.month, dt.date.date);
  M5.Display.setCursor(10, 85);
  M5.Display.printf("%02d:%02d:%02d", dt.time.hours, dt.time.minutes, dt.time.seconds);

  M5.Display.setCursor(10, 110);
  M5.Display.print("Upload main sketch");
  M5.Display.setCursor(10, 125);
  M5.Display.print("to resume normal use.");

  M5.Display.endWrite();
  M5.Display.waitDisplay();
  M5.Display.sleep();
}

void loop(void) {}
