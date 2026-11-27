// 3A6A: campaign state declarations
#ifndef REVENGE_DATA_CAMPAIGN_STATE_DATA_H
#define REVENGE_DATA_CAMPAIGN_STATE_DATA_H

#include "revenge/types.h"

/* DS:8337-8339: the hour, the minute and the battle's subtick, the byte the
 * battle loop steps; the scenario's saved clock (DS:855F) is the same three. */
extern uint8_t CampaignClockState[0x03];

#define TacticalClockSubtick (CampaignClockState[0x02])

extern uint8_t SavedCampaignClockState[0x03];
extern uint16_t CampaignFlags;
/* CampaignFlags' bits, and SavedCampaignFlags' (the flags the stage began
 * with). The battle bits are set as a battle ends (BattleRuntime.cpp); the
 * milestone bits by the scene's CAMPAIGN_MILESTONE selectors
 * (SceneInterpreter.cpp). 0010h (scenario 37h, unit 1 out) and 8000h stay
 * numbers: see docs/UNNAMED_VALUES.md. */
/* The battle ended with result 2, which the battle sets when every enemy
 * unit is out or immobile and a scene sets to end the battle; every stage
 * takes it as the way forward. */
inline constexpr uint16_t CAMPAIGN_FLAG_BATTLE_WON = 0x0001;
/* Unit 0 was out at the battle's end, the in-battle menu left the battle, or
 * stage 20's battle wasn't won; every stage takes it as the replay. */
inline constexpr uint16_t CAMPAIGN_FLAG_BATTLE_LOST = 0x0002;
/* Unit 0 out (in scenario 35h, immobile) and the last attacking unit a
 * BattleMech. */
inline constexpr uint16_t CAMPAIGN_FLAG_LOST_TO_BATTLEMECH = 0x0004;
/* Unit 1 out at the battle's end. */
inline constexpr uint16_t CAMPAIGN_FLAG_UNIT_1_LOST = 0x0008;
/* The ejected pilot's unit still deployed and mobile (milestone 3). */
inline constexpr uint16_t CAMPAIGN_FLAG_EJECTED_PILOT_ALIVE = 0x0020;
/* The battle ended at hour 12 or later, or at 07:30 or later in scenarios 36h
 * and 37h. */
inline constexpr uint16_t CAMPAIGN_FLAG_CLOCK_PAST_LIMIT = 0x0040;
/* Milestone 0; stages 4-6, 11 and 12 also set it when unit 8 is out. */
inline constexpr uint16_t CAMPAIGN_FLAG_APC_BRIDGE = 0x0080;
inline constexpr uint16_t CAMPAIGN_FLAG_MILESTONE_UNKNOWN_04 = 0x0100;
/* Scenario 53h's battle clears it when it sets COMMANDER_CRIPPLED. */
inline constexpr uint16_t CAMPAIGN_FLAG_MILESTONE_UNKNOWN_05 = 0x0200;
inline constexpr uint16_t CAMPAIGN_FLAG_COMMANDER_CRIPPLED = 0x0400;
inline constexpr uint16_t CAMPAIGN_FLAG_SRM_SHIPMENT = 0x0800;
inline constexpr uint16_t CAMPAIGN_FLAG_LRM_SHIPMENT = 0x1000;
/* The stage began with UNIT_1_LOST in SavedCampaignFlags. */
inline constexpr uint16_t CAMPAIGN_FLAG_UNIT_1_LOST_BEFORE = 0x2000;
inline constexpr uint16_t CAMPAIGN_FLAG_MECH_SHOP_REACHED = 0x4000;
extern uint16_t CampaignStageIndex;
extern uint16_t ScenarioUnitZeroAvailable;
extern int16_t ScenarioUnitFifteenState;
extern uint16_t ScenarioUnitFourteenEnabled;
extern uint16_t EjectedPilotRosterState;
extern uint16_t ScenarioUnitThirteenEnabled;
extern uint8_t StoryUnitDeployedFlag;
extern uint8_t SelectedSaveSlotIndex;
extern uint8_t SaveLoadMenuDefaultSelection;
extern uint8_t ScenarioRosterPrepared;
extern int8_t PendingCampaignStageDelta;
extern uint8_t PendingCampaignPhaseDelta;
extern int16_t ScenarioVariantIndex;
extern uint8_t CampaignPhase;
extern uint8_t ReplayCampaignStateRequested;
extern uint8_t TrainingSequenceFlag;
extern uint16_t DeferredStage16Story;
extern uint16_t SavedCampaignFlags;
extern uint8_t SuppressPostStageMenu;
extern uint8_t CampaignRestartState;
extern uint16_t ModalSavedTextWindow;
extern uint16_t ModalBackingX;
extern uint16_t ModalBackingY;
extern uint16_t ModalBackingWidth;
extern uint16_t ModalBackingHeight;
extern uint8_t ActivePilotListWindowHeight;
extern uint8_t CommanderListWindowHeight;
extern uint8_t FallenPilotListWindowHeight;
extern uint16_t SaveTransferState;
extern uint16_t SavedSaveTransferState;
extern uint16_t CampaignScenarioState;
extern uint8_t ReplayScenarioRequested;
extern uint8_t CampaignMenuRequested;
extern uint16_t ScenarioFileSuffix;
extern uint8_t CampaignUpgradeAwards[0x04];
extern uint8_t CampaignUpgradeMarkerX[0x04];
extern uint8_t CampaignUpgradeMarkerY[0x04];
extern uint8_t ScenarioDestructionCompletionState;
extern uint8_t ScenarioDestructionCompletionMarker;
extern uint16_t ProtectUnitZero;
extern uint16_t LrmShipmentLoaded;
extern uint16_t SrmShipmentLoaded;
extern uint8_t SceneTransitionAbortFlag;
extern uint16_t PresentationDismissTimeout;
extern uint16_t PresentationCallbackOffset;
extern uint16_t PresentationCallbackSegment;
/* The far callback's values: the offsets in segment 0D73 of the procedures it
 * names (src/Support/presentation_callback_bridge.cpp calls them). */
inline constexpr uint16_t PRESENTATION_CALLBACK_SEGMENT = 0x0D73;
inline constexpr uint16_t PRESENTATION_CALLBACK_ACTIVE_PILOT_LIST = 0x214B;
inline constexpr uint16_t PRESENTATION_CALLBACK_SURVIVING_COMMANDER_LIST = 0x223D;
inline constexpr uint16_t PRESENTATION_CALLBACK_FALLEN_PILOT_LIST = 0x2361;
inline constexpr uint16_t PRESENTATION_CALLBACK_CAMPAIGN_PATH_SELECTION = 0x249E;
inline constexpr uint16_t PRESENTATION_CALLBACK_SCENARIO_VARIANT_SELECTION = 0x24B3;
inline constexpr uint16_t PRESENTATION_CALLBACK_ALTERNATE_SCENARIO_VARIANT_SELECTION = 0x24C9;

#endif
