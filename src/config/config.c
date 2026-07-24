#include "config/config.h"

#include <stdio.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "save/save.h" // SAVE_MAX_SLOTS

void config_set_defaults(Config *config)
{
    // Window
    config->window.width = 1280;
    config->window.height = 720;
    config->window.target_fps = 60;
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
    snprintf(config->ui.loading_menu.title_text, sizeof(config->ui.loading_menu.title_text), "%s", "VAGRANT RIM");
    config->ui.loading_menu.title_size = 72;
    snprintf(config->ui.loading_menu.tagline_text, sizeof(config->ui.loading_menu.tagline_text), "%s", "a space scavenger");
    config->ui.loading_menu.tagline_size = 20;

    // Debug
    config->debug.show_fps = false;

    // Save
    config->save.slots = 6;
}

// Read an integer field from the table on top of the stack into *out.
// Leaves *out untouched (keeping its default) if the key is missing or not a number.
static void read_int_field(lua_State *L, const char *key, int *out)
{
    lua_getfield(L, -1, key);
    if (lua_isnumber(L, -1)) {
        *out = (int)lua_tointeger(L, -1);
    }
    lua_pop(L, 1);
}

// Read a boolean field from the table on top of the stack into *out.
static void read_bool_field(lua_State *L, const char *key, bool *out)
{
    lua_getfield(L, -1, key);
    if (lua_isboolean(L, -1)) {
        *out = lua_toboolean(L, -1);
    }
    lua_pop(L, 1);
}

// Read a string field from the table on top of the stack into a fixed buffer.
static void read_string_field(lua_State *L, const char *key, char *out, int out_size)
{
    lua_getfield(L, -1, key);
    if (lua_isstring(L, -1)) {
        snprintf(out, (size_t)out_size, "%s", lua_tostring(L, -1));
    }
    lua_pop(L, 1);
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
        read_string_field(L, "style_file", ui->style_file, (int)sizeof(ui->style_file));
        read_string_field(L, "font_file", ui->font_file, (int)sizeof(ui->font_file));
        read_int_field(L, "font_size", &ui->font_size);

        // ui.button = { ... } — shared button layout
        lua_getfield(L, -1, "button");
        if (lua_istable(L, -1)) {
            read_int_field(L, "width", &ui->button.width);
            read_int_field(L, "height", &ui->button.height);
            read_int_field(L, "gap", &ui->button.gap);
        }
        lua_pop(L, 1); // button

        // ui.loading_menu = { ... }
        lua_getfield(L, -1, "loading_menu");
        if (lua_istable(L, -1)) {
            read_string_field(L, "title_text", ui->loading_menu.title_text, (int)sizeof(ui->loading_menu.title_text));
            read_int_field(L, "title_size", &ui->loading_menu.title_size);
            read_string_field(L, "tagline_text", ui->loading_menu.tagline_text, (int)sizeof(ui->loading_menu.tagline_text));
            read_int_field(L, "tagline_size", &ui->loading_menu.tagline_size);
        }
        lua_pop(L, 1); // loading_menu
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
        read_int_field(L, "width", &config->window.width);
        read_int_field(L, "height", &config->window.height);
        read_int_field(L, "target_fps", &config->window.target_fps);
        read_string_field(L, "title", config->window.title, (int)sizeof(config->window.title));
    }
    lua_pop(L, 1); // window

    // ui = { menu = { ... } }
    read_ui(L, &config->ui);

    // debug = { ... }
    lua_getfield(L, -1, "debug");
    if (lua_istable(L, -1)) {
        read_bool_field(L, "show_fps", &config->debug.show_fps);
    }
    lua_pop(L, 1); // debug

    // save = { ... }
    lua_getfield(L, -1, "save");
    if (lua_istable(L, -1)) {
        read_int_field(L, "slots", &config->save.slots);
    }
    lua_pop(L, 1); // save

    // Clamp slot count to a sane, array-safe range.
    if (config->save.slots < 1) {
        config->save.slots = 1;
    } else if (config->save.slots > SAVE_MAX_SLOTS) {
        config->save.slots = SAVE_MAX_SLOTS;
    }

    lua_close(L);
    return true;
}
