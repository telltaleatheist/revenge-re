#include "revenge/platform_backend.h"
#include "revenge/audio.h"
#include "revenge/debug.h"
#include "revenge/heap.h"
#include "revenge/input.h"
#include "revenge/original_data.h"
#include "revenge/video.h"
#include "harness.h"

#include <SDL3/SDL.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum
{
    WindowScale = 0x03,
    RetracesPerSecond = 0x46,
    /* The retrace: the last tenth of a frame, in thousandths of one. */
    RetraceStart = 900,
    /* A host present between the game's own, at most 60 a second. */
    VideoPresentPeriodMS = 17
};

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
/* The whole VGA segment (revenge/video.h): the first 64000 bytes are the
 * screen, shown and hashed; the rest is off the screen. */
static byte indexedFramebuffer[VIDEO_SEGMENT_SIZE];
static uint32_t scanoutPixels[VIDEO_SCREEN_WIDTH * VIDEO_SCREEN_HEIGHT];
static uint32_t logicalPalette[VGA_DAC_ENTRY_COUNT];
static byte currentVideoMode = 0x03;

/*
 * The text screen. Before its first picture the game sets the BIOS's 80x25
 * text mode (slot 05, mode 02h) and prints its banner through DOS
 * (3327:0007); its start-up failures print there too. The window shows that
 * page while a text mode is set: light grey on black, SDL's own 8x8 font
 * (SDL_RenderDebugText) drawn twice as tall, 640x400, the screen's shape.
 * A copy of what is printed goes to stderr, where faults go.
 */
enum
{
    TextColumns = 80,
    TextRows = 25,
    TextGlyphSize = 8,
    TextScreenWidth = TextColumns * TextGlyphSize,
    TextScreenHeight = TextRows * TextGlyphSize * 2
};

static char textPage[TextRows][TextColumns];
static uint16 textRow;
static uint16 textColumn;
static SDL_Texture *textScreen;
static uint64_t nextVideoPresentTick;
static void (*gameClockHandler)(void);
static word16 mouseEventMask;
static word16 hostMouseButtons;
static byte pointerOutside;
static byte lastInsideMouseSeen;
static word16 lastInsideMouseX;
static word16 lastInsideMouseY;

enum
{
    SmokeInputCapacity = 0x80,
    SmokeShotNameLength = 0x40
};

typedef enum SmokeInputKind
{
    SmokeKey,
    SmokeMouse,
    SmokeShot
} SmokeInputKind;

typedef struct SmokeInputEvent
{
    uint32_t delayMilliseconds;
    SmokeInputKind kind;
    int scancode;
    byte modifiers;
    word16 mouseX;
    word16 mouseY;
    word16 mouseButtons;
    char shotName[SmokeShotNameLength];
} SmokeInputEvent;

static SmokeInputEvent smokeInputEvents[SmokeInputCapacity];
static uint16 smokeInputEventCount;
static uint16 smokeInputEventIndex;
static uint64_t smokeInputDueTime;
static byte smokeInputInitialized;
static word16 smokeMouseButtons;
static char smokeShotPending[SmokeShotNameLength];
static byte keyboardHandlerInstalled;
static byte deliveringHostEvents;
static byte quitRequested;

/* One smoke item after its delay: a key name as the harness's scripts write
 * them (Shift+, Ctrl+, Alt+ allowed), mouse:X/Y[/B] (the game's 320x200 and
 * the buttons held, 1 left, 2 right, 4 middle) or shot:NAME. */
static int ParseSmokeItem(const char *item, SmokeInputEvent *event)
{
    if (strncmp(item, "mouse:", 0x06) == 0x00)
    {
        unsigned x;
        unsigned y;
        unsigned buttons = 0x00;
        int fields = SDL_sscanf(item + 0x06, "%u/%u/%u", &x, &y, &buttons);

        if (fields < 0x02 || x >= VIDEO_SCREEN_WIDTH ||
            y >= VIDEO_SCREEN_HEIGHT || buttons > 0x07)
            return FALSE;
        event->kind = SmokeMouse;
        event->mouseX = (word16) x;
        event->mouseY = (word16) y;
        event->mouseButtons = (word16) buttons;
        return TRUE;
    }
    if (strncmp(item, "shot:", 0x05) == 0x00)
    {
        const char *name = item + 0x05;
        size_t i;

        if (*name == '\0' || strlen(name) >= SmokeShotNameLength)
            return FALSE;
        for (i = 0x00; name[i] != '\0'; ++i)
            if (!isalnum((unsigned char) name[i]) && name[i] != '_' &&
                name[i] != '-')
                return FALSE;
        event->kind = SmokeShot;
        SDL_strlcpy(event->shotName, name, sizeof(event->shotName));
        return TRUE;
    }
    event->kind = SmokeKey;
    return Harness_ParseKey(item, &event->scancode, &event->modifiers);
}

/* REVENGE_SMOKE_INPUT: DELAY_MS:ITEM[,DELAY_MS:ITEM...]. */
static void InitializeSmokeInput(void)
{
    const char *cursor;

    if (smokeInputInitialized != FALSE)
        return;
    smokeInputInitialized = TRUE;
    cursor = SDL_getenv("REVENGE_SMOKE_INPUT");
    while (cursor != NULL && *cursor != '\0' &&
        smokeInputEventCount < SmokeInputCapacity)
    {
        char *end;
        char item[0x50];
        size_t length;
        unsigned long delay = strtoul(cursor, &end, 0x0A);
        SmokeInputEvent *event = &smokeInputEvents[smokeInputEventCount];

        if (end == cursor || *end != ':')
            break;
        cursor = end + 0x01;
        length = strcspn(cursor, ",");
        if (length == 0x00 || length >= sizeof(item))
            break;
        memcpy(item, cursor, length);
        item[length] = '\0';
        if (!ParseSmokeItem(item, event))
        {
            RevengeTrace("input: smoke item '%s' not understood\n", item);
            break;
        }
        event->delayMilliseconds = (uint32_t) delay;
        ++smokeInputEventCount;
        cursor += length;
        if (*cursor == ',')
            ++cursor;
    }
    if (smokeInputEventCount != 0x00)
        smokeInputDueTime = HostClock_NowMS() +
            smokeInputEvents[0x00].delayMilliseconds;
    RevengeTrace("input: smoke events=%u first-due=%llu\n",
        smokeInputEventCount, (unsigned long long) smokeInputDueTime);
}

/* The smoke input goes in as the devices' own events would: pushed on SDL's
 * queue, and taken off it with the rest by DeliverHostEvents. */
static void PushSmokeKey(SDL_Scancode scancode, int down)
{
    SDL_Event event;

    SDL_zero(event);
    event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    event.key.timestamp = SDL_GetTicksNS();
    event.key.windowID = window != NULL ? SDL_GetWindowID(window) : 0x00;
    event.key.scancode = scancode;
    event.key.key = SDL_GetKeyFromScancode(scancode, SDL_KMOD_NONE, false);
    event.key.down = down ? true : false;
    (void) SDL_PushEvent(&event);
}

static void PushSmokeKeystroke(SDL_Scancode scancode, byte modifiers)
{
    if ((modifiers & 0x01) != 0x00)
        PushSmokeKey(SDL_SCANCODE_LSHIFT, TRUE);
    if ((modifiers & 0x02) != 0x00)
        PushSmokeKey(SDL_SCANCODE_LCTRL, TRUE);
    if ((modifiers & 0x04) != 0x00)
        PushSmokeKey(SDL_SCANCODE_LALT, TRUE);
    PushSmokeKey(scancode, TRUE);
    PushSmokeKey(scancode, FALSE);
    if ((modifiers & 0x04) != 0x00)
        PushSmokeKey(SDL_SCANCODE_LALT, FALSE);
    if ((modifiers & 0x02) != 0x00)
        PushSmokeKey(SDL_SCANCODE_LCTRL, FALSE);
    if ((modifiers & 0x01) != 0x00)
        PushSmokeKey(SDL_SCANCODE_LSHIFT, FALSE);
}

/* The mouse moved to the game's X,Y, as the window's coordinates, then each
 * button whose state changed pressed or released there. */
static void PushSmokeMouse(word16 x, word16 y, word16 buttons)
{
    static const Uint8 sdlButtons[0x03] = { SDL_BUTTON_LEFT, SDL_BUTTON_RIGHT, SDL_BUTTON_MIDDLE };
    SDL_Event event;
    float windowX = (float) x + 0.5f;
    float windowY = (float) y + 0.5f;
    SDL_WindowID windowID = window != NULL ? SDL_GetWindowID(window) : 0x00;
    int i;

    if (renderer != NULL)
        (void) SDL_RenderCoordinatesToWindow(renderer, (float) x + 0.5f,
            (float) y + 0.5f, &windowX, &windowY);
    SDL_zero(event);
    event.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.timestamp = SDL_GetTicksNS();
    event.motion.windowID = windowID;
    event.motion.x = windowX;
    event.motion.y = windowY;
    (void) SDL_PushEvent(&event);
    for (i = 0x00; i < 0x03; ++i)
    {
        word16 bit = (word16) (0x01 << i);

        if (((buttons ^ smokeMouseButtons) & bit) == 0x00)
            continue;
        SDL_zero(event);
        event.type = (buttons & bit) != 0x00 ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
        event.button.timestamp = SDL_GetTicksNS();
        event.button.windowID = windowID;
        event.button.button = sdlButtons[i];
        event.button.down = (buttons & bit) != 0x00 ? true : false;
        event.button.clicks = 0x01;
        event.button.x = windowX;
        event.button.y = windowY;
        (void) SDL_PushEvent(&event);
    }
    smokeMouseButtons = buttons;
}

static void SendDueSmokeInput(void)
{
    const SmokeInputEvent *event;

    InitializeSmokeInput();
    if (smokeInputEventIndex >= smokeInputEventCount ||
        HostClock_NowMS() < smokeInputDueTime)
        return;
    event = &smokeInputEvents[smokeInputEventIndex];
    RevengeTrace("input: smoke event=%u/%u kind=%d\n",
        smokeInputEventIndex + 0x01, smokeInputEventCount, (int) event->kind);
    switch (event->kind)
    {
    case SmokeKey:
        PushSmokeKeystroke((SDL_Scancode) event->scancode, event->modifiers);
        break;
    case SmokeMouse:
        PushSmokeMouse(event->mouseX, event->mouseY, event->mouseButtons);
        break;
    case SmokeShot:
        SDL_strlcpy(smokeShotPending, event->shotName, sizeof(smokeShotPending));
        nextVideoPresentTick = 0x00;
        break;
    }
    ++smokeInputEventIndex;
    if (smokeInputEventIndex < smokeInputEventCount)
        smokeInputDueTime = HostClock_NowMS() +
            smokeInputEvents[smokeInputEventIndex].delayMilliseconds;
}

/* A smoke shot: what the window shows, as rendered, to NAME.bmp in the
 * folder REVENGE_SMOKE_SHOTS names (none without it). */
static void SaveSmokeShot(void)
{
    const char *folder = SDL_getenv("REVENGE_SMOKE_SHOTS");
    SDL_Surface *surface;
    char path[0x400];

    if (smokeShotPending[0x00] == '\0')
        return;
    if (folder != NULL && *folder != '\0')
    {
        surface = SDL_RenderReadPixels(renderer, NULL);
        SDL_snprintf(path, sizeof(path), "%s/%s.bmp", folder,
            smokeShotPending);
        if (surface == NULL || !SDL_SaveBMP(surface, path))
            RevengeTrace("input: smoke shot %s failed: %s\n", path,
                SDL_GetError());
        else
            RevengeTrace("input: smoke shot %s\n", path);
        SDL_DestroySurface(surface);
    }
    smokeShotPending[0x00] = '\0';
}

/*
 * The game's clock. Each of the original's timer interrupts is a tick of
 * 1193182 / divisor Hz (the PIT's input clock), counted from when it started:
 * tick k falls at start + k * divisor / 1193182 s, exactly, from the clock's
 * milliseconds. A pass runs every tick due by now, earliest first, ties in the
 * order below (the music timer's handler, 30B2:0010, ran its effect tick,
 * then the BIOS's chain, then the driver and the sequencer).
 */
enum
{
    PitInputHz = 0x1234DE,
    /* The BIOS's own 18.2 Hz, and the music timer's chain to it. */
    BiosTickDivisor = 0x10000,
    /* 30B2:0010 ran the effect tick once every MusicServiceReload + 1
     * interrupts: 1000h either way. */
    SpeakerEffectDivisor = 0x1000,
    /* ... and the sequencer once in 16 interrupts, with a music driver. */
    MIDISequenceInterrupts = 0x10
};

typedef enum ClockTimerId
{
    SpeakerEffectTimer,
    BiosTickTimer,
    MIDIDriverTimer,
    MIDISequenceTimer,
    DigitalPlaybackTimer,
    ClockTimerCount
} ClockTimerId;

typedef struct ClockTimer
{
    uint64_t startMS;
    uint64_t ticks;
    uint32_t divisor; /* zero: stopped */
} ClockTimer;

static ClockTimer clockTimers[ClockTimerCount] = {
    { 0, 0, 0 },              /* SpeakerEffectTimer */
    { 0, 0, BiosTickDivisor } /* BiosTickTimer */
};
static_assert(SpeakerEffectTimer == 0 && BiosTickTimer == 1,
    "the clock timers' initialiser lists them in this order");

static void StartClockTimer(ClockTimerId id, uint32_t divisor)
{
    clockTimers[id].startMS = HostClock_NowMS();
    clockTimers[id].ticks = 0;
    clockTimers[id].divisor = divisor;
}

/* The next tick's time, in 1/1193182 ms. */
static uint64_t NextTickTime(const ClockTimer *timer)
{
    return timer->startMS * PitInputHz +
        (timer->ticks + 1) * timer->divisor * UINT64_C(1000);
}

static void RunClockTimer(ClockTimerId id)
{
    switch (id)
    {
    case SpeakerEffectTimer:
        DecodePCSpeakerEffectTick();
        break;
    case BiosTickTimer:
        if (gameClockHandler != NULL)
            gameClockHandler();
        break;
    case MIDIDriverTimer:
        if (Seg426B.MusicDriverMode != 0x00)
            ServiceActiveMIDIDriver();
        break;
    case MIDISequenceTimer:
        if (Seg426B.MusicDriverMode != 0x00)
            ServiceMIDISequenceTick();
        break;
    case DigitalPlaybackTimer:
        ServiceDigitalAudioTick();
        break;
    default:
        break;
    }
}

static void DeliverHostEvents(void);

/* 3390:0024: the game's shutdown (2C7F:02E3) and the C library's exit, as
 * they end the game after its own quit (the title's, the in-game menu's).
 * 3390:0244, for the harness's fail item. */
void ShutdownAndExit(word16 exitCode);
void CloseTrackedFile(int16 handleSlot);

/* A quit the player asked of the host ends the program between the game's
 * timer services, as an interrupt would have come, the way the game's own
 * quit ends it: main returns 0 after ShutdownGame (the music stopped, the
 * timers and handlers given back, the screen reset), and the C library's
 * start-up passes that to its exit (0800:0102-0108, 38C6:0010), which is
 * ShutdownAndExit(0) (3390:0024). Then the host's end (audio and window
 * closed, files closed by the C runtime's exit). Nothing is saved: the
 * game saves only when the player asks it to. Once: the shutdown's own
 * waits service the clock. */
static void QuitIfAsked(void)
{
    static byte quitting;

    if (quitRequested == FALSE || quitting != FALSE)
        return;
    quitting = TRUE;
    RevengeTrace("quit: asked; the game's shutdown\n");
    ShutdownAndExit(0x0000);
}

/* A harness script's fail item (harness.h), taken where a quit would be.
 * The allocation: the heap's next request fails as if the host had no
 * memory, and the game goes on until it asks (revenge/heap.h: a fault, exit
 * 70). The abort: one of the game's fatal paths, a tracked file's slot
 * closed though it isn't open (3390:0244, then 3390:0002's two messages); it
 * ends the program with exit code 1 after its keys and doesn't return. */
static int failureRequested;

static void FailIfAsked(void)
{
    int failure = failureRequested;

    if (failure == HARNESS_FAIL_NONE)
        return;
    failureRequested = HARNESS_FAIL_NONE;
    RevengeTrace("fail: %s\n", failure == HARNESS_FAIL_ALLOCATION ? "allocation" : "abort");
    if (failure == HARNESS_FAIL_ALLOCATION)
    {
        Heap_FailNextAllocation();
        return;
    }
    for (int16 slot = 0x0000; slot < ORIGINAL_TRACKED_FILE_COUNT; ++slot)
    {
        if (ResourceFiles.trackedHandles[slot] == ORIGINAL_TRACKED_FILE_UNUSED)
        {
            CloseTrackedFile(slot);
            break;
        }
    }
    RevengeFault("fail: the abort returned");
}

void Platform_RequestHarnessFailure(int failure)
{
    failureRequested = failure;
}

/* A harness script's quit item (harness.h), as the host's quit comes. */
void Platform_RequestHarnessQuit(void)
{
    quitRequested = TRUE;
}

void SDLBackend_ServiceTimers(void)
{
    uint64_t now;

    HostClock_Step();
    now = HostClock_NowMS() * PitInputHz;
    for (;;)
    {
        int due = -1;
        uint64_t dueTime = 0;
        int id;

        for (id = 0; id < ClockTimerCount; ++id)
        {
            uint64_t time;

            if (clockTimers[id].divisor == 0)
                continue;
            time = NextTickTime(&clockTimers[id]);
            if (time <= now && (due < 0 || time < dueTime))
            {
                due = id;
                dueTime = time;
            }
        }
        if (due < 0)
            break;
        ++clockTimers[due].ticks;
        RunClockTimer((ClockTimerId) due);
    }
    SDLBackend_AdvanceAudio();
    /* The keyboard's and the mouse's interrupts, delivered at the same
     * passes as the timer's. */
    DeliverHostEvents();
    QuitIfAsked();
    FailIfAsked();
}

void SDLBackend_SetGameClockHandler(void (*handler)(void))
{
    gameClockHandler = handler;
}

/* 30B2:02D9 programmed the PIT to 1193182 / pitDivisor Hz for its handler. */
void SDLBackend_StartMusicClock(word16 pitDivisor)
{
    StartClockTimer(SpeakerEffectTimer, SpeakerEffectDivisor);
    StartClockTimer(MIDIDriverTimer, pitDivisor);
    StartClockTimer(MIDISequenceTimer,
        (uint32_t) pitDivisor * MIDISequenceInterrupts);
}

void SDLBackend_StopMusicClock(void)
{
    clockTimers[SpeakerEffectTimer].divisor = 0;
    clockTimers[MIDIDriverTimer].divisor = 0;
    clockTimers[MIDISequenceTimer].divisor = 0;
}

void SDLBackend_StartDigitalPlaybackClock(word16 pitDivisor)
{
    StartClockTimer(DigitalPlaybackTimer, pitDivisor);
}

void SDLBackend_StopDigitalPlaybackClock(void)
{
    clockTimers[DigitalPlaybackTimer].divisor = 0;
}

void SDLBackend_HoldTimers(uint64_t milliseconds)
{
    int id;

    for (id = 0; id < ClockTimerCount; ++id)
        clockTimers[id].startMS += milliseconds;
}

static void StoreRgba(uint32_t *pixel, byte red, byte green, byte blue)
{
    byte *components;

    components = (byte *) pixel;
    components[0x00] = red;
    components[0x01] = green;
    components[0x02] = blue;
    components[0x03] = 0xFF;
}

static void SetEgaColor(byte logicalColor, byte egaColor)
{
    byte red;
    byte green;
    byte blue;
    byte intensity;

    /* The palette a mode set leaves: the first 16 DAC entries hold the
       EGA's default colours, as on an RGBI monitor (bit 4 the shared
       intensity), the rest black. */
    intensity = (egaColor & 0x10) != 0 ? 0x55 : 0x00;
    red = (byte) (((egaColor & 0x04) != 0 ? 0xAA : 0x00) + intensity);
    green = (byte) (((egaColor & 0x02) != 0 ? 0xAA : 0x00) + intensity);
    blue = (byte) (((egaColor & 0x01) != 0 ? 0xAA : 0x00) + intensity);
    if ((egaColor & 0x17) == 0x06)
        green = 0x55;
    StoreRgba(&logicalPalette[logicalColor & 0x0F], red, green, blue);
}

static void ResetEgaPalette(void)
{
    static const byte defaultEgaColors[EGA_LOGICAL_PALETTE_ENTRY_COUNT] = {
        0x00, 0x01, 0x02, 0x03,
        0x04, 0x05, 0x14, 0x07,
        0x38, 0x39, 0x3A, 0x3B,
        0x3C, 0x3D, 0x3E, 0x3F
    };
    uint16 color;

    for (color = 0x0000; color < VGA_DAC_ENTRY_COUNT; ++color)
        StoreRgba(&logicalPalette[color], 0x00, 0x00, 0x00);
    for (color = 0x0000;
        color < EGA_LOGICAL_PALETTE_ENTRY_COUNT;
        ++color)
        SetEgaColor((byte) color, defaultEgaColors[color]);
}

enum
{
    ScanCodeExtended = 0x0100
};

/* A key's set-1 scan code, as the keyboard controller handed it to IRQ1;
 * ScanCodeExtended marks the codes that follow E0h (the grey keys, the right
 * Ctrl and Alt, the keypad's Enter and slash). Zero for a key with none, and
 * none above 58h (KeyboardInterruptHandler indexes its table by it). */
static uint16 GetSet1ScanCode(SDL_Scancode scanCode)
{
    switch (scanCode)
    {
    case SDL_SCANCODE_ESCAPE: return 0x01;
    case SDL_SCANCODE_1: return 0x02;
    case SDL_SCANCODE_2: return 0x03;
    case SDL_SCANCODE_3: return 0x04;
    case SDL_SCANCODE_4: return 0x05;
    case SDL_SCANCODE_5: return 0x06;
    case SDL_SCANCODE_6: return 0x07;
    case SDL_SCANCODE_7: return 0x08;
    case SDL_SCANCODE_8: return 0x09;
    case SDL_SCANCODE_9: return 0x0A;
    case SDL_SCANCODE_0: return 0x0B;
    case SDL_SCANCODE_MINUS: return 0x0C;
    case SDL_SCANCODE_EQUALS: return 0x0D;
    case SDL_SCANCODE_BACKSPACE: return 0x0E;
    case SDL_SCANCODE_TAB: return 0x0F;
    case SDL_SCANCODE_Q: return 0x10;
    case SDL_SCANCODE_W: return 0x11;
    case SDL_SCANCODE_E: return 0x12;
    case SDL_SCANCODE_R: return 0x13;
    case SDL_SCANCODE_T: return 0x14;
    case SDL_SCANCODE_Y: return 0x15;
    case SDL_SCANCODE_U: return 0x16;
    case SDL_SCANCODE_I: return 0x17;
    case SDL_SCANCODE_O: return 0x18;
    case SDL_SCANCODE_P: return 0x19;
    case SDL_SCANCODE_LEFTBRACKET: return 0x1A;
    case SDL_SCANCODE_RIGHTBRACKET: return 0x1B;
    case SDL_SCANCODE_RETURN: return 0x1C;
    case SDL_SCANCODE_KP_ENTER: return ScanCodeExtended | 0x1C;
    case SDL_SCANCODE_LCTRL: return 0x1D;
    case SDL_SCANCODE_RCTRL: return ScanCodeExtended | 0x1D;
    case SDL_SCANCODE_A: return 0x1E;
    case SDL_SCANCODE_S: return 0x1F;
    case SDL_SCANCODE_D: return 0x20;
    case SDL_SCANCODE_F: return 0x21;
    case SDL_SCANCODE_G: return 0x22;
    case SDL_SCANCODE_H: return 0x23;
    case SDL_SCANCODE_J: return 0x24;
    case SDL_SCANCODE_K: return 0x25;
    case SDL_SCANCODE_L: return 0x26;
    case SDL_SCANCODE_SEMICOLON: return 0x27;
    case SDL_SCANCODE_APOSTROPHE: return 0x28;
    case SDL_SCANCODE_GRAVE: return 0x29;
    case SDL_SCANCODE_LSHIFT: return 0x2A;
    case SDL_SCANCODE_BACKSLASH: return 0x2B;
    case SDL_SCANCODE_Z: return 0x2C;
    case SDL_SCANCODE_X: return 0x2D;
    case SDL_SCANCODE_C: return 0x2E;
    case SDL_SCANCODE_V: return 0x2F;
    case SDL_SCANCODE_B: return 0x30;
    case SDL_SCANCODE_N: return 0x31;
    case SDL_SCANCODE_M: return 0x32;
    case SDL_SCANCODE_COMMA: return 0x33;
    case SDL_SCANCODE_PERIOD: return 0x34;
    case SDL_SCANCODE_SLASH: return 0x35;
    case SDL_SCANCODE_KP_DIVIDE: return ScanCodeExtended | 0x35;
    case SDL_SCANCODE_RSHIFT: return 0x36;
    case SDL_SCANCODE_KP_MULTIPLY: return 0x37;
    case SDL_SCANCODE_LALT: return 0x38;
    case SDL_SCANCODE_RALT: return ScanCodeExtended | 0x38;
    case SDL_SCANCODE_SPACE: return 0x39;
    case SDL_SCANCODE_CAPSLOCK: return 0x3A;
    case SDL_SCANCODE_F1: return 0x3B;
    case SDL_SCANCODE_F2: return 0x3C;
    case SDL_SCANCODE_F3: return 0x3D;
    case SDL_SCANCODE_F4: return 0x3E;
    case SDL_SCANCODE_F5: return 0x3F;
    case SDL_SCANCODE_F6: return 0x40;
    case SDL_SCANCODE_F7: return 0x41;
    case SDL_SCANCODE_F8: return 0x42;
    case SDL_SCANCODE_F9: return 0x43;
    case SDL_SCANCODE_F10: return 0x44;
    case SDL_SCANCODE_NUMLOCKCLEAR: return 0x45;
    case SDL_SCANCODE_SCROLLLOCK: return 0x46;
    case SDL_SCANCODE_KP_7: return 0x47;
    case SDL_SCANCODE_HOME: return ScanCodeExtended | 0x47;
    case SDL_SCANCODE_KP_8: return 0x48;
    case SDL_SCANCODE_UP: return ScanCodeExtended | 0x48;
    case SDL_SCANCODE_KP_9: return 0x49;
    case SDL_SCANCODE_PAGEUP: return ScanCodeExtended | 0x49;
    case SDL_SCANCODE_KP_MINUS: return 0x4A;
    case SDL_SCANCODE_KP_4: return 0x4B;
    case SDL_SCANCODE_LEFT: return ScanCodeExtended | 0x4B;
    case SDL_SCANCODE_KP_5: return 0x4C;
    case SDL_SCANCODE_KP_6: return 0x4D;
    case SDL_SCANCODE_RIGHT: return ScanCodeExtended | 0x4D;
    case SDL_SCANCODE_KP_PLUS: return 0x4E;
    case SDL_SCANCODE_KP_1: return 0x4F;
    case SDL_SCANCODE_END: return ScanCodeExtended | 0x4F;
    case SDL_SCANCODE_KP_2: return 0x50;
    case SDL_SCANCODE_DOWN: return ScanCodeExtended | 0x50;
    case SDL_SCANCODE_KP_3: return 0x51;
    case SDL_SCANCODE_PAGEDOWN: return ScanCodeExtended | 0x51;
    case SDL_SCANCODE_KP_0: return 0x52;
    case SDL_SCANCODE_INSERT: return ScanCodeExtended | 0x52;
    case SDL_SCANCODE_KP_PERIOD: return 0x53;
    case SDL_SCANCODE_DELETE: return ScanCodeExtended | 0x53;
    case SDL_SCANCODE_F11: return 0x57;
    case SDL_SCANCODE_F12: return 0x58;
    default: return 0x0000;
    }
}

static void SendScanCode(byte scanCode)
{
    if (keyboardHandlerInstalled != FALSE)
        KeyboardInterruptHandler(scanCode);
}

static void SendKeyEvent(SDL_Scancode scancode, int down)
{
    uint16 scanCode = GetSet1ScanCode(scancode);

    if (scanCode == 0x0000)
        return;
    RevengeTrace("input: key scancode=%03X down=%u\n", scanCode, down ? 1 : 0);
    if ((scanCode & ScanCodeExtended) != 0x0000)
        SendScanCode(0xE0);
    SendScanCode((byte) ((scanCode & 0x00FF) | (down ? 0x00 : 0x80)));
}

uint16 Platform_ScanCodeForKey(int scancode)
{
    return GetSet1ScanCode((SDL_Scancode) scancode) & 0x00FF;
}

/* A scripted key (the harness's), sent as REVSNAP sends
 * it to the original (tools/capture/revsnap.asm): Shift, Ctrl, Alt pressed
 * as asked, the key pressed and released, the modifiers released; the key's
 * code without E0h, as tools/capture/keys.py has it. */
static void SendScriptedKey(int scancode, byte modifiers)
{
    byte scanCode = (byte) Platform_ScanCodeForKey(scancode);

    if ((modifiers & 0x01) != 0x00)
        SendScanCode(0x2A);
    if ((modifiers & 0x02) != 0x00)
        SendScanCode(0x1D);
    if ((modifiers & 0x04) != 0x00)
        SendScanCode(0x38);
    SendScanCode(scanCode);
    SendScanCode((byte) (scanCode | 0x80));
    if ((modifiers & 0x04) != 0x00)
        SendScanCode(0xB8);
    if ((modifiers & 0x02) != 0x00)
        SendScanCode(0x9D);
    if ((modifiers & 0x01) != 0x00)
        SendScanCode(0xAA);
}

/* Makes the renderer and the screen texture with the named SDL driver (NULL
 * for SDL's choice, which SDL_RENDER_DRIVER can set). Leaves neither behind
 * when any step fails. */
static int CreateScreenRenderer(const char *driver)
{
    renderer = SDL_CreateRenderer(window, driver);
    if (renderer != 0 && SDL_SetRenderLogicalPresentation(renderer, VIDEO_SCREEN_WIDTH, VIDEO_SCREEN_HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX))
    {
        texture = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_RGBA32,
            SDL_TEXTUREACCESS_STREAMING,
            VIDEO_SCREEN_WIDTH,
            VIDEO_SCREEN_HEIGHT);
        if (texture != 0 && SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST))
            return TRUE;
    }
    RevengeTrace("video: renderer %s failed: %s\n",
        driver != 0 ? driver : "(default)", SDL_GetError());
    SDL_DestroyTexture(texture);
    texture = 0;
    SDL_DestroyRenderer(renderer);
    renderer = 0;
    return FALSE;
}

const char *SDLBackend_RendererName(void)
{
    return renderer != 0 ? SDL_GetRendererName(renderer) : 0;
}

/* The window, its renderer and the audio, in turn, once SDL is up; FALSE at
 * the first that fails, with what was made left for SDLBackend_Close. */
static int OpenWindowRendererAndAudio(void)
{
    window = SDL_CreateWindow(
        "BattleTech: The Crescent Hawks' Revenge",
        VIDEO_SCREEN_WIDTH * WindowScale,
        VIDEO_SCREEN_HEIGHT * WindowScale,
        SDL_WINDOW_RESIZABLE);
    if (window == 0)
        return FALSE;

    /* Without a working GPU driver the game still runs: SDL's software
     * renderer draws the 320x200 screen on the CPU. */
    if (!CreateScreenRenderer(0) && !CreateScreenRenderer(SDL_SOFTWARE_RENDERER))
        return FALSE;
    RevengeTrace("video: renderer %s\n", SDLBackend_RendererName());

    ResetEgaPalette();
    memset(indexedFramebuffer, 0, sizeof(indexedFramebuffer));
    hostMouseButtons = 0x0000;
    return SDLBackend_OpenAudio();
}

int SDLBackend_Open(void)
{
    if (window != 0)
    {
        RevengeTrace("video: open reused window=%p\n", (void *) window);
        return TRUE;
    }

    RevengeTrace("video: creating SDL window\n");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO))
        return FALSE;
    if (!OpenWindowRendererAndAudio())
    {
        SDLBackend_Close();
        return FALSE;
    }
    RevengeTrace("video: SDL window ready window=%p renderer=%p\n",
        (void *) window, (void *) renderer);
    return TRUE;
}

void SDLBackend_Close(void)
{
    RevengeTrace("video: closing SDL window=%p\n", (void *) window);
    SDLBackend_CloseAudio();
    SDL_DestroyTexture(textScreen);
    textScreen = 0;
    SDL_DestroyTexture(texture);
    texture = 0;
    SDL_DestroyRenderer(renderer);
    renderer = 0;
    SDL_DestroyWindow(window);
    window = 0;
    SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO);
}

int SDLBackend_PollQuit(void)
{
    SDL_Event event;

    if (Harness_Active())
        return FALSE;
    if (quitRequested != FALSE)
    {
        quitRequested = FALSE;
        return TRUE;
    }
    SDL_PumpEvents();
    return SDL_PeepEvents(
               &event,
               0x01,
               SDL_GETEVENT,
               SDL_EVENT_QUIT,
               SDL_EVENT_QUIT) > 0x00;
}

uint64_t SDLBackend_Milliseconds(void)
{
    return HostClock_NowMS();
}

void SDLBackend_Delay(uint32_t milliseconds)
{
    HostClock_DelayMS(milliseconds);
}

byte SDLBackend_GetVideoMode(void)
{
    return currentVideoMode;
}

/* Modes 00h-03h and 07h are the BIOS's text modes. */
static int IsTextMode(byte videoMode)
{
    return videoMode <= 0x03 || videoMode == 0x07;
}

static void ClearTextPage(void)
{
    memset(textPage, ' ', sizeof(textPage));
    textRow = 0x0000;
    textColumn = 0x0000;
}

static void NextTextRow(void)
{
    if (++textRow < TextRows)
        return;
    memmove(textPage[0x00], textPage[0x01], sizeof(textPage) - TextColumns);
    memset(textPage[TextRows - 1], ' ', TextColumns);
    textRow = TextRows - 1;
}

/* DOS INT 21h/AH=06h, output: the BIOS's teletype rules, CR to the row's
 * start, LF down a row (the page scrolls up off the bottom), BS back one,
 * BEL nothing; any other byte its glyph, the row wrapping at 80. */
uint16 SDLBackend_DirectConsoleIO(byte character)
{
    if (character == 0xFF)
        return 0x0600;

    (void) fputc(character, stderr);
    if (character == '\n')
        (void) fflush(stderr);
    switch (character)
    {
    case '\r':
        textColumn = 0x0000;
        break;
    case '\n':
        NextTextRow();
        break;
    case 0x08:
        if (textColumn != 0x0000)
            --textColumn;
        break;
    case 0x07:
        break;
    default:
        textPage[textRow][textColumn] = (char) character;
        if (++textColumn == TextColumns)
        {
            textColumn = 0x0000;
            NextTextRow();
        }
        break;
    }
    return (uint16) (0x0600 | character);
}

const char *Platform_TextRow(uint16 row)
{
    return textPage[row < TextRows ? row : TextRows - 1];
}

/* The text page into the window's 320x200: drawn at 640x400 on a texture of
 * its own, then scaled as the screen is. */
static int RenderTextScreen(void)
{
    uint16 row;

    if (textScreen == 0)
    {
        textScreen = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
            SDL_TEXTUREACCESS_TARGET, TextScreenWidth, TextScreenHeight);
        if (textScreen == 0)
            return FALSE;
    }
    if (!SDL_SetRenderTarget(renderer, textScreen) || !SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF) || !SDL_RenderClear(renderer) || !SDL_SetRenderScale(renderer, 1.0f, 2.0f) || !SDL_SetRenderDrawColor(renderer, 0xAA, 0xAA, 0xAA, 0xFF))
    {
        (void) SDL_SetRenderTarget(renderer, 0);
        return FALSE;
    }
    for (row = 0x0000; row < TextRows; ++row)
    {
        char line[TextColumns + 1];
        uint16 column;

        for (column = 0x0000; column < TextColumns; ++column)
        {
            unsigned char glyph = (unsigned char) textPage[row][column];

            line[column] = glyph >= 0x20 && glyph < 0x7F ? (char) glyph : ' ';
        }
        line[TextColumns] = '\0';
        (void) SDL_RenderDebugText(renderer, 0.0f,
            (float) (row * TextGlyphSize), line);
    }
    (void) SDL_SetRenderScale(renderer, 1.0f, 1.0f);
    return SDL_SetRenderTarget(renderer, 0) && SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF) && SDL_RenderClear(renderer) && SDL_RenderTexture(renderer, textScreen, 0, 0);
}

void SDLBackend_SetVideoMode(byte videoMode)
{
    currentVideoMode = videoMode;
    if (IsTextMode(videoMode))
        ClearTextPage();
    if (videoMode != 0x03 && window == 0)
        (void) SDLBackend_Open();
}

void SDLBackend_LoadVgaPalette(const byte *paletteRgbTriplets)
{
    uint16 paletteIndex;

    if (paletteRgbTriplets == 0)
        return;
    for (paletteIndex = 0x0000;
        paletteIndex < VGA_DAC_ENTRY_COUNT;
        ++paletteIndex)
    {
        const byte *source;
        byte red;
        byte green;
        byte blue;

        source = paletteRgbTriplets + paletteIndex * 0x03;
        red = (byte) ((source[0x00] << 0x02) | (source[0x00] >> 0x04));
        green = (byte) ((source[0x01] << 0x02) | (source[0x01] >> 0x04));
        blue = (byte) ((source[0x02] << 0x02) | (source[0x02] >> 0x04));
        StoreRgba(&logicalPalette[paletteIndex], red, green, blue);
    }
}

/* The VGA's 70 Hz frames on the game's clock: frame f starts at f * 1000 / 70
 * ms, and its last tenth is the retrace. Times here are in 1/70 ms. */
static uint64_t FramePosition(uint64_t milliseconds)
{
    return milliseconds * RetracesPerSecond;
}

/* The first whole millisecond at or after a time in 1/70 ms. */
static uint64_t CeilingMS(uint64_t position)
{
    return (position + RetracesPerSecond - 1) / RetracesPerSecond;
}

/* The next frame's start after `now`, in milliseconds. */
static uint64_t NextFrameMS(uint64_t now)
{
    return CeilingMS((FramePosition(now) / 1000 + 1) * 1000);
}

void SDLBackend_WaitVerticalRetrace(byte waitForRetraceStart)
{
    uint64_t now;
    uint64_t frame;
    uint64_t until;

    SDLBackend_ServiceTimers();
    now = HostClock_NowMS();
    frame = FramePosition(now) / 1000;
    if (waitForRetraceStart == VERTICAL_RETRACE_WAIT_FOR_START)
    {
        /* The next retrace to begin: this frame's, or the next one's. */
        until = CeilingMS(frame * 1000 + RetraceStart);
        if (until <= now)
            until = CeilingMS((frame + 1) * 1000 + RetraceStart);
    }
    else
    {
        /* The original waits for retrace to begin and then end, so even an
         * invocation during active display targets the next frame. */
        until = NextFrameMS(now);
    }

    HostClock_DelayMS(until - now);
    (void) SDLBackend_PresentIndexedFramebuffer();
}

uint16 Platform_PaletteRgb(byte out[0x300])
{
    uint16 color;

    for (color = 0x0000; color < VGA_DAC_ENTRY_COUNT; ++color)
        memcpy(&out[color * 0x03], &logicalPalette[color], 0x03);
    return VGA_DAC_ENTRY_COUNT;
}

byte *SDLBackend_GetIndexedFramebuffer(void)
{
    return indexedFramebuffer;
}

void SDLBackend_ClearIndexedFramebuffer(byte color)
{
    memset(indexedFramebuffer, color, VIDEO_SCREEN_WIDTH * VIDEO_SCREEN_HEIGHT);
}

void SDLBackend_DrawIndexedPixel(uint16 x, uint16 y, byte color)
{
    if (x < VIDEO_SCREEN_WIDTH && y < VIDEO_SCREEN_HEIGHT)
        indexedFramebuffer[(uint32_t) y * VIDEO_SCREEN_WIDTH + x] = color;
}

int SDLBackend_PresentIndexedFramebuffer(void)
{
    uint32_t pixelIndex;

    /* Under the harness the indexed frame is what is captured; nothing is
     * shown. */
    if (texture == 0 || renderer == 0 || Harness_Active())
        return FALSE;

    if (IsTextMode(currentVideoMode))
    {
        if (!RenderTextScreen())
            return FALSE;
        SaveSmokeShot();
        return SDL_RenderPresent(renderer);
    }

    for (pixelIndex = 0x00000000;
        pixelIndex < VIDEO_SCREEN_WIDTH * VIDEO_SCREEN_HEIGHT;
        ++pixelIndex)
    {
        scanoutPixels[pixelIndex] =
            logicalPalette[indexedFramebuffer[pixelIndex]];
    }

    if (!SDL_UpdateTexture(
            texture,
            0,
            scanoutPixels,
            VIDEO_SCREEN_WIDTH * sizeof(uint32_t)) ||
        !SDL_RenderClear(renderer) || !SDL_RenderTexture(renderer, texture, 0, 0))
        return FALSE;
    SaveSmokeShot();
    return SDL_RenderPresent(renderer);
}

void Video_PresentTransientFrame(void)
{
    /* DOS effects become visible as soon as their pixels reach A000h.  SDL
     * has an explicit scanout stage, so publish the complete tactical-effect
     * frame before the preserved redraw overwrites it.  One VGA scan period
     * also prevents the much faster host loop from consuming every animation
     * frame between two visible refreshes. */
    (void) SDLBackend_PresentIndexedFramebuffer();
    {
        uint64_t now = HostClock_NowMS();

        HostClock_DelayMS(NextFrameMS(now) - now);
    }
}

static void PresentPendingVideoFrame(void)
{
    uint64_t now;

    now = HostClock_NowMS();
    if (now < nextVideoPresentTick)
        return;

    (void) SDLBackend_PresentIndexedFramebuffer();
    nextVideoPresentTick = now + VideoPresentPeriodMS;
}

/* Every event SDL has queued: keys to the game's keyboard handler, the
 * mouse's moves and buttons to its mouse handler, a quit (the window's
 * close button, the system's quit, SIGTERM) asked of the game's clock
 * (QuitIfAsked), the rest dropped.
 * Nothing under the harness, whose keys come at the game's polls. */
/* A mouse event as the driver called the game's handler: the buttons (1
 * left, 2 right, 4 middle) and the position in the game's 320x200, from the
 * window's coordinates. A position outside the game's picture (the pointer
 * gone past the window's edge, or over the letterbox) is not passed on: the
 * game keeps the last one seen inside, so the pointer leaving the window
 * never parks the game's cursor on an edge, where a battle scrolls. A
 * button there is passed at that last position. */
static void SendMouseEvent(float windowX, float windowY, int moved)
{
    float logicalX = windowX;
    float logicalY = windowY;

    if (mouseEventMask == 0x0000)
        return;
    if (renderer != NULL)
        (void) SDL_RenderCoordinatesFromWindow(
            renderer, windowX, windowY, &logicalX, &logicalY);
    if (pointerOutside == FALSE &&
        logicalX >= 0.0f && logicalX < VIDEO_SCREEN_WIDTH &&
        logicalY >= 0.0f && logicalY < VIDEO_SCREEN_HEIGHT)
    {
        lastInsideMouseX = (word16) logicalX;
        lastInsideMouseY = (word16) logicalY;
        lastInsideMouseSeen = TRUE;
    }
    else if (moved != FALSE || lastInsideMouseSeen == FALSE)
        return;
    RevengeTrace("input: mouse x=%u y=%u buttons=%u\n",
        (unsigned) lastInsideMouseX, (unsigned) lastInsideMouseY,
        (unsigned) hostMouseButtons);
    MouseEventCallback(hostMouseButtons, lastInsideMouseX, lastInsideMouseY);
}

static word16 MouseButtonBit(Uint8 button)
{
    switch (button)
    {
    case SDL_BUTTON_LEFT: return MOUSE_BUTTON_LEFT;
    case SDL_BUTTON_RIGHT: return MOUSE_BUTTON_RIGHT;
    case SDL_BUTTON_MIDDLE: return MOUSE_BUTTON_MIDDLE;
    default: return 0x0000;
    }
}

static void DeliverHostEvents(void)
{
    SDL_Event event;

    if (Harness_Active() || deliveringHostEvents != FALSE)
        return;
    deliveringHostEvents = TRUE;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            quitRequested = TRUE;
            break;
        case SDL_EVENT_KEY_DOWN:
            /* SDL's repeats are the keyboard's typematic make codes. */
            SendKeyEvent(event.key.scancode, TRUE);
            break;
        case SDL_EVENT_KEY_UP:
            SendKeyEvent(event.key.scancode, FALSE);
            break;
        case SDL_EVENT_WINDOW_MOUSE_LEAVE:
            pointerOutside = TRUE;
            break;
        case SDL_EVENT_WINDOW_MOUSE_ENTER:
            pointerOutside = FALSE;
            break;
        case SDL_EVENT_MOUSE_MOTION:
            SendMouseEvent(event.motion.x, event.motion.y, TRUE);
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            hostMouseButtons |= MouseButtonBit(event.button.button);
            SendMouseEvent(event.button.x, event.button.y, FALSE);
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            hostMouseButtons &= (word16) ~MouseButtonBit(event.button.button);
            SendMouseEvent(event.button.x, event.button.y, FALSE);
            break;
        default:
            break;
        }
    }
    deliveringHostEvents = FALSE;
}

void SDLBackend_PollInput(void)
{
    /* DOS tested the keyboard ring without waiting. Sleeping here turns the
       preserved queue-drain loops into multi-second stalls when a held key
       has generated repeat make codes. */
    SDLBackend_ServiceTimers();
    PresentPendingVideoFrame();
    if (Harness_Active())
    {
        if (Harness_KeyPending())
        {
            int scancode;
            byte modifiers;

            Harness_ReadKey(&scancode, &modifiers);
            SendScriptedKey(scancode, modifiers);
        }
        return;
    }
    SendDueSmokeInput();
}

void SDLBackend_WaitInput(void)
{
    if (!Harness_Active())
        (void) SDL_WaitEventTimeout(NULL, 0x08);
    SDLBackend_PollInput();
}

byte SDLBackend_ReadBiosKeyboardFlags(word16 offset)
{
    SDL_Keymod modifiers;
    byte flags = 0x00;

    if (Harness_Active())
        return 0x00;
    modifiers = SDL_GetModState();
    switch (offset)
    {
    case 0x0017:
        if ((modifiers & SDL_KMOD_RSHIFT) != 0)
            flags |= 0x01;
        if ((modifiers & SDL_KMOD_LSHIFT) != 0)
            flags |= 0x02;
        if ((modifiers & SDL_KMOD_CTRL) != 0)
            flags |= 0x04;
        if ((modifiers & SDL_KMOD_ALT) != 0)
            flags |= 0x08;
        if ((modifiers & SDL_KMOD_NUM) != 0)
            flags |= 0x20;
        if ((modifiers & SDL_KMOD_CAPS) != 0)
            flags |= 0x40;
        break;
    case 0x0018:
        if ((modifiers & SDL_KMOD_LCTRL) != 0)
            flags |= 0x01;
        if ((modifiers & SDL_KMOD_LALT) != 0)
            flags |= 0x02;
        break;
    case 0x0096:
        if ((modifiers & SDL_KMOD_RCTRL) != 0)
            flags |= 0x04;
        if ((modifiers & SDL_KMOD_RALT) != 0)
            flags |= 0x08;
        break;
    default:
        break;
    }
    return flags;
}

void SDLBackend_InstallKeyboardHandlers(void)
{
    keyboardHandlerInstalled = TRUE;
}

void SDLBackend_RestoreKeyboardHandlers(void)
{
    keyboardHandlerInstalled = FALSE;
}

/* A harness script's mouse item (harness.h), as REVSNAP calls the handler
 * for one. */
void Platform_SendHarnessMouse(uint16 x, uint16 y, byte buttons)
{
    if (mouseEventMask != 0x0000)
        MouseEventCallback(buttons, x, y);
}

void SDLBackend_SetMouseEventHandler(word16 eventMask)
{
    mouseEventMask = eventMask;
}

void SDLBackend_SetMouseVisible(uint16 visible)
{
    if (visible != FALSE)
        (void) SDL_ShowCursor();
    else
        (void) SDL_HideCursor();
}

void SDLBackend_SetMousePosition(int16 logicalX, int16 logicalY)
{
    float windowX;
    float windowY;

    /* Under the harness the mouse moves only by the script's items, each
     * with its own position. */
    if (Harness_Active() || window == NULL)
        return;
    windowX = (float) logicalX;
    windowY = (float) logicalY;
    if (renderer != NULL)
    {
        (void) SDL_RenderCoordinatesToWindow(
            renderer, windowX, windowY, &windowX, &windowY);
    }
    SDL_WarpMouseInWindow(window, windowX, windowY);
}
