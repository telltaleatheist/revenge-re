// 3968: integer format
// Reconstructed original integer formatting utilities (segment 3968).

#include "Utility/Utility.h"

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
    uint8_t reversedDigits[0x22];
    uint16_t digitCount;
    uint32_t magnitude;

    result = destination;
    digitCount = 0x00;

    if (radix < 0x02 || radix > 0x24)
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
        if (digit < 0x0A)
            *destination++ = (char) ('0' + digit);
        else
            *destination++ = (char) (alphabeticDigitBase + digit - 0x0A);
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

    if (radix == 0x0A)
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
        radix == 0x0A,
        radix,
        destination,
        (uint32_t) value);
}
