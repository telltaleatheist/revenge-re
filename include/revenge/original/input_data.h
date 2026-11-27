#ifndef REVENGE_ORIGINAL_INPUT_DATA_H
#define REVENGE_ORIGINAL_INPUT_DATA_H

#include <stddef.h>

#include "revenge/types.h"

enum
{
    ORIGINAL_UI_REGION_COUNT = 0x10,
    ORIGINAL_UI_SELECTION_PENDING = 0x00FF
};

#pragma pack(push, 1)
typedef struct OriginalUiRegionRecord
{
    uint16_t leftColumn; /* +00 */
    uint16_t topRow; /* +02 */
    uint16_t widthColumns; /* +04 */
    uint16_t rowCount; /* +06 */
    uint16_t selectedRow; /* +08 */
    uint16_t unknown000A;
    uint16_t xorColor; /* +0C */
    uint16_t unknown000E;
} OriginalUiRegionRecord;
#pragma pack(pop)

static_assert(offsetof(OriginalUiRegionRecord, selectedRow) == 0x08,
    "The selected UI row must remain at record offset 08h.");
static_assert(offsetof(OriginalUiRegionRecord, xorColor) == 0x0C,
    "The UI highlight color must remain at record offset 0Ch.");
static_assert(sizeof(OriginalUiRegionRecord) == 0x10,
    "OriginalUiRegionRecord must retain its DOS layout.");

extern OriginalUiRegionRecord UiRegionTable[ORIGINAL_UI_REGION_COUNT];

#endif
