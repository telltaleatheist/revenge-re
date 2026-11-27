#include "revenge/platform_backend.h"
#include "revenge/original_data.h"

#include <SDL3/SDL.h>
#include <assert.h>

/* 0800:36A6, declared in its caller's file only. */
void InitializeMouseSupport(void);

/* The pointer leaving the window: SDL's mouse events reach the game's mouse
 * handler (src/SDL/platform.cpp) only while the pointer is over the game's
 * picture. A move past the window's edge, or any move after SDL's
 * mouse-leave, is not passed on, so the game's cursor keeps the last
 * position seen inside and is never parked on an edge, where a battle
 * scrolls by itself (1A1F TranslateTacticalMouseInput). A button pressed
 * meanwhile arrives at that last position. The dummy video driver: no
 * window is shown. */

static SDL_Window *window;

static void WindowPoint(float x, float y, float *windowX, float *windowY)
{
    assert(SDL_RenderCoordinatesToWindow(SDL_GetRenderer(window),
        x + 0.5f, y + 0.5f, windowX, windowY));
}

static void Push(SDL_Event *event)
{
    assert(SDL_PushEvent(event));
}

static void Move(float windowX, float windowY)
{
    SDL_Event event;

    SDL_zero(event);
    event.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.windowID = SDL_GetWindowID(window);
    event.motion.x = windowX;
    event.motion.y = windowY;
    Push(&event);
}

static void MoveTo(float x, float y)
{
    float windowX;
    float windowY;

    WindowPoint(x, y, &windowX, &windowY);
    Move(windowX, windowY);
}

static void Crossing(SDL_EventType type)
{
    SDL_Event event;

    SDL_zero(event);
    event.type = type;
    event.window.windowID = SDL_GetWindowID(window);
    Push(&event);
}

static void Button(int down, float windowX, float windowY)
{
    SDL_Event event;

    SDL_zero(event);
    event.type = down ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
    event.button.windowID = SDL_GetWindowID(window);
    event.button.button = SDL_BUTTON_LEFT;
    event.button.down = down ? true : false;
    event.button.x = windowX;
    event.button.y = windowY;
    Push(&event);
}

static void Deliver(void)
{
    SDLBackend_ServiceTimers();
}

static void ExpectCursor(int16 x, word16 y)
{
    assert(MouseCursorRequestedX == x && MouseY == y);
}

int main(void)
{
    SDL_Window **windows;
    int width;
    int height;

    assert(SDL_setenv_unsafe("SDL_MAC_BACKGROUND_APP", "1", 1) == 0);
    assert(SDL_setenv_unsafe("SDL_VIDEO_DRIVER", "dummy", 1) == 0);
    assert(SDL_setenv_unsafe("SDL_AUDIO_DRIVER", "dummy", 1) == 0);
    assert(SDLBackend_Open());
    windows = SDL_GetWindows(NULL);
    assert(windows != NULL && windows[0] != NULL);
    window = windows[0];
    SDL_free(windows);
    assert(SDL_GetWindowSize(window, &width, &height));
    InitializeMouseSupport();
    Deliver();

    /* Inside, the cursor follows. */
    MoveTo(150.0f, 100.0f);
    Deliver();
    ExpectCursor(150, 100);

    /* Out past the right edge (as SDL reports a drag), then the leave, then
	 * the edge itself after it: the cursor stays where it was seen last. */
    Move((float) width + 20.0f, (float) height / 2.0f);
    Crossing(SDL_EVENT_WINDOW_MOUSE_LEAVE);
    Move((float) width - 1.0f, (float) height / 2.0f);
    Move(0.0f, 0.0f);
    Deliver();
    ExpectCursor(150, 100);

    /* A button while outside: pressed at the last position. */
    Button(TRUE, (float) width + 20.0f, 0.0f);
    Deliver();
    assert(MouseButtonState == 0x0001);
    ExpectCursor(150, 100);
    Button(FALSE, (float) width + 20.0f, 0.0f);
    Deliver();
    assert(MouseButtonState == 0x0000);

    /* Back in: the cursor follows again. */
    Crossing(SDL_EVENT_WINDOW_MOUSE_ENTER);
    MoveTo(200.0f, 120.0f);
    Deliver();
    ExpectCursor(200, 120);

    /* Over the edge but still on the picture: passed on, as the player
	 * means it (the battle's edge scroll). */
    MoveTo(319.0f, 120.0f);
    Deliver();
    ExpectCursor(319, 120);

    SDLBackend_Close();
    return 0;
}
