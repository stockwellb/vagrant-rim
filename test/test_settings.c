// Unit tests for persisted player preferences (settings/settings). Covers the
// save/load round-trip, load-side volume clamping, the "absent fields keep the
// incoming value" override semantics, and the missing-file failure path.
// Headless — uses Lua (bundled with the core lib); no window needed.
//
// settings_load/save read and write "saves/settings.lua" relative to the working
// directory, so main() chdir's into a throwaway temp dir first. Order matters:
// the missing-file check runs before any save creates the file.
#include "unity.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h> // mkdir
#include <unistd.h>   // chdir, mkdtemp

#include "settings/settings.h"

void setUp(void) {}
void tearDown(void) {}

// Write `contents` straight to the settings file, creating saves/ if needed.
// Used by tests that need precise on-disk contents (partial fields, bad values).
static void write_settings_file(const char *contents)
{
    mkdir("saves", 0777); // harmless if it already exists
    FILE *f = fopen("saves/settings.lua", "w");
    TEST_ASSERT_NOT_NULL_MESSAGE(f, "could not open saves/settings.lua for writing");
    fputs(contents, f);
    fclose(f);
}

// --- Missing file (must run before anything writes the file) ----------------

static void test_load_missing_returns_false_and_leaves_untouched(void)
{
    Settings s = { .music_volume = 0.33f, .sfx_volume = 0.66f, .muted = true };
    TEST_ASSERT_FALSE(settings_load(&s)); // no saves/settings.lua exists yet
    TEST_ASSERT_EQUAL_FLOAT(0.33f, s.music_volume); // all untouched on failure
    TEST_ASSERT_EQUAL_FLOAT(0.66f, s.sfx_volume);
    TEST_ASSERT_TRUE(s.muted);
}

// --- Round-trip -------------------------------------------------------------

static void test_roundtrip_preserves_values_unmuted(void)
{
    Settings in = { .music_volume = 0.25f, .sfx_volume = 0.75f, .muted = false };
    TEST_ASSERT_TRUE(settings_save(&in));

    Settings out = { 0 };
    TEST_ASSERT_TRUE(settings_load(&out));
    TEST_ASSERT_EQUAL_FLOAT(0.25f, out.music_volume);
    TEST_ASSERT_EQUAL_FLOAT(0.75f, out.sfx_volume);
    TEST_ASSERT_FALSE(out.muted);
}

static void test_roundtrip_preserves_muted_true(void)
{
    Settings in = { .music_volume = 0.5f, .sfx_volume = 0.5f, .muted = true };
    TEST_ASSERT_TRUE(settings_save(&in));

    Settings out = { 0 };
    TEST_ASSERT_TRUE(settings_load(&out));
    TEST_ASSERT_TRUE(out.muted); // the true/false formatting round-trips
}

// --- Load-side clamping -----------------------------------------------------

static void test_out_of_range_volumes_clamped_on_load(void)
{
    write_settings_file("return { music_volume = 5.0, sfx_volume = -1.0, muted = false }\n");

    Settings s = { .music_volume = 0.5f, .sfx_volume = 0.5f, .muted = true };
    TEST_ASSERT_TRUE(settings_load(&s));
    TEST_ASSERT_EQUAL_FLOAT(1.0f, s.music_volume); // clamped down to 1
    TEST_ASSERT_EQUAL_FLOAT(0.0f, s.sfx_volume);   // clamped up to 0
    TEST_ASSERT_FALSE(s.muted);
}

// --- Override semantics -----------------------------------------------------

static void test_absent_fields_keep_incoming_value(void)
{
    write_settings_file("return { muted = true }\n"); // volumes intentionally absent

    Settings s = { .music_volume = 0.42f, .sfx_volume = 0.11f, .muted = false };
    TEST_ASSERT_TRUE(settings_load(&s));
    TEST_ASSERT_EQUAL_FLOAT(0.42f, s.music_volume); // absent on disk → kept
    TEST_ASSERT_EQUAL_FLOAT(0.11f, s.sfx_volume);   // absent on disk → kept
    TEST_ASSERT_TRUE(s.muted);                      // present on disk → overridden
}

int main(void)
{
    char tmpl[] = "/tmp/vagrant_rim_test_settings_XXXXXX";
    char *dir = mkdtemp(tmpl);
    if (dir == NULL || chdir(dir) != 0) {
        fprintf(stderr, "test_settings: could not create/enter temp dir\n");
        return 1;
    }

    UNITY_BEGIN();
    RUN_TEST(test_load_missing_returns_false_and_leaves_untouched); // before any write
    RUN_TEST(test_roundtrip_preserves_values_unmuted);
    RUN_TEST(test_roundtrip_preserves_muted_true);
    RUN_TEST(test_out_of_range_volumes_clamped_on_load);
    RUN_TEST(test_absent_fields_keep_incoming_value);
    return UNITY_END();
}
