// 0D73: cursor backing
/* Cursor-backing procedure from original segment 0D73. */

#include "REVENGE.h"

#include "revenge/original_data.h"
#include "revenge/video.h"

/* 0D73:4126-4154 */
void RebuildMouseCursorBackingBuffer(void)
{
    uint8_t *replacement;

    Heap_Free(SharedGraphicsBackingBuffers[0x00]);
    replacement = BuildDisplayBackingBuffer(
        MouseCursorBackingSourceTileX,
        MouseCursorBackingSourcePixelY,
        MouseCursorBackingWidthTiles,
        MouseCursorBackingHeightPixels);
    SharedGraphicsBackingBuffers[0x00] = replacement;
}
