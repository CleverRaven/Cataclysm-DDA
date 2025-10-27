#pragma once

// Added due to clang 21.1 issue https://github.com/llvm/llvm-project/issues/154493
#ifdef __clang__
#   if __clang_major__ == 21 && __clang_minor__ <= 1
#       define LAMBDA_NORETURN_CLANG21x1 __attribute__( ( noreturn ) )
#   endif
#else
#   define LAMBDA_NORETURN_CLANG21x1
#endif

// Added for MSVC optimizations
// ref: https://github.com/CleverRaven/Cataclysm-DDA/pull/75376
#ifndef CATA_FORCEINLINE
#   ifdef _MSC_VER
#       define CATA_FORCEINLINE __forceinline
#   else
#       define CATA_FORCEINLINE inline __attribute__( ( always_inline ) )
#   endif
#endif // CATA_FORCEINLINE
