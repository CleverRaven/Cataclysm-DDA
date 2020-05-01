#include "translations.h"

#include <array>
#include <clocale>
#include <cstdlib>
#include <functional>

#if defined(LOCALIZE) && defined(__STRICT_ANSI__)
#undef __STRICT_ANSI__ // _putenv in minGW need that
#include <cstdlib>

#define __STRICT_ANSI__
#endif

#include <algorithm>
#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "cata_utility.h"
#include "catacharset.h"
#include "cursesdef.h"
#include "json.h"
#include "name.h"
#include "output.h"
#include "path_info.h"
#include "rng.h"
#include "text_style_check.h"

extern bool test_mode;

// Names depend on the language settings. They are loaded from different files
// based on the currently used language. If that changes, we have to reload the
// names.
static void reload_names()
{
    Name::clear();
    Name::load_from_file( PATH_INFO::names() );
}

static bool sanity_checked_genders = false;

#if defined(LOCALIZE)
#include "debug.h"
#include "options.h"
#include "ui.h"
#if defined(_WIN32)
#if 1 // Prevent IWYU reordering platform_win.h below mmsystem.h
#   include "platform_win.h"
#endif
#   include "mmsystem.h"
#endif

#if defined(MACOSX)
#   include <CoreFoundation/CFLocale.h>
#   include <CoreFoundation/CoreFoundation.h>

#include "cata_utility.h"

std::string getOSXSystemLang();
#endif

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

bool isValidLanguage( const std::string &lang )
{
    const auto languages = get_options().get_option( "USE_LANG" ).getItems();
    return std::find_if( languages.begin(),
    languages.end(), [&lang]( const options_manager::id_and_option & pair ) {
        return pair.first == lang || pair.first == lang.substr( 0, pair.first.length() );
    } ) != languages.end();
}

/* "Useful" links:
 *  https://www.science.co.il/language/Locale-codes.php
 *  https://support.microsoft.com/de-de/help/193080/how-to-use-the-getuserdefaultlcid-windows-api-function-to-determine-op
 *  https://msdn.microsoft.com/en-us/library/cc233965.aspx
 */
std::string getLangFromLCID( const int &lcid )
{
    static std::map<std::string, std::set<int>> lang_lcid;
    if( lang_lcid.empty() ) {
        lang_lcid["en"] = {{ 1033, 2057, 3081, 4105, 5129, 6153, 7177, 8201, 9225, 10249, 11273 }};
        lang_lcid["fr"] = {{ 1036, 2060, 3084, 4108, 5132 }};
        lang_lcid["de"] = {{ 1031, 2055, 3079, 4103, 5127 }};
        lang_lcid["it_IT"] = {{ 1040, 2064 }};
        lang_lcid["es_AR"] = { 11274 };
        lang_lcid["es_ES"] = {{ 1034, 2058, 3082, 4106, 5130, 6154, 7178, 8202, 9226, 10250, 12298, 13322, 14346, 15370, 16394, 17418, 18442, 19466, 20490 }};
        lang_lcid["ja"] = { 1041 };
        lang_lcid["ko"] = { 1042 };
        lang_lcid["pl"] = { 1045 };
        lang_lcid["pt_BR"] = {{ 1046, 2070 }};
        lang_lcid["ru"] = { 1049 };
        lang_lcid["zh_CN"] = {{ 2052, 3076, 4100 }};
        lang_lcid["zh_TW"] = { 1028 };
    }

    for( auto &lang : lang_lcid ) {
        if( lang.second.find( lcid ) != lang.second.end() ) {
            return lang.first;
        }
    }
    return "";
}

void select_language()
{
    auto languages = get_options().get_option( "USE_LANG" ).getItems();

    languages.erase( std::remove_if( languages.begin(),
    languages.end(), []( const options_manager::id_and_option & lang ) {
        return lang.first.empty() || lang.second.empty();
    } ), languages.end() );

    wrefresh( catacurses::stdscr );

    uilist sm;
    sm.allow_cancel = false;
    sm.text = _( "Select your language" );
    for( size_t i = 0; i < languages.size(); i++ ) {
        sm.addentry( i, true, MENU_AUTOASSIGN, languages[i].second.translated() );
    }
    sm.query();

    get_options().get_option( "USE_LANG" ).setValue( languages[sm.ret].first );
    get_options().save();
}

void set_language()
{
    std::string win_or_mac_lang;
#if defined(_WIN32)
    win_or_mac_lang = getLangFromLCID( GetUserDefaultLCID() );
#endif
#if defined(MACOSX)
    win_or_mac_lang = getOSXSystemLang();
#endif
    // Step 1. Setup locale settings.
    std::string lang_opt = get_option<std::string>( "USE_LANG" ).empty() ? win_or_mac_lang :
                           get_option<std::string>( "USE_LANG" );
    if( !lang_opt.empty() ) {
        // Not 'System Language'
        // Overwrite all system locale settings. Use CDDA settings. User wants this.
#if defined(_WIN32)
        std::string lang_env = "LANGUAGE=" + lang_opt;
        if( _putenv( lang_env.c_str() ) != 0 ) {
            DebugLog( D_WARNING, D_MAIN ) << "Can't set 'LANGUAGE' environment variable";
        }
#else
        if( setenv( "LANGUAGE", lang_opt.c_str(), true ) != 0 ) {
            DebugLog( D_WARNING, D_MAIN ) << "Can't set 'LANGUAGE' environment variable";
        }
#endif
        else {
            const auto env = getenv( "LANGUAGE" );
            if( env != nullptr ) {
                DebugLog( D_INFO, D_MAIN ) << "Language is set to: '" << env << '\'';
            } else {
                DebugLog( D_WARNING, D_MAIN ) << "Can't get 'LANGUAGE' environment variable";
            }
        }
    }

#if defined(_WIN32)
    // Use the ANSI code page 1252 to work around some language output bugs.
    if( setlocale( LC_ALL, ".1252" ) == nullptr ) {
        DebugLog( D_WARNING, D_MAIN ) << "Error while setlocale(LC_ALL, '.1252').";
    }
#endif

    // Step 2. Bind to gettext domain.
    std::string locale_dir;
#if defined(__ANDROID__)
    // HACK: Since we're using libintl-lite instead of libintl on Android, we hack the locale_dir to point directly to the .mo file.
    // This is because of our hacky libintl-lite bindtextdomain() implementation.
    auto env = getenv( "LANGUAGE" );
    locale_dir = std::string( PATH_INFO::base_path() + "lang/mo/" + ( env ? env : "none" ) +
                              "/LC_MESSAGES/cataclysm-dda.mo" );
#elif (defined(__linux__) || (defined(MACOSX) && !defined(TILES)))
    if( !PATH_INFO::base_path().empty() ) {
        locale_dir = PATH_INFO::base_path() + "share/locale";
    } else {
        locale_dir = "lang/mo";
    }
#else
    locale_dir = "lang/mo";
#endif

    const char *locale_dir_char = locale_dir.c_str();
    bindtextdomain( "cataclysm-dda", locale_dir_char );
    bind_textdomain_codeset( "cataclysm-dda", "UTF-8" );
    textdomain( "cataclysm-dda" );

    reload_names();

    sanity_checked_genders = false;
}

#if defined(MACOSX)
std::string getOSXSystemLang()
{
    // Get the user's language list (in order of preference)
    CFArrayRef langs = CFLocaleCopyPreferredLanguages();
    if( CFArrayGetCount( langs ) == 0 ) {
        return "en_US";
    }

    CFStringRef lang = static_cast<CFStringRef>( CFArrayGetValueAtIndex( langs, 0 ) );
    const char *lang_code_raw_fast = CFStringGetCStringPtr( lang, kCFStringEncodingUTF8 );
    std::string lang_code;
    if( lang_code_raw_fast ) { // fast way, probably it's never works
        lang_code = lang_code_raw_fast;
    } else { // fallback to slow way
        CFIndex length = CFStringGetLength( lang ) + 1;
        std::vector<char> lang_code_raw_slow( length, '\0' );
        bool success = CFStringGetCString( lang, lang_code_raw_slow.data(), length, kCFStringEncodingUTF8 );
        if( !success ) {
            return "en_US";
        }
        lang_code = lang_code_raw_slow.data();
    }

    // Convert to the underscore format expected by gettext
    std::replace( lang_code.begin(), lang_code.end(), '-', '_' );

    /**
     * Handle special case for simplified/traditional Chinese. Simplified/Traditional
     * is actually denoted by the region code in older iterations of the
     * language codes, whereas now (at least on OS X) region is distinct.
     * That is, CDDA expects 'zh_CN' but OS X might give 'zh-Hans-CN'.
     */
    if( string_starts_with( lang_code, "zh_Hans" ) ) {
        return "zh_CN";
    } else if( string_starts_with( lang_code, "zh_Hant" ) ) {
        return "zh_TW";
    }

    return isValidLanguage( lang_code ) ? lang_code : "en_US";
}
#endif

#else // !LOCALIZE

#include <cstring> // strcmp
#include <map>

bool isValidLanguage( const std::string &/*lang*/ )
{
    return true;
}

std::string getLangFromLCID( const int &/*lcid*/ )
{
    return "";
}

void select_language()
{
    return;
}

void set_language()
{
    reload_names();
    return;
}

#endif // LOCALIZE

static void sanity_check_genders( const std::vector<std::string> &language_genders )
{
    if( sanity_checked_genders ) {
        return;
    }
    sanity_checked_genders = true;

    constexpr std::array<const char *, 3> all_genders = {{"f", "m", "n"}};

    for( const std::string &gender : language_genders ) {
        if( find( all_genders.begin(), all_genders.end(), gender ) == all_genders.end() ) {
            debugmsg( "Unexpected gender '%s' in grammatical gender list for "
                      "this language", gender );
        }
    }
}

std::string gettext_gendered( const GenderMap &genders, const std::string &msg )
{
    //~ Space-separated list of grammatical genders. Default should be first.
    //~ Use short names and try to be consistent between languages as far as
    //~ possible.  Current choices are m (male), f (female), n (neuter).
    //~ As appropriate we might add e.g. a (animate) or c (common).
    //~ New genders must be added to all_genders in lang/extract_json_strings.py
    //~ and src/translations.cpp.
    //~ The primary purpose of this is for NPC dialogue which might depend on
    //~ gender.  Only add genders to the extent needed by such translations.
    //~ They are likely only needed if they affect the first and second
    //~ person.  For example, one gender suffices for English even though
    //~ third person pronouns differ.
    std::string language_genders_s = pgettext( "grammatical gender list", "n" );
    std::vector<std::string> language_genders = string_split( language_genders_s, ' ' );

    sanity_check_genders( language_genders );

    if( language_genders.empty() ) {
        language_genders.push_back( "n" );
    }

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

translation::translation()
    : ctxt( cata::nullopt ), raw_pl( cata::nullopt )
{
}

translation::translation( const plural_tag )
    : ctxt( cata::nullopt ), raw_pl( std::string() )
{
}

translation::translation( const std::string &ctxt, const std::string &raw )
    : ctxt( ctxt ), raw( raw ), raw_pl( cata::nullopt ), needs_translation( true )
{
}

translation::translation( const std::string &raw )
    : ctxt( cata::nullopt ), raw( raw ), raw_pl( cata::nullopt ), needs_translation( true )
{
}

translation::translation( const std::string &raw, const std::string &raw_pl,
                          const plural_tag )
    : ctxt( cata::nullopt ), raw( raw ), raw_pl( raw_pl ), needs_translation( true )
{
}

translation::translation( const std::string &ctxt, const std::string &raw,
                          const std::string &raw_pl, const plural_tag )
    : ctxt( ctxt ), raw( raw ), raw_pl( raw_pl ), needs_translation( true )
{
}

translation::translation( const std::string &str, const no_translation_tag )
    : ctxt( cata::nullopt ), raw( str ), raw_pl( cata::nullopt )
{
}

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
            raw_pl = raw;
        }
    } else if( !raw_pl ) {
        // just mark plural form as enabled
        raw_pl = std::string();
    }
}

void translation::deserialize( JsonIn &jsin )
{
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
        ctxt = cata::nullopt;
        raw = jsin.get_string();
        // if plural form is enabled
        if( raw_pl ) {
            raw_pl = raw + "s";
            auto_plural = true;
        }
        needs_translation = true;
    } else {
        JsonObject jsobj = jsin.get_object();
        if( jsobj.has_string( "ctxt" ) ) {
            ctxt = jsobj.get_string( "ctxt" );
        } else {
            ctxt = cata::nullopt;
        }
        if( jsobj.has_member( "str_sp" ) ) {
            // same singular and plural forms
            raw = jsobj.get_string( "str_sp" );
            is_str_sp = true;
            // if plural form is enabled
            if( raw_pl ) {
                raw_pl = raw;
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
                    raw_pl = jsobj.get_string( "str_pl" );
                } else {
                    raw_pl = raw + "s";
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
        if( raw_pl && !auto_plural && raw_pl.value() == raw + "s" ) {
            log_error( "\"str_pl\" is not necessary here since the "
                       "plural form can be automatically generated.",
                       1 + raw.length() );
        }
        if( !is_str_sp && raw_pl && !auto_plural && raw == raw_pl.value() ) {
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
    } else if( !ctxt ) {
        if( !raw_pl ) {
            return _( raw );
        } else {
            return ngettext( raw.c_str(), raw_pl->c_str(), num );
        }
    } else {
        if( !raw_pl ) {
            return pgettext( ctxt->c_str(), raw.c_str() );
        } else {
            return npgettext( ctxt->c_str(), raw.c_str(), raw_pl->c_str(), num );
        }
    }
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
    return ctxt == that.ctxt && raw == that.raw && raw_pl == that.raw_pl &&
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

bool localized_comparator::operator()( const std::string &l, const std::string &r ) const
{
    return std::locale()( l, r );
}
