// Unit tests for the save system (save/save). Exercises the on-disk data path:
// the low-level write/load round-trip, failure handling, and slot scanning.
// Headless — no window; uses Lua (bundled with the core lib) and raylib's file
// helpers, none of which need a GL context.
//
// main() chdir's into a fresh temp directory before running, so the slot-based
// tests write their "saves/" tree there instead of polluting the real project
// saves. Each slot test wipes that tree first (reset_saves), so they're
// self-contained and independent of run order.
#include "unity.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // chdir, mkdtemp

#include "save/save.h"

void setUp(void) {}
void tearDown(void) {}

// Wipe the on-disk saves/ tree so a slot test starts from a known empty state.
// The CWD is a throwaway mkdtemp dir (see main), so this only touches scratch.
static void reset_saves(void)
{
    (void)system("rm -rf saves");
}

// --- Low-level path API (CWD-independent) -----------------------------------

static void test_roundtrip_preserves_all_fields(void)
{
    GameSave in;
    save_new(&in);
    in.fuel = 73;
    in.resource_a = 1;
    in.resource_b = 22;
    in.resource_c = 333;
    in.player_x = 12.5f;
    in.player_y = -8.25f;
    in.saved_at = 1700000000LL;

    TEST_ASSERT_TRUE(save_write(&in, "roundtrip.lua"));

    GameSave out;
    memset(&out, 0, sizeof(out));
    TEST_ASSERT_TRUE(save_load(&out, "roundtrip.lua"));

    TEST_ASSERT_EQUAL_INT(in.version, out.version);
    TEST_ASSERT_EQUAL_INT64(in.created_at, out.created_at);
    TEST_ASSERT_EQUAL_INT64(in.saved_at, out.saved_at);
    TEST_ASSERT_EQUAL_INT(in.fuel, out.fuel);
    TEST_ASSERT_EQUAL_INT(in.resource_a, out.resource_a);
    TEST_ASSERT_EQUAL_INT(in.resource_b, out.resource_b);
    TEST_ASSERT_EQUAL_INT(in.resource_c, out.resource_c);
    TEST_ASSERT_EQUAL_FLOAT(in.player_x, out.player_x);
    TEST_ASSERT_EQUAL_FLOAT(in.player_y, out.player_y);
}

static void test_load_missing_file_returns_false(void)
{
    GameSave s;
    TEST_ASSERT_FALSE(save_load(&s, "no_such_file.lua"));
}

static void test_load_corrupt_file_leaves_out_untouched(void)
{
    FILE *f = fopen("corrupt.lua", "w");
    TEST_ASSERT_NOT_NULL(f);
    fputs("this is not a lua table = = =\n", f); // a syntax error, not data
    fclose(f);

    GameSave s;
    save_new(&s);
    s.fuel = 999; // sentinel: must survive a failed load
    TEST_ASSERT_FALSE(save_load(&s, "corrupt.lua"));
    TEST_ASSERT_EQUAL_INT(999, s.fuel);
}

static void test_load_non_table_returns_false(void)
{
    FILE *f = fopen("scalar.lua", "w");
    TEST_ASSERT_NOT_NULL(f);
    fputs("return 42\n", f); // valid Lua, but not a table
    fclose(f);

    GameSave s;
    TEST_ASSERT_FALSE(save_load(&s, "scalar.lua"));
}

// --- Slot API (writes a "saves/" tree under the temp CWD) -------------------

static void test_most_recent_slot_empty_returns_negative_one(void)
{
    reset_saves(); // no save files exist
    TEST_ASSERT_EQUAL_INT(-1, save_most_recent_slot(6));
}

static void test_any_slot_used_empty_is_false(void)
{
    reset_saves();
    TEST_ASSERT_FALSE(save_any_slot_used(6));
}

static void test_most_recent_slot_picks_newest_saved_at(void)
{
    reset_saves();
    GameSave s;
    save_new(&s);

    s.saved_at = 100;
    TEST_ASSERT_TRUE(save_write_slot(&s, 0));
    s.saved_at = 300; // newest
    TEST_ASSERT_TRUE(save_write_slot(&s, 1));
    s.saved_at = 200;
    TEST_ASSERT_TRUE(save_write_slot(&s, 2));

    TEST_ASSERT_EQUAL_INT(1, save_most_recent_slot(6));
}

static void test_any_slot_used_true_after_write(void)
{
    reset_saves();
    GameSave s;
    save_new(&s);
    TEST_ASSERT_TRUE(save_write_slot(&s, 0));
    TEST_ASSERT_TRUE(save_any_slot_used(6));
}

int main(void)
{
    // Sandbox all file I/O in a throwaway temp directory.
    char tmpl[] = "/tmp/vagrant_rim_test_XXXXXX";
    char *dir = mkdtemp(tmpl);
    if (dir == NULL || chdir(dir) != 0) {
        fprintf(stderr, "test_save: could not create/enter temp dir\n");
        return 1;
    }

    UNITY_BEGIN();
    // Empty-state checks (each resets saves/ first).
    RUN_TEST(test_most_recent_slot_empty_returns_negative_one);
    RUN_TEST(test_any_slot_used_empty_is_false);
    // Low-level round-trip and failure handling.
    RUN_TEST(test_roundtrip_preserves_all_fields);
    RUN_TEST(test_load_missing_file_returns_false);
    RUN_TEST(test_load_corrupt_file_leaves_out_untouched);
    RUN_TEST(test_load_non_table_returns_false);
    // Slot scanning (each resets + writes its own saves/ under the temp CWD).
    RUN_TEST(test_most_recent_slot_picks_newest_saved_at);
    RUN_TEST(test_any_slot_used_true_after_write);
    return UNITY_END();
}
