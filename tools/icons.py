#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Generate original Arcade joystick artwork, using only Python's standard library."""
import argparse
import pathlib
import struct
import zlib

S = 8
ART = {"icon-128.png": 128, "icon.png": 52, "icon-40.png": 40, "pane.png": 96}
PURPLE = (94, 69, 194)
WHITE = (255, 255, 255)


def rounded(x, y, left, top, right, bottom, radius):
    if not (left <= x <= right and top <= y <= bottom):
        return False
    cx = min(max(x, left + radius), right - radius)
    cy = min(max(y, top + radius), bottom - radius)
    return (x-cx)**2 + (y-cy)**2 <= radius**2


def circle(x, y, cx, cy, radius):
    return (x-cx)**2 + (y-cy)**2 <= radius**2


def glyph(x, y):
    # Arcade cabinet with joystick and two buttons; cutouts are true negative space.
    shell = rounded(x, y, 18, 13, 82, 87, 8)
    if not shell:
        return False
    screen = rounded(x, y, 26, 21, 74, 51, 4)
    # Pixel alien on the screen, recognizable even at 40 px.
    alien = ("00100100", "00011000", "01111110", "11011011",
             "11111111", "10100101", "00100100")
    if screen:
        col, row = int((x-34)//4), int((y-23)//4)
        return 0 <= col < 8 and 0 <= row < 7 and alien[row][col] == "1"
    if rounded(x, y, 25, 57, 75, 79, 4):
        stick = circle(x,y,39,63,5) or (37<=x<=41 and 63<=y<=73)
        base = rounded(x,y,32,72,46,76,2)
        return stick or base or circle(x,y,58,68,4) or circle(x,y,69,68,4)
    return True


def sample(x, y, pane):
    if pane:
        # Fill most of the 96px pane without touching its edges.
        x, y = (x-50)/1.19+50, (y-50)/1.19+50
        return WHITE + (255 if glyph(x,y) else 0,)
    if not rounded(x,y,0,0,100,100,22):
        return PURPLE + (0,)
    return (WHITE if glyph(x,y) else PURPLE) + (255,)


def chunk(kind, data):
    return (struct.pack(">I", len(data)) + kind + data +
            struct.pack(">I", zlib.crc32(kind + data) & 0xffffffff))


def png(size, pane):
    rows = bytearray()
    for y in range(size):
        rows.append(0)
        for x in range(size):
            sums = [0,0,0]
            alpha = 0
            for sy in range(S):
                for sx in range(S):
                    rr,gg,bb,aa = sample((x+(sx+.5)/S)*100/size,
                                         (y+(sy+.5)/S)*100/size,pane)
                    alpha += aa
                    for i,c in enumerate((rr,gg,bb)):
                        sums[i] += c*aa
            rgb = [round(c/alpha) for c in sums] if alpha else list(WHITE if pane else PURPLE)
            rows.extend(rgb+[round(alpha/(S*S))])
    return (b"\x89PNG\r\n\x1a\n" +
            chunk(b"IHDR",struct.pack(">IIBBBBB",size,size,8,6,0,0,0)) +
            chunk(b"IDAT",zlib.compress(rows,9)) + chunk(b"IEND",b""))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output",type=pathlib.Path,required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True,exist_ok=True)
    for name,size in ART.items():
        (args.output/name).write_bytes(png(size,name=="pane.png"))


if __name__ == "__main__":
    main()
