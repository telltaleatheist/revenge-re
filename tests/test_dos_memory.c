/* The DOS memory block's far addresses (revenge/dos_memory.h): a byte of the
 * block and its normalized far address, both ways; NULL and 0:0; the far
 * addresses outside the block (the program break's FFFF:FFFF, the VGA page)
 * as the numbers they are. */
#include "revenge/dos_memory.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static int failures;

static void Check(int condition, const char *message)
{
	if (!condition)
	{
		fprintf(stderr, "FAIL: %s\n", message);
		++failures;
	}
}

int main(void)
{
	static byte block[0x1000];
	uint32 capacity;
	word16 segment;

	Check(DOSMemory_Block(&capacity, &segment) == NULL,
		"no block before the game sizes it");
	DOSMemory_SetBlock(block, sizeof(block), 0x07F0);
	Check(DOSMemory_Block(&capacity, &segment) == block &&
		capacity == sizeof(block) && segment == 0x07F0,
		"the block, its capacity and its segment");

	Check(DOSMemory_FarAddress(NULL) == 0x00000000UL &&
		DOSMemory_Pointer(0x00000000UL) == NULL,
		"NULL is 0000:0000");
	Check(DOSMemory_FarAddress(block) == 0x07F00000UL &&
		DOSMemory_Pointer(0x07F00000UL) == block,
		"the block's first byte is its segment");
	Check(DOSMemory_FarAddress(block + 0x0123) == 0x08020003UL,
		"a byte's far address is normalized, the offset 0-Fh");
	Check(DOSMemory_Pointer(0x08020003UL) == block + 0x0123 &&
		DOSMemory_Pointer(0x07F00123UL) == block + 0x0123,
		"any segment and offset naming the byte give it");
	Check(DOSMemory_Linear(block + 0x0123) == 0x8023UL,
		"a byte's linear address");
	Check(DOSMemory_Pointer(0x08EF000FUL) == block + 0x0FFF &&
		DOSMemory_Pointer(0x08F00000UL) == (void *) (uintptr_t) 0x08F00000UL,
		"the block's last byte is its own; the next is outside it");

	Check(DOSMemory_Pointer(0xFFFFFFFFUL) == (void *) (uintptr_t) 0xFFFFFFFFUL &&
		DOSMemory_FarAddress((void *) (uintptr_t) 0xFFFFFFFFUL) == 0xFFFFFFFFUL,
		"FFFF:FFFF outside the block stays the number");
	Check(DOSMemory_Pointer(0xA0000000UL) == (void *) (uintptr_t) 0xA0000000UL &&
		DOSMemory_FarAddress((void *) (uintptr_t) 0xA0000000UL) == 0xA0000000UL,
		"the VGA page stays A000:0000");

	DOSMemory_SetBlock(NULL, 0x00000000UL, 0x0000);
	Check(DOSMemory_Pointer(0x07F00000UL) == (void *) (uintptr_t) 0x07F00000UL,
		"a released block's far addresses are no longer its bytes");
	if (failures == 0)
		printf("dos memory: ok\n");
	return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
