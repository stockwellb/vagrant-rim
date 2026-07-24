#ifndef VR_UI_UI_H
#define VR_UI_UI_H

#include "raylib.h"

// Small shared helpers over the active raygui theme, so every screen draws text
// from one source of truth (the loaded UI font + the .rgs style) instead of
// repeating the same GuiGetFont/GuiGetStyle boilerplate.

// The font raygui is currently drawing with (our loaded UI font, or the built-in).
Font ui_font(void);

// The style's global text spacing, as a float (for MeasureTextEx/DrawTextEx).
float ui_spacing(void);

// GetColor(GuiGetStyle(DEFAULT, property)) — e.g. ui_style_color(TEXT_COLOR_NORMAL).
Color ui_style_color(int property);

// Draw `text` at font `size`, horizontally centered within [left, left + width)
// at vertical position `y`, using the active UI font and spacing.
void ui_text_centered(const char *text, float left, float width, float y, float size,
                      Color color);

// --- Menu navigation -------------------------------------------------------
// raygui buttons are immediate-mode and mouse-only: GuiButton reports a click
// but there is no notion of a "focused" button or keyboard/gamepad traversal.
// MenuNav adds that thin layer so keyboard, gamepad, and mouse all drive the
// same vertical (or horizontal) list of buttons, sharing a single highlight.
//
// Per frame a screen does:
//   ui_menu_nav_begin(nav, item_count);          // read input, latch intent
//   if (ui_menu_button(nav, 0, r0, "A", true)) { ... }   // draw + test each item
//   if (ui_menu_button(nav, 1, r1, "B", ok))   { ... }
//   ui_menu_nav_end(nav);                        // apply movement past disabled

// Upper bound on buttons in a single menu (slots + Back is the largest list).
#define UI_MENU_MAX_ITEMS 16

typedef struct MenuNav {
    int focus;                         // index of the currently focused item
    int count;                         // items registered this frame (set by begin)
    int pending_dir;                   // -1 up / +1 down, applied in end()
    bool activate;                     // Enter / gamepad-A fired this frame
    bool cancel;                       // Esc / gamepad-B fired this frame
    bool stick_latched;                // left stick held past the step threshold
    bool enabled[UI_MENU_MAX_ITEMS];   // per-item enabled, recorded during draw
} MenuNav;

// Reset focus to the top and clear latched state; call on every screen change so
// a new menu opens with its first item focused rather than a stale index.
void ui_menu_nav_reset(MenuNav *nav);

// Read this frame's navigation input for a menu of `count` items. Latches the
// move direction, activate, and cancel edges; actual focus movement happens in
// ui_menu_nav_end once the per-item enabled state is known.
void ui_menu_nav_begin(MenuNav *nav, int count);

// Draw one focus-aware button. Mouse hover claims focus (so pointer and
// keyboard never disagree); the focused item draws highlighted. Returns true if
// clicked OR activated (Enter/gamepad-A while focused). Disabled items draw
// dimmed and are skipped by keyboard/gamepad traversal.
bool ui_menu_button(MenuNav *nav, int index, Rectangle bounds, const char *text,
                    bool enabled);

// Apply the latched move direction, wrapping around the list and skipping
// disabled items, then snap focus onto an enabled item if it isn't on one.
void ui_menu_nav_end(MenuNav *nav);

// True when the player pressed the pause/menu control (Esc or gamepad Start).
bool ui_nav_menu_pressed(void);

// True when the player pressed cancel/back (Esc or gamepad-B).
bool ui_nav_cancel_pressed(void);

// Sample controller state for this frame. Call once per frame before any of the
// nav queries above (drives the macOS GameController-framework input path;
// harmless no-op elsewhere).
void ui_input_poll(void);

#endif // VR_UI_UI_H
