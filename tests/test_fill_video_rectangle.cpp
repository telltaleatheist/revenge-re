#include "Data/RuntimeData.h"
#include "Graphics/Graphics.h"
#include "revenge/video.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct SpanCall
{
    int16_t y;
    int16_t leftX;
    int16_t rightX;
    uint16_t color;
} SpanCall;

static int failures;
static SpanCall spanCalls[0x08];
static uint16_t spanCallCount;

static void Check(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL: %s\n", message);
        ++failures;
    }
}

void DrawHorizontalSpan(
    int16_t y, int16_t leftX, int16_t rightX, uint16_t color)
{
    SpanCall *call = &spanCalls[spanCallCount++];
    call->y = y;
    call->leftX = leftX;
    call->rightX = rightX;
    call->color = color;
}

static void ResetCalls(void)
{
    spanCallCount = 0x00;
}

static void TestInclusiveSpanFallback(void)
{
    ResetCalls();
    VideoAdapterType = VIDEO_ADAPTER_VGA;
    FillVideoRectangle(0x02, 0x03, 0x07, 0x05, 0x0123);

    Check(spanCallCount == 0x03,
        "the VGA draws every inclusive scanline");
    Check(spanCalls[0x00].y == 0x03 &&
            spanCalls[0x02].y == 0x05,
        "fallback scanline loop includes both vertical endpoints");
    Check(spanCalls[0x01].leftX == 0x02 &&
            spanCalls[0x01].rightX == 0x07 &&
            spanCalls[0x01].color == 0x0123,
        "fallback forwards horizontal bounds and the full color word");
}

int main(void)
{
    TestInclusiveSpanFallback();
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
