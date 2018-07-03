#pragma once
#ifndef TRANSLATIONS_H
#define TRANSLATIONS_H

#ifndef translate_marker
/**
 * Marks a string literal to be extracted for translation. This is only for running `xgettext` via
 * "lang/update_pot.sh". Use `_` to extract *and* translate at run time. The macro itself does not
 * do anything, the argument is passed through it without any changes.
 */
#define translate_marker(x) x
#endif
#ifndef translate_marker_context
/**
 * Same as @ref translate_marker, but also provides a context (string literal). This is similar
 * to @ref pgettext, but it does not translate at run time. Like @ref translate_marker it just
 * passes the *second* argument through.
 */
#define translate_marker_context(c, x) x
#endif

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

#if defined(__GNUC__)
#  define ATTRIBUTE_FORMAT_ARG(a) __attribute__((format_arg(a)))
#else
#  define ATTRIBUTE_FORMAT_ARG(a)
#endif

const char *_( const char *msg ) ATTRIBUTE_FORMAT_ARG( 1 );
inline const char *_( const char *msg )
{
    return ( msg[0] == '\0' ) ? msg : gettext( msg );
}

const char *pgettext( const char *context, const char *msgid ) ATTRIBUTE_FORMAT_ARG( 2 );

// same as pgettext, but supports plural forms like ngettext
const char *npgettext( const char *context, const char *msgid, const char *msgid_plural,
                       unsigned long int n ) ATTRIBUTE_FORMAT_ARG( 2 );

#else // !LOCALIZE

// on some systems <locale> pulls in libintl.h anyway,
// so preemptively include it before the gettext overrides.
#include <locale>

#define _(STRING) (STRING)

#define ngettext(STRING1, STRING2, COUNT) (COUNT < 2 ? _(STRING1) : _(STRING2))
#define pgettext(STRING1, STRING2) _(STRING2)
#define npgettext(STRING0, STRING1, STRING2, COUNT) ngettext(STRING1, STRING2, COUNT)

#endif // LOCALIZE
bool isValidLanguage( const std::string &lang );
std::string getLangFromLCID( const int &lcid );
void select_language();
void set_language();

#endif // _TRANSLATIONS_H_
