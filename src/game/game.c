#include "game/game.h"

#include <stdio.h>

#include "raylib.h"
#include "raygui.h"

#include "screen/loading_menu.h"

// Candidate locations for the config file, tried in order. Covers both running
// from the project root during development and the installed bin/assets layout.
static const char *kConfigPaths[] = {
    "assets/config.lua",
    "bin/assets/config.lua",
    "config.lua",
};

// Asset directory prefixes tried when resolving a relative asset path. Mirrors
// the layout assumptions in kConfigPaths (dev root vs. installed bin/assets).
static const char *kAssetPrefixes[] = { "assets/", "bin/assets/", "" };

// Resolve a relative asset path against the known prefixes into `out`.
// Returns true and fills `out` if an existing file is found.
static bool resolve_asset(const char *rel, char *out, int out_size)
{
    for (int i = 0; i < (int)(sizeof(kAssetPrefixes) / sizeof(kAssetPrefixes[0])); i++) {
        snprintf(out, (size_t)out_size, "%s%s", kAssetPrefixes[i], rel);
        if (FileExists(out)) {
            return true;
        }
    }
    return false;
}

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

// Apply the configured raygui style (.rgs), if any. Must run after InitWindow.
// In this toolchain a .rgs supplies COLORS only, not a font (see load_ui_font).
static void load_gui_style(const Game *game)
{
    const char *style = game->config.ui.style_file;
    if (style[0] == '\0') {
        return; // no style configured — keep raygui default colors
    }

    char path[512];
    if (!resolve_asset(style, path, sizeof(path))) {
        TraceLog(LOG_WARNING, "STYLE: '%s' not found — using raygui default", style);
        return;
    }

    GuiLoadStyle(path);
    TraceLog(LOG_INFO, "STYLE: loaded '%s'", path);

    // Defensive: if a style ever repoints raylib's shapes texture to a font
    // atlas whose white-pixel rec isn't opaque white, raygui rectangles render
    // transparent. Reset to the default 1x1 white pixel so fills always draw.
    SetShapesTexture((Texture2D){ 0 }, (Rectangle){ 0, 0, 0, 0 });
}

// Load the configured UI font (.ttf/.otf) and make raygui use it. Loaded
// ourselves because the .rgs cannot supply a usable font here. The atlas is
// rasterized at max(font_size, title_size) with bilinear filtering so both
// button text and the large title render smoothly. Must run after InitWindow.
static void load_ui_font(const Game *game)
{
    const char *font = game->config.ui.font_file;
    int size = game->config.ui.font_size;
    GuiSetStyle(DEFAULT, TEXT_SIZE, size); // widget text size, even with built-in font

    if (font[0] == '\0') {
        return; // keep raygui's built-in font
    }

    char path[512];
    if (!resolve_asset(font, path, sizeof(path))) {
        TraceLog(LOG_WARNING, "FONT: '%s' not found — using built-in font", font);
        return;
    }

    // Rasterize at the largest size we draw so downscaling stays crisp.
    int atlas = size;
    if (game->config.ui.loading_menu.title_size > atlas) {
        atlas = game->config.ui.loading_menu.title_size;
    }
    Font f = LoadFontEx(path, atlas, NULL, 0);
    SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
    GuiSetFont(f);
    TraceLog(LOG_INFO, "FONT: loaded '%s' (atlas %dpx, text %dpx)", path, atlas, size);
}

void game_init(Game *game)
{
    load_config(game);

    game->screen_width = game->config.window.width;
    game->screen_height = game->config.window.height;
    game->screen = SCREEN_LOADING_MENU;
    game->has_save = false; // no save system yet; gates the Continue option

    InitWindow(game->screen_width, game->screen_height, game->config.window.title);
    SetTargetFPS(game->config.window.target_fps);

    load_gui_style(game);
    load_ui_font(game);
}

void game_update(Game *game)
{
    (void)game;
    // Simulation updates land here as subsystems come online.
}

// Handle an action emitted by the loading menu.
static void handle_loading_menu_action(Game *game, LoadingMenuAction action)
{
    switch (action) {
        case LOADING_MENU_CONTINUE:
            TraceLog(LOG_INFO, "LOADING_MENU: continue (not yet implemented)");
            break;
        case LOADING_MENU_LOAD:
            TraceLog(LOG_INFO, "LOADING_MENU: load (not yet implemented)");
            break;
        case LOADING_MENU_NEW:
            TraceLog(LOG_INFO, "LOADING_MENU: new game");
            game->screen = SCREEN_PLAYING;
            break;
        case LOADING_MENU_NONE:
            break;
    }
}

void game_draw(Game *game)
{
    BeginDrawing();
    // Background comes from the active raygui style, so the whole UI shares one
    // theme source (the .rgs); falls back to raygui's default when none is set.
    ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

    switch (game->screen) {
        case SCREEN_LOADING_MENU: {
            LoadingMenuAction action = loading_menu_draw(
                game->screen_width, game->screen_height, game->has_save,
                &game->config.ui.loading_menu, &game->config.ui.button);
            handle_loading_menu_action(game, action);
            break;
        }
        case SCREEN_PLAYING:
            DrawText("[ flying — nothing here yet ]", 20,
                     game->screen_height / 2, 20, GRAY);
            break;
    }

    if (game->config.debug.show_fps) {
        DrawFPS(10, 10);
    }

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
