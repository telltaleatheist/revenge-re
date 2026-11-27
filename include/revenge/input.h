#ifndef REVENGE_INPUT_H
#define REVENGE_INPUT_H

#include "revenge/types.h"

inline constexpr int KEY_BACKSPACE = 0x08;
inline constexpr int KEY_TAB = 0x09;
inline constexpr int KEY_CARRIAGE_RETURN = 0x0D;
inline constexpr int KEY_CTRL_S = 0x13;
inline constexpr int KEY_ESCAPE = 0x1B;
inline constexpr int KEY_SPACE = 0x20;
inline constexpr int KEY_ONE = 0x31;
inline constexpr int KEY_TWO = 0x32;
inline constexpr int KEY_THREE = 0x33;
inline constexpr int KEY_FOUR = 0x34;
inline constexpr int KEY_SIX = 0x36;
inline constexpr int KEY_SEVEN = 0x37;
inline constexpr int KEY_EIGHT = 0x38;
inline constexpr int KEY_NINE = 0x39;
inline constexpr int KEY_DELETE = 0xAD;
inline constexpr int KEY_PAGE_DOWN = 0xAF;
inline constexpr int KEY_DOWN = 0xB0;
inline constexpr int KEY_END = 0xB1;
inline constexpr int KEY_RIGHT = 0xB3;
inline constexpr int KEY_LEFT = 0xB5;
inline constexpr int KEY_PAGE_UP = 0xB7;
inline constexpr int KEY_UP = 0xB8;
inline constexpr int KEY_HOME = 0xB9;
inline constexpr int KEY_F10 = 0xBC;
inline constexpr int KEY_F9 = 0xBD;
inline constexpr int KEY_F8 = 0xBE;
inline constexpr int KEY_F7 = 0xBF;
inline constexpr int KEY_F6 = 0xC0;
inline constexpr int KEY_F5 = 0xC1;
inline constexpr int KEY_F4 = 0xC2;
inline constexpr int KEY_F3 = 0xC3;
inline constexpr int KEY_F2 = 0xC4;
inline constexpr int KEY_F1 = 0xC5;
inline constexpr int MOUSE_BUTTON_LEFT = 0x01;
inline constexpr int MOUSE_BUTTON_RIGHT = 0x02;
inline constexpr int MOUSE_BUTTON_MIDDLE = 0x04;

static inline int16_t TranslatedExtendedKey(uint8_t keyCode)
{
    return (int16_t) (int8_t) keyCode;
}

void KeyboardInterruptHandler(uint8_t scanCode);
uint16_t KeyboardInputPending(void);
uint16_t ReadRawKeypress(void);
int8_t TranslateKeyCode(void);
int16_t ReadTranslatedKeypress(void);
uint16_t WaitForDismissInput(void);
uint16_t WaitForKeyboardOrMouseInput(void);
void FlushPendingInput(void);
void InstallKeyboardHandlers(void);
void RestoreKeyboardHandlers(void);
void MouseEventCallback(uint16_t buttons, uint16_t x, uint16_t y);

#endif
