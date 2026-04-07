# BLE Beacon フォーマット

## 概要

Deep sleep前に3秒間、Non-connectable Advertising (`ADV_TYPE_NONCONN_IND`) でセンサデータをbroadcastする。
Advertising interval: 20-40ms、Company ID: `0xFFFF` (test/private)。

識別: Company ID `0xFFFF` + Magic `"SM"` (0x53, 0x4D) でフィルタリングする。

## Raw Advertisement パケット (19 bytes)

| Offset | Size | Type | 内容 |
|--------|------|------|------|
| 0 | 1 | Length | `0x02` |
| 1 | 1 | AD Type | `0x01` (Flags) |
| 2 | 1 | Flags | `0x06` (LE General Discoverable + BR/EDR Not Supported) |
| 3 | 1 | Length | `0x0F` (15) |
| 4 | 1 | AD Type | `0xFF` (Manufacturer Specific Data) |
| 5-18 | 14 | Data | Manufacturer Data (下記) |

## Manufacturer Data (14 bytes)

| Offset | Size | 型 | フィールド | 値の範囲 |
|--------|------|----|-----------|---------|
| 0-1 | 2 | uint16 LE | Company ID | `0xFFFF` 固定 |
| 2-3 | 2 | char[2] | Magic | `"SM"` (0x53, 0x4D) |
| 4 | 1 | uint8 | Year - 2000 | 0-255 |
| 5 | 1 | uint8 | Month | 1-12 |
| 6 | 1 | uint8 | Day | 1-31 |
| 7 | 1 | uint8 | Hour | 0-23 |
| 8 | 1 | uint8 | Minute | 0-59 |
| 9 | 1 | uint8 | Battery % | 0-100 |
| 10-11 | 2 | uint16 LE | Battery mV | 例: 3700-4200 |
| 12 | 1 | uint8 | Sensor #1 | bit7: OK flag, bits0-6: pF値 |
| 13 | 1 | uint8 | Sensor #2 | bit7: OK flag, bits0-6: pF値 |

### Sensor byte の解釈

```
bit7    = 1: センサ正常, 0: センサ未検出
bits0-6 = 静電容量 (pF), 0-127
```

## パース例

### Python (bleak)

```python
import asyncio, struct
from bleak import BleakScanner

MAGIC = b"SM"

def parse(data: bytes):
    magic, year, month, day, hour, minute, batt_pct, batt_mv, s1, s2 = struct.unpack(
        "<2sBBBBBBHBB", data
    )
    if magic != MAGIC:
        return None
    return {
        "timestamp": f"20{year:02d}/{month}/{day} {hour:02d}:{minute:02d}",
        "battery_pct": batt_pct,
        "battery_mv": batt_mv,
        "sensor1": {"ok": bool(s1 & 0x80), "pf": s1 & 0x7F},
        "sensor2": {"ok": bool(s2 & 0x80), "pf": s2 & 0x7F},
    }

def callback(device, adv_data):
    mfr = adv_data.manufacturer_data
    if 0xFFFF not in mfr:
        return
    result = parse(mfr[0xFFFF])
    if result:
        print(f"{device.address}: {result}")

async def main():
    scanner = BleakScanner(callback)
    await scanner.start()
    await asyncio.sleep(30)

asyncio.run(main())
```

### nRF Connect / BLE Scanner

Manufacturer Specific Data の先頭4バイトが `FF FF 53 4D` のデバイスを探す。
