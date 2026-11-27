// 0800: encoded sprites
/* Adapter sprite decoders from REVENGE_0800. */

#include "revenge/original_data.h"
#include "revenge/video.h"

/* 0800:14B7-1B10 */
void DrawCgaTandyEncodedSprite(
    byte *destination,
    const byte *encodedSprite,
    int16 x,
    int16 y)
{
    word16 height = (word16) encodedSprite[0x01] + 0x0001;
    word16 widthUnits = encodedSprite[0x02];
    const byte *pixels = encodedSprite + 0x04;
    word16 row;
    byte *savedRenderTarget = ActiveRenderTarget;

    /* Host replacement for the original ES:DI destination addressing. */
    ActiveRenderTarget = destination;
    /* The CGA's two-bit masked rows went with the CGA; the VGA takes the
	 * Tandy's four-bit rows below (3005:0008 calls this for every adapter
	 * but the EGA). */
    {
        word16 rowByteCount = widthUnits << 0x02;

        for (row = 0x0000; row < height; ++row)
        {
            word16 packedX;

            for (packedX = 0x0000; packedX < rowByteCount; ++packedX)
            {
                byte packedPixels = pixels[(uint32) row * rowByteCount + packedX];
                byte highColor = packedPixels >> 0x04;
                byte lowColor = packedPixels & 0x0F;
                int16 pixelX = (int16) (x + (packedX << 0x01));
                int16 pixelY = (int16) (y + row);

                if (highColor != 0x00 &&
                    pixelX >= (int16) (EncodedSpriteClipLeftTile << 0x03) &&
                    pixelX < (int16) (SpriteClipRightTile << 0x03) &&
                    pixelY >= (int16) SpriteClipTopPixel &&
                    pixelY < (int16) EncodedSpriteClipBottomPixel)
                    DrawClippedPixel(pixelX, pixelY, highColor);
                if (lowColor != 0x00 &&
                    pixelX + 0x01 >=
                        (int16) (EncodedSpriteClipLeftTile << 0x03) &&
                    pixelX + 0x01 <
                        (int16) (SpriteClipRightTile << 0x03) &&
                    pixelY >= (int16) SpriteClipTopPixel &&
                    pixelY < (int16) EncodedSpriteClipBottomPixel)
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
    byte *destination,
    const byte *encodedSprite,
    int16 x,
    int16 y)
{
    word16 height = (word16) encodedSprite[0x01] + 0x0001;
    word16 rowWidth = (word16) encodedSprite[0x02] << 0x03;
    const byte *source = encodedSprite + 0x04;
    word16 row;
    byte *savedRenderTarget = ActiveRenderTarget;

    /* Host replacement for the original ES:DI destination addressing. */
    ActiveRenderTarget = destination;
    for (row = 0x0000; row < height; ++row)
    {
        word16 column = 0x0000;

        while (column < rowWidth)
        {
            byte token = *source++;

            if (token == 0x00)
            {
                byte transparentRun = *source++;

                column = (word16) (column + transparentRun);
                continue;
            }
            if ((int16) (x + column) >=
                    (int16) (EncodedSpriteClipLeftTile << 0x03) &&
                (int16) (x + column) <
                    (int16) (SpriteClipRightTile << 0x03) &&
                (int16) (y + row) >= (int16) SpriteClipTopPixel &&
                (int16) (y + row) < (int16) EncodedSpriteClipBottomPixel)
            {
                DrawClippedPixel(
                    (int16) (x + column), (int16) (y + row), token);
            }
            ++column;
        }
    }
    ActiveRenderTarget = savedRenderTarget;
}
