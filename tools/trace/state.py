#!/usr/bin/env python3
"""Turns a run of REVSNAP captures of the original (tools/capture/) into a
trace in the port's format, so check.py compares the original with the goldens.

    tools/trace/state.py --script SCRIPT.keys --run RUN_DIR --out TRACE.jsonl
        [--frames DIR]

Each capture (RVSnnnnn.BIN, in the script's @snap order) holds the frame, the
DAC and the first 640K of memory; the state lines, the frame's and palette's
hashes and the save block are made from them as the harness makes them from
the port's globals (src/SDL/harness.cpp). Addresses are the listing's (the image
loaded at 0800, the data segment 3A6A); the capture names the real load
segment. --frames writes NAME.indices, NAME.palette and NAME.save, as
record.py does.
"""
import argparse
import json
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import record  # noqa: E402

LISTING_BASE = 0x0800
DS = 0x3A6A
# 'RVS1', version 2, mode, load, polls, reads, ticks, CRTC start, attribute
# registers 0-15, 10h and 14h
HEADER = struct.Struct("<4sBBHIIHH18s")
PLANE_BYTES = 8000
DAC_BYTES = 768
FRAME_BYTES = 64000
MEMORY_BYTES = 0xA0000

UNIT_COUNT = 0x18
UNIT_SIZE = 0x96
UNIT_DURABILITY = (0x04, 0x13)
UNIT_HEAT, UNIT_ENGINE, UNIT_PILOT = 0x87, 0x85, 0x92

# The save block (SaveGame, 0D73:4866): the campaign snapshot, field by field
# from where the game keeps it, then the 24 live unit records.
SNAPSHOT = (
    (0x0000, 2, 0x24D6), (0x0002, 2, 0x24D8), (0x0004, 1, 0x24DA), (0x0005, 1, 0x24DC),
    (0x0006, 2, 0x24DF), (0x0008, 2, 0x24E1), (0x000A, 2, 0xA967), (0x000C, 2, 0x9991),
    (0x000E, 2, 0x88F1), (0x0010, 2, 0x8925), (0x0012, 2, 0x99C5), (0x0014, 2, 0xA965),
    (0x0016, 2, 0x8594), (0x0018, 2, 0x8108), (0x001A, 2, 0x810A), (0x001C, 3, 0x855F),
    (0x001F, 6, 0x00A0), (0x0025, 24, 0x8562), (0x003D, 24, 0x857B), (0x0055, 7, 0x1B85),
    (0x005C, 1, 0x1B8E), (0x005D, 15, 0x1BAD), (0x006C, 270, 0x1BBC), (0x017A, 130, 0x1CCA),
)
SNAPSHOT_SIZE = 0x221
PILOT_AVAILABILITY = (0x01FC, 37, 0x1D4F, 4)    # save offset, pilots, first, stride
LIVE_UNITS = 0x99E0
PERSISTENT_UNITS = 0x8B81
UNIT_MAP_X, UNIT_MAP_Y = 0x8562, 0x857B


class Capture:
    def __init__(self, path):
        with open(path, "rb") as f:
            data = f.read()
        (magic, version, self.mode, self.load, self.polls, self.reads, self.ticks, self.start,
         self.attributes) = HEADER.unpack_from(data)
        if magic != b"RVS1" or version != 2:
            sys.exit(f"state: {path} isn't a version 2 REVSNAP capture")
        at = HEADER.size
        self.dac = data[at:at + DAC_BYTES]
        at += DAC_BYTES
        if self.mode == 0x0D:
            planes = [data[at + p * PLANE_BYTES:at + (p + 1) * PLANE_BYTES] for p in range(4)]
            at += 4 * PLANE_BYTES
            self.frame = planar_pixels(planes)
        else:
            self.frame = data[at:at + FRAME_BYTES]
            at += FRAME_BYTES
        if data[at:at + 4] != b"MEMS" or len(data) < at + 4 + MEMORY_BYTES:
            sys.exit(f"state: {path} is cut short")
        self.memory = data[at + 4:at + 4 + MEMORY_BYTES]

    def at(self, segment, offset, size):
        linear = (segment - LISTING_BASE + self.load) * 16 + offset
        return self.memory[linear:linear + size]

    def ds(self, offset, size=1):
        return self.at(DS, offset, size)

    def byte(self, offset):
        return self.ds(offset)[0]

    def word(self, offset):
        return struct.unpack("<H", self.ds(offset, 2))[0]

    def dword(self, offset):
        return struct.unpack("<I", self.ds(offset, 4))[0]

    def palette(self):
        """The colours the screen can show, 8 bits each as the port keeps
        them: in mode 0Dh the 16 the attribute registers pick from the DAC,
        otherwise the DAC's 256."""
        dac = bytes(((v & 0x3F) << 2) | ((v & 0x3F) >> 4) for v in self.dac)
        if self.mode != 0x0D:
            return dac
        control, select = self.attributes[16], self.attributes[17]
        colours = bytearray()
        for value in self.attributes[:16]:
            if control & 0x80:
                index = (value & 0x0F) | ((select & 0x0F) << 4)
            else:
                index = (value & 0x3F) | ((select & 0x0C) << 4)
            colours += dac[index * 3:index * 3 + 3]
        return bytes(colours)

    def save_block(self):
        block = bytearray(SNAPSHOT_SIZE)
        for offset, size, source in SNAPSHOT:
            block[offset:offset + size] = self.ds(source, size)
        offset, count, first, stride = PILOT_AVAILABILITY
        for index in range(count):
            block[offset + index] = self.byte(first + index * stride)
        return bytes(block) + self.ds(LIVE_UNITS, UNIT_COUNT * UNIT_SIZE)

    def state(self):
        random = self.at(0x4184, 0x0E48, 3)
        variant = struct.unpack("<h", self.ds(0x24D8, 2))[0]
        lines = [
            "state random %02x %02x %02x" % tuple(random),
            "state input polls %d reads %d clock %d" % (self.polls, self.reads, self.ticks * 65536 * 1000 // 1193182),
            "state campaign stage %u phase %u variant %d flags %04x saved %04x training %u"
            % (self.word(0x24D6), self.byte(0x24DA), variant, self.word(0x24DF), self.word(0x24E1),
               self.byte(0x24DC)),
            "state timer game %u animation %u" % (self.dword(0x591B), self.dword(0x591F)),
            "state clock %u %u %u subtick %u" % (self.byte(0x8337), self.byte(0x8338), self.byte(0x8339),
                                                 self.byte(0x8339)),
            "state battle result %u selected %04x overview %u delay %u"
            % (self.word(0x833C), self.word(0x03DC), self.word(0x8104), self.word(0x5923)),
            "state cursor map %u %u viewport %u %u"
            % (self.word(0xA96D), self.word(0xA96F), self.word(0x8331), self.word(0x8333)),
        ]
        for index in range(UNIT_COUNT):
            unit = self.ds(PERSISTENT_UNITS + index * UNIT_SIZE, UNIT_SIZE)
            x, y = self.byte(UNIT_MAP_X + index), self.byte(UNIT_MAP_Y + index)
            if unit[0] == 0 and x == 0 and y == 0:
                continue
            durability = sum(unit[UNIT_DURABILITY[0]:UNIT_DURABILITY[0] + UNIT_DURABILITY[1]])
            lines.append("state unit %u deploy %u at %u %u durability %u heat %u engine %u pilot %u"
                         % (index, unit[0], x, y, durability, unit[UNIT_HEAT], unit[UNIT_ENGINE], unit[UNIT_PILOT]))
        return lines


def planar_pixels(planes):
    """Mode 0Dh's four planes as 320x200 colour indices: plane p is bit p,
    the most significant bit of a byte the leftmost pixel."""
    pixels = bytearray(FRAME_BYTES)
    for offset in range(PLANE_BYTES):
        bytes4 = [plane[offset] if offset < len(plane) else 0 for plane in planes]
        base = offset * 8
        for bit in range(8):
            shift = 7 - bit
            pixels[base + bit] = (((bytes4[0] >> shift) & 1) | (((bytes4[1] >> shift) & 1) << 1)
                                  | (((bytes4[2] >> shift) & 1) << 2) | (((bytes4[3] >> shift) & 1) << 3))
    return bytes(pixels)


def digest(data):
    import hashlib
    return hashlib.sha256(data).hexdigest()[:32]


def trace(script, run, frames=None):
    groups = record.tokens_by_snap(script)
    captures = sorted(n for n in os.listdir(run) if n.startswith("RVS") and n.endswith(".BIN"))
    if len(captures) < len(groups):
        sys.exit(f"state: {len(captures)} captures for the script's {len(groups)} snaps")
    records = []
    for (name, keys), file in zip(groups, captures):
        capture = Capture(os.path.join(run, file))
        palette = capture.palette()
        save = capture.save_block()
        records.append({
            "snap": name,
            "keys": keys,
            "state": capture.state(),
            "frame": {"indices": digest(capture.frame), "palette": digest(palette)},
            "save": digest(save),
        })
        if frames:
            os.makedirs(frames, exist_ok=True)
            with open(os.path.join(frames, name + ".indices"), "wb") as f:
                f.write(capture.frame)
            with open(os.path.join(frames, name + ".palette"), "wb") as f:
                f.write(palette)
            with open(os.path.join(frames, name + ".save"), "wb") as f:
                f.write(save)
    return records


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--script", required=True)
    parser.add_argument("--run", required=True)
    parser.add_argument("--out", required=True)
    parser.add_argument("--frames")
    args = parser.parse_args()
    records = trace(args.script, args.run, args.frames)
    record.write(records, args.out)
    print(f"state: {len(records)} snaps to {args.out}")


if __name__ == "__main__":
    main()
