#include "screen/loading_menu.h"

#include "raygui.h"
#include "raylib.h"

#include "ui/ui.h"

LoadingMenuAction loading_menu_draw(int screen_width, int screen_height, bool has_save,
                                    const LoadingMenuConfig *menu,
                                    const ButtonConfig *button)
{
    // Title/tagline share the loaded UI font so the whole screen is consistent.
    // Title uses the style's normal text color; the tagline is that same color
    // dimmed 30% — readable, but clearly secondary. (The style's DISABLED color
    // was too dim to read on the dark background.)
    Color title_color = ui_style_color(TEXT_COLOR_NORMAL);
    Color tagline_color = ColorBrightness(title_color, -0.30f);

    // --- Title -----------------------------------------------------------
    float title_y = screen_height / 4.0f;
    ui_text_centered(menu->title_text, 0.0f, (float)screen_width, title_y,
                     (float)menu->title_size, title_color);
    ui_text_centered(menu->tagline_text, 0.0f, (float)screen_width,
                     title_y + menu->title_size + 8.0f, (float)menu->tagline_size,
                     tagline_color);

    // --- Menu buttons ----------------------------------------------------
    const float btn_w = (float)button->width;
    const float btn_h = (float)button->height;
    const float gap = (float)button->gap;
    float x = (screen_width - btn_w) / 2.0f;
    float y = screen_height / 2.0f;

    LoadingMenuAction action = LOADING_MENU_NONE;

    // Continue is only selectable when a save exists.
    if (!has_save) {
        GuiSetState(STATE_DISABLED);
    }
    if (GuiButton((Rectangle){ x, y, btn_w, btn_h }, menu->continue_text)) {
        action = LOADING_MENU_CONTINUE;
    }
    GuiSetState(STATE_NORMAL);

    y += btn_h + gap;
    if (GuiButton((Rectangle){ x, y, btn_w, btn_h }, menu->load_text)) {
        action = LOADING_MENU_LOAD;
    }

    y += btn_h + gap;
    if (GuiButton((Rectangle){ x, y, btn_w, btn_h }, menu->new_text)) {
        action = LOADING_MENU_NEW;
    }

    y += btn_h + gap;
    if (GuiButton((Rectangle){ x, y, btn_w, btn_h }, menu->exit_text)) {
        action = LOADING_MENU_EXIT;
    }

    return action;
}
