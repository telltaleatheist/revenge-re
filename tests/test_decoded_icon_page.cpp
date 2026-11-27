#include "Data/RuntimeData.h"
#include "Graphics/Graphics.h"
#include "revenge/video.h"

#include <assert.h>

static uint16_t linearDecodeCount;
static uint16_t columnDecodeCount;
static const uint8_t *decoderSource;
static uint8_t *decoderDestination;

void DecodeLinearRleScreen(
    const uint8_t *compressedData,
    uint8_t *destination)
{
    ++linearDecodeCount;
    decoderSource = compressedData;
    decoderDestination = destination;
}

void DecodeColumnMajorRleScreen(
    const uint8_t *compressedData,
    uint8_t *destination)
{
    ++columnDecodeCount;
    decoderSource = compressedData;
    decoderDestination = destination;
}

static void ResetObservations(void)
{
    linearDecodeCount = 0x0000;
    columnDecodeCount = 0x0000;
    decoderSource = 0;
    decoderDestination = 0;
}

/* 35C2:000B: the page's first byte picks the decoder; the rest is its
 * stream. The VGA keeps the decoded bytes as they are. */
static void TestPage(uint8_t layoutType)
{
    uint8_t packedDisplayBuffer[0x10] = { 0x00 };
    uint8_t iconPageDestination[0x10];

    ResetObservations();
    packedDisplayBuffer[0x00] = layoutType;
    VideoAdapterType = VIDEO_ADAPTER_VGA;
    CopyDecodedIconPage(packedDisplayBuffer, iconPageDestination);

    assert(decoderSource == packedDisplayBuffer + 0x01);
    assert(decoderDestination == iconPageDestination);
    assert(linearDecodeCount == (layoutType == 0x01 ? 0x0001 : 0x0000));
    assert(columnDecodeCount == (layoutType == 0x01 ? 0x0000 : 0x0001));
}

int main(void)
{
    TestPage(0x01);
    TestPage(0x02);
    return 0;
}
