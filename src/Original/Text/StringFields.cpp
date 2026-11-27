// 0D73: string fields
/* Fixed-width string procedures from original segment 0D73. */

#include "Data/CampaignData.h"

#include "revenge/text.h"

#include <string.h>

// 0D73:41E8: void RemoveLeadingSpacesFrom(char far *text, word16 oneBasedStart)
// Called from:
//      DrawCenteredClippedText
void RemoveLeadingSpacesFrom(char *text, uint16_t oneBasedStart)
{
    uint16_t textLength = (uint16_t) strlen(text);
    uint16_t sourceIndex = oneBasedStart - 0x01;
    uint16_t destinationIndex = sourceIndex;

    while (sourceIndex < textLength && text[sourceIndex] == ' ')
        ++sourceIndex;
    while (sourceIndex <= textLength)
        text[destinationIndex++] = text[sourceIndex++];
}

// 0D73:4238: void TrimTrailingEncodedSpaces(char far *text)
// Called from:
//      DrawCenteredClippedText
//      EditTextField
void TrimTrailingEncodedSpaces(char *text)
{
    int16_t index;
    for (index = (uint16_t) strlen(text) - 0x01; index >= 0x00; --index)
    {
        if ((text[index] & 0x3F) != ' ')
        {
            text[index + 0x01] = '\0';
            return;
        }
    }
    text[0x00] = '\0';
}

// 0D73:42E4: void PadStringToWidth(char far *text, word16 width)
// Called from:
//      EditTextField
void PadStringToWidth(char *text, uint16_t width)
{
    uint16_t index = (uint16_t) strlen(text);
    while (index < width)
        text[index++] = ' ';
    text[width] = '\0';
}

// 0D73:454A: void FillTextEntryBuffer(word16 length, byte fillCharacter)
// Called from:
//      EditTextField
void FillTextEntryBuffer(uint16_t length, uint8_t fillCharacter)
{
    uint16_t index;
    for (index = 0x00; index < length; ++index)
        TextEntryBuffer[index] = fillCharacter;
    TextEntryBuffer[index++] = '\0';
    if (index <= 0x15)
        TextEntryBuffer[index] = '\0';
}
