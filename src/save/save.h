#ifndef VR_SAVE_H
#define VR_SAVE_H

#include <stdbool.h>

// Bump when the on-disk save layout changes incompatibly.
#define SAVE_VERSION 1

// Hard upper bound on save slots (for fixed-size arrays); config picks the
// actual count within this.
#define SAVE_MAX_SLOTS 12

// A single game save. Persisted as a Lua file that returns a table (loaded the
// same way as config.lua). Fields are intentionally minimal for now and map to
// the GOALS.md economy; gameplay systems will grow this over time.
typedef struct GameSave {
    int version;
    long long created_at; // unix seconds when the save was first created
    long long saved_at;   // unix seconds of the most recent write

    int fuel;             // starts full, replenished at the mothership
    int resource_a;       // common -> rare crafting resources
    int resource_b;
    int resource_c;

    float player_x;       // ship position; starts docked at the mothership (0,0)
    float player_y;
} GameSave;

// Fill *save with fresh new-game state (stamps created_at/saved_at with now).
void save_new(GameSave *save);

// Current wall-clock time in unix seconds (for stamping saved_at before a write).
long long save_now(void);

// Write *save to a Lua file at `path`. The parent directory must already exist.
// Returns true on success.
bool save_write(const GameSave *save, const char *path);

// Load the save Lua file at `path` into *save. Returns true on success; on
// failure *save is left untouched.
bool save_load(GameSave *save, const char *path);

// Whether a readable save file exists at `path`.
bool save_exists(const char *path);

#endif // VR_SAVE_H
