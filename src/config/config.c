#include "config/config.h"

#include <stdio.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "util/lua_util.h" // shared Lua table field readers
#include "util/mathx.h"    // clampi / clampf
#include "save/save.h"     // SAVE_MAX_SLOTS

void config_set_defaults(Config *config)
{
    // Window
    config->window.width = 1280;
    config->window.height = 720;
    config->window.target_fps = 60;
    config->window.fullscreen = true;
    snprintf(config->window.title, sizeof(config->window.title), "%s", "Vagrant Rim");

    // UI: raygui style (colors) and font — empty means use built-in defaults
    config->ui.style_file[0] = '\0';
    config->ui.font_file[0] = '\0';
    config->ui.font_size = 28;

    // UI: shared button layout
    config->ui.button.width = 240;
    config->ui.button.height = 48;
    config->ui.button.gap = 14;

    // UI: loading / title menu
    LoadingMenuConfig *lm = &config->ui.loading_menu;
    snprintf(lm->title_text, sizeof(lm->title_text), "%s", "VAGRANT RIM");
    lm->title_size = 72;
    snprintf(lm->tagline_text, sizeof(lm->tagline_text), "%s", "a space scavenger");
    lm->tagline_size = 20;
    snprintf(lm->continue_text, sizeof(lm->continue_text), "%s", "CONTINUE");
    snprintf(lm->load_text, sizeof(lm->load_text), "%s", "LOAD");
    snprintf(lm->new_text, sizeof(lm->new_text), "%s", "NEW GAME");
    snprintf(lm->settings_text, sizeof(lm->settings_text), "%s", "SETTINGS");
    snprintf(lm->exit_text, sizeof(lm->exit_text), "%s", "EXIT");

    // UI: pause overlay
    PauseMenuConfig *pm = &config->ui.pause_menu;
    snprintf(pm->title_text, sizeof(pm->title_text), "%s", "PAUSED");
    pm->title_size = 48;
    snprintf(pm->resume_text, sizeof(pm->resume_text), "%s", "RESUME");
    snprintf(pm->save_text, sizeof(pm->save_text), "%s", "SAVE");
    snprintf(pm->settings_text, sizeof(pm->settings_text), "%s", "SETTINGS");
    snprintf(pm->quit_text, sizeof(pm->quit_text), "%s", "QUIT TO MENU");
    snprintf(pm->saved_notice, sizeof(pm->saved_notice), "%s", "saved");
    pm->saved_notice_seconds = 2.0f;
    pm->scrim_alpha = 180;

    // UI: slot picker
    SlotPickerConfig *sp = &config->ui.slot_picker;
    snprintf(sp->load_title, sizeof(sp->load_title), "%s", "LOAD GAME");
    snprintf(sp->new_title, sizeof(sp->new_title), "%s", "NEW GAME");
    sp->title_size = 48;
    snprintf(sp->back_text, sizeof(sp->back_text), "%s", "BACK");
    snprintf(sp->empty_text, sizeof(sp->empty_text), "%s", "- empty -");
    sp->row_width = 700;
    sp->row_height = 48;
    sp->row_gap = 10;

    // UI: quit-confirmation modal
    ConfirmQuitConfig *cq = &config->ui.confirm_quit;
    snprintf(cq->title_text, sizeof(cq->title_text), "%s", "QUIT TO MENU");
    cq->title_size = 28;
    snprintf(cq->message_text, sizeof(cq->message_text), "%s", "Save before quitting?");
    cq->message_size = 22;
    snprintf(cq->confirm_text, sizeof(cq->confirm_text), "%s", "YES");
    snprintf(cq->cancel_text, sizeof(cq->cancel_text), "%s", "NO");
    cq->box_width = 560;
    cq->box_height = 240;
    cq->scrim_alpha = 180;

    // UI: settings screen
    SettingsMenuConfig *st = &config->ui.settings_menu;
    snprintf(st->title_text, sizeof(st->title_text), "%s", "SETTINGS");
    st->title_size = 48;
    snprintf(st->music_label, sizeof(st->music_label), "%s", "MUSIC");
    snprintf(st->sfx_label, sizeof(st->sfx_label), "%s", "SFX");
    snprintf(st->mute_label, sizeof(st->mute_label), "%s", "MUTE");
    snprintf(st->mute_on_text, sizeof(st->mute_on_text), "%s", "ON");
    snprintf(st->mute_off_text, sizeof(st->mute_off_text), "%s", "OFF");
    snprintf(st->back_text, sizeof(st->back_text), "%s", "BACK");

    // Audio
    AudioConfig *au = &config->audio;
    snprintf(au->music_file, sizeof(au->music_file), "%s", "audio/music.ogg");
    snprintf(au->nav_sfx_file, sizeof(au->nav_sfx_file), "%s", "audio/focus.ogg");
    snprintf(au->select_sfx_file, sizeof(au->select_sfx_file), "%s", "audio/select.ogg");
    au->music_volume = 0.5f;
    au->sfx_volume = 0.7f;
    au->muted = false;

    // Debug
    config->debug.show_fps = false;

    // Save
    config->save.slots = 6;
}

// Force values that feed raylib/font APIs into safe ranges, so a malformed
// config can't crash the window, blow up the font atlas, or wrap a scrim alpha.
static void clamp_config(Config *config)
{
    config->window.width = clampi(config->window.width, 320, 7680);
    config->window.height = clampi(config->window.height, 240, 4320);
    config->window.target_fps = clampi(config->window.target_fps, 1, 1000);

    config->ui.font_size = clampi(config->ui.font_size, 8, 256);
    config->ui.loading_menu.title_size = clampi(config->ui.loading_menu.title_size, 1, 512);
    config->ui.loading_menu.tagline_size = clampi(config->ui.loading_menu.tagline_size, 1, 512);
    config->ui.pause_menu.title_size = clampi(config->ui.pause_menu.title_size, 1, 512);
    config->ui.slot_picker.title_size = clampi(config->ui.slot_picker.title_size, 1, 512);
    config->ui.confirm_quit.title_size = clampi(config->ui.confirm_quit.title_size, 1, 512);
    config->ui.confirm_quit.message_size = clampi(config->ui.confirm_quit.message_size, 1, 512);
    config->ui.settings_menu.title_size = clampi(config->ui.settings_menu.title_size, 1, 512);

    // Scrim alphas are cast to unsigned char at the draw site; keep them in range
    // so an out-of-bounds value can't wrap to a near-transparent scrim.
    config->ui.pause_menu.scrim_alpha = clampi(config->ui.pause_menu.scrim_alpha, 0, 255);
    config->ui.confirm_quit.scrim_alpha = clampi(config->ui.confirm_quit.scrim_alpha, 0, 255);

    // Slot count must stay array-safe.
    config->save.slots = clampi(config->save.slots, 1, SAVE_MAX_SLOTS);

    // Volumes feed raylib's Set*Volume, which expects a 0..1 linear gain.
    config->audio.music_volume = clampf(config->audio.music_volume, 0.0f, 1.0f);
    config->audio.sfx_volume = clampf(config->audio.sfx_volume, 0.0f, 1.0f);
}

static void fail(char *err, int err_size, const char *msg)
{
    if (err && err_size > 0) {
        snprintf(err, (size_t)err_size, "%s", msg);
    }
}

// Read the `ui` table and its subtables. Expects the root config table on top
// of the stack.
static void read_ui(lua_State *L, UiConfig *ui)
{
    lua_getfield(L, -1, "ui");
    if (lua_istable(L, -1)) {
        lua_read_string_field(L, "style_file", ui->style_file, (int)sizeof(ui->style_file));
        lua_read_string_field(L, "font_file", ui->font_file, (int)sizeof(ui->font_file));
        lua_read_int_field(L, "font_size", &ui->font_size);

        // ui.button = { ... } — shared button layout
        lua_getfield(L, -1, "button");
        if (lua_istable(L, -1)) {
            lua_read_int_field(L, "width", &ui->button.width);
            lua_read_int_field(L, "height", &ui->button.height);
            lua_read_int_field(L, "gap", &ui->button.gap);
        }
        lua_pop(L, 1); // button

        // ui.loading_menu = { ... }
        lua_getfield(L, -1, "loading_menu");
        if (lua_istable(L, -1)) {
            LoadingMenuConfig *lm = &ui->loading_menu;
            lua_read_string_field(L, "title_text", lm->title_text, (int)sizeof(lm->title_text));
            lua_read_int_field(L, "title_size", &lm->title_size);
            lua_read_string_field(L, "tagline_text", lm->tagline_text, (int)sizeof(lm->tagline_text));
            lua_read_int_field(L, "tagline_size", &lm->tagline_size);
            lua_read_string_field(L, "continue_text", lm->continue_text, (int)sizeof(lm->continue_text));
            lua_read_string_field(L, "load_text", lm->load_text, (int)sizeof(lm->load_text));
            lua_read_string_field(L, "new_text", lm->new_text, (int)sizeof(lm->new_text));
            lua_read_string_field(L, "settings_text", lm->settings_text, (int)sizeof(lm->settings_text));
            lua_read_string_field(L, "exit_text", lm->exit_text, (int)sizeof(lm->exit_text));
        }
        lua_pop(L, 1); // loading_menu

        // ui.pause_menu = { ... }
        lua_getfield(L, -1, "pause_menu");
        if (lua_istable(L, -1)) {
            PauseMenuConfig *pm = &ui->pause_menu;
            lua_read_string_field(L, "title_text", pm->title_text, (int)sizeof(pm->title_text));
            lua_read_int_field(L, "title_size", &pm->title_size);
            lua_read_string_field(L, "resume_text", pm->resume_text, (int)sizeof(pm->resume_text));
            lua_read_string_field(L, "save_text", pm->save_text, (int)sizeof(pm->save_text));
            lua_read_string_field(L, "settings_text", pm->settings_text, (int)sizeof(pm->settings_text));
            lua_read_string_field(L, "quit_text", pm->quit_text, (int)sizeof(pm->quit_text));
            lua_read_string_field(L, "saved_notice", pm->saved_notice, (int)sizeof(pm->saved_notice));
            lua_read_float_field(L, "saved_notice_seconds", &pm->saved_notice_seconds);
            lua_read_int_field(L, "scrim_alpha", &pm->scrim_alpha);
        }
        lua_pop(L, 1); // pause_menu

        // ui.slot_picker = { ... }
        lua_getfield(L, -1, "slot_picker");
        if (lua_istable(L, -1)) {
            SlotPickerConfig *sp = &ui->slot_picker;
            lua_read_string_field(L, "load_title", sp->load_title, (int)sizeof(sp->load_title));
            lua_read_string_field(L, "new_title", sp->new_title, (int)sizeof(sp->new_title));
            lua_read_int_field(L, "title_size", &sp->title_size);
            lua_read_string_field(L, "back_text", sp->back_text, (int)sizeof(sp->back_text));
            lua_read_string_field(L, "empty_text", sp->empty_text, (int)sizeof(sp->empty_text));
            lua_read_int_field(L, "row_width", &sp->row_width);
            lua_read_int_field(L, "row_height", &sp->row_height);
            lua_read_int_field(L, "row_gap", &sp->row_gap);
        }
        lua_pop(L, 1); // slot_picker

        // ui.confirm_quit = { ... }
        lua_getfield(L, -1, "confirm_quit");
        if (lua_istable(L, -1)) {
            ConfirmQuitConfig *cq = &ui->confirm_quit;
            lua_read_string_field(L, "title_text", cq->title_text, (int)sizeof(cq->title_text));
            lua_read_int_field(L, "title_size", &cq->title_size);
            lua_read_string_field(L, "message_text", cq->message_text, (int)sizeof(cq->message_text));
            lua_read_int_field(L, "message_size", &cq->message_size);
            lua_read_string_field(L, "confirm_text", cq->confirm_text, (int)sizeof(cq->confirm_text));
            lua_read_string_field(L, "cancel_text", cq->cancel_text, (int)sizeof(cq->cancel_text));
            lua_read_int_field(L, "box_width", &cq->box_width);
            lua_read_int_field(L, "box_height", &cq->box_height);
            lua_read_int_field(L, "scrim_alpha", &cq->scrim_alpha);
        }
        lua_pop(L, 1); // confirm_quit

        // ui.settings_menu = { ... }
        lua_getfield(L, -1, "settings_menu");
        if (lua_istable(L, -1)) {
            SettingsMenuConfig *st = &ui->settings_menu;
            lua_read_string_field(L, "title_text", st->title_text, (int)sizeof(st->title_text));
            lua_read_int_field(L, "title_size", &st->title_size);
            lua_read_string_field(L, "music_label", st->music_label, (int)sizeof(st->music_label));
            lua_read_string_field(L, "sfx_label", st->sfx_label, (int)sizeof(st->sfx_label));
            lua_read_string_field(L, "mute_label", st->mute_label, (int)sizeof(st->mute_label));
            lua_read_string_field(L, "mute_on_text", st->mute_on_text, (int)sizeof(st->mute_on_text));
            lua_read_string_field(L, "mute_off_text", st->mute_off_text, (int)sizeof(st->mute_off_text));
            lua_read_string_field(L, "back_text", st->back_text, (int)sizeof(st->back_text));
        }
        lua_pop(L, 1); // settings_menu
    }
    lua_pop(L, 1); // ui
}

bool config_load(Config *config, const char *path, char *err, int err_size)
{
    config_set_defaults(config);

    lua_State *L = luaL_newstate();
    if (!L) {
        fail(err, err_size, "failed to create Lua state");
        return false;
    }
    luaL_openlibs(L);

    // Run the config file; it is expected to return a table.
    if (luaL_dofile(L, path) != LUA_OK) {
        fail(err, err_size, lua_tostring(L, -1));
        lua_close(L);
        return false;
    }

    if (!lua_istable(L, -1)) {
        fail(err, err_size, "config file did not return a table");
        lua_close(L);
        return false;
    }

    // window = { ... }
    lua_getfield(L, -1, "window");
    if (lua_istable(L, -1)) {
        lua_read_int_field(L, "width", &config->window.width);
        lua_read_int_field(L, "height", &config->window.height);
        lua_read_int_field(L, "target_fps", &config->window.target_fps);
        lua_read_string_field(L, "title", config->window.title, (int)sizeof(config->window.title));
        lua_read_bool_field(L, "fullscreen", &config->window.fullscreen);
    }
    lua_pop(L, 1); // window

    // ui = { menu = { ... } }
    read_ui(L, &config->ui);

    // audio = { ... }
    lua_getfield(L, -1, "audio");
    if (lua_istable(L, -1)) {
        AudioConfig *au = &config->audio;
        lua_read_string_field(L, "music_file", au->music_file, (int)sizeof(au->music_file));
        lua_read_string_field(L, "nav_sfx_file", au->nav_sfx_file, (int)sizeof(au->nav_sfx_file));
        lua_read_string_field(L, "select_sfx_file", au->select_sfx_file,
                              (int)sizeof(au->select_sfx_file));
        lua_read_float_field(L, "music_volume", &au->music_volume);
        lua_read_float_field(L, "sfx_volume", &au->sfx_volume);
        lua_read_bool_field(L, "muted", &au->muted);
    }
    lua_pop(L, 1); // audio

    // debug = { ... }
    lua_getfield(L, -1, "debug");
    if (lua_istable(L, -1)) {
        lua_read_bool_field(L, "show_fps", &config->debug.show_fps);
    }
    lua_pop(L, 1); // debug

    // save = { ... }
    lua_getfield(L, -1, "save");
    if (lua_istable(L, -1)) {
        lua_read_int_field(L, "slots", &config->save.slots);
    }
    lua_pop(L, 1); // save

    clamp_config(config);

    lua_close(L);
    return true;
}
