// 2FEA: planar display backing buffer
/* PlanarDisplayBacking.cpp -- packed display-backing builder. */

#include "Data/RuntimeData.h"
#include "Data/TacticalData.h"
#include "Graphics/Graphics.h"
#include "Utility/Utility.h"
#include "revenge/heap.h"

#include "revenge/video.h"

/* 2FEA:000A-01B1 */
uint8_t *BuildPlanarDisplayBackingBuffer(
    uint16_t sourceTileX,
    uint16_t sourcePixelY,
    uint16_t widthTiles,
    uint16_t heightPixels)
{
    uint16_t height = heightPixels;
    uint16_t width = widthTiles;
    uint16_t uncompressedPayloadBytes = width * height << 0x02;
    uint8_t *framebuffer;
    uint8_t *source;
    uint8_t *backingBuffer;
    uint16_t sourceRowSkip;

    framebuffer = PrimaryScreenBuffer;
    sourceTileX <<= 0x02;
    source = framebuffer + sourcePixelY * 0x00A0 + sourceTileX;

    /* The EGA's sparse encoding (2FEA:0024-002D and the block it jumps
	 * to) went with the EGA; the rebuild is VGA only. */
    backingBuffer = (uint8_t *) Heap_Allocate((uint32_t) uncompressedPayloadBytes + 0x04, "2FEA:0038");
    if (backingBuffer == NULL)
        return NULL;

    backingBuffer[0x01] = (uint8_t) height - 0x01;
    backingBuffer[0x02] = (uint8_t) width;
    backingBuffer[0x03] = (uint8_t) height - 0x01;
    width <<= 0x01;
    sourceRowSkip = 0x00A0 - (width << 0x01);
    EncodeDisplayBackingRows(
        source,
        backingBuffer + 0x04,
        width,
        (uint8_t) height,
        (int16_t) sourceRowSkip);
    /* The CGA's second pass (2FEA:00B3-00F1) went with the CGA. */

    {
        uint32_t backingBufferExtent = ((uint32_t) width * height) << 0x02;

        PlanarBackingBufferExtentLow = LOW_WORD(backingBufferExtent);
        PlanarBackingBufferExtentHigh = HIGH_WORD(backingBufferExtent);
    }
    return backingBuffer;
}
