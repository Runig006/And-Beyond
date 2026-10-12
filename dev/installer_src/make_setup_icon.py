"""Draws BuzzSetup.exe's icon, an old-style installer one (a beige monitor and a CD in front), and writes
src\\setup.ico with a 32x32 and a 16x16 image (32 bits, hard edges and the 16-colour Windows palette, as in 1998).

Use:
  python dev\\installer_src\\make_setup_icon.py
"""
import struct
from pathlib import Path

BLACK = (0, 0, 0)
DARK = (128, 128, 128)
LIGHT = (192, 192, 192)
WHITE = (255, 255, 255)
BEIGE = (224, 216, 184)
TEAL = (0, 128, 128)
CYAN = (0, 255, 255)
NAVY = (0, 0, 128)
YELLOW = (255, 255, 0)
MAGENTA = (255, 0, 255)


def draw(size):
    img = [[None] * size for _ in range(size)]
    s = size / 32

    def rect(x0, y0, x1, y1, c):
        for y in range(int(y0 * s), int(y1 * s)):
            for x in range(int(x0 * s), int(x1 * s)):
                img[y][x] = c

    # monitor: outline, case, screen with a highlight, stand and base
    rect(2, 2, 26, 22, BLACK)
    rect(3, 3, 25, 21, BEIGE)
    rect(5, 5, 23, 18, DARK)
    rect(6, 6, 22, 17, TEAL)
    rect(7, 7, 12, 8, CYAN)
    rect(7, 8, 8, 11, CYAN)
    rect(10, 22, 18, 24, BLACK)
    rect(11, 22, 17, 23, BEIGE)
    rect(6, 24, 22, 27, BLACK)
    rect(7, 24, 21, 26, BEIGE)
    # the CD, in front at the bottom right
    cx, cy, r = 23.5 * s, 23.5 * s, 8 * s
    for y in range(size):
        for x in range(size):
            d = ((x + 0.5 - cx) ** 2 + (y + 0.5 - cy) ** 2) ** 0.5
            if d <= r:
                c = LIGHT
                if d > r - 1.2 * max(1, s):
                    c = BLACK
                elif d < 1.6 * s:
                    c = BLACK if d > 0.9 * s else None
                elif d < 3 * s:
                    c = WHITE
                elif abs((x + 0.5 - cx) - (y + 0.5 - cy)) < 1.2 * s and x + 0.5 < cx:
                    c = YELLOW if size == 32 and (x + y) % 4 < 2 else MAGENTA if size == 32 else WHITE
                img[y][x] = c
    return img


def ico_image(img):
    size = len(img)
    pixels = b"".join(bytes((c[2], c[1], c[0], 255)) if c else bytes(4) for row in reversed(img) for c in row)
    # AND mask: 1 bit per pixel, rows padded to 4 bytes; 1 = transparent
    stride = (size + 31) // 32 * 4
    mask = bytearray()
    for row in reversed(img):
        bits = bytearray(stride)
        for x, c in enumerate(row):
            if c is None:
                bits[x // 8] |= 0x80 >> (x % 8)
        mask += bits
    bih = struct.pack("<IiiHHIIiiII", 40, size, size * 2, 1, 32, 0, len(pixels) + len(mask), 0, 0, 0, 0)
    return bih + pixels + bytes(mask)


def main():
    images = [ico_image(draw(32)), ico_image(draw(16))]
    head = struct.pack("<HHH", 0, 1, len(images))
    offset = 6 + 16 * len(images)
    entries = b""
    for size, data in zip((32, 16), images):
        entries += struct.pack("<BBBBHHII", size, size, 0, 0, 1, 32, len(data), offset)
        offset += len(data)
    out = Path(__file__).parent / "src" / "setup.ico"
    out.write_bytes(head + entries + b"".join(images))
    print(out)


if __name__ == "__main__":
    main()
