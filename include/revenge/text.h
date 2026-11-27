#ifndef REVENGE_TEXT_H
#define REVENGE_TEXT_H

#include "revenge/types.h"

enum
{
    REVENGE_FONT_GLYPH_COUNT = 0x80,
    REVENGE_TEXT_WINDOW_COUNT = 0x17
};

typedef struct TextWindowContext
{
    uint16_t leftTile;
    uint16_t topTile;
    uint16_t widthTiles;
    uint16_t heightTiles;
    uint16_t foregroundColor;
    uint16_t backgroundColor;
    uint16_t cursorColumn;
    uint16_t cursorRow;
} TextWindowContext;

typedef struct ActiveTextWindowState
{
    uint16_t index;
    uint16_t leftTile;
    uint16_t topTile;
    uint16_t widthTiles;
    uint16_t heightTiles;
    uint16_t foregroundColor;
    uint16_t backgroundColor;
    uint16_t cursorColumn;
    uint16_t cursorRow;
} ActiveTextWindowState;

uint8_t *LoadFontResource(const char *fileName);
void SetActiveFontResource(uint8_t *fontResource);
uint16_t MeasureTextWidth(const char *text);
uint8_t TextSystem_GetActiveFontWidth(void);
uint8_t TextSystem_GetActiveFontHeight(void);

void DrawFontGlyph(uint16_t glyphIndex, uint16_t x, uint16_t y);
void DrawTextString(
    const char *text,
    uint16_t x,
    uint16_t y,
    uint8_t foregroundColor,
    uint8_t backgroundColor);
void RemoveLeadingSpacesFrom(char *text, uint16_t oneBasedStart);
void TrimTrailingEncodedSpaces(char *text);
void PadStringToWidth(char *text, uint16_t width);
void FillTextEntryBuffer(uint16_t length, uint8_t fillCharacter);

void TextSystem_SetWindowContexts(
    const TextWindowContext *contexts,
    uint16_t contextCount);
void TextSystem_SetWindowContext(
    uint16_t windowIndex,
    const TextWindowContext *context);
void TextSystem_SetCompressionTables(
    const uint8_t firstByteTable[0x10],
    const uint8_t secondByteTable[0x80]);
const ActiveTextWindowState *TextSystem_GetActiveWindow(void);
void SelectTextWindow(uint16_t windowIndex);
void SelectAndClearTextWindow(uint16_t windowIndex);
void ClearActiveTextWindow(void);
void RenderTextToActiveWindow(char *text, uint16_t advanceToNextLine);
void RenderFormattedText(const char *formattedText);
void DrawTileAlignedText(
    const char *text,
    uint16_t tileX,
    uint16_t tileY,
    uint8_t foregroundColor,
    uint8_t backgroundColor);
void DrawCenteredTextInActiveWindow(
    const char *text,
    uint16_t rowOffset,
    uint16_t foregroundColor,
    uint16_t backgroundColor);
void DrawCenteredClippedText(
    const char *sourceText,
    uint16_t splitIndex,
    uint16_t leftX,
    uint16_t y,
    uint16_t maximumWidth,
    uint8_t foregroundColor,
    uint8_t backgroundColor);
void DrawLeftAlignedClippedText(
    const char *sourceText,
    uint16_t x,
    uint16_t y,
    uint16_t maximumWidth,
    uint8_t foregroundColor,
    uint8_t backgroundColor);
void DrawRightAlignedClippedText(
    const char *sourceText,
    uint16_t leftX,
    uint16_t y,
    uint16_t maximumWidth,
    uint8_t foregroundColor,
    uint8_t backgroundColor);
void DrawActiveTextWindowBorder(uint16_t color);
void DrawBeveledMenuButton(
    uint16_t textColumn,
    uint16_t textRow,
    const char *text,
    uint16_t widthPixels);

#endif
