// Unit tests for asset path resolution (util/assets). asset_resolve tries a
// fixed list of prefixes — "assets/" (dev tree), "bin/assets/" (installed), then
// "" (already-qualified path) — and returns the first that exists on disk. Only
// raylib's FileExists is touched, which needs no window, so this runs headless.
//
// main() chdir's into a throwaway temp dir and builds an assets/ and bin/assets/
// tree there, so the tests probe real files without depending on the project's
// own asset layout.
#include "unity.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> // mkdir
#include <unistd.h>   // chdir

#include "util/assets.h"

void setUp(void) {}
void tearDown(void) {}

static void write_file(const char *path)
{
    FILE *f = fopen(path, "w");
    if (f) {
        fputs("x", f);
        fclose(f);
    }
}

static void test_resolves_under_assets_prefix(void)
{
    write_file("assets/music.ogg");
    char out[256] = "";
    TEST_ASSERT_TRUE(asset_resolve("music.ogg", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("assets/music.ogg", out);
}

static void test_resolves_under_bin_assets_prefix(void)
{
    // Present only under the installed layout, not assets/.
    write_file("bin/assets/font.ttf");
    char out[256] = "";
    TEST_ASSERT_TRUE(asset_resolve("font.ttf", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("bin/assets/font.ttf", out);
}

static void test_empty_prefix_passes_through_qualified_path(void)
{
    // A path that exists as-is (no asset prefix) resolves via the "" prefix.
    write_file("already_qualified.txt");
    char out[256] = "";
    TEST_ASSERT_TRUE(asset_resolve("already_qualified.txt", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("already_qualified.txt", out);
}

static void test_assets_prefix_wins_over_bin_assets(void)
{
    // When the same relative path exists under both, the dev-tree "assets/" is
    // tried first and wins.
    write_file("assets/dup.rgs");
    write_file("bin/assets/dup.rgs");
    char out[256] = "";
    TEST_ASSERT_TRUE(asset_resolve("dup.rgs", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("assets/dup.rgs", out);
}

static void test_missing_file_returns_false(void)
{
    char out[256] = "sentinel";
    TEST_ASSERT_FALSE(asset_resolve("does_not_exist.png", out, sizeof(out)));
}

int main(void)
{
    // Sandbox all file lookups in a throwaway temp directory with an assets/ and
    // bin/assets/ tree.
    char tmpl[] = "/tmp/vagrant_rim_assets_XXXXXX";
    char *dir = mkdtemp(tmpl);
    if (dir == NULL || chdir(dir) != 0 || mkdir("assets", 0777) != 0 ||
        mkdir("bin", 0777) != 0 || mkdir("bin/assets", 0777) != 0) {
        fprintf(stderr, "test_assets: could not set up temp asset tree\n");
        return 1;
    }

    UNITY_BEGIN();
    RUN_TEST(test_resolves_under_assets_prefix);
    RUN_TEST(test_resolves_under_bin_assets_prefix);
    RUN_TEST(test_empty_prefix_passes_through_qualified_path);
    RUN_TEST(test_assets_prefix_wins_over_bin_assets);
    RUN_TEST(test_missing_file_returns_false);
    return UNITY_END();
}
