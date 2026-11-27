/* The host heap (revenge/heap.h): blocks zeroed, numbered in allocation
 * order and found by an address inside them; a freed block no longer found,
 * and its number not given again; a freed block given back, zeroed, to the
 * next request of its exact size, the one freed last first; a live block's
 * caller.
 *
 *   (no argument)  the above
 *   fail           a request the host can't meet is a fault naming its size
 *                  and caller (ctest matches it)
 *   zero           so is a request of 0 bytes
 *   twice          so is a free of a block freed already
 *   reused         so is an owner's free of its block after another request
 *                  has taken it: the music's free in a battle (0D73:025E)
 *                  again, after the block went to a backing of its size
 */
#include "revenge/heap.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int failures;

static void Check(int condition, const char *message)
{
	if (!condition)
	{
		fprintf(stderr, "FAIL: %s\n", message);
		++failures;
	}
}

int main(int argc, char **argv)
{
	byte *first;
	byte *second;
	byte *third;
	uint32 number;
	uint32 offset;
	uint32 index;
	int zeroed = 1;

	if (argc > 1 && strcmp(argv[1], "fail") == 0)
	{
		Heap_FailNextAllocation();
		(void) Heap_Allocate(0xFFF0, "1234:5678");
		return 1;
	}
	if (argc > 1 && strcmp(argv[1], "zero") == 0)
	{
		(void) Heap_Allocate(0x0000, "1234:5678");
		return 1;
	}
	if (argc > 1 && strcmp(argv[1], "twice") == 0)
	{
		first = (byte *) Heap_Allocate(0x30D4, "3014:011E");
		Heap_Free(first);
		Heap_Free(first);
		return 1;
	}
	if (argc > 1 && strcmp(argv[1], "reused") == 0)
	{
		first = (byte *) Heap_Allocate(0x30D4, "3014:011E");
		Heap_FreeFrom(first, "3014:011E", "3014:0133");
		second = (byte *) Heap_Allocate(0x30D4, "0D73:562F");
		if (second != first || Heap_Caller(second) == NULL ||
			strcmp(Heap_Caller(second), "0D73:562F") != 0)
			return 1;
		Heap_FreeFrom(first, "3014:011E", "3014:0133");
		return 1;
	}

	first = (byte *) Heap_Allocate(0x0240, "0800:36C0");
	second = (byte *) Heap_Allocate(0x0001FD00UL, "2C7F:0203");
	for (index = 0; index < 0x0001FD00UL; ++index)
		zeroed &= second[index] == 0x00;
	Check(zeroed, "a block starts zeroed");

	Check(Heap_Find((uintptr_t) first, &number, &offset) && number == 1 && offset == 0,
		"the first block is number 1, at offset 0");
	Check(Heap_Caller(first) != NULL && strcmp(Heap_Caller(first), "0800:36C0") == 0,
		"a live block's caller");
	Check(Heap_Caller(first + 1) == NULL, "no caller for an address inside a block");
	Check(Heap_Find((uintptr_t) (second + 0x10000), &number, &offset) &&
		number == 2 && offset == 0x10000,
		"an address inside the second block is its number and the offset");
	Check(!Heap_Find((uintptr_t) (first + 0x0240), &number, &offset),
		"the byte past a block is not in it");

	Heap_FreeFrom(first, "0800:36C0", "0800:3700");
	Check(!Heap_Find((uintptr_t) first, &number, &offset), "a freed block is not found");
	Check(Heap_Caller(first) == NULL, "no caller for a freed block");
	third = (byte *) Heap_Allocate(0x0010, "3014:011E");
	Check(Heap_Find((uintptr_t) third, &number, &offset) && number == 3,
		"a new block takes the next number, not a freed one's");
	Check(Heap_Find((uintptr_t) second, &number, &offset) && number == 2,
		"a block keeps its number when another is freed");
	{
		byte *a = (byte *) Heap_Allocate(0x0100, "0D73:562F");
		byte *b = (byte *) Heap_Allocate(0x0100, "0D73:562F");
		byte *c;
		byte *d;

		memset(a, 0xAA, 0x0100);
		memset(b, 0xBB, 0x0100);
		Heap_Free(a);
		Heap_Free(b);
		c = (byte *) Heap_Allocate(0x0101, "0D73:562F");
		Check(c != a && c != b, "a request of another size takes no freed block");
		d = (byte *) Heap_Allocate(0x0100, "0D73:562F");
		Check(d == b, "a request takes the block of its size freed last");
		Check(d[0x00] == 0x00 && d[0xFF] == 0x00, "a block given back is zeroed");
		Check(Heap_Find((uintptr_t) d, &number, &offset) && number == 7,
			"a block given back takes a new number");
		Check((byte *) Heap_Allocate(0x0100, "0D73:562F") == a,
			"then the one freed before it");
	}
	Heap_Free(NULL);
	Heap_FreeFrom(NULL, "3014:011E", "3014:0133");
	Heap_Free(second);
	Heap_Free(third);
	return failures != 0;
}
