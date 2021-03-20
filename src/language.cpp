#include "language.h"

#if defined(LOCALIZE) && defined(__STRICT_ANSI__)
#  undef __STRICT_ANSI__ // _putenv in minGW need that
#  include <cstdlib>
#  define __STRICT_ANSI__
#endif

#if defined(LOCALIZE)
#  if defined(_WIN32)
#  if 1 // Prevent IWYU reordering platform_win.h below mmsystem.h
#    include "platform_win.h"
#  endif
#    include "mmsystem.h"
#  endif // _WIN32
#  if defined(MACOSX)
#    include <CoreFoundation/CFLocale.h>
#    include <CoreFoundation/CoreFoundation.h>
#  endif
#
#  include <libintl.h>
#endif // LOCALIZE

#include "cached_options.h"
#include "debug.h"
#include "name.h"
#include "options.h"
#include "output.h"
#include "path_info.h"
#include "translations.h"
#include "ui.h"
#if defined(LOCALIZE)
#  include "json.h"
#  include "ui_manager.h"
#endif

std::string to_valid_language( const std::string &lang );
void update_global_locale();

static std::string sys_c_locale;
static std::string sys_cpp_locale;

// System user preferred UI language (nullptr if failed to detect)
static language_info const *system_language = nullptr;

// Cached current game language.
// Unlike USE_LANG option value, it's synchronized with whatever language
// gettext should be using.
// May be nullptr if language hasn't been set yet.
static language_info const *current_language = nullptr;

static language_info fallback_language = { "en", R"(English)", "en_US.UTF-8", { "n" }, "", { 1033 } };

std::vector<language_info> lang_options = { fallback_language };

static language_info const *get_lang_info( const std::string &lang )
{
    for( const language_info &li : lang_options ) {
        if( li.id == lang ) {
            return &li;
        }
    }
    return &fallback_language;
}

const std::vector<language_info> &list_available_languages()
{
    return lang_options;
}

// Names depend on the language settings. They are loaded from different files
// based on the currently used language. If that changes, we have to reload the
// names.
static void reload_names()
{
    Name::clear();
    Name::load_from_file( PATH_INFO::names() );
}


// A tiny hack to make debug output look nice when running unit tests
// (DebugLog does not append newlines, so we have to do it ourselves).
#define dbg(lvl, ...) \
    if (!test_mode || lvl & D_ERROR ) { \
        DebugLog(lvl, D_MAIN) << string_format(__VA_ARGS__); \
    } else { \
        cata_printf(__VA_ARGS__); \
        cata_printf("\n"); \
    }

#if defined(LOCALIZE)
#if defined(MACOSX)
static std::string getSystemUILang()
{
    // Get the user's language list (in order of preference)
    CFArrayRef langs = CFLocaleCopyPreferredLanguages();
    if( CFArrayGetCount( langs ) == 0 ) {
        return "";
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
            return "";
        }
        lang_code = lang_code_raw_slow.data();
    }

    // Convert to the underscore format expected by gettext
    std::replace( lang_code.begin(), lang_code.end(), '-', '_' );

    for( const language_info &info : lang_options ) {
        if( !info.osx.empty() && string_starts_with( lang_code, info.osx ) ) {
            return info.id;
        }
    }

    return to_valid_language( lang_code );
}
#endif // MACOSX

std::string to_valid_language( const std::string &lang )
{
    if( lang.empty() ) {
        return lang;
    }
    for( const language_info &info : lang_options ) {
        if( string_starts_with( lang, info.id ) ) {
            return info.id;
        }
    }
    const size_t p = lang.find( '_' );
    if( p != std::string::npos ) {
        std::string lang2 = lang.substr( 0, p );
        for( const language_info &info : lang_options ) {
            if( string_starts_with( lang2, info.id ) ) {
                return info.id;
            }
        }
    }
    return "";
}

#if defined(_WIN32)
static std::string getLangFromLCID( const int &lcid )
{
    for( const language_info &info : lang_options ) {
        for( int lang_lcid : info.lcids ) {
            if( lang_lcid == lcid ) {
                return info.id;
            }
        }
    }
    return "";
}

static std::string getSystemUILang()
{
    return getLangFromLCID( GetUserDefaultUILanguage() );
}
#elif !defined(MACOSX)
// Linux / Android
static std::string getSystemUILang()
{
    std::string ret;

    const char *language = getenv( "LANGUAGE" );
    if( language && language[0] != '\0' ) {
        ret = language;
    } else {
        const char *loc = setlocale( LC_MESSAGES, nullptr );
        if( loc != nullptr ) {
            ret = loc;
        }
    }

    if( ret == "C" || string_starts_with( ret, "C." ) ) {
        ret = "en";
    }

    return to_valid_language( ret );
}
#endif // _WIN32 / !MACOSX

static bool cata_setenv( const std::string &name, const std::string &value )
{
#if defined(_WIN32)
    std::string s = name + "=" + value;
    return _putenv( s.c_str() ) == 0;
#else
    return setenv( name.c_str(), value.c_str(), true ) == 0;
#endif
}

void set_language()
{
    // Step 1. Choose language id
    std::string lang_opt = get_option<std::string>( "USE_LANG" );
    if( lang_opt.empty() ) {
        if( system_language ) {
            lang_opt = system_language->id;
            current_language = system_language;
        } else {
            lang_opt = fallback_language.id;
            current_language = &fallback_language;
        }
    } else {
        current_language = get_lang_info( lang_opt );
    }

    // Step 2. Setup locale & environment variables.
    // By default, gettext uses current locale to determine which language to use.
    // Since locale for desired language may be missing from user system,
    // we need to explicitly specify it.
    if( !cata_setenv( "LANGUAGE", lang_opt ) ) {
        dbg( D_WARNING, "Can't set 'LANGUAGE' environment variable" );
    } else {
        const auto env = getenv( "LANGUAGE" );
        if( env != nullptr ) {
            dbg( D_INFO, "[lang] Language is set to: '%s'/'%s'", lang_opt, env );
        } else {
            dbg( D_WARNING, "Can't get 'LANGUAGE' environment variable" );
        }
    }
    update_global_locale();

    // Step 3. Bind to gettext domain.
    std::string locale_dir;
#if defined(__ANDROID__)
    // HACK: Since we're using libintl-lite instead of libintl on Android, we hack the locale_dir to point directly to the .mo file.
    // This is because of our hacky libintl-lite bindtextdomain() implementation.
    auto env = getenv( "LANGUAGE" );
    locale_dir = std::string( PATH_INFO::base_path() + "lang/mo/" + ( env ? env : "none" ) +
                              "/LC_MESSAGES/cataclysm-bn.mo" );
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
    bindtextdomain( "cataclysm-bn", locale_dir_char );
    bind_textdomain_codeset( "cataclysm-bn", "UTF-8" );
    textdomain( "cataclysm-bn" );

    // Step 4. Finalize
    invalidate_translations();
    reload_names();
}

static std::vector<language_info> load_languages( const std::string &filepath )
{
    std::vector<language_info> ret;
    try {
        std::ifstream stream( filepath, std::ios_base::binary );
        if( !stream.is_open() ) {
            throw std::runtime_error( string_format( "File '%s' not found", filepath ) );
        }
        JsonIn json( stream );
        JsonArray arr = json.get_array();
        for( const JsonObject &obj : arr ) {
            language_info info;
            info.id = obj.get_string( "id" );
            info.name = obj.get_string( "name" );
            info.locale = obj.get_string( "locale" );
            info.genders = obj.get_string_array( "genders" );
            info.osx = obj.get_string( "osx", "" );
            info.lcids = obj.get_int_array( "lcids" );
            ret.push_back( info );
        }
    } catch( const std::exception &e ) {
        debugmsg( "[lang] Failed to read language definitions: %s", e.what() );
        return std::vector<language_info>();
    }

    // Sanity check genders
    const std::vector<std::string> all_genders = {{"f", "m", "n"}};

    for( language_info &info : ret ) {
        for( const std::string &g : info.genders ) {
            if( find( all_genders.begin(), all_genders.end(), g ) == all_genders.end() ) {
                debugmsg( "Unexpected gender '%s' in grammatical gender list for language '%d'",
                          g, info.id );
            }
        }
        if( info.genders.empty() ) {
            info.genders.push_back( "n" );
        }
    }

    return ret;
}
#else // !LOCALIZE

void set_language()
{
    current_language = &fallback_language;
    update_global_locale();
    reload_names();
    return;
}

#endif // LOCALIZE

bool init_language_system()
{
    // OS X does not populate locale env vars correctly
    // (they usually default to "C") so don't bother
    // trying to set the locale based on them.
#if !defined(MACOSX)
    if( setlocale( LC_ALL, "" ) == nullptr ) {
        DebugLog( D_WARNING, D_MAIN ) << "[lang] Error while setlocale(LC_ALL, '').";
    } else {
#endif
        try {
            std::locale::global( std::locale( "" ) );
        } catch( const std::exception & ) {
            // if user default locale retrieval isn't implemented by system
            try {
                // default to basic C locale
                std::locale::global( std::locale::classic() );
            } catch( const std::exception &err ) {
                DebugLog( D_ERROR, D_MAIN ) << err.what();
                return false;
            }
        }
#if !defined(MACOSX)
    }
#endif

    sys_c_locale = setlocale( LC_ALL, nullptr );
    sys_cpp_locale = std::locale().name();
    DebugLog( D_INFO, D_MAIN ) << "[lang] C locale on startup: " << sys_c_locale;
    DebugLog( D_INFO, D_MAIN ) << "[lang] C++ locale on startup: " << sys_cpp_locale;

#if defined(LOCALIZE)
    lang_options = load_languages( PATH_INFO::language_defs_file() );
    if( lang_options.empty() ) {
        lang_options = { fallback_language };
    }

    std::string lang = getSystemUILang();
    if( lang.empty() ) {
        system_language = nullptr;
        DebugLog( D_WARNING, D_MAIN ) << "[lang] Failed to detect system UI language.";
    } else {
        system_language = get_lang_info( lang );
        DebugLog( D_INFO, D_MAIN ) << "[lang] Detected system UI language as '" << lang << "'";
    }
#else // LOCALIZE
    system_language = &fallback_language;
    lang_options = { fallback_language };
#endif // LOCALIZE

    return true;
}

void prompt_select_lang_on_startup()
{
    if( !get_option<std::string>( "USE_LANG" ).empty() || system_language ) {
        return;
    }

    std::string res;

    if( lang_options.empty() ) {
        res = fallback_language.id;
    } else if( lang_options.size() == 1 ) {
        res = lang_options[0].id;
    } else {
        uilist sm;
        sm.allow_cancel = false;
        sm.text = _( "Select your language" );
        for( size_t i = 0; i < lang_options.size(); i++ ) {
            sm.addentry( i, true, MENU_AUTOASSIGN, lang_options[i].name );
        }
        sm.query();
        res = lang_options[sm.ret].id;
    }

    get_options().get_option( "USE_LANG" ).setValue( res );
    get_options().save();

    set_language();
}

const language_info &get_language()
{
    if( current_language ) {
        return *current_language;
    } else {
        return fallback_language;
    }
}

void update_global_locale()
{
#if defined(_WIN32)
    // Use the ANSI code page 1252 to work around some language output bugs.
    if( setlocale( LC_ALL, ".1252" ) == nullptr ) {
        dbg( D_WARNING, "[lang] Error while setlocale(LC_ALL, '.1252')." );
    }
#else // _WIN32
    std::string lang = ::get_option<std::string>( "USE_LANG" );

    bool set_user = false;
    if( lang.empty() ) {
        // Restore user locale
        set_user = true;
    } else {
        // Try specific locale
        try {
            std::locale::global( std::locale( get_lang_info( lang )->locale ) );
        } catch( std::runtime_error &e ) {
            // Try fairly neutral UTF-8 locale
            try {
                std::locale::global( std::locale( fallback_language.locale ) );
            } catch( std::runtime_error &e ) {
                // No choice left
                set_user = true;
            }
        }
    }
    if( set_user ) {
        setlocale( LC_ALL, sys_c_locale.c_str() );
        try {
            std::locale::global( std::locale( sys_cpp_locale ) );
        } catch( std::runtime_error &e ) {
            std::locale::global( std::locale::classic() );
        }
    }

#endif // _WIN32

    dbg( D_INFO, "[lang] C locale set to '%s'", setlocale( LC_ALL, nullptr ) );
    dbg( D_INFO, "[lang] C++ locale set to '%s'", std::locale().name() );
}

bool localized_comparator::operator()( const std::string &l, const std::string &r ) const
{
    // We need different implementations on each platform.  MacOS seems to not
    // support localized comparison of strings via the standard library at all,
    // so resort to MacOS-specific solution.  Windows cannot be expected to be
    // using a UTF-8 locale (whereas our strings are always UTF-8) and so we
    // must convert to wstring for comparison there.  Linux seems to work as
    // expected on regular strings; no workarounds needed.
    // See https://github.com/CleverRaven/Cataclysm-DDA/pull/40041 for further
    // discussion.
#if defined(MACOSX)
    CFStringRef lr = CFStringCreateWithCStringNoCopy( kCFAllocatorDefault, l.c_str(),
                     kCFStringEncodingUTF8, kCFAllocatorNull );
    CFStringRef rr = CFStringCreateWithCStringNoCopy( kCFAllocatorDefault, r.c_str(),
                     kCFStringEncodingUTF8, kCFAllocatorNull );
    bool result = CFStringCompare( lr, rr, kCFCompareLocalized ) < 0;
    CFRelease( lr );
    CFRelease( rr );
    return result;
#elif defined(_WIN32)
    return ( *this )( utf8_to_wstr( l ), utf8_to_wstr( r ) );
#else
    return std::locale()( l, r );
#endif
}

bool localized_comparator::operator()( const std::wstring &l, const std::wstring &r ) const
{
#if defined(MACOSX)
    return ( *this )( wstr_to_utf8( l ), wstr_to_utf8( r ) );
#else
    return std::locale()( l, r );
#endif
}
