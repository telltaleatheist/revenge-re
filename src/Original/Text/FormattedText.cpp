// 35CE: formatted text
// Reconstructed original formatted text interpreter (segment 35CE).

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "REVENGE.h"
#include "revenge/original_data.h"

#include <string.h>

enum FormattedTextControl
{
    FORMATTED_TEXT_END = 0x00,
    FORMATTED_TEXT_SET_BACKGROUND = 0x02,
    FORMATTED_TEXT_SET_FOREGROUND = 0x06,
    FORMATTED_TEXT_SET_COLUMN = 0x09,
    FORMATTED_TEXT_NEWLINE = 0x0D,
    FORMATTED_TEXT_PAD_TO_COLUMN = 0x13,
    FORMATTED_TEXT_SPACE = 0x20
};

// 35CE:000A: void RenderFormattedText(Stack far pointer formattedText)
// Called from:
//      ShowConfirmationDialog
//      ShowScenarioMessage
//      UpdateTextEntryCursorBlink
//      EditTextField
//      ShowInvalidSaveVersionDialog
//      SaveGame
//      ShowSaveLoadMenu
//      RenameSaveSlot
//      WaitForAcknowledgement
//      PromptYesNo
//      RunGameOptionsMenu
void RenderFormattedText(const char *formattedText)
{
    // Verified against ASM 35CE:000A-0530. Render the encoded NUL-terminated
    // text stream, including the original two-byte dictionary expansion,
    // wrapping, layout controls, color controls and mutable render buffers.
    char composedLine[0x52];
    char tokenBuffer[0x52];
    // V: 35CE:0031 explicitly writes this terminator; Reko omitted it.
    composedLine[0x00] = '\0';
    uint16_t allowLeadingSpace = 0x01;
    uint16_t reachedEnd = 0x00;
    uint16_t hasPendingExpandedByte = 0x00;
    uint8_t pendingExpandedByte = 0x00;
    uint16_t tokenLength = 0x00;
    uint16_t textOffset = 0x00;
    uint16_t lineColumn = ActiveTextCursorColumn;
    uint16_t requestedColumn;
    uint16_t paddingTargetColumn;
    uint16_t remainingPaddingColumns;
    while (reachedEnd == 0x00)
    {
        uint8_t formatByte;
        if (hasPendingExpandedByte == 0x00)
        {
            formatByte = (uint8_t) formattedText[textOffset];
            ++textOffset;
        }
        else
        {
            formatByte = pendingExpandedByte;
            hasPendingExpandedByte = 0x00;
        }
        if ((formatByte & 0x80) != 0x00)
        {
            uint8_t dictionaryIndex = (uint8_t) (((int16_t) formatByte & 0x7F) >> 0x03);
            pendingExpandedByte = CompressedTextSecondByteTable[(uint16_t) dictionaryIndex * 0x08 + (formatByte & 0x07)];
            formatByte = CompressedTextFirstByteTable[dictionaryIndex];
            hasPendingExpandedByte = 0x01;
        }
        switch (formatByte)
        {
        case FORMATTED_TEXT_END:
            reachedEnd = 0x01;
            break;
        case FORMATTED_TEXT_NEWLINE:
            tokenBuffer[tokenLength] = 0x00;
            if (ActiveFontGlyphAdvanceX *
                    (tokenLength + lineColumn) <=
                ActiveTextWindowWidthTiles << 0x03)
            {
                strcat(composedLine, tokenBuffer);
                RenderTextToActiveWindow(composedLine, 0x01);
            }
            else
            {
                RenderTextToActiveWindow(composedLine, 0x01);
                RenderTextToActiveWindow(tokenBuffer, 0x01);
            }
            tokenLength = 0x00;
            lineColumn = 0x00;
            allowLeadingSpace = 0x01;
            break;
        case FORMATTED_TEXT_SET_BACKGROUND:
            tokenBuffer[tokenLength] = 0x00;
            if (ActiveFontGlyphAdvanceX *
                    (tokenLength + lineColumn) <=
                ActiveTextWindowWidthTiles << 0x03)
            {
                strcat(composedLine, tokenBuffer);
                RenderTextToActiveWindow(composedLine, 0x00);
            }
            else
            {
                RenderTextToActiveWindow(composedLine, 0x01);
                RenderTextToActiveWindow(tokenBuffer, 0x00);
            }
            lineColumn = ActiveTextCursorColumn;
            ActiveTextBackgroundColor =
                (uint16_t) (int8_t) formattedText[textOffset];
            tokenLength = 0x00;
            ++textOffset;
            break;
        case FORMATTED_TEXT_SET_FOREGROUND:
            tokenBuffer[tokenLength] = 0x00;
            if (ActiveFontGlyphAdvanceX *
                    (tokenLength + lineColumn) <=
                ActiveTextWindowWidthTiles << 0x03)
            {
                strcat(composedLine, tokenBuffer);
                RenderTextToActiveWindow(composedLine, 0x00);
            }
            else
            {
                RenderTextToActiveWindow(composedLine, 0x01);
                RenderTextToActiveWindow(tokenBuffer, 0x00);
            }
            lineColumn = ActiveTextCursorColumn;
            ActiveTextForegroundColor =
                (uint16_t) (int8_t) formattedText[textOffset];
            tokenLength = 0x00;
            ++textOffset;
            break;
        case FORMATTED_TEXT_SET_COLUMN:
            tokenBuffer[tokenLength] = 0x00;
            if (ActiveFontGlyphAdvanceX *
                    (tokenLength + lineColumn) <=
                ActiveTextWindowWidthTiles << 0x03)
            {
                strcat(composedLine, tokenBuffer);
                RenderTextToActiveWindow(composedLine, 0x00);
            }
            else
            {
                RenderTextToActiveWindow(composedLine, 0x01);
                RenderTextToActiveWindow(tokenBuffer, 0x00);
            }
            requestedColumn = (uint16_t) (int8_t) formattedText[textOffset];
            ActiveTextCursorColumn = requestedColumn;
            tokenLength = 0x00;
            lineColumn = requestedColumn;
            ++textOffset;
            if (ActiveFontGlyphAdvanceX *
                    (ActiveTextCursorColumn + 0x01) >=
                ActiveTextWindowWidthTiles << 0x03)
            {
                ActiveTextCursorColumn = 0x00;
                lineColumn = 0x00;
            }
            break;
        case FORMATTED_TEXT_PAD_TO_COLUMN:
            paddingTargetColumn = (uint16_t) (int8_t) formattedText[textOffset];
            remainingPaddingColumns = paddingTargetColumn;
            ++textOffset;
            if ((int16_t) ActiveTextCursorColumn < (int16_t) paddingTargetColumn)
            {
                for (;
                    (int16_t) ActiveTextCursorColumn <
                    (int16_t) remainingPaddingColumns;
                    --remainingPaddingColumns)
                {
                    tokenBuffer[tokenLength] = 0x20;
                    tokenLength = (uint16_t) tokenLength + 0x01;
                }
                tokenBuffer[tokenLength] = 0x00;
                strcat(composedLine, tokenBuffer);
                lineColumn += tokenLength;
                tokenLength = 0x00;
            }
            break;
        case FORMATTED_TEXT_SPACE:
            if (ActiveFontGlyphAdvanceX *
                    (tokenLength + lineColumn) <=
                ActiveTextWindowWidthTiles << 0x03)
            {
                if (ActiveFontGlyphAdvanceX *
                            ((tokenLength + lineColumn) + 0x01) <=
                        ActiveTextWindowWidthTiles << 0x03 &&
                    (tokenLength + lineColumn != 0x00 || allowLeadingSpace != 0x00))
                {
                    tokenBuffer[tokenLength] = 0x20;
                    tokenLength = (uint16_t) tokenLength + 0x01;
                }
                tokenBuffer[tokenLength] = 0x00;
                strcat(composedLine, tokenBuffer);
            }
            else
            {
                if (ActiveFontGlyphAdvanceX *
                            ((uint16_t) tokenLength + 0x01) <=
                        ActiveTextWindowWidthTiles << 0x03 &&
                    tokenLength != 0x00)
                {
                    tokenBuffer[tokenLength] = 0x20;
                    tokenLength = (uint16_t) tokenLength + 0x01;
                }
                tokenBuffer[tokenLength] = 0x00;
                RenderTextToActiveWindow(composedLine, 0x01);
                strcpy(composedLine, tokenBuffer);
                lineColumn = 0x00;
            }
            lineColumn += tokenLength;
            tokenLength = 0x00;
            allowLeadingSpace = 0x00;
            break;
        default:
            if (tokenLength * ActiveFontGlyphAdvanceX >=
                ActiveTextWindowWidthTiles << 0x03)
            {
                if (composedLine[0x00] != 0x00)
                    RenderTextToActiveWindow(composedLine, 0x01);
                tokenBuffer[tokenLength] = 0x00;
                strcat(composedLine, tokenBuffer);
                RenderTextToActiveWindow(composedLine, 0x01);
                lineColumn = 0x00;
                tokenBuffer[0x00] = formatByte;
                tokenLength = 0x01;
            }
            else
            {
                tokenBuffer[tokenLength] = formatByte;
                tokenLength = (uint16_t) tokenLength + 0x01;
            }
            break;
        }
    }
    if (tokenLength != 0x00 || composedLine[0x00] != 0x00)
    {
        tokenBuffer[tokenLength] = 0x00;
        if ((ActiveTextWindowWidthTiles << 0x03) -
                ActiveFontGlyphAdvanceX >=
            ActiveFontGlyphAdvanceX *
                (tokenLength + lineColumn))
        {
            strcat(composedLine, tokenBuffer);
            RenderTextToActiveWindow(composedLine, 0x00);
        }
        else
        {
            RenderTextToActiveWindow(composedLine, 0x01);
            RenderTextToActiveWindow(tokenBuffer, 0x00);
        }
    }
}
