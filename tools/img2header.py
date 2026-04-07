#!/usr/bin/env python3
"""
画像をモノクロbitmapに変換し、Cヘッダファイルとして出力する。

Usage:
  python3 tools/img2header.py img/face_dead.png --name face_dead --size 48 -o faceIcon.h
  python3 tools/img2header.py img/*.png --size 48 -o faceIcon.h

出力形式: Adafruit GFX互換 (MSB first, 行ごとにbyte境界パディング)
M5.Display.drawBitmap(x, y, bitmap, w, h, TFT_BLACK) で描画可能。
"""

import argparse
import glob
import sys
from pathlib import Path

from PIL import Image, ImageFilter, ImageOps


def image_to_bitmap(path: str, size: int, threshold: int, invert: bool, padding: int = 1) -> tuple[list[int], int, int, Image.Image]:
    img = Image.open(path).convert("RGBA")

    # 白背景に合成（透明部分を白に）
    bg = Image.new("RGBA", img.size, (255, 255, 255, 255))
    bg.paste(img, mask=img)
    gray = bg.convert("L")

    # オートトリミング（白余白を除去）
    bw_trim = gray.point(lambda p: 0 if p < threshold else 255, mode="1")
    bbox = bw_trim.getbbox()
    if bbox:
        gray = gray.crop(bbox)

    # アスペクト比維持でリサイズ（padding分を差し引いた領域に収める）
    inner = max(1, size - padding * 2)
    gray.thumbnail((inner, inner), Image.LANCZOS)
    # padding付きキャンバスに配置（中央寄せ）
    inner_w, inner_h = gray.size
    canvas = Image.new("L", (inner_w + padding * 2, inner_h + padding * 2), 255)
    canvas.paste(gray, (padding, padding))
    gray = canvas
    w, h = gray.size

    # シャープネス強調してから二値化
    gray = gray.filter(ImageFilter.SHARPEN)
    bw = gray.point(lambda p: 0 if p < threshold else 255, mode="1")

    if invert:
        bw = bw.point(lambda p: 255 - p, mode="1")

    # Adafruit GFX形式: MSB first, 行ごとにbyte境界
    bytes_per_row = (w + 7) // 8
    data = []
    for y in range(h):
        for bx in range(bytes_per_row):
            byte = 0
            for bit in range(8):
                x = bx * 8 + bit
                if x < w and bw.getpixel((x, y)) == 0:
                    byte |= 0x80 >> bit
            data.append(byte)

    return data, w, h, bw


def to_var_name(name: str) -> str:
    return name.replace("-", "_").replace(" ", "_")


def generate_header(entries: list[tuple[str, list[int], int, int]], guard: str) -> str:
    lines = []
    lines.append(f"#ifndef {guard}")
    lines.append(f"#define {guard}")
    lines.append("")
    lines.append("#include <stdint.h>")
    lines.append("")

    for name, data, w, h in entries:
        var = to_var_name(name)
        lines.append(f"#define {var.upper()}_WIDTH  {w}")
        lines.append(f"#define {var.upper()}_HEIGHT {h}")
        lines.append(f"static const uint8_t {var}_bitmap[] PROGMEM = {{")

        for i in range(0, len(data), 12):
            chunk = data[i : i + 12]
            hex_str = ", ".join(f"0x{b:02x}" for b in chunk)
            lines.append(f"  {hex_str},")

        lines.append("};")
        lines.append("")

    lines.append(f"#endif // {guard}")
    lines.append("")
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description="画像→Cヘッダ bitmap変換")
    parser.add_argument("inputs", nargs="+", help="入力画像パス (glob可)")
    parser.add_argument("--name", help="変数名 (単一画像時)")
    parser.add_argument("--size", type=int, default=48, help="リサイズ最大辺 (default: 48)")
    parser.add_argument("--threshold", type=int, default=128, help="二値化閾値 0-255 (default: 128)")
    parser.add_argument("--padding", type=int, default=1, help="トリミング後のpadding (default: 1)")
    parser.add_argument("--invert", action="store_true", help="白黒反転")
    parser.add_argument("--bmp-dir", default=None, help="二値化BMP画像の出力先ディレクトリ")
    parser.add_argument("-o", "--output", default="faceIcon.h", help="出力ヘッダファイル")
    args = parser.parse_args()

    # glob展開
    paths = []
    for pattern in args.inputs:
        expanded = sorted(glob.glob(pattern))
        paths.extend(expanded if expanded else [pattern])

    if not paths:
        print("Error: no input files", file=sys.stderr)
        sys.exit(1)

    entries = []
    for p in paths:
        name = args.name if (args.name and len(paths) == 1) else Path(p).stem
        data, w, h, bw_img = image_to_bitmap(p, args.size, args.threshold, args.invert, args.padding)
        entries.append((name, data, w, h))
        print(f"  {name}: {w}x{h} ({len(data)} bytes)")

        if args.bmp_dir:
            bmp_dir = Path(args.bmp_dir)
            bmp_dir.mkdir(parents=True, exist_ok=True)
            bmp_path = bmp_dir / f"{name}.bmp"
            bw_img.save(bmp_path)
            print(f"  -> {bmp_path}")

    guard = Path(args.output).stem.upper().replace("-", "_") + "_H"
    header = generate_header(entries, guard)

    out_path = Path(args.output)
    out_path.write_text(header)
    print(f"-> {out_path}")


if __name__ == "__main__":
    main()
