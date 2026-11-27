#define _CRT_SECURE_NO_WARNINGS   /* original_import_for_tests.h uses fopen */
#include "REVENGE.h"
#include "revenge/original_data.h"
#include "revenge/original/campaign_data.h"

#include "original_import_for_tests.h"
#include <assert.h>
#include <string.h>

static char renderedText[0x08][0x52];
static uint16_t renderedAdvance[0x08];
static uint16_t renderCount;

void RenderTextToActiveWindow(char *text, uint16_t advanceToNextLine)
{
    size_t length = strlen(text);
    assert(length < sizeof(renderedText[renderCount]));
    memcpy(renderedText[renderCount], text, length + 0x01);
    renderedAdvance[renderCount] = advanceToNextLine;
    ++renderCount;
    text[0x00] = '\0';
}

static void ResetRenderer(uint16_t widthTiles)
{
    memset(renderedText, 0x00, sizeof(renderedText));
    memset(renderedAdvance, 0x00, sizeof(renderedAdvance));
    renderCount = 0x0000;
    ActiveTextWindowWidthTiles = widthTiles;
    ActiveTextCursorColumn = 0x0000;
    ActiveFontGlyphAdvanceX = 0x01;
    ActiveTextForegroundColor = 0x0007;
    ActiveTextBackgroundColor = 0x0000;
}

int main(void)
{
    static const char plain[] = "Hello";
    static const char newline[] = "One\rTwo";
    static const char foreground[] = { 'A', 0x06, 0x05, 'B', 0x00 };
    static const char wrapped[] = "123456789";
    static const uint8_t compressedPair[] = { 0x80, 0x00 };
    uint16_t messageIndex;

    RequireOriginalImport();
    assert(CompressedTextFirstByteTable[0x00] == ' ');
    assert(CompressedTextFirstByteTable[0x0F] == 'm');
    assert(CompressedTextSecondByteTable[0x00] == 't');
    assert(CompressedTextSecondByteTable[0x7F] == 0x80);
    /* The messages come expanded: none is empty, and no byte keeps the
	 * high bit of a compressed pair. */
    assert(CampaignMessageTextTable[0x01][0x00] != 0x00);
    for (messageIndex = 0x0000;
        messageIndex < CAMPAIGN_MESSAGE_COUNT;
        ++messageIndex)
    {
        const uint8_t *message =
            (const uint8_t *) CampaignMessageTextTable[messageIndex];
        while (*message != 0x00)
        {
            assert((*message & 0x80) == 0x00);
            ++message;
        }
    }

    ResetRenderer(0x0028);
    RenderFormattedText(plain);
    assert(renderCount == 0x0001);
    assert(strcmp(renderedText[0x00], "Hello") == 0x00);
    assert(renderedAdvance[0x00] == 0x0000);

    ResetRenderer(0x0028);
    RenderFormattedText(newline);
    assert(renderCount == 0x0002);
    assert(strcmp(renderedText[0x00], "One") == 0x00);
    assert(renderedAdvance[0x00] == 0x0001);
    assert(strcmp(renderedText[0x01], "Two") == 0x00);

    ResetRenderer(0x0028);
    RenderFormattedText(foreground);
    assert(renderCount == 0x0002);
    assert(strcmp(renderedText[0x00], "A") == 0x00);
    assert(strcmp(renderedText[0x01], "B") == 0x00);
    assert(ActiveTextForegroundColor == 0x0005);

    ResetRenderer(0x0001);
    RenderFormattedText(wrapped);
    assert(renderCount == 0x0002);
    assert(strcmp(renderedText[0x00], "12345678") == 0x00);
    assert(renderedAdvance[0x00] == 0x0001);
    assert(strcmp(renderedText[0x01], "9") == 0x00);

    ResetRenderer(0x0028);
    RenderFormattedText((const char *) compressedPair);
    assert(renderCount == 0x0001);
    assert(strcmp(renderedText[0x00], " t") == 0x00);

    return 0x00;
}
