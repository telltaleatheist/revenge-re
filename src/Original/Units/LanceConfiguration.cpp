// 2507: lance configuration
// LanceConfiguration.cpp
// Reconstructed from the original procedures in segment 2507.

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "LanceEditorInternal.h"
#include "Audio/Audio.h"
#include "Campaign/Campaign.h"
#include "Data/RuntimeData.h"
#include "Data/StartupData.h"
#include "Data/TacticalData.h"
#include "Graphics/Graphics.h"
#include "Input/Input.h"
#include "Runtime/Runtime.h"
#include "Text/Text.h"
#include "revenge/tactical_panel_data.h"
#include "revenge/text.h"

#include <string.h>

// 2507:1848: prepare and run the combined unit/pilot lance editor.
// VERIFIED against ASM 2507:1848-18AF.
void RunLanceConfiguration(
    uint16_t selectionFamily,
    uint16_t scenarioVariantIndex)
{
    if (ReplayScenarioRequested != FALSE)
        return;

    SceneTransitionAbortFlag = FALSE;
    LanceEditorFirstBriefingPending = 0x0001;
    LanceEditorBriefingPresented = 0x0000;
    SnapshotLateCampaignLanceUnits();
    PrepareLanceSelectionRecords(
        selectionFamily + 0x0003,
        0x0001,
        FALSE);
    PreparePilotSelectionRecords(
        selectionFamily,
        0x0001,
        FALSE);
    LanceEditorReloadGraphics = 0x0001;
    do
    {
        /* The editor returns nonzero while another pass is required. */
    } while (RunLanceConfigurationEditor(
                 selectionFamily, scenarioVariantIndex) != FALSE);
    SceneTransitionAbortFlag = FALSE;
}

// 2507:18B0: run the four-section lance configuration menu.
// VERIFIED against ASM 2507:18B0-1EBD.
int16_t RunLanceConfigurationEditor(
    uint16_t selectionFamily,
    uint16_t scenarioVariantIndex)
{
    uint16_t repeatEditor;
    uint16_t selectedSection;
    uint16_t previousSection;
    uint16_t selectionActivated;
    uint16_t highlightPhase;
    uint16_t sectionIndex;
    uint16_t mouseButtonState;
    uint16_t mouseButtonEvent;
    int16_t mouseX;
    int16_t mouseY;
    int16_t leftX;
    int16_t topY;
    int16_t width;
    int16_t heightMinusOne;
    int16_t inputCode;
    uint16_t configurationIndex;
    const char *sectionLabel;
    uint8_t *primaryFramebuffer;
    uint8_t *displayBuffer;
    uint8_t *sectionSprite;

    repeatEditor = TRUE;
    configurationIndex = selectionFamily + scenarioVariantIndex;
    primaryFramebuffer = VideoFramebuffer;
    displayBuffer = PrimaryScreenBuffer;

    if (LanceEditorReloadGraphics != 0x0000)
    {
        LoadGraphicsResourceToBuffers(
            0x0011,
            SecondaryScreenBuffer,
            displayBuffer);
        LoadLanceConfigurationGraphicsBuffers();
        LoadAndPresentScreen(0x0010);
        primaryFramebuffer = VideoFramebuffer;
        displayBuffer = PrimaryScreenBuffer;
        HideMouseCursorOverlay();
        CopyClippedVideoRegion(
            0x0000, 0x0000, 0x0000, 0x0000, 0x0028, 0x00C8,
            primaryFramebuffer, displayBuffer);
    }
    else
    {
        HideMouseCursorOverlay();
    }

    selectedSection = 0x0000;
    previousSection = selectedSection;
    sectionSprite = UnitEditorGraphicsBuffer(0x0013 + selectedSection);
    DrawClippedSprite(
        primaryFramebuffer,
        sectionSprite,
        (int16_t) (LanceEditorLayout.UnitEditorSectionLeft[selectedSection] << 0x03),
        (int16_t) LanceEditorLayout.UnitEditorSectionTop[selectedSection],
        0x0000);
    sectionLabel = UnitEditorSectionLabelTextTable[selectedSection];
    DrawTextString(
        sectionLabel,
        LanceEditorLayout.UnitEditorSectionLeft[selectedSection + 0x0004] << 0x03,
        LanceEditorLayout.UnitEditorSectionTop[selectedSection + 0x0004],
        0x0F,
        0x00);
    ShowMouseCursorOverlay();
    FlushPendingInput();
    highlightPhase = 0x0000;
    selectionActivated = FALSE;
    inputCode = 0x0000;

    while (selectionActivated == FALSE)
    {
        if (ServiceMusicAndCheckKeyboardInput() != 0x0000)
        {
            inputCode = NormalizeNavigationKeyCode(
                ReadTranslatedKeypress());
            if (inputCode == TranslatedExtendedKey(KEY_UP))
            {
                --selectedSection;
            }
            else if (inputCode == TranslatedExtendedKey(KEY_DOWN))
            {
                ++selectedSection;
            }
            else if (inputCode == KEY_CARRIAGERETURN || inputCode == KEY_SPACE)
            {
                if (selectedSection == 0x0003)
                {
                    if (ValidatePilotConfiguration(
                            configurationIndex + 0x0001) == FALSE)
                    {
                        selectedSection = 0x0001;
                    }
                    else if (ValidateLanceConfiguration(
                                 configurationIndex + 0x0001) == FALSE)
                    {
                        selectedSection = 0x0002;
                    }
                    else
                    {
                        selectionActivated = TRUE;
                    }
                }
                else
                {
                    selectionActivated = TRUE;
                }
            }
            else if (inputCode == KEY_ESC)
            {
                if (ValidatePilotConfiguration(
                        configurationIndex + 0x0001) == FALSE)
                {
                    selectedSection = 0x0001;
                }
                else if (ValidateLanceConfiguration(
                             configurationIndex + 0x0001) == FALSE)
                {
                    selectedSection = 0x0002;
                }
                else
                {
                    selectedSection = 0x0003;
                    selectionActivated = TRUE;
                    repeatEditor = FALSE;
                }
            }

            if ((int16_t) selectedSection < 0x0000)
                selectedSection = 0x0003;
            if ((int16_t) selectedSection > 0x0003)
                selectedSection = 0x0000;
        }
        else if (MouseInputDisabled == 0x0000)
        {
            mouseX = (int16_t) MouseCursorRequestedX;
            mouseY = (int16_t) MouseY;
            mouseButtonState = MouseButtonState;
            mouseButtonEvent = PendingMouseButtonEvent;
            PendingMouseButtonEvent = 0x0000;

            if (mouseButtonEvent != 0x0000 &&
                (mouseButtonState & MOUSE_BUTTON_RIGHT) != 0x0000)
            {
                if (ValidatePilotConfiguration(
                        configurationIndex + 0x0001) == FALSE)
                {
                    selectedSection = 0x0001;
                }
                else if (ValidateLanceConfiguration(
                             configurationIndex + 0x0001) == FALSE)
                {
                    selectedSection = 0x0002;
                }
                else
                {
                    selectedSection = 0x0003;
                    selectionActivated = TRUE;
                    repeatEditor = FALSE;
                }
            }
            else
            {
                for (sectionIndex = 0x0000; sectionIndex < 0x0004;
                    ++sectionIndex)
                {
                    leftX = (int16_t) (LanceEditorLayout.UnitEditorSectionLeft[sectionIndex] << 0x03);
                    topY = (int16_t) LanceEditorLayout.UnitEditorSectionTop[sectionIndex];
                    width = (int16_t) (LanceEditorLayout.UnitEditorSectionWidth[sectionIndex] << 0x03);
                    heightMinusOne = (int16_t) (LanceEditorLayout.UnitEditorSectionHeight[sectionIndex] - 0x0001);
                    if (mouseX >= leftX && mouseX < leftX + width &&
                        mouseY >= topY && mouseY < topY + heightMinusOne)
                    {
                        selectedSection = sectionIndex;
                        if (mouseButtonEvent != 0x0000 &&
                            (mouseButtonState & MOUSE_BUTTON_LEFT) != 0x0000)
                        {
                            if (selectedSection == 0x0003)
                            {
                                if (ValidatePilotConfiguration(
                                        configurationIndex + 0x0001) == FALSE)
                                    selectedSection = 0x0001;
                                else if (ValidateLanceConfiguration(
                                             configurationIndex + 0x0001) == FALSE)
                                    selectedSection = 0x0002;
                                else
                                    selectionActivated = TRUE;
                            }
                            else
                            {
                                selectionActivated = TRUE;
                            }
                        }
                    }
                }
            }
        }

        if (selectedSection != previousSection)
        {
            HideMouseCursorOverlay();
            CopyClippedVideoRegion(
                (int16_t) LanceEditorLayout.UnitEditorSectionLeft[previousSection],
                (int16_t) LanceEditorLayout.UnitEditorSectionTop[previousSection],
                (int16_t) LanceEditorLayout.UnitEditorSectionLeft[previousSection],
                (int16_t) LanceEditorLayout.UnitEditorSectionTop[previousSection],
                LanceEditorLayout.UnitEditorSectionWidth[previousSection],
                LanceEditorLayout.UnitEditorSectionHeight[previousSection],
                displayBuffer,
                primaryFramebuffer);
            sectionSprite = UnitEditorGraphicsBuffer(0x0013 + selectedSection);
            DrawClippedSprite(
                primaryFramebuffer,
                sectionSprite,
                (int16_t) (LanceEditorLayout.UnitEditorSectionLeft[selectedSection] << 0x03),
                (int16_t) LanceEditorLayout.UnitEditorSectionTop[selectedSection],
                0x0000);

            sectionIndex = previousSection + 0x0004;
            CopyClippedVideoRegion(
                (int16_t) LanceEditorLayout.UnitEditorSectionLeft[sectionIndex],
                (int16_t) LanceEditorLayout.UnitEditorSectionTop[sectionIndex],
                (int16_t) LanceEditorLayout.UnitEditorSectionLeft[sectionIndex],
                (int16_t) LanceEditorLayout.UnitEditorSectionTop[sectionIndex],
                LanceEditorLayout.UnitEditorSectionWidth[sectionIndex],
                LanceEditorLayout.UnitEditorSectionHeight[sectionIndex],
                displayBuffer,
                primaryFramebuffer);
            sectionLabel = UnitEditorSectionLabelTextTable[selectedSection];
            DrawTextString(
                sectionLabel,
                LanceEditorLayout.UnitEditorSectionLeft[selectedSection + 0x0004] << 0x03,
                LanceEditorLayout.UnitEditorSectionTop[selectedSection + 0x0004],
                0x0F,
                0x00);
            highlightPhase = 0x0000;
            previousSection = selectedSection;
            ShowMouseCursorOverlay();
        }

        if (selectionActivated != FALSE)
        {
            HideMouseCursorOverlay();
            sectionIndex = previousSection + 0x0004;
            CopyClippedVideoRegion(
                (int16_t) LanceEditorLayout.UnitEditorSectionLeft[sectionIndex],
                (int16_t) LanceEditorLayout.UnitEditorSectionTop[sectionIndex],
                (int16_t) LanceEditorLayout.UnitEditorSectionLeft[sectionIndex],
                (int16_t) LanceEditorLayout.UnitEditorSectionTop[sectionIndex],
                LanceEditorLayout.UnitEditorSectionWidth[sectionIndex],
                LanceEditorLayout.UnitEditorSectionHeight[sectionIndex],
                displayBuffer,
                primaryFramebuffer);
            previousSection = selectedSection;
            highlightPhase = 0x0000;
            ShowMouseCursorOverlay();
        }

        WaitVerticalRetraces(0x0002);
        HideMouseCursorOverlay();
        if (highlightPhase != 0x0000)
        {
            sectionSprite = UnitEditorGraphicsBuffer(0x0013 + selectedSection);
            DrawClippedSprite(
                primaryFramebuffer,
                sectionSprite,
                (int16_t) (LanceEditorLayout.UnitEditorSectionLeft[selectedSection] << 0x03),
                (int16_t) LanceEditorLayout.UnitEditorSectionTop[selectedSection],
                0x0000);
        }
        else
        {
            CopyClippedVideoRegion(
                (int16_t) LanceEditorLayout.UnitEditorSectionLeft[selectedSection],
                (int16_t) LanceEditorLayout.UnitEditorSectionTop[selectedSection],
                (int16_t) LanceEditorLayout.UnitEditorSectionLeft[selectedSection],
                (int16_t) LanceEditorLayout.UnitEditorSectionTop[selectedSection],
                LanceEditorLayout.UnitEditorSectionWidth[selectedSection],
                LanceEditorLayout.UnitEditorSectionHeight[selectedSection],
                displayBuffer,
                primaryFramebuffer);
        }
        ShowMouseCursorOverlay();
        highlightPhase ^= 0x0001;
    }

    if (inputCode == KEY_ESC || selectedSection == 0x0000 ||
        selectedSection == 0x0003 ||
        (selectedSection == 0x0001 && UnitEditorPilotCardCount != 0x0000) ||
        (selectedSection == 0x0002 && UnitEditorScenarioCardCount != 0x0000))
    {
        FreeGraphicsBufferRange(0x0013, 0x0016);
        LanceEditorReloadGraphics = 0x0001;
    }
    else
    {
        LanceEditorReloadGraphics = 0x0000;
    }

    if (inputCode == KEY_ESC)
    {
        if (LanceEditorBriefingPresented == 0x0000)
            ShowLanceConfigurationBriefing(configurationIndex);
        repeatEditor = FALSE;
    }
    else if (selectedSection == 0x0000)
    {
        ShowLanceConfigurationBriefing(configurationIndex);
    }
    else if (selectedSection == 0x0001)
    {
        if (UnitEditorPilotCardCount != 0x0000)
            RunPilotSelection(configurationIndex, 0x0000);
        else
            ShowNoSelectionOptions();
    }
    else if (selectedSection == 0x0002)
    {
        if (UnitEditorScenarioCardCount != 0x0000)
            PrepareLanceSelection(configurationIndex + 0x0003, 0x0000);
        else
            ShowNoSelectionOptions();
    }
    else
    {
        if (LanceEditorBriefingPresented == 0x0000)
            ShowLanceConfigurationBriefing(configurationIndex);
        repeatEditor = FALSE;
    }

    return (int16_t) repeatEditor;
}

// 2507:1EBE: show the unavailable-selection message and wait for dismissal.
// Verified against 2507:1EBE-1F04.
void ShowNoSelectionOptions(void)
{
    uint16_t savedTextOutputMode;

    savedTextOutputMode = TextOutputMode;
    TextOutputMode = 0x0001;
    OpenModalTextWindow(0x000E, TRUE);
    HideMouseCursorOverlay();
    RenderFormattedText(NoSelectionOptionsText);
    ShowMouseCursorOverlay();
    FlushPendingInput();
    WaitForKeyboardOrMouseInput();
    RestoreModalTextWindow();
    TextOutputMode = savedTextOutputMode;
}

// 2507:1F03: show the briefing associated with a lance configuration.
// VERIFIED against ASM 2507:1F03-2004, including the seven-entry jump table.
void ShowLanceConfigurationBriefing(uint16_t configurationIndex)
{
    switch (configurationIndex)
    {
    case 0x0000:
        PlayMusicResource(0x004C);
        if (LanceEditorFirstBriefingPending != 0x0000)
        {
            PresentStoryScreenAndMessage(0x001C, 0x0089);
            ShowStoryMessage(0x008A);
            ShowStoryMessage(0x008B);
            LanceEditorFirstBriefingPending = 0x0000;
        }
        else
        {
            PresentStoryScreenAndMessage(0x001C, 0x008B);
        }
        break;

    case 0x0001:
        PlayMusicResource(0x004C);
        PresentStoryScreenAndMessage(0x001C, 0x008F);
        break;

    case 0x0002:
        PlayMusicResource(0x004C);
        PresentStoryScreenAndMessage(0x001C, 0x0092);
        break;

    case 0x0003:
        PlayMusicResource(0x004A);
        PresentStoryScreenAndMessage(0x001C, 0x0094);
        break;

    case 0x0004:
    case 0x0005:
        PlayMusicResource(0x004A);
        PresentStoryScreenAndMessage(0x001C, 0x0098);
        break;

    case 0x0006:
        PlayMusicResource(0x004A);
        PresentStoryScreenAndMessage(0x001C, 0x009C);
        break;
    }

    LanceEditorBriefingPresented = 0x0001;
}

// 2507:2005: present one of two scenario-route choices and return its index.
// VERIFIED against ASM 2507:2005-26ED.
int16_t SelectScenarioVariant(uint16_t presentationMode)
{
    uint16_t selectedVariant;
    uint16_t previousVariant;
    uint16_t selectionComplete;
    uint16_t highlightPhase;
    uint16_t variantIndex;
    uint16_t variantTextBaseIndex;
    int16_t leftX;
    int16_t topY;
    int16_t width;
    int16_t height;
    int16_t mouseX;
    int16_t mouseY;
    uint16_t mouseButtonState;
    uint16_t mouseButtonEvent;
    int16_t inputCode;
    const char *label;
    uint8_t *primaryFramebuffer;
    uint8_t *displayBuffer;
    uint8_t *selectionSprite;

    SceneTransitionAbortFlag = FALSE;
    primaryFramebuffer = VideoFramebuffer;
    displayBuffer = PrimaryScreenBuffer;

    if (presentationMode != 0x0000)
    {
        LoadGraphicsResourceToBuffers(
            0x0013,
            SecondaryScreenBuffer,
            displayBuffer);
        LoadScenarioVariantGraphicsBuffers();
    }

    LoadAndPresentScreen(presentationMode + 0x0014);
    primaryFramebuffer = VideoFramebuffer;
    displayBuffer = PrimaryScreenBuffer;
    HideMouseCursorOverlay();
    FillVideoRectangle(0x0000, 0x00A8, 0x013F, 0x00AE, 0x000E);
    FillVideoRectangle(0x009F, 0x00A8, 0x00A0, 0x00C7, 0x000D);

    selectedVariant = 0x0000;
    previousVariant = selectedVariant;
    leftX = 0x0000;
    topY = 0x0000;
    width = 0x009F;
    height = 0x00A7;

    if (presentationMode == 0x0000)
    {
        DrawClippedRectangleOutline(
            (int16_t) LanceEditorLayout.ScenarioVariantTextLeft[selectedVariant],
            (int16_t) topY,
            (int16_t) (LanceEditorLayout.ScenarioVariantTextLeft[selectedVariant] + width),
            (int16_t) (topY + height),
            LanceEditorLayout.ScenarioVariantNormalColor);
        selectedVariant ^= 0x0001;
        DrawClippedRectangleOutline(
            (int16_t) LanceEditorLayout.ScenarioVariantTextLeft[selectedVariant],
            (int16_t) topY,
            (int16_t) (LanceEditorLayout.ScenarioVariantTextLeft[selectedVariant] + width),
            (int16_t) (topY + height),
            LanceEditorLayout.ScenarioVariantNormalColor);
    }
    else
    {
        for (selectedVariant = 0x0000; selectedVariant < 0x0002;
            ++selectedVariant)
        {
            CopyClippedVideoRegion(
                (int16_t) LanceEditorLayout.ScenarioVariantRegionLeft[selectedVariant],
                (int16_t) LanceEditorLayout.ScenarioVariantRegionTop[selectedVariant],
                (int16_t) LanceEditorLayout.ScenarioVariantRegionLeft[selectedVariant],
                (int16_t) LanceEditorLayout.ScenarioVariantRegionTop[selectedVariant],
                LanceEditorLayout.ScenarioVariantRegionWidth[selectedVariant],
                LanceEditorLayout.ScenarioVariantRegionHeight[selectedVariant],
                primaryFramebuffer,
                displayBuffer);
        }
        selectedVariant = 0x0000;
        selectionSprite = UnitEditorGraphicsBuffer(0x0011);
        DrawClippedSprite(
            primaryFramebuffer,
            selectionSprite,
            (int16_t) (LanceEditorLayout.ScenarioVariantRegionLeft[0x00] << 0x03),
            (int16_t) LanceEditorLayout.ScenarioVariantRegionTop[0x00],
            0x0000);
    }

    variantTextBaseIndex = presentationMode << 0x01;
    label = ScenarioVariantDescriptionTextTable[variantTextBaseIndex];
    DrawCenteredClippedText(
        label, 0x0000, 0x0000, 0x00A9, 0x00A0, 0x0F, 0x00);
    label = ScenarioVariantDescriptionTextTable[variantTextBaseIndex + 0x0001];
    DrawCenteredClippedText(
        label, 0x0000, 0x00A0, 0x00A9, 0x00A0, 0x0F, 0x00);

    highlightPhase = 0x0000;
    selectedVariant = 0x0000;
    if (presentationMode == 0x0000)
    {
        DrawUnitRosterScreen(selectedVariant, 0x000F);
    }
    else
    {
        for (variantIndex = 0x0000; variantIndex < 0x0004; ++variantIndex)
        {
            label = ScenarioRouteLabelTextTable[variantIndex];
            DrawCenteredClippedText(
                label,
                0x0000,
                LanceEditorLayout.ScenarioVariantTextLeft[variantIndex >> 0x01] + 0x0001,
                (variantIndex & 0x0001) == 0x0000 ? 0x00B0 : 0x00B8,
                (uint16_t) ((uint16_t) strlen(label) *
                    ActiveFontGlyphAdvanceX),
                0x0F,
                0x00);
        }
    }

    ShowMouseCursorOverlay();
    FlushPendingInput();
    selectionComplete = FALSE;
    while (selectionComplete == FALSE)
    {
        if (ServiceMusicAndCheckKeyboardInput() != 0x0000)
        {
            inputCode = NormalizeNavigationKeyCode(
                ReadTranslatedKeypress());
            if (inputCode == TranslatedExtendedKey(KEY_UP) ||
                inputCode == TranslatedExtendedKey(KEY_DOWN) ||
                inputCode == TranslatedExtendedKey(KEY_LEFT) ||
                inputCode == TranslatedExtendedKey(KEY_RIGHT))
            {
                selectedVariant ^= 0x0001;
            }
            else if (inputCode == KEY_CARRIAGERETURN || inputCode == KEY_SPACE)
            {
                selectionComplete = TRUE;
            }
        }
        else if (MouseInputDisabled == 0x0000)
        {
            mouseX = (int16_t) MouseCursorRequestedX;
            mouseY = (int16_t) MouseY;
            mouseButtonState = MouseButtonState;
            mouseButtonEvent = PendingMouseButtonEvent;
            PendingMouseButtonEvent = 0x0000;

            for (variantIndex = 0x0000; variantIndex < 0x0002; ++variantIndex)
            {
                if (presentationMode == 0x0000)
                {
                    leftX = LanceEditorLayout.ScenarioVariantTextLeft[variantIndex];
                    topY = 0x0000;
                }
                else
                {
                    leftX = 0x0000;
                    topY = LanceEditorLayout.ScenarioVariantTextTop[variantIndex];
                    width = 0x013F;
                    height = 0x0053;
                }

                if (mouseX >= leftX && mouseX <= leftX + width &&
                    mouseY >= topY && mouseY <= topY + height)
                {
                    selectedVariant = variantIndex;
                    if (mouseButtonEvent != 0x0000 &&
                        (mouseButtonState & MOUSE_BUTTON_LEFT) != 0x0000)
                        selectionComplete = TRUE;
                }
            }
        }

        if (selectedVariant != previousVariant)
        {
            HideMouseCursorOverlay();
            if (presentationMode == 0x0000)
            {
                DrawClippedRectangleOutline(
                    (int16_t) LanceEditorLayout.ScenarioVariantTextLeft[previousVariant],
                    (int16_t) topY,
                    (int16_t) (LanceEditorLayout.ScenarioVariantTextLeft[previousVariant] + width),
                    (int16_t) (topY + height),
                    LanceEditorLayout.ScenarioVariantNormalColor);
                DrawUnitRosterScreen(previousVariant, 0x000C);
                DrawClippedRectangleOutline(
                    (int16_t) LanceEditorLayout.ScenarioVariantTextLeft[selectedVariant],
                    (int16_t) topY,
                    (int16_t) (LanceEditorLayout.ScenarioVariantTextLeft[selectedVariant] + width),
                    (int16_t) (topY + height),
                    LanceEditorLayout.ScenarioVariantSelectedColor);
                DrawUnitRosterScreen(selectedVariant, 0x000F);
            }
            else
            {
                CopyClippedVideoRegion(
                    (int16_t) LanceEditorLayout.ScenarioVariantRegionLeft[previousVariant],
                    (int16_t) LanceEditorLayout.ScenarioVariantRegionTop[previousVariant],
                    (int16_t) LanceEditorLayout.ScenarioVariantRegionLeft[previousVariant],
                    (int16_t) LanceEditorLayout.ScenarioVariantRegionTop[previousVariant],
                    LanceEditorLayout.ScenarioVariantRegionWidth[previousVariant],
                    LanceEditorLayout.ScenarioVariantRegionHeight[previousVariant],
                    displayBuffer,
                    primaryFramebuffer);
                label = ScenarioVariantDescriptionTextTable[variantTextBaseIndex + previousVariant];
                DrawCenteredClippedText(
                    label, 0x0000,
                    LanceEditorLayout.ScenarioVariantTextLeft[previousVariant],
                    0x00A9, 0x00A0, 0x0F, 0x00);

                selectionSprite = UnitEditorGraphicsBuffer(
                    0x0011 + selectedVariant);
                DrawClippedSprite(
                    primaryFramebuffer,
                    selectionSprite,
                    (int16_t) (LanceEditorLayout.ScenarioVariantRegionLeft[selectedVariant] << 0x03),
                    (int16_t) LanceEditorLayout.ScenarioVariantRegionTop[selectedVariant],
                    0x0000);
                label = ScenarioVariantDescriptionTextTable[variantTextBaseIndex + selectedVariant];
                DrawCenteredClippedText(
                    label, 0x0000,
                    LanceEditorLayout.ScenarioVariantTextLeft[selectedVariant],
                    0x00A9, 0x00A0, 0x0F, 0x00);
            }

            highlightPhase = 0x0000;
            previousVariant = selectedVariant;
            ShowMouseCursorOverlay();
        }

        WaitVerticalRetraces(0x0005);
        if (selectionComplete != FALSE)
            highlightPhase = 0x0001;
        HideMouseCursorOverlay();

        if (presentationMode == 0x0000)
        {
            DrawClippedRectangleOutline(
                (int16_t) LanceEditorLayout.ScenarioVariantTextLeft[selectedVariant],
                (int16_t) topY,
                (int16_t) (LanceEditorLayout.ScenarioVariantTextLeft[selectedVariant] + width),
                (int16_t) (topY + height),
                highlightPhase == 0x0000 ? LanceEditorLayout.ScenarioVariantNormalColor : LanceEditorLayout.ScenarioVariantSelectedColor);
        }
        else if (highlightPhase != 0x0000)
        {
            selectionSprite = UnitEditorGraphicsBuffer(
                0x0011 + selectedVariant);
            DrawClippedSprite(
                primaryFramebuffer,
                selectionSprite,
                (int16_t) (LanceEditorLayout.ScenarioVariantRegionLeft[selectedVariant] << 0x03),
                (int16_t) LanceEditorLayout.ScenarioVariantRegionTop[selectedVariant],
                0x0000);
            label = ScenarioVariantDescriptionTextTable[variantTextBaseIndex + selectedVariant];
            DrawCenteredClippedText(
                label, 0x0000,
                LanceEditorLayout.ScenarioVariantTextLeft[selectedVariant],
                0x00A9, 0x00A0,
                (uint8_t) LanceEditorLayout.ScenarioVariantLabelColor[variantTextBaseIndex + selectedVariant],
                (uint8_t) 0x00);
        }
        else
        {
            CopyClippedVideoRegion(
                (int16_t) LanceEditorLayout.ScenarioVariantRegionLeft[selectedVariant],
                (int16_t) LanceEditorLayout.ScenarioVariantRegionTop[selectedVariant],
                (int16_t) LanceEditorLayout.ScenarioVariantRegionLeft[selectedVariant],
                (int16_t) LanceEditorLayout.ScenarioVariantRegionTop[selectedVariant],
                LanceEditorLayout.ScenarioVariantRegionWidth[selectedVariant],
                LanceEditorLayout.ScenarioVariantRegionHeight[selectedVariant],
                displayBuffer,
                primaryFramebuffer);
            label = ScenarioVariantDescriptionTextTable[variantTextBaseIndex + selectedVariant];
            DrawCenteredClippedText(
                label, 0x0000,
                LanceEditorLayout.ScenarioVariantTextLeft[selectedVariant],
                0x00A9, 0x00A0, 0x0F, 0x00);
        }

        ShowMouseCursorOverlay();
        highlightPhase ^= 0x0001;
    }

    if (presentationMode != 0x0000)
        FreeGraphicsBufferRange(0x0011, 0x0012);
    return (int16_t) selectedVariant;
}

// 2507:26EE: draw five pilot/unit roster labels in the normal or alternate
// assignment layout. VERIFIED against ASM 2507:26EE-2BD8.
void DrawUnitRosterScreen(uint16_t useAlternateAssignmentLayout, uint16_t textColor)
{
    char rosterLine[0x2A];
    uint8_t assignedUnitTypeIds[0x03];
    uint8_t assignedPilotIds[0x03];
    uint8_t fixedPilotIds[0x02];
    uint8_t fixedUnitTypeIds[0x02];
    uint16_t assignmentIndex;
    uint16_t leftX;
    uint16_t rightX;
    const char *pilotName;
    const char *unitName;
    const char *separator;

    for (assignmentIndex = 0x0000; assignmentIndex < 0x0003;
        ++assignmentIndex)
    {
        uint16_t liveUnitSlot =
            LanceAssignments[assignmentIndex].liveUnitSlot;
        assignedPilotIds[assignmentIndex] =
            LanceAssignments[assignmentIndex].pilotId;
        assignedUnitTypeIds[assignmentIndex] =
            LiveScenarioUnits[liveUnitSlot].unitTypeId;
    }
    fixedPilotIds[0x00] = FixedPilotIds[0x00];
    fixedPilotIds[0x01] = FixedPilotIds[0x01];
    fixedUnitTypeIds[0x00] = FixedUnitTypeIds[0x00];
    fixedUnitTypeIds[0x01] = FixedUnitTypeIds[0x01];
    leftX = LanceEditorLayout.UnitRosterLeftColumn;
    rightX = LanceEditorLayout.UnitRosterRightColumn;
    separator = UnitRosterSeparator;

    if (useAlternateAssignmentLayout == FALSE)
    {
        pilotName = PilotSurnameTextTable[assignedPilotIds[0x00]];
        unitName = UnitNameTable[assignedUnitTypeIds[0x00]];
        strcpy(rosterLine, pilotName);
        strcat(rosterLine, separator);
        strcat(rosterLine, unitName);
        DrawCenteredClippedText(rosterLine, 0x00,
            leftX + UNIT_ROSTER_COLUMN_X_ADJUSTMENT,
            UNIT_ROSTER_FIRST_ROW_Y, UNIT_ROSTER_TEXT_WIDTH_PIXELS,
            (uint8_t) textColor, 0x00);

        pilotName = PilotSurnameTextTable[assignedPilotIds[0x01]];
        unitName = UnitNameTable[assignedUnitTypeIds[0x01]];
        strcpy(rosterLine, pilotName);
        strcat(rosterLine, separator);
        strcat(rosterLine, unitName);
        DrawCenteredClippedText(rosterLine, 0x00,
            leftX + UNIT_ROSTER_COLUMN_X_ADJUSTMENT,
            UNIT_ROSTER_FIRST_ROW_Y + 0x08,
            UNIT_ROSTER_TEXT_WIDTH_PIXELS, (uint8_t) textColor, 0x00);

        pilotName = PilotSurnameTextTable[assignedPilotIds[0x02]];
        unitName = UnitNameTable[assignedUnitTypeIds[0x02]];
        strcpy(rosterLine, pilotName);
        strcat(rosterLine, separator);
        strcat(rosterLine, unitName);
        DrawCenteredClippedText(rosterLine, 0x00,
            leftX + UNIT_ROSTER_COLUMN_X_ADJUSTMENT,
            UNIT_ROSTER_FIRST_ROW_Y + 0x10,
            UNIT_ROSTER_TEXT_WIDTH_PIXELS, (uint8_t) textColor, 0x00);

        pilotName = PilotSurnameTextTable[fixedPilotIds[0x00]];
        unitName = UnitNameTable[fixedUnitTypeIds[0x00]];
        strcpy(rosterLine, pilotName);
        strcat(rosterLine, separator);
        strcat(rosterLine, unitName);
        DrawCenteredClippedText(rosterLine, 0x00,
            rightX + UNIT_ROSTER_COLUMN_X_ADJUSTMENT,
            UNIT_ROSTER_FIRST_ROW_Y, UNIT_ROSTER_TEXT_WIDTH_PIXELS,
            (uint8_t) textColor, 0x00);

        pilotName = PilotSurnameTextTable[fixedPilotIds[0x01]];
        unitName = UnitNameTable[fixedUnitTypeIds[0x01]];
        strcpy(rosterLine, pilotName);
        strcat(rosterLine, separator);
        strcat(rosterLine, unitName);
        DrawCenteredClippedText(rosterLine, 0x00,
            rightX + UNIT_ROSTER_COLUMN_X_ADJUSTMENT,
            UNIT_ROSTER_FIRST_ROW_Y + 0x08,
            UNIT_ROSTER_TEXT_WIDTH_PIXELS, (uint8_t) textColor, 0x00);
        return;
    }

    pilotName = PilotSurnameTextTable[assignedPilotIds[0x01]];
    unitName = UnitNameTable[assignedUnitTypeIds[0x01]];
    strcpy(rosterLine, pilotName);
    strcat(rosterLine, separator);
    strcat(rosterLine, unitName);
    DrawCenteredClippedText(rosterLine, 0x00,
        leftX + UNIT_ROSTER_COLUMN_X_ADJUSTMENT,
        UNIT_ROSTER_FIRST_ROW_Y, UNIT_ROSTER_TEXT_WIDTH_PIXELS,
        (uint8_t) textColor, 0x00);

    pilotName = PilotSurnameTextTable[assignedPilotIds[0x02]];
    unitName = UnitNameTable[assignedUnitTypeIds[0x02]];
    strcpy(rosterLine, pilotName);
    strcat(rosterLine, separator);
    strcat(rosterLine, unitName);
    DrawCenteredClippedText(rosterLine, 0x00,
        leftX + UNIT_ROSTER_COLUMN_X_ADJUSTMENT,
        UNIT_ROSTER_FIRST_ROW_Y + 0x08,
        UNIT_ROSTER_TEXT_WIDTH_PIXELS, (uint8_t) textColor, 0x00);

    pilotName = PilotSurnameTextTable[fixedPilotIds[0x00]];
    unitName = UnitNameTable[assignedUnitTypeIds[0x00]];
    strcpy(rosterLine, pilotName);
    strcat(rosterLine, separator);
    strcat(rosterLine, unitName);
    DrawCenteredClippedText(rosterLine, 0x00,
        leftX + UNIT_ROSTER_COLUMN_X_ADJUSTMENT,
        UNIT_ROSTER_FIRST_ROW_Y + 0x10,
        UNIT_ROSTER_TEXT_WIDTH_PIXELS, (uint8_t) textColor, 0x00);

    pilotName = PilotSurnameTextTable[assignedPilotIds[0x00]];
    unitName = UnitNameTable[fixedUnitTypeIds[0x00]];
    strcpy(rosterLine, pilotName);
    strcat(rosterLine, separator);
    strcat(rosterLine, unitName);
    DrawCenteredClippedText(rosterLine, 0x00,
        rightX + UNIT_ROSTER_COLUMN_X_ADJUSTMENT,
        UNIT_ROSTER_FIRST_ROW_Y, UNIT_ROSTER_TEXT_WIDTH_PIXELS,
        (uint8_t) textColor, 0x00);

    pilotName = PilotSurnameTextTable[fixedPilotIds[0x01]];
    unitName = UnitNameTable[fixedUnitTypeIds[0x01]];
    strcpy(rosterLine, pilotName);
    strcat(rosterLine, separator);
    strcat(rosterLine, unitName);
    DrawCenteredClippedText(rosterLine, 0x00,
        rightX + UNIT_ROSTER_COLUMN_X_ADJUSTMENT,
        UNIT_ROSTER_FIRST_ROW_Y + 0x08,
        UNIT_ROSTER_TEXT_WIDTH_PIXELS, (uint8_t) textColor, 0x00);
}

// 2507:2BD9: prepare unit-selection records and their sprite buffers.
// VERIFIED against ASM 2507:2BD9-2E49.
void PrepareLanceSelectionRecords(
    uint16_t firstRosterIndex,
    int16_t selectionMode,
    uint16_t loadPresentationResources)
{
    uint16_t lanceGroup;
    uint16_t scenarioSlot;
    uint16_t unitType;
    uint16_t persistentUnitSlot;
    uint16_t unitTypeId;
    uint16_t graphicsBufferIndex;
    uint16_t poolIndex;
    UnitSelectionCard *scenarioRecord;

    if (loadPresentationResources != FALSE)
    {
        LoadGraphicsResourceToBuffers(
            0x0023,
            SecondaryScreenBuffer,
            PrimaryScreenBuffer);
        LoadGraphicsBufferRange(0x0001, 0x0008);
        LoadGraphicsResourceToBuffers(
            0x0024,
            SecondaryScreenBuffer,
            PrimaryScreenBuffer);
        LoadGraphicsBufferRange(0x0009, 0x0010);
    }

    if ((int16_t) firstRosterIndex < 0x0003)
    {
        UnitEditorScenarioCardCount = 0x0006;
        if (firstRosterIndex == 0x0000)
        {
            UnitEditorScenarioGroupCount = 0x0001;
            UnitEditorLanceGroupCount = 0x0001;
            if (selectionMode != 0x0000)
                UnitEditorLanceSlotCount[0x00] = 0x0004;
        }
        else if (firstRosterIndex == 0x0001)
        {
            UnitEditorScenarioCardCount = 0x0004;
            UnitEditorScenarioGroupCount = 0x0001;
            UnitEditorLanceGroupCount = 0x0001;
            if (selectionMode != 0x0000)
                UnitEditorLanceSlotCount[0x00] = 0x0004;
        }
        else
        {
            UnitEditorScenarioGroupCount = 0x0003;
            UnitEditorLanceGroupCount = 0x0003;
            if (selectionMode != 0x0000)
            {
                UnitEditorLanceSlotCount[0x00] = 0x0004;
                UnitEditorLanceSlotCount[0x01] = 0x0004;
                UnitEditorLanceSlotCount[0x02] = 0x0004;
            }
        }
    }
    else
    {
        UnitEditorScenarioCardCount =
            UnitEditorScenarioCardCountByFamily[firstRosterIndex];
        UnitEditorScenarioGroupCount = 0x0001;
        UnitEditorLanceGroupCount = 0x0003;
        if (selectionMode != 0x0000)
        {
            RandomUnitTypeCount =
                RandomUnitTypeCountByFamily[firstRosterIndex];
            for (poolIndex = 0x0000; poolIndex < 0x0007; ++poolIndex)
            {
                RandomUnitTypePool[poolIndex] =
                    RandomUnitTypePoolByFamily[(firstRosterIndex - 0x0003) * 0x0007 + poolIndex];
            }
            UnitEditorLanceSlotCount[0x00] = 0x0004;
            UnitEditorLanceSlotCount[0x01] = 0x0004;
            UnitEditorLanceSlotCount[0x02] = 0x0004;
        }
    }

    for (lanceGroup = 0x0000;
        (int16_t) lanceGroup < (int16_t) UnitEditorScenarioGroupCount;
        ++lanceGroup)
    {
        for (scenarioSlot = 0x0000; scenarioSlot < 0x0006; ++scenarioSlot)
        {
            scenarioRecord = &UnitSelectionCards[lanceGroup * 0x0006 + scenarioSlot];
            if ((int16_t) scenarioSlot < (int16_t) UnitEditorScenarioCardCount)
            {
                if (selectionMode != 0x0000)
                {
                    if ((int16_t) firstRosterIndex < 0x0003)
                    {
                        unitType = PredefinedUnitTypeTable[(firstRosterIndex + lanceGroup) * 0x0006 +
                            scenarioSlot];
                    }
                    else
                    {
                        unitType = (uint16_t) PopRandomUnitType();
                    }

                    if (selectionMode == 0x0001)
                    {
                        persistentUnitSlot =
                            lanceGroup * 0x0006 + scenarioSlot;
                        scenarioRecord->persistentUnitSlot =
                            (RevengeUnitSlot) persistentUnitSlot;
                        scenarioRecord->assignmentSlot =
                            REVENGE_ASSIGNMENT_SLOT_NONE;
                    }

                    LoadUnitTemplateRecord((int16_t) unitType);
                    persistentUnitSlot = scenarioRecord->persistentUnitSlot;
                    PersistentUnitRoster[REVENGE_INDEX(PersistentUnitRoster,
                        persistentUnitSlot)] = EstimatedUnitRecord;
                }

                if (scenarioRecord->assignmentSlot != REVENGE_ASSIGNMENT_SLOT_NONE)
                    scenarioRecord->state = 0x00;
                else
                    scenarioRecord->state = 0x01;

                // A card's slot is set by the campaign's first selection (mode
                // 1) before any other: the image's start cards hold none (FFh),
                // which the original would widen to FFFFh and read before the
                // roster. Play never prepares an unset card, so one is a fault.
                persistentUnitSlot = scenarioRecord->persistentUnitSlot;
                unitTypeId = PersistentUnitRoster[REVENGE_INDEX(PersistentUnitRoster, persistentUnitSlot)].unitTypeId;
                graphicsBufferIndex = UnitEditorUnitSpriteIndex[unitTypeId];
                scenarioRecord->spriteBuffer =
                    UnitEditorGraphicsBuffer(graphicsBufferIndex);
            }
            else
            {
                scenarioRecord->state = 0x00;
            }
        }
    }

    InitializeLanceAssignments(firstRosterIndex, selectionMode);
}

// 2507:2E4A: initialize the active lance-assignment records.
// VERIFIED against ASM 2507:2E4A-2FCC.
void InitializeLanceAssignments(
    uint16_t firstRosterIndex,
    int16_t selectionMode)
{
    uint16_t lanceGroup;
    uint16_t lanceSlot;
    uint16_t assignmentIndex;
    RevengePilotId pilotId;
    RevengeUnitSlot liveUnitSlot;
    RevengeUnitLanceAssignment *assignment;

    for (lanceGroup = 0x0000;
        (int16_t) lanceGroup < (int16_t) UnitEditorLanceGroupCount;
        ++lanceGroup)
    {
        for (lanceSlot = 0x0000; lanceSlot < 0x0004; ++lanceSlot)
        {
            assignmentIndex = lanceGroup * 0x0004 + lanceSlot;
            assignment = &LanceAssignments[assignmentIndex];

            if ((int16_t) UnitEditorLanceSlotCount[lanceGroup] >
                (int16_t) lanceSlot)
            {
                if (selectionMode == 0x0001 && firstRosterIndex < 0x0003)
                {
                    if (firstRosterIndex == 0x0001)
                    {
                        assignment->style = 0x06;
                        assignment->liveUnitSlot = REVENGE_UNIT_SLOT_UNAVAILABLE;
                    }
                    else
                    {
                        assignment->style = 0x02;
                        assignment->liveUnitSlot = REVENGE_UNIT_SLOT_NONE;
                    }

                    assignment->pilotId = (RevengePilotId)
                        DefaultPilotIdsByScenario[(firstRosterIndex + lanceGroup) * 0x0004 +
                            lanceSlot];
                    assignment->persistentUnitSlot = REVENGE_UNIT_SLOT_NONE;
                }
                else
                {
                    assignment->style = 0x00;
                    pilotId = assignment->pilotId;
                    if (pilotId != REVENGE_PILOT_ID_NONE)
                    {
                        if (PilotRoster[pilotId].availability !=
                            0xFF)
                        {
                            assignment->style |= 0x02;
                        }
                        else
                        {
                            assignment->pilotId = REVENGE_PILOT_ID_NONE;
                        }
                    }

                    liveUnitSlot = assignment->liveUnitSlot;
                    if (liveUnitSlot != REVENGE_UNIT_SLOT_NONE)
                    {
                        if (LiveScenarioUnits[liveUnitSlot].deploymentState != FALSE)
                        {
                            assignment->style |= 0x04;
                        }
                        else
                        {
                            assignment->liveUnitSlot = REVENGE_UNIT_SLOT_NONE;
                        }
                    }

                    if (assignment->pilotId == REVENGE_PILOT_ID_NONE)
                    {
                        UnitEditorLanceSlotCount[lanceGroup] =
                            lanceSlot;
                    }
                }
            }
            else
            {
                assignment->style = 0x00;
                assignment->persistentUnitSlot = REVENGE_UNIT_SLOT_NONE;
                assignment->liveUnitSlot = REVENGE_UNIT_SLOT_NONE;
                assignment->pilotId = REVENGE_PILOT_ID_NONE;
            }
        }
    }
}

// 2507:2FCD: prepare, display, and edit one lance-selection configuration.
// VERIFIED against ASM 2507:2FCD-300E.
void PrepareLanceSelection(
    uint16_t firstRosterIndex,
    int16_t selectionMode)
{
    if (ReplayScenarioRequested != FALSE)
        return;

    SceneTransitionAbortFlag = FALSE;
    (void) PrepareLanceSelectionRecords(
        firstRosterIndex,
        selectionMode,
        TRUE);
    DrawLanceSelectionScreen(firstRosterIndex);
    RunUnitAssignmentEditor(firstRosterIndex);
    FreeGraphicsBufferRange(0x0001, 0x0010);
}

// 2507:300F: validate the currently assigned live-unit slots for one
// lance-selection family.
// VERIFIED against ASM 2507:300F-30FB.
uint16_t ValidateLanceConfiguration(uint16_t selectionFamily)
{
    uint16_t configurationValid;
    uint16_t lanceGroup;
    uint16_t remainingScenarioUnits;
    uint16_t firstScenarioRecord;
    uint16_t scenarioSlot;
    uint16_t firstAssignmentRecord;
    uint16_t assignmentSlot;

    configurationValid = TRUE;
    if (UnitEditorScenarioCardCount != 0x0000)
    {
        lanceGroup = 0x0000;
        do
        {
            if (UnitEditorSectionEnabled[selectionFamily * LANCE_EDITOR_SECTION_COUNT] != FALSE)
            {
                remainingScenarioUnits = UnitEditorScenarioCardCount;
                if (UnitEditorScenarioGroupCount == 0x0001)
                    firstScenarioRecord = 0x0000;
                else
                    firstScenarioRecord = lanceGroup * 0x0006;

                for (scenarioSlot = 0x0000;
                    (int16_t) scenarioSlot <
                    (int16_t) UnitEditorScenarioCardCount;
                    ++scenarioSlot)
                {
                    if ((UnitSelectionCards[firstScenarioRecord + scenarioSlot].state &
                            0x01) == 0x00)
                    {
                        --remainingScenarioUnits;
                    }
                }

                if (remainingScenarioUnits != 0x0000)
                {
                    firstAssignmentRecord = lanceGroup * 0x0004;
                    assignmentSlot = 0x0000;
                    do
                    {
                        if (LanceAssignments[firstAssignmentRecord + assignmentSlot].liveUnitSlot ==
                            REVENGE_UNIT_SLOT_NONE)
                        {
                            configurationValid = FALSE;
                        }
                        ++assignmentSlot;
                    } while ((int16_t) assignmentSlot < (int16_t)
                                                            UnitEditorLanceSlotCount[lanceGroup] &&
                        configurationValid != FALSE);
                }
            }
            ++lanceGroup;
        } while (lanceGroup < 0x0003 && configurationValid != FALSE);
    }

    UnitEditorSectionEnabled[selectionFamily * LANCE_EDITOR_SECTION_COUNT + 0x03] =
        (uint8_t) configurationValid;
    return configurationValid;
}
