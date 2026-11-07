#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned short u16;
typedef unsigned int u32;

#define LAUNCHER_SELECTED_TEXT 0x7FFF

static const u16 gImage_MENU[1] = {0};
static const u16 gImage_MENU_BLANK[1] = {1};
static const u16 gImage_MENU_DARK[1] = {2};
static char *gl_menu_btn;
static u16 gl_color_selectBG_sd = 0x1234;

struct clear_call
{
	u32 x;
	u32 y;
	u32 w;
	u32 h;
	u16 color;
	u32 transparent;
};

static struct clear_call clear_calls[2];
static u32 clear_count;
static u32 draw_pic_count;
static char drawn_text[64];
static u32 drawn_x;
static u32 drawn_y;
static u16 drawn_color;
static u32 drawn_count;

static void Clear(u32 x, u32 y, u32 w, u32 h, u16 color, u32 transparent)
{
	if(clear_count < 2)
	{
		clear_calls[clear_count].x = x;
		clear_calls[clear_count].y = y;
		clear_calls[clear_count].w = w;
		clear_calls[clear_count].h = h;
		clear_calls[clear_count].color = color;
		clear_calls[clear_count].transparent = transparent;
	}
	clear_count++;
}

static void DrawPic(u16 *image, u32 x, u32 y, u32 w, u32 h,
	u32 custom, u32 mode, u32 copy)
{
	(void)image;
	(void)custom;
	(void)mode;
	(void)copy;
	if(x == 36 && y == 25 && w == 168 && h == 110)
		draw_pic_count++;
}

static void DrawHZText12(char *text, u32 max_chars, u32 x, u32 y,
	u16 color, u32 opaque)
{
	(void)max_chars;
	(void)opaque;
	snprintf(drawn_text, sizeof(drawn_text), "%s", text);
	drawn_x = x;
	drawn_y = y;
	drawn_color = color;
	drawn_count++;
}

#include "../source/popup_chrome.h"

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
	char hints[] = " (B) No     (A) OK";
	gl_menu_btn = hints;
	Launcher_DrawPopupPanel(36, 25);
	Launcher_DrawPopupButtonHints();

	check(draw_pic_count == 1, "draw the popup panel before its controls");
	check(clear_count == 2, "restore both button-hint backgrounds");
	check(clear_calls[0].x == 60 && clear_calls[0].y == 117 &&
		clear_calls[0].w == 55 && clear_calls[0].h == 14,
		"restore the first button background at the popup footer");
	check(clear_calls[1].x == 125 && clear_calls[1].y == 117 &&
		clear_calls[1].w == 55 && clear_calls[1].h == 14,
		"restore the second button background at the popup footer");
	check(clear_calls[0].color == gl_color_selectBG_sd &&
		clear_calls[1].color == gl_color_selectBG_sd,
		"use the current popup button color");
	check(drawn_count == 1 && strcmp(drawn_text, hints) == 0,
		"draw the selected language's A/B button labels");
	check(drawn_x == 60 && drawn_y == 118 && drawn_color == LAUNCHER_SELECTED_TEXT,
		"draw the button labels in the visible popup footer");

	puts("Popup button hint scenarios passed.");
	return 0;
}
