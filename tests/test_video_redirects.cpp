#include "fake_backend.h"
#include "Data/RuntimeData.h"
#include "Data/StartupData.h"
#include "Data/TacticalData.h"
#include "Data/TacticalGraphicsData.h"
#include "Data/TextIOData.h"
#include "Graphics/Graphics.h"
#include "Runtime/Runtime.h"
#include "revenge/video.h"

#include <assert.h>
#include <string.h>

void SetActiveRenderTarget(uint8_t *renderTarget);
void Draw8x8VideoTile(
    const uint8_t *tilePixels,
    int16_t x,
    uint16_t y);

int main(void)
{
    static uint8_t primaryScreen[VIDEO_SCREEN_WIDTH * VIDEO_SCREEN_HEIGHT];
    uint8_t vgaPalette[VGA_PALETTE_BYTE_COUNT];
    uint8_t tile[0x40];
    uint16_t pixel;
    uint16_t row;

    FakeBackend_Reset();
    fakeBackend.videoMode = 0x03;

    assert(GetCurrentBIOSVideoMode() == 0x03);
    SavedBIOSVideoMode = GetCurrentBIOSVideoMode();
    SetVideoModeForAdapter(VIDEO_ADAPTER_VGA);
    assert(ActiveVideoAdapterType == VIDEO_ADAPTER_VGA);
    assert(fakeBackend.videoMode == 0x13);
    SetVideoModeForAdapter(0x0006);
    assert(fakeBackend.videoMode == 0x03);
    assert(ActiveVideoAdapterType == VIDEO_ADAPTER_VGA);

    memset(vgaPalette, 0x2A, sizeof(vgaPalette));
    VGA_LoadPalette(vgaPalette);
    assert(memcmp(fakeBackend.vgaPalette, vgaPalette,
               sizeof(vgaPalette)) == 0x00);
    Video_ClearIndexedFramebuffer(0xA5);
    assert(fakeBackend.indexedFramebuffer[0x0000] == 0xA5);
    DrawClippedPixel(0x0001, 0x0001, 0x00C7);
    assert(fakeBackend.indexedFramebuffer[0x0141] == 0xC7);

    /* The VGA's colours are whole bytes and its runs one pixel a byte. */
    DrawVerticalSpan(0x0007, 0x0005, 0x0007, 0x00C6);
    assert(fakeBackend.indexedFramebuffer[0x0005 * 0x0140 + 0x0007] == 0xC6);
    assert(fakeBackend.indexedFramebuffer[0x0007 * 0x0140 + 0x0007] == 0xC6);
    assert(fakeBackend.indexedFramebuffer[0x0008 * 0x0140 + 0x0007] == 0xA5);
    DrawHorizontalVideoRun(0x0008, 0x0009, 0x0002, 0x00B7);
    assert(fakeBackend.indexedFramebuffer[0x0009 * 0x0140 + 0x0008] == 0xB7);
    assert(fakeBackend.indexedFramebuffer[0x0009 * 0x0140 + 0x0009] == 0xB7);
    assert(fakeBackend.indexedFramebuffer[0x0009 * 0x0140 + 0x000A] == 0xA5);

    Video_ClearIndexedFramebuffer(0x05);
    FillVideoTileRectangle(0x0002, 0x0003, 0x0004, 0x0002, 0xE1);
    assert(fakeBackend.indexedFramebuffer[0x0018 * 0x0140 + 0x0010] == 0xE1);
    assert(fakeBackend.indexedFramebuffer[0x0027 * 0x0140 + 0x002F] == 0xE1);
    assert(fakeBackend.indexedFramebuffer[0x0017 * 0x0140 + 0x0010] == 0x05);
    assert(fakeBackend.indexedFramebuffer[0x0018 * 0x0140 + 0x0030] == 0x05);

    /* The primary buffer as the render target takes the drawing; the
     * screen keeps its pixels. */
    memset(primaryScreen, 0x05, sizeof(primaryScreen));
    PrimaryScreenBuffer = primaryScreen;
    VideoFramebuffer = (uint8_t *) (uintptr_t) 0xA0000000UL;
    SetActiveRenderTarget(primaryScreen);
    assert(ActiveRenderTarget == primaryScreen);
    DrawClippedPixel(0x000A, 0x000B, 0x000C);
    FillVideoTileRectangle(0x0002, 0x0003, 0x0001, 0x0001, 0x00);
    assert(primaryScreen[0x000B * VIDEO_SCREEN_WIDTH + 0x000A] == 0x0C);
    assert(primaryScreen[0x0018 * VIDEO_SCREEN_WIDTH + 0x0010] == 0x00);
    assert(fakeBackend.indexedFramebuffer[0x000B * VIDEO_SCREEN_WIDTH +
               0x000A] == 0x05);
    VideoFramebuffer = NULL;
    ActiveRenderTarget = NULL;

    Video_ClearIndexedFramebuffer(0x05);
    XorVideoRectangle(0x0003, 0x0004, 0x0001, 0x0002, 0xFF);
    assert(fakeBackend.indexedFramebuffer[0x0002 * 0x0140 + 0x0001] == 0xFA);
    assert(fakeBackend.indexedFramebuffer[0x0004 * 0x0140 + 0x0003] == 0xFA);
    assert(fakeBackend.indexedFramebuffer[0x0001 * 0x0140 + 0x0001] == 0x05);

    /* An 8x8 tile is 64 bytes, eight to a row. */
    for (pixel = 0x0000; pixel < 0x0040; ++pixel)
        tile[pixel] = (uint8_t) (0x40 + pixel);
    Video_ClearIndexedFramebuffer(0xA5);
    Draw8x8VideoTile(tile, 0x0005, 0x0003);
    for (row = 0x0000; row < 0x0008; ++row)
        for (pixel = 0x0000; pixel < 0x0008; ++pixel)
            assert(fakeBackend.indexedFramebuffer[(0x0018 + row) * VIDEO_SCREEN_WIDTH + 0x0028 + pixel] ==
                (uint8_t) (0x40 + row * 0x08 + pixel));
    assert(fakeBackend.indexedFramebuffer[0x0018 * VIDEO_SCREEN_WIDTH + 0x0030] == 0xA5);

    Video_ClearIndexedFramebuffer(0xA5);
    ClearActiveVideoBuffer();
    assert(fakeBackend.indexedFramebuffer[0x0000] == 0x00);
    assert(fakeBackend.indexedFramebuffer[0xF9FF] == 0x00);

    VerticalRetraceEdgeMode = VERTICAL_RETRACE_WAIT_FOR_END;
    WaitVerticalRetraces(0x0003);
    assert(fakeBackend.retraceCount == 0x0003);
    assert(fakeBackend.firstByte == VERTICAL_RETRACE_WAIT_FOR_END);

    return 0x00;
}
