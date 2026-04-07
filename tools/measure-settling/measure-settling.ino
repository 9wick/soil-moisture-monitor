// リセット後のセンサ安定化タイミングを計測するツール
// Serial Monitor (115200baud) で結果を確認する
#include <M5Unified.h>
#include <Wire.h>
#include <SparkFun_CY8CMBR3.h>

static const uint8_t SENSOR1_ADDR = 0x37;
static const uint8_t SENSOR2_ADDR = 0x38;
static const int SAMPLE_INTERVAL_MS = 20;
static const int SAMPLE_DURATION_MS = 3000;

SfeCY8CMBR3ArdI2C sensor1;
SfeCY8CMBR3ArdI2C sensor2;

struct Sample {
  unsigned long ms;
  uint16_t raw;
  uint16_t baseline;
  uint8_t pf;
  bool valid;
};

static const int MAX_SAMPLES = SAMPLE_DURATION_MS / SAMPLE_INTERVAL_MS + 1;
Sample samples1[MAX_SAMPLES];
Sample samples2[MAX_SAMPLES];

void measureSensor(SfeCY8CMBR3ArdI2C &sensor, uint8_t addr,
                   const char *label, Sample *samples, int &count) {
  Serial.printf("\n=== %s (0x%02X) ===\n", label, addr);

  unsigned long t0 = millis();
  Serial.printf("begin() ...\n");
  if (!sensor.begin(addr, Wire1)) {
    Serial.printf("%s: begin() failed\n", label);
    count = 0;
    return;
  }
  Serial.printf("begin() OK (%lums)\n", millis() - t0);

  Serial.printf("defaultMoistureSensorInit() ...\n");
  unsigned long tInit = millis();
  if (!sensor.defaultMoistureSensorInit()) {
    Serial.printf("%s: init failed\n", label);
    count = 0;
    return;
  }
  unsigned long tReady = millis();
  Serial.printf("init done (%lums)\n", tReady - tInit);

  // リセット直後から計測開始
  Serial.printf("\nms_since_init, raw, baseline, pF\n");

  count = 0;
  unsigned long start = millis();
  while (millis() - start < SAMPLE_DURATION_MS && count < MAX_SAMPLES) {
    unsigned long now = millis();
    Sample &s = samples[count];
    s.ms = now - start;
    s.raw = sensor.readRawCount();
    s.baseline = sensor.readBaselineCount();
    s.pf = sensor.readCapacitancePF();
    s.valid = (s.raw > 0);

    Serial.printf("%4lu, %5u, %5u, %3u\n", s.ms, s.raw, s.baseline, s.pf);
    count++;

    unsigned long elapsed = millis() - now;
    if (elapsed < SAMPLE_INTERVAL_MS) {
      delay(SAMPLE_INTERVAL_MS - elapsed);
    }
  }
}

// raw countの変動が収束したタイミングを検出
int findSettledIndex(Sample *samples, int count, uint16_t threshold) {
  if (count < 5) return -1;

  for (int i = 4; i < count; i++) {
    bool stable = true;
    uint16_t ref = samples[i].raw;
    for (int j = 1; j <= 4; j++) {
      uint16_t diff = (samples[i - j].raw > ref)
        ? samples[i - j].raw - ref
        : ref - samples[i - j].raw;
      if (diff > threshold) {
        stable = false;
        break;
      }
    }
    if (stable) return i - 4;
  }
  return -1;
}

void printSummary(const char *label, Sample *samples, int count) {
  if (count == 0) {
    Serial.printf("\n%s: no data\n", label);
    return;
  }

  // 閾値: raw countの0.5%
  uint16_t lastRaw = samples[count - 1].raw;
  uint16_t threshold = lastRaw / 200;
  if (threshold < 10) threshold = 10;

  int idx = findSettledIndex(samples, count, threshold);

  Serial.printf("\n--- %s summary ---\n", label);
  Serial.printf("Final raw: %u, baseline: %u, pF: %u\n",
    lastRaw, samples[count - 1].baseline, samples[count - 1].pf);
  Serial.printf("Threshold: +/-%u counts (0.5%%)\n", threshold);

  if (idx >= 0) {
    Serial.printf("Settled at: %lums (sample #%d)\n", samples[idx].ms, idx);
  } else {
    Serial.printf("Not settled within %dms\n", SAMPLE_DURATION_MS);
  }
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

  Serial.begin(115200);
  delay(2000);
  Serial.println("=== Settling Time Measurement ===");

  Wire1.begin(21, 22);

  int count1 = 0, count2 = 0;
  measureSensor(sensor1, SENSOR1_ADDR, "Sensor1", samples1, count1);
  measureSensor(sensor2, SENSOR2_ADDR, "Sensor2", samples2, count2);

  printSummary("Sensor1", samples1, count1);
  printSummary("Sensor2", samples2, count2);

  Serial.println("\n=== Done ===");

  // E-ink に結果サマリを表示
  M5.Display.startWrite();
  M5.Display.setEpdMode(epd_mode_t::epd_quality);
  M5.Display.setRotation(1);
  M5.Display.clear(TFT_WHITE);
  M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
  M5.Display.setTextSize(1);

  M5.Display.setCursor(10, 5);
  M5.Display.print("SETTLING TIME TEST");
  M5.Display.drawFastHLine(0, 16, 200, TFT_BLACK);

  for (int si = 0; si < 2; si++) {
    Sample *samples = (si == 0) ? samples1 : samples2;
    int count = (si == 0) ? count1 : count2;
    int y = 22 + si * 90;

    M5.Display.setCursor(10, y);
    M5.Display.printf("Sensor%d:", si + 1);

    if (count == 0) {
      M5.Display.setCursor(20, y + 14);
      M5.Display.print("N/A");
      continue;
    }

    uint16_t lastRaw = samples[count - 1].raw;
    uint16_t threshold = lastRaw / 200;
    if (threshold < 10) threshold = 10;
    int idx = findSettledIndex(samples, count, threshold);

    M5.Display.setCursor(20, y + 14);
    M5.Display.printf("raw:%u bl:%u pF:%u",
      lastRaw, samples[count - 1].baseline, samples[count - 1].pf);

    M5.Display.setCursor(20, y + 28);
    if (idx >= 0) {
      M5.Display.printf("Settled: %lums", samples[idx].ms);
    } else {
      M5.Display.printf("Not settled in %dms", SAMPLE_DURATION_MS);
    }

    M5.Display.setCursor(20, y + 42);
    M5.Display.printf("1st:%u last:%u", samples[0].raw, lastRaw);

    M5.Display.setCursor(20, y + 56);
    M5.Display.printf("(+/-%u counts)", threshold);
  }

  M5.Display.setCursor(10, 190);
  M5.Display.print("See Serial for full log");

  M5.Display.endWrite();
  M5.Display.waitDisplay();
  M5.Display.sleep();
}

void loop(void) {}
