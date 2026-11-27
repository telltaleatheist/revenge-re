// 2507: editor graphics
// EditorGraphics.cpp
// Reconstructed from the original procedures in segment 2507.

#include "LanceEditorInternal.h"
#include "Data/StartupData.h"
#include "Graphics/Graphics.h"
#include "Text/Text.h"
#include "revenge/heap.h"
#include "revenge/text.h"

// 2507:60D2: draw the first four unit stat headers (UnitStatHeaderTextTable 0-3), 7 pixels apart.
void DrawRangeAndSpeedStatHeaders(uint16_t topY)
{
    uint16_t yOffset = 0x00;
    uint16_t labelIndex;
    for (labelIndex = 0x00; labelIndex < 0x04; ++labelIndex)
    {
        DrawTextString(UnitStatHeaderTextTable[labelIndex],
            0x01, topY + yOffset, 0x0F, 0x00);
        yOffset += 0x07;
    }
}

// 2507:6111: draw the last two unit stat headers (UnitStatHeaderTextTable 4-5), 9 pixels apart.
void DrawExperienceAndGunneryHeaders(uint16_t topY)
{
    uint16_t yOffset = 0x00;
    uint16_t labelIndex;
    for (labelIndex = 0x00; labelIndex < 0x02; ++labelIndex)
    {
        DrawTextString(UnitStatHeaderTextTable[labelIndex + 0x04],
            0x01, topY + yOffset, 0x0F, 0x00);
        yOffset += 0x09;
    }
}

// 2507:6150: allocate the four display backing buffers used by the lance
// configuration editor.
// VERIFIED against ASM 2507:6150-6198.
void LoadLanceConfigurationGraphicsBuffers(void)
{
    uint16_t tableIndex;
    uint16_t bufferIndex;
    uint8_t *backingBuffer;

    bufferIndex = 0x0013;
    for (tableIndex = 0x0000; tableIndex < 0x0004; ++tableIndex)
    {
        backingBuffer = BuildDisplayBackingBuffer(
            LanceEditorLayout.LanceBackingSourceTileX[tableIndex],
            LanceEditorLayout.LanceBackingSourcePixelY[tableIndex],
            LanceEditorLayout.LanceBackingWidthTiles[tableIndex],
            LanceEditorLayout.LanceBackingHeightPixels[tableIndex]);
        UnitEditorGraphicsBuffer(bufferIndex) = backingBuffer;
        ++bufferIndex;
    }
}

// 2507:6199: allocate the two display backing buffers used by the scenario
// variant selector.
// VERIFIED against ASM 2507:6199-61E1.
void LoadScenarioVariantGraphicsBuffers(void)
{
    uint16_t tableIndex;
    uint16_t bufferIndex;
    uint8_t *backingBuffer;

    bufferIndex = 0x0011;
    for (tableIndex = 0x0000; tableIndex < 0x0002; ++tableIndex)
    {
        backingBuffer = BuildDisplayBackingBuffer(
            LanceEditorLayout.ScenarioBackingSourceTileX[tableIndex],
            LanceEditorLayout.ScenarioBackingSourcePixelY[tableIndex],
            LanceEditorLayout.ScenarioBackingWidthTiles[tableIndex],
            LanceEditorLayout.ScenarioBackingHeightPixels[tableIndex]);
        UnitEditorGraphicsBuffer(bufferIndex) = backingBuffer;
        ++bufferIndex;
    }
}

// 2507:61E2: build an inclusive indexed range of display backing buffers.
// VERIFIED against ASM 2507:61E2-623C.
void LoadGraphicsBufferRange(
    int16_t firstBufferIndex,
    int16_t lastBufferIndex)
{
    int16_t rangeOffset;
    int16_t bufferIndex;
    uint8_t *backingBuffer;

    bufferIndex = firstBufferIndex;
    rangeOffset = 0x0000;
    do
    {
        backingBuffer = BuildDisplayBackingBuffer(
            LanceEditorLayout.EditorBackingSourceTileX[rangeOffset],
            LanceEditorLayout.EditorBackingSourcePixelY[rangeOffset],
            LanceEditorLayout.EditorBackingWidthTiles[rangeOffset],
            LanceEditorLayout.EditorBackingHeightPixels[rangeOffset]);
        UnitEditorGraphicsBuffer((uint16_t) bufferIndex) = backingBuffer;
        ++bufferIndex;
        ++rangeOffset;
    } while (rangeOffset <= (int16_t) (lastBufferIndex - firstBufferIndex));
}

// 2507:623D: free an inclusive range of indexed graphics backing buffers.
// Called from:
//      PrepareMissionDateScreen
// VERIFIED against ASM 2507:623D-625F and the segment-end epilogue.
void FreeGraphicsBufferRange(int16_t firstBufferIndex, int16_t lastBufferIndex)
{
    int16_t bufferIndex;
    for (bufferIndex = firstBufferIndex; bufferIndex <= lastBufferIndex; ++bufferIndex)
    {
        Heap_Free(UnitEditorGraphicsBuffer(bufferIndex));
    }
}

const uint8_t *LanceEditorCardSprite(const UnitSelectionCard *card)
{
    const uint8_t *sprite = card->spriteBuffer;
    uint16_t bufferIndex;

    for (bufferIndex = 0x0000;
        bufferIndex < sizeof(SharedGraphicsBackingBuffers) / sizeof(SharedGraphicsBackingBuffers[0]);
        ++bufferIndex)
    {
        if (sprite != NULL && sprite == UnitEditorGraphicsBuffer(bufferIndex))
            return sprite;
    }
    RevengeFault("a selection card's sprite pointer %04X:%04X is none of the "
                 "editor's graphics buffers (the card was drawn before it was built)",
        (unsigned) card->loadedSpriteBuffer.segment,
        (unsigned) card->loadedSpriteBuffer.offset);
}
