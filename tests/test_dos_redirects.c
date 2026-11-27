#include "fake_backend.h"
#include "revenge/dos.h"

#include <assert.h>
#include <string.h>

static void TestConsole(void)
{
    FakeBackend_Reset();

    assert(DOS_PrintCharacter('R') == 0x0652);
    DOS_PrintString("CH");

    assert(fakeBackend.consoleCharacterCount == 0x0003);
    assert(fakeBackend.consoleCharacters[0x00] == 'R');
    assert(fakeBackend.consoleCharacters[0x01] == 'C');
    assert(fakeBackend.consoleCharacters[0x02] == 'H');
}

static void TestSystemServices(void)
{
    DOS_Date date;
    DOS_Time timeValue;

    FakeBackend_Reset();

    memset(&date, 0, sizeof(date));
    memset(&timeValue, 0, sizeof(timeValue));
    DOS_GetSystemDate(&date);
    DOS_GetSystemTime(&timeValue);
    assert(date.year == 0x07EA && date.month == 0x09 && date.day == 0x1A);
    assert(timeValue.hour == 0x0C && timeValue.minute == 0x22);
    assert(timeValue.second == 0x38 && timeValue.hundredth == 0x4E);
}

int main(void)
{
    TestConsole();
    TestSystemServices();
    return 0;
}
