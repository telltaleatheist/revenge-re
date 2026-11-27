#include "Graphics/Graphics.h"
#include "Utility/Utility.h"

#include <assert.h>

static uint16_t nextRandomValue;

uint16_t GeneratePseudoRandomByte(void)
{
    return nextRandomValue;
}

int main(void)
{
    nextRandomValue = 0x0000;
    assert(RandomInRangeInclusive(0x0003, 0x000A) == 0x0003);
    nextRandomValue = 0x0007;
    assert(RandomInRangeInclusive(0x0003, 0x000A) == 0x000A);
    nextRandomValue = 0x0008;
    assert(RandomInRangeInclusive(0x0003, 0x000A) == 0x0003);
    nextRandomValue = 0x00FF;
    assert(RandomInRangeInclusive(0x0003, 0x000A) == 0x000A);
    nextRandomValue = 0x0000;
    assert(RandomInRangeInclusive(0x0009, 0x0004) == 0x0009);
    return 0x00;
}
