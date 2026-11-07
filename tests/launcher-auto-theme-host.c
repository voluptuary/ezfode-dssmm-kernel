#include <stdio.h>

typedef unsigned int u32;
typedef unsigned short u16;

static u16 launcher_auto_theme;
static u16 launcher_auto_theme_pending;
static u16 launcher_dark_mode;
static u32 launcher_theme_apply_count;

static void Launcher_ApplyThemeColours(void)
{
	launcher_theme_apply_count++;
}

#include "settings_auto_theme.h"

static int expect(u32 hour, u16 current, u16 expected, int enabled)
{
	launcher_auto_theme = (u16)enabled;
	launcher_dark_mode = current;
	launcher_auto_theme_pending = 0;
	Launcher_QueueAutoTheme(hour);
	if(launcher_auto_theme_pending != expected)
	{
		fprintf(stderr, "FAIL: hour %u, mode %u produced pending %u, expected %u\n",
			hour, current, launcher_auto_theme_pending, expected);
		return 0;
	}
	if(expected)
	{
		u16 old = launcher_dark_mode;
		if(Launcher_ApplyPendingAutoTheme() != (old != expected - 1) ||
			launcher_dark_mode != expected - 1 || launcher_auto_theme_pending != 0)
		{
			fprintf(stderr, "FAIL: hour %u did not apply pending mode\n", hour);
			return 0;
		}
	}
	else if(Launcher_ApplyPendingAutoTheme() != 0)
	{
		fprintf(stderr, "FAIL: hour %u applied a redundant theme\n", hour);
		return 0;
	}
	return 1;
}

int main(void)
{
	int ok = 1;
	ok &= expect(7, 0, 2, 1);
	ok &= expect(8, 1, 1, 1);
	ok &= expect(19, 0, 0, 1);
	ok &= expect(20, 0, 2, 1);
	ok &= expect(23, 1, 0, 1);
	ok &= expect(24, 0, 0, 1);
	ok &= expect(20, 0, 0, 0);
	ok &= expect(20, 1, 0, 1);
	if(launcher_theme_apply_count != 3)
	{
		fprintf(stderr, "FAIL: expected 3 actual theme changes, got %u\n", launcher_theme_apply_count);
		ok = 0;
	}
	return ok ? 0 : 1;
}
