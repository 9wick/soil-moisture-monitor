#include <M5Unified.h>
#include "faceIcon.h"
#include <Wire.h>
#include <SparkFun_CY8CMBR3.h>
#include <esp_wifi.h>
#include <BLEDevice.h>
#include <BLEAdvertising.h>

static const uint8_t SENSOR1_ADDR = 0x37;
static const uint8_t SENSOR2_ADDR = 0x38;
static const int SLEEP_SECONDS = 60;

SfeCY8CMBR3ArdI2C sensor1;
SfeCY8CMBR3ArdI2C sensor2;

// センサのpF値→%変換用（環境に合わせて調整）
static const uint8_t MOISTURE_DRY_PF  = 10;
static const uint8_t MOISTURE_WET_PF  = 60;

static bool sensor1Ok = false;
static bool sensor2Ok = false;

bool initSensor(SfeCY8CMBR3ArdI2C &sensor, uint8_t addr) {
  if (!sensor.begin(addr, Wire1)) return false;
  if (!sensor.defaultMoistureSensorInit()) return false;
  return true;
}

// baseline確立までポーリング（pFが安定する条件）
bool waitForBaseline(SfeCY8CMBR3ArdI2C &sensor, unsigned long timeoutMs) {
  unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    if (sensor.readBaselineCount() > 0) return true;
    delay(20);
  }
  return false;
}

void readSensors(uint8_t &pf1, uint8_t &pf2) {
  Serial.begin(115200);
  Wire1.begin(21, 22);
  Wire1.setTimeOut(500);

  sensor1Ok = initSensor(sensor1, SENSOR1_ADDR);
  if (sensor1Ok) sensor1Ok = waitForBaseline(sensor1, 1000);
  pf1 = sensor1Ok ? sensor1.readCapacitancePF() : 0;

  sensor2Ok = initSensor(sensor2, SENSOR2_ADDR);
  if (sensor2Ok) sensor2Ok = waitForBaseline(sensor2, 1000);
  pf2 = sensor2Ok ? sensor2.readCapacitancePF() : 0;
}

int moistureToPercent(uint8_t pf) {
  if (pf <= MOISTURE_DRY_PF) return 0;
  if (pf >= MOISTURE_WET_PF) return 100;
  return (int)((pf - MOISTURE_DRY_PF) * 100L / (MOISTURE_WET_PF - MOISTURE_DRY_PF));
}

// 水分レベルに応じたアイコン (0=枯れ, 1=しおれ, 2=元気, 3=潤沢)
int moistureLevel(int pct) {
  if (pct < 20) return 0;
  if (pct < 45) return 1;
  if (pct < 75) return 2;
  return 3;
}

static bool gDebugMode = false;

// ──── Display helpers ────

void drawDottedHLine(int x, int y, int len, int gap) {
  for (int i = 0; i < len; i += gap) {
    M5.Display.drawPixel(x + i, y, TFT_BLACK);
  }
}

// ──── Pot (立体感のある鉢) ────

void drawPot(int cx, int cy) {
  // cy = 土の表面ライン
  M5.Display.fillRect(cx - 18, cy - 2, 37, 3, TFT_BLACK);
  M5.Display.drawPixel(cx - 10, cy - 1, TFT_WHITE);
  M5.Display.drawPixel(cx + 7, cy - 1, TFT_WHITE);
  M5.Display.drawPixel(cx - 2, cy, TFT_WHITE);

  M5.Display.fillRect(cx - 20, cy + 1, 41, 3, TFT_BLACK);

  for (int row = 0; row < 14; row++) {
    int hw = 18 - row * 5 / 14;
    M5.Display.drawFastHLine(cx - hw, cy + 4 + row, hw * 2 + 1, TFT_BLACK);
  }

  for (int row = 2; row < 12; row++) {
    M5.Display.drawPixel(cx - 15 + row / 4, cy + 5 + row, TFT_WHITE);
  }

  M5.Display.fillRect(cx - 14, cy + 18, 29, 2, TFT_BLACK);
}

// ──── Plant illustrations ────

void drawFaceDead(int cx, int cy) {
  int bx = cx - FACE_DEAD_WIDTH / 2;
  int by = cy - FACE_DEAD_HEIGHT;
  M5.Display.drawBitmap(bx, by, face_dead_bitmap, FACE_DEAD_WIDTH, FACE_DEAD_HEIGHT, TFT_BLACK);
}

void drawFaceDry(int cx, int cy) {
  int bx = cx - FACE_DRY_WIDTH / 2;
  int by = cy - FACE_DRY_HEIGHT;
  M5.Display.drawBitmap(bx, by, face_dry_bitmap, FACE_DRY_WIDTH, FACE_DRY_HEIGHT, TFT_BLACK);
}

void drawFaceGood(int cx, int cy) {
  int bx = cx - FACE_GOOD_WIDTH / 2;
  int by = cy - FACE_GOOD_HEIGHT;
  M5.Display.drawBitmap(bx, by, face_good_bitmap, FACE_GOOD_WIDTH, FACE_GOOD_HEIGHT, TFT_BLACK);
}

void drawFaceWet(int cx, int cy) {
  int bx = cx - FACE_WET_WIDTH / 2;
  int by = cy - FACE_WET_HEIGHT;
  M5.Display.drawBitmap(bx, by, face_wet_bitmap, FACE_WET_WIDTH, FACE_WET_HEIGHT, TFT_BLACK);
}

void drawPlantWilting(int cx, int cy) {
  drawPot(cx, cy);

  M5.Display.drawLine(cx, cy - 2, cx, cy - 18, TFT_BLACK);
  M5.Display.drawLine(cx + 1, cy - 2, cx + 1, cy - 18, TFT_BLACK);
  M5.Display.drawLine(cx, cy - 18, cx + 2, cy - 35, TFT_BLACK);
  M5.Display.drawLine(cx + 1, cy - 18, cx + 3, cy - 35, TFT_BLACK);

  M5.Display.fillTriangle(cx, cy - 26, cx - 12, cy - 20, cx - 4, cy - 28, TFT_BLACK);
  M5.Display.fillTriangle(cx + 2, cy - 20, cx + 14, cy - 14, cx + 8, cy - 22, TFT_BLACK);

  M5.Display.drawLine(cx + 2, cy - 35, cx + 6, cy - 35, TFT_BLACK);
  M5.Display.drawLine(cx + 6, cy - 35, cx + 10, cy - 33, TFT_BLACK);
  M5.Display.fillCircle(cx + 10, cy - 33, 2, TFT_BLACK);
}

void drawPlantHealthy(int cx, int cy) {
  drawPot(cx, cy);

  M5.Display.fillRect(cx - 1, cy - 30, 3, 28, TFT_BLACK);

  M5.Display.fillCircle(cx, cy - 38, 12, TFT_BLACK);

  M5.Display.fillTriangle(cx - 12, cy - 38, cx - 20, cy - 42, cx - 14, cy - 44, TFT_BLACK);
  M5.Display.fillTriangle(cx + 12, cy - 38, cx + 20, cy - 42, cx + 14, cy - 44, TFT_BLACK);
  M5.Display.fillTriangle(cx - 10, cy - 30, cx - 18, cy - 28, cx - 12, cy - 34, TFT_BLACK);
  M5.Display.fillTriangle(cx + 10, cy - 30, cx + 18, cy - 28, cx + 12, cy - 34, TFT_BLACK);

  M5.Display.fillTriangle(cx - 1, cy - 18, cx - 10, cy - 14, cx - 4, cy - 20, TFT_BLACK);
  M5.Display.fillTriangle(cx + 2, cy - 12, cx + 11, cy - 8, cx + 5, cy - 14, TFT_BLACK);

  M5.Display.fillCircle(cx, cy - 50, 3, TFT_BLACK);
  M5.Display.fillCircle(cx, cy - 50, 1, TFT_WHITE);
  M5.Display.drawLine(cx, cy - 46, cx, cy - 48, TFT_BLACK);
}

void drawPlantWet(int cx, int cy) {
  drawPlantHealthy(cx, cy);

  M5.Display.fillCircle(cx - 8, cy - 34, 4, TFT_BLACK);
  M5.Display.fillCircle(cx + 8, cy - 34, 4, TFT_BLACK);

  M5.Display.fillCircle(cx - 24, cy - 12, 2, TFT_BLACK);
  M5.Display.fillTriangle(cx - 26, cy - 12, cx - 22, cy - 12, cx - 24, cy - 18, TFT_BLACK);

  M5.Display.fillCircle(cx + 24, cy - 20, 2, TFT_BLACK);
  M5.Display.fillTriangle(cx + 22, cy - 20, cx + 26, cy - 20, cx + 24, cy - 26, TFT_BLACK);

  M5.Display.fillCircle(cx + 14, cy - 6, 1, TFT_BLACK);
  M5.Display.fillTriangle(cx + 13, cy - 6, cx + 15, cy - 6, cx + 14, cy - 10, TFT_BLACK);
}

void drawPlantIcon(int cx, int cy, int level) {
  switch (level) {
    case 0: drawFaceDead(cx, cy); break;
    case 1: drawFaceDry(cx, cy); break;
    case 2: drawFaceGood(cx, cy); break;
    case 3: drawFaceWet(cx, cy); break;
  }
}

// ──── UI components ────

void drawMoistureBar(int x, int y, int w, int h, int pct) {
  M5.Display.drawRect(x, y, w, h, TFT_BLACK);

  int fillW = (w - 2) * pct / 100;
  if (fillW > 0) {
    M5.Display.fillRect(x + 1, y + 1, fillW, h - 2, TFT_BLACK);
  }

  for (int t = 1; t <= 3; t++) {
    int tx = x + w * t / 4;
    M5.Display.drawPixel(tx, y, TFT_WHITE);
    M5.Display.drawPixel(tx, y + h - 1, TFT_WHITE);
  }
}

void drawHeader() {
  M5.Display.setTextSize(1);

  const char *title = gDebugMode ? "[debug] SOIL MOISTURE" : "SOIL MOISTURE";
  int titleW = strlen(title) * 6;
  int titleX = (200 - titleW) / 2;

  M5.Display.setCursor(titleX, 2);
  M5.Display.print(title);

  M5.Display.drawFastHLine(4, 6, titleX - 8, TFT_BLACK);
  M5.Display.drawFastHLine(titleX + titleW + 4, 6, 200 - titleX - titleW - 8, TFT_BLACK);

  M5.Display.fillCircle(2, 6, 1, TFT_BLACK);
  M5.Display.fillCircle(197, 6, 1, TFT_BLACK);

  M5.Display.drawFastHLine(0, 12, 200, TFT_BLACK);
}

void drawSensorPanel(int x, int y, int w, const char *label, uint8_t pf, bool ok) {
  int h = 172;

  M5.Display.drawRoundRect(x, y, w, h, 3, TFT_BLACK);
  M5.Display.drawFastHLine(x + 1, y + 1, w - 2, TFT_BLACK);

  M5.Display.setTextSize(1.5);
  M5.Display.setCursor(x + 6, y + 4);
  M5.Display.print(label);

  M5.Display.drawFastHLine(x + 4, y + 17, w - 8, TFT_BLACK);

  if (!ok) {
    int iconCx = x + w / 2;
    M5.Display.drawLine(iconCx - 10, y + 70, iconCx + 10, y + 90, TFT_BLACK);
    M5.Display.drawLine(iconCx + 10, y + 70, iconCx - 10, y + 90, TFT_BLACK);

    M5.Display.setTextSize(1.5);
    M5.Display.setCursor(x + (w - 27) / 2, y + 96);
    M5.Display.print("N/A");
    return;
  }

  int pct = moistureToPercent(pf);
  int level = moistureLevel(pct);

  drawPlantIcon(x + w / 2, y + 108, level);

  int barW = 56;
  int barX = x + 5;
  int barY = y + 134;
  drawMoistureBar(barX, barY, barW, 8, pct);

  M5.Display.setTextSize(1);
  M5.Display.setCursor(barX + barW + 3, barY);
  M5.Display.printf("%d%%", pct);

  static const char *statusLabels[] = {"DEAD", "DRY", "GOOD", "WET"};
  M5.Display.setCursor(x + 5, y + 148);
  M5.Display.printf("%dpF", pf);
  int sLen = strlen(statusLabels[level]);
  M5.Display.setCursor(x + w - sLen * 6 - 4, y + 148);
  M5.Display.print(statusLabels[level]);
}

void drawBatteryStatus(int x, int y, int pct, int mv) {
  // Battery icon (20x8, inner 16x4 → 10%刻みで視認可)
  M5.Display.drawRect(x, y, 20, 8, TFT_BLACK);
  M5.Display.fillRect(x + 20, y + 2, 2, 4, TFT_BLACK);
  int fillW = 16 * pct / 100;
  if (fillW > 0) {
    M5.Display.fillRect(x + 2, y + 2, fillW, 4, TFT_BLACK);
  }

  M5.Display.setTextSize(1);
  M5.Display.setCursor(x + 25, y);
  M5.Display.printf("%d%% %dmV", pct, mv);
}

// ──── BLE Advertising ────

void advertiseSensorData(uint8_t pf1, uint8_t pf2, int battPct, uint16_t battMv) {
  BLEDevice::init("");

  auto dt = M5.Rtc.getDateTime();

  char mfr[14];
  mfr[0] = 0xFF; mfr[1] = 0xFF;
  mfr[2] = 'S';  mfr[3] = 'M';
  mfr[4] = (char)(dt.date.year - 2000);
  mfr[5] = (char)dt.date.month;
  mfr[6] = (char)dt.date.date;
  mfr[7] = (char)dt.time.hours;
  mfr[8] = (char)dt.time.minutes;
  mfr[9] = (char)battPct;
  mfr[10] = battMv & 0xFF;
  mfr[11] = (battMv >> 8) & 0xFF;
  mfr[12] = (sensor1Ok ? 0x80 : 0x00) | (pf1 & 0x7F);
  mfr[13] = (sensor2Ok ? 0x80 : 0x00) | (pf2 & 0x7F);

  BLEAdvertisementData advData;
  advData.setFlags(0x06);
  advData.setManufacturerData(String(mfr, sizeof(mfr)));

  BLEAdvertising *pAdv = BLEDevice::getAdvertising();
  pAdv->setAdvertisementData(advData);
  pAdv->setAdvertisementType(ADV_TYPE_NONCONN_IND);
  pAdv->setMinInterval(0xA0);
  pAdv->setMaxInterval(0x0140);
  pAdv->start();

  delay(1500);

  pAdv->stop();
  BLEDevice::deinit(false);
}

static const uint8_t LEVEL_TO_PF[] = {15, 25, 40, 55};
static int gDebugLevel1 = 0;
static int gDebugLevel2 = 0;
static bool gPrev37 = true, gPrev39 = true;

void updateDisplay(uint8_t pf1, uint8_t pf2, int battPct, int battMv) {
  M5.Display.startWrite();
  M5.Display.setEpdMode(epd_mode_t::epd_quality);
  M5.Display.setRotation(1);
  M5.Display.clear(TFT_WHITE);
  M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);

  drawHeader();

  drawSensorPanel(2, 15, 96, "#1", pf1, sensor1Ok);
  drawSensorPanel(102, 15, 96, "#2", pf2, sensor2Ok);

  drawBatteryStatus(4, 190, battPct, battMv);

  auto dt = M5.Rtc.getDateTime();
  char timeBuf[18];
  sprintf(timeBuf, "%d/%d/%d %02d:%02d",
    dt.date.year, dt.date.month, dt.date.date,
    dt.time.hours, dt.time.minutes);
  int timeW = strlen(timeBuf) * 6;
  M5.Display.setTextSize(1);
  M5.Display.setCursor(198 - timeW, 190);
  M5.Display.print(timeBuf);

  M5.Display.endWrite();
  M5.Display.waitDisplay();
}

void setup(void) {
  auto cfg = M5.config();
  cfg.clear_display = false;
  cfg.output_power = false;
  cfg.internal_imu = false;
  cfg.internal_rtc = true;
  cfg.internal_mic = false;
  cfg.internal_spk = false;
  cfg.external_imu = false;
  cfg.external_rtc = false;
  cfg.led_brightness = 0;
  M5.begin(cfg);
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);

  esp_wifi_stop();

  // G38を押しながら起動 → デバッグモード
  pinMode(37, INPUT_PULLUP);
  pinMode(38, INPUT_PULLUP);
  pinMode(39, INPUT_PULLUP);
  delay(50);
  bool enterDebug = (digitalRead(38) == LOW);

  if (enterDebug) {
    gDebugMode = true;
    sensor1Ok = true;
    sensor2Ok = true;
    int bp = M5.Power.getBatteryLevel();
    int bv = M5.Power.getBatteryVoltage();
    updateDisplay(LEVEL_TO_PF[gDebugLevel1], LEVEL_TO_PF[gDebugLevel2], bp, bv);
  } else {
    uint8_t pf1, pf2;
    readSensors(pf1, pf2);
    int battPct = M5.Power.getBatteryLevel();
    uint16_t battMv = M5.Power.getBatteryVoltage();
    updateDisplay(pf1, pf2, battPct, battMv);
    M5.Display.sleep();
    M5.Power.setExtOutput(false);
    advertiseSensorData(pf1, pf2, battPct, battMv);
    M5.Power.timerSleep(SLEEP_SECONDS);
  }
}

void loop(void) {
  bool cur37 = digitalRead(37);
  bool cur39 = digitalRead(39);
  bool changed = false;

  if (gPrev37 && !cur37) {
    gDebugLevel1 = (gDebugLevel1 + 1) % 4;
    changed = true;
  }
  if (gPrev39 && !cur39) {
    gDebugLevel2 = (gDebugLevel2 + 1) % 4;
    changed = true;
  }

  gPrev37 = cur37;
  gPrev39 = cur39;

  if (changed) {
    int bp = M5.Power.getBatteryLevel();
    int bv = M5.Power.getBatteryVoltage();
    updateDisplay(LEVEL_TO_PF[gDebugLevel1], LEVEL_TO_PF[gDebugLevel2], bp, bv);
  }

  delay(50);
}
