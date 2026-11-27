#include "fake_backend.h"
#include "revenge/original_data.h"
#include "revenge/video.h"

#include <assert.h>
#include <string.h>

void SetActiveRenderTarget(byte *renderTarget);
void Draw8x8VideoTile(
    const byte *tilePixels,
    int16 x,
    word16 y);

int main(void)
{
    static byte primaryScreen[VIDEO_SCREEN_WIDTH * VIDEO_SCREEN_HEIGHT];
    byte vgaPalette[VGA_PALETTE_BYTE_COUNT];
    byte tile[0x40];
    word16 pixel;
    word16 row;

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
    VideoFramebuffer = (byte *)(uintptr_t)0xA0000000UL;
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
        tile[pixel] = (byte) (0x40 + pixel);
    Video_ClearIndexedFramebuffer(0xA5);
    Draw8x8VideoTile(tile, 0x0005, 0x0003);
    for (row = 0x0000; row < 0x0008; ++row)
        for (pixel = 0x0000; pixel < 0x0008; ++pixel)
            assert(fakeBackend.indexedFramebuffer[
                (0x0018 + row) * VIDEO_SCREEN_WIDTH + 0x0028 + pixel] ==
                (byte)(0x40 + row * 0x08 + pixel));
    assert(fakeBackend.indexedFramebuffer[
        0x0018 * VIDEO_SCREEN_WIDTH + 0x0030] == 0xA5);

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
