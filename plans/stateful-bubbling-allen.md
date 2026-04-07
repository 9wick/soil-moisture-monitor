# Plan: baselineポーリングによるpF安定化

## Context

CY8CMBR3102は毎回電源OFFからの起動でSmartSenseが再調整するため、起動直後のpF値が不安定（baseline=0の間ゴミ値が出る）。
実測で、init直後pF=64→57→8と変動し、~400msでbaselineが確立して安定することを確認済み。

電源OFF方式が最も省電力（スリープ中0uA）なので、この方式を維持しつつ、固定delayではなくbaselineレジスタのポーリングで最短の待ち時間を実現する。

## 変更対象

- `soil-moisture-monitor.ino` の `readSensors()` 関数のみ

## 変更内容

### `readSensors()` の修正

現状:
```
init → 即readCapacitancePF()
```

修正後:
```
init → baselineが非ゼロになるまでポーリング(timeout 1000ms) → readCapacitancePF()
```

具体的には:

```cpp
bool waitForBaseline(SfeCY8CMBR3ArdI2C &sensor, unsigned long timeoutMs) {
  unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    if (sensor.readBaselineCount() > 0) return true;
    delay(20);
  }
  return false;
}

void readSensors(uint8_t &pf1, uint8_t &pf2) {
  Wire1.begin(21, 22);

  sensor1Ok = initSensor(sensor1, SENSOR1_ADDR);
  if (sensor1Ok) waitForBaseline(sensor1, 1000);
  pf1 = sensor1Ok ? sensor1.readCapacitancePF() : 0;

  sensor2Ok = initSensor(sensor2, SENSOR2_ADDR);
  if (sensor2Ok) waitForBaseline(sensor2, 1000);
  pf2 = sensor2Ok ? sensor2.readCapacitancePF() : 0;
}
```

### 変更しないもの

- `MOISTURE_DRY_PF` / `MOISTURE_WET_PF` のキャリブレーション値 — 安定化後に実測で再調整が必要なため今は触らない
- E-ink表示ロジック
- Deep Sleepの仕組み

## 検証方法

1. `tools/measure-settling/` で修正前後のpF安定性を比較
2. 乾いた土 / 湿った土 でpF値の差を確認（以前の12/17pFより安定した値が出るはず）
3. バッテリー影響: 最悪ケースでも追加400ms × 44uA = 約18uA・s/サイクル（60秒サイクルで平均+0.3uA）
