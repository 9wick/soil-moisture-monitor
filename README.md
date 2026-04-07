# Soil Moisture Monitor

M5Stack CoreInk + Qwiic土壌センサによる土壌水分モニター。

## ハードウェア構成

- M5Stack CoreInk (ESP32-PICO-D4)
- Qwiic土壌センサ（背面ピンに接続）

### 配線

| Qwiicケーブル色 | 信号 | CoreInk背面ピン |
|----------------|------|-----------------|
| 赤 | VCC (3.3V) | 3V3 |
| 黒 | GND | GND |
| 青 | SDA | G21 |
| 黄 | SCL | G22 |

## 電源管理

- `M5.Power.powerOff()` による完全shutdown（~2.6uA）
- 復帰: PWRボタン(G27) 2秒長押し
- USB接続中はshutdownできない（バッテリー動作のみ）
