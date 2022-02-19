
#include <string>
#include <vector>

#if defined(_WIN32)
#if 1 // Prevent IWYU reordering platform_win.h below mmsystem.h
#   include "platform_win.h"
#endif
#   include "mmsystem.h"
static std::string getWindowsLanguage();
#elif defined(__APPLE__)
#include <CoreFoundation/CFLocale.h>
#include <CoreFoundation/CoreFoundation.h>
static std::string getAppleSystemLanguage();
#elif defined(__ANDROID__)
#include <jni.h>
#include "sdl_wrappers.h" // for SDL_AndroidGetJNIEnv()
static std::string getAndroidSystemLanguage();
#endif

#include "cata_utility.h"
#include "options.h"
#include "system_language.h"

#ifndef _WIN32
// Try to match language code to a supported game language by prefix
// For example, "fr_CA.UTF-8" -> "fr"
static std::string matchGameLanguage( const std::string &lang )
{
    const std::vector<options_manager::id_and_option> available_languages =
        get_options().get_option( "USE_LANG" ).getItems();
    for( const options_manager::id_and_option &available_language : available_languages ) {
        if( !available_language.first.empty() && string_starts_with( lang, available_language.first ) ) {
            return available_language.first;
        }
    }
    return std::string();
}
#endif

std::string getSystemLanguage()
{
#if defined(_WIN32)
    return getWindowsLanguage();
#elif defined(__APPLE__)
    return getAppleSystemLanguage(); // macOS and iOS
#elif defined(__ANDROID__)
    return getAndroidSystemLanguage();
#else
    const char *locale = setlocale( LC_ALL, nullptr );
    if( locale == nullptr ) {
        return std::string();
    }
    if( strcmp( locale, "C" ) == 0 ) {
        return "en";
    }
    return matchGameLanguage( locale );
#endif
}

std::string getSystemLanguageOrEnglish()
{
    const std::string system_language = getSystemLanguage();
    if( system_language.empty() ) {
        return "en";
    }
    return system_language;
}

#if defined(_WIN32)
/* "Useful" links:
 *  https://www.science.co.il/language/Locale-codes.php
 *  https://support.microsoft.com/de-de/help/193080/how-to-use-the-getuserdefaultlcid-windows-api-function-to-determine-op
 *  https://msdn.microsoft.com/en-us/library/cc233965.aspx
 */
static std::string getWindowsLanguage()
{
    static std::map<std::string, std::set<int>> lang_lcid;
    if( lang_lcid.empty() ) {
        lang_lcid["en"] = {{ 1033, 2057, 3081, 4105, 5129, 6153, 7177, 8201, 9225, 10249, 11273 }};
        lang_lcid["ar"] = {{ 1025, 2049, 3073, 4097, 5121, 6145, 7169, 8193, 9217, 10241, 11265, 12289, 13313, 14337, 15361, 16385 }};
        lang_lcid["cs"] = { 1029 };
        lang_lcid["da"] = { 1030 };
        lang_lcid["de"] = {{ 1031, 2055, 3079, 4103, 5127 }};
        lang_lcid["el"] = { 1032 };
        lang_lcid["es_AR"] = { 11274 };
        lang_lcid["es_ES"] = {{ 1034, 2058, 3082, 4106, 5130, 6154, 7178, 8202, 9226, 10250, 12298, 13322, 14346, 15370, 16394, 17418, 18442, 19466, 20490 }};
        lang_lcid["fr"] = {{ 1036, 2060, 3084, 4108, 5132 }};
        lang_lcid["hu"] = { 1038 };
        lang_lcid["id"] = { 1057 };
        lang_lcid["is"] = { 1039 };
        lang_lcid["it_IT"] = {{ 1040, 2064 }};
        lang_lcid["ja"] = { 1041 };
        lang_lcid["ko"] = { 1042 };
        lang_lcid["nb"] = {{ 1044, 2068 }};
        lang_lcid["nl"] = { 1043 };
        lang_lcid["pl"] = { 1045 };
        lang_lcid["pt_BR"] = {{ 1046, 2070 }};
        lang_lcid["ru"] = {{ 25, 1049, 2073 }};
        lang_lcid["sr"] = { 3098 };
        lang_lcid["tr"] = { 1055 };
        lang_lcid["uk_UA"] = { 1058 };
        lang_lcid["zh_CN"] = {{ 4, 2052, 4100, 30724 }};
        lang_lcid["zh_TW"] = {{ 1028, 3076, 5124, 31748 }};
    }

    const int lcid = GetUserDefaultUILanguage();
    for( auto &lang : lang_lcid ) {
        if( lang.second.find( lcid ) != lang.second.end() ) {
            return lang.first;
        }
    }
    return std::string();
}
#elif defined(__APPLE__)
static std::string getAppleSystemLanguage()
{
    // Get the user's language list (in order of preference)
    CFArrayRef langs = CFLocaleCopyPreferredLanguages();
    if( CFArrayGetCount( langs ) == 0 ) {
        return std::string();
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
            return std::string();
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
}
#elif defined(__ANDROID__)
static std::string getAndroidSystemLanguage()
{
    JNIEnv *env = ( JNIEnv * )SDL_AndroidGetJNIEnv();
    jobject activity = ( jobject )SDL_AndroidGetActivity();
    jclass clazz( env->GetObjectClass( activity ) );
    jmethodID method_id = env->GetMethodID( clazz, "getSystemLang", "()Ljava/lang/String;" );
    jstring ans = ( jstring )env->CallObjectMethod( activity, method_id, 0 );
    const char *ans_c_str = env->GetStringUTFChars( ans, 0 );
    if( ans_c_str == nullptr ) {
        // fail-safe if retrieving Java string failed
        return std::string();
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
}
#endif
