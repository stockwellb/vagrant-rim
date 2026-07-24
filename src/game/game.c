#include "game/game.h"

#include <stdio.h>

#include "raylib.h"
#include "raygui.h"

#include "screen/loading_menu.h"
#include "screen/pause_menu.h"
#include "screen/slot_picker.h"
#include "screen/confirm_quit.h"
#include "save/save.h"

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
    game->has_save = save_any_slot_used(game->config.save.slots); // enables Continue/Load
    game->active_slot = 0;
    game->last_save_time = -1000.0;
    game->should_quit = false;
    game->confirm_quit = false;
    game->picker_mode = SLOT_PICKER_LOAD;
    game->slot_info_count = 0;

    InitWindow(game->screen_width, game->screen_height, game->config.window.title);
    SetTargetFPS(game->config.window.target_fps);
    SetExitKey(KEY_NULL); // Esc is our pause toggle, not a window-quit

    load_gui_style(game);
    load_ui_font(game);
}

void game_update(Game *game)
{
    // Esc toggles the pause overlay while playing.
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (game->screen == SCREEN_PLAYING) {
            game->screen = SCREEN_PAUSE;
            game->confirm_quit = false;
        } else if (game->screen == SCREEN_PAUSE) {
            if (game->confirm_quit) {
                game->confirm_quit = false; // Esc dismisses the prompt
            } else {
                game->screen = SCREEN_PLAYING;
            }
        }
    }
    // Simulation updates land here as subsystems come online.
}

// Persist the active save to its slot and note the time for the confirmation.
static void save_game(Game *game)
{
    game->save.saved_at = save_now();
    if (save_write_slot(&game->save, game->active_slot)) {
        game->has_save = true;
        game->last_save_time = GetTime();
        TraceLog(LOG_INFO, "SAVE: wrote slot %d", game->active_slot);
    } else {
        TraceLog(LOG_WARNING, "SAVE: failed to write slot %d", game->active_slot);
    }
}

// Handle an action emitted by the pause menu.
static void handle_pause_action(Game *game, PauseMenuAction action)
{
    switch (action) {
        case PAUSE_RESUME:
            game->screen = SCREEN_PLAYING;
            break;
        case PAUSE_SAVE:
            save_game(game);
            break;
        case PAUSE_QUIT:
            game->confirm_quit = true; // ask whether to save before leaving
            break;
        case PAUSE_NONE:
            break;
    }
}

// Handle an action emitted by the quit-confirmation modal. Both outcomes return
// to the loading menu; only the confirm path saves first.
static void handle_confirm_quit(Game *game, ConfirmQuitAction action)
{
    switch (action) {
        case CONFIRM_QUIT_SAVE:
            save_game(game);
            game->confirm_quit = false;
            game->screen = SCREEN_LOADING_MENU;
            break;
        case CONFIRM_QUIT_DISCARD:
            game->confirm_quit = false;
            game->screen = SCREEN_LOADING_MENU;
            break;
        case CONFIRM_QUIT_NONE:
            break;
    }
}

// Enter the slot picker for the given purpose, refreshing its cached summaries.
static void enter_slot_picker(Game *game, SlotPickerMode mode);

// Handle an action emitted by the loading menu.
static void handle_loading_menu_action(Game *game, LoadingMenuAction action)
{
    switch (action) {
        case LOADING_MENU_CONTINUE: {
            // Quick-load the most-recently-saved slot.
            int slot = save_most_recent_slot(game->config.save.slots);
            if (slot >= 0 && save_load_slot(&game->save, slot)) {
                game->active_slot = slot;
                game->screen = SCREEN_PLAYING;
                TraceLog(LOG_INFO, "CONTINUE: loaded slot %d", slot);
            }
            break;
        }
        case LOADING_MENU_LOAD:
            // Choose a slot to load.
            enter_slot_picker(game, SLOT_PICKER_LOAD);
            break;
        case LOADING_MENU_NEW:
            // Choose a slot to start a new game in (so New can't clobber blindly).
            enter_slot_picker(game, SLOT_PICKER_NEW);
            break;
        case LOADING_MENU_EXIT:
            TraceLog(LOG_INFO, "LOADING_MENU: exit");
            game->should_quit = true;
            break;
        case LOADING_MENU_NONE:
            break;
    }
}

// Handle the slot chosen (or Back) in the slot picker.
static void handle_slot_picker(Game *game, int chosen)
{
    if (chosen == SLOT_PICKER_NONE) {
        return;
    }
    if (chosen == SLOT_PICKER_BACK) {
        game->screen = SCREEN_LOADING_MENU;
        return;
    }

    // chosen >= 0: a slot index
    game->active_slot = chosen;
    if (game->picker_mode == SLOT_PICKER_LOAD) {
        if (save_load_slot(&game->save, chosen)) {
            game->screen = SCREEN_PLAYING;
            TraceLog(LOG_INFO, "LOAD: loaded slot %d", chosen);
        } else {
            TraceLog(LOG_WARNING, "LOAD: failed to load slot %d", chosen);
        }
    } else { // SLOT_PICKER_NEW
        save_new(&game->save);
        save_game(game); // writes to active_slot (== chosen)
        game->screen = SCREEN_PLAYING;
        TraceLog(LOG_INFO, "NEW: started in slot %d", chosen);
    }
}

// Rebuild the cached slot summaries the picker renders. Reads each save file
// once (each is a Lua parse), so this is done on entering the picker rather than
// every frame while it is open.
static void refresh_slot_infos(Game *game)
{
    int count = game->config.save.slots;
    long long now = save_now();
    for (int i = 0; i < count; i++) {
        SlotInfo *out = &game->slot_infos[i];
        GameSave s;
        if (save_load_slot(&s, i)) {
            out->used = true;
            long long age = now - s.saved_at;
            long long mins = age / 60;
            const char *when = (mins < 1)    ? "just now"
                               : (mins < 60) ? TextFormat("%lldm ago", mins)
                                             : TextFormat("%lldh ago", mins / 60);
            snprintf(out->label, sizeof(out->label),
                     "fuel %d   A:%d B:%d C:%d   %s",
                     s.fuel, s.resource_a, s.resource_b, s.resource_c, when);
        } else {
            out->used = false;
            snprintf(out->label, sizeof(out->label), "%s",
                     game->config.ui.slot_picker.empty_text);
        }
    }
    game->slot_info_count = count;
}

// Enter the slot picker for the given purpose, refreshing its cached summaries.
static void enter_slot_picker(Game *game, SlotPickerMode mode)
{
    game->picker_mode = mode;
    refresh_slot_infos(game);
    game->screen = SCREEN_SLOT_PICKER;
}

// Draw the (placeholder) play scene: a readout of the active save so we can
// confirm New/Continue populated game->save. Real gameplay lands here.
static void draw_playing(const Game *game)
{
    Font f = GuiGetFont();
    Color c = GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL));
    float spc = (float)GuiGetStyle(DEFAULT, TEXT_SPACING);
    const GameSave *s = &game->save;
    DrawTextEx(f, "[ FLYING ]", (Vector2){ 24, 24 }, 28, spc, c);
    DrawTextEx(f, TextFormat("fuel        %d", s->fuel), (Vector2){ 24, 74 }, 20, spc, c);
    DrawTextEx(f, TextFormat("resources   A:%d  B:%d  C:%d",
                             s->resource_a, s->resource_b, s->resource_c),
               (Vector2){ 24, 102 }, 20, spc, c);
    DrawTextEx(f, TextFormat("position    %.0f, %.0f", s->player_x, s->player_y),
               (Vector2){ 24, 130 }, 20, spc, c);
    DrawTextEx(f, "esc: pause", (Vector2){ 24, (float)game->screen_height - 40 }, 20, spc,
               GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_DISABLED)));
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
            draw_playing(game);
            break;
        case SCREEN_PAUSE: {
            draw_playing(game); // frozen scene behind the overlay
            if (game->confirm_quit) {
                // Esc cancels (handled in game_update)
                ConfirmQuitAction action = confirm_quit_draw(
                    game->screen_width, game->screen_height,
                    &game->config.ui.confirm_quit, &game->config.ui.button);
                handle_confirm_quit(game, action);
            } else {
                bool recently_saved =
                    (GetTime() - game->last_save_time) <
                    game->config.ui.pause_menu.saved_notice_seconds;
                PauseMenuAction action = pause_menu_draw(
                    game->screen_width, game->screen_height, &game->config.ui.pause_menu,
                    &game->config.ui.button, recently_saved);
                handle_pause_action(game, action);
            }
            break;
        }
        case SCREEN_SLOT_PICKER: {
            int chosen = slot_picker_draw(game->screen_width, game->screen_height,
                                          game->picker_mode, &game->config.ui.slot_picker,
                                          game->slot_infos, game->slot_info_count);
            handle_slot_picker(game, chosen);
            break;
        }
    }

    if (game->config.debug.show_fps) {
        DrawFPS(10, 10);
    }

    EndDrawing();
}

bool game_should_close(const Game *game)
{
    return game->should_quit || WindowShouldClose();
}

void game_shutdown(Game *game)
{
    (void)game;
    CloseWindow();
}
