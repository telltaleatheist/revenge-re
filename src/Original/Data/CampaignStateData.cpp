// 3A6A: campaign state
// CampaignStateData.cpp
// Typed objects recovered from REVENGE.EXE DS segment 3A6A.

#include "Data/CampaignStateData.h"

uint8_t CampaignClockState[0x03];
uint8_t SavedCampaignClockState[0x03];
uint16_t CampaignFlags;
uint16_t CampaignStageIndex;
uint16_t ScenarioUnitZeroAvailable;
int16_t ScenarioUnitFifteenState;
uint16_t ScenarioUnitFourteenEnabled;
uint16_t EjectedPilotRosterState;
uint16_t ScenarioUnitThirteenEnabled;
uint8_t StoryUnitDeployedFlag;
uint8_t SelectedSaveSlotIndex;
uint8_t SaveLoadMenuDefaultSelection;
uint8_t ScenarioRosterPrepared;
int8_t PendingCampaignStageDelta;
uint8_t PendingCampaignPhaseDelta;
int16_t ScenarioVariantIndex;
uint8_t CampaignPhase;
uint8_t ReplayCampaignStateRequested;
uint8_t TrainingSequenceFlag;
uint16_t DeferredStage16Story;
uint16_t SavedCampaignFlags;
uint8_t SuppressPostStageMenu;
uint8_t CampaignRestartState;
uint16_t ModalSavedTextWindow;
uint16_t ModalBackingX;
uint16_t ModalBackingY;
uint16_t ModalBackingWidth;
uint16_t ModalBackingHeight;
uint8_t ActivePilotListWindowHeight;
uint8_t CommanderListWindowHeight;
uint8_t FallenPilotListWindowHeight;
uint16_t SaveTransferState;
uint16_t SavedSaveTransferState;
uint16_t CampaignScenarioState;
uint8_t ReplayScenarioRequested;
uint8_t CampaignMenuRequested;
uint16_t ScenarioFileSuffix;
uint8_t CampaignUpgradeAwards[0x04];
uint8_t CampaignUpgradeMarkerX[0x04];
uint8_t CampaignUpgradeMarkerY[0x04];
uint8_t ScenarioDestructionCompletionState;
uint8_t ScenarioDestructionCompletionMarker;
uint16_t ProtectUnitZero;
uint16_t LrmShipmentLoaded;
uint16_t SrmShipmentLoaded;
uint8_t SceneTransitionAbortFlag;
/* DS:240F: filled at start from data/start/game.json (revenge/rules.h). */
uint16_t PresentationDismissTimeout;
uint16_t PresentationCallbackOffset;
uint16_t PresentationCallbackSegment;
