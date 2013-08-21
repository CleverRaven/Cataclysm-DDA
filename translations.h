#ifndef _TRANSLATIONS_H_
#define _TRANSLATIONS_H_

#ifdef LOCALIZE
// MingW flips out if you don't define this before you try to statically link libintl.
// This should prevent 'undefined reference to `_imp__libintl_gettext`' errors.

#if defined _WIN32 || defined __CYGWIN__
 #ifndef LIBINTL_STATIC
  #define LIBINTL_STATIC
 #endif
#endif

#include <cstdio>
#include <libintl.h>
#include <clocale>
#define _(STRING) gettext(STRING)
#else
#define _(STRING) STRING
#define ngettext(STRING1, STRING2, COUNT) (COUNT < 2 ? STRING1 : STRING2)
#endif

#endif // _TRANSLATIONS_H_
