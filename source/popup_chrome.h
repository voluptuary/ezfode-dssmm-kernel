#ifndef DS_STYLE_POPUP_CHROME_H
#define DS_STYLE_POPUP_CHROME_H

static void Launcher_DrawPopupPanel(u32 x, u32 y)
{
    const u16 *image = (const u16*)gImage_MENU;
    u32 custom = image != (const u16*)gImage_MENU_BLANK && image != (const u16*)gImage_MENU_DARK;
    DrawPic((u16*)image, x, y, 168, 110, custom, 0, 1);
}

static void Launcher_DrawPopupButtonHints(void)
{
    Clear(60, 117, 55, 14, gl_color_selectBG_sd, 1);
    Clear(125, 117, 55, 14, gl_color_selectBG_sd, 1);
    DrawHZText12(gl_menu_btn, 0, 60, 118, LAUNCHER_SELECTED_TEXT, 1);
}

#endif
