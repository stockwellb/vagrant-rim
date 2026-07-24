#include "save/save.h"

#include <stdio.h>
#include <time.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

long long save_now(void)
{
    return (long long)time(NULL);
}

void save_new(GameSave *save)
{
    save->version = SAVE_VERSION;
    save->created_at = save_now();
    save->saved_at = save->created_at;

    save->fuel = 100;
    save->resource_a = 0;
    save->resource_b = 0;
    save->resource_c = 0;

    save->player_x = 0.0f; // docked at the mothership
    save->player_y = 0.0f;
}

bool save_write(const GameSave *save, const char *path)
{
    FILE *f = fopen(path, "w");
    if (!f) {
        return false;
    }

    fprintf(f,
            "-- Vagrant Rim save file (generated)\n"
            "return {\n"
            "    version = %d,\n"
            "    created_at = %lld,\n"
            "    saved_at = %lld,\n"
            "    fuel = %d,\n"
            "    resources = { a = %d, b = %d, c = %d },\n"
            "    player = { x = %g, y = %g },\n"
            "}\n",
            save->version, save->created_at, save->saved_at, save->fuel,
            save->resource_a, save->resource_b, save->resource_c,
            (double)save->player_x, (double)save->player_y);

    fclose(f);
    return true;
}

// Read an integer field from the table on top of the stack, or `fallback`.
static int get_int(lua_State *L, const char *key, int fallback)
{
    int out = fallback;
    lua_getfield(L, -1, key);
    if (lua_isnumber(L, -1)) {
        out = (int)lua_tointeger(L, -1);
    }
    lua_pop(L, 1);
    return out;
}

static long long get_llong(lua_State *L, const char *key, long long fallback)
{
    long long out = fallback;
    lua_getfield(L, -1, key);
    if (lua_isnumber(L, -1)) {
        out = (long long)lua_tointeger(L, -1);
    }
    lua_pop(L, 1);
    return out;
}

static float get_float(lua_State *L, const char *key, float fallback)
{
    float out = fallback;
    lua_getfield(L, -1, key);
    if (lua_isnumber(L, -1)) {
        out = (float)lua_tonumber(L, -1);
    }
    lua_pop(L, 1);
    return out;
}

bool save_load(GameSave *save, const char *path)
{
    lua_State *L = luaL_newstate();
    if (!L) {
        return false;
    }
    luaL_openlibs(L);

    if (luaL_dofile(L, path) != LUA_OK || !lua_istable(L, -1)) {
        lua_close(L);
        return false;
    }

    GameSave tmp;
    tmp.version = get_int(L, "version", SAVE_VERSION);
    tmp.created_at = get_llong(L, "created_at", 0);
    tmp.saved_at = get_llong(L, "saved_at", tmp.created_at);
    tmp.fuel = get_int(L, "fuel", 0);

    tmp.resource_a = tmp.resource_b = tmp.resource_c = 0;
    lua_getfield(L, -1, "resources");
    if (lua_istable(L, -1)) {
        tmp.resource_a = get_int(L, "a", 0);
        tmp.resource_b = get_int(L, "b", 0);
        tmp.resource_c = get_int(L, "c", 0);
    }
    lua_pop(L, 1); // resources

    tmp.player_x = tmp.player_y = 0.0f;
    lua_getfield(L, -1, "player");
    if (lua_istable(L, -1)) {
        tmp.player_x = get_float(L, "x", 0.0f);
        tmp.player_y = get_float(L, "y", 0.0f);
    }
    lua_pop(L, 1); // player

    lua_close(L);
    *save = tmp;
    return true;
}

bool save_exists(const char *path)
{
    FILE *f = fopen(path, "r");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}
