/*
 * Tactical 16x16 tile raster leaves from REVENGE_0800.
 *
 * The original procedures write adapter-specific packed bytes to CGA,
 * Tandy, EGA, or VGA memory; the rebuild is VGA only, and the CGA's and the
 * EGA's tile procedures and branches went with those adapters. The host
 * build preserves the VGA's source formats, coordinates, bank selection,
 * dispatch, and composition rules, then replaces only the physical
 * video-memory write with the indexed SDL framebuffer.
 */

#include "revenge/original_data.h"
#include "revenge/video.h"

enum
{
    TACTICAL_TILE_WIDTH = 0x0010,
    TACTICAL_TILE_HEIGHT = 0x0010,
    TACTICAL_PACKED_ROW_BYTES = 0x0008,
    TACTICAL_PACKED_TILE_BYTES = 0x0080,
    TACTICAL_HIGH_SPRITE_BANK_OFFSET = 0x8000
};

/* 0800:0B07-0C05. The DOS routine writes one packed two-by-two overview
 * pixel through each adapter's layout; the rebuild keeps the VGA's (the
 * CGA's colour tables went with the CGA). The host expands the same color
 * pair at the final video-memory boundary. */
void PlotPixel(word16 x, word16 y, byte color)
{
    byte leftColor;
    byte rightColor;
    int16 pixelX = (int16) (x << 0x01);
    int16 pixelY = (int16) (y << 0x01);

    leftColor = (byte) (color >> 0x04);
    rightColor = (byte) (color & 0x0F);
    DrawClippedPixel(pixelX, pixelY, leftColor);
    DrawClippedPixel(pixelX + 0x01, pixelY, rightColor);
    DrawClippedPixel(pixelX, pixelY + 0x01, leftColor);
    DrawClippedPixel(pixelX + 0x01, pixelY + 0x01, rightColor);
}

/* 0800:1040-1116; the CGA's two-bit tiles went with the CGA. */
void DrawNonEgaTacticalTile(
    const byte *spritePage,
    word16 spriteId,
    word16 destinationX,
    word16 destinationY)
{
    const byte *tile = spritePage +
        (uint32) spriteId * TACTICAL_PACKED_TILE_BYTES;
    word16 y;

    for (y = 0x0000; y < TACTICAL_TILE_HEIGHT; ++y)
    {
        word16 x;

        for (x = 0x0000; x < TACTICAL_TILE_WIDTH; ++x)
        {
            byte packedPixels;
            byte color;

            packedPixels = tile[(uint32) y * TACTICAL_PACKED_ROW_BYTES +
                (x >> 0x01)];
            color = (x & 0x0001) == 0x0000 ? packedPixels >> 0x04 : packedPixels & 0x0F;
            DrawClippedPixel(
                (int16) ((destinationX << 0x03) + x),
                (int16) (destinationY + y), color);
        }
    }
}

/* 0800:116F-1184 */
void DrawTacticalTileSprite(
    const byte *spritePage,
    word16 spriteId,
    word16 destinationX,
    word16 destinationY)
{
    /* The EGA's planar tile (0800:1117) went with the EGA. */
    DrawNonEgaTacticalTile(
        spritePage, spriteId, destinationX, destinationY);
}

/* 0800:2483-264A */
void DrawCompositeTacticalTile(
    word16 foregroundSpriteId,
    word16 backgroundSpriteId,
    const byte *spritePage,
    word16 destinationX,
    word16 destinationY)
{
    word16 row;

    if (ActiveVideoAdapterType != VIDEO_ADAPTER_EGA &&
        (foregroundSpriteId & 0xFF00) != 0x0000 &&
        (backgroundSpriteId & 0xFF00) != 0x0000)
    {
        foregroundSpriteId -= 0x0100;
        backgroundSpriteId -= 0x0100;
        spritePage += TACTICAL_HIGH_SPRITE_BANK_OFFSET;
    }

    /* The EGA's plane composition (0800:264B) and the CGA's masked tiles
	 * went with those adapters; the rebuild is VGA only. */
    for (row = 0x0000; row < TACTICAL_TILE_HEIGHT; ++row)
    {
        word16 x;

        for (x = 0x0000; x < TACTICAL_TILE_WIDTH; ++x)
        {
            const byte *foregroundTile = spritePage +
                (uint32) foregroundSpriteId * TACTICAL_PACKED_TILE_BYTES;
            const byte *backgroundTile = spritePage +
                (uint32) backgroundSpriteId * TACTICAL_PACKED_TILE_BYTES;
            byte foregroundPacked = foregroundTile[(uint32) row * TACTICAL_PACKED_ROW_BYTES +
                (x >> 0x01)];
            byte backgroundPacked = backgroundTile[(uint32) row * TACTICAL_PACKED_ROW_BYTES +
                (x >> 0x01)];
            byte foregroundColor = (x & 0x0001) == 0x0000 ? foregroundPacked >> 0x04 : foregroundPacked & 0x0F;
            byte backgroundColor = (x & 0x0001) == 0x0000 ? backgroundPacked >> 0x04 : backgroundPacked & 0x0F;
            byte color = foregroundColor != 0x00 ? foregroundColor : backgroundColor;

            DrawClippedPixel(
                (int16) ((destinationX << 0x03) + x),
                (int16) (destinationY + row), color);
        }
    }
}
