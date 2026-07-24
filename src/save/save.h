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

// --- Slot addressing --------------------------------------------------------
// The save module owns both the on-disk format (above) and the slot naming /
// location convention: slots live at "saves/slot<N>.lua" relative to the working
// directory. Callers supply the slot count (owned by config), not the module.

// Build the on-disk path for `slot` into `out`.
void save_slot_path(int slot, char *out, int out_size);

// Write *save to `slot`, creating the save directory if needed. Stamp
// save->saved_at before calling. Returns true on success.
bool save_write_slot(const GameSave *save, int slot);

// Load `slot` into *save. Returns true on success; on failure *save is untouched.
bool save_load_slot(GameSave *save, int slot);

// Whether any of slots [0, slot_count) exists on disk (gates Continue/Load).
bool save_any_slot_used(int slot_count);

// Index of the slot with the newest saved_at in [0, slot_count), or -1 if none.
int save_most_recent_slot(int slot_count);

// --- Low-level path API (the slot API above wraps these) ---------------------

// Write *save to a Lua file at `path`. The parent directory must already exist.
// Returns true on success.
bool save_write(const GameSave *save, const char *path);

// Load the save Lua file at `path` into *save. Returns true on success; on
// failure *save is left untouched.
bool save_load(GameSave *save, const char *path);

// Whether a readable save file exists at `path`.
bool save_exists(const char *path);

#endif // VR_SAVE_H
