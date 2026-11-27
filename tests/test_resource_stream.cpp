#include "REVENGE.h"
#include "revenge/game_file.h"
#include "revenge/original_data.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;
static uint16_t readCalls;
static uint16_t writeCalls;
static uint16_t lastReadCount;
static uint16_t lastWriteCount;
static const void *firstReadPointer;
static const void *secondReadPointer;
static const void *firstWritePointer;
static const void *secondWritePointer;
static int16_t nextOpenHandle = 0x0005;

static void Check(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL: %s\n", message);
        ++failures;
    }
}

static uint8_t errorCalls[0x04];
static uint16_t errorCallCount;
static uint16_t shutdownCalls;

static void RecordErrorCall(uint8_t callId)
{
    if (errorCallCount < sizeof(errorCalls))
        errorCalls[errorCallCount] = callId;
    ++errorCallCount;
}

/* 3390:071E and 3390:097B, here only the order they're called in. */
static void DisplayError(void) { RecordErrorCall(0x01); }
static void DismissError(void) { RecordErrorCall(0x02); }
ResourceErrorCallback ResourceErrorDisplayCallback = DisplayError;
ResourceErrorCallback ResourceErrorDismissCallback = DismissError;

void ShutdownGame(void) { ++shutdownCalls; }
void RunExitHandlersAndTerminate(uint16_t exitCode) { (void) exitCode; }
int16_t GameFile_Open(const char *name, GameFileAccess access)
{
    (void) name;
    (void) access;
    return nextOpenHandle;
}

int16_t GameFile_Close(int16_t fileHandle)
{
    return fileHandle;
}

int16_t GameFile_Read(int16_t fileHandle, void *buffer, uint16_t requestedBytes)
{
    (void) fileHandle;
    if (readCalls == 0x0000)
        firstReadPointer = buffer;
    else if (readCalls == 0x0001)
        secondReadPointer = buffer;
    ++readCalls;
    lastReadCount = requestedBytes;
    return (int16_t) requestedBytes;
}

int16_t GameFile_Write(int16_t fileHandle, const void *buffer, uint16_t requestedBytes)
{
    (void) fileHandle;
    if (writeCalls == 0x0000)
        firstWritePointer = buffer;
    else if (writeCalls == 0x0001)
        secondWritePointer = buffer;
    ++writeCalls;
    lastWriteCount = requestedBytes;
    return (int16_t) requestedBytes;
}

int16_t GameFile_Delete(const char *name)
{
    (void) name;
    return 0x0000;
}

int32_t GameFile_Seek(int16_t fileHandle, int32_t offset, GameFileOrigin origin)
{
    (void) fileHandle;
    (void) origin;
    return offset;
}

void *Heap_Allocate(uint32_t byteCount, const char *caller)
{
    (void) caller;
    return malloc(byteCount);
}

void Heap_Free(void *block)
{
    free(block);
}

static void ResetStreamState(void)
{
    uint16_t index;

    for (index = 0x0000; index < ORIGINAL_TRACKED_FILE_COUNT; ++index)
        ResourceFiles.trackedHandles[index] = ORIGINAL_TRACKED_FILE_UNUSED;
    readCalls = 0x0000;
    writeCalls = 0x0000;
    firstReadPointer = NULL;
    secondReadPointer = NULL;
    firstWritePointer = NULL;
    secondWritePointer = NULL;
}

static void TestDescriptorCatalogue(void)
{
    char path[] = "mechtype.dat";
    uint16_t index;

    OriginalData_InitializeResourceDescriptorPointers();
    index = FindResourceDescriptorIndex(path);
    Check(index == 0x0040, "MECHTYPE.DAT retains descriptor index 64");
    Check(strcmp(path, "MECHTYPE.DAT") == 0,
        "resource lookup uppercases its caller-owned path");
    Check(CachedFileDescriptors[index].fileLength == 0x000033CDUL,
        "MECHTYPE.DAT retains its executable length");
    Check(*CachedFileDescriptors[0x0059].fileName == '\0',
        "descriptor 89 is the empty sentinel");
}

static void TestCachedStream(void)
{
    uint8_t block[0x20];
    uint8_t *source = block;
    uint8_t destination[0x10];
    uint16_t index;

    for (index = 0x0000; index < sizeof(block); ++index)
        block[index] = (uint8_t) (index + 0x18);
    memset(destination, 0x00, sizeof(destination));
    CachedFileDescriptors[0x0000].cachedData = source;
    CachedFileDescriptors[0x0000].fileLength = 0x20;
    CachedFileDescriptors[0x0000].currentPosition = 0x00000004UL;

    Check(ReadCachedResource(0x0000, destination, 0x00000008UL) == 0x0008,
        "cached read reports copied byte count");
    Check(memcmp(destination, source + 0x04, 0x08) == 0,
        "cached read copies from current position");
    Check(CachedFileDescriptors[0x0000].currentPosition == 0x0000000CUL,
        "cached read advances descriptor position");

    ResourceFiles.trackedHandles[0x0000] = ORIGINAL_TRACKED_FILE_CACHED;
    SeekTrackedFile(0x0000, -0x02, 0x0001);
    Check(CachedFileDescriptors[0x0000].currentPosition == 0x0000000AUL,
        "cached relative seek uses current position");
    SeekTrackedFile(0x0000, -0x04, 0x0002);
    Check(CachedFileDescriptors[0x0000].currentPosition == 0x0000001CUL,
        "cached end-relative seek uses file length");
    SeekTrackedFile(0x0000, 0x40, 0x0000);
    Check(CachedFileDescriptors[0x0000].currentPosition == 0x20,
        "cached absolute seek clamps to file length");
    SeekTrackedFile(0x0000, -0x01, 0x0000);
    Check(CachedFileDescriptors[0x0000].currentPosition == 0xFFFFFFFFUL,
        "cached absolute seek preserves the original signed-high comparison");
}

static void TestChunkedStreams(void)
{
    uint8_t buffer[0x10];

    ResourceFiles.trackedHandles[0x0001] = 0x0005;
    Check(ReadTrackedFile(0x0001, buffer, 0x00010010UL) == 0x00010010UL,
        "tracked read accumulates a 32-bit result");
    Check(readCalls == 0x0002 && lastReadCount == 0x0610,
        "tracked read splits at FA00h");
    Check(firstReadPointer == secondReadPointer,
        "tracked read preserves the assembly's repeated far pointer");

    Check(WriteTrackedFile(0x0001, buffer, 0x00010010UL) == 0x00010010UL,
        "tracked write accumulates a 32-bit result");
    Check(writeCalls == 0x0002 && lastWriteCount == 0x0610,
        "tracked write splits at FA00h");
    Check(firstWritePointer == secondWritePointer,
        "tracked write preserves the assembly's repeated far pointer");
}

static void TestOpenCloseAndWindowMargin(void)
{
    char path[] = "unknown.dat";
    int16_t slot;

    nextOpenHandle = 0x0005;
    slot = OpenTrackedFile(path, 0x0001);
    Check(slot == 0x0000 && ResourceFiles.trackedHandles[slot] == 0x0005,
        "ordinary open occupies the first free tracked slot");
    CloseTrackedFile(slot);
    Check(ResourceFiles.trackedHandles[slot] == ORIGINAL_TRACKED_FILE_UNUSED,
        "close releases the tracked slot");

    ActiveTextWindowLeftTile = 0x0005;
    ActiveTextWindowTopTile = 0x0006;
    ActiveTextWindowWidthTiles = 0x000A;
    ActiveTextWindowHeightTiles = 0x000B;
    AdjustActiveTextWindowMargin(0x0002);
    Check(ActiveTextWindowLeftTile == 0x0003 &&
            ActiveTextWindowTopTile == 0x0004 &&
            ActiveTextWindowWidthTiles == 0x000E &&
            ActiveTextWindowHeightTiles == 0x000F,
        "text-window margin expands all four edges");
}

/* 3390:0002: closing a slot that isn't open is a fatal error. The error's
 * text goes to ResourceFiles.errorText, then the display callback shows it,
 * the dismiss callback (the fatal disk error) follows, and the game shuts
 * down: both are called, in that order, as 3390:0010-0014 calls them. */
static void TestAbortCallsBothMessages(void)
{
    errorCallCount = 0x0000;
    shutdownCalls = 0x0000;
    CloseTrackedFile(0x0000);
    Check(errorCallCount == 0x0002 && errorCalls[0x00] == 0x01 &&
            errorCalls[0x01] == 0x02,
        "abort shows the error, then the fatal error");
    Check(shutdownCalls == 0x0001, "abort shuts the game down");
    Check(ResourceFiles.errorText == ResourceCloseUnopenedErrorText,
        "abort's text is the close error");
}

int main(void)
{
    ResetStreamState();
    TestDescriptorCatalogue();
    TestCachedStream();
    ResetStreamState();
    TestChunkedStreams();
    ResetStreamState();
    TestOpenCloseAndWindowMargin();
    ResetStreamState();
    TestAbortCallsBothMessages();

    if (failures != 0)
        return EXIT_FAILURE;
    puts("resource-stream tests passed");
    return EXIT_SUCCESS;
}
