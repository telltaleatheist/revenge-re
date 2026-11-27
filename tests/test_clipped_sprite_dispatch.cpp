#include "fake_backend.h"
#include "Data/RuntimeData.h"
#include "Data/GraphicsData.h"
#include "Data/TacticalGraphicsData.h"
#include "Data/TextIOData.h"
#include "Graphics/Graphics.h"
#include "revenge/video.h"

#include <assert.h>

void DrawClippedSprite(
    uint8_t *destination,
    const uint8_t *encodedSprite,
    int16_t x,
    int16_t y,
    uint16_t clippingRegionIndex);

static void ResetState(void)
{
    FakeBackend_Reset();
    SpriteDrawBusy = FALSE;
    TextWindowContexts[0x02].leftTile = 0x0002;
    TextWindowContexts[0x02].topTile = 0x0001;
    TextWindowContexts[0x02].widthTiles = 0x0001;
    TextWindowContexts[0x02].heightTiles = 0x0001;
}

static void TestVgaDispatchAndWindowRelativeCoordinates(void)
{
    const uint8_t sprite[] = {
        0x00, 0x00, 0x01, 0x00,
        0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C
    };
    uint32_t row = 0x0008U * VIDEO_SCREEN_WIDTH;

    ResetState();
    VideoAdapterType = VIDEO_ADAPTER_VGA;
    ActiveVideoAdapterType = VIDEO_ADAPTER_VGA;
    DrawClippedSprite(NULL, sprite, 0x0000, 0x0000, 0x0002);

    assert(SpriteDrawBusy == FALSE);
    assert(EncodedSpriteClipLeftTile == 0x0002);
    assert(SpriteClipRightTile == 0x0003);
    assert(SpriteClipTopPixel == 0x0008);
    assert(EncodedSpriteClipBottomPixel == 0x0010);
    assert(SpriteClipLeftTileMirror == EncodedSpriteClipLeftTile);
    assert(SpriteClipRightTileMirror == SpriteClipRightTile);
    assert(SpriteClipTopPixelMirror == SpriteClipTopPixel);
    assert(SpriteClipBottomPixelMirror == EncodedSpriteClipBottomPixel);
    assert(fakeBackend.indexedFramebuffer[row + 0x0010] == 0x05);
    assert(fakeBackend.indexedFramebuffer[row + 0x0017] == 0x0C);
}

static void TestExplicitDestinationIsHonored(void)
{
    const uint8_t sprite[] = {
        0x00, 0x00, 0x01, 0x00,
        0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C
    };
    uint8_t destination[VIDEO_SCREEN_WIDTH * VIDEO_SCREEN_HEIGHT] = { 0x00 };
    uint32_t row = 0x0008U * VIDEO_SCREEN_WIDTH;

    ResetState();
    VideoAdapterType = VIDEO_ADAPTER_VGA;
    ActiveVideoAdapterType = VIDEO_ADAPTER_VGA;
    ActiveRenderTarget = fakeBackend.indexedFramebuffer;
    DrawClippedSprite(destination, sprite, 0x0000, 0x0000, 0x0002);

    assert(destination[row + 0x0010] == 0x05);
    assert(destination[row + 0x0017] == 0x0C);
    assert(fakeBackend.indexedFramebuffer[row + 0x0010] == 0x00);
    assert(ActiveRenderTarget == fakeBackend.indexedFramebuffer);
}

static void TestReentryGuardAndNullSprite(void)
{
    ResetState();
    EncodedSpriteClipLeftTile = 0x1234;
    SpriteDrawBusy = TRUE;
    DrawClippedSprite(NULL, NULL, 0x0000, 0x0000, 0x0002);
    assert(SpriteDrawBusy == TRUE);
    assert(EncodedSpriteClipLeftTile == 0x1234);

    SpriteDrawBusy = FALSE;
    DrawClippedSprite(NULL, NULL, 0x0000, 0x0000, 0x0002);
    assert(SpriteDrawBusy == FALSE);
    assert(EncodedSpriteClipLeftTile == 0x0002);
}

int main(void)
{
    TestVgaDispatchAndWindowRelativeCoordinates();
    TestExplicitDestinationIsHonored();
    TestReentryGuardAndNullSprite();
    return 0;
}
