// REVENGE_3A6A_LANCE_EDITOR_DATA.c
// Canonical C objects for the unit and pilot editor's DS:3A6A data.

#include "revenge/original/lance_editor_data.h"

UnitSelectionCard UnitSelectionCards[UNIT_SELECTION_CARD_COUNT];
word16 LanceEditorFirstBriefingPending;
word16 LanceEditorBriefingPresented;
word16 LanceEditorReloadGraphics;


word16 UnitEditorLanceSlotCount[0x03];
byte RandomUnitTypeCount;
byte UnitEditorUnitSpriteIndex[LANCE_EDITOR_UNIT_TYPE_COUNT];
byte UnitMovementClassByType[LANCE_EDITOR_UNIT_TYPE_COUNT];
/* Filled at start from data/rules/units.json (revenge/rules.h). */
byte PredefinedUnitTypeTable[0x1E];
byte UnitEditorSectionEnabled[0x1D];
byte UnitEditorScenarioCardCountByFamily[0x07];
/* Filled at start from data/rules/units.json (revenge/rules.h). */
byte RandomUnitTypeCountByFamily[0x0A];
byte RandomUnitTypePoolByFamily[0x31];
byte PilotPortraitIndexById[0x21];
/* Filled at start from data/rules/pilots.json (revenge/rules.h). */
word16 DefaultPilotIdsByScenario[0x14];
byte UnitEditorPilotCardCountByFamily[0x07];

/* Filled at start from data/rules/pilots.json (revenge/rules.h). */
byte FixedPilotIds[0x02];
/* Filled at start from data/rules/units.json (revenge/rules.h). */
byte FixedUnitTypeIds[0x02];
/* The training's unit lists by class, each ended by FFh (DS:3298), filled at
 * start from data/rules/units.json; UnitClassTypeLists points into it. */
byte UnitClassTypeListBytes[0x44];

LanceEditorLayoutData LanceEditorLayout;
