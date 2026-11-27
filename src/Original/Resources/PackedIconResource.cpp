// 34B3: packed icon resource
// PackedIconResource.cpp
// Length-prefixed tactical graphics resource loader.

#include "Resources/Resources.h"

// 34B3:0008
void LoadPackedIconResource(
    const char *resourcePath,
    uint8_t *packedDisplayBuffer)
{
    uint16_t packedByteCount;
    int16_t fileHandle;

    fileHandle = OpenTrackedFile((char *) resourcePath, TRACKED_FILE_OPEN_READ);
    ReadTrackedFile(fileHandle, &packedByteCount, sizeof packedByteCount);
    ReadTrackedFile(fileHandle, packedDisplayBuffer, packedByteCount);
    CloseTrackedFile(fileHandle);
}
