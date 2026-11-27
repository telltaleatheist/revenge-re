#ifndef REVENGE_TESTS_ORIGINAL_IMPORT_FOR_TESTS_H
#define REVENGE_TESTS_ORIGINAL_IMPORT_FOR_TESTS_H

/* For tests that check the game's own text or data: they import it from the
 * copy REVENGE_GAME names, and skip (77) when none is given. */

/* A test including this defines _CRT_SECURE_NO_WARNINGS first, for MSVC,
 * which otherwise rejects fopen under /W4 /WX. */
#include "revenge/original_import.h"
#include "revenge/story.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void RequireOriginalImport(void)
{
	static const char *const names[] = { "REVENGE.EXE", "revenge.exe", "Revenge.exe" };
	const char *directory = getenv("REVENGE_GAME");
	char path[4096];
	size_t index;

	if (directory == NULL || directory[0] == '\0')
	{
		printf("skipped: set REVENGE_GAME to a copy of the game\n");
		exit(77);
	}
	for (index = 0; index < sizeof(names) / sizeof(names[0]); ++index)
	{
		FILE *file;

		snprintf(path, sizeof(path), "%s/%s", directory, names[index]);
		file = fopen(path, "rb");
		if (file != NULL)
		{
			fclose(file);
			if (!OriginalImport_Load(path))
				exit(1);
			/* Its texts too, in memory: no story file. */
			StoryHost_Prepare(path, NULL);
			return;
		}
	}
	printf("skipped: no REVENGE.EXE in REVENGE_GAME\n");
	exit(77);
}

#endif
