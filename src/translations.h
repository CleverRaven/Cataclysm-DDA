#pragma once
#ifndef CATA_SRC_TRANSLATIONS_H
#define CATA_SRC_TRANSLATIONS_H

// on some systems <locale> pulls in libintl.h anyway,
// so preemptively include it before the gettext overrides.
#include <locale> // IWYU pragma: keep

#include "translation.h"

#if !defined(translate_marker)
/**
 * Marks a string literal to be extracted for translation. This is only for running `xgettext` via
 * "lang/update_pot.sh". Use `_` to extract *and* translate at run time. The macro itself does not
 * do anything, the argument is passed through it without any changes.
 */
#define translate_marker(x) x
#endif
#if !defined(translate_marker_context)
/**
 * Same as @ref translate_marker, but also provides a context (string literal). This is similar
 * to @ref pgettext, but it does not translate at run time. Like @ref translate_marker it just
 * passes the *second* argument through.
 */
#define translate_marker_context(c, x) x
#endif

#if defined(LOCALIZE)

#include "translation_cache.h"

#if defined(__GNUC__)
#  define ATTRIBUTE_FORMAT_ARG(a) __attribute__((format_arg(a)))
#else
#  define ATTRIBUTE_FORMAT_ARG(a)
#endif

void select_language();

// For code analysis purposes in our clang-tidy plugin we need to be able to
// detect when something is the argument to a translation function.  The _
// macro makes this really tricky, so we add an otherwise unnecessary call to
// this no-op function just so that there's something to detect.
template<typename T>
inline const T &translation_argument_identity( const T &t )
{
    return t;
}

// Note: in case of std::string argument, the result is copied, this is intended (for safety)
// Note that _ triggers reserved identifier warnings, but we suppress all
// three because it's a common use of _ and thus not likely to be a problem in
// practice.
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
#define _( msg ) \
    ( ( []( const auto & arg ) { \
        static auto cache = detail::get_local_translation_cache( arg ); \
        return cache( arg ); \
    } )( translation_argument_identity( msg ) ) )

inline const char *n_gettext( const char *msgid, const char *msgid_plural,
                              std::size_t n ) ATTRIBUTE_FORMAT_ARG( 1 );

inline const char *n_gettext( const char *msgid, const char *msgid_plural,
                              std::size_t n )
{
    return TranslationManager::GetInstance().TranslatePlural( msgid, msgid_plural, n );
}

inline const char *pgettext( const char *context, const char *msgid ) ATTRIBUTE_FORMAT_ARG( 2 );

inline const char *pgettext( const char *context, const char *msgid )
{
    return TranslationManager::GetInstance().TranslateWithContext( context, msgid );
}

inline const char *npgettext( const char *context, const char *msgid,
                              const char *msgid_plural, unsigned long long n ) ATTRIBUTE_FORMAT_ARG( 2 );

inline const char *npgettext( const char *const context, const char *const msgid,
                              const char *const msgid_plural, const unsigned long long n )
{
    return TranslationManager::GetInstance().TranslatePluralWithContext( context, msgid, msgid_plural,
            n );
}

#else // !LOCALIZE

#define _(STRING) (STRING)

#define n_gettext(STRING1, STRING2, COUNT) ((COUNT) == 1 ? _(STRING1) : _(STRING2))
#define pgettext(STRING1, STRING2) _(STRING2)
#define npgettext(STRING0, STRING1, STRING2, COUNT) n_gettext(STRING1, STRING2, COUNT)

#endif // LOCALIZE

std::string locale_dir();

void set_language();

#endif // CATA_SRC_TRANSLATIONS_H
