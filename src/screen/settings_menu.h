#ifndef VR_SCREEN_SETTINGS_MENU_H
#define VR_SCREEN_SETTINGS_MENU_H

#include "config/config.h"
#include "ui/ui.h" // MenuNav

// Actions the settings screen can emit in a given frame.
typedef enum SettingsMenuAction {
    SETTINGS_MENU_NONE = 0,
    SETTINGS_MENU_BACK, // leave the screen (caller persists + returns to origin)
} SettingsMenuAction;

// Draw the settings screen — music- and SFX-volume sliders plus a mute toggle
// and a Back button — and return the action chosen this frame. Reads and writes
// the live audio mixer directly, so changes are heard immediately; persisting
// them is the caller's job on BACK. Content comes from `menu`, row geometry from
// `button`. Must be called within a BeginDrawing()/EndDrawing() pass.
SettingsMenuAction settings_menu_draw(MenuNav *nav, int screen_width, int screen_height,
                                      const SettingsMenuConfig *menu,
                                      const ButtonConfig *button);

#endif // VR_SCREEN_SETTINGS_MENU_H
