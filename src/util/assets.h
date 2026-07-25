#ifndef VR_UTIL_ASSETS_H
#define VR_UTIL_ASSETS_H

#include <stdbool.h>

// Resolve a relative asset path (e.g. "audio/music.ogg") against the known asset
// search prefixes — dev-tree "assets/", installed "bin/assets/", or bare — and
// write the first path that exists into `out`. Returns true on a hit, false if
// no candidate file is found.
bool asset_resolve(const char *rel, char *out, int out_size);

#endif // VR_UTIL_ASSETS_H
