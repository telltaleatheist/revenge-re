#ifndef REVENGE_ORIGINAL_RESOURCE_DATA_H
#define REVENGE_ORIGINAL_RESOURCE_DATA_H

#include <stddef.h>

#include "revenge/types.h"

enum
{
    ORIGINAL_RESOURCE_DESCRIPTOR_COUNT = 0x5A,
    ORIGINAL_TRACKED_FILE_COUNT = 0x0A,
    ORIGINAL_TRACKED_FILE_UNUSED = 0x7FFF,
    ORIGINAL_TRACKED_FILE_CACHED = 0x8000,
    ORIGINAL_TRACKED_FILE_INDEX_MASK = 0x7FFF
};

/* The resource layer's state (DS:B632-B64F) and the cached-file
 * descriptors (DS:10B0, 12h each). The original's far pointers are host
 * pointers here; the places of the rest are given beside them. */
typedef struct ResourceFileRuntime
{
    const char *errorText; /* DS:B632 */
    uint16_t pendingDriveMask; /* DS:B636 */
    uint16_t availableDriveMask; /* DS:B638 */
    uint16_t trackedHandles[ORIGINAL_TRACKED_FILE_COUNT]; /* DS:B63A */
    uint16_t preloadFiles; /* DS:B64E */
} ResourceFileRuntime;

typedef struct CachedFileDescriptor
{
    char *fileName; /* +00h */
    uint32_t fileLength; /* +04h */
    uint8_t *cachedData; /* +08h */
    uint16_t resourceDriveMask; /* +0Ch */
    uint32_t currentPosition; /* +0Eh */
} CachedFileDescriptor;

extern ResourceFileRuntime ResourceFiles;
extern CachedFileDescriptor CachedFileDescriptors[ORIGINAL_RESOURCE_DESCRIPTOR_COUNT];
extern char ResourceFileNames[ORIGINAL_RESOURCE_DESCRIPTOR_COUNT][0x10];
extern const uint32_t ResourceFileLengths[ORIGINAL_RESOURCE_DESCRIPTOR_COUNT];

void OriginalData_InitializeResourceDescriptorPointers(void);

#endif
