#pragma once

// Added due to clang 21.1 issue https://github.com/llvm/llvm-project/issues/154493
#if defined(__clang__)
#   if __clang_major__ == 21 && __clang_minor__ <= 1
#       define LAMBDA_NORETURN_CLANG21x1 __attribute__( ( noreturn ) )
#   endif
#else
#   define LAMBDA_NORETURN_CLANG21x1
#endif
