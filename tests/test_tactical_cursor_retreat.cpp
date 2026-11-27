#include "revenge/original_data.h"

#include <assert.h>

/* 1A1F:14E1 steps the 19x12 cursor back one cell: dec, then a signed
 * compare (jge) wraps column 0 to 12h on the row before, and row 0 to 0Bh. */
void RetreatTacticalCursorCell(void);

static void Check(word16 x, word16 y, word16 expectedX, word16 expectedY)
{
	TacticalCursorViewportX = x;
	TacticalCursorViewportY = y;
	RetreatTacticalCursorCell();
	assert(TacticalCursorViewportX == expectedX);
	assert(TacticalCursorViewportY == expectedY);
}

int main(void)
{
	Check(0x0005, 0x0003, 0x0004, 0x0003);
	Check(0x0000, 0x0003, 0x0012, 0x0002);
	Check(0x0000, 0x0000, 0x0012, 0x000B);
	return 0;
}
