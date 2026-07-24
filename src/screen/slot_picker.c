#include "screen/slot_picker.h"

#include "raygui.h"
#include "raylib.h"

#include "ui/ui.h"

int slot_picker_draw(MenuNav *nav, int screen_width, int screen_height, SlotPickerMode mode,
                     const SlotPickerConfig *cfg, const ButtonConfig *button,
                     const SlotInfo *slots, int count)
{
    // One focusable item per slot, plus Back at the end (index == count).
    ui_menu_nav_begin(nav, count + 1);

    Color text_color = ui_style_color(TEXT_COLOR_NORMAL);

    // --- Title -----------------------------------------------------------
    const char *title = (mode == SLOT_PICKER_LOAD) ? cfg->load_title : cfg->new_title;
    ui_text_centered(title, 0.0f, (float)screen_width, 60.0f, (float)cfg->title_size,
                     text_color);

    // --- Layout ----------------------------------------------------------
    // Anchor Back to the bottom so it is always visible, then fit the slot rows
    // into the space above it. When there are more slots than fit at the
    // configured row height (up to SAVE_MAX_SLOTS), shrink the per-row stride so
    // no row — or the Back button — ever renders offscreen.
    const float row_w = (float)cfg->row_width;
    const float gap = (float)cfg->row_gap;
    const float back_w = (float)button->width;
    const float back_h = (float)button->height;
    const float back_margin = 24.0f;
    const float top = 150.0f;
    float x = (screen_width - row_w) / 2.0f;
    float back_y = (float)screen_height - back_h - back_margin;

    float stride = (float)cfg->row_height + gap;
    float region = back_y - back_margin - top; // vertical space available for rows
    if (count > 0 && (float)count * stride > region) {
        stride = region / (float)count;
    }
    float row_h = stride - gap;
    if (row_h < 16.0f) {
        row_h = stride; // stride shrank so far the gap would dominate; drop the gap
    }

    // Left-align row text so the slot number + summary read as a list.
    int prev_align = GuiGetStyle(DEFAULT, TEXT_ALIGNMENT);
    int prev_pad = GuiGetStyle(DEFAULT, TEXT_PADDING);
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
    GuiSetStyle(DEFAULT, TEXT_PADDING, 14);

    int chosen = SLOT_PICKER_NONE;

    float y = top;
    for (int i = 0; i < count; i++) {
        const char *label = TextFormat("SLOT %d   %s", i + 1, slots[i].label);

        // In load mode, empty slots can't be selected.
        bool selectable = (mode == SLOT_PICKER_LOAD) ? slots[i].used : true;
        if (ui_menu_button(nav, i, (Rectangle){ x, y, row_w, row_h }, label, selectable)) {
            chosen = i;
        }

        y += stride;
    }

    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, prev_align);
    GuiSetStyle(DEFAULT, TEXT_PADDING, prev_pad);

    // --- Back (bottom-anchored, centered) --------------------------------
    if (ui_menu_button(nav, count,
                       (Rectangle){ (screen_width - back_w) / 2.0f, back_y, back_w, back_h },
                       cfg->back_text, true)) {
        chosen = SLOT_PICKER_BACK;
    }

    ui_menu_nav_end(nav);

    // Esc / gamepad-B backs out, matching the Back button.
    if (nav->cancel) {
        chosen = SLOT_PICKER_BACK;
    }
    return chosen;
}
