#ifndef REVENGE_SDL_SETTINGS_H
#define REVENGE_SDL_SETTINGS_H

/*
 * The player's settings file, settings.txt in our write folder: the game's
 * four media options (video, music, sound, input) in our own words. At the
 * first start it is made from the install's MEDIA.CFG (the DOS installer's
 * choices), or from our defaults when there is none; from then on the game
 * reads only it. Without a write folder the choices are made the same way
 * and kept in memory.
 *
 * Returns 1, or stops with a fault naming the file and line when the file
 * holds something it can't read.
 */
int HostSettings_Prepare(void);

/* The file's name, for messages and tests. */
#define HOST_SETTINGS_FILE_NAME "settings.txt"

#endif
