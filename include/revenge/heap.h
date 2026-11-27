#ifndef REVENGE_HEAP_H
#define REVENGE_HEAP_H

#include "revenge/types.h"

#include <stdint.h>

/*
 * The game's heap, from the host. The original's Borland far heap (38CD,
 * 398D, 398B) on its program break (38FE, 3913) and its wrappers with the
 * out-of-memory screen (0D56) are gone: the game's peak is 261,360 bytes
 * (the second battle) of the 368,576 its DOS block held, and the host has
 * far more, so neither the screen nor the start-up's 110h KB check
 * (2C7F:012A-0149) is reachable.
 *
 * Heap_Allocate gives byteCount zeroed bytes. A request of 0 bytes, or one
 * the host can't meet, stops the program with a fault naming the request's
 * size and its caller: the original's address of the call that asked for
 * it. No caller asks for 0 bytes.
 *
 * A freed block is kept, not given back to the host: the next request of
 * its exact size takes the one freed last, zeroed. So the game's requests,
 * the same every run, get their blocks at the same places every run, and a
 * pointer the game keeps to a freed block (the cards' sprites after the lance
 * editor, 2507:623D) lands in the same block every run; the host's own reuse
 * depends on its other threads. What is kept stays under 250 KB in the
 * longest trace scripts (second_battle, restart, save_load, battle_steps);
 * the size that varies most, the cursor's backing as it moves, is under 400
 * bytes (2 tiles by 16 rows).
 *
 * With REVENGE_HEAP_FREE set, a freed block goes back to the host at once
 * instead, for a sanitizer's run.
 *
 * Each block is numbered in allocation order, from 1, a reused one anew; the
 * map dump names a pointer into a live block by that number and the offset
 * (src/SDL/map_dump.c).
 */
void *Heap_Allocate(uint32 byteCount, const char *caller);
/* NULL: nothing. A pointer that isn't a live block's start, a block freed
 * already among them, is a fault. */
void Heap_Free(void *block);
/* Heap_Free of a block the caller holds from the request at allocator's
 * address: a live block there that another request has taken since is a
 * fault naming the free's caller and both requests. */
void Heap_FreeFrom(void *block, const char *allocator, const char *caller);
/* The caller of the request that took the live block starting there; NULL
 * for any other pointer. */
const char *Heap_Caller(const void *block);

/* The live block holding the address: its number and the address's offset,
 * and 1; else 0. */
int Heap_Find(uintptr_t address, uint32 *number, uint32 *offset);

/* The harness's fail:allocation (src/SDL/harness.h): the next request fails
 * as if the host had no memory for it. */
void Heap_FailNextAllocation(void);

#endif
