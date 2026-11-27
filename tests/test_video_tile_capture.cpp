#include "REVENGE.h"
#include "revenge/original_data.h"
#include "revenge/video.h"

#include <assert.h>
#include <string.h>

static void FillRowMarkers(uint8_t *framebuffer, uint16_t rowStride,
    uint16_t firstOffset, uint16_t copiedBytes)
{
    uint16_t column;
    uint16_t row;

    for (row = 0x0000; row < 0x0008; ++row)
    {
        for (column = 0x0000; column < copiedBytes; ++column)
        {
            framebuffer[firstOffset + row * rowStride + column] =
                (uint8_t) (row * 0x10 + column);
        }
    }
}

static void AssertCapturedRows(const uint8_t *destination, uint16_t copiedBytes)
{
    uint16_t column;
    uint16_t row;

    for (row = 0x0000; row < 0x0008; ++row)
    {
        for (column = 0x0000; column < copiedBytes; ++column)
        {
            assert(destination[row * copiedBytes + column] ==
                (uint8_t) (row * 0x10 + column));
        }
    }
}

static void TestPackedAdapter(uint16_t adapterType, uint16_t rowStride,
    uint16_t tileRowStride, uint16_t tileByteWidth)
{
    uint8_t framebuffer[0x2000];
    uint8_t destination[0x0040];
    uint16_t firstOffset;

    memset(framebuffer, 0xCC, sizeof(framebuffer));
    memset(destination, 0xDD, sizeof(destination));
    firstOffset = (uint16_t) (tileRowStride + tileByteWidth);
    FillRowMarkers(framebuffer, rowStride, firstOffset, tileByteWidth);
    VideoAdapterType = adapterType;
    Capture8x8VideoTile(framebuffer, destination, 0x0001, 0x0001);
    AssertCapturedRows(destination, tileByteWidth);
}

int main(void)
{
    TestPackedAdapter(VIDEO_ADAPTER_VGA, 0x0140, 0x0A00, 0x0008);
    return 0;
}
