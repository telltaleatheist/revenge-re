// 3A6A: runtime data declarations
#ifndef REVENGE_DATA_RUNTIME_DATA_H
#define REVENGE_DATA_RUNTIME_DATA_H

#include "revenge/original/input_data.h"
#include "revenge/types.h"

extern const uint8_t CharacterClassTable[0x100];
/* CharacterClassTable's bits, as its rows set them: white space, digits,
 * capitals and small letters. */
inline constexpr uint8_t CHARACTER_CLASS_SPACE = 0x01;
inline constexpr uint8_t CHARACTER_CLASS_DIGIT = 0x02;
inline constexpr uint8_t CHARACTER_CLASS_UPPER = 0x04;
inline constexpr uint8_t CHARACTER_CLASS_LOWER = 0x08;
inline constexpr uint8_t CHARACTER_CLASS_LETTER = CHARACTER_CLASS_UPPER | CHARACTER_CLASS_LOWER;
extern const uint16_t VideoPixelAlignmentMasks[0x08];
extern const uint16_t PackedPixelShiftCounts[0x03];
extern uint16_t TextWindowFrameDefinition[0x0C];
extern const uint16_t *const TextWindowFrameDefinitionTable[0x01];
extern uint8_t *ActiveFontResource;
extern uint8_t MusicDriverIndex;
extern int8_t ActiveFontGlyphAdvanceX;
extern int8_t ActiveFontLineHeight;
extern uint8_t *TacticalTileStrip;
extern int16_t BusyWaitLoopCalibration;
extern uint16_t ActiveUiHighlightXorColor;
extern uint16_t ActiveUiHighlightHeight;
extern uint16_t ActiveUiHighlightWidth;
extern uint16_t ActiveUiHighlightLeft;
extern uint16_t ActiveUiHighlightTop;
extern uint8_t *PrimaryScreenBuffer;
extern uint8_t SavedUiRegionSelectionRows[ORIGINAL_UI_REGION_COUNT];
extern int16_t ActiveUiRegionIndex;
extern uint16_t ExpandFirstUiRowHighlight;
extern uint16_t VideoAdapterType;

#endif
