#include "REVENGE.h"
#include "revenge/original_data.h"
#include "revenge/platform_backend.h"
#include "revenge/video.h"

#include <assert.h>
#include <string.h>

/* The VGA segment A000 is 64K: the screen's 64000 bytes and then video
 * memory off the screen (A000:FA00-FFFF). CopyVideoRegion (0800:2B00) moves
 * each row with rep movsb, its offsets 16-bit, so a row past FFFFh carries on
 * at the segment's start, as the original's does. A text window reaching
 * past row 200 saves and restores those off-screen rows. */
void CopyVideoRegion(int16 sourceX, int16 sourceY, int16 destinationX,
    int16 destinationY, word16 width, word16 height, const byte *sourceBase,
    byte *destinationBase);

static byte row[VIDEO_SCREEN_WIDTH];
static byte saved[VIDEO_SCREEN_WIDTH * 0x02];

int main(void)
{
    byte *screen = SDLBackend_GetIndexedFramebuffer();
    byte *videoSegment = (byte *) (uintptr_t) 0xA0000000UL;
    word16 index;

    ActiveVideoAdapterType = VIDEO_ADAPTER_VGA;
    for (index = 0; index < sizeof(row); ++index)
        row[index] = (byte) (index + 0x01);
    memset(screen, 0x00, VIDEO_SCREEN_WIDTH * VIDEO_SCREEN_HEIGHT);

    /* Row 204 starts at FF00h: its first 100h bytes end the segment and
	 * the other 40h are the first 40h of the screen. */
    CopyVideoRegion(0x0000, 0x0000, 0x0000, 0x00CC, 0x0028, 0x0001, row, videoSegment);
    assert(memcmp(&screen[0xFF00], row, 0x0100) == 0);
    assert(memcmp(&screen[0x0000], &row[0x0100], 0x0040) == 0);
    assert(screen[0x0040] == 0x00);

    /* Rows 200-201, off the screen, round trip through a save. */
    memset(&screen[0xFA00], 0x5A, VIDEO_SCREEN_WIDTH * 0x02);
    CopyVideoRegion(0x0000, 0x00C8, 0x0000, 0x0000, 0x0028, 0x0002, videoSegment, saved);
    assert(saved[0x0000] == 0x5A && saved[sizeof(saved) - 0x01] == 0x5A);
    memset(&screen[0xFA00], 0x00, VIDEO_SCREEN_WIDTH * 0x02);
    CopyVideoRegion(0x0000, 0x0000, 0x0000, 0x00C8, 0x0028, 0x0002, saved, videoSegment);
    assert(screen[0xFA00] == 0x5A && screen[0xFA00 + sizeof(saved) - 0x01] == 0x5A);
    return 0;
}
