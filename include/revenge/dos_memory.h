#ifndef REVENGE_DOS_MEMORY_H
#define REVENGE_DOS_MEMORY_H

#include "revenge/types.h"

/*
 * The DOS memory block (AH=4Ah) the program break and the far heap live in,
 * as the host holds it: its first byte, its capacity and its segment. A far
 * address in it is a byte of it, and back, by its linear address:
 * segment * 10h + offset, normalized to an offset of 0-Fh, as the original's
 * huge pointers are.
 *
 * A far address outside the block stays the number it is, as a pointer (the
 * program break's FFFF:FFFF failure, the VGA page at A000:0000), and such a
 * pointer gives that number back. A host pointer outside the block that is no
 * such number is a fault: the original has no far address for it.
 */

void DOSMemory_SetBlock(byte *block, uint32 capacity, word16 segment);
/* NULL before the game sizes it. */
const byte *DOSMemory_Block(uint32 *capacity, word16 *segment);

void *DOSMemory_Pointer(uint32 farAddress);
uint32 DOSMemory_FarAddress(const void *pointer);
uint32 DOSMemory_Linear(const void *pointer);

#endif
