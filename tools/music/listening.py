#!/usr/bin/env python3
"""The listening renders: every song, through the jukebox, to WAV files with a
one-page sheet for judging data/music/instruments.json. Nothing is played
aloud.

    tools/music/listening.py --jukebox BUILD/revenge_jukebox
        --soundfont BUILD/soundfont/FluidR3_GM-subset.sf2 --out DIR
        [--game GAME_DIR] [--only SONG ...]

The jukebox (tools/music/jukebox.cpp) plays each song through the game's own
sequencer and Roland driver into the music on the soundfont. Out, in DIR: one
SONG.wav each (48 kHz, 16-bit stereo) and LISTENING.md: each song's length,
peak and level, and every MT-32 timbre a note sounded with, with the General
MIDI program the table gives it (the soundfont's name for it). Entries of the
table whose note asks for listening are listed first. GAME_DIR defaults to
the REVENGE_GAME variable; without one, exit 77. Exit 1 if a song clips or
sounds a timbre the table doesn't list. Keep DIR outside the repo: the
renders are the game's music.
"""
import argparse, json, os, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
TABLE = os.path.join(ROOT, "data", "music", "instruments.json")
SKIP = 77


def label(timbre):
    """A timbre as the jukebox counts them: A0-63, B0-63, M0-63 (memory)."""
    return "%s%d" % ("ABM"[timbre // 64], timbre % 64)


def read_table(path):
    with open(path, encoding="utf-8") as f:
        table = json.load(f)
    timbres = {}
    for entry in table["timbres"]:
        group = {"a": 0, "b": 64, "memory": 128}[entry["group"]]
        timbres[group + entry["number"]] = entry
    rhythm = {entry["timbre"]: entry for entry in table["rhythm"]}
    return table, timbres, rhythm


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--jukebox", required=True)
    ap.add_argument("--soundfont", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--game", default=os.environ.get("REVENGE_GAME", ""))
    ap.add_argument("--table", default=TABLE)
    ap.add_argument("--only", nargs="*", default=[])
    a = ap.parse_args()
    if not a.game or not os.path.isdir(a.game):
        print("listening: skipped, no game folder (REVENGE_GAME)")
        return SKIP
    table, timbres, rhythm = read_table(a.table)
    os.makedirs(a.out, exist_ok=True)
    try:
        run = subprocess.run([a.jukebox, a.table, a.soundfont, a.game, a.out] + a.only,
                             capture_output=True, text=True, timeout=600)
    except subprocess.TimeoutExpired:
        sys.exit("listening: the jukebox ran past 10 minutes")
    if run.returncode:
        sys.exit("listening: the jukebox failed\n" + run.stderr[-2000:])
    songs = [json.loads(line) for line in run.stdout.splitlines() if line.strip()]
    if not songs:
        sys.exit("listening: the jukebox played no song")

    problems = []
    rows = []
    heard_timbres, heard_rhythm = set(), set()
    for song in songs:
        parts = []
        for t in song["timbres"]:
            heard_timbres.add(t["timbre"])
            entry = timbres.get(t["timbre"])
            if entry is None:
                problems.append("%s: timbre %s is not in the table (the fallback plays)" % (song["song"], label(t["timbre"])))
            mt32 = (" " + entry["mt32"]) if entry and "mt32" in entry else ""
            parts.append("%s%s -> %d %s" % (label(t["timbre"]), mt32, t["program"], t["name"]))
        for r in song["rhythm"]:
            heard_rhythm.add(r["timbre"])
            if r["timbre"] not in rhythm:
                problems.append("%s: rhythm timbre %d is not in the table" % (song["song"], r["timbre"]))
        drums = ", ".join("%d -> key %d" % (r["timbre"], r["key"]) for r in song["rhythm"])
        if song["peak"] >= 1.0:
            problems.append("%s: clips (peak %.3f)" % (song["song"], song["peak"]))
        if song["cut"]:
            problems.append("%s: cut at %.0f s" % (song["song"], song["seconds"]))
        rows.append("| %s | %.1f s | %.2f | %.1f dBFS | %s | %s |" % (
            song["song"], song["seconds"], song["peak"], song["rms_dbfs"], "; ".join(parts) or "-", drums or "-"))

    listen = []
    for timbre, entry in sorted(timbres.items()):
        if "listen" in entry.get("note", "") and timbre in heard_timbres:
            listen.append("- timbre %s plays program %d: %s" % (label(timbre), entry["program"], entry["note"]))
    for timbre, entry in sorted(rhythm.items()):
        if "listen" in entry.get("note", "") and timbre in heard_rhythm:
            listen.append("- rhythm timbre %d plays key %d: %s" % (timbre, entry["key"], entry["note"]))

    lines = [
        "# Listening sheet: the music",
        "",
        "Each song from the start to its end, through the game's own sequencer and Roland driver at the",
        "original's rates, into the music on the soundfont (tools/music/jukebox.cpp), from the MT-32 as it",
        "powers on. What to judge: data/music/instruments.json, the General MIDI program that stands in",
        "for each MT-32 timbre (and the drum key for each rhythm timbre), and the level (gain_db %.1f)." % table["gain_db"],
        "",
        "## To listen for",
        "",
    ] + (listen or ["- nothing flagged"]) + [
        "",
        "## The songs",
        "",
        "| Song | Length | Peak | Level | Timbre -> program | Rhythm timbre -> key |",
        "| --- | --- | --- | --- | --- | --- |",
    ] + rows + [""]
    if problems:
        lines += ["## Problems", ""] + ["- " + p for p in problems] + [""]
    with open(os.path.join(a.out, "LISTENING.md"), "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines))
    for song in songs:
        print("listening: %s %.1f s, peak %.2f, %d timbres" % (song["song"], song["seconds"], song["peak"], len(song["timbres"])))
    for p in problems:
        print("listening: " + p)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
