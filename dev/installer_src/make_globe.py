"""Draws the installer's "Translation" button icon (a globe) and writes src\\globe.ico (32x32, 32 bits).

Our own picture: the original installer only has the install, play and cancel buttons. Drawn 8 times bigger and
averaged down, for soft edges. Colours: blue globe, white meridians, dark purple rim like the labels' outline.

Use:
  python dev\\installer_src\\make_globe.py
"""
import math
import struct
from pathlib import Path

SIZE = 32
SUB = 8
RIM = (40, 0, 80)
SEA = (40, 120, 215)
SEA_LIGHT = (110, 185, 245)
LINE = (255, 255, 255)


def pixel(x, y):
    """Colour and coverage of a point, in icon pixels (centre 16,16)."""
    cx = cy = SIZE / 2
    r = 14.5
    dx, dy = x - cx, y - cy
    d = math.hypot(dx, dy)
    if d > r:
        return None
    if d > r - 1.6:
        return RIM
    nx, ny = dx / r, dy / r
    # meridians: ellipses x = k * sqrt(1 - y^2); parallels: y = constant
    half = math.sqrt(max(0.0, 1 - ny * ny))
    for k in (0.0, 0.55):
        if half > 0 and abs(abs(nx) - k * half) < 0.06:
            return LINE
    for k in (0.0, 0.5):
        if abs(abs(ny) - k) < 0.05:
            return LINE
    # light from the top left
    t = max(0.0, min(1.0, 0.5 - (nx + ny) * 0.45))
    return tuple(int(a + (b - a) * t) for a, b in zip(SEA, SEA_LIGHT))


def main():
    rows = []
    for py in range(SIZE):
        row = []
        for px in range(SIZE):
            acc = [0, 0, 0, 0]
            for sy in range(SUB):
                for sx in range(SUB):
                    c = pixel(px + (sx + 0.5) / SUB, py + (sy + 0.5) / SUB)
                    if c:
                        acc[0] += c[0]
                        acc[1] += c[1]
                        acc[2] += c[2]
                        acc[3] += 1
            n = acc[3]
            if n:
                row.append((acc[0] // n, acc[1] // n, acc[2] // n, n * 255 // (SUB * SUB)))
            else:
                row.append((0, 0, 0, 0))
        rows.append(row)
    # .ico: header, one directory entry, BITMAPINFOHEADER (double height), BGRA bottom-up, AND mask
    pixels = b"".join(bytes((b, g, r, a)) for row in reversed(rows) for r, g, b, a in row)
    mask = bytes(4 * SIZE)
    bih = struct.pack("<IiiHHIIiiII", 40, SIZE, SIZE * 2, 1, 32, 0, len(pixels) + len(mask), 0, 0, 0, 0)
    image = bih + pixels + mask
    ico = struct.pack("<HHH", 0, 1, 1) + struct.pack("<BBBBHHII", SIZE, SIZE, 0, 0, 1, 32, len(image), 22) + image
    out = Path(__file__).parent / "src" / "globe.ico"
    out.write_bytes(ico)
    print(out, len(ico))


if __name__ == "__main__":
    main()
