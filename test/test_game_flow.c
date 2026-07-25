// Flow tests for the game state machine (game.c's handle_* action handlers).
// These are the transitions the game loop drives every frame: which screen comes
// next, when a save is written, how the quit prompt resolves. The handlers are
// exposed via game_internal.h (test-only) so we can drive them directly on a
// stack Game, with no window — the only raylib touched is file I/O and GetTime
// (which returns 0 pre-window, harmless here since transitions don't read it).
//
// main() chdir's into a fresh temp dir so save/settings writes land in a
// throwaway "saves/" tree instead of the real project. A minimal Config is built
// per test (via base_config) — just the fields the handlers read.
#include "unity.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // chdir, mkdtemp

#include "game/game.h"
#include "game/game_internal.h"
#include "save/save.h"

void setUp(void) {}
void tearDown(void) {}

// Does slot N have a save file on disk? (Defined below main's helpers.)
static bool save_exists_slot(int slot);

// A zeroed Game with just the config fields the handlers depend on. Screen starts
// at a neutral non-target value so a missing transition is visible.
static Game make_game(void)
{
    Game g;
    memset(&g, 0, sizeof(g));
    g.config.save.slots = 6;
    // The slot picker's refresh labels empty slots with this text; give it a
    // value so enter_slot_picker (reached by NEW/LOAD) has something to copy.
    snprintf(g.config.ui.slot_picker.empty_text,
             sizeof(g.config.ui.slot_picker.empty_text), "- empty -");
    g.screen = SCREEN_PLAYING; // neutral starting point for most cases
    g.active_slot = 0;
    return g;
}

// Wipe the on-disk saves/ tree so a test starts from a known slot state. The CWD
// is a throwaway mkdtemp dir (see main), so this only touches test scratch. Tests
// that read or write slots call this first rather than relying on run order —
// otherwise slots written by an earlier test leak into a later one.
static void reset_saves(void)
{
    if (system("rm -rf saves") != 0) {
        // Nothing to remove on the first call is fine; a real failure surfaces as
        // a downstream slot assertion.
    }
}

// Write a valid save into `slot` with an explicit saved_at (so "most recent"
// ordering is deterministic instead of racing on the wall clock).
static void seed_slot(int slot, int fuel, long long saved_at)
{
    GameSave s;
    save_new(&s);
    s.fuel = fuel;
    s.saved_at = saved_at;
    TEST_ASSERT_TRUE(save_write_slot(&s, slot));
}

// --- Slot picker: NEW -------------------------------------------------------

static void test_new_game_writes_slot_and_enters_play(void)
{
    reset_saves();
    Game g = make_game();
    g.picker_mode = SLOT_PICKER_NEW;
    g.screen = SCREEN_SLOT_PICKER;

    handle_slot_picker(&g, 2); // start a new game in slot 2

    TEST_ASSERT_EQUAL_INT(SCREEN_PLAYING, g.screen);
    TEST_ASSERT_EQUAL_INT(2, g.active_slot);
    TEST_ASSERT_TRUE(g.has_save);

    // The new game must have actually been persisted to slot 2.
    GameSave loaded;
    TEST_ASSERT_TRUE(save_load_slot(&loaded, 2));
}

// --- Slot picker: LOAD ------------------------------------------------------

static void test_load_existing_slot_enters_play_with_save(void)
{
    reset_saves();
    seed_slot(3, 55, save_now());

    Game g = make_game();
    g.picker_mode = SLOT_PICKER_LOAD;
    g.screen = SCREEN_SLOT_PICKER;

    handle_slot_picker(&g, 3);

    TEST_ASSERT_EQUAL_INT(SCREEN_PLAYING, g.screen);
    TEST_ASSERT_EQUAL_INT(3, g.active_slot);
    TEST_ASSERT_EQUAL_INT(55, g.save.fuel); // the loaded save is now active
}

static void test_load_unreadable_slot_toasts_and_stays(void)
{
    reset_saves();
    Game g = make_game();
    g.picker_mode = SLOT_PICKER_LOAD;
    g.screen = SCREEN_SLOT_PICKER;

    handle_slot_picker(&g, 5); // slot 5 was never written

    TEST_ASSERT_EQUAL_INT(SCREEN_SLOT_PICKER, g.screen); // no transition to play
    TEST_ASSERT_TRUE(g.toast_text[0] != '\0');           // player is told
}

// --- Slot picker: sentinels -------------------------------------------------

static void test_slot_picker_back_returns_to_loading_menu(void)
{
    Game g = make_game();
    g.screen = SCREEN_SLOT_PICKER;
    handle_slot_picker(&g, SLOT_PICKER_BACK);
    TEST_ASSERT_EQUAL_INT(SCREEN_LOADING_MENU, g.screen);
}

static void test_slot_picker_none_is_a_noop(void)
{
    Game g = make_game();
    g.screen = SCREEN_SLOT_PICKER;
    handle_slot_picker(&g, SLOT_PICKER_NONE);
    TEST_ASSERT_EQUAL_INT(SCREEN_SLOT_PICKER, g.screen); // unchanged
}

// --- Loading menu -----------------------------------------------------------

static void test_continue_with_no_save_toasts(void)
{
    reset_saves();
    Game g = make_game();
    g.screen = SCREEN_LOADING_MENU;
    handle_loading_menu_action(&g, LOADING_MENU_CONTINUE);
    TEST_ASSERT_NOT_EQUAL(SCREEN_PLAYING, g.screen);
    TEST_ASSERT_TRUE(g.toast_text[0] != '\0');
}

static void test_continue_loads_most_recent_slot(void)
{
    reset_saves();
    seed_slot(0, 10, 100);
    seed_slot(2, 15, 200);
    seed_slot(1, 20, 300); // newest saved_at => the slot Continue picks

    Game g = make_game();
    g.screen = SCREEN_LOADING_MENU;
    handle_loading_menu_action(&g, LOADING_MENU_CONTINUE);

    TEST_ASSERT_EQUAL_INT(SCREEN_PLAYING, g.screen);
    TEST_ASSERT_EQUAL_INT(1, g.active_slot);
    TEST_ASSERT_EQUAL_INT(20, g.save.fuel);
}

static void test_loading_menu_new_opens_picker_in_new_mode(void)
{
    Game g = make_game();
    g.screen = SCREEN_LOADING_MENU;
    handle_loading_menu_action(&g, LOADING_MENU_NEW);
    TEST_ASSERT_EQUAL_INT(SCREEN_SLOT_PICKER, g.screen);
    TEST_ASSERT_EQUAL_INT(SLOT_PICKER_NEW, g.picker_mode);
}

static void test_loading_menu_load_opens_picker_in_load_mode(void)
{
    Game g = make_game();
    g.screen = SCREEN_LOADING_MENU;
    handle_loading_menu_action(&g, LOADING_MENU_LOAD);
    TEST_ASSERT_EQUAL_INT(SCREEN_SLOT_PICKER, g.screen);
    TEST_ASSERT_EQUAL_INT(SLOT_PICKER_LOAD, g.picker_mode);
}

static void test_loading_menu_settings_records_origin(void)
{
    Game g = make_game();
    g.screen = SCREEN_LOADING_MENU;
    handle_loading_menu_action(&g, LOADING_MENU_SETTINGS);
    TEST_ASSERT_EQUAL_INT(SCREEN_SETTINGS, g.screen);
    TEST_ASSERT_EQUAL_INT(SCREEN_LOADING_MENU, g.settings_from);
}

static void test_loading_menu_exit_requests_quit(void)
{
    Game g = make_game();
    g.screen = SCREEN_LOADING_MENU;
    handle_loading_menu_action(&g, LOADING_MENU_EXIT);
    TEST_ASSERT_TRUE(g.should_quit);
}

// --- Pause menu -------------------------------------------------------------

static void test_pause_resume_returns_to_play(void)
{
    Game g = make_game();
    g.screen = SCREEN_PAUSE;
    handle_pause_action(&g, PAUSE_RESUME);
    TEST_ASSERT_EQUAL_INT(SCREEN_PLAYING, g.screen);
}

static void test_pause_settings_records_pause_origin(void)
{
    Game g = make_game();
    g.screen = SCREEN_PAUSE;
    handle_pause_action(&g, PAUSE_SETTINGS);
    TEST_ASSERT_EQUAL_INT(SCREEN_SETTINGS, g.screen);
    TEST_ASSERT_EQUAL_INT(SCREEN_PAUSE, g.settings_from);
}

static void test_pause_quit_raises_confirm_prompt(void)
{
    Game g = make_game();
    g.screen = SCREEN_PAUSE;
    g.confirm_quit = false;
    handle_pause_action(&g, PAUSE_QUIT);
    TEST_ASSERT_TRUE(g.confirm_quit); // asks before leaving, doesn't quit outright
}

static void test_pause_save_writes_active_slot(void)
{
    Game g = make_game();
    g.active_slot = 4;
    save_new(&g.save);
    g.save.fuel = 88;

    handle_pause_action(&g, PAUSE_SAVE);

    TEST_ASSERT_TRUE(g.has_save);
    GameSave loaded;
    TEST_ASSERT_TRUE(save_load_slot(&loaded, 4));
    TEST_ASSERT_EQUAL_INT(88, loaded.fuel);
}

// --- Confirm-quit modal -----------------------------------------------------

static void test_confirm_quit_save_persists_then_exits_to_menu(void)
{
    Game g = make_game();
    g.screen = SCREEN_PAUSE;
    g.confirm_quit = true;
    g.active_slot = 1;
    save_new(&g.save);
    g.save.fuel = 33;

    handle_confirm_quit(&g, CONFIRM_QUIT_SAVE);

    TEST_ASSERT_FALSE(g.confirm_quit);
    TEST_ASSERT_EQUAL_INT(SCREEN_LOADING_MENU, g.screen);
    GameSave loaded;
    TEST_ASSERT_TRUE(save_load_slot(&loaded, 1)); // it saved before leaving
    TEST_ASSERT_EQUAL_INT(33, loaded.fuel);
}

static void test_confirm_quit_discard_exits_without_saving(void)
{
    reset_saves(); // so slot 5 is known-absent regardless of earlier tests
    Game g = make_game();
    g.screen = SCREEN_PAUSE;
    g.confirm_quit = true;
    g.active_slot = 5; // never written

    handle_confirm_quit(&g, CONFIRM_QUIT_DISCARD);

    TEST_ASSERT_FALSE(g.confirm_quit);
    TEST_ASSERT_EQUAL_INT(SCREEN_LOADING_MENU, g.screen);
    TEST_ASSERT_FALSE(save_exists_slot(5)); // no save was written on discard
}

static void test_confirm_quit_cancel_stays_in_pause(void)
{
    Game g = make_game();
    g.screen = SCREEN_PAUSE;
    g.confirm_quit = true;

    handle_confirm_quit(&g, CONFIRM_QUIT_CANCEL);

    TEST_ASSERT_FALSE(g.confirm_quit);              // prompt dismissed
    TEST_ASSERT_EQUAL_INT(SCREEN_PAUSE, g.screen);  // but still in-game (paused)
}

// --- Settings screen --------------------------------------------------------

static void test_settings_back_returns_to_origin(void)
{
    Game g = make_game();
    g.screen = SCREEN_SETTINGS;
    g.settings_from = SCREEN_PAUSE;
    handle_settings_action(&g, SETTINGS_MENU_BACK);
    TEST_ASSERT_EQUAL_INT(SCREEN_PAUSE, g.screen); // back to whoever opened settings
}

// Helper used by the discard test: does slot N have a save file on disk?
static bool save_exists_slot(int slot)
{
    char path[256];
    save_slot_path(slot, path, sizeof(path));
    return save_exists(path);
}

int main(void)
{
    char tmpl[] = "/tmp/vagrant_rim_flow_XXXXXX";
    char *dir = mkdtemp(tmpl);
    if (dir == NULL || chdir(dir) != 0) {
        fprintf(stderr, "test_game_flow: could not create/enter temp dir\n");
        return 1;
    }

    UNITY_BEGIN();
    // Slot picker
    RUN_TEST(test_new_game_writes_slot_and_enters_play);
    RUN_TEST(test_load_existing_slot_enters_play_with_save);
    RUN_TEST(test_load_unreadable_slot_toasts_and_stays);
    RUN_TEST(test_slot_picker_back_returns_to_loading_menu);
    RUN_TEST(test_slot_picker_none_is_a_noop);
    // Loading menu
    RUN_TEST(test_continue_with_no_save_toasts);
    RUN_TEST(test_continue_loads_most_recent_slot);
    RUN_TEST(test_loading_menu_new_opens_picker_in_new_mode);
    RUN_TEST(test_loading_menu_load_opens_picker_in_load_mode);
    RUN_TEST(test_loading_menu_settings_records_origin);
    RUN_TEST(test_loading_menu_exit_requests_quit);
    // Pause menu
    RUN_TEST(test_pause_resume_returns_to_play);
    RUN_TEST(test_pause_settings_records_pause_origin);
    RUN_TEST(test_pause_quit_raises_confirm_prompt);
    RUN_TEST(test_pause_save_writes_active_slot);
    // Confirm-quit modal
    RUN_TEST(test_confirm_quit_save_persists_then_exits_to_menu);
    RUN_TEST(test_confirm_quit_discard_exits_without_saving);
    RUN_TEST(test_confirm_quit_cancel_stays_in_pause);
    // Settings
    RUN_TEST(test_settings_back_returns_to_origin);
    return UNITY_END();
}
