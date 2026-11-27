#include "revenge/debug.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* REVENGE_INDEX passes an index inside its table and stops the program,
 * naming the table, on one past it or on a negative index. Run with "past"
 * or "negative", the test ends in that fault (ctest matches its message). */
static const unsigned char GuardedTable[0x05] = { 1, 2, 3, 4, 5 };

int main(int argc, char **argv)
{
    size_t index;
    int negative = -1;

    for (index = 0; index < sizeof(GuardedTable); ++index)
        assert(GuardedTable[REVENGE_INDEX(GuardedTable, index)] == index + 1);
    if (argc > 1 && strcmp(argv[1], "past") == 0)
        printf("%u\n", GuardedTable[REVENGE_INDEX(GuardedTable, sizeof(GuardedTable))]);
    if (argc > 1 && strcmp(argv[1], "negative") == 0)
        printf("%u\n", GuardedTable[REVENGE_INDEX(GuardedTable, negative)]);
    return 0;
}
