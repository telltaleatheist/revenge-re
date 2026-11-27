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

/* WestwoodCompressionHeader.compressionType */
inline constexpr uint8_t WESTWOOD_COMPRESSION_RAW = 0x00;
inline constexpr uint8_t WESTWOOD_COMPRESSION_LZW12 = 0x01;
inline constexpr uint8_t WESTWOOD_COMPRESSION_LZW14 = 0x02;
inline constexpr uint8_t WESTWOOD_COMPRESSION_RLE = 0x03;
inline constexpr uint16_t WESTWOOD_HEADER_SIZE = (uint16_t) sizeof(WestwoodCompressionHeader);
/* The LZW dictionaries: codes from 100h up to 1000h (12-bit) or 4000h
 * (14-bit). A 12-bit entry is four bytes (the previous code's word, the
 * first byte, a spare); the 14-bit dictionary keeps the words, then the
 * bytes. */
inline constexpr uint16_t LZW_FIRST_DICTIONARY_CODE = 0x0100;
inline constexpr uint16_t LZW12_CODE_LIMIT = 0x1000;
inline constexpr uint16_t LZW14_CODE_LIMIT = 0x4000;
inline constexpr int LZW12_DICTIONARY_SIZE = (LZW12_CODE_LIMIT - LZW_FIRST_DICTIONARY_CODE) * 0x04;
inline constexpr int LZW14_DICTIONARY_BYTES = (LZW14_CODE_LIMIT - LZW_FIRST_DICTIONARY_CODE) * 0x02;
inline constexpr int LZW14_DICTIONARY_SIZE = (LZW14_CODE_LIMIT - LZW_FIRST_DICTIONARY_CODE) * 0x03;

/* OpenTrackedFile's modes: read, make it anew, and read and write (made when
 * missing). */
inline constexpr uint16_t TRACKED_FILE_OPEN_READ = 0x0001;
inline constexpr uint16_t TRACKED_FILE_CREATE = 0x0002;
inline constexpr uint16_t TRACKED_FILE_OPEN_READ_WRITE = 0x0003;
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
