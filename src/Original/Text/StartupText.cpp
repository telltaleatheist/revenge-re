// 0D73: startup text
/* Startup string procedures from original segment 0D73. */

#include "revenge/types.h"

#include <string.h>

// 0D73:418E: void RightAlignStringField(char far *text, word16 fieldWidth)
// Called from:
//      InitializeGame
void RightAlignStringField(char *text, uint16_t fieldWidth)
{
    uint16_t textLength = (uint16_t) strlen(text);
    if (textLength >= fieldWidth)
        return;
    uint16_t leadingSpaces = fieldWidth - textLength;
    int16_t index;

    // The original moves the NUL and one following byte, preserving the
    // double-NUL convention used by the startup version buffer.
    for (index = textLength + 0x01; index >= 0x00; --index)
        text[index + leadingSpaces] = text[index];
    for (index = 0x00; index < leadingSpaces; ++index)
        text[index] = ' ';
}

// 0D73:4288: void TranslateCharactersInPlace(char far *text, const char far *sourceAlphabet, const char far *replacementAlphabet)
// Called from:
//      InitializeGame
void TranslateCharactersInPlace(
    char *text,
    const char *sourceAlphabet,
    const char *replacementAlphabet)
{
    uint16_t textLength = (uint16_t) strlen(text);
    uint16_t alphabetLength = (uint16_t) strlen(sourceAlphabet);
    uint16_t textIndex;
    for (textIndex = 0x00; textIndex < textLength; ++textIndex)
    {
        uint16_t alphabetIndex;
        for (alphabetIndex = 0x00; alphabetIndex < alphabetLength; ++alphabetIndex)
        {
            if (text[textIndex] == sourceAlphabet[alphabetIndex])
                text[textIndex] = replacementAlphabet[alphabetIndex];
        }
    }
}
