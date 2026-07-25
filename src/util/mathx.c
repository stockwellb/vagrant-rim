#include "util/mathx.h"

float clampf(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

int clampi(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

float clamp01(float v) { return clampf(v, 0.0f, 1.0f); }

float clamp_unit(float v) { return clampf(v, -1.0f, 1.0f); }
