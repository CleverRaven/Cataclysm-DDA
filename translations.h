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
const char * pgettext(const char *context, const char *msgid);

#else // !LOCALIZE

// on some systems <locale> pulls in libintl.h anyway,
// so preemptively include it before the gettext overrides.
#include <locale>

const char* strip_positional_formatting(const char* msgid);

#define _(STRING) strip_positional_formatting(STRING)
#define ngettext(STRING1, STRING2, COUNT) (COUNT < 2 ? _(STRING1) : _(STRING2))
#define pgettext(STRING1, STRING2) _(STRING2)

#endif // LOCALIZE

#endif // _TRANSLATIONS_H_
