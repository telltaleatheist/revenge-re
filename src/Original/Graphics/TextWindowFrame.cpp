// 334D: text window frame
// Reconstructed original text-window frame renderer (segment 334D).

#include "Data/RuntimeData.h"
#include "Data/TextIOData.h"
#include "Graphics/Graphics.h"
#include "revenge/video.h"

// 334D:0001: void DrawActiveTextWindowFrame(word16 frameStyleIndex)
// Called from:
//      OpenModalTextWindow
//      RunGameOptionsMenu
void DrawActiveTextWindowFrame(uint16_t frameStyleIndex)
{
    uint8_t tileShift = 0x05;
    if (VideoAdapterType == VIDEO_ADAPTER_VGA)
        tileShift = 0x06;
    const uint16_t *frameDefinition =
        TextWindowFrameDefinitionTable[frameStyleIndex];
    const uint8_t *tileAtlas = TacticalTileStrip;
    int16_t left = ActiveTextWindowLeftTile;
    int16_t top = ActiveTextWindowTopTile;
    uint16_t width = ActiveTextWindowWidthTiles;
    uint16_t height = ActiveTextWindowHeightTiles;
    Draw8x8VideoTile((const uint8_t *) tileAtlas + (frameDefinition[0x00] << tileShift), left - 0x01, top - 0x01);
    Draw8x8VideoTile((const uint8_t *) tileAtlas + (frameDefinition[0x01] << tileShift), left + width, top - 0x01);
    Draw8x8VideoTile((const uint8_t *) tileAtlas + (frameDefinition[0x02] << tileShift), left - 0x01, top + height);
    Draw8x8VideoTile((const uint8_t *) tileAtlas + (frameDefinition[0x03] << tileShift), left + width, top + height);

    uint16_t edgeSequenceIndex = 0x04;
    edgeSequenceIndex = DrawRepeatingFrameTileSequence(frameDefinition, edgeSequenceIndex, left, top - 0x01, width, TRUE);
    edgeSequenceIndex = DrawRepeatingFrameTileSequence(frameDefinition, edgeSequenceIndex, left - 0x01, top, height, FALSE);
    edgeSequenceIndex = DrawRepeatingFrameTileSequence(frameDefinition, edgeSequenceIndex, left + width, top, height, FALSE);
    DrawRepeatingFrameTileSequence(frameDefinition, edgeSequenceIndex, left, top + height, width, TRUE);
}
