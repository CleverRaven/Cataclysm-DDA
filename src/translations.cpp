#if defined(LOCALIZE) && defined(__STRICT_ANSI__)
#undef __STRICT_ANSI__ // _putenv in minGW need that
#include <stdlib.h>
#define __STRICT_ANSI__
#endif

#include "translations.h"

#include <string>
#include <algorithm>

#ifdef LOCALIZE
#include <stdlib.h> // for getenv()/setenv()/putenv()
#include "options.h"
#include "path_info.h"
#include "debug.h"
#include "ui.h"

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
    const char *translation = dcgettext( NULL, msg_ctxt_id, LC_MESSAGES );
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
    const char *const translation = dcngettext( nullptr, msg_ctxt_id, msgid_plural, n, LC_MESSAGES );
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
    languages.end(), [&lang]( const std::pair<std::string, std::string> &pair ) {
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
    languages.end(), []( const std::pair<std::string, std::string> &lang ) {
        return lang.first.empty() || lang.second.empty();
    } ), languages.end() );

    wrefresh( stdscr );

    uimenu sm;
    sm.selected = 0;
    sm.text = _( "Select your language" );
    for( size_t i = 0; i < languages.size(); i++ ) {
        sm.addentry( i, true, MENU_AUTOASSIGN, languages[i].second );
    }
    sm.query();

    get_options().get_option( "USE_LANG" ).setValue( languages[sm.ret].first );
    get_options().save();
}

void set_language()
{
    std::string win_lang = "";
#if (defined _WIN32 || defined WINDOWS)
    win_lang = getLangFromLCID( GetUserDefaultLCID() );
#endif
    // Step 1. Setup locale settings.
    std::string lang_opt = get_option<std::string>( "USE_LANG" ).empty() ? win_lang :
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
            auto env = getenv( "LANGUAGE" );
            if( env != NULL ) {
                DebugLog( D_INFO, D_MAIN ) << "Language is set to: '" << env << '\'';
            } else {
                DebugLog( D_WARNING, D_MAIN ) << "Can't get 'LANGUAGE' environment variable";
            }
        }
    }

#if (defined _WIN32 || defined WINDOWS)
    // Use the ANSI code page 1252 to work around some language output bugs.
    if( setlocale( LC_ALL, ".1252" ) == NULL ) {
        DebugLog( D_WARNING, D_MAIN ) << "Error while setlocale(LC_ALL, '.1252').";
    }
#endif

    // Step 2. Bind to gettext domain.
    std::string locale_dir;
#if (defined __linux__ || (defined MACOSX && !defined TILES))
    if( !FILENAMES["base_path"].empty() ) {
        locale_dir = FILENAMES["base_path"] + "share/locale";
    } else {
        locale_dir = "lang/mo";
    }
#else
    locale_dir = "lang/mo";
#endif // __linux__

    const char *locale_dir_char = locale_dir.c_str();
    bindtextdomain( "cataclysm-dda", locale_dir_char );
    bind_textdomain_codeset( "cataclysm-dda", "UTF-8" );
    textdomain( "cataclysm-dda" );
}

#else // !LOCALIZE

#include <cstring> // strcmp
#include <map>

bool isValidLanguage( const std::string &lang )
{
    return true;
}

std::string getLangFromLCID( const int &lcid )
{
    return "";
}

void select_language()
{
    return;
}

void set_language()
{
    return;
}

// sanitized message cache
std::map<std::string, std::string> &sanitized_messages()
{
    static std::map<std::string, std::string> sanitized_messages;
    return sanitized_messages;
}

const char *strip_positional_formatting( const char *msgid )
{
    // first check if we have it cached
    std::string s( msgid );
    auto iter = sanitized_messages().find( s );
    if( iter != sanitized_messages().end() ) {
        return iter->second.c_str();
    }

    // basic usage is just to change all "%{number}$" to "%".
    // thus for example "%2$s" will change to simply "%s".
    // strings must have their parameters in strict order,
    // or else this will not work correctly.
    size_t pos = 0;
    size_t len = s.length();
    while( pos < len ) {
        pos = s.find( '%', pos );
        if( pos == std::string::npos || pos + 2 >= len ) {
            break;
        }
        size_t dollarpos = pos + 1;
        while( dollarpos < len && s[dollarpos] >= '0' && s[dollarpos] <= '9' ) {
            ++dollarpos;
        }
        if( dollarpos >= len ) {
            break;
        }
        if( s[dollarpos] != '$' ) {
            pos = dollarpos + 1; // skip format type (also skips %%)
            continue;
        }
        s.erase( pos + 1, dollarpos - pos );
        len = s.length(); // because it ain't da same no more
        ++pos;
    }

    std::string &ret_msg = sanitized_messages()[std::string( msgid )];
    ret_msg = s;
    return ret_msg.c_str();
}

#endif // LOCALIZE
