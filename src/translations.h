#pragma once
#ifndef TRANSLATIONS_H
#define TRANSLATIONS_H

#include <map>
#include <string>
#include <vector>
#include <type_traits>

#include "optional.h"

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

// MingW flips out if you don't define this before you try to statically link libintl.
// This should prevent 'undefined reference to `_imp__libintl_gettext`' errors.
#if (defined(_WIN32) || defined(__CYGWIN__)) && !defined(_MSC_VER)
#   if !defined(LIBINTL_STATIC)
#       define LIBINTL_STATIC
#   endif
#endif

// IWYU pragma: begin_exports
#include <libintl.h>
// IWYU pragma: end_exports

#if defined(__GNUC__)
#  define ATTRIBUTE_FORMAT_ARG(a) __attribute__((format_arg(a)))
#else
#  define ATTRIBUTE_FORMAT_ARG(a)
#endif

const char *_( const char *msg ) ATTRIBUTE_FORMAT_ARG( 1 );
inline const char *_( const char *msg )
{
    return msg[0] == '\0' ? msg : gettext( msg );
}
inline std::string _( const std::string &msg )
{
    return _( msg.c_str() );
}

// ngettext overload taking an unsigned long long so that people don't need
// to cast at call sites.  This is particularly relevant on 64-bit Windows where
// size_t is bigger than unsigned long, so MSVC will try to encourage you to
// add a cast.
template<typename T, typename = std::enable_if_t<std::is_same<T, unsigned long long>::value>>
ATTRIBUTE_FORMAT_ARG( 1 )
inline const char *ngettext( const char *msgid, const char *msgid_plural, T n )
{
    // Leaving this long because it matches the underlying API.
    // NOLINTNEXTLINE(cata-no-long)
    return ngettext( msgid, msgid_plural, static_cast<unsigned long>( n ) );
}

const char *pgettext( const char *context, const char *msgid ) ATTRIBUTE_FORMAT_ARG( 2 );

// same as pgettext, but supports plural forms like ngettext
const char *npgettext( const char *context, const char *msgid, const char *msgid_plural,
                       unsigned long long n ) ATTRIBUTE_FORMAT_ARG( 2 );

#else // !LOCALIZE

// on some systems <locale> pulls in libintl.h anyway,
// so preemptively include it before the gettext overrides.
#include <locale>

#define _(STRING) (STRING)

#define ngettext(STRING1, STRING2, COUNT) (COUNT < 2 ? _(STRING1) : _(STRING2))
#define pgettext(STRING1, STRING2) _(STRING2)
#define npgettext(STRING0, STRING1, STRING2, COUNT) ngettext(STRING1, STRING2, COUNT)

#endif // LOCALIZE

using GenderMap = std::map<std::string, std::vector<std::string>>;
/**
 * Translation with a gendered context
 *
 * Similar to pgettext, but the context is a collection of genders.
 * @param genders A map where each key is a subject name (a string which should
 * make sense to the translator in the context of the line to be translated)
 * and the corresponding value is a list of potential genders for that subject.
 * The first gender from the list of genders for the current language will be
 * chosen for each subject (or the language default if there are no genders in
 * common).
 */
std::string gettext_gendered( const GenderMap &genders, const std::string &msg );

bool isValidLanguage( const std::string &lang );
std::string getLangFromLCID( const int &lcid );
void select_language();
void set_language();

class JsonIn;

/**
 * Class for storing translation context and raw string for deferred translation
 **/
class translation
{
    public:
        translation();
        /**
         * Create a deferred translation with context
         **/
        translation( const std::string &ctxt, const std::string &raw );

        /**
         * Create a deferred translation without context
         **/
        explicit translation( const std::string &raw );

        /**
         * Store a string that needs no translation.
         **/
        static translation no_translation( const std::string &str );

        /**
         * Deserialize from json. Json format is:
         *     "text"
         * or
         *     { "ctxt": "foo", "str": "bar" }
         **/
        void deserialize( JsonIn &jsin );

        /**
         * Returns raw string if no translation is needed, otherwise returns
         * the translated string.
         **/
        std::string translated() const;

        /**
         * Whether the underlying string is empty, not matter what the context
         * is or whether translation is needed.
         **/
        bool empty() const;

        /**
         * Compare translations by their translated strings.
         *
         * Be especially careful when using these to sort translations, as the
         * translated result will change when switching the language.
         **/
        bool operator<( const translation &that ) const;
        bool operator==( const translation &that ) const;
        bool operator!=( const translation &that ) const;
    private:
        struct no_translation_tag {};
        translation( const std::string &str, const no_translation_tag );

        cata::optional<std::string> ctxt;
        std::string raw;
        bool needs_translation = false;
};

/**
 * Shorthand for translation::no_translation
 **/
translation no_translation( const std::string &str );

#endif // _TRANSLATIONS_H_
