#ifndef CATA_MATH_DEFINES_H
#define CATA_MATH_DEFINES_H

// On Visual Studio math.h only provides the defines (M_PI, etc.) if
// _USE_MATH_DEFINES is defined before including it.
// We centralize that requirement in this header so that IWYU can force this
// header to be included when such defines are used, via mappings.
// At time of writing only M_PI has a mapping, which is the most commonly used
// one.  If more are needed, add them to tools/iwyu/vs.stdlib.imp.

// Note that it's important to use math.h here, not cmath.  See
// https://stackoverflow.com/questions/6563810/m-pi-works-with-math-h-but-not-with-cmath-in-visual-studio/6563891

#define _USE_MATH_DEFINES
#include <math.h>

#endif // CATA_MATH_DEFINES_H
