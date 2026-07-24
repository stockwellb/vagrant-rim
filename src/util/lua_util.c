#include "util/lua_util.h"

#include <stdio.h>

#include "lua.h"

void lua_read_int_field(lua_State *L, const char *key, int *out)
{
    lua_getfield(L, -1, key);
    if (lua_isnumber(L, -1)) {
        *out = (int)lua_tointeger(L, -1);
    }
    lua_pop(L, 1);
}

void lua_read_llong_field(lua_State *L, const char *key, long long *out)
{
    lua_getfield(L, -1, key);
    if (lua_isnumber(L, -1)) {
        *out = (long long)lua_tointeger(L, -1);
    }
    lua_pop(L, 1);
}

void lua_read_float_field(lua_State *L, const char *key, float *out)
{
    lua_getfield(L, -1, key);
    if (lua_isnumber(L, -1)) {
        *out = (float)lua_tonumber(L, -1);
    }
    lua_pop(L, 1);
}

void lua_read_bool_field(lua_State *L, const char *key, bool *out)
{
    lua_getfield(L, -1, key);
    if (lua_isboolean(L, -1)) {
        *out = lua_toboolean(L, -1);
    }
    lua_pop(L, 1);
}

void lua_read_string_field(lua_State *L, const char *key, char *out, int out_size)
{
    lua_getfield(L, -1, key);
    if (lua_isstring(L, -1)) {
        snprintf(out, (size_t)out_size, "%s", lua_tostring(L, -1));
    }
    lua_pop(L, 1);
}
