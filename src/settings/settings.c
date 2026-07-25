#include "settings/settings.h"

#include <stdio.h>

#include "raylib.h" // DirectoryExists / MakeDirectory

#include "lua.h"
#include "lauxlib.h" // luaL_newstate / luaL_dofile (no lualib: loaded sandboxed)

#include "util/atomic_file.h" // shared temp-file + atomic-rename writer
#include "util/lua_util.h"    // shared Lua table field readers
#include "util/mathx.h"       // clamp01

// Settings live next to the save slots, relative to the working directory (the
// project root during development via set_rundir).
static const char *kSettingsDir = "saves";
static const char *kSettingsPath = "saves/settings.lua";

bool settings_load(Settings *s)
{
    // Loaded with no standard libraries, exactly like save files: pure data, and
    // hand-edited copies get no access to os/io and thus no code execution.
    lua_State *L = luaL_newstate();
    if (!L) {
        return false;
    }
    if (luaL_dofile(L, kSettingsPath) != LUA_OK || !lua_istable(L, -1)) {
        lua_close(L);
        return false;
    }

    lua_read_float_field(L, "music_volume", &s->music_volume);
    lua_read_float_field(L, "sfx_volume", &s->sfx_volume);
    lua_read_bool_field(L, "muted", &s->muted);
    lua_close(L);

    s->music_volume = clamp01(s->music_volume);
    s->sfx_volume = clamp01(s->sfx_volume);
    return true;
}

bool settings_save(const Settings *s)
{
    if (!DirectoryExists(kSettingsDir)) {
        MakeDirectory(kSettingsDir);
    }

    char buf[256];
    int n = snprintf(buf, sizeof(buf),
            "-- Vagrant Rim settings (generated)\n"
            "return {\n"
            "    music_volume = %g,\n"
            "    sfx_volume = %g,\n"
            "    muted = %s,\n"
            "}\n",
            (double)s->music_volume, (double)s->sfx_volume, s->muted ? "true" : "false");
    if (n < 0 || (size_t)n >= sizeof(buf)) {
        return false; // formatted settings didn't fit — refuse rather than truncate
    }

    // Temp file + atomic rename (shared with the save writer): an interrupted
    // write can't corrupt the existing settings.
    return atomic_write(kSettingsPath, buf);
}
