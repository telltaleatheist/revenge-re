// 3037: keyboard
// Keyboard.cpp
// Assembly-led translation of the resident keyboard handler, its ring's
// consumers and the translator.

#include "REVENGE.h"
#include "revenge/input.h"
#include "revenge/original_data.h"
#include "revenge/platform_backend.h"

#include <stddef.h>

enum
{
    KEYBOARD_EVENT_COUNT = 0x40,
    KEYBOARD_EVENT_WRAP_MASK = 0x7F,
    KEY_ID_ASCII_TABLE_LIMIT = 0x3E,
    KEY_ID_EXTENDED_LOOKUP_FIRST = 0x4B,
    KEY_ID_ESCAPE = 0x6E,
    KEY_EXTENDED_OUTPUT_FLAG = 0x80,
    KEY_MODIFIER_SHIFT = 0x01,
    KEY_MODIFIER_CTRL = 0x02,
    KEY_MODIFIER_ALT = 0x04,
    KEY_MODIFIER_ANY_MASK = 0x07,
    KEY_CTRL_OUTPUT_MASK = 0x1F,
    KEYBOARD_LOCK_NUM = 0x02,
    KEYBOARD_MODE_FORCE_EXTENDED_TABLE = 0x0200,
    KEY_SCAN_F1 = 0x70,
    KEY_SCAN_F10 = 0x79,
    KEYCODE_F1_BASE = 0xC5,
    KEYCODE_SHIFT_F1_BASE = 0xAC,
    KEYCODE_CTRL_F1_BASE = 0xA2,
    KEYCODE_ALT_F1_BASE = 0x98,
    BIOS_SHIFT_FLAGS = 0x0017,
    BIOS_EXTENDED_SHIFT_FLAGS = 0x0018,
    BIOS_KEYBOARD_STATUS = 0x0096,
    SCAN_PAUSE_PREFIX = 0xE1,
    SCAN_EXTENDED_PREFIX = 0xE0,
    SCAN_BREAK_FLAG = 0x80,
    KEYBOARD_MODE_REPEAT = 0x0001,
    KEYBOARD_MODE_EXTENDED_KEY_IDS = 0x0002,
    KEYBOARD_MODE_PAUSE_TO_BIOS = 0x0080,
    KEYBOARD_EXTENDED_KEY_COUNT = 0x11,
    KEYBOARD_BIOS_KEY_COUNT = 0x09,
    KEY_ID_SOUND_TOGGLE = 0x0420,
    KEY_ID_MUSIC_TOGGLE = 0x0434,
    KEY_ID_NARRATION_CYCLE = 0x0431,
    KEY_ID_CTRL_ALT_KEYPAD_DELETE = 0x0668,
    KEY_ID_CTRL_ALT_DELETE = 0x064C
};

static const byte KeyBitMasks[0x08] = {
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80
};

static const byte CtrlTranslationBitmap[0x08] = {
    0x00, 0x00, 0xFE, 0x87, 0xFF, 0xC0, 0x1F, 0x00
};

static const byte AsciiTranslationTable[0x3E] = {
    0x00, 0x60, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
    0x37, 0x38, 0x39, 0x30, 0x2D, 0x3D, 0x00, 0x08,
    0x09, 0x71, 0x77, 0x65, 0x72, 0x74, 0x79, 0x75,
    0x69, 0x6F, 0x70, 0x5B, 0x5D, 0x5C, 0x00, 0x61,
    0x73, 0x64, 0x66, 0x67, 0x68, 0x6A, 0x6B, 0x6C,
    0x3B, 0x27, 0x5C, 0x0D, 0x00, 0x00, 0x7A, 0x78,
    0x63, 0x76, 0x62, 0x6E, 0x6D, 0x2C, 0x2E, 0x2F,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x20
};

static const byte ShiftedAsciiTranslationTable[0x3E] = {
    0x00, 0x7E, 0x21, 0x40, 0x23, 0x24, 0x25, 0x5E,
    0x26, 0x2A, 0x28, 0x29, 0x5F, 0x2B, 0x00, 0x08,
    0x09, 0x51, 0x57, 0x45, 0x52, 0x54, 0x59, 0x55,
    0x49, 0x4F, 0x50, 0x7B, 0x7D, 0x7C, 0x00, 0x41,
    0x53, 0x44, 0x46, 0x47, 0x48, 0x4A, 0x4B, 0x4C,
    0x3A, 0x22, 0x7C, 0x0D, 0x00, 0x00, 0x5A, 0x58,
    0x43, 0x56, 0x42, 0x4E, 0x4D, 0x3C, 0x3E, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x20
};

static const byte ExtendedTranslationTable[0x23] = {
    0xAE, 0xAD, 0x00, 0x00, 0xB5, 0xB9, 0xB1, 0x00,
    0xB8, 0xB0, 0xB7, 0xAF, 0x00, 0x00, 0xB3, 0x00,
    0xB9, 0xB5, 0xB1, 0x00, 0x2F, 0xB8, 0x00, 0xB0,
    0xAE, 0x2A, 0xB7, 0xB3, 0xAF, 0xAD, 0x2D, 0x2B,
    0x00, 0x0D, 0x00
};

static const byte NumericKeypadTranslationTable[0x14] = {
    0x00, 0x37, 0x34, 0x31, 0x00, 0x2F, 0x38, 0x35,
    0x32, 0x30, 0x2A, 0x39, 0x36, 0x33, 0x2E, 0x2D,
    0x2B, 0x00, 0x0D, 0x00
};

#pragma pack(push, 1)
typedef struct KeyboardRuntime
{
    word16 events[KEYBOARD_EVENT_COUNT]; /* CS:007C */
    word16 readOffset; /* CS:00FC */
    word16 writeOffset; /* CS:00FE */
    word16 translationMode; /* CS:0100 */
    word16 modeFlags; /* CS:0102 */
} KeyboardRuntime;
#pragma pack(pop)

static_assert(sizeof(KeyboardRuntime) == 0x88,
    "The reconstructed CS:007C-0103 keyboard runtime block must be exact.");
static_assert(offsetof(KeyboardRuntime, readOffset) == 0x80,
    "The keyboard read offset must remain at CS:00FC.");
static_assert(offsetof(KeyboardRuntime, translationMode) == 0x84,
    "The translation mode must remain at CS:0100.");

static KeyboardRuntime Keyboard = {
    { 0x0000 },
    0x0000,
    0x0000,
    0x0110,
    0x0000
};

/* The handler's tables. CS:004C: the scan codes that follow an E0h prefix;
 * CS:005C: their key ids, which the key-state bits and the modifier tests
 * use; CS:006C: the key ids queued for them unless the mode's bit 1 is set
 * (the grey keys as the keypad's, the right Ctrl and Alt as the left). The
 * 17th entry of CS:005C is the first of CS:006C; E0h 3Eh, which no keyboard
 * sends and the host never does, would read the ring for its queued id. */
static const byte ExtendedScanCodes[KEYBOARD_EXTENDED_KEY_COUNT] = {
    0x38, 0x1D, 0x52, 0x53, 0x4B, 0x47, 0x4F, 0x48,
    0x50, 0x49, 0x51, 0x4D, 0x35, 0x1C, 0x37, 0x46, 0x3E
};

static const byte ExtendedKeyIds[KEYBOARD_EXTENDED_KEY_COUNT] = {
    0x3E, 0x40, 0x4B, 0x4C, 0x4F, 0x50, 0x51, 0x53,
    0x54, 0x55, 0x56, 0x59, 0x5F, 0x6C, 0x7C, 0x00, 0x3C
};

static const byte ExtendedQueuedKeyIds[KEYBOARD_EXTENDED_KEY_COUNT - 0x01] = {
    0x3C, 0x3A, 0x63, 0x68, 0x5C, 0x5B, 0x5D, 0x60,
    0x62, 0x65, 0x67, 0x66, 0x37, 0x2B, 0x7C, 0x00
};

/* CS:0104: a scan code's key id, for codes 00h-58h. The host's keyboard
 * sends no other code (src/SDL/platform.cpp); the handler indexes it without
 * a bound. */
static const byte ScanCodeKeyIds[0x59] = {
    0x7F, 0x6E, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0F, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
    0x19, 0x1A, 0x1B, 0x1C, 0x2B, 0x3A, 0x1F, 0x20,
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x01, 0x2C, 0x1D, 0x2E, 0x2F, 0x30, 0x31,
    0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x39, 0x64,
    0x3C, 0x3D, 0x1E, 0x70, 0x71, 0x72, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x78, 0x79, 0x5A, 0x7D, 0x5B,
    0x60, 0x65, 0x69, 0x5C, 0x61, 0x66, 0x6A, 0x5D,
    0x62, 0x67, 0x63, 0x68, 0x7F, 0x7F, 0x7F, 0x7A,
    0x7B
};

/* CS:0197: the keys the BIOS's handler also took, so that its shift and
 * lock flags followed them: Caps Lock, both Shifts, both Ctrls, both Alts,
 * Num Lock, and 80h. */
static const byte BiosKeyIds[KEYBOARD_BIOS_KEY_COUNT] = {
    0x1E, 0x2C, 0x39, 0x3A, 0x3C, 0x3E, 0x40, 0x5A, 0x80
};

/* CS:017D: one bit a key id, set while the key is down. Byte 5 bit 4 is the
 * left Shift (2Ch), byte 7 bits 1, 2, 4 and 6 the right Shift, the left
 * Ctrl, the left Alt and the right Alt (39h, 3Ah, 3Ch, 3Eh), byte 8 bit 0 the
 * right Ctrl (40h). */
static byte KeyDownBits[0x10];
static byte ExtendedPrefixPending; /* CS:018D */
static byte PausePrefixRemaining; /* CS:018E */

// 3037:0510: the keyboard's interrupt handler, for one byte from the
// keyboard controller (port 60h). Host: SDL's key events arrive here a byte
// at a time (src/SDL/platform.cpp), as IRQ1 brought them. Where the handler
// ends by passing the byte on to the BIOS's handler (the saved INT 09h at
// CS:018F) or by acknowledging it (port 61h, the BIOS status byte 0040:0096
// and the end of interrupt), the host just returns: the BIOS's handler is
// not the host's, and SDL keeps the shift and lock state.
void KeyboardInterruptHandler(byte scanCode)
{
    byte keyId = scanCode;
    byte modifiers = 0x00;
    byte makeBit = 0x01;
    byte keepBits;
    byte wasDown;
    byte shiftFlags;
    byte extendedKey = FALSE; /* CS:075F */
    word16 extendedIndex = 0x0000; /* CS:0760 */
    word16 mode;
    word16 bitIndex;
    word16 keyword;
    word16 nextWriteOffset;
    word16 index;

    Keyboard.modeFlags &= ~(word16) 0x0003;
    shiftFlags = SDLBackend_ReadBiosKeyboardFlags(BIOS_SHIFT_FLAGS);
    if ((shiftFlags & 0x40) != 0x00)
        Keyboard.modeFlags |= 0x0001;
    if ((shiftFlags & 0x20) != 0x00)
        Keyboard.modeFlags |= KEYBOARD_LOCK_NUM;
    Keyboard.translationMode = StartupInputMode;
    mode = Keyboard.translationMode;

    if (keyId == SCAN_PAUSE_PREFIX)
        PausePrefixRemaining = 0x03;
    if (PausePrefixRemaining != 0x00)
    {
        /* Pause's six bytes; to the BIOS when the mode's bit 7 is set
		 * (3037:0570-057B). */
        --PausePrefixRemaining;
        return;
    }
    if (keyId == SCAN_EXTENDED_PREFIX)
    {
        ExtendedPrefixPending = 0x01;
        return;
    }
    if ((keyId & SCAN_BREAK_FLAG) != 0x00)
    {
        makeBit = 0x00;
        keyId &= (byte) ~SCAN_BREAK_FLAG;
    }
    if (ExtendedPrefixPending != 0x00)
    {
        ExtendedPrefixPending = 0x00;
        extendedKey = TRUE;
        for (index = 0x0000; index < KEYBOARD_EXTENDED_KEY_COUNT; ++index)
            if (ExtendedScanCodes[index] == keyId)
                break;
        if (index == KEYBOARD_EXTENDED_KEY_COUNT)
            return;
        keyId = ExtendedKeyIds[index];
        extendedIndex = index;
    }
    else
        keyId = ScanCodeKeyIds[keyId];

    bitIndex = keyId >> 0x03;
    makeBit = (byte) (makeBit << (keyId & 0x07));
    keepBits = (byte) ~(0x01 << (keyId & 0x07));
    wasDown = KeyDownBits[bitIndex] & makeBit;
    KeyDownBits[bitIndex] &= keepBits;
    KeyDownBits[bitIndex] |= makeBit;
    if ((KeyDownBits[0x07] & 0x04) != 0x00 ||
        (KeyDownBits[0x08] & 0x01) != 0x00)
        modifiers |= KEY_MODIFIER_CTRL;
    if ((KeyDownBits[0x07] & 0x50) != 0x00)
        modifiers |= KEY_MODIFIER_ALT;

    if (extendedKey != FALSE &&
        (mode & KEYBOARD_MODE_EXTENDED_KEY_IDS) == 0x0000)
    {
        /* The queued id, and no Shift (3037:0621-062C: the test against
		 * 21h always passes). */
        keyId = ExtendedQueuedKeyIds[extendedIndex];
    }
    else
    {
        if ((KeyDownBits[0x05] & 0x10) != 0x00 ||
            (KeyDownBits[0x07] & 0x02) != 0x00)
            modifiers |= KEY_MODIFIER_SHIFT;
        /* 3037:0641-0666 flips Shift with Caps Lock or Num Lock when the
		 * key's bit in CS:015D or CS:016D meets CH, which the handler never
		 * sets: it is the interrupted code's. The host has none, so CH is
		 * taken as zero and neither lock flips it. */
    }

    if ((mode & KEYBOARD_MODE_REPEAT) == 0x0000 && wasDown != 0x00)
        return;
    keyword = (word16) ((modifiers << 0x08) | keyId);
    if (keyword == KEY_ID_CTRL_ALT_KEYPAD_DELETE ||
        keyword == KEY_ID_CTRL_ALT_DELETE)
        return;
    if (makeBit == 0x00)
        return;

    nextWriteOffset = (Keyboard.writeOffset + 0x02) &
        KEYBOARD_EVENT_WRAP_MASK;
    if (nextWriteOffset != Keyboard.readOffset)
    {
        Keyboard.events[Keyboard.writeOffset >> 0x01] = keyword;
        Keyboard.writeOffset = nextWriteOffset;
    }

    for (index = 0x0000; index < KEYBOARD_BIOS_KEY_COUNT; ++index)
        if (BiosKeyIds[index] == keyId)
            return;
    if (keyword == KEY_ID_SOUND_TOGGLE)
        SoundEffectsEnabled ^= 0x0001;
    else if (keyword == KEY_ID_MUSIC_TOGGLE)
        RequestedMusicEnabled ^= 0x0001;
    else if (keyword == KEY_ID_NARRATION_CYCLE)
    {
        word16 narrationMode = NarrationMode + 0x0001;

        if (narrationMode > 0x0002)
            narrationMode = 0x0000;
        NarrationMode = narrationMode;
    }
    /* 3037:06FA-070A calls the far pointer at CS:0048 for Escape when its
	 * segment is set; nothing in the game sets it. 3037:070B-0736 sends the
	 * keys listed at CS:0014 on to the BIOS's handler when their mask meets
	 * the mode, and any other when its bit 2 is set; on the host the byte
	 * ends here either way. */
}

// 3037:01A0. Host: SDLBackend_PollInput first delivers what the keyboard
// and the mouse sent since the last pass, as their interrupts would have
// (src/SDL/platform.cpp).
word16 KeyboardInputPending(void)
{
    SDLBackend_PollInput();
    return Keyboard.readOffset ^ Keyboard.writeOffset;
}

// 3037:01B5. Host: each pass of the spin first delivers the keyboard's and
// the mouse's input, as KeyboardInputPending does; passes after the first
// wait briefly for it (SDLBackend_WaitInput). The harness and REVSNAP
// (tools/capture/revsnap.asm) count every pass as a poll.
word16 ReadRawKeypress(void)
{
    word16 readOffset;
    word16 rawKeypress;

    SDLBackend_PollInput();
    for (;;)
    {
        readOffset = Keyboard.readOffset;
        if (readOffset != Keyboard.writeOffset)
            break;
        SDLBackend_WaitInput();
    }
    rawKeypress = Keyboard.events[readOffset >> 0x01];
    Keyboard.readOffset = (readOffset + 0x02) &
        KEYBOARD_EVENT_WRAP_MASK;
    return rawKeypress;
}

// 3037:01D7
int8 TranslateKeyCode(void)
{
    word16 rawKeypress = ReadRawKeypress();
    byte keyId = LOW_BYTE(rawKeypress);
    byte modifierFlags = HIGH_BYTE(rawKeypress);
    int8 translatedKeyCode;

    if (keyId == KEY_ID_ESCAPE)
        return KEY_ESCAPE;
    if (keyId < KEY_ID_ASCII_TABLE_LIMIT)
    {
        word16 tableIndex = keyId & 0x3F;
        translatedKeyCode = (int8) ((modifierFlags & KEY_MODIFIER_SHIFT) == 0x00 ? AsciiTranslationTable[tableIndex] : ShiftedAsciiTranslationTable[tableIndex]);
        if ((modifierFlags & KEY_MODIFIER_CTRL) != 0x00 &&
            (CtrlTranslationBitmap[tableIndex >> 0x03] &
                KeyBitMasks[tableIndex & 0x07]) != 0x00)
            translatedKeyCode &= KEY_CTRL_OUTPUT_MASK;
        return translatedKeyCode;
    }
    if (keyId >= KEY_ID_EXTENDED_LOOKUP_FIRST && keyId < KEY_ID_ESCAPE)
    {
        word16 translationIndex = keyId - KEY_ID_EXTENDED_LOOKUP_FIRST;
        if ((Keyboard.translationMode &
                KEYBOARD_MODE_FORCE_EXTENDED_TABLE) == 0x00 &&
            (Keyboard.modeFlags & KEYBOARD_LOCK_NUM) != 0x00)
            return (int8) NumericKeypadTranslationTable[translationIndex - 0x0F];
        return (int8) ExtendedTranslationTable[translationIndex];
    }
    if (keyId >= KEY_SCAN_F1 && keyId <= KEY_SCAN_F10)
    {
        byte functionKeyCode = KEYCODE_F1_BASE;
        if ((modifierFlags & KEY_MODIFIER_ANY_MASK) != 0x00)
        {
            functionKeyCode = KEYCODE_ALT_F1_BASE;
            if ((modifierFlags & KEY_MODIFIER_ALT) == 0x00)
            {
                functionKeyCode = KEYCODE_CTRL_F1_BASE;
                if ((modifierFlags & KEY_MODIFIER_CTRL) == 0x00)
                    functionKeyCode = KEYCODE_SHIFT_F1_BASE;
            }
        }
        return (int8) (functionKeyCode - (keyId - KEY_SCAN_F1));
    }
    return (int8) (keyId | KEY_EXTENDED_OUTPUT_FLAG);
}

// 3037:0468. The BIOS's keyboard bytes (0040:0017, 0018 and 0096) seed the
// lock flags, the held modifiers and the pending prefixes; SDL supplies them
// (SDLBackend_ReadBiosKeyboardFlags). The handlers' installation (INT 21h
// AH=35h and 25h for INT 09h at 3037:04D4-04ED and INT 23h at 04EF-0508)
// becomes SDLBackend_InstallKeyboardHandlers: from here SDL's key events go
// to KeyboardInterruptHandler. The Ctrl+Break handler at 3037:0762 was a bare
// IRET; the host has no Ctrl+Break to catch.
void InstallKeyboardHandlers(void)
{
    byte flags = SDLBackend_ReadBiosKeyboardFlags(BIOS_SHIFT_FLAGS);
    byte heldModifiers;

    if ((flags & 0x40) != 0x00)
        Keyboard.modeFlags |= 0x0001;
    if ((flags & 0x20) != 0x00)
        Keyboard.modeFlags |= KEYBOARD_LOCK_NUM;
    if ((flags & 0x02) != 0x00)
        KeyDownBits[0x05] |= 0x10;
    heldModifiers = (byte) ((flags & 0x01) << 0x01);
    flags = SDLBackend_ReadBiosKeyboardFlags(BIOS_EXTENDED_SHIFT_FLAGS);
    heldModifiers |= (byte) ((flags << 0x02) & 0x0C);
    flags = SDLBackend_ReadBiosKeyboardFlags(BIOS_KEYBOARD_STATUS);
    if ((flags & 0x08) != 0x00)
        heldModifiers |= 0x40;
    KeyDownBits[0x07] = heldModifiers;
    if ((flags & 0x04) != 0x00)
        KeyDownBits[0x08] |= 0x01;
    if ((flags & 0x02) != 0x00)
        ExtendedPrefixPending = 0x01;
    if ((flags & 0x01) != 0x00)
        PausePrefixRemaining = 0x02;
    SDLBackend_InstallKeyboardHandlers();
}

// 3037:0763. The vectors' restoration (INT 23h, then INT 09h) becomes
// SDLBackend_RestoreKeyboardHandlers: SDL's key events stop going to the
// handler.
void RestoreKeyboardHandlers(void)
{
    SDLBackend_RestoreKeyboardHandlers();
}
