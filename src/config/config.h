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

// Shared button layout, from `ui.button` in config.lua. Reused by every menu.
// Appearance (colors, text size) comes from the raygui style (.rgs), not here;
// this struct holds only geometry the .rgs format can't express.
typedef struct ButtonConfig {
    int width;
    int height;
    int gap; // vertical spacing between stacked buttons
} ButtonConfig;

// Loading / title screen configuration, from `ui.loading_menu` in config.lua.
// This is one of many menus; it owns only its own title/tagline content and
// button labels.
typedef struct LoadingMenuConfig {
    char title_text[128];
    int title_size;
    char tagline_text[128];
    int tagline_size;
    char continue_text[32];
    char load_text[32];
    char new_text[32];
    char exit_text[32];
} LoadingMenuConfig;

// Pause overlay configuration, from `ui.pause_menu` in config.lua.
typedef struct PauseMenuConfig {
    char title_text[128];
    int title_size;
    char resume_text[32];
    char save_text[32];
    char quit_text[32];
    char saved_notice[32];        // brief confirmation shown next to Save after a write
    float saved_notice_seconds;   // how long that confirmation stays up
    int scrim_alpha;              // 0-255 dimming of the frozen scene behind the menu
} PauseMenuConfig;

// "Save before quitting?" modal configuration, from `ui.confirm_quit`.
typedef struct ConfirmQuitConfig {
    char title_text[128];
    int title_size;
    char message_text[128];
    int message_size;
    char confirm_text[32]; // YES: save then quit to menu
    char cancel_text[32];  // NO: quit to menu without saving
    int box_width;
    int box_height;
    int scrim_alpha; // 0-255 dimming of the frozen scene behind the modal
} ConfirmQuitConfig;

// Slot picker configuration, from `ui.slot_picker` in config.lua. Titles differ
// by mode (load vs. new); row geometry is a list, so it is separate from the
// shared button layout.
typedef struct SlotPickerConfig {
    char load_title[128]; // shown in load mode
    char new_title[128];  // shown in new-game mode
    int title_size;
    char back_text[32];
    char empty_text[32]; // label for an unused slot
    int row_width;
    int row_height;
    int row_gap;
} SlotPickerConfig;

// UI configuration, mirrored from the `ui` table in config.lua.
typedef struct UiConfig {
    // raygui style file (.rgs) authored in rGuiStyler, resolved against the
    // asset search paths. Empty string means use raygui's built-in default.
    // NOTE: .rgs supplies COLORS only in this toolchain, not a font.
    char style_file[256];
    // UI font (.ttf/.otf), resolved against the asset search paths. Empty string
    // keeps raygui's built-in font. Loaded ourselves because the .rgs cannot
    // supply a usable font here.
    char font_file[256];
    int font_size; // widget text size; also the font atlas base resolution
    ButtonConfig button;
    LoadingMenuConfig loading_menu;
    PauseMenuConfig pause_menu;
    SlotPickerConfig slot_picker;
    ConfirmQuitConfig confirm_quit;
} UiConfig;

// Developer/debug options, mirrored from the `debug` table in config.lua.
typedef struct DebugConfig {
    bool show_fps;
} DebugConfig;

// Save-system options, mirrored from the `save` table in config.lua.
typedef struct SaveConfig {
    int slots; // number of save slots (clamped to [1, SAVE_MAX_SLOTS])
} SaveConfig;

// Top-level game configuration loaded from Lua at startup. Visual theme (colors,
// widget text size) is owned by the raygui style (.rgs), read via GuiGetStyle at
// the draw site; it is intentionally not duplicated here.
typedef struct Config {
    WindowConfig window;
    UiConfig ui;
    DebugConfig debug;
    SaveConfig save;
} Config;

// Populate `config` with built-in defaults. Always succeeds.
void config_set_defaults(Config *config);

// Load configuration from the Lua file at `path`, overriding defaults for any
// keys present. Returns true on success. On failure, `config` is left holding
// defaults and an error is written to `err` (if non-NULL, up to `err_size`).
bool config_load(Config *config, const char *path, char *err, int err_size);

#endif // VR_CONFIG_H
