#ifndef VR_SCREEN_PAUSE_MENU_H
#define VR_SCREEN_PAUSE_MENU_H

#include <stdbool.h>

#include "config/config.h"
#include "ui/ui.h" // MenuNav

// Actions the pause menu can emit in a given frame.
typedef enum PauseMenuAction {
    PAUSE_NONE = 0,
    PAUSE_RESUME,
    PAUSE_SAVE,
    PAUSE_QUIT, // quit to the main menu
} PauseMenuAction;

// Draw the pause overlay (a dim scrim over the frozen play scene, a title, and
// buttons) and return the action chosen this frame. `recently_saved` shows a
// brief confirmation next to the Save button. Content comes from `menu`, button
// geometry from `button`; colors/font from the active style.
// Must be called within a BeginDrawing()/EndDrawing() pass.
PauseMenuAction pause_menu_draw(MenuNav *nav, int screen_width, int screen_height,
                                const PauseMenuConfig *menu, const ButtonConfig *button,
                                bool recently_saved);

#endif // VR_SCREEN_PAUSE_MENU_H
