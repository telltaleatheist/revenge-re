// 34C6, 3968, 39C4, 3A04: Utility declarations
#ifndef REVENGE_UTILITY_UTILITY_H
#define REVENGE_UTILITY_UTILITY_H

#include "revenge/types.h"

#define LOW_BYTE(value)  ((uint8_t) ((uint32_t) (value) & 0x00FFU))
#define HIGH_BYTE(value) ((uint8_t) (((uint32_t) (value) >> 0x08) & 0x00FFU))
#define LOW_WORD(value)  ((uint16_t) ((uint32_t) (value) & 0xFFFFU))
#define HIGH_WORD(value) ((uint16_t) (((uint32_t) (value) >> 0x10) & 0xFFFFU))
#define MAKE_WORD(highByte, lowByte) \
    ((uint16_t) (((uint16_t) (uint8_t) (highByte) << 0x08) | \
        (uint8_t) (lowByte)))
#define MAKE_DWORD(highWord, lowWord) \
    ((uint32_t) (((uint32_t) (uint16_t) (highWord) << 0x10) | \
        (uint16_t) (lowWord)))
#define MAKE_FAR_POINTER(segment, offset) MAKE_DWORD((segment), (offset))

int16_t RandomInRangeInclusive(uint16_t minimum, uint16_t maximum);
char *UppercaseStringInPlace(char *text);
char *FormatLongInteger(
    uint8_t alphabeticDigitBase,
    uint8_t signedConversion,
    uint16_t radix,
    char *destination,
    uint32_t value);
char *IntegerToString(
    int16_t value,
    char *destination,
    uint16_t radix);
char *LongToString(
    int32_t value,
    char *destination,
    uint16_t radix);
long ParseSignedDecimalLong(const char *text);

#endif
