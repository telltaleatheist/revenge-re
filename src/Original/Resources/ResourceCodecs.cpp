// 0800: resource codecs
// GraphicsRuntime.cpp
// Runtime, codec, tactical-map, and video procedures reconstructed from
// REVENGE.EXE segment 0800.

// Packed-resource codecs and DOS runtime edges from segment 0800.
#include "Data/ResourceData.h"
#include "Resources/Resources.h"
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
void DOS_TerminateProcess(uint16_t exitCode)
{
    /* 0800:0112 called InstallExceptionHandlers (0800:0172) first, which
	 * put back the divide, overflow, bounds and invalid-opcode vectors (00h,
	 * 04h, 05h, 06h) saved at start-up: Replaced, there is no vector table. */
    RuntimeTerminationHook();
    // ANNOTATION: DOS INT 21h/AH=4Ch process termination.
    SDLBackend_TerminateProcess((uint8_t) exitCode);
}

// 0800:0C34: copy a 32-bit byte count between normalized far buffers.
// Called from:
//      DecodeWestwoodPackedResource
void CopyUncompressedFarBlock(
    const void *source,
    void *destination,
    uint32_t byteCount)
{
    const uint8_t *sourceByte = (const uint8_t *) source;
    uint8_t *destinationByte = (uint8_t *) destination;

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
    const uint8_t *compressedBlock,
    uint8_t *destination)
{
    uint32_t remainingOutputBytes;
    uint16_t sourceOffset;

    memcpy(&remainingOutputBytes, compressedBlock + 0x02,
        sizeof(remainingOutputBytes));
    memcpy(&sourceOffset, compressedBlock + 0x06, sizeof(sourceOffset));
    compressedBlock += sourceOffset + 0x08;
    while (remainingOutputBytes != 0x00000000UL)
    {
        uint8_t command = *compressedBlock++;
        uint16_t runLength;

        if (command == 0x00)
        {
            runLength = ((uint16_t) compressedBlock[0x00] << 0x08) |
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
            runLength = (uint16_t) (uint8_t) -command;
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
    uint16_t OutputByteCount;
    uint16_t Reserved;
    const uint8_t *CompressedStream;
    uint8_t CodePhase;
} WestwoodLzwDecoderState;

static WestwoodLzwDecoderState WestwoodLzwDecoder;

uint16_t ReadWestwoodLzw12Code(
    uint8_t nibbleShift,
    uint16_t sourceOffset,
    uint16_t *nextSourceOffset);
uint16_t ReadWestwoodLzw14Code(
    uint16_t sourceOffset,
    uint16_t *nextSourceOffset);

uint32_t PrepareRLEOutputRun(
    uint32_t remainingOutputBytes,
    uint16_t runLength,
    uint16_t destinationOffset,
    uint16_t destinationSegment,
    uint16_t *normalizedOffsetOut,
    uint16_t *normalizedSegmentOut)
{
    uint32_t nextRemainingOutputBytes;

    nextRemainingOutputBytes = remainingOutputBytes - (uint32_t) runLength;
    *normalizedOffsetOut = destinationOffset & 0x000F;
    *normalizedSegmentOut = destinationSegment + (destinationOffset >> 0x04);
    return nextRemainingOutputBytes;
}

// 0800:0D1A: expand the original 12-bit Westwood dictionary stream.
void DecodeWestwoodLzw12(
    const uint8_t *compressed,
    uint8_t *destination,
    uint8_t *dictionary)
{
    uint16_t sourceOffset;
    uint16_t outputOffset;
    uint16_t nextCode;
    uint16_t previousCode;
    uint8_t previousFirst;

    memcpy(&WestwoodLzwDecoder.OutputByteCount,
        compressed + 0x02, sizeof(uint16_t));
    memcpy(&sourceOffset, compressed + 0x06, sizeof(sourceOffset));
    WestwoodLzwDecoder.CompressedStream = compressed;
    WestwoodLzwDecoder.CodePhase = 0x00;
    sourceOffset = sourceOffset + 0x08;
    outputOffset = 0x0000;
    nextCode = 0x0100;
    previousCode = ReadWestwoodLzw12Code(
        0x04, sourceOffset, &sourceOffset);
    previousFirst = (uint8_t) previousCode;
    destination[outputOffset++] = previousFirst;
    while (outputOffset < WestwoodLzwDecoder.OutputByteCount)
    {
        uint16_t code = ReadWestwoodLzw12Code(
            0x04, sourceOffset, &sourceOffset);
        uint16_t expandedCode = code;
        uint16_t sequenceStart = outputOffset;
        uint16_t left;
        uint16_t right;

        if (code >= nextCode)
        {
            destination[outputOffset++] = previousFirst;
            expandedCode = previousCode;
        }
        while (expandedCode >= 0x0100)
        {
            uint16_t dictionaryOffset = (expandedCode - 0x0100) << 0x02;

            destination[outputOffset++] = dictionary[dictionaryOffset + 0x02];
            memcpy(&expandedCode, dictionary + dictionaryOffset,
                sizeof(expandedCode));
        }
        previousFirst = (uint8_t) expandedCode;
        destination[outputOffset++] = previousFirst;
        left = sequenceStart;
        right = outputOffset - 0x01;
        while (left < right)
        {
            uint8_t swap = destination[left];

            destination[left++] = destination[right];
            destination[right--] = swap;
        }
        if (nextCode < 0x1000)
        {
            uint16_t dictionaryOffset = (nextCode - 0x0100) << 0x02;

            memcpy(dictionary + dictionaryOffset, &previousCode,
                sizeof(previousCode));
            dictionary[dictionaryOffset + 0x02] = previousFirst;
            ++nextCode;
        }
        previousCode = code;
    }
}

uint16_t ReadWestwoodLzw12Code(
    uint8_t nibbleShift,
    uint16_t sourceOffset,
    uint16_t *nextSourceOffset)
{
    const uint8_t *compressedData;
    uint16_t packedCode;

    compressedData = WestwoodLzwDecoder.CompressedStream + sourceOffset;
    ++WestwoodLzwDecoder.CodePhase;
    if ((WestwoodLzwDecoder.CodePhase & 0x01) != 0x00)
    {
        packedCode = ((uint16_t) compressedData[0x00] << 0x08) |
            compressedData[0x01];
        *nextSourceOffset = sourceOffset + 0x01;
        return packedCode >> nibbleShift;
    }

    *nextSourceOffset = sourceOffset + 0x02;
    return ((uint16_t) (compressedData[0x00] & 0x0F) << 0x08) |
        compressedData[0x01];
}

// 0800:0E16: expand the original 14-bit Westwood dictionary stream.
void DecodeWestwoodLzw14(
    const uint8_t *compressed,
    uint8_t *destination,
    uint8_t *dictionary)
{
    uint16_t sourceOffset;
    uint16_t outputOffset;
    uint16_t nextCode;
    uint16_t previousCode;
    uint8_t previousFirst;

    memcpy(&WestwoodLzwDecoder.OutputByteCount,
        compressed + 0x02, sizeof(uint16_t));
    memcpy(&sourceOffset, compressed + 0x06, sizeof(sourceOffset));
    WestwoodLzwDecoder.CompressedStream = compressed;
    WestwoodLzwDecoder.CodePhase = 0x00;
    sourceOffset = sourceOffset + 0x08;
    outputOffset = 0x0000;
    nextCode = 0x0100;
    previousCode = ReadWestwoodLzw14Code(sourceOffset, &sourceOffset);
    previousFirst = (uint8_t) previousCode;
    destination[outputOffset++] = previousFirst;
    while (outputOffset < WestwoodLzwDecoder.OutputByteCount)
    {
        uint16_t code = ReadWestwoodLzw14Code(
            sourceOffset, &sourceOffset);
        uint16_t expandedCode = code;
        uint16_t sequenceStart = outputOffset;
        uint16_t left;
        uint16_t right;

        if (code >= nextCode)
        {
            destination[outputOffset++] = previousFirst;
            expandedCode = previousCode;
        }
        while (expandedCode >= 0x0100)
        {
            uint16_t dictionaryIndex = expandedCode - 0x0100;

            destination[outputOffset++] = dictionary[0x7E00 + dictionaryIndex];
            memcpy(&expandedCode,
                dictionary + (dictionaryIndex << 0x01),
                sizeof(expandedCode));
        }
        previousFirst = (uint8_t) expandedCode;
        destination[outputOffset++] = previousFirst;
        left = sequenceStart;
        right = outputOffset - 0x01;
        while (left < right)
        {
            uint8_t swap = destination[left];

            destination[left++] = destination[right];
            destination[right--] = swap;
        }
        if (nextCode < 0x4000)
        {
            uint16_t dictionaryIndex = nextCode - 0x0100;

            memcpy(dictionary + (dictionaryIndex << 0x01),
                &previousCode, sizeof(previousCode));
            dictionary[0x7E00 + dictionaryIndex] = previousFirst;
            ++nextCode;
        }
        previousCode = code;
    }
}

uint16_t ReadWestwoodLzw14Code(
    uint16_t sourceOffset,
    uint16_t *nextSourceOffset)
{
    const uint8_t *compressedData;
    uint8_t phase;

    compressedData = WestwoodLzwDecoder.CompressedStream + sourceOffset;
    phase = (WestwoodLzwDecoder.CodePhase + 0x01) & 0x03;
    WestwoodLzwDecoder.CodePhase = phase;
    switch (phase)
    {
    case 0x01:
        *nextSourceOffset = sourceOffset + 0x01;
        return ((uint16_t) compressedData[0x00] << 0x06) |
            (compressedData[0x01] >> 0x02);
    case 0x02:
        *nextSourceOffset = sourceOffset + 0x02;
        return ((uint16_t) (compressedData[0x00] & 0x03) << 0x0C) |
            ((uint16_t) compressedData[0x01] << 0x04) |
            (compressedData[0x02] >> 0x04);
    case 0x03:
        *nextSourceOffset = sourceOffset + 0x02;
        return ((uint16_t) (compressedData[0x00] & 0x0F) << 0x0A) |
            ((uint16_t) compressedData[0x01] << 0x02) |
            (compressedData[0x02] >> 0x06);
    default:
        *nextSourceOffset = sourceOffset + 0x02;
        return ((uint16_t) (compressedData[0x00] & 0x3F) << 0x08) |
            compressedData[0x01];
    }
}
