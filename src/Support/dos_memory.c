#include "revenge/dos_memory.h"
#include "revenge/debug.h"

#include <stddef.h>
#include <stdint.h>

static byte *dosBlock;
static uint32 dosBlockCapacity;
static word16 dosBlockSegment;

void DOSMemory_SetBlock(byte *block, uint32 capacity, word16 segment)
{
	dosBlock = block;
	dosBlockCapacity = block != NULL ? capacity : 0x00000000UL;
	dosBlockSegment = block != NULL ? segment : 0x0000;
}

const byte *DOSMemory_Block(uint32 *capacity, word16 *segment)
{
	*capacity = dosBlockCapacity;
	*segment = dosBlockSegment;
	return dosBlock;
}

static uint32 BlockLinear(void)
{
	return (uint32) dosBlockSegment << 0x04;
}

void *DOSMemory_Pointer(uint32 farAddress)
{
	uint32 linear = ((farAddress >> 0x10) << 0x04) + (farAddress & 0xFFFFU);

	if (farAddress == 0x00000000UL)
		return NULL;
	if (dosBlock != NULL && linear >= BlockLinear() &&
		linear - BlockLinear() < dosBlockCapacity)
		return dosBlock + (linear - BlockLinear());
	return (void *) (uintptr_t) farAddress;
}

uint32 DOSMemory_Linear(const void *pointer)
{
	uint32 farAddress = DOSMemory_FarAddress(pointer);

	return ((farAddress >> 0x10) << 0x04) + (farAddress & 0xFFFFU);
}

uint32 DOSMemory_FarAddress(const void *pointer)
{
	const byte *bytes = (const byte *) pointer;
	uint32 linear;

	if (pointer == NULL)
		return 0x00000000UL;
	if (dosBlock != NULL && bytes >= dosBlock &&
		(uint32) (bytes - dosBlock) < dosBlockCapacity)
	{
		linear = BlockLinear() + (uint32) (bytes - dosBlock);
		return ((linear >> 0x04) << 0x10) | (linear & 0x0F);
	}
	if ((uintptr_t) pointer <= UINT32_MAX)
		return (uint32) (uintptr_t) pointer;
	RevengeFault("a pointer the original holds as a far address is outside the DOS memory block");
}
