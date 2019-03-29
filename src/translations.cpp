#include "translations.h"

#if defined(LOCALIZE) && defined(__STRICT_ANSI__)
#undef __STRICT_ANSI__ // _putenv in minGW need that
#include <cstdlib>

#define __STRICT_ANSI__
#endif

#include <algorithm>
#include <set>
#include <string>

#include "cata_utility.h"
#include "json.h"
#include "name.h"
#include "path_info.h"

// Names depend on the language settings. They are loaded from different files
// based on the currently used language. If that changes, we have to reload the
// names.
static void reload_names()
{
    Name::clear();
    Name::load_from_file( PATH_INFO::find_translated_file( "namesdir", ".json", "names" ) );
}

#ifdef LOCALIZE
#include <cstdlib> // for getenv()/setenv()/putenv()

#include "options.h"
#include "debug.h"
#include "ui.h"
#if (defined _WIN32 || defined WINDOWS)
#include "platform_win.h"
#include "mmsystem.h"
#endif

#if (defined MACOSX)
#include <CoreFoundation/CFLocale.h>
#include <CoreFoundation/CoreFoundation.h>

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
#ifdef __ANDROID__
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
                       const char *const msgid_plural, const unsigned long int n )
{
    const std::string context_id = std::string( context ) + '\004' + msgid;
    const char *const msg_ctxt_id = context_id.c_str();
#ifdef __ANDROID__
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
#if (defined _WIN32 || defined WINDOWS)
    win_or_mac_lang = getLangFromLCID( GetUserDefaultLCID() );
#endif
#if (defined MACOSX)
    win_or_mac_lang = getOSXSystemLang();
#endif
    // Step 1. Setup locale settings.
    std::string lang_opt = get_option<std::string>( "USE_LANG" ).empty() ? win_or_mac_lang :
                           get_option<std::string>( "USE_LANG" );
    if( !lang_opt.empty() ) { // Not 'System Language'
        // Overwrite all system locale settings. Use CDDA settings. User wants this.
#if (defined _WIN32 || defined WINDOWS)
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

#if (defined _WIN32 || defined WINDOWS)
    // Use the ANSI code page 1252 to work around some language output bugs.
    if( setlocale( LC_ALL, ".1252" ) == nullptr ) {
        DebugLog( D_WARNING, D_MAIN ) << "Error while setlocale(LC_ALL, '.1252').";
    }
#endif

    // Step 2. Bind to gettext domain.
    std::string locale_dir;
#if defined __ANDROID__
    // Since we're using libintl-lite instead of libintl on Android, we hack the locale_dir to point directly to the .mo file.
    // This is because of our hacky libintl-lite bindtextdomain() implementation.
    auto env = getenv( "LANGUAGE" );
    locale_dir = std::string( FILENAMES["base_path"] + "lang/mo/" + ( env ? env : "none" ) +
                              "/LC_MESSAGES/cataclysm-dda.mo" );
#elif (defined __linux__ || (defined MACOSX && !defined TILES))
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
}

#if (defined MACOSX)
std::string getOSXSystemLang()
{
    // Get the user's language list (in order of preference)
    CFArrayRef langs = CFLocaleCopyPreferredLanguages();
    if( CFArrayGetCount( langs ) == 0 ) {
        return "en_US";
    }

    const char *lang_code_raw = CFStringGetCStringPtr(
                                    ( CFStringRef )CFArrayGetValueAtIndex( langs, 0 ),
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

translation::translation()
    : ctxt( cata::nullopt )
{
}

translation::translation( const std::string &ctxt, const std::string &raw )
    : ctxt( ctxt ), raw( raw ), needs_translation( true )
{
}

translation::translation( const std::string &raw )
    : ctxt( cata::nullopt ), raw( raw ), needs_translation( true )
{
}

translation::translation( const std::string &str, const no_translation_tag )
    : ctxt( cata::nullopt ), raw( str )
{
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
        needs_translation = true;
    } else {
        JsonObject jsobj = jsin.get_object();
        if( jsobj.has_string( "ctxt" ) ) {
            ctxt = jsobj.get_string( "ctxt" );
        } else {
            ctxt = cata::nullopt;
        }
        raw = jsobj.get_string( "str" );
        needs_translation = true;
    }
}

std::string translation::translated() const
{
    if( !needs_translation || raw.empty() ) {
        return raw;
    } else if( !ctxt ) {
        return _( raw.c_str() );
    } else {
        return pgettext( ctxt->c_str(), raw.c_str() );
    }
}

bool translation::empty() const
{
    return raw.empty();
}

bool translation::operator<( const translation &that ) const
{
    return translated() < that.translated();
}

bool translation::operator==( const translation &that ) const
{
    return translated() == that.translated();
}

bool translation::operator!=( const translation &that ) const
{
    return !operator==( that );
}

translation no_translation( const std::string &str )
{
    return translation::no_translation( str );
}
