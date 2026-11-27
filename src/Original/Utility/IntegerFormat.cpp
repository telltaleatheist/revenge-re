// 3968: integer format
// Reconstructed original integer formatting utilities (segment 3968).

#include "Utility/Utility.h"

/* The radixes the formatter takes (2 to 36: digits, then letters), the one
 * that signs, and its buffer: 32 binary digits, a sign and the NUL. */
constexpr uint16_t MINIMUM_RADIX = 0x02;
constexpr uint16_t MAXIMUM_RADIX = 0x24;
constexpr uint16_t DECIMAL_RADIX = 0x0A;
constexpr int FORMATTED_DIGITS_CAPACITY = 0x22;

// 3968:0009: Register far pointer FormatLongInteger(
//     Stack byte alphabeticDigitBase, Stack byte signedConversion,
//     Stack uint16 radix, Stack far pointer destination, Stack uint32 value)
// Called from:
//      IntegerToString
//      LongToString
char *FormatLongInteger(
    uint8_t alphabeticDigitBase,
    uint8_t signedConversion,
    uint16_t radix,
    char *destination,
    uint32_t value)
{
    char *result;
    uint8_t reversedDigits[FORMATTED_DIGITS_CAPACITY];
    uint16_t digitCount;
    uint32_t magnitude;

    result = destination;
    digitCount = 0x00;

    if (radix < MINIMUM_RADIX || radix > MAXIMUM_RADIX)
    {
        *destination = 0x00;
        return result;
    }

    magnitude = value;
    if (signedConversion != 0x00 && (int32_t) value < 0x00)
    {
        *destination++ = '-';
        magnitude = 0x00UL - value;
    }

    do
    {
        reversedDigits[digitCount++] = (uint8_t) (magnitude % radix);
        magnitude /= radix;
    } while (magnitude != 0x00);

    while (digitCount != 0x00)
    {
        uint8_t digit;

        digit = reversedDigits[--digitCount];
        if (digit < DECIMAL_RADIX)
            *destination++ = (char) ('0' + digit);
        else
            *destination++ = (char) (alphabeticDigitBase + digit - DECIMAL_RADIX);
    }

    *destination = 0x00;
    return result;
}

// 3968:008D: Register far pointer IntegerToString(
//     Stack int16 value, Stack far pointer destination, Stack uint16 radix)
// Called from mission-date, campaign-clock, unit-status, and startup rendering.
char *IntegerToString(
    int16_t value,
    char *destination,
    uint16_t radix)
{
    uint32_t extendedValue;

    if (radix == DECIMAL_RADIX)
        extendedValue = (uint32_t) (int32_t) value;
    else
        extendedValue = (uint16_t) value;

    return FormatLongInteger(
        'a',
        0x01,
        radix,
        destination,
        extendedValue);
}

// 3968:00DA: Register far pointer LongToString(
//     Stack int32 value, Stack far pointer destination, Stack uint16 radix)
// Called from:
char *LongToString(
    int32_t value,
    char *destination,
    uint16_t radix)
{
    return FormatLongInteger(
        'a',
        radix == DECIMAL_RADIX,
        radix,
        destination,
        (uint32_t) value);
}
