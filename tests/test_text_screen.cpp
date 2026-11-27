/* The text screen (src/SDL/platform.cpp): the BIOS's 80x25 text mode clears
 * the page; DOS's console output puts each byte's glyph at the cursor, CR
 * to the row's start, LF down a row, BS back one, BEL nothing; a row wraps
 * at 80, and the page scrolls up off the bottom. */
#include "revenge/dos.h"
#include "revenge/platform_backend.h"
#include "harness.h"

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

static int RowIs(uint16_t row, const char *text)
{
    char expected[80];
    size_t length = strlen(text);

    memset(expected, ' ', sizeof(expected));
    memcpy(expected, text, length);
    return memcmp(Platform_TextRow(row), expected, sizeof(expected)) == 0;
}

int main(void)
{
    int line;

    SDLBackend_SetVideoMode(0x02);
    Check(RowIs(0x00, ""), "the mode clears the page");

    DOS_PrintString("REVENGE\x07\r\nab\x08X\r\n");
    Check(RowIs(0x00, "REVENGE"), "a row, the bell printing nothing");
    Check(RowIs(0x01, "aX"), "a backspace steps back");
    Check(RowIs(0x02, ""), "the cursor on the next row");

    for (line = 0; line < 81; ++line)
        DOS_PrintCharacter((char) ('0' + line % 10));
    Check(Platform_TextRow(0x02)[79] == '9' && RowIs(0x03, "0"), "a row wraps at 80");

    for (line = 0; line < 22; ++line)
        DOS_PrintString("\r\n");
    Check(RowIs(0x00, "aX") && RowIs(0x18, ""), "the page scrolls up");

    SDLBackend_SetVideoMode(0x02);
    Check(RowIs(0x00, "") && RowIs(0x02, ""), "a mode set clears it again");
    return failures != 0;
}
