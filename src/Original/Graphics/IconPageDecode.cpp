// 35C2: icon page decode
// IconPageDecode.cpp - icon-page decoding and adapter conversion
// Tactical icon-page decompression and adapter conversion.

#include "REVENGE.h"
#include "revenge/original_data.h"
#include "revenge/video.h"

void DecodeLinearRleScreen(
    const uint8_t *compressedData,
    uint8_t *destination);
void DecodeColumnMajorRleScreen(
    const uint8_t *compressedData,
    uint8_t *destination);
// 35C2:000B
void CopyDecodedIconPage(
    const uint8_t *packedDisplayBuffer,
    uint8_t *iconPageDestination)
{
    uint8_t layoutType;
    const uint8_t *compressedData;

    layoutType = packedDisplayBuffer[0x00];
    compressedData = packedDisplayBuffer + 0x01;

    if (layoutType == 0x01)
        DecodeLinearRleScreen(compressedData, iconPageDestination);
    else
        DecodeColumnMajorRleScreen(compressedData, iconPageDestination);
    /* The CGA's conversion (35C2:0045-0083, 0800:3571, 32ED) went with
	 * the CGA; the rebuild is VGA only. */
}
