#ifndef VR_GAME_H
#define VR_GAME_H

#include <stdbool.h>

#include "config/config.h"

// Which screen the game is currently presenting.
typedef enum ScreenId {
    SCREEN_LOADING_MENU = 0,
    SCREEN_PLAYING,
} ScreenId;

// Top-level game state. Individual subsystems (ship, world, economy, ...)
// will hang off of this struct as the project grows.
typedef struct Game {
    Config config;
    int screen_width;
    int screen_height;
    ScreenId screen;
    bool has_save; // whether a saved game exists (gates the Continue option)
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
