// REVENGE_3390_RESOURCE_STREAM.cpp
// Assembly-led transcription of the tracked-file and resident-resource layer.

#include "REVENGE.h"
#include "revenge/original_data.h"
#include "revenge/debug.h"
#include "revenge/game_file.h"

#include <string.h>

// 3390:0002
void AbortRuntimeWithMessage(const char *errorText)
{
    ResourceFiles.errorText = errorText;
    ResourceErrorDisplayCallback();
    ResourceErrorDismissCallback();
    ShutdownAndExit(0x0001);
}

// 3390:0024
void ShutdownAndExit(word16 exitCode)
{
    ShutdownGame();
    RunExitHandlersAndTerminate(exitCode);
}

// 3390:0037
int16 OpenTrackedFile(char *path, word16 openMode)
{
    GameFileAccess access;
    int16 trackedHandle;
    int16 handleSlot;

    if (openMode == 0x0001)
    {
        /* 3390:007C-0091 asked for the disk the file is on (SelectResourceDrive).
		 * Every file is in the install or the save folder here, so a file
		 * in neither is missing from the copy. */
        if (IsResourceCached(path) == FALSE && CanOpenFileForRead(path) == FALSE)
            RevengeFault("can't open %s: it is in neither the game's folder nor the save folder", path);
        access = GAME_FILE_READ; /* 8001h */
    }
    else if (openMode == 0x0002)
    {
        DeleteFileIgnoringErrors(path);
        access = GAME_FILE_WRITE; /* 8002h */
    }
    else if (openMode == 0x0003)
    {
        /* 3390:00BB-010A asked again, ResourceRetryCount times, when the file
		 * couldn't be made (a full or protected disk). The save folder is
		 * the only place it can go, so the first failure stops. */
        if (CanOpenFileForRead(path) == FALSE)
        {
            if (CanOpenFileForWrite(path) == FALSE || CanOpenFileForRead(path) == FALSE)
                RevengeFault("can't create %s in the save folder", path);
        }
        access = GAME_FILE_READ_WRITE; /* 8004h */
    }
    else
    {
        ResourceFiles.errorText = ResourceIllegalOpenModeErrorText;
        ResourceErrorDisplayCallback();
        ResourceErrorDismissCallback();
        ShutdownAndExit(0x0001);
        access = GAME_FILE_READ;
    }

    if (IsResourceCached(path) != FALSE)
    {
        word16 resourceIndex = FindResourceDescriptorIndex(path);

        CachedFileDescriptors[resourceIndex].currentPosition = 0x00000000UL;
        trackedHandle = (int16) (resourceIndex | ORIGINAL_TRACKED_FILE_CACHED);
    }
    else
    {
        /* 3390:0178-01BF asked for the file's disk and tried again,
		 * ResourceRetryCount times. A failure here is the copy's, and no
		 * disk can mend it. */
        trackedHandle = GameFile_Open(path, access);
        if (trackedHandle == -0x01)
            RevengeFault("can't open %s (open mode %u)", path, (unsigned) openMode);
    }

    handleSlot = ORIGINAL_TRACKED_FILE_COUNT;
    for (int16 slot = 0x0000; slot < ORIGINAL_TRACKED_FILE_COUNT; ++slot)
    {
        if (ResourceFiles.trackedHandles[slot] == ORIGINAL_TRACKED_FILE_UNUSED)
        {
            handleSlot = slot;
            break;
        }
    }
    if (handleSlot == ORIGINAL_TRACKED_FILE_COUNT)
    {
        ResourceFiles.errorText = ResourceTooManyOpenFilesErrorText;
        ResourceErrorDisplayCallback();
        ResourceErrorDismissCallback();
        /* DS:B64E immediately follows the ten-entry table in the executable. */
        ResourceFiles.preloadFiles = (word16) trackedHandle;
    }
    else
    {
        ResourceFiles.trackedHandles[handleSlot] = (word16) trackedHandle;
    }
    return handleSlot;
}

// 3390:0244
void CloseTrackedFile(int16 handleSlot)
{
    word16 trackedHandle = ResourceFiles.trackedHandles[handleSlot];

    if (trackedHandle == ORIGINAL_TRACKED_FILE_UNUSED)
        AbortRuntimeWithMessage(ResourceCloseUnopenedErrorText);
    if ((trackedHandle & ORIGINAL_TRACKED_FILE_CACHED) == 0x0000)
        GameFile_Close((int16) trackedHandle);
    ResourceFiles.trackedHandles[handleSlot] = ORIGINAL_TRACKED_FILE_UNUSED;
}

// 3390:028C
uint32 ReadTrackedFile(int16 handleSlot, void *destination, uint32 byteCount)
{
    word16 trackedHandle = ResourceFiles.trackedHandles[handleSlot];
    uint32 totalBytesRead = 0x00000000UL;

    if (trackedHandle == ORIGINAL_TRACKED_FILE_UNUSED)
        AbortRuntimeWithMessage(ResourceReadUnopenedErrorText);
    if ((trackedHandle & ORIGINAL_TRACKED_FILE_CACHED) != 0x0000)
        return ReadCachedResource(trackedHandle & 0x7FFF, destination, byteCount);

    while ((int16) HIGH_WORD(byteCount) > 0x0000 ||
        (HIGH_WORD(byteCount) == 0x0000 && LOW_WORD(byteCount) > 0xFA00))
    {
        int16 chunkBytesRead = GameFile_Read(
            (int16) trackedHandle, destination, 0xFA00);

        byteCount -= 0x0000FA00UL;
        if (chunkBytesRead == -0x01)
            return 0x00000000UL;
        totalBytesRead += (word16) chunkBytesRead;
    }

    {
        int16 finalBytesRead = GameFile_Read(
            (int16) trackedHandle, destination, LOW_WORD(byteCount));

        if (finalBytesRead == -0x01)
            return 0x00000000UL;
        return totalBytesRead + (word16) finalBytesRead;
    }
}

// 3390:0372
uint32 WriteTrackedFile(int16 handleSlot, const void *source, uint32 byteCount)
{
    word16 trackedHandle = ResourceFiles.trackedHandles[handleSlot];
    uint32 totalBytesWritten = 0x00000000UL;

    if (trackedHandle == ORIGINAL_TRACKED_FILE_UNUSED)
        AbortRuntimeWithMessage(ResourceWriteUnopenedErrorText);
    if ((trackedHandle & ORIGINAL_TRACKED_FILE_CACHED) != 0x0000)
        AbortRuntimeWithMessage(ResourceWriteCachedErrorText);

    while ((int16) HIGH_WORD(byteCount) > 0x0000 ||
        (HIGH_WORD(byteCount) == 0x0000 && LOW_WORD(byteCount) > 0xFA00))
    {
        int16 chunkBytesWritten = GameFile_Write(
            (int16) trackedHandle, source, 0xFA00);

        if (chunkBytesWritten == -0x01)
            return 0x00000000UL;
        byteCount -= 0x0000FA00UL;
        totalBytesWritten += (word16) chunkBytesWritten;
    }

    {
        int16 finalBytesWritten = GameFile_Write(
            (int16) trackedHandle, source, LOW_WORD(byteCount));

        if (finalBytesWritten == -0x01)
            return 0x00000000UL;
        return totalBytesWritten + (word16) finalBytesWritten;
    }
}

// 3390:0441
void SeekTrackedFile(int16 handleSlot, int32 distance, word16 origin)
{
    word16 trackedHandle = ResourceFiles.trackedHandles[handleSlot];

    if (trackedHandle == ORIGINAL_TRACKED_FILE_UNUSED)
        AbortRuntimeWithMessage(ResourceSeekUnopenedErrorText);
    if (origin > 0x0002)
        AbortRuntimeWithMessage(ResourceIllegalSeekOriginErrorText);

    if ((trackedHandle & ORIGINAL_TRACKED_FILE_CACHED) == 0x0000)
    {
        (void) GameFile_Seek((int16) trackedHandle, distance, (GameFileOrigin) origin);
    }
    else
    {
        CachedFileDescriptor *resource =
            &CachedFileDescriptors[trackedHandle & 0x7FFF];
        int32 basePosition;
        int32 newPosition;

        if (origin == 0x0000)
        {
            int16 lengthHigh = (int16) HIGH_WORD(resource->fileLength);
            int16 distanceHigh = (int16) HIGH_WORD((uint32) distance);

            if (lengthHigh > distanceHigh ||
                (lengthHigh == distanceHigh &&
                    LOW_WORD(resource->fileLength) >= LOW_WORD((uint32) distance)))
            {
                resource->currentPosition = (uint32) distance;
            }
            else
            {
                resource->currentPosition = resource->fileLength;
            }
            return;
        }
        if (origin == 0x0001)
            basePosition = (int32) resource->currentPosition;
        else
            basePosition = (int32) resource->fileLength;

        newPosition = basePosition + distance;
        if (newPosition < 0x00000000L)
            newPosition = 0x00000000L;
        if ((uint32) newPosition > resource->fileLength)
            newPosition = (int32) resource->fileLength;
        resource->currentPosition = (uint32) newPosition;
    }
}

// 3390:061F
word16 CanOpenFileForRead(const char *path)
{
    int16 fileHandle = GameFile_Open(path, GAME_FILE_READ); /* 8001h */

    if (fileHandle == -0x01)
        return FALSE;
    GameFile_Close(fileHandle);
    return TRUE;
}

// 3390:064F
void DeleteFileIgnoringErrors(char *filePath)
{
    (void) GameFile_Delete(filePath);
}

// 3390:0668
word16 CanOpenFileForWrite(const char *path)
{
    int16 fileHandle = GameFile_Open(path, GAME_FILE_CREATE); /* 8102h */

    if (fileHandle == -0x01)
        return FALSE;
    GameFile_Close(fileHandle);
    return TRUE;
}

// 3390:069C
word16 GetResourceDriveMask(char *path)
{
    word16 resourceIndex = 0x0000;
    char *normalizedPath = UppercaseStringInPlace(path);

    while (*CachedFileDescriptors[resourceIndex].fileName != '\0')
    {
        CachedFileDescriptor *resource = &CachedFileDescriptors[resourceIndex];

        if (strcmp(
                resource->fileName,
                normalizedPath) == 0x0000)
            return resource->resourceDriveMask;
        ++resourceIndex;
    }
    AbortRuntimeWithMessage(ResourceUnknownDriveErrorText);
    return 0x0000;
}

// 3390:098C
void ReleaseCachedResources(void)
{
    word16 resourceIndex = 0x0000;
    int16 handleSlot;

    while (*CachedFileDescriptors[resourceIndex].fileName != '\0')
    {
        CachedFileDescriptor *resource = &CachedFileDescriptors[resourceIndex];

        if (resource->cachedData != NULL)
        {
            Heap_Free(resource->cachedData);
            resource->cachedData = NULL;
        }
        ++resourceIndex;
    }

    for (handleSlot = 0x0000; handleSlot < ORIGINAL_TRACKED_FILE_COUNT; ++handleSlot)
    {
        if (ResourceFiles.trackedHandles[handleSlot] != ORIGINAL_TRACKED_FILE_UNUSED)
            AbortRuntimeWithMessage(ResourceOpenFilesAtExitErrorText);
    }
}

/* What 0800:33A8 answered when DOS had its A000h paragraphs free: 280h KiB,
 * the most it reports. The host has no conventional-memory arena. */
enum
{
    HostConventionalMemoryKB = 0x0280
};

// 3390:0A22
void InitializeResourceCache(void)
{
    int16 remainingConventionalMemoryKB;
    word16 resourceIndex;
    int16 handleSlot;

    /* The names and lengths the image holds at DS:10B0. */
    OriginalData_InitializeResourceDescriptorPointers();
    for (handleSlot = 0x0000; handleSlot < ORIGINAL_TRACKED_FILE_COUNT; ++handleSlot)
        ResourceFiles.trackedHandles[handleSlot] = ORIGINAL_TRACKED_FILE_UNUSED;

    remainingConventionalMemoryKB = (int16) (HostConventionalMemoryKB - ReservedConventionalMemoryKB);
    ResourceFiles.availableDriveMask = 0x0000;
    resourceIndex = 0x0000;
    while (*CachedFileDescriptors[resourceIndex].fileName != '\0')
    {
        CachedFileDescriptor *resource = &CachedFileDescriptors[resourceIndex];
        word16 allocationKB;
        byte *cachedData;
        int16 fileHandle;
        int32 bytesRead;

        ResourceFiles.availableDriveMask |= resource->resourceDriveMask;
        if (ResourceFiles.preloadFiles == FALSE)
        {
            ++resourceIndex;
            continue;
        }

        resource->fileName = UppercaseStringInPlace(resource->fileName);
        allocationKB = (word16) (resource->fileLength / 0x00000400UL) + 0x0001;
        cachedData = NULL;
        if (remainingConventionalMemoryKB >= (int16) allocationKB)
            cachedData = (byte *) Heap_Allocate(resource->fileLength, "3390:0AF1");
        if (cachedData == NULL)
        {
            ++resourceIndex;
            continue;
        }

        fileHandle = GameFile_Open(resource->fileName, GAME_FILE_READ); /* 8001h */
        if (fileHandle == -0x01)
        {
            Heap_Free(cachedData);
            ++resourceIndex;
            continue;
        }

        bytesRead = GameFile_Read(
            fileHandle, cachedData, LOW_WORD(resource->fileLength));
        if ((uint32) bytesRead == resource->fileLength)
        {
            resource->cachedData = cachedData;
            GameFile_Close(fileHandle);
            remainingConventionalMemoryKB -= (int16) allocationKB;
        }
        else
        {
            Heap_Free(cachedData);
            GameFile_Close(fileHandle);
        }
        ++resourceIndex;
    }
}

// 3390:0BFF
word16 IsResourceCached(char *path)
{
    word16 resourceIndex = 0x0000;
    char *normalizedPath = UppercaseStringInPlace(path);

    while (*CachedFileDescriptors[resourceIndex].fileName != '\0')
    {
        CachedFileDescriptor *resource = &CachedFileDescriptors[resourceIndex];

        if (strcmp(
                resource->fileName,
                normalizedPath) == 0x0000)
            return resource->cachedData != NULL ? TRUE : FALSE;
        ++resourceIndex;
    }
    return FALSE;
}

// 3390:0C86
word16 FindResourceDescriptorIndex(char *path)
{
    word16 resourceIndex = 0x0000;
    char *normalizedPath = UppercaseStringInPlace(path);

    while (*CachedFileDescriptors[resourceIndex].fileName != '\0')
    {
        if (strcmp(
                CachedFileDescriptors[resourceIndex].fileName,
                normalizedPath) == 0x0000)
            return resourceIndex;
        ++resourceIndex;
    }
    AbortRuntimeWithMessage(ResourceUnknownCacheIndexErrorText);
    return resourceIndex;
}

// 3390:0CF7
word16 ReadCachedResource(
    word16 resourceIndex,
    void *destination,
    uint32 requestedBytes)
{
    CachedFileDescriptor *resource = &CachedFileDescriptors[resourceIndex];
    uint32 bytesToRead = requestedBytes;

    if (resource->currentPosition + bytesToRead > resource->fileLength)
        bytesToRead = resource->fileLength - resource->currentPosition;
    /* 3390:0D86-0DA1 add the position's low word to the heap block's
	 * offset (8) in 16 bits, which would wrap from position FFF8h on. Play
	 * never reads here: the game's main leaves files uncached (preloadFiles,
	 * 2C7F:000F), so the cache holds none. */
    memcpy(destination, resource->cachedData + resource->currentPosition,
        bytesToRead);
    resource->currentPosition += bytesToRead;
    return LOW_WORD(bytesToRead);
}

// 3390:0DFC: expand the active text window equally on all four sides.
// Verified against 3390:0DFC-0E1C.
void AdjustActiveTextWindowMargin(word16 marginTiles)
{
    ActiveTextWindowLeftTile -= marginTiles;
    ActiveTextWindowTopTile -= marginTiles;
    ActiveTextWindowWidthTiles += marginTiles << 0x01;
    ActiveTextWindowHeightTiles += marginTiles << 0x01;
}
