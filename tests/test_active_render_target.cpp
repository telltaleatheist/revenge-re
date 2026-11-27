#include "Data/RuntimeData.h"
#include "Data/GraphicsData.h"
#include "Graphics/Graphics.h"
#include "revenge/video.h"

#include <stdio.h>
#include <stdlib.h>

static int failures;

static void Check(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL: %s\n", message);
        ++failures;
    }
}

static uint8_t primaryBuffer[0x10];
static uint8_t ordinaryBuffer[0x10];

static void TestOrdinaryTarget(void)
{
    VideoAdapterType = VIDEO_ADAPTER_VGA;
    PrimaryScreenBuffer = primaryBuffer;
    SetActiveRenderTarget(ordinaryBuffer);

    Check(ActiveRenderTarget == ordinaryBuffer,
        "ordinary targets retain the selected buffer");
}

static void TestPrimaryTarget(void)
{
    VideoAdapterType = VIDEO_ADAPTER_VGA;
    PrimaryScreenBuffer = primaryBuffer;
    SetActiveRenderTarget(primaryBuffer);

    Check(ActiveRenderTarget == primaryBuffer,
        "the primary buffer is a target as it is");
}

int main(void)
{
    TestOrdinaryTarget();
    TestPrimaryTarget();
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
