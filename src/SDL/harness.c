/* The port's test harness: a test clock, poll-fed key scripts and snaps
 * (harness.h, tools/trace/README.md). */
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "harness.h"
#include "map_dump.h"

#include "revenge/audio_log.h"
#include "revenge/original_data.h"
#include "revenge/platform_backend.h"
#include "revenge/original/campaign_data.h"
#include "revenge/original/lance_editor_data.h"
#include "revenge/random.h"
#include "revenge/save_data.h"
#include "revenge/video.h"

#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum
{
	ITEM_KEY,
	ITEM_SNAP,
	ITEM_MOUSE,
	ITEM_QUIT,
	ITEM_FAIL,
	POLLS_PER_WAIT = 4,
	NAME_CAPACITY = 48,
	PATH_CAPACITY = 1024,
	/* A run whose script stops advancing for this many polls is stuck. */
	STALL_POLLS = 50000000
};

typedef struct HarnessItem
{
	int kind;
	unsigned idle;              /* extra polls before it */
	unsigned steps;             /* battle steps before it, since the last key or snap */
	unsigned still;             /* polls without a step before it (a message or menu up) */
	int scancode;               /* ITEM_KEY: SDL's scancode */
	byte modifiers;             /* ITEM_KEY: 1 Shift, 2 Ctrl, 4 Alt */
	uint16 x, y;                /* ITEM_MOUSE */
	byte buttons;
	char name[NAME_CAPACITY];   /* ITEM_SNAP */
	int failure;                /* ITEM_FAIL: HARNESS_FAIL_* */
} HarnessItem;

static int state;               /* 0 not read, 1 on, -1 off */
static char outDirectory[PATH_CAPACITY];
static HarnessItem *items;
static size_t itemCount;
static size_t itemIndex;
static uint64_t clockMS;
static uint64_t polls;
static uint64_t reads;
static uint64_t pollsSinceItem;
static unsigned stepsSinceItem; /* battle steps seen since the last key or snap */
static byte lastSubtick;        /* TacticalClockSubtick at the previous poll */
static uint64_t pollsStill;     /* polls since the last step, key or snap */

static void Stop(const char *what, const char *detail)
{
	fprintf(stderr, "harness: %s%s%s\n", what, detail != NULL ? ": " : "", detail != NULL ? detail : "");
	exit(2);
}

static void OutPath(char *out, const char *name, const char *suffix)
{
	if ((size_t) SDL_snprintf(out, PATH_CAPACITY, "%s/%s%s", outDirectory, name, suffix) >= PATH_CAPACITY)
		Stop("an output path is too long", name);
}

static void WriteFile(const char *name, const char *suffix, const void *bytes, size_t length)
{
	char path[PATH_CAPACITY];
	FILE *file;

	OutPath(path, name, suffix);
	file = fopen(path, "wb");
	if (file == NULL || fwrite(bytes, 1, length, file) != length || fclose(file) != 0)
		Stop("can't write", path);
}

int Harness_ParseKey(const char *token, int *scancode, byte *modifiers)
{
	SDL_Scancode key;

	*modifiers = 0x00;
	for (;;)
	{
		if (SDL_strncasecmp(token, "Shift+", 6) == 0)
		{
			*modifiers |= 0x01;
			token += 6;
		}
		else if (SDL_strncasecmp(token, "Ctrl+", 5) == 0)
		{
			*modifiers |= 0x02;
			token += 5;
		}
		else if (SDL_strncasecmp(token, "Alt+", 4) == 0)
		{
			*modifiers |= 0x04;
			token += 4;
		}
		else
			break;
	}
	key = SDL_GetScancodeFromName(token);
	if (key == SDL_SCANCODE_UNKNOWN)
		return 0;
	*scancode = (int) key;
	return Platform_ScanCodeForKey((int) key) != 0x0000;
}

static int ValidName(const char *name)
{
	size_t length = strlen(name);
	size_t index;

	if (length == 0 || length >= NAME_CAPACITY)
		return 0;
	for (index = 0; index < length; ++index)
	{
		char c = name[index];

		if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-'))
			return 0;
	}
	return 1;
}

/* Tokens are separated by spaces, commas or new lines; '#' starts a comment. */
/* A wait, NAME:N, adds N to *total; returns whether the token was one. */
static int ReadWait(const char *token, const char *name, unsigned *total)
{
	size_t length = strlen(name);
	char *end;
	unsigned long count;

	if (strncmp(token, name, length) != 0 || token[length] != ':')
		return 0;
	count = strtoul(token + length + 1, &end, 10);
	if (end == token + length + 1 || *end != '\0' || count > 1000000UL)
		Stop("a bad wait count (idle:N, steps:N or still:N, N up to 1000000)", token);
	*total += (unsigned) count;
	return 1;
}

static void ReadScript(const char *path)
{
	size_t length = 0;
	char *text = (char *) SDL_LoadFile(path, &length);
	char *cursor;
	size_t capacity = 0;
	unsigned idle = 0;
	unsigned steps = 0;
	unsigned still = 0;

	if (text == NULL)
		Stop("can't read the script", path);
	cursor = text;
	while (*cursor != '\0')
	{
		char token[128];
		size_t tokenLength = 0;
		HarnessItem item;

		if (*cursor == '#')
		{
			while (*cursor != '\0' && *cursor != '\n')
				++cursor;
			continue;
		}
		if (*cursor == ' ' || *cursor == '\t' || *cursor == '\r' || *cursor == '\n' || *cursor == ',')
		{
			++cursor;
			continue;
		}
		while (cursor[tokenLength] != '\0' && cursor[tokenLength] != ' ' && cursor[tokenLength] != '\t' &&
			cursor[tokenLength] != '\r' && cursor[tokenLength] != '\n' && cursor[tokenLength] != ',' &&
			cursor[tokenLength] != '#')
			++tokenLength;
		if (tokenLength >= sizeof(token))
			Stop("a script token is too long", NULL);
		memcpy(token, cursor, tokenLength);
		token[tokenLength] = '\0';
		cursor += tokenLength;
		memset(&item, 0, sizeof(item));
		if (ReadWait(token, "idle", &idle) || ReadWait(token, "steps", &steps) || ReadWait(token, "still", &still))
			continue;
		if (strncmp(token, "@snap:", 6) == 0)
		{
			if (!ValidName(token + 6))
				Stop("a bad snap name", token);
			item.kind = ITEM_SNAP;
			memcpy(item.name, token + 6, strlen(token + 6) + 1);
		}
		else if (strcmp(token, "quit") == 0)
			item.kind = ITEM_QUIT;
		else if (strcmp(token, "fail:allocation") == 0 || strcmp(token, "fail:abort") == 0)
		{
			item.kind = ITEM_FAIL;
			item.failure = strcmp(token, "fail:allocation") == 0 ? HARNESS_FAIL_ALLOCATION : HARNESS_FAIL_ABORT;
		}
		else if (strncmp(token, "mouse:", 6) == 0)
		{
			unsigned x, y, buttons = 0;
			int fields = sscanf(token + 6, "%u/%u/%u", &x, &y, &buttons);

			if (fields < 2 || x >= VIDEO_SCREEN_WIDTH || y >= VIDEO_SCREEN_HEIGHT || buttons > 3)
				Stop("a bad mouse position (mouse:X/Y or mouse:X/Y/BUTTONS)", token);
			item.kind = ITEM_MOUSE;
			item.x = (uint16) x;
			item.y = (uint16) y;
			item.buttons = (byte) buttons;
		}
		else
		{
			item.kind = ITEM_KEY;
			if (!Harness_ParseKey(token, &item.scancode, &item.modifiers))
				Stop("not a key the game reads", token);
		}
		item.idle = idle;
		item.steps = steps;
		item.still = still;
		idle = 0;
		steps = 0;
		still = 0;
		if (itemCount == capacity)
		{
			HarnessItem *grown;

			capacity = capacity == 0 ? 64 : capacity * 2;
			grown = (HarnessItem *) realloc(items, capacity * sizeof(HarnessItem));
			if (grown == NULL)
				Stop("no memory for the script", NULL);
			items = grown;
		}
		items[itemCount++] = item;
	}
	SDL_free(text);
}

int Harness_Active(void)
{
	if (state == 0)
	{
		const char *script = SDL_getenv("REVENGE_HARNESS_SCRIPT");
		const char *out = SDL_getenv("REVENGE_HARNESS_OUT");

		state = -1;
		if (script == NULL || script[0] == '\0')
			return 0;
		if (out == NULL || out[0] == '\0' || strlen(out) >= sizeof(outDirectory) || !SDL_GetPathInfo(out, NULL))
			Stop("REVENGE_HARNESS_OUT must name an existing folder", out);
		memcpy(outDirectory, out, strlen(out) + 1);
		state = 1;
		ReadScript(script);
	}
	return state == 1;
}

uint64_t HostClock_NowMS(void)
{
	return Harness_Active() ? clockMS : SDL_GetTicks();
}

void HostClock_DelayMS(uint64_t milliseconds)
{
	if (Harness_Active())
		clockMS += milliseconds;
	else
		SDL_Delay((Uint32) milliseconds);
}

void HostClock_Step(void)
{
	if (Harness_Active())
		++clockMS;
}

int HostClock_CalendarTime(struct tm *out, uint32_t *hundredths)
{
	if (Harness_Active())
	{
		/* 1991-01-01 12:00:00, plus the clock. */
		uint64_t seconds = clockMS / 1000;
		uint64_t days = (seconds + 12 * 3600) / 86400;
		uint64_t inDay = (seconds + 12 * 3600) % 86400;
		static const int monthDays[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
		int year = 1991;
		int month = 0;

		for (;;)
		{
			int leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
			uint64_t yearDays = leap ? 366 : 365;

			if (days < yearDays)
				break;
			days -= yearDays;
			++year;
		}
		for (;;)
		{
			int leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
			uint64_t length = (uint64_t) monthDays[month] + (month == 1 && leap ? 1 : 0);

			if (days < length)
				break;
			days -= length;
			++month;
		}
		memset(out, 0, sizeof(*out));
		out->tm_year = year - 1900;
		out->tm_mon = month;
		out->tm_mday = (int) days + 1;
		out->tm_hour = (int) (inDay / 3600);
		out->tm_min = (int) (inDay / 60 % 60);
		out->tm_sec = (int) (inDay % 60);
		if (hundredths != NULL)
			*hundredths = (uint32_t) (clockMS / 10 % 100);
		return 1;
	}
	else
	{
		struct timespec now;
#if !defined(_WIN32)
		struct tm *local;
#endif

		if (timespec_get(&now, TIME_UTC) != TIME_UTC)
			return 0;
#if defined(_WIN32)
		if (localtime_s(out, &now.tv_sec) != 0)
			return 0;
#else
		local = localtime(&now.tv_sec);
		if (local == NULL)
			return 0;
		*out = *local;
#endif
		if (hundredths != NULL)
			*hundredths = (uint32_t) (now.tv_nsec / 10000000L);
		return 1;
	}
}

/* The block SaveGame (0D73:4866) writes for a slot, as it would write it now:
 * the campaign snapshot filled from the live state, then the live units. The
 * roster preparation SaveGame first runs when it hasn't run is left out, as
 * it changes the game. */
static void SaveBlock(byte *out)
{
	RevengeCampaignSnapshot snapshot;
	word16 index;

	memset(&snapshot, 0, sizeof(snapshot));
	snapshot.campaignStageIndex = CampaignStageIndex;
	snapshot.scenarioVariantIndex = (word16) ScenarioVariantIndex;
	snapshot.campaignPhase = CampaignPhase;
	snapshot.trainingSequenceFlag = TrainingSequenceFlag;
	snapshot.campaignFlags = CampaignFlags;
	snapshot.state24E1 = SavedCampaignFlags;
	snapshot.stateA967 = ScenarioUnitZeroAvailable;
	snapshot.scene3RosterSelector9991 = EjectedPilotRosterState;
	snapshot.state88F1 = ScenarioUnitFifteenState;
	snapshot.state8925 = ScenarioUnitFourteenEnabled;
	snapshot.state99C5 = ScenarioUnitThirteenEnabled;
	snapshot.stateA965 = LastAttackingUnitIndex;
	snapshot.state8594 = CampaignScenarioState;
	snapshot.lrmShipmentLoaded = LrmShipmentLoaded;
	snapshot.srmShipmentLoaded = SrmShipmentLoaded;
	memcpy(snapshot.scenarioClock, SavedCampaignClockState, sizeof(snapshot.scenarioClock));
	memcpy(snapshot.trainingScenarioCompletionFlags, TrainingScenarioCompletionFlags,
		sizeof(snapshot.trainingScenarioCompletionFlags));
	for (index = 0; index < REVENGE_SAVE_UNIT_COUNT; ++index)
	{
		snapshot.scenarioUnitMapX[index] = ScenarioUnitMapX[index];
		snapshot.scenarioUnitMapY[index] = ScenarioUnitMapY[index];
	}
	memcpy(snapshot.state1B85, ReplayCampaignState, sizeof(snapshot.state1B85));
	snapshot.state1B8E = RandomPilotPoolCounts.replay;
	memcpy(snapshot.state1BAD, ReplayExtendedCampaignState, sizeof(snapshot.state1BAD));
	for (index = 0; index < REVENGE_SAVE_SCENARIO_RECORD_COUNT; ++index)
		snapshot.scenarioRecords[index] = UnitSelectionCards[index];
	for (index = 0; index < REVENGE_SAVE_LANCE_ASSIGNMENT_COUNT; ++index)
		snapshot.lanceAssignments[index] = LanceAssignments[index];
	for (index = 0; index < REVENGE_SAVE_PILOT_COUNT; ++index)
		snapshot.pilotAvailability[index] = PilotRoster[index].availability;
	memcpy(out, &snapshot, sizeof(snapshot));
	memcpy(out + sizeof(snapshot), LiveScenarioUnits, sizeof(LiveScenarioUnits));
}

/* The named state: numbers only, never the game's text. */
static void WriteState(const char *name)
{
	char path[PATH_CAPACITY];
	FILE *file;
	word16 index;

	OutPath(path, name, ".state");
	file = fopen(path, "wb");
	if (file == NULL)
		Stop("can't write", path);
	fprintf(file, "state random %02x %02x %02x\n", PseudoRandomState[0], PseudoRandomState[1],
		PseudoRandomState[2]);
	fprintf(file, "state input polls %llu reads %llu clock %llu\n", (unsigned long long) polls,
		(unsigned long long) reads, (unsigned long long) clockMS);
	fprintf(file, "state campaign stage %u phase %u variant %d flags %04x saved %04x training %u\n",
		(unsigned) CampaignStageIndex, (unsigned) CampaignPhase, (int) ScenarioVariantIndex,
		(unsigned) CampaignFlags, (unsigned) SavedCampaignFlags, (unsigned) TrainingSequenceFlag);
	fprintf(file, "state timer game %lu animation %lu\n", (unsigned long) GameTimerTicks,
		(unsigned long) GameAnimationTicks);
	fprintf(file, "state clock %u %u %u subtick %u\n", (unsigned) CampaignClockState[0],
		(unsigned) CampaignClockState[1], (unsigned) CampaignClockState[2], (unsigned) TacticalClockSubtick);
	fprintf(file, "state battle result %u selected %04x overview %u delay %u\n", (unsigned) TacticalBattleResult,
		(unsigned) TacticalSelectedUnitIndex, (unsigned) TacticalOverviewActive, (unsigned) TacticalUpdateDelayTicks);
	fprintf(file, "state cursor map %u %u viewport %u %u\n", (unsigned) TacticalCursorMapX,
		(unsigned) TacticalCursorMapY, (unsigned) TacticalCursorViewportX, (unsigned) TacticalCursorViewportY);
	for (index = 0; index < REVENGE_SAVE_UNIT_COUNT; ++index)
	{
		const RevengeUnitRecord *unit = &PersistentUnitRoster[index];   /* the battle's units */
		unsigned durability = 0;
		word16 part;

		if (unit->deploymentState == 0x00 && ScenarioUnitMapX[index] == 0x00 && ScenarioUnitMapY[index] == 0x00)
			continue;
		for (part = 0; part < sizeof(unit->currentDurability); ++part)
			durability += unit->currentDurability[part];
		fprintf(file, "state unit %u deploy %u at %u %u durability %u heat %u engine %u pilot %u\n",
			(unsigned) index, (unsigned) unit->deploymentState, (unsigned) ScenarioUnitMapX[index],
			(unsigned) ScenarioUnitMapY[index], durability, (unsigned) unit->currentHeat,
			(unsigned) unit->engineHits, (unsigned) unit->pilotId);
	}
	if (fclose(file) != 0)
		Stop("can't write", path);
}

static void Snap(const char *name)
{
	byte palette[0x300];
	byte save[REVENGE_CAMPAIGN_SNAPSHOT_SIZE + sizeof(LiveScenarioUnits)];
	char path[PATH_CAPACITY];
	FILE *list;

	WriteFile(name, ".indices", SDLBackend_GetIndexedFramebuffer(), VIDEO_SCREEN_WIDTH * VIDEO_SCREEN_HEIGHT);
	WriteFile(name, ".palette", palette, (size_t) Platform_PaletteRgb(palette) * 0x03);
	SaveBlock(save);
	WriteFile(name, ".save", save, sizeof(save));
	WriteState(name);
	MapDump_Write(name);
	AudioLog_Snap(name);
	OutPath(path, "snaps", ".txt");
	list = fopen(path, "ab");
	if (list == NULL || fprintf(list, "%s\n", name) < 0 || fclose(list) != 0)
		Stop("can't write", path);
}

static void Finish(void)
{
	char text[128];

	SDL_snprintf(text, sizeof(text), "done items %u polls %llu reads %llu\n", (unsigned) itemCount,
		(unsigned long long) polls, (unsigned long long) reads);
	WriteFile("done", ".txt", text, strlen(text));
	exit(0);
}

/* The 4th poll since the last key or snap, plus the item's idle:N; its
 * steps:N battle steps since then; its still:N polls without a step. */
static int ItemDue(const HarnessItem *item)
{
	return pollsSinceItem >= POLLS_PER_WAIT + (uint64_t) item->idle && stepsSinceItem >= item->steps
		&& pollsStill >= item->still;
}

static void ItemTaken(void)
{
	pollsSinceItem = 0;
	stepsSinceItem = 0;
	pollsStill = 0;
}

int Harness_KeyPending(void)
{
	++polls;
	++pollsSinceItem;
	++pollsStill;
	/* A battle step moves the subtick, and the battle loop polls at least
	 * once a pass, so a poll that sees it changed counts one step. */
	if (TacticalClockSubtick != lastSubtick)
	{
		lastSubtick = TacticalClockSubtick;
		++stepsSinceItem;
		pollsStill = 0;
	}
	while (itemIndex < itemCount)
	{
		const HarnessItem *item = &items[itemIndex];

		if (!ItemDue(item))
		{
			if (pollsSinceItem > STALL_POLLS)
				Stop("the script stopped advancing", item->kind == ITEM_SNAP ? item->name : NULL);
			return 0;
		}
		if (item->kind == ITEM_KEY)
			return 1;
		if (item->kind == ITEM_SNAP)
		{
			Snap(item->name);
			++itemIndex;
			ItemTaken();
			return 0;
		}
		++itemIndex;
		if (item->kind == ITEM_QUIT)
		{
			ItemTaken();
			Platform_RequestHarnessQuit();
			return 0;
		}
		if (item->kind == ITEM_FAIL)
		{
			ItemTaken();
			Platform_RequestHarnessFailure(item->failure);
			return 0;
		}
		Platform_SendHarnessMouse(item->x, item->y, item->buttons);
	}
	if (pollsSinceItem >= POLLS_PER_WAIT)
		Finish();
	return 0;
}

void Harness_ReadKey(int *scancode, byte *modifiers)
{
	while (itemIndex >= itemCount || items[itemIndex].kind != ITEM_KEY || !ItemDue(&items[itemIndex]))
	{
		HostClock_Step();
		(void) Harness_KeyPending();
	}
	*scancode = items[itemIndex].scancode;
	*modifiers = items[itemIndex].modifiers;
	++itemIndex;
	++reads;
	ItemTaken();
}

