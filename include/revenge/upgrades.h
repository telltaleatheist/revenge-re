#ifndef REVENGE_UPGRADES_H
#define REVENGE_UPGRADES_H

/*
 * Ours: the UPGRADES switches, fixes for the original's own bugs that a
 * player can turn on (docs/ORIGINAL_BUGS.md, Switches). Each is off unless
 * the player turns it on, so the game plays as the original did by default.
 * The player's settings file keeps them as "upgrade_<key> = on|off"
 * (src/SDL/settings.cpp); the UPGRADES page changes them.
 *
 * Under the trace harness or the kit's test mode the file's switches are not
 * read: every switch is off, unless the run names some on itself (a script's
 * "# upgrades:" line, src/SDL/settings.cpp), so every recorded run plays the
 * original's path.
 */
enum
{
    /* A lance left waiting on a leader that can't walk goes after the last
     * enemy once nothing is in sight (Tactical/UnitState.cpp, 2D1B:08FA). */
    UPGRADE_LANCE_PURSUIT = 0,
    UPGRADE_COUNT = 1
};

/* The switch's word in the settings file, after "upgrade_". */
const char *Upgrades_Key(int upgrade);
/* The switch a word names (any case), or -1. */
int Upgrades_Index(const char *key);

/* 1 if the switch is on. */
int Upgrades_IsOn(int upgrade);
void Upgrades_Set(int upgrade, int on);

/* Test mode (the harness, or the kit's RE_KIT_TEST): the switches come from
 * the run, not the player's file, and the page saves nothing. */
void Upgrades_SetTestMode(int on);
int Upgrades_TestMode(void);

#endif
