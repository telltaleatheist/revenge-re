#ifndef REVENGE_SDL_SETUP_HOST_H
#define REVENGE_SDL_SETUP_HOST_H

/*
 * The program's side of the setup (src/Setup/Setup.h is the recipe): the
 * launcher contract of re-setup-kit (extern/re-setup-kit, DESIGN.md there),
 * the launcher's start options, and the game folder a start plays from.
 * Nothing here is the original's: the DOS game was started from its own
 * folder by hand.
 */


/*
 * Everything the command line asks before the game starts. Returns -1 when
 * the game goes on, else the exit code:
 *
 *   --status                  one JSON line on stdout, nothing written, no window
 *   --setup                   the setup screen, nothing played (0 ready, 1 cancelled, 2 failed)
 *   --setup --describe | --search | --copies | --look PICK | --install PICK [--folder DIR]
 *                             the setup on the launcher's own screens: JSON lines on stdout
 *   --help                    the usage
 *
 * and, for a start that plays, the launcher's options: --windowed, --window
 * WxH, --nosound, --hidden, --write-dir DIR and --ready-file FILE. A start
 * plays from REVENGE_ASSET_DIRECTORY when it is set, or as REVENGE.CFG
 * beside the program says when there is one (a build tree's); else from the
 * folder the setup chose (setup.txt in the player's folder), found, or
 * picked on the setup screen, which then becomes REVENGE_ASSET_DIRECTORY.
 */
int SetupHost_Start(int argc, char **argv);


#endif
