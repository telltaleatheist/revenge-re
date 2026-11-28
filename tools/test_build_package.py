"""The packaging's own parts, without a build, SDL or the game's files (ctest package.build_package):

    python3 -I -m unittest discover -s tools -p test_build_package.py -v

The Mac's signing runs the kit's sign_notarize.sh check for real and mocks every command that would sign. The identity
is a made-up word: a real one is never written anywhere.
"""
import os
from pathlib import Path
import plistlib
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import DEFAULT, patch

sys.path.insert(0, str(Path(__file__).resolve().parent))
import build_package  # noqa: E402
from build_package import background_app  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
SIGNING = ("RE_SIGN_IDENTITY", "RE_NOTARY_PROFILE", "RE_ALLOW_UNSIGNED",
           "MW3_SIGN_IDENTITY", "MW3_NOTARY_PROFILE", "MW3_ALLOW_UNSIGNED")


def signing_env(**names):
    """The environment with only these of the kit's signing names set."""
    env = {k: v for k, v in os.environ.items() if k not in SIGNING}
    env.update(names)
    return patch.dict(os.environ, env, clear=True)


class Names(unittest.TestCase):
    def test_a_bad_label_is_refused_before_anything_is_built(self):
        with patch.object(build_package, "configure_and_build") as build:
            with self.assertRaises(ValueError):
                build_package.package(ROOT, Path("build"), Path("dist"), "../x")
            build.assert_not_called()


@unittest.skipIf(os.name == "nt", "The kit's signer is a bash script, for a Mac")
class MacSigning(unittest.TestCase):
    """What kind of Mac build this is, by the kit's own check; a release's needs a Developer ID and a notary profile."""

    def test_the_signer_is_the_kits(self):
        self.assertTrue((ROOT / build_package.SIGNER).is_file())

    def test_without_an_identity_a_build_is_refused(self):
        with signing_env():
            with self.assertRaisesRegex(RuntimeError, "would not be notarized"):
                build_package.mac_signing()

    def test_a_build_not_for_release_is_unsigned_only_when_allowed(self):
        with signing_env(RE_ALLOW_UNSIGNED="1"):
            self.assertEqual(build_package.mac_signing(), "unsigned")

    def test_an_identity_without_a_notary_profile_is_refused(self):
        with signing_env(RE_SIGN_IDENTITY="made-up"):
            with self.assertRaisesRegex(RuntimeError, "RE_NOTARY_PROFILE") as refused:
                build_package.mac_signing()
            self.assertNotIn("made-up", str(refused.exception))

    def test_the_kits_check_comes_before_anything_is_built(self):
        with patch.object(build_package, "mac_signing", side_effect=RuntimeError("refused")), \
                patch.object(build_package, "configure_and_build") as build:
            with self.assertRaisesRegex(RuntimeError, "refused"):
                build_package.package_mac(ROOT, Path("build"), Path("dist"), "x", None)
            build.assert_not_called()

    def test_only_a_build_not_for_release_says_how_to_open_it(self):
        self.assertEqual(build_package.FIRST_OPEN["notarize"], "")
        self.assertIn("right-click", build_package.FIRST_OPEN["unsigned"])


class SigningHooks(unittest.TestCase):
    """The commands only: run is a mock, so nothing is signed."""

    def test_the_kit_alone_signs_and_notarizes_and_never_sees_the_identity(self):
        root = Path("repo")
        app, dmg = root / "dist/Revenge Rebuild.app", root / "dist/CHR-Rebuild-x-mac.dmg"
        with signing_env(RE_SIGN_IDENTITY="made-up", RE_NOTARY_PROFILE="made-up-profile"), \
                patch.object(build_package, "run") as run:
            build_package.sign_app(root, app)
            build_package.notarize_dmg(root, dmg, app)
        signer = root / build_package.SIGNER
        calls = [c.args for c in run.call_args_list]
        self.assertEqual(calls, [(signer, "app", app, "--entitlements", root / "tools/package/chr.entitlements"),
                                 (signer, "dmg", dmg, app)])
        self.assertFalse(any("made-up" in str(a) for c in calls for a in c))


class CheckPaths(unittest.TestCase):
    """tools/package/check_paths.py on a made-up name: allowed only in a public GitHub address."""

    def test_the_name_is_allowed_only_in_a_public_github_address(self):
        tool = Path(__file__).resolve().parent / "package/check_paths.py"
        for text, code in [("https://github.com/someonemadeup/x/releases", 0),
                           ("https://api.github.com/repos/someonemadeup/x", 0),
                           ("https://raw.githubusercontent.com/someonemadeup/x/main/catalog/games.json", 0),
                           ("https://raw.githubusercontent.com/someosomeonemadeup/x", 0),  # in overlapping pieces
                           ("https://example.com/someonemadeup/x", 1),
                           ("built by someonemadeup", 1)]:
            with tempfile.TemporaryDirectory() as folder:
                (Path(folder) / "chr.exe").write_text(text)
                found = subprocess.run([sys.executable, "-I", str(tool), folder, "someonemadeup"],
                                       capture_output=True, text=True)
                self.assertEqual(found.returncode, code, text)


class LinuxTarball(unittest.TestCase):
    """The tarball's own parts: its layout as unpacked, and the program's libraries."""

    def test_a_program_without_the_appimage_tools_is_refused_before_anything_is_built(self):
        with patch.object(build_package, "configure_and_build") as build:
            with self.assertRaisesRegex(RuntimeError, "--appimage-tools"):
                build_package.package_linux(ROOT, Path("build"), Path("dist"), "x", None, None)
            build.assert_not_called()

    def test_the_tarball_unpacks_to_its_folder_with_the_program_runnable(self):
        with tempfile.TemporaryDirectory() as directory:
            stage = Path(directory) / "CHR-Rebuild-x-linux-x86_64"
            for name, data in (("chr", b"#!/bin/sh\n"), ("data/rules/x.json", b"{}"), ("licenses/project/a", b"a"),
                               ("README.txt", b"r")):
                (stage / name).parent.mkdir(parents=True, exist_ok=True)
                (stage / name).write_bytes(data)
            (stage / "chr").chmod(0o755)
            archive = Path(directory) / "x.tar.gz"
            build_package.make_tarball(stage, archive)
            out = Path(directory) / "out"
            out.mkdir()
            build_package.unpack_tarball(archive, out)
            self.assertEqual([p.name for p in out.iterdir()], [stage.name])
            files = sorted(f.relative_to(out / stage.name).as_posix() for f in (out / stage.name).rglob("*") if f.is_file())
            self.assertEqual(files, ["README.txt", "chr", "data/rules/x.json", "licenses/project/a"])
            if os.name != "nt":
                self.assertTrue(os.access(out / stage.name / "chr", os.X_OK))

    def test_a_member_outside_the_folder_is_refused(self):
        import io
        import tarfile
        with tempfile.TemporaryDirectory() as directory:
            archive = Path(directory) / "bad.tar.gz"
            with tarfile.open(archive, "w:gz") as target:
                member = tarfile.TarInfo("../outside.txt")
                member.size = 1
                target.addfile(member, io.BytesIO(b"x"))
            out = Path(directory) / "out"
            out.mkdir()
            with self.assertRaises(tarfile.TarError):
                build_package.unpack_tarball(archive, out)
            self.assertFalse((Path(directory) / "outside.txt").exists())

    def test_only_the_systems_libraries_are_needed(self):
        shown = """
Dynamic section at offset 0x1 contains 30 entries:
  Tag        Type                         Name/Value
 0x0000000000000001 (NEEDED)             Shared library: [libstdc++.so.6]
 0x0000000000000001 (NEEDED)             Shared library: [libm.so.6]
 0x0000000000000001 (NEEDED)             Shared library: [libc.so.6]
 0x000000000000000e (SONAME)             Library soname: [libnot.so.1]
"""
        self.assertEqual(build_package.needed_libraries(shown), ["libstdc++.so.6", "libm.so.6", "libc.so.6"])
        done = subprocess.CompletedProcess([], 0, stdout=shown, stderr="")
        with patch.object(build_package.subprocess, "run", return_value=done):
            build_package.check_linux_libraries(Path("chr"))
        extra = subprocess.CompletedProcess([], 0, stdout=shown.replace("libm.so.6", "libSDL3.so.0"), stderr="")
        with patch.object(build_package.subprocess, "run", return_value=extra):
            with self.assertRaisesRegex(RuntimeError, "libSDL3.so.0"):
                build_package.check_linux_libraries(Path("chr"))


def smoke_checks():
    return patch.multiple(build_package, check_usage=DEFAULT, check_served_setup=DEFAULT, check_first_run=DEFAULT)


@unittest.skipIf(os.name == "nt", "A Mac app's layout")
class BackgroundApp(unittest.TestCase):
    """A check's runs of a Mac app are its background copy's; the app's own program starts once, with --status."""

    def app(self, folder, program=b"#!/bin/sh\nexit 0\n"):
        app = Path(folder) / "A game.app"
        for name in ("MacOS/chr", "Resources/data/rules/x.json"):
            (app / "Contents" / name).parent.mkdir(parents=True, exist_ok=True)
            (app / "Contents" / name).write_bytes(program if name == "MacOS/chr" else b"{}")
        (app / "Contents/MacOS/chr").chmod(0o755)
        (app / "Contents/Info.plist").write_bytes(plistlib.dumps(build_package.mac_plist(ROOT, "12.0")))
        subprocess.run(["codesign", "--force", "--sign", "-", str(app)], check=True, capture_output=True)
        return app

    @staticmethod
    def files(app):
        return {str(f.relative_to(app)): f.read_bytes() for f in Path(app).rglob("*") if f.is_file()}

    def test_a_program_outside_an_app_runs_itself(self):
        with tempfile.TemporaryDirectory() as directory:
            for program in (Path(directory) / "chr", Path(directory) / "Contents/MacOS/chr"):  # the second: no Info.plist
                program.parent.mkdir(parents=True, exist_ok=True)
                program.write_bytes(b"")
                with background_app.runnable(program) as runs:
                    self.assertEqual(runs, program)

    def test_an_architecture_is_for_an_app(self):
        with self.assertRaisesRegex(RuntimeError, "a Mac app only"):
            with background_app.runnable(Path("chr"), "x86_64"):
                self.fail("ran")

    @unittest.skipIf(sys.platform != "darwin", "A Mac's apps and signatures")
    def test_the_smoke_runs_the_copy_and_leaves_the_app(self):
        with tempfile.TemporaryDirectory() as directory:
            app = self.app(directory)
            before, ran = self.files(app), []

            def seen(program, *args):
                copy = program.parents[2]
                ran.append(copy)
                self.assertEqual(program.name, "chr")
                self.assertFalse(os.path.samefile(copy, app))
                subprocess.run(["codesign", "--verify", "--deep", "--strict", str(copy)], check=True)
                now = self.files(copy)
                self.assertEqual(plistlib.loads(now["Contents/Info.plist"]),
                                 {**plistlib.loads(before["Contents/Info.plist"]), "LSBackgroundOnly": True})
                self.assertEqual({k for k in before if before[k] != now.get(k)} - {"Contents/Info.plist"},
                                 {k for k in before if "_CodeSignature" in k and before[k] != now.get(k)})
                self.assertEqual(now.keys(), before.keys())

            with smoke_checks() as checks:
                for check in checks.values():
                    check.side_effect = seen
                build_package.smoke(app / "Contents/MacOS/chr", Path(directory))
                for check in checks.values():
                    check.assert_called_once()
            self.assertEqual(len(set(ran)), 1)
            self.assertFalse(ran[0].exists())
            self.assertEqual(self.files(app), before)

    @unittest.skipIf(sys.platform != "darwin", "A Mac's apps and signatures")
    def test_the_copy_of_a_runtime_signed_app_has_no_runtime(self):
        def flags(app):
            shown = subprocess.run(["codesign", "-dv", str(app)], capture_output=True, text=True).stderr
            return re.search(r"flags=\S+", shown).group()

        with tempfile.TemporaryDirectory() as directory:
            app = self.app(directory)
            subprocess.run(["codesign", "--force", "--sign", "-", "--options", "runtime", str(app)], check=True,
                           capture_output=True)
            self.assertIn("runtime", flags(app))
            out = Path(directory) / "out"
            out.mkdir()
            copy = background_app.background_copy(app, out)
            self.assertNotIn("runtime", flags(copy))
            self.assertIn("runtime", flags(app))
            subprocess.run(["codesign", "--verify", "--deep", "--strict", str(copy)], check=True)

    @unittest.skipIf(sys.platform != "darwin", "A Mac's apps and signatures")
    def test_the_smoke_starts_the_apps_own_program_once_with_status(self):
        with tempfile.TemporaryDirectory() as directory:
            log = Path(directory) / "started.txt"
            app = self.app(directory, f'#!/bin/sh\necho "$0 $*" >> "{log}"\nexit 0\n'.encode())
            with smoke_checks():
                build_package.smoke(app / "Contents/MacOS/chr", Path(directory))
            self.assertEqual(log.read_text().splitlines(), [f"{app}/Contents/MacOS/chr --status"])

    @unittest.skipIf(sys.platform != "darwin", "A Mac's apps and signatures")
    def test_an_app_whose_own_program_fails_is_not_smoked(self):
        with tempfile.TemporaryDirectory() as directory:
            app = self.app(directory, b"#!/bin/sh\nexit 6\n")
            with smoke_checks() as checks:
                with self.assertRaisesRegex(RuntimeError, "failed to start as signed: 6"):
                    build_package.smoke(app / "Contents/MacOS/chr", Path(directory))
                for check in checks.values():
                    check.assert_not_called()

    @unittest.skipIf(sys.platform != "darwin", "A Mac's apps and signatures")
    def test_an_app_with_a_broken_signature_is_not_run(self):
        with tempfile.TemporaryDirectory() as directory:
            app = self.app(directory)
            (app / "Contents/Resources/data/rules/x.json").write_text("changed")
            with smoke_checks() as checks, patch.object(build_package, "check_own_signature"):
                with self.assertRaisesRegex(RuntimeError, "codesign"):
                    build_package.smoke(app / "Contents/MacOS/chr", Path(directory))
                for check in checks.values():
                    check.assert_not_called()

    @unittest.skipIf(sys.platform != "darwin", "A Mac's apps and signatures")
    def test_one_half_of_the_program(self):
        with tempfile.TemporaryDirectory() as directory:
            app = self.app(directory, program=Path("/usr/bin/true").read_bytes())  # a universal one; only looked at
            before = self.files(app)

            def archs(file):
                return subprocess.run(["lipo", "-archs", str(file)], check=True, capture_output=True,
                                      text=True).stdout.split()

            self.assertGreater(len(archs(app / "Contents/MacOS/chr")), 1)
            with background_app.runnable(app / "Contents/MacOS/chr", "x86_64") as program:
                self.assertEqual(archs(program), ["x86_64"])
                subprocess.run(["codesign", "--verify", "--deep", "--strict", str(program.parents[2])], check=True)
            with self.assertRaisesRegex(RuntimeError, "lipo"):
                with background_app.runnable(app / "Contents/MacOS/chr", "ppc"):
                    self.fail("ran")
            self.assertEqual(self.files(app), before)



class RuntimeNotice(unittest.TestCase):
    def test_the_appimage_needs_the_runtimes_notices(self):
        notices = sorted(f.name for f in (ROOT / build_package.RUNTIME_NOTICE).iterdir())
        self.assertIn("NOTICE.txt", notices)
        self.assertIn("libfuse-LGPL2.txt", notices)
        with tempfile.TemporaryDirectory() as d:
            licenses = Path(d) / "licenses"
            with self.assertRaisesRegex(RuntimeError, "runtime's notice"):
                build_package.check_runtime_notice(ROOT, licenses)
            shutil.copytree(ROOT / build_package.RUNTIME_NOTICE, licenses / "appimage-runtime")
            build_package.check_runtime_notice(ROOT, licenses)
            (licenses / "appimage-runtime/NOTICE.txt").write_text("changed")
            with self.assertRaisesRegex(RuntimeError, "NOTICE.txt"):
                build_package.check_runtime_notice(ROOT, licenses)


class ProgramRecord(unittest.TestCase):
    """CHECK MY FILES' record (rebuild-files.sha256), written by the kit's files_manifest.py before a package is sealed
    and checked in what the package holds."""

    def test_the_record_names_every_file_and_finds_a_change(self):
        with tempfile.TemporaryDirectory() as directory:
            folder = Path(directory)
            for name in ("chr", "data/rules/x.json", "licenses/LICENSE"):
                (folder / name).parent.mkdir(parents=True, exist_ok=True)
                (folder / name).write_bytes(name.encode())
            build_package.write_record(ROOT, folder)
            self.assertEqual(len((folder / build_package.RECORD).read_text().splitlines()), 3)
            build_package.check_record(folder)
            (folder / "data/rules/x.json").write_bytes(b"{}")
            with self.assertRaisesRegex(RuntimeError, "changed since it was made"):
                build_package.check_record(folder)
            build_package.write_record(ROOT, folder)
            (folder / "extra.txt").write_bytes(b"")
            with self.assertRaisesRegex(RuntimeError, "leaves out"):
                build_package.check_record(folder)

    def test_the_kits_settings_are_this_games(self):
        build_package.check_kit_settings(ROOT / "config")
        with tempfile.TemporaryDirectory() as directory:
            folder = Path(directory)
            with self.assertRaisesRegex(RuntimeError, build_package.KIT_SETTINGS):
                build_package.check_kit_settings(folder)
            (folder / build_package.KIT_SETTINGS).write_bytes(
                (ROOT / "extern/re-setup-kit/assets" / build_package.KIT_SETTINGS).read_bytes())
            with self.assertRaisesRegex(RuntimeError, "not this game's"):
                build_package.check_kit_settings(folder)

    @unittest.skipIf(sys.platform != "darwin", "A Mac's apps and signatures")
    def test_a_mac_apps_record_is_in_resources_and_keeps_the_signature(self):
        with tempfile.TemporaryDirectory() as directory:
            app = Path(directory) / "A game.app"
            for name in ("MacOS/chr", "Resources/data/rules/x.json"):
                (app / "Contents" / name).parent.mkdir(parents=True, exist_ok=True)
                (app / "Contents" / name).write_bytes(b"#!/bin/sh\nexit 0\n" if name == "MacOS/chr" else b"{}")
            (app / "Contents/MacOS/chr").chmod(0o755)
            (app / "Contents/Info.plist").write_bytes(plistlib.dumps(build_package.mac_plist(ROOT, "12.0")))
            resources = app / "Contents/Resources"
            build_package.write_record(ROOT, resources)
            subprocess.run(["codesign", "--force", "--sign", "-", str(app)], check=True, capture_output=True)
            subprocess.run(["codesign", "--verify", "--deep", "--strict", str(app)], check=True, capture_output=True)
            build_package.check_record(resources)
            self.assertEqual((resources / build_package.RECORD).read_text().split(), [
                __import__("hashlib").sha256(b"{}").hexdigest(), "data/rules/x.json"])

if __name__ == "__main__":
    unittest.main()
