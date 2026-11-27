#include "fake_backend.h"
#include "revenge/input.h"
#include "Data/AudioData.h"
#include "Data/StartupData.h"
#include "Input/Input.h"

#include <assert.h>

/* DS:800E, declared in Data/AudioData.h. */
extern uint16_t NarrationMode;

/* Scan codes as the keyboard controller hands them to IRQ1 (set 1). */
enum
{
    SCAN_ESCAPE = 0x01,
    SCAN_CTRL = 0x1D,
    SCAN_S = 0x1F,
    SCAN_LEFT_SHIFT = 0x2A,
    SCAN_V = 0x2F,
    SCAN_M = 0x32,
    SCAN_ALT = 0x38,
    SCAN_F1 = 0x3B,
    SCAN_KEYPAD_8 = 0x48,
    SCAN_KEYPAD_4 = 0x4B,
    SCAN_DELETE = 0x53,
    SCAN_BREAK = 0x80
};

static void Send(uint8_t scanCode)
{
    KeyboardInterruptHandler(scanCode);
}

static void Tap(uint8_t scanCode)
{
    Send(scanCode);
    Send((uint8_t) (scanCode | SCAN_BREAK));
}

static void TapExtended(uint8_t scanCode)
{
    Send(0xE0);
    Send(scanCode);
    Send(0xE0);
    Send((uint8_t) (scanCode | SCAN_BREAK));
}

/* KeyboardInputPending is the ring's two offsets XORed: nonzero when an
 * event is waiting. */
static uint16_t ReadOne(void)
{
    assert(KeyboardInputPending() != 0x0000);
    return ReadRawKeypress();
}

/* A modifier pressed: the handler queues it too (its id translates to 0). */
static void Hold(uint8_t scanCode, uint16_t queued)
{
    Send(scanCode);
    assert(ReadOne() == queued);
}

static void ExpectEmpty(void)
{
    assert(KeyboardInputPending() == 0x0000);
}

static void TestMakeCodesQueueBreakCodesDont(void)
{
    Send(SCAN_S);
    assert(ReadOne() == 0x0020);
    Send(SCAN_S | SCAN_BREAK);
    ExpectEmpty();
}

static void TestModifiersFromHeldKeys(void)
{
    Send(SCAN_LEFT_SHIFT);
    Tap(SCAN_S);
    Send(SCAN_LEFT_SHIFT | SCAN_BREAK);
    assert(ReadOne() == 0x012C);
    assert(ReadOne() == 0x0120);

    Hold(SCAN_CTRL, 0x023A);
    Tap(SCAN_S);
    Send(SCAN_CTRL | SCAN_BREAK);
    assert(ReadOne() == 0x0220);
    ExpectEmpty();
}

static void ExpectTranslation(uint8_t scanCode, int16_t translated)
{
    Tap(scanCode);
    assert((int16_t) (int8_t) TranslateKeyCode() == translated);
}

static void TestTranslation(void)
{
    Send(SCAN_LEFT_SHIFT);
    assert(TranslateKeyCode() == 0x00);
    ExpectTranslation(SCAN_S, 'S');
    Send(SCAN_LEFT_SHIFT | SCAN_BREAK);
    ExpectTranslation(SCAN_S, 's');
    Hold(SCAN_CTRL, 0x023A);
    ExpectTranslation(SCAN_S, 0x0013);
    Send(SCAN_CTRL | SCAN_BREAK);
    ExpectTranslation(SCAN_ESCAPE, KEY_ESCAPE);
    ExpectTranslation(SCAN_F1, -0x003B);
    ExpectTranslation(SCAN_KEYPAD_4, -0x004B);
    ExpectEmpty();
}

static void TestGreyKeys(void)
{
    /* In the game's mode (0201h) a grey key queues the keypad's id, without
     * Shift; the keypad's own key takes Shift. */
    TapExtended(SCAN_KEYPAD_4);
    assert(ReadOne() == 0x005C);
    Hold(SCAN_LEFT_SHIFT, 0x012C);
    TapExtended(SCAN_KEYPAD_4);
    Tap(SCAN_KEYPAD_4);
    Send(SCAN_LEFT_SHIFT | SCAN_BREAK);
    assert(ReadOne() == 0x005C);
    assert(ReadOne() == 0x015C);

    /* With the mode's bit 1, the grey key's own id. */
    StartupInputMode = 0x0203;
    TapExtended(SCAN_KEYPAD_4);
    assert(ReadOne() == 0x004F);
    TapExtended(SCAN_KEYPAD_4);
    assert((int16_t) (int8_t) TranslateKeyCode() == -0x004B);
    StartupInputMode = 0x0201;
    ExpectEmpty();
}

static void TestRepeats(void)
{
    Send(SCAN_S);
    Send(SCAN_S);
    Send(SCAN_S | SCAN_BREAK);
    assert(ReadOne() == 0x0020);
    assert(ReadOne() == 0x0020);

    StartupInputMode = 0x0200;
    Send(SCAN_S);
    Send(SCAN_S);
    Send(SCAN_S | SCAN_BREAK);
    assert(ReadOne() == 0x0020);
    ExpectEmpty();
    StartupInputMode = 0x0201;
}

static void TestCtrlAltDeleteAndPause(void)
{
    Hold(SCAN_CTRL, 0x023A);
    Hold(SCAN_ALT, 0x063C);
    TapExtended(SCAN_DELETE);
    Tap(SCAN_DELETE);
    Send(SCAN_ALT | SCAN_BREAK);
    Send(SCAN_CTRL | SCAN_BREAK);
    ExpectEmpty();

    Send(0xE1);
    Send(0x1D);
    Send(0x45);
    Send(0xE1);
    Send(0x9D);
    Send(0xC5);
    ExpectEmpty();
    Tap(SCAN_S);
    assert(ReadOne() == 0x0020);
}

static void TestHotkeys(void)
{
    SoundEffectsEnabled = TRUE;
    RequestedMusicEnabled = TRUE;
    NarrationMode = 0x0002;

    Hold(SCAN_ALT, 0x043C);
    Tap(SCAN_S);
    Tap(SCAN_M);
    Tap(SCAN_V);
    Tap(SCAN_V);
    Send(SCAN_ALT | SCAN_BREAK);
    assert(SoundEffectsEnabled == FALSE);
    assert(RequestedMusicEnabled == FALSE);
    assert(NarrationMode == 0x0001);
    assert(ReadOne() == 0x0420);
    assert(ReadOne() == 0x0434);
    assert(ReadOne() == 0x0431);
    assert(ReadOne() == 0x0431);

    /* Shift with Alt is another key: no toggle. */
    Hold(SCAN_ALT, 0x043C);
    Hold(SCAN_LEFT_SHIFT, 0x052C);
    Tap(SCAN_S);
    Send(SCAN_LEFT_SHIFT | SCAN_BREAK);
    Send(SCAN_ALT | SCAN_BREAK);
    assert(SoundEffectsEnabled == FALSE);
    assert(ReadOne() == 0x0520);
    ExpectEmpty();
}

static void TestNumLockWithoutForcedTable(void)
{
    /* Num Lock picks the keypad's digits only when the mode doesn't force
     * the extended table (bit 9). */
    fakeBackend.biosKeyboardFlags[0x00] = 0x20;
    StartupInputMode = 0x0001;
    ExpectTranslation(SCAN_KEYPAD_8, '8');
    StartupInputMode = 0x0201;
    ExpectTranslation(SCAN_KEYPAD_8, -0x0048);
    fakeBackend.biosKeyboardFlags[0x00] = 0x00;
}

static void TestInstallSeedsHeldKeys(void)
{
    fakeBackend.biosKeyboardFlags[0x00] = 0x02;
    InstallKeyboardHandlers();
    assert(fakeBackend.keyboardHandlerInstalled == TRUE);
    fakeBackend.biosKeyboardFlags[0x00] = 0x00;
    Tap(SCAN_S);
    assert(ReadOne() == 0x0120);
    Send(SCAN_LEFT_SHIFT | SCAN_BREAK);
    Tap(SCAN_S);
    assert(ReadOne() == 0x0020);
    RestoreKeyboardHandlers();
    assert(fakeBackend.keyboardHandlerInstalled == FALSE);
}

int main(void)
{
    FakeBackend_Reset();
    StartupInputMode = 0x0201;

    TestMakeCodesQueueBreakCodesDont();
    TestModifiersFromHeldKeys();
    TestTranslation();
    TestGreyKeys();
    TestRepeats();
    TestCtrlAltDeleteAndPause();
    TestHotkeys();
    TestNumLockWithoutForcedTable();
    TestInstallSeedsHeldKeys();
    assert(fakeBackend.inputPolls != 0x0000);
    return 0x00;
}
