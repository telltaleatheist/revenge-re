#!/bin/bash
# The music soundfont's source, once per machine: Debian's copy of the
# fluid-soundfont 3.1 upstream tarball (FluidR3 GM, MIT), pinned by its
# SHA-256; a download that does not match is refused. The build cuts the
# subset from it (tools/soundfont/subset.py) and never downloads.
#
#   tools/deps/fetch_soundfont.sh
#
# Out: build-deps/soundfont/fluid-soundfont_3.1.orig.tar.gz, which
# CMakeLists.txt finds (or REVENGE_SOUNDFONT_SOURCE names another copy).
# Windows: python tools\soundfont\subset.py fetch.
set -euo pipefail
cd "$(dirname "$0")/../.."
exec python3 -I tools/soundfont/subset.py fetch
