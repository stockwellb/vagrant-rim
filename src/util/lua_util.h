#ifndef VR_UTIL_LUA_UTIL_H
#define VR_UTIL_LUA_UTIL_H

#include <stdbool.h>

// Forward-declared so includers don't have to pull in the Lua headers.
typedef struct lua_State lua_State;

// Typed readers for a field of the Lua table currently on top of the stack.
// Each leaves *out untouched — preserving its prior/default value — when the key
// is missing or is not of the expected type, and keeps the stack balanced. This
// "override defaults in place" idiom is shared by config and save loading.
void lua_read_int_field(lua_State *L, const char *key, int *out);
void lua_read_llong_field(lua_State *L, const char *key, long long *out);
void lua_read_float_field(lua_State *L, const char *key, float *out);
void lua_read_bool_field(lua_State *L, const char *key, bool *out);

// Copies a string field into the fixed buffer `out` (capacity `out_size`);
// leaves `out` untouched when the key is missing or not a string.
void lua_read_string_field(lua_State *L, const char *key, char *out, int out_size);

#endif // VR_UTIL_LUA_UTIL_H
