/* REVENGE_2FEA_DISPLAY_BACKING.cpp -- packed display-backing builder. */

#include "REVENGE.h"

#include "revenge/original_data.h"
#include "revenge/video.h"

/* 2FEA:000A-01B1 */
byte *BuildPlanarDisplayBackingBuffer(
    word16 sourceTileX,
    word16 sourcePixelY,
    word16 widthTiles,
    word16 heightPixels)
{
    word16 height = heightPixels;
    word16 width = widthTiles;
    word16 uncompressedPayloadBytes = width * height << 0x02;
    byte *framebuffer;
    byte *source;
    byte *backingBuffer;
    word16 sourceRowSkip;

    framebuffer = PrimaryScreenBuffer;
    sourceTileX <<= 0x02;
    source = framebuffer + sourcePixelY * 0x00A0 + sourceTileX;

    /* The EGA's sparse encoding (2FEA:0024-002D and the block it jumps
	 * to) went with the EGA; the rebuild is VGA only. */
    backingBuffer = (byte *) Heap_Allocate((uint32) uncompressedPayloadBytes + 0x04, "2FEA:0038");
    if (backingBuffer == NULL)
        return NULL;

    backingBuffer[0x01] = (byte) height - 0x01;
    backingBuffer[0x02] = (byte) width;
    backingBuffer[0x03] = (byte) height - 0x01;
    width <<= 0x01;
    sourceRowSkip = 0x00A0 - (width << 0x01);
    EncodeDisplayBackingRows(
        source,
        backingBuffer + 0x04,
        width,
        (byte) height,
        (int16) sourceRowSkip);
    /* The CGA's second pass (2FEA:00B3-00F1) went with the CGA. */

    {
        uint32 backingBufferExtent = ((uint32) width * height) << 0x02;

        PlanarBackingBufferExtentLow = LOW_WORD(backingBufferExtent);
        PlanarBackingBufferExtentHigh = HIGH_WORD(backingBufferExtent);
    }
    return backingBuffer;
}
