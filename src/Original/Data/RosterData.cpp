// 3A6A: roster data
// RosterData.cpp
// Typed objects recovered from REVENGE.EXE DS segment 3A6A.

#include "Data/RosterData.h"

RevengeUnitRecord PersistentUnitRoster[0x18];
RevengeUnitRecord BlankUnitRecord;
/* DS:1D4C, the pilots as the game starts them: from the player's copy
 * (src/Support/original_import_pieces.txt). */
/* Its starting records, from data/rules/pilots.json (revenge/rules.h); the
 * campaign changes each pilot's availability. */
RevengePilotRosterRecord PilotRoster[REVENGE_PILOT_ROSTER_COUNT];
/* DS:2148: what slot FF reads as a roster entry; from the player's copy. */
RevengePilotRosterRecord PilotRosterRecordPastEnd;
/* Filled at start from data/ui/lance_editor.json (the places) and
 * data/start/lance_editor.json (the starting state); revenge/rules.h. */
RevengeUnitLanceAssignment
    LanceAssignments[REVENGE_LANCE_ASSIGNMENT_COUNT];
uint8_t AlternateLanceSelection;
/* DS:1B8C-1B8E: filled at start from data/start/new_campaign.json
 * (revenge/rules.h). */
RandomPilotPoolCountCopies RandomPilotPoolCounts;
uint8_t UnitEditorAllegiance;
uint16_t UnitEditorScenarioGroupCount;
uint16_t UnitEditorScenarioCardCount;
uint16_t UnitEditorPilotCardCount;
uint16_t UnitEditorLanceGroupCount;
uint16_t SelectedUnitEquipmentLocation;
RevengeUnitRecord EstimatedUnitRecord;
