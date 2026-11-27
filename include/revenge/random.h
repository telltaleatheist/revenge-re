#ifndef REVENGE_RANDOM_H
#define REVENGE_RANDOM_H

#include "revenge/types.h"

/* 4184:0E48, the generator's state (GraphicsRuntime.cpp). */
extern uint8_t PseudoRandomState[0x03];

uint16_t GeneratePseudoRandomByte(void);
int16_t RandomInRangeInclusive(uint16_t minimum, uint16_t maximum);

#endif
