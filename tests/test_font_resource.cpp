#include "Data/RuntimeData.h"
#include "Text/Text.h"
#include "revenge/heap.h"
#include "revenge/text.h"
#include "revenge/game_file.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum
{
    TEST_FONT_SIZE = 0x0404
};

static int failures;
static uint8_t fontFile[TEST_FONT_SIZE];
static int32_t filePosition;
static int32_t reportedFileLength;
static uint16_t closeCount;
static uint16_t allocationCount;
static uint16_t freeCount;

static void Check(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL: %s\n", message);
        ++failures;
    }
}

int16_t GameFile_Open(const char *name, GameFileAccess access)
{
    (void) name;
    (void) access;
    filePosition = 0x00;
    return 0x05;
}

int16_t GameFile_Close(int16_t fileHandle)
{
    (void) fileHandle;
    ++closeCount;
    return 0x00;
}

int16_t GameFile_Read(int16_t fileHandle, void *buffer, uint16_t requestedBytes)
{
    uint16_t available;
    uint16_t byteCount;

    (void) fileHandle;
    if (filePosition < 0x00 || filePosition >= TEST_FONT_SIZE)
        return 0x00;
    available = (uint16_t) (TEST_FONT_SIZE - filePosition);
    byteCount = requestedBytes < available ? requestedBytes : available;
    memcpy(buffer, &fontFile[filePosition], byteCount);
    filePosition += byteCount;
    return (int16_t) byteCount;
}

int32_t GameFile_Length(int16_t fileHandle)
{
    (void) fileHandle;
    return reportedFileLength;
}

int32_t GameFile_Seek(int16_t fileHandle, int32_t offset, GameFileOrigin origin)
{
    (void) fileHandle;
    if (origin == GAME_FILE_FROM_START)
        filePosition = offset;
    else if (origin == GAME_FILE_FROM_CURRENT)
        filePosition += offset;
    else
        filePosition = TEST_FONT_SIZE + offset;
    return filePosition;
}

void *Heap_Allocate(uint32_t byteCount, const char *caller)
{
    uint8_t *allocation = (uint8_t *) malloc(byteCount);

    (void) caller;
    if (allocation != NULL)
        ++allocationCount;
    return allocation;
}

void Heap_Free(void *block)
{
    ++freeCount;
    free(block);
}

static void PrepareValidFont(void)
{
    memset(fontFile, 0x5A, sizeof(fontFile));
    fontFile[0x00] = 0x02;
    fontFile[0x01] = 0x04;
    fontFile[0x0102] = 0x06;
    fontFile[0x0103] = 0x06;
    filePosition = 0x00;
    reportedFileLength = TEST_FONT_SIZE;
    closeCount = 0x00;
    allocationCount = 0x00;
    freeCount = 0x00;
}

static void TestValidFontLoad(void)
{
    uint8_t *fontResource;

    PrepareValidFont();
    fontResource = LoadFontResource("FONT6.FNT");
    Check(fontResource != NULL, "a structurally valid font loads");
    Check(closeCount == 0x01 && allocationCount == 0x01 && freeCount == 0x00,
        "successful load closes the file and retains its allocation");
    if (fontResource == NULL)
        return;
    Check(memcmp(fontResource, fontFile, TEST_FONT_SIZE) == 0,
        "font loader rewinds and reads the complete file");

    SetActiveFontResource(fontResource);
    Check(ActiveFontResource == fontResource,
        "active font is retained as a native resource pointer");
    Check(ActiveFontGlyphAdvanceX == 0x06 &&
            ActiveFontLineHeight == 0x06,
        "active font caches width and height header bytes");
    Check(MeasureTextWidth("ABC") == 0x0012,
        "text width resolves the active font and multiplies string length");

    Heap_Free(fontResource);
}

static void TestInvalidPhysicalLength(void)
{
    uint8_t *fontResource;

    PrepareValidFont();
    reportedFileLength = TEST_FONT_SIZE - 0x01;
    fontResource = LoadFontResource("FONT6.FNT");
    Check(fontResource == NULL,
        "a physical-length mismatch rejects the font");
    Check(closeCount == 0x01 && allocationCount == 0x00,
        "rejected metadata still closes without allocating");
}

int main(void)
{
    TestValidFontLoad();
    TestInvalidPhysicalLength();
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
