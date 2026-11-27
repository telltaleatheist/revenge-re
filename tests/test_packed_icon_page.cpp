#include "REVENGE.h"
#include "revenge/original_data.h"
#include "revenge/video.h"

#include <assert.h>

static uint16_t callOrder;
static uint16_t loadOrder;
static uint16_t copyOrder;
static const char *loadedPath;
static uint8_t *loadedPackedBuffer;
static uint8_t *copiedPackedBuffer;
static uint8_t *copiedDestination;

void LoadPackedIconResource(
    const char *resourcePath,
    uint8_t *packedDisplayBuffer)
{
    loadOrder = ++callOrder;
    loadedPath = resourcePath;
    loadedPackedBuffer = packedDisplayBuffer;
}

void CopyDecodedIconPage(
    const uint8_t *packedDisplayBuffer,
    uint8_t *iconPageDestination)
{
    copyOrder = ++callOrder;
    copiedPackedBuffer = (uint8_t *) packedDisplayBuffer;
    copiedDestination = iconPageDestination;
}

static void ResetObservations(void)
{
    callOrder = 0x0000;
    loadOrder = 0x0000;
    copyOrder = 0x0000;
    loadedPath = 0;
    loadedPackedBuffer = 0;
    copiedPackedBuffer = 0;
    copiedDestination = 0;
}

int main(void)
{
    static const char path[] = "ICONS.PAK";
    uint8_t packedBuffer[0x10];
    uint8_t pageBuffer[0x10];

    ResetObservations();
    VideoAdapterType = VIDEO_ADAPTER_VGA;
    LoadPackedIconPageDirect(path, packedBuffer, pageBuffer);
    assert(loadOrder == 0x0001);
    assert(copyOrder == 0x0002);
    assert(callOrder == 0x0002);
    assert(loadedPath == path);
    assert(loadedPackedBuffer == packedBuffer);
    assert(copiedPackedBuffer == packedBuffer);
    assert(copiedDestination == pageBuffer);
    assert(VideoAdapterType == VIDEO_ADAPTER_VGA);
    return 0;
}
