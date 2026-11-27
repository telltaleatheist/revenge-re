#ifndef REVENGE_TACTICAL_PANEL_DATA_H
#define REVENGE_TACTICAL_PANEL_DATA_H

#include "revenge/types.h"

extern uint8_t MovementRatingByWalkMP[0x09];  /* the rules' */
extern const char *UnitNameTable[0x59];
extern const char *PilotSurnameTable[0x3B];
extern const char *UnitAffiliationNameTable[0x04];
extern const char *PilotExperienceNameTable[0x04];
extern const char *HeatScaleDescriptionTable[0x07];
extern const char *MovementDescriptionTable[0x05];
extern const char *DurabilityDescriptionTable[0x04];
extern const char *ConditionDescriptionTable[0x05];
extern const char *FriendlyEquipmentDescriptionTable[0x02];
extern const char *EnemyAssessmentTextTable[0x12];

#endif
