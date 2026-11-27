// 3A6A: hit location data
// BattleTech hit-location tables recovered from REVENGE.EXE DS:1038/3CFE.

#include "revenge/original_data.h"

/* Filled at start from data/rules/combat.json (revenge/rules.h). */
uint16_t HitLocationDirectionSelectorTable[0x10];

/* Five tables of eleven armour location codes, one for each 2d6 roll from 2
 * to 12, and a zero word after each; the fifth is the infantry table
 * (HIT_LOCATION_TABLE_INFANTRY). Filled at start from data/rules/combat.json
 * (revenge/rules.h). */
uint16_t HitLocationTable[HIT_LOCATION_TABLE_COUNT][HIT_LOCATION_TABLE_WORD_COUNT];
