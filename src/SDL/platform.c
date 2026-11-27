#include "revenge/platform_backend.h"
#include "revenge/audio.h"
#include "revenge/debug.h"
#include "revenge/input.h"
#include "revenge/original_data.h"
#include "revenge/video.h"
#include "harness.h"

#include <SDL3/SDL.h>
#include <stdlib.h>
#include <string.h>

enum
{
    WindowScale = 0x03,
    RetracesPerSecond = 0x46,
    RetraceStatusBit = 0x08
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
static uint64_t nextSpeakerEffectTick;
static uint64_t nextMIDIServiceTick;
static uint64_t nextMIDIDriverTick;
static uint64_t nextGameClockTick;
static uint64_t nextDigitalPlaybackTick;
static uint64_t nextVideoPresentTick;
static byte previousMouseButtons;

enum
{
    SmokeInputCapacity = 0x80
};


typedef struct SmokeInputEvent
{
    uint32_t delayMilliseconds;
    int scancode;
    byte modifiers;
} SmokeInputEvent;

static SmokeInputEvent smokeInputEvents[SmokeInputCapacity];
static uint16 smokeInputEventCount;
static uint16 smokeInputEventIndex;
static uint64_t smokeInputDueTime;
static byte smokeInputInitialized;
static byte keyboardHandlerInstalled;
static byte deliveringHostEvents;
static byte quitRequested;

static void SendScriptedKey(int scancode, byte modifiers);

/* REVENGE_SMOKE_INPUT: DELAY_MS:KEY[,DELAY_MS:KEY...], each KEY a key name
 * as the harness's scripts write them (Shift+, Ctrl+, Alt+ allowed). */
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
        char name[0x40];
        size_t length;
        unsigned long delay = strtoul(cursor, &end, 0x0A);
        SmokeInputEvent *event = &smokeInputEvents[smokeInputEventCount];

        if (end == cursor || *end != ':')
            break;
        cursor = end + 0x01;
        length = strcspn(cursor, ",");
        if (length == 0x00 || length >= sizeof(name))
            break;
        memcpy(name, cursor, length);
        name[length] = '\0';
        if (!Harness_ParseKey(name, &event->scancode, &event->modifiers))
            break;
        event->delayMilliseconds = (uint32_t)delay;
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

static void SendDueSmokeInput(void)
{
    const SmokeInputEvent *event;

    InitializeSmokeInput();
    if (smokeInputEventIndex >= smokeInputEventCount ||
        HostClock_NowMS() < smokeInputDueTime)
        return;
    event = &smokeInputEvents[smokeInputEventIndex];
    RevengeTrace("input: smoke event=%u/%u scancode=%d modifiers=%u\n",
        smokeInputEventIndex + 0x01, smokeInputEventCount,
        event->scancode, event->modifiers);
    SendScriptedKey(event->scancode, event->modifiers);
    ++smokeInputEventIndex;
    if (smokeInputEventIndex < smokeInputEventCount)
        smokeInputDueTime = HostClock_NowMS() +
            smokeInputEvents[smokeInputEventIndex].delayMilliseconds;
}

static void DeliverHostEvents(void);

void SDLBackend_ServiceTimers(void)
{
    const uint64_t speakerEffectPeriod =
        UINT64_C(60000000000) / 0x4440;
    const uint64_t midiServicePeriod =
        UINT64_C(60000000000) / 0x1110;
    /* With a music driver, the music timer ran the driver's own service on
     * every IRQ0, the PIT at 1193182 / 400h Hz (30B2:0010, 30B2:02D9). */
    const uint64_t midiDriverPeriod =
        UINT64_C(1000000000) * 0x400 / 0x1234DE;
    const uint64_t gameClockPeriod =
        UINT64_C(1000000000) * 0x10000 / 0x1234DE;
    uint64_t now;

    HostClock_Step();
    now = HostClock_NowNS();
    if (DigitalSoundBusy != FALSE &&
        DigitalPlaybackRateHz != 0x0000)
    {
        const uint64_t digitalPlaybackPeriod = UINT64_C(1000000000) /
            DigitalPlaybackRateHz;

        if (nextDigitalPlaybackTick == 0x0000000000000000ULL)
            nextDigitalPlaybackTick = now;
        while (nextDigitalPlaybackTick <= now &&
            DigitalSoundBusy != FALSE)
        {
            /* DOS delivered the currently installed IRQ0 handler from the
             * programmable interval timer.  The hosted boundary performs
             * the same dispatch cooperatively. */
            SDLBackend_DispatchInstalledInterruptVector(0x08);
            nextDigitalPlaybackTick += digitalPlaybackPeriod;
        }
        if (DigitalSoundBusy == FALSE)
            nextDigitalPlaybackTick = 0x0000000000000000ULL;
    }
    else
    {
        nextDigitalPlaybackTick = 0x0000000000000000ULL;
    }
    while (nextSpeakerEffectTick <= now)
    {
        DecodePCSpeakerEffectTick();
        nextSpeakerEffectTick += speakerEffectPeriod;
    }
    while (nextMIDIDriverTick <= now)
    {
        if (Seg426B.MusicDriverMode != 0x00)
            ServiceActiveMIDIDriver();
        nextMIDIDriverTick += midiDriverPeriod;
    }
    while (nextMIDIServiceTick <= now)
    {
        ServiceMIDISequenceTick();
        nextMIDIServiceTick += midiServicePeriod;
    }
    while (nextGameClockTick <= now)
    {
        SDLBackend_DispatchInterrupt(0x1C);
        nextGameClockTick += gameClockPeriod;
    }
    SDLBackend_AdvanceAudio();
    /* The keyboard's and the mouse's interrupts, delivered at the same
     * passes as the timer's. */
    DeliverHostEvents();
}

void SDLBackend_HoldTimers(uint64_t nanoseconds)
{
    nextSpeakerEffectTick += nanoseconds;
    nextMIDIDriverTick += nanoseconds;
    nextMIDIServiceTick += nanoseconds;
    nextGameClockTick += nanoseconds;
}

static void StoreRgba(uint32_t *pixel, byte red, byte green, byte blue)
{
    byte *components;

    components = (byte *)pixel;
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
    red = (byte)(((egaColor & 0x04) != 0 ? 0xAA : 0x00) + intensity);
    green = (byte)(((egaColor & 0x02) != 0 ? 0xAA : 0x00) + intensity);
    blue = (byte)(((egaColor & 0x01) != 0 ? 0xAA : 0x00) + intensity);
    if ((egaColor & 0x17) == 0x06)
        green = 0x55;
    StoreRgba(&logicalPalette[logicalColor & 0x0F], red, green, blue);
}

static void ResetEgaPalette(void)
{
    static const byte defaultEgaColors[EGA_LOGICAL_PALETTE_ENTRY_COUNT] =
    {
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
        SetEgaColor((byte)color, defaultEgaColors[color]);
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
    SendScanCode((byte)((scanCode & 0x00FF) | (down ? 0x00 : 0x80)));
}

uint16 Platform_ScanCodeForKey(int scancode)
{
    return GetSet1ScanCode((SDL_Scancode)scancode) & 0x00FF;
}

/* A scripted key (the harness's, the smoke input's), sent as REVSNAP sends
 * it to the original (tools/capture/revsnap.asm): Shift, Ctrl, Alt pressed
 * as asked, the key pressed and released, the modifiers released; the key's
 * code without E0h, as tools/capture/keys.py has it. */
static void SendScriptedKey(int scancode, byte modifiers)
{
    byte scanCode = (byte)Platform_ScanCodeForKey(scancode);

    if ((modifiers & 0x01) != 0x00)
        SendScanCode(0x2A);
    if ((modifiers & 0x02) != 0x00)
        SendScanCode(0x1D);
    if ((modifiers & 0x04) != 0x00)
        SendScanCode(0x38);
    SendScanCode(scanCode);
    SendScanCode((byte)(scanCode | 0x80));
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
    if (renderer != 0
        && SDL_SetRenderLogicalPresentation(
            renderer,
            VIDEO_SCREEN_WIDTH,
            VIDEO_SCREEN_HEIGHT,
            SDL_LOGICAL_PRESENTATION_LETTERBOX))
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

int SDLBackend_Open(void)
{
    if (window != 0)
    {
        RevengeTrace("video: open reused window=%p\n", (void *)window);
        return TRUE;
    }

    RevengeTrace("video: creating SDL window\n");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO))
        return FALSE;

    window = SDL_CreateWindow(
        "BattleTech: The Crescent Hawks' Revenge",
        VIDEO_SCREEN_WIDTH * WindowScale,
        VIDEO_SCREEN_HEIGHT * WindowScale,
        SDL_WINDOW_RESIZABLE);
    if (window == 0)
        goto failed;

    /* Without a working GPU driver the game still runs: SDL's software
     * renderer draws the 320x200 screen on the CPU. */
    if (!CreateScreenRenderer(0)
        && !CreateScreenRenderer(SDL_SOFTWARE_RENDERER))
        goto failed;
    RevengeTrace("video: renderer %s\n", SDLBackend_RendererName());

    ResetEgaPalette();
    memset(indexedFramebuffer, 0, sizeof(indexedFramebuffer));
    nextSpeakerEffectTick = HostClock_NowNS();
    nextMIDIServiceTick = nextSpeakerEffectTick;
    nextMIDIDriverTick = nextSpeakerEffectTick;
    nextGameClockTick = nextSpeakerEffectTick;
    nextDigitalPlaybackTick = 0x0000000000000000ULL;
    previousMouseButtons = 0x00;
    if (!SDLBackend_OpenAudio())
        goto failed;
    RevengeTrace("video: SDL window ready window=%p renderer=%p\n",
        (void *)window, (void *)renderer);
    return TRUE;

failed:
    SDLBackend_Close();
    return FALSE;
}

void SDLBackend_Close(void)
{
    RevengeTrace("video: closing SDL window=%p\n", (void *)window);
    SDLBackend_CloseAudio();
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
    HostClock_DelayNS((uint64_t)milliseconds * UINT64_C(1000000));
}

byte SDLBackend_GetVideoMode(void)
{
    return currentVideoMode;
}

void SDLBackend_SetVideoMode(byte videoMode)
{
    currentVideoMode = videoMode;
    if (videoMode != 0x03 && window == 0)
        (void)SDLBackend_Open();
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
        red = (byte)((source[0x00] << 0x02) | (source[0x00] >> 0x04));
        green = (byte)((source[0x01] << 0x02) | (source[0x01] >> 0x04));
        blue = (byte)((source[0x02] << 0x02) | (source[0x02] >> 0x04));
        StoreRgba(&logicalPalette[paletteIndex], red, green, blue);
    }
}

uint16 SDLBackend_IsVerticalRetraceActive(void)
{
    uint64_t period;
    uint64_t phase;

    HostClock_Step();
    period = UINT64_C(1000000000) / RetracesPerSecond;
    phase = HostClock_NowNS() % period;
    return phase >= period - period / 0x0A ? RetraceStatusBit : 0x0000;
}

void SDLBackend_WaitVerticalRetrace(byte waitForRetraceStart)
{
    uint64_t delay;
    uint64_t now;
    uint64_t period;
    uint64_t phase;
    uint64_t retraceStart;

    SDLBackend_ServiceTimers();
    period = UINT64_C(1000000000) / RetracesPerSecond;
    now = HostClock_NowNS();
    phase = now % period;
    retraceStart = period - period / 0x0A;

    if (waitForRetraceStart == VERTICAL_RETRACE_WAIT_FOR_START)
    {
        if (phase < retraceStart)
            delay = retraceStart - phase;
        else
            delay = period - phase + retraceStart;
    }
    else
    {
        /* The original waits for retrace to begin and then end, so even an
         * invocation during active display targets the next period boundary. */
        delay = period - phase;
    }

    HostClock_DelayNS(delay);
    (void)SDLBackend_PresentIndexedFramebuffer();
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
        indexedFramebuffer[(uint32_t)y * VIDEO_SCREEN_WIDTH + x] = color;
}

int SDLBackend_PresentIndexedFramebuffer(void)
{
    uint32_t pixelIndex;

    /* Under the harness the indexed frame is what is captured; nothing is
     * shown. */
    if (texture == 0 || renderer == 0 || Harness_Active())
        return FALSE;

    for (pixelIndex = 0x00000000;
         pixelIndex < VIDEO_SCREEN_WIDTH * VIDEO_SCREEN_HEIGHT;
         ++pixelIndex)
    {
        scanoutPixels[pixelIndex] =
            logicalPalette[indexedFramebuffer[pixelIndex]];
    }

    return SDL_UpdateTexture(
        texture,
        0,
        scanoutPixels,
        VIDEO_SCREEN_WIDTH * sizeof(uint32_t))
        && SDL_RenderClear(renderer)
        && SDL_RenderTexture(renderer, texture, 0, 0)
        && SDL_RenderPresent(renderer);
}

void Video_PresentTransientFrame(void)
{
    /* DOS effects become visible as soon as their pixels reach A000h.  SDL
     * has an explicit scanout stage, so publish the complete tactical-effect
     * frame before the preserved redraw overwrites it.  One VGA scan period
     * also prevents the much faster host loop from consuming every animation
     * frame between two visible refreshes. */
    (void)SDLBackend_PresentIndexedFramebuffer();
    HostClock_DelayNS(UINT64_C(1000000000) / RetracesPerSecond);
}

static void PresentPendingVideoFrame(void)
{
    const uint64_t videoPresentPeriod = UINT64_C(1000000000) / 0x3C;
    uint64_t now;

    now = HostClock_NowNS();
    if (now < nextVideoPresentTick)
        return;

    (void)SDLBackend_PresentIndexedFramebuffer();
    nextVideoPresentTick = now + videoPresentPeriod;
}

/* Every event SDL has queued: keys to the game's keyboard handler, closing
 * the window as Escape (as the port has always taken it), the rest dropped.
 * Nothing under the harness, whose keys come at the game's polls. */
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
            SendScanCode(0x01);
            SendScanCode(0x81);
            break;
        case SDL_EVENT_KEY_DOWN:
            /* SDL's repeats are the keyboard's typematic make codes. */
            SendKeyEvent(event.key.scancode, TRUE);
            break;
        case SDL_EVENT_KEY_UP:
            SendKeyEvent(event.key.scancode, FALSE);
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
        (void)SDL_WaitEventTimeout(NULL, 0x08);
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

void ReadMouseInputState(MouseInputState *state)
{
    float windowX;
    float windowY;
    float logicalX;
    float logicalY;
    SDL_MouseButtonFlags hostButtons;
    byte buttons;

    if (state == NULL)
        return;
    if (Harness_Active())
    {
        Harness_Mouse(&state->x, &state->y, &buttons);
        state->buttonState = buttons;
        state->buttonEvent = (byte)((buttons & (byte)~previousMouseButtons)
            != 0x00);
        previousMouseButtons = buttons;
        return;
    }
    SDL_PumpEvents();
    hostButtons = SDL_GetMouseState(&windowX, &windowY);
    logicalX = windowX;
    logicalY = windowY;
    if (renderer != NULL)
        (void)SDL_RenderCoordinatesFromWindow(
            renderer, windowX, windowY, &logicalX, &logicalY);
    if (logicalX < 0.0f)
        logicalX = 0.0f;
    if (logicalY < 0.0f)
        logicalY = 0.0f;
    if (logicalX >= VIDEO_SCREEN_WIDTH)
        logicalX = VIDEO_SCREEN_WIDTH - 0x01;
    if (logicalY >= VIDEO_SCREEN_HEIGHT)
        logicalY = VIDEO_SCREEN_HEIGHT - 0x01;

    buttons = 0x00;
    if ((hostButtons & SDL_BUTTON_LMASK) != 0x00)
        buttons |= MOUSE_BUTTON_LEFT;
    if ((hostButtons & SDL_BUTTON_RMASK) != 0x00)
        buttons |= MOUSE_BUTTON_RIGHT;
    state->x = (uint16)logicalX;
    state->y = (uint16)logicalY;
    state->buttonState = buttons;
    state->buttonEvent = (byte)((buttons & (byte)~previousMouseButtons)
        != 0x00);
    previousMouseButtons = buttons;
}

void SDLBackend_SetMouseVisible(uint16 visible)
{
    if (visible != FALSE)
        (void)SDL_ShowCursor();
    else
        (void)SDL_HideCursor();
}

void SDLBackend_SetMousePosition(int16 logicalX, int16 logicalY)
{
    float windowX;
    float windowY;

    if (Harness_Active())
    {
        Harness_PlaceMouse(logicalX, logicalY);
        return;
    }
    if (window == NULL)
        return;
    windowX = (float)logicalX;
    windowY = (float)logicalY;
    if (renderer != NULL)
    {
        (void)SDL_RenderCoordinatesToWindow(
            renderer, windowX, windowY, &windowX, &windowY);
    }
    SDL_WarpMouseInWindow(window, windowX, windowY);
}
