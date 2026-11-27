#include "REVENGE.h"

#include <stdio.h>
#include <stdlib.h>
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

static void TestFarMemoryFill(void)
{
	byte bytes[0x08];

	memset(bytes, 0x11, sizeof(bytes));
	FillRuntimeFarMemoryBytes(&bytes[0x02], 0x03, 0x5A);
	Check(bytes[0x01] == 0x11 && bytes[0x02] == 0x5A &&
		bytes[0x04] == 0x5A && bytes[0x05] == 0x11,
		"0800 far fill preserves the REP STOSB extent");
}

static void TestFarMemoryCopy(void)
{
	byte source[0x08] = { 0x10, 0x21, 0x32, 0x43, 0x54, 0x65, 0x76, 0x87 };
	byte destination[0x08];

	memset(destination, 0xCC, sizeof(destination));
	CopyFarMemory(0x05, source + 0x01, destination + 0x02);
	Check(destination[0x01] == 0xCC && destination[0x02] == 0x21 &&
		destination[0x06] == 0x65 && destination[0x07] == 0xCC,
		"far copy handles the MOVSW body and odd MOVSB tail");
}

static void TestLongArithmeticHelpers(void)
{
	Check(MultiplyUnsignedLong(0x5678, 0x0000, 0x1234, 0x0003) ==
		0x369D0368UL,
		"unsigned long multiply returns the low 32 product bits");
	Check(ShiftLeft32_DXAX(0x5678, 0x04, 0x1234) == 0x23456780UL,
		"DX:AX left shift crosses the word boundary");
	Check(ShiftLeft32_DXAX(0x5678, 0x14, 0x1234) == 0x67800000UL,
		"DX:AX left shift follows the count-at-least-16 branch");
	Check(ShiftRight32_DXAX(0x5678, 0x04, 0x1234) == 0x01234567UL,
		"DX:AX right shift crosses the word boundary");
	Check(ShiftRight32_DXAX(0x5678, 0x14, 0x1234) == 0x00000123UL,
		"DX:AX right shift follows the count-at-least-16 branch");
}

int main(void)
{
	TestFarMemoryFill();
	TestFarMemoryCopy();
	TestLongArithmeticHelpers();
	return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
