#ifndef VR_SCREEN_LOADING_MENU_H
#define VR_SCREEN_LOADING_MENU_H

#include <stdbool.h>

#include "config/config.h"
#include "ui/ui.h" // MenuNav

// Actions the loading menu can emit in a given frame. LOADING_MENU_NONE means
// the player has not chosen anything yet this frame.
typedef enum LoadingMenuAction {
    LOADING_MENU_NONE = 0,
    LOADING_MENU_CONTINUE,
    LOADING_MENU_LOAD,
    LOADING_MENU_NEW,
    LOADING_MENU_EXIT,
} LoadingMenuAction;

// Draw the loading / title menu and return the action chosen this frame (if
// any). `has_save` controls whether the Continue option is selectable. Content
// comes from `menu`, button geometry from `button`. Colors and widget text size
// come from the active raygui style (GuiGetStyle), not from config.
// Must be called within a BeginDrawing()/EndDrawing() pass (raygui is immediate mode).
LoadingMenuAction loading_menu_draw(MenuNav *nav, int screen_width, int screen_height,
                                    bool has_save, const LoadingMenuConfig *menu,
                                    const ButtonConfig *button);

#endif // VR_SCREEN_LOADING_MENU_H
