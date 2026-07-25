// Unit tests for the shared numeric clamps (util/mathx). Pure functions, no
// window or device needed.
#include "unity.h"

#include "util/mathx.h"

void setUp(void) {}
void tearDown(void) {}

static void test_clampf_passes_value_within_range(void)
{
    TEST_ASSERT_EQUAL_FLOAT(0.5f, clampf(0.5f, 0.0f, 1.0f));
}

static void test_clampf_pins_to_bounds(void)
{
    TEST_ASSERT_EQUAL_FLOAT(0.0f, clampf(-3.0f, 0.0f, 1.0f)); // below low
    TEST_ASSERT_EQUAL_FLOAT(1.0f, clampf(9.0f, 0.0f, 1.0f));  // above high
    TEST_ASSERT_EQUAL_FLOAT(0.0f, clampf(0.0f, 0.0f, 1.0f));  // at low bound
    TEST_ASSERT_EQUAL_FLOAT(1.0f, clampf(1.0f, 0.0f, 1.0f));  // at high bound
}

static void test_clampi_pins_to_bounds(void)
{
    TEST_ASSERT_EQUAL_INT(1, clampi(0, 1, 12));    // below low
    TEST_ASSERT_EQUAL_INT(12, clampi(999, 1, 12)); // above high
    TEST_ASSERT_EQUAL_INT(6, clampi(6, 1, 12));    // within range
    TEST_ASSERT_EQUAL_INT(1, clampi(1, 1, 12));    // at low bound
    TEST_ASSERT_EQUAL_INT(12, clampi(12, 1, 12));  // at high bound
}

static void test_clamp01_normalizes_fractions(void)
{
    TEST_ASSERT_EQUAL_FLOAT(0.0f, clamp01(-0.5f));
    TEST_ASSERT_EQUAL_FLOAT(1.0f, clamp01(1.5f));
    TEST_ASSERT_EQUAL_FLOAT(0.3f, clamp01(0.3f));
}

static void test_clamp_unit_bounds_axes(void)
{
    TEST_ASSERT_EQUAL_FLOAT(-1.0f, clamp_unit(-2.0f));
    TEST_ASSERT_EQUAL_FLOAT(1.0f, clamp_unit(2.0f));
    TEST_ASSERT_EQUAL_FLOAT(-0.4f, clamp_unit(-0.4f));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_clampf_passes_value_within_range);
    RUN_TEST(test_clampf_pins_to_bounds);
    RUN_TEST(test_clampi_pins_to_bounds);
    RUN_TEST(test_clamp01_normalizes_fractions);
    RUN_TEST(test_clamp_unit_bounds_axes);
    return UNITY_END();
}
