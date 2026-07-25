// Unit tests for the typed Lua field readers (util/lua_util). Each reader pulls
// one field off the table on top of the stack and, crucially, leaves *out
// untouched when the key is missing or the wrong type — the "override defaults in
// place" idiom config and save loading both rely on. Headless: pure Lua, no
// raylib, no window.
//
// Every test builds a fresh table, leaves it at the top of the stack (index -1,
// where the readers expect it), and closes the state in tearDown.
#include "unity.h"

#include "lua.h"
#include "lauxlib.h"

#include "util/lua_util.h"

static lua_State *L;

void setUp(void)
{
    L = luaL_newstate();
    lua_newtable(L); // the table under test, left at the top of the stack
}

void tearDown(void)
{
    lua_close(L);
    L = NULL;
}

// Helpers to set a field on the table currently at the top of the stack.
static void set_int(const char *k, lua_Integer v)
{
    lua_pushinteger(L, v);
    lua_setfield(L, -2, k);
}
static void set_num(const char *k, lua_Number v)
{
    lua_pushnumber(L, v);
    lua_setfield(L, -2, k);
}
static void set_str(const char *k, const char *v)
{
    lua_pushstring(L, v);
    lua_setfield(L, -2, k);
}
static void set_bool(const char *k, int v)
{
    lua_pushboolean(L, v);
    lua_setfield(L, -2, k);
}

// --- Correct reads ----------------------------------------------------------

static void test_int_reads_value(void)
{
    set_int("fuel", 73);
    int out = 0;
    lua_read_int_field(L, "fuel", &out);
    TEST_ASSERT_EQUAL_INT(73, out);
}

static void test_llong_reads_value(void)
{
    set_int("saved_at", 1700000000LL);
    long long out = 0;
    lua_read_llong_field(L, "saved_at", &out);
    TEST_ASSERT_EQUAL_INT64(1700000000LL, out);
}

static void test_float_reads_value(void)
{
    set_num("volume", 0.25);
    float out = 0.0f;
    lua_read_float_field(L, "volume", &out);
    TEST_ASSERT_EQUAL_FLOAT(0.25f, out);
}

static void test_bool_reads_true(void)
{
    set_bool("muted", 1);
    bool out = false;
    lua_read_bool_field(L, "muted", &out);
    TEST_ASSERT_TRUE(out);
}

static void test_string_reads_value(void)
{
    set_str("title", "Vagrant Rim");
    char out[32] = "";
    lua_read_string_field(L, "title", out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING("Vagrant Rim", out);
}

// --- Missing key leaves the default in place --------------------------------

static void test_int_missing_key_keeps_default(void)
{
    int out = 999; // sentinel default
    lua_read_int_field(L, "absent", &out);
    TEST_ASSERT_EQUAL_INT(999, out);
}

static void test_bool_missing_key_keeps_default(void)
{
    bool out = true;
    lua_read_bool_field(L, "absent", &out);
    TEST_ASSERT_TRUE(out); // untouched
}

static void test_string_missing_key_keeps_default(void)
{
    char out[32] = "fallback";
    lua_read_string_field(L, "absent", out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING("fallback", out);
}

// --- Wrong type leaves the default in place ---------------------------------

static void test_int_wrong_type_keeps_default(void)
{
    set_str("fuel", "not a number");
    int out = 42;
    lua_read_int_field(L, "fuel", &out);
    TEST_ASSERT_EQUAL_INT(42, out);
}

static void test_float_wrong_type_keeps_default(void)
{
    set_bool("volume", 1);
    float out = 0.5f;
    lua_read_float_field(L, "volume", &out);
    TEST_ASSERT_EQUAL_FLOAT(0.5f, out);
}

static void test_bool_wrong_type_keeps_default(void)
{
    // A number is not a boolean to lua_isboolean, so the reader must no-op.
    set_int("muted", 1);
    bool out = false;
    lua_read_bool_field(L, "muted", &out);
    TEST_ASSERT_FALSE(out);
}

static void test_string_wrong_type_keeps_default(void)
{
    set_bool("title", 1);
    char out[32] = "keep me";
    lua_read_string_field(L, "title", out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING("keep me", out);
}

// --- String truncation ------------------------------------------------------

static void test_string_truncates_to_buffer(void)
{
    set_str("title", "0123456789ABCDEF"); // 16 chars
    char out[8] = "";                     // room for 7 chars + NUL
    lua_read_string_field(L, "title", out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING("0123456", out);
}

// --- Stack balance ----------------------------------------------------------

static void test_readers_keep_stack_balanced(void)
{
    // Each reader pushes the field then pops it; the table must remain the sole
    // stack entry regardless of hit or miss.
    set_int("fuel", 1);
    int i = 0;
    float f = 0.0f;
    char s[8] = "";
    lua_read_int_field(L, "fuel", &i);
    lua_read_int_field(L, "absent", &i);
    lua_read_float_field(L, "absent", &f);
    lua_read_string_field(L, "absent", s, sizeof(s));
    TEST_ASSERT_EQUAL_INT(1, lua_gettop(L)); // only the table remains
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_int_reads_value);
    RUN_TEST(test_llong_reads_value);
    RUN_TEST(test_float_reads_value);
    RUN_TEST(test_bool_reads_true);
    RUN_TEST(test_string_reads_value);
    RUN_TEST(test_int_missing_key_keeps_default);
    RUN_TEST(test_bool_missing_key_keeps_default);
    RUN_TEST(test_string_missing_key_keeps_default);
    RUN_TEST(test_int_wrong_type_keeps_default);
    RUN_TEST(test_float_wrong_type_keeps_default);
    RUN_TEST(test_bool_wrong_type_keeps_default);
    RUN_TEST(test_string_wrong_type_keeps_default);
    RUN_TEST(test_string_truncates_to_buffer);
    RUN_TEST(test_readers_keep_stack_balanced);
    return UNITY_END();
}
