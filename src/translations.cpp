#include <string>

#include "cata_utility.h"
#include "debug.h"
#include "get_version.h"
#include "path_info.h"
#include "text_snippets.h"
#include "translations.h"
#include "translation_gendered.h"

// int version/generation that is incremented each time language is changed
// used to invalidate translation cache
static int current_language_version = INVALID_LANGUAGE_VERSION + 1;

int detail::get_current_language_version()
{
    return current_language_version;
}

#if defined(LOCALIZE)
#include "options.h"
#include "system_locale.h"
#include "uilist.h"

std::string select_language()
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

    return languages[sm.ret].first;
}
#endif // LOCALIZE

std::string locale_dir()
{
    std::string loc_dir = PATH_INFO::langdir();
#if defined(LOCALIZE)

#if (defined(__DragonFly__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)) && !defined(CATA_IS_ON_BSD)
#define CATA_IS_ON_BSD
#endif

#if !defined(__ANDROID__) && ((defined(__linux__) || defined(CATA_IS_ON_BSD) || (defined(MACOSX) && !defined(TILES))))
    if( !PATH_INFO::base_path().get_logical_root_path().empty() ) {
        loc_dir = ( PATH_INFO::base_path() / "share" / "locale" ).generic_u8string();
    } else {
        loc_dir = PATH_INFO::langdir();
    }
#endif
#endif // LOCALIZE
    return loc_dir;
}

void set_language_from_options()
{
#if defined(LOCALIZE)
    const std::string system_lang = SystemLocale::Language().value_or( "en" );
    std::string lang_opt = get_option<std::string>( "USE_LANG" ).empty() ? system_lang :
                           get_option<std::string>( "USE_LANG" );
    set_language( lang_opt );
#else
    set_language( "en" );
#endif
}

void set_language( const std::string &lang )
{
#if defined(LOCALIZE)
    DebugLog( D_INFO, D_MAIN ) << "Setting language to: '" << lang << '\'';
    TranslationManager::GetInstance().SetLanguage( lang );
#if defined(_WIN32)
    // Use the ANSI code page 1252 to work around some language output bugs. (#8665)
    if( setlocale( LC_ALL, ".1252" ) == nullptr ) {
        DebugLog( D_WARNING, D_MAIN ) << "Error while setlocale(LC_ALL, '.1252').";
    }
#endif

    reset_sanity_check_genders();

    // increment version to invalidate translation cache
    do {
        current_language_version++;
    } while( current_language_version == INVALID_LANGUAGE_VERSION );

#else
    // Silence unused var warning
    ( void ) lang;
#endif // LOCALIZE

    // Names depend on the language settings. They are loaded from different files
    // based on the currently used language. If that changes, we have to reload the
    // names.
    SNIPPET.reload_names( PATH_INFO::names() );

    set_title( string_format( _( "Cataclysm: Dark Days Ahead - %s" ), getVersionString() ) );
}
