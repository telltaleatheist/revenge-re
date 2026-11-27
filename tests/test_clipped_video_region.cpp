#include "REVENGE.h"
#include "revenge/original_data.h"
#include "revenge/video.h"

#include <assert.h>

typedef struct CopyCall
{
    int16_t sourceX;
    int16_t sourceY;
    int16_t destinationX;
    int16_t destinationY;
    uint16_t width;
    uint16_t height;
    const uint8_t *source;
    uint8_t *destination;
} CopyCall;

static CopyCall copyCall;

void CopyVideoRegion(
    int16_t sourceX,
    int16_t sourceY,
    int16_t destinationX,
    int16_t destinationY,
    uint16_t width,
    uint16_t height,
    const uint8_t *source,
    uint8_t *destination)
{
    copyCall.sourceX = sourceX;
    copyCall.sourceY = sourceY;
    copyCall.destinationX = destinationX;
    copyCall.destinationY = destinationY;
    copyCall.width = width;
    copyCall.height = height;
    copyCall.source = source;
    copyCall.destination = destination;
}

int main(void)
{
    uint8_t sourceBuffer[0x01] = { 0x00 };
    uint8_t destinationBuffer[0x01] = { 0x00 };
    uint8_t *primaryScreenBuffer;
    static uint8_t otherScreen[0x10];

    VideoAdapterType = VIDEO_ADAPTER_CGA;
    CopyClippedVideoRegion(
        -0x0002,
        -0x0003,
        0x0003,
        0x0004,
        0x0032,
        0x00FA,
        sourceBuffer,
        destinationBuffer);
    assert(copyCall.sourceX == 0x0000);
    assert(copyCall.sourceY == 0x0000);
    assert(copyCall.destinationX == 0x0005);
    assert(copyCall.destinationY == 0x0007);
    assert(copyCall.width == 0x0026);
    assert(copyCall.height == 0x00C5);
    assert(copyCall.source == sourceBuffer);
    assert(copyCall.destination == destinationBuffer);
    CopyClippedVideoRegion(
        0x0005,
        0x0006,
        -0x0002,
        -0x0003,
        0x000A,
        0x0014,
        sourceBuffer,
        destinationBuffer);
    assert(copyCall.sourceX == 0x0007);
    assert(copyCall.sourceY == 0x0009);
    assert(copyCall.destinationX == 0x0000);
    assert(copyCall.destinationY == 0x0000);
    assert(copyCall.width == 0x0008);
    assert(copyCall.height == 0x0011);

    primaryScreenBuffer = otherScreen;
    PrimaryScreenBuffer = primaryScreenBuffer;
    VideoAdapterType = VIDEO_ADAPTER_VGA;
    CopyClippedVideoRegion(
        0x0001,
        0x0002,
        0x0003,
        0x0004,
        0x0005,
        0x0006,
        primaryScreenBuffer,
        primaryScreenBuffer);
    assert(copyCall.source == primaryScreenBuffer);
    assert(copyCall.destination == primaryScreenBuffer);
    return 0x00;
}
