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

/* Writes the UPGRADES switches as they are now (include/revenge/upgrades.h)
 * to the settings file, its media lines as they were; nothing in test mode
 * or without a write folder. */
void HostSettings_SaveUpgrades(void);

/* The setup menu's RESET SETTINGS (src/SDL/kit_menu.cpp): the file
 * removed and made again as a first start makes it (from the install's
 * MEDIA.CFG, else our defaults, every UPGRADES switch off), and taken in.
 * The switches change at once; the media choices pick the game's drivers
 * as it starts, so they take effect at the next start. Returns 0 when the
 * file is there and can't be removed (nothing changed). */
int HostSettings_Reset(void);

/* The file's name, for messages and tests. */
#define HOST_SETTINGS_FILE_NAME "settings.txt"

#endif
