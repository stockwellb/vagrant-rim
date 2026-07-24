#ifndef VR_SCREEN_CONFIRM_QUIT_H
#define VR_SCREEN_CONFIRM_QUIT_H

#include "config/config.h"
#include "ui/ui.h" // MenuNav

// Actions the quit-confirmation modal can emit in a given frame.
typedef enum ConfirmQuitAction {
    CONFIRM_QUIT_NONE = 0,
    CONFIRM_QUIT_SAVE,    // confirm: save, then quit to the menu
    CONFIRM_QUIT_DISCARD, // cancel: quit to the menu without saving
} ConfirmQuitAction;

// Draw the "save before quitting?" modal (a dim scrim, a bordered box, a title,
// a message, and confirm/cancel buttons) over the frozen scene and return the
// action chosen this frame. Content and box geometry come from `cfg`; button
// height from `button`; colors/font from the active style. (Dismissing via Esc
// is handled by the caller, not this screen.)
// Must be called within a BeginDrawing()/EndDrawing() pass.
ConfirmQuitAction confirm_quit_draw(MenuNav *nav, int screen_width, int screen_height,
                                    const ConfirmQuitConfig *cfg,
                                    const ButtonConfig *button);

#endif // VR_SCREEN_CONFIRM_QUIT_H
