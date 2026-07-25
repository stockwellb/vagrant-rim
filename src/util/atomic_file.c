#include "util/atomic_file.h"

#include <stdio.h>

bool atomic_write(const char *path, const char *contents)
{
    char tmp[1024];
    int n = snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    if (n < 0 || (size_t)n >= sizeof(tmp)) {
        return false; // path too long to form a temp name
    }

    FILE *f = fopen(tmp, "w");
    if (!f) {
        return false;
    }
    // fputs then fclose: both must succeed (fclose flushes buffered data) before
    // the temp file is trustworthy enough to rename over the target.
    if (fputs(contents, f) == EOF) {
        fclose(f);
        remove(tmp);
        return false;
    }
    if (fclose(f) != 0) {
        remove(tmp);
        return false;
    }
    if (rename(tmp, path) != 0) {
        remove(tmp);
        return false;
    }
    return true;
}
