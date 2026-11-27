#!/bin/bash
# The setup's archive libraries (re-setup-kit reads the player's download
# through them): libarchive 3.7.9 with zlib 1.3.1 and liblzma (xz 5.8.1),
# static and position-independent, linked into the program, so packages carry
# no more libraries. From their official release sources, each pinned by its
# SHA-256; a download that does not match is refused; no prebuilt binaries.
# The pins and options are the other -re games' (mech1-re, mech3-re).
#
#   tools/deps/build_archive.sh mac     universal (arm64, x86_64), macOS 12 and later
#   tools/deps/build_archive.sh linux   this machine's architecture
#
# Out: build-deps/<platform>/prefix (include, lib, share/licenses), which
# CMakeLists.txt finds. Windows: tools/deps/build_archive.cmd.
set -euo pipefail
cd "$(dirname "$0")/../.."
PLATFORM="${1:?mac or linux}"
ROOT="$PWD/build-deps"
DL="$ROOT/downloads"
PREFIX="$ROOT/$PLATFORM/prefix"
WORK="$ROOT/$PLATFORM/work"
JOBS="${JOBS:-4}"

# name | url | sha256
PINS=(
  "zlib-1.3.1.tar.gz|https://github.com/madler/zlib/releases/download/v1.3.1/zlib-1.3.1.tar.gz|9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23"
  "xz-5.8.1.tar.xz|https://github.com/tukaani-project/xz/releases/download/v5.8.1/xz-5.8.1.tar.xz|0b54f79df85912504de0b14aec7971e3f964491af1812d83447005807513cd9e"
  "libarchive-3.7.9.tar.xz|https://www.libarchive.org/downloads/libarchive-3.7.9.tar.xz|ed8b5732e4cd6e30fae909fb945cad8ff9cb7be5c6cdaa3944ec96e4a200c04c"
)

sha256() { if command -v sha256sum >/dev/null; then sha256sum "$1" | cut -d' ' -f1; else shasum -a 256 "$1" | cut -d' ' -f1; fi; }

mkdir -p "$DL"
for p in "${PINS[@]}"; do
  IFS='|' read -r name url sum <<< "$p"
  [ -f "$DL/$name" ] || curl -sSfL -o "$DL/$name" "$url"
  got="$(sha256 "$DL/$name")"
  if [ "$got" != "$sum" ]; then echo "$name: sha256 $got, pinned $sum: refused" >&2; exit 1; fi
done
echo "sources: all ${#PINS[@]} match their pins"

rm -rf "$WORK" "$PREFIX"
mkdir -p "$WORK/src" "$PREFIX"
unpack() { tar -xf "$DL/$1" -C "$WORK/src"; }

CMAKE_COMMON=(-G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PREFIX" -DCMAKE_PREFIX_PATH="$PREFIX"
              -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_POSITION_INDEPENDENT_CODE=ON)
if [ "$PLATFORM" = mac ]; then
  CMAKE_COMMON+=(-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0)
fi
cmake_build() {  # source dir, then its options
  local src="$1"; shift
  cmake -S "$src" -B "$src/_build" "${CMAKE_COMMON[@]}" "$@" >/dev/null
  cmake --build "$src/_build" -j "$JOBS" >/dev/null
  cmake --install "$src/_build" >/dev/null
}

# No crypto, XML, bzip2, zstd, LZ4 or iconv: none of the game's downloads needs them.
unpack zlib-1.3.1.tar.gz
cmake_build "$WORK/src/zlib-1.3.1" -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DZLIB_BUILD_EXAMPLES=OFF
rm -f "$PREFIX"/lib/libz*.dylib "$PREFIX"/lib/libz.so*  # the static one only
echo "built zlib"
unpack xz-5.8.1.tar.xz
cmake_build "$WORK/src/xz-5.8.1" -DBUILD_SHARED_LIBS=OFF -DXZ_TOOL_XZ=OFF \
  -DXZ_TOOL_XZDEC=OFF -DXZ_TOOL_LZMADEC=OFF -DXZ_TOOL_LZMAINFO=OFF -DXZ_TOOL_SCRIPTS=OFF -DXZ_DOC=OFF -DXZ_NLS=OFF
echo "built liblzma"
unpack libarchive-3.7.9.tar.xz
cmake_build "$WORK/src/libarchive-3.7.9" -DBUILD_SHARED_LIBS=OFF \
  -DENABLE_TAR=OFF -DENABLE_CPIO=OFF -DENABLE_CAT=OFF -DENABLE_UNZIP=OFF -DENABLE_TEST=OFF -DENABLE_INSTALL=ON \
  -DENABLE_ZLIB=ON -DENABLE_LZMA=ON -DENABLE_BZip2=OFF -DENABLE_ZSTD=OFF -DENABLE_LZ4=OFF -DENABLE_LIBB2=OFF \
  -DENABLE_OPENSSL=OFF -DENABLE_MBEDTLS=OFF -DENABLE_NETTLE=OFF -DENABLE_CNG=OFF -DENABLE_LIBXML2=OFF \
  -DENABLE_EXPAT=OFF -DENABLE_PCREPOSIX=OFF -DENABLE_PCRE2POSIX=OFF \
  -DENABLE_ICONV=OFF -DENABLE_ACL=OFF -DENABLE_XATTR=OFF -DENABLE_LZO=OFF
rm -f "$PREFIX"/lib/libarchive*.dylib "$PREFIX"/lib/libarchive.so*
echo "built libarchive"
# Their licences, for the packages (linked in, so no library file carries them).
for l in "libarchive-3.7.9 COPYING" "zlib-1.3.1 LICENSE" "xz-5.8.1 COPYING COPYING.0BSD"; do
  set -- $l; d=$1; shift
  mkdir -p "$PREFIX/share/licenses/$d"
  for f in "$@"; do cp "$WORK/src/$d/$f" "$PREFIX/share/licenses/$d/"; done
done
echo "prefix: $PREFIX"
