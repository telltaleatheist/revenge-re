// 3A6A: hit location data declarations
#ifndef REVENGE_DATA_HIT_LOCATION_DATA_H
#define REVENGE_DATA_HIT_LOCATION_DATA_H

#include "revenge/combat_data.h"
#include "revenge/types.h"

extern uint16_t HitLocationDirectionSelectorTable[0x10];  /* the rules' */
extern uint16_t HitLocationTable[HIT_LOCATION_TABLE_COUNT]
                                [HIT_LOCATION_TABLE_WORD_COUNT];  /* the rules' */

#endif
