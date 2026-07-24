#include "game/game.h"

#include <stdio.h>

#include "raylib.h"

// Candidate locations for the config file, tried in order. Covers both running
// from the project root during development and the installed bin/assets layout.
static const char *kConfigPaths[] = {
    "assets/config.lua",
    "bin/assets/config.lua",
    "config.lua",
};

static void load_config(Game *game)
{
    char err[256];
    for (int i = 0; i < (int)(sizeof(kConfigPaths) / sizeof(kConfigPaths[0])); i++) {
        if (FileExists(kConfigPaths[i])) {
            if (config_load(&game->config, kConfigPaths[i], err, sizeof(err))) {
                TraceLog(LOG_INFO, "CONFIG: loaded '%s'", kConfigPaths[i]);
            } else {
                TraceLog(LOG_WARNING, "CONFIG: failed to load '%s': %s — using defaults",
                         kConfigPaths[i], err);
                config_set_defaults(&game->config);
            }
            return;
        }
    }
    TraceLog(LOG_WARNING, "CONFIG: no config file found — using defaults");
    config_set_defaults(&game->config);
}

void game_init(Game *game)
{
    load_config(game);

    game->screen_width = game->config.window.width;
    game->screen_height = game->config.window.height;

    InitWindow(game->screen_width, game->screen_height, game->config.window.title);
    SetTargetFPS(game->config.window.target_fps);
}

void game_update(Game *game)
{
    (void)game;
    // Simulation updates land here as subsystems come online.
}

void game_draw(Game *game)
{
    BeginDrawing();
    ClearBackground(BLACK);

    const char *title = "VAGRANT RIM";
    int font_size = 40;
    int text_width = MeasureText(title, font_size);
    DrawText(title,
             (game->screen_width - text_width) / 2,
             game->screen_height / 2 - font_size,
             font_size,
             RAYWHITE);

    // Show the config-driven screen dimensions so we can confirm the values
    // are coming from config.lua and not a hardcoded fallback.
    char dims[64];
    snprintf(dims, sizeof(dims), "%d x %d  (from config.lua)",
             game->config.window.width, game->config.window.height);
    DrawText(dims,
             (game->screen_width - MeasureText(dims, 20)) / 2,
             game->screen_height / 2 + 10,
             20,
             GRAY);

    DrawFPS(10, 10);
    EndDrawing();
}

bool game_should_close(const Game *game)
{
    (void)game;
    return WindowShouldClose();
}

void game_shutdown(Game *game)
{
    (void)game;
    CloseWindow();
}
