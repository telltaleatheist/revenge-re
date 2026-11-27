#include "Data/StartupData.h"
#include "Graphics/Graphics.h"
#include "revenge/heap.h"

#include <assert.h>

static uint8_t oldBacking[0x10];
static uint8_t replacementBacking[0x10];
static uint8_t *freedBacking;
static uint16_t capturedSourceTileX;
static uint16_t capturedSourcePixelY;
static uint16_t capturedWidthTiles;
static uint16_t capturedHeightPixels;

void Heap_Free(void *block)
{
    freedBacking = (uint8_t *) block;
}

uint8_t *BuildDisplayBackingBuffer(
    uint16_t sourceTileX,
    uint16_t sourcePixelY,
    uint16_t widthTiles,
    uint16_t heightPixels)
{
    capturedSourceTileX = sourceTileX;
    capturedSourcePixelY = sourcePixelY;
    capturedWidthTiles = widthTiles;
    capturedHeightPixels = heightPixels;
    return replacementBacking;
}

int main(void)
{
    assert(MouseCursorBackingSourceTileX == 0x0000);
    assert(MouseCursorBackingSourcePixelY == 0x0008);
    assert(MouseCursorBackingWidthTiles == 0x0002);
    assert(MouseCursorBackingHeightPixels == 0x0010);
    InputDeviceType = 0x0001;
    assert(TacticalCursorAutoScroll == 0x0001);

    SharedGraphicsBackingBuffers[0x00] = oldBacking;
    MouseCursorBackingSourceTileX = 0x0011;
    MouseCursorBackingSourcePixelY = 0x0022;
    MouseCursorBackingWidthTiles = 0x0033;
    MouseCursorBackingHeightPixels = 0x0044;

    RebuildMouseCursorBackingBuffer();
    assert(freedBacking == oldBacking);
    assert(capturedSourceTileX == 0x0011);
    assert(capturedSourcePixelY == 0x0022);
    assert(capturedWidthTiles == 0x0033);
    assert(capturedHeightPixels == 0x0044);
    assert(SharedGraphicsBackingBuffers[0x00] == replacementBacking);

    return 0;
}
