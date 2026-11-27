// 21E7: packed resources
// PackedResources.cpp
// Maintained annotation of the packed-resource loading leaf routines.

#include "Resources/Resources.h"
#include "revenge/heap.h"

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

    fileHandle = OpenTrackedFile((char *) resourceName, TRACKED_FILE_OPEN_READ);
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

    fileHandle = OpenTrackedFile((char *) resourceName, TRACKED_FILE_OPEN_READ);
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

    fileHandle = OpenTrackedFile((char *) resourceName, TRACKED_FILE_OPEN_READ);
    ReadTrackedFile(fileHandle, &storedByteCount, sizeof storedByteCount);
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
    case WESTWOOD_COMPRESSION_RAW:
        CopyUncompressedFarBlock(
            (const uint8_t *) compressedBlock + WESTWOOD_HEADER_SIZE,
            destination,
            header->uncompressedByteCount);
        break;

    case WESTWOOD_COMPRESSION_LZW12:
    {
        uint8_t *dictionary = (uint8_t *) Heap_Allocate(LZW12_DICTIONARY_SIZE, "21E7:0163");
        DecodeWestwoodLzw12((const uint8_t *) compressedBlock, (uint8_t *) destination,
            dictionary);
        Heap_Free(dictionary);
        break;
    }

    case WESTWOOD_COMPRESSION_LZW14:
    {
        uint8_t *dictionary = (uint8_t *) Heap_Allocate(LZW14_DICTIONARY_SIZE, "21E7:01B8");
        DecodeWestwoodLzw14((const uint8_t *) compressedBlock, (uint8_t *) destination,
            dictionary);
        Heap_Free(dictionary);
        break;
    }

    case WESTWOOD_COMPRESSION_RLE:
        DecodeRLEBlock((const uint8_t *) compressedBlock, (uint8_t *) destination);
        break;
    }
}
