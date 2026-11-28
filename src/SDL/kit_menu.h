#ifndef REVENGE_SDL_KIT_MENU_H
#define REVENGE_SDL_KIT_MENU_H
#include <SDL3/SDL.h>

/*
 * re-setup-kit's setup menu over the running game (extern/re-setup-kit,
 * rekit/sdl/game_menu.h; DESIGN.md there, "The menu inside a game"): Shift +
 * Alt + F1 (Cmd + Shift + F1 on a Mac), or the UPGRADES page's Setup menu
 * line. Nothing of it is the original's: the DOS game had none. There is no
 * menu under the trace harness, and with RE_KIT_TEST=1 the kit takes no keys
 * and shows no hint.
 *
 * src/SDL/platform.cpp hands it every SDL event first (the game's come back
 * through `deliver`), and asks for its picture after each pass of events.
 * While it is open the game stands still where it is (in the event pass) and
 * its clock is paused (harness.h, HostClock_Pause), so nothing catches up
 * when it closes.
 */

/* What the platform calls (src/SDL/platform.cpp, Platform_SetMenu): the
 * functions below, registered by the program on a start that plays
 * (src/SDL/setup_host.cpp). A program or test without them has no menu. */
typedef struct PlatformMenu
{
    void (*start)(SDL_Window *window, void (*deliver)(const SDL_Event *event));
    void (*stop)(void);
    int (*event)(const SDL_Event *event);
    int (*frame)(int now);
    int (*isOpen)(void);
    void (*hide)(void);
    void (*show)(void);
    void (*drawOver)(SDL_Renderer *renderer);
} PlatformMenu;
void Platform_SetMenu(const PlatformMenu *menu);
const PlatformMenu *KitMenu_Platform(void);

/* Makes the menu for this window, once; `deliver` takes the game's events. */
void KitMenu_Start(SDL_Window *window, void (*deliver)(const SDL_Event *event));
void KitMenu_Stop(void);
/* 0 when there is no menu (the caller handles the event); else 1, the event
 * kept by the kit or handed to `deliver`. */
int KitMenu_Event(const SDL_Event *event);
/* The kit's frame (the menu or the hint, or neither); 1 when the picture
 * over the game changed (drawn, changed or gone), so the frame is shown
 * again. At most one a 16 ms unless `now`. */
int KitMenu_Frame(int now);
int KitMenu_IsOpen(void);
void KitMenu_Show(void);
void KitMenu_Hide(void);
/* The picture over the frame on `renderer`, at the window's pixel size,
 * when there is one. */
void KitMenu_DrawOver(SDL_Renderer *renderer);

#endif
