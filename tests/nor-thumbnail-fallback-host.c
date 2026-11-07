#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char u8;
typedef unsigned int u32;

#define MAX_path_len 256

static const char *thumbnail_folder = "/SYSTEM/IMGS";
static char loaded_path[160];
static u32 load_result = 1;
static u32 custom_result;
static u32 gamecode_result;
static u32 custom_calls;
static u32 gamecode_calls;
static u32 custom_name_seen;
static u32 mirrored_result;
static u32 mirrored_calls;
static char mirrored_dir_seen[160];
static char mirrored_name_seen[104];

static const char *Launcher_ThumbnailFolder(void)
{
	return thumbnail_folder;
}

static u32 Launcher_LoadThumbnailFile(const char *path, u8 *dst)
{
	(void)dst;
	snprintf(loaded_path, sizeof(loaded_path), "%s", path);
	return load_result;
}

static u32 Launcher_LoadCustomThumbnailByName(const char *name, u8 *dst)
{
	(void)dst;
	custom_calls++;
	custom_name_seen = strcmp(name, "Long game") == 0;
	return custom_result;
}

static u32 Launcher_LoadThumbnailByGameCode(const u8 gamecode[4], u8 *dst)
{
	(void)gamecode;
	(void)dst;
	gamecode_calls++;
	return gamecode_result;
}

static u32 Launcher_LoadMirroredThumbnail(const char *rom_dir,
	const char *filename, u8 *dst)
{
	(void)dst;
	mirrored_calls++;
	snprintf(mirrored_dir_seen, sizeof(mirrored_dir_seen), "%s", rom_dir);
	snprintf(mirrored_name_seen, sizeof(mirrored_name_seen), "%s", filename);
	return mirrored_result;
}

#include "nor-thumbnail-fallback.impl.h"

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
	u8 thumbnail[1];
	const u8 gamecode[4] = { 'A', 'B', 'C', 'D' };
	u32 custom_thumbnail = 1;

	check(Launcher_LoadNorThumbnail("Long game.gba", "Folder", gamecode, thumbnail,
		&custom_thumbnail), "load root artwork when custom and header art are absent");
	check(strcmp(loaded_path, "/SYSTEM/IMGS/Long game.bmp") == 0,
		"use the filename stem in the selected title-art root");
	check(custom_calls == 1 && custom_name_seen, "check custom artwork first");
	check(gamecode_calls == 1, "check header-code artwork before filename artwork");
	check(custom_thumbnail == 0, "do not mark root-named artwork as custom");
	check(mirrored_calls == 1 && strcmp(mirrored_dir_seen, "Folder") == 0 &&
		strcmp(mirrored_name_seen, "Long game.gba") == 0,
		"preserve a NOR title's source directory for mirrored artwork lookup");

	thumbnail_folder = "/SYSTEM/IMGS2";
	custom_calls = gamecode_calls = 0;
	loaded_path[0] = '\0';
	check(Launcher_LoadNorThumbnail("Long game.gba", "", gamecode, thumbnail,
		&custom_thumbnail), "load filename artwork in box-art mode");
	check(strcmp(loaded_path, "/SYSTEM/IMGS2/Long game.bmp") == 0,
		"follow the selected box-art root");

	custom_result = 1;
	custom_calls = gamecode_calls = 0;
	loaded_path[0] = '\0';
	custom_thumbnail = 0;
	check(Launcher_LoadNorThumbnail("Long game.gba", "", gamecode, thumbnail,
		&custom_thumbnail), "prefer custom artwork when available");
	check(custom_thumbnail == 1 && custom_calls == 1 && gamecode_calls == 0 &&
		loaded_path[0] == '\0', "stop fallback after custom artwork succeeds");

	custom_result = 0;
	gamecode_result = 1;
	custom_calls = gamecode_calls = 0;
	loaded_path[0] = '\0';
	check(Launcher_LoadNorThumbnail("Long game.gba", "", gamecode, thumbnail,
		&custom_thumbnail), "prefer header-code art over filename-matched art");
	check(gamecode_calls == 1 && loaded_path[0] == '\0',
		"stop fallback after header-code artwork succeeds");

	gamecode_result = 0;
	load_result = 0;
	custom_calls = gamecode_calls = 0;
	loaded_path[0] = '\0';
	check(!Launcher_LoadNorThumbnail("Missing.gba", "", gamecode, thumbnail,
		&custom_thumbnail), "report a missing filename-matched artwork file");
	check(strcmp(loaded_path, "/SYSTEM/IMGS2/Missing.bmp") == 0,
		"probe the expected filename-matched artwork path");
	check(!Launcher_LoadNorThumbnail(NULL, "", gamecode, thumbnail,
		&custom_thumbnail),
		"reject a missing NOR filename");

	load_result = 1;
	mirrored_result = 0;
	mirrored_calls = 0;
	check(Launcher_LoadNorThumbnail("First.gba", "", gamecode, thumbnail,
		&custom_thumbnail), "load artwork for the first NOR catalog entry");
	check(Launcher_LoadNorThumbnail("Second.gba", "", gamecode, thumbnail,
		&custom_thumbnail), "load artwork for a later NOR catalog entry");
	check(Launcher_LoadNorThumbnail("Third.gba", "", gamecode, thumbnail,
		&custom_thumbnail), "load artwork for another later NOR catalog entry");
	check(strcmp(loaded_path, "/SYSTEM/IMGS2/Third.bmp") == 0,
		"resolve each NOR entry independently by its filename");

	mirrored_result = 1;
	loaded_path[0] = '\0';
	check(Launcher_LoadNorThumbnail("Second.gba", "/ROMs", gamecode, thumbnail,
		&custom_thumbnail), "load artwork from a NOR title's source subdirectory");
	check(strcmp(mirrored_dir_seen, "/ROMs") == 0 &&
		strcmp(mirrored_name_seen, "Second.gba") == 0 && loaded_path[0] == '\0',
		"prefer the matching mirrored artwork before the root-level fallback");

	puts("NOR filename-art fallback scenarios passed.");
	return 0;
}
