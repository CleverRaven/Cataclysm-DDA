#ifndef TRANSLATIONS_H
#define TRANSLATIONS_H

#ifdef LOCALIZE

// MingW flips out if you don't define this before you try to statically link libintl.
// This should prevent 'undefined reference to `_imp__libintl_gettext`' errors.
#if (defined _WIN32 || defined __CYGWIN__) && !defined _MSC_VER
#ifndef LIBINTL_STATIC
#define LIBINTL_STATIC
#endif
#endif

#include <string>
#include <cstdio>
#include <libintl.h>
#include <clocale>

inline const char *_( const char *msg )
{
    return ( msg[0] == '\0' ) ? msg : gettext( msg );
}

const char *pgettext( const char *context, const char *msgid );

// same as pgettext, but supports plural forms like ngettext
const char *npgettext( const char *context, const char *msgid, const char *msgid_plural,
                       unsigned long int n );

#else // !LOCALIZE

// on some systems <locale> pulls in libintl.h anyway,
// so preemptively include it before the gettext overrides.
#include <locale>

const char *strip_positional_formatting( const char *msgid );

// If PRINTF_CHECKS is enabled, have to skip the function call so GCC can see the format string
#if defined(PRINTF_CHECKS) && defined(__GNUC__)
#define _(STRING) (STRING)
#else
#define _(STRING) strip_positional_formatting(STRING)
#endif

#define ngettext(STRING1, STRING2, COUNT) (COUNT < 2 ? _(STRING1) : _(STRING2))
#define pgettext(STRING1, STRING2) _(STRING2)
#define npgettext(STRING0, STRING1, STRING2, COUNT) ngettext(STRING1, STRING2, COUNT)

#endif // LOCALIZE
void set_language();

#endif // _TRANSLATIONS_H_
