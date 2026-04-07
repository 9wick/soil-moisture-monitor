# 書き込み手順

## 前提

- Arduino CLI: `arduino-cli`
- ボード: `m5stack:esp32:m5stack_coreink`
- ポート: `/dev/cu.usbserial-*` (環境により異なる)

ポート確認:
```bash
ls /dev/cu.usb*
```

## 本体スケッチ

```bash
arduino-cli compile --fqbn m5stack:esp32:m5stack_coreink:PartitionScheme=no_ota soil-moisture-monitor.ino
arduino-cli upload --fqbn m5stack:esp32:m5stack_coreink:PartitionScheme=no_ota --port /dev/cu.usbserial-XXXX soil-moisture-monitor.ino
```

## RTC時刻セット

初回または時刻がずれた場合に実行。コンパイル時の時刻がRTCにセットされる。

```bash
arduino-cli compile --fqbn m5stack:esp32:m5stack_coreink tools/set-rtc/set-rtc.ino
arduino-cli upload --fqbn m5stack:esp32:m5stack_coreink:PartitionScheme=no_ota --port /dev/cu.usbserial-XXXX tools/set-rtc/set-rtc.ino
```

セット後、本体スケッチを再書き込みすること。

## センサ安定化時間の計測

リセット後、センサのraw countが安定するまでの時間を計測する。

```bash
arduino-cli compile --fqbn m5stack:esp32:m5stack_coreink tools/measure-settling/measure-settling.ino
arduino-cli upload --fqbn m5stack:esp32:m5stack_coreink:PartitionScheme=no_ota --port /dev/cu.usbserial-XXXX tools/measure-settling/measure-settling.ino
arduino-cli monitor --port /dev/cu.usbserial-XXXX --config baudrate=115200
```

計測後、本体スケッチを再書き込みすること。

## Serial Monitor

`arduino-cli monitor` は非対話環境では即終了するため、stdinを維持する必要がある。

```bash
sleep 60 | arduino-cli monitor -p /dev/cu.usbserial-XXXX -c baudrate=115200 --quiet --timestamp
```

`sleep` の秒数が監視時間の上限。デバイスのリセットボタンを押すとboot時からの出力が確認できる。

## デバッグモード

G38ボタンを押しながら電源ON → イラスト確認モード

- G37: パネル#1のイラスト切替 (DEAD→DRY→GOOD→WET→...)
- G39: パネル#2のイラスト切替
- 終了: 電源ボタンで電源OFF
