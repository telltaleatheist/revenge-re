// 0D73: sprite
/* Clipped-sprite procedure from original segment 0D73. */

#include "REVENGE.h"

#include "revenge/original_data.h"
#include "revenge/video.h"

/* 0D73:566E-57AA */
void DrawClippedSprite(
    uint8_t *destination,
    const uint8_t *encodedSprite,
    int16_t x,
    int16_t y,
    uint16_t clippingRegionIndex)
{
    TextWindowContext *clippingRegion;
    int16_t clippedX;
    int16_t clippedY;

    if (SpriteDrawBusy != FALSE)
        return;

    SpriteDrawBusy = TRUE;
    clippingRegion = &TextWindowContexts[clippingRegionIndex];
    EncodedSpriteClipLeftTile = clippingRegion->leftTile;
    SpriteClipLeftTileMirror = clippingRegion->leftTile;
    SpriteClipRightTile =
        clippingRegion->leftTile + clippingRegion->widthTiles;
    SpriteClipRightTileMirror = SpriteClipRightTile;
    SpriteClipTopPixel = clippingRegion->topTile << 0x03;
    SpriteClipTopPixelMirror = SpriteClipTopPixel;
    EncodedSpriteClipBottomPixel = SpriteClipTopPixel +
        (clippingRegion->heightTiles << 0x03);
    SpriteClipBottomPixelMirror = EncodedSpriteClipBottomPixel;

    clippedX = x + (int16_t) (EncodedSpriteClipLeftTile << 0x03);
    clippedY = y + (int16_t) SpriteClipTopPixel;
    if (encodedSprite != NULL)
    {
        /* The CGA's and the Tandy's decoder (0D73:5743, 0800:14B7) and the
		 * EGA's (0D73:570F, 5777, 0800:30FD), with its A200
		 * routing, went with those adapters; the rebuild is VGA only. */
        if (VideoAdapterType == VIDEO_ADAPTER_VGA)
        {
            DrawVgaRleSprite(
                destination, encodedSprite, clippedX, clippedY);
        }
    }
    SpriteDrawBusy = FALSE;
}
