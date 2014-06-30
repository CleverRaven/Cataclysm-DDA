#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef _MSC_VER
// MSVC doesn't have ssize_t, so use int instead
#include <stddef.h>
#include <basetsd.h>
typedef SSIZE_T ssize_t;
#endif

#endif