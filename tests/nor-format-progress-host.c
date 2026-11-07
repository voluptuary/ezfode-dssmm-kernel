#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned int u32;
typedef char TCHAR;

#define MAX_path_len 256

#include "nor_art_path.h"
#include "nor-format-progress.impl.h"

static void check(int condition, const char *message)
{
	if(!condition)
	{
		fprintf(stderr, "FAIL: %s\n", message);
		exit(1);
	}
}

int main(void)
{
	char art_path[MAX_path_len];
	char deep_path[MAX_path_len];
	char legacy_path[MAX_path_len];
	u32 i;

	check(NOR_ProgressStepCount(0) == 0,
		"format progress bar starts empty without leading zero digits");
	check(NOR_ProgressStepCount(32) == 8,
		"format progress bar fills proportionally");
	check(NOR_ProgressStepCount(64) == 16,
		"format progress bar reaches its full width");
	check(NOR_ProgressStepCount(65) == 16,
		"format progress bar remains capped at full width");

	check(NOR_ShouldReportEraseProgress(0x000FFFFE),
		"report verification progress at the first one-megabyte boundary");
	check(NOR_ShouldReportEraseProgress(0x001FFFFE),
		"continue reporting verification progress at later boundaries");
	check(!NOR_ShouldReportEraseProgress(0x000FFFFC),
		"avoid redraws for every verified word");

	for(i = 0; i < sizeof(deep_path) - 2; i++)
		deep_path[i] = (i % 8 == 0) ? '/' : 'D';
	deep_path[sizeof(deep_path) - 2] = '\0';
	NOR_StoreArtPathRecord(art_path, sizeof(art_path), deep_path);
	check(strcmp(NOR_ValidArtPathRecord(art_path, sizeof(art_path)),
		deep_path) == 0, "retain a deeply nested source path");

	memset(legacy_path, 0xFF, sizeof(legacy_path));
	check(strcmp(NOR_ValidArtPathRecord(legacy_path, sizeof(legacy_path)), "") == 0,
		"treat old erased metadata as no saved artwork path");
	memset(legacy_path, 'X', sizeof(legacy_path));
	check(strcmp(NOR_ValidArtPathRecord(legacy_path, sizeof(legacy_path)), "") == 0,
		"reject a corrupt path record without a terminator");

	puts("NOR format progress and art-path metadata scenarios passed.");
	return 0;
}
