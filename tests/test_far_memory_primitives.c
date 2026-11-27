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
	TestLongArithmeticHelpers();
	return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
