// 0800, 0D73, 3338, 3349, 3477, 356D, 356F: Text declarations
#ifndef REVENGE_TEXT_TEXT_H
#define REVENGE_TEXT_TEXT_H

#include "revenge/types.h"

void ShowScenarioMessage(uint16_t messageIndex);
void ShowMissionFollowupMessage(uint16_t messageIndex);
void UpdateTextEntryCursorBlink(void);
uint16_t EditTextField(
    uint16_t maximumLength,
    const char *initialText,
    uint8_t allowedCharacterClasses);

void DrawTextString(
    const char *text,
    uint16_t x,
    uint16_t y,
    uint8_t foregroundColor,
    uint8_t backgroundColor);
uint16_t MeasureTextWidth(const char *text);
void SelectTextWindow(uint16_t windowIndex);
void SelectAndClearTextWindow(uint16_t windowIndex);
void ClearActiveTextWindow(void);
uint8_t *LoadFontResource(const char *fileName);
void SetActiveFontResource(uint8_t *fontResource);
void RenderTextToActiveWindow(
    char *text, uint16_t advanceToNextLine);

#endif
