#include <M5Unified.h>
#include <esp_wifi.h>
#include <esp_bt.h>
#include "driver/rtc_io.h"

static const uint64_t SLEEP_DURATION_US = 60 * 1000000ULL;
static const gpio_num_t BUTTON_PIN = GPIO_NUM_38;

void updateDisplay(void) {
  M5.Display.startWrite();
  M5.Display.setEpdMode(epd_mode_t::epd_quality);
  M5.Display.clear(TFT_WHITE);
  M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
  M5.Display.setTextSize(2);

  int battery = M5.Power.getBatteryLevel();
  int voltage = M5.Power.getBatteryVoltage();

  M5.Display.setCursor(20, 40);
  M5.Display.printf("Battery: %d%%\n", battery);
  M5.Display.setCursor(20, 80);
  M5.Display.printf("Voltage: %dmV\n", voltage);

  auto charging = M5.Power.isCharging();
  M5.Display.setCursor(20, 120);
  // debug: 0=charge_unknown, 1=is_charging, 2=is_discharging
  M5.Display.printf("State: %d\n", (int)charging);
  M5.Display.setCursor(20, 150);
  M5.Display.printf("Current: %dmA\n", M5.Power.getBatteryCurrent());
  M5.Display.endWrite();
  M5.Display.waitDisplay();
}

static const gpio_num_t POWER_HOLD_PIN = GPIO_NUM_12;

void enterDeepSleep(void) {
  M5.Power.setExtOutput(false);
  digitalWrite(10, HIGH);

  // GPIO12をRTC GPIOとして制御（bokunimo方式）
  rtc_gpio_init(POWER_HOLD_PIN);
  rtc_gpio_set_direction(POWER_HOLD_PIN, RTC_GPIO_MODE_OUTPUT_ONLY);
  rtc_gpio_set_level(POWER_HOLD_PIN, 1);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);

  esp_sleep_enable_timer_wakeup(SLEEP_DURATION_US);
  esp_sleep_enable_ext0_wakeup(BUTTON_PIN, 0);
  esp_deep_sleep(SLEEP_DURATION_US);
}

void setup(void) {
  auto cfg = M5.config();
  cfg.clear_display = false;
  cfg.output_power = false;
  cfg.internal_imu = false;
  cfg.internal_rtc = false;
  cfg.internal_mic = false;
  cfg.internal_spk = false;
  cfg.external_imu = false;
  cfg.external_rtc = false;
  cfg.led_brightness = 0;
  M5.begin(cfg);
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);

  esp_wifi_stop();
  esp_bt_controller_disable();

  updateDisplay();
  enterDeepSleep();
}

void loop(void) {
}
