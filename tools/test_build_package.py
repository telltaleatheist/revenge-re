"""The packaging's own parts, without a build, SDL or the game's files (ctest package.build_package):

    python3 -I -m unittest discover -s tools -p test_build_package.py -v

The Mac's signing runs the kit's sign_notarize.sh check for real and mocks every command that would sign. The identity
is a made-up word: a real one is never written anywhere.
"""
import os
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parent))
import build_package  # noqa: E402

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


if __name__ == "__main__":
    unittest.main()
