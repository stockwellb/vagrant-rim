#include "util/assets.h"

#include <stdio.h>

#include "raylib.h" // FileExists

// Asset directory prefixes tried when resolving a relative asset path. Covers
// running from the project root during development ("assets/") and the installed
// bin/assets layout; the empty prefix lets an already-qualified path pass through.
static const char *asset_prefixes[] = { "assets/", "bin/assets/", "" };

bool asset_resolve(const char *rel, char *out, int out_size)
{
    for (int i = 0; i < (int)(sizeof(asset_prefixes) / sizeof(asset_prefixes[0])); i++) {
        snprintf(out, (size_t)out_size, "%s%s", asset_prefixes[i], rel);
        if (FileExists(out)) {
            return true;
        }
    }
    return false;
}
