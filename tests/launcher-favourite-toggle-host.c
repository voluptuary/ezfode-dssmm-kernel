#include <stdio.h>
#include <string.h>

typedef unsigned int u32;
typedef int s32;

#define LAUNCHER_MAX_FAVOURITES 3
#define LAUNCHER_FAVOURITE_PATH_LEN 32

static u32 launcher_favourite_count;
static u32 launcher_favourite_index;
static u32 launcher_favourites_cache_blocked;
static char favourites[LAUNCHER_MAX_FAVOURITES][LAUNCHER_FAVOURITE_PATH_LEN];
static u32 loads;
static u32 saves;

static char (*Launcher_FavouritesBuffer(void))[LAUNCHER_FAVOURITE_PATH_LEN]
{
	return favourites;
}

static void Launcher_LoadFavourites(void)
{
	loads++;
}

static s32 Launcher_FindFavouriteFullPath(const char *fullpath)
{
	u32 i;
	for(i = 0; i < launcher_favourite_count; i++)
		if(strcmp(favourites[i], fullpath) == 0)
			return (s32)i;
	return -1;
}

static void Launcher_SaveFavourites(void)
{
	saves++;
}

static u32 Launcher_AppendFavouriteFullPath(const char *fullpath)
{
	if(launcher_favourite_count >= LAUNCHER_MAX_FAVOURITES ||
		strlen(fullpath) >= LAUNCHER_FAVOURITE_PATH_LEN)
		return 0;
	strcpy(favourites[launcher_favourite_count++], fullpath);
	saves++;
	return 1;
}

static void dmaCopy(const void *src, void *dst, u32 size)
{
	memcpy(dst, src, size);
}

#include FUNCTION_FILE

static int check(int condition, const char *message)
{
	if(condition)
		return 1;
	fprintf(stderr, "FAIL: %s\n", message);
	return 0;
}

int main(void)
{
	int ok = 1;
	strcpy(favourites[0], "/games/a.gba");
	strcpy(favourites[1], "/games/b.gba");
	strcpy(favourites[2], "/games/c.gba");
	launcher_favourite_count = 3;
	launcher_favourite_index = 2;

	ok &= check(Launcher_ToggleFavouriteFullPath("/games/new.gba") == 0,
		"full favourites list rejects an addition");
	ok &= check(launcher_favourite_count == 3 && saves == 0,
		"rejected addition does not mutate or save");
	ok &= check(Launcher_ToggleFavouriteFullPath("/games/b.gba") == 1,
		"existing favourite toggles off");
	ok &= check(launcher_favourite_count == 2 &&
		strcmp(favourites[0], "/games/a.gba") == 0 &&
		strcmp(favourites[1], "/games/c.gba") == 0 &&
		favourites[2][0] == '\0',
		"removal compacts and clears the final slot");
	ok &= check(launcher_favourite_index == 0 && saves == 1,
		"removal clamps the selected favourite and persists");
	ok &= check(Launcher_ToggleFavouriteFullPath("/games/new.gba") == 1,
		"missing favourite toggles on");
	ok &= check(launcher_favourite_count == 3 &&
		strcmp(favourites[2], "/games/new.gba") == 0 && saves == 2,
		"addition persists the new path");

	launcher_favourites_cache_blocked = 1;
	ok &= check(Launcher_ToggleFavouriteFullPath("/games/a.gba") == 0,
		"blocked favourite cache rejects a toggle");
	ok &= check(launcher_favourite_count == 3 && saves == 2,
		"blocked toggle preserves existing favourites");
	ok &= check(Launcher_ToggleFavouriteFullPath("") == 0 &&
		Launcher_ToggleFavouriteFullPath(0) == 0,
		"empty paths are rejected");
	ok &= check(loads == 4, "each valid toggle loads favourites before changing them");
	return ok ? 0 : 1;
}
