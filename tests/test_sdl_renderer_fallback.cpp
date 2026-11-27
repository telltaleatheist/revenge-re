#include "revenge/platform_backend.h"

#include <SDL3/SDL.h>
#include <assert.h>
#include <string.h>

/* With no usable renderer of SDL's choosing (here SDL_RENDER_DRIVER names
 * one that doesn't exist), the screen opens on SDL's software renderer. The
 * test sets the dummy video and audio drivers, so no window is shown. */
int main(void)
{
    const char *name;

    assert(SDL_setenv_unsafe("SDL_MAC_BACKGROUND_APP", "1", 1) == 0);
    assert(SDL_setenv_unsafe("SDL_VIDEO_DRIVER", "dummy", 1) == 0);
    assert(SDL_setenv_unsafe("SDL_AUDIO_DRIVER", "dummy", 1) == 0);
    assert(SDL_setenv_unsafe("SDL_RENDER_DRIVER", "no-such-renderer", 1) == 0);
    assert(SDLBackend_RendererName() == NULL);
    assert(SDLBackend_Open());
    name = SDLBackend_RendererName();
    assert(name != NULL && strcmp(name, SDL_SOFTWARE_RENDERER) == 0);
    assert(SDLBackend_PresentIndexedFramebuffer());
    SDLBackend_Close();
    assert(SDLBackend_RendererName() == NULL);
    return 0;
}
