// 0800, 21E7, 3390, 34B3: Resources declarations
#ifndef REVENGE_RESOURCES_RESOURCES_H
#define REVENGE_RESOURCES_RESOURCES_H

#include "Data/ResourceData.h"
#include "revenge/types.h"

void CopyUncompressedFarBlock(
    const void *source,
    void *destination,
    uint32_t byteCount);
uint16_t ReadWholeResourceFile(
    const char *resourceName,
    void *destination,
    uint16_t byteCount);
void ReadResourceFileSpan(
    const char *resourceName,
    int32_t fileOffset,
    void *destination,
    uint16_t byteCount);
void LoadAndDecodePackedResource(
    const char *resourceName,
    void *compressedBuffer,
    void *destination);
void DecodeWestwoodPackedResource(
    const void *compressedBlock,
    void *destination);
void RuntimeTerminationStub(void);
void DOS_TerminateProcess(uint16_t exitCode);

void AbortRuntimeWithMessage(const char *errorText);
void ShutdownAndExit(uint16_t exitCode);
int16_t OpenTrackedFile(char *path, uint16_t openMode);
void CloseTrackedFile(int16_t handleSlot);
uint32_t ReadTrackedFile(
    int16_t handleSlot,
    void *destination,
    uint32_t byteCount);
uint32_t WriteTrackedFile(
    int16_t handleSlot,
    const void *source,
    uint32_t byteCount);
void SeekTrackedFile(int16_t handleSlot, int32_t distance, uint16_t origin);
uint16_t CanOpenFileForRead(const char *path);
void DeleteFileIgnoringErrors(char *filePath);
uint16_t CanOpenFileForWrite(const char *path);
uint16_t GetResourceDriveMask(char *path);
void ReleaseCachedResources(void);
void InitializeResourceCache(void);
uint16_t IsResourceCached(char *path);
uint16_t FindResourceDescriptorIndex(char *path);
uint16_t ReadCachedResource(
    uint16_t resourceIndex,
    void *destination,
    uint32_t requestedBytes);
void AdjustActiveTextWindowMargin(uint16_t marginTiles);
void LoadPackedIconResource(
    const char *resourcePath,
    uint8_t *packedDisplayBuffer);
extern OriginalExitHandler RuntimeTerminationHook;

#endif
