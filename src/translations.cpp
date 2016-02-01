#include "translations.h"

#include <string>
#ifdef LOCALIZE
#undef __STRICT_ANSI__ // _putenv in minGW need that
#include <stdlib.h> // for getenv()/setenv()/putenv()
#include "options.h"
#include "path_info.h"
#include "debug.h"
#else // !LOCALIZE
#include <cstring> // strcmp
#include <map>
#endif // LOCALIZE


#ifdef LOCALIZE

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

void set_language( bool reload_options )
{
    // Step 1. Setup locale settings.
    std::string lang_opt = OPTIONS["USE_LANG"].getValue();
    if( lang_opt != "" ) { // Not 'System Language'
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
    const char *locale_dir;
#if (defined __linux__ || (defined MACOSX && !defined TILES))
    if( !FILENAMES["base_path"].empty() ) {
        locale_dir = std::string( FILENAMES["base_path"] + "share/locale" ).c_str();
    } else {
        locale_dir = "lang/mo";
    }
#else
    locale_dir = "lang/mo";
#endif // __linux__

    bindtextdomain( "cataclysm-dda", locale_dir );
    bind_textdomain_codeset( "cataclysm-dda", "UTF-8" );
    textdomain( "cataclysm-dda" );

    // Step 3. Reload options strings with right language
    if( reload_options ) {
        get_options().init();
        get_options().load();
    }
}
#else // !LOCALIZE
void set_language( bool reload_options )
{
    ( void ) reload_options; // Cancels MinGW warning on Windows
}

// sanitized message cache
std::map<const char *, std::string> &sanitized_messages()
{
    static std::map<const char *, std::string> sanitized_messages;
    return sanitized_messages;
}

const char *strip_positional_formatting( const char *msgid )
{
    // first check if we have it cached
    if( sanitized_messages().find( msgid ) != sanitized_messages().end() ) {
        if( sanitized_messages()[msgid] == "" ) {
            return msgid;
        } else {
            return sanitized_messages()[msgid].c_str();
        }
    }
    std::string s( msgid );
    // basic usage is just to change all "%{number}$" to "%".
    // thus for example "%2$s" will change to simply "%s".
    // strings must have their parameters in strict order,
    // or else this will not work correctly.
    size_t pos = 0;
    size_t len = s.length();
    bool changed = false;
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
        changed = true;
        ++pos;
    }

    if( !changed ) {
        sanitized_messages()[msgid] = "";
        return msgid;
    } else {
        sanitized_messages()[msgid] = s;
        return sanitized_messages()[msgid].c_str();
    }
    return msgid;
}

#endif // LOCALIZE
