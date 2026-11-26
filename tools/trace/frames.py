#!/usr/bin/env python3
"""Turns a harness run's frames into PNGs for a person to look at.

    tools/trace/frames.py RUN_DIR OUT_DIR [--scale N]

RUN_DIR holds NAME.indices and NAME.palette for each snap (the harness's
output, or record.py's --frames copy). The frames are the game's art: keep
them, and the PNGs, outside the repo.
"""
import os
import struct
import sys
import zlib

WIDTH, HEIGHT = 320, 200


def png(path, rows, width, height):
    def chunk(kind, data):
        body = kind + data
        return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)
    raw = b"".join(b"\0" + row for row in rows)
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)))
        f.write(chunk(b"IDAT", zlib.compress(raw, 9)))
        f.write(chunk(b"IEND", b""))


def main():
    args = sys.argv[1:]
    scale = 2
    if "--scale" in args:
        at = args.index("--scale")
        scale = int(args[at + 1])
        del args[at:at + 2]
    if len(args) != 2:
        sys.exit(__doc__)
    source, out = args
    os.makedirs(out, exist_ok=True)
    names = sorted(n[:-8] for n in os.listdir(source) if n.endswith(".indices"))
    for name in names:
        with open(os.path.join(source, name + ".indices"), "rb") as f:
            indices = f.read()
        with open(os.path.join(source, name + ".palette"), "rb") as f:
            palette = f.read()
        if len(indices) != WIDTH * HEIGHT or len(palette) != 0x300:
            sys.exit(f"frames: {name} is not a 320x200 frame with 256 colours")
        rows = []
        for y in range(HEIGHT):
            row = b"".join(palette[i * 3:i * 3 + 3] * scale for i in indices[y * WIDTH:(y + 1) * WIDTH])
            rows.extend([row] * scale)
        png(os.path.join(out, name + ".png"), rows, WIDTH * scale, HEIGHT * scale)
    print(f"frames: {len(names)} PNG(s) in {out}")


if __name__ == "__main__":
    main()
