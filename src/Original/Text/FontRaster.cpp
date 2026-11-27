// 0800: font raster
/* Font raster procedures from REVENGE_0800. */

#include "Data/RuntimeData.h"
#include "Graphics/Graphics.h"
#include "Text/Text.h"
#include "revenge/text.h"
#include "revenge/video.h"

/* Mutable code-segment state used by 0800:4227 and 0800:470B. */
static uint8_t DrawColor;
static uint8_t BackgroundColor;

/* 0800:4227-470A */
void DrawFontGlyph(uint16_t glyphIndex, uint16_t x, uint16_t y)
{
    uint8_t *font = ActiveFontResource;
    uint16_t glyphWidth;
    uint16_t glyphHeight;
    uint16_t rowByteCount;
    uint16_t glyphOffset;
    uint16_t row;

    if (font == NULL || x >= VIDEO_SCREEN_WIDTH || y >= VIDEO_SCREEN_HEIGHT)
        return;
    glyphWidth = font[0x0103];
    glyphHeight = font[0x0102];
    if (glyphWidth == 0x0000 || glyphWidth >= 0x0019 ||
        x + glyphWidth > VIDEO_SCREEN_WIDTH ||
        y + glyphHeight > VIDEO_SCREEN_HEIGHT)
        return;
    rowByteCount = (uint16_t) ((glyphWidth + 0x0007) >> 0x03);
    glyphOffset = (uint16_t) (font[(glyphIndex + 0x0001) << 0x01] |
        ((uint16_t) font[((glyphIndex + 0x0001) << 0x01) + 0x0001] << 0x08));

    for (row = 0x0000; row < glyphHeight; ++row)
    {
        uint16_t column;

        for (column = 0x0000; column < glyphWidth; ++column)
        {
            uint8_t glyphByte = font[glyphOffset +
                (uint32_t) row * rowByteCount + (column >> 0x03)];
            uint16_t glyphBitSet =
                (glyphByte & (0x80 >> (column & 0x0007))) != 0x00;
            uint8_t color = glyphBitSet != FALSE ? DrawColor : BackgroundColor;

            if (color != 0x00)
                DrawClippedPixel(
                    (int16_t) (x + column), (int16_t) (y + row), color);
        }
    }
}

/* 0800:470B-484D */
void DrawTextString(
    const char *text,
    uint16_t x,
    uint16_t y,
    uint8_t foregroundColor,
    uint8_t backgroundColor)
{
    uint8_t *font = ActiveFontResource;
    uint16_t lineStartX = x;
    uint16_t currentX = x;
    uint16_t currentY = y;
    uint16_t glyphWidth;
    uint16_t lineHeight;

    if (font == NULL || x >= VIDEO_SCREEN_WIDTH)
        return;
    glyphWidth = font[0x0103];
    lineHeight = font[0x0102];
    DrawColor = foregroundColor;
    BackgroundColor = backgroundColor;

    while (*text != '\0')
    {
        if (*text == '\r')
        {
            currentX = lineStartX;
            currentY = (uint16_t) (currentY + lineHeight);
            ++text;
            continue;
        }
        if (currentX + glyphWidth > VIDEO_SCREEN_WIDTH)
        {
            currentX = lineStartX;
            currentY = (uint16_t) (currentY + lineHeight);
        }
        if (currentY >= VIDEO_SCREEN_HEIGHT)
            return;
        DrawFontGlyph((uint8_t) *text & 0x7F, currentX, currentY);
        currentX = (uint16_t) (currentX + glyphWidth);
        ++text;
    }
}
