#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned short u16;
typedef unsigned int u32;
typedef char TCHAR;
static u16 launcher_clean_list;
static int launcher_force_full_redraw;
static unsigned char launcher_list_row_cache_valid;
static unsigned char launcher_list_art_screen_has_art;

static void Launcher_GetDisplayTitleBounded(const TCHAR *src, char *dst, u32 dst_size)
{
	(void)src;
	strncpy(dst, "cleaned title", dst_size - 1);
	dst[dst_size - 1] = '\0';
}

#include CLEAN_LIST_FILE

static void expect(int condition, const char *message)
{
	if(!condition)
	{
		fprintf(stderr, "FAIL: %s\n", message);
		exit(1);
	}
}

int main(void)
{
	launcher_clean_list = 1;
	launcher_list_row_cache_valid = 1;
	launcher_list_art_screen_has_art = 1;
	launcher_force_full_redraw = 0;

	Launcher_SetCleanListEnabled(0);
	expect(!launcher_clean_list, "Clean list can be disabled");
	expect(!launcher_list_row_cache_valid, "changing Clean list invalidates cached rows");
	expect(!launcher_list_art_screen_has_art, "changing Clean list clears cached artwork state");
	expect(launcher_force_full_redraw, "changing Clean list requests a full redraw");

	launcher_list_row_cache_valid = 1;
	launcher_force_full_redraw = 0;
	Launcher_SetCleanListEnabled(0);
	expect(launcher_list_row_cache_valid, "unchanged Clean list setting preserves the cache");
	expect(!launcher_force_full_redraw, "unchanged Clean list setting avoids a redraw");

	{
		char title[64];
		launcher_clean_list = 0;
		Launcher_GetBrowserDisplayTitle("Example (USA) [Rev 1].gba", title, sizeof(title));
		expect(strcmp(title, "Example (USA) [Rev 1].gba") == 0,
			"Clean list Off preserves extensions and bracketed filename text in browser titles");

		launcher_clean_list = 1;
		Launcher_GetBrowserDisplayTitle("Example (USA) [Rev 1].gba", title, sizeof(title));
		expect(strcmp(title, "cleaned title") == 0,
			"Clean list On uses the cleaned browser title");
	}

	puts("Clean list cache and display scenarios passed.");
	return 0;
}
