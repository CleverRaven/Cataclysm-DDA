#include "translations.h"

#include <clocale>
#include <array>
#include <functional>
#include <iterator>
#include <new>

#if defined(LOCALIZE) && defined(__STRICT_ANSI__)
#undef __STRICT_ANSI__ // _putenv in minGW need that

#define __STRICT_ANSI__
#endif

#if defined(__APPLE__)
// needed by localized_comparator
#include <CoreFoundation/CoreFoundation.h>
#endif

#include <algorithm>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "cached_options.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "debug.h"
#include "generic_factory.h"
#include "json.h"
#include "name.h"
#include "output.h"
#include "path_info.h"
#include "rng.h"
#include "text_style_check.h"

// Names depend on the language settings. They are loaded from different files
// based on the currently used language. If that changes, we have to reload the
// names.
static void reload_names()
{
    Name::clear();
    Name::load_from_file( PATH_INFO::names() );
}

static bool sanity_checked_genders = false;

// int version/generation that is incremented each time language is changed
// used to invalidate translation cache
static int current_language_version = INVALID_LANGUAGE_VERSION + 1;

int detail::get_current_language_version()
{
    return current_language_version;
}

#if defined(LOCALIZE)
#include "options.h"
#include "ui.h"
#if defined(_WIN32)
#if 1 // Prevent IWYU reordering platform_win.h below mmsystem.h
#   include "platform_win.h"
#endif
#   include "mmsystem.h"
static std::string getWindowsLanguage();
#elif defined(__APPLE__)
#include <CoreFoundation/CFLocale.h>
static std::string getAppleSystemLanguage();
#elif defined(__ANDROID__)
#include <jni.h>
#include "sdl_wrappers.h" // for SDL_AndroidGetJNIEnv()
static std::string getAndroidSystemLanguage();
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

void select_language()
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

#if (defined(__DragonFly__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)) && !defined(BSD)
#define BSD
#endif
std::string locale_dir()
{
    std::string loc_dir;
#if !defined(__ANDROID__) && ((defined(__linux__) || defined(BSD) || (defined(MACOSX) && !defined(TILES))))
    if( !PATH_INFO::base_path().empty() ) {
        loc_dir = PATH_INFO::base_path() + "share/locale";
    } else {
        loc_dir = PATH_INFO::langdir();
    }
#else
    loc_dir = PATH_INFO::langdir();
#endif
    return loc_dir;
}

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

void set_language()
{
    const std::string system_lang = getSystemLanguageOrEnglish();
    // Step 1. Setup locale settings.
    std::string lang_opt = get_option<std::string>( "USE_LANG" ).empty() ? system_lang :
                           get_option<std::string>( "USE_LANG" );
    DebugLog( D_INFO, D_MAIN ) << "Setting language to: '" << lang_opt << '\'';
    if( !lang_opt.empty() ) {
        // Not 'System Language'
        // Overwrite all system locale settings. Use CDDA settings. User wants this.
        // LANGUAGE is ignored if LANG is set to C or unset
        // in this case we need to set LANG to something other than C to activate localization
        // Reference: https://www.gnu.org/software/gettext/manual/html_node/The-LANGUAGE-variable.html#The-LANGUAGE-variable
        const char *env_lang = getenv( "LANG" );
        if( env_lang == nullptr || strcmp( env_lang, "C" ) == 0 ) {
#if defined(_WIN32)
            if( _putenv( ( std::string( "LANG=" ) + lang_opt ).c_str() ) != 0 ) {
#else
            if( setenv( "LANG", lang_opt.c_str(), true ) != 0 ) {
#endif
                DebugLog( D_WARNING, D_MAIN ) << "Can't set 'LANG' environment variable";
            }
        }
#if defined(_WIN32)
        if( _putenv( ( std::string( "LANGUAGE=" ) + lang_opt ).c_str() ) != 0 ) {
#else
        if( setenv( "LANGUAGE", lang_opt.c_str(), true ) != 0 ) {
#endif
            DebugLog( D_WARNING, D_MAIN ) << "Can't set 'LANGUAGE' environment variable";
        } else {
            const char *env = getenv( "LANGUAGE" );
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
    DebugLog( D_INFO, DC_ALL ) << "[translations] C locale set to " << setlocale( LC_ALL, nullptr );
    DebugLog( D_INFO, DC_ALL ) << "[translations] C++ locale set to " << std::locale().name();
#endif

    // Step 2. Bind to gettext domain.
    std::string loc_dir = locale_dir();
#if defined(__ANDROID__)
    // HACK: Since we're using libintl-lite instead of libintl on Android, we hack the locale_dir to point directly to the .mo file.
    // This is because of our hacky libintl-lite bindtextdomain() implementation.
    const char *env = getenv( "LANGUAGE" );
    loc_dir += std::string( ( env ? env : "none" ) ) + "/LC_MESSAGES/cataclysm-dda.mo";
#endif

    const char *locale_dir_char = loc_dir.c_str();
    bindtextdomain( "cataclysm-dda", locale_dir_char );
    bind_textdomain_codeset( "cataclysm-dda", "UTF-8" );
    textdomain( "cataclysm-dda" );

    reload_names();

    sanity_checked_genders = false;

    // increment version to invalidate translation cache
    do {
        current_language_version++;
    } while( current_language_version == INVALID_LANGUAGE_VERSION );
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

#else // !LOCALIZE

std::string locale_dir()
{
    return "mo/";
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
        language_genders.emplace_back( "n" );
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
    return translation{ raw };
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

    if( jsin.test_string() ) {
        ctxt = nullptr;
        // if plural form is enabled
        if( raw_pl ) {
            // strings with plural forms are currently only simple names, and
            // need no text style check.
            raw = jsin.get_string();
            raw_pl = cata::make_value<std::string>( raw + "s" );
        } else {
            raw = text_style_check_reader( text_style_check_reader::allow_object::no )
                  .get_next( jsin );
        }
        needs_translation = true;
    } else {
        JsonObject jsobj = jsin.get_object();
        if( jsobj.has_member( "ctxt" ) ) {
            ctxt = cata::make_value<std::string>( jsobj.get_string( "ctxt" ) );
        } else {
            ctxt = nullptr;
        }
        if( jsobj.has_member( "str_sp" ) ) {
            // same singular and plural forms
            // strings with plural forms are currently only simple names, and
            // need no text style check.
            raw = jsobj.get_string( "str_sp" );
            // if plural form is enabled
            if( raw_pl ) {
                raw_pl = cata::make_value<std::string>( raw );
            } else {
                try {
                    jsobj.throw_error( "str_sp not supported here", "str_sp" );
                } catch( const JsonError &e ) {
                    debugmsg( "(json-error)\n%s", e.what() );
                }
            }
        } else {
#ifndef CATA_IN_TOOL
            const bool check_style = test_mode && !jsobj.has_member( "//NOLINT(cata-text-style)" );
#else
            const bool check_style = false;
#endif
            if( raw_pl || !check_style ) {
                // strings with plural forms are currently only simple names, and
                // need no text style check.
                raw = jsobj.get_string( "str" );
            } else {
                raw = text_style_check_reader( text_style_check_reader::allow_object::no )
                      .get_next( *jsobj.get_raw( "str" ) );
            }
            // if plural form is enabled
            if( raw_pl ) {
                if( jsobj.has_member( "str_pl" ) ) {
                    raw_pl = cata::make_value<std::string>( jsobj.get_string( "str_pl" ) );
#ifndef CATA_IN_TOOL
                    if( check_style ) {
                        try {
                            if( *raw_pl == raw + "s" ) {
                                jsobj.throw_error( "\"str_pl\" is not necessary here since the "
                                                   "plural form can be automatically generated.",
                                                   "str_pl" );
                            } else if( *raw_pl == raw ) {
                                jsobj.throw_error( "Please use \"str_sp\" instead of \"str\" and \"str_pl\" "
                                                   "for text with identical singular and plural forms",
                                                   "str_pl" );
                            }
                        } catch( const JsonError &e ) {
                            debugmsg( "(json-error)\n%s", e.what() );
                        }
                    }
#endif
                } else {
                    raw_pl = cata::make_value<std::string>( raw + "s" );
                }
            } else if( jsobj.has_member( "str_pl" ) ) {
                try {
                    jsobj.throw_error( "str_pl not supported here", "str_pl" );
                } catch( const JsonError &e ) {
                    debugmsg( "(json-error)\n%s", e.what() );
                }
            }
        }
        needs_translation = true;
    }
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

text_style_check_reader::text_style_check_reader( const allow_object object_allowed )
    : object_allowed( object_allowed )
{
}

std::string text_style_check_reader::get_next( JsonIn &jsin ) const
{
#ifndef CATA_IN_TOOL
    int origin = 0;
#endif
    std::string raw;
    bool check_style = true;
    if( object_allowed == allow_object::yes && jsin.test_object() ) {
        JsonObject jsobj = jsin.get_object();
#ifndef CATA_IN_TOOL
        if( test_mode ) {
            origin = jsobj.get_raw( "str" )->tell();
        }
#endif
        raw = jsobj.get_string( "str" );
        if( jsobj.has_member( "//NOLINT(cata-text-style)" ) ) {
            check_style = false;
        }
    } else {
#ifndef CATA_IN_TOOL
        if( test_mode ) {
            origin = jsin.tell();
        }
#endif
        raw = jsin.get_string();
    }
#ifndef CATA_IN_TOOL
    if( test_mode && check_style ) {
        const auto text_style_callback =
            [&jsin, origin]
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
            const int previous_pos = jsin.tell();
            try {
                jsin.seek( origin );
                jsin.string_error( err, std::distance( beg, to ) );
            } catch( const JsonError &e ) {
                debugmsg( "(json-error)\n%s", e.what() );
            }
            // seek to previous pos (end of string) so subsequent json input
            // can continue.
            jsin.seek( previous_pos );
        };

        const std::u32string raw32 = utf8_to_utf32( raw );
        text_style_check<std::u32string::const_iterator>( raw32.cbegin(), raw32.cend(),
                fix_end_of_string_spaces::yes, escape_unicode::no, text_style_callback );
    }
#endif
    return raw;
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
#if defined(__APPLE__) // macOS and iOS
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
#if defined(__APPLE__) // macOS and iOS
    return ( *this )( wstr_to_utf8( l ), wstr_to_utf8( r ) );
#else
    return std::locale()( l, r );
#endif
}

bool localized_comparator::operator()( const translation &l, const translation &r ) const
{
    return l.translated_lt( r );
}

// silence -Wunused-macro
#ifdef __STRICT_ANSI__
#undef __STRICT_ANSI__
#endif
