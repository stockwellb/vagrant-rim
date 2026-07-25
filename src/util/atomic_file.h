#ifndef VR_UTIL_ATOMIC_FILE_H
#define VR_UTIL_ATOMIC_FILE_H

#include <stdbool.h>

// Write `contents` to `path` atomically: write to a "<path>.tmp" file, flush and
// close it, then rename it over `path`. An interrupted or failed write leaves the
// existing file intact, because the swap is a single rename. Returns false (and
// leaves no temp file behind) on any error.
bool atomic_write(const char *path, const char *contents);

#endif // VR_UTIL_ATOMIC_FILE_H
