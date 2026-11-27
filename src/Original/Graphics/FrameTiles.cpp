// 3386: frame tiles
// Reconstructed original repeating frame-tile renderer (segment 3386).

#include "Data/RuntimeData.h"
#include "Graphics/Graphics.h"
#include "revenge/video.h"

// 3386:0008: word16 DrawRepeatingFrameTileSequence(...)
// Called from:
//      DrawActiveTextWindowFrame
uint16_t DrawRepeatingFrameTileSequence(
    const uint16_t *frameDefinition,
    uint16_t sequenceIndex,
    int16_t x,
    int16_t y,
    uint16_t tileCount,
    uint16_t horizontal)
{
    uint8_t tileShift = VideoAdapterType == VIDEO_ADAPTER_VGA ? VGA_TILE_BYTE_SHIFT : EGA_TILE_BYTE_SHIFT;
    uint16_t currentIndex = sequenceIndex;
    uint16_t drawn;
    for (drawn = 0x00; drawn < tileCount; ++drawn)
    {
        const uint8_t *tileAtlas = TacticalTileStrip;
        Draw8x8VideoTile(
            (const uint8_t *) tileAtlas +
                (frameDefinition[currentIndex] << tileShift),
            x,
            y);
        if (horizontal != FALSE)
            ++x;
        else
            ++y;
        ++currentIndex;
        if (frameDefinition[currentIndex] == FRAME_DEFINITION_END)
            currentIndex = sequenceIndex;
    }
    while (frameDefinition[currentIndex] != FRAME_DEFINITION_END)
        ++currentIndex;
    return currentIndex + 0x01;
}
