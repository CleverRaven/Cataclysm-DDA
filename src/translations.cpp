#include "translations.h"

#include <algorithm>

#include "cached_options.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "json.h"
#include "language.h"
#include "output.h"
#include "rng.h"
#include "text_style_check.h"

// int version/generation that is incremented each time language is changed
// used to invalidate translation cache
static int current_language_version = INVALID_LANGUAGE_VERSION + 1;

int detail::get_current_language_version()
{
    return current_language_version;
}

void invalidate_translations()
{
    // increment version to invalidate translation cache
    do {
        current_language_version++;
    } while( current_language_version == INVALID_LANGUAGE_VERSION );
}

#if defined(LOCALIZE)

const char *pgettext( const char *context, const char *msgid )
{
    // need to construct the string manually,
    // to correctly handle strings loaded from json.
    // could probably do this more efficiently without using std::string.
    std::string context_id( context );
    context_id += '\004';
    context_id += msgid;
    // null domain, uses global translation domain
    const char *msg_ctxt_id = context_id.c_str();
#if defined(__ANDROID__)
    const char *translation = gettext( msg_ctxt_id );
#else
    const char *translation = dcgettext( nullptr, msg_ctxt_id, LC_MESSAGES );
#endif
    if( translation == msg_ctxt_id ) {
        return msgid;
    } else {
        return translation;
    }
}

const char *npgettext( const char *const context, const char *const msgid,
                       const char *const msgid_plural, const unsigned long long n )
{
    const std::string context_id = std::string( context ) + '\004' + msgid;
    const char *const msg_ctxt_id = context_id.c_str();
#if defined(__ANDROID__)
    const char *const translation = ngettext( msg_ctxt_id, msgid_plural, n );
#else
    const char *const translation = dcngettext( nullptr, msg_ctxt_id, msgid_plural, n, LC_MESSAGES );
#endif
    if( translation == msg_ctxt_id ) {
        return n == 1 ? msgid : msgid_plural;
    } else {
        return translation;
    }
}

#endif // LOCALIZE

std::string gettext_gendered( const GenderMap &genders, const std::string &msg )
{
    const std::vector<std::string> &language_genders = get_language().genders;

    std::vector<std::string> chosen_genders;
    for( const auto &subject_genders : genders ) {
        // default if no match
        std::string chosen_gender = language_genders[0];
        for( const std::string &gender : subject_genders.second ) {
            if( std::find( language_genders.begin(), language_genders.end(), gender ) !=
                language_genders.end() ) {
                chosen_gender = gender;
                break;
            }
        }
        chosen_genders.push_back( subject_genders.first + ":" + chosen_gender );
    }
    std::string context = join( chosen_genders, " " );
    return pgettext( context.c_str(), msg.c_str() );
}

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
    return { raw };
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

void translation::deserialize( JsonIn &jsin )
{
    // reset the cache
    cached_language_version = INVALID_LANGUAGE_VERSION;
    cached_translation = nullptr;

#ifndef CATA_IN_TOOL
    bool check_style = false;
    std::function<void( const std::string &msg, int offset )> log_error;
    bool auto_plural = false;
    bool is_str_sp = false;
#endif
    if( jsin.test_string() ) {
#ifndef CATA_IN_TOOL
        if( test_mode ) {
            const int origin = jsin.tell();
            check_style = true;
            log_error = [&jsin, origin]( const std::string & msg, const int offset ) {
                try {
                    jsin.seek( origin );
                    jsin.error( msg, offset );
                } catch( const JsonError &e ) {
                    debugmsg( "\n%s", e.what() );
                }
            };
        }
#endif
        ctxt = nullptr;
        raw = jsin.get_string();
        // if plural form is enabled
        if( raw_pl ) {
            raw_pl = cata::make_value<std::string>( raw + "s" );
            auto_plural = true;
        }
        needs_translation = true;
    } else {
        JsonObject jsobj = jsin.get_object();
        if( jsobj.has_string( "ctxt" ) ) {
            ctxt = cata::make_value<std::string>( jsobj.get_string( "ctxt" ) );
        } else {
            ctxt = nullptr;
        }
        if( jsobj.has_member( "str_sp" ) ) {
            // same singular and plural forms
            raw = jsobj.get_string( "str_sp" );
            is_str_sp = true;
            // if plural form is enabled
            if( raw_pl ) {
                raw_pl = cata::make_value<std::string>( raw );
            } else {
                try {
                    jsobj.throw_error( "str_sp not supported here", "str_sp" );
                } catch( const JsonError &e ) {
                    debugmsg( "\n%s", e.what() );
                }
            }
        } else {
            raw = jsobj.get_string( "str" );
            // if plural form is enabled
            if( raw_pl ) {
                if( jsobj.has_string( "str_pl" ) ) {
                    raw_pl = cata::make_value<std::string>( jsobj.get_string( "str_pl" ) );
                } else {
                    raw_pl = cata::make_value<std::string>( raw + "s" );
                    auto_plural = true;
                }
            } else if( jsobj.has_string( "str_pl" ) ) {
                try {
                    jsobj.throw_error( "str_pl not supported here", "str_pl" );
                } catch( const JsonError &e ) {
                    debugmsg( "\n%s", e.what() );
                }
            }
        }
        needs_translation = true;
#ifndef CATA_IN_TOOL
        if( test_mode ) {
            check_style = !jsobj.has_member( "//NOLINT(cata-text-style)" );
            // Copying jsobj to avoid use-after-free
            log_error = [jsobj]( const std::string & msg, const int offset ) {
                try {
                    if( jsobj.has_member( "str" ) ) {
                        jsobj.get_raw( "str" )->error( msg, offset );
                    } else {
                        jsobj.get_raw( "str_sp" )->error( msg, offset );
                    }
                } catch( const JsonError &e ) {
                    debugmsg( "\n%s", e.what() );
                }
            };
        }
#endif
    }
#ifndef CATA_IN_TOOL
    // Check text style in translatable json strings.
    if( test_mode && check_style ) {
        if( raw_pl && !auto_plural && *raw_pl == raw + "s" ) {
            log_error( "\"str_pl\" is not necessary here since the "
                       "plural form can be automatically generated.",
                       1 + raw.length() );
        }
        if( !is_str_sp && raw_pl && !auto_plural && raw == *raw_pl ) {
            log_error( "Please use \"str_sp\" instead of \"str\" and \"str_pl\" "
                       "for text with identical singular and plural forms",
                       1 + raw.length() );
        }
        if( !raw_pl ) {
            // Check for punctuation and spacing. Strings with plural forms are
            // curently simple names, for which these checks are not necessary.
            const auto text_style_callback =
                [&log_error]
                ( const text_style_fix type, const std::string & msg,
                  const std::u32string::const_iterator & beg, const std::u32string::const_iterator & /*end*/,
                  const std::u32string::const_iterator & /*at*/,
                  const std::u32string::const_iterator & from, const std::u32string::const_iterator & to,
                  const std::string & fix
            ) {
                std::string err;
                switch( type ) {
                    case text_style_fix::removal:
                        err = msg + "\n"
                              + "    Suggested fix: remove \"" + utf32_to_utf8( std::u32string( from, to ) ) + "\"\n"
                              + "    At the following position (marked with caret)";
                        break;
                    case text_style_fix::insertion:
                        err = msg + "\n"
                              + "    Suggested fix: insert \"" + fix + "\"\n"
                              + "    At the following position (marked with caret)";
                        break;
                    case text_style_fix::replacement:
                        err = msg + "\n"
                              + "    Suggested fix: replace \"" + utf32_to_utf8( std::u32string( from, to ) )
                              + "\" with \"" + fix + "\"\n"
                              + "    At the following position (marked with caret)";
                        break;
                }
                const std::string str_before = utf32_to_utf8( std::u32string( beg, to ) );
                // +1 for the starting quotation mark
                // TODO: properly handle escape sequences inside strings, instead
                // of using `length()` here.
                log_error( err, 1 + str_before.length() );
            };

            const std::u32string raw32 = utf8_to_utf32( raw );
            text_style_check<std::u32string::const_iterator>( raw32.cbegin(), raw32.cend(),
                    fix_end_of_string_spaces::yes, escape_unicode::no, text_style_callback );
        }
    }
#endif
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
    if( cached_language_version != current_language_version ||
        ( raw_pl && cached_num != num ) || !cached_translation ) {
        cached_language_version = current_language_version;
        cached_num = num;

        if( !ctxt ) {
            if( !raw_pl ) {
                cached_translation = cata::make_value<std::string>( detail::_translate_internal( raw ) );
            } else {
                cached_translation = cata::make_value<std::string>(
                                         ngettext( raw.c_str(), raw_pl->c_str(), num ) );
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

std::pair<bool, int> translation::legacy_hash() const
{
    if( needs_translation && !ctxt && !raw_pl ) {
        return {true, djb2_hash( reinterpret_cast<const unsigned char *>( raw.c_str() ) )};
    }
    // Otherwise the translation must have been added after snippets were changed
    // to use string ids only, so the translation doesn't have a legacy hash value.
    return {false, 0};
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
