#ifndef VR_UTIL_MATHX_H
#define VR_UTIL_MATHX_H

// Small shared numeric clamps, so the same [0,1] / [-1,1] helpers aren't
// re-defined in every module that happens to need one.

// Clamp v to [lo, hi].
float clampf(float v, float lo, float hi);

// Clamp v to [lo, hi] (integer) — sizes, counts, and other bounded ints.
int clampi(int v, int lo, int hi);

// Clamp v to [0, 1] — volumes and other normalized fractions.
float clamp01(float v);

// Clamp v to [-1, 1] — analog stick axes.
float clamp_unit(float v);

#endif // VR_UTIL_MATHX_H
