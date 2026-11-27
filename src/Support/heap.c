#include "revenge/heap.h"
#include "revenge/debug.h"

#include <stdlib.h>
#include <string.h>

typedef struct HeapBlock
{
	byte *start;
	uint32 size;
	uint32 number;
} HeapBlock;

static HeapBlock *blocks;
static size_t blockCount;
static size_t blockCapacity;
static HeapBlock *freed;
static size_t freedCount;
static size_t freedCapacity;
static uint32 blocksMade;
static int failNext;

static int Grow(HeapBlock **list, size_t count, size_t *capacity)
{
	size_t wanted;
	HeapBlock *grown;

	if (count < *capacity)
		return 1;
	wanted = *capacity != 0 ? *capacity * 2 : 64;
	grown = (HeapBlock *) realloc(*list, wanted * sizeof(HeapBlock));
	if (grown == NULL)
		return 0;
	*list = grown;
	*capacity = wanted;
	return 1;
}

/* The block freed last of exactly this size, taken off the freed list and
 * zeroed; else NULL. */
static byte *Reuse(uint32 byteCount)
{
	size_t index = freedCount;

	while (index-- > 0)
	{
		if (freed[index].size == byteCount)
		{
			byte *start = freed[index].start;

			freed[index] = freed[--freedCount];
			memset(start, 0x00, (size_t) byteCount);
			return start;
		}
	}
	return NULL;
}

void *Heap_Allocate(uint32 byteCount, const char *caller)
{
	byte *start;

	if (byteCount == 0x00000000UL)
		RevengeFault("a 0-byte allocation, called from %s", caller);
	start = NULL;
	if (failNext == 0 && Grow(&blocks, blockCount, &blockCapacity))
	{
		start = Reuse(byteCount);
		if (start == NULL)
			start = (byte *) calloc(1, (size_t) byteCount);
	}
	failNext = 0;
	if (start == NULL)
		RevengeFault("no host memory for a %lu-byte allocation, called from %s",
			(unsigned long) byteCount, caller);
	blocks[blockCount].start = start;
	blocks[blockCount].size = byteCount;
	blocks[blockCount].number = ++blocksMade;
	++blockCount;
	return start;
}

void Heap_Free(void *block)
{
	size_t index;

	if (block == NULL)
		return;
	for (index = 0; index < blockCount; ++index)
	{
		if (blocks[index].start == (byte *) block)
		{
			if (!Grow(&freed, freedCount, &freedCapacity))
				RevengeFault("no host memory to keep a freed block");
			freed[freedCount++] = blocks[index];
			blocks[index] = blocks[--blockCount];
			return;
		}
	}
	for (index = 0; index < freedCount; ++index)
	{
		if (freed[index].start == (byte *) block)
			return;
	}
	RevengeFault("a free of a pointer the heap didn't give");
}

int Heap_Find(uintptr_t address, uint32 *number, uint32 *offset)
{
	size_t index;

	for (index = 0; index < blockCount; ++index)
	{
		uintptr_t start = (uintptr_t) blocks[index].start;

		if (address >= start && address - start < blocks[index].size)
		{
			*number = blocks[index].number;
			*offset = (uint32) (address - start);
			return 1;
		}
	}
	return 0;
}

void Heap_FailNextAllocation(void)
{
	failNext = 1;
}
