/*
 * Link-only boundaries for preservation tests which exercise the resident
 * 31FB tables and playback path without loading external resource files.
 * None of these routines is reached by the tests; keeping them here avoids
 * admitting unrelated, still-Unchecked translation units merely to satisfy
 * references from other procedures in the same original object file.
 */

#include "Resources/Resources.h"
#include "revenge/heap.h"

void *Heap_Allocate(uint32_t byteCount, const char *caller)
{
    (void) byteCount;
    (void) caller;
    return (uint8_t *) 0;
}

void Heap_Free(void *block)
{
    (void) block;
}

uint16_t ReadWholeResourceFile(
    const char *resourceName,
    void *destination,
    uint16_t byteCount)
{
    (void) resourceName;
    (void) destination;
    (void) byteCount;
    return 0x0000;
}

void ReadResourceFileSpan(
    const char *resourceName,
    int32_t fileOffset,
    void *destination,
    uint16_t byteCount)
{
    (void) resourceName;
    (void) fileOffset;
    (void) destination;
    (void) byteCount;
}
