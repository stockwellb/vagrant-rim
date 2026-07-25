#include "game/game.h"

#include <stdio.h>

#include "raylib.h"
#include "raygui.h"

#include "screen/loading_menu.h"
#include "screen/pause_menu.h"
#include "screen/slot_picker.h"
#include "screen/confirm_quit.h"
#include "screen/settings_menu.h"
#include "audio/audio.h"
#include "save/save.h"
#include "settings/settings.h"
#include "ui/ui.h"
#include "util/assets.h"

#include "game/game_internal.h" // declarations for the handlers below (tests call them)

// Candidate locations for the config file, tried in order. Covers both running
// from the project root during development and the installed bin/assets layout.
static const char *kConfigPaths[] = {
    "assets/config.lua",
    "bin/assets/config.lua",
    "config.lua",
};

// Overlay any persisted audio settings onto the config defaults, so the player's
// last-chosen volumes/mute win over config.lua. Done before audio_init so the
// device starts at the right levels.
static void load_settings(Game *game)
{
    Settings s = {
        .music_volume = game->config.audio.music_volume,
        .sfx_volume = game->config.audio.sfx_volume,
        .muted = game->config.audio.muted,
    };
    if (settings_load(&s)) {
        game->config.audio.music_volume = s.music_volume;
        game->config.audio.sfx_volume = s.sfx_volume;
        game->config.audio.muted = s.muted;
        TraceLog(LOG_INFO, "SETTINGS: loaded persisted audio preferences");
    }
}

// Persist the current mixer state (called when leaving the settings screen).
static void save_settings(void)
{
    Settings s = {
        .music_volume = audio_music_volume(),
        .sfx_volume = audio_sfx_volume(),
        .muted = audio_muted(),
    };
    if (!settings_save(&s)) {
        TraceLog(LOG_WARNING, "SETTINGS: failed to write settings file");
    }
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
    if (!asset_resolve(style, path, sizeof(path))) {
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

// Load an up-to-date SDL_GameControllerDB so controllers whose GUID isn't in
// GLFW's bundled mapping table still register as gamepads. Without a matching
// mapping, glfwGetGamepadState() returns nothing and raylib reports zero button
// and axis input even though the pad is "available" (raylib #3651). MUST run
// after InitWindow — applying mappings before the GLFW window/joystick layer is
// up has no effect on macOS.
static void load_gamepad_mappings(void)
{
    char path[512];
    if (!asset_resolve("gamecontrollerdb.txt", path, sizeof(path))) {
        TraceLog(LOG_WARNING, "GAMEPAD: gamecontrollerdb.txt not found — "
                              "controllers may not register any input");
        return;
    }

    char *db = LoadFileText(path);
    if (db == NULL) {
        TraceLog(LOG_WARNING, "GAMEPAD: failed to read '%s'", path);
        return;
    }
    // Synchronous read + parse of the full SDL DB measures ~3 ms one-time here —
    // negligible against a 16 ms frame — so it stays inline rather than deferred:
    // lazy-loading risks a controller present at launch missing its mapping on
    // the first press.
    int ok = SetGamepadMappings(db); // 1 on success, 0 if GLFW rejected the DB
    UnloadFileText(db);
    if (ok == 1) {
        TraceLog(LOG_INFO, "GAMEPAD: applied mappings from '%s'", path);
    } else {
        TraceLog(LOG_WARNING, "GAMEPAD: '%s' was rejected (malformed DB?) — "
                              "controllers may not register any input", path);
    }
}

// Load the configured UI font (.ttf/.otf) and make raygui use it. Loaded
// ourselves because the .rgs cannot supply a usable font here. The atlas is
// rasterized at max(font_size, title_size) with bilinear filtering so both
// button text and the large title render smoothly. Must run after InitWindow.
static void load_ui_font(Game *game)
{
    const char *font = game->config.ui.font_file;
    int size = game->config.ui.font_size;
    GuiSetStyle(DEFAULT, TEXT_SIZE, size); // widget text size, even with built-in font

    if (font[0] == '\0') {
        return; // keep raygui's built-in font
    }

    char path[512];
    if (!asset_resolve(font, path, sizeof(path))) {
        TraceLog(LOG_WARNING, "FONT: '%s' not found — using built-in font", font);
        return;
    }

    // Rasterize at the largest size we draw so downscaling stays crisp.
    int atlas = size;
    if (game->config.ui.loading_menu.title_size > atlas) {
        atlas = game->config.ui.loading_menu.title_size;
    }
    Font f = LoadFontEx(path, atlas, NULL, 0);
    // On failure LoadFontEx returns the built-in default font; don't adopt (or
    // later unload) that as if it were ours.
    if (f.texture.id == 0 || f.texture.id == GetFontDefault().texture.id) {
        TraceLog(LOG_WARNING, "FONT: failed to load '%s' — using built-in font", path);
        return;
    }
    SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
    GuiSetFont(f);
    game->ui_font = f;
    game->ui_font_loaded = true; // remember to UnloadFont it at shutdown
    TraceLog(LOG_INFO, "FONT: loaded '%s' (atlas %dpx, text %dpx)", path, atlas, size);
}

void game_init(Game *game)
{
    load_config(game);
    load_settings(game); // persisted audio prefs override config.lua defaults

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
    game->ui_font_loaded = false;
    game->toast_text[0] = '\0';
    game->toast_until = 0.0;
    ui_menu_nav_reset(&game->nav);
    game->prev_screen = game->screen;
    game->prev_confirm = game->confirm_quit;
    game->settings_from = SCREEN_LOADING_MENU;

    InitWindow(game->screen_width, game->screen_height, game->config.window.title);
    SetTargetFPS(game->config.window.target_fps);
    SetExitKey(KEY_NULL); // Esc is our pause toggle, not a window-quit

    // Borderless fullscreen at the monitor's native resolution (no video-mode
    // change, alt-tab friendly). Adopt the resulting draw size so every screen's
    // centered layout fills the display instead of the initial windowed size.
    if (game->config.window.fullscreen) {
        ToggleBorderlessWindowed();
        game->screen_width = GetScreenWidth();
        game->screen_height = GetScreenHeight();
        // The window still needs pulling to the monitor's top-left (macOS drops
        // it below the menu bar), but game_update re-asserts that every frame, so
        // there's no separate placement fixup here.
    }

    load_gui_style(game);
    load_ui_font(game);
    load_gamepad_mappings(); // after InitWindow — see function comment
    audio_init(&game->config.audio); // opens the audio device; starts music
}

void game_update(Game *game)
{
    ui_input_poll();  // refresh controller state once per frame before nav queries
    audio_update();   // keep the music stream fed

    // Track the real drawable size every frame. Borderless-fullscreen resizes
    // apply a frame or two after the toggle on macOS, so reading it once at init
    // isn't enough — this keeps every screen's centered layout matched to the
    // actual window (and handles later resizes / monitor moves for free).
    game->screen_width = GetScreenWidth();
    game->screen_height = GetScreenHeight();

    // Keep the borderless window pinned to the monitor's top-left. macOS applies
    // the post-toggle placement a frame or two late and can re-drop the window
    // into the work area (below the menu bar); correct it whenever it drifts.
    // The guard makes this a no-op once the origin is right, so no per-frame fight.
    if (game->config.window.fullscreen) {
        Vector2 mpos = GetMonitorPosition(GetCurrentMonitor());
        Vector2 wpos = GetWindowPosition();
        if ((int)wpos.x != (int)mpos.x || (int)wpos.y != (int)mpos.y) {
            SetWindowPosition((int)mpos.x, (int)mpos.y);
        }
    }

    // The pause key toggles PLAYING <-> PAUSE. This one input spans two screens,
    // so it's resolved here (atomically, via if/else-if) rather than in a screen's
    // draw — otherwise the same Esc press that opened the overlay would be seen
    // again by pause_menu_draw and close it the same frame. Every *other* screen
    // reports its own Back/cancel through its returned action instead (including
    // the quit prompt, which owns Esc/B via CONFIRM_QUIT_CANCEL).
    if (game->screen == SCREEN_PLAYING) {
        if (ui_nav_menu_pressed()) {
            game->screen = SCREEN_PAUSE;
            game->confirm_quit = false;
        }
    } else if (game->screen == SCREEN_PAUSE) {
        // Resume only from the pause menu itself; while the quit prompt is up it
        // owns Esc/B (see confirm_quit_draw), so the toggle stands down.
        if (!game->confirm_quit && (ui_nav_menu_pressed() || ui_nav_cancel_pressed())) {
            game->screen = SCREEN_PLAYING;
        }
    }
    // Simulation updates land here as subsystems come online.
}

// Show a brief status message centered near the bottom of the screen. Used to
// surface outcomes the player would otherwise not see (failed save, no save to
// continue, unreadable slot).
static void game_toast(Game *game, const char *text)
{
    snprintf(game->toast_text, sizeof(game->toast_text), "%s", text);
    game->toast_until = GetTime() + 2.5;
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
        game_toast(game, "SAVE FAILED"); // don't let a failed write look like success
        TraceLog(LOG_WARNING, "SAVE: failed to write slot %d", game->active_slot);
    }
}

// Open the settings screen, remembering which screen to return to on Back.
static void enter_settings(Game *game, ScreenId from)
{
    game->settings_from = from;
    game->screen = SCREEN_SETTINGS;
}

// Handle an action emitted by the pause menu.
void handle_pause_action(Game *game, PauseMenuAction action)
{
    switch (action) {
        case PAUSE_RESUME:
            game->screen = SCREEN_PLAYING;
            break;
        case PAUSE_SAVE:
            save_game(game);
            break;
        case PAUSE_SETTINGS:
            enter_settings(game, SCREEN_PAUSE);
            break;
        case PAUSE_QUIT:
            game->confirm_quit = true; // ask whether to save before leaving
            break;
        case PAUSE_NONE:
            break;
    }
}

// Handle an action emitted by the settings screen: on Back, persist the mixer
// state and return to whichever screen opened settings.
void handle_settings_action(Game *game, SettingsMenuAction action)
{
    if (action == SETTINGS_MENU_BACK) {
        save_settings();
        game->screen = game->settings_from;
    }
}

// Handle an action emitted by the quit-confirmation modal. Both outcomes return
// to the loading menu; only the confirm path saves first.
void handle_confirm_quit(Game *game, ConfirmQuitAction action)
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
        case CONFIRM_QUIT_CANCEL:
            game->confirm_quit = false; // back out to the pause menu, staying in-game
            break;
        case CONFIRM_QUIT_NONE:
            break;
    }
}

// Enter the slot picker for the given purpose, refreshing its cached summaries.
static void enter_slot_picker(Game *game, SlotPickerMode mode);

// Handle an action emitted by the loading menu.
void handle_loading_menu_action(Game *game, LoadingMenuAction action)
{
    switch (action) {
        case LOADING_MENU_CONTINUE: {
            // Quick-load the most-recently-saved slot. `has_save` only checks that
            // a slot file exists, so a present-but-unreadable save can still land
            // here with nothing loadable — tell the player rather than no-op.
            int slot = save_most_recent_slot(game->config.save.slots);
            if (slot >= 0 && save_load_slot(&game->save, slot)) {
                game->active_slot = slot;
                game->screen = SCREEN_PLAYING;
                TraceLog(LOG_INFO, "CONTINUE: loaded slot %d", slot);
            } else {
                game_toast(game, "NO SAVE TO CONTINUE");
                TraceLog(LOG_WARNING, "CONTINUE: no loadable save found");
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
        case LOADING_MENU_SETTINGS:
            enter_settings(game, SCREEN_LOADING_MENU);
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
void handle_slot_picker(Game *game, int chosen)
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
            game_toast(game, "SAVE UNREADABLE");
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
    // Reset menu focus to the top whenever the visible menu changes — either a
    // new screen, or the pause menu toggling to/from the quit prompt (which have
    // different button counts). Done here, after game_update, so it catches every
    // transition regardless of which handler triggered it.
    if (game->screen != game->prev_screen || game->confirm_quit != game->prev_confirm) {
        ui_menu_nav_reset(&game->nav);
        game->prev_screen = game->screen;
        game->prev_confirm = game->confirm_quit;
    }

    BeginDrawing();
    // Background comes from the active raygui style, so the whole UI shares one
    // theme source (the .rgs); falls back to raygui's default when none is set.
    ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

    switch (game->screen) {
        case SCREEN_LOADING_MENU: {
            LoadingMenuAction action = loading_menu_draw(
                &game->nav, game->screen_width, game->screen_height, game->has_save,
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
                // Esc / gamepad-B backs out via CONFIRM_QUIT_CANCEL (below).
                ConfirmQuitAction action = confirm_quit_draw(
                    &game->nav, game->screen_width, game->screen_height,
                    &game->config.ui.confirm_quit, &game->config.ui.button);
                handle_confirm_quit(game, action);
            } else {
                bool recently_saved =
                    (GetTime() - game->last_save_time) <
                    game->config.ui.pause_menu.saved_notice_seconds;
                PauseMenuAction action = pause_menu_draw(
                    &game->nav, game->screen_width, game->screen_height,
                    &game->config.ui.pause_menu, &game->config.ui.button, recently_saved);
                handle_pause_action(game, action);
            }
            break;
        }
        case SCREEN_SLOT_PICKER: {
            int chosen = slot_picker_draw(&game->nav, game->screen_width, game->screen_height,
                                          game->picker_mode, &game->config.ui.slot_picker,
                                          &game->config.ui.button, game->slot_infos,
                                          game->slot_info_count);
            handle_slot_picker(game, chosen);
            break;
        }
        case SCREEN_SETTINGS: {
            SettingsMenuAction action =
                settings_menu_draw(&game->nav, game->screen_width, game->screen_height,
                                   &game->config.ui.settings_menu, &game->config.ui.button);
            handle_settings_action(game, action);
            break;
        }
    }

    // Transient status/error toast, drawn over whatever screen is active.
    if (game->toast_text[0] != '\0' && GetTime() < game->toast_until) {
        ui_text_centered(game->toast_text, 0.0f, (float)game->screen_width,
                         (float)game->screen_height - 80.0f, 22.0f,
                         (Color){ 220, 90, 70, 255 });
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
    audio_shutdown(); // release sounds/music and close the audio device
    if (game->ui_font_loaded) {
        UnloadFont(game->ui_font); // release the atlas texture we own
    }
    CloseWindow();
}
