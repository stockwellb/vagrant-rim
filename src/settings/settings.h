#ifndef VR_SETTINGS_SETTINGS_H
#define VR_SETTINGS_SETTINGS_H

#include <stdbool.h>

// Persisted player preferences — currently just the audio mixer. Stored as a Lua
// data file alongside the save slots and loaded the same sandboxed way, so it
// survives across runs and overrides the config.lua audio defaults at startup.
typedef struct Settings {
    float music_volume; // 0..1
    float sfx_volume;   // 0..1
    bool muted;
} Settings;

// Load persisted settings into *s, overriding any field present on disk (fields
// absent from the file keep their incoming value, so seed *s with defaults
// first). Returns true if a settings file was read; on absence or parse failure
// *s is left untouched.
bool settings_load(Settings *s);

// Persist *s atomically (temp file + rename). Returns true on success.
bool settings_save(const Settings *s);

#endif // VR_SETTINGS_SETTINGS_H
