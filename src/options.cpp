#include "options.h"

#include <locale.h>
#include <cfloat>
#include <climits>
#include <iterator>
#include <stdexcept>

#include "calendar.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "color.h"
#include "cursesdef.h"
#include "cursesport.h"
#include "debug.h"
#include "filesystem.h"
#include "game.h"
#include "game_constants.h"
#include "input.h"
#include "json.h"
#include "line.h"
#include "mapsharing.h"
#include "output.h"
#include "path_info.h"
#include "point.h"
#include "popup.h"
#include "sdlsound.h"
#include "sdltiles.h"
#include "sounds.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translations.h"
#include "ui_manager.h"
#include "worldfactory.h"

#if defined(TILES)
#include "cata_tiles.h"
#endif // TILES

#if defined(__ANDROID__)
#include <jni.h>
#endif

#include <algorithm>
#include <cstdlib>
#include <exception>
#include <locale>
#include <memory>
#include <sstream>
#include <string>

bool use_tiles;
bool log_from_top;
int message_ttl;
int message_cooldown;
bool fov_3d;
int fov_3d_z_range;
bool tile_iso;

std::map<std::string, std::string> TILESETS; // All found tilesets: <name, tileset_dir>
std::map<std::string, std::string> SOUNDPACKS; // All found soundpacks: <name, soundpack_dir>

std::vector<options_manager::id_and_option> options_manager::lang_options = {
    { "", translate_marker( "System language" ) },
    // Note: language names are in their own language and are *not* translated at all.
    // Note: Somewhere in Github PR was better link to msdn.microsoft.com with language names.
    // http://en.wikipedia.org/wiki/List_of_language_names
    { "en", no_translation( R"(English)" ) },
    { "de", no_translation( R"(Deutsch)" ) },
    { "es_AR", no_translation( R"(Español (Argentina))" ) },
    { "es_ES", no_translation( R"(Español (España))" ) },
    { "fr", no_translation( R"(Français)" ) },
    { "hu", no_translation( R"(Magyar)" ) },
    { "ja", no_translation( R"(日本語)" ) },
    { "ko", no_translation( R"(한국어)" ) },
    { "pl", no_translation( R"(Polski)" ) },
    { "pt_BR", no_translation( R"(Português (Brasil))" )},
    { "ru", no_translation( R"(Русский)" ) },
    { "zh_CN", no_translation( R"(中文 (天朝))" ) },
    { "zh_TW", no_translation( R"(中文 (台灣))" ) },
};

options_manager &get_options()
{
    static options_manager single_instance;
    return single_instance;
}

options_manager::options_manager() :
    general_page_( "general", to_translation( "General" ) ),
    interface_page_( "interface", to_translation( "Interface" ) ),
    graphics_page_( "graphics", to_translation( "Graphics" ) ),
    debug_page_( "debug", to_translation( "Debug" ) ),
    world_default_page_( "world_default", to_translation( "World Defaults" ) ),
    android_page_( "android", to_translation( "Android" ) )
{
    pages_.emplace_back( general_page_ );
    pages_.emplace_back( interface_page_ );
    pages_.emplace_back( graphics_page_ );
    // when sharing maps only admin is allowed to change these.
    if( !MAP_SHARING::isCompetitive() || MAP_SHARING::isAdmin() ) {
        pages_.emplace_back( debug_page_ );
    }
    // when sharing maps only admin is allowed to change these.
    if( !MAP_SHARING::isCompetitive() || MAP_SHARING::isAdmin() ) {
        pages_.emplace_back( world_default_page_ );
    }
#if defined(__ANDROID__)
    pages_.emplace_back( android_page_ );
#endif

    mMigrateOption = { {"DELETE_WORLD", { "WORLD_END", { {"no", "keep" }, {"yes", "delete"} } } } };

    enable_json( "DEFAULT_REGION" );
    // to allow class based init_data functions to add values to a 'string' type option, add:
    //   enable_json("OPTION_KEY_THAT_GETS_STRING_ENTRIES_ADDED_VIA_JSON");
    // then, in the my_class::load_json (or post-json setup) method:
    //   get_options().add_value("OPTION_KEY_THAT_GETS_STRING_ENTRIES_ADDED_VIA_JSON", "thisvalue");
}

static const std::string blank_value( 1, 001 ); // because "" might be valid

void options_manager::enable_json( const std::string &lvar )
{
    post_json_verify[ lvar ] = blank_value;
}

void options_manager::add_retry( const std::string &lvar, const::std::string &lval )
{
    std::map<std::string, std::string>::const_iterator it = post_json_verify.find( lvar );
    if( it != post_json_verify.end() && it->second == blank_value ) {
        // initialized with impossible value: valid
        post_json_verify[ lvar ] = lval;
    }
}

void options_manager::add_value( const std::string &lvar, const std::string &lval,
                                 const translation &lvalname )
{
    std::map<std::string, std::string>::const_iterator it = post_json_verify.find( lvar );
    if( it != post_json_verify.end() ) {
        auto ot = options.find( lvar );
        if( ot != options.end() && ot->second.sType == "string_select" ) {
            for( auto &vItem : ot->second.vItems ) {
                if( vItem.first == lval ) { // already in
                    return;
                }
            }
            ot->second.vItems.emplace_back( lval, lvalname );
            // our value was saved, then set to default, so set it again.
            if( it->second == lval ) {
                options[ lvar ].setValue( lval );
            }
        }

    }
}

void options_manager::addOptionToPage( const std::string &name, const std::string &page )
{
    for( Page &p : pages_ ) {
        if( p.id_ == page ) {
            // Don't add duplicate options to the page
            for( const cata::optional<std::string> &i : p.items_ ) {
                if( i.has_value() && i.value() == name ) {
                    return;
                }
            }
            p.items_.emplace_back( name );
            return;
        }
    }
    // @TODO handle the case when an option has no valid page id (note: consider hidden external options as well)
}

options_manager::cOpt::cOpt()
{
    sType = "VOID";
    eType = CVT_VOID;
    hide = COPT_NO_HIDE;
}

static options_manager::cOpt::COPT_VALUE_TYPE get_value_type( const std::string &sType )
{
    using CVT = options_manager::cOpt::COPT_VALUE_TYPE;

    static std::unordered_map<std::string, CVT> vt_map = {
        { "float", CVT::CVT_FLOAT },
        { "bool", CVT::CVT_BOOL },
        { "int", CVT::CVT_INT },
        { "int_map", CVT::CVT_INT },
        { "string_select", CVT::CVT_STRING },
        { "string_input", CVT::CVT_STRING },
        { "VOID", CVT::CVT_VOID }
    };
    auto result = vt_map.find( sType );
    return result != vt_map.end() ? result->second : options_manager::cOpt::CVT_UNKNOWN;
}

//add hidden external option with value
void options_manager::add_external( const std::string &sNameIn, const std::string &sPageIn,
                                    const std::string &sType,
                                    const std::string &sMenuTextIn, const std::string &sTooltipIn )
{
    cOpt thisOpt;

    thisOpt.sName = sNameIn;
    thisOpt.sPage = sPageIn;
    thisOpt.sMenuText = sMenuTextIn;
    thisOpt.sTooltip = sTooltipIn;
    thisOpt.sType = sType;
    thisOpt.verbose = false;

    thisOpt.eType = get_value_type( thisOpt.sType );

    switch( thisOpt.eType ) {
        case cOpt::CVT_BOOL:
            thisOpt.bSet = false;
            thisOpt.bDefault = false;
            break;
        case cOpt::CVT_INT:
            thisOpt.iMin = INT_MIN;
            thisOpt.iMax = INT_MAX;
            thisOpt.iDefault = 0;
            thisOpt.iSet = 0;
            break;
        case cOpt::CVT_FLOAT:
            thisOpt.fMin = FLT_MIN;
            thisOpt.fMax = FLT_MAX;
            thisOpt.fDefault = 0;
            thisOpt.fSet = 0;
            thisOpt.fStep = 1;
            break;
        default:
            // all other type-specific values have default constructors
            break;
    }

    thisOpt.hide = COPT_ALWAYS_HIDE;
    addOptionToPage( sNameIn, sPageIn );

    options[sNameIn] = thisOpt;
}

//add string select option
void options_manager::add( const std::string &sNameIn, const std::string &sPageIn,
                           const std::string &sMenuTextIn, const std::string &sTooltipIn,
                           const std::vector<id_and_option> &sItemsIn, std::string sDefaultIn,
                           copt_hide_t opt_hide )
{
    cOpt thisOpt;

    thisOpt.sName = sNameIn;
    thisOpt.sPage = sPageIn;
    thisOpt.sMenuText = sMenuTextIn;
    thisOpt.sTooltip = sTooltipIn;
    thisOpt.sType = "string_select";
    thisOpt.eType = get_value_type( thisOpt.sType );

    thisOpt.hide = opt_hide;
    thisOpt.vItems = sItemsIn;

    if( thisOpt.getItemPos( sDefaultIn ) == -1 ) {
        sDefaultIn = thisOpt.vItems[0].first;
    }

    thisOpt.sDefault = sDefaultIn;
    thisOpt.sSet = sDefaultIn;

    addOptionToPage( sNameIn, sPageIn );

    options[sNameIn] = thisOpt;
}

//add string input option
void options_manager::add( const std::string &sNameIn, const std::string &sPageIn,
                           const std::string &sMenuTextIn, const std::string &sTooltipIn,
                           const std::string &sDefaultIn, const int iMaxLengthIn,
                           copt_hide_t opt_hide )
{
    cOpt thisOpt;

    thisOpt.sName = sNameIn;
    thisOpt.sPage = sPageIn;
    thisOpt.sMenuText = sMenuTextIn;
    thisOpt.sTooltip = sTooltipIn;
    thisOpt.sType = "string_input";
    thisOpt.eType = get_value_type( thisOpt.sType );

    thisOpt.hide = opt_hide;

    thisOpt.iMaxLength = iMaxLengthIn;
    thisOpt.sDefault = thisOpt.iMaxLength > 0 ? sDefaultIn.substr( 0, thisOpt.iMaxLength ) : sDefaultIn;
    thisOpt.sSet = thisOpt.sDefault;

    addOptionToPage( sNameIn, sPageIn );

    options[sNameIn] = thisOpt;
}

//add bool option
void options_manager::add( const std::string &sNameIn, const std::string &sPageIn,
                           const std::string &sMenuTextIn, const std::string &sTooltipIn,
                           const bool bDefaultIn, copt_hide_t opt_hide )
{
    cOpt thisOpt;

    thisOpt.sName = sNameIn;
    thisOpt.sPage = sPageIn;
    thisOpt.sMenuText = sMenuTextIn;
    thisOpt.sTooltip = sTooltipIn;
    thisOpt.sType = "bool";
    thisOpt.eType = get_value_type( thisOpt.sType );

    thisOpt.hide = opt_hide;

    thisOpt.bDefault = bDefaultIn;
    thisOpt.bSet = bDefaultIn;

    addOptionToPage( sNameIn, sPageIn );

    options[sNameIn] = thisOpt;
}

//add int option
void options_manager::add( const std::string &sNameIn, const std::string &sPageIn,
                           const std::string &sMenuTextIn, const std::string &sTooltipIn,
                           const int iMinIn, int iMaxIn, int iDefaultIn,
                           copt_hide_t opt_hide, const std::string &format )
{
    cOpt thisOpt;

    thisOpt.sName = sNameIn;
    thisOpt.sPage = sPageIn;
    thisOpt.sMenuText = sMenuTextIn;
    thisOpt.sTooltip = sTooltipIn;
    thisOpt.sType = "int";
    thisOpt.eType = get_value_type( thisOpt.sType );

    thisOpt.format = format;

    thisOpt.hide = opt_hide;

    if( iMinIn > iMaxIn ) {
        iMaxIn = iMinIn;
    }

    thisOpt.iMin = iMinIn;
    thisOpt.iMax = iMaxIn;

    if( iDefaultIn < iMinIn || iDefaultIn > iMaxIn ) {
        iDefaultIn = iMinIn;
    }

    thisOpt.iDefault = iDefaultIn;
    thisOpt.iSet = iDefaultIn;

    addOptionToPage( sNameIn, sPageIn );

    options[sNameIn] = thisOpt;
}

//add int map option
void options_manager::add( const std::string &sNameIn, const std::string &sPageIn,
                           const std::string &sMenuTextIn, const std::string &sTooltipIn,
                           const std::vector< std::tuple<int, std::string> > &mIntValuesIn,
                           int iInitialIn, int iDefaultIn, copt_hide_t opt_hide, const bool verbose )
{
    cOpt thisOpt;

    thisOpt.sName = sNameIn;
    thisOpt.sPage = sPageIn;
    thisOpt.sMenuText = sMenuTextIn;
    thisOpt.sTooltip = sTooltipIn;
    thisOpt.sType = "int_map";
    thisOpt.eType = get_value_type( thisOpt.sType );
    thisOpt.verbose = verbose;

    thisOpt.format = "%i";

    thisOpt.hide = opt_hide;

    thisOpt.mIntValues = mIntValuesIn;

    auto item = thisOpt.findInt( iInitialIn );
    if( !item ) {
        iInitialIn = std::get<0>( mIntValuesIn[0] );
    }

    item = thisOpt.findInt( iDefaultIn );
    if( !item ) {
        iDefaultIn = std::get<0>( mIntValuesIn[0] );
    }

    thisOpt.iDefault = iDefaultIn;
    thisOpt.iSet = iInitialIn;

    addOptionToPage( sNameIn, sPageIn );

    options[sNameIn] = thisOpt;
}

//add float option
void options_manager::add( const std::string &sNameIn, const std::string &sPageIn,
                           const std::string &sMenuTextIn, const std::string &sTooltipIn,
                           const float fMinIn, float fMaxIn, float fDefaultIn,
                           float fStepIn, copt_hide_t opt_hide, const std::string &format )
{
    cOpt thisOpt;

    thisOpt.sName = sNameIn;
    thisOpt.sPage = sPageIn;
    thisOpt.sMenuText = sMenuTextIn;
    thisOpt.sTooltip = sTooltipIn;
    thisOpt.sType = "float";
    thisOpt.eType = get_value_type( thisOpt.sType );

    thisOpt.format = format;

    thisOpt.hide = opt_hide;

    if( fMinIn > fMaxIn ) {
        fMaxIn = fMinIn;
    }

    thisOpt.fMin = fMinIn;
    thisOpt.fMax = fMaxIn;
    thisOpt.fStep = fStepIn;

    if( fDefaultIn < fMinIn || fDefaultIn > fMaxIn ) {
        fDefaultIn = fMinIn;
    }

    thisOpt.fDefault = fDefaultIn;
    thisOpt.fSet = fDefaultIn;

    addOptionToPage( sNameIn, sPageIn );

    options[sNameIn] = thisOpt;
}

void options_manager::cOpt::setPrerequisites( const std::string &sOption,
        const std::vector<std::string> &sAllowedValues )
{
    const bool hasOption = get_options().has_option( sOption );
    if( !hasOption ) {
        debugmsg( "setPrerequisite: unknown option %s", sType );
        return;
    }

    const cOpt &existingOption = get_options().get_option( sOption );
    const std::string &existingOptionType = existingOption.getType();
    bool isOfSupportType = false;
    for( const std::string &sSupportedType : getPrerequisiteSupportedTypes() ) {
        if( existingOptionType == sSupportedType ) {
            isOfSupportType = true;
            break;
        }
    }

    if( !isOfSupportType ) {
        debugmsg( "setPrerequisite: option %s not of supported type", sType );
        return;
    }

    sPrerequisite = sOption;
    sPrerequisiteAllowedValues = sAllowedValues;
}

std::string options_manager::cOpt::getPrerequisite() const
{
    return sPrerequisite;
}

bool options_manager::cOpt::hasPrerequisite() const
{
    return !sPrerequisite.empty();
}

bool options_manager::cOpt::checkPrerequisite() const
{
    if( !hasPrerequisite() ) {
        return true;
    }
    bool isPrerequisiteFulfilled = false;
    const std::string prerequisite_option_value = get_options().get_option( sPrerequisite ).getValue();
    for( const std::string &sAllowedPrerequisiteValue : sPrerequisiteAllowedValues ) {
        if( prerequisite_option_value == sAllowedPrerequisiteValue ) {
            isPrerequisiteFulfilled = true;
            break;
        }
    }
    return isPrerequisiteFulfilled;
}

//helper functions
bool options_manager::cOpt::is_hidden() const
{
    switch( hide ) {
        case COPT_NO_HIDE:
            return false;

        case COPT_SDL_HIDE:
#if defined(TILES)
            return true;
#else
            return false;
#endif

        case COPT_CURSES_HIDE:
#if !defined(TILES) // If not defined.  it's curses interface.
            return true;
#else
            return false;
#endif

        case COPT_POSIX_CURSES_HIDE:
            // Check if we on windows and using wincurses.
#if defined(TILES) || defined(_WIN32)
            return false;
#else
            return true;
#endif

        case COPT_NO_SOUND_HIDE:
#if !defined(SDL_SOUND) // If not defined, we have no sound support.
            return true;
#else
            return false;
#endif

        case COPT_ALWAYS_HIDE:
            return true;
    }
    // Make compiler happy, this is unreachable.
    return false;
}

std::string options_manager::cOpt::getName() const
{
    return sName;
}

std::string options_manager::cOpt::getPage() const
{
    return sPage;
}

std::string options_manager::cOpt::getMenuText() const
{
    return _( sMenuText );
}

std::string options_manager::cOpt::getTooltip() const
{
    return _( sTooltip );
}

std::string options_manager::cOpt::getType() const
{
    return sType;
}

bool options_manager::cOpt::operator==( const cOpt &rhs ) const
{
    if( sType != rhs.sType ) {
        return false;
    } else if( sType == "string_select" || sType == "string_input" ) {
        return sSet == rhs.sSet;
    } else if( sType == "bool" ) {
        return bSet == rhs.bSet;
    } else if( sType == "int" || sType == "int_map" ) {
        return iSet == rhs.iSet;
    } else if( sType == "float" ) {
        return fSet == rhs.fSet;
    } else if( sType == "VOID" ) {
        return true;
    } else {
        debugmsg( "unknown option type %s", sType );
        return false;
    }
}

std::string options_manager::cOpt::getValue( bool classis_locale ) const
{
    if( sType == "string_select" || sType == "string_input" ) {
        return sSet;

    } else if( sType == "bool" ) {
        return bSet ? "true" : "false";

    } else if( sType == "int" || sType == "int_map" ) {
        return string_format( format, iSet );

    } else if( sType == "float" ) {
        std::ostringstream ssTemp;
        ssTemp.imbue( classis_locale ? std::locale::classic() : std::locale() );
        ssTemp.precision( 2 );
        ssTemp.setf( std::ios::fixed, std::ios::floatfield );
        ssTemp << fSet;
        return ssTemp.str();
    }

    return "";
}

template<>
std::string options_manager::cOpt::value_as<std::string>() const
{
    if( eType != CVT_STRING ) {
        debugmsg( "%s tried to get string value from option of type %s", sName, sType );
    }
    return sSet;
}

template<>
bool options_manager::cOpt::value_as<bool>() const
{
    if( eType != CVT_BOOL ) {
        debugmsg( "%s tried to get boolean value from option of type %s", sName, sType );
    }
    return bSet;
}

template<>
float options_manager::cOpt::value_as<float>() const
{
    if( eType != CVT_FLOAT ) {
        debugmsg( "%s tried to get float value from option of type %s", sName, sType );
    }
    return fSet;
}

template<>
int options_manager::cOpt::value_as<int>() const
{
    if( eType != CVT_INT ) {
        debugmsg( "%s tried to get integer value from option of type %s", sName, sType );
    }
    return iSet;
}

std::string options_manager::cOpt::getValueName() const
{
    if( sType == "string_select" ) {
        const auto iter = std::find_if( vItems.begin(),
        vItems.end(), [&]( const id_and_option & e ) {
            return e.first == sSet;
        } );
        if( iter != vItems.end() ) {
            return iter->second.translated();
        }

    } else if( sType == "bool" ) {
        return bSet ? _( "True" ) : _( "False" );

    } else if( sType == "int_map" ) {
        const std::string name = std::get<1>( *findInt( iSet ) );
        if( verbose ) {
            return string_format( _( "%d: %s" ), iSet, name );
        } else {
            return string_format( _( "%s" ), name );
        }
    }

    return getValue();
}

std::string options_manager::cOpt::getDefaultText( const bool bTranslated ) const
{
    if( sType == "string_select" ) {
        const auto iter = std::find_if( vItems.begin(), vItems.end(),
        [this]( const id_and_option & elem ) {
            return elem.first == sDefault;
        } );
        const std::string defaultName = iter == vItems.end() ? std::string() :
                                        bTranslated ? iter->second.translated() : iter->first;
        const std::string &sItems = enumerate_as_string( vItems.begin(), vItems.end(),
        [bTranslated]( const id_and_option & elem ) {
            return bTranslated ? elem.second.translated() : elem.first;
        }, enumeration_conjunction::none );
        return string_format( _( "Default: %s - Values: %s" ), defaultName, sItems );

    } else if( sType == "string_input" ) {
        return string_format( _( "Default: %s" ), sDefault );

    } else if( sType == "bool" ) {
        return bDefault ? _( "Default: True" ) : _( "Default: False" );

    } else if( sType == "int" ) {
        return string_format( _( "Default: %d - Min: %d, Max: %d" ), iDefault, iMin, iMax );

    } else if( sType == "int_map" ) {
        const std::string name = std::get<1>( *findInt( iDefault ) );
        if( verbose ) {
            return string_format( _( "Default: %d: %s" ), iDefault, name );
        } else {
            return string_format( _( "Default: %s" ), name );
        }

    } else if( sType == "float" ) {
        return string_format( _( "Default: %.2f - Min: %.2f, Max: %.2f" ), fDefault, fMin, fMax );
    }

    return "";
}

int options_manager::cOpt::getItemPos( const std::string &sSearch ) const
{
    if( sType == "string_select" ) {
        for( size_t i = 0; i < vItems.size(); i++ ) {
            if( vItems[i].first == sSearch ) {
                return i;
            }
        }
    }

    return -1;
}

std::vector<options_manager::id_and_option> options_manager::cOpt::getItems() const
{
    return vItems;
}

int options_manager::cOpt::getIntPos( const int iSearch ) const
{
    if( sType == "int_map" ) {
        for( size_t i = 0; i < mIntValues.size(); i++ ) {
            if( std::get<0>( mIntValues[i] ) == iSearch ) {
                return i;
            }
        }
    }

    return -1;
}

cata::optional< std::tuple<int, std::string> > options_manager::cOpt::findInt(
    const int iSearch ) const
{
    int i = static_cast<int>( getIntPos( iSearch ) );
    if( i == -1 ) {
        return cata::nullopt;
    }
    return mIntValues[i];
}

int options_manager::cOpt::getMaxLength() const
{
    if( sType == "string_input" ) {
        return iMaxLength;
    }

    return 0;
}

//set to next item
void options_manager::cOpt::setNext()
{
    if( sType == "string_select" ) {
        int iNext = getItemPos( sSet ) + 1;
        if( iNext >= static_cast<int>( vItems.size() ) ) {
            iNext = 0;
        }

        sSet = vItems[iNext].first;

    } else if( sType == "string_input" ) {
        int iMenuTextLength = utf8_width( _( sMenuText ) );
        string_input_popup()
        .width( iMaxLength > 80 ? 80 : iMaxLength < iMenuTextLength ? iMenuTextLength : iMaxLength + 1 )
        .description( _( sMenuText ) )
        .max_length( iMaxLength )
        .edit( sSet );

    } else if( sType == "bool" ) {
        bSet = !bSet;

    } else if( sType == "int" ) {
        iSet++;
        if( iSet > iMax ) {
            iSet = iMin;
        }

    } else if( sType == "int_map" ) {
        unsigned int iNext = getIntPos( iSet ) + 1;
        if( iNext >= mIntValues.size() ) {
            iNext = 0;
        }
        iSet = std::get<0>( mIntValues[iNext] );

    } else if( sType == "float" ) {
        fSet += fStep;
        if( fSet > fMax ) {
            fSet = fMin;
        }
    }
}

//set to previous item
void options_manager::cOpt::setPrev()
{
    if( sType == "string_select" ) {
        int iPrev = static_cast<int>( getItemPos( sSet ) ) - 1;
        if( iPrev < 0 ) {
            iPrev = vItems.size() - 1;
        }

        sSet = vItems[iPrev].first;

    } else if( sType == "string_input" ) {
        setNext();

    } else if( sType == "bool" ) {
        bSet = !bSet;

    } else if( sType == "int" ) {
        iSet--;
        if( iSet < iMin ) {
            iSet = iMax;
        }

    } else if( sType == "int_map" ) {
        int iPrev = static_cast<int>( getIntPos( iSet ) ) - 1;
        if( iPrev < 0 ) {
            iPrev = mIntValues.size() - 1;
        }
        iSet = std::get<0>( mIntValues[iPrev] );

    } else if( sType == "float" ) {
        fSet -= fStep;
        if( fSet < fMin ) {
            fSet = fMax;
        }
    }
}

//set value
void options_manager::cOpt::setValue( float fSetIn )
{
    if( sType != "float" ) {
        debugmsg( "tried to set a float value to a %s option", sType );
        return;
    }
    fSet = fSetIn;
    if( fSet < fMin || fSet > fMax ) {
        fSet = fDefault;
    }
}

//set value
void options_manager::cOpt::setValue( int iSetIn )
{
    if( sType != "int" ) {
        debugmsg( "tried to set an int value to a %s option", sType );
        return;
    }
    iSet = iSetIn;
    if( iSet < iMin || iSet > iMax ) {
        iSet = iDefault;
    }
}

//set value
void options_manager::cOpt::setValue( std::string sSetIn )
{
    if( sType == "string_select" ) {
        if( getItemPos( sSetIn ) != -1 ) {
            sSet = sSetIn;
        }

    } else if( sType == "string_input" ) {
        sSet = iMaxLength > 0 ? sSetIn.substr( 0, iMaxLength ) : sSetIn;

    } else if( sType == "bool" ) {
        bSet = sSetIn == "True" || sSetIn == "true" || sSetIn == "T" || sSetIn == "t";

    } else if( sType == "int" ) {
        iSet = atoi( sSetIn.c_str() );

        if( iSet < iMin || iSet > iMax ) {
            iSet = iDefault;
        }

    } else if( sType == "int_map" ) {
        iSet = atoi( sSetIn.c_str() );

        auto item = findInt( iSet );
        if( !item ) {
            iSet = iDefault;
        }

    } else if( sType == "float" ) {
        std::istringstream ssTemp( sSetIn );
        ssTemp.imbue( std::locale::classic() );
        float tmpFloat;
        ssTemp >> tmpFloat;
        if( ssTemp ) {
            setValue( tmpFloat );
        } else {
            debugmsg( "invalid floating point option: %s", sSetIn );
        }
    }
}

/** Fill a mapping with values.
 * Scans all directories in @p dirname directory for
 * a file named @p filename.
 * All found values added to resource_option as name, resource_dir.
 * Furthermore, it builds possible values list for cOpt class.
 */
static std::vector<options_manager::id_and_option> build_resource_list(
    std::map<std::string, std::string> &resource_option, const std::string &operation_name,
    const std::string &dirname, const std::string &filename )
{
    std::vector<options_manager::id_and_option> resource_names;

    resource_option.clear();
    const auto resource_dirs = get_directories_with( filename, dirname, true );

    for( auto &resource_dir : resource_dirs ) {
        read_from_file( resource_dir + "/" + filename, [&]( std::istream & fin ) {
            std::string resource_name;
            std::string view_name;
            // should only have 2 values inside it, otherwise is going to only load the last 2 values
            while( !fin.eof() ) {
                std::string sOption;
                fin >> sOption;

                if( sOption.empty() ) {
                    getline( fin, sOption );    // Empty line, chomp it
                } else if( sOption[0] == '#' ) { // # indicates a comment
                    getline( fin, sOption );
                } else {
                    if( sOption.find( "NAME" ) != std::string::npos ) {
                        resource_name.clear();
                        getline( fin, resource_name );
                        resource_name = trim( resource_name );
                    } else if( sOption.find( "VIEW" ) != std::string::npos ) {
                        view_name.clear();
                        getline( fin, view_name );
                        view_name = trim( view_name );
                        break;
                    }
                }
            }
            resource_names.emplace_back( resource_name,
                                         view_name.empty() ? no_translation( resource_name ) : to_translation( view_name ) );
            if( resource_option.count( resource_name ) != 0 ) {
                debugmsg( "Found \"%s\" duplicate with name \"%s\" (new definition will be ignored)",
                          operation_name, resource_name );
            } else {
                resource_option.insert( std::pair<std::string, std::string>( resource_name, resource_dir ) );
            }
        } );
    }

    return resource_names;
}

std::vector<options_manager::id_and_option> options_manager::load_tilesets_from(
    const std::string &path )
{
    // Use local map as build_resource_list will clear the first parameter
    std::map<std::string, std::string> local_tilesets;
    auto tileset_names = build_resource_list( local_tilesets, "tileset", path,
                         PATH_INFO::tileset_conf() );

    // Copy found tilesets
    TILESETS.insert( local_tilesets.begin(), local_tilesets.end() );

    return tileset_names;
}

std::vector<options_manager::id_and_option> options_manager::build_tilesets_list()
{
    // Clear tilesets
    TILESETS.clear();
    std::vector<id_and_option> result;

    // Load from data directory
    std::vector<options_manager::id_and_option> data_tilesets = load_tilesets_from(
                PATH_INFO::gfxdir() );
    result.insert( result.end(), data_tilesets.begin(), data_tilesets.end() );

    // Load from user directory
    std::vector<options_manager::id_and_option> user_tilesets = load_tilesets_from(
                PATH_INFO::user_gfx() );
    for( options_manager::id_and_option id : user_tilesets ) {
        if( std::find( result.begin(), result.end(), id ) == result.end() ) {
            result.emplace_back( id );
        }
    }

    // Default values
    if( result.empty() ) {
        result.emplace_back( "hoder", to_translation( "Hoder's" ) );
        result.emplace_back( "deon", to_translation( "Deon's" ) );
    }
    return result;
}

std::vector<options_manager::id_and_option> options_manager::load_soundpack_from(
    const std::string &path )
{
    // build_resource_list will clear &resource_option - first param
    std::map<std::string, std::string> local_soundpacks;
    auto soundpack_names = build_resource_list( local_soundpacks, "soundpack", path,
                           PATH_INFO::soundpack_conf() );

    // Copy over found soundpacks
    SOUNDPACKS.insert( local_soundpacks.begin(), local_soundpacks.end() );

    // Return found soundpack names for further processing
    return soundpack_names;
}

std::vector<options_manager::id_and_option> options_manager::build_soundpacks_list()
{
    // Clear soundpacks before loading
    SOUNDPACKS.clear();
    std::vector<id_and_option> result;

    // Search data directory for sound packs
    auto data_soundpacks = load_soundpack_from( PATH_INFO::data_sound() );
    result.insert( result.end(), data_soundpacks.begin(), data_soundpacks.end() );

    // Search user directory for sound packs
    auto user_soundpacks = load_soundpack_from( PATH_INFO::user_sound() );
    result.insert( result.end(), user_soundpacks.begin(), user_soundpacks.end() );

    // Select default built-in sound pack
    if( result.empty() ) {
        result.emplace_back( "basic", to_translation( "Basic" ) );
    }
    return result;
}

#if defined(__ANDROID__)
bool android_get_default_setting( const char *settings_name, bool default_value )
{
    JNIEnv *env = ( JNIEnv * )SDL_AndroidGetJNIEnv();
    jobject activity = ( jobject )SDL_AndroidGetActivity();
    jclass clazz( env->GetObjectClass( activity ) );
    jmethodID method_id = env->GetMethodID( clazz, "getDefaultSetting", "(Ljava/lang/String;Z)Z" );
    jboolean ans = env->CallBooleanMethod( activity, method_id, env->NewStringUTF( settings_name ),
                                           default_value );
    env->DeleteLocalRef( activity );
    env->DeleteLocalRef( clazz );
    return ans;
}
#endif

void options_manager::Page::removeRepeatedEmptyLines()
{
    const auto empty = [&]( const cata::optional<std::string> &v ) -> bool {
        return !v || get_options().get_option( *v ).is_hidden();
    };

    while( !items_.empty() && empty( items_.front() ) ) {
        items_.erase( items_.begin() );
    }
    while( !items_.empty() && empty( items_.back() ) ) {
        items_.erase( items_.end() - 1 );
    }
    for( auto iter = std::next( items_.begin() ); iter != items_.end(); ) {
        if( empty( *std::prev( iter ) ) && empty( *iter ) ) {
            iter = items_.erase( iter );
        } else {
            ++iter;
        }
    }
}

void options_manager::init()
{
    options.clear();
    for( Page &p : pages_ ) {
        p.items_.clear();
    }

    add_options_general();
    add_options_interface();
    add_options_graphics();
    add_options_debug();
    add_options_world_default();
    add_options_android();

    for( Page &p : pages_ ) {
        p.removeRepeatedEmptyLines();
    }
}

void options_manager::add_options_general()
{
    const auto add_empty_line = [&]() {
        general_page_.items_.emplace_back();
    };

    add( "DEF_CHAR_NAME", "general", translate_marker( "Default character name" ),
         translate_marker( "Set a default character name that will be used instead of a random name on character creation." ),
         "", 30
       );

    add_empty_line();

    add( "AUTO_PICKUP", "general", translate_marker( "Auto pickup enabled" ),
         translate_marker( "Enable item auto pickup.  Change pickup rules with the Auto Pickup Manager." ),
         false
       );

    add( "AUTO_PICKUP_ADJACENT", "general", translate_marker( "Auto pickup adjacent" ),
         translate_marker( "If true, will enable to pickup items one tile around to the player.  You can assign No Auto Pickup zones with the Zones Manager 'Y' key for e.g.  your homebase." ),
         false
       );

    get_option( "AUTO_PICKUP_ADJACENT" ).setPrerequisite( "AUTO_PICKUP" );

    add( "AUTO_PICKUP_WEIGHT_LIMIT", "general", translate_marker( "Auto pickup weight limit" ),
         translate_marker( "Auto pickup items with weight less than or equal to [option] * 50 grams.  You must also set the small items option.  '0' disables this option" ),
         0, 20, 0
       );

    get_option( "AUTO_PICKUP_WEIGHT_LIMIT" ).setPrerequisite( "AUTO_PICKUP" );

    add( "AUTO_PICKUP_VOL_LIMIT", "general", translate_marker( "Auto pickup volume limit" ),
         translate_marker( "Auto pickup items with volume less than or equal to [option] * 50 milliliters.  You must also set the light items option.  '0' disables this option" ),
         0, 20, 0
       );

    get_option( "AUTO_PICKUP_VOL_LIMIT" ).setPrerequisite( "AUTO_PICKUP" );

    add( "AUTO_PICKUP_SAFEMODE", "general", translate_marker( "Auto pickup safe mode" ),
         translate_marker( "Auto pickup is disabled as long as you can see monsters nearby.  This is affected by 'Safe Mode proximity distance'." ),
         false
       );

    get_option( "AUTO_PICKUP_SAFEMODE" ).setPrerequisite( "AUTO_PICKUP" );

    add( "NO_AUTO_PICKUP_ZONES_LIST_ITEMS", "general",
         translate_marker( "List items within no auto pickup zones" ),
         translate_marker( "If false, you will not see messages about items, you step on, within no auto pickup zones." ),
         true
       );

    get_option( "NO_AUTO_PICKUP_ZONES_LIST_ITEMS" ).setPrerequisite( "AUTO_PICKUP" );

    add_empty_line();

    add( "AUTO_FEATURES", "general", translate_marker( "Additional auto features" ),
         translate_marker( "If true, enables configured auto features below.  Disabled as long as any enemy monster is seen." ),
         false
       );

    add( "AUTO_PULP_BUTCHER", "general", translate_marker( "Auto pulp or butcher" ),
         translate_marker( "Action to perform when 'Auto pulp or butcher' is enabled.  Pulp: Pulp corpses you stand on.  - Pulp Adjacent: Also pulp corpses adjacent from you.  - Butcher: Butcher corpses you stand on." ),
    { { "off", to_translation( "options", "Disabled" ) }, { "pulp", translate_marker( "Pulp" ) }, { "pulp_adjacent", translate_marker( "Pulp Adjacent" ) }, { "butcher", translate_marker( "Butcher" ) } },
    "off"
       );

    get_option( "AUTO_PULP_BUTCHER" ).setPrerequisite( "AUTO_FEATURES" );

    add( "AUTO_MINING", "general", translate_marker( "Auto mining" ),
         translate_marker( "If true, enables automatic use of wielded pickaxes and jackhammers whenever trying to move into mineable terrain." ),
         false
       );

    get_option( "AUTO_MINING" ).setPrerequisite( "AUTO_FEATURES" );

    add( "AUTO_FORAGING", "general", translate_marker( "Auto foraging" ),
         translate_marker( "Action to perform when 'Auto foraging' is enabled.  Bushes: Only forage bushes.  - Trees: Only forage trees.  - Everything: Forage bushes, trees, and everything else including flowers, cattails etc." ),
    { { "off", to_translation( "options", "Disabled" ) }, { "bushes", translate_marker( "Bushes" ) }, { "trees", translate_marker( "Trees" ) }, { "both", translate_marker( "Everything" ) } },
    "off"
       );

    get_option( "AUTO_FORAGING" ).setPrerequisite( "AUTO_FEATURES" );

    add_empty_line();

    add( "DANGEROUS_PICKUPS", "general", translate_marker( "Dangerous pickups" ),
         translate_marker( "If false, will cause player to drop new items that cause them to exceed the weight limit." ),
         false
       );

    add( "DANGEROUS_TERRAIN_WARNING_PROMPT", "general",
         translate_marker( "Dangerous terrain warning prompt" ),
         translate_marker( "Always: You will be prompted to move onto dangerous tiles.  Running: You will only be able to move onto dangerous tiles while running and will be prompted.  Crouching: You will only be able to move onto a dangerous tile while crouching and will be prompted.  Never:  You will not be able to move onto a dangerous tile unless running and will not be warned or prompted." ),
    { { "ALWAYS", to_translation( "Always" ) }, { "RUNNING", translate_marker( "Running" ) }, { "CROUCHING", translate_marker( "Crouching" ) }, { "NEVER", translate_marker( "Never" ) } },
    "ALWAYS"
       );

    add_empty_line();

    add( "SAFEMODE", "general", translate_marker( "Safe mode" ),
         translate_marker( "If true, will hold the game and display a warning if a hostile monster/npc is approaching." ),
         true
       );

    add( "SAFEMODEPROXIMITY", "general", translate_marker( "Safe mode proximity distance" ),
         translate_marker( "If safe mode is enabled, distance to hostiles at which safe mode should show a warning.  0 = Max player view distance.  This option only has effect when no safe mode rule is specified.  Otherwise, edit the default rule in Safe Mode Manager instead of this value." ),
         0, MAX_VIEW_DISTANCE, 0
       );

    add( "SAFEMODEVEH", "general", translate_marker( "Safe mode when driving" ),
         translate_marker( "When true, safe mode will alert you of hostiles while you are driving a vehicle." ),
         false
       );

    add( "AUTOSAFEMODE", "general", translate_marker( "Auto reactivate safe mode" ),
         translate_marker( "If true, safe mode will automatically reactivate after a certain number of turns.  See option 'Turns to auto reactivate safe mode.'" ),
         false
       );

    add( "AUTOSAFEMODETURNS", "general", translate_marker( "Turns to auto reactivate safe mode" ),
         translate_marker( "Number of turns after which safe mode is reactivated.  Will only reactivate if no hostiles are in 'Safe mode proximity distance.'" ),
         1, 600, 50
       );

    add( "SAFEMODEIGNORETURNS", "general", translate_marker( "Turns to remember ignored monsters" ),
         translate_marker( "Number of turns an ignored monster stays ignored after it is no longer seen.  0 disables this option and monsters are permanently ignored." ),
         0, 3600, 200
       );

    add_empty_line();

    add( "TURN_DURATION", "general", translate_marker( "Realtime turn progression" ),
         translate_marker( "If enabled, monsters will take periodic gameplay turns.  This value is the delay between each turn, in seconds.  Works best with Safe Mode disabled.  0 = disabled." ),
         0.0, 10.0, 0.0, 0.05
       );

    add_empty_line();

    add( "AUTOSAVE", "general", translate_marker( "Autosave" ),
         translate_marker( "If true, game will periodically save the map.  Autosaves occur based on in-game turns or real-time minutes, whichever is larger." ),
         true
       );

    add( "AUTOSAVE_TURNS", "general", translate_marker( "Game turns between autosaves" ),
         translate_marker( "Number of game turns between autosaves" ),
         10, 1000, 50
       );

    get_option( "AUTOSAVE_TURNS" ).setPrerequisite( "AUTOSAVE" );

    add( "AUTOSAVE_MINUTES", "general", translate_marker( "Real minutes between autosaves" ),
         translate_marker( "Number of real time minutes between autosaves" ),
         0, 127, 5
       );

    get_option( "AUTOSAVE_MINUTES" ).setPrerequisite( "AUTOSAVE" );

    add_empty_line();

    add( "AUTO_NOTES", "general", translate_marker( "Auto notes" ),
         translate_marker( "If true, automatically sets notes" ),
         true
       );

    add( "AUTO_NOTES_STAIRS", "general", translate_marker( "Auto notes (stairs)" ),
         translate_marker( "If true, automatically sets notes on places that have stairs that go up or down" ),
         false
       );

    get_option( "AUTO_NOTES_STAIRS" ).setPrerequisite( "AUTO_NOTES" );

    add( "AUTO_NOTES_MAP_EXTRAS", "general", translate_marker( "Auto notes (map extras)" ),
         translate_marker( "If true, automatically sets notes on places that contain various map extras" ),
         true
       );

    get_option( "AUTO_NOTES_MAP_EXTRAS" ).setPrerequisite( "AUTO_NOTES" );

    add_empty_line();

    add( "CIRCLEDIST", "general", translate_marker( "Circular distances" ),
         translate_marker( "If true, the game will calculate range in a realistic way: light sources will be circles, diagonal movement will cover more ground and take longer.  If disabled, everything is square: moving to the northwest corner of a building takes as long as moving to the north wall." ),
         true
       );

    add( "DROP_EMPTY", "general", translate_marker( "Drop empty containers" ),
         translate_marker( "Set to drop empty containers after use.  No: Don't drop any.  - Watertight: All except watertight containers.  - All: Drop all containers." ),
    { { "no", translate_marker( "No" ) }, { "watertight", translate_marker( "Watertight" ) }, { "all", translate_marker( "All" ) } },
    "no"
       );

    add( "DEATHCAM", "general", translate_marker( "DeathCam" ),
         translate_marker( "Always: Always start deathcam.  Ask: Query upon death.  Never: Never show deathcam." ),
    { { "always", translate_marker( "Always" ) }, { "ask", translate_marker( "Ask" ) }, { "never", translate_marker( "Never" ) } },
    "ask"
       );

    add_empty_line();

    add( "SOUND_ENABLED", "general", translate_marker( "Sound Enabled" ),
         translate_marker( "If true, music and sound are enabled." ),
         true, COPT_NO_SOUND_HIDE
       );

    add( "SOUNDPACKS", "general", translate_marker( "Choose soundpack" ),
         translate_marker( "Choose the soundpack you want to use.  Requires restart." ),
         build_soundpacks_list(), "basic", COPT_NO_SOUND_HIDE
       ); // populate the options dynamically

    get_option( "SOUNDPACKS" ).setPrerequisite( "SOUND_ENABLED" );

    add( "MUSIC_VOLUME", "general", translate_marker( "Music volume" ),
         translate_marker( "Adjust the volume of the music being played in the background." ),
         0, 128, 100, COPT_NO_SOUND_HIDE
       );

    get_option( "MUSIC_VOLUME" ).setPrerequisite( "SOUND_ENABLED" );

    add( "SOUND_EFFECT_VOLUME", "general", translate_marker( "Sound effect volume" ),
         translate_marker( "Adjust the volume of sound effects being played by the game." ),
         0, 128, 100, COPT_NO_SOUND_HIDE
       );

    get_option( "SOUND_EFFECT_VOLUME" ).setPrerequisite( "SOUND_ENABLED" );

    add( "AMBIENT_SOUND_VOLUME", "general", translate_marker( "Ambient sound volume" ),
         translate_marker( "Adjust the volume of ambient sounds being played by the game." ),
         0, 128, 100, COPT_NO_SOUND_HIDE
       );

    get_option( "AMBIENT_SOUND_VOLUME" ).setPrerequisite( "SOUND_ENABLED" );
}

void options_manager::add_options_interface()
{
    const auto add_empty_line = [&]() {
        interface_page_.items_.emplace_back();
    };

    // TODO: scan for languages like we do for tilesets.
    add( "USE_LANG", "interface", translate_marker( "Language" ),
         translate_marker( "Switch Language." ), options_manager::lang_options, "" );

    add_empty_line();

    add( "USE_CELSIUS", "interface", translate_marker( "Temperature units" ),
         translate_marker( "Switch between Celsius, Fahrenheit and Kelvin." ),
    { { "fahrenheit", translate_marker( "Fahrenheit" ) }, { "celsius", translate_marker( "Celsius" ) }, { "kelvin", translate_marker( "Kelvin" ) } },
    "fahrenheit"
       );

    add( "USE_METRIC_SPEEDS", "interface", translate_marker( "Speed units" ),
         translate_marker( "Switch between mph, km/h and tiles/turn." ),
    { { "mph", translate_marker( "mph" ) }, { "km/h", translate_marker( "km/h" ) }, { "t/t", translate_marker( "tiles/turn" ) } },
    "mph"
       );

    add( "USE_METRIC_WEIGHTS", "interface", translate_marker( "Mass units" ),
         translate_marker( "Switch between kg and lbs." ),
    { { "lbs", translate_marker( "lbs" ) }, { "kg", translate_marker( "kg" ) } }, "lbs"
       );

    add( "VOLUME_UNITS", "interface", translate_marker( "Volume units" ),
         translate_marker( "Switch between the Cup ( c ), Liter ( L ) or Quart ( qt )." ),
    { { "c", translate_marker( "Cup" ) }, { "l", translate_marker( "Liter" ) }, { "qt", translate_marker( "Quart" ) } },
    "l"
       );
    add( "DISTANCE_UNITS", "interface", translate_marker( "Distance units" ),
         translate_marker( "Metric or Imperial" ),
    { { "metric", translate_marker( "Metric" ) }, { "imperial", translate_marker( "Imperial" ) } },
    "imperial" );

    add( "24_HOUR", "interface", translate_marker( "Time format" ),
         translate_marker( "12h: AM/PM, e.g. 7:31 AM - Military: 24h Military, e.g. 0731 - 24h: Normal 24h, e.g. 7:31" ),
         //~ 12h time, e.g.  11:59pm
    {   { "12h", translate_marker( "12h" ) },
        //~ Military time, e.g.  2359
        { "military", translate_marker( "Military" ) },
        //~ 24h time, e.g.  23:59
        { "24h", translate_marker( "24h" ) }
    },
    "12h" );

    add_empty_line();

    add( "FORCE_CAPITAL_YN", "interface", translate_marker( "Force Y/N in prompts" ),
         translate_marker( "If true, Y/N prompts are case-sensitive and y and n are not accepted." ),
         true
       );

    add( "SNAP_TO_TARGET", "interface", translate_marker( "Snap to target" ),
         translate_marker( "If true, automatically follow the crosshair when firing/throwing." ),
         false
       );

    add( "QUERY_DISASSEMBLE", "interface", translate_marker( "Query on disassembly while butchering" ),
         translate_marker( "If true, will query before disassembling items while butchering." ),
         true
       );

    add( "QUERY_KEYBIND_REMOVAL", "interface", translate_marker( "Query on keybinding removal" ),
         translate_marker( "If true, will query before removing a keybinding from a hotkey." ),
         true
       );

    add( "CLOSE_ADV_INV", "interface", translate_marker( "Close advanced inventory on move all" ),
         translate_marker( "If true, will close the advanced inventory when the move all items command is used." ),
         false
       );

    add( "OPEN_DEFAULT_ADV_INV", "interface",
         translate_marker( "Open default advanced inventory layout" ),
         translate_marker( "Open default advanced inventory layout instead of last opened layout" ),
         false
       );

    add( "INV_USE_ACTION_NAMES", "interface", translate_marker( "Display actions in Use Item menu" ),
         translate_marker( "If true, actions ( like \"Read\", \"Smoke\", \"Wrap tighter\" ) will be displayed next to the corresponding items." ),
         true
       );

    add( "AUTOSELECT_SINGLE_VALID_TARGET", "interface",
         translate_marker( "Autoselect if exactly one valid target" ),
         translate_marker( "If true, directional actions ( like \"Examine\", \"Open\", \"Pickup\" ) "
                           "will autoselect an adjacent tile if there is exactly one valid target." ),
         true
       );

    add_empty_line();

    add( "DIAG_MOVE_WITH_MODIFIERS_MODE", "interface",
         translate_marker( "Diagonal movement with cursor keys and modifiers" ),
         /*
         Possible modes:

         # None

         # Mode 1: Numpad Emulation

         * Press and keep holding Ctrl
         * Press and release ↑ to set it as the modifier (until Ctrl is released)
         * Press and release → to get the move ↑ + → = ↗ i.e. just like pressing and releasing 9
         * Holding → results in repeated ↗, so just like holding 9
         * If I press any other direction, they are similarly modified by ↑, both for single presses and while holding.

         # Mode 2: CW/CCW

         * `Shift` + `Cursor Left` -> `7` = `Move Northwest`;
         * `Shift` + `Cursor Right` -> `3` = `Move Southeast`;
         * `Shift` + `Cursor Up` -> `9` = `Move Northeast`;
         * `Shift` + `Cursor Down` -> `1` = `Move Southwest`.

         and

         * `Ctrl` + `Cursor Left` -> `1` = `Move Southwest`;
         * `Ctrl` + `Cursor Right` -> `9` = `Move Northeast`;
         * `Ctrl` + `Cursor Up` -> `7` = `Move Northwest`;
         * `Ctrl` + `Cursor Down` -> `3` = `Move Southeast`.

         # Mode 3: L/R Tilt

         * `Shift` + `Cursor Left` -> `7` = `Move Northwest`;
         * `Ctrl` + `Cursor Left` -> `3` = `Move Southeast`;
         * `Shift` + `Cursor Right` -> `9` = `Move Northeast`;
         * `Ctrl` + `Cursor Right` -> `1` = `Move Southwest`.

         */
    translate_marker( "Allows diagonal movement with cursor keys using CTRL and SHIFT modifiers.  Diagonal movement action keys are taken from keybindings, so you need these to be configured." ), { { "none", translate_marker( "None" ) }, { "mode1", translate_marker( "Mode 1: Numpad Emulation" ) }, { "mode2", translate_marker( "Mode 2: CW/CCW" ) }, { "mode3", translate_marker( "Mode 3: L/R Tilt" ) } },
    "none", COPT_CURSES_HIDE );

    add_empty_line();

    add( "VEHICLE_ARMOR_COLOR", "interface", translate_marker( "Vehicle plating changes part color" ),
         translate_marker( "If true, vehicle parts will change color if they are armor plated" ),
         true
       );

    add( "DRIVING_VIEW_OFFSET", "interface", translate_marker( "Auto-shift the view while driving" ),
         translate_marker( "If true, view will automatically shift towards the driving direction" ),
         true
       );

    add( "VEHICLE_DIR_INDICATOR", "interface", translate_marker( "Draw vehicle facing indicator" ),
         translate_marker( "If true, when controlling a vehicle, a white 'X' ( in curses version ) or a crosshair ( in tiles version ) at distance 10 from the center will display its current facing." ),
         true
       );

    add( "REVERSE_STEERING", "interface", translate_marker( "Reverse steering direction in reverse" ),
         translate_marker( "If true, when driving a vehicle in reverse, steering should also reverse like real life." ),
         false
       );

    add_empty_line();

    add( "SIDEBAR_POSITION", "interface", translate_marker( "Sidebar position" ),
         translate_marker( "Switch between sidebar on the left or on the right side.  Requires restart." ),
         //~ sidebar position
    { { "left", translate_marker( "Left" ) }, { "right", translate_marker( "Right" ) } }, "right"
       );

    add( "SIDEBAR_SPACERS", "interface", translate_marker( "Draw sidebar spacers" ),
         translate_marker( "If true, adds an extra space between sidebar panels." ),
         false
       );

    add( "LOG_FLOW", "interface", translate_marker( "Message log flow" ),
         translate_marker( "Where new log messages should show." ),
         //~ sidebar/message log flow direction
    { { "new_top", translate_marker( "Top" ) }, { "new_bottom", translate_marker( "Bottom" ) } },
    "new_bottom"
       );

    add( "MESSAGE_TTL", "interface", translate_marker( "Sidebar log message display duration" ),
         translate_marker( "Number of turns after which a message will be removed from the sidebar log.  '0' disables this option." ),
         0, 1000, 0
       );

    add( "MESSAGE_COOLDOWN", "interface", translate_marker( "Message cooldown" ),
         translate_marker( "Number of turns during which similar messages are hidden.  '0' disables this option." ),
         0, 1000, 0
       );

    add( "NO_UNKNOWN_COMMAND_MSG", "interface",
         translate_marker( "Suppress \"unknown command\" messages" ),
         translate_marker( "If true, pressing a key with no set function will not display a notice in the chat log." ),
         false
       );

    add( "LOOKAROUND_POSITION", "interface", translate_marker( "Look around position" ),
         translate_marker( "Switch between look around panel being left or right." ),
    { { "left", translate_marker( "Left" ) }, { "right", translate_marker( "Right" ) } },
    "right"
       );

    add( "PICKUP_POSITION", "interface", translate_marker( "Pickup position" ),
         translate_marker( "Switch between pickup panel being left, right, or overlapping the sidebar." ),
    { { "left", translate_marker( "Left" ) }, { "right", translate_marker( "Right" ) }, { "overlapping", translate_marker( "Overlapping" ) } },
    "left"
       );

    add( "ACCURACY_DISPLAY", "interface", translate_marker( "Aim window display style" ),
         translate_marker( "How should confidence and steadiness be communicated to the player." ),
         //~ aim bar style - bars or numbers
    { { "numbers", translate_marker( "Numbers" ) }, { "bars", translate_marker( "Bars" ) } }, "bars"
       );

    add( "MORALE_STYLE", "interface", translate_marker( "Morale style" ),
         translate_marker( "Morale display style in sidebar." ),
    { { "vertical", translate_marker( "Vertical" ) }, { "horizontal", translate_marker( "Horizontal" ) } },
    "Vertical"
       );

    add( "AIM_WIDTH", "interface", translate_marker( "Full screen Advanced Inventory Manager" ),
         translate_marker( "If true, Advanced Inventory Manager menu will fit full screen, otherwise it will leave sidebar visible." ),
         false
       );

    add_empty_line();

    add( "MOVE_VIEW_OFFSET", "interface", translate_marker( "Move view offset" ),
         translate_marker( "Move view by how many squares per keypress." ),
         1, 50, 1
       );

    add( "FAST_SCROLL_OFFSET", "interface", translate_marker( "Overmap fast scroll offset" ),
         translate_marker( "With Fast Scroll option enabled, shift view on the overmap and while looking around by this many squares per keypress." ),
         1, 50, 5
       );

    add( "MENU_SCROLL", "interface", translate_marker( "Centered menu scrolling" ),
         translate_marker( "If true, menus will start scrolling in the center of the list, and keep the list centered." ),
         true
       );

    add( "SHIFT_LIST_ITEM_VIEW", "interface", translate_marker( "Shift list item view" ),
         translate_marker( "Centered or to edge, shift the view toward the selected item if it is outside of your current viewport." ),
    { { "false", translate_marker( "False" ) }, { "centered", translate_marker( "Centered" ) }, { "edge", translate_marker( "To edge" ) } },
    "centered"
       );

    add( "AUTO_INV_ASSIGN", "interface", translate_marker( "Auto inventory letters" ),
         translate_marker( "Enabled: automatically assign letters to any carried items that lack them.  Disabled: do not auto-assign letters.  "
    "Favorites: only auto-assign letters to favorited items." ), {
        { "disabled", translate_marker( "Disabled" ) },
        { "enabled", translate_marker( "Enabled" ) },
        { "favorites", translate_marker( "Favorites" ) }
    },
    "favorites" );

    add( "ITEM_HEALTH_BAR", "interface", translate_marker( "Show item health bars" ),
         // NOLINTNEXTLINE(cata-text-style): one space after "etc."
         translate_marker( "If true, show item health bars instead of reinforced, scratched etc. text." ),
         true
       );

    add( "ITEM_SYMBOLS", "interface", translate_marker( "Show item symbols" ),
         translate_marker( "If true, show item symbols in inventory and pick up menu." ),
         false
       );
    add( "AMMO_IN_NAMES", "interface", translate_marker( "Add ammo to weapon/magazine names" ),
         translate_marker( "If true, the default ammo is added to weapon and magazine names.  For example \"Mosin-Nagant M44 (4/5)\" becomes \"Mosin-Nagant M44 (4/5 7.62x54mm)\"." ),
         true
       );

    add_empty_line();

    add( "ENABLE_JOYSTICK", "interface", translate_marker( "Enable joystick" ),
         translate_marker( "Enable input from joystick." ),
         true, COPT_CURSES_HIDE
       );

    add( "HIDE_CURSOR", "interface", translate_marker( "Hide mouse cursor" ),
         translate_marker( "Show: Cursor is always shown.  Hide: Cursor is hidden.  HideKB: Cursor is hidden on keyboard input and unhidden on mouse movement." ),
         //~ show mouse cursor
    {   { "show", translate_marker( "Show" ) },
        //~ hide mouse cursor
        { "hide", translate_marker( "Hide" ) },
        //~ hide mouse cursor when keyboard is used
        { "hidekb", translate_marker( "HideKB" ) }
    },
    "show", COPT_CURSES_HIDE );

    add( "EDGE_SCROLL", "interface", translate_marker( "Edge scrolling" ),
    translate_marker( "Edge scrolling with the mouse." ), {
        std::make_tuple( -1, translate_marker( "Disabled" ) ),
        std::make_tuple( 100, translate_marker( "Slow" ) ),
        std::make_tuple( 30, translate_marker( "Normal" ) ),
        std::make_tuple( 10, translate_marker( "Fast" ) )
    },
    30, 30, COPT_CURSES_HIDE );

}

void options_manager::add_options_graphics()
{
    const auto add_empty_line = [&]() {
        graphics_page_.items_.emplace_back();
    };

    add( "ANIMATIONS", "graphics", translate_marker( "Animations" ),
         translate_marker( "If true, will display enabled animations." ),
         true
       );

    add( "ANIMATION_RAIN", "graphics", translate_marker( "Rain animation" ),
         translate_marker( "If true, will display weather animations." ),
         true
       );

    get_option( "ANIMATION_RAIN" ).setPrerequisite( "ANIMATIONS" );

    add( "ANIMATION_PROJECTILES", "graphics", translate_marker( "Projectile animation" ),
         translate_marker( "If true, will display animations for projectiles like bullets, arrows, and thrown items." ),
         true
       );

    get_option( "ANIMATION_PROJECTILES" ).setPrerequisite( "ANIMATIONS" );

    add( "ANIMATION_SCT", "graphics", translate_marker( "SCT animation" ),
         translate_marker( "If true, will display scrolling combat text animations." ),
         true
       );

    get_option( "ANIMATION_SCT" ).setPrerequisite( "ANIMATIONS" );

    add( "ANIMATION_SCT_USE_FONT", "graphics", translate_marker( "SCT with Unicode font" ),
         translate_marker( "If true, will display scrolling combat text with Unicode font." ),
         true
       );

    get_option( "ANIMATION_SCT_USE_FONT" ).setPrerequisite( "ANIMATION_SCT" );

    add( "ANIMATION_DELAY", "graphics", translate_marker( "Animation delay" ),
         translate_marker( "The amount of time to pause between animation frames in ms." ),
         0, 100, 10
       );

    get_option( "ANIMATION_DELAY" ).setPrerequisite( "ANIMATIONS" );

    add( "FORCE_REDRAW", "graphics", translate_marker( "Force redraw" ),
         translate_marker( "If true, forces the game to redraw at least once per turn." ),
         true
       );

    add_empty_line();

    add( "TERMINAL_X", "graphics", translate_marker( "Terminal width" ),
         translate_marker( "Set the size of the terminal along the X axis.  Requires restart." ),
         80, 960, 80, COPT_POSIX_CURSES_HIDE
       );

    add( "TERMINAL_Y", "graphics", translate_marker( "Terminal height" ),
         translate_marker( "Set the size of the terminal along the Y axis.  Requires restart." ),
         24, 270, 24, COPT_POSIX_CURSES_HIDE
       );

    add_empty_line();

    add( "FONT_BLENDING", "graphics", translate_marker( "Font blending" ),
         translate_marker( "If true, fonts will look better." ),
         false, COPT_CURSES_HIDE
       );

    add( "FONT_WIDTH", "graphics", translate_marker( "Font width" ),
         translate_marker( "Set the font width.  Requires restart." ),
         8, 100, 8, COPT_CURSES_HIDE
       );

    add( "FONT_HEIGHT", "graphics", translate_marker( "Font height" ),
         translate_marker( "Set the font height.  Requires restart." ),
         8, 100, 16, COPT_CURSES_HIDE
       );

    add( "FONT_SIZE", "graphics", translate_marker( "Font size" ),
         translate_marker( "Set the font size.  Requires restart." ),
         8, 100, 16, COPT_CURSES_HIDE
       );

    add( "MAP_FONT_WIDTH", "graphics", translate_marker( "Map font width" ),
         translate_marker( "Set the map font width.  Requires restart." ),
         8, 100, 16, COPT_CURSES_HIDE
       );

    add( "MAP_FONT_HEIGHT", "graphics", translate_marker( "Map font height" ),
         translate_marker( "Set the map font height.  Requires restart." ),
         8, 100, 16, COPT_CURSES_HIDE
       );

    add( "MAP_FONT_SIZE", "graphics", translate_marker( "Map font size" ),
         translate_marker( "Set the map font size.  Requires restart." ),
         8, 100, 16, COPT_CURSES_HIDE
       );

    add( "OVERMAP_FONT_WIDTH", "graphics", translate_marker( "Overmap font width" ),
         translate_marker( "Set the overmap font width.  Requires restart." ),
         8, 100, 16, COPT_CURSES_HIDE
       );

    add( "OVERMAP_FONT_HEIGHT", "graphics", translate_marker( "Overmap font height" ),
         translate_marker( "Set the overmap font height.  Requires restart." ),
         8, 100, 16, COPT_CURSES_HIDE
       );

    add( "OVERMAP_FONT_SIZE", "graphics", translate_marker( "Overmap font size" ),
         translate_marker( "Set the overmap font size.  Requires restart." ),
         8, 100, 16, COPT_CURSES_HIDE
       );

    add( "USE_DRAW_ASCII_LINES_ROUTINE", "graphics", translate_marker( "SDL ASCII lines" ),
         translate_marker( "Use SDL ASCII line drawing routine instead of Unicode Line Drawing characters.  Use this option when your selected font doesn't contain necessary glyphs." ),
         true, COPT_CURSES_HIDE
       );

    add( "ENABLE_ASCII_ART_ITEM", "graphics",
         translate_marker( "Enable ASCII art in item descriptions" ),
         translate_marker( "When available item description will show a picture of the item in ascii art." ),
         true, COPT_NO_HIDE
       );

    add_empty_line();

    add( "USE_TILES", "graphics", translate_marker( "Use tiles" ),
         translate_marker( "If true, replaces some TTF rendered text with tiles." ),
         true, COPT_CURSES_HIDE
       );

    add( "TILES", "graphics", translate_marker( "Choose tileset" ),
         translate_marker( "Choose the tileset you want to use." ),
         build_tilesets_list(), "UNDEAD_PEOPLE", COPT_CURSES_HIDE
       ); // populate the options dynamically

    get_option( "TILES" ).setPrerequisite( "USE_TILES" );

    add_empty_line();

    add( "MEMORY_MAP_MODE", "graphics", translate_marker( "Memory map drawing mode" ),
    translate_marker( "Specified the mode in which the memory map is drawn.  Requires restart." ), {
        { "color_pixel_darken", translate_marker( "Darkened" ) },
        { "color_pixel_sepia", translate_marker( "Sepia" ) }
    }, "color_pixel_sepia", COPT_CURSES_HIDE
       );

    add_empty_line();

    add( "PIXEL_MINIMAP", "graphics", translate_marker( "Pixel minimap" ),
         translate_marker( "If true, shows the pixel-detail minimap in game after the save is loaded.  Use the 'Toggle Pixel Minimap' action key to change its visibility during gameplay." ),
         true, COPT_CURSES_HIDE
       );

    add( "PIXEL_MINIMAP_MODE", "graphics", translate_marker( "Pixel minimap drawing mode" ),
    translate_marker( "Specified the mode in which the minimap drawn." ), {
        { "solid", translate_marker( "Solid" ) },
        { "squares", translate_marker( "Squares" ) },
        { "dots", translate_marker( "Dots" ) }
    }, "dots", COPT_CURSES_HIDE
       );

    get_option( "PIXEL_MINIMAP_MODE" ).setPrerequisite( "PIXEL_MINIMAP" );

    add( "PIXEL_MINIMAP_BRIGHTNESS", "graphics", translate_marker( "Pixel minimap brightness" ),
         translate_marker( "Overall brightness of pixel-detail minimap." ),
         10, 300, 100, COPT_CURSES_HIDE
       );

    get_option( "PIXEL_MINIMAP_BRIGHTNESS" ).setPrerequisite( "PIXEL_MINIMAP" );

    add( "PIXEL_MINIMAP_HEIGHT", "graphics", translate_marker( "Pixel minimap height" ),
         translate_marker( "Height of pixel-detail minimap, measured in terminal rows.  Set to 0 for default spacing." ),
         0, 100, 0, COPT_CURSES_HIDE
       );

    get_option( "PIXEL_MINIMAP_HEIGHT" ).setPrerequisite( "PIXEL_MINIMAP" );

    add( "PIXEL_MINIMAP_SCALE_TO_FIT", "graphics", translate_marker( "Scale pixel minimap" ),
         translate_marker( "Scale pixel minimap to fit its surroundings.  May produce crappy results, especially in modes other than \"Solid\"." ),
         false, COPT_CURSES_HIDE
       );

    get_option( "PIXEL_MINIMAP_SCALE_TO_FIT" ).setPrerequisite( "PIXEL_MINIMAP" );

    add( "PIXEL_MINIMAP_RATIO", "graphics", translate_marker( "Maintain pixel minimap aspect ratio" ),
         translate_marker( "Preserves the square shape of tiles shown on the pixel minimap." ),
         true, COPT_CURSES_HIDE
       );

    get_option( "PIXEL_MINIMAP_RATIO" ).setPrerequisite( "PIXEL_MINIMAP" );

    add( "PIXEL_MINIMAP_BEACON_SIZE", "graphics",
         translate_marker( "Creature beacon size" ),
         translate_marker( "Controls how big the creature beacons are.  Value is in minimap tiles." ),
         1, 4, 2, COPT_CURSES_HIDE
       );

    get_option( "PIXEL_MINIMAP_BEACON_SIZE" ).setPrerequisite( "PIXEL_MINIMAP" );

    add( "PIXEL_MINIMAP_BLINK", "graphics", translate_marker( "Hostile creature beacon blink speed" ),
         translate_marker( "Controls how fast the hostile creature beacons blink on the pixel minimap.  Value is multiplied by 200 ms.  Set to 0 to disable." ),
         0, 50, 10, COPT_CURSES_HIDE
       );

    get_option( "PIXEL_MINIMAP_BLINK" ).setPrerequisite( "PIXEL_MINIMAP" );

    add_empty_line();

#if defined(TILES)
    std::vector<options_manager::id_and_option> display_list = cata_tiles::build_display_list();
    add( "DISPLAY", "graphics", translate_marker( "Display" ),
         translate_marker( "Sets which video display will be used to show the game.  Requires restart." ),
         display_list,
         display_list.front().first, COPT_CURSES_HIDE );
#endif

#if !defined(__ANDROID__) // Android is always fullscreen
    add( "FULLSCREEN", "graphics", translate_marker( "Fullscreen" ),
         translate_marker( "Starts Cataclysm in one of the fullscreen modes.  Requires restart." ),
    { { "no", translate_marker( "No" ) }, { "maximized", translate_marker( "Maximized" ) }, { "fullscreen", translate_marker( "Fullscreen" ) }, { "windowedbl", translate_marker( "Windowed borderless" ) } },
    "windowedbl", COPT_CURSES_HIDE
       );
#endif

#if !defined(__ANDROID__)
#   if !defined(TILES)
    // No renderer selection in non-TILES mode
    add( "RENDERER", "graphics", translate_marker( "Renderer" ),
    translate_marker( "Set which renderer to use.  Requires restart." ),   {   { "software", translate_marker( "software" ) } },
    "software", COPT_CURSES_HIDE );
#   else
    std::vector<options_manager::id_and_option> renderer_list = cata_tiles::build_renderer_list();
    add( "RENDERER", "graphics", translate_marker( "Renderer" ),
         translate_marker( "Set which renderer to use.  Requires restart." ), renderer_list,
         renderer_list.front().first, COPT_CURSES_HIDE );
#   endif

#else
    add( "SOFTWARE_RENDERING", "graphics", translate_marker( "Software rendering" ),
         translate_marker( "Use software renderer instead of graphics card acceleration.  Requires restart." ),
         // take default setting from pre-game settings screen - important as both software + hardware rendering have issues with specific devices
         android_get_default_setting( "Software rendering", false ),
         COPT_CURSES_HIDE
       );
#endif

#if defined(SDL_HINT_RENDER_BATCHING)
    add( "RENDER_BATCHING", "graphics", translate_marker( "Allow render batching" ),
         translate_marker( "Use render batching for 2D render API to make it more efficient.  Requires restart." ),
         true, COPT_CURSES_HIDE
       );
#endif
    add( "FRAMEBUFFER_ACCEL", "graphics", translate_marker( "Software framebuffer acceleration" ),
         translate_marker( "Use hardware acceleration for the framebuffer when using software rendering.  Requires restart." ),
         false, COPT_CURSES_HIDE
       );

#if defined(__ANDROID__)
    get_option( "FRAMEBUFFER_ACCEL" ).setPrerequisite( "SOFTWARE_RENDERING" );
#else
    get_option( "FRAMEBUFFER_ACCEL" ).setPrerequisite( "RENDERER", "software" );
#endif

    add( "USE_COLOR_MODULATED_TEXTURES", "graphics", translate_marker( "Use color modulated textures" ),
         translate_marker( "If true, tries to use color modulated textures to speed-up ASCII drawing.  Requires restart." ),
         false, COPT_CURSES_HIDE
       );

    add( "SCALING_MODE", "graphics", translate_marker( "Scaling mode" ),
         translate_marker( "Sets the scaling mode, 'none' ( default ) displays at the game's native resolution, 'nearest'  uses low-quality but fast scaling, and 'linear' provides high-quality scaling." ),
         //~ Do not scale the game image to the window size.
    {   { "none", translate_marker( "No scaling" ) },
        //~ An algorithm for image scaling.
        { "nearest", translate_marker( "Nearest neighbor" ) },
        //~ An algorithm for image scaling.
        { "linear", translate_marker( "Linear filtering" ) }
    },
    "none", COPT_CURSES_HIDE );

#if !defined(__ANDROID__)
    add( "SCALING_FACTOR", "graphics", translate_marker( "Scaling factor" ),
    translate_marker( "Factor by which to scale the display.  Requires restart." ), {
        { "1", translate_marker( "1x" ) },
        { "2", translate_marker( "2x" )},
        { "4", translate_marker( "4x" )}
    },
    "1", COPT_CURSES_HIDE );
#endif

}

void options_manager::add_options_debug()
{
    const auto add_empty_line = [&]() {
        debug_page_.items_.emplace_back();
    };

    add( "DISTANCE_INITIAL_VISIBILITY", "debug", translate_marker( "Distance initial visibility" ),
         translate_marker( "Determines the scope, which is known in the beginning of the game." ),
         3, 20, 15
       );

    add_empty_line();

    add( "INITIAL_STAT_POINTS", "debug", translate_marker( "Initial stat points" ),
         translate_marker( "Initial points available to spend on stats on character generation." ),
         0, 1000, 6
       );

    add( "INITIAL_TRAIT_POINTS", "debug", translate_marker( "Initial trait points" ),
         translate_marker( "Initial points available to spend on traits on character generation." ),
         0, 1000, 0
       );

    add( "INITIAL_SKILL_POINTS", "debug", translate_marker( "Initial skill points" ),
         translate_marker( "Initial points available to spend on skills on character generation." ),
         0, 1000, 2
       );

    add( "MAX_TRAIT_POINTS", "debug", translate_marker( "Maximum trait points" ),
         translate_marker( "Maximum trait points available for character generation." ),
         0, 1000, 12
       );

    add_empty_line();

    add( "SKILL_TRAINING_SPEED", "debug", translate_marker( "Skill training speed" ),
         translate_marker( "Scales experience gained from practicing skills and reading books.  0.5 is half as fast as default, 2.0 is twice as fast, 0.0 disables skill training except for NPC training." ),
         0.0, 100.0, 1.0, 0.1
       );

    add_empty_line();

    add( "SKILL_RUST", "debug", translate_marker( "Skill rust" ),
         translate_marker( "Set the level of skill rust.  Vanilla: Vanilla Cataclysm - Capped: Capped at skill levels 2 - Int: Intelligence dependent - IntCap: Intelligence dependent, capped - Off: None at all." ),
         //~ plain, default, normal
    {   { "vanilla", translate_marker( "Vanilla" ) },
        //~ capped at a value
        { "capped", translate_marker( "Capped" ) },
        //~ based on intelligence
        { "int", translate_marker( "Int" ) },
        //~ based on intelligence and capped
        { "intcap", translate_marker( "IntCap" ) },
        { "off", translate_marker( "Off" ) }
    },
    "off" );

    add_empty_line();

    add( "FOV_3D", "debug", translate_marker( "Experimental 3D field of vision" ),
         translate_marker( "If false, vision is limited to current z-level.  If true and the world is in z-level mode, the vision will extend beyond current z-level.  Currently very bugged!" ),
         false
       );

    add( "FOV_3D_Z_RANGE", "debug", translate_marker( "Vertical range of 3D field of vision" ),
         translate_marker( "How many levels up and down the experimental 3D field of vision reaches.  (This many levels up, this many levels down.)  3D vision of the full height of the world can slow the game down a lot.  Seeing fewer Z-levels is faster." ),
         0, OVERMAP_LAYERS, 4
       );

    get_option( "FOV_3D_Z_RANGE" ).setPrerequisite( "FOV_3D" );

    add( "ENCODING_CONV", "debug", translate_marker( "Experimental path name encoding conversion" ),
         translate_marker( "If true, file path names are going to be transcoded from system encoding to UTF-8 when reading and will be transcoded back when writing.  Mainly for CJK Windows users." ),
         true
       );

    add( "ENABLE_EVENTS", "debug", translate_marker( "Event bus system" ),
         translate_marker( "If false, achievements and some Magiclysm functionality won't work, but performance will be better." ),
         true
       );

    add( "ELECTRIC_GRID", "debug", translate_marker( "Electric grid testing" ),
         translate_marker( "If true, enables completely unfinished electric grid system that only slows downs the game." ),
         false
       );
}

void options_manager::add_options_world_default()
{
    const auto add_empty_line = [&]() {
        world_default_page_.items_.emplace_back();
    };

    add( "CORE_VERSION", "world_default", translate_marker( "Core version data" ),
         translate_marker( "Controls what migrations are applied for legacy worlds" ),
         1, core_version, core_version, COPT_ALWAYS_HIDE
       );

    add_empty_line();

    add( "WORLD_END", "world_default", translate_marker( "World end handling" ),
    translate_marker( "Handling of game world when last character dies." ), {
        { "reset", translate_marker( "Reset" ) }, { "delete", translate_marker( "Delete" ) },
        { "query", translate_marker( "Query" ) }, { "keep", translate_marker( "Keep" ) }
    }, "reset"
       );

    add_empty_line();

    add( "CITY_SIZE", "world_default", translate_marker( "Size of cities" ),
         translate_marker( "A number determining how large cities are.  0 disables cities, roads and any scenario requiring a city start." ),
         0, 16, 8
       );

    add( "CITY_SPACING", "world_default", translate_marker( "City spacing" ),
         translate_marker( "A number determining how far apart cities are.  Warning, small numbers lead to very slow mapgen." ),
         0, 8, 4
       );

    add( "SPAWN_DENSITY", "world_default", translate_marker( "Spawn rate scaling factor" ),
         translate_marker( "A scaling factor that determines density of monster spawns." ),
         0.0, 50.0, 1.0, 0.1
       );

    add( "CARRION_SPAWNRATE", "world_default", translate_marker( "Carrion spawn rate scaling factor" ),
         translate_marker( "A scaling factor that determines how often creatures spawn from rotting material." ),
         0, 1000, 100, COPT_NO_HIDE, "%i%%"
       );

    add( "ITEM_SPAWNRATE", "world_default", translate_marker( "Item spawn scaling factor" ),
         translate_marker( "A scaling factor that determines density of item spawns." ),
         0.01, 10.0, 1.0, 0.01
       );

    add( "NPC_DENSITY", "world_default", translate_marker( "NPC spawn rate scaling factor" ),
         translate_marker( "A scaling factor that determines density of dynamic NPC spawns." ),
         0.0, 100.0, 0.1, 0.01
       );

    add( "MONSTER_UPGRADE_FACTOR", "world_default",
         translate_marker( "Monster evolution scaling factor" ),
         translate_marker( "A scaling factor that determines the time between monster upgrades.  A higher number means slower evolution.  Set to 0.00 to turn off monster upgrades." ),
         0.0, 100, 4.0, 0.01
       );

    add_empty_line();

    add( "MONSTER_SPEED", "world_default", translate_marker( "Monster speed" ),
         translate_marker( "Determines the movement rate of monsters.  A higher value increases monster speed and a lower reduces it.  Requires world reset." ),
         1, 1000, 100, COPT_NO_HIDE, "%i%%"
       );

    add( "MONSTER_RESILIENCE", "world_default", translate_marker( "Monster resilience" ),
         translate_marker( "Determines how much damage monsters can take.  A higher value makes monsters more resilient and a lower makes them more flimsy.  Requires world reset." ),
         1, 1000, 100, COPT_NO_HIDE, "%i%%"
       );

    add_empty_line();

    add( "DEFAULT_REGION", "world_default", translate_marker( "Default region type" ),
         translate_marker( "( WIP feature ) Determines terrain, shops, plants, and more." ),
    { { "default", "default" } }, "default"
       );

    add_empty_line();

    add( "INITIAL_TIME", "world_default", translate_marker( "Initial time" ),
         translate_marker( "Initial starting time of day on character generation." ),
         0, 23, 8
       );

    add( "INITIAL_DAY", "world_default", translate_marker( "Initial day" ),
         translate_marker( "How many days into the year the cataclysm occurred.  Day 0 is Spring 1.  Day -1 randomizes the start date.  Can be overridden by scenarios.  This does not advance food rot or monster evolution." ),
         -1, 999, 60
       );

    add( "SPAWN_DELAY", "world_default", translate_marker( "Spawn delay" ),
         translate_marker( "How many days after the cataclysm the player spawns.  Day 0 is the day of the cataclysm.  Can be overridden by scenarios.  Increasing this will cause food rot and monster evolution to advance." ),
         0, 9999, 0
       );

    add( "SEASON_LENGTH", "world_default", translate_marker( "Season length" ),
         translate_marker( "Season length, in days.  Warning: Very little other than the duration of seasons scales with this value, so adjusting it may cause nonsensical results." ),
         14, 127, 91
       );

    add( "CONSTRUCTION_SCALING", "world_default", translate_marker( "Construction scaling" ),
         translate_marker( "Sets the time of construction in percents.  '50' is two times faster than default, '200' is two times longer.  '0' automatically scales construction time to match the world's season length." ),
         0, 1000, 100
       );

    add( "ETERNAL_SEASON", "world_default", translate_marker( "Eternal season" ),
         translate_marker( "Keep the initial season for ever." ),
         false
       );

    add_empty_line();

    add( "WANDER_SPAWNS", "world_default", translate_marker( "Wander spawns" ),
         translate_marker( "Emulation of zombie hordes.  Zombie spawn points wander around cities and may go to noise.  Must reset world directory after changing for it to take effect." ),
         false
       );

    add( "BLACK_ROAD", "world_default", translate_marker( "Surrounded start" ),
         translate_marker( "If true, spawn zombies at shelters.  Makes the starting game a lot harder." ),
         false
       );

    add_empty_line();

    add( "STATIC_NPC", "world_default", translate_marker( "Static NPCs" ),
         translate_marker( "If true, static NPCs will spawn at pre-defined locations.  Requires world reset." ),
         true
       );

    add( "STARTING_NPC", "world_default", translate_marker( "Starting NPCs spawn" ),
         translate_marker( "Determines whether starting NPCs should spawn, and if they do, how exactly." ),
    { { "never", translate_marker( "Never" ) }, { "always", translate_marker( "Always" ) }, { "scenario", translate_marker( "Scenario-based" ) } },
    "scenario"
       );

    get_option( "STARTING_NPC" ).setPrerequisite( "STATIC_NPC" );

    add( "RANDOM_NPC", "world_default", translate_marker( "Random NPCs" ),
         translate_marker( "If true, the game will randomly spawn NPCs during gameplay." ),
         false
       );

    add_empty_line();

    add( "RAD_MUTATION", "world_default", translate_marker( "Mutations by radiation" ),
         translate_marker( "If true, radiation causes the player to mutate." ),
         true
       );

    add_empty_line();

    add( "ZLEVELS", "world_default", translate_marker( "Z-levels" ),
         translate_marker( "If true, enables several features related to vertical movement, such as hauling items up stairs, climbing downspouts, and flying aircraft.  May cause problems if toggled mid-game." ),
         true
       );

    add_empty_line();

    add( "CHARACTER_POINT_POOLS", "world_default", translate_marker( "Character point pools" ),
         translate_marker( "Allowed point pools for character generation." ),
    { { "any", translate_marker( "Any" ) }, { "multi_pool", translate_marker( "Multi-pool only" ) }, { "no_freeform", translate_marker( "No freeform" ) } },
    "any"
       );

    add( "DISABLE_LIFTING", "world_default",
         translate_marker( "Disables lifting requirements for vehicle parts." ),
         translate_marker( "If true, strength checks and/or lifting qualities no longer need to be met in order to change parts." ),
         false, COPT_ALWAYS_HIDE
       );
}

void options_manager::add_options_android()
{
#if defined(__ANDROID__)
    const auto add_empty_line = [&]() {
        android_page_.items_.emplace_back();
    };

    add( "ANDROID_QUICKSAVE", "android", translate_marker( "Quicksave on app lose focus" ),
         translate_marker( "If true, quicksave whenever the app loses focus (screen locked, app moved into background etc.) WARNING: Experimental. This may result in corrupt save games." ),
         false
       );

    add_empty_line();

    add( "ANDROID_TRAP_BACK_BUTTON", "android", translate_marker( "Trap Back button" ),
         translate_marker( "If true, the back button will NOT back out of the app and will be passed to the application as SDL_SCANCODE_AC_BACK.  Requires restart." ),
         // take default setting from pre-game settings screen - important as there are issues with Back button on Android 9 with specific devices
         android_get_default_setting( "Trap Back button", true )
       );

    add( "ANDROID_AUTO_KEYBOARD", "android", translate_marker( "Auto-manage virtual keyboard" ),
         translate_marker( "If true, automatically show/hide the virtual keyboard when necessary based on context. If false, virtual keyboard must be toggled manually." ),
         true
       );

    add( "ANDROID_KEYBOARD_SCREEN_SCALE", "android",
         translate_marker( "Virtual keyboard screen scale" ),
         translate_marker( "When the virtual keyboard is visible, scale the screen to prevent overlapping. Useful for text entry so you can see what you're typing." ),
         true
       );

    add_empty_line();

    add( "ANDROID_VIBRATION", "android", translate_marker( "Vibration duration" ),
         translate_marker( "If non-zero, vibrate the device for this long on input, in milliseconds. Ignored if hardware keyboard connected." ),
         0, 200, 10
       );

    add_empty_line();

    add( "ANDROID_SHOW_VIRTUAL_JOYSTICK", "android", translate_marker( "Show virtual joystick" ),
         translate_marker( "If true, show the virtual joystick when touching and holding the screen. Gives a visual indicator of deadzone and stick deflection." ),
         true
       );

    add( "ANDROID_VIRTUAL_JOYSTICK_OPACITY", "android", translate_marker( "Virtual joystick opacity" ),
         translate_marker( "The opacity of the on-screen virtual joystick, as a percentage." ),
         0, 100, 20
       );

    add( "ANDROID_DEADZONE_RANGE", "android", translate_marker( "Virtual joystick deadzone size" ),
         translate_marker( "While using the virtual joystick, deflecting the stick beyond this distance will trigger directional input. Specified as a percentage of longest screen edge." ),
         0.01f, 0.2f, 0.03f, 0.001f, COPT_NO_HIDE, "%.3f"
       );

    add( "ANDROID_REPEAT_DELAY_RANGE", "android", translate_marker( "Virtual joystick size" ),
         translate_marker( "While using the virtual joystick, deflecting the stick by this much will repeat input at the deflected rate (see below). Specified as a percentage of longest screen edge." ),
         0.05f, 0.5f, 0.10f, 0.001f, COPT_NO_HIDE, "%.3f"
       );

    add( "ANDROID_VIRTUAL_JOYSTICK_FOLLOW", "android",
         translate_marker( "Virtual joystick follows finger" ),
         translate_marker( "If true, the virtual joystick will follow when sliding beyond its range." ),
         false
       );

    add( "ANDROID_REPEAT_DELAY_MAX", "android",
         translate_marker( "Virtual joystick repeat rate (centered)" ),
         translate_marker( "When the virtual joystick is centered, how fast should input events repeat, in milliseconds." ),
         50, 1000, 500
       );

    add( "ANDROID_REPEAT_DELAY_MIN", "android",
         translate_marker( "Virtual joystick repeat rate (deflected)" ),
         translate_marker( "When the virtual joystick is fully deflected, how fast should input events repeat, in milliseconds." ),
         50, 1000, 100
       );

    add( "ANDROID_SENSITIVITY_POWER", "android",
         translate_marker( "Virtual joystick repeat rate sensitivity" ),
         translate_marker( "As the virtual joystick moves from centered to fully deflected, this value is an exponent that controls the blend between the two repeat rates defined above. 1.0 = linear." ),
         0.1f, 5.0f, 0.75f, 0.05f, COPT_NO_HIDE, "%.2f"
       );

    add( "ANDROID_INITIAL_DELAY", "android", translate_marker( "Input repeat delay" ),
         translate_marker( "While touching the screen, wait this long before showing the virtual joystick and repeating input, in milliseconds. Also used to determine tap/double-tap detection, flick detection and toggling quick shortcuts." ),
         150, 1000, 300
       );

    add( "ANDROID_HIDE_HOLDS", "android", translate_marker( "Virtual joystick hides shortcuts" ),
         translate_marker( "If true, hides on-screen keyboard shortcuts while using the virtual joystick. Helps keep the view uncluttered while traveling long distances and navigating menus." ),
         true
       );

    add_empty_line();

    add( "ANDROID_SHORTCUT_DEFAULTS", "android", translate_marker( "Default gameplay shortcuts" ),
         translate_marker( "The default set of gameplay shortcuts to show. Used on starting a new game and whenever all gameplay shortcuts are removed." ),
         "0mi", 30
       );

    add( "ANDROID_ACTIONMENU_AUTOADD", "android",
         translate_marker( "Add shortcuts for action menu selections" ),
         translate_marker( "If true, automatically add a shortcut for actions selected via the in-game action menu." ),
         true
       );

    add( "ANDROID_INVENTORY_AUTOADD", "android",
         translate_marker( "Add shortcuts for inventory selections" ),
         translate_marker( "If true, automatically add a shortcut for items selected via the inventory." ),
         true
       );

    add_empty_line();

    add( "ANDROID_TAP_KEY", "android", translate_marker( "Tap key (in-game)" ),
         translate_marker( "The key to press when tapping during gameplay." ),
         ".", 1
       );

    add( "ANDROID_2_TAP_KEY", "android", translate_marker( "Two-finger tap key (in-game)" ),
         translate_marker( "The key to press when tapping with two fingers during gameplay." ),
         "i", 1
       );

    add( "ANDROID_2_SWIPE_UP_KEY", "android", translate_marker( "Two-finger swipe up key (in-game)" ),
         translate_marker( "The key to press when swiping up with two fingers during gameplay." ),
         "K", 1
       );

    add( "ANDROID_2_SWIPE_DOWN_KEY", "android",
         translate_marker( "Two-finger swipe down key (in-game)" ),
         translate_marker( "The key to press when swiping down with two fingers during gameplay." ),
         "J", 1
       );

    add( "ANDROID_2_SWIPE_LEFT_KEY", "android",
         translate_marker( "Two-finger swipe left key (in-game)" ),
         translate_marker( "The key to press when swiping left with two fingers during gameplay." ),
         "L", 1
       );

    add( "ANDROID_2_SWIPE_RIGHT_KEY", "android",
         translate_marker( "Two-finger swipe right key (in-game)" ),
         translate_marker( "The key to press when swiping right with two fingers during gameplay." ),
         "H", 1
       );

    add( "ANDROID_PINCH_IN_KEY", "android", translate_marker( "Pinch in key (in-game)" ),
         translate_marker( "The key to press when pinching in during gameplay." ),
         "Z", 1
       );

    add( "ANDROID_PINCH_OUT_KEY", "android", translate_marker( "Pinch out key (in-game)" ),
         translate_marker( "The key to press when pinching out during gameplay." ),
         "z", 1
       );

    add_empty_line();

    add( "ANDROID_SHORTCUT_AUTOADD", "android",
         translate_marker( "Auto-manage contextual gameplay shortcuts" ),
         translate_marker( "If true, contextual in-game shortcuts are added and removed automatically as needed: examine, close, butcher, move up/down, control vehicle, pickup, toggle enemy + safe mode, sleep." ),
         true
       );

    add( "ANDROID_SHORTCUT_AUTOADD_FRONT", "android",
         translate_marker( "Move contextual gameplay shortcuts to front" ),
         translate_marker( "If the above option is enabled, specifies whether contextual in-game shortcuts will be added to the front or back of the shortcuts list. True makes them easier to reach, False reduces shuffling of shortcut positions." ),
         false
       );

    add( "ANDROID_SHORTCUT_MOVE_FRONT", "android", translate_marker( "Move used shortcuts to front" ),
         translate_marker( "If true, using an existing shortcut will always move it to the front of the shortcuts list. If false, only shortcuts typed via keyboard will move to the front." ),
         false
       );

    add( "ANDROID_SHORTCUT_ZONE", "android",
         translate_marker( "Separate shortcuts for No Auto Pickup zones" ),
         translate_marker( "If true, separate gameplay shortcuts will be used within No Auto Pickup zones. Useful for keeping home base actions separate from exploring actions." ),
         true
       );

    add( "ANDROID_SHORTCUT_REMOVE_TURNS", "android",
         translate_marker( "Turns to remove unused gameplay shortcuts" ),
         translate_marker( "If non-zero, unused gameplay shortcuts will be removed after this many turns (as in discrete player actions, not world calendar turns)." ),
         0, 1000, 0
       );

    add( "ANDROID_SHORTCUT_PERSISTENCE", "android", translate_marker( "Shortcuts persistence" ),
         translate_marker( "If true, shortcuts are saved/restored with each save game. If false, shortcuts reset between sessions." ),
         true
       );

    add_empty_line();

    add( "ANDROID_SHORTCUT_POSITION", "android", translate_marker( "Shortcuts position" ),
         translate_marker( "Switch between shortcuts on the left or on the right side of the screen." ),
    { { "left", translate_marker( "Left" ) }, { "right", translate_marker( "Right" ) } }, "left"
       );

    add( "ANDROID_SHORTCUT_SCREEN_PERCENTAGE", "android",
         translate_marker( "Shortcuts screen percentage" ),
         translate_marker( "How much of the screen can shortcuts occupy, as a percentage of total screen width." ),
         10, 100, 100
       );

    add( "ANDROID_SHORTCUT_OVERLAP", "android", translate_marker( "Shortcuts overlap screen" ),
         translate_marker( "If true, shortcuts will be drawn transparently overlapping the game screen. If false, the game screen size will be reduced to fit the shortcuts below." ),
         true
       );

    add( "ANDROID_SHORTCUT_OPACITY_BG", "android", translate_marker( "Shortcut opacity (background)" ),
         translate_marker( "The background opacity of on-screen keyboard shortcuts, as a percentage." ),
         0, 100, 75
       );

    add( "ANDROID_SHORTCUT_OPACITY_SHADOW", "android", translate_marker( "Shortcut opacity (shadow)" ),
         translate_marker( "The shadow opacity of on-screen keyboard shortcuts, as a percentage." ),
         0, 100, 100
       );

    add( "ANDROID_SHORTCUT_OPACITY_FG", "android", translate_marker( "Shortcut opacity (text)" ),
         translate_marker( "The foreground opacity of on-screen keyboard shortcuts, as a percentage." ),
         0, 100, 100
       );

    add( "ANDROID_SHORTCUT_COLOR", "android", translate_marker( "Shortcut color" ),
         translate_marker( "The color of on-screen keyboard shortcuts." ),
         0, 15, 15
       );

    add( "ANDROID_SHORTCUT_BORDER", "android", translate_marker( "Shortcut border" ),
         translate_marker( "The border of each on-screen keyboard shortcut in pixels. ." ),
         0, 16, 0
       );

    add( "ANDROID_SHORTCUT_WIDTH_MIN", "android", translate_marker( "Shortcut width (min)" ),
         translate_marker( "The minimum width of each on-screen keyboard shortcut in pixels. Only relevant when lots of shortcuts are visible at once." ),
         20, 1000, 50
       );

    add( "ANDROID_SHORTCUT_WIDTH_MAX", "android", translate_marker( "Shortcut width (max)" ),
         translate_marker( "The maximum width of each on-screen keyboard shortcut in pixels." ),
         50, 1000, 160
       );

    add( "ANDROID_SHORTCUT_HEIGHT", "android", translate_marker( "Shortcut height" ),
         translate_marker( "The height of each on-screen keyboard shortcut in pixels." ),
         50, 1000, 130
       );

#endif
}

#if defined(TILES)
// Helper method to isolate #ifdeffed tiles code.
static void refresh_tiles( bool used_tiles_changed, bool pixel_minimap_height_changed, bool ingame )
{
    if( used_tiles_changed ) {
        //try and keep SDL calls limited to source files that deal specifically with them
        try {
            tilecontext->reinit();
            tilecontext->load_tileset( get_option<std::string>( "TILES" ) );
            //g->init_ui is called when zoom is changed
            g->reset_zoom();
            if( ingame ) {
                g->refresh_all();
            }
            tilecontext->do_tile_loading_report();
        } catch( const std::exception &err ) {
            popup( _( "Loading the tileset failed: %s" ), err.what() );
            use_tiles = false;
        }
    } else if( ingame && g->pixel_minimap_option && pixel_minimap_height_changed ) {
        g->init_ui();
        g->refresh_all();
    }
}
#else
static void refresh_tiles( bool, bool, bool )
{
}
#endif // TILES

static void draw_borders_external(
    const catacurses::window &w, int horizontal_level, const std::map<int, bool> &mapLines,
    const bool world_options_only )
{
    if( !world_options_only ) {
        draw_border( w, BORDER_COLOR, _( " OPTIONS " ) );
    }
    // intersections
    mvwputch( w, point( 0, horizontal_level ), BORDER_COLOR, LINE_XXXO ); // |-
    mvwputch( w, point( getmaxx( w ) - 1, horizontal_level ), BORDER_COLOR, LINE_XOXX ); // -|
    for( auto &mapLine : mapLines ) {
        if( mapLine.second ) {
            mvwputch( w, point( mapLine.first + 1, getmaxy( w ) - 1 ), BORDER_COLOR, LINE_XXOX ); // _|_
        }
    }
    wrefresh( w );
}

static void draw_borders_internal( const catacurses::window &w, std::map<int, bool> &mapLines )
{
    for( int i = 0; i < getmaxx( w ); ++i ) {
        if( mapLines[i] ) {
            // intersection
            mvwputch( w, point( i, 0 ), BORDER_COLOR, LINE_OXXX );
        } else {
            // regular line
            mvwputch( w, point( i, 0 ), BORDER_COLOR, LINE_OXOX );
        }
    }
    wrefresh( w );
}

std::string options_manager::show( bool ingame, const bool world_options_only,
                                   const std::function<bool()> &on_quit )
{
    const int iWorldOptPage = std::find_if( pages_.begin(), pages_.end(), [&]( const Page & p ) {
        return &p == &world_default_page_;
    } ) - pages_.begin();

    // temporary alias so the code below does not need to be changed
    options_container &OPTIONS = options;
    options_container &ACTIVE_WORLD_OPTIONS = world_options.has_value() ?
            *world_options.value() :
            OPTIONS;

    auto OPTIONS_OLD = OPTIONS;
    auto WOPTIONS_OLD = ACTIVE_WORLD_OPTIONS;
    if( world_generator->active_world == nullptr ) {
        ingame = false;
    }

    std::map<int, bool> mapLines;
    mapLines[4] = true;
    mapLines[60] = true;

    int iCurrentPage = world_options_only ? iWorldOptPage : 0;
    int iCurrentLine = 0;
    int iStartPos = 0;

    input_context ctxt( "OPTIONS" );
    ctxt.register_cardinal();
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    const int iWorldOffset = world_options_only ? 2 : 0;
    int iMinScreenWidth = 0;
    const int iTooltipHeight = 4;
    int iContentHeight = 0;

    catacurses::window w_options_border;
    catacurses::window w_options_tooltip;
    catacurses::window w_options_header;
    catacurses::window w_options;

    const auto init_windows = [&]( ui_adaptor & ui ) {
        if( OPTIONS.find( "TERMINAL_X" ) != OPTIONS.end() ) {
            if( OPTIONS_OLD.find( "TERMINAL_X" ) != OPTIONS_OLD.end() ) {
                OPTIONS_OLD["TERMINAL_X"] = OPTIONS["TERMINAL_X"];
            }
            if( WOPTIONS_OLD.find( "TERMINAL_X" ) != WOPTIONS_OLD.end() ) {
                WOPTIONS_OLD["TERMINAL_X"] = OPTIONS["TERMINAL_X"];
            }
        }
        if( OPTIONS.find( "TERMINAL_Y" ) != OPTIONS.end() ) {
            if( OPTIONS_OLD.find( "TERMINAL_Y" ) != OPTIONS_OLD.end() ) {
                OPTIONS_OLD["TERMINAL_Y"] = OPTIONS["TERMINAL_Y"];
            }
            if( WOPTIONS_OLD.find( "TERMINAL_Y" ) != WOPTIONS_OLD.end() ) {
                WOPTIONS_OLD["TERMINAL_Y"] = OPTIONS["TERMINAL_Y"];
            }
        }

        iMinScreenWidth = std::max( FULL_SCREEN_WIDTH, TERMX / 2 );
        const int iOffsetX = TERMX > FULL_SCREEN_WIDTH ? ( TERMX - iMinScreenWidth ) / 2 : 0;
        iContentHeight = TERMY - 3 - iTooltipHeight - iWorldOffset;

        w_options_border  = catacurses::newwin( TERMY, iMinScreenWidth,
                                                point( iOffsetX, 0 ) );
        w_options_tooltip = catacurses::newwin( iTooltipHeight, iMinScreenWidth - 2,
                                                point( 1 + iOffsetX, 1 + iWorldOffset ) );
        w_options_header  = catacurses::newwin( 1, iMinScreenWidth - 2,
                                                point( 1 + iOffsetX, 1 + iTooltipHeight + iWorldOffset ) );
        w_options         = catacurses::newwin( iContentHeight, iMinScreenWidth - 2,
                                                point( 1 + iOffsetX, iTooltipHeight + 2 + iWorldOffset ) );

        ui.position_from_window( w_options_border );
    };

    ui_adaptor ui;
    ui.on_screen_resize( init_windows );
    init_windows( ui );
    ui.on_redraw( [&]( const ui_adaptor & ) {
        if( world_options_only ) {
            worldfactory::draw_worldgen_tabs( w_options_border, 1 );
        }

        draw_borders_external( w_options_border, iTooltipHeight + 1 + iWorldOffset, mapLines,
                               world_options_only );
        draw_borders_internal( w_options_header, mapLines );

        Page &page = pages_[iCurrentPage];
        auto &page_items = page.items_;

        auto &cOPTIONS = ( ingame || world_options_only ) && iCurrentPage == iWorldOptPage ?
                         ACTIVE_WORLD_OPTIONS : OPTIONS;

        //Clear the lines
        for( int i = 0; i < iContentHeight; i++ ) {
            for( int j = 0; j < iMinScreenWidth - 2; j++ ) {
                if( mapLines[j] ) {
                    mvwputch( w_options, point( j, i ), BORDER_COLOR, LINE_XOXO );
                } else {
                    mvwputch( w_options, point( j, i ), c_black, ' ' );
                }

                if( i < iTooltipHeight ) {
                    mvwputch( w_options_tooltip, point( j, i ), c_black, ' ' );
                }
            }
        }

        calcStartPos( iStartPos, iCurrentLine, iContentHeight, page_items.size() );

        // where the column with the names starts
        const size_t name_col = 5;
        // where the column with the values starts
        const size_t value_col = 62;
        // 2 for the space between name and value column, 3 for the ">> "
        const size_t name_width = value_col - name_col - 2 - 3;
        const size_t value_width = getmaxx( w_options ) - value_col;
        //Draw options
        size_t iBlankOffset = 0; // Offset when blank line is printed.
        for( int i = iStartPos;
             i < iStartPos + ( iContentHeight > static_cast<int>( page_items.size() ) ?
                               static_cast<int>( page_items.size() ) : iContentHeight ); i++ ) {

            int line_pos = i - iStartPos; // Current line position in window.

            mvwprintz( w_options, point( 1, line_pos ), c_white, "%d", i + 1 - iBlankOffset );

            if( iCurrentLine == i ) {
                mvwprintz( w_options, point( name_col, line_pos ), c_yellow, ">> " );
            } else {
                mvwprintz( w_options, point( name_col, line_pos ), c_yellow, "   " );
            }

            const auto &opt_name = page_items[i];
            if( !opt_name ) {
                continue;
            }

            nc_color cLineColor = c_light_green;
            const cOpt &current_opt = cOPTIONS[*opt_name];
            const bool hasPrerequisite = current_opt.hasPrerequisite();
            const bool hasPrerequisiteFulfilled = current_opt.checkPrerequisite();

            const std::string name = utf8_truncate( current_opt.getMenuText(), name_width );
            mvwprintz( w_options, point( name_col + 3, line_pos ), !hasPrerequisite ||
                       hasPrerequisiteFulfilled ? c_white : c_light_gray, name );

            if( hasPrerequisite && !hasPrerequisiteFulfilled ) {
                cLineColor = c_light_gray;

            } else if( current_opt.getValue() == "false" || current_opt.getValue() == "disabled" ||
                       current_opt.getValue() == "off" ) {
                cLineColor = c_light_red;
            }

            const std::string value = utf8_truncate( current_opt.getValueName(), value_width );
            mvwprintz( w_options, point( value_col, line_pos ),
                       iCurrentLine == i ? hilite( cLineColor ) : cLineColor,
                       value );
        }

        draw_scrollbar( w_options_border, iCurrentLine, iContentHeight,
                        page_items.size(), point( 0, iTooltipHeight + 2 + iWorldOffset ), BORDER_COLOR );
        wrefresh( w_options_border );

        //Draw Tabs
        if( !world_options_only ) {
            mvwprintz( w_options_header, point( 7, 0 ), c_white, "" );
            for( int i = 0; i < static_cast<int>( pages_.size() ); i++ ) {
                wprintz( w_options_header, c_white, "[" );
                if( ingame && i == iWorldOptPage ) {
                    wprintz( w_options_header, iCurrentPage == i ? hilite( c_light_green ) : c_light_green,
                             _( "Current world" ) );
                } else {
                    wprintz( w_options_header, iCurrentPage == i ? hilite( c_light_green ) : c_light_green,
                             "%s", pages_[i].get().name_ );
                }
                wprintz( w_options_header, c_white, "]" );
                wputch( w_options_header, BORDER_COLOR, LINE_OXOX );
            }
        }

        wrefresh( w_options_header );

        const std::string &opt_name = *page_items[iCurrentLine];
        cOpt &current_opt = cOPTIONS[opt_name];

#if defined(TILES) || defined(_WIN32)
        if( opt_name == "TERMINAL_X" ) {
            int new_terminal_x = 0;
            int new_window_width = 0;
            std::stringstream value_conversion( current_opt.getValueName() );

            value_conversion >> new_terminal_x;
            new_window_width = projected_window_width();

            fold_and_print( w_options_tooltip, point_zero, iMinScreenWidth - 2, c_white,
                            ngettext( "%s #%s -- The window will be %d pixel wide with the selected value.",
                                      "%s #%s -- The window will be %d pixels wide with the selected value.",
                                      new_window_width ),
                            current_opt.getTooltip(),
                            current_opt.getDefaultText(),
                            new_window_width );
        } else if( opt_name == "TERMINAL_Y" ) {
            int new_terminal_y = 0;
            int new_window_height = 0;
            std::stringstream value_conversion( current_opt.getValueName() );

            value_conversion >> new_terminal_y;
            new_window_height = projected_window_height();

            fold_and_print( w_options_tooltip, point_zero, iMinScreenWidth - 2, c_white,
                            ngettext( "%s #%s -- The window will be %d pixel tall with the selected value.",
                                      "%s #%s -- The window will be %d pixels tall with the selected value.",
                                      new_window_height ),
                            current_opt.getTooltip(),
                            current_opt.getDefaultText(),
                            new_window_height );
        } else
#endif
        {
            fold_and_print( w_options_tooltip, point_zero, iMinScreenWidth - 2, c_white, "%s #%s",
                            current_opt.getTooltip(),
                            current_opt.getDefaultText() );
        }

        if( ingame && iCurrentPage == iWorldOptPage ) {
            mvwprintz( w_options_tooltip, point( 3, 3 ), c_light_red, "%s", _( "Note: " ) );
            wprintz( w_options_tooltip, c_white, "%s",
                     _( "Some of these options may produce unexpected results if changed." ) );
        }
        wrefresh( w_options_tooltip );

        wrefresh( w_options );
    } );

    while( true ) {
        ui_manager::redraw();

        Page &page = pages_[iCurrentPage];
        auto &page_items = page.items_;

        auto &cOPTIONS = ( ingame || world_options_only ) && iCurrentPage == iWorldOptPage ?
                         ACTIVE_WORLD_OPTIONS : OPTIONS;

        const std::string &opt_name = *page_items[iCurrentLine];
        cOpt &current_opt = cOPTIONS[opt_name];

        const std::string action = ctxt.handle_input();

        if( world_options_only && ( action == "NEXT_TAB" || action == "PREV_TAB" ||
                                    ( action == "QUIT" && ( !on_quit || on_quit() ) ) ) ) {
            return action;
        }

        bool hasPrerequisite = current_opt.hasPrerequisite();
        bool hasPrerequisiteFulfilled = current_opt.checkPrerequisite();

        if( hasPrerequisite && !hasPrerequisiteFulfilled &&
            ( action == "RIGHT" || action == "LEFT" || action == "CONFIRM" ) ) {
            popup( _( "Prerequisite for this option not met!\n(%s)" ),
                   get_options().get_option( current_opt.getPrerequisite() ).getMenuText() );
            continue;
        }

        if( action == "DOWN" ) {
            do {
                iCurrentLine++;
                if( iCurrentLine >= static_cast<int>( page_items.size() ) ) {
                    iCurrentLine = 0;
                }
            } while( !page_items[iCurrentLine] );
        } else if( action == "UP" ) {
            do {
                iCurrentLine--;
                if( iCurrentLine < 0 ) {
                    iCurrentLine = page_items.size() - 1;
                }
            } while( !page_items[iCurrentLine] );
        } else if( action == "RIGHT" ) {
            current_opt.setNext();
        } else if( action == "LEFT" ) {
            current_opt.setPrev();
        } else if( action == "NEXT_TAB" ) {
            iCurrentLine = 0;
            iStartPos = 0;
            iCurrentPage++;
            if( iCurrentPage >= static_cast<int>( pages_.size() ) ) {
                iCurrentPage = 0;
            }
            sfx::play_variant_sound( "menu_move", "default", 100 );
        } else if( action == "PREV_TAB" ) {
            iCurrentLine = 0;
            iStartPos = 0;
            iCurrentPage--;
            if( iCurrentPage < 0 ) {
                iCurrentPage = pages_.size() - 1;
            }
            sfx::play_variant_sound( "menu_move", "default", 100 );
        } else if( action == "CONFIRM" ) {
            if( current_opt.getType() == "bool" || current_opt.getType() == "string_select" ||
                current_opt.getType() == "string_input" || current_opt.getType() == "int_map" ) {
                current_opt.setNext();
            } else {
                const bool is_int = current_opt.getType() == "int";
                const bool is_float = current_opt.getType() == "float";
                const std::string old_opt_val = current_opt.getValueName();
                const std::string opt_val = string_input_popup()
                                            .title( current_opt.getMenuText() )
                                            .width( 10 )
                                            .text( old_opt_val )
                                            .only_digits( is_int )
                                            .query_string();
                if( !opt_val.empty() && opt_val != old_opt_val ) {
                    if( is_float ) {
                        std::istringstream ssTemp( opt_val );
                        // This uses the current locale, to allow the users
                        // to use their own decimal format.
                        float tmpFloat;
                        ssTemp >> tmpFloat;
                        if( ssTemp ) {
                            current_opt.setValue( tmpFloat );

                        } else {
                            popup( _( "Invalid input: not a number" ) );
                        }
                    } else {
                        // option is of type "int": string_input_popup
                        // has taken care that the string contains
                        // only digits, parsing is done in setValue
                        current_opt.setValue( opt_val );
                    }
                }
            }
        } else if( action == "QUIT" ) {
            break;
        }
    }

    //Look for changes
    bool options_changed = false;
    bool world_options_changed = false;
    bool lang_changed = false;
    bool used_tiles_changed = false;
    bool pixel_minimap_changed = false;
    bool terminal_size_changed = false;

    for( auto &iter : OPTIONS_OLD ) {
        if( iter.second != OPTIONS[iter.first] ) {
            options_changed = true;

            if( iter.second.getPage() == "world_default" ) {
                world_options_changed = true;
            }

            if( iter.first == "PIXEL_MINIMAP_HEIGHT"
                || iter.first == "PIXEL_MINIMAP_RATIO"
                || iter.first == "PIXEL_MINIMAP_MODE"
                || iter.first == "PIXEL_MINIMAP_SCALE_TO_FIT" ) {
                pixel_minimap_changed = true;

            } else if( iter.first == "TILES" || iter.first == "USE_TILES" ) {
                used_tiles_changed = true;

            } else if( iter.first == "USE_LANG" ) {
                lang_changed = true;

            } else if( iter.first == "TERMINAL_X" || iter.first == "TERMINAL_Y" ) {
                terminal_size_changed = true;
            }
        }
    }
    for( auto &iter : WOPTIONS_OLD ) {
        if( iter.second != ACTIVE_WORLD_OPTIONS[iter.first] ) {
            options_changed = true;
            world_options_changed = true;
        }
    }

    if( options_changed ) {
        if( query_yn( _( "Save changes?" ) ) ) {
            static_popup popup;
            popup.message( "%s", _( "Please wait…\nApplying option changes…" ) );
            ui_manager::redraw();
            refresh_display();

            save();
            if( ingame && world_options_changed ) {
                world_generator->active_world->WORLD_OPTIONS = ACTIVE_WORLD_OPTIONS;
                world_generator->active_world->save();
            }
            g->on_options_changed();
        } else {
            lang_changed = false;
            terminal_size_changed = false;
            used_tiles_changed = false;
            pixel_minimap_changed = false;
            OPTIONS = OPTIONS_OLD;
            if( ingame && world_options_changed ) {
                ACTIVE_WORLD_OPTIONS = WOPTIONS_OLD;
            }
        }
    }

    if( lang_changed ) {
        update_global_locale();
        set_language();
    }
    calendar::set_eternal_season( ::get_option<bool>( "ETERNAL_SEASON" ) );
    calendar::set_season_length( ::get_option<int>( "SEASON_LENGTH" ) );

#if !defined(__ANDROID__) && (defined(TILES) || defined(_WIN32))
    if( terminal_size_changed ) {
        int scaling_factor = get_scaling_factor();
        int TERMX = ::get_option<int>( "TERMINAL_X" );
        int TERMY = ::get_option<int>( "TERMINAL_Y" );
        TERMX -= TERMX % scaling_factor;
        TERMY -= TERMY % scaling_factor;
        get_option( "TERMINAL_X" ).setValue( std::max( FULL_SCREEN_WIDTH * scaling_factor, TERMX ) );
        get_option( "TERMINAL_Y" ).setValue( std::max( FULL_SCREEN_HEIGHT * scaling_factor, TERMY ) );
        save();

        handle_resize( projected_window_width(), projected_window_height() );
    }
#else
    ( void ) terminal_size_changed;
#endif

    refresh_tiles( used_tiles_changed, pixel_minimap_changed, ingame );

    return "";
}

void options_manager::serialize( JsonOut &json ) const
{
    json.start_array();

    for( const Page &p : pages_ ) {
        for( const cata::optional<std::string> &opt_name : p.items_ ) {
            if( !opt_name ) {
                continue;
            }
            const auto iter = options.find( *opt_name );
            if( iter != options.end() ) {
                const auto &opt = iter->second;

                json.start_object();

                json.member( "info", opt.getTooltip() );
                json.member( "default", opt.getDefaultText( false ) );
                json.member( "name", opt.getName() );
                json.member( "value", opt.getValue( true ) );

                json.end_object();
            }
        }
    }

    json.end_array();
}

void options_manager::deserialize( JsonIn &jsin )
{
    jsin.start_array();
    while( !jsin.end_array() ) {
        JsonObject joOptions = jsin.get_object();
        joOptions.allow_omitted_members();

        const std::string name = migrateOptionName( joOptions.get_string( "name" ) );
        const std::string value = migrateOptionValue( joOptions.get_string( "name" ),
                                  joOptions.get_string( "value" ) );

        add_retry( name, value );
        options[ name ].setValue( value );
    }
}

std::string options_manager::migrateOptionName( const std::string &name ) const
{
    const auto iter = mMigrateOption.find( name );
    return iter != mMigrateOption.end() ? iter->second.first : name;
}

std::string options_manager::migrateOptionValue( const std::string &name,
        const std::string &val ) const
{
    const auto iter = mMigrateOption.find( name );
    if( iter == mMigrateOption.end() ) {
        return val;
    }

    const auto iter_val = iter->second.second.find( val );
    return iter_val != iter->second.second.end() ? iter_val->second : val;
}

bool options_manager::save()
{
    const auto savefile = PATH_INFO::options();

    // cache to global due to heavy usage.
    trigdist = ::get_option<bool>( "CIRCLEDIST" );
    use_tiles = ::get_option<bool>( "USE_TILES" );
    log_from_top = ::get_option<std::string>( "LOG_FLOW" ) == "new_top";
    message_ttl = ::get_option<int>( "MESSAGE_TTL" );
    message_cooldown = ::get_option<int>( "MESSAGE_COOLDOWN" );
    fov_3d = ::get_option<bool>( "FOV_3D" );
    fov_3d_z_range = ::get_option<int>( "FOV_3D_Z_RANGE" );

    update_music_volume();

    return write_to_file( savefile, [&]( std::ostream & fout ) {
        JsonOut jout( fout, true );
        serialize( jout );
    }, _( "options" ) );
}

void options_manager::load()
{
    const auto file = PATH_INFO::options();
    if( !read_from_file_optional_json( file, [&]( JsonIn & jsin ) {
    deserialize( jsin );
    } ) ) {
        if( load_legacy() ) {
            if( save() ) {
                remove_file( PATH_INFO::legacy_options() );
                remove_file( PATH_INFO::legacy_options2() );
            }
        }
    }

    update_global_locale();

    // cache to global due to heavy usage.
    trigdist = ::get_option<bool>( "CIRCLEDIST" );
    use_tiles = ::get_option<bool>( "USE_TILES" );
    log_from_top = ::get_option<std::string>( "LOG_FLOW" ) == "new_top";
    message_ttl = ::get_option<int>( "MESSAGE_TTL" );
    message_cooldown = ::get_option<int>( "MESSAGE_COOLDOWN" );
    fov_3d = ::get_option<bool>( "FOV_3D" );
    fov_3d_z_range = ::get_option<int>( "FOV_3D_Z_RANGE" );
#if defined(SDL_SOUND)
    sounds::sound_enabled = ::get_option<bool>( "SOUND_ENABLED" );
#endif
}

bool options_manager::load_legacy()
{
    const auto reader = [&]( std::istream & fin ) {
        std::string sLine;
        while( !fin.eof() ) {
            getline( fin, sLine );

            if( !sLine.empty() && sLine[0] != '#' && std::count( sLine.begin(), sLine.end(), ' ' ) == 1 ) {
                int iPos = sLine.find( ' ' );
                const std::string loadedvar = migrateOptionName( sLine.substr( 0, iPos ) );
                const std::string loadedval = migrateOptionValue( sLine.substr( 0, iPos ), sLine.substr( iPos + 1,
                                              sLine.length() ) );
                // option with values from post init() might get clobbered

                add_retry( loadedvar, loadedval ); // stash it until update();

                options[ loadedvar ].setValue( loadedval );
            }
        }
    };

    return read_from_file_optional( PATH_INFO::legacy_options(), reader ) ||
           read_from_file_optional( PATH_INFO::legacy_options2(), reader );
}

bool options_manager::has_option( const std::string &name ) const
{
    return options.count( name );
}

options_manager::cOpt &options_manager::get_option( const std::string &name )
{
    if( options.count( name ) == 0 ) {
        debugmsg( "requested non-existing option %s", name );
    }
    if( !world_options.has_value() ) {
        // Global options contains the default for new worlds, which is good enough here.
        return options[name];
    }
    auto &wopts = *world_options.value();
    if( wopts.count( name ) == 0 ) {
        auto &opt = options[name];
        if( opt.getPage() != "world_default" ) {
            // Requested a non-world option, deliver it.
            return opt;
        }
        // May be a new option and an old world - import default from global options.
        wopts[name] = opt;
    }
    return wopts[name];
}

options_manager::options_container options_manager::get_world_defaults() const
{
    std::unordered_map<std::string, cOpt> result;
    for( auto &elem : options ) {
        if( elem.second.getPage() == "world_default" ) {
            result.insert( elem );
        }
    }
    return result;
}

void options_manager::set_world_options( options_container *options )
{
    if( options == nullptr ) {
        world_options.reset();
    } else {
        world_options = options;
    }
}

void options_manager::update_global_locale()
{
    std::string lang = ::get_option<std::string>( "USE_LANG" );
    try {
        if( lang == "en" ) {
            std::locale::global( std::locale( "en_US.UTF-8" ) );
        } else if( lang == "de" ) {
            std::locale::global( std::locale( "de_DE.UTF-8" ) );
        } else if( lang == "es_AR" ) {
            std::locale::global( std::locale( "es_AR.UTF-8" ) );
        } else if( lang == "es_ES" ) {
            std::locale::global( std::locale( "es_ES.UTF-8" ) );
        } else if( lang == "fr" ) {
            std::locale::global( std::locale( "fr_FR.UTF-8" ) );
        } else if( lang == "hu" ) {
            std::locale::global( std::locale( "hu_HU.UTF-8" ) );
        } else if( lang == "ja" ) {
            std::locale::global( std::locale( "ja_JP.UTF-8" ) );
        } else if( lang == "ko" ) {
            std::locale::global( std::locale( "ko_KR.UTF-8" ) );
        } else if( lang == "pl" ) {
            std::locale::global( std::locale( "pl_PL.UTF-8" ) );
        } else if( lang == "pt_BR" ) {
            std::locale::global( std::locale( "pt_BR.UTF-8" ) );
        } else if( lang == "ru" ) {
            std::locale::global( std::locale( "ru_RU.UTF-8" ) );
        } else if( lang == "zh_CN" ) {
            std::locale::global( std::locale( "zh_CN.UTF-8" ) );
        } else if( lang == "zh_TW" ) {
            std::locale::global( std::locale( "zh_TW.UTF-8" ) );
        }
    } catch( std::runtime_error &e ) {
        std::locale::global( std::locale() );
    }

    DebugLog( D_INFO, DC_ALL ) << "[options] C locale set to " << setlocale( LC_ALL, nullptr );
    DebugLog( D_INFO, DC_ALL ) << "[options] C++ locale set to " << std::locale().name();
}
