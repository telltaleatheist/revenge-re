// 3A6A: lance editor data
// LanceEditorData.cpp
// Canonical C objects for the unit and pilot editor's DS:3A6A data.

#include "revenge/original/lance_editor_data.h"

/* Filled at start from data/ui/lance_editor.json (the places) and
 * data/start/lance_editor.json (the starting state); revenge/rules.h. */
UnitSelectionCard UnitSelectionCards[UNIT_SELECTION_CARD_COUNT];
RevengeSavedSelectionCard UnitSelectionCardStart[UNIT_SELECTION_CARD_COUNT];
uint16_t LanceEditorFirstBriefingPending;
uint16_t LanceEditorBriefingPresented;
/* Filled at start from data/start/lance_editor.json (revenge/rules.h). */
uint16_t LanceEditorReloadGraphics;


/* Filled at start from data/start/lance_editor.json (revenge/rules.h). */
uint16_t UnitEditorLanceSlotCount[0x03];
uint8_t RandomUnitTypeCount;
/* Filled at start from data/ui/lance_editor.json (revenge/rules.h). */
uint8_t UnitEditorUnitSpriteIndex[LANCE_EDITOR_UNIT_TYPE_COUNT];
/* Taken from the player's copy at start (src/Support/original_import_pieces.txt). */
uint8_t UnitNameSplitIndex[LANCE_EDITOR_UNIT_TYPE_COUNT];
/* Filled at start from data/rules/units.json (revenge/rules.h). */
uint8_t PredefinedUnitTypeTable[0x1E];
/* Filled at start from data/start/lance_editor.json (revenge/rules.h). */
uint8_t UnitEditorSectionEnabled[0x1D];
/* Filled at start from data/rules/campaign.json (revenge/rules.h). */
uint8_t UnitEditorScenarioCardCountByFamily[0x07];
/* Filled at start from data/rules/units.json (revenge/rules.h). */
uint8_t RandomUnitTypeCountByFamily[0x0A];
uint8_t RandomUnitTypePoolByFamily[0x31];
/* Filled at start from data/ui/lance_editor.json (revenge/rules.h). */
uint8_t PilotPortraitIndexById[0x21];
/* Filled at start from data/rules/pilots.json (revenge/rules.h). */
uint16_t DefaultPilotIdsByScenario[0x14];
/* Filled at start from data/rules/campaign.json (revenge/rules.h). */
uint8_t UnitEditorPilotCardCountByFamily[0x07];

/* Filled at start from data/rules/pilots.json (revenge/rules.h). */
uint8_t FixedPilotIds[0x02];
/* Filled at start from data/rules/units.json (revenge/rules.h). */
uint8_t FixedUnitTypeIds[0x02];
/* The training's unit lists by class, each ended by FFh (DS:3298), filled at
 * start from data/rules/units.json; UnitClassTypeLists points into it. */
uint8_t UnitClassTypeListBytes[0x44];

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
    uint16_t index;

    for (index = 0x0000; index < UNIT_SELECTION_CARD_COUNT; ++index)
        UnitSelectionCard_Load(&UnitSelectionCards[index],
            &UnitSelectionCardStart[index]);
}
