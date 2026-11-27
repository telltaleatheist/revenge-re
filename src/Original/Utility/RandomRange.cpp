// 34C6: random range
// Reconstructed original inclusive random-range utility (segment 34C6).

#include "REVENGE.h"

// 34C6:0008: int16 RandomInRangeInclusive(uint16 minimum, uint16 maximum)
int16_t RandomInRangeInclusive(uint16_t minimum, uint16_t maximum)
{
    int16_t range;
    int16_t randomValue;
    int16_t remainder;

    range = (int16_t) (maximum - minimum + 0x01);
    randomValue = (int16_t) GeneratePseudoRandomByte();
    remainder = randomValue % range;
    return (int16_t) (remainder + (int16_t) minimum);
}
