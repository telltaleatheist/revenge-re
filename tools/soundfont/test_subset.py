#!/usr/bin/env python3
"""subset.py's subset, on a small SF2 bank this test builds (no download).

    python3 -I tools/soundfont/test_subset.py [--renderer build/soundfont_render_test]

The bank has four presets: a mono looped sample, a stereo pair behind a global
zone, an instrument two presets share, and a percussion kit (bank 128). The
subset must keep exactly the listed presets and what they use, with the same
generators and sample points, and must be the same bytes every time. With
--renderer (or REVENGE_SOUNDFONT_RENDERER), TinySoundFont must find the same
inputs in both banks (tests/soundfont_render_test.cpp), and the test must
refuse a subset with one sample point changed or an unlisted preset kept.
From the MW1 rebuild (mech1-re), tools/soundfont/test_subset.py.
"""
import importlib.util, math, os, struct, sys, unittest

HERE = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location("subset", os.path.join(HERE, "subset.py"))
subset = importlib.util.module_from_spec(spec)
spec.loader.exec_module(subset)
RENDERER = None

KEY_RANGE, SAMPLE_MODES, PAN, SAMPLE_ID, INSTRUMENT = 43, 54, 17, 53, 41


def name20(text):
    return text.encode("ascii").ljust(20, b"\0")


def gen(oper, amount):
    return struct.pack("<HH", oper, amount & 0xFFFF)


def key_range(lo, hi):
    return struct.pack("<HBB", KEY_RANGE, lo, hi)


def build_bank():
    """A valid SF2: samples 0 mono (looped), 1/2 a stereo pair, 3 a short hit; instruments 0 (sample 0),
    1 (global pan zone, then the pair), 2 (sample 3); presets (0,0)->0, (0,1)->1, (0,2)->0, (128,0)->2."""
    pad = [0] * subset.SAMPLE_PAD
    shdr, points = [], []

    def add(label, n, freq, rate, kind, link, loop=None):
        start = len(points)
        points.extend(int(12000 * math.sin(2 * math.pi * freq * i / rate) * (1 - i / (2 * n))) for i in range(n))
        end = len(points)
        points.extend(pad)
        loop0, loop1 = (start + loop[0], start + loop[1]) if loop else (start, end)
        shdr.append(name20(label) + struct.pack("<IIIIIBbHH", start, end, loop0, loop1, rate, 60, 0, link, kind))

    add("mono", 4000, 440, 22050, 1, 0, loop=(1000, 3000))
    add("left", 3000, 330, 22050, 4, 2)
    add("right", 3000, 331, 22050, 2, 1)
    add("hit", 1500, 900, 22050, 1, 0)
    shdr.append(name20("EOS") + bytes(26))
    smpl = struct.pack(f"<{len(points)}h", *points)

    def table(items, terminal):
        """items: (header without bagNdx, [zone gens]) -> header, bag, mod, gen records."""
        hdr, bag, mod, gens = [], [], [], []
        for head, zones in items:
            hdr.append(head(len(bag)))
            for zone in zones:
                bag.append(struct.pack("<HH", len(gens), 0))
                gens.extend(zone)
        hdr.append(terminal(len(bag)))
        bag.append(struct.pack("<HH", len(gens), 0))
        return hdr, bag, [bytes(10)], gens + [bytes(4)]

    inst_head = lambda label: (lambda n: name20(label) + struct.pack("<H", n))
    inst, ibag, imod, igen = table([
        (inst_head("i0"), [[key_range(0, 127), gen(SAMPLE_MODES, 1), gen(SAMPLE_ID, 0)]]),
        (inst_head("i1"), [[gen(PAN, 0)], [key_range(0, 127), gen(PAN, -500), gen(SAMPLE_ID, 1)],
                           [key_range(0, 127), gen(PAN, 500), gen(SAMPLE_ID, 2)]]),
        (inst_head("i2"), [[key_range(30, 50), gen(SAMPLE_ID, 3)]]),
    ], inst_head("EOI"))
    preset_head = lambda label, bank, program: (lambda n: name20(label) + struct.pack("<HHHIII", program, bank, n, 0, 0, 0))
    phdr, pbag, pmod, pgen = table([
        (preset_head("p0", 0, 0), [[gen(INSTRUMENT, 0)]]),
        (preset_head("p1", 0, 1), [[gen(INSTRUMENT, 1)]]),
        (preset_head("p2", 0, 2), [[key_range(40, 80), gen(INSTRUMENT, 0)]]),
        (preset_head("kit", 128, 0), [[gen(INSTRUMENT, 2)]]),
    ], preset_head("EOP", 0, 0))

    def chunk(cid, payload):
        return cid.encode() + struct.pack("<I", len(payload)) + payload + (b"\0" if len(payload) & 1 else b"")

    info = chunk("ifil", struct.pack("<HH", 2, 1)) + chunk("INAM", b"test bank\0\0")
    pdta = b"".join(chunk(c, b"".join(r)) for c, r in (("phdr", phdr), ("pbag", pbag), ("pmod", pmod), ("pgen", pgen),
                                                      ("inst", inst), ("ibag", ibag), ("imod", imod), ("igen", igen),
                                                      ("shdr", shdr)))
    body = b"sfbk" + chunk("LIST", b"INFO" + info) + chunk("LIST", b"sdta" + chunk("smpl", smpl)) + chunk("LIST", b"pdta" + pdta)
    return b"RIFF" + struct.pack("<I", len(body)) + body


def presets(bank):
    return [(subset.word(r, 22), subset.word(r, 20)) for r in bank["phdr"][:-1]]


def sample_points(bank, index):
    start, end = struct.unpack_from("<II", bank["shdr"][index], 20)
    return bank["smpl"][start * 2:end * 2]


class Subset(unittest.TestCase):
    def setUp(self):
        self.full = build_bank()

    def test_keeps_exactly_the_listed_presets(self):
        part = subset.parse(subset.subset(self.full, [(0, 1), (128, 0)]))
        self.assertEqual(presets(part), [(0, 1), (128, 0)])
        self.assertEqual(len(part["inst"]) - 1, 2)
        self.assertEqual(len(part["shdr"]) - 1, 3)  # the pair and the hit

    def test_a_stereo_pair_comes_whole_and_relinked(self):
        part = subset.parse(subset.subset(self.full, [(0, 1)]))
        full = subset.parse(self.full)
        self.assertEqual(len(part["shdr"]) - 1, 2)
        self.assertEqual((subset.word(part["shdr"][0], 42), subset.word(part["shdr"][1], 42)), (1, 0))
        for new, old in ((0, 1), (1, 2)):
            self.assertEqual(sample_points(part, new), sample_points(full, old))

    def test_generators_and_loops_kept(self):
        part = subset.parse(subset.subset(self.full, [(0, 2)]))
        full = subset.parse(self.full)
        self.assertEqual(part["igen"][:-1], full["igen"][:3])  # instrument 0's zone, sample 0 is still 0
        self.assertEqual(part["pgen"][:-1], [key_range(40, 80), gen(INSTRUMENT, 0)])
        s, f = part["shdr"][0], full["shdr"][0]
        rel = lambda r: [v - struct.unpack_from("<I", r, 20)[0] for v in struct.unpack_from("<IIII", r, 20)]
        self.assertEqual(rel(s), rel(f))
        self.assertEqual(sample_points(part, 0), sample_points(full, 0))

    def test_shared_instrument_kept_once(self):
        part = subset.parse(subset.subset(self.full, [(0, 0), (0, 2)]))
        self.assertEqual(len(part["inst"]) - 1, 1)

    def test_deterministic(self):
        self.assertEqual(subset.subset(self.full, [(0, 1), (0, 0)]), subset.subset(self.full, [(0, 0), (0, 1)]))

    def test_refuses_a_missing_preset(self):
        with self.assertRaises(subset.Sf2Error):
            subset.subset(self.full, [(0, 9)])

    def test_refuses_what_is_not_a_bank(self):
        with self.assertRaises(subset.Sf2Error):
            subset.parse(b"RIFF\0\0\0\0WAVE")

    def test_plays_as_the_full_bank(self):
        renderer = RENDERER or os.environ.get("REVENGE_SOUNDFONT_RENDERER")
        if not renderer:
            self.skipTest("needs --renderer")
        wanted = [(0, 0), (0, 1), (128, 0)]
        part = subset.subset(self.full, wanted)
        self.assertTrue(subset.render_check(self.full, part, wanted, renderer))
        # One sample point changed: refused.
        broken = bytearray(part)
        at = broken.index(b"smpl") + 8 + 2 * 1200
        broken[at] ^= 0x40
        self.assertFalse(subset.render_check(self.full, bytes(broken), wanted, renderer))
        # A preset left out of the list but kept in the subset: refused.
        self.assertFalse(subset.render_check(self.full, part, wanted[:2], renderer))


if __name__ == "__main__":
    if "--renderer" in sys.argv:
        i = sys.argv.index("--renderer")
        RENDERER = sys.argv[i + 1]
        del sys.argv[i:i + 2]
    unittest.main()
