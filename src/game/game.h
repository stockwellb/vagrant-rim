#ifndef VR_GAME_H
#define VR_GAME_H

#include <stdbool.h>

#include "raylib.h" // Font, for the loaded UI font handle

#include "config/config.h"
#include "save/save.h"
#include "screen/slot_picker.h"
#include "ui/ui.h" // MenuNav

// Which screen the game is currently presenting.
typedef enum ScreenId {
    SCREEN_LOADING_MENU = 0,
    SCREEN_PLAYING,
    SCREEN_PAUSE,       // pause overlay drawn on top of the frozen play scene
    SCREEN_SLOT_PICKER, // choose a slot to load into or start a new game in
} ScreenId;

// Top-level game state. Individual subsystems (ship, world, economy, ...)
// will hang off of this struct as the project grows.
typedef struct Game {
    Config config;
    int screen_width;
    int screen_height;
    ScreenId screen;
    bool has_save;         // whether any save exists on disk (gates Continue)
    GameSave save;         // the active/most-recent game save
    int active_slot;       // slot the current game loaded from / saves to
    double last_save_time; // GetTime() of the last save, for the "saved" confirmation
    bool should_quit;      // set by an Exit action to end the game loop
    bool confirm_quit;     // pause menu is showing the "save before quitting?" prompt
    SlotPickerMode picker_mode; // what the slot picker is currently for
    SlotInfo slot_infos[SAVE_MAX_SLOTS]; // slot summaries, rebuilt on entering the picker
    int slot_info_count;        // number of valid entries in slot_infos
    Font ui_font;               // custom UI font handle; valid only if ui_font_loaded
    bool ui_font_loaded;        // whether we own ui_font and must UnloadFont it at shutdown
    char toast_text[64];        // transient status/error message (e.g. "SAVE FAILED")
    double toast_until;         // GetTime() before which the toast is drawn
    MenuNav nav;                // keyboard/gamepad focus state for the active menu
    ScreenId prev_screen;       // screen drawn last frame, to detect transitions
    bool prev_confirm;          // confirm_quit state last frame (pause vs. modal)
} Game;

// Lifecycle: create the window and initialize subsystems.
void game_init(Game *game);

// Advance simulation by one frame.
void game_update(Game *game);

// Render the current frame.
void game_draw(Game *game);

// Whether the game loop should terminate (window closed, quit requested).
bool game_should_close(const Game *game);

// Tear down subsystems and close the window.
void game_shutdown(Game *game);

#endif // VR_GAME_H
