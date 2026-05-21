#pragma once

#include "snmalloc/snmalloc.h"

#ifndef SNMALLOC_EXPORT
#  define SNMALLOC_EXPORT
#endif
#ifdef SNMALLOC_STATIC_LIBRARY_PREFIX
#  define __SN_CONCAT(a, b) a##b
#  define __SN_EVALUATE(a, b) __SN_CONCAT(a, b)
#  define SNMALLOC_NAME_MANGLE(a) \
    __SN_EVALUATE(SNMALLOC_STATIC_LIBRARY_PREFIX, a)
#elif !defined(SNMALLOC_NAME_MANGLE)
#  define SNMALLOC_NAME_MANGLE(a) a
#endif
