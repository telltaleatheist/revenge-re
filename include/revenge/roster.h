#ifndef REVENGE_ROSTER_H
#define REVENGE_ROSTER_H

#include "revenge/far_memory.h"
#include "revenge/types.h"

#include <stddef.h>

enum
{
    REVENGE_PILOT_ROSTER_COUNT = 0x3B,
    REVENGE_PLAYER_PILOT_COUNT = 0x25,
    REVENGE_LANCE_ASSIGNMENT_COUNT = 0x0D
};

#pragma pack(push, 1)
typedef struct RevengePilotRosterRecord
{
    int8 identityId;          /* +00 */
    int8 experienceIndex;     /* +01 */
    int8 gunneryIndex;        /* +02 */
    byte availability;        /* +03 */
} RevengePilotRosterRecord;

typedef struct RevengeUnitLanceAssignment
{
    uint16 left;              /* +00 */
    uint16 top;               /* +02 */
    uint16 width;             /* +04 */
    byte style;               /* +06 */
    RevengeUnitSlot persistentUnitSlot;  /* +07 */
    RevengeUnitSlot liveUnitSlot;        /* +08 */
    RevengePilotId pilotId;              /* +09 */
} RevengeUnitLanceAssignment;

/* A selection card as the image (DS:1BBC) and the save's snapshot lay it
 * out: its sprite is the original's far pointer. */
typedef struct RevengeSavedSelectionCard
{
    word16 left;
    word16 top;
    word16 width;
    byte state;
    RevengeUnitSlot persistentUnitSlot;
    RevengeAssignmentSlot assignmentSlot;
    RevengePilotId selectionId;
    RevengePilotId alternateId;
    DosAddress spriteBuffer;
} RevengeSavedSelectionCard;
#pragma pack(pop)

/* A selection card as the game holds it. The sprite is the editor's buffer
 * the card is drawn from, set before the card is drawn; a card a load
 * brought has none, and keeps the far pointer the save held to write it
 * back as it was (UnitSelectionCard_Store). */
typedef struct UnitSelectionCard
{
    word16 left;
    word16 top;
    word16 width;
    byte state;
    RevengeUnitSlot persistentUnitSlot;
    RevengeAssignmentSlot assignmentSlot;
    RevengePilotId selectionId;
    RevengePilotId alternateId;
    const byte *spriteBuffer;
    DosAddress loadedSpriteBuffer;
} UnitSelectionCard;

/* The save boundary: a card from its saved record, and back. A sprite the
 * editor set is stored as its far address in the DOS memory block. */
void UnitSelectionCard_Load(UnitSelectionCard *card,
    const RevengeSavedSelectionCard *saved);
void UnitSelectionCard_Store(RevengeSavedSelectionCard *saved,
    const UnitSelectionCard *card);
/* The cards from the rules' starting records (UnitSelectionCardStart). */
void UnitSelectionCards_LoadStart(void);

_Static_assert(sizeof(RevengePilotRosterRecord) == 0x04,
    "Pilot roster records must retain their original four-byte layout.");
_Static_assert(offsetof(RevengePilotRosterRecord, availability) == 0x03,
    "Pilot availability must remain at record offset +03.");
_Static_assert(sizeof(RevengeUnitLanceAssignment) == 0x0A,
    "Lance assignments must retain their original ten-byte layout.");
_Static_assert(offsetof(RevengeUnitLanceAssignment, style) == 0x06,
    "Lance assignment style must remain at record offset +06.");
_Static_assert(offsetof(RevengeUnitLanceAssignment, persistentUnitSlot) == 0x07,
    "Persistent unit slot must remain at record offset +07.");
_Static_assert(offsetof(RevengeUnitLanceAssignment, liveUnitSlot) == 0x08,
    "Live unit slot must remain at record offset +08.");
_Static_assert(offsetof(RevengeUnitLanceAssignment, pilotId) == 0x09,
    "Pilot ID must remain at record offset +09.");
_Static_assert(sizeof(RevengeSavedSelectionCard) == 0x0F,
    "A saved selection card keeps its 0Fh-byte layout");
_Static_assert(offsetof(RevengeSavedSelectionCard, state) == 0x06,
    "A saved selection card's state is at offset 06h");
_Static_assert(offsetof(RevengeSavedSelectionCard, persistentUnitSlot) == 0x07,
    "A saved selection card's persistent slot is at offset 07h");
_Static_assert(offsetof(RevengeSavedSelectionCard, assignmentSlot) == 0x08,
    "A saved selection card's assignment slot is at offset 08h");
_Static_assert(offsetof(RevengeSavedSelectionCard, selectionId) == 0x09,
    "A saved selection card's selection ID is at offset 09h");
_Static_assert(offsetof(RevengeSavedSelectionCard, spriteBuffer) == 0x0B,
    "A saved selection card's sprite pointer is at offset 0Bh");

#endif
