#include "screen/confirm_quit.h"

#include "raygui.h"
#include "raylib.h"

#include "ui/ui.h"

ConfirmQuitAction confirm_quit_draw(int screen_width, int screen_height,
                                    const ConfirmQuitConfig *cfg,
                                    const ButtonConfig *button)
{
    // Dim the frozen scene behind the modal.
    DrawRectangle(0, 0, screen_width, screen_height,
                  (Color){ 0, 0, 0, (unsigned char)cfg->scrim_alpha });

    Color txt = ui_style_color(TEXT_COLOR_NORMAL);
    Color fill = ui_style_color(BASE_COLOR_NORMAL);
    Color border = ui_style_color(BORDER_COLOR_NORMAL);

    // --- Box -------------------------------------------------------------
    const float bw = (float)cfg->box_width;
    const float bh = (float)cfg->box_height;
    Rectangle box = { (screen_width - bw) / 2.0f, (screen_height - bh) / 2.0f, bw, bh };
    DrawRectangleRec(box, fill);
    DrawRectangleLinesEx(box, 2.0f, border);

    // --- Title / message -------------------------------------------------
    ui_text_centered(cfg->title_text, box.x, bw, box.y + 28.0f, (float)cfg->title_size, txt);
    ui_text_centered(cfg->message_text, box.x, bw, box.y + 92.0f, (float)cfg->message_size,
                     txt);

    // --- Buttons (YES / NO), sharing the configured button geometry ------
    const float btn_w = (float)button->width;
    const float btn_h = (float)button->height;
    const float col_gap = 24.0f;
    float total = btn_w * 2.0f + col_gap;
    float bx = box.x + (bw - total) / 2.0f;
    float by = box.y + bh - btn_h - 28.0f;

    ConfirmQuitAction action = CONFIRM_QUIT_NONE;
    if (GuiButton((Rectangle){ bx, by, btn_w, btn_h }, cfg->confirm_text)) {
        action = CONFIRM_QUIT_SAVE;
    }
    if (GuiButton((Rectangle){ bx + btn_w + col_gap, by, btn_w, btn_h }, cfg->cancel_text)) {
        action = CONFIRM_QUIT_DISCARD;
    }
    return action;
}
