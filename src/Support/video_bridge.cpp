#include "revenge/platform_backend.h"
#include "revenge/video.h"
#include "Data/RuntimeData.h"
#include "Data/StartupData.h"
#include "Data/TacticalData.h"

#include <stdint.h>

uint8_t *Video_GetIndexedFramebuffer(void)
{
    return SDLBackend_GetIndexedFramebuffer();
}

void Video_ClearIndexedFramebuffer(uint8_t color)
{
    SDLBackend_ClearIndexedFramebuffer(color);
}

uint8_t *Video_ResolveIndexedRenderTarget(const uint8_t *renderTarget)
{
    uintptr_t encodedTarget = (uintptr_t) renderTarget;

    if (renderTarget == VideoFramebuffer || encodedTarget == 0xA0000000UL)
        return SDLBackend_GetIndexedFramebuffer();
    if (renderTarget == PrimaryScreenBuffer)
        return PrimaryScreenBuffer;
    return (uint8_t *) renderTarget;
}

void Video_DrawIndexedPixelToActiveTarget(uint16_t x, uint16_t y, uint8_t color)
{
    uint8_t *target;

    if (x >= VIDEO_SCREEN_WIDTH || y >= VIDEO_SCREEN_HEIGHT)
        return;
    target = Video_ResolveIndexedRenderTarget(ActiveRenderTarget);
    if (target == NULL)
        target = SDLBackend_GetIndexedFramebuffer();
    target[(uint32_t) y * VIDEO_SCREEN_WIDTH + x] = color;
}
