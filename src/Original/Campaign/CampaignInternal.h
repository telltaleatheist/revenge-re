// 0D73: campaign declarations
#ifndef REVENGE_CAMPAIGN_CAMPAIGN_INTERNAL_H
#define REVENGE_CAMPAIGN_CAMPAIGN_INTERNAL_H

#include "revenge/original/resource_data.h"

void InitializeTacticalScenario(void);
#include "revenge/original/campaign_data.h"
#include "revenge/original/lance_editor_data.h"
#include "revenge/input.h"
#include "revenge/save_data.h"
#include "revenge/video.h"

/* A narrow declaration rather than the whole backend header. */
void SDLBackend_ServiceTimers(void);

#define CACHED_FILE_NAME(index) (CachedFileDescriptors[(index)].fileName)

inline constexpr int SAVE_SLOT_COUNT = 0x06;
inline constexpr uint16_t SAVE_LOAD_MODE_SAVE = 0x03;
inline constexpr uint16_t SAVE_LOAD_MODE_LOAD = 0x04;
inline constexpr int16_t SAVE_LOAD_ACTION_CANCEL = 0x00;
inline constexpr int16_t SAVE_LOAD_ACTION_SLOT = 0x01;
inline constexpr int16_t SAVE_LOAD_ACTION_TRAINING = 0x02;
inline constexpr int16_t SAVE_LOAD_ACTION_REOPEN_MENU = -0x01;
inline constexpr int16_t SAVE_LOAD_ACTION_EXIT_DOS = -0x02;
inline constexpr int16_t UI_SELECTION_PENDING = 0x00FF;
inline constexpr uint16_t IN_GAME_MENU_CONTINUE = 0x00;
inline constexpr uint16_t IN_GAME_MENU_JUMP_STORY = 0x01;
inline constexpr uint16_t IN_GAME_MENU_SAVE_CONTINUE = 0x02;
inline constexpr uint16_t IN_GAME_MENU_SAVE_QUIT = 0x03;
inline constexpr uint16_t IN_GAME_MENU_LOAD = 0x04;
inline constexpr uint16_t IN_GAME_MENU_REPLAY_SCENARIO = 0x05;
inline constexpr uint16_t IN_GAME_MENU_REPLAY_STORY = 0x06;
inline constexpr uint16_t IN_GAME_MENU_RESTART = 0x07;
inline constexpr uint16_t IN_GAME_MENU_OPTIONS = 0x08;
inline constexpr uint16_t IN_GAME_MENU_OPTION_COUNT = 0x09;

typedef int16_t (*CampaignStageHandler)(void);

#define DECLARE_CAMPAIGN_STAGE(index) \
    int16_t RunCampaignStage##index(void)
DECLARE_CAMPAIGN_STAGE(00);
DECLARE_CAMPAIGN_STAGE(01);
DECLARE_CAMPAIGN_STAGE(02);
DECLARE_CAMPAIGN_STAGE(03);
DECLARE_CAMPAIGN_STAGE(04);
DECLARE_CAMPAIGN_STAGE(05);
DECLARE_CAMPAIGN_STAGE(06);
DECLARE_CAMPAIGN_STAGE(07);
DECLARE_CAMPAIGN_STAGE(08);
DECLARE_CAMPAIGN_STAGE(09);
DECLARE_CAMPAIGN_STAGE(10);
DECLARE_CAMPAIGN_STAGE(11);
DECLARE_CAMPAIGN_STAGE(12);
DECLARE_CAMPAIGN_STAGE(13);
DECLARE_CAMPAIGN_STAGE(14);
DECLARE_CAMPAIGN_STAGE(15);
DECLARE_CAMPAIGN_STAGE(16);
DECLARE_CAMPAIGN_STAGE(17);
DECLARE_CAMPAIGN_STAGE(18);
DECLARE_CAMPAIGN_STAGE(19);
DECLARE_CAMPAIGN_STAGE(20);
DECLARE_CAMPAIGN_STAGE(21);
#undef DECLARE_CAMPAIGN_STAGE

int16_t RunSaveLoadMenu(uint16_t menuMode);
int16_t ShowSaveLoadMenu(uint16_t menuMode);
uint16_t IsBlankSaveSlotName(const char *slotName);
uint16_t RenameSaveSlot(int16_t slotIndex);
void WaitForAcknowledgement(void);
int16_t PromptYesNo(uint16_t initialSelection);
void ShowStoryMessageAndWait(uint16_t messageIndex);
void ShowStoryMessage(uint16_t messageIndex);
void StopLoadedMusicTrack(void);
void LoadScenarioDigitalSoundBank(int8_t scenarioCode);
void ReleaseDigitalSoundBuffer(void);
void DispatchWeaponEffect(int16_t weaponEffectId);
void LoadUnitTemplateRecord(int16_t unitType);
void PlayScenarioDigitalSoundSequence(int8_t scenarioCode);
void PromoteReserveUnitsForCampaignStage(void);
uint16_t WaitForDismissInput(void);
uint16_t WaitForKeyboardOrMouseInput(void);
void FlushPendingInput(void);
uint16_t ServiceMusicAndCheckKeyboardInput(void);
void CopyClippedVideoRegion(
    int16_t sourceX,
    int16_t sourceY,
    int16_t destinationX,
    int16_t destinationY,
    uint16_t widthColumns,
    uint16_t heightPixels,
    const uint8_t *source,
    uint8_t *destination);
void XorVideoRectangle(
    int16_t leftX, int16_t topY, int16_t rightX, int16_t bottomY, uint8_t color);
uint16_t ReadSaveSlot(uint16_t cachedFileIndex, int16_t saveSlot);
uint16_t WriteSaveSlot(uint16_t cachedFileIndex, int16_t saveSlot);

extern void (*PresentationCallback)(void);
void RunCampaignPathSelectionCallback(void);
void RunScenarioVariantSelectionCallback(void);
void RunAlternateScenarioVariantSelectionCallback(void);
void LoadDigitalWeaponEffectBank(void);
void ReleaseDigitalWeaponEffectBank(void);
void PrepareLanceSelection(uint16_t firstRosterIndex, int16_t selectionMode);
void RunLanceConfiguration(
    uint16_t selectionFamily,
    uint16_t scenarioVariantIndex);
void EnableTrainingUnitSelectionSlots(void);
int16_t RunTrainingScenarioSelection(void);
int16_t PrepareTrainingDeployment(void);
void AssignPilotRosterEntryToUnit(
    uint16_t unitIndex,
    uint16_t rosterSlot);
uint8_t PrepareActivePilotList(void);
void RenderActivePilotList(void);
uint8_t PrepareSurvivingCommanderList(void);
void RenderSurvivingCommanderList(void);
uint16_t HasFallenPilots(void);
uint8_t PrepareFallenPilotList(void);
void RenderFallenPilotList(void);
void RenderPilotName(RevengePilotId pilotId);
void BuildMissionSpriteBackingBuffer(uint16_t spriteLayoutIndex);
void FreeGraphicsBufferRange(
    int16_t firstBufferIndex,
    int16_t lastBufferIndex);
void UpdatePilotAvailability(void);
int16_t RunTitleAndMainMenu(void);
int16_t RunInGameMenu(int16_t firstMenuOption);
/* Ours (upgrade_restart_pilots): the new campaign's replay pilots for a
 * Restart (CampaignMenus.cpp). */
void ApplyRestartPilotsUpgrade(void);
uint16_t SaveGame(void);
uint16_t LoadGame(void);
void OpenModalTextWindow(uint16_t windowIndex, uint16_t clearAndFrame);
void RestoreModalTextWindow(void);
uint16_t RunGameOptionsMenu(uint16_t embeddedMode);
uint8_t MissionFadeColor(uint16_t fadeIndex);
uint16_t AnimateTitleTextLine(const char *text);
void PresentAnimatedScreenAndMessage(
    uint16_t screenResourceIndex,
    uint16_t messageIndex);
void PresentStoryScreenAndMessage(
    uint16_t graphicsResourceIndex,
    uint16_t messageIndex);
void PresentCampaignScene(
    uint16_t graphicsResourceIndex,
    uint16_t spriteLayoutIndex,
    uint16_t messageIndex);
void ShowCampaignMessageAndWait(uint16_t messageIndex);
void PresentMissionBriefing(
    uint16_t graphicsResourceIndex,
    uint16_t spriteLayoutIndex,
    uint16_t messageIndex);
void DisplayMissionDate(
    int16_t presentationMode,
    int16_t blinkRegionIndex,
    uint16_t dateEntryIndex,
    uint16_t followupMessageIndex,
    uint16_t redrawPresentation);
int16_t SelectScenarioVariant(uint16_t presentationMode);
uint16_t RunTwoOptionMenuSelection(
    const char *const menuOptionText[0x02]);
uint16_t RunMainMenuSelection(const char *const menuOptionText[0x03]);
uint16_t PlayTitleTextSequence(void);

#endif
