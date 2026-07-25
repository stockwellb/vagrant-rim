// Unit tests for the atomic temp-file + rename writer (util/atomic_file), the
// shared write path underneath both saves and settings. Verifies the happy path,
// atomic overwrite, that no ".tmp" sidecar is left behind on success, and the two
// failure guards (path too long to form a temp name; unwritable target dir) each
// return false without leaving a file. Headless — plain stdio, no window.
//
// main() chdir's into a throwaway temp dir so the test files are isolated.
#include "unity.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // chdir, mkdtemp

#include "util/atomic_file.h"

void setUp(void) {}
void tearDown(void) {}

// Read the whole file at `path` into `out` (NUL-terminated). Returns false if it
// couldn't be opened.
static bool read_file(const char *path, char *out, size_t out_size)
{
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        return false;
    }
    size_t n = fread(out, 1, out_size - 1, f);
    out[n] = '\0';
    fclose(f);
    return true;
}

// --- Happy path -------------------------------------------------------------

static void test_write_creates_file_with_exact_contents(void)
{
    TEST_ASSERT_TRUE(atomic_write("out.txt", "hello world\n"));

    char buf[64];
    TEST_ASSERT_TRUE(read_file("out.txt", buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("hello world\n", buf);
}

static void test_write_overwrites_existing_file(void)
{
    TEST_ASSERT_TRUE(atomic_write("ovr.txt", "first"));
    TEST_ASSERT_TRUE(atomic_write("ovr.txt", "second-and-longer"));

    char buf[64];
    TEST_ASSERT_TRUE(read_file("ovr.txt", buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("second-and-longer", buf);
}

static void test_success_leaves_no_tmp_sidecar(void)
{
    TEST_ASSERT_TRUE(atomic_write("clean.txt", "data"));

    // The "<path>.tmp" file must have been renamed away, not left behind.
    FILE *tmp = fopen("clean.txt.tmp", "r");
    TEST_ASSERT_NULL(tmp);
}

// --- Failure guards ---------------------------------------------------------

static void test_path_too_long_returns_false(void)
{
    // Longer than the writer's internal temp-name buffer, so forming "<path>.tmp"
    // overflows and the call must bail before touching the filesystem.
    char longpath[1100];
    memset(longpath, 'a', sizeof(longpath) - 1);
    longpath[sizeof(longpath) - 1] = '\0';

    TEST_ASSERT_FALSE(atomic_write(longpath, "data"));
}

static void test_unwritable_directory_returns_false(void)
{
    // Parent directory does not exist, so the temp file can't be opened.
    TEST_ASSERT_FALSE(atomic_write("no_such_dir/file.txt", "data"));

    // Nothing should have been created at the target path.
    FILE *f = fopen("no_such_dir/file.txt", "r");
    TEST_ASSERT_NULL(f);
}

int main(void)
{
    char tmpl[] = "/tmp/vagrant_rim_test_atomic_XXXXXX";
    char *dir = mkdtemp(tmpl);
    if (dir == NULL || chdir(dir) != 0) {
        fprintf(stderr, "test_atomic_file: could not create/enter temp dir\n");
        return 1;
    }

    UNITY_BEGIN();
    RUN_TEST(test_write_creates_file_with_exact_contents);
    RUN_TEST(test_write_overwrites_existing_file);
    RUN_TEST(test_success_leaves_no_tmp_sidecar);
    RUN_TEST(test_path_too_long_returns_false);
    RUN_TEST(test_unwritable_directory_returns_false);
    return UNITY_END();
}
