// 0800, 0D73, 2B2D, 2FEA, 3005, 334A, 334D, 3365, 337C, 3386, 3471, 3495, 34B8, 34D5, 34DF, 3596, 35C2: Graphics declarations
#ifndef REVENGE_GRAPHICS_GRAPHICS_H
#define REVENGE_GRAPHICS_GRAPHICS_H

#include "revenge/types.h"

void SetVideoAdapterType(uint16_t adapterType);
void RebuildTacticalVideoResources(
    uint16_t adapterType,
    uint16_t showScenarioCompletion);
void DrawMissionHeaderFrame(void);
void WaitVerticalRetraces(uint16_t retraceCount);
void WaitForVerticalRetraceEdge(uint8_t waitForRetraceStart);
void CalibrateVideoTiming(void);

uint16_t GeneratePseudoRandomByte(void);
uint16_t EncodeDisplayBackingRows(
    const uint8_t *source,
    uint8_t *destination,
    uint16_t groupsPerRow,
    uint8_t rowCount,
    int16_t sourceRowSkip);
void InstallDamagedTerrainSpriteLookup(void);
void ClearActiveVideoBuffer(void);
void DrawVerticalSpan(
    uint16_t x,
    uint16_t yStart,
    uint16_t yEnd,
    uint16_t color);
void DrawHorizontalVideoRun(
    int16_t x,
    uint16_t y,
    uint16_t packedWidth,
    uint16_t color);
void Draw8x8VideoTile(
    const uint8_t *tilePixels,
    int16_t tileX,
    uint16_t tileY);
void CopyBufferToVideoRegion(
    int16_t xColumn,
    uint16_t yCharacterRow,
    uint16_t widthColumns,
    uint16_t heightCharacterRows,
    const uint8_t *source);
void CopyVideoRegionToBuffer(
    int16_t xColumn,
    uint16_t yCharacterRow,
    uint16_t widthColumns,
    uint16_t heightCharacterRows,
    uint8_t *destination);
void DrawClippedSprite(
    uint8_t *destination,
    const uint8_t *encodedSprite,
    int16_t x,
    int16_t y,
    uint16_t preserveExistingPixels);
void LoadVideoPalette(const uint8_t *paletteData);
void InstallVgaPaletteBankAnchors(void);
uint8_t *BuildDisplayBackingBuffer(
    uint16_t sourceTileX,
    uint16_t sourcePixelY,
    uint16_t widthTiles,
    uint16_t heightPixels);
uint8_t *BuildVgaDisplayBackingBuffer(
    uint16_t sourceTileX,
    uint16_t sourcePixelY,
    uint16_t widthTiles,
    uint16_t heightPixels);
uint8_t *BuildPlanarDisplayBackingBuffer(
    uint16_t sourceTileX,
    uint16_t sourcePixelY,
    uint16_t widthTiles,
    uint16_t heightPixels);
void RebuildMouseCursorBackingBuffer(void);
void LoadGraphicsResourceToBuffers(
    uint16_t resourceIndex,
    uint8_t *decodeBuffer,
    uint8_t *displayBuffer);
void InstallGameClockTimer(void);
void RestoreGameClockTimer(void);
void GameClockInt1CHandler(void);
void DrawClippedPixel(int16_t x, int16_t y, uint16_t color);
void DrawHorizontalSpan(
    int16_t y, int16_t startX, int16_t endX, uint16_t color);
void DrawClippedAxisAlignedLine(
    int16_t x1,
    int16_t y1,
    int16_t x2,
    int16_t y2,
    uint16_t color);
uint16_t DrawRepeatingFrameTileSequence(
    const uint16_t *frameDefinition,
    uint16_t sequenceIndex,
    int16_t x,
    int16_t y,
    uint16_t tileCount,
    uint16_t horizontal);
void DrawActiveTextWindowFrame(uint16_t frameStyleIndex);
void DrawTacticalClippedSprite(
    uint8_t *renderTarget,
    const uint8_t *encodedSprite,
    int16_t relativeX,
    int16_t relativeY,
    uint16_t textWindowIndex);
void DrawClippedRectangleOutline(
    int16_t left,
    int16_t top,
    int16_t right,
    int16_t bottom,
    uint16_t color);
void FillVideoRectangle(
    int16_t leftX,
    int16_t topY,
    int16_t rightX,
    int16_t bottomY,
    uint16_t color);
void Capture8x8VideoTile(
    uint8_t *framebuffer,
    uint8_t *destination,
    int16_t tileX,
    uint16_t tileY);
void LoadPackedIconPageDirect(
    const char *resourcePath,
    uint8_t *packedDisplayBuffer,
    uint8_t *iconPageDestination);
uint8_t *CaptureVideoTileStrip(
    uint8_t *source,
    int16_t startTileX,
    uint16_t startTileY,
    int16_t tileCount);
void CopyDecodedIconPage(
    const uint8_t *packedDisplayBuffer,
    uint8_t *iconPageDestination);
void SetActiveRenderTarget(uint8_t *renderTarget);

#endif
