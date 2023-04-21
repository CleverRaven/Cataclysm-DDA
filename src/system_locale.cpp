#include <cstring>
#include <set>
#include <vector>

#if defined(_WIN32)
#if 1 // Prevent IWYU reordering platform_win.h below mmsystem.h
#   include "platform_win.h"
#endif
#   include "mmsystem.h"
#elif defined(__APPLE__)
#include <CoreFoundation/CFLocale.h>
#include <CoreFoundation/CoreFoundation.h>
#elif defined(__ANDROID__)
#include <jni.h>
#include "sdl_wrappers.h" // for SDL_AndroidGetJNIEnv()
#include "debug.h" // for DebugLog/D_INFO/D_MAIN
#elif defined(__linux__)
#include <langinfo.h>
#endif

#include "cata_utility.h"
#include "options.h"
#include "system_locale.h"

#ifndef _WIN32
namespace
{
// Try to match language code to a supported game language by prefix
// For example, "fr_CA.UTF-8" -> "fr"
// Returns std::nullopt if the language is not supported by the game
std::optional<std::string> matchGameLanguage( const std::string &lang )
{
    const std::vector<options_manager::id_and_option> available_languages =
        get_options().get_option( "USE_LANG" ).getItems();
    for( const options_manager::id_and_option &available_language : available_languages ) {
        if( !available_language.first.empty() && string_starts_with( lang, available_language.first ) ) {
            return available_language.first;
        }
    }
    return std::nullopt;
}
} // namespace
#endif

namespace SystemLocale
{

std::optional<std::string> Language()
{
#if defined(_WIN32)
    /* "Useful" links:
     *  https://www.science.co.il/language/Locale-codes.php
     *  https://support.microsoft.com/de-de/help/193080/how-to-use-the-getuserdefaultlcid-windows-api-function-to-determine-op
     *  https://msdn.microsoft.com/en-us/library/cc233965.aspx
     */
    static const std::map<std::string, std::set<int>> lang_lcid {
        {"en", { 1033, 2057, 3081, 4105, 5129, 6153, 7177, 8201, 9225, 10249, 11273 }},
        {"ar", {{ 1025, 2049, 3073, 4097, 5121, 6145, 7169, 8193, 9217, 10241, 11265, 12289, 13313, 14337, 15361, 16385 }} },
        {"cs", { 1029 } },
        {"da", { 1030 } },
        {"de", {{ 1031, 2055, 3079, 4103, 5127 }} },
        {"el", { 1032 } },
        {"es_AR", { 11274 } },
        {"es_ES", {{ 1034, 2058, 3082, 4106, 5130, 6154, 7178, 8202, 9226, 10250, 12298, 13322, 14346, 15370, 16394, 17418, 18442, 19466, 20490 }} },
        {"fr", {{ 1036, 2060, 3084, 4108, 5132 }} },
        {"hu", { 1038 } },
        {"id", { 1057 } },
        {"is", { 1039 } },
        {"it_IT", {{ 1040, 2064 }} },
        {"ja", { 1041 } },
        {"ko", { 1042 } },
        {"nb", {{ 1044, 2068 }} },
        {"nl", { 1043 } },
        {"pl", { 1045 } },
        {"pt_BR", {{ 1046, 2070 }} },
        {"ru", {{ 25, 1049, 2073 }} },
        {"sr", { 3098 } },
        {"tr", { 1055 } },
        {"uk_UA", { 1058 } },
        {"zh_CN", {{ 4, 2052, 4100, 30724 }} },
        {"zh_TW", {{ 1028, 3076, 5124, 31748 }} }
    };

    const int lcid = GetUserDefaultUILanguage();
    for( auto &lang : lang_lcid ) {
        if( lang.second.find( lcid ) != lang.second.end() ) {
            return lang.first;
        }
    }
    return std::nullopt;
#elif defined(__APPLE__)
    // Get the user's language list (in order of preference)
    CFArrayRef langs = CFLocaleCopyPreferredLanguages();
    if( CFArrayGetCount( langs ) == 0 ) {
        return std::nullopt;
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
            return std::nullopt;
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

    return matchGameLanguage( lang_code );
#elif defined(__ANDROID__)
    JNIEnv *env = ( JNIEnv * )SDL_AndroidGetJNIEnv();
    jobject activity = ( jobject )SDL_AndroidGetActivity();
    jclass clazz( env->GetObjectClass( activity ) );
    jmethodID method_id = env->GetMethodID( clazz, "getSystemLang", "()Ljava/lang/String;" );
    jstring ans = ( jstring )env->CallObjectMethod( activity, method_id, 0 );
    const char *ans_c_str = env->GetStringUTFChars( ans, 0 );
    if( ans_c_str == nullptr ) {
        // fail-safe if retrieving Java string failed
        return std::nullopt;
    }
    const std::string lang( ans_c_str );
    env->ReleaseStringUTFChars( ans, ans_c_str );
    env->DeleteLocalRef( activity );
    env->DeleteLocalRef( clazz );
    DebugLog( D_INFO, D_MAIN ) << "Read Android system language: '" << lang << '\'';
    if( string_starts_with( lang, "zh_Hans" ) ) {
        return "zh_CN";
    } else if( string_starts_with( lang, "zh_Hant" ) ) {
        return "zh_TW";
    }
    return matchGameLanguage( lang );
#else
    const char *locale = setlocale( LC_ALL, nullptr );
    if( locale == nullptr ) {
        return std::nullopt;
    }
    if( std::strcmp( locale, "C" ) == 0 ) {
        return "en";
    }
    return matchGameLanguage( locale );
#endif
}

std::optional<bool> UseMetricSystem()
{
#if defined(_WIN32)
    // https://docs.microsoft.com/en-us/globalization/locale/units-of-measurement
    DWORD measurementUnit;
    if( GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_IMEASURE | LOCALE_RETURN_NUMBER,
                       reinterpret_cast<LPSTR>( &measurementUnit ),
                       sizeof( measurementUnit ) / sizeof( TCHAR ) ) == 0 ) {
        return std::nullopt;
    }
    // measurementUnit == 0 => Metric System
    // measurementUnit == 1 => Imperial System
    return measurementUnit == 0;
#elif defined(__APPLE__)
    CFLocaleRef localeRef = CFLocaleCopyCurrent();
    CFTypeRef useMetricSystem = CFLocaleGetValue( localeRef, kCFLocaleUsesMetricSystem );
    return static_cast<bool>( CFBooleanGetValue( static_cast<CFBooleanRef>( useMetricSystem ) ) );
#elif defined(__linux__) && defined(_NL_MEASUREMENT_MEASUREMENT)
    std::string const measurement( nl_langinfo( _NL_MEASUREMENT_MEASUREMENT ) );
    if( !measurement.empty() ) {
        return measurement.front() == 1;
    }
    return std::nullopt;
#else
    return std::nullopt;
#endif
}

} // namespace SystemLocale
