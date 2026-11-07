#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

typedef unsigned short u16;
typedef unsigned int u32;
typedef enum { SD_list, NOR_list } PAGE_NUM;

#define LAUNCHER_WRAP_OFF 0
#define LAUNCHER_WRAP_ON 1
#define LAUNCHER_WRAP_EXCEPT_SYSTEM 2

static u16 launcher_wrap_around;
static PAGE_NUM launcher_active_page;
static int recents_view_active;
static char currentpath[256];

#include WRAP_HELPERS_FILE

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
	launcher_active_page = SD_list;
	launcher_wrap_around = LAUNCHER_WRAP_EXCEPT_SYSTEM;
	recents_view_active = 0;
	strcpy(currentpath, "/");
	expect(Launcher_BrowserWrapAroundEnabled(), "special mode wraps outside SYSTEM");

	strcpy(currentpath, "/SYSTEM");
	expect(!Launcher_BrowserWrapAroundEnabled(), "special mode disables wrap at SYSTEM root");
	strcpy(currentpath, "/system/KERNELS");
	expect(!Launcher_BrowserWrapAroundEnabled(), "special mode disables wrap in SYSTEM subdirectories");
	strcpy(currentpath, "/SYSTEMX");
	expect(Launcher_BrowserWrapAroundEnabled(), "SYSTEM-like sibling path is not excluded");

	launcher_wrap_around = LAUNCHER_WRAP_ON;
	strcpy(currentpath, "/SYSTEM/BACKUP");
	expect(Launcher_BrowserWrapAroundEnabled(), "On remains enabled inside SYSTEM");
	launcher_wrap_around = LAUNCHER_WRAP_OFF;
	strcpy(currentpath, "/");
	expect(!Launcher_BrowserWrapAroundEnabled(), "Off remains disabled outside SYSTEM");

	launcher_wrap_around = LAUNCHER_WRAP_EXCEPT_SYSTEM;
	strcpy(currentpath, "/SYSTEM");
	recents_view_active = 1;
	expect(Launcher_BrowserWrapAroundEnabled(), "virtual Recents view is not treated as SYSTEM browsing");
	recents_view_active = 0;
	launcher_active_page = NOR_list;
	expect(Launcher_BrowserWrapAroundEnabled(), "NOR browsing does not inherit the SYSTEM exception");

	puts("Wrap around SYSTEM scope scenarios passed.");
	return 0;
}
