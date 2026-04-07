// 2個目の土壌センサのI2Cアドレスを0x37→0x38に変更するワンショットスケッチ
// 変更対象のセンサのみ接続した状態で実行すること
#include <Wire.h>
#include <SparkFun_CY8CMBR3.h>

SfeCY8CMBR3ArdI2C sensor;

static const uint8_t OLD_ADDR = 0x37;
static const uint8_t NEW_ADDR = 0x38;

void setup(void) {
  Serial.begin(115200);
  delay(2000);
  Serial.println("=== Address Change ===");

  Wire1.begin(21, 22);

  // まずスキャン
  Serial.println("I2C scan:");
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire1.beginTransmission(addr);
    if (Wire1.endTransmission() == 0)
      Serial.printf("  Found: 0x%02X\n", addr);
  }

  // 0x37と0x38の両方を試す
  uint8_t found = 0;
  for (uint8_t addr : {OLD_ADDR, NEW_ADDR}) {
    Serial.printf("Trying 0x%02X...\n", addr);
    if (sensor.begin(addr, Wire1)) {
      Serial.printf("Found at 0x%02X!\n", addr);
      found = addr;
      break;
    }
    Serial.printf("Not at 0x%02X\n", addr);
  }

  if (found == 0) {
    Serial.println("Sensor not found at either address.");
  } else if (found == NEW_ADDR) {
    Serial.println("Already at new address. Done.");
  } else {
    if (sensor.setI2CAddress(NEW_ADDR)) {
      Serial.printf("Address changed: 0x%02X -> 0x%02X\n", OLD_ADDR, NEW_ADDR);
    } else {
      Serial.println("Address change FAILED.");
    }
  }
}

void loop(void) {
  delay(3000);
  Serial.println("--- scan ---");
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire1.beginTransmission(addr);
    if (Wire1.endTransmission() == 0)
      Serial.printf("  Found: 0x%02X\n", addr);
  }
}
