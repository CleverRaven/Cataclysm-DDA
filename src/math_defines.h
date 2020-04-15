#pragma once
#ifndef CATA_SRC_MATH_DEFINES_H
#define CATA_SRC_MATH_DEFINES_H

// On Visual Studio math.h only provides the defines (M_PI, etc.) if
// _USE_MATH_DEFINES is defined before including it.
// We centralize that requirement in this header so that IWYU can force this
// header to be included when such defines are used, via mappings.
// At time of writing only M_PI has a mapping, which is the most commonly used
// one.  If more are needed, add them to tools/iwyu/vs.stdlib.imp.

// Note that it's important to use math.h here, not cmath.  See
// https://stackoverflow.com/questions/6563810/m-pi-works-with-math-h-but-not-with-cmath-in-visual-studio/6563891

#define _USE_MATH_DEFINES
#include <cmath>

// And on mingw even that doesn't work, so we are forced to have our own
// fallback definitions.
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif

#ifndef M_SQRT2
#define M_SQRT2 1.41421356237309504880
#endif

#endif // CATA_SRC_MATH_DEFINES_H
