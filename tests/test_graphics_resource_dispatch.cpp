#include "REVENGE.h"

#include "revenge/original_data.h"
#include "revenge/video.h"

#include <assert.h>

static char resourceName[] = "TEST.CPS";
static uint8_t decodeBuffer[0x10];
static uint8_t displayBuffer[0x10];
static const char *capturedName;
static void *capturedCompressed;
static void *capturedDestination;

void LoadAndDecodePackedResource(
    const char *name,
    void *compressedBuffer,
    void *destination)
{
    capturedName = name;
    capturedCompressed = compressedBuffer;
    capturedDestination = destination;
}

static void ResetCapture(void)
{
    capturedName = NULL;
    capturedCompressed = NULL;
    capturedDestination = NULL;
}

int main(void)
{
    CachedFileDescriptors[0x03].fileName = resourceName;

    ResetCapture();
    VideoAdapterType = VIDEO_ADAPTER_VGA;
    LoadGraphicsResourceToBuffers(
        0x0003, decodeBuffer, displayBuffer);
    assert(capturedName == resourceName);
    assert(capturedCompressed == decodeBuffer);
    assert(capturedDestination == displayBuffer);

    return 0;
}
