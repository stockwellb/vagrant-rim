#ifndef VR_SCREEN_SLOT_PICKER_H
#define VR_SCREEN_SLOT_PICKER_H

#include <stdbool.h>

#include "config/config.h"
#include "ui/ui.h" // MenuNav

// Per-slot summary the picker renders. `label` is a human-readable one-liner
// built by the caller (e.g. "fuel 100  A:0 B:0 C:0  2m ago" or "- empty -").
typedef struct SlotInfo {
    bool used;
    char label[96];
} SlotInfo;

// What the picker is for. Determines the title and which slots are selectable.
typedef enum SlotPickerMode {
    SLOT_PICKER_LOAD, // select a used slot to load
    SLOT_PICKER_NEW,  // select any slot to start a new game (overwrites if used)
} SlotPickerMode;

// Non-slot return values from slot_picker_draw (>= 0 means a slot was chosen).
#define SLOT_PICKER_NONE (-1) // nothing chosen this frame
#define SLOT_PICKER_BACK (-2) // back/cancel pressed

// Draw the slot picker and return the slot index chosen this frame, or
// SLOT_PICKER_NONE / SLOT_PICKER_BACK. `slots` has `count` entries. Content and
// row geometry come from `cfg`; the Back button uses the shared `button`
// geometry; colors/font from the active style. Rows are shrunk to fit when there
// are more slots than fit at the configured height, so nothing renders offscreen.
// Must be called within a BeginDrawing()/EndDrawing() pass.
int slot_picker_draw(MenuNav *nav, int screen_width, int screen_height, SlotPickerMode mode,
                     const SlotPickerConfig *cfg, const ButtonConfig *button,
                     const SlotInfo *slots, int count);

#endif // VR_SCREEN_SLOT_PICKER_H
