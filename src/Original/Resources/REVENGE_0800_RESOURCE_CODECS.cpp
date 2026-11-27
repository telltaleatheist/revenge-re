// REVENGE_0800_GRAPHICS_RUNTIME.cpp
// Runtime, codec, tactical-map, and video procedures reconstructed from
// REVENGE.EXE segment 0800.

// Packed-resource codecs and DOS runtime edges from segment 0800.
#include "REVENGE.h"
#include "revenge/original_data.h"
#include "revenge/platform_backend.h"
#include "revenge/video.h"

#include <stdint.h>
#include <string.h>

enum
{
    EGA_GC_ENABLE_SET_RESET_REGISTER = 0x01,
    EGA_GC_MODE_REGISTER = 0x05,
    EGA_GC_BIT_MASK_REGISTER = 0x08,
    EGA_GC_WRITE_MODE_0 = 0x00,
    EGA_GC_WRITE_MODE_1 = 0x01,
    EGA_GC_WRITE_MODE_2 = 0x02,
    EGA_SEQ_MAP_MASK_REGISTER = 0x02,
    EGA_ALL_PLANES = 0x0F,
    EGA_ALL_WRITABLE_PIXEL_BITS = 0xFF
};

// 0800:019F: void RuntimeTerminationStub(void)
// The near hook DOS_TerminateProcess calls through DS:71A4 (with CS pushed,
// so it ends in retf): mov word ptr [0081h],0000h; retf. The image holds
// 019Fh there and no instruction writes it, nor names DS:0081, which no
// object of the rebuild's stands for: nothing is left to do.
void RuntimeTerminationStub(void)
{
}

/* DS:71A4, a near pointer: 0800:019F in the image, never written. */
OriginalExitHandler RuntimeTerminationHook = RuntimeTerminationStub;

// 0800:010D: void DOS_TerminateProcess(word16 exitCode)
// Called from:
//      RunExitHandlersAndTerminate
void DOS_TerminateProcess(word16 exitCode)
{
    /* 0800:0112 called InstallExceptionHandlers (0800:0172) first, which
	 * put back the divide, overflow, bounds and invalid-opcode vectors (00h,
	 * 04h, 05h, 06h) saved at start-up: Replaced, there is no vector table. */
    RuntimeTerminationHook();
    // ANNOTATION: DOS INT 21h/AH=4Ch process termination.
    SDLBackend_TerminateProcess((byte) exitCode);
}

// 0800:0C34: copy a 32-bit byte count between normalized far buffers.
// Called from:
//      DecodeWestwoodPackedResource
void CopyUncompressedFarBlock(
    const void *source,
    void *destination,
    uint32 byteCount)
{
    const byte *sourceByte = (const byte *) source;
    byte *destinationByte = (byte *) destination;

    /* 0800:0C34-0C86 uses the low word with REP MOVSW/MOVSB, then
	 * normalizes both far pointers before every high-word 10000h-byte block. */
    while (byteCount != 0x00000000UL)
    {
        *destinationByte = *sourceByte;
        ++sourceByte;
        ++destinationByte;
        --byteCount;
    }
}

// 0800:0C9A: decode signed literal/repeat runs until the header's 32-bit
// output count is exhausted.
void DecodeRLEBlock(
    const byte *compressedBlock,
    byte *destination)
{
    uint32 remainingOutputBytes;
    word16 sourceOffset;

    memcpy(&remainingOutputBytes, compressedBlock + 0x02,
        sizeof(remainingOutputBytes));
    memcpy(&sourceOffset, compressedBlock + 0x06, sizeof(sourceOffset));
    compressedBlock += sourceOffset + 0x08;
    while (remainingOutputBytes != 0x00000000UL)
    {
        byte command = *compressedBlock++;
        word16 runLength;

        if (command == 0x00)
        {
            runLength = ((word16) compressedBlock[0x00] << 0x08) |
                compressedBlock[0x01];
            compressedBlock += 0x02;
            memset(destination, *compressedBlock++, runLength);
        }
        else if (command <= 0x7F)
        {
            runLength = command;
            memcpy(destination, compressedBlock, runLength);
            compressedBlock += runLength;
        }
        else
        {
            runLength = (word16) (byte) -command;
            memset(destination, *compressedBlock++, runLength);
        }
        destination += runLength;
        remainingOutputBytes -= runLength;
    }
}


/* CS:0C93-0C99. The stream's segment (CS:0C97) is its pointer: the
 * caller gives a segment alone (21E7:00F4), so the stream starts at offset
 * 0 and a code's offset is counted from there, in 16 bits. */
typedef struct WestwoodLzwDecoderState
{
    word16 OutputByteCount;
    word16 Reserved;
    const byte *CompressedStream;
    byte CodePhase;
} WestwoodLzwDecoderState;

static WestwoodLzwDecoderState WestwoodLzwDecoder;

uint16 ReadWestwoodLzw12Code(
    byte nibbleShift,
    word16 sourceOffset,
    word16 *nextSourceOffset);
uint16 ReadWestwoodLzw14Code(
    word16 sourceOffset,
    word16 *nextSourceOffset);


uint32 PrepareRLEOutputRun(
    uint32 remainingOutputBytes,
    word16 runLength,
    word16 destinationOffset,
    word16 destinationSegment,
    word16 *normalizedOffsetOut,
    word16 *normalizedSegmentOut)
{
    uint32 nextRemainingOutputBytes;

    nextRemainingOutputBytes = remainingOutputBytes - (uint32) runLength;
    *normalizedOffsetOut = destinationOffset & 0x000F;
    *normalizedSegmentOut = destinationSegment + (destinationOffset >> 0x04);
    return nextRemainingOutputBytes;
}

// 0800:0D1A: expand the original 12-bit Westwood dictionary stream.
void DecodeWestwoodLzw12(
    const byte *compressed,
    byte *destination,
    byte *dictionary)
{
    word16 sourceOffset;
    word16 outputOffset;
    word16 nextCode;
    word16 previousCode;
    byte previousFirst;

    memcpy(&WestwoodLzwDecoder.OutputByteCount,
        compressed + 0x02, sizeof(word16));
    memcpy(&sourceOffset, compressed + 0x06, sizeof(sourceOffset));
    WestwoodLzwDecoder.CompressedStream = compressed;
    WestwoodLzwDecoder.CodePhase = 0x00;
    sourceOffset = sourceOffset + 0x08;
    outputOffset = 0x0000;
    nextCode = 0x0100;
    previousCode = ReadWestwoodLzw12Code(
        0x04, sourceOffset, &sourceOffset);
    previousFirst = (byte) previousCode;
    destination[outputOffset++] = previousFirst;
    while (outputOffset < WestwoodLzwDecoder.OutputByteCount)
    {
        word16 code = ReadWestwoodLzw12Code(
            0x04, sourceOffset, &sourceOffset);
        word16 expandedCode = code;
        word16 sequenceStart = outputOffset;
        word16 left;
        word16 right;

        if (code >= nextCode)
        {
            destination[outputOffset++] = previousFirst;
            expandedCode = previousCode;
        }
        while (expandedCode >= 0x0100)
        {
            word16 dictionaryOffset = (expandedCode - 0x0100) << 0x02;

            destination[outputOffset++] = dictionary[dictionaryOffset + 0x02];
            memcpy(&expandedCode, dictionary + dictionaryOffset,
                sizeof(expandedCode));
        }
        previousFirst = (byte) expandedCode;
        destination[outputOffset++] = previousFirst;
        left = sequenceStart;
        right = outputOffset - 0x01;
        while (left < right)
        {
            byte swap = destination[left];

            destination[left++] = destination[right];
            destination[right--] = swap;
        }
        if (nextCode < 0x1000)
        {
            word16 dictionaryOffset = (nextCode - 0x0100) << 0x02;

            memcpy(dictionary + dictionaryOffset, &previousCode,
                sizeof(previousCode));
            dictionary[dictionaryOffset + 0x02] = previousFirst;
            ++nextCode;
        }
        previousCode = code;
    }
}

uint16 ReadWestwoodLzw12Code(
    byte nibbleShift,
    word16 sourceOffset,
    word16 *nextSourceOffset)
{
    const byte *compressedData;
    word16 packedCode;

    compressedData = WestwoodLzwDecoder.CompressedStream + sourceOffset;
    ++WestwoodLzwDecoder.CodePhase;
    if ((WestwoodLzwDecoder.CodePhase & 0x01) != 0x00)
    {
        packedCode = ((word16) compressedData[0x00] << 0x08) |
            compressedData[0x01];
        *nextSourceOffset = sourceOffset + 0x01;
        return packedCode >> nibbleShift;
    }

    *nextSourceOffset = sourceOffset + 0x02;
    return ((word16) (compressedData[0x00] & 0x0F) << 0x08) |
        compressedData[0x01];
}

// 0800:0E16: expand the original 14-bit Westwood dictionary stream.
void DecodeWestwoodLzw14(
    const byte *compressed,
    byte *destination,
    byte *dictionary)
{
    word16 sourceOffset;
    word16 outputOffset;
    word16 nextCode;
    word16 previousCode;
    byte previousFirst;

    memcpy(&WestwoodLzwDecoder.OutputByteCount,
        compressed + 0x02, sizeof(word16));
    memcpy(&sourceOffset, compressed + 0x06, sizeof(sourceOffset));
    WestwoodLzwDecoder.CompressedStream = compressed;
    WestwoodLzwDecoder.CodePhase = 0x00;
    sourceOffset = sourceOffset + 0x08;
    outputOffset = 0x0000;
    nextCode = 0x0100;
    previousCode = ReadWestwoodLzw14Code(sourceOffset, &sourceOffset);
    previousFirst = (byte) previousCode;
    destination[outputOffset++] = previousFirst;
    while (outputOffset < WestwoodLzwDecoder.OutputByteCount)
    {
        word16 code = ReadWestwoodLzw14Code(
            sourceOffset, &sourceOffset);
        word16 expandedCode = code;
        word16 sequenceStart = outputOffset;
        word16 left;
        word16 right;

        if (code >= nextCode)
        {
            destination[outputOffset++] = previousFirst;
            expandedCode = previousCode;
        }
        while (expandedCode >= 0x0100)
        {
            word16 dictionaryIndex = expandedCode - 0x0100;

            destination[outputOffset++] = dictionary[0x7E00 + dictionaryIndex];
            memcpy(&expandedCode,
                dictionary + (dictionaryIndex << 0x01),
                sizeof(expandedCode));
        }
        previousFirst = (byte) expandedCode;
        destination[outputOffset++] = previousFirst;
        left = sequenceStart;
        right = outputOffset - 0x01;
        while (left < right)
        {
            byte swap = destination[left];

            destination[left++] = destination[right];
            destination[right--] = swap;
        }
        if (nextCode < 0x4000)
        {
            word16 dictionaryIndex = nextCode - 0x0100;

            memcpy(dictionary + (dictionaryIndex << 0x01),
                &previousCode, sizeof(previousCode));
            dictionary[0x7E00 + dictionaryIndex] = previousFirst;
            ++nextCode;
        }
        previousCode = code;
    }
}

uint16 ReadWestwoodLzw14Code(
    word16 sourceOffset,
    word16 *nextSourceOffset)
{
    const byte *compressedData;
    byte phase;

    compressedData = WestwoodLzwDecoder.CompressedStream + sourceOffset;
    phase = (WestwoodLzwDecoder.CodePhase + 0x01) & 0x03;
    WestwoodLzwDecoder.CodePhase = phase;
    switch (phase)
    {
    case 0x01:
        *nextSourceOffset = sourceOffset + 0x01;
        return ((word16) compressedData[0x00] << 0x06) |
            (compressedData[0x01] >> 0x02);
    case 0x02:
        *nextSourceOffset = sourceOffset + 0x02;
        return ((word16) (compressedData[0x00] & 0x03) << 0x0C) |
            ((word16) compressedData[0x01] << 0x04) |
            (compressedData[0x02] >> 0x04);
    case 0x03:
        *nextSourceOffset = sourceOffset + 0x02;
        return ((word16) (compressedData[0x00] & 0x0F) << 0x0A) |
            ((word16) compressedData[0x01] << 0x02) |
            (compressedData[0x02] >> 0x06);
    default:
        *nextSourceOffset = sourceOffset + 0x02;
        return ((word16) (compressedData[0x00] & 0x3F) << 0x08) |
            compressedData[0x01];
    }
}
