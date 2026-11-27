#define _CRT_SECURE_NO_WARNINGS

#include "revenge/heap.h"
#include "revenge/debug.h"

#include <stdlib.h>
#include <string.h>

typedef struct HeapBlock
{
    uint8_t *start;
    uint32_t size;
    uint32_t number;
    const char *caller;
} HeapBlock;

static HeapBlock *blocks;
static size_t blockCount;
static size_t blockCapacity;
static HeapBlock *freed;
static size_t freedCount;
static size_t freedCapacity;
static uint32_t blocksMade;
static int failNext;
static int realFree = -1;

/* REVENGE_HEAP_FREE set: a freed block goes back to the host at once and
 * none is kept, so a sanitizer sees every read of a freed one. */
static int RealFree(void)
{
    if (realFree < 0)
        realFree = getenv("REVENGE_HEAP_FREE") != NULL;
    return realFree;
}

static size_t LiveIndex(const void *block)
{
    size_t index;

    for (index = 0; index < blockCount; ++index)
    {
        if (blocks[index].start == (const uint8_t *) block)
            return index;
    }
    return blockCount;
}

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
static uint8_t *Reuse(uint32_t byteCount)
{
    size_t index = freedCount;

    if (RealFree())
        return NULL;
    while (index-- > 0)
    {
        if (freed[index].size == byteCount)
        {
            uint8_t *start = freed[index].start;

            freed[index] = freed[--freedCount];
            memset(start, 0x00, (size_t) byteCount);
            return start;
        }
    }
    return NULL;
}

void *Heap_Allocate(uint32_t byteCount, const char *caller)
{
    uint8_t *start;

    if (byteCount == 0x00000000UL)
        RevengeFault("a 0-byte allocation, called from %s", caller);
    start = NULL;
    if (failNext == 0 && Grow(&blocks, blockCount, &blockCapacity))
    {
        start = Reuse(byteCount);
        if (start == NULL)
            start = (uint8_t *) calloc(1, (size_t) byteCount);
    }
    failNext = 0;
    if (start == NULL)
        RevengeFault("no host memory for a %lu-byte allocation, called from %s",
            (unsigned long) byteCount, caller);
    blocks[blockCount].start = start;
    blocks[blockCount].size = byteCount;
    blocks[blockCount].number = ++blocksMade;
    blocks[blockCount].caller = caller;
    ++blockCount;
    return start;
}

void Heap_Free(void *block)
{
    size_t index;

    if (block == NULL)
        return;
    index = LiveIndex(block);
    if (index == blockCount)
        RevengeFault("a free of a pointer that is no live block: freed already, or never given");
    if (RealFree())
        free(block);
    else
    {
        if (!Grow(&freed, freedCount, &freedCapacity))
            RevengeFault("no host memory to keep a freed block");
        freed[freedCount++] = blocks[index];
    }
    blocks[index] = blocks[--blockCount];
}

void Heap_FreeFrom(void *block, const char *allocator, const char *caller)
{
    size_t index = LiveIndex(block);

    if (block != NULL && index < blockCount &&
        strcmp(blocks[index].caller, allocator) != 0)
        RevengeFault("the free called from %s, of the block %s allocated, finds one %s allocated since",
            caller, allocator, blocks[index].caller);
    Heap_Free(block);
}

const char *Heap_Caller(const void *block)
{
    size_t index = LiveIndex(block);

    return block != NULL && index < blockCount ? blocks[index].caller : NULL;
}

int Heap_Find(uintptr_t address, uint32_t *number, uint32_t *offset)
{
    size_t index;

    for (index = 0; index < blockCount; ++index)
    {
        uintptr_t start = (uintptr_t) blocks[index].start;

        if (address >= start && address - start < blocks[index].size)
        {
            *number = blocks[index].number;
            *offset = (uint32_t) (address - start);
            return 1;
        }
    }
    return 0;
}

void Heap_FailNextAllocation(void)
{
    failNext = 1;
}
