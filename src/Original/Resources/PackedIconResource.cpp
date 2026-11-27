// 34B3: packed icon resource
// PackedIconResource.cpp
// Length-prefixed tactical graphics resource loader.

#include "REVENGE.h"

// 34B3:0008
void LoadPackedIconResource(
    const char *resourcePath,
    uint8_t *packedDisplayBuffer)
{
    uint16_t packedByteCount;
    int16_t fileHandle;

    fileHandle = OpenTrackedFile((char *) resourcePath, 0x0001);
    ReadTrackedFile(fileHandle, &packedByteCount, 0x00000002U);
    ReadTrackedFile(fileHandle, packedDisplayBuffer, packedByteCount);
    CloseTrackedFile(fileHandle);
}
