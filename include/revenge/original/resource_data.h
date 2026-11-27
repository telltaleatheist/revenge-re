#ifndef REVENGE_ORIGINAL_RESOURCE_DATA_H
#define REVENGE_ORIGINAL_RESOURCE_DATA_H

#include <stddef.h>

#include "revenge/types.h"

enum
{
    ORIGINAL_RESOURCE_DESCRIPTOR_COUNT = 0x5A,
    ORIGINAL_TRACKED_FILE_COUNT = 0x0A,
    ORIGINAL_TRACKED_FILE_UNUSED = 0x7FFF,
    ORIGINAL_TRACKED_FILE_CACHED = 0x8000
};

/* The resource layer's state (DS:B632-B64F) and the cached-file
 * descriptors (DS:10B0, 12h each). The original's far pointers are host
 * pointers here; the places of the rest are given beside them. */
typedef struct ResourceFileRuntime
{
    const char *errorText; /* DS:B632 */
    word16 pendingDriveMask; /* DS:B636 */
    word16 availableDriveMask; /* DS:B638 */
    word16 trackedHandles[ORIGINAL_TRACKED_FILE_COUNT]; /* DS:B63A */
    word16 preloadFiles; /* DS:B64E */
} ResourceFileRuntime;

typedef struct CachedFileDescriptor
{
    char *fileName; /* +00h */
    uint32 fileLength; /* +04h */
    byte *cachedData; /* +08h */
    word16 resourceDriveMask; /* +0Ch */
    uint32 currentPosition; /* +0Eh */
} CachedFileDescriptor;

extern ResourceFileRuntime ResourceFiles;
extern CachedFileDescriptor CachedFileDescriptors[ORIGINAL_RESOURCE_DESCRIPTOR_COUNT];
extern char ResourceFileNames[ORIGINAL_RESOURCE_DESCRIPTOR_COUNT][0x10];
extern const uint32 ResourceFileLengths[ORIGINAL_RESOURCE_DESCRIPTOR_COUNT];

void OriginalData_InitializeResourceDescriptorPointers(void);

#endif
