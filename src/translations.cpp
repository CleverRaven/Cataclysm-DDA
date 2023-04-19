#include <string>

#include "cata_utility.h"
#include "debug.h"
#include "get_version.h"
#include "name.h"
#include "path_info.h"
#include "translations.h"
#include "translation_gendered.h"

// Names depend on the language settings. They are loaded from different files
// based on the currently used language. If that changes, we have to reload the
// names.
static void reload_names()
{
    Name::clear();
    Name::load_from_file( PATH_INFO::names() );
}

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
#include "ui.h"

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
#endif // LOCALIZE

std::string locale_dir()
{
    std::string loc_dir = PATH_INFO::langdir();
#if defined(LOCALIZE)

#if (defined(__DragonFly__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)) && !defined(BSD)
#define BSD
#endif

#if !defined(__ANDROID__) && ((defined(__linux__) || defined(BSD) || (defined(MACOSX) && !defined(TILES))))
    if( !PATH_INFO::base_path().empty() ) {
        loc_dir = PATH_INFO::base_path() + "share/locale";
    } else {
        loc_dir = PATH_INFO::langdir();
    }
#endif
#endif // LOCALIZE
    return loc_dir;
}

void set_language()
{
#if defined(LOCALIZE)
    const std::string system_lang = SystemLocale::Language().value_or( "en" );
    std::string lang_opt = get_option<std::string>( "USE_LANG" ).empty() ? system_lang :
                           get_option<std::string>( "USE_LANG" );
    DebugLog( D_INFO, D_MAIN ) << "Setting language to: '" << lang_opt << '\'';
    TranslationManager::GetInstance().SetLanguage( lang_opt );
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

#endif // LOCALIZE

    reload_names();

    set_title( string_format( _( "Cataclysm: Dark Days Ahead - %s" ), getVersionString() ) );
}
