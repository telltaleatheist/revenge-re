// 0D73: campaign menus
// CampaignMenus.cpp
// Procedures reconstructed from REVENGE.EXE segment 0D73.

#include "CampaignInternal.h"
#include "Audio/Audio.h"
#include "Campaign/Campaign.h"
#include "Data/RuntimeData.h"
#include "Data/StartupData.h"
#include "Data/CampaignStateData.h"
#include "Data/RosterData.h"
#include "Data/TacticalUnitData.h"
#include "Data/TextIOData.h"
#include "Input/Input.h"
#include "Text/Text.h"
#include "UI/UI.h"
#include "Units/Units.h"
#include "revenge/debug.h"

#include <string.h>

// 0D73:2414: word16 ConfirmExitToDos(void)
// Called from:
//      RunInGameMenu
//      RunTitleAndMainMenu
uint16_t ConfirmExitToDos(void)
{
    /* DS:0822: the confirmation's region (7) starts on its second row. */
    UiRegionTable[0x07].selectedRow = 0x0001;
    return ShowConfirmationDialog(ExitToDosPrompt) == 0x00;
}

/* 0D73:242E is reconstructed in UI/Confirmation.cpp. */

// 0D73:2448: word16 ShowConfirmationDialog(const char far *message)
// Called from:
//      ConfirmExitToDos
//      ConfirmRestartGame
uint16_t ShowConfirmationDialog(const char *message)
{
    uint16_t selection;

    OpenModalTextWindow(0x0E, 0x01);
    HideMouseCursorOverlay();
    RenderFormattedText(message);
    ShowMouseCursorOverlay();
    FlushPendingInput();
    do
    {
        selection = RunSingleUiRegionSelection(0x07);
        if (selection == KEY_ESCAPE)
            ToggleActiveUiRegionHighlight();
    } while (selection == KEY_ESCAPE);
    RestoreModalTextWindow();
    return selection;
}

// 0D73:249E: callback that runs the DS:25CD two-option menu and stores its
// zero-based byte selection at DS:24E5.
// VERIFIED against 0D73:249E-24B2.
void RunCampaignPathSelectionCallback(void)
{
    RunTwoOptionMenuSelection(
        CampaignPathMenuTextTable);
    CampaignRestartState =
        (uint8_t) (ActiveUiRegionIndex - 0x0008);
}

// 0D73:24B3: callback that runs the DS:25D5 two-option menu and stores its
// zero-based word selection at DS:24D8.
// VERIFIED against 0D73:24B3-24C8.
void RunScenarioVariantSelectionCallback(void)
{
    RunTwoOptionMenuSelection(
        ScenarioVariantMenuTextTable);
    ScenarioVariantIndex =
        (uint16_t) (ActiveUiRegionIndex - 0x0008);
}

// 0D73:24C9: callback that runs the DS:25DD two-option menu and stores its
// zero-based word selection at DS:24D8.
// VERIFIED against 0D73:24C9-24DE.
void RunAlternateScenarioVariantSelectionCallback(void)
{
    RunTwoOptionMenuSelection(
        AlternateScenarioMenuTextTable);
    ScenarioVariantIndex =
        (uint16_t) (ActiveUiRegionIndex - 0x0008);
}

// 0D73:24DF: void ShowMainMenu(void)
// Called from:
//      RunTitleAndMainMenu
void ShowMainMenu(void)
{
    RunMainMenuSelection(
        MainMenuTextTable);
    ScenarioVariantIndex =
        (uint16_t) (ActiveUiRegionIndex - 0x0008);
}

// 0D73:24F5: draw and select from a two-option far-string table.
// VERIFIED against 0D73:24F5-25F4.
uint16_t RunTwoOptionMenuSelection(
    const char *const menuOptionText[0x02])
{
    int16_t menuRow;
    int16_t secondColumn;
    int16_t glyphWidth;
    uint16_t firstLength;
    uint16_t secondLength;
    uint16_t selection;

    HideMouseCursorOverlay();
    menuRow = (int16_t) ((ActiveTextWindowHeightTiles << 0x03) /
                  ActiveFontLineHeight) -
        0x0002;
    glyphWidth = ActiveFontGlyphAdvanceX;

    const char *firstText = menuOptionText[0x00];
    const char *secondText = menuOptionText[0x01];
    firstLength = (uint16_t) strlen(firstText);
    DrawBeveledMenuButton(0x0005, menuRow,
        firstText,
        (uint16_t) (glyphWidth * firstLength));
    UiRegionTable[0x08].leftColumn = 0x0005;
    UiRegionTable[0x08].topRow = menuRow;
    UiRegionTable[0x08].widthColumns = (uint16_t) strlen(firstText);

    secondLength = (uint16_t) strlen(secondText);
    secondColumn = (int16_t) (0x0108 / glyphWidth) - secondLength;
    DrawBeveledMenuButton(secondColumn, menuRow,
        secondText,
        (uint16_t) (glyphWidth * secondLength));
    UiRegionTable[0x09].leftColumn = secondColumn;
    UiRegionTable[0x09].topRow = menuRow;
    UiRegionTable[0x09].widthColumns = secondLength;

    ShowMouseCursorOverlay();
    FlushPendingInput();
    do
    {
        selection = RunUiRegionSelectionLoop(0x0008, 0x0002);
        if (selection == KEY_ESCAPE)
            ToggleActiveUiRegionHighlight();
    } while (selection == KEY_ESCAPE);
    return selection;
}

// 0D73:25F5: word16 RunMainMenuSelection(const far-pointer table menuOptionText)
// Called from:
//      ShowMainMenu
uint16_t RunMainMenuSelection(const char *const menuOptionText[0x03])
{
    int16_t menuRow;
    int16_t glyphWidth;
    int16_t secondColumn;
    int16_t thirdColumn;
    uint16_t firstLength;
    uint16_t secondLength;
    uint16_t thirdLength;
    uint16_t selection;

    // Verified against 0D73:25F5-274C.
    HideMouseCursorOverlay();
    menuRow = (int16_t) ((ActiveTextWindowHeightTiles << 0x03) /
                  ActiveFontLineHeight) -
        0x02;
    glyphWidth = ActiveFontGlyphAdvanceX;
    const char *firstText = menuOptionText[0x00];
    const char *secondText = menuOptionText[0x01];
    const char *thirdText = menuOptionText[0x02];
    firstLength = (uint16_t) strlen(firstText);
    DrawBeveledMenuButton(0x03, menuRow, firstText,
        (uint16_t) (glyphWidth * firstLength));
    UiRegionTable[0x08].leftColumn = 0x03;
    UiRegionTable[0x08].topRow = menuRow;
    firstLength = (uint16_t) strlen(firstText);
    UiRegionTable[0x08].widthColumns = firstLength;

    secondLength = (uint16_t) strlen(secondText);
    secondColumn = (int16_t) ((0x0130 / glyphWidth) - secondLength);
    secondColumn >>= 0x01;
    DrawBeveledMenuButton(secondColumn, menuRow, secondText,
        (uint16_t) (secondLength * glyphWidth));
    UiRegionTable[0x09].leftColumn = secondColumn;
    UiRegionTable[0x09].topRow = menuRow;
    UiRegionTable[0x09].widthColumns = secondLength;

    thirdLength = (uint16_t) strlen(thirdText);
    thirdColumn = (int16_t) ((0x0130 / glyphWidth) - thirdLength);
    DrawBeveledMenuButton(thirdColumn, menuRow, thirdText,
        (uint16_t) (thirdLength * glyphWidth));
    UiRegionTable[0x0A].leftColumn = thirdColumn;
    UiRegionTable[0x0A].topRow = menuRow;
    UiRegionTable[0x0A].widthColumns = thirdLength;
    ShowMouseCursorOverlay();
    FlushPendingInput();
    do
    {
        selection = RunUiRegionSelectionLoop(0x08, 0x03);
        if (selection == KEY_ESCAPE)
            ToggleActiveUiRegionHighlight();
    } while (selection == KEY_ESCAPE);
    return selection;
}

// 0D73:274D: int16 RunInGameMenu(int16 firstMenuOption)
// Called from:
//      RunCampaignLoop
// VERIFIED against 0D73:274D-2A5C.
int16_t RunInGameMenu(int16_t firstMenuOption)
{
    int16_t actionResult;
    int16_t result;
    uint16_t index;
    uint16_t menuWindowTop;
    uint16_t menuItemCount;
    uint16_t menuY;
    uint16_t optionIndex;
    uint16_t selection;

    result = -0x0001;

    if (firstMenuOption == 0x00)
    {
        menuWindowTop = 0x09;
        menuItemCount = IN_GAME_MENU_OPTION_COUNT;
    }
    else
    {
        menuWindowTop = 0x0A;
        menuItemCount = 0x05;
    }
    TextWindowContexts[0x11].topTile = menuWindowTop;
    TextWindowContexts[0x11].heightTiles = (uint16_t) ((menuItemCount *
                                                               ActiveFontLineHeight +
                                                           0x0007) >>
        0x03);
    OpenModalTextWindow(0x11, 0x01);
    HideMouseCursorOverlay();
    menuY = TextWindowContexts[0x11].topTile << 0x03;
    for (optionIndex = (uint16_t) firstMenuOption;
        optionIndex < IN_GAME_MENU_OPTION_COUNT;
        ++optionIndex)
    {
        DrawCenteredClippedText(
            InGameMenuTextTable[optionIndex + 0x02],
            0x00,
            TextWindowContexts[0x11].leftTile << 0x03,
            menuY,
            TextWindowContexts[0x11].widthTiles << 0x03,
            (uint8_t) ActiveTextForegroundColor,
            (uint8_t) ActiveTextBackgroundColor); // TABLE DS:25F9: MenuOptionTextTable entry 5 onward
        menuY += (int16_t) ActiveFontLineHeight;
    }
    ShowMouseCursorOverlay();
    UiRegionTable[0x0B].rowCount = menuItemCount;
    UiRegionTable[0x0B].widthColumns = 0x11;
    FlushPendingInput();
    do
    {
        selection = RunSingleUiRegionSelection(0x0B);
        if (selection == KEY_ESCAPE)
            ToggleActiveUiRegionHighlight();
    } while (selection == KEY_ESCAPE);
    RestoreModalTextWindow();

    selection += firstMenuOption;
    if (selection > IN_GAME_MENU_OPTIONS)
        return result;

    switch (selection)
    {
    case IN_GAME_MENU_CONTINUE:
    case IN_GAME_MENU_JUMP_STORY:
    {
        if (ScenarioRosterPrepared == FALSE)
        {
            PrepareScenarioUnitRosters();
            CampaignStageIndex += PendingCampaignStageDelta;
            CampaignPhase += PendingCampaignPhaseDelta;
        }
        ReplayScenarioRequested = FALSE;
        SceneTransitionAbortFlag = selection == IN_GAME_MENU_JUMP_STORY;
        result = 0x01;
        break;
    }
    case IN_GAME_MENU_SAVE_CONTINUE:
    {
        actionResult = (int16_t) SaveGame();
        result = actionResult == 0x0000 ? -0x0001 : actionResult;
        if (actionResult < 0x0000)
            result = ConfirmExitToDos() ? 0x0000 : -0x0001;
        break;
    }
    case IN_GAME_MENU_SAVE_QUIT:
    {
        actionResult = (int16_t) SaveGame();
        result = actionResult == 0x0001 ? 0x0000 : (actionResult == 0x0000 ? -0x0001 : actionResult);
        if (actionResult < 0x0000)
            result = ConfirmExitToDos() ? 0x0000 : -0x0001;
        break;
    }
    case IN_GAME_MENU_LOAD:
    {
        actionResult = (int16_t) LoadGame();
        result = actionResult == 0x0000 ? -0x0001 : actionResult;
        if (actionResult < 0x0000)
            result = ConfirmExitToDos() ? 0x0000 : -0x0001;
        break;
    }
    case IN_GAME_MENU_REPLAY_SCENARIO:
        ReplayCampaignStateRequested = TRUE;
        ReplayScenarioRequested = TRUE;
        result = 0x01;
        break;
    case IN_GAME_MENU_REPLAY_STORY:
        PromoteReserveUnitsForCampaignStage();
        ReplayCampaignStateRequested = TRUE;
        TrainingSequenceFlag = FALSE;
        result = 0x01;
        break;
    case IN_GAME_MENU_RESTART:
        if (!ConfirmRestartGame())
            break;
        CampaignRestartState = FALSE;
        ReplayCampaignStateRequested = FALSE;
        for (index = 0x0000; index < CAMPAIGN_PRIMARY_STATE_COUNT; ++index)
        {
            ReplayCampaignState[index] =
                InitialCampaignState[index];
        }
        RandomPilotPoolCounts.replay = RandomPilotPoolCounts.initial;
        for (index = 0x0000; index < CAMPAIGN_EXTENDED_STATE_COUNT; ++index)
        {
            ReplayExtendedCampaignState[index] =
                InitialExtendedCampaignState[index];
        }
        CampaignStageIndex = 0x00;
        ScenarioVariantIndex = 0x00;
        CampaignPhase = FALSE;
        TrainingSequenceFlag = FALSE;
        CampaignFlags = 0x00;
        SavedCampaignFlags = 0x00;
        SrmShipmentLoaded = 0x00;
        LrmShipmentLoaded = 0x00;
        CampaignScenarioState = 0x00;
        LastAttackingUnitIndex = 0x00;
        ScenarioUnitThirteenEnabled = 0x00;
        ScenarioUnitFourteenEnabled = 0x00;
        ScenarioUnitFifteenState = 0x00;
        EjectedPilotRosterState = 0x00;
        ScenarioUnitZeroAvailable = 0x00;
        for (index = 0x0000; index < TRAINING_SCENARIO_COUNT; ++index)
            TrainingScenarioCompletionFlags[index] = 0x00;
        result = 0x01;
        break;
    case IN_GAME_MENU_OPTIONS:
        result = RunGameOptionsMenu(0x0000) == 0x0004 ? 0x0000 : -0x0001;
        break;
    }

    ReplayScenarioRequested = selection == IN_GAME_MENU_REPLAY_SCENARIO;
    if (selection != IN_GAME_MENU_CONTINUE && selection != IN_GAME_MENU_JUMP_STORY)
        SceneTransitionAbortFlag = FALSE;
    return result;
}

// 0D73:309C: int16 RunTitleAndMainMenu(void)
// Called from:
//      RunCampaignLoop
// Verified against 0D73:309C-324D.
int16_t RunTitleAndMainMenu(void)
{
    int16_t selectedGameMode = 0x00;
    uint16_t transitionPending;
    uint16_t showTitle;

    SelectTextWindow(0x00);
    LoadScenarioDigitalSoundBank(-0x01);
    PlayScenarioDigitalSoundSequence(~0x00);
    ReleaseDigitalSoundBuffer();
    PresentationDismissTimeout = 0x00;
    transitionPending = PresentScreenAndMessage(0x0F, 0x00);
    LoadScenarioDigitalSoundBank(-0x02);
    PlayScenarioDigitalSoundSequence(~0x01);
    ReleaseDigitalSoundBuffer();
    PlayMusicResource(0x4D);
    PresentationDismissTimeout = 0x01C2;
    transitionPending = WaitForDismissInputOrTimeout(PresentationDismissTimeout);
    PresentationDismissTimeout = 0x0258;
    showTitle = TRUE;
    while (showTitle != FALSE)
    {
        uint16_t titleCompleted;
        if (SceneTransitionAbortFlag == 0x00 && transitionPending == 0x00)
        {
            titleCompleted = PlayTitleTextSequence();
        }
        else
        {
            transitionPending = 0x00;
            titleCompleted = FALSE;
        }
        showTitle = titleCompleted;
        if (titleCompleted == FALSE)
        {
            SceneTransitionAbortFlag = 0x00;
            HideMouseCursorOverlay();
            CopyClippedVideoRegion(0x01, 0xB7, 0x01, 0xB7, 0x25, 0x0B, VideoFramebuffer, PrimaryScreenBuffer);
            ShowMouseCursorOverlay();
            ShowMainMenu();
            REVENGE_PRESERVATION_TRACE("menu: main selection=%u activeRegion=%d\n",
                ScenarioVariantIndex, ActiveUiRegionIndex);
            if (ScenarioVariantIndex == 0x00)
            {
                CampaignStageIndex = 0x00;
                selectedGameMode = 0x01;
            }
            else if (ScenarioVariantIndex == 0x01)
            {
                int16_t loadResult = (int16_t) LoadGame();
                selectedGameMode = loadResult;
                if (loadResult == 0x00)
                    showTitle = TRUE;
                else if (loadResult < 0x00)
                {
                    selectedGameMode = ConfirmExitToDos() == FALSE;
                    if (selectedGameMode == FALSE)
                    {
                        VideoPathUnavailable = TRUE;
                    }
                    else
                        showTitle = TRUE;
                }
                ScenarioVariantIndex = 0x00;
            }
            else
            {
                if (RunGameOptionsMenu(0x00) != 0x04)
                    showTitle = TRUE;
                else
                {
                    selectedGameMode = 0x00;
                    showTitle = FALSE;
                }
            }
            HideMouseCursorOverlay();
            CopyClippedVideoRegion(0x01, 0xB7, 0x01, 0xB7, 0x25, 0x0B, PrimaryScreenBuffer, VideoFramebuffer);
            ShowMouseCursorOverlay();
            if (showTitle != FALSE)
                transitionPending = WaitForDismissInputOrTimeout(PresentationDismissTimeout);
        }
    }
    PresentationDismissTimeout = 0x012C;
    return selectedGameMode;
}
