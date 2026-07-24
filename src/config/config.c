#include "config/config.h"

#include <stdio.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

void config_set_defaults(Config *config)
{
    config->window.width = 1280;
    config->window.height = 720;
    config->window.target_fps = 60;
    snprintf(config->window.title, sizeof(config->window.title), "%s", "Vagrant Rim");
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

    lua_close(L);
    return true;
}
