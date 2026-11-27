#ifndef REVENGE_ROSTER_H
#define REVENGE_ROSTER_H

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
    int8_t identityId;          /* +00 */
    int8_t experienceIndex;     /* +01 */
    int8_t gunneryIndex;        /* +02 */
    uint8_t availability;        /* +03 */
} RevengePilotRosterRecord;

/* RevengeUnitLanceAssignment.style's bits: under the cursor (the editor's
 * exit button state uses it too), a pilot assigned, a deployed unit
 * assigned. */
inline constexpr uint8_t LANCE_ASSIGNMENT_HIGHLIGHTED = 0x01;
inline constexpr uint8_t LANCE_ASSIGNMENT_HAS_PILOT = 0x02;
inline constexpr uint8_t LANCE_ASSIGNMENT_HAS_UNIT = 0x04;

/* RevengePilotRosterRecord.availability: in the lance and out of it for good. */
inline constexpr uint8_t REVENGE_PILOT_ACTIVE = 0x01;
inline constexpr uint8_t REVENGE_PILOT_FALLEN = 0xFF;

typedef struct RevengeUnitLanceAssignment
{
    uint16_t left;              /* +00 */
    uint16_t top;               /* +02 */
    uint16_t width;             /* +04 */
    uint8_t style;               /* +06 */
    RevengeUnitSlot persistentUnitSlot;  /* +07 */
    RevengeUnitSlot liveUnitSlot;        /* +08 */
    RevengePilotId pilotId;              /* +09 */
} RevengeUnitLanceAssignment;

/* A selection card as the image (DS:1BBC) and the save's snapshot lay it
 * out: its sprite is the original's far pointer. */
typedef struct RevengeSavedSelectionCard
{
    uint16_t left;
    uint16_t top;
    uint16_t width;
    uint8_t state;
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
    uint16_t left;
    uint16_t top;
    uint16_t width;
    uint8_t state;
    RevengeUnitSlot persistentUnitSlot;
    RevengeAssignmentSlot assignmentSlot;
    RevengePilotId selectionId;
    RevengePilotId alternateId;
    const uint8_t *spriteBuffer;
    DosAddress loadedSpriteBuffer;
} UnitSelectionCard;

/* The save boundary: a card from its saved record, and back. The sprite
 * pointer is stored as the card was loaded with it, from a save or the
 * image's start cards (0000:0000 in every one), never as one the editor set:
 * the host's pointer has no far address, and the original sets each card's
 * sprite before it draws the card, after a new campaign (its 0000:0000) and
 * after a load (another session's heap) alike, so it never reads the saved
 * one. */
void UnitSelectionCard_Load(UnitSelectionCard *card,
    const RevengeSavedSelectionCard *saved);
void UnitSelectionCard_Store(RevengeSavedSelectionCard *saved,
    const UnitSelectionCard *card);
/* The cards from the rules' starting records (UnitSelectionCardStart). */
void UnitSelectionCards_LoadStart(void);

static_assert(sizeof(RevengePilotRosterRecord) == 0x04,
    "Pilot roster records must retain their original four-byte layout.");
static_assert(offsetof(RevengePilotRosterRecord, availability) == 0x03,
    "Pilot availability must remain at record offset +03.");
static_assert(sizeof(RevengeUnitLanceAssignment) == 0x0A,
    "Lance assignments must retain their original ten-byte layout.");
static_assert(offsetof(RevengeUnitLanceAssignment, style) == 0x06,
    "Lance assignment style must remain at record offset +06.");
static_assert(offsetof(RevengeUnitLanceAssignment, persistentUnitSlot) == 0x07,
    "Persistent unit slot must remain at record offset +07.");
static_assert(offsetof(RevengeUnitLanceAssignment, liveUnitSlot) == 0x08,
    "Live unit slot must remain at record offset +08.");
static_assert(offsetof(RevengeUnitLanceAssignment, pilotId) == 0x09,
    "Pilot ID must remain at record offset +09.");
static_assert(sizeof(RevengeSavedSelectionCard) == 0x0F,
    "A saved selection card keeps its 0Fh-byte layout");
static_assert(offsetof(RevengeSavedSelectionCard, state) == 0x06,
    "A saved selection card's state is at offset 06h");
static_assert(offsetof(RevengeSavedSelectionCard, persistentUnitSlot) == 0x07,
    "A saved selection card's persistent slot is at offset 07h");
static_assert(offsetof(RevengeSavedSelectionCard, assignmentSlot) == 0x08,
    "A saved selection card's assignment slot is at offset 08h");
static_assert(offsetof(RevengeSavedSelectionCard, selectionId) == 0x09,
    "A saved selection card's selection ID is at offset 09h");
static_assert(offsetof(RevengeSavedSelectionCard, spriteBuffer) == 0x0B,
    "A saved selection card's sprite pointer is at offset 0Bh");

#endif
