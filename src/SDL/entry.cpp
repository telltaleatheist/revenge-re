#include "Campaign/Campaign.h"
#include "Data/StartupData.h"
#include "IO/IO.h"
#include "Runtime/Runtime.h"
#include "revenge/audio_log.h"
#include "revenge/platform_backend.h"
#include "revenge/debug.h"

#include "host_config.h"
#include "revenge/original_import.h"
#include "game_paths.h"
#include "harness.h"
#include "settings.h"
#include "setup_host.h"
#include "revenge/rules.h"
#include "revenge/story.h"
#include "revenge/upgrades.h"

#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

#ifndef CHR_VERSION
#define CHR_VERSION "development"
#endif

void InitializeGame(void);
void RunCampaignLoop(void);
void ShutdownGame(void);
void RunExitHandlersAndTerminate(uint16_t exitCode);

/* The audio log's times: the game's clock, in nanoseconds. */
static uint64_t AudioLogClock(void)
{
    return HostClock_NowMS() * UINT64_C(1000000);
}

/*
 * Host entry shim for the preserved 2C7F:000F flow.  The Original library is
 * compiled without a DOS main so tests may provide their own entry points;
 * these five statements are the complete original main, not new game flow,
 * and its end is the C library start-up's exit after main returns.
 */
int main(int argc, char **argv)
{
    int setupExit;

    if (argc == 0x02 && strcmp(argv[0x01], "--version") == 0x00)
    {
        printf("Revenge %s\n", CHR_VERSION);
        return 0x00;
    }
    /* The launcher's asks, its start options and the game folder this start
	 * plays from (setup_host.h). */
    setupExit = SetupHost_Start(argc, argv);
    if (setupExit >= 0x00)
        return setupExit;
    if (!SDLHost_ConfigureAssetDirectory())
        return 0x01;
    /* The game's own text and data: the story file in our write folder,
	 * taken from the player's copy the first time (or when it is outdated).
	 * Without a write folder, from the copy each time. */
    if (GamePaths_WriteDirectory() == NULL &&
        !OriginalImport_Load(GamePaths_Executable()))
        return 0x01;
    if (!StoryHost_Prepare(GamePaths_Executable(), GamePaths_WriteDirectory()))
        return 0x01;
    /* Nothing reads the copy from here on. */
    OriginalImport_Release();
    /* The rules, ours, from the data folder beside the program. */
    Rules_Load(GamePaths_DataDirectory());
    /* The UPGRADES page's words, ours (include/revenge/upgrades.h). */
    Upgrades_LoadText(GamePaths_DataDirectory());
    RevengeInstallCrashDiagnostics();
    /* What the game asks to be heard, for tools/trace/audio. */
    AudioLog_Open(SDL_getenv("REVENGE_AUDIO_LOG"), AudioLogClock);
    RevengeTrace("entry: initialize\n");
    /* The player's settings stand for MEDIA.CFG from here on. */
    if (!HostSettings_Prepare())
        return 0x01;
    ResourceFiles.preloadFiles = FALSE;
    StartupResourceState = 0x01;
    NumericKeypadNavigationEnabled = FALSE;
    InitializeGame();
    RevengeTrace("entry: initialized=%u\n", GameInitialized);
    if (GameInitialized != FALSE)
    {
        RunCampaignLoop();
    }
    RevengeTrace("entry: campaign returned; shutdown\n");
    ShutdownGame();
    RevengeTrace("entry: shutdown complete\n");
    /* main's 0, to the C library's exit as its start-up passes it
	 * (0800:0102-0108): the exit handlers and hooks, then the host's end. */
    RunExitHandlersAndTerminate(0x00);
    return 0x00;
}
