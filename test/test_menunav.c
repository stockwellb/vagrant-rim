// Unit tests for the menu-focus traversal logic (ui_menu_nav_end). This is the
// pure part of MenuNav: given a latched move direction and a per-item enabled
// set, it advances focus, wrapping around the list and skipping disabled items,
// then snaps onto an enabled item. Headless — the audio "thud" on a focus move
// no-ops with no audio device, so we can drive the struct directly without a
// window.
//
// Note: a memset-zeroed MenuNav has every enabled[] flag false, so make_nav()
// sets the first `count` items enabled to model a normal menu.
#include "unity.h"

#include <string.h>

#include "ui/ui.h"

void setUp(void) {}
void tearDown(void) {}

// A nav for `count` items, all enabled, focus at 0, nothing latched.
static MenuNav make_nav(int count)
{
    MenuNav nav;
    memset(&nav, 0, sizeof(nav));
    nav.count = count;
    for (int i = 0; i < count && i < UI_MENU_MAX_ITEMS; i++) {
        nav.enabled[i] = true;
    }
    return nav;
}

// --- Basic movement ---------------------------------------------------------

static void test_move_down_within_range(void)
{
    MenuNav nav = make_nav(3);
    nav.focus = 0;
    nav.pending_dir = 1;
    ui_menu_nav_end(&nav);
    TEST_ASSERT_EQUAL_INT(1, nav.focus);
}

static void test_move_up_within_range(void)
{
    MenuNav nav = make_nav(3);
    nav.focus = 2;
    nav.pending_dir = -1;
    ui_menu_nav_end(&nav);
    TEST_ASSERT_EQUAL_INT(1, nav.focus);
}

static void test_no_direction_keeps_focus(void)
{
    MenuNav nav = make_nav(3);
    nav.focus = 1;
    nav.pending_dir = 0;
    ui_menu_nav_end(&nav);
    TEST_ASSERT_EQUAL_INT(1, nav.focus);
}

// --- Wrap-around ------------------------------------------------------------

static void test_wrap_down_last_to_first(void)
{
    MenuNav nav = make_nav(3);
    nav.focus = 2;
    nav.pending_dir = 1;
    ui_menu_nav_end(&nav);
    TEST_ASSERT_EQUAL_INT(0, nav.focus);
}

static void test_wrap_up_first_to_last(void)
{
    MenuNav nav = make_nav(3);
    nav.focus = 0;
    nav.pending_dir = -1;
    ui_menu_nav_end(&nav);
    TEST_ASSERT_EQUAL_INT(2, nav.focus);
}

// --- Skipping disabled items ------------------------------------------------

static void test_skips_single_disabled_item(void)
{
    MenuNav nav = make_nav(3);
    nav.enabled[1] = false; // {T, F, T}
    nav.focus = 0;
    nav.pending_dir = 1;
    ui_menu_nav_end(&nav);
    TEST_ASSERT_EQUAL_INT(2, nav.focus); // hops over 1
}

static void test_skips_consecutive_disabled_items(void)
{
    MenuNav nav = make_nav(4);
    nav.enabled[1] = false;
    nav.enabled[2] = false; // {T, F, F, T}
    nav.focus = 0;
    nav.pending_dir = 1;
    ui_menu_nav_end(&nav);
    TEST_ASSERT_EQUAL_INT(3, nav.focus);
}

static void test_skips_disabled_across_wrap(void)
{
    MenuNav nav = make_nav(3);
    nav.enabled[0] = false; // {F, T, T}
    nav.focus = 1;
    nav.pending_dir = -1;   // from 1, up: 0 is disabled, wrap to 2
    ui_menu_nav_end(&nav);
    TEST_ASSERT_EQUAL_INT(2, nav.focus);
}

// --- Snap onto an enabled item ---------------------------------------------

static void test_snaps_off_disabled_focus_when_idle(void)
{
    // Focus is parked on a now-disabled item with no move requested (e.g. the
    // enabled set changed under it): it should snap to the first enabled item.
    MenuNav nav = make_nav(3);
    nav.enabled[1] = false;
    nav.focus = 1;        // sitting on the disabled item
    nav.pending_dir = 0;  // no movement this frame
    ui_menu_nav_end(&nav);
    TEST_ASSERT_EQUAL_INT(0, nav.focus);
}

// --- Degenerate counts ------------------------------------------------------

static void test_zero_count_is_a_noop(void)
{
    MenuNav nav = make_nav(0);
    nav.focus = 5;       // stale index
    nav.pending_dir = 1;
    ui_menu_nav_end(&nav);
    TEST_ASSERT_EQUAL_INT(5, nav.focus); // untouched: nothing to navigate
}

// --- Adjust-as-focus fallback (VERTICAL menus with a plain focused row) ------

static void test_unconsumed_adjust_moves_focus(void)
{
    // In a vertical menu, left/right report as `adjust`. If the focused row is a
    // plain button (no slider consumed it), adjust falls back to a focus move.
    MenuNav nav = make_nav(3);
    nav.focus = 0;
    nav.pending_dir = 0;
    nav.adjust = 1;
    nav.adjust_consumed = false;
    ui_menu_nav_end(&nav);
    TEST_ASSERT_EQUAL_INT(1, nav.focus);
}

static void test_consumed_adjust_does_not_move_focus(void)
{
    // When a focused slider consumed the adjust, it must NOT also move focus.
    MenuNav nav = make_nav(3);
    nav.focus = 0;
    nav.pending_dir = 0;
    nav.adjust = 1;
    nav.adjust_consumed = true;
    ui_menu_nav_end(&nav);
    TEST_ASSERT_EQUAL_INT(0, nav.focus);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_move_down_within_range);
    RUN_TEST(test_move_up_within_range);
    RUN_TEST(test_no_direction_keeps_focus);
    RUN_TEST(test_wrap_down_last_to_first);
    RUN_TEST(test_wrap_up_first_to_last);
    RUN_TEST(test_skips_single_disabled_item);
    RUN_TEST(test_skips_consecutive_disabled_items);
    RUN_TEST(test_skips_disabled_across_wrap);
    RUN_TEST(test_snaps_off_disabled_focus_when_idle);
    RUN_TEST(test_zero_count_is_a_noop);
    RUN_TEST(test_unconsumed_adjust_moves_focus);
    RUN_TEST(test_consumed_adjust_does_not_move_focus);
    return UNITY_END();
}
