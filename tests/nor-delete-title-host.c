#include <stdio.h>
#include <stdlib.h>

typedef unsigned int u32;

static u32 gl_color_text;

static u32 DrawText12VisibleLength(char *text)
{
	u32 length = 0;
	while(text && text[length])
		length++;
	return length;
}

static void Launcher_DrawPopupPanel(u32 x, u32 y)
{
	(void)x;
	(void)y;
}

static void DrawHZText12(char *text, u32 length, u32 x, u32 y, u32 color,
	u32 draw_direct)
{
	(void)text;
	(void)length;
	(void)x;
	(void)y;
	(void)color;
	(void)draw_direct;
}

static u32 erase_addresses[8];
static u32 erase_count;
static u32 fail_at;

static u32 Block_Erase(u32 address)
{
	if(erase_count < sizeof(erase_addresses) / sizeof(erase_addresses[0]))
		erase_addresses[erase_count] = address;
	erase_count++;
	return erase_count != fail_at;
}

#include "nor-delete-title.impl.h"

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
	check(Launcher_EraseNorTitle(0, 0x60000),
		"erase every allocated block of a title starting at NOR offset zero");
	check(erase_count == 3 && erase_addresses[0] == 0 &&
		erase_addresses[1] == 0x20000 && erase_addresses[2] == 0x40000,
		"erase title blocks in ascending address order");

	erase_count = 0;
	fail_at = 2;
	check(!Launcher_EraseNorTitle(0x20000, 0x60000),
		"report a failed block erase");
	check(erase_count == 2, "stop after the first failed erase");

	erase_count = 0;
	fail_at = 0;
	check(!Launcher_EraseNorTitle(1, 0x20000),
		"reject an unaligned title address");
	check(!Launcher_EraseNorTitle(0x03FE0000, 0x40000),
		"reject an erase range beyond NOR capacity");
	check(!Launcher_EraseNorTitle(0, 0),
		"reject an empty title range");
	check(erase_count == 0, "reject invalid ranges without erasing flash");

	puts("NOR title deletion scenarios passed.");
	return 0;
}
