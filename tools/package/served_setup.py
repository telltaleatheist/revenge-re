"""The setup served to the launcher (docs/RELEASING.md, "The setup served to the launcher"), asked of a built or
packaged program as the launcher asks it: stdin and stdout are pipes. Every run has its own player's folder
(REVENGE_SAVE_DIRECTORY) and home under `scratch`, so the search finds nothing of the machine's. A copy of the game
can't be made up (REVENGE.EXE is known by its bytes), so the checks that need one take the game's folder, and are named
as skipped without it. Nothing of the game is kept; names and counts only are printed.

tests/status_setup_test.py runs these on the build, and tools/build_package.py on each package.
"""
import json
import os
from pathlib import Path
import shutil
import subprocess

SECONDS = 120
HOOKS = ("REKIT_SETUP_", "REVENGE_SETUP_", "REVENGE_HARNESS_")


def asking_env(env, user):
    """`env` with the player's folder `user` and a home of its own, no game folder named, no window and no sound."""
    out = {k: v for k, v in env.items() if not k.startswith(HOOKS) and k != "REVENGE_ASSET_DIRECTORY"}
    home = str(Path(user).parent / "home")
    out.update(REVENGE_SAVE_DIRECTORY=str(user), SDL_VIDEO_DRIVER="dummy", SDL_AUDIO_DRIVER="dummy",
               SDL_MAC_BACKGROUND_APP="1", HOME=home, APPDATA=home, XDG_DATA_HOME=os.path.join(home, "share"))
    return out


class Answer:
    """What one ask answered: its exit, its stdout's lines as JSON (None for a line that isn't), its stderr."""

    def __init__(self, code, stdout, stderr):
        self.code, self.stderr = code, stderr
        self.lines = []
        for line in stdout.splitlines():
            try:
                self.lines.append(json.loads(line))
            except ValueError:
                self.lines.append(None)
        self.last = self.lines[-1] if self.lines and isinstance(self.lines[-1], dict) else {}

    def clean(self):
        """Every line a JSON object, and only the last one done."""
        return bool(self.lines) and all(isinstance(m, dict) for m in self.lines) and \
            [bool(m.get("done")) for m in self.lines] == [False] * (len(self.lines) - 1) + [True]

    def refused(self):
        return self.clean() and len(self.lines) == 1 and self.code == 2 and not self.last.get("ok") and \
            bool(self.last.get("summary"))

    def __str__(self):
        return "exit %s, %d lines" % (self.code, len(self.lines))


def ask(program, args, env):
    p = subprocess.run([str(program)] + [str(a) for a in args], env=env, input="", capture_output=True, text=True,
                       timeout=SECONDS)
    return Answer(p.returncode, p.stdout, p.stderr)


def tree(folder):
    folder = Path(folder)
    if not folder.exists():
        return []
    return sorted(str(p.relative_to(folder)) for p in folder.rglob("*"))


def same(a, b):
    return bool(a) and bool(b) and os.path.realpath(str(a)).rstrip("/") == os.path.realpath(str(b)).rstrip("/")


def remembered(user):
    """setup.txt's game_folder, as the program writes it."""
    try:
        for line in (Path(user) / "setup.txt").read_text().splitlines():
            key, _, value = line.partition("=")
            if key.strip() == "game_folder":
                return value.strip()
    except OSError:
        pass
    return ""


def checks(program, env, scratch, game=None):
    """(passed, what) for each check; passed None for one skipped without the game."""
    scratch = Path(scratch)

    def user_env(name):
        return asking_env(env, scratch / name)

    def status(name):
        a = ask(program, ["--status"], user_env(name))
        return a, (a.lines[0] if len(a.lines) == 1 and isinstance(a.lines[0], dict) else {})

    a, st = status("empty")
    yield a.code == 0 and st.get("kit") == 1 and st.get("game") == "chr", "--status: one JSON line, kit 1, game chr (%s)" % a
    yield st.get("setup") == 1 and st.get("play") == 1, "--status: the setup is served and the start options taken"
    yield st.get("files", {}).get("state") == "missing", "--status: nothing set up, missing"
    yield "story import" in st.get("first_run", {}).get("pending", []), "--status: the story's import pending"
    d = ask(program, ["--setup", "--describe"], user_env("empty"))
    yield d.clean() and len(d.lines) == 1 and d.code == 0 and d.last.get("ok") and \
        d.last.get("title") == "BattleTech: The Crescent Hawks' Revenge", "describe: the find screen's words (%s)" % d
    yield d.last.get("into", "").endswith("install"), "describe: where it would go"
    s = ask(program, ["--setup", "--search"], user_env("empty"))
    yield s.clean() and s.code == 0 and s.last.get("ok") and not s.last.get("path"), "search: nothing in a home of its own (%s)" % s
    c = ask(program, ["--setup", "--copies"], user_env("empty"))
    yield c.clean() and len(c.lines) == 1 and c.code == 0 and c.last.get("ok"), "copies: none (%s)" % c
    yield not (scratch / "empty").exists(), "break: --status, --describe, --search and --copies make nothing in the player's folder"

    for bad in (["--look"], ["--install"], ["--describe", "x"], ["--copies", "x"], ["--install", "x", "--folder"],
                ["--look", "x", "y"]):
        a = ask(program, ["--setup"] + bad, user_env("bad"))
        yield a.refused(), "break: --setup %s: one line saying why, exit 2 (%s)" % (" ".join(bad), a)
    for other in (["--add", "x"], ["--first-run", "story import"]):
        a = ask(program, ["--setup"] + other, user_env("bad"))
        yield a.refused() and "not a setup this game answers" in a.last.get("summary", ""), \
            "%s: not a setup this game answers (%s)" % (other[0], a)
    yield not (scratch / "bad").exists(), "break: nor do wrong arguments"
    hooked = dict(asking_env(env, scratch / "hook"), REVENGE_SETUP_PICK="x")
    del hooked["REVENGE_SAVE_DIRECTORY"]
    a = ask(program, ["--setup", "--search"], hooked)
    yield a.refused() and "REVENGE_SETUP_PICK" in a.last.get("summary", ""), "break: a test's hook without its own folder: refused (%s)" % a

    notgame = scratch / "notgame"
    notgame.mkdir(parents=True, exist_ok=True)
    (notgame / "README.TXT").write_text("not a game\n")
    a = ask(program, ["--setup", "--look", notgame], user_env("look"))
    yield a.clean() and a.code == 2 and "No copy of" in a.last.get("summary", ""), "look: not the game (%s)" % a
    other = scratch / "other"
    other.mkdir(parents=True, exist_ok=True)
    (other / "REVENGE.EXE").write_bytes(b"MZ" + bytes(200))
    a = ask(program, ["--setup", "--look", other], user_env("look"))
    yield a.clean() and a.code == 2 and "an edition this program doesn't know" in a.last.get("summary", ""), \
        "look: a REVENGE.EXE of another edition (%s)" % a
    a = ask(program, ["--setup", "--install", other], user_env("look"))
    yield a.clean() and a.code == 2 and not a.last.get("ok"), "break: install: not the game (%s)" % a
    yield tree(scratch / "look") == ["setup.log"], "break: nothing made in the player's folder but the log"

    if not game:
        for part in ("look: the game", "install: copied and remembered", "install: --status ready there",
                     "copies: the install, in use", "look: the install in place"):
            yield None, part
        return
    copy = scratch / "copy"
    shutil.copytree(str(game), str(copy))
    before = tree(copy)
    user = scratch / "user"
    install = user / "install"
    a = ask(program, ["--setup", "--look", copy], user_env("user"))
    yield a.clean() and a.code == 0 and a.last.get("ok") and not a.last.get("in_place") and same(a.last.get("path"), copy) and \
        "(1.00)" in a.last.get("words", "") and same(a.last.get("into"), install), "look: the game (%s)" % a
    a = ask(program, ["--setup", "--install", copy], user_env("user"))
    yield a.clean() and a.code == 0 and a.last.get("ok") and same(a.last.get("where"), install) and \
        (install / "REVENGE.EXE").is_file() and tree(copy) == before and same(remembered(user), install), \
        "install: copied and remembered, the original left as it was (%s)" % a
    _, st = status("user")
    yield st.get("files", {}).get("state") == "ready" and same(st["files"].get("where"), install) and \
        st["files"].get("data") == "1.00", "install: --status ready there"
    c = ask(program, ["--setup", "--copies"], user_env("user"))
    # The copy it was made from is on this computer too (beside the program, for the search): listed, not in use.
    rows = [m for m in c.lines if isinstance(m, dict) and m.get("copy")]
    mine = [m for m in rows if same(m["copy"], install)]
    yield c.clean() and c.code == 0 and len(mine) == 1 and mine[0].get("in_use") and mine[0].get("data") == "1.00" and \
        not mine[0].get("missing") and sum(1 for m in rows if m.get("in_use")) == 1, "copies: the install, in use (%s)" % c
    a = ask(program, ["--setup", "--look", install], user_env("user"))
    yield a.clean() and a.code == 0 and a.last.get("in_place") and same(a.last.get("where"), install), \
        "look: the install in place (%s)" % a
    shutil.rmtree(str(copy), ignore_errors=True)
