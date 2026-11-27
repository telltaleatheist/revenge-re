/* The Borland run-time string helpers the rebuild keeps, because the C
 * library has no function that does what they do (the rest are the C
 * library's: strcpy, strcat, strncpy, strcmp, strlen, strtok, memset,
 * memcpy):
 *
 * - strupr (3A04:000C): upper-cases a-z only, in place, and returns its
 *   argument; every other byte stays as it is.
 * - atol (39C4:0005): skips the white space of the run time's character
 *   table (DS:7029, bit 0: 09h-0Dh and the space), takes one sign, reads
 *   decimal digits up to the first byte that isn't one.
 *
 * itoa and ltoa (3968) are utility.original_integer_format's. */
#include "Utility/Utility.h"

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

static void TestUppercase(void)
{
    char text[] = "media.cfg {z}a\x84\xE1";

    Check(UppercaseStringInPlace(text) == text,
        "strupr returns its argument");
    Check(strcmp(text, "MEDIA.CFG {Z}A\x84\xE1") == 0x00,
        "strupr changes a-z only: punctuation, braces and bytes 80h-FFh stay");
}

static void TestDecimalParse(void)
{
    Check(ParseSignedDecimalLong("5EDT") == 0x05L,
        "atol stops at the first byte that isn't a digit");
    Check(ParseSignedDecimalLong(" \t\n\v\f\r-12") == -0x0CL,
        "atol skips 09h-0Dh and the space, then takes the sign");
    Check(ParseSignedDecimalLong("+7") == 0x07L, "atol takes a plus sign");
    Check(ParseSignedDecimalLong("--7") == 0x00L, "atol takes one sign only");
    Check(ParseSignedDecimalLong("\x1C"
                                 "7") == 0x00L,
        "bytes outside the table's white space end the parse");
    Check(ParseSignedDecimalLong("") == 0x00L, "an empty string is 0");
}

int main(void)
{
    TestUppercase();
    TestDecimalParse();
    if (failures == 0)
        printf("borland strings: ok\n");
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
