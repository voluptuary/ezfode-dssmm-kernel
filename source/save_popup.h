#ifndef DS_STYLE_SAVE_POPUP_H
#define DS_STYLE_SAVE_POPUP_H

#include "save_popup_text.h"

static u32 Launcher_SaveArtworkFormatValid(const u8 *raw, u32 width, u32 height)
{
    u32 offset, dib, file_width, file_height, compression;

    if(!raw)
        return 0;
    memcpy(&offset, raw + 10, 4);
    memcpy(&dib, raw + 14, 4);
    memcpy(&file_width, raw + 18, 4);
    memcpy(&file_height, raw + 22, 4);
    memcpy(&compression, raw + 30, 4);
    return raw[0] == 'B' && raw[1] == 'M' && offset == 54 && dib == 40 &&
        file_width == width &&
        (file_height == height || file_height == (0u - height)) &&
        (raw[26] == 0 || raw[26] == 1) && !raw[27] &&
        raw[28] == 16 && !raw[29] && !compression;
}

/* Save recovery metadata keeps the ROM basename but not its directory. If the
   recent-path index cannot resolve a root-level game, its conventional GBA
   save basename still identifies the ROM beside the SD root. */
static u32 Launcher_LoadRootHeaderArtworkForSave(const char *save_name,
	const char *stem, u8 *dst)
{
	static const char *const extensions[] = {"gba", "agb"};
	const char *save_extension;
	u8 original_gamecode[4];
	u32 i;
	int written;

	if(!save_name || !stem || !stem[0] || !dst)
		return 0;
	save_extension = strrchr(save_name, '.');
	if(!save_extension || strcasecmp(save_extension, ".sav"))
		return 0;
	memcpy(original_gamecode, GAMECODE, sizeof(original_gamecode));
	for(i = 0; i < sizeof(extensions) / sizeof(extensions[0]); i++)
	{
		written = snprintf(launcher_custom_thumb_path, sizeof(launcher_custom_thumb_path),
			"%s.%s", stem, extensions[i]);
		if(written > 0 && (u32)written < sizeof(launcher_custom_thumb_path) &&
			Launcher_LoadThumbnailFromRomHeaderPath("/", launcher_custom_thumb_path, dst))
		{
			memcpy(GAMECODE, original_gamecode, sizeof(original_gamecode));
			return 1;
		}
	}
	memcpy(GAMECODE, original_gamecode, sizeof(original_gamecode));
	return 0;
}

/* The selected thumbnail style is staged before save I/O, then painted once
   into VRAM. */
static u32 launcher_save_art_custom;
static u32 Launcher_LoadSaveArtwork(const char *save_name)
{
    char stem[MAX_path_len], recent_stem[MAX_path_len];
    char path[MAX_path_len], name[MAX_path_len];
    u8 *raw = pReadCache + 0x10002;
    u8 code[4];
    u32 found, recent_found, width, height;
    width = Launcher_ThumbnailSourceWidth();
    height = Launcher_ThumbnailSourceHeight();
    launcher_save_art_custom = 0;
    Launcher_CustomThumbFileName(save_name, stem, sizeof(stem));
    found = stem[0] && Launcher_LoadCustomThumbnailByName(stem, raw);
    if(found && !Launcher_SaveArtworkFormatValid(raw, width, height))
		found = 0;
    launcher_save_art_custom = found;
    recent_found = !found && Read_last_played_entry(path, sizeof(path), name, sizeof(name));
    if(recent_found)
    {
        Launcher_CustomThumbFileName(name, recent_stem, sizeof(recent_stem));
        memcpy(code, GAMECODE, 4);
        if(!strcasecmp(stem, recent_stem))
            found = Launcher_LoadMirroredThumbnail(path, name, raw);
        if(found && !Launcher_SaveArtworkFormatValid(raw, width, height))
			found = 0;
        if(!found && Launcher_IsGbaFilename(name))
            found = Launcher_LoadThumbnailFromRomHeaderPath(path, name, raw);
        if(found && !Launcher_SaveArtworkFormatValid(raw, width, height))
			found = 0;
        memcpy(GAMECODE, code, 4);
    }
    if(!found && (!recent_found || !strcmp(path, "/")))
        found = Launcher_LoadRootHeaderArtworkForSave(save_name, stem, raw);
    if(found && !Launcher_SaveArtworkFormatValid(raw, width, height))
		found = 0;
    if(found)
    {
		Launcher_ScaleThumbToBox((const u16*)(raw + 54), width, height,
			(u16*)(pReadCache + 0x18000), 48, 32);
    }
    return found;
}

static void Launcher_SavePopupFit(const char *src, char *out, u32 size, u32 cells)
{
    u32 length = DrawText12VisibleLength((char*)src);
    DrawText12CopyVisible(out, size, (char*)src, length > cells ? cells - 3 : cells);
    if(length > cells) strncat(out, "...", size - strlen(out) - 1);
}

static void Launcher_SavePopupLine(const char *src, int x, int y, int width, u32 centre)
{
    char line[100];
    Launcher_SavePopupFit(src, line, sizeof(line), width / 6);
    if(centre) x += (width - DrawText12VisibleLength(line) * 6) / 2;
    DrawHZText12(line, 0, x, y, gl_color_text, 1);
}

static void Launcher_SavePopupName(const char *name, u32 artwork)
{
    char title[128], first[100], second[100], *space;
    u32 cells = artwork ? 15 : 25, used;
    int x = artwork ? 101 : 44, width = artwork ? 90 : 152;
    Launcher_CleanTitle(name, title, sizeof(title));
    DrawText12CopyVisible(first, sizeof(first), title, cells);
    used = DrawText12ByteOffsetForGlyphs(title, cells);
    if(title[used] && (space = strrchr(first, ' ')) && space - first > (int)cells / 2)
    {
        used = space - first;
        *space = 0;
    }
    while(title[used] == ' ') used++;
    Launcher_SavePopupFit(title + used, second, sizeof(second), cells);
    Launcher_SavePopupLine(first, x, second[0] ? 63 : 70, width, !artwork);
    if(second[0]) Launcher_SavePopupLine(second, x, 77, width, !artwork);
}

static void Launcher_SavePopupHeading(u32 writing)
{
    Launcher_SavePopupLine(Launcher_SavePopupText(writing ? 1 : 0), 44, 40, 152, 1);
}

/* Match the established rounded rectangle at the smaller preview size. */
static int Launcher_SaveArtInset(int row)
{
    if(!Launcher_RoundedCornersForCustom(launcher_save_art_custom)) return 0;
    if(row > 15) row = 31 - row;
    return row == 0 ? 3 : row < 3 ? 1 : 0;
}

static void Launcher_DrawSaveArtwork(const u16 *src, int x, int y, int w)
{
    int row, inset, start;
    u16 colour = Launcher_ArtBorderColourEx(1);
    for(row = -1; row <= 32; row++)
    {
        if(row == -1 || row == 32)
        {
            if(launcher_art_border_mode)
            {
                inset = Launcher_SaveArtInset(0);
                Clear(x + (inset ? inset : -1), y + row,
                    inset ? w - 2 * inset : w + 2, 1, colour, 1);
            }
            continue;
        }
        inset = Launcher_SaveArtInset(row);
        Launcher_DrawPicClipStride(src + row * 48 + inset, 48,
            x + inset, y + row, w - 2 * inset, 1);
        if(launcher_art_border_mode)
        {
            /* Join adjacent rows of the rounded rectangle. */
            start = inset - 1;
            if(row > 0 && Launcher_SaveArtInset(row - 1) < start) start = Launcher_SaveArtInset(row - 1);
            if(row < 31 && Launcher_SaveArtInset(row + 1) < start) start = Launcher_SaveArtInset(row + 1);
            Clear(x + start, y + row, inset - start, 1, colour, 1);
            Clear(x + w - inset, y + row, inset - start, 1, colour, 1);
        }
    }
}

static void Launcher_DrawSavePopup(const char *name, u32 artwork, u32 writing)
{
    u16 edge = RGB(15,15,15);
    Launcher_DrawPopupPanel(36, 25);
    Launcher_SavePopupHeading(writing);
    Clear(44, 56, 152, 1, edge, 1);
    if(artwork)
    {
        if(launcher_thumbnail_style == LAUNCHER_THUMB_STYLE_BOX)
            Launcher_DrawSaveArtwork(((const u16*)(pReadCache + 0x18000)) + 8, 53, 62, 32);
        else
            Launcher_DrawSaveArtwork((const u16*)(pReadCache + 0x18000), 45, 62, 48);
    }
    Launcher_SavePopupName(name, artwork);
    Launcher_SavePopupLine(Launcher_SavePopupText(2), 44, 97, 152, 1);
    Launcher_SavePopupLine(Launcher_SavePopupText(3), 44, 111, 152, 1);
}

static void Launcher_SavePopupBeginWrite(void)
{
    /* Only the heading changes; the name, art and safety text stay in place. */
    Launcher_RestoreLegacyMenuRow(39, 15);
    Launcher_SavePopupHeading(1);
}

static void Launcher_DrawSaveTopbar(void)
{
    u8 dt[3];
    char clock[16];
    Launcher_DrawTopbarName(SD_list);
    rtc_enable(); rtc_gettime(dt); rtc_disenable();
    Launcher_FormatClock(clock, sizeof(clock), UNBCD(dt[0] & 0x3F),
        UNBCD(dt[1] & 0x7F), UNBCD(dt[2] & 0x7F));
    DrawHZText12(clock, 0, 189, 3, gl_color_topbar_text, 1);
}

#endif
