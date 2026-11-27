// 3005: tactical sprite
// TacticalSprite.cpp - tactical clipped-sprite dispatch
// Tactical clipped-sprite rendering wrapper.

#include "REVENGE.h"
#include "revenge/original_data.h"
#include "revenge/video.h"

// 3005:0008
void DrawTacticalClippedSprite(
    uint8_t *renderTarget,
    const uint8_t *encodedSprite,
    int16_t relativeX,
    int16_t relativeY,
    uint16_t textWindowIndex)
{
    TextWindowContext *textWindow;
    int16_t screenX;
    int16_t screenY;

    if (SpriteDrawBusy != FALSE)
        return;

    SpriteDrawBusy = TRUE;
    textWindow = &TextWindowContexts[textWindowIndex];
    TacticalSpriteClipLeftTile = textWindow->leftTile;
    SpriteClipLeftTileMirror = textWindow->leftTile;
    TacticalSpriteClipRightTile =
        textWindow->leftTile + textWindow->widthTiles;
    SpriteClipRightTileMirror = TacticalSpriteClipRightTile;
    TacticalSpriteClipTopPixel =
        textWindow->topTile << 0x03;
    SpriteClipTopPixelMirror = TacticalSpriteClipTopPixel;
    TacticalSpriteClipBottomPixel = TacticalSpriteClipTopPixel +
        (textWindow->heightTiles << 0x03);
    SpriteClipBottomPixelMirror = TacticalSpriteClipBottomPixel;

    screenX = relativeX +
        (TacticalSpriteClipLeftTile << 0x03);
    screenY = relativeY + TacticalSpriteClipTopPixel;

    /* The host decoders retain segment 4184's private clipping copies. */
    EncodedSpriteClipLeftTile = TacticalSpriteClipLeftTile;
    SpriteClipRightTile = TacticalSpriteClipRightTile;
    SpriteClipTopPixel = TacticalSpriteClipTopPixel;
    EncodedSpriteClipBottomPixel = TacticalSpriteClipBottomPixel;
    /* The EGA's A200 routing and decoder (3005:009F, 0800:30FD) went
	 * with the EGA; the rebuild is VGA only. */
    if (VideoAdapterType != VIDEO_ADAPTER_EGA)
        DrawCgaTandyEncodedSprite(
            renderTarget,
            encodedSprite,
            screenX,
            screenY);

    SpriteDrawBusy = FALSE;
}
