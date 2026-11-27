#include "revenge/platform_backend.h"

#include <stdio.h>

uint16 SDLBackend_DirectConsoleIO(byte character)
{
    if (character == 0xFF)
        return 0x0600;

    (void)fwrite(&character, 0x01, 0x01, stdout);
    (void)fflush(stdout);
    return (uint16)(0x0600 | character);
}
