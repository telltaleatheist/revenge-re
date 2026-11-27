// 3A6A: roster data declarations
#ifndef REVENGE_DATA_ROSTER_DATA_H
#define REVENGE_DATA_ROSTER_DATA_H

#include "revenge/roster.h"
#include "revenge/types.h"
#include "revenge/unit.h"

extern RevengeUnitRecord PersistentUnitRoster[0x18];
extern RevengeUnitRecord BlankUnitRecord;
extern RevengePilotRosterRecord PilotRoster[REVENGE_PILOT_ROSTER_COUNT];
/* DS:2148, the roster entry past its end that slot FF reads (1A1F:339F,
 * 30F1:0AFC): in the image, four bytes of a table of file names. From the
 * player's copy (src/Support/original_import_pieces.txt). */
extern RevengePilotRosterRecord PilotRosterRecordPastEnd;
extern RevengeUnitLanceAssignment
    LanceAssignments[REVENGE_LANCE_ASSIGNMENT_COUNT];
extern uint8_t AlternateLanceSelection;

/* DS:1B8C-1B8E: the random pilot pool's count, in the campaign's three
 * copies of its state, as the pool has them (InitialExtendedCampaignState,
 * ForwardExtendedCampaignState and ReplayExtendedCampaignState). The
 * replay copy is the live count. */
typedef struct RandomPilotPoolCountCopies
{
    uint8_t initial; /* DS:1B8C */
    uint8_t forward; /* DS:1B8D */
    uint8_t replay; /* DS:1B8E */
} RandomPilotPoolCountCopies;
static_assert(sizeof(RandomPilotPoolCountCopies) == 0x03,
    "DS:1B8C-1B8E holds the three copies of the count.");

extern RandomPilotPoolCountCopies RandomPilotPoolCounts;
extern uint8_t UnitEditorAllegiance;
extern uint16_t UnitEditorScenarioGroupCount;
extern uint16_t UnitEditorScenarioCardCount;
extern uint16_t UnitEditorPilotCardCount;
extern uint16_t UnitEditorLanceGroupCount;

/* DS:1D42 is LanceAssignments[0x0C], not a second object. */
#define UnitEditorExitButtonLeft \
    (LanceAssignments[0x0C].left)
#define UnitEditorExitButtonTop \
    (LanceAssignments[0x0C].top)
#define UnitEditorExitButtonState \
    (LanceAssignments[0x0C].style)
extern uint16_t SelectedUnitEquipmentLocation;
extern RevengeUnitRecord EstimatedUnitRecord;

#endif
