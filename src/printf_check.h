#pragma once
#ifndef PRINTF_CHECK_H
#define PRINTF_CHECK_H

// PRINTF_LIKE is used to tag functions that take printf-like format messages and vararg
// parameters. When PRINTF_CHECKS is defined, this macro expands to a GCC extension attribute that
// makes GCC check the format string and provided arguments for errors.

// PRINTF_LIKE takes two arguments: the parameter number that is the format string, and the
// parameter number that is the first parameter to the format (usually, the index of a varargs '...'
// parameter).  The first parameter is index 1.

// When applying PRINTF_LIKE to a method (rather than a bare function), add 1 to the parameter
// indexes, as there is an invisible "this" parameter as the first parameter.

#if defined(PRINTF_CHECKS) && !defined(LOCALIZE) && defined(__GNUC__)
#  define PRINTF_LIKE(a,b) __attribute__((format(gnu_printf,a,b)))
#else
#  define PRINTF_LIKE(a,b) /**/
#endif

#endif
