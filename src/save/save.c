#include "save/save.h"

#include <stdio.h>
#include <time.h>

#include "raylib.h" // DirectoryExists / MakeDirectory for the slot directory

#include "lua.h"
#include "lauxlib.h" // luaL_newstate / luaL_dofile (no lualib: saves run sandboxed)

#include "util/atomic_file.h" // shared temp-file + atomic-rename writer
#include "util/lua_util.h"    // shared Lua table field readers

// Save directory, resolved relative to the working directory (the project root
// during development via set_rundir).
static const char *save_dir = "saves";

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
    // Format the save, then hand it to the shared temp-file + atomic-rename
    // writer: an interrupted or failed write can't truncate or corrupt an
    // existing save — the old file stays intact until the complete new one
    // replaces it in one step.
    char buf[512];
    int n = snprintf(buf, sizeof(buf),
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
    if (n < 0 || (size_t)n >= sizeof(buf)) {
        return false; // formatted save didn't fit — refuse rather than truncate
    }

    return atomic_write(path, buf);
}

bool save_load(GameSave *save, const char *path)
{
    // Load saves in a Lua state with NO standard libraries opened. A save file is
    // pure data (`return { ... }` of numbers), so it needs none — and since saves
    // are the kind of file players share, this denies a hand-crafted save any
    // access to os/io and thus any path to code execution when we run it.
    lua_State *L = luaL_newstate();
    if (!L) {
        return false;
    }

    if (luaL_dofile(L, path) != LUA_OK || !lua_istable(L, -1)) {
        lua_close(L);
        return false;
    }

    GameSave tmp;
    tmp.version = SAVE_VERSION;
    lua_read_int_field(L, "version", &tmp.version);
    tmp.created_at = 0;
    lua_read_llong_field(L, "created_at", &tmp.created_at);
    tmp.saved_at = tmp.created_at; // default to created_at if absent
    lua_read_llong_field(L, "saved_at", &tmp.saved_at);
    tmp.fuel = 0;
    lua_read_int_field(L, "fuel", &tmp.fuel);

    tmp.resource_a = tmp.resource_b = tmp.resource_c = 0;
    lua_getfield(L, -1, "resources");
    if (lua_istable(L, -1)) {
        lua_read_int_field(L, "a", &tmp.resource_a);
        lua_read_int_field(L, "b", &tmp.resource_b);
        lua_read_int_field(L, "c", &tmp.resource_c);
    }
    lua_pop(L, 1); // resources

    tmp.player_x = tmp.player_y = 0.0f;
    lua_getfield(L, -1, "player");
    if (lua_istable(L, -1)) {
        lua_read_float_field(L, "x", &tmp.player_x);
        lua_read_float_field(L, "y", &tmp.player_y);
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

void save_slot_path(int slot, char *out, int out_size)
{
    snprintf(out, (size_t)out_size, "%s/slot%d.lua", save_dir, slot);
}

bool save_write_slot(const GameSave *save, int slot)
{
    if (!DirectoryExists(save_dir)) {
        MakeDirectory(save_dir);
    }
    char path[512];
    save_slot_path(slot, path, sizeof(path));
    return save_write(save, path);
}

bool save_load_slot(GameSave *save, int slot)
{
    char path[512];
    save_slot_path(slot, path, sizeof(path));
    return save_load(save, path);
}

bool save_any_slot_used(int slot_count)
{
    char path[512];
    for (int i = 0; i < slot_count; i++) {
        save_slot_path(i, path, sizeof(path));
        if (save_exists(path)) {
            return true;
        }
    }
    return false;
}

int save_most_recent_slot(int slot_count)
{
    char path[512];
    int best = -1;
    long long best_time = -1;
    for (int i = 0; i < slot_count; i++) {
        save_slot_path(i, path, sizeof(path));
        GameSave s;
        if (save_load(&s, path) && s.saved_at > best_time) {
            best_time = s.saved_at;
            best = i;
        }
    }
    return best;
}
