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
#endif // LOCALIZE

#include "cached_options.h"
#include "debug.h"
#include "name.h"
#include "options.h"
#include "output.h"
#include "path_info.h"
#include "translations.h"
#if defined(LOCALIZE)
#  include "json.h"
#  include "ui_manager.h"
#endif

const std::string &to_valid_language( const std::string &lang );
bool isValidLanguage( const std::string &lang );
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

static language_info fallback_language = { "en", R"(English)", "en_US.UTF-8", "", { 1033 } };

std::vector<language_info> lang_options = {
    // Note: language names are in their own language and are *not* translated at all.
    // Note: Somewhere in Github PR was better link to msdn.microsoft.com with language names.
    // http://en.wikipedia.org/wiki/List_of_language_names
    // "Useful" links for LCID numbers:
    // https://www.science.co.il/language/Locale-codes.php
    // https://support.microsoft.com/de-de/help/193080/how-to-use-the-getuserdefaultlcid-windows-api-function-to-determine-op
    // https://msdn.microsoft.com/en-us/library/cc233965.aspx
    { "en", R"(English)", "en_US.UTF-8", "", {{ 1033, 2057, 3081, 4105, 5129, 6153, 7177, 8201, 9225, 10249, 11273 }} },
#if defined(LOCALIZE)
    { "de", R"(Deutsch)", "de_DE.UTF-8", "", {{ 1031, 2055, 3079, 4103, 5127 }} },
    { "es_AR", R"(Español (Argentina))", "es_AR.UTF-8", "", { 11274 } },
    { "es_ES", R"(Español (España))", "es_ES.UTF-8", "", {{ 1034, 2058, 3082, 4106, 5130, 6154, 7178, 8202, 9226, 10250, 12298, 13322, 14346, 15370, 16394, 17418, 18442, 19466, 20490 }} },
    { "fr", R"(Français)", "fr_FR.UTF-8", "", {{ 1036, 2060, 3084, 4108, 5132 }} },
    { "hu", R"(Magyar)", "hu_HU.UTF-8", "", { 1038 } },
    { "ja", R"(日本語)", "ja_JP.UTF-8", "", { 1041 } },
    { "ko", R"(한국어)", "ko_KR.UTF-8", "", { 1042 } },
    { "pl", R"(Polski)", "pl_PL.UTF-8", "", { 1045 } },
    { "pt_BR", R"(Português (Brasil))", "pt_BR.UTF-8", "", {{ 1046, 2070 }} },
    { "ru", R"(Русский)", "ru_RU.UTF-8", "", { 1049 } },
    { "zh_CN", R"(中文 (天朝))", "zh_CN.UTF-8", "zh_Hans", {{ 2052, 3076, 4100 }} },
    { "zh_TW", R"(中文 (台灣))", "zh_TW.UTF-8", "zh_Hant", { 1028 } },
#endif // LOCALIZE
};

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

#if defined(LOCALIZE)
#if defined(MACOSX)
const std::string &getSystemUILang()
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

    /**
     * Handle special case for simplified/traditional Chinese. Simplified/Traditional
     * is actually denoted by the region code in older iterations of the
     * language codes, whereas now (at least on OS X) region is distinct.
     * That is, CDDA expects 'zh_CN' but OS X might give 'zh-Hans-CN'.
     */
    for( const language_info &info : lang_options ) {
        if( !info.osx.empty() && string_starts_with( lang_code, info.osx ) ) {
            return info.id;
        }
    }

    return to_valid_language( lang_code );
}
#endif // MACOSX

const std::string &to_valid_language( const std::string &lang )
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
    static std::string invalid_lang;
    return invalid_lang;
}

bool isValidLanguage( const std::string &lang )
{
    return !to_valid_language( lang ).empty();
}

#if defined(_WIN32)
static const std::string &getLangFromLCID( const int &lcid )
{
    for( const language_info &info : lang_options ) {
        for( int lang_lcid : info.lcids ) {
            if( lang_lcid == lcid ) {
                return info.id;
            }
        }
    }
    static std::string unknown;
    return unknown;
}

static const std::string &getSystemUILang()
{
    return getLangFromLCID( GetUserDefaultUILanguage() );
}
#elif !defined(MACOSX)
// Linux / Android
static const std::string &getSystemUILang()
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

static void select_language()
{
    auto languages = get_options().get_option( "USE_LANG" ).getItems();

    languages.erase( std::remove_if( languages.begin(),
    languages.end(), []( const options_manager::id_and_option & lang ) {
        return lang.first.empty() || lang.second.empty();
    } ), languages.end() );

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
        DebugLog( D_WARNING, D_MAIN ) << "Can't set 'LANGUAGE' environment variable";
    } else {
        const auto env = getenv( "LANGUAGE" );
        if( env != nullptr ) {
            DebugLog( D_INFO, D_MAIN ) << "[lang] Language is set to: '" <<
                                       lang_opt << "'/'" << env << '\'';
        } else {
            DebugLog( D_WARNING, D_MAIN ) << "Can't get 'LANGUAGE' environment variable";
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
    DebugLog( D_INFO, DC_ALL ) << "[lang] C locale on startup: " << sys_c_locale;
    DebugLog( D_INFO, DC_ALL ) << "[lang] C++ locale on startup: " << sys_cpp_locale;

    // TODO: scan for languages like we do for tilesets.

#if defined(LOCALIZE)
    std::string lang = getSystemUILang();
    if( lang.empty() ) {
        system_language = nullptr;
        DebugLog( D_WARNING, DC_ALL ) << "[lang] Failed to detect system UI language.";
    } else {
        system_language = get_lang_info( lang );
        DebugLog( D_INFO, DC_ALL ) << "[lang] Detected system UI language as '" << lang << "'";
    }
#else // LOCALIZE
    system_language = &fallback_language;
#endif // LOCALIZE

    return true;
}

void prompt_select_lang_on_startup()
{
    if( get_option<std::string>( "USE_LANG" ).empty() && !system_language ) {
        select_language();
        set_language();
    }
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
        DebugLog( D_WARNING, D_MAIN ) << "[lang] Error while setlocale(LC_ALL, '.1252').";
    }
#else // _WIN32
    std::string lang = ::get_option<std::string>( "USE_LANG" );

    if( lang.empty() ) {
        // Restore user locale
        setlocale( LC_ALL, sys_c_locale.c_str() );
        try {
            std::locale::global( std::locale( sys_cpp_locale ) );
        } catch( std::runtime_error &e ) {
            std::locale::global( std::locale() );
        }
    } else {
        // Overwrite user locale
        try {
            std::locale::global( std::locale( get_lang_info( lang )->locale ) );
        } catch( std::runtime_error &e ) {
            std::locale::global( std::locale() );
        }
    }
#endif // _WIN32

    DebugLog( D_INFO, DC_ALL ) << "[lang] C locale set to " << setlocale( LC_ALL, nullptr );
    DebugLog( D_INFO, DC_ALL ) << "[lang] C++ locale set to " << std::locale().name();
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
