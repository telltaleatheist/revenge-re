// 2507: training
// Training.cpp
// Reconstructed from the original procedures in segment 2507.

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "LanceEditorInternal.h"
#include "Campaign/Campaign.h"
#include "Data/RuntimeData.h"
#include "Data/StartupData.h"
#include "Data/TacticalData.h"
#include "Data/TextIOData.h"
#include "Graphics/Graphics.h"
#include "Input/Input.h"
#include "Runtime/Runtime.h"
#include "Text/Text.h"
#include "UI/UI.h"
#include "revenge/tactical_panel_data.h"
#include "revenge/story_text.h"

#include <string.h>

// 2507:0008: enable the pilots used by the three training-selection rows.
// VERIFIED against physical-alias ASM 24C5:0428-0475 (2507:0008-0055).
void EnableTrainingUnitSelectionSlots(void)
{
    uint16_t pilotRow;
    uint16_t lanceSlot;
    uint16_t pilotId;

    for (pilotRow = 0x0002; pilotRow < 0x0005; ++pilotRow)
    {
        for (lanceSlot = 0x0000; lanceSlot < 0x0004; ++lanceSlot)
        {
            pilotId = DefaultPilotIdsByScenario[pilotRow * 0x0004 + lanceSlot];
            PilotRoster[pilotId].availability = 0x01;
        }
    }
}

// 2507:16E9: draw one entry in the two-choice training category window.
// Verified against 2507:16E9-1777.
void DrawTrainingCategoryOption(
    uint16_t optionIndex,
    uint16_t selectedState)
{
    uint16_t previousWindow = ActiveTextWindowIndex;
    uint16_t leftX;
    uint16_t topY;
    uint16_t width;
    const char *label;

    SelectTextWindow(0x13);
    HideMouseCursorOverlay();
    leftX = ActiveTextWindowLeftTile << 0x03;
    topY = (uint16_t) ((ActiveTextWindowTopTile + 0x02) << 0x03);
    topY = (uint16_t) (topY + optionIndex * (ActiveFontLineHeight + 0x02));
    width = ActiveTextWindowWidthTiles << 0x03;
    label = TrainingCategoryLabelTextTable[optionIndex];
    DrawCenteredClippedText(label, 0x00, leftX, topY, width,
        (uint8_t) LanceEditorLayout.TrainingCategoryTextColor[selectedState],
        (uint8_t) LanceEditorLayout.TrainingCategoryBackgroundColor[selectedState]);
    ShowMouseCursorOverlay();
    SelectTextWindow(previousWindow);
}

// 2507:1778: draw one entry in the six-choice training scenario window.
// Verified against 2507:1778-1847.
void DrawTrainingScenarioOption(
    uint16_t scenarioIndex,
    uint16_t selectedState)
{
    uint16_t previousWindow = ActiveTextWindowIndex;
    uint16_t leftX;
    uint16_t topY;
    uint16_t width;
    char *label;

    SelectTextWindow(0x14);
    HideMouseCursorOverlay();
    leftX = (uint16_t) (ActiveTextWindowLeftTile << 0x03);
    leftX = (uint16_t) (leftX +
        (scenarioIndex & 0x01) * (ActiveTextWindowWidthTiles << 0x02));
    topY = (uint16_t) ((ActiveTextWindowTopTile + 0x02) << 0x03);
    topY = (uint16_t) (topY + (scenarioIndex >> 0x01) * (ActiveFontLineHeight + 0x02));
    width = ActiveTextWindowWidthTiles << 0x02;
    label = TrainingScenarioLabelTextTable[scenarioIndex];
    label[0x00] = TrainingScenarioAvailability[scenarioIndex] == 0x01 ? '*' : ' ';
    DrawCenteredClippedText(label, 0x00, leftX, topY, width,
        (uint8_t) LanceEditorLayout.TrainingScenarioTextColor[selectedState],
        (uint8_t) LanceEditorLayout.TrainingScenarioBackgroundColor[selectedState]);
    ShowMouseCursorOverlay();
    SelectTextWindow(previousWindow);
}

// 2507:0E04: select the training mode and, for normal training, its scenario.
// Verified against 2507:0E04-1141.
int16_t RunTrainingScenarioSelection(void)
{
    uint16_t previousWindow;
    uint16_t categoryIndex = 0x00;
    uint16_t previousIndex = 0x00;
    uint16_t accepted;
    uint16_t chooseLance = FALSE;
    uint16_t inputCode;
    uint16_t mouseButtonState;
    uint16_t mouseButtonEvent;
    int16_t mouseX;
    int16_t mouseY;
    uint16_t entryIndex;
    int16_t leftX;
    int16_t topY;
    int16_t width;
    int16_t height;

    ReplayScenarioRequested = FALSE;
    SceneTransitionAbortFlag = FALSE;
    previousWindow = ActiveTextWindowIndex;
    SelectTextWindow(0x12);
    DrawTrainingScenarioSelectionScreen();
    FlushPendingInput();
    SelectTextWindow(0x13);
    DrawTrainingCategoryOption(categoryIndex, TRUE);
    FlushPendingInput();
    accepted = FALSE;
    while (accepted == FALSE)
    {
        if (ServiceMusicAndCheckKeyboardInput() != FALSE)
        {
            inputCode = (uint16_t) NormalizeNavigationKeyCode(
                ReadTranslatedKeypress());
            if ((int16_t) inputCode == TranslatedExtendedKey(KEY_UP))
                --categoryIndex;
            else if ((int16_t) inputCode == TranslatedExtendedKey(KEY_DOWN))
                ++categoryIndex;
            else if (inputCode == KEY_CARRIAGE_RETURN || inputCode == KEY_SPACE)
            {
                if (categoryIndex == 0x01)
                    chooseLance = TRUE;
                accepted = TRUE;
            }
            if ((int16_t) categoryIndex < 0x00)
                categoryIndex = 0x01;
            if (categoryIndex > 0x01)
                categoryIndex = 0x00;
        }
        else if (MouseInputDisabled == FALSE)
        {
            mouseX = MouseCursorRequestedX;
            mouseY = (int16_t) MouseY;
            mouseButtonState = MouseButtonState;
            mouseButtonEvent = PendingMouseButtonEvent;
            PendingMouseButtonEvent = FALSE;
            for (entryIndex = 0x00; entryIndex < 0x02; ++entryIndex)
            {
                leftX = (int16_t) ((ActiveTextWindowLeftTile << 0x03) +
                    LanceEditorLayout.TrainingCategoryRegionLeft[entryIndex]);
                topY = (int16_t) ((ActiveTextWindowTopTile << 0x03) +
                    LanceEditorLayout.TrainingCategoryRegionTop[entryIndex]);
                width = (int16_t) LanceEditorLayout.TrainingCategoryRegionWidth[entryIndex];
                height = (int16_t) LanceEditorLayout.TrainingCategoryRegionHeight[entryIndex];
                if (mouseX >= leftX && mouseX < leftX + width &&
                    mouseY >= topY && mouseY < topY + height)
                {
                    categoryIndex = entryIndex;
                    if (mouseButtonEvent != FALSE &&
                        (mouseButtonState & MOUSE_BUTTON_LEFT) != 0x00)
                    {
                        if (categoryIndex == 0x01)
                            chooseLance = TRUE;
                        accepted = TRUE;
                    }
                }
            }
        }
        if (categoryIndex != previousIndex)
        {
            DrawTrainingCategoryOption(previousIndex, FALSE);
            DrawTrainingCategoryOption(categoryIndex, TRUE);
            previousIndex = categoryIndex;
        }
    }

    if (chooseLance == FALSE)
    {
        uint16_t scenarioIndex = 0x00;
        previousIndex = 0x00;
        SelectTextWindow(0x14);
        DrawTrainingScenarioOption(scenarioIndex, TRUE);
        FlushPendingInput();
        accepted = FALSE;
        while (accepted == FALSE)
        {
            if (ServiceMusicAndCheckKeyboardInput() != FALSE)
            {
                inputCode = (uint16_t) NormalizeNavigationKeyCode(
                    ReadTranslatedKeypress());
                if ((int16_t) inputCode == TranslatedExtendedKey(KEY_UP))
                    scenarioIndex -= 0x02;
                else if ((int16_t) inputCode == TranslatedExtendedKey(KEY_DOWN))
                    scenarioIndex += 0x02;
                else if ((int16_t) inputCode == TranslatedExtendedKey(KEY_LEFT) ||
                    (int16_t) inputCode == TranslatedExtendedKey(KEY_RIGHT))
                    scenarioIndex ^= 0x01;
                else if (inputCode == KEY_CARRIAGE_RETURN || inputCode == KEY_SPACE)
                    accepted = TRUE;
                if ((int16_t) scenarioIndex < 0x00)
                    scenarioIndex += 0x06;
                if (scenarioIndex > 0x05)
                    scenarioIndex -= 0x06;
            }
            else if (MouseInputDisabled == FALSE)
            {
                mouseX = MouseCursorRequestedX;
                mouseY = (int16_t) MouseY;
                mouseButtonState = MouseButtonState;
                mouseButtonEvent = PendingMouseButtonEvent;
                PendingMouseButtonEvent = FALSE;
                for (entryIndex = 0x00; entryIndex < 0x06; ++entryIndex)
                {
                    leftX = (int16_t) ((ActiveTextWindowLeftTile << 0x03) +
                        LanceEditorLayout.TrainingScenarioRegionLeft[entryIndex]);
                    topY = (int16_t) ((ActiveTextWindowTopTile << 0x03) +
                        LanceEditorLayout.TrainingScenarioRegionTop[entryIndex]);
                    width = (int16_t) LanceEditorLayout.TrainingScenarioRegionWidth[entryIndex];
                    height = (int16_t) LanceEditorLayout.TrainingScenarioRegionHeight[entryIndex];
                    if (mouseX >= leftX && mouseX < leftX + width &&
                        mouseY >= topY && mouseY < topY + height)
                    {
                        scenarioIndex = entryIndex;
                        if (mouseButtonEvent != FALSE &&
                            (mouseButtonState & MOUSE_BUTTON_LEFT) != 0x00)
                            accepted = TRUE;
                    }
                }
            }
            if (scenarioIndex != previousIndex)
            {
                DrawTrainingScenarioOption(previousIndex, FALSE);
                DrawTrainingScenarioOption(scenarioIndex, TRUE);
                previousIndex = scenarioIndex;
            }
        }
        ScenarioVariantIndex = scenarioIndex;
    }
    SelectTextWindow(previousWindow);
    if (chooseLance != FALSE)
        PrepareLanceSelection(0x02, -0x01);
    return (int16_t) chooseLance;
}

// 2507:1142: compose the training deployment/scenario selection screen.
// Verified against 2507:1142-152D.
void DrawTrainingScenarioSelectionScreen(void)
{
    uint16_t previousWindow = ActiveTextWindowIndex;
    uint16_t leftX;
    uint16_t topY;
    uint16_t width;
    uint16_t gridTopY;
    uint16_t gridX;
    uint16_t groupIndex;
    uint16_t rowIndex;
    uint16_t halfLineOffset;
    uint16_t fullLineOffset;
    uint16_t index;
    const char *label;
    uint8_t *displayBuffer = PrimaryScreenBuffer;
    uint8_t *videoBuffer = VideoFramebuffer;

    HideMouseCursorOverlay();
    SetActiveRenderTarget(displayBuffer);
    SelectTextWindow(0x12);
    FillVideoRectangle(
        (int16_t) (ActiveTextWindowLeftTile << 0x03),
        (int16_t) (ActiveTextWindowTopTile << 0x03),
        (int16_t) (((ActiveTextWindowLeftTile + ActiveTextWindowWidthTiles) << 0x03) - 0x01),
        (int16_t) (((ActiveTextWindowTopTile + ActiveTextWindowHeightTiles) << 0x03) - 0x01),
        0x0C);
    leftX = ActiveTextWindowLeftTile << 0x03;
    topY = ActiveTextWindowTopTile << 0x03;
    width = ActiveTextWindowWidthTiles << 0x03;
    for (index = 0x00; index < 0x02; ++index)
    {
        label = TrainingDeploymentRosterLabelTextTable[index];
        DrawCenteredClippedText(label, 0x00, leftX, topY, width,
            0x0F, 0x00);
        topY = (uint16_t) (topY +
            (ActiveFontLineHeight << 0x01));
    }
    gridTopY = (uint16_t) (((ActiveTextWindowTopTile + 0x04) << 0x03) +
        ActiveFontLineHeight);
    halfLineOffset = (uint16_t) (((uint8_t) ActiveFontLineHeight >> 0x01) - 0x02);
    fullLineOffset = (uint16_t) (ActiveFontLineHeight - 0x02);
    DrawClippedAxisAlignedLine(
        (int16_t) LanceEditorLayout.TrainingDeploymentFrameLeft[0x00],
        (int16_t) (gridTopY + halfLineOffset),
        (int16_t) (LanceEditorLayout.TrainingDeploymentFrameLeft[0x02] + 0x01),
        (int16_t) (gridTopY + halfLineOffset), 0x0D);
    DrawClippedAxisAlignedLine(
        (int16_t) LanceEditorLayout.TrainingDeploymentFrameLeft[0x00],
        (int16_t) (gridTopY + halfLineOffset + 0x01),
        (int16_t) (LanceEditorLayout.TrainingDeploymentFrameLeft[0x02] + 0x01),
        (int16_t) (gridTopY + halfLineOffset + 0x01), 0x0D);
    for (groupIndex = 0x00; groupIndex < 0x03; ++groupIndex)
    {
        gridX = LanceEditorLayout.TrainingDeploymentFrameLeft[groupIndex];
        topY = gridTopY;
        for (rowIndex = 0x00; rowIndex < 0x05; ++rowIndex)
        {
            uint16_t lineTopY = topY;
            if (rowIndex == 0x00 && groupIndex != 0x01)
                lineTopY = (uint16_t) (topY + halfLineOffset);
            DrawClippedAxisAlignedLine((int16_t) gridX,
                (int16_t) lineTopY, (int16_t) gridX,
                (int16_t) (topY + fullLineOffset), 0x0D);
            DrawClippedAxisAlignedLine((int16_t) (gridX + 0x01),
                (int16_t) lineTopY, (int16_t) (gridX + 0x01),
                (int16_t) (topY + fullLineOffset), 0x0D);
            topY = (uint16_t) (topY +
                (ActiveFontLineHeight *
                    (rowIndex == 0x00 ? 0x02 : 0x03)));
        }
    }
    for (index = 0x00; index < 0x10; ++index)
        DrawTrainingDeploymentSlot(index, FALSE);

    SelectTextWindow(0x13);
    FillVideoRectangle(
        (int16_t) (ActiveTextWindowLeftTile << 0x03),
        (int16_t) (ActiveTextWindowTopTile << 0x03),
        (int16_t) (((ActiveTextWindowLeftTile + ActiveTextWindowWidthTiles) << 0x03) - 0x01),
        (int16_t) (((ActiveTextWindowTopTile + ActiveTextWindowHeightTiles) << 0x03) - 0x01),
        0x0C);
    label = TrainingCategoryHeadingText;
    DrawCenteredClippedText(label, 0x00,
        ActiveTextWindowLeftTile << 0x03,
        ActiveTextWindowTopTile << 0x03,
        ActiveTextWindowWidthTiles << 0x03, 0x0F, 0x00);
    for (index = 0x00; index < 0x02; ++index)
        DrawTrainingCategoryOption(index, FALSE);

    SelectTextWindow(0x14);
    FillVideoRectangle(
        (int16_t) (ActiveTextWindowLeftTile << 0x03),
        (int16_t) (ActiveTextWindowTopTile << 0x03),
        (int16_t) (((ActiveTextWindowLeftTile + ActiveTextWindowWidthTiles) << 0x03) - 0x01),
        (int16_t) (((ActiveTextWindowTopTile + ActiveTextWindowHeightTiles) << 0x03) - 0x01),
        0x0C);
    label = TrainingScenarioHeadingText;
    DrawCenteredClippedText(label, 0x00,
        ActiveTextWindowLeftTile << 0x03,
        ActiveTextWindowTopTile << 0x03,
        ActiveTextWindowWidthTiles << 0x03, 0x0F, 0x00);
    for (index = 0x00; index < 0x06; ++index)
        DrawTrainingScenarioOption(index, FALSE);

    DrawClippedAxisAlignedLine(0x00, 0x9F, 0x013F, 0x9F, 0x0D);
    DrawClippedAxisAlignedLine(0x57, 0x9F, 0x57, 0xC7, 0x0D);
    CopyClippedVideoRegion(0x00, 0x00, 0x00, 0x00,
        0x28, 0xC8, displayBuffer, videoBuffer);
    SetActiveRenderTarget(videoBuffer);
    ShowMouseCursorOverlay();
    SelectTextWindow(previousWindow);
}

// 2507:152E: draw one of the sixteen training deployment roster cells.
// Verified against 2507:152E-16E8.
void DrawTrainingDeploymentSlot(
    uint16_t slotIndex,
    uint16_t selectedState)
{
    uint16_t previousWindow = ActiveTextWindowIndex;
    uint16_t groupIndex;
    uint16_t groupSlot;
    uint16_t rosterIndex;
    uint16_t leftX;
    uint16_t topY;
    uint16_t width;
    const char *label;
    RevengeUnitRecord *unitRecord;

    SelectTextWindow(0x12);
    HideMouseCursorOverlay();
    topY = (uint16_t) ((ActiveTextWindowTopTile + 0x04) << 0x03);
    groupIndex = slotIndex / 0x05;
    groupSlot = slotIndex % 0x05;
    if (groupIndex == 0x03)
    {
        leftX = ActiveTextWindowLeftTile << 0x03;
        width = ActiveTextWindowWidthTiles << 0x03;
        /* Entry 03h of the table at DS:3380 is the dword after it,
		 * DS:338C: FriendlyEnemySuffixText's pointer. */
        label = FriendlyEnemySuffixText;
    }
    else
    {
        leftX = LanceEditorLayout.TrainingRosterLabelLeft[groupIndex];
        width = LanceEditorLayout.TrainingRosterLabelWidth[groupIndex];
        if (groupSlot == 0x04)
        {
            topY = (uint16_t) (topY +
                (ActiveFontLineHeight << 0x01));
            label = UnitRosterColumnLabelTextTable[groupIndex];
        }
        else
        {
            topY = (uint16_t) (topY +
                (groupSlot * 0x03 + 0x04) *
                    ActiveFontLineHeight);
            rosterIndex = groupIndex * 0x04 + groupSlot;
            unitRecord = &LiveScenarioUnits[rosterIndex];
            label = PilotSurnameTextTable[unitRecord->pilotId];
            DrawCenteredClippedText(label, 0x00, leftX, topY, width,
                (uint8_t) LanceEditorLayout.TrainingDeploymentTextColor[selectedState],
                (uint8_t) LanceEditorLayout.TrainingDeploymentBackgroundColor[selectedState]);
            topY = (uint16_t) (topY + ActiveFontLineHeight);
            label = UnitNameTable[unitRecord->unitTypeId];
        }
    }
    DrawCenteredClippedText(label, 0x00, leftX, topY, width,
        (uint8_t) LanceEditorLayout.TrainingDeploymentTextColor[selectedState],
        (uint8_t) LanceEditorLayout.TrainingDeploymentBackgroundColor[selectedState]);
    ShowMouseCursorOverlay();
    SelectTextWindow(previousWindow);
}

// 2507:0C70: draw the raised frame and centered label used by training choices.
// Verified against 2507:0C70-0E03.
void DrawTrainingChoiceButton(
    uint16_t leftX,
    uint16_t topY,
    const char *label,
    uint16_t width)
{
    uint16_t lineHeight = ActiveFontLineHeight;

    HideMouseCursorOverlay();
    FillVideoRectangle((int16_t) (leftX - 0x02),
        (int16_t) (topY - 0x02),
        (int16_t) (leftX + width + 0x02),
        (int16_t) (topY + lineHeight + 0x01), 0x0E);
    DrawCenteredClippedText(label, 0x00, leftX, topY, width, 0x0F, 0x00);
    DrawClippedAxisAlignedLine((int16_t) (leftX - 0x04),
        (int16_t) (topY - 0x03), (int16_t) (leftX + width + 0x02),
        (int16_t) (topY - 0x03), 0x0F);
    DrawClippedAxisAlignedLine((int16_t) (leftX - 0x03),
        (int16_t) (topY - 0x02), (int16_t) (leftX + width + 0x01),
        (int16_t) (topY - 0x02), 0x0F);
    DrawClippedAxisAlignedLine((int16_t) (leftX - 0x04),
        (int16_t) (topY - 0x02), (int16_t) (leftX - 0x04),
        (int16_t) (topY + lineHeight + 0x01), 0x0F);
    DrawClippedAxisAlignedLine((int16_t) (leftX - 0x03),
        (int16_t) (topY - 0x01), (int16_t) (leftX - 0x03),
        (int16_t) (topY + lineHeight), 0x0F);
    DrawClippedAxisAlignedLine((int16_t) (leftX + width + 0x02),
        (int16_t) (topY + lineHeight + 0x02),
        (int16_t) (leftX + width + 0x02),
        (int16_t) (topY - 0x02), 0x0D);
    DrawClippedAxisAlignedLine((int16_t) (leftX + width + 0x01),
        (int16_t) (topY + lineHeight + 0x01),
        (int16_t) (leftX + width + 0x01),
        (int16_t) (topY - 0x01), 0x0D);
    DrawClippedAxisAlignedLine((int16_t) (leftX - 0x02),
        (int16_t) (topY + lineHeight + 0x01),
        (int16_t) (leftX + width + 0x01),
        (int16_t) (topY + lineHeight + 0x01), 0x0D);
    DrawClippedAxisAlignedLine((int16_t) (leftX - 0x01),
        (int16_t) (topY + lineHeight + 0x02),
        (int16_t) (leftX + width + 0x02),
        (int16_t) (topY + lineHeight + 0x02), 0x0D);
    ShowMouseCursorOverlay();
}

// 2507:0AB1: draw or toggle one of twelve training unit choices.
// Verified against 2507:0AB1-0BC2.
void DrawTrainingUnitChoice(uint16_t choiceIndex, uint16_t selectedState)
{
    uint16_t row = choiceIndex / 0x04;
    uint16_t column = choiceIndex % 0x04;
    uint16_t unitIndex = (uint16_t) (UnitEditorAllegiance *
            0x0C +
        choiceIndex);
    uint16_t leftX = LanceEditorLayout.TrainingDeploymentSlotLeft[row];
    uint16_t topY = (uint16_t) (((ActiveTextWindowTopTile + 0x07) << 0x03) +
        (ActiveFontLineHeight * (column * 0x03 + 0x05)) - 0x03);
    RevengeUnitRecord *unitRecord = &LiveScenarioUnits[unitIndex];
    const char *label;

    HideMouseCursorOverlay();
    if (selectedState != FALSE)
    {
        if (unitRecord->unitTypeId == REVENGE_UNIT_TYPE_NONE)
            label = UnknownUnitLabel;
        else
            label = UnitNameTable[unitRecord->unitTypeId];
        DrawTrainingChoiceButton(leftX, topY, label, 0x4E);
    }
    else
    {
        XorVideoRectangle((int16_t) leftX, (int16_t) topY,
            (int16_t) (leftX + 0x4D),
            (int16_t) (topY + ActiveFontLineHeight - 0x01),
            0x01);
    }
    ShowMouseCursorOverlay();
}

// 2507:0BC3: draw or toggle one of four training control choices.
// Verified against 2507:0BC3-0C6F.
void DrawTrainingControlChoice(uint16_t choiceIndex, uint16_t selectedState)
{
    uint16_t group = choiceIndex / 0x02;
    uint16_t row = choiceIndex % 0x02;
    uint16_t leftX = LanceEditorLayout.TrainingControlChoiceColumn[group];
    uint16_t topY = (uint16_t) (((ActiveTextWindowTopTile + 0x07) << 0x03) +
        (ActiveFontLineHeight * (row * 0x03 + 0x13)) - 0x03);
    const char *label = TrainingControlLabelTextTable[choiceIndex];

    HideMouseCursorOverlay();
    if (selectedState != FALSE)
        DrawTrainingChoiceButton(leftX, topY, label, 0x5A);
    else
        XorVideoRectangle((int16_t) leftX, (int16_t) topY,
            (int16_t) (leftX + 0x59),
            (int16_t) (topY + ActiveFontLineHeight - 0x01),
            0x01);
    ShowMouseCursorOverlay();
}

// 2507:0794: compose the training unit-selection screen.
// Verified against 2507:0794-0AB0.
void DrawTrainingDeploymentScreen(void)
{
    uint16_t index;
    uint16_t leftX;
    uint16_t topY;
    uint16_t width;
    uint16_t gridTopY;
    uint16_t gridX;
    uint16_t groupIndex;
    uint16_t rowIndex;
    uint16_t halfLineOffset;
    uint16_t fullLineOffset;
    const char *label;
    uint8_t *displayBuffer = PrimaryScreenBuffer;
    uint8_t *videoBuffer = VideoFramebuffer;

    HideMouseCursorOverlay();
    SetActiveRenderTarget(displayBuffer);
    SelectAndClearTextWindow(0x00);
    leftX = ActiveTextWindowLeftTile << 0x03;
    topY = ActiveTextWindowTopTile << 0x03;
    width = ActiveTextWindowWidthTiles << 0x03;
    for (index = 0x00; index < 0x04; ++index)
    {
        label = TrainingDeploymentLabelTextTable[index];
        DrawCenteredClippedText(label, 0x00, leftX, topY, width,
            0x0F, 0x00);
        topY = (uint16_t) (topY +
            (ActiveFontLineHeight << 0x01));
    }
    topY = (uint16_t) ((ActiveTextWindowTopTile + 0x07) << 0x03);
    UnitEditorAllegiance = 0x00;
    DrawUnitAllegianceHeader(TRUE);
    gridTopY = (uint16_t) (topY +
        ActiveFontLineHeight);
    halfLineOffset = (uint16_t) (((uint8_t) ActiveFontLineHeight >> 0x01) - 0x02);
    fullLineOffset = (uint16_t) (ActiveFontLineHeight - 0x02);
    DrawClippedAxisAlignedLine(
        (int16_t) LanceEditorLayout.TrainingUnitGroupLeft[0x00],
        (int16_t) (gridTopY + halfLineOffset),
        (int16_t) (LanceEditorLayout.TrainingUnitGroupLeft[0x02] + 0x01),
        (int16_t) (gridTopY + halfLineOffset), 0x0D);
    DrawClippedAxisAlignedLine(
        (int16_t) LanceEditorLayout.TrainingUnitGroupLeft[0x00],
        (int16_t) (gridTopY + halfLineOffset + 0x01),
        (int16_t) (LanceEditorLayout.TrainingUnitGroupLeft[0x02] + 0x01),
        (int16_t) (gridTopY + halfLineOffset + 0x01), 0x0D);
    for (groupIndex = 0x00; groupIndex < 0x03; ++groupIndex)
    {
        gridX = LanceEditorLayout.TrainingUnitGroupLabelLeft[groupIndex];
        width = LanceEditorLayout.TrainingUnitGroupLabelWidth[groupIndex];
        label = UnitRosterColumnLabelTextTable[groupIndex];
        DrawCenteredClippedText(label, 0x00, gridX,
            (uint16_t) (gridTopY +
                ActiveFontLineHeight),
            width, 0x0E, 0x0C);
        gridX = LanceEditorLayout.TrainingUnitGroupLeft[groupIndex];
        topY = gridTopY;
        for (rowIndex = 0x00; rowIndex < 0x05; ++rowIndex)
        {
            uint16_t lineTopY = topY;
            if (rowIndex == 0x00 && groupIndex != 0x01)
                lineTopY = (uint16_t) (topY + halfLineOffset);
            DrawClippedAxisAlignedLine((int16_t) gridX,
                (int16_t) lineTopY, (int16_t) gridX,
                (int16_t) (topY + fullLineOffset), 0x0D);
            DrawClippedAxisAlignedLine((int16_t) (gridX + 0x01),
                (int16_t) lineTopY, (int16_t) (gridX + 0x01),
                (int16_t) (topY + fullLineOffset), 0x0D);
            topY = (uint16_t) (topY +
                (ActiveFontLineHeight *
                    (rowIndex == 0x00 ? 0x02 : 0x03)));
        }
    }
    for (index = 0x00; index < 0x0C; ++index)
        DrawTrainingUnitChoice(index, TRUE);
    for (index = 0x00; index < 0x04; ++index)
        DrawTrainingControlChoice(index, TRUE);
    CopyClippedVideoRegion(0x00, 0x00, 0x00, 0x00,
        0x28, 0xC8, displayBuffer, videoBuffer);
    SetActiveRenderTarget(videoBuffer);
    ShowMouseCursorOverlay();
}

// 2507:0057: configure the two twelve-unit training forces.
// Verified against 2507:0057-0500.
int16_t PrepareTrainingDeployment(void)
{
    uint16_t previousWindow;
    uint16_t selectedChoice = 0x00;
    uint16_t previousChoice = 0x00;
    uint16_t row;
    uint16_t column;
    uint16_t activated;
    uint16_t finished = FALSE;
    uint16_t result = FALSE;
    uint16_t inputCode;
    uint16_t index;
    uint16_t mouseButtonState;
    uint16_t mouseButtonEvent;
    int16_t mouseX;
    int16_t mouseY;
    int16_t leftX;
    int16_t topY;
    int16_t width;
    int16_t height;
    RevengeUnitRecord *unitRecord;
    int16_t unitType;

    ReplayScenarioRequested = FALSE;
    SceneTransitionAbortFlag = FALSE;
    previousWindow = ActiveTextWindowIndex;
    DrawTrainingDeploymentScreen();
    DrawTrainingUnitChoice(selectedChoice, FALSE);
    FlushPendingInput();
    while (finished == FALSE)
    {
        activated = FALSE;
        if (ServiceMusicAndCheckKeyboardInput() != FALSE)
        {
            inputCode = (uint16_t) NormalizeNavigationKeyCode(
                ReadTranslatedKeypress());
            row = selectedChoice / 0x04;
            column = selectedChoice % 0x04;
            if ((int16_t) inputCode == TranslatedExtendedKey(KEY_UP))
            {
                if (row == 0x03)
                {
                    if (column == 0x00)
                        selectedChoice = 0x03;
                    else if (column == 0x02)
                        selectedChoice = 0x0B;
                    else
                        --selectedChoice;
                }
                else if (column == 0x00)
                    selectedChoice = row == 0x02 ? 0x0F : 0x0D;
                else
                    --selectedChoice;
            }
            else if ((int16_t) inputCode == TranslatedExtendedKey(KEY_DOWN))
            {
                if (row == 0x03)
                {
                    if (column == 0x01)
                        selectedChoice = 0x00;
                    else if (column == 0x03)
                        selectedChoice = 0x08;
                    else
                        ++selectedChoice;
                }
                else if (column == 0x03)
                    selectedChoice = row == 0x02 ? 0x0E : 0x0C;
                else
                    ++selectedChoice;
            }
            else if ((int16_t) inputCode == TranslatedExtendedKey(KEY_LEFT) ||
                (int16_t) inputCode == TranslatedExtendedKey(KEY_RIGHT))
            {
                if (row == 0x03)
                    selectedChoice = column < 0x02 ? (uint16_t) (selectedChoice + 0x02) : (uint16_t) (selectedChoice - 0x02);
                else if ((int16_t) inputCode == TranslatedExtendedKey(KEY_LEFT))
                    selectedChoice = row == 0x00 ? (uint16_t) (selectedChoice + 0x08) : (uint16_t) (selectedChoice - 0x04);
                else
                    selectedChoice = row == 0x02 ? (uint16_t) (selectedChoice - 0x08) : (uint16_t) (selectedChoice + 0x04);
            }
            else if (inputCode == KEY_CARRIAGE_RETURN || inputCode == KEY_SPACE)
                activated = TRUE;
            else if (inputCode == KEY_ESCAPE)
            {
                selectedChoice = 0x0F;
                activated = TRUE;
            }
        }
        else if (MouseInputDisabled == FALSE)
        {
            mouseX = MouseCursorRequestedX;
            mouseY = (int16_t) MouseY;
            mouseButtonState = MouseButtonState;
            mouseButtonEvent = PendingMouseButtonEvent;
            PendingMouseButtonEvent = FALSE;
            for (index = 0x00; index < 0x0C; ++index)
            {
                leftX = (int16_t) LanceEditorLayout.TrainingUnitChoiceLeft[index];
                topY = (int16_t) LanceEditorLayout.TrainingUnitChoiceTop[index];
                if (mouseX >= leftX && mouseX < leftX + 0x4E &&
                    mouseY >= topY && mouseY < topY + ActiveFontLineHeight)
                {
                    selectedChoice = index;
                    if (mouseButtonEvent != FALSE &&
                        (mouseButtonState & MOUSE_BUTTON_LEFT) != 0x00)
                        activated = TRUE;
                }
            }
            for (index = 0x00; index < 0x04; ++index)
            {
                leftX = (int16_t) LanceEditorLayout.TrainingControlChoiceLeft[index];
                topY = (int16_t) LanceEditorLayout.TrainingControlChoiceTop[index];
                width = 0x5A;
                height = ActiveFontLineHeight;
                if (mouseX >= leftX && mouseX < leftX + width &&
                    mouseY >= topY && mouseY < topY + height)
                {
                    selectedChoice = (uint16_t) (index + 0x0C);
                    if (mouseButtonEvent != FALSE &&
                        (mouseButtonState & MOUSE_BUTTON_LEFT) != 0x00)
                        activated = TRUE;
                }
            }
        }

        if (selectedChoice != previousChoice)
        {
            if (previousChoice < 0x0C)
                DrawTrainingUnitChoice(previousChoice, FALSE);
            else
                DrawTrainingControlChoice(
                    (uint16_t) (previousChoice - 0x0C), FALSE);
            if (selectedChoice < 0x0C)
                DrawTrainingUnitChoice(selectedChoice, FALSE);
            else
                DrawTrainingControlChoice(
                    (uint16_t) (selectedChoice - 0x0C), FALSE);
            previousChoice = selectedChoice;
        }
        if (activated == FALSE)
            continue;

        if (selectedChoice == 0x0C || selectedChoice == 0x0D)
        {
            DrawUnitAllegianceHeader(FALSE);
            UnitEditorAllegiance = selectedChoice == 0x0C ? 0x00 : 0x01;
            DrawUnitAllegianceHeader(TRUE);
            for (index = 0x00; index < 0x0C; ++index)
                DrawTrainingUnitChoice(index, TRUE);
            continue;
        }
        if (selectedChoice == 0x0E)
        {
            result = RunGameOptionsMenu(FALSE);
            if (result != 0x04)
                result = TRUE;
            else
            {
                result = FALSE;
                finished = TRUE;
            }
            continue;
        }
        if (selectedChoice == 0x0F)
        {
            if (LiveScenarioUnits[0x00].unitTypeId != REVENGE_UNIT_TYPE_NONE &&
                LiveScenarioUnits[0x0C].unitTypeId != REVENGE_UNIT_TYPE_NONE)
            {
                result = TRUE;
                finished = TRUE;
            }
            else
            {
                uint16_t previousTextMode = TextOutputMode;
                TextOutputMode = 0x03;
                OpenModalTextWindow(0x0E, TRUE);
                HideMouseCursorOverlay();
                RenderFormattedText(TrainingUnitSelectionPrompt);
                ShowMouseCursorOverlay();
                WaitForDismissInput();
                RestoreModalTextWindow();
                TextOutputMode = previousTextMode;
            }
            continue;
        }

        unitType = SelectUnitClass();
        unitRecord = &LiveScenarioUnits[UnitEditorAllegiance * 0x0C +
            selectedChoice];
        unitRecord->unitTypeId = (uint8_t) unitType;
        DrawTrainingUnitChoice(selectedChoice, TRUE);
        DrawTrainingUnitChoice(selectedChoice, FALSE);
    }

    for (index = 0x00; index < 0x18; ++index)
    {
        unitRecord = &LiveScenarioUnits[index];
        if (unitRecord->unitTypeId == REVENGE_UNIT_TYPE_NONE)
            *unitRecord = BlankUnitRecord;
        else
        {
            LoadUnitTemplateRecord(unitRecord->unitTypeId);
            EstimatedUnitRecord.deploymentState = 0x01;
            *unitRecord = EstimatedUnitRecord;
            if (index < 0x0C)
                unitRecord->pilotId = 0x3A;
        }
    }
    SelectTextWindow(previousWindow);
    return (int16_t) result;
}

// 2507:0501: select one of six unit classes, then a type from that class.
// Returns the selected MECHTYPE.DAT type ID, or -1 for Cancel.
// Verified against 2507:0501-0624.
int16_t SelectUnitClass(void)
{
    TextWindowContext *menuWindow =
        &TextWindowContexts[UNIT_SELECTION_TEXT_WINDOW_INDEX];
    OriginalUiRegionRecord *menuRegion =
        &UiRegionTable[UNIT_SELECTION_UI_REGION_INDEX];
    uint16_t menuHeightTiles =
        ((ActiveFontLineHeight * UNIT_CLASS_MENU_ENTRY_COUNT) +
            (TEXT_WINDOW_TILE_SIZE_PIXELS - 0x01)) >>
        0x03;
    menuWindow->heightTiles = menuHeightTiles;
    menuWindow->topTile =
        (VIDEO_SCREEN_HEIGHT / TEXT_WINDOW_TILE_SIZE_PIXELS - menuHeightTiles) >> 0x01;
    OpenModalTextWindow(UNIT_SELECTION_TEXT_WINDOW_INDEX, TRUE);
    HideMouseCursorOverlay();
    uint16_t textWidth = menuWindow->widthTiles * TEXT_WINDOW_TILE_SIZE_PIXELS;
    uint16_t textLeft = menuWindow->leftTile * TEXT_WINDOW_TILE_SIZE_PIXELS;
    uint16_t textTop = menuWindow->topTile * TEXT_WINDOW_TILE_SIZE_PIXELS;
    uint16_t classIndex;
    for (classIndex = 0x00; classIndex < UNIT_CLASS_MENU_ENTRY_COUNT; ++classIndex)
    {
        const char *label = UnitClassLabelTextTable[classIndex];
        DrawCenteredClippedText(
            label,
            0x00, textLeft, textTop, textWidth,
            (uint8_t) ActiveTextForegroundColor,
            (uint8_t) ActiveTextBackgroundColor);
        textTop += ActiveFontLineHeight;
    }
    ShowMouseCursorOverlay();
    menuRegion->rowCount = UNIT_CLASS_MENU_ENTRY_COUNT;
    menuRegion->widthColumns = UNIT_SELECTION_TEXT_WINDOW_INDEX;
    FlushPendingInput();
    int16_t selectedMenuEntry;
    do
    {
        selectedMenuEntry =
            RunSingleUiRegionSelection(UNIT_SELECTION_UI_REGION_INDEX);
        if (selectedMenuEntry == KEY_ESCAPE)
            ToggleActiveUiRegionHighlight();
    } while (selectedMenuEntry == KEY_ESCAPE);
    RestoreModalTextWindow();

    if (selectedMenuEntry == UNIT_CLASS_MENU_CANCEL_INDEX)
        return -0x01;
    if (selectedMenuEntry >= 0x00 && selectedMenuEntry < UNIT_CLASS_COUNT)
    {
        const uint8_t *unitTypeList = UnitClassTypeLists[selectedMenuEntry];
        return SelectUnitTypeFromList(
            unitTypeList);
    }
    return -0x01;
}

// 2507:0647-065E: the list is the Omni one when its far address is DS:32D7,
// the sixth class's list. The lists are the rules' (UnitClassTypeLists), so
// that is the sixth list wherever the rules put it.
bool IsOmniUnitTypeList(const uint8_t *unitTypeIdList)
{
    return unitTypeIdList == UnitClassTypeLists[UNIT_CLASS_OMNI_INDEX];
}

// 2507:0625: int16 SelectUnitTypeFromList(const byte far *unitTypeIdList)
// Called from:
//      SelectUnitClass
// Verified against 2507:0625-0793.
int16_t SelectUnitTypeFromList(const uint8_t *unitTypeIdList)
{
    // The FFh-terminated input contains MECHTYPE.DAT / UnitNameTable
    // type IDs. The Omni list ends with Elementals, which are offered only
    // while choosing an Enemy unit. Every displayed list ends with Cancel.
    uint16_t unitTypeCount = 0x00;
    while (unitTypeIdList[unitTypeCount] != 0xFF)
        ++unitTypeCount;

    bool isOmniUnitTypeList = IsOmniUnitTypeList(unitTypeIdList);
    uint16_t selectableUnitTypeCount = unitTypeCount;
    if (isOmniUnitTypeList &&
        UnitEditorAllegiance != UNIT_ALLEGIANCE_ENEMY)
        --selectableUnitTypeCount;
    uint16_t menuEntryCount = selectableUnitTypeCount + 0x01;

    TextWindowContext *menuWindow =
        &TextWindowContexts[UNIT_SELECTION_TEXT_WINDOW_INDEX];
    OriginalUiRegionRecord *menuRegion =
        &UiRegionTable[UNIT_SELECTION_UI_REGION_INDEX];
    int16_t menuHeightTiles = (int16_t) (uint16_t) ((uint16_t) (ActiveFontLineHeight) *menuEntryCount + (TEXT_WINDOW_TILE_SIZE_PIXELS - 0x01));
    menuHeightTiles = (int16_t) (((uint16_t) menuHeightTiles >> 0x01) |
        ((uint16_t) menuHeightTiles & 0x8000));
    menuHeightTiles = (int16_t) (((uint16_t) menuHeightTiles >> 0x01) |
        ((uint16_t) menuHeightTiles & 0x8000));
    menuHeightTiles = (int16_t) (((uint16_t) menuHeightTiles >> 0x01) |
        ((uint16_t) menuHeightTiles & 0x8000));
    menuWindow->heightTiles = menuHeightTiles;
    menuWindow->topTile = (uint16_t) ((int16_t) (VIDEO_SCREEN_HEIGHT / TEXT_WINDOW_TILE_SIZE_PIXELS -
                                          menuHeightTiles) >>
        0x01);
    OpenModalTextWindow(UNIT_SELECTION_TEXT_WINDOW_INDEX, TRUE);
    HideMouseCursorOverlay();
    uint16_t textWidth = menuWindow->widthTiles * TEXT_WINDOW_TILE_SIZE_PIXELS;
    uint16_t textLeft = menuWindow->leftTile * TEXT_WINDOW_TILE_SIZE_PIXELS;
    uint16_t textTop = menuWindow->topTile * TEXT_WINDOW_TILE_SIZE_PIXELS;
    uint16_t menuEntryIndex;
    for (menuEntryIndex = 0x00; menuEntryIndex < menuEntryCount; ++menuEntryIndex)
    {
        const char *label;
        if (menuEntryIndex < selectableUnitTypeCount)
            label = UnitNameTable[unitTypeIdList[menuEntryIndex]];
        else
            label = STORY_TEXT(SELECT_UNIT_TYPE_FROM_LIST_0); // TEXT DS:437E
        DrawCenteredClippedText(label, 0x00, textLeft, textTop, textWidth,
            (uint8_t) ActiveTextForegroundColor,
            (uint8_t) ActiveTextBackgroundColor);
        textTop += ActiveFontLineHeight;
    }
    ShowMouseCursorOverlay();
    menuRegion->rowCount = menuEntryCount;
    menuRegion->widthColumns = UNIT_SELECTION_TEXT_WINDOW_INDEX;
    FlushPendingInput();
    int16_t selectedEntry;
    do
    {
        selectedEntry =
            RunSingleUiRegionSelection(UNIT_SELECTION_UI_REGION_INDEX);
        if (selectedEntry == KEY_ESCAPE)
            ToggleActiveUiRegionHighlight();
    } while (selectedEntry == KEY_ESCAPE);
    RestoreModalTextWindow();
    if ((int16_t) selectableUnitTypeCount <= selectedEntry)
        return -0x01;
    return unitTypeIdList[selectedEntry];
}

// 2507:0A20: draw the active Friendly/Enemy heading for a unit-panel row.
// VERIFIED against ASM 2507:0A20-0AB0.
void DrawUnitAllegianceHeader(uint16_t headerColorIndex)
{
    char headerText[0x14];
    int16_t allegianceIndex;
    uint16_t textLeft;
    uint16_t textTop;
    uint16_t textWidth;
    const char *allegianceText;
    const char *suffixText;

    textLeft = (uint16_t) (ActiveTextWindowLeftTile << 0x03);
    textTop = (uint16_t) ((ActiveTextWindowTopTile + 0x07) << 0x03);
    textWidth = (uint16_t) (ActiveTextWindowWidthTiles << 0x03);
    allegianceIndex = UnitEditorAllegiance;
    allegianceText = UnitAllegianceLabelTextTable[allegianceIndex];
    suffixText = FriendlyEnemySuffixText;
    strcpy(headerText, allegianceText);
    strcat(headerText, suffixText);
    HideMouseCursorOverlay();
    DrawCenteredClippedText(
        headerText,
        0x0000,
        textLeft,
        textTop,
        textWidth,
        LanceEditorLayout.AllegianceHeaderColor[headerColorIndex],
        0x0C);
    ShowMouseCursorOverlay();
}
