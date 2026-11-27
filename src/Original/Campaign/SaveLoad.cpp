// 0D73: save load
// SaveLoad.cpp
// Procedures reconstructed from REVENGE.EXE segment 0D73.

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "CampaignInternal.h"
#include "Data/CampaignData.h"
#include "Data/RuntimeData.h"
#include "Data/TacticalData.h"
#include "Data/TextIOData.h"
#include "IO/IO.h"
#include "Input/Input.h"
#include "Resources/Resources.h"
#include "Text/Text.h"
#include "Units/Units.h"
#include "revenge/debug.h"
#include "revenge/story_text.h"

#include <string.h>

// 0D73:4866: word16 SaveGame(void)
// Called from:
//      RunInGameMenu
// VERIFIED against 0D73:4866-4A7D.
uint16_t SaveGame(void)
{
    RevengeCampaignSnapshot *snapshot;
    uint16_t lanceIndex;
    uint16_t menuAction;
    uint16_t pilotIndex;
    uint16_t scenarioRecordIndex;
    uint16_t unitIndex;

    OpenModalTextWindow(0x0016, 0x0001);
    snapshot = &SaveCampaignSnapshot;
    menuAction = UI_SELECTION_PENDING;
    while (menuAction == UI_SELECTION_PENDING)
    {
        menuAction = RunSaveLoadMenu(SAVE_LOAD_MODE_SAVE);
        if ((int16_t) menuAction > 0x0000)
        {
            ActiveTextCursorRow = 0x000A;
            HideMouseCursorOverlay();
            RenderFormattedText(STORY_TEXT(SAVE_GAME_0)); // TEXT DS:261D
            ShowMouseCursorOverlay();
            if (ScenarioRosterPrepared == FALSE)
            {
                PrepareScenarioUnitRosters();
                CampaignStageIndex += PendingCampaignStageDelta;
                CampaignPhase += PendingCampaignPhaseDelta;
                if ((SavedCampaignFlags & 0x0008) != 0x0000)
                    CampaignFlags |= 0x2000;
            }
            ScenarioRosterPrepared = TRUE;
            snapshot->campaignStageIndex = CampaignStageIndex;
            snapshot->scenarioVariantIndex = ScenarioVariantIndex;
            snapshot->campaignPhase = CampaignPhase;
            snapshot->trainingSequenceFlag = TrainingSequenceFlag;
            snapshot->campaignFlags = CampaignFlags;
            snapshot->state24E1 = SavedCampaignFlags;
            snapshot->stateA967 = ScenarioUnitZeroAvailable;
            snapshot->scene3RosterSelector9991 = EjectedPilotRosterState;
            snapshot->state88F1 = ScenarioUnitFifteenState;
            snapshot->state8925 = ScenarioUnitFourteenEnabled;
            snapshot->state99C5 = ScenarioUnitThirteenEnabled;
            snapshot->stateA965 = LastAttackingUnitIndex;
            snapshot->state8594 = CampaignScenarioState;
            snapshot->lrmShipmentLoaded = LrmShipmentLoaded;
            snapshot->srmShipmentLoaded = SrmShipmentLoaded;
            memcpy(
                snapshot->scenarioClock,
                SavedCampaignClockState,
                sizeof(snapshot->scenarioClock));
            memcpy(
                snapshot->trainingScenarioCompletionFlags,
                TrainingScenarioCompletionFlags,
                sizeof(snapshot->trainingScenarioCompletionFlags));
            for (unitIndex = 0x0000;
                unitIndex < TACTICAL_UNIT_COUNT;
                ++unitIndex)
            {
                snapshot->scenarioUnitMapX[unitIndex] = ScenarioUnitMapX[unitIndex];
                snapshot->scenarioUnitMapY[unitIndex] = ScenarioUnitMapY[unitIndex];
            }
            memcpy(
                snapshot->state1B85,
                ReplayCampaignState,
                sizeof(snapshot->state1B85));
            snapshot->state1B8E = RandomPilotPoolCounts.replay;
            memcpy(
                snapshot->state1BAD,
                ReplayExtendedCampaignState,
                sizeof(snapshot->state1BAD));
            for (scenarioRecordIndex = 0x0000;
                scenarioRecordIndex < 0x0012;
                ++scenarioRecordIndex)
            {
                UnitSelectionCard_Store(
                    &snapshot->scenarioRecords[scenarioRecordIndex],
                    &UnitSelectionCards[scenarioRecordIndex]);
            }
            for (lanceIndex = 0x0000; lanceIndex < 0x000D; ++lanceIndex)
            {
                snapshot->lanceAssignments[lanceIndex] =
                    LanceAssignments[lanceIndex];
            }
            pilotIndex = 0x0000;
            while (pilotIndex < 0x0025)
            {
                snapshot->pilotAvailability[pilotIndex] =
                    PilotRoster[pilotIndex].availability;
                ++pilotIndex;
            }
            WriteSaveSlot(
                0x0044,
                (int16_t) SelectedSaveSlotIndex + 0x0001);
            HideMouseCursorOverlay();
            RenderFormattedText(STORY_TEXT(SAVE_GAME_1)); // TEXT DS:2630
            ShowMouseCursorOverlay();
            SaveDirectory.slots[SelectedSaveSlotIndex].occupied = TRUE;
            SavedSaveTransferState = SaveTransferState;
            WriteSaveSlot(0x0044, 0x0000);
        }
    }
    RestoreModalTextWindow();
    return menuAction;
}

// 0D73:4A7E: word16 WriteSaveSlot(word16 cachedFileIndex, int16 saveSlot)
// Called from:
//      SaveGame
//      ShowSaveLoadMenu
uint16_t WriteSaveSlot(uint16_t cachedFileIndex, int16_t saveSlot)
{
    uint16_t slotEndOffset;
    uint16_t slotStartOffset;
    uint16_t slotByteCount;
    int16_t directoryOffset;
    int16_t fileHandle;

    // Verified against 0D73:4A7E-4B85.
    // Write either the save index (slot zero) or a selected serialized game slot.
    fileHandle = EnsureCompatibleSaveFile(
        OpenTrackedFile(
            CACHED_FILE_NAME(cachedFileIndex),
            0x03),
        CACHED_FILE_NAME(cachedFileIndex),
        0x03);
    directoryOffset = (int16_t) ((uint16_t) (saveSlot + 0x01) << 0x01);
    SeekTrackedFile(fileHandle, (uint32_t) (int32_t) directoryOffset, 0x00);
    ReadTrackedFile(fileHandle, &slotStartOffset, 0x02);
    ReadTrackedFile(fileHandle, &slotEndOffset, 0x02);
    SeekTrackedFile(
        fileHandle, (uint32_t) (int32_t) (int16_t) slotStartOffset, 0x00);
    slotByteCount = slotEndOffset - slotStartOffset;
    if (saveSlot == 0x00)
    {
        WriteTrackedFile(fileHandle, SaveDirectory.raw,
            (uint32_t) (int32_t) (int16_t) slotByteCount);
    }
    else
    {
        WriteTrackedFile(fileHandle, &SaveCampaignSnapshot, REVENGE_CAMPAIGN_SNAPSHOT_SIZE);
        WriteTrackedFile(fileHandle, LiveScenarioUnits,
            (uint32_t) (int32_t) (int16_t) (slotByteCount - REVENGE_CAMPAIGN_SNAPSHOT_SIZE));
    }
    CloseTrackedFile(fileHandle);
    return 0x01;
}

// 0D73:4B86: word16 LoadGame(void)
// Called from:
//      RunInGameMenu
//      RunTitleAndMainMenu
// VERIFIED against 0D73:4B86-4D8E.
uint16_t LoadGame(void)
{
    RevengeCampaignSnapshot *snapshot;
    uint8_t stateValue;
    uint16_t lanceIndex;
    uint16_t menuAction;
    uint16_t pilotIndex;
    uint16_t scenarioRecordIndex;
    uint16_t stateIndex;
    uint16_t unitIndex;

    OpenModalTextWindow(0x0016, 0x0001);
    snapshot = &SaveCampaignSnapshot;
    menuAction = UI_SELECTION_PENDING;
    while (menuAction == UI_SELECTION_PENDING)
    {
        menuAction = RunSaveLoadMenu(SAVE_LOAD_MODE_LOAD);
        if ((int16_t) menuAction > 0x0000)
        {
            if (menuAction == SAVE_LOAD_ACTION_TRAINING &&
                SaveTransferState == 0x8000)
            {
                CampaignRestartState = FALSE;
                CampaignStageIndex = 0x0015;
                CampaignPhase = 0x00;
                ScenarioVariantIndex = 0x0000;
                SavedCampaignFlags = 0x0000;
                CampaignFlags = 0x0000;
                ReplayCampaignStateRequested = FALSE;
                TrainingSequenceFlag = FALSE;
                SrmShipmentLoaded = 0x0000;
                LrmShipmentLoaded = 0x0000;
                CampaignScenarioState = 0x0000;
                LastAttackingUnitIndex = 0x0000;
                ScenarioUnitThirteenEnabled = 0x0000;
                ScenarioUnitFourteenEnabled = 0x0000;
                ScenarioUnitFifteenState = 0x0000;
                EjectedPilotRosterState = 0x0000;
                ScenarioUnitZeroAvailable = 0x0000;
                SavedCampaignClockState[0x02] = 0x00;
                SavedCampaignClockState[0x01] = 0x00;
                SavedCampaignClockState[0x00] = 0x00;
                ReplayScenarioRequested = FALSE;
                SceneTransitionAbortFlag = FALSE;
            }
            else if (ReadSaveSlot(
                         0x0044,
                         SelectedSaveSlotIndex + 0x0001) != FALSE)
            {
                CampaignRestartState = FALSE;
                CampaignStageIndex = snapshot->campaignStageIndex;
                ScenarioVariantIndex = snapshot->scenarioVariantIndex;
                CampaignPhase = snapshot->campaignPhase;
                TrainingSequenceFlag = snapshot->trainingSequenceFlag;
                CampaignFlags = snapshot->campaignFlags;
                SavedCampaignFlags = snapshot->state24E1;
                ScenarioUnitZeroAvailable = snapshot->stateA967;
                EjectedPilotRosterState = snapshot->scene3RosterSelector9991;
                ScenarioUnitFifteenState = snapshot->state88F1;
                ScenarioUnitFourteenEnabled = snapshot->state8925;
                ScenarioUnitThirteenEnabled = snapshot->state99C5;
                LastAttackingUnitIndex = snapshot->stateA965;
                CampaignScenarioState = snapshot->state8594;
                LrmShipmentLoaded = snapshot->lrmShipmentLoaded;
                SrmShipmentLoaded = snapshot->srmShipmentLoaded;
                memcpy(
                    SavedCampaignClockState,
                    snapshot->scenarioClock,
                    sizeof(snapshot->scenarioClock));
                memcpy(
                    TrainingScenarioCompletionFlags,
                    snapshot->trainingScenarioCompletionFlags,
                    sizeof(snapshot->trainingScenarioCompletionFlags));
                for (unitIndex = 0x0000;
                    unitIndex < TACTICAL_UNIT_COUNT;
                    ++unitIndex)
                {
                    ScenarioUnitMapX[unitIndex] = snapshot->scenarioUnitMapX[unitIndex];
                    ScenarioUnitMapY[unitIndex] = snapshot->scenarioUnitMapY[unitIndex];
                }
                REVENGE_PRESERVATION_TRACE(
                    "save: loaded slot=%u stage=%u variant=%u unit0=%u,%u unit1=%u,%u\n",
                    SelectedSaveSlotIndex + 0x01,
                    CampaignStageIndex, ScenarioVariantIndex,
                    ScenarioUnitMapX[0x00],
                    ScenarioUnitMapY[0x00],
                    ScenarioUnitMapX[0x01],
                    ScenarioUnitMapY[0x01]);
                for (stateIndex = 0x0000;
                    stateIndex < sizeof(snapshot->state1B85);
                    ++stateIndex)
                {
                    stateValue = snapshot->state1B85[stateIndex];
                    ReplayCampaignState[stateIndex] = stateValue;
                    ForwardCampaignState[stateIndex] = stateValue;
                }
                RandomPilotPoolCounts.replay = snapshot->state1B8E;
                for (stateIndex = 0x0000;
                    stateIndex < sizeof(snapshot->state1BAD);
                    ++stateIndex)
                {
                    stateValue = snapshot->state1BAD[stateIndex];
                    ReplayExtendedCampaignState[stateIndex] = stateValue;
                    ForwardExtendedCampaignState[stateIndex] = stateValue;
                }
                for (scenarioRecordIndex = 0x0000;
                    scenarioRecordIndex < 0x0012;
                    ++scenarioRecordIndex)
                {
                    UnitSelectionCard_Load(
                        &UnitSelectionCards[scenarioRecordIndex],
                        &snapshot->scenarioRecords[scenarioRecordIndex]);
                }
                for (lanceIndex = 0x0000; lanceIndex < 0x000D; ++lanceIndex)
                {
                    LanceAssignments[lanceIndex] =
                        snapshot->lanceAssignments[lanceIndex];
                }
                pilotIndex = 0x0000;
                while (pilotIndex < 0x0025)
                {
                    PilotRoster[pilotIndex].availability =
                        snapshot->pilotAvailability[pilotIndex];
                    ++pilotIndex;
                }
            }
        }
    }
    RestoreModalTextWindow();
    return menuAction;
}

// 0D73:4D8F: word16 ReadSaveSlot(word16 cachedFileIndex, int16 saveSlot)
// Called from:
//      LoadGame
//      RunSaveLoadMenu
uint16_t ReadSaveSlot(uint16_t cachedFileIndex, int16_t saveSlot)
{
    uint16_t slotEndOffset;
    uint16_t slotStartOffset;
    uint16_t slotByteCount;
    uint16_t readResult;
    int16_t directoryOffset;
    int16_t fileHandle;

    // Verified against 0D73:4D8F-4E9C.
    // Read either the save index (slot zero) or a selected serialized game slot.
    fileHandle = EnsureCompatibleSaveFile(
        OpenTrackedFile(
            CACHED_FILE_NAME(cachedFileIndex),
            0x01),
        CACHED_FILE_NAME(cachedFileIndex),
        0x01);
    directoryOffset = (int16_t) ((uint16_t) (saveSlot + 0x01) << 0x01);
    SeekTrackedFile(fileHandle, (uint32_t) (int32_t) directoryOffset, 0x00);
    ReadTrackedFile(fileHandle, &slotStartOffset, 0x02);
    ReadTrackedFile(fileHandle, &slotEndOffset, 0x02);
    SeekTrackedFile(
        fileHandle, (uint32_t) (int32_t) (int16_t) slotStartOffset, 0x00);
    slotByteCount = slotEndOffset - slotStartOffset;
    if (saveSlot == 0x00)
    {
        ReadTrackedFile(fileHandle, SaveDirectory.raw,
            (uint32_t) (int32_t) (int16_t) slotByteCount);
        readResult = SavedSaveTransferState;
        SaveTransferState = readResult;
    }
    else
    {
        ReadTrackedFile(fileHandle, &SaveCampaignSnapshot, REVENGE_CAMPAIGN_SNAPSHOT_SIZE);
        ReadTrackedFile(fileHandle, LiveScenarioUnits,
            (uint32_t) (int32_t) (int16_t) (slotByteCount - REVENGE_CAMPAIGN_SNAPSHOT_SIZE));
    }
    CloseTrackedFile(fileHandle);
    return 0x01;
}

// 0D73:4E9D: int16 RunSaveLoadMenu(word16 menuMode)
// Called from:
//      SaveGame
//      LoadGame
// VERIFIED against 0D73:4E9D-4EC6.
int16_t RunSaveLoadMenu(uint16_t menuMode)
{
    int16_t menuAction;

    menuAction = ReadSaveSlot(0x0044, 0x0000);
    if (menuAction == TRUE)
        menuAction = ShowSaveLoadMenu(menuMode);
    return menuAction;
}

// 0D73:4EC7: int16 ShowSaveLoadMenu(word16 menuMode)
// Called from:
//      RunSaveLoadMenu
// VERIFIED against 0D73:4EC7-51F8.
int16_t ShowSaveLoadMenu(uint16_t menuMode)
{
    RevengeSaveSlotDescriptor *selectedSlot;
    char previousSlotName[0x16];
    uint8_t previousDefaultSelection;
    int16_t menuAction;
    int16_t promptX;
    int16_t promptY;
    int16_t selectedMenuItem;
    uint16_t cancelItemIndex;
    uint16_t directoryChanged;
    uint16_t exitDosItemIndex;
    uint16_t menuItemCount;
    uint16_t previousBackground;
    uint16_t renameItemIndex;
    uint16_t slotIndex;
    int16_t trainingItemIndex;

    directoryChanged = FALSE;
    menuAction = SAVE_LOAD_ACTION_CANCEL;

    HideMouseCursorOverlay();
    ClearActiveTextWindow();
    previousDefaultSelection = SaveLoadMenuDefaultSelection;

    for (slotIndex = 0x0000; slotIndex < SAVE_SLOT_COUNT; ++slotIndex)
    {
        /* The ASM uses a byte displacement into the packed far-pointer table. */
        RenderFormattedText(
            InGameMenuTextTable[menuMode - SAVE_LOAD_MODE_SAVE]);
        RenderFormattedText(SaveGameSeparatorText);
        RenderFormattedText(SaveDirectory.slots[slotIndex].name);
        RenderFormattedText("\r"); // TEXT DS:47B4: CR before unit-name table; Locust starts at DS:47B6
    }

    menuItemCount = SAVE_SLOT_COUNT;
    renameItemIndex = menuItemCount;
    RenderFormattedText(STORY_TEXT(SHOW_SAVE_LOAD_MENU_0)); // TEXT DS:265B
    if (menuMode == SAVE_LOAD_MODE_LOAD)
        RenderFormattedText(STORY_TEXT(SHOW_SAVE_LOAD_MENU_1)); // TEXT DS:2662
    else
        RenderFormattedText(STORY_TEXT(SHOW_SAVE_LOAD_MENU_2)); // TEXT DS:2668
    RenderFormattedText(SaveGameSeparatorText);
    RenderFormattedText("\r"); // TEXT DS:47B4: CR before unit-name table; Locust starts at DS:47B6

    exitDosItemIndex = ++menuItemCount;
    RenderFormattedText(STORY_TEXT(SHOW_SAVE_LOAD_MENU_3)); // TEXT DS:26AF
    trainingItemIndex = -0x000A;
    if (menuMode == SAVE_LOAD_MODE_LOAD && SaveTransferState == 0x8000)
    {
        ++SaveLoadMenuDefaultSelection;
        trainingItemIndex = ++menuItemCount;
        RenderFormattedText(STORY_TEXT(SHOW_SAVE_LOAD_MENU_4)); // TEXT DS:26FA
    }
    cancelItemIndex = ++menuItemCount;
    RenderFormattedText(STORY_TEXT(SHOW_SAVE_LOAD_MENU_5)); // TEXT DS:2675
    RenderFormattedText("\r"); // TEXT DS:47B4: CR before unit-name table; Locust starts at DS:47B6
    ShowMouseCursorOverlay();
    UiRegionTable[0x0C].rowCount = menuItemCount + 0x0001;
    FlushPendingInput();
    selectedMenuItem = RunSingleUiRegionSelection(0x0C);

    if (selectedMenuItem < (int16_t) renameItemIndex)
    {
        selectedSlot = &SaveDirectory.slots[selectedMenuItem];
        SelectedSaveSlotIndex = (uint8_t) selectedMenuItem;
        if (menuMode == SAVE_LOAD_MODE_LOAD && !selectedSlot->occupied)
        {
            HideMouseCursorOverlay();
            RenderFormattedText(STORY_TEXT(SHOW_SAVE_LOAD_MENU_6)); // TEXT DS:263D
            ShowMouseCursorOverlay();
            WaitForAcknowledgement();
        }
        else
        {
            menuAction = SAVE_LOAD_ACTION_SLOT;
            if (menuMode == SAVE_LOAD_MODE_SAVE)
            {
                strcpy(previousSlotName, selectedSlot->name);
                directoryChanged = RenameSaveSlot(selectedMenuItem);
                if (directoryChanged != FALSE &&
                    (strcmp(previousSlotName, selectedSlot->name) == 0x0000 ||
                        IsBlankSaveSlotName(selectedSlot->name)))
                {
                    ActiveTextCursorColumn = 0x0000;
                    ActiveTextCursorRow = 0x0009;
                    HideMouseCursorOverlay();
                    previousBackground =
                        ActiveTextBackgroundColor;
                    ActiveTextBackgroundColor = 0x0C;
                    promptX = (int16_t) (ActiveTextWindowLeftTile << 0x03);
                    promptY = (int16_t) ((ActiveTextWindowTopTile << 0x03) +
                        0x0009 * ActiveFontGlyphAdvanceX);
                    RenderFormattedText(STORY_TEXT(SHOW_SAVE_LOAD_MENU_7)); // TEXT DS:26BC
                    XorVideoRectangle(promptX, promptY,
                        (uint16_t) (promptX +
                            ActiveFontGlyphAdvanceX *
                                (uint16_t) strlen(STORY_TEXT(SHOW_SAVE_LOAD_MENU_7)) -
                            0x0001),
                        (uint16_t) (promptY +
                            ActiveFontLineHeight - 0x0001),
                        0x0001);
                    ActiveTextBackgroundColor = previousBackground;
                    ShowMouseCursorOverlay();
                    if (PromptYesNo(0x0000) != FALSE)
                    {
                        strcpy(selectedSlot->name, previousSlotName);
                        directoryChanged = FALSE;
                        menuAction = SAVE_LOAD_ACTION_REOPEN_MENU;
                    }
                }
            }
            if (directoryChanged != FALSE)
                selectedSlot->occupied = TRUE;
        }
    }
    if (selectedMenuItem == (int16_t) renameItemIndex)
    {
        directoryChanged = RenameSaveSlot(SAVE_LOAD_ACTION_REOPEN_MENU);
        menuAction = SAVE_LOAD_ACTION_REOPEN_MENU;
    }
    if (selectedMenuItem == (int16_t) exitDosItemIndex)
        menuAction = SAVE_LOAD_ACTION_EXIT_DOS;
    if (selectedMenuItem == trainingItemIndex)
        menuAction = SAVE_LOAD_ACTION_TRAINING;
    if (selectedMenuItem == (int16_t) cancelItemIndex)
        menuAction = SAVE_LOAD_ACTION_CANCEL;
    if (directoryChanged != FALSE)
        WriteSaveSlot(0x0044, 0x0000);
    SaveLoadMenuDefaultSelection = previousDefaultSelection;
    return menuAction;
}

// 0D73:51F9: word16 IsBlankSaveSlotName(const char far *slotName)
// Called from:
//      ShowSaveLoadMenu
// VERIFIED against 0D73:51F9-5232.
uint16_t IsBlankSaveSlotName(const char *slotName)
{
    int16_t characterIndex;
    int16_t nameLength;
    uint16_t isBlank;

    isBlank = TRUE;
    nameLength = (int16_t) strlen(slotName);
    for (characterIndex = 0x0000;
        characterIndex < nameLength;
        ++characterIndex)
    {
        if (slotName[characterIndex] != ' ')
            isBlank = FALSE;
    }
    return isBlank;
}

// 0D73:5233: word16 RenameSaveSlot(int16 slotIndex)
// Called from:
//      ShowSaveLoadMenu
// VERIFIED against 0D73:5233-53E0.
uint16_t RenameSaveSlot(int16_t slotIndex)
{
    RevengeSaveSlotDescriptor *slot;
    int16_t promptX;
    int16_t promptY;
    uint16_t previousBackground;
    uint16_t wasRenamed;

    previousBackground = ActiveTextBackgroundColor;
    ActiveTextBackgroundColor = 0x0C;
    if (slotIndex < 0x0000)
    {
        ActiveTextCursorColumn = 0x0005;
        ActiveTextCursorRow = SaveLoadMenuDefaultSelection;
        promptX = (int16_t) ((ActiveTextWindowLeftTile << 0x03) +
            ActiveTextCursorColumn *
                ActiveFontGlyphAdvanceX);
        promptY = (int16_t) ((ActiveTextWindowTopTile << 0x03) +
            ActiveTextCursorRow *
                ActiveFontGlyphAdvanceX);
        HideMouseCursorOverlay();
        RenderFormattedText(STORY_TEXT(RENAME_SAVE_SLOT_0)); // TEXT DS:267D
        XorVideoRectangle(
            promptX,
            promptY,
            (uint16_t) (promptX +
                ActiveFontGlyphAdvanceX *
                    (uint16_t) strlen(STORY_TEXT(RENAME_SAVE_SLOT_0)) -
                0x0001),
            (uint16_t) (promptY +
                ActiveFontLineHeight -
                0x0001),
            0x0001); // TEXT DS:267D
        ShowMouseCursorOverlay();
        UiRegionTable[0x0C].rowCount = SAVE_SLOT_COUNT;
        UiRegionTable[0x0C].selectedRow = 0x0000;
        FlushPendingInput();
        slotIndex = RunSingleUiRegionSelection(0x0C);
    }

    wasRenamed = FALSE;
    if (slotIndex == KEY_ESCAPE)
        return wasRenamed;

    slot = &SaveDirectory.slots[slotIndex];
    ActiveTextCursorColumn = 0x0005;
    ActiveTextCursorRow = SaveLoadMenuDefaultSelection;
    promptX = (int16_t) ((ActiveTextWindowLeftTile << 0x03) +
        ActiveTextCursorColumn *
            ActiveFontGlyphAdvanceX);
    promptY = (int16_t) ((ActiveTextWindowTopTile << 0x03) +
        ActiveTextCursorRow *
            ActiveFontGlyphAdvanceX);
    HideMouseCursorOverlay();
    RenderFormattedText(STORY_TEXT(RENAME_SAVE_SLOT_1)); // TEXT DS:2696
    XorVideoRectangle(
        promptX,
        promptY,
        (uint16_t) (promptX +
            ActiveFontGlyphAdvanceX *
                (uint16_t) strlen(STORY_TEXT(RENAME_SAVE_SLOT_1)) -
            0x0001),
        (uint16_t) (promptY +
            ActiveFontLineHeight -
            0x0001),
        0x0001); // TEXT DS:2696
    ShowMouseCursorOverlay();
    ActiveTextCursorColumn = 0x000A;
    ActiveTextCursorRow = slotIndex;
    do
    {
        wasRenamed = EditTextField(0x0014, slot->name, 0x07);
    } while (wasRenamed == FALSE);
    strcpy(slot->name, (const char *) TextEntryBuffer);
    ActiveTextBackgroundColor = previousBackground;
    return TRUE;
}

// 0D73:53E1: void WaitForAcknowledgement(void)
// Called from:
//      ShowSaveLoadMenu
// VERIFIED against 0D73:53E1-546E.
void WaitForAcknowledgement(void)
{
    uint16_t bottomY;
    uint16_t leftX;
    uint16_t rightX;
    uint16_t topY;

    HideMouseCursorOverlay();
    if (ActiveTextCursorColumn != 0x0000)
        RenderFormattedText("\r"); // TEXT DS:47B4: CR before unit-name table; Locust starts at DS:47B6
    leftX = (uint16_t) ((ActiveTextWindowLeftTile << 0x03) +
        ActiveTextCursorColumn *
            ActiveFontGlyphAdvanceX);
    topY = (uint16_t) ((ActiveTextWindowTopTile << 0x03) +
        ActiveTextCursorRow *
            ActiveFontGlyphAdvanceX);
    RenderFormattedText(STORY_TEXT(WAIT_FOR_ACKNOWLEDGEMENT_0)); // TEXT DS:26DB
    rightX = (uint16_t) (leftX +
        ActiveFontGlyphAdvanceX *
            (uint16_t) strlen(STORY_TEXT(WAIT_FOR_ACKNOWLEDGEMENT_0)) -
        0x0001);
    bottomY = (uint16_t) (topY +
        ActiveFontLineHeight -
        0x0001);
    XorVideoRectangle(
        leftX,
        topY,
        rightX,
        bottomY,
        0x0001); // TEXT DS:26DB
    ShowMouseCursorOverlay();
    FlushPendingInput();
    WaitForDismissInput();
}

// 0D73:546F: int16 PromptYesNo(word16 initialSelection)
// Called from:
//      ShowSaveLoadMenu
// VERIFIED against 0D73:546F-54B8.
int16_t PromptYesNo(uint16_t initialSelection)
{
    HideMouseCursorOverlay();
    if (ActiveTextCursorColumn != 0x00)
        RenderFormattedText("\r"); // TEXT DS:47B4: CR before unit-name table; Locust starts at DS:47B6
    UiRegionTable[0x0D].topRow = ActiveTextCursorRow;
    UiRegionTable[0x0D].selectedRow = initialSelection;
    RenderFormattedText(STORY_TEXT(PROMPT_YES_NO_0)); // TEXT DS:26F2
    ShowMouseCursorOverlay();
    FlushPendingInput();
    return RunSingleUiRegionSelection(0x0D);
}

void UnitSelectionCard_Store(RevengeSavedSelectionCard *saved,
    const UnitSelectionCard *card)
{
    saved->left = card->left;
    saved->top = card->top;
    saved->width = card->width;
    saved->state = card->state;
    saved->persistentUnitSlot = card->persistentUnitSlot;
    saved->assignmentSlot = card->assignmentSlot;
    saved->selectionId = card->selectionId;
    saved->alternateId = card->alternateId;
    saved->spriteBuffer = card->loadedSpriteBuffer;
}
