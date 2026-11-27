// 34DF: packed icons
// PackedIcons.cpp - direct packed icon-page loading
// Direct tactical icon-page loader used by the page-one special case.

#include "REVENGE.h"
#include "revenge/original_data.h"
#include "revenge/video.h"

void LoadPackedIconResource(
    const char *resourcePath,
    uint8_t *packedDisplayBuffer);

// 34DF:0003
void LoadPackedIconPageDirect(
    const char *resourcePath,
    uint8_t *packedDisplayBuffer,
    uint8_t *iconPageDestination)
{
    LoadPackedIconResource(resourcePath, packedDisplayBuffer);
    /* The CGA's switch to the Tandy's decode and back, with its mask
	 * builder (34DF:001D-0036, 004F-0070, 0800:35D8), went with the CGA;
	 * the rebuild is VGA only. */
    CopyDecodedIconPage(packedDisplayBuffer, iconPageDestination);
}
