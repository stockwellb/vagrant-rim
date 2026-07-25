#include "screen/settings_menu.h"

#include "raygui.h"
#include "raylib.h"

#include "audio/audio.h"
#include "ui/ui.h"

// Volume nudged per left/right step, as a fraction of the 0..1 range.
#define SETTINGS_VOLUME_STEP 0.05f

SettingsMenuAction settings_menu_draw(MenuNav *nav, int screen_width, int screen_height,
                                      const SettingsMenuConfig *menu,
                                      const ButtonConfig *button)
{
    // Music (0), SFX (1), Mute (2), Back (3). VERTICAL so left/right adjust the
    // focused slider instead of moving focus.
    ui_menu_nav_begin_mode(nav, 4, MENU_NAV_VERTICAL);

    Color text_color = ui_style_color(TEXT_COLOR_NORMAL);

    // --- Title -----------------------------------------------------------
    ui_text_centered(menu->title_text, 0.0f, (float)screen_width, screen_height / 4.0f,
                     (float)menu->title_size, text_color);

    // --- Rows ------------------------------------------------------------
    // Wider than a plain button so the sliders have room; height/gap are shared.
    const float row_w = 520.0f;
    const float row_h = (float)button->height;
    const float gap = (float)button->gap;
    float x = (screen_width - row_w) / 2.0f;
    float y = screen_height / 2.0f;

    // Sliders read the live mixer, draw/adjust, and write straight back — so the
    // change is audible this frame. Persistence happens on Back, in the caller.
    Rectangle row = { x, y, row_w, row_h };
    audio_set_music_volume(
        ui_menu_slider(nav, 0, row, menu->music_label, audio_music_volume(),
                       SETTINGS_VOLUME_STEP));

    row.y += row_h + gap;
    audio_set_sfx_volume(
        ui_menu_slider(nav, 1, row, menu->sfx_label, audio_sfx_volume(),
                       SETTINGS_VOLUME_STEP));

    // Mute is a plain button whose label carries its state; activating flips it.
    row.y += row_h + gap;
    bool muted = audio_muted();
    const char *mute_text =
        TextFormat("%s:  %s", menu->mute_label, muted ? menu->mute_on_text : menu->mute_off_text);
    if (ui_menu_button(nav, 2, row, mute_text, true)) {
        audio_set_muted(!muted);
    }

    row.y += row_h + gap;
    SettingsMenuAction action = SETTINGS_MENU_NONE;
    if (ui_menu_button(nav, 3, row, menu->back_text, true)) {
        action = SETTINGS_MENU_BACK;
    }

    // Esc / gamepad-B also backs out.
    if (nav->cancel) {
        action = SETTINGS_MENU_BACK;
    }

    ui_menu_nav_end(nav);
    return action;
}
