// 0800: encoded sprites
/* Adapter sprite decoders from REVENGE_0800. */

#include "Data/TacticalData.h"
#include "Data/TacticalGraphicsData.h"
#include "Graphics/Graphics.h"
#include "revenge/video.h"

/* 0800:14B7-1B10 */
void DrawCgaTandyEncodedSprite(
    uint8_t *destination,
    const uint8_t *encodedSprite,
    int16_t x,
    int16_t y)
{
    uint16_t height = (uint16_t) encodedSprite[0x01] + 0x0001;
    uint16_t widthUnits = encodedSprite[0x02];
    const uint8_t *pixels = encodedSprite + 0x04;
    uint16_t row;
    uint8_t *savedRenderTarget = ActiveRenderTarget;

    /* Host replacement for the original ES:DI destination addressing. */
    ActiveRenderTarget = destination;
    /* The CGA's two-bit masked rows went with the CGA; the VGA takes the
	 * Tandy's four-bit rows below (3005:0008 calls this for every adapter
	 * but the EGA). */
    {
        uint16_t rowByteCount = widthUnits << 0x02;

        for (row = 0x0000; row < height; ++row)
        {
            uint16_t packedX;

            for (packedX = 0x0000; packedX < rowByteCount; ++packedX)
            {
                uint8_t packedPixels = pixels[(uint32_t) row * rowByteCount + packedX];
                uint8_t highColor = packedPixels >> 0x04;
                uint8_t lowColor = packedPixels & 0x0F;
                int16_t pixelX = (int16_t) (x + (packedX << 0x01));
                int16_t pixelY = (int16_t) (y + row);

                if (highColor != 0x00 &&
                    pixelX >= (int16_t) (EncodedSpriteClipLeftTile << 0x03) &&
                    pixelX < (int16_t) (SpriteClipRightTile << 0x03) &&
                    pixelY >= (int16_t) SpriteClipTopPixel &&
                    pixelY < (int16_t) EncodedSpriteClipBottomPixel)
                    DrawClippedPixel(pixelX, pixelY, highColor);
                if (lowColor != 0x00 &&
                    pixelX + 0x01 >=
                        (int16_t) (EncodedSpriteClipLeftTile << 0x03) &&
                    pixelX + 0x01 <
                        (int16_t) (SpriteClipRightTile << 0x03) &&
                    pixelY >= (int16_t) SpriteClipTopPixel &&
                    pixelY < (int16_t) EncodedSpriteClipBottomPixel)
                    DrawClippedPixel(pixelX + 0x01, pixelY, lowColor);
            }
        }
    }
    ActiveRenderTarget = savedRenderTarget;
}

/* The EGA's decoder (0800:30FD) went with the EGA; the rebuild is VGA
 * only. */

/* 0800:50A1-5265 */
void DrawVgaRleSprite(
    uint8_t *destination,
    const uint8_t *encodedSprite,
    int16_t x,
    int16_t y)
{
    uint16_t height = (uint16_t) encodedSprite[0x01] + 0x0001;
    uint16_t rowWidth = (uint16_t) encodedSprite[0x02] << 0x03;
    const uint8_t *source = encodedSprite + 0x04;
    uint16_t row;
    uint8_t *savedRenderTarget = ActiveRenderTarget;

    /* Host replacement for the original ES:DI destination addressing. */
    ActiveRenderTarget = destination;
    for (row = 0x0000; row < height; ++row)
    {
        uint16_t column = 0x0000;

        while (column < rowWidth)
        {
            uint8_t token = *source++;

            if (token == 0x00)
            {
                uint8_t transparentRun = *source++;

                column = (uint16_t) (column + transparentRun);
                continue;
            }
            if ((int16_t) (x + column) >=
                    (int16_t) (EncodedSpriteClipLeftTile << 0x03) &&
                (int16_t) (x + column) <
                    (int16_t) (SpriteClipRightTile << 0x03) &&
                (int16_t) (y + row) >= (int16_t) SpriteClipTopPixel &&
                (int16_t) (y + row) < (int16_t) EncodedSpriteClipBottomPixel)
            {
                DrawClippedPixel(
                    (int16_t) (x + column), (int16_t) (y + row), token);
            }
            ++column;
        }
    }
    ActiveRenderTarget = savedRenderTarget;
}
