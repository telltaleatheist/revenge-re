// 4184: tactical graphics data declarations
#ifndef REVENGE_DATA_TACTICAL_GRAPHICS_DATA_H
#define REVENGE_DATA_TACTICAL_GRAPHICS_DATA_H

#include "revenge/types.h"

extern uint16_t ActiveVideoAdapterType;
extern uint16_t EncodedSpriteClipLeftTile;
extern uint16_t SpriteClipRightTile;
extern uint16_t SpriteClipTopPixel;
extern uint16_t EncodedSpriteClipBottomPixel;
extern uint16_t SpritePreserveMode;
extern uint8_t WeaponCombatRatingByType[0x17];         /* the rules' */
extern const uint8_t RelativeAttackDirectionByMask[0x10];
extern const int16_t LinePrimaryYStep[0x08];
extern const int16_t LinePrimaryXStep[0x08];
extern int16_t LinePrimaryRowOffset[0x08];
extern const int16_t LineSecondaryYStep[0x08];
extern const int16_t LineSecondaryXStep[0x08];
extern int16_t LineSecondaryRowOffset[0x08];

#endif
