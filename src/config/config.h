#ifndef VR_CONFIG_H
#define VR_CONFIG_H

#include <stdbool.h>

// Window configuration, mirrored from the `window` table in config.lua.
typedef struct WindowConfig {
    int width;
    int height;
    char title[128];
    int target_fps;
} WindowConfig;

// Top-level game configuration loaded from Lua at startup.
typedef struct Config {
    WindowConfig window;
} Config;

// Populate `config` with built-in defaults. Always succeeds.
void config_set_defaults(Config *config);

// Load configuration from the Lua file at `path`, overriding defaults for any
// keys present. Returns true on success. On failure, `config` is left holding
// defaults and an error is written to `err` (if non-NULL, up to `err_size`).
bool config_load(Config *config, const char *path, char *err, int err_size);

#endif // VR_CONFIG_H
