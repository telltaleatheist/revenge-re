#ifndef REVENGE_RANDOM_H
#define REVENGE_RANDOM_H

#include "revenge/types.h"

/* 4184:0E48, the generator's state (REVENGE_0800_GRAPHICS_RUNTIME.c). */
extern byte PseudoRandomState[0x03];

word16 GeneratePseudoRandomByte(void);
int16 RandomInRangeInclusive(uint16 minimum, uint16 maximum);

#endif
