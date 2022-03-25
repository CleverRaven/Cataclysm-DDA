
#include "cached_options.h"
#include "cata_utility.h"
#include "debug.h"
#include "generic_factory.h"
#include "json.h"
#include "localized_comparator.h"
#include "translation.h"

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
    bool certainly_irregular = false;
    bool possibly_irregular = false;
    if( !raw.empty() &&
        ( raw.back() < 'a' || raw.back() > 'z' ) &&
        ( raw.back() < 'A' || raw.back() > 'Z' ) ) {
        // not ending with English alphabets
        possibly_irregular = true;
    }
    // endings with possible irregular forms
    // false = possibly irregular, true = certainly irregular
    static const std::vector<std::pair<std::string, bool>> irregular_endings = {
        { "ch", false },
        { "f", false },
        { "fe", false },
        { "o", false },
        { "s", true },
        { "sh", true },
        { "x", true },
        { "y", false },
        { ")", true },
    };
    for( const std::pair<std::string, bool> &ending : irregular_endings ) {
        if( string_ends_with( raw, ending.first ) ) {
            if( ending.second ) {
                certainly_irregular = true;
            } else {
                possibly_irregular = true;
            }
        }
    }
    // magiclysm enchantments
    for( auto it = raw.rbegin(); it < raw.rend(); ++it ) {
        if( *it < '0' || *it > '9' ) {
            if( *it == '+' && it != raw.rbegin() ) {
                certainly_irregular = true;
            }
            break;
        }
    }
    // compound words
    // false = possibly irregular, true = certainly irregular
    static const std::vector<std::pair<std::string, bool>> irregular_patterns = {
        { " of ", false },
        { " with ", true },
        { " for ", true },
        { ",", false },
    };
    for( const std::pair<std::string, bool> &pattern : irregular_patterns ) {
        if( raw.find( pattern.first ) != std::string::npos ) {
            if( pattern.second ) {
                certainly_irregular = true;
            } else {
                possibly_irregular = true;
            }
        }
    }
    const bool report_as_irregular = certainly_irregular
                                     || ( possibly_irregular && check_plural == check_plural_t::possible );
    return { !report_as_irregular, plural };
#else
    return { true, plural };
#endif
}

void translation::deserialize( JsonIn &jsin )
{
    // reset the cache
    cached_language_version = INVALID_LANGUAGE_VERSION;
    cached_num = 0;
    cached_translation = nullptr;
    int end_offset = jsin.tell();

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
            end_offset = jsin.tell();
        } else {
            // We know it's a string, we need to save the offset after the string.
            JsonValue jv = jsin.get_value();
            jv.get_string();
            end_offset = jsin.tell();
            raw = text_style_check_reader( text_style_check_reader::allow_object::no )
                  .get_next( jv );
        }
        // if plural form is enabled
        if( raw_pl ) {
            const std::pair<bool, std::string> suggested_pl = possible_plural_of( raw );
            raw_pl = cata::make_value<std::string>( suggested_pl.second );
#ifndef CATA_IN_TOOL
            if( !suggested_pl.first && check_style ) {
                try {
                    jsin.error( "Cannot autogenerate plural form.  "
                                "Please specify the plural form explicitly." );
                } catch( const JsonError &e ) {
                    debugmsg( "(json-error)\n%s", e.what() );
                }
            }
#endif
        }
        needs_translation = true;
    } else {
        JsonObject jsobj = jsin.get_object();
        end_offset = jsin.tell();
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
                            jsobj.throw_error( "\"str_sp\" is not necessary here since the "
                                               "plural form can be automatically generated.",
                                               "str_sp" );
                        }
                    } catch( const JsonError &e ) {
                        debugmsg( "(json-error)\n%s", e.what() );
                    }
                }
#endif
            } else {
                try {
                    jsobj.throw_error( "str_sp not supported here", "str_sp" );
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
                                jsobj.throw_error( "\"str_pl\" is not necessary here since the "
                                                   "plural form can be automatically generated.",
                                                   "str_pl" );
                            } else if( *raw_pl == raw ) {
                                jsobj.throw_error( "Please use \"str_sp\" instead of \"str\" and \"str_pl\" "
                                                   "for text with identical singular and plural forms",
                                                   "str_pl" );
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
                            jsobj.throw_error( "Cannot autogenerate plural form.  "
                                               "Please specify the plural form explicitly.",
                                               "str" );
                        } catch( const JsonError &e ) {
                            debugmsg( "(json-error)\n%s", e.what() );
                        }
                    }
#endif
                }
            } else if( jsobj.has_member( "str_pl" ) ) {
                try {
                    jsobj.throw_error( "str_pl not supported here", "str_pl" );
                } catch( const JsonError &e ) {
                    debugmsg( "(json-error)\n%s", e.what() );
                }
            }
        }
        needs_translation = true;
    }
    jsin.seek( end_offset );
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

bool translation::translated_eq( const translation &that ) const
{
    return translated() == that.translated();
}

bool translation::translated_ne( const translation &that ) const
{
    return !translated_eq( that );
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

cata::optional<int> translation::legacy_hash() const
{
    if( needs_translation && !ctxt && !raw_pl ) {
        return djb2_hash( reinterpret_cast<const unsigned char *>( raw.c_str() ) );
    }
    // Otherwise the translation must have been added after snippets were changed
    // to use string ids only, so the translation doesn't have a legacy hash value.
    return cata::nullopt;
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
