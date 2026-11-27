// 21E7: packed resources
// PackedResources.cpp
// Maintained annotation of the packed-resource loading leaf routines.

#include "REVENGE.h"
#include "revenge/original_data.h"

#pragma pack(push, 1)
typedef struct WestwoodCompressionHeader
{
    uint8_t compressionType;          // +00: 0 raw, 1 LZW-12, 2 LZW-14, 3 RLE
    uint8_t reserved_01;
    uint32_t uncompressedByteCount;  // +02
    uint16_t optionalHeaderByteCount;// +06; codec-specific header byte count
} WestwoodCompressionHeader;
#pragma pack(pop)

static_assert(sizeof(WestwoodCompressionHeader) == 0x08,
    "WestwoodCompressionHeader must retain its DOS layout");

void CopyUncompressedFarBlock(
    const void *source,
    void *destination,
    uint32_t byteCount);
void DecodeRLEBlock(
    const uint8_t *compressedBlock,
    uint8_t *destination);
void DecodeWestwoodLzw12(
    const uint8_t *compressedBlock,
    uint8_t *destination,
    uint8_t *dictionary);
void DecodeWestwoodLzw14(
    const uint8_t *compressedBlock,
    uint8_t *destination,
    uint8_t *dictionary);

// 21E7:000C: word16 ReadWholeResourceFile(
//     const char far *resourceName, void far *destination, word16 byteCount)
// Called from:
//      LoadScreenPalette
//      LoadMusicResource
uint16_t ReadWholeResourceFile(
    const char *resourceName,
    void *destination,
    uint16_t byteCount)
{
    int16_t fileHandle;
    uint16_t bytesRead;

    fileHandle = OpenTrackedFile((char *) resourceName, 0x01);
    bytesRead = (uint16_t) ReadTrackedFile(
        fileHandle, destination, (uint32_t) byteCount);
    CloseTrackedFile(fileHandle);
    return bytesRead;
}

// 21E7:0044: open a resource, seek to a 32-bit file offset, read a bounded
// span into a far destination, then close the file.
// Called from:
//      LoadScenarioDigitalSoundBank
void ReadResourceFileSpan(
    const char *resourceName,
    int32_t fileOffset,
    void *destination,
    uint16_t byteCount)
{
    int16_t fileHandle;

    fileHandle = OpenTrackedFile((char *) resourceName, 0x01);
    SeekTrackedFile(fileHandle, fileOffset, 0x00);
    ReadTrackedFile(fileHandle, destination, (uint32_t) byteCount);
    CloseTrackedFile(fileHandle);
}

// 21E7:008E: void LoadAndDecodePackedResource(
//     const char far *resourceName, void far *compressedBuffer,
//     void far *destination)
// Called from:
//      LoadScreenAssetToWorkBuffer
//      LoadGraphicsResourceToBuffers
void LoadAndDecodePackedResource(
    const char *resourceName,
    void *compressedBuffer,
    void *destination)
{
    uint16_t storedByteCount;
    int16_t fileHandle;

    fileHandle = OpenTrackedFile((char *) resourceName, 0x01);
    ReadTrackedFile(fileHandle, &storedByteCount, 0x02);
    ReadTrackedFile(
        fileHandle, compressedBuffer, (uint32_t) storedByteCount);
    CloseTrackedFile(fileHandle);

    DecodeWestwoodPackedResource(compressedBuffer, destination);
}

// 21E7:00F4: dispatch the common Westwood compression header.
// Both workspaces are paragraph aligned; the four original codecs consume
// segment values and begin at offset zero.
void DecodeWestwoodPackedResource(
    const void *compressedBlock,
    void *destination)
{
    const WestwoodCompressionHeader *header =
        (const WestwoodCompressionHeader *) compressedBlock;

    // Verified against 21E7:00F4-0201. The LZW cases first compare the
    // shared dictionary's size (DS:2F70) with what they need and use the
    // shared one (segment DS:2F6E) when it is big enough (21E7:0193, 01E8).
    // Both words are 0 in the image and nothing in the program writes
    // either, so the size is always 0 and each decode allocates its own
    // dictionary.
    switch (header->compressionType)
    {
    case 0x00:
        CopyUncompressedFarBlock(
            (const uint8_t *) compressedBlock + 0x08,
            destination,
            header->uncompressedByteCount);
        break;

    case 0x01:
    {
        uint8_t *dictionary = (uint8_t *) Heap_Allocate(0x3C00, "21E7:0163");
        DecodeWestwoodLzw12((const uint8_t *) compressedBlock, (uint8_t *) destination,
            dictionary);
        Heap_Free(dictionary);
        break;
    }

    case 0x02:
    {
        uint8_t *dictionary = (uint8_t *) Heap_Allocate(0xBD00, "21E7:01B8");
        DecodeWestwoodLzw14((const uint8_t *) compressedBlock, (uint8_t *) destination,
            dictionary);
        Heap_Free(dictionary);
        break;
    }

    case 0x03:
        DecodeRLEBlock((const uint8_t *) compressedBlock, (uint8_t *) destination);
        break;
    }
}
