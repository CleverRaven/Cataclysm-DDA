#pragma once
#ifndef CATA_SRC_TRANSLATION_H
#define CATA_SRC_TRANSLATION_H

#include <optional>
#include <string>

#include "translation_cache.h"
#include "value_ptr.h"

class JsonObject;
class JsonValue;

/**
 * Class for storing translation context and raw string for deferred translation
 **/
class translation
{
    public:
        struct plural_tag {};

        // need to have user-defined constructor to work around clang 3.8 bug
        // translation() = default doesn't work!
        // see: https://stackoverflow.com/a/47368753/1349366
        // NOLINTNEXTLINE default constructor
        translation() {}
        /**
         * Same as `translation()`, but with plural form enabled.
         **/
        explicit translation( plural_tag );

        /**
         * Store a string, an optional plural form, and an optional context for translation
         **/
        static translation to_translation( const std::string &raw );
        static translation to_translation( const std::string &ctxt, const std::string &raw );
        static translation pl_translation( const std::string &raw, const std::string &raw_pl );
        static translation pl_translation( const std::string &ctxt, const std::string &raw,
                                           const std::string &raw_pl );
        /**
         * Store a string that needs no translation.
         **/
        static translation no_translation( const std::string &str );

        /**
         * Can be used to ensure a translation object has plural form enabled
         * before loading into it from JSON. If plural form has not been enabled
         * yet, the plural string will be set to the original singular string.
         * `n_gettext` will ignore the new plural string and correctly retrieve
         * the original translation.
         *     Note that a `make_singular()` function is not provided due to the
         * potential loss of information.
         **/
        void make_plural();

        /**
         * Deserialize from json. Json format is:
         *     "text"
         * or
         *     { "ctxt": "foo", "str": "bar", "str_pl": "baz" }
         * "ctxt" and "str_pl" are optional. "str_pl" is only valid when an object
         * of this class is constructed with `plural_tag` or `pl_translation()`,
         * or converted using `make_plural()`.
         **/
        void deserialize( const JsonValue &jsin );
        void deserialize( const JsonObject &jsobj );

        /**
         * Returns raw string if no translation is needed, otherwise returns
         * the translated string. A number can be used to translate the plural
         * form if the object has it.
         **/
        std::string translated( int num = 1 ) const;

        /**
         * Methods exposing the underlying raw strings are not implemented, and
         * probably should not if there's no good reason to do so. Most importantly,
         * the underlying strings should not be re-saved to JSON: doing so risk
         * the original string being changed during development and the saved
         * string will then not be properly translated when loaded back. If you
         * really want to save a translation, translate it early on, store it using
         * `no_translation`, and retrieve it using `translated()` when saving.
         * This ensures consistent behavior before and after saving and loading.
         **/
        std::string untranslated() const = delete;

        /**
         * Similar to `untranslated`, serialization should not be defined
         * because the saved string will not translate correctly when the
         * source changes.
         **/
        void serialize( JsonOut &jsout ) const = delete;

        /**
         * Whether the underlying string is empty, not matter what the context
         * is or whether translation is needed.
         **/
        bool empty() const;

        /**
         * Compare translations by their translated strings (singular form).
         *
         * Be especially careful when using these to sort translations, as the
         * translated result will change when switching the language.
         *
         * When updating the comparison functions, please also update
         * `tools/clang-tidy-plugin/NoStaticTranslationCheck.cpp` and
         * `tools/clang-tidy-plugin/test/no-static-translation.cpp`.
         **/
        bool translated_lt( const translation &that ) const; // <
        bool translated_gt( const translation &that ) const; // >
        bool translated_le( const translation &that ) const; // <=
        bool translated_ge( const translation &that ) const; // >=
        bool translated_eq( const translation &that ) const; // ==
        bool translated_ne( const translation &that ) const; // !=

        /**
         * Compare translations by their context, raw strings (singular / plural),
         * and no-translation flag. Use `translated_(eq|ne)` if you want to do
         * localized comparison.
         */
        bool operator==( const translation &that ) const;
        bool operator!=( const translation &that ) const;
        /**
         * Not defined for now. Use `translated_(lt|gt|le|ge)` if you want
         * to do localized comparison.
         */
        bool operator<( const translation &that ) const = delete;
        bool operator>( const translation &that ) const = delete;
        bool operator<=( const translation &that ) const = delete;
        bool operator>=( const translation &that ) const = delete;

        /**
         * Only used for migrating old snippet hashes into snippet ids.
         */
        std::optional<int> legacy_hash() const;
    private:
        translation( const std::string &ctxt, const std::string &raw );
        explicit translation( const std::string &raw );
        translation( const std::string &raw, const std::string &raw_pl, plural_tag );
        translation( const std::string &ctxt, const std::string &raw, const std::string &raw_pl,
                     plural_tag );
        struct no_translation_tag {};
        translation( const std::string &str, no_translation_tag );

        cata::value_ptr<std::string> ctxt;
        std::string raw;
        cata::value_ptr<std::string> raw_pl;
        bool needs_translation = false;
        // translation cache. For "plural" translation only latest `num` is optimistically cached
        mutable int cached_language_version = INVALID_LANGUAGE_VERSION;
        // `num`, which `cached_translation` corresponds to
        mutable int cached_num = 0;
        mutable cata::value_ptr<std::string> cached_translation;
};

/**
 * Function objects for comparing translations by their translated strings
 * (singular form). Useful as the template parameter of sets and maps when
 * doing localized sorting. Do be careful when using these to sort translations,
 * as the translated result will change when switching the language.
 *
 * When updating the comparison function objects, please also update
 * `tools/clang-tidy-plugin/NoStaticTranslationCheck.cpp` and
 * `tools/clang-tidy-plugin/test/no-static-translation.cpp`.
 **/
class translated_less
{
    public:
        bool operator()( const translation &lhs, const translation &rhs ) const;
};

class translated_greater
{
    public:
        bool operator()( const translation &lhs, const translation &rhs ) const;
};

class translated_less_equal
{
    public:
        bool operator()( const translation &lhs, const translation &rhs ) const;
};

class translated_greater_equal
{
    public:
        bool operator()( const translation &lhs, const translation &rhs ) const;
};

class translated_equal_to
{
    public:
        bool operator()( const translation &lhs, const translation &rhs ) const;
};

class translated_not_equal_to
{
    public:
        bool operator()( const translation &lhs, const translation &rhs ) const;
};

/**
 * Shorthands for translation::to_translation
 **/
translation to_translation( const std::string &raw );
translation to_translation( const std::string &ctxt, const std::string &raw );
/**
 * Shorthands for translation::pl_translation
 **/
translation pl_translation( const std::string &raw, const std::string &raw_pl );
translation pl_translation( const std::string &ctxt, const std::string &raw,
                            const std::string &raw_pl );
/**
 * Shorthand for translation::no_translation
 **/
translation no_translation( const std::string &str );

/**
 * Stream output and concatenation of translations. Singular forms are used.
 **/
std::ostream &operator<<( std::ostream &out, const translation &t );
std::string operator+( const translation &lhs, const std::string &rhs );
std::string operator+( const std::string &lhs, const translation &rhs );
std::string operator+( const translation &lhs, const translation &rhs );

#endif // CATA_SRC_TRANSLATION_H
