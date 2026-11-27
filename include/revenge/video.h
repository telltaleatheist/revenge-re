#ifndef REVENGE_VIDEO_H
#define REVENGE_VIDEO_H

#include "revenge/types.h"

/* The VGA segment A000 is 64K: the screen's 64000 bytes, then video memory
 * off the screen (A000:FA00-FFFF). The host keeps all of it, because the
 * game's window saves read and write those rows (a text window reaching past
 * row 200). Phase 3's canvas must keep the off-screen rows too. */
#define VIDEO_SEGMENT_SIZE 0x10000UL

/* Plain ints, not enumerators: C++20 deprecates arithmetic between two
 * unnamed enums, and these meet other modules' constants and floats. */
inline constexpr int VIDEO_SCREEN_WIDTH = 0x0140;
inline constexpr int VIDEO_SCREEN_HEIGHT = 0x00C8;

enum
{
    VIDEO_SCREEN_MIN_X = 0x0000,
    VIDEO_SCREEN_MAX_X = VIDEO_SCREEN_WIDTH - 0x0001,
    VIDEO_SCREEN_MIN_Y = 0x0000,
    VIDEO_SCREEN_MAX_Y = VIDEO_SCREEN_HEIGHT - 0x0001,
    VIDEO_SCREEN_8PX_COLUMN_COUNT = 0x0028,
    VIDEO_SCREEN_MIN_COLUMN = 0x0000,
    EGA_LOGICAL_PALETTE_ENTRY_COUNT = 0x10,
    VGA_DAC_ENTRY_COUNT = 0x0100,
    VGA_PALETTE_BYTE_COUNT = 0x0300,
    VIDEO_ADAPTER_CGA = 0x00,
    VIDEO_ADAPTER_TANDY = 0x01,
    VIDEO_ADAPTER_EGA = 0x02,
    VIDEO_ADAPTER_VGA = 0x03,
    VIDEO_ADAPTER_MCGA_CONFIG = 0x07,
    VERTICAL_RETRACE_WAIT_FOR_END = 0x00,
    VERTICAL_RETRACE_WAIT_FOR_START = 0x01
};

extern uint16_t VerticalRetraceEdgeMode;

uint16_t GetCurrentBIOSVideoMode(void);
void SetVideoModeForAdapter(uint16_t adapterOrRestoreSlot);
void VGA_LoadPalette(const uint8_t *paletteRgbTriplets);
void SetVideoAdapterType(uint16_t adapterType);
void WaitForVerticalRetraceEdge(uint8_t waitForRetraceStart);
void WaitVerticalRetraces(uint16_t retraceCount);
uint8_t *Video_GetIndexedFramebuffer(void);
void Video_ClearIndexedFramebuffer(uint8_t color);
/* Host termination points for original procedures which address DOS video
 * pages through segment-shaped pointer values. */
uint8_t *Video_ResolveIndexedRenderTarget(const uint8_t *renderTarget);
void Video_DrawIndexedPixelToActiveTarget(uint16_t x, uint16_t y, uint8_t color);
/* SDL replacement for the visibility of an immediate write to DOS video
 * memory.  Use only for transient frames which the preserved game draws and
 * then overwrites before the host's ordinary presentation poll. */
void Video_PresentTransientFrame(void);
void DrawClippedPixel(int16_t x, int16_t y, uint16_t color);
void DrawClippedAxisAlignedLine(
    int16_t x1,
    int16_t y1,
    int16_t x2,
    int16_t y2,
    uint16_t color);
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
void XorVideoRectangle(
    int16_t leftX,
    int16_t topY,
    int16_t rightX,
    int16_t bottomY,
    uint8_t color);
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
void FillVideoTileRectangle(
    int16_t tileX,
    uint16_t tileY,
    uint16_t widthTiles,
    uint16_t heightTiles,
    uint8_t color);
void CopyClippedVideoRegion(
    int16_t sourceXColumn,
    int16_t sourceY,
    int16_t destinationXColumn,
    int16_t destinationY,
    uint16_t widthColumns,
    uint16_t heightPixels,
    const uint8_t *source,
    uint8_t *destination);
void ClearActiveVideoBuffer(void);
void DrawVerticalSpan(
    uint16_t x,
    uint16_t yStart,
    uint16_t yEnd,
    uint16_t color);
void DrawHorizontalVideoRun(
    int16_t x,
    uint16_t y,
    uint16_t nativeRunLength,
    uint16_t color);
void DrawNonEgaTacticalTile(
    const uint8_t *spritePage,
    uint16_t spriteId,
    uint16_t destinationX,
    uint16_t destinationY);
void DrawTacticalTileSprite(
    const uint8_t *spritePage,
    uint16_t spriteId,
    uint16_t destinationX,
    uint16_t destinationY);
void PlotPixel(uint16_t x, uint16_t y, uint8_t color);
void DrawCompositeTacticalTile(
    uint16_t foregroundSpriteId,
    uint16_t backgroundSpriteId,
    const uint8_t *spritePage,
    uint16_t destinationX,
    uint16_t destinationY);
void DrawCgaTandyEncodedSprite(
    uint8_t *destination,
    const uint8_t *encodedSprite,
    int16_t x,
    int16_t y);
void DrawVgaRleSprite(
    uint8_t *destination,
    const uint8_t *encodedSprite,
    int16_t x,
    int16_t y);

#endif
