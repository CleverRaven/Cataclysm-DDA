/**
* @name Use of c-style math functions
* @description c-style math functions imported by <cmath> or "math.h" interact badly with the C++ type system.
* @kind problem
* @problem.severity warning
* @precision high
* @id cpp/use-cstyle-math-functions
* @tags correctness
*/

import cpp

from FunctionCall call, Function fcn
where
  call.getTarget() = fcn and fcn.isTopLevel() and not exists ( call.getNameQualifier() ) and
  ( fcn.hasGlobalName("abs") or fcn.hasGlobalName("fabs") or fcn.hasGlobalName("fabsf") or fcn.hasGlobalName("fabsl") or
    fcn.hasGlobalName("fmod") or fcn.hasGlobalName("fmodf") or fcn.hasGlobalName("fmodl") or
    fcn.hasGlobalName("remainder") or fcn.hasGlobalName("remainderf") or fcn.hasGlobalName("remainderl") or
    fcn.hasGlobalName("remquo") or fcn.hasGlobalName("remquof") or fcn.hasGlobalName("remquol") or
    fcn.hasGlobalName("fma") or fcn.hasGlobalName("fmaf") or fcn.hasGlobalName("fmal") or
    fcn.hasGlobalName("fmax") or fcn.hasGlobalName("fmaxf") or fcn.hasGlobalName("fmaxl") or
    fcn.hasGlobalName("fmin") or fcn.hasGlobalName("fminf") or fcn.hasGlobalName("fminl") or
    fcn.hasGlobalName("fdim") or fcn.hasGlobalName("fdimf") or fcn.hasGlobalName("fdiml") or
    fcn.hasGlobalName("nan") or fcn.hasGlobalName("nanf") or fcn.hasGlobalName("nanl") or
    fcn.hasGlobalName("exp") or fcn.hasGlobalName("expf") or fcn.hasGlobalName("expl") or
    fcn.hasGlobalName("exp2") or fcn.hasGlobalName("exp2f") or fcn.hasGlobalName("exp2l") or
    fcn.hasGlobalName("expm") or fcn.hasGlobalName("expmf") or fcn.hasGlobalName("expml") or
    fcn.hasGlobalName("log") or fcn.hasGlobalName("logf") or fcn.hasGlobalName("logl") or
    fcn.hasGlobalName("log10") or fcn.hasGlobalName("log10f") or fcn.hasGlobalName("log10l") or
    fcn.hasGlobalName("log2") or fcn.hasGlobalName("log2f") or fcn.hasGlobalName("log2l") or
    fcn.hasGlobalName("loglp") or fcn.hasGlobalName("loglpf") or fcn.hasGlobalName("loglpl") or
    fcn.hasGlobalName("pow") or fcn.hasGlobalName("powf") or fcn.hasGlobalName("powl") or
    fcn.hasGlobalName("sqrt") or fcn.hasGlobalName("sqrtf") or fcn.hasGlobalName("sqrtl") or
    fcn.hasGlobalName("cbrt") or fcn.hasGlobalName("cbrtf") or fcn.hasGlobalName("cbrtl") or
    fcn.hasGlobalName("hpot") or fcn.hasGlobalName("hpotf") or fcn.hasGlobalName("hpotl") or
    fcn.hasGlobalName("sin") or fcn.hasGlobalName("sinf") or fcn.hasGlobalName("sinl") or
    fcn.hasGlobalName("cos") or fcn.hasGlobalName("cosf") or fcn.hasGlobalName("cosl") or
    fcn.hasGlobalName("tan") or fcn.hasGlobalName("tanf") or fcn.hasGlobalName("tanl") or
    fcn.hasGlobalName("asinh") or fcn.hasGlobalName("asinhf") or fcn.hasGlobalName("asinhl") or
    fcn.hasGlobalName("acosh") or fcn.hasGlobalName("acoshf") or fcn.hasGlobalName("acoshl") or
    fcn.hasGlobalName("atanh") or fcn.hasGlobalName("atanhf") or fcn.hasGlobalName("atanhl") or
    fcn.hasGlobalName("ceil") or fcn.hasGlobalName("ceilf") or fcn.hasGlobalName("ceill") or
    fcn.hasGlobalName("floor") or fcn.hasGlobalName("floorf") or fcn.hasGlobalName("floorl") or
    fcn.hasGlobalName("trunc") or fcn.hasGlobalName("truncf") or fcn.hasGlobalName("truncl") or
    fcn.hasGlobalName("round") or fcn.hasGlobalName("roundf") or fcn.hasGlobalName("roundl") or
    fcn.hasGlobalName("lround") or fcn.hasGlobalName("lroundf") or fcn.hasGlobalName("lroundl") or
    fcn.hasGlobalName("llround") or fcn.hasGlobalName("llroundf") or fcn.hasGlobalName("llroundl") )
select call, "Use of c-style math function"
