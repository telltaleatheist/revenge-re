// 0D73: display backing buffers
/* Display-backing procedures from original segment 0D73. */

#include "REVENGE.h"

#include "revenge/original_data.h"
#include "revenge/video.h"

#include <string.h>

/* 0D73:54B9-5507 */
uint8_t *BuildDisplayBackingBuffer(
    uint16_t sourceTileX,
    uint16_t sourcePixelY,
    uint16_t widthTiles,
    uint16_t heightPixels)
{
    if (VideoAdapterType == VIDEO_ADAPTER_VGA)
    {
        return BuildVgaDisplayBackingBuffer(
            sourceTileX, sourcePixelY, widthTiles, heightPixels);
    }
    return BuildPlanarDisplayBackingBuffer(
        sourceTileX, sourcePixelY, widthTiles, heightPixels);
}

/* 0D73:5508-566D */
uint8_t *BuildVgaDisplayBackingBuffer(
    uint16_t sourceTileX,
    uint16_t sourcePixelY,
    uint16_t widthTiles,
    uint16_t heightPixels)
{
    uint8_t *encodedBuffer;
    uint8_t *output;
    uint8_t *framebuffer;
    uint8_t *sourceRow;
    uint32_t encodedByteCount;
    uint16_t rowsRemaining;
    uint8_t *allocation;

    encodedBuffer = PlanarScreenBuffer;
    output = encodedBuffer;
    encodedByteCount = 0x00000004UL;
    *output++ = 0x00;
    *output++ = (uint8_t) heightPixels - 0x01;
    *output++ = (uint8_t) widthTiles;
    *output++ = (uint8_t) heightPixels - 0x01;

    framebuffer = PrimaryScreenBuffer;
    sourceRow = framebuffer + sourcePixelY * 0x0140 +
        (sourceTileX << 0x03);
    rowsRemaining = heightPixels;
    while (rowsRemaining-- != 0x0000)
    {
        uint8_t *source = sourceRow;
        uint16_t sourceBytesRemaining = widthTiles << 0x03;

        while (sourceBytesRemaining != 0x0000)
        {
            if (*source != 0x00)
            {
                *output++ = *source++;
                --sourceBytesRemaining;
                ++encodedByteCount;
            }
            else
            {
                uint16_t zeroRunLength = 0x0001;

                *output++ = 0x00;
                ++encodedByteCount;
                while (zeroRunLength < 0x00FF &&
                    sourceBytesRemaining - zeroRunLength > 0x0000 &&
                    source[zeroRunLength] == 0x00)
                {
                    ++zeroRunLength;
                }
                source += zeroRunLength;
                *output++ = (uint8_t) zeroRunLength;
                ++encodedByteCount;
                sourceBytesRemaining -= zeroRunLength;
            }
        }
        sourceRow += 0x0140;
    }

    allocation = (uint8_t *) Heap_Allocate(encodedByteCount, "0D73:562F");
    if (allocation != NULL)
    {
        memcpy(allocation, encodedBuffer, encodedByteCount);
    }
    return allocation;
}
