#include "screen/pause_menu.h"

#include "raygui.h"
#include "raylib.h"

#include "ui/ui.h"

PauseMenuAction pause_menu_draw(MenuNav *nav, int screen_width, int screen_height,
                                const PauseMenuConfig *menu, const ButtonConfig *button,
                                bool recently_saved)
{
    // Resume (0), Save (1), Quit (2).
    ui_menu_nav_begin(nav, 3);

    // Dim the frozen play scene behind the menu.
    DrawRectangle(0, 0, screen_width, screen_height,
                  (Color){ 0, 0, 0, (unsigned char)menu->scrim_alpha });

    Color text_color = ui_style_color(TEXT_COLOR_NORMAL);

    // --- Title -----------------------------------------------------------
    ui_text_centered(menu->title_text, 0.0f, (float)screen_width, screen_height / 4.0f,
                     (float)menu->title_size, text_color);

    // --- Buttons ---------------------------------------------------------
    const float bw = (float)button->width;
    const float bh = (float)button->height;
    const float gap = (float)button->gap;
    float x = (screen_width - bw) / 2.0f;
    float y = screen_height / 2.0f;

    PauseMenuAction action = PAUSE_NONE;

    if (ui_menu_button(nav, 0, (Rectangle){ x, y, bw, bh }, menu->resume_text, true)) {
        action = PAUSE_RESUME;
    }

    y += bh + gap;
    if (ui_menu_button(nav, 1, (Rectangle){ x, y, bw, bh }, menu->save_text, true)) {
        action = PAUSE_SAVE;
    }
    if (recently_saved) {
        const float notice_size = 20.0f;
        DrawTextEx(ui_font(), menu->saved_notice,
                   (Vector2){ x + bw + 16.0f, y + (bh - notice_size) / 2.0f },
                   notice_size, ui_spacing(), text_color);
    }

    y += bh + gap;
    if (ui_menu_button(nav, 2, (Rectangle){ x, y, bw, bh }, menu->quit_text, true)) {
        action = PAUSE_QUIT;
    }

    ui_menu_nav_end(nav);
    return action;
}
