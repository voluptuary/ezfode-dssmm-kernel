/*
 * Differential test for the on/off settings readers. The reader functions are extracted VERBATIM from
 * source/ezkernelnew.c (see scripts/test-launcher-settings-readers-host.sh) from two versions of the
 * file and run over the same inputs; the printed results must be identical.
 */
#include <stdio.h>
#include <string.h>
#include <strings.h>
#if defined(__has_include)
#if __has_include("launcher_setting_parse.h")
#include "launcher_setting_parse.h"
#endif
#endif

typedef unsigned short u16;
typedef unsigned int u32;
#define LAUNCHER_WRAP_OFF 0
#define LAUNCHER_WRAP_ON 1
#define LAUNCHER_WRAP_EXCEPT_SYSTEM 2
static u16 launcher_sounds_enabled, launcher_hide_system_files, launcher_list_folders;
static u16 launcher_clean_list, launcher_wrap_around;
static unsigned launcher_settings_migration_pending;

static int g_present;
static const char *g_value;
static const char *g_on_text = "On", *g_off_text = "Off";
#define DSTEXT_ON g_on_text
#define DSTEXT_OFF g_off_text

static int Launcher_SettingsReadValue(const char *key, char *out, size_t out_size)
{
	(void)key;
	if(!g_present)
		return 0;
	strncpy(out, g_value, out_size - 1);
	out[out_size - 1] = '\0';
	return 1;
}

#include READERS_FILE

typedef struct { const char *name; void (*read)(void); u16 *target; } Reader;
static const Reader readers[] = {
	{ "Sounds", Launcher_ReadSoundsSetting, &launcher_sounds_enabled },
	{ "HideSystemFiles", Launcher_ReadHideSystemFilesSetting, &launcher_hide_system_files },
	{ "ListFolders", Launcher_ReadListFoldersSetting, &launcher_list_folders },
	{ "CleanList", Launcher_ReadCleanListSetting, &launcher_clean_list },
	{ "WrapAround", Launcher_ReadWrapAroundSetting, &launcher_wrap_around },
};

int main(void)
{
	static const char *values[] = { "", "On", "on", "ON", "oN", "Off", "off", "OFF", "oFf", "Yes", "yes", "YES",
		"No", "no", "NO", "1", "0", "10", "01", "1x", "0x", "00", "11", "2", "9", "x", "true", "false",
		"enabled", "disabled", "On ", " On", "Off ", "O", "Of", "Onn", "Offf", "\xE5\xBC\x80", "\xE5\x85\xB3",
		"ON_L", "OFF_L", "1on", "0off", "Y", "N", "Yes please", "No thanks", "-1", "+1", "On, Not SYSTEM" };
	static const char *on_set[]  = { "On", "\xE5\xBC\x80", "ON_L", "1on", "Yes" };
	static const char *off_set[] = { "Off", "\xE5\x85\xB3", "OFF_L", "0off", "No" };
	unsigned r, v, s, d;

	for(r = 0; r < sizeof(readers) / sizeof(readers[0]); r++)
		for(s = 0; s < sizeof(on_set) / sizeof(on_set[0]); s++)
		{
			g_on_text = on_set[s]; g_off_text = off_set[s];
			for(d = 0; d < 2; d++)           /* absent key, with both possible starting values */
			{
				*readers[r].target = (u16)(d ? 0xBEEF : 0x1234);
				launcher_settings_migration_pending = 0;
				g_present = 0; g_value = "";
				readers[r].read();
				printf("%s|set%u|absent|%u|%u\n", readers[r].name, s, *readers[r].target, launcher_settings_migration_pending);
			}
			for(v = 0; v < sizeof(values) / sizeof(values[0]); v++)
			{
				*readers[r].target = 0xBEEF;
				launcher_settings_migration_pending = 0;
				g_present = 1; g_value = values[v];
				readers[r].read();
				printf("%s|set%u|%s|%u|%u\n", readers[r].name, s, values[v], *readers[r].target, launcher_settings_migration_pending);
			}
		}
	return 0;
}
