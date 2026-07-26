// Unit tests for config loading + clamping (config/config). config_load reads a
// Lua table into typed structs, overriding defaults only for keys present, then
// clamps everything that feeds raylib/font APIs into safe ranges so a malformed
// config can't crash the window or blow up the font atlas. Headless — uses Lua
// (bundled with the core lib); no window needed.
//
// Each test writes a small Lua file to a fixed temp path and loads it.
#include "unity.h"

#include <stdio.h>
#include <string.h>

#include "config/config.h"
#include "save/save.h" // SAVE_MAX_SLOTS

static const char *tmp_cfg = "/tmp/vagrant_rim_test_config.lua";

void setUp(void) {}
void tearDown(void) {}

// Write `contents` to the temp config path; fail the test if it can't be written.
static void write_config(const char *contents)
{
    FILE *f = fopen(tmp_cfg, "w");
    TEST_ASSERT_NOT_NULL_MESSAGE(f, "could not open temp config for writing");
    fputs(contents, f);
    fclose(f);
}

// --- Defaults & override semantics ------------------------------------------

static void test_defaults_are_sane(void)
{
    Config c;
    config_set_defaults(&c);
    TEST_ASSERT_EQUAL_INT(1280, c.window.width);
    TEST_ASSERT_EQUAL_INT(720, c.window.height);
    TEST_ASSERT_EQUAL_INT(60, c.window.target_fps);
    TEST_ASSERT_EQUAL_INT(6, c.save.slots);
}

static void test_empty_table_keeps_defaults(void)
{
    write_config("return {}\n");
    Config c;
    char err[128] = {0};
    TEST_ASSERT_TRUE(config_load(&c, tmp_cfg, err, sizeof(err)));
    TEST_ASSERT_EQUAL_INT(1280, c.window.width); // untouched default
    TEST_ASSERT_EQUAL_INT(60, c.window.target_fps);
}

static void test_present_keys_override_only_themselves(void)
{
    write_config("return { window = { width = 800, title = \"Test Title\" } }\n");
    Config c;
    char err[128] = {0};
    TEST_ASSERT_TRUE(config_load(&c, tmp_cfg, err, sizeof(err)));
    TEST_ASSERT_EQUAL_INT(800, c.window.width);            // overridden
    TEST_ASSERT_EQUAL_STRING("Test Title", c.window.title); // overridden
    TEST_ASSERT_EQUAL_INT(720, c.window.height);           // still default
    TEST_ASSERT_EQUAL_INT(60, c.window.target_fps);        // still default
}

// --- Clamping ---------------------------------------------------------------

static void test_out_of_range_values_are_clamped(void)
{
    write_config(
        "return {\n"
        "  window = { width = 10, height = 999999 },\n"
        "  ui = { font_size = 9999, pause_menu = { scrim_alpha = 9999 } },\n"
        "  audio = { music_volume = 5.0, sfx_volume = -1.0 },\n"
        "  save = { slots = 999 },\n"
        "}\n");
    Config c;
    char err[128] = {0};
    TEST_ASSERT_TRUE(config_load(&c, tmp_cfg, err, sizeof(err)));

    TEST_ASSERT_EQUAL_INT(320, c.window.width);   // clamped up to min
    TEST_ASSERT_EQUAL_INT(4320, c.window.height); // clamped down to max
    TEST_ASSERT_EQUAL_INT(256, c.ui.font_size);   // font atlas cap
    TEST_ASSERT_EQUAL_INT(255, c.ui.pause_menu.scrim_alpha); // 0..255
    TEST_ASSERT_EQUAL_FLOAT(1.0f, c.audio.music_volume);     // 0..1
    TEST_ASSERT_EQUAL_FLOAT(0.0f, c.audio.sfx_volume);       // 0..1
    TEST_ASSERT_EQUAL_INT(SAVE_MAX_SLOTS, c.save.slots);     // array-safe cap
}

static void test_slots_clamped_up_to_one(void)
{
    write_config("return { save = { slots = 0 } }\n");
    Config c;
    char err[128] = {0};
    TEST_ASSERT_TRUE(config_load(&c, tmp_cfg, err, sizeof(err)));
    TEST_ASSERT_EQUAL_INT(1, c.save.slots); // at least one slot
}

// --- Failure handling -------------------------------------------------------

static void test_missing_file_returns_false_with_defaults(void)
{
    Config c;
    char err[128] = {0};
    TEST_ASSERT_FALSE(config_load(&c, "/tmp/vr_no_such_config.lua", err, sizeof(err)));
    TEST_ASSERT_EQUAL_INT(1280, c.window.width); // left holding defaults
}

static void test_non_table_return_is_false(void)
{
    write_config("return 42\n"); // valid Lua, but not a table
    Config c;
    char err[128] = {0};
    TEST_ASSERT_FALSE(config_load(&c, tmp_cfg, err, sizeof(err)));
    TEST_ASSERT_EQUAL_INT(1280, c.window.width); // defaults preserved
}

static void test_syntax_error_reports_message(void)
{
    write_config("return { this is not valid lua =\n");
    Config c;
    char err[128] = {0};
    TEST_ASSERT_FALSE(config_load(&c, tmp_cfg, err, sizeof(err)));
    TEST_ASSERT_TRUE(err[0] != '\0'); // an error message was written
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_defaults_are_sane);
    RUN_TEST(test_empty_table_keeps_defaults);
    RUN_TEST(test_present_keys_override_only_themselves);
    RUN_TEST(test_out_of_range_values_are_clamped);
    RUN_TEST(test_slots_clamped_up_to_one);
    RUN_TEST(test_missing_file_returns_false_with_defaults);
    RUN_TEST(test_non_table_return_is_false);
    RUN_TEST(test_syntax_error_reports_message);
    return UNITY_END();
}
