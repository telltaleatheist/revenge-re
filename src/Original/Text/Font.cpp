// 3477: font
// Reconstructed original font resource loader and metrics (segment 3477).

#include "Data/RuntimeData.h"
#include "Text/Text.h"
#include "Utility/Utility.h"
#include "revenge/heap.h"
#include "revenge/text.h"
#include "revenge/game_file.h"

#include <string.h>

// 3477:000B: unsigned char *LoadFontResource(
//     const char *fileName)
uint8_t *LoadFontResource(const char *fileName)
{
    uint16_t totalFileSize = 0x00;
    uint8_t dimensions[0x02];
    uint8_t *fontResource = NULL;
    int16_t fileHandle = GameFile_Open(fileName, GAME_FILE_READ); /* 8001h */

    if (fileHandle != -0x01)
    {
        uint16_t headerBytesRead = GameFile_Read(
            fileHandle,
            (uint8_t *) &totalFileSize,
            0x02);
        totalFileSize = (uint16_t) (totalFileSize + 0x02);
        if (headerBytesRead == 0x02 && totalFileSize > 0x0104)
        {
            int32_t physicalFileLength = GameFile_Length(fileHandle);
            if (HIGH_WORD(physicalFileLength) == 0x00 &&
                LOW_WORD(physicalFileLength) == totalFileSize)
            {
                int32_t seekPosition = GameFile_Seek(
                    fileHandle, 0x0102L, GAME_FILE_FROM_START);
                if (seekPosition == 0x0102L)
                {
                    uint16_t dimensionsRead = GameFile_Read(
                        fileHandle,
                        dimensions,
                        0x02);
                    if (dimensionsRead == 0x02)
                    {
                        int16_t glyphHeight = (int8_t) dimensions[0x00];
                        int16_t bytesPerGlyphRow =
                            ((int8_t) dimensions[0x01] + 0x07) / 0x08;
                        uint16_t glyphDataSize = (uint16_t) ((uint16_t) glyphHeight *
                                (uint16_t) bytesPerGlyphRow
                            << 0x07);
                        uint16_t allocationSize =
                            (uint16_t) (glyphDataSize + 0x0104);

                        if (allocationSize == totalFileSize)
                        {
                            fontResource = (uint8_t *)
                                Heap_Allocate(allocationSize, "3477:00FF");
                            if (fontResource != NULL)
                            {
                                if (GameFile_Seek(
                                        fileHandle,
                                        0x00L,
                                        GAME_FILE_FROM_START) == 0x00L)
                                {
                                    uint16_t fontBytesRead =
                                        GameFile_Read(
                                            fileHandle,
                                            fontResource,
                                            totalFileSize);
                                    if (fontBytesRead != totalFileSize)
                                    {
                                        Heap_Free(fontResource);
                                        fontResource = NULL;
                                    }
                                }
                                else
                                {
                                    Heap_Free(fontResource);
                                    fontResource = NULL;
                                }
                            }
                        }
                    }
                }
            }
        }
        GameFile_Close(fileHandle);
    }
    return fontResource;
}

// 3477:018F: void SetActiveFontResource(byte *fontResource)
void SetActiveFontResource(uint8_t *fontResource)
{
    ActiveFontResource = fontResource;
    ActiveFontGlyphAdvanceX = (int8_t) fontResource[0x0103];
    ActiveFontLineHeight = (int8_t) fontResource[0x0102];
}

// 3477:01B5: uint16 MeasureTextWidth(const char *text)
uint16_t MeasureTextWidth(const char *text)
{
    const uint8_t *activeFont = ActiveFontResource;
    int16_t glyphAdvance = (int8_t) activeFont[0x0103];

    return (uint16_t) ((uint16_t) strlen(text) * glyphAdvance);
}
