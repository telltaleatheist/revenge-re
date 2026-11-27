#ifndef REVENGE_PILOT_H
#define REVENGE_PILOT_H

#include "revenge/types.h"
#include "revenge/unit.h"
#include "revenge/roster.h"

const RevengePilotRosterRecord *GetPilotRosterRecord(
    uint16_t rosterSlot);
void InitializePilotRosterMetadata(
    uint8_t pilotRoster[REVENGE_PILOT_ROSTER_COUNT][0x04]);
void AssignPilotRosterEntryToUnit(
    uint8_t unitRecord[UNIT_RECORD_SIZE],
    uint16_t rosterSlot);
const char *GetPilotName(uint16_t identityId);
const char *GetPilotGivenName(uint16_t identityId);
const char *GetPilotExperienceName(uint16_t experienceIndex);
const char *GetPilotGunneryName(uint16_t gunneryIndex);

#endif
