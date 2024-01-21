#include "translation.h"

#include <regex>

#include "cached_options.h"
#include "cata_utility.h"
#include "debug.h"
#include "generic_factory.h"
#include "json.h"
#include "localized_comparator.h"

translation::translation( const plural_tag ) : raw_pl( cata::make_value<std::string>() ) {}

translation::translation( const std::string &ctxt, const std::string &raw )
    : ctxt( cata::make_value<std::string>( ctxt ) ), raw( raw ), needs_translation( true )
{
}

translation::translation( const std::string &raw )
    : raw( raw ), needs_translation( true )
{
}

translation::translation( const std::string &raw, const std::string &raw_pl,
                          const plural_tag )
    : raw( raw ), raw_pl( cata::make_value<std::string>( raw_pl ) ), needs_translation( true )
{
}

translation::translation( const std::string &ctxt, const std::string &raw,
                          const std::string &raw_pl, const plural_tag )
    : ctxt( cata::make_value<std::string>( ctxt ) ),
      raw( raw ), raw_pl( cata::make_value<std::string>( raw_pl ) ), needs_translation( true )
{
}

translation::translation( const std::string &str, const no_translation_tag ) : raw( str ) {}

translation translation::to_translation( const std::string &raw )
{
    return translation{ raw };
}

translation translation::to_translation( const std::string &ctxt, const std::string &raw )
{
    return { ctxt, raw };
}

translation translation::pl_translation( const std::string &raw, const std::string &raw_pl )
{
    return { raw, raw_pl, plural_tag() };
}

translation translation::pl_translation( const std::string &ctxt, const std::string &raw,
        const std::string &raw_pl )
{
    return { ctxt, raw, raw_pl, plural_tag() };
}

translation translation::no_translation( const std::string &str )
{
    return { str, no_translation_tag() };
}

void translation::make_plural()
{
    if( needs_translation ) {
        // if plural form has not been enabled yet
        if( !raw_pl ) {
            // copy the singular string without appending "s" to preserve the original behavior
            raw_pl = cata::make_value<std::string>( raw );
        }
    } else if( !raw_pl ) {
        // just mark plural form as enabled
        raw_pl = cata::make_value<std::string>();
    }
    // reset the cache
    cached_language_version = INVALID_LANGUAGE_VERSION;
    cached_translation = nullptr;
}

// return { true, suggested plural } if no irregular form is detected,
// { false, suggested plural } otherwise. do have false positive/negatives.
static std::pair<bool, std::string> possible_plural_of( const std::string &raw )
{
    const std::string plural = raw + "s";
#ifndef CATA_IN_TOOL
    if( !test_mode || check_plural == check_plural_t::none ) {
        return { true, plural };
    }

    static const std::regex certainly_irregular_regex(
        // Not ending with an alphabet or number
        "(" R"([^a-zA-Z0-9]$)"
        // Some ending letters suggest irregular form with high certainty
        "|" R"((s|sh|x|tch|[rtpsdfgklzxcvnm]y|quy|[a-z]by)$)"
        // Magiclysm enchantment names (e.g. `cestus +1`)
        "|" R"(\+[0-9]+$)"
        // Some patterns suggest irregular form with high certainty
        "|" R"(([ -]with[ -]|[ -]for[ -]))"
        ")" );

    static const std::regex possibly_irregular_regex(
        // Not ending with an alphabet
        "(" R"([^a-zA-Z]$)"
        // Some ending letters suggest possible irregular form
        "|" R"((ch|f|fe|o)$)"
        // Some patterns suggest possible irregular form
        "|" R"(([ -]of[ -]|[,:]))"
        ")" );

    const bool certainly_irregular = std::regex_search( raw, certainly_irregular_regex );
    const bool possibly_irregular = std::regex_search( raw, possibly_irregular_regex );
    const bool report_as_irregular = certainly_irregular
                                     || ( possibly_irregular && check_plural == check_plural_t::possible );
    return { !report_as_irregular, plural };
#else
    return { true, plural };
#endif
}

void translation::deserialize( const JsonValue &jsin )
{
    // reset the cache
    cached_language_version = INVALID_LANGUAGE_VERSION;
    cached_num = 0;
    cached_translation = nullptr;

    if( jsin.test_string() ) {
        ctxt = nullptr;
#ifndef CATA_IN_TOOL
        const bool check_style = test_mode;
#else
        const bool check_style = false;
#endif
        if( raw_pl || !check_style ) {
            // strings with plural forms are currently only simple names, and
            // need no text style check.
            raw = jsin.get_string();
        } else {
            // We know it's a string, we need to save the offset after the string.
            raw = text_style_check_reader( text_style_check_reader::allow_object::no )
                  .get_next( jsin );
        }
        // if plural form is enabled
        if( raw_pl ) {
            const std::pair<bool, std::string> suggested_pl = possible_plural_of( raw );
            raw_pl = cata::make_value<std::string>( suggested_pl.second );
#ifndef CATA_IN_TOOL
            if( !suggested_pl.first && check_style ) {
                try {
                    jsin.throw_error_after( "Cannot autogenerate plural form.  "
                                            "Please specify the plural form explicitly using "
                                            "'str' and 'str_pl', or 'str_sp' if the singular "
                                            "and plural forms are the same." );
                } catch( const JsonError &e ) {
                    debugmsg( "(json-error)\n%s", e.what() );
                }
            }
#endif
        }
        needs_translation = true;
    } else {
        JsonObject jsobj = jsin.get_object();
        if( std::optional<JsonValue> comments = jsobj.get_member_opt( "//~" );
            comments.has_value() && !comments->test_string() ) {
            // Ensure we have a string and mark the member as visited in the process
            try {
                jsobj.throw_error_at( "//~", "\"//~\" should be a string that contains comments for translators." );
            } catch( const JsonError &e ) {
                debugmsg( "(json-error)\n%s", e.what() );
            }
        }
        if( jsobj.has_member( "ctxt" ) ) {
            ctxt = cata::make_value<std::string>( jsobj.get_string( "ctxt" ) );
        } else {
            ctxt = nullptr;
        }
#ifndef CATA_IN_TOOL
        const bool check_style = test_mode && !jsobj.has_member( "//NOLINT(cata-text-style)" );
#else
        const bool check_style = false;
#endif
        if( jsobj.has_member( "str_sp" ) ) {
            // same singular and plural forms
            // strings with plural forms are currently only simple names, and
            // need no text style check.
            raw = jsobj.get_string( "str_sp" );
            // if plural form is enabled
            if( raw_pl ) {
                raw_pl = cata::make_value<std::string>( raw );
#ifndef CATA_IN_TOOL
                if( check_style ) {
                    try {
                        const std::pair<bool, std::string> suggested_pl = possible_plural_of( raw );
                        if( suggested_pl.first && *raw_pl == suggested_pl.second ) {
                            jsobj.throw_error_at(
                                "str_sp",
                                "\"str_sp\" is not necessary here since the plural form can be "
                                "automatically generated." );
                        }
                    } catch( const JsonError &e ) {
                        debugmsg( "(json-error)\n%s", e.what() );
                    }
                }
#endif
            } else {
                try {
                    jsobj.throw_error_at( "str_sp", "str_sp not supported here" );
                } catch( const JsonError &e ) {
                    debugmsg( "(json-error)\n%s", e.what() );
                }
            }
        } else {
            if( raw_pl || !check_style ) {
                // strings with plural forms are currently only simple names, and
                // need no text style check.
                raw = jsobj.get_string( "str" );
            } else {
                raw = text_style_check_reader( text_style_check_reader::allow_object::no )
                      .get_next( jsobj.get_member( "str" ) );
            }
            // if plural form is enabled
            if( raw_pl ) {
                if( jsobj.has_member( "str_pl" ) ) {
                    raw_pl = cata::make_value<std::string>( jsobj.get_string( "str_pl" ) );
#ifndef CATA_IN_TOOL
                    if( check_style ) {
                        try {
                            const std::pair<bool, std::string> suggested_pl = possible_plural_of( raw );
                            if( suggested_pl.first && *raw_pl == suggested_pl.second ) {
                                jsobj.throw_error_at(
                                    "str_pl",
                                    "\"str_pl\" is not necessary here since the plural form can "
                                    "be automatically generated." );
                            } else if( *raw_pl == raw ) {
                                jsobj.throw_error_at(
                                    "str_pl",
                                    "Please use \"str_sp\" instead of \"str\" and \"str_pl\" "
                                    "for text with identical singular and plural forms" );
                            }
                        } catch( const JsonError &e ) {
                            debugmsg( "(json-error)\n%s", e.what() );
                        }
                    }
#endif
                } else {
                    const std::pair<bool, std::string> suggested_pl = possible_plural_of( raw );
                    raw_pl = cata::make_value<std::string>( suggested_pl.second );
#ifndef CATA_IN_TOOL
                    if( !suggested_pl.first && check_style ) {
                        try {
                            jsobj.throw_error_at(
                                "str",
                                "Cannot autogenerate plural form.  Please specify the plural "
                                "form explicitly using 'str' and 'str_pl', or 'str_sp' if the "
                                "singular and plural forms are the same." );
                        } catch( const JsonError &e ) {
                            debugmsg( "(json-error)\n%s", e.what() );
                        }
                    }
#endif
                }
            } else if( jsobj.has_member( "str_pl" ) ) {
                try {
                    jsobj.throw_error_at( "str_pl", "str_pl not supported here" );
                } catch( const JsonError &e ) {
                    debugmsg( "(json-error)\n%s", e.what() );
                }
            }
        }
        needs_translation = true;
    }

    // Reset the underlying jsonin stream because errors leave it in an undefined state.
    // This will be removed once everything is migrated off JsonIn.
    if( jsin.test_string() ) {
        jsin.get_string();

    } else if( jsin.test_object() ) {
        jsin.get_object().allow_omitted_members();
    }
}

std::string translation::translated( const int num ) const
{
    if( !needs_translation || raw.empty() ) {
        return raw;
    }
    // Note1: `raw`, `raw_pl` and `ctxt` are effectively immutable for caching purposes:
    // in the places where they are changed, cache is explicitly invalidated
    // Note2: if `raw_pl` is defined, `num` becomes part of the "cache key"
    // otherwise `num` is ignored (for both translation and cache)
    if( cached_language_version != detail::get_current_language_version() ||
        ( raw_pl && cached_num != num ) || !cached_translation ) {
        cached_language_version = detail::get_current_language_version();
        cached_num = num;

        if( !ctxt ) {
            if( !raw_pl ) {
                cached_translation = cata::make_value<std::string>( detail::_translate_internal( raw ) );
            } else {
                cached_translation = cata::make_value<std::string>(
                                         n_gettext( raw.c_str(), raw_pl->c_str(), num ) );
            }
        } else {
            if( !raw_pl ) {
                cached_translation = cata::make_value<std::string>( pgettext( ctxt->c_str(), raw.c_str() ) );
            } else {
                cached_translation = cata::make_value<std::string>(
                                         npgettext( ctxt->c_str(), raw.c_str(), raw_pl->c_str(), num ) );
            }
        }
    }
    return *cached_translation;
}

bool translation::empty() const
{
    return raw.empty();
}

bool translation::translated_lt( const translation &that ) const
{
    return localized_compare( translated(), that.translated() );
}

bool translation::translated_gt( const translation &that ) const
{
    return that.translated_lt( *this );
}

bool translation::translated_le( const translation &that ) const
{
    return !that.translated_lt( *this );
}

bool translation::translated_ge( const translation &that ) const
{
    return !translated_lt( that );
}

bool translation::translated_eq( const translation &that ) const
{
    return translated() == that.translated();
}

bool translation::translated_ne( const translation &that ) const
{
    return !translated_eq( that );
}

bool translated_less::operator()( const translation &lhs,
                                  const translation &rhs ) const
{
    return lhs.translated_lt( rhs );
}

bool translated_greater::operator()( const translation &lhs,
                                     const translation &rhs ) const
{
    return lhs.translated_gt( rhs );
}

bool translated_less_equal::operator()( const translation &lhs,
                                        const translation &rhs ) const
{
    return lhs.translated_le( rhs );
}

bool translated_greater_equal::operator()( const translation &lhs,
        const translation &rhs ) const
{
    return lhs.translated_ge( rhs );
}

bool translated_equal_to::operator()( const translation &lhs,
                                      const translation &rhs ) const
{
    return lhs.translated_eq( rhs );
}

bool translated_not_equal_to::operator()( const translation &lhs,
        const translation &rhs ) const
{
    return lhs.translated_ne( rhs );
}

bool translation::operator==( const translation &that ) const
{
    return value_ptr_equals( ctxt, that.ctxt ) && raw == that.raw &&
           value_ptr_equals( raw_pl, that.raw_pl ) &&
           needs_translation == that.needs_translation;
}

bool translation::operator!=( const translation &that ) const
{
    return !operator==( that );
}

std::optional<int> translation::legacy_hash() const
{
    if( needs_translation && !ctxt && !raw_pl ) {
        return djb2_hash( reinterpret_cast<const unsigned char *>( raw.c_str() ) );
    }
    // Otherwise the translation must have been added after snippets were changed
    // to use string ids only, so the translation doesn't have a legacy hash value.
    return std::nullopt;
}

translation to_translation( const std::string &raw )
{
    return translation::to_translation( raw );
}

translation to_translation( const std::string &ctxt, const std::string &raw )
{
    return translation::to_translation( ctxt, raw );
}

translation pl_translation( const std::string &raw, const std::string &raw_pl )
{
    return translation::pl_translation( raw, raw_pl );
}

translation pl_translation( const std::string &ctxt, const std::string &raw,
                            const std::string &raw_pl )
{
    return translation::pl_translation( ctxt, raw, raw_pl );
}

translation no_translation( const std::string &str )
{
    return translation::no_translation( str );
}

std::ostream &operator<<( std::ostream &out, const translation &t )
{
    return out << t.translated();
}

std::string operator+( const translation &lhs, const std::string &rhs )
{
    return lhs.translated() + rhs;
}

std::string operator+( const std::string &lhs, const translation &rhs )
{
    return lhs + rhs.translated();
}

std::string operator+( const translation &lhs, const translation &rhs )
{
    return lhs.translated() + rhs.translated();
}
