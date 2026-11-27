#include "Data/RuntimeData.h"
#include "Graphics/Graphics.h"
#include "revenge/heap.h"
#include "revenge/video.h"

#include <assert.h>

typedef struct CaptureCall
{
    uint8_t *framebuffer;
    uint8_t *destination;
    int16_t tileX;
    uint16_t tileY;
} CaptureCall;

static uint8_t allocation[0x0200];
static int32_t requestedAllocationSize;
static CaptureCall captureCalls[0x08];
static uint16_t captureCallCount;

void *Heap_Allocate(uint32_t byteCount, const char *caller)
{
    (void) caller;
    requestedAllocationSize = byteCount;
    return allocation;
}

void Capture8x8VideoTile(
    uint8_t *framebuffer,
    uint8_t *destination,
    int16_t tileX,
    uint16_t tileY)
{
    CaptureCall *call;

    call = &captureCalls[captureCallCount++];
    call->framebuffer = framebuffer;
    call->destination = destination;
    call->tileX = tileX;
    call->tileY = tileY;
}

static void TestStrip(uint16_t adapterType, int32_t expectedAllocation,
    uint16_t expectedTileStride)
{
    uint8_t framebuffer[0x10];
    uint8_t *result;

    captureCallCount = 0x0000;
    requestedAllocationSize = 0x00000000;
    VideoAdapterType = adapterType;
    result = CaptureVideoTileStrip(
        framebuffer, 0x0027, 0x0005, 0x0003);

    assert(result == allocation);
    assert(requestedAllocationSize == expectedAllocation);
    assert(captureCallCount == 0x0003);
    assert(captureCalls[0x00].framebuffer == framebuffer);
    assert(captureCalls[0x00].destination == allocation);
    assert(captureCalls[0x00].tileX == 0x0027);
    assert(captureCalls[0x00].tileY == 0x0005);
    assert(captureCalls[0x01].destination == allocation + expectedTileStride);
    assert(captureCalls[0x01].tileX == 0x0000);
    assert(captureCalls[0x01].tileY == 0x0006);
    assert(captureCalls[0x02].destination ==
        allocation + expectedTileStride * 0x02);
    assert(captureCalls[0x02].tileX == 0x0001);
    assert(captureCalls[0x02].tileY == 0x0006);
}

int main(void)
{
    TestStrip(VIDEO_ADAPTER_CGA, 0x00000060, 0x0020);
    TestStrip(VIDEO_ADAPTER_EGA, 0x00000060, 0x0020);
    TestStrip(VIDEO_ADAPTER_TANDY, 0x00000060, 0x0020);
    TestStrip(VIDEO_ADAPTER_VGA, 0x000000C0, 0x0040);
    return 0;
}
