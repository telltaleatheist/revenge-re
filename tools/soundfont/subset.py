#!/usr/bin/env python3
"""The music soundfont: a subset of FluidR3 GM holding only the programs we use.

    subset.py fetch                          # once per machine: the pinned source into build-deps/soundfont
    subset.py generate --out FILE            # the subset (the build runs this); refused unless it hashes as pinned
    subset.py check [--renderer EXE]         # regenerate and prove it: hash, licence texts, and it plays as the bank
    subset.py record                         # after changing the program list: the new hash and the licence texts

The pinned source is Debian's copy of the upstream tarball, fluid-soundfont 3.1
(SOURCE_URL on snapshot.debian.org, checked by SOURCE_SHA256). It carries
FluidR3_GM.sf2 (checked by BANK_SHA256), Frank Wen's COPYING and his README.
The source is build-deps/soundfont/<SOURCE_NAME>, or REVENGE_SOUNDFONT_SOURCE
names a copy. Only `fetch` uses the network: a build or a player's install
never does, and CMakeLists.txt refuses to configure without the source.

The programs come from data/music/soundfont_programs.txt, one "bank program"
pair per line (bank 128 holds the percussion kits). The subset keeps those
presets, the instruments they use and the samples those use, with every
generator, modulator and sample byte unchanged, so each kept program plays
exactly as it does in the full bank (tests/soundfont_render_test.cpp proves it).
The repo holds the list, the subset's SHA-256 (data/music/soundfont_subset.sha256)
and the licence texts. The subset itself is generated into the build.

From the MW1 rebuild (mech1-re), tools/soundfont/subset.py, with REVENGE_ in
its environment names; the program list is Revenge's.

Exit status: 0 ok, 1 a mismatch or a failure.
"""
import argparse, hashlib, io, os, struct, subprocess, sys, tarfile, tempfile, urllib.request

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
MUSIC = os.path.join(ROOT, "data", "music")
PROGRAMS = os.path.join(MUSIC, "soundfont_programs.txt")
SUBSET_HASH = os.path.join(MUSIC, "soundfont_subset.sha256")
SUBSET_NAME = "FluidR3_GM-subset.sf2"
LICENCE_TEXTS = {"COPYING": os.path.join(MUSIC, "FluidR3_GM-COPYING.txt"),
                 "README": os.path.join(MUSIC, "FluidR3_GM-README.txt")}
# snapshot.debian.org keeps every file it has seen at a URL named by its SHA-1.
SOURCE_URL = "https://snapshot.debian.org/file/433118a6776867176ae710d1e3370540be30be0a"
SOURCE_NAME = "fluid-soundfont_3.1.orig.tar.gz"
SOURCE_SHA256 = "2621acaa1c78e4abdb24bdd163230cc577e61276936d6aa6e3180582142f0343"
DEFAULT_SOURCE = os.path.join(ROOT, "build-deps", "soundfont", SOURCE_NAME)
BANK_MEMBER = "fluid-soundfont-3.1/FluidR3_GM.sf2"
BANK_SHA256 = "74594e8f4250680adf590507a306655a299935343583256f3b722c48a1bc1cb0"

# pdta record sizes and the generators that index other records.
SIZES = {"phdr": 38, "pbag": 4, "pmod": 10, "pgen": 4, "inst": 22, "ibag": 4, "imod": 10, "igen": 4, "shdr": 46}
GEN_INSTRUMENT, GEN_SAMPLE_ID = 41, 53
SAMPLE_PAD = 46  # the SF2 spec's minimum run of data points after each sample


class Sf2Error(ValueError):
    pass


def chunks(data, start, end):
    """(id, payload start, payload end) for each chunk in data[start:end]."""
    out = []
    while start + 8 <= end:
        cid, size = data[start:start + 4].decode("latin-1"), struct.unpack_from("<I", data, start + 4)[0]
        if start + 8 + size > end:
            raise Sf2Error(f"chunk {cid} runs past its parent")
        out.append((cid, start + 8, start + 8 + size))
        start += 8 + size + (size & 1)
    return out


def parse(data):
    """The bank's parts: INFO bytes, smpl (and sm24) bytes, and each pdta table as a list of records."""
    if data[:4] != b"RIFF" or data[8:12] != b"sfbk":
        raise Sf2Error("not an SF2 bank")
    lists = {}
    for cid, a, b in chunks(data, 12, 8 + struct.unpack_from("<I", data, 4)[0]):
        if cid == "LIST":
            lists[data[a:a + 4].decode("latin-1")] = (a + 4, b)
    if set(lists) < {"INFO", "sdta", "pdta"}:
        raise Sf2Error("missing INFO, sdta or pdta")
    bank = {"INFO": data[lists["INFO"][0]:lists["INFO"][1]], "sm24": None}
    for cid, a, b in chunks(data, *lists["sdta"]):
        if cid in ("smpl", "sm24"):
            bank[cid] = data[a:b]
    for cid, a, b in chunks(data, *lists["pdta"]):
        if cid in SIZES:
            size = SIZES[cid]
            if (b - a) % size:
                raise Sf2Error(f"{cid} is not a whole number of records")
            bank[cid] = [data[i:i + size] for i in range(a, b, size)]
    if any(t not in bank for t in SIZES) or "smpl" not in bank:
        raise Sf2Error("missing a pdta table or smpl")
    return bank


def word(record, at):
    return struct.unpack_from("<H", record, at)[0]


def zones(bags, gens, mods, first, last):
    """For bags first..last-1: each zone's (generator records, modulator records)."""
    out = []
    for i in range(first, last):
        g0, m0 = word(bags[i], 0), word(bags[i], 2)
        g1, m1 = word(bags[i + 1], 0), word(bags[i + 1], 2)
        out.append((gens[g0:g1], mods[m0:m1]))
    return out


def subset(data, wanted):
    """The SF2 bytes holding only the presets in `wanted` ((bank, program) pairs), and what they use."""
    b = parse(data)
    phdr, inst, shdr = b["phdr"], b["inst"], b["shdr"]
    keep_presets = []
    for i, rec in enumerate(phdr[:-1]):
        if (word(rec, 22), word(rec, 20)) in wanted:
            keep_presets.append(i)
    found = {(word(phdr[i], 22), word(phdr[i], 20)) for i in keep_presets}
    if found != set(wanted):
        raise Sf2Error(f"not in the bank: {sorted(set(wanted) - found)}")

    preset_zones = {i: zones(b["pbag"], b["pgen"], b["pmod"], word(phdr[i], 24), word(phdr[i + 1], 24))
                    for i in keep_presets}
    used_inst = sorted({word(g, 2) for zs in preset_zones.values() for gens, _ in zs for g in gens
                        if word(g, 0) == GEN_INSTRUMENT})
    inst_zones = {i: zones(b["ibag"], b["igen"], b["imod"], word(inst[i], 20), word(inst[i + 1], 20))
                  for i in used_inst}
    used_samples = {word(g, 2) for zs in inst_zones.values() for gens, _ in zs for g in gens
                    if word(g, 0) == GEN_SAMPLE_ID}
    while True:  # a stereo or linked sample brings its partner
        linked = {word(shdr[s], 42) for s in used_samples if word(shdr[s], 44) & 0x7FFF not in (0, 1)}
        if linked <= used_samples:
            break
        used_samples |= linked
    used_samples = sorted(used_samples)
    inst_index = {old: new for new, old in enumerate(used_inst)}
    sample_index = {old: new for new, old in enumerate(used_samples)}

    def remap(gens, oper, table):
        out = []
        for g in gens:
            if word(g, 0) == oper:
                g = g[:2] + struct.pack("<H", table[word(g, 2)])
            out.append(g)
        return out

    def tables(headers, zone_lists, oper, table, name_size, tail):
        """Header, bag, mod and gen records for the kept items, each table ending in its terminal record."""
        hdr, bag, mod, gen = [], [], [], []
        for header, zs in zip(headers, zone_lists):
            hdr.append(header[:name_size] + struct.pack("<H", len(bag)) + header[name_size + 2:])
            for gens, mods in zs:
                bag.append(struct.pack("<HH", len(gen), len(mod)))
                gen += remap(gens, oper, table)
                mod += mods
        hdr.append(tail[:name_size] + struct.pack("<H", len(bag)) + tail[name_size + 2:])
        bag.append(struct.pack("<HH", len(gen), len(mod)))
        mod.append(bytes(10))
        gen.append(bytes(4))
        return hdr, bag, mod, gen

    new_phdr, pbag, pmod, pgen = tables([phdr[i] for i in keep_presets], [preset_zones[i] for i in keep_presets],
                                        GEN_INSTRUMENT, inst_index, 24, phdr[-1])
    new_inst, ibag, imod, igen = tables([inst[i] for i in used_inst], [inst_zones[i] for i in used_inst],
                                        GEN_SAMPLE_ID, sample_index, 20, inst[-1])

    # Each kept sample's data points, with the points the spec puts after it, as they were.
    smpl, sm24, new_shdr = bytearray(), bytearray(), []
    points = len(b["smpl"]) // 2
    for s in used_samples:
        rec = shdr[s]
        start, end, loop0, loop1 = struct.unpack_from("<IIII", rec, 20)
        first, last = min(start, loop0), min(max(end, loop1) + SAMPLE_PAD, points)
        if not first <= start <= end <= last:
            raise Sf2Error(f"sample {s} lies outside smpl")
        base = len(smpl) // 2 - first
        smpl += b["smpl"][first * 2:last * 2]
        if b["sm24"] is not None:
            sm24 += b["sm24"][first:last]
        link = word(rec, 42)
        link = sample_index.get(link, 0) if word(rec, 44) & 0x7FFF not in (0, 1) else link
        new_shdr.append(rec[:20] + struct.pack("<IIII", start + base, end + base, loop0 + base, loop1 + base)
                        + rec[36:42] + struct.pack("<H", link) + rec[44:])
    new_shdr.append(shdr[-1])

    def chunk(cid, payload):
        payload = bytes(payload)
        return cid.encode("latin-1") + struct.pack("<I", len(payload)) + payload + (b"\0" if len(payload) & 1 else b"")

    def lst(kind, body):
        return chunk("LIST", kind.encode("latin-1") + body)

    sdta = chunk("smpl", smpl) + (chunk("sm24", sm24 + (b"\0" if len(sm24) & 1 else b"")) if b["sm24"] is not None else b"")
    pdta = b"".join(chunk(cid, b"".join(recs)) for cid, recs in
                    (("phdr", new_phdr), ("pbag", pbag), ("pmod", pmod), ("pgen", pgen), ("inst", new_inst),
                     ("ibag", ibag), ("imod", imod), ("igen", igen), ("shdr", new_shdr)))
    body = b"sfbk" + lst("INFO", b["INFO"]) + lst("sdta", sdta) + lst("pdta", pdta)
    return b"RIFF" + struct.pack("<I", len(body)) + body


def read_programs(path=PROGRAMS):
    wanted = []
    with open(path, encoding="utf-8") as f:
        for n, line in enumerate(f, 1):
            line = line.split("#", 1)[0].split()
            if not line:
                continue
            if len(line) != 2 or not all(x.isdigit() for x in line):
                sys.exit(f"{path}:{n}: expected 'bank program'")
            pair = (int(line[0]), int(line[1]))
            if pair in wanted:
                sys.exit(f"{path}:{n}: {pair} listed twice")
            wanted.append(pair)
    return wanted


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def source_path(source=None):
    return source or os.environ.get("REVENGE_SOUNDFONT_SOURCE") or DEFAULT_SOURCE


def fetch():
    """The pinned tarball into build-deps/soundfont, once; a download that doesn't match is refused."""
    if os.path.isfile(DEFAULT_SOURCE):
        with open(DEFAULT_SOURCE, "rb") as f:
            if sha256(f.read()) == SOURCE_SHA256:
                print(f"{os.path.relpath(DEFAULT_SOURCE, ROOT)}: present, matches its pin")
                return 0
    os.makedirs(os.path.dirname(DEFAULT_SOURCE), exist_ok=True)
    print(f"downloading {SOURCE_NAME} (135 MB) from {SOURCE_URL}")
    with urllib.request.urlopen(SOURCE_URL, timeout=600) as r:
        data = r.read()
    if sha256(data) != SOURCE_SHA256:
        sys.exit(f"{SOURCE_NAME}: SHA-256 {sha256(data)}, pinned {SOURCE_SHA256}: refused")
    partial = DEFAULT_SOURCE + ".part"
    with open(partial, "wb") as f:
        f.write(data)
    os.replace(partial, DEFAULT_SOURCE)
    print(f"{os.path.relpath(DEFAULT_SOURCE, ROOT)}: fetched, matches its pin")
    return 0


def source_files(source):
    """The full bank and the licence texts from the pinned tarball, each checked."""
    path = source_path(source)
    if not os.path.isfile(path):
        sys.exit(f"no soundfont source at {path}: run tools/deps/fetch_soundfont.sh (or subset.py fetch), "
                 "or set REVENGE_SOUNDFONT_SOURCE")
    with open(path, "rb") as f:
        tarball = f.read()
    if sha256(tarball) != SOURCE_SHA256:
        sys.exit(f"{path}: SHA-256 {sha256(tarball)}, pinned {SOURCE_SHA256}")
    with tarfile.open(fileobj=io.BytesIO(tarball)) as t:
        bank = t.extractfile(BANK_MEMBER).read()
        texts = {name: t.extractfile(f"fluid-soundfont-3.1/{name}").read() for name in LICENCE_TEXTS}
    if sha256(bank) != BANK_SHA256:
        sys.exit(f"FluidR3_GM.sf2: SHA-256 {sha256(bank)}, pinned {BANK_SHA256}")
    return bank, texts


def pinned_subset_hash():
    try:
        with open(SUBSET_HASH, encoding="utf-8") as f:
            return f.read().split()[0]
    except (OSError, IndexError):
        return None


def render_check(full, part, wanted, renderer):
    """Run the render test (tests/soundfont_render_test.cpp) on the full bank and the subset."""
    with tempfile.TemporaryDirectory() as work:
        paths = [os.path.join(work, n) for n in ("full.sf2", "subset.sf2")]
        for p, d in zip(paths, (full, part)):
            with open(p, "wb") as f:
                f.write(d)
        return subprocess.call([renderer] + paths + [f"{b}:{p}" for b, p in wanted]) == 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("mode", choices=("fetch", "generate", "check", "record"))
    ap.add_argument("--source", help=f"a copy of {SOURCE_NAME} (default REVENGE_SOUNDFONT_SOURCE, then build-deps/soundfont)")
    ap.add_argument("--out", help="generate: the subset file to write")
    ap.add_argument("--renderer", help="check: tests/soundfont_render_test, to prove the subset plays as the full bank")
    a = ap.parse_args()
    if a.mode == "fetch":
        return fetch()
    wanted = read_programs()
    full, texts = source_files(a.source)
    part = subset(full, wanted)
    digest, pinned = sha256(part), pinned_subset_hash()
    print(f"subset: {len(wanted)} presets, {len(part)} bytes, SHA-256 {digest}", flush=True)
    if a.mode == "record":
        with open(SUBSET_HASH, "w", encoding="utf-8") as f:
            f.write(f"{digest}  {SUBSET_NAME}\n")
        for name, path in LICENCE_TEXTS.items():
            with open(path, "wb") as f:
                f.write(texts[name])
        print(f"recorded {os.path.relpath(SUBSET_HASH, ROOT)} and the licence texts")
        return 0
    if digest != pinned:
        print(f"the subset hashes {digest}, data/music pins {pinned}: if the program list changed on purpose, "
              "run subset.py record and commit the new hash")
        return 1
    if a.mode == "generate":
        if not a.out:
            sys.exit("generate needs --out")
        os.makedirs(os.path.dirname(os.path.abspath(a.out)), exist_ok=True)
        with open(a.out + ".part", "wb") as f:
            f.write(part)
        os.replace(a.out + ".part", a.out)
        return 0
    ok = True
    for name, path in LICENCE_TEXTS.items():
        try:
            with open(path, "rb") as f:
                same = f.read() == texts[name]
        except OSError:
            same = False
        if not same:
            print(f"{os.path.relpath(path, ROOT)} is not the source's {name}, verbatim")
            ok = False
    if a.renderer:
        if render_check(full, part, wanted, a.renderer):
            print(f"all {len(wanted)} presets: the same regions and samples as in the full bank")
        else:
            print("the subset does not play as the full bank")
            ok = False
    print("the subset and its licence texts match the pinned source" if ok else "check failed")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
