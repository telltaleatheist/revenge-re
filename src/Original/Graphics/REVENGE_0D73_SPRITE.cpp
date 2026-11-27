/* Clipped-sprite procedure from original segment 0D73. */

#include "REVENGE.h"

#include "revenge/original_data.h"
#include "revenge/video.h"

/* 0D73:566E-57AA */
void DrawClippedSprite(
    byte *destination,
    const byte *encodedSprite,
    int16 x,
    int16 y,
    word16 clippingRegionIndex)
{
    TextWindowContext *clippingRegion;
    int16 clippedX;
    int16 clippedY;

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

    clippedX = x + (int16) (EncodedSpriteClipLeftTile << 0x03);
    clippedY = y + (int16) SpriteClipTopPixel;
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
