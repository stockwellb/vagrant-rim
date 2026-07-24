#include "game/game.h"

#include <stdio.h>

#include "raylib.h"
#include "raygui.h"

#include "screen/loading_menu.h"
#include "screen/pause_menu.h"
#include "screen/slot_picker.h"
#include "save/save.h"

// Show the "saved" confirmation in the pause menu for this many seconds.
#define SAVE_NOTICE_SECONDS 2.0

// Save directory, resolved relative to the working directory (the project root
// during development via set_rundir).
static const char *kSaveDir = "saves";

// Build the file path for a save slot into `out`.
static void slot_path(int slot, char *out, int out_size)
{
    snprintf(out, (size_t)out_size, "%s/slot%d.lua", kSaveDir, slot);
}

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

// Whether any save slot exists on disk (gates Continue/Load).
static bool any_slot_used(const Game *game)
{
    char path[512];
    for (int i = 0; i < game->config.save.slots; i++) {
        slot_path(i, path, sizeof(path));
        if (save_exists(path)) {
            return true;
        }
    }
    return false;
}

// Find the slot with the newest saved_at, or -1 if no slots exist.
static int most_recent_slot(const Game *game)
{
    char path[512];
    int best = -1;
    long long best_time = -1;
    for (int i = 0; i < game->config.save.slots; i++) {
        slot_path(i, path, sizeof(path));
        GameSave s;
        if (save_load(&s, path) && s.saved_at > best_time) {
            best_time = s.saved_at;
            best = i;
        }
    }
    return best;
}

void game_init(Game *game)
{
    load_config(game);

    game->screen_width = game->config.window.width;
    game->screen_height = game->config.window.height;
    game->screen = SCREEN_LOADING_MENU;
    game->has_save = any_slot_used(game); // enables Continue/Load if a save exists
    game->active_slot = 0;
    game->last_save_time = -1000.0;
    game->should_quit = false;
    game->confirm_quit = false;
    game->picker_mode = SLOT_PICKER_LOAD;

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
    if (!DirectoryExists(kSaveDir)) {
        MakeDirectory(kSaveDir);
    }
    game->save.saved_at = save_now();

    char path[512];
    slot_path(game->active_slot, path, sizeof(path));
    if (save_write(&game->save, path)) {
        game->has_save = true;
        game->last_save_time = GetTime();
        TraceLog(LOG_INFO, "SAVE: wrote '%s'", path);
    } else {
        TraceLog(LOG_WARNING, "SAVE: failed to write '%s'", path);
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

// Handle an action emitted by the loading menu.
static void handle_loading_menu_action(Game *game, LoadingMenuAction action)
{
    char path[512];
    switch (action) {
        case LOADING_MENU_CONTINUE: {
            // Quick-load the most-recently-saved slot.
            int slot = most_recent_slot(game);
            if (slot >= 0) {
                slot_path(slot, path, sizeof(path));
                if (save_load(&game->save, path)) {
                    game->active_slot = slot;
                    game->screen = SCREEN_PLAYING;
                    TraceLog(LOG_INFO, "CONTINUE: loaded slot %d", slot);
                }
            }
            break;
        }
        case LOADING_MENU_LOAD:
            // Choose a slot to load.
            game->picker_mode = SLOT_PICKER_LOAD;
            game->screen = SCREEN_SLOT_PICKER;
            break;
        case LOADING_MENU_NEW:
            // Choose a slot to start a new game in (so New can't clobber blindly).
            game->picker_mode = SLOT_PICKER_NEW;
            game->screen = SCREEN_SLOT_PICKER;
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
        char path[512];
        slot_path(chosen, path, sizeof(path));
        if (save_load(&game->save, path)) {
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

// Build the summary array the slot picker renders. Returns the slot count.
static int build_slot_infos(const Game *game, SlotInfo *out)
{
    int count = game->config.save.slots;
    long long now = save_now();
    char path[512];
    for (int i = 0; i < count; i++) {
        slot_path(i, path, sizeof(path));
        GameSave s;
        if (save_load(&s, path)) {
            out[i].used = true;
            long long age = now - s.saved_at;
            long long mins = age / 60;
            const char *when = (mins < 1)    ? "just now"
                               : (mins < 60) ? TextFormat("%lldm ago", mins)
                                             : TextFormat("%lldh ago", mins / 60);
            snprintf(out[i].label, sizeof(out[i].label),
                     "fuel %d   A:%d B:%d C:%d   %s",
                     s.fuel, s.resource_a, s.resource_b, s.resource_c, when);
        } else {
            out[i].used = false;
            snprintf(out[i].label, sizeof(out[i].label), "%s", "- empty -");
        }
    }
    return count;
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

// Draw the "save before quitting?" modal and handle its buttons. Uses buttons
// sized like the rest of the UI (GuiMessageBox hardcodes 24px buttons, which
// collide with our larger font).
static void draw_confirm_quit(Game *game)
{
    int w = game->screen_width, h = game->screen_height;
    DrawRectangle(0, 0, w, h, (Color){ 0, 0, 0, 180 });

    Font f = GuiGetFont();
    float spc = (float)GuiGetStyle(DEFAULT, TEXT_SPACING);
    Color txt = GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL));
    Color fill = GetColor(GuiGetStyle(DEFAULT, BASE_COLOR_NORMAL));
    Color border = GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_NORMAL));

    float bw = 560.0f, bh = 240.0f;
    Rectangle box = { (w - bw) / 2.0f, (h - bh) / 2.0f, bw, bh };
    DrawRectangleRec(box, fill);
    DrawRectangleLinesEx(box, 2.0f, border);

    const char *title = "QUIT TO MENU";
    Vector2 ts = MeasureTextEx(f, title, 28.0f, spc);
    DrawTextEx(f, title, (Vector2){ box.x + (bw - ts.x) / 2.0f, box.y + 28.0f }, 28.0f, spc, txt);

    const char *msg = "Save before quitting?";
    Vector2 ms = MeasureTextEx(f, msg, 22.0f, spc);
    DrawTextEx(f, msg, (Vector2){ box.x + (bw - ms.x) / 2.0f, box.y + 92.0f }, 22.0f, spc, txt);

    float btn_w = 200.0f;
    float btn_h = (float)game->config.ui.button.height;
    float gap = 24.0f;
    float total = btn_w * 2.0f + gap;
    float bx = box.x + (bw - total) / 2.0f;
    float by = box.y + bh - btn_h - 28.0f;

    if (GuiButton((Rectangle){ bx, by, btn_w, btn_h }, "YES")) {
        save_game(game);
        game->confirm_quit = false;
        game->screen = SCREEN_LOADING_MENU;
    }
    if (GuiButton((Rectangle){ bx + btn_w + gap, by, btn_w, btn_h }, "NO")) {
        game->confirm_quit = false;
        game->screen = SCREEN_LOADING_MENU;
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
            draw_playing(game);
            break;
        case SCREEN_PAUSE: {
            draw_playing(game); // frozen scene behind the overlay
            if (game->confirm_quit) {
                draw_confirm_quit(game); // Esc cancels (handled in game_update)
            } else {
                bool recently_saved =
                    (GetTime() - game->last_save_time) < SAVE_NOTICE_SECONDS;
                PauseMenuAction action = pause_menu_draw(
                    game->screen_width, game->screen_height, &game->config.ui.button,
                    recently_saved);
                handle_pause_action(game, action);
            }
            break;
        }
        case SCREEN_SLOT_PICKER: {
            SlotInfo slots[SAVE_MAX_SLOTS];
            int count = build_slot_infos(game, slots);
            int chosen = slot_picker_draw(game->screen_width, game->screen_height,
                                          game->picker_mode, slots, count);
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
