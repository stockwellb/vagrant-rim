#ifndef VR_GAME_INTERNAL_H
#define VR_GAME_INTERNAL_H

// Test-facing declarations for game.c's screen-action handlers. These are the
// pure state-machine transitions (which screen comes next, when a save is
// written, how the quit prompt resolves) that the game loop drives from
// game_draw. They are NOT a public API — this header exists so the headless flow
// tests (test/test_game_flow.c) can call them directly without standing up a
// window. Production code reaches them only from within game.c.

#include "game/game.h"

#include "screen/confirm_quit.h"
#include "screen/loading_menu.h"
#include "screen/pause_menu.h"
#include "screen/settings_menu.h"

// Apply an action emitted by a screen to the game state (see game.c for the
// per-case rationale). Each mutates *game in place: screen transitions, save
// writes, and the quit-confirmation flags.
void handle_loading_menu_action(Game *game, LoadingMenuAction action);
void handle_pause_action(Game *game, PauseMenuAction action);
void handle_settings_action(Game *game, SettingsMenuAction action);
void handle_confirm_quit(Game *game, ConfirmQuitAction action);

// Apply the slot chosen (or SLOT_PICKER_NONE / SLOT_PICKER_BACK) in the picker.
// In LOAD mode a chosen slot loads and enters play; in NEW mode it starts and
// writes a fresh save. Reads game->picker_mode to tell the two apart.
void handle_slot_picker(Game *game, int chosen);

#endif // VR_GAME_INTERNAL_H
