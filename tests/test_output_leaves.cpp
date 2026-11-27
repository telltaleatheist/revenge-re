#include "REVENGE.h"
#include "revenge/original_data.h"

#include <assert.h>

static char printedCharacters[0x10];
static uint16_t printedCharacterCount;
static uint8_t retraceModes[0x10];
static uint16_t retraceCallCount;

uint16_t DOS_PrintCharacter(char character)
{
    printedCharacters[printedCharacterCount++] = character;
    return (uint16_t) (uint8_t) character;
}

void WaitForVerticalRetraceEdge(uint8_t waitForRetraceStart)
{
    retraceModes[retraceCallCount++] = waitForRetraceStart;
}

int main(void)
{
    DOS_PrintString("LANTERN");
    assert(printedCharacterCount == 0x0007);
    assert(printedCharacters[0x00] == 'L');
    assert(printedCharacters[0x06] == 'N');

    VerticalRetraceEdgeMode = 0x0001;
    WaitVerticalRetraces(0x0003);
    assert(retraceCallCount == 0x0003);
    assert(retraceModes[0x00] == 0x01);
    assert(retraceModes[0x02] == 0x01);
    WaitVerticalRetraces(0x0000);
    assert(retraceCallCount == 0x0003);
    return 0x00;
}
