#include "translations.h"

#include <clocale>
#include <array>

#if defined(LOCALIZE) && defined(__STRICT_ANSI__)
#undef __STRICT_ANSI__ // _putenv in minGW need that
#include <cstdlib>

#define __STRICT_ANSI__
#endif

#include <algorithm>
#include <set>
#include <string>
#include <map>
#include <memory>
#include <ostream>
#include <utility>
#include <vector>

#include "json.h"
#include "name.h"
#include "output.h"
#include "path_info.h"
#include "cursesdef.h"
#include "cata_utility.h"

// Names depend on the language settings. They are loaded from different files
// based on the currently used language. If that changes, we have to reload the
// names.
static void reload_names()
{
    Name::clear();
    Name::load_from_file( PATH_INFO::find_translated_file( "namesdir", ".json", "names" ) );
}

static bool sanity_checked_genders = false;

#if defined(LOCALIZE)
#include "options.h"
#include "debug.h"
#include "ui.h"
#if defined(_WIN32)
#   include "platform_win.h"
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
    if( !lang_opt.empty() ) { // Not 'System Language'
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
    // Since we're using libintl-lite instead of libintl on Android, we hack the locale_dir to point directly to the .mo file.
    // This is because of our hacky libintl-lite bindtextdomain() implementation.
    auto env = getenv( "LANGUAGE" );
    locale_dir = std::string( FILENAMES["base_path"] + "lang/mo/" + ( env ? env : "none" ) +
                              "/LC_MESSAGES/cataclysm-dda.mo" );
#elif (defined(__linux__) || (defined(MACOSX) && !defined(TILES)))
    if( !FILENAMES["base_path"].empty() ) {
        locale_dir = FILENAMES["base_path"] + "share/locale";
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

    const char *lang_code_raw = CFStringGetCStringPtr(
                                    reinterpret_cast<CFStringRef>( CFArrayGetValueAtIndex( langs, 0 ) ),
                                    kCFStringEncodingUTF8 );
    if( !lang_code_raw ) {
        return "en_US";
    }

    // Convert to the underscore format expected by gettext
    std::string lang_code( lang_code_raw );
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
        std::string chosen_gender = language_genders[0]; // default if no match
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
    : ctxt( cata::nullopt ), raw(), raw_pl( cata::nullopt ), needs_translation( false )
{
}

translation::translation( const plural_tag )
    : ctxt( cata::nullopt ), raw(), raw_pl( std::string() ), needs_translation( false )
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
    : ctxt( cata::nullopt ), raw( str ), raw_pl( cata::nullopt ), needs_translation( false )
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

void translation::deserialize( JsonIn &jsin )
{
    if( jsin.test_string() ) {
        ctxt = cata::nullopt;
        raw = jsin.get_string();
        // if plural form is enabled
        if( raw_pl ) {
            raw_pl = raw + "s";
        }
        needs_translation = true;
    } else {
        JsonObject jsobj = jsin.get_object();
        if( jsobj.has_string( "ctxt" ) ) {
            ctxt = jsobj.get_string( "ctxt" );
        } else {
            ctxt = cata::nullopt;
        }
        raw = jsobj.get_string( "str" );
        // if plural form is enabled
        if( raw_pl ) {
            if( jsobj.has_string( "str_pl" ) ) {
                raw_pl = jsobj.get_string( "str_pl" );
            } else {
                raw_pl = raw + "s";
            }
        }
        needs_translation = true;
    }
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
    return translated() < that.translated();
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
