// 3A6A: lance editor data
// LanceEditorData.cpp
// Canonical C objects for the unit and pilot editor's DS:3A6A data.

#include "revenge/original/lance_editor_data.h"

/* Filled at start from data/ui/lance_editor.json (the places) and
 * data/start/lance_editor.json (the starting state); revenge/rules.h. */
UnitSelectionCard UnitSelectionCards[UNIT_SELECTION_CARD_COUNT];
RevengeSavedSelectionCard UnitSelectionCardStart[UNIT_SELECTION_CARD_COUNT];
word16 LanceEditorFirstBriefingPending;
word16 LanceEditorBriefingPresented;
/* Filled at start from data/start/lance_editor.json (revenge/rules.h). */
word16 LanceEditorReloadGraphics;


/* Filled at start from data/start/lance_editor.json (revenge/rules.h). */
word16 UnitEditorLanceSlotCount[0x03];
byte RandomUnitTypeCount;
/* Filled at start from data/ui/lance_editor.json (revenge/rules.h). */
byte UnitEditorUnitSpriteIndex[LANCE_EDITOR_UNIT_TYPE_COUNT];
/* Taken from the player's copy at start (src/Support/original_import_pieces.txt). */
byte UnitNameSplitIndex[LANCE_EDITOR_UNIT_TYPE_COUNT];
/* Filled at start from data/rules/units.json (revenge/rules.h). */
byte PredefinedUnitTypeTable[0x1E];
/* Filled at start from data/start/lance_editor.json (revenge/rules.h). */
byte UnitEditorSectionEnabled[0x1D];
/* Filled at start from data/rules/campaign.json (revenge/rules.h). */
byte UnitEditorScenarioCardCountByFamily[0x07];
/* Filled at start from data/rules/units.json (revenge/rules.h). */
byte RandomUnitTypeCountByFamily[0x0A];
byte RandomUnitTypePoolByFamily[0x31];
/* Filled at start from data/ui/lance_editor.json (revenge/rules.h). */
byte PilotPortraitIndexById[0x21];
/* Filled at start from data/rules/pilots.json (revenge/rules.h). */
word16 DefaultPilotIdsByScenario[0x14];
/* Filled at start from data/rules/campaign.json (revenge/rules.h). */
byte UnitEditorPilotCardCountByFamily[0x07];

/* Filled at start from data/rules/pilots.json (revenge/rules.h). */
byte FixedPilotIds[0x02];
/* Filled at start from data/rules/units.json (revenge/rules.h). */
byte FixedUnitTypeIds[0x02];
/* The training's unit lists by class, each ended by FFh (DS:3298), filled at
 * start from data/rules/units.json; UnitClassTypeLists points into it. */
byte UnitClassTypeListBytes[0x44];

/* Filled at start from data/ui/lance_editor.json (revenge/rules.h). */
LanceEditorLayoutData LanceEditorLayout;

void UnitSelectionCard_Load(UnitSelectionCard *card,
    const RevengeSavedSelectionCard *saved)
{
    card->left = saved->left;
    card->top = saved->top;
    card->width = saved->width;
    card->state = saved->state;
    card->persistentUnitSlot = saved->persistentUnitSlot;
    card->assignmentSlot = saved->assignmentSlot;
    card->selectionId = saved->selectionId;
    card->alternateId = saved->alternateId;
    card->spriteBuffer = NULL;
    card->loadedSpriteBuffer = saved->spriteBuffer;
}

void UnitSelectionCards_LoadStart(void)
{
    word16 index;

    for (index = 0x0000; index < UNIT_SELECTION_CARD_COUNT; ++index)
        UnitSelectionCard_Load(&UnitSelectionCards[index],
            &UnitSelectionCardStart[index]);
}
