"""Build and smoke-test a redistributable package, without original game data (docs/RELEASING.md).

    build_package.py --label LABEL [--game GAME_DIR] [--installer] [--appimage-tools DIR]
    build_package.py --smoke PACKAGE_DIR [--game GAME_DIR] [--arch x86_64|arm64]

LABEL is a release's tag (chr-vX.Y.Z, made only from that tag, whose X.Y.Z is VERSION's) or any other name for a
test build. On Windows it makes CHR-Rebuild-LABEL-win64.zip (and with --installer the Inno Setup installer,
CHR-Rebuild-LABEL-Setup.exe); on a Mac the universal app and CHR-Rebuild-LABEL-mac.dmg (macOS 12 and later), signed
and notarized through re-setup-kit's tools/macos/sign_notarize.sh, or signed ad hoc with RE_ALLOW_UNSIGNED=1 for a
build not for release; on Linux CHR-Rebuild-LABEL-linux-x86_64.tar.gz and CHR-Rebuild-LABEL-x86_64.AppImage (with
--appimage-tools, the folder tools/deps/get_appimage_tools.sh fills). Each gets a .sha256 beside it;
tools/package/sums.py writes the release's SHA256SUMS over them all.

A package holds the program (chr), our data/ (rules, layout, the new campaign, the speaker's notes, the music and its
soundfont, which the build cuts), the read-me and the notices; never a file of the original game. The smoke test runs
the packaged program: --help prints its usage, it answers the launcher's asks about the setup
(tools/package/served_setup.py), and with a game folder (--game, or REVENGE_GAME) a first run from an empty save
folder imports the story and reaches the game's first input wait (the trace harness's first snap). Nothing is shown:
no window, no sound, and on a Mac the app runs as a background copy (tools/package/background_app.py). --smoke runs
the same checks on a package already made (a folder, an extracted zip or tarball, an app, an extracted AppImage's
usr/bin).
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import plistlib
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
import time
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parent / "package"))
import background_app  # noqa: E402
import served_setup  # noqa: E402

STEM = "CHR-Rebuild"
APP_NAME = "Revenge Rebuild"
PROGRAM = "chr"  # the shipped program (the build's target stays Revenge, which the tools and traces run)
BUNDLE_ID = "io.github.revengere.chr"  # its own, so it never replaces another game's settings
FIRST_RUN_SECONDS = 120
TITLE_SECONDS = 60
# Where the smoke keeps the title's captures (--shots); None: each is checked and dropped. Never in the repo: it is the
# player's game on the screen.
SHOTS = None
# Our data beside the program: each folder must hold JSON files.
DATA_FOLDERS = ("data/rules", "data/ui", "data/start", "data/sound", "data/music")
MAC_ARCHITECTURES = ("x86_64", "arm64")
SIGNER = "extern/re-setup-kit/tools/macos/sign_notarize.sh"

README = """Revenge Rebuild

A rebuild of BattleTech: The Crescent Hawks' Revenge (1990)'s program. Your own copy of
the DOS game is required and is not included: no game data or saves are in this package.

It plays the US DOS release 1.00. The first time it starts without your game, it sets up a
copy it finds on your computer (showing first anything wrong with it), or else downloads
release 1.00; if that can't be done, it asks for your copy: its REVENGE.EXE, any file in
its folder, or the archive it came in (a .zip, for example). The launcher can do this on
its own screens too. The game is copied into your own folder and played from there; your
copy is never changed.
Your settings (settings.txt, setup.txt), saves, the story imported from your copy and the
copied game live in your own folder:
  Windows: %APPDATA%\\BattleTech\\Revenge Rebuild
  macOS:   ~/Library/Application Support/BattleTech/Revenge Rebuild
  Linux:   ~/.local/share/BattleTech/Revenge Rebuild

The game options menu has an UPGRADES line: optional fixes for the original's bugs, all
off to start with, each explained on its page.

While playing, Shift+Alt+F1 (Cmd+Shift+F1 on a Mac) opens the setup menu: your game files,
updates, reporting a problem (revenge.log, in your folder, is the log to send), checking
the files, and resetting the settings. The game waits while it is open.

From a command line: chr --setup sets the game up again; chr --help lists the rest.
{first_open}
The Crescent Hawks' Revenge (1990) was developed by Westwood Associates and published by
Infocom. This rebuild is a fan project, not affiliated with or endorsed by any of them or
by any holder of rights in the game. It is built on Elthial's Crescent Hawks' Revenge SDL
port. LICENSE and the licenses folder say what covers it and what doesn't.
"""
# The Mac's first open, by how the app is signed (sign_notarize.sh check).
FIRST_OPEN = {
    "notarize": "",
    "unsigned": "\nThis is a test build, not notarized: the first time, open it with right-click (or\n"
                "Control-click) and Open, then Open again.\n",
}


def run(*args, **kwargs):
    return subprocess.run([str(a) for a in args], check=True, **kwargs)


def platform_name():
    if os.name == "nt":
        return "win64"
    if sys.platform == "darwin":
        return "mac"
    return "linux"


def release_tag(label):
    """The tag stamped in the program for the launcher (src/Setup/BuildTag.cpp): the label when it is a release's, else ""."""
    return label if re.fullmatch(r"chr-v\d+\.\d+\.\d+", label, re.ASCII) else ""


def project_version(root):
    return (root / "VERSION").read_text(encoding="utf-8").strip()


def check_version(root, label):
    """A release's tag names the version the program reports (VERSION, CMake's PROJECT_VERSION)."""
    tag = release_tag(label)
    if tag and tag[len("chr-v"):] != project_version(root):
        raise RuntimeError(f"{tag} is not VERSION's {project_version(root)}: change VERSION first")


def check_stamp(binary, label):
    """The launcher reads a program's version from this string in its file: a release holds its tag, no other build any."""
    stamps = set(re.findall(rb"MW3-PROGRAM-VERSION:([^\0]*)\0", Path(binary).read_bytes()))
    wanted = {release_tag(label).encode()} if release_tag(label) else set()
    if stamps != wanted:
        raise RuntimeError(f"The program's version stamp is {sorted(stamps)}, wanted {sorted(wanted)}")


def check_release_tree(root, label):
    """A release's package is made from its tag and nothing else: the tree is at that tag, with no file changed,
    added or missing (the submodules' too). Any other label packages whatever is there."""
    tag = release_tag(label)
    if not tag:
        return

    def git(*args):
        try:
            return subprocess.run(["git", "-C", str(root), *args], capture_output=True, text=True, check=True).stdout.strip()
        except (OSError, subprocess.CalledProcessError) as e:
            why = (getattr(e, "stderr", None) or str(e)).strip()
            raise RuntimeError(f"{tag} can only be packaged from its tag in a git tree: {why}") from None

    if Path(git("rev-parse", "--show-toplevel")).resolve() != Path(root).resolve():
        raise RuntimeError(f"{tag} can only be packaged from its tag in a git tree: {root} is not the top of one")
    head, tagged = git("rev-parse", "HEAD"), git("rev-parse", "--verify", f"refs/tags/{tag}^{{commit}}")
    if head != tagged:
        raise RuntimeError(f"{tag} is {tagged[:9]}, but the tree is at {head[:9]}")
    changed = git("status", "--porcelain", "--untracked-files=normal").splitlines()
    if changed:
        raise RuntimeError(f"{tag} can't be packaged from a tree with changes: " + "; ".join(line.strip() for line in changed[:5])
                           + (" ..." if len(changed) > 5 else ""))


def mac_signing():
    """What kind of Mac build this is, before anything is built: "notarize" or "unsigned" (the kit refuses an unsigned
    one unless RE_ALLOW_UNSIGNED=1)."""
    root = Path(__file__).resolve().parents[1]
    done = subprocess.run([str(root / SIGNER), "check"], capture_output=True, text=True)
    if done.returncode != 0:
        raise RuntimeError(done.stderr.strip() or "sign_notarize.sh check refused this build")
    if done.stderr.strip():
        print(done.stderr.strip())
    return done.stdout.strip()


def sign_app(root, app):
    """The app's signature, the kit's `app`: with a Developer ID every Mach-O, then the app, with the hardened runtime and
    a timestamp; ad hoc for a build not for release. The kit then checks it (codesign --verify --deep --strict). Who
    signs comes from the environment alone (RE_SIGN_IDENTITY), never from here."""
    run(root / SIGNER, "app", app, "--entitlements", root / "tools/package/chr.entitlements")


def notarize_dmg(root, dmg, app):
    """The kit's `dmg`: the dmg signed and, with a Developer ID, notarized (refused unless Apple accepts it), its ticket
    stapled to it and to the app, and Gatekeeper's verdict checked. Stapling changes the dmg, so its digest comes
    after."""
    run(root / SIGNER, "dmg", dmg, app)


def configure_and_build(root, build, label):
    args = ["cmake", "-S", root, "-B", build, "-DCHR_TRIM_PATHS=ON", f"-DCHR_RELEASE_TAG={release_tag(label)}",
            "-DBUILD_TESTING=OFF"]
    if os.name == "nt":
        args += ["-A", "x64", "-DCHR_WINDOWS_GUI=ON"]
    else:
        args += ["-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release"]
    run(*args)
    run("cmake", "--build", build, "--config", "Release", "--target", "Revenge", "--parallel", "4")
    return build / "Release/Revenge.exe" if os.name == "nt" else build / "Revenge"


def sdl_source(build):
    source = build / "_deps/sdl3-src"
    if not (source / "LICENSE.txt").is_file():
        raise RuntimeError(f"SDL's source is not in {source}: the build fetches it (CHR_FETCH_SDL)")
    return source


# libarchive, zlib and liblzma, linked in statically (tools/deps/build_archive.sh, build_archive.cmd put their licences
# in <prefix>/share/licenses): <folder>/<file> -> the name in licenses/.
ARCHIVE_LICENCES = {"libarchive-3.7.9/COPYING": "libarchive-COPYING", "zlib-1.3.1/LICENSE": "zlib-LICENSE",
                    "xz-5.8.1/COPYING": "xz-COPYING", "xz-5.8.1/COPYING.0BSD": "xz-COPYING.0BSD"}


def archive_licences(build, notices):
    """The archive libraries' licences into `notices`. A build without the pinned static libarchive (none, or a
    system one) is refused: the setup could not read a download, and its licences would be missing."""
    cache = build / "CMakeCache.txt"
    lib = ""
    for line in cache.read_text(encoding="utf-8", errors="replace").splitlines() if cache.is_file() else []:
        if line.startswith("REKIT_LIBARCHIVE_LIB:"):
            lib = line.split("=", 1)[1].strip()
    if not lib or lib.endswith("-NOTFOUND") or not re.search(r"\.(a|lib)$", lib):
        raise RuntimeError("Missing the static libarchive: build it first (tools/deps/build_archive.sh mac|linux, "
                           "tools/deps/build_archive.cmd) so the setup reads downloads")
    licences = Path(lib).parent.parent / "share/licenses"
    for source, name in ARCHIVE_LICENCES.items():
        if not (licences / source).is_file():
            raise RuntimeError(f"Missing licence: {licences / source} (libarchive {lib} is not the pinned static build)")
        shutil.copy2(licences / source, notices / name)


# The music soundfont the build cuts from FluidR3 GM (tools/soundfont/subset.py), and its licence texts.
SOUNDFONT = "FluidR3_GM-subset.sf2"
SOUNDFONT_LICENCES = ("FluidR3_GM-COPYING.txt", "FluidR3_GM-README.txt")


def stage_data(root, build, resources, notices):
    """Our data/ into `resources`, with the build's soundfont in data/music (refused unless it hashes as
    data/music/soundfont_subset.sha256 pins) and the setup screen's badge (the package icon) in data/setup."""
    for folder in DATA_FOLDERS:
        if not list((root / folder).glob("*.json")):
            raise RuntimeError(f"Missing game data: {folder}/*.json")
    shutil.copytree(root / "data", resources / "data")
    bank = build / "soundfont" / SOUNDFONT
    if not bank.is_file():
        raise RuntimeError(f"Missing soundfont: {bank} (fetch its source once: tools/deps/fetch_soundfont.sh)")
    pinned = (root / "data/music/soundfont_subset.sha256").read_text(encoding="utf-8").split()[0]
    digest = hashlib.sha256(bank.read_bytes()).hexdigest()
    if digest != pinned:
        raise RuntimeError(f"Soundfont {bank} hashes {digest}, data/music/soundfont_subset.sha256 pins {pinned}")
    shutil.copy2(bank, resources / "data/music" / SOUNDFONT)
    for name in SOUNDFONT_LICENCES:
        shutil.copy2(root / "data/music" / name, notices / name)
    (resources / "data/setup").mkdir()
    shutil.copy2(root / "tools/package/revenge.png", resources / "data/setup/badge.png")


# What every package holds the licence or notice of, in its licenses/ folder.
NOTICES = ("NOTICE.md", "THIRD_PARTY_NOTICES.md", "SDL-LICENSE.txt", "hidapi-LICENSE-bsd.txt", "re-setup-kit-LICENSE",
           "re-setup-kit-OFL.txt", "stb-LICENSE", "miniz-LICENSE", "monocypher-LICENCE.md", "TinySoundFont-LICENSE")


def check_notices(licenses):
    """The notices are in the finished package itself (`licenses`, its folder), not only staged for it."""
    wanted = [*NOTICES, *ARCHIVE_LICENCES.values(), *SOUNDFONT_LICENCES, "project"]
    missing = [n for n in wanted if not (licenses / n).exists() or (licenses / n).is_file() and not (licenses / n).stat().st_size]
    if missing:
        raise RuntimeError("Missing notices in the package: " + ", ".join(missing))


def stage_common(root, build, resources, first_open=""):
    """Our data, the read-me and the notices, into `resources` (beside the program, or a Mac app's
    Contents/Resources)."""
    sdl = sdl_source(build)
    shutil.copy2(root / "LICENSE", resources / "LICENSE")
    notices = resources / "licenses"
    notices.mkdir()
    shutil.copytree(root / "LICENSES", notices / "project")
    stage_data(root, build, resources, notices)
    for source in [root / "NOTICE.md", root / "THIRD_PARTY_NOTICES.md"]:
        shutil.copy2(source, notices / source.name)
    archive_licences(build, notices)
    # SDL, linked in, and hidapi, which SDL compiles in under its BSD-style licence; the setup kit and its own
    # third-party code (re-setup-kit THIRD-PARTY.md), with the SIL Open Font License of the Saira Condensed fonts its
    # screens draw with (the kit's REKIT_NOTICE_FILES); the music's synthesiser. A package without them is refused.
    kit = root / "extern/re-setup-kit"
    for name, source in [("SDL-LICENSE.txt", sdl / "LICENSE.txt"), ("hidapi-LICENSE-bsd.txt", sdl / "src/hidapi/LICENSE-bsd.txt"),
                         ("re-setup-kit-LICENSE", kit / "LICENSE"), ("re-setup-kit-OFL.txt", kit / "assets/OFL.txt"),
                         ("stb-LICENSE", kit / "third_party/stb/LICENSE"),
                         ("miniz-LICENSE", kit / "third_party/miniz/LICENSE"),
                         ("monocypher-LICENCE.md", kit / "third_party/monocypher/LICENCE.md"),
                         ("TinySoundFont-LICENSE", root / "extern/TinySoundFont/LICENSE")]:
        if not source.is_file():
            raise RuntimeError(f"Missing licence: {source}")
        shutil.copy2(source, notices / name)
    (resources / "README.txt").write_text(README.format(first_open=first_open), encoding="utf-8")
    shutil.copy2(root / "config" / KIT_SETTINGS, resources / KIT_SETTINGS)


# The setup kit's settings for this game (re-setup-kit DESIGN.md, Settings; config/): read from MenuHooks::program_dir,
# so beside the program, in a Mac app's Contents/Resources and at an AppImage's AppDir top, and in the files record.
KIT_SETTINGS = "setup-kit-settings.json"


def check_kit_settings(folder):
    """`folder`'s copy of the kit's settings: there, read as JSON, with this game's report page and catalogs."""
    try:
        values = json.loads((folder / KIT_SETTINGS).read_text(encoding="utf-8"))
    except (OSError, ValueError) as e:
        raise RuntimeError(f"{folder / KIT_SETTINGS}: {e}") from None
    if "revenge-re" not in values.get("report_url", "") or len(values.get("catalogs", [])) != 2:
        raise RuntimeError(f"{folder / KIT_SETTINGS}: not this game's report page and catalogs")


# CHECK MY FILES' record of the program (re-setup-kit include/rekit/manifest.h): sha256sum's format, at the top of
# the folder it covers. The program reads it from MenuHooks::program_dir (src/SDL/kit_menu.cpp): the program's own
# folder; a Mac app's Contents/Resources, as codesign refuses a plain file at the top of Contents and the signature
# seals Info.plist and the Mach-Os anyway; an AppImage's AppDir.
RECORD = "rebuild-files.sha256"


def write_record(root, folder):
    """The record of `folder`, by the kit's tools/files_manifest.py: on the finished folder, before anything seals it
    (the zip, the Setup, the tarball, the AppImage, codesign)."""
    run(sys.executable, "-I", root / "extern/re-setup-kit/tools/files_manifest.py", folder)


def check_record(folder):
    """`folder`'s record, as the package holds it: every file named, each as it is now, and no other."""
    named = {}
    for line in (folder / RECORD).read_text(encoding="utf-8").splitlines():
        digest, name = line.split("  ", 1)
        named[name] = digest
    present = {p.relative_to(folder).as_posix() for p in folder.rglob("*")
               if p.is_file() and not p.is_symlink() and p.name != RECORD}
    if set(named) != present:
        raise RuntimeError(f"{folder / RECORD}: names {sorted(set(named) - present)[:5]} not there, "
                           f"leaves out {sorted(present - set(named))[:5]}")
    changed = [n for n, d in named.items() if hashlib.sha256((folder / n).read_bytes()).hexdigest() != d]
    if changed:
        raise RuntimeError(f"{folder / RECORD}: changed since it was made: {changed[:5]}")


def check_paths(root, folder, *extra):
    """No local path, user name or file of the game in anything shipped (tools/package/check_paths.py), nor the folders
    the build ran in (`extra`, which __FILE__ would name)."""
    run(sys.executable, "-I", root / "tools/package/check_paths.py", folder, *[str(Path(p).resolve()) for p in extra])


def write_digest(file):
    digest = hashlib.sha256(file.read_bytes()).hexdigest()
    # Bytes: text would end in CRLF on Windows, and `sha256sum -c` elsewhere would look for a name ending in CR.
    (file.parent / (file.name + ".sha256")).write_bytes(f"{digest}  {file.name}\n".encode("utf-8"))


# ---- the smoke test


def resources_of(executable):
    """Where the program finds data/: beside it, or a Mac app's Contents/Resources."""
    if executable.parent.name == "MacOS" and executable.parent.parent.name == "Contents":
        return executable.parent.parent / "Resources"
    return executable.parent


def quiet_env(saves):
    """No window, no sound, never the foreground, and the player's folder `saves`: never the real one."""
    keep = {k: v for k, v in os.environ.items()
            if not k.startswith(("REVENGE_", "REKIT_SETUP_", "SDL_VIDEO", "SDL_AUDIO"))}
    return dict(keep, SDL_VIDEO_DRIVER="dummy", SDL_AUDIO_DRIVER="dummy", SDL_MAC_BACKGROUND_APP="1",
                RE_KIT_TEST="1", REVENGE_SAVE_DIRECTORY=str(saves))


def check_usage(executable):
    with tempfile.TemporaryDirectory(prefix="chr-usage-") as scratch:
        result = subprocess.run([str(executable), "--help"], cwd=scratch, env=quiet_env(Path(scratch) / "saves"),
                                capture_output=True, text=True, timeout=30)
    if result.returncode != 0 or f"Usage: {PROGRAM}" not in result.stdout:
        raise RuntimeError(f"Packaged program failed to start: {result.returncode}\n{result.stdout}\n{result.stderr}")


def check_served_setup(executable, game):
    """The setup as the launcher asks for it (tools/package/served_setup.py). Without the game the parts that need a
    copy are skipped, and said to be."""
    with tempfile.TemporaryDirectory(prefix="chr-served-") as scratch:
        done, skipped = 0, []
        for passed, what in served_setup.checks(executable, os.environ, Path(scratch), game):
            if passed is None:
                skipped.append(what)
            elif not passed:
                raise RuntimeError("The packaged program's served setup: " + what)
            else:
                done += 1
    print(f"Served setup from the package: {done} checks passed, "
          + (f"{len(skipped)} PARTS SKIPPED (no game given):" if skipped else "none skipped"))
    for what in skipped:
        print("  skipped: " + what)


def check_first_run(executable, game):
    """The package's first start on `game` with an empty save folder: it imports the story (story/import.json) and
    reaches the game's first input wait, where the trace harness (tools/trace/README.md) takes its one snap and ends
    the run."""
    home = resources_of(executable)
    for folder in DATA_FOLDERS:
        if not list((home / folder).glob("*.json")):
            raise RuntimeError(f"The package has no {folder}")
    with tempfile.TemporaryDirectory(prefix="chr-first-run-") as scratch:
        scratch = Path(scratch)
        saves, out = scratch / "saves", scratch / "harness"
        out.mkdir()
        script = scratch / "first.keys"
        script.write_text("@snap:first_wait\n", encoding="utf-8")
        env = dict(quiet_env(saves), REVENGE_ASSET_DIRECTORY=str(Path(game).resolve()),
                   REVENGE_HARNESS_SCRIPT=str(script), REVENGE_HARNESS_OUT=str(out))
        try:
            result = subprocess.run([str(executable)], cwd=scratch, env=env, capture_output=True, text=True,
                                    timeout=FIRST_RUN_SECONDS)
        except subprocess.TimeoutExpired:
            raise RuntimeError(f"Packaged first run did not reach the game's first input wait in {FIRST_RUN_SECONDS} s") from None
        snaps = (out / "snaps.txt").read_text(encoding="utf-8").split() if (out / "snaps.txt").is_file() else []
        imported = (saves / "story" / "import.json").is_file()
        if result.returncode != 0 or snaps != ["first_wait"] or not (out / "done.txt").is_file() or not imported:
            tail = (result.stdout + result.stderr).splitlines()[-10:]
            raise RuntimeError(f"Packaged first run: exit {result.returncode}, story imported {imported}, snaps {snaps}:\n"
                               + "\n".join(tail))
        print("First run from the package: story imported, the game reached its first input wait")
        check_title_shot(executable, game, saves, scratch)


def bmp_picture(path):
    """(width, height, colours seen) of a finished BMP SDL wrote, or None while it is incomplete."""
    data = Path(path).read_bytes()
    if len(data) < 54 or data[:2] != b"BM" or len(data) < int.from_bytes(data[2:6], "little"):
        return None
    offset = int.from_bytes(data[10:14], "little")
    width, height = int.from_bytes(data[18:22], "little", signed=True), abs(int.from_bytes(data[22:26], "little", signed=True))
    step = int.from_bytes(data[28:30], "little") // 8 or 1
    pixels = data[offset:]
    return width, height, len({pixels[i:i + step] for i in range(0, len(pixels) - step, step * 7)})


def check_title_shot(executable, game, saves, scratch):
    """A second start, the story imported by the first: the window's picture at the title, through the smoke input's
    shot:NAME (docs/CAMPAIGN_SMOKE_TEST.md), asked for once a second until one holds a picture (more than 16 colours,
    at least 320x200). The title waits for a key, so the run is then ended as the system's quit ends it (SIGTERM; on
    Windows the process is ended), and killed only if it has not gone in 10 s. With --shots the capture is kept there."""
    shots = scratch / "shots"
    shots.mkdir()
    items = ",".join(f"1000:shot:t{n:02}" for n in range(1, TITLE_SECONDS + 1))
    env = dict(quiet_env(saves), REVENGE_ASSET_DIRECTORY=str(Path(game).resolve()), REVENGE_SMOKE_INPUT=items,
               REVENGE_SMOKE_SHOTS=str(shots))
    run = subprocess.Popen([str(executable)], cwd=scratch, env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    found, deadline = None, time.monotonic() + TITLE_SECONDS + 15
    try:
        while found is None and run.poll() is None and time.monotonic() < deadline:
            time.sleep(0.5)
            for shot in sorted(shots.glob("t*.bmp")):
                picture = bmp_picture(shot)
                if picture and picture[0] >= 320 and picture[1] >= 200 and picture[2] > 16:
                    found = shot, picture
                    break
    finally:
        if run.poll() is None:
            run.terminate()
            try:
                run.wait(timeout=10)
            except subprocess.TimeoutExpired:
                run.kill()
        run.wait()
    if found is None:
        raise RuntimeError(f"Packaged start: no picture in the window in {TITLE_SECONDS} s (exit {run.returncode})")
    shot, (width, height, _) = found
    kept = ""
    if SHOTS:
        SHOTS.mkdir(parents=True, exist_ok=True)
        target = SHOTS / f"title-{platform_name()}-{len(list(SHOTS.glob('title-*.bmp'))) + 1}.bmp"
        shutil.copy2(shot, target)
        kept = f", kept as {target}"
    print(f"Title from the package: the window showed a picture {width}x{height} after {int(shot.stem[1:])} s{kept}")


def check_own_signature(executable, architecture=None):
    """A Mac app's own program, as signed, starts and answers --status, hidden. A release's signature has the hardened
    runtime, which the background copy can't keep (background_app.py), so this is the one start of the app itself.
    --status opens no window, so the run is no foreground app."""
    with tempfile.TemporaryDirectory(prefix="chr-own-signature-") as scratch:
        args = (["arch", f"-{architecture}"] if architecture else []) + [str(executable), "--status"]
        result = subprocess.run(args, cwd=scratch, env=quiet_env(Path(scratch) / "saves"), stdin=subprocess.DEVNULL,
                                capture_output=True, text=True, timeout=60)
    if result.returncode != 0:
        raise RuntimeError(f"The app's own program failed to start as signed: {result.returncode}\n{result.stderr[-2000:]}")


def smoke(executable, game, architecture=None):
    """The checks on a package's program. A Mac app's runs are its background copy's (background_app.py): the app itself
    would come up in the foreground. Its own program starts once, with --status (check_own_signature). `architecture`:
    one half of a Mac app's program."""
    if sys.platform == "darwin" and background_app.app_of(executable) is not None:
        check_own_signature(executable, architecture)
    with background_app.runnable(executable, architecture) as program:
        check_usage(program)
        check_served_setup(program, game)
        if game:
            check_first_run(program, game)
        else:
            print("First run from the package: SKIPPED (no game given: --game or REVENGE_GAME)")


# ---- Windows


MSVC_RUNTIME = ("msvcp140.dll", "vcruntime140.dll", "vcruntime140_1.dll")


def dumpbin_imports(dumpbin, file):
    lines = subprocess.run([str(dumpbin), "/nologo", "/dependents", str(file)], capture_output=True, text=True, check=True).stdout
    return re.findall(r"^\s+(\S+\.dll)\s*$", lines, re.MULTILINE)


def check_windows_imports(folder, imports, system=None):
    """Every DLL a program or DLL in `folder` loads is in it or part of Windows; the MSVC runtime only counts in
    the folder (a player may not have it)."""
    system = system or Path(os.environ.get("SystemRoot", r"C:\Windows")) / "System32"
    files = [f for f in sorted(folder.iterdir()) if f.suffix.lower() in (".exe", ".dll")]
    here = {f.name.lower() for f in files}
    missing = []
    for file in files:
        for dll in (d.lower() for d in imports(file)):
            windows = re.match(r"(api|ext)-ms-win-", dll) or (
                not re.match(r"(vcruntime|msvcp|concrt|vccorlib)\d", dll) and (system / dll).is_file())
            if dll not in here and not windows:
                missing.append(f"{file.name} needs {dll}")
    if missing:
        raise RuntimeError("Not in the package: " + ", ".join(missing))


def find_iscc():
    found = shutil.which("iscc")
    if found:
        return found
    programs = [os.environ.get("ProgramFiles(x86)"), os.environ.get("ProgramFiles")]
    folders = [Path(base) / f"Inno Setup {version}" for version in (7, 6) for base in programs if base]
    folders.insert(2, Path(os.path.expandvars(r"%USERPROFILE%\scoop\apps\inno-setup\current")))
    for folder in folders:
        if (folder / "ISCC.exe").is_file():
            return str(folder / "ISCC.exe")
    raise RuntimeError("Inno Setup's ISCC.exe not found (scoop install inno-setup, or jrsoftware.org)")


def windows_installer(root, stage, output, label):
    """CHR-Rebuild-<label>-Setup.exe from the staged folder (tools/package/installer.iss)."""
    stem = f"{STEM}-{label}"
    iscc = find_iscc()
    run(iscc, "/Qp", f"/DAppVersion={project_version(root)}", f"/DDist={stage.resolve()}", f"/DStem={stem}",
        f"/O{output.resolve()}", root / "tools/package/installer.iss")
    setup = output / f"{stem}-Setup.exe"
    if not setup.is_file():
        raise RuntimeError(f"Inno Setup made no {setup.name}")
    write_digest(setup)
    print(f"Installer {setup}")


def package_windows(root, build, output, label, game, installer):
    name = f"{STEM}-{label}-win64"
    stage = output / name
    if stage.exists():
        shutil.rmtree(stage)
    stage.mkdir(parents=True)
    binary = configure_and_build(root, build, label)
    check_stamp(binary, label)
    shutil.copy2(binary, stage / (PROGRAM + ".exe"))
    vswhere = Path(os.environ["ProgramFiles(x86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
    vs = Path(subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-requires",
              "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"], text=True).strip())
    crt_dirs = sorted((vs / "VC/Redist/MSVC").glob("*/x64/Microsoft.VC*.CRT"))
    if not crt_dirs:
        raise RuntimeError("Cannot locate the redistributable MSVC runtime")
    for dll in MSVC_RUNTIME:
        shutil.copy2(crt_dirs[-1] / dll, stage / dll)
    dumpbins = sorted((vs / "VC/Tools/MSVC").glob("*/bin/Hostx64/x64/dumpbin.exe"))
    if not dumpbins:
        raise RuntimeError("Cannot locate dumpbin")
    check_windows_imports(stage, lambda file: dumpbin_imports(dumpbins[-1], file))
    stage_common(root, build, stage)
    check_paths(root, stage, root, build)
    write_record(root, stage)
    archive = output / (name + ".zip")
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as target:
        for file in sorted(stage.rglob("*")):
            if file.is_file():
                target.write(file, file.relative_to(output))
    # The archive's own layout is what is checked, not the build's.
    extracted = output / "smoke"
    if extracted.exists():
        shutil.rmtree(extracted)
    with zipfile.ZipFile(archive) as source:
        source.extractall(extracted)
    check_notices(extracted / name / "licenses")
    check_record(extracted / name)
    check_kit_settings(extracted / name)
    smoke(extracted / name / (PROGRAM + ".exe"), game)
    write_digest(archive)
    print(f"Packaged and smoke-tested {archive}")
    if installer:
        windows_installer(root, stage, output, label)
    return archive


# ---- Linux


def appimage_tools():
    """The AppImage's tools and runtime with the SHA-256 tools/deps/get_appimage_tools.sh pins for each: what ships
    must not move under a package (unnamed, appimagetool would download the latest runtime)."""
    script = (Path(__file__).resolve().parent / "deps/get_appimage_tools.sh").read_text(encoding="utf-8")
    pins = dict(re.findall(r"^get (\S+) https://\S+ ([0-9a-f]{64})$", script, re.M))
    if set(pins) != {"linuxdeploy-x86_64.AppImage", "appimagetool-x86_64.AppImage", "runtime-x86_64"}:
        raise RuntimeError(f"tools/deps/get_appimage_tools.sh pins {sorted(pins)}, not the three tools")
    return pins


RUNTIME_NOTICE = "tools/package/appimage-runtime"  # the runtime's notices: it links libfuse (LGPL 2.1) and more


def check_runtime_notice(root, licenses):
    """`licenses` (the AppImage's usr/share/doc/chr/licenses) holds the runtime's notices, each as in the repo."""
    wanted = sorted(f for f in (root / RUNTIME_NOTICE).iterdir() if f.is_file())
    if not wanted:
        raise RuntimeError(f"No notices in {RUNTIME_NOTICE}")
    for notice in wanted:
        shipped = licenses / "appimage-runtime" / notice.name
        if not shipped.is_file() or shipped.read_bytes() != notice.read_bytes():
            raise RuntimeError(f"The AppImage lacks the runtime's notice {notice.name} (licenses/appimage-runtime)")


def check_appimage_tools(tools):
    for name, wanted in appimage_tools().items():
        file = tools / name
        if not file.is_file():
            raise RuntimeError(f"Missing AppImage tool: {file} (tools/deps/get_appimage_tools.sh {tools})")
        found = hashlib.sha256(file.read_bytes()).hexdigest()
        if found != wanted:
            raise RuntimeError(f"AppImage tool {file} hashes {found}, wanted {wanted}")


# A Linux program built here needs only these (SDL, libarchive, zlib and liblzma are linked in; SDL opens the
# display's and the sound's libraries itself when it starts). Anything else would be missing on a player's system.
LINUX_SYSTEM_LIBRARIES = {"libc.so.6", "libm.so.6", "libstdc++.so.6", "libgcc_s.so.1", "libdl.so.2", "libpthread.so.0",
                          "librt.so.1", "ld-linux-x86-64.so.2"}


def needed_libraries(readelf_dynamic):
    """The NEEDED entries of `readelf -d`'s output."""
    return re.findall(r"\(NEEDED\)\s+Shared library: \[([^\]]+)\]", readelf_dynamic)


def check_linux_libraries(program, readelf="readelf"):
    """The program loads only the system's libraries: the tarball has none beside it."""
    shown = subprocess.run([readelf, "-d", str(program)], capture_output=True, text=True, check=True).stdout
    outside = sorted(set(needed_libraries(shown)) - LINUX_SYSTEM_LIBRARIES)
    if outside:
        raise RuntimeError("The program loads libraries a player's system may not have: " + ", ".join(outside))


def make_tarball(stage, archive):
    """`archive` (.tar.gz) holding the folder `stage` under its own name, as the zip holds the Windows one."""
    with tarfile.open(archive, "w:gz") as target:
        target.add(stage, arcname=stage.name)


def unpack_tarball(archive, into):
    """`archive` unpacked into `into` as a player would, refusing a member outside it (tarfile's data filter)."""
    with tarfile.open(archive) as source:
        source.extractall(into, filter="data")


def package_linux(root, build, output, label, game, tools):
    """CHR-Rebuild-<label>-linux-x86_64.tar.gz: chr with data/, the read-me and the notices beside it (the zip's
    layout), smoke-tested from the unpacked archive; then CHR-Rebuild-<label>-x86_64.AppImage from the same folder:
    usr/bin/chr with data/ beside it (SDL_GetBasePath), the notices in usr/share/doc (the runtime's in
    licenses/appimage-runtime), any library by linuxdeploy."""
    if not tools:
        raise RuntimeError("--appimage-tools DIR (linuxdeploy, appimagetool and the runtime) is needed for the AppImage")
    tools = Path(tools).resolve()
    check_appimage_tools(tools)
    binary = configure_and_build(root, build, label)
    check_stamp(binary, label)
    name = f"{STEM}-{label}-linux-x86_64"
    stage = output / name
    if stage.exists():
        shutil.rmtree(stage)
    stage.mkdir(parents=True)
    shutil.copy2(binary, stage / PROGRAM)
    check_linux_libraries(stage / PROGRAM)
    stage_common(root, build, stage)
    check_paths(root, stage, root, build)
    write_record(root, stage)
    archive = output / f"{name}.tar.gz"
    make_tarball(stage, archive)
    # The archive's own layout is what is checked, not the build's.
    extracted = output / "smoke"
    if extracted.exists():
        shutil.rmtree(extracted)
    extracted.mkdir()
    unpack_tarball(archive, extracted)
    check_stamp(extracted / name / PROGRAM, label)
    check_notices(extracted / name / "licenses")
    check_record(extracted / name)
    check_kit_settings(extracted / name)
    smoke(extracted / name / PROGRAM, game)
    write_digest(archive)
    print(f"Packaged and smoke-tested {archive}")

    appdir = output / "AppDir"
    if appdir.exists():
        shutil.rmtree(appdir)
    bin_dir, doc = appdir / "usr/bin", appdir / f"usr/share/doc/{PROGRAM}"
    bin_dir.mkdir(parents=True)
    doc.mkdir(parents=True)
    shutil.copy2(stage / PROGRAM, bin_dir / PROGRAM)
    shutil.copytree(stage / "data", bin_dir / "data")
    for item in ("LICENSE", "README.txt"):
        shutil.copy2(stage / item, doc / item)
    shutil.copytree(stage / "licenses", doc / "licenses")
    shutil.copytree(root / RUNTIME_NOTICE, doc / "licenses/appimage-runtime")
    smoke(bin_dir / PROGRAM, game)
    env = dict(os.environ, APPIMAGE_EXTRACT_AND_RUN="1", ARCH="x86_64")
    run(tools / "linuxdeploy-x86_64.AppImage", "--appdir", appdir, "-e", bin_dir / PROGRAM,
        "-d", root / "tools/package/chr.desktop", "-i", root / "tools/package/revenge.png", env=env)
    check_paths(root, appdir, root, build)
    check_notices(doc / "licenses")
    check_runtime_notice(root, doc / "licenses")
    shutil.copy2(stage / KIT_SETTINGS, appdir / KIT_SETTINGS)
    write_record(root, appdir)
    image = output / f"{STEM}-{label}-x86_64.AppImage"
    run(tools / "appimagetool-x86_64.AppImage", "--runtime-file", tools / "runtime-x86_64", appdir, image, env=env)
    result = subprocess.run([str(image.resolve()), "--help"], capture_output=True, text=True, timeout=60, env=env)
    if result.returncode != 0 or f"Usage: {PROGRAM}" not in result.stdout:
        raise RuntimeError(f"The AppImage did not start: {result.returncode}\n{result.stderr}")
    # As the launcher takes it: unpacked by itself, the program started from the unpacked folder.
    unpacked = output / "appimage-unpacked"
    if unpacked.exists():
        shutil.rmtree(unpacked)
    unpacked.mkdir()
    run(image.resolve(), "--appimage-extract", cwd=unpacked, stdout=subprocess.DEVNULL, timeout=600)
    program = unpacked / "squashfs-root/usr/bin" / PROGRAM
    check_stamp(program, label)
    check_notices(unpacked / f"squashfs-root/usr/share/doc/{PROGRAM}/licenses")
    check_runtime_notice(root, unpacked / f"squashfs-root/usr/share/doc/{PROGRAM}/licenses")
    check_record(unpacked / "squashfs-root")
    check_kit_settings(unpacked / "squashfs-root")
    smoke(program, game)
    write_digest(image)
    print(f"AppImage {image}")
    return image


# ---- the Mac


def check_architectures(files):
    wrong = []
    for file in files:
        found = subprocess.run(["lipo", "-archs", str(file)], capture_output=True, text=True, check=True).stdout.split()
        if set(found) != set(MAC_ARCHITECTURES):
            wrong.append(f"{file.name} ({' '.join(found) or 'none'})")
    if wrong:
        raise RuntimeError("Not universal: " + ", ".join(wrong))


def mac_plist(root, minos):
    version = project_version(root)
    return {"CFBundleName": APP_NAME, "CFBundleDisplayName": APP_NAME, "CFBundleIdentifier": BUNDLE_ID,
            "CFBundleExecutable": PROGRAM, "CFBundleIconFile": "revenge", "CFBundlePackageType": "APPL",
            "CFBundleShortVersionString": version, "CFBundleVersion": version, "CFBundleDevelopmentRegion": "en",
            "CFBundleInfoDictionaryVersion": "6.0", "LSMinimumSystemVersion": minos,
            "LSApplicationCategoryType": "public.app-category.strategy-games", "NSHighResolutionCapable": True,
            "NSHumanReadableCopyright": "A fan rebuild, built on Elthial's SDL port; BattleTech is a trademark of its "
                                        "owners. Needs your own copy of the game (1990)."}


def package_mac(root, build, output, label, game):
    """Revenge Rebuild.app and CHR-Rebuild-<label>-mac.dmg: the program in Contents/MacOS (SDL and the archive libraries
    linked in, universal), data/, the read-me and the notices in Contents/Resources (SDL_GetBasePath). Signed and
    notarized by the kit's sign_notarize.sh, or ad hoc for a build not for release (RE_ALLOW_UNSIGNED=1)."""
    mode = mac_signing()  # refused before anything is built when it couldn't be notarized
    binary = configure_and_build(root, build, label)
    check_stamp(binary, label)
    app = output / f"{APP_NAME}.app"
    if app.exists():
        shutil.rmtree(app)
    macos, resources = app / "Contents/MacOS", app / "Contents/Resources"
    for d in (macos, resources):
        d.mkdir(parents=True)
    exe = macos / PROGRAM
    shutil.copy2(binary, exe)
    lines = subprocess.run(["otool", "-L", str(exe)], capture_output=True, text=True, check=True).stdout.splitlines()
    loads = {line.strip().split(" (")[0] for line in lines[1:] if line.startswith(("\t", " "))}
    outside = sorted(d for d in loads if not d.startswith(("/System/", "/usr/lib/")))
    if outside:
        raise RuntimeError("The program loads libraries from outside the system: " + ", ".join(outside))
    check_architectures([exe])
    stage_common(root, build, resources, FIRST_OPEN[mode])
    shutil.copy2(root / "tools/package/revenge.icns", resources / "revenge.icns")
    minos = re.search(r"minos (\S+)", subprocess.run(["otool", "-l", str(exe)], capture_output=True, text=True,
                                                      check=True).stdout)
    with open(app / "Contents/Info.plist", "wb") as f:
        plistlib.dump(mac_plist(root, minos.group(1) if minos else "12.0"), f)
    write_record(root, resources)  # before codesign, which seals it
    sign_app(root, app)  # before the smoke test, so a release's hardened runtime is what is started
    check_record(resources)
    smoke(exe, game)

    dmg = output / f"{STEM}-{label}-mac.dmg"
    dmg_stage = output / "dmg"
    for old in (dmg_stage, dmg):
        if old.is_dir():
            shutil.rmtree(old)
        elif old.exists():
            old.unlink()
    dmg_stage.mkdir()
    run("ditto", app, dmg_stage / app.name)
    (dmg_stage / "Applications").symlink_to("/Applications")
    shutil.copy2(resources / "README.txt", dmg_stage / "Read me.txt")
    check_paths(root, dmg_stage, root, build)
    check_notices(dmg_stage / app.name / "Contents/Resources/licenses")
    check_record(dmg_stage / app.name / "Contents/Resources")
    check_kit_settings(dmg_stage / app.name / "Contents/Resources")
    # Made from the folder, never mounted on the screen.
    run("hdiutil", "create", "-volname", APP_NAME, "-srcfolder", dmg_stage, "-ov", "-format", "UDZO", dmg,
        stdout=subprocess.DEVNULL)
    shutil.rmtree(dmg_stage)
    notarize_dmg(root, dmg, app)
    write_digest(dmg)
    print(f"App: {app}\nDMG: {dmg} ({mode})")
    return dmg


def package(root, build, output, label, game=None, installer=False, tools=None):
    if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9._-]*", label):
        raise ValueError("Invalid package label")
    check_version(root, label)
    check_release_tree(root, label)
    output.mkdir(parents=True, exist_ok=True)
    if platform_name() == "mac":
        return package_mac(root, build, output, label, game)
    if platform_name() == "win64":
        return package_windows(root, build, output, label, game, installer)
    return package_linux(root, build, output, label, game, tools)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--label")
    parser.add_argument("--smoke", type=Path, help="a package already made: run the smoke test only")
    parser.add_argument("--arch", choices=MAC_ARCHITECTURES, help="with --smoke of a Mac app: run this half of it")
    parser.add_argument("--installer", action="store_true", help="Windows: also the Inno Setup installer")
    parser.add_argument("--appimage-tools", type=Path, help="Linux: the folder with linuxdeploy, appimagetool and the runtime")
    parser.add_argument("--shots", type=Path, help="keep the smoke's capture of the title here (never in the repo)")
    parser.add_argument("--game", type=Path, default=os.environ.get("REVENGE_GAME") or None,
                        help="an original game folder for the first-run check (default REVENGE_GAME; never packaged)")
    options = parser.parse_args()
    SHOTS = options.shots.resolve() if options.shots else None
    try:
        if options.smoke:
            names = [PROGRAM + ".exe", PROGRAM, "Contents/MacOS/" + PROGRAM, "usr/bin/" + PROGRAM]
            executable = next((options.smoke / n for n in names if (options.smoke / n).is_file()), None)
            if not executable:
                parser.error(f"no {PROGRAM} program in {options.smoke}")
            smoke(executable.resolve(), options.game, options.arch)
        else:
            if not options.label:
                parser.error("--label is required to package")
            root = Path(__file__).resolve().parents[1]
            package(root, root / "build-package", root / "dist", options.label, options.game, options.installer,
                    options.appimage_tools)
    except (RuntimeError, ValueError) as e:
        sys.exit(f"build_package: {e}")
