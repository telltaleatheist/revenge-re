#!/usr/bin/env python3
"""Records what the game asks to be heard, and checks it against the goldens.

    tools/trace/audio.py check  --exe BUILD/Revenge [--game GAME_DIR] [--only NAME ...]
        [--devices card speaker] [--keep DIR]
    tools/trace/audio.py record --exe BUILD/Revenge [--game GAME_DIR] [--only NAME ...]
        [--devices card speaker] [--keep DIR]

Each key script (tools/trace/scripts with a golden trace, and
tools/trace/audio/scripts) runs once per device setting with the audio log on
(REVENGE_AUDIO_LOG, revenge/audio_log.h): "card" is music on the MT-32 path
and effects on the Sound Blaster, "speaker" both on the speaker. The log is
cut into sections at its snaps (what follows the last snap is the shutdown),
and each section is kept as its line count, its count of each kind of line
and the SHA-256 of its text: tools/trace/audio/traces/NAME.DEVICE.jsonl.
The goldens hold no note or byte of the game's, only counts and digests.
--keep DIR leaves each run's whole log there (outside the repo) to compare
by hand.

GAME_DIR defaults to the REVENGE_GAME variable; without one, exit 77
(ctest's skip). check exits 0 when every section matches.
"""
import argparse
import collections
import hashlib
import json
import os
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import record  # noqa: E402

SCRIPTS = os.path.join(HERE, "scripts")
GOLDEN_TRACES = os.path.join(HERE, "traces")
AUDIO_SCRIPTS = os.path.join(HERE, "audio", "scripts")
AUDIO_TRACES = os.path.join(HERE, "audio", "traces")
SKIP = 77

DEVICES = {
    "card": "music = mt-32\nsound = sound-blaster 220\n",
    "speaker": "music = pc-speaker\nsound = pc-speaker\n",
}


def settings(device):
    return "video = vga\n" + DEVICES[device] + "input = keys\n"


def scripts():
    """Every script the audio goldens cover, by name."""
    found = {}
    for name in os.listdir(GOLDEN_TRACES):
        if name.endswith(".jsonl"):
            found[name[:-6]] = os.path.join(SCRIPTS, name[:-6] + ".keys")
    for name in os.listdir(AUDIO_SCRIPTS):
        if name.endswith(".keys"):
            found[name[:-5]] = os.path.join(AUDIO_SCRIPTS, name)
    return dict(sorted(found.items()))


def kind(line):
    parts = line.split()
    if len(parts) < 2:
        return "?"
    if parts[1] in ("midi", "pcm") and len(parts) > 2:
        if parts[1] == "midi" and parts[2] not in ("reset", "--"):
            status = int(parts[2], 16)
            return "midi " + (f"{status & 0xF0:02X}" if status < 0xF0 else f"{status:02X}")
        return parts[1] + " " + parts[2]
    return parts[1]


def sections(text):
    """The log's sections, one per snap, as golden records."""
    records = []
    current = []
    for line in text.splitlines():
        if not line.strip():
            continue
        current.append(line)
        parts = line.split()
        if len(parts) >= 3 and parts[1] == "snap":
            body = "\n".join(current) + "\n"
            records.append({
                "snap": parts[2],
                "lines": len(current),
                "kinds": dict(sorted(collections.Counter(kind(l) for l in current).items())),
                "digest": hashlib.sha256(body.encode("ascii")).hexdigest()[:16],
            })
            current = []
    return records


def run(args, name, script, device, work):
    """Runs one script on one device; returns its sections, or stops naming why."""
    wanted = record.start_save(script)
    if wanted and wanted[1] is None:
        return None
    log = os.path.join(work, "audio.log")
    code, output = record.run(args.exe, args.game, script, work, settings=settings(device),
                              env_extra={"REVENGE_AUDIO_LOG": os.path.abspath(log)})
    if code != 0 or not os.path.exists(os.path.join(work, "done.txt")):
        last = [line for line in output.splitlines() if line.startswith(("harness:", "fault:"))]
        sys.exit(f"audio: {name}.{device} didn't run to its end (exit {code})"
                 + (f": {last[-1]}" if last else ""))
    with open(log, encoding="ascii") as f:
        text = f.read()
    if args.keep:
        os.makedirs(args.keep, exist_ok=True)
        shutil.copyfile(log, os.path.join(args.keep, f"{name}.{device}.log"))
    return sections(text)


def compare(label, golden, candidate):
    if [r["snap"] for r in golden] != [r["snap"] for r in candidate]:
        return [f"{label}: snaps {[r['snap'] for r in candidate]}, golden {[r['snap'] for r in golden]}"]
    problems = []
    for want, got in zip(golden, candidate):
        if want != got:
            kinds = sorted(set(want["kinds"]) | set(got["kinds"]))
            changed = [f"{k} {want['kinds'].get(k, 0)}->{got['kinds'].get(k, 0)}" for k in kinds
                       if want["kinds"].get(k, 0) != got["kinds"].get(k, 0)]
            problems.append(f"{label} @{want['snap']}: "
                            + (", ".join(changed) if changed else "same counts, other lines"))
    return problems


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("action", choices=("check", "record"))
    parser.add_argument("--exe", required=True)
    parser.add_argument("--game", default=os.environ.get("REVENGE_GAME", ""))
    parser.add_argument("--only", nargs="*")
    parser.add_argument("--devices", nargs="*", choices=sorted(DEVICES), default=sorted(DEVICES))
    parser.add_argument("--keep")
    parser.add_argument("--work")
    args = parser.parse_args()
    if not (args.game and os.path.isdir(args.game)):
        print("audio: no game folder (REVENGE_GAME); skipped")
        return SKIP
    names = scripts()
    if args.only:
        unknown = sorted(set(args.only) - set(names))
        if unknown:
            sys.exit(f"audio: no script {', '.join(unknown)}")
        names = {n: p for n, p in names.items() if n in args.only}
    with tempfile.TemporaryDirectory(prefix="revenge-audio-") as temporary:
        work = args.work or temporary
        problems = []
        for name, script in names.items():
            for device in args.devices:
                label = f"{name}.{device}"
                found = run(args, name, script, device, os.path.join(work, label))
                if found is None:
                    print(f"audio: {label}: skipped, no start save")
                    continue
                path = os.path.join(AUDIO_TRACES, label + ".jsonl")
                if args.action == "record":
                    os.makedirs(AUDIO_TRACES, exist_ok=True)
                    with open(path, "w", encoding="ascii", newline="\n") as f:
                        for section in found:
                            f.write(json.dumps(section, sort_keys=True) + "\n")
                    print(f"audio: {label}: recorded {len(found)} sections, "
                          f"{sum(s['lines'] for s in found)} lines")
                    continue
                if not os.path.exists(path):
                    problems.append(f"{label}: no golden")
                    continue
                with open(path, encoding="ascii") as f:
                    golden = [json.loads(line) for line in f if line.strip()]
                found_problems = compare(label, golden, found)
                print(f"audio: {label}: " + ("matches" if not found_problems
                                              else f"{len(found_problems)} difference(s)")
                      + f" ({sum(s['lines'] for s in golden)} lines)")
                problems += found_problems
        for problem in problems:
            print(problem)
        return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
