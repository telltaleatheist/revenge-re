// 0D73: text editor
/* Preserved fixed-width text-field editor from REVENGE_0D73. */

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "REVENGE.h"

#include "revenge/input.h"
#include "revenge/original_data.h"
#include "revenge/text.h"

#include <string.h>

/* 0D73:4317-4549 */
uint16_t EditTextField(
    uint16_t maximumLength,
    const char *initialText,
    uint8_t allowedCharacterClasses)
{
    uint16_t punctuationCount;
    int16_t glyphAdvance;
    uint16_t effectiveMaximumLength;
    uint16_t windowWidthPixels;
    uint16_t savedCursorColumn;
    uint16_t savedCursorRow;
    uint16_t cursorIndex;
    int16_t keyCode;
    uint16_t firstKey;

    HideMouseCursorOverlay();
    TextEntryActive = TRUE;
    punctuationCount = (uint16_t) strlen(TextEntryPunctuation);
    glyphAdvance = ActiveFontGlyphAdvanceX;
    effectiveMaximumLength = maximumLength;
    windowWidthPixels = ActiveTextWindowWidthTiles << 0x03;
    if ((int16_t) ((maximumLength + ActiveTextCursorColumn) *
            (uint16_t) glyphAdvance) >= (int16_t) windowWidthPixels)
    {
        effectiveMaximumLength = (uint16_t) ((int16_t) (windowWidthPixels - ActiveTextCursorColumn * (uint16_t) glyphAdvance) / glyphAdvance);
    }

    if (*initialText == ' ')
        FillTextEntryBuffer(effectiveMaximumLength, ' ');
    else
        strcpy((char *) TextEntryBuffer, initialText);

    savedCursorColumn = ActiveTextCursorColumn;
    savedCursorRow = ActiveTextCursorRow;
    if ((uint16_t) strlen((const char *) TextEntryBuffer) <
        effectiveMaximumLength)
    {
        PadStringToWidth(
            (char *) TextEntryBuffer, effectiveMaximumLength);
    }
    RenderFormattedText(TextEntryBuffer);
    ActiveTextCursorColumn = savedCursorColumn;
    ActiveTextCursorRow = savedCursorRow;

    cursorIndex = 0x0000;
    keyCode = 0x0000;
    firstKey = TRUE;
    while (cursorIndex <= effectiveMaximumLength && keyCode != KEY_CARRIAGE_RETURN)
    {
        uint16_t acceptedCharacter;

        TextEntryCursorIndex = cursorIndex;
        keyCode = (int16_t) WaitForKeyboardOrMouseInput();
        if (firstKey != FALSE)
        {
            firstKey = FALSE;
            if (keyCode != KEY_CARRIAGE_RETURN)
                FillTextEntryBuffer(effectiveMaximumLength, ' ');
        }

        acceptedCharacter = FALSE;
        if (keyCode == KEY_ESCAPE || keyCode == KEY_CTRL_S)
        {
            FillTextEntryBuffer(effectiveMaximumLength, ' ');
            cursorIndex = 0x0000;
        }
        else if (keyCode == KEY_DELETE || keyCode == KEY_BACKSPACE)
        {
            if (cursorIndex > 0x0000)
                --cursorIndex;
            TextEntryBuffer[cursorIndex] = ' ';
        }
        else if (keyCode != KEY_CARRIAGE_RETURN && keyCode >= 0x0000)
        {
            uint8_t characterClass;

            characterClass = CharacterClassTable[(uint8_t) keyCode];
            if ((allowedCharacterClasses & 0x02) != 0x00 &&
                ((characterClass & 0x0C) != 0x00 || keyCode == KEY_SPACE))
            {
                keyCode &= 0x007F;
                acceptedCharacter = TRUE;
            }
            if ((allowedCharacterClasses & 0x01) != 0x00 &&
                acceptedCharacter == FALSE &&
                (characterClass & 0x02) != 0x00)
            {
                acceptedCharacter = TRUE;
            }
            if ((allowedCharacterClasses & 0x04) != 0x00 &&
                acceptedCharacter == FALSE)
            {
                uint16_t punctuationIndex;

                punctuationIndex = 0x0000;
                while (punctuationIndex < punctuationCount &&
                    (uint8_t) TextEntryPunctuation[punctuationIndex] !=
                        (uint8_t) keyCode)
                {
                    ++punctuationIndex;
                }
                if (punctuationIndex != punctuationCount)
                    acceptedCharacter = TRUE;
            }
            if (acceptedCharacter != FALSE &&
                cursorIndex < effectiveMaximumLength)
            {
                TextEntryBuffer[cursorIndex++] = (uint8_t) keyCode;
                TextEntryBuffer[cursorIndex] = ' ';
            }
        }

        savedCursorColumn = ActiveTextCursorColumn;
        savedCursorRow = ActiveTextCursorRow;
        RenderFormattedText(TextEntryBuffer);
        ActiveTextCursorColumn = savedCursorColumn;
        ActiveTextCursorRow = savedCursorRow;
    }

    TrimTrailingEncodedSpaces((char *) TextEntryBuffer);
    TextEntryActive = FALSE;
    ShowMouseCursorOverlay();
    return (uint16_t) strlen((const char *) TextEntryBuffer);
}
