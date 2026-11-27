#include "Setup.h"

// CHR_RELEASE_TAG is a release's tag (CMake refuses any other), undefined in any other build, which then holds no
// stamp at all. The stamp's words are the launcher's (mw3-re, docs/launcher.md), the same in every -re game.
#if defined(CHR_RELEASE_TAG)
#define CHR_STAMP "MW3-PROGRAM-VERSION:"
// Kept in the file: `used`, and read by build_tag.
extern "C"
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((used))
#endif
    const char chr_version_tag[] = CHR_STAMP CHR_RELEASE_TAG;
const char* chr::setup::build_tag() { return chr_version_tag + sizeof(CHR_STAMP) - 1; }
#else
const char* chr::setup::build_tag() { return ""; }
#endif
