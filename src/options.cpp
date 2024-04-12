#include "options.h"

#include <cfloat>
#include <climits>
#include <clocale>
#include <iterator>
#include <stdexcept>

#include "cached_options.h"
#include "calendar.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "color.h"
#include "cursesdef.h"
#include "cursesport.h" // IWYU pragma: keep
#include "debug.h"
#include "filesystem.h"
#include "game.h"
#include "game_constants.h"
#include "generic_factory.h"
#include "input_context.h"
#include "json.h"
#include "lang_stats.h"
#include "line.h"
#include "mapsharing.h"
#include "output.h"
#include "path_info.h"
#include "point.h"
#include "popup.h"
#include "sdltiles.h" // IWYU pragma: keep
#include "sdlsound.h"
#include "sounds.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "system_locale.h"
#include "translations.h"
#include "try_parse_integer.h"
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
#include <memory>
#include <sstream>
#include <string>

std::map<std::string, cata_path> TILESETS; // All found tilesets: <name, tileset_dir>
std::map<std::string, cata_path> SOUNDPACKS; // All found soundpacks: <name, soundpack_dir>

namespace
{

generic_factory<option_slider> option_slider_factory( "option slider" );

} // namespace

/** @relates string_id */
template<>
const option_slider &string_id<option_slider>::obj() const
{
    return option_slider_factory.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<option_slider>::is_valid() const
{
    return option_slider_factory.is_valid( *this );
}

void option_slider::reset()
{
    option_slider_factory.reset();
}

const std::vector<option_slider> &option_slider::get_all()
{
    return option_slider_factory.get_all();
}

void option_slider::load_option_sliders( const JsonObject &jo, const std::string &src )
{
    option_slider_factory.load( jo, src );
}

void option_slider::finalize_all()
{
    for( const option_slider &opt : option_slider::get_all() ) {
        option_slider &o = const_cast<option_slider &>( opt );
        o.reorder_opts();
    }
}

void option_slider::check_consistency()
{
    for( const option_slider &opt : option_slider::get_all() ) {
        opt.check();
    }
}

void option_slider::load( const JsonObject &jo, const std::string_view )
{
    mandatory( jo, was_loaded, "name", _name );
    optional( jo, was_loaded, "default", _default_level, 0 );
    optional( jo, was_loaded, "context", _context );
    mandatory( jo, was_loaded, "levels", _levels );
}

void option_slider::option_slider_level::deserialize( const JsonObject &jo )
{
    mandatory( jo, false, "level", _level );
    mandatory( jo, false, "name", _name );
    optional( jo, false, "description", _desc );
    _opts.clear();
    for( JsonObject jobj : jo.get_array( "options" ) ) {
        const std::string stype = jobj.get_string( "type" );
        std::stringstream valss;
        if( stype == "int" ) {
            valss << jobj.get_int( "val" );
        } else if( stype == "float" ) {
            valss << jobj.get_float( "val" );
        } else if( stype == "bool" ) {
            valss << ( jobj.get_bool( "val" ) ? "true" : "false" );
        } else if( stype == "string" ) {
            valss << jobj.get_string( "val" );
        }
        _opts.emplace_back( jobj.get_string( "option" ), stype, valss.str() );
    }
}

void option_slider::check() const
{
    std::set<int> lvls;
    for( const option_slider_level &lvl : _levels ) {
        if( lvl.level() < 0 || lvl.level() >= static_cast<int>( _levels.size() ) ) {
            debugmsg( "Option slider level \"%s\" (from option slider \"%s\") has a numeric "
                      "level of %d, but must be between 0 and %d (total number of levels minus 1)",
                      lvl.name().translated().c_str(), id.c_str(), lvl.level(),
                      static_cast<int>( _levels.size() ) - 1 );
        }
        lvls.emplace( lvl.level() );
    }

    if( lvls.size() != _levels.size() ) {
        debugmsg( "Option slider \"%s\" has duplicate slider levels.  Each slider level must "
                  "be unique, from 0 (zero) to %d (total number of levels minus 1)",
                  id.c_str(), static_cast<int>( _levels.size() ) - 1 );
    }

    if( lvls.count( _default_level ) == 0 ) {
        debugmsg( "Default slider level (%d) for option slider \"%s\" does not match any of the "
                  "defined levels", _default_level, id.c_str() );
    }
}

bool option_slider::option_slider_level::remove( const std::string &opt )
{
    auto iter = std::find_if( _opts.begin(), _opts.end(), [&opt]( const opt_slider_option & o ) {
        return o._opt == opt;
    } );
    if( iter == _opts.end() ) {
        return false;
    }
    _opts.erase( iter );
    return true;
}

void option_slider::option_slider_level::apply_opts( options_manager::options_container &OPTIONS )
const
{
    for( const opt_slider_option &opt : _opts ) {
        auto iter = OPTIONS.find( opt._opt );
        if( iter != OPTIONS.end() ) {
            iter->second.setValue( opt._val );
        }
    }
}

int option_slider::random_level() const
{
    return rng( 0, _levels.size() - 1 );
}

// Map from old option name to pair of <new option name and map of old option value to new option value>
// Options and values not listed here will not be changed.
static const std::map<std::string, std::pair<std::string, std::map<std::string, std::string>>>
&get_migrated_options()
{
    static const std::map<std::string, std::pair<std::string, std::map<std::string, std::string>>> opt
    = { {"DELETE_WORLD", { "WORLD_END", { {"no", "keep" }, {"yes", "delete"} } } } };
    return opt;
}

options_manager &get_options()
{
    static options_manager single_instance;
    return single_instance;
}

options_manager::options_manager()
{
    pages_.emplace_back( "general", to_translation( "General" ) );
    pages_.emplace_back( "interface", to_translation( "Interface" ) );
    pages_.emplace_back( "graphics", to_translation( "Graphics" ) );
    // when sharing maps only admin is allowed to change these.
    if( !MAP_SHARING::isCompetitive() || MAP_SHARING::isAdmin() ) {
        pages_.emplace_back( "world_default", to_translation( "World Defaults" ) );
        pages_.emplace_back( "debug", to_translation( "Debug" ) );
    }

#if defined(__ANDROID__)
    pages_.emplace_back( "android", to_translation( "Android" ) );
#endif

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
            for( const PageItem &i : p.items_ ) {
                if( i.type == ItemType::Option && i.data == name ) {
                    return;
                }
            }
            p.items_.emplace_back( ItemType::Option, name, adding_to_group_ );
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
                                    const translation &sMenuTextIn, const translation &sTooltipIn )
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
                           const translation &sMenuTextIn, const translation &sTooltipIn,
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
                           const translation &sMenuTextIn, const translation &sTooltipIn,
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
                           const translation &sMenuTextIn, const translation &sTooltipIn,
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
                           const translation &sMenuTextIn, const translation &sTooltipIn,
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
                           const translation &sMenuTextIn, const translation &sTooltipIn,
                           const std::vector<int_and_option> &mIntValuesIn,
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
        iInitialIn = mIntValuesIn[0].first;
    }

    item = thisOpt.findInt( iDefaultIn );
    if( !item ) {
        iDefaultIn = mIntValuesIn[0].first;
    }

    thisOpt.iDefault = iDefaultIn;
    thisOpt.iSet = iInitialIn;

    addOptionToPage( sNameIn, sPageIn );

    options[sNameIn] = thisOpt;
}

//add float option
void options_manager::add( const std::string &sNameIn, const std::string &sPageIn,
                           const translation &sMenuTextIn, const translation &sTooltipIn,
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

void options_manager::add_empty_line( const std::string &sPageIn )
{
    for( Page &p : pages_ ) {
        if( p.id_ == sPageIn ) {
            p.items_.emplace_back( ItemType::BlankLine, "", adding_to_group_ );
            break;
        }
    }
}

void options_manager::add_option_group( const std::string &page_id,
                                        const options_manager::Group &group,
                                        const std::function<void( const std::string & )> &entries )
{
    if( !adding_to_group_.empty() ) {
        // Nested groups are not allowed
        debugmsg( "Tried to create option group '%s' from within group '%s'.",
                  group.id_, adding_to_group_ );
        return;
    }
    for( Group &g : groups_ ) {
        if( g.id_ == group.id_ ) {
            debugmsg( "Option group with id '%s' already exists", group.id_ );
            return;
        }
    }
    groups_.push_back( group );
    adding_to_group_ = groups_.back().id_;

    for( Page &p : pages_ ) {
        if( p.id_ == page_id ) {
            p.items_.emplace_back( ItemType::GroupHeader, group.id_, adding_to_group_ );
            break;
        }
    }

    entries( page_id );

    adding_to_group_.clear();
}

const options_manager::Group &options_manager::find_group( const std::string &id ) const
{
    static Group null_group;
    if( id.empty() ) {
        return null_group;
    }
    for( const Group &g : groups_ ) {
        if( g.id_ == id ) {
            return g;
        }
    }
    debugmsg( "Option group with id '%s' does not exist.", id );
    return null_group;
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
        case COPT_NO_HIDE: // NOLINT(bugprone-branch-clone)
            return false;

        case COPT_SDL_HIDE:
#if defined(TILES)
            return true;
#else
            return false;
#endif

        case COPT_CURSES_HIDE: // NOLINT(bugprone-branch-clone)
#if !defined(TILES) // If not defined, it's the curses interface.
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
    return sMenuText.translated();
}

std::string options_manager::cOpt::getTooltip() const
{
    return sTooltip.translated();
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

template<typename T>
std::optional<T> options_manager::cOpt::_convert() const
{
    if constexpr( std::is_same_v<T, std::string> ) {
        return getValue( true );
    } else {
        if( eType == CVT_BOOL ) {
            return static_cast<T>( bSet );
        } else if( eType == CVT_FLOAT ) {
            return static_cast<T>( fSet );
        } else if( eType == CVT_INT ) {
            return static_cast<T>( iSet );
        }
    }
    return std::nullopt;
}

template<>
std::string options_manager::cOpt::value_as<std::string>( bool convert ) const
{
    if( std::optional<std::string> ret = _convert<std::string>(); convert && ret ) {
        return *ret;
    }
    if( eType != CVT_STRING ) {
        debugmsg( "%s tried to get string value from option of type %s", sName, sType );
    }
    return sSet;
}

template<>
bool options_manager::cOpt::value_as<bool>( bool convert ) const
{
    if( std::optional<bool> ret = _convert<bool>(); convert && ret ) {
        return *ret;
    }
    if( eType != CVT_BOOL ) {
        debugmsg( "%s tried to get boolean value from option of type %s", sName, sType );
    }
    return bSet;
}

template<>
float options_manager::cOpt::value_as<float>( bool convert ) const
{
    if( std::optional<float> ret = _convert<float>(); convert && ret ) {
        return *ret;
    }
    if( eType != CVT_FLOAT ) {
        debugmsg( "%s tried to get float value from option of type %s", sName, sType );
    }
    return fSet;
}

template<>
int options_manager::cOpt::value_as<int>( bool convert ) const
{
    if( std::optional<int> ret = _convert<int>(); convert && ret ) {
        return *ret;
    }
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
        const std::optional<int_and_option> opt = findInt( iSet );
        if( opt ) {
            if( verbose ) {
                return string_format( _( "%d: %s" ), iSet, opt->second );
            } else {
                return opt->second.translated();
            }
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
        const std::optional<int_and_option> opt = findInt( iDefault );
        if( opt ) {
            if( verbose ) {
                return string_format( _( "Default: %d: %s" ), iDefault, opt->second );
            } else {
                return string_format( _( "Default: %s" ), opt->second );
            }
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
            if( mIntValues[i].first == iSearch ) {
                return i;
            }
        }
    }

    return -1;
}

std::optional<options_manager::int_and_option> options_manager::cOpt::findInt(
    const int iSearch ) const
{
    int i = static_cast<int>( getIntPos( iSearch ) );
    if( i == -1 ) {
        return std::nullopt;
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
        int iMenuTextLength = utf8_width( sMenuText.translated() );
        string_input_popup()
        .width( iMaxLength > 80 ? 80 : iMaxLength < iMenuTextLength ? iMenuTextLength : iMaxLength + 1 )
        .title( sMenuText.translated() )
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
        iSet = mIntValues[iNext].first;

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
        iSet = mIntValues[iPrev].first;

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
void options_manager::cOpt::setValue( const std::string &sSetIn )
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
        // Some integer values are stored with a '%', e.g. "100%".
        std::string without_percent = sSetIn;
        if( string_ends_with( without_percent, "%" ) ) {
            without_percent.erase( without_percent.end() - 1 );
        }
        ret_val<int> val = try_parse_integer<int>( without_percent, false );

        if( val.success() ) {
            iSet = val.value();

            if( iSet < iMin || iSet > iMax ) {
                iSet = iDefault;
            }
        } else {
            debugmsg( "Error parsing option as integer: %s", val.str() );
            iSet = iDefault;
        }

    } else if( sType == "int_map" ) {
        ret_val<int> val = try_parse_integer<int>( sSetIn, false );

        if( val.success() ) {
            iSet = val.value();

            auto item = findInt( iSet );
            if( !item ) {
                iSet = iDefault;
            }
        } else {
            debugmsg( "Error parsing option as integer: %s", val.str() );
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
    const std::string slash_filename = "/" + filename;

    for( const std::string &resource_dir : resource_dirs ) {
        read_from_file( resource_dir + slash_filename, [&]( std::istream & fin ) {
            std::string resource_name;
            std::string view_name;
            // should only have 2 values inside it, otherwise is going to only load the last 2 values
            while( !fin.eof() ) {
                std::string sOption;
                fin >> sOption;

                if( sOption.empty() || sOption[0] == '#' ) {
                    getline( fin, sOption );    // Empty line or comment, chomp it
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

static std::vector<options_manager::id_and_option> build_resource_list(
    std::map<std::string, cata_path> &resource_option, const std::string &operation_name,
    const cata_path &dirname, const std::string &filename )
{
    std::vector<options_manager::id_and_option> resource_names;

    resource_option.clear();
    if( !dir_exist( dirname.get_unrelative_path() ) ) {
        return resource_names; // don't try to enumerate non-existing directories
    }
    const auto resource_dirs = get_directories_with( filename, dirname, true );

    for( const cata_path &resource_dir : resource_dirs ) {
        read_from_file( resource_dir / filename, [&]( std::istream & fin ) {
            std::string resource_name;
            std::string view_name;
            // should only have 2 values inside it, otherwise is going to only load the last 2 values
            while( !fin.eof() ) {
                std::string sOption;
                fin >> sOption;

                if( sOption.empty() || sOption[0] == '#' ) {
                    getline( fin, sOption );  // Empty line or comment, chomp it
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
                resource_option.insert( std::pair<std::string, cata_path>( resource_name, resource_dir ) );
            }
        } );
    }

    return resource_names;
}

void options_manager::search_resource(
    std::map<std::string, std::string> &storage, std::vector<id_and_option> &option_list,
    const std::vector<std::string> &search_paths, const std::string &resource_name,
    const std::string &resource_filename )
{
    // Clear the result containers.
    storage.clear();
    option_list.clear();

    // Loop through each search path and add its resources.
    for( const std::string &search_path : search_paths ) {
        // Get the resource list from the search path.
        std::map<std::string, std::string> resources;
        std::vector<id_and_option> resource_names = build_resource_list( resources, resource_name,
                search_path, resource_filename );

        // Add any new resources from this path to the result containers.
        // First, add to the resource mapping.
        storage.insert( resources.begin(), resources.end() );
        // Next, add to the option list.
        for( const id_and_option &name : resource_names ) {
            // Only add if not a duplicate.
            if( std::find( option_list.begin(), option_list.end(), name ) == option_list.end() ) {
                option_list.emplace_back( name );
            }
        }
    }
}

void options_manager::search_resource(
    std::map<std::string, cata_path> &storage, std::vector<id_and_option> &option_list,
    const std::vector<cata_path> &search_paths, const std::string &resource_name,
    const std::string &resource_filename )
{
    // Clear the result containers.
    storage.clear();
    option_list.clear();

    // Loop through each search path and add its resources.
    for( const cata_path &search_path : search_paths ) {
        // Get the resource list from the search path.
        std::map<std::string, cata_path> resources;
        std::vector<id_and_option> resource_names = build_resource_list( resources, resource_name,
                search_path, resource_filename );

        // Add any new resources from this path to the result containers.
        // First, add to the resource mapping.
        storage.insert( resources.begin(), resources.end() );
        // Next, add to the option list.
        for( const id_and_option &name : resource_names ) {
            // Only add if not a duplicate.
            if( std::find( option_list.begin(), option_list.end(), name ) == option_list.end() ) {
                option_list.emplace_back( name );
            }
        }
    }
}

std::vector<options_manager::id_and_option> options_manager::build_tilesets_list()
{
    std::vector<id_and_option> result;

    search_resource( TILESETS, result, { PATH_INFO::user_gfx(), PATH_INFO::gfxdir() },
                     "tileset",
                     PATH_INFO::tileset_conf() );

    // Default values
    if( result.empty() ) {
        result.emplace_back( "hoder", to_translation( "Hoder's" ) );
        result.emplace_back( "deon", to_translation( "Deon's" ) );
    }
    return result;
}

std::vector<options_manager::id_and_option> options_manager::build_soundpacks_list()
{
    std::vector<id_and_option> result;

    search_resource( SOUNDPACKS, result, { PATH_INFO::user_sound(), PATH_INFO::data_sound() },
                     "soundpack", PATH_INFO::soundpack_conf() );

    // Select default built-in sound pack
    if( result.empty() ) {
        result.emplace_back( "basic", to_translation( "Basic" ) );
    }
    return result;
}

std::unordered_set<std::string> options_manager::get_langs_with_translation_files()
{
#if defined(LOCALIZE)
    const std::string start_str = locale_dir();
    std::vector<std::string> lang_dirs =
        get_directories_with( PATH_INFO::lang_file(), start_str, true );
    std::for_each( lang_dirs.begin(), lang_dirs.end(), [&]( std::string & dir ) {
        const std::size_t end = dir.rfind( "/LC_MESSAGES" );
        const std::size_t begin = dir.rfind( '/', end - 1 ) + 1;
        dir = dir.substr( begin, end - begin );
    } );
    return std::unordered_set<std::string>( lang_dirs.begin(), lang_dirs.end() );
#else // !LOCALIZE
    return std::unordered_set<std::string>();
#endif // LOCALIZE
}

std::vector<options_manager::id_and_option> options_manager::get_lang_options()
{
    std::vector<id_and_option> lang_options = {
        { "", to_translation( "System language" ) },
    };

    constexpr std::array<std::pair<const char *, const char *>, 25> language_names = {{
            // Note: language names are in their own language and are *not* translated at all.
            // Note: Somewhere in Github PR was better link to msdn.microsoft.com with language names.
            // http://en.wikipedia.org/wiki/List_of_language_names
            { "en", R"(English)" },
            { "ar", R"(العربية)" },
            { "cs", R"(Český Jazyk)" },
            { "da", R"(Dansk)" },
            { "de", R"(Deutsch)" },
            { "el", R"(Ελληνικά)" },
            { "es_AR", R"(Español (Argentina))" },
            { "es_ES", R"(Español (España))" },
            { "fr", R"(Français)" },
            { "hu", R"(Magyar)" },
            { "id", R"(Bahasa Indonesia)" },
            { "is", R"(Íslenska)" },
            { "it_IT", R"(Italiano)" },
            { "ja", R"(日本語)" },
            { "ko", R"(한국어)" },
            { "nb", R"(Norsk)" },
            { "nl", R"(Nederlands)" },
            { "pl", R"(Polski)" },
            { "pt_BR", R"(Português (Brasil))" },
            { "ru", R"(Русский)" },
            { "sr", R"(Српски)" },
            { "tr", R"(Türkçe)" },
            { "uk_UA", R"(Українська)" },
            { "zh_CN", R"(中文 (天朝))" },
            { "zh_TW", R"(中文 (台灣))" },
        }
    };

    for( const auto& [lang_id, lang_name] : language_names ) {
        std::string name = lang_name;
        if( const lang_stats *stats = lang_stats_for( lang_id ) ) {
            name += string_format( _( " <color_dark_gray>(%.1f%%)</color>" ),
                                   stats->percent_translated() );
        }
        lang_options.emplace_back( lang_id, no_translation( name ) );
    }

    std::unordered_set<std::string> lang_list = get_langs_with_translation_files();

    std::vector<id_and_option> options;

    lang_list.insert( "" ); // for System language option
    lang_list.insert( "en" ); // for English option

    std::copy_if( lang_options.begin(), lang_options.end(), std::back_inserter( options ),
    [&lang_list]( const options_manager::id_and_option & pair ) {
        return lang_list.count( pair.first );
    } );
    return options;
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
    const auto empty = [&]( const PageItem & it ) -> bool {
        return it.type == ItemType::BlankLine ||
        ( it.type == ItemType::Option && get_options().get_option( it.data ).is_hidden() );
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
    add_options_world_default();
    add_options_debug();
    add_options_android();

    for( Page &p : pages_ ) {
        p.removeRepeatedEmptyLines();
    }
}

void options_manager::add_options_general()
{
    const auto add_empty_line = [&]() {
        this->add_empty_line( "general" );
    };

    add( "DEF_CHAR_NAME", "general", to_translation( "Default character name" ),
         to_translation( "Set a default character name that will be used instead of a random name on character creation." ),
         "", 30
       );

    add_empty_line();
    add_option_group( "general", Group( "auto_pick_opts", to_translation( "Auto Pickup Options" ),
                                        to_translation( "Options regarding auto pickup." ) ),
    [&]( const std::string & page_id ) {
        add( "AUTO_PICKUP", page_id, to_translation( "Auto pickup enabled" ),
             to_translation( "If true, enable item auto pickup.  Change pickup rules with the Auto pickup manager." ),
             false
           );

        add( "AUTO_PICKUP_ADJACENT", page_id, to_translation( "Auto pickup adjacent" ),
             to_translation( "If true, enable auto pickup items one tile around to the player.  You can assign No Auto Pickup zones with the Zones manager for e.g. your homebase." ),
             false
           );

        get_option( "AUTO_PICKUP_ADJACENT" ).setPrerequisite( "AUTO_PICKUP" );

        add( "AUTO_PICKUP_OWNED", page_id, to_translation( "Auto pickup owned items" ),
             to_translation( "If true, items that belong to your faction will be included in auto pickup." ),
             false
           );

        get_option( "AUTO_PICKUP_OWNED" ).setPrerequisite( "AUTO_PICKUP" );

        add( "AUTO_PICKUP_WEIGHT_LIMIT", page_id, to_translation( "Auto pickup weight limit" ),
             to_translation( "Auto pickup items with weight less than or equal to [option] * 50 grams.  You must also set the small items option.  0 = disabled." ),
             0, 100, 0
           );

        get_option( "AUTO_PICKUP_WEIGHT_LIMIT" ).setPrerequisite( "AUTO_PICKUP" );

        add( "AUTO_PICKUP_VOLUME_LIMIT", page_id, to_translation( "Auto pickup volume limit" ),
             to_translation( "Auto pickup items with volume less than or equal to [option] * 50 milliliters.  You must also set the light items option.  0 = disabled." ),
             0, 100, 0
           );

        get_option( "AUTO_PICKUP_VOLUME_LIMIT" ).setPrerequisite( "AUTO_PICKUP" );

        add( "AUTO_PICKUP_SAFEMODE", page_id, to_translation( "Auto pickup safe mode" ),
             to_translation( "If true, auto pickup is disabled as long as you can see monsters nearby.  This is affected by 'Safe mode proximity distance'." ),
             false
           );

        get_option( "AUTO_PICKUP_SAFEMODE" ).setPrerequisite( "AUTO_PICKUP" );

        add( "NO_AUTO_PICKUP_ZONES_LIST_ITEMS", page_id,
             to_translation( "List items within no auto pickup zones" ),
             to_translation( "If true, you will see messages about items you step on, within no auto pickup zones." ),
             true
           );

        get_option( "NO_AUTO_PICKUP_ZONES_LIST_ITEMS" ).setPrerequisite( "AUTO_PICKUP" );
    } );

    add_empty_line();

    add_option_group( "general", Group( "auto_feat_opts", to_translation( "Auto Features Options" ),
                                        to_translation( "Options regarding auto features." ) ),
    [&]( const std::string & page_id ) {
        add( "AUTO_FEATURES", page_id, to_translation( "Additional auto features" ),
             to_translation( "If true, enables configured auto features below.  Disabled as long as any enemy monster is seen." ),
             false
           );

        add( "AUTO_PULP_BUTCHER", page_id, to_translation( "Auto pulp or butcher" ),
             to_translation( "Action to perform when 'Auto pulp or butcher' is enabled.  Pulp: Pulp corpses you stand on.  - Pulp Adjacent: Also pulp corpses adjacent from you.  - Butcher: Butcher corpses you stand on." ),
        { { "off", to_translation( "options", "Disabled" ) }, { "pulp", to_translation( "Pulp" ) }, { "pulp_zombie_only", to_translation( "Pulp Zombies Only" ) }, { "pulp_adjacent", to_translation( "Pulp Adjacent" ) }, { "pulp_adjacent_zombie_only", to_translation( "Pulp Adjacent Zombie Only" ) }, { "butcher", to_translation( "Butcher" ) } },
        "off"
           );

        get_option( "AUTO_PULP_BUTCHER" ).setPrerequisite( "AUTO_FEATURES" );

        add( "AUTO_MINING", page_id, to_translation( "Auto mining" ),
             to_translation( "If true, enables automatic use of wielded pickaxes and jackhammers whenever trying to move into mineable terrain." ),
             false
           );

        get_option( "AUTO_MINING" ).setPrerequisite( "AUTO_FEATURES" );

        add( "AUTO_MOPPING", page_id, to_translation( "Auto mopping" ),
             to_translation( "If true, enables automatic use of wielded mops to clean surrounding terrain." ),
             false
           );

        get_option( "AUTO_MOPPING" ).setPrerequisite( "AUTO_FEATURES" );

        add( "AUTO_FORAGING", page_id, to_translation( "Auto foraging" ),
             to_translation( "Action to perform when 'Auto foraging' is enabled.  Bushes: Only forage bushes.  - Trees: Only forage trees.  - Crops: Only forage crops.  - Everything: Forage bushes, trees, crops, and everything else including flowers, cattails etc." ),
        { { "off", to_translation( "options", "Disabled" ) }, { "bushes", to_translation( "Bushes" ) }, { "trees", to_translation( "Trees" ) }, { "crops", to_translation( "Crops" ) }, { "all", to_translation( "Everything" ) } },
        "off"
           );

        get_option( "AUTO_FORAGING" ).setPrerequisite( "AUTO_FEATURES" );
    } );

    add_empty_line();
    add_option_group( "general", Group( "player_safe_opts", to_translation( "Player Safety Options" ),
                                        to_translation( "Options regarding player safety." ) ),
    [&]( const std::string & page_id ) {
        add( "DANGEROUS_PICKUPS", page_id, to_translation( "Dangerous pickups" ),
             to_translation( "If true, will allow player to pick new items, even if it causes them to exceed the weight limit." ),
             false
           );

        add( "DANGEROUS_TERRAIN_WARNING_PROMPT", page_id,
             to_translation( "Dangerous terrain warning prompt" ),
             to_translation( "Always: You will be prompted to move onto dangerous tiles.  Running: You will only be able to move onto dangerous tiles while running and will be prompted.  Crouching: You will only be able to move onto a dangerous tile while crouching and will be prompted.  Never:  You will not be able to move onto a dangerous tile unless running and will not be warned or prompted." ),
        { { "ALWAYS", to_translation( "Always" ) }, { "RUNNING", to_translation( "Running" ) }, { "CROUCHING", to_translation( "Crouching" ) }, { "NEVER", to_translation( "Never" ) } },
        "ALWAYS"
           );

        add( "FORCE_SMART_CONTROLLER_OFF_ON_ENGINE_STOP", page_id,
             to_translation( "Force smart engine controller off" ),
             to_translation( "If enabled, turn off the smart engine controller when you turn off the engine of the car without an electric motor." ),
        {
            { "disabled", to_translation( "options", "Disabled" ) },
            { "enabled", to_translation( "Enabled" ) },
            { "ask", to_translation( "Ask" ) }
        }, "ask"
           );
    } );

    add_empty_line();

    add_option_group( "general", Group( "safe_mode_opts", to_translation( "Safe Mode Options" ),
                                        to_translation( "Options regarding safe mode." ) ),
    [&]( const std::string & page_id ) {
        add( "SAFEMODE", page_id, to_translation( "Safe mode" ),
             to_translation( "If true, will hold the game and display a warning if a hostile monster/NPC is approaching." ),
             true
           );

        add( "SAFEMODEPROXIMITY", page_id, to_translation( "Safe mode proximity distance" ),
             to_translation( "If safe mode is enabled, distance to hostiles at which safe mode should show a warning.  0 = Max player view distance.  This option only has effect when no safe mode rule is specified.  Otherwise, edit the default rule in Safe mode manager instead of this value." ),
             0, MAX_VIEW_DISTANCE, 0
           );

        add( "SAFEMODEVEH", page_id, to_translation( "Safe mode when driving" ),
             to_translation( "If true, safe mode will alert you of hostiles while you are driving a vehicle." ),
             false
           );

        add( "AUTOSAFEMODE", page_id, to_translation( "Auto reactivate safe mode" ),
             to_translation( "If true, safe mode will automatically reactivate after a certain number of turns.  See option 'Turns to auto reactivate safe mode.'" ),
             false
           );

        add( "AUTOSAFEMODETURNS", page_id, to_translation( "Turns to auto reactivate safe mode" ),
             to_translation( "Number of turns after which safe mode is reactivated.  Will only reactivate if no hostiles are in 'Safe mode proximity distance.'" ),
             1, 600, 50
           );

        add( "SAFEMODEIGNORETURNS", page_id, to_translation( "Turns to remember ignored monsters" ),
             to_translation( "Number of turns an ignored monster stays ignored after it is no longer seen.  0 disables this option and monsters are permanently ignored." ),
             0, 3600, 200
           );
    } );

    add_empty_line();

    add( "TURN_DURATION", "general", to_translation( "Realtime turn progression" ),
         to_translation( "If higher than 0, monsters will take periodic gameplay turns.  This value is the delay between each turn, in seconds.  Works best with Safe Mode disabled.  0 = disabled." ),
         0.0, 10.0, 0.0, 0.05
       );

    add_empty_line();

    add_option_group( "general", Group( "auto_save_opts", to_translation( "Autosave Options" ),
                                        to_translation( "Options regarding autosave." ) ),
    [&]( const std::string & page_id ) {
        add( "AUTOSAVE", page_id, to_translation( "Autosave" ),
             to_translation( "If true, game will periodically save the map.  Autosaves occur based on in-game turns or realtime minutes, whichever is larger." ),
             true
           );

        add( "AUTOSAVE_TURNS", page_id, to_translation( "Game turns between autosaves" ),
             to_translation( "Number of game turns between autosaves." ),
             10, 1000, 50
           );

        get_option( "AUTOSAVE_TURNS" ).setPrerequisite( "AUTOSAVE" );

        add( "AUTOSAVE_MINUTES", page_id, to_translation( "Real minutes between autosaves" ),
             to_translation( "Number of realtime minutes between autosaves." ),
             0, 127, 5
           );

        get_option( "AUTOSAVE_MINUTES" ).setPrerequisite( "AUTOSAVE" );
    } );

    add_empty_line();

    add_option_group( "general", Group( "auto_note_opts", to_translation( "Auto notes Options" ),
                                        to_translation( "Options regarding auto notes." ) ),
    [&]( const std::string & page_id ) {
        add( "AUTO_NOTES", page_id, to_translation( "Auto notes" ),
             to_translation( "If true, automatically sets notes." ),
             false
           );

        add( "AUTO_NOTES_STAIRS", page_id, to_translation( "Auto notes (stairs)" ),
             to_translation( "If true, automatically sets notes on places that have stairs that go up or down." ),
             false
           );

        get_option( "AUTO_NOTES_STAIRS" ).setPrerequisite( "AUTO_NOTES" );

        add( "AUTO_NOTES_MAP_EXTRAS", page_id, to_translation( "Auto notes (map extras)" ),
             to_translation( "If true, automatically sets notes on places that contain various map extras." ),
             false
           );

        get_option( "AUTO_NOTES_MAP_EXTRAS" ).setPrerequisite( "AUTO_NOTES" );

        add( "AUTO_NOTES_DROPPED_FAVORITES", page_id, to_translation( "Auto notes (dropped favorites)" ),
             to_translation( "If true, automatically sets notes when player drops favorited items." ),
             false
           );

        get_option( "AUTO_NOTES_DROPPED_FAVORITES" ).setPrerequisite( "AUTO_NOTES" );
    } );

    add_empty_line();

    add_option_group( "general", Group( "misc_general_opts", to_translation( "Misc Options" ),
                                        to_translation( "Miscellaneous Options." ) ),
    [&]( const std::string & page_id ) {
        add( "CIRCLEDIST", page_id, to_translation( "Circular distances" ),
             to_translation( "If true, the game will calculate range in a realistic way: light sources will be circles, diagonal movement will cover more ground and take longer.  If false, everything is square: moving to the northwest corner of a building takes as long as moving to the north wall." ),
             true
           );

        add( "DROP_EMPTY", page_id, to_translation( "Drop empty containers" ),
             to_translation( "Set to drop empty containers after use.  No: Don't drop any.  - Watertight: All except watertight containers.  - All: Drop all containers." ),
        { { "no", to_translation( "No" ) }, { "watertight", to_translation( "Watertight" ) }, { "all", to_translation( "All" ) } },
        "no"
           );

        add( "DEATHCAM", page_id, to_translation( "DeathCam" ),
             to_translation( "Always: Always start deathcam.  Ask: Query upon death.  Never: Never show deathcam." ),
        { { "always", to_translation( "Always" ) }, { "ask", to_translation( "Ask" ) }, { "never", to_translation( "Never" ) } },
        "ask"
           );

        add( "EVENT_SPAWNS", page_id, to_translation( "Special event spawns" ),
             to_translation( "If not disabled, unique items and/or monsters can spawn during special events (Christmas, Halloween, etc.)" ),
        { { "off", to_translation( "Disabled" ) }, { "items", to_translation( "Items" ) }, { "monsters", to_translation( "Monsters" ) }, { "both", to_translation( "Both" ) } },
        "off" );
    } );

    add_empty_line();

    add_option_group( "general", Group( "soundpacks_opts", to_translation( "Soundpack Options" ),
                                        to_translation( "Options regarding Soundpack." ) ),
    [&]( const std::string & page_id ) {
        add( "SOUND_ENABLED", page_id, to_translation( "Sound enabled" ),
             to_translation( "If true, music and sound are enabled." ),
             true, COPT_NO_SOUND_HIDE
           );

        add( "SOUNDPACKS", page_id, to_translation( "Choose soundpack" ),
             to_translation( "Choose the soundpack you want to use.  Requires restart." ),
             build_soundpacks_list(), "basic", COPT_NO_SOUND_HIDE
           ); // populate the options dynamically

        get_option( "SOUNDPACKS" ).setPrerequisite( "SOUND_ENABLED" );

        add( "MUSIC_VOLUME", page_id, to_translation( "Music volume" ),
             to_translation( "Adjust the volume of the music being played in the background." ),
             0, 128, 100, COPT_NO_SOUND_HIDE
           );

        get_option( "MUSIC_VOLUME" ).setPrerequisite( "SOUND_ENABLED" );

        add( "SOUND_EFFECT_VOLUME", page_id, to_translation( "Sound effect volume" ),
             to_translation( "Adjust the volume of sound effects being played by the game." ),
             0, 128, 100, COPT_NO_SOUND_HIDE
           );

        get_option( "SOUND_EFFECT_VOLUME" ).setPrerequisite( "SOUND_ENABLED" );

        add( "AMBIENT_SOUND_VOLUME", page_id, to_translation( "Ambient sound volume" ),
             to_translation( "Adjust the volume of ambient sounds being played by the game." ),
             0, 128, 100, COPT_NO_SOUND_HIDE
           );

        get_option( "AMBIENT_SOUND_VOLUME" ).setPrerequisite( "SOUND_ENABLED" );
    } );
}

void options_manager::add_options_interface()
{
    const auto add_empty_line = [&]() {
        this->add_empty_line( "interface" );
    };

    add_empty_line();

    add( "USE_LANG", "interface", to_translation( "Language" ),
         to_translation( "Switch language.  Each percentage is the fraction of strings translated "
                         "for that language." ),
         options_manager::get_lang_options(), "" );

    add_empty_line();

    add_option_group( "interface", Group( "measurement_unit", to_translation( "Measurement Units" ),
                                          to_translation( "Options regarding measurement units." ) ),
    [&]( const std::string & page_id ) {
        add( "USE_CELSIUS", page_id, to_translation( "Temperature units" ),
             to_translation( "Switch between Fahrenheit, Celsius, and Kelvin." ),
        { { "fahrenheit", to_translation( "Fahrenheit" ) }, { "celsius", to_translation( "Celsius" ) }, { "kelvin", to_translation( "Kelvin" ) } },
        "fahrenheit"
           );

        add( "USE_METRIC_SPEEDS", page_id, to_translation( "Speed units" ),
             to_translation( "Switch between mph, km/h, and tiles/turn." ),
        { { "mph", to_translation( "mph" ) }, { "km/h", to_translation( "km/h" ) }, { "t/t", to_translation( "tiles/turn" ) } },
        ( SystemLocale::UseMetricSystem().value_or( false ) ? "km/h" : "mph" )
           );

        add( "USE_METRIC_WEIGHTS", page_id, to_translation( "Mass units" ),
             to_translation( "Switch between lbs and kg." ),
        { { "lbs", to_translation( "lbs" ) }, { "kg", to_translation( "kg" ) } },
        ( SystemLocale::UseMetricSystem().value_or( false ) ? "kg" : "lbs" )
           );

        add( "VOLUME_UNITS", page_id, to_translation( "Volume units" ),
             to_translation( "Switch between the cups (c), liters (L), and quarts (qt)." ),
        { { "c", to_translation( "Cup" ) }, { "l", to_translation( "Liter" ) }, { "qt", to_translation( "Quart" ) } },
        "l"
           );
        add( "DISTANCE_UNITS", page_id, to_translation( "Distance units" ),
             to_translation( "Switch between metric and imperial distance units." ),
        { { "metric", to_translation( "Metric" ) }, { "imperial", to_translation( "Imperial" ) } },
        ( SystemLocale::UseMetricSystem().value_or( false ) ? "metric" : "imperial" ) );

        add( "24_HOUR", page_id, to_translation( "Time format" ),
             to_translation( "12h: AM/PM, e.g. 7:31 AM - Military: 24h Military, e.g. 0731 - 24h: Normal 24h, e.g. 7:31" ),
             //~ 12h time, e.g.  11:59pm
        {   { "12h", to_translation( "12h" ) },
            //~ Military time, e.g.  2359
            { "military", to_translation( "Military" ) },
            //~ 24h time, e.g.  23:59
            { "24h", to_translation( "24h" ) }
        },
        "12h" );
        add( "SHOW_VITAMIN_MASS", page_id, to_translation( "Show vitamin masses" ),
             to_translation( "Display the masses of vitamins in addition to units/RDA values in item descriptions." ),
             true );
    } );

    add_empty_line();

    add_option_group( "interface", Group( "additional_messages_in_log",
                                          to_translation( "Additional messages in the log" ),
                                          to_translation( "If true, some additional messages will be shown in the sidebar message log." ) ),
    [&]( const std::string & page_id ) {

        add( "LOG_ITEMS_ON_THE_GROUND", page_id, to_translation( "Items on the ground" ),
             to_translation( "Show messages about items lying on the ground when character steps on tile with them." ),
             true
           );

        add( "LOG_MONSTER_MOVEMENT", page_id, to_translation( "Special monster movement" ),
             to_translation( "Show messages about special monster movement: flying over the fence, diving into the water etc." ),
             true
           );

        add( "LOG_MONSTER_MOVE_EFFECTS", page_id, to_translation( "Monster attempts to free itself" ),
             to_translation( "Show messages about monster trying to free itself from effects preventing it from moving: downed, tied etc." ),
             true
           );

        add( "LOG_MONSTER_ATTACK_MONSTER", page_id, to_translation( "Monster/NPC attacking monster/NPC" ),
             to_translation( "Show messages about non-playable creatures attacking other non-playable creatures." ),
             true
           );
    } );

    add_empty_line();

    add_option_group( "interface", Group( "naming_opts", to_translation( "Naming Options" ),
                                          to_translation( "Options regarding the naming of items." ) ),
    [&]( const std::string & page_id ) {
        add( "SHOW_DRUG_VARIANTS", page_id, to_translation( "Show drug brand names" ),
             to_translation( "If true, show brand names for drugs, instead of generic functional names - 'Adderall', instead of 'prescription stimulant'." ),
             false );
        add( "SHOW_GUN_VARIANTS", page_id, to_translation( "Show gun brand names" ),
             to_translation( "If true, show brand names for guns, instead of generic functional names - 'm4a1' or 'h&k416a5' instead of 'NATO assault rifle'." ),
             false );
        add( "AMMO_IN_NAMES", page_id, to_translation( "Add ammo to weapon/magazine names" ),
             to_translation( "If true, the default ammo is added to weapon and magazine names.  For example \"Mosin-Nagant M44 (4/5)\" becomes \"Mosin-Nagant M44 (4/5 7.62x54mm)\"." ),
             true
           );
        add( "DETAILED_CONTAINERS", page_id, to_translation( "Detailed containers" ),
             to_translation( "All: every container has detailed remaining volume info.  - Worn: only worn containers have detailed remaining volume info.  - None: no additional info is provided." ),
        {
            { "ALL", to_translation( "All" ) },
            { "WORN", to_translation( "Worn" ) },
            { "NONE", to_translation( "None" ) }
        },
        "WORN" );
    } );

    add_empty_line();

    add_option_group( "interface", Group( "accessibility", to_translation( "Accessibility Options" ),
                                          to_translation( "Options regarding the accessibility." ) ),
    [&]( const std::string & page_id ) {
        add( "SDL_KEYBOARD_MODE", page_id, to_translation( "Use key code input mode" ),
             to_translation( "Use key code or symbol input on SDL.  "
                             "Symbol is recommended for non-qwerty layouts since currently "
                             "the default keybindings for key code mode only supports qwerty.  "
                             "Key code is currently WIP and bypasses IMEs, caps lock, and num lock." ),
        { { "keychar", to_translation( "Symbol" ) }, { "keycode", to_translation( "Key code" ) } },
        "keychar", COPT_CURSES_HIDE );

        add( "USE_PINYIN_SEARCH", page_id, to_translation( "Use pinyin in search" ),
             to_translation( "If true, pinyin (pronunciation of Chinese characters) can be used in searching/filtering "
                             "(may cause major slowdown when searching through too many entries.)" ),
             false
           );

        add( "FORCE_CAPITAL_YN", page_id,
             to_translation( "Force capital/modified letters in prompts" ),
             to_translation( "If true, prompts such as Y/N queries only accepts capital or modified letters, while "
                             "lower case and unmodified letters only snap the cursor to the corresponding option." ),
             true
           );

        add( "SNAP_TO_TARGET", page_id, to_translation( "Snap to target" ),
             to_translation( "If true, automatically follow the crosshair when firing/throwing." ),
             false
           );

        add( "AIM_AFTER_FIRING", page_id, to_translation( "Reaim after firing" ),
             to_translation( "If true, after firing automatically aim again if targets are available." ),
             true
           );

        add( "QUERY_DISASSEMBLE", page_id, to_translation( "Query on disassembly while butchering" ),
             to_translation( "If true, will query before disassembling items while butchering." ),
             true
           );

        add( "QUERY_KEYBIND_REMOVAL", page_id, to_translation( "Query on keybinding removal" ),
             to_translation( "If true, will query before removing a keybinding from a hotkey." ),
             true
           );

        add( "CLOSE_ADV_INV", page_id, to_translation( "Close advanced inventory on move all" ),
             to_translation( "If true, will close the advanced inventory when the move all items command is used." ),
             false
           );

        add( "OPEN_DEFAULT_ADV_INV", page_id,
             to_translation( "Open default advanced inventory layout" ),
             to_translation( "If true, open default advanced inventory layout instead of last opened layout" ),
             false
           );

        add( "INV_USE_ACTION_NAMES", page_id, to_translation( "Display actions in \"Use item\" menu" ),
             to_translation(
                 R"(If true, actions (like "Read", "Smoke", "Wrap tighter") will be displayed next to the corresponding items.)" ),
             true
           );

        add( "AUTOSELECT_SINGLE_VALID_TARGET", page_id,
             to_translation( "Autoselect if exactly one valid target" ),
             to_translation( "If true, directional actions ( like \"Examine\", \"Open\", \"Pickup\" ) "
                             "will autoselect an adjacent tile if there is exactly one valid target." ),
             true
           );

        add( "INVENTORY_HIGHLIGHT", page_id,
             to_translation( "Inventory highlight mode" ),
             to_translation( "Highlight selected item's contents and parent container in inventory screen.  "
        "\"Symbol\" shows a highlighted caret and \"Highlight\" uses font highlighting." ), {
            { "symbol", to_translation( "Symbol" ) },
            { "highlight", to_translation( "Highlight" ) },
            { "disable", to_translation( "Disabled" ) }
        },
        "symbol"
           );

        add( "HIGHLIGHT_UNREAD_RECIPES", page_id,
             to_translation( "Highlight unread recipes" ),
             to_translation( "If true, highlight unread recipes to allow tracking of newly learned recipes." ),
             true
           );

        add( "SCREEN_READER_MODE", page_id, to_translation( "Screen reader mode" ),
             to_translation( "On supported UI screens, tweaks display of text to optimize for screen readers.  Targeted towards using the open-source screen reader 'orca' using curses for display." ),
             // See doc/USER_INTERFACE_AND_ACCESSIBILITY.md for testing and implementation notes
             false
           );
    } );

    add_empty_line();

    add( "DIAG_MOVE_WITH_MODIFIERS_MODE", "interface",
         to_translation( "Diagonal movement with cursor keys and modifiers" ),
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

         # Mode 4: Diagonal Lock

         * Holding Ctrl or Shift locks movement to diagonal only
         * This ensures that pressing ↑ + → will results in ↗ and not ↑ or →
         * Reject input if it doesn't make sense
         * Example 1: Press → while holding Shift and ↑ results in ↗
         * Example 2: Press → while holding Shift, ↑ and ← results in input rejection
         * Example 3: Press → while holding Shift and ← results in input rejection

         */
    to_translation( "Allows diagonal movement with cursor keys using CTRL and SHIFT modifiers.  Diagonal movement action keys are taken from keybindings, so you need these to be configured." ), { { "none", to_translation( "None" ) }, { "mode1", to_translation( "Mode 1: Numpad Emulation" ) }, { "mode2", to_translation( "Mode 2: CW/CCW" ) }, { "mode3", to_translation( "Mode 3: L/R Tilt" ) }, { "mode4", to_translation( "Mode 4: Diagonal Lock" ) } },
    "none", COPT_CURSES_HIDE );

    add_empty_line();

    add_option_group( "interface", Group( "veh_intf_opts",
                                          to_translation( "Vehicle Interface Options" ),
                                          to_translation( "Options regarding vehicle interface." ) ),
    [&]( const std::string & page_id ) {
        add( "VEHICLE_ARMOR_COLOR", page_id, to_translation( "Vehicle plating changes part color" ),
             to_translation( "If true, vehicle parts will change color if they are armor plated." ),
             true
           );

        add( "DRIVING_VIEW_OFFSET", page_id, to_translation( "Auto-shift the view while driving" ),
             to_translation( "If true, view will automatically shift towards the driving direction." ),
             true
           );

        add( "VEHICLE_DIR_INDICATOR", page_id, to_translation( "Draw vehicle facing indicator" ),
             to_translation( "If true, when controlling a vehicle, a white 'X' (in curses version) or a crosshair (in tiles version) at distance 10 from the center will display its current facing." ),
             true
           );

        add( "REVERSE_STEERING", page_id, to_translation( "Reverse steering direction in reverse" ),
             to_translation( "If true, when driving a vehicle in reverse, steering should also reverse like real life." ),
             false
           );
    } );

    add_empty_line();

    add_option_group( "interface", Group( "sidb_opts", to_translation( "Sidebar Interface Options" ),
                                          to_translation( "Options regarding sidebar interface." ) ),
    [&]( const std::string & page_id ) {
        add( "SIDEBAR_POSITION", page_id, to_translation( "Sidebar position" ),
             to_translation( "Switch between sidebar on the left or on the right side.  Requires restart." ),
             //~ sidebar position
        { { "left", to_translation( "Left" ) }, { "right", to_translation( "Right" ) } }, "right"
           );

        add( "SIDEBAR_SPACERS", page_id, to_translation( "Draw sidebar spacers" ),
             to_translation( "If true, adds an extra space between sidebar panels." ),
             false
           );

        add( "LOG_FLOW", page_id, to_translation( "Message log flow" ),
             to_translation( "Where new log messages should show." ),
             //~ sidebar/message log flow direction
        { { "new_top", to_translation( "Top" ) }, { "new_bottom", to_translation( "Bottom" ) } },
        "new_bottom"
           );

        add( "MESSAGE_TTL", page_id, to_translation( "Sidebar log message display duration" ),
             to_translation( "Number of turns after which a message will be removed from the sidebar log.  0 = disabled." ),
             0, 1000, 0
           );

        add( "MESSAGE_COOLDOWN", page_id, to_translation( "Message cooldown" ),
             to_translation( "Number of turns during which similar messages are hidden.  0 = disabled." ),
             0, 1000, 0
           );

        add( "MESSAGE_LIMIT", page_id, to_translation( "Limit message history" ),
             to_translation( "Number of messages to preserve in the history, and when saving." ),
             1, 10000, 255
           );

        add( "NO_UNKNOWN_COMMAND_MSG", page_id,
             to_translation( "Suppress \"Unknown command\" messages" ),
             to_translation( "If true, pressing a key with no set function will not display a notice in the chat log." ),
             false
           );

        add( "ACHIEVEMENT_COMPLETED_POPUP", page_id,
             to_translation( "Popup window when achievement completed" ),
             to_translation( "Whether to trigger a popup window when completing an achievement.  "
                             "First: when completing an achievement that has not been completed in "
        "a previous game." ), {
            { "never", to_translation( "Never" ) },
            { "always", to_translation( "Always" ) },
            { "first", to_translation( "First" ) }
        },
        "first" );

        add( "LOOKAROUND_POSITION", page_id, to_translation( "Look around position" ),
             to_translation( "Switch between look around panel being left or right." ),
        { { "left", to_translation( "Left" ) }, { "right", to_translation( "Right" ) } },
        "right"
           );

        add( "ACCURACY_DISPLAY", page_id, to_translation( "Aim window display style" ),
             to_translation( "How should confidence and steadiness be communicated to the player." ),
             //~ aim bar style - bars or numbers
        { { "numbers", to_translation( "Numbers" ) }, { "bars", to_translation( "Bars" ) } }, "bars"
           );

        add( "MORALE_STYLE", page_id, to_translation( "Morale style" ),
             to_translation( "Morale display style in sidebar." ),
        { { "vertical", to_translation( "Vertical" ) }, { "horizontal", to_translation( "Horizontal" ) } },
        "Vertical"
           );

        add( "AIM_WIDTH", page_id, to_translation( "Full screen Advanced Inventory Manager" ),
             to_translation( "If true, Advanced Inventory Manager menu will fit full screen, otherwise it will leave sidebar visible." ),
             false
           );
    } );

    add_empty_line();
    add_option_group( "interface", Group( "qol_opts", to_translation( "QOL Options" ),
                                          to_translation( "Options regarding QOL." ) ),
    [&]( const std::string & page_id ) {
        add( "MOVE_VIEW_OFFSET", page_id, to_translation( "Move view offset" ),
             to_translation( "Move view by how many squares per keypress." ),
             1, 50, 1
           );

        add( "FAST_SCROLL_OFFSET", page_id, to_translation( "Overmap fast scroll offset" ),
             to_translation( "With Fast Scroll option enabled, shift view on the overmap and while looking around by this many squares per keypress." ),
             1, 50, 5
           );

        add( "MENU_SCROLL", page_id, to_translation( "Centered menu scrolling" ),
             to_translation( "If true, menus will start scrolling in the center of the list, and keep the list centered." ),
             true
           );

        add( "SHIFT_LIST_ITEM_VIEW", page_id, to_translation( "Shift list item view" ),
             to_translation( "Centered or to edge, shift the view toward the selected item if it is outside of your current viewport." ),
        { { "false", to_translation( "False" ) }, { "centered", to_translation( "Centered" ) }, { "edge", to_translation( "To edge" ) } },
        "centered"
           );

        add( "AUTO_INV_ASSIGN", page_id, to_translation( "Auto inventory letters" ),
             to_translation( "Enabled: automatically assign letters to any carried items that lack them.  Disabled: do not auto-assign letters.  "
        "Favorites: only auto-assign letters to favorited items." ), {
            { "disabled", to_translation( "options", "Disabled" ) },
            { "enabled", to_translation( "Enabled" ) },
            { "favorites", to_translation( "Favorites" ) }
        },
        "favorites" );

        add( "ITEM_HEALTH", "interface", to_translation( "Show item health" ),
             // NOLINTNEXTLINE(cata-text-style): one space after "etc."
        to_translation( "Show item health bars, descriptions like reinforced, scratched etc. or both." ), {
            { "bars", to_translation( "Bars" ) },
            { "descriptions", to_translation( "Descriptions" ) },
            { "both", to_translation( "Both" ) }
        },
        "bars" );

        add( "ITEM_SYMBOLS", page_id, to_translation( "Show item symbols" ),
             to_translation( "If true, show item symbols in inventory and pick up menu." ),
             false
           );

        add( "ITEM_BODYGRAPH", page_id, to_translation( "Show armor coverage map" ),
             to_translation( "If true, show a visual representation of armor coverage in the item info window." ),
             true
           );

        add( "ASTERISK_POSITION", page_id, to_translation( "Favorited item's mark position" ),
             to_translation( "Where to place mark of the favorited item (asterisk): before item's name (prefix) or after item's name (suffix)." ),
        { { "prefix", to_translation( "Prefix" ) }, { "suffix", to_translation( "Suffix" ) } },
        "right"
           );
    } );

    add_empty_line();
    add_option_group( "interface", Group( "mouse_cont_opts", to_translation( "Mouse Control Options" ),
                                          to_translation( "Options regarding mouse control." ) ),
    [&]( const std::string & page_id ) {
        add( "ENABLE_JOYSTICK", page_id, to_translation( "Enable joystick" ),
             to_translation( "If true, enable input from joystick." ),
             true, COPT_CURSES_HIDE
           );

        add( "HIDE_CURSOR", page_id, to_translation( "Hide mouse cursor" ),
             to_translation( "Show: cursor is always shown.  Hide: cursor is hidden.  HideKB: cursor is hidden on keyboard input and unhidden on mouse movement." ),
             //~ show mouse cursor
        {   { "show", to_translation( "Show" ) },
            //~ hide mouse cursor
            { "hide", to_translation( "Hide" ) },
            //~ hide mouse cursor when keyboard is used
            { "hidekb", to_translation( "HideKB" ) }
        },
        "show", COPT_CURSES_HIDE );

        add( "EDGE_SCROLL", page_id, to_translation( "Edge scrolling" ),
        to_translation( "Edge scrolling with the mouse." ), {
            { -1, to_translation( "options", "Disabled" ) },
            { 100, to_translation( "Slow" ) },
            { 30, to_translation( "Normal" ) },
            { 10, to_translation( "Fast" ) },
        },
        30, 30, COPT_CURSES_HIDE );
    } );

}

void options_manager::add_options_graphics()
{
    const auto add_empty_line = [&]() {
        this->add_empty_line( "graphics" );
    };

    add_option_group( "graphics", Group( "anim_opts", to_translation( "Animation Options" ),
                                         to_translation( "Options regarding animations." ) ),
    [&]( const std::string & page_id ) {
        add( "ANIMATIONS", page_id, to_translation( "Animations" ),
             to_translation( "If true, will display enabled animations." ),
             true
           );

        add( "ANIMATION_RAIN", page_id, to_translation( "Rain animation" ),
             to_translation( "If true, will display weather animations." ),
             true
           );

        get_option( "ANIMATION_RAIN" ).setPrerequisite( "ANIMATIONS" );

        add( "ANIMATION_PROJECTILES", page_id, to_translation( "Projectile animation" ),
             to_translation( "If true, will display animations for projectiles like bullets, arrows, and thrown items." ),
             true
           );

        get_option( "ANIMATION_PROJECTILES" ).setPrerequisite( "ANIMATIONS" );

        add( "ANIMATION_SCT", page_id, to_translation( "SCT animation" ),
             to_translation( "If true, will display scrolling combat text animations." ),
             true
           );

        get_option( "ANIMATION_SCT" ).setPrerequisite( "ANIMATIONS" );

        add( "ANIMATION_SCT_USE_FONT", page_id, to_translation( "SCT with Unicode font" ),
             to_translation( "If true, will display scrolling combat text with Unicode font." ),
             true
           );

        get_option( "ANIMATION_SCT_USE_FONT" ).setPrerequisite( "ANIMATION_SCT" );

        add( "ANIMATION_DELAY", page_id, to_translation( "Animation delay" ),
             to_translation( "The amount of time to pause between animation frames in ms." ),
             0, 100, 10
           );

        get_option( "ANIMATION_DELAY" ).setPrerequisite( "ANIMATIONS" );

        add( "BLINK_SPEED", page_id, to_translation( "Blinking effects speed" ),
             to_translation( "The speed of every blinking effects in ms." ),
             100, 5000, 300
           );

        add( "FORCE_REDRAW", page_id, to_translation( "Force redraw" ),
             to_translation( "If true, forces the game to redraw at least once per turn." ),
             true
           );
    } );

    add_empty_line();

    add_option_group( "graphics", Group( "ascii_opts", to_translation( "ASCII Graphic Options" ),
                                         to_translation( "Options regarding ASCII graphic." ) ),
    [&]( const std::string & page_id ) {
        add( "ENABLE_ASCII_TITLE", page_id,
             to_translation( "Enable ASCII art on the title screen" ),
             to_translation( "If true, shows an ASCII graphic on the title screen.  If false, shows a text-only title screen." ),
             true
           );

        add( "SEASONAL_TITLE", page_id, to_translation( "Use seasonal title screen" ),
             to_translation( "If true, the title screen will use the art appropriate for the season." ),
             true
           );

        get_option( "SEASONAL_TITLE" ).setPrerequisite( "ENABLE_ASCII_TITLE" );

        add( "ALT_TITLE", page_id, to_translation( "Alternative title screen frequency" ),
             to_translation( "Set the probability of the alternate title screen appearing." ), 0, 100, 10
           );

        get_option( "ALT_TITLE" ).setPrerequisite( "ENABLE_ASCII_TITLE" );
    } );

    add_empty_line();

    add_option_group( "graphics", Group( "term_opts", to_translation( "Terminal Display Options" ),
                                         to_translation( "Options regarding terminal display." ) ),
    [&]( const std::string & page_id ) {
        add( "TERMINAL_X", page_id, to_translation( "Terminal width" ),
             to_translation( "Set the size of the terminal along the X axis." ),
             80, 960, 80, COPT_POSIX_CURSES_HIDE
           );

        add( "TERMINAL_Y", page_id, to_translation( "Terminal height" ),
             to_translation( "Set the size of the terminal along the Y axis." ),
             24, 270, 24, COPT_POSIX_CURSES_HIDE
           );
    } );

    add_empty_line();

#if defined(TILES)
    add_option_group( "graphics", Group( "font_params", to_translation( "Font settings" ),
                                         to_translation( "Font display settings.  To change font type or source file, edit fonts.json in config directory." ) ),
    [&]( const std::string & page_id ) {
        add( "FONT_BLENDING", page_id, to_translation( "Font blending" ),
             to_translation( "If true, vector fonts may look better." ),
             false, COPT_CURSES_HIDE
           );

        add( "FONT_WIDTH", page_id, to_translation( "Font width" ),
             to_translation( "Set the font width.  Requires restart." ),
             6, 100, 8, COPT_CURSES_HIDE
           );

        add( "FONT_HEIGHT", page_id, to_translation( "Font height" ),
             to_translation( "Set the font height.  Requires restart." ),
             8, 100, 16, COPT_CURSES_HIDE
           );

        add( "FONT_SIZE", page_id, to_translation( "Font size" ),
             to_translation( "Set the font size.  Requires restart." ),
             8, 100, 16, COPT_CURSES_HIDE
           );

        add( "MAP_FONT_WIDTH", page_id, to_translation( "Map font width" ),
             to_translation( "Set the map font width.  Requires restart." ),
             6, 100, 16, COPT_CURSES_HIDE
           );

        add( "MAP_FONT_HEIGHT", page_id, to_translation( "Map font height" ),
             to_translation( "Set the map font height.  Requires restart." ),
             8, 100, 16, COPT_CURSES_HIDE
           );

        add( "MAP_FONT_SIZE", page_id, to_translation( "Map font size" ),
             to_translation( "Set the map font size.  Requires restart." ),
             8, 100, 16, COPT_CURSES_HIDE
           );

        add( "OVERMAP_FONT_WIDTH", page_id, to_translation( "Overmap font width" ),
             to_translation( "Set the overmap font width.  Requires restart." ),
             6, 100, 16, COPT_CURSES_HIDE
           );

        add( "OVERMAP_FONT_HEIGHT", page_id, to_translation( "Overmap font height" ),
             to_translation( "Set the overmap font height.  Requires restart." ),
             8, 100, 16, COPT_CURSES_HIDE
           );

        add( "OVERMAP_FONT_SIZE", page_id, to_translation( "Overmap font size" ),
             to_translation( "Set the overmap font size.  Requires restart." ),
             8, 100, 16, COPT_CURSES_HIDE
           );

        add( "USE_DRAW_ASCII_LINES_ROUTINE", page_id, to_translation( "SDL ASCII lines" ),
             to_translation( "If true, use SDL ASCII line drawing routine instead of Unicode Line Drawing characters.  Use this option when your selected font doesn't contain necessary glyphs." ),
             true, COPT_CURSES_HIDE
           );
    } );
#endif // TILES

    add_empty_line();

    add( "ENABLE_ASCII_ART", "graphics",
         to_translation( "Enable ASCII art in item and monster descriptions" ),
         to_translation( "If true, item and monster description will show a picture of the object in ASCII art when available." ),
         true, COPT_NO_HIDE
       );

    add_empty_line();

    add_option_group( "graphics", Group( "tileset_opts", to_translation( "Tileset Options" ),
                                         to_translation( "Options regarding tileset." ) ),
    [&]( const std::string & page_id ) {
        add( "USE_TILES", page_id, to_translation( "Use tiles" ),
             to_translation( "If true, replaces some TTF rendered text with tiles." ),
             true, COPT_CURSES_HIDE
           );

        add( "TILES", page_id, to_translation( "Choose tileset" ),
             to_translation( "Choose the tileset you want to use." ),
             build_tilesets_list(), "UltimateCataclysm", COPT_CURSES_HIDE
           ); // populate the options dynamically

        add( "USE_DISTANT_TILES", page_id, to_translation( "Use separate tileset for far" ),
             to_translation( "If true, when very zoomed out you will use a separate tileset." ),
             false, COPT_CURSES_HIDE
           );

        add( "DISTANT_TILES", page_id, to_translation( "Choose distant tileset" ),
             to_translation( "Choose the tileset you want to use for far zoom." ),
             build_tilesets_list(), "UltimateCataclysm", COPT_CURSES_HIDE
           ); // populate the options dynamically

        add( "SWAP_ZOOM", page_id, to_translation( "Zoom Threshold" ),
             to_translation( "Choose when you should swap tileset (lower is more zoomed out)." ),
             1, 4, 2, COPT_CURSES_HIDE
           ); // populate the options dynamically

        get_option( "TILES" ).setPrerequisite( "USE_TILES" );
        get_option( "USE_DISTANT_TILES" ).setPrerequisite( "USE_TILES" );
        get_option( "DISTANT_TILES" ).setPrerequisite( "USE_DISTANT_TILES" );
        get_option( "SWAP_ZOOM" ).setPrerequisite( "USE_DISTANT_TILES" );

        add( "USE_OVERMAP_TILES", page_id, to_translation( "Use tiles to display overmap" ),
             to_translation( "If true, replaces some TTF-rendered text with tiles for overmap display." ),
             true, COPT_CURSES_HIDE
           );

        get_option( "USE_OVERMAP_TILES" ).setPrerequisite( "USE_TILES" );

        std::vector<options_manager::id_and_option> om_tilesets = build_tilesets_list();
        // filter out iso tilesets from overmap tilesets
        om_tilesets.erase( std::remove_if( om_tilesets.begin(), om_tilesets.end(), []( const auto & it ) {
            static const std::string iso_suffix = "_iso";
            const std::string &id = it.first;
            return id.size() >= iso_suffix.size() &&
                   id.compare( id.size() - iso_suffix.size(), iso_suffix.size(), iso_suffix ) == 0;
        } ), om_tilesets.end() );

        add( "OVERMAP_TILES", page_id, to_translation( "Choose overmap tileset" ),
             to_translation( "Choose the overmap tileset you want to use." ),
             om_tilesets, "Larwick Overmap", COPT_CURSES_HIDE
           ); // populate the options dynamically

        get_option( "OVERMAP_TILES" ).setPrerequisite( "USE_OVERMAP_TILES" );
    } );

    add_empty_line();

    add_option_group( "graphics", Group( "color_ovl_opts", to_translation( "Color Overlay Options" ),
                                         to_translation( "Options regarding color overlay." ) ),
    [&]( const std::string & page_id ) {
        add( "NV_GREEN_TOGGLE", page_id, to_translation( "Night vision color overlay" ),
             to_translation( "If true, toggle the color overlay from night vision goggles and other similar tools." ),
             true, COPT_CURSES_HIDE
           );

        add( "MEMORY_MAP_MODE", page_id, to_translation( "Memory map overlay preset" ),
        to_translation( "Specify the overlay in which the memory map is drawn.  Requires restart.  For custom overlay, define RGB values for dark and bright colors as well as gamma." ), {
            { "color_pixel_darken", to_translation( "Darkened" ) },
            { "color_pixel_sepia_light", to_translation( "Sepia" ) },
            { "color_pixel_sepia_dark", to_translation( "Sepia Dark" ) },
            { "color_pixel_blue_dark", to_translation( "Blue Dark" ) },
            { "color_pixel_custom", to_translation( "Custom" ) },
        }, "color_pixel_sepia_light", COPT_CURSES_HIDE
           );

        add( "MEMORY_RGB_DARK_RED", page_id, to_translation( "Custom dark color RGB overlay - RED" ),
             to_translation( "Specify RGB value for color RED for dark color overlay." ),
             0, 255, 39, COPT_CURSES_HIDE );

        get_option( "MEMORY_RGB_DARK_RED" ).setPrerequisite( "MEMORY_MAP_MODE", "color_pixel_custom" );

        add( "MEMORY_RGB_DARK_GREEN", page_id,
             to_translation( "Custom dark color RGB overlay - GREEN" ),
             to_translation( "Specify RGB value for color GREEN for dark color overlay." ),
             0, 255, 23, COPT_CURSES_HIDE );

        get_option( "MEMORY_RGB_DARK_GREEN" ).setPrerequisite( "MEMORY_MAP_MODE", "color_pixel_custom" );

        add( "MEMORY_RGB_DARK_BLUE", page_id, to_translation( "Custom dark color RGB overlay - BLUE" ),
             to_translation( "Specify RGB value for color BLUE for dark color overlay." ),
             0, 255, 19, COPT_CURSES_HIDE );

        get_option( "MEMORY_RGB_DARK_BLUE" ).setPrerequisite( "MEMORY_MAP_MODE", "color_pixel_custom" );

        add( "MEMORY_RGB_BRIGHT_RED", page_id,
             to_translation( "Custom bright color RGB overlay - RED" ),
             to_translation( "Specify RGB value for color RED for bright color overlay." ),
             0, 255, 241, COPT_CURSES_HIDE );

        get_option( "MEMORY_RGB_BRIGHT_RED" ).setPrerequisite( "MEMORY_MAP_MODE", "color_pixel_custom" );

        add( "MEMORY_RGB_BRIGHT_GREEN", page_id,
             to_translation( "Custom bright color RGB overlay - GREEN" ),
             to_translation( "Specify RGB value for color GREEN for bright color overlay." ),
             0, 255, 220, COPT_CURSES_HIDE );

        get_option( "MEMORY_RGB_BRIGHT_GREEN" ).setPrerequisite( "MEMORY_MAP_MODE", "color_pixel_custom" );

        add( "MEMORY_RGB_BRIGHT_BLUE", page_id,
             to_translation( "Custom bright color RGB overlay - BLUE" ),
             to_translation( "Specify RGB value for color BLUE for bright color overlay." ),
             0, 255, 163, COPT_CURSES_HIDE );

        get_option( "MEMORY_RGB_BRIGHT_BLUE" ).setPrerequisite( "MEMORY_MAP_MODE", "color_pixel_custom" );

        add( "MEMORY_GAMMA", page_id, to_translation( "Custom gamma for overlay" ),
             to_translation( "Specify gamma value for overlay." ),
             1.0f, 3.0f, 1.6f, 0.1f, COPT_CURSES_HIDE );

        get_option( "MEMORY_GAMMA" ).setPrerequisite( "MEMORY_MAP_MODE", "color_pixel_custom" );
    } );
    add_empty_line();

    add_option_group( "graphics", Group( "pix_minimap_opts", to_translation( "Pixel Minimap Options" ),
                                         to_translation( "Options regarding pixel minimap." ) ),
    [&]( const std::string & page_id ) {
        add( "PIXEL_MINIMAP", page_id, to_translation( "Pixel minimap" ),
             to_translation( "If true, shows the pixel-detail minimap in game after the save is loaded.  Use the 'Toggle Pixel Minimap' action key to change its visibility during gameplay." ),
             true, COPT_CURSES_HIDE
           );

        add( "PIXEL_MINIMAP_MODE", page_id, to_translation( "Pixel minimap drawing mode" ),
        to_translation( "Specified the mode in which the minimap drawn." ), {
            { "solid", to_translation( "Solid" ) },
            { "squares", to_translation( "Squares" ) },
            { "dots", to_translation( "Dots" ) }
        }, "dots", COPT_CURSES_HIDE
           );

        get_option( "PIXEL_MINIMAP_MODE" ).setPrerequisite( "PIXEL_MINIMAP" );

        add( "PIXEL_MINIMAP_BRIGHTNESS", page_id, to_translation( "Pixel minimap brightness" ),
             to_translation( "Overall brightness of pixel-detail minimap." ),
             10, 300, 100, COPT_CURSES_HIDE
           );

        get_option( "PIXEL_MINIMAP_BRIGHTNESS" ).setPrerequisite( "PIXEL_MINIMAP" );

        add( "PIXEL_MINIMAP_HEIGHT", page_id, to_translation( "Pixel minimap height" ),
             to_translation( "Height of pixel-detail minimap, measured in terminal rows.  Set to 0 for default spacing." ),
             0, 100, 0, COPT_CURSES_HIDE
           );

        get_option( "PIXEL_MINIMAP_HEIGHT" ).setPrerequisite( "PIXEL_MINIMAP" );

        add( "PIXEL_MINIMAP_SCALE_TO_FIT", page_id, to_translation( "Scale pixel minimap" ),
             to_translation( "If true, scale pixel minimap to fit its surroundings.  May produce crappy results, especially in modes other than \"Solid\"." ),
             false, COPT_CURSES_HIDE
           );

        get_option( "PIXEL_MINIMAP_SCALE_TO_FIT" ).setPrerequisite( "PIXEL_MINIMAP" );

        add( "PIXEL_MINIMAP_RATIO", page_id, to_translation( "Maintain pixel minimap aspect ratio" ),
             to_translation( "If true, preserves the square shape of tiles shown on the pixel minimap." ),
             true, COPT_CURSES_HIDE
           );

        get_option( "PIXEL_MINIMAP_RATIO" ).setPrerequisite( "PIXEL_MINIMAP" );

        add( "PIXEL_MINIMAP_BEACON_SIZE", page_id,
             to_translation( "Creature beacon size" ),
             to_translation( "Controls how big the creature beacons are.  Value is in minimap tiles." ),
             1, 4, 2, COPT_CURSES_HIDE
           );

        get_option( "PIXEL_MINIMAP_BEACON_SIZE" ).setPrerequisite( "PIXEL_MINIMAP" );

        add( "PIXEL_MINIMAP_BLINK", page_id, to_translation( "Hostile creature beacon blink speed" ),
             to_translation( "Controls how fast the hostile creature beacons blink on the pixel minimap.  Value is multiplied by 200ms.  0 = disabled." ),
             0, 50, 10, COPT_CURSES_HIDE
           );

        get_option( "PIXEL_MINIMAP_BLINK" ).setPrerequisite( "PIXEL_MINIMAP" );

        add( "PIXEL_MINIMAP_BG", page_id, to_translation( "Background color" ),
             to_translation( "What color the minimap background should be.  Either based on color theme or (0,0,0) black." ),
        {
            { "theme", to_translation( "Theme" ) },
            { "black", to_translation( "Black" ) }
        }, "black", COPT_CURSES_HIDE
           );

        get_option( "PIXEL_MINIMAP_BLINK" ).setPrerequisite( "PIXEL_MINIMAP" );
    } );

    add_empty_line();

    add_option_group( "graphics", Group( "graph_display_opts",
                                         to_translation( "Graphical Display Options" ),
                                         to_translation( "Options regarding graphical display." ) ),
    [&]( const std::string & page_id ) {
#if defined(TILES)
        std::vector<options_manager::id_and_option> display_list = cata_tiles::build_display_list();
        add( "DISPLAY", page_id, to_translation( "Display" ),
             to_translation( "Sets which video display will be used to show the game.  Requires restart." ),
             display_list,
             display_list.front().first, COPT_CURSES_HIDE );
#endif

#if !defined(__ANDROID__) // Android is always fullscreen
        add( "FULLSCREEN", page_id, to_translation( "Fullscreen" ),
             to_translation( "Starts Cataclysm in one of the fullscreen modes.  Requires restart." ),
        { { "no", to_translation( "No" ) }, { "maximized", to_translation( "Maximized" ) }, { "fullscreen", to_translation( "Fullscreen" ) }, { "windowedbl", to_translation( "Windowed borderless" ) } },
        // Borderless window is bad for debugging in Visual Studio
#if defined(_MSC_VER)
        "maximized", COPT_CURSES_HIDE
#else
        "windowedbl", COPT_CURSES_HIDE
#endif
           );
#endif

#if !defined(__ANDROID__)
#   if !defined(TILES)
        // No renderer selection in non-TILES mode
        add( "RENDERER", page_id, to_translation( "Renderer" ),
        to_translation( "Set which renderer to use.  Requires restart." ), { { "software", to_translation( "software" ) } },
        "software", COPT_CURSES_HIDE );
#   else
        std::vector<options_manager::id_and_option> renderer_list = cata_tiles::build_renderer_list();
        std::string default_renderer = renderer_list.front().first;
#   if defined(_WIN32)
        for( const id_and_option &renderer : renderer_list ) {
            if( renderer.first == "direct3d11" ) {
                default_renderer = renderer.first;
                break;
            }
        }
#   endif
        add( "RENDERER", page_id, to_translation( "Renderer" ),
             to_translation( "Set which renderer to use.  Requires restart." ), renderer_list,
             default_renderer, COPT_CURSES_HIDE );
#   endif

#else
        add( "SOFTWARE_RENDERING", page_id, to_translation( "Software rendering" ),
             to_translation( "Use software renderer instead of graphics card acceleration.  Requires restart." ),
             // take default setting from pre-game settings screen - important as both software + hardware rendering have issues with specific devices
             android_get_default_setting( "Software rendering", false ),
             COPT_CURSES_HIDE
           );
#endif

#if defined(SDL_HINT_RENDER_BATCHING)
        add( "RENDER_BATCHING", page_id, to_translation( "Allow render batching" ),
             to_translation( "If true, use render batching for 2D render API to make it more efficient.  Requires restart." ),
             true, COPT_CURSES_HIDE
           );
#endif
        add( "FRAMEBUFFER_ACCEL", page_id, to_translation( "Software framebuffer acceleration" ),
             to_translation( "If true, use hardware acceleration for the framebuffer when using software rendering.  Requires restart." ),
             false, COPT_CURSES_HIDE
           );

#if defined(__ANDROID__)
        get_option( "FRAMEBUFFER_ACCEL" ).setPrerequisite( "SOFTWARE_RENDERING" );
#else
        get_option( "FRAMEBUFFER_ACCEL" ).setPrerequisite( "RENDERER", "software" );
#endif

        add( "USE_COLOR_MODULATED_TEXTURES", page_id, to_translation( "Use color modulated textures" ),
             to_translation( "If true, tries to use color modulated textures to speed-up ASCII drawing.  Requires restart." ),
             false, COPT_CURSES_HIDE
           );

        add( "SCALING_MODE", page_id, to_translation( "Scaling mode" ),
             to_translation( "Sets the scaling mode, 'none' (default) displays at the game's native resolution, 'nearest' uses low-quality but fast scaling, and 'linear' provides high-quality scaling." ),
             //~ Do not scale the game image to the window size.
        {   { "none", to_translation( "No scaling" ) },
            //~ An algorithm for image scaling.
            { "nearest", to_translation( "Nearest neighbor" ) },
            //~ An algorithm for image scaling.
            { "linear", to_translation( "Linear filtering" ) }
        },
        "none", COPT_CURSES_HIDE );

#if !defined(__ANDROID__)
        add( "SCALING_FACTOR", page_id, to_translation( "Scaling factor" ),
        to_translation( "Factor by which to scale the display.  Requires restart." ), {
            { "1", to_translation( "1x" )},
            { "2", to_translation( "2x" )},
            { "4", to_translation( "4x" )}
        },
        "1", COPT_CURSES_HIDE );
#endif
    } );

}

void options_manager::add_options_world_default()
{
    const auto add_empty_line = [&]() {
        this->add_empty_line( "world_default" );
    };

    add( "WORLD_END", "world_default", to_translation( "World end handling" ),
    to_translation( "Handling of game world when last character dies." ), {
        { "reset", to_translation( "Reset" ) }, { "delete", to_translation( "Delete" ) },
        { "query", to_translation( "Query" ) }, { "keep", to_translation( "Keep" ) }
    }, "reset"
       );

    add_empty_line();

    add_option_group( "world_default", Group( "game_world_opts", to_translation( "Game World Options" ),
                      to_translation( "Options regarding game world." ) ),
    [&]( const std::string & page_id ) {
        add( "CITY_SIZE", page_id, to_translation( "Size of cities" ),
             to_translation( "A number determining how large cities are.  A higher number means larger cities.  0 disables cities, roads and any scenario requiring a city start." ),
             0, 16, 8
           );

        add( "CITY_SPACING", page_id, to_translation( "City spacing" ),
             to_translation( "A number determining how far apart cities are.  A higher number means cities are further apart.  Warning, small numbers lead to very slow mapgen." ),
             0, 8, 4
           );

        add( "SPAWN_DENSITY", page_id, to_translation( "Spawn rate scaling factor" ),
             to_translation( "A scaling factor that determines density of monster spawns.  A higher number means more monsters." ),
             0.0, 50.0, 1.0, 0.1
           );

        add( "ITEM_SPAWNRATE", page_id, to_translation( "Item spawn scaling factor" ),
             to_translation( "A scaling factor that determines density of item spawns.  A higher number means more items." ),
             0.01, 10.0, 1.0, 0.01
           );

        add( "NPC_SPAWNTIME", page_id, to_translation( "Random NPC spawn time" ),
             to_translation( "Baseline average number of days between random NPC spawns.  Average duration goes up with the number of NPCs already spawned.  A higher number means fewer NPCs.  Set to 0 days to disable random NPCs." ),
             0.0, 100.0, 4.0, 0.01
           );

        add( "MONSTER_UPGRADE_FACTOR", page_id,
             to_translation( "Monster evolution slowdown" ),
             to_translation( "A scaling factor that determines the time between monster upgrades.  A higher number means slower evolution.  Set to 0.00 to turn off monster upgrades." ),
             0.0, 100, 4.0, 0.01
           );
    } );
    add_empty_line();

    add_option_group( "world_default", Group( "monster_props_opts",
                      to_translation( "Monster Properties Options" ),
                      to_translation( "Options regarding monster properties." ) ),
    [&]( const std::string & page_id ) {
        add( "MONSTER_SPEED", page_id, to_translation( "Monster speed" ),
             to_translation( "Determines the movement rate of monsters.  A higher value increases monster speed and a lower reduces it.  Requires world reset." ),
             1, 1000, 100, COPT_NO_HIDE, "%i%%"
           );

        add( "MONSTER_RESILIENCE", page_id, to_translation( "Monster resilience" ),
             to_translation( "Determines how much damage monsters can take.  A higher value makes monsters more resilient and a lower makes them more flimsy.  Requires world reset." ),
             1, 1000, 100, COPT_NO_HIDE, "%i%%"
           );
    } );

    add_empty_line();

    add( "DEFAULT_REGION", "world_default", to_translation( "Default region type" ),
         to_translation( "(WIP feature) Determines terrain, shops, plants, and more." ),
    { { "default", to_translation( "default" ) } }, "default"
       );

    add_empty_line();

    add_option_group( "world_default", Group( "spawn_time_opts", to_translation( "Spawn Time Options" ),
                      to_translation( "Options regarding spawn time." ) ),
    [&]( const std::string & page_id ) {
        add( "SEASON_LENGTH", page_id, to_translation( "Season length" ),
             to_translation( "Season length, in days.  Warning: Very little other than the duration of seasons scales with this value, so adjusting it may cause nonsensical results." ),
             14, 127, 91
           );

        add( "CONSTRUCTION_SCALING", page_id, to_translation( "Construction scaling" ),
             to_translation( "Sets the time of construction in percents.  '50' is two times faster than default, '200' is two times longer.  '0' automatically scales construction time to match the world's season length." ),
             0, 1000, 100
           );

        add( "ETERNAL_SEASON", page_id, to_translation( "Eternal season" ),
             to_translation( "If true, keep the initial season for ever." ),
             false
           );

        add( "ETERNAL_TIME_OF_DAY", page_id, to_translation( "Day/night cycle" ),
        to_translation( "Day/night cycle settings.  'Normal' sets a normal cycle.  'Eternal Day' sets eternal day.  'Eternal Night' sets eternal night." ), {
            { "normal", to_translation( "Normal" ) },
            { "day", to_translation( "Eternal Day" ) },
            { "night", to_translation( "Eternal Night" ) },
        }, "normal"
           );
    } );

    add_empty_line();

    add_option_group( "world_default", Group( "misc_worlddef_opts", to_translation( "Misc Options" ),
                      to_translation( "Miscellaneous options." ) ),
    [&]( const std::string & page_id ) {
        add( "WANDER_SPAWNS", page_id, to_translation( "Wandering hordes" ),
             to_translation( "If true, emulates zombie hordes.  Zombies can group together into hordes, which can wander around cities and will sometimes move towards noise.  Note: the current implementation does not properly respect obstacles, so hordes can appear to walk through walls under some circumstances.  Must reset world directory after changing for it to take effect." ),
             false
           );

        add( "BLACK_ROAD", page_id, to_translation( "Surrounded start" ),
             to_translation( "If true, spawn zombies at shelters.  Makes the starting game a lot harder." ),
             false
           );
    } );

    add_empty_line();

    add( "RAD_MUTATION", "world_default", to_translation( "Mutations by radiation" ),
         to_translation( "If true, radiation causes the player to mutate." ),
         true
       );

    add_empty_line();

    add( "CHARACTER_POINT_POOLS", "world_default", to_translation( "Character point pools" ),
         to_translation( "Allowed point pools for character generation." ),
    { { "any", to_translation( "Any" ) }, { "multi_pool", to_translation( "Legacy Multipool" ) }, { "story_teller", to_translation( "Survivor" ) } },
    "story_teller"
       );

    add_empty_line();

    add( "META_PROGRESS", "world_default", to_translation( "Meta Progression" ), to_translation(
             "Will you need to complete certain achievements to enable certain scenarios "
             "and professions?  Achievements of saved characters from any world will be "
             "checked.  Disabling this will spoil factions and situations you may otherwise "
             "stumble upon naturally while playing.  Some scenarios are frustrating for the "
             "uninitiated, and some professions skip portions of the game's content.  If "
             "new to the game, meta progression will help you be introduced to mechanics at "
             "a reasonable pace." ),
         true
       );
}

void options_manager::add_options_debug()
{
    const auto add_empty_line = [&]() {
        this->add_empty_line( "debug" );
    };

    add( "DISTANCE_INITIAL_VISIBILITY", "debug", to_translation( "Distance initial visibility" ),
         to_translation( "Determines the scope, which is known in the beginning of the game." ),
         3, 20, 15
       );

    add_empty_line();

    add_option_group( "debug", Group( "chargen_point_opts",
                                      to_translation( "Character Generation Points Options" ),
                                      to_translation( "Options regarding character generation points." ) ),
    [&]( const std::string & page_id ) {
        add( "INITIAL_STAT_POINTS", page_id, to_translation( "Initial stat points" ),
             to_translation( "Initial points available to spend on stats on character generation." ),
             0, 1000, 6
           );

        add( "INITIAL_TRAIT_POINTS", page_id, to_translation( "Initial trait points" ),
             to_translation( "Initial points available to spend on traits on character generation." ),
             0, 1000, 0
           );

        add( "INITIAL_SKILL_POINTS", page_id, to_translation( "Initial skill points" ),
             to_translation( "Initial points available to spend on skills on character generation." ),
             0, 1000, 2
           );

        add( "MAX_TRAIT_POINTS", page_id, to_translation( "Maximum trait points" ),
             to_translation( "Maximum trait points available for character generation." ),
             0, 1000, 12
           );
    } );

    add_empty_line();

    add( "DEBUG_DIFFICULTIES", "debug", to_translation( "Show values for character creation" ),
         to_translation( "In character creation will show the underlying value that is used to determine difficulty." ),
         false
       );

    add_empty_line();

    add( "SKILL_TRAINING_SPEED", "debug", to_translation( "Skill training speed" ),
         to_translation( "Scales experience gained from practicing skills and reading books.  0.5 is half as fast as default, 2.0 is twice as fast, 0.0 disables skill training except for NPC training." ),
         0.0, 100.0, 1.0, 0.1
       );

    add_empty_line();

    add( "PROFICIENCY_TRAINING_SPEED", "debug", to_translation( "Proficiency training speed" ),
         to_translation( "Scales experience gained from practicing proficiencies.  0.5 is half as fast as default, 2.0 is twice as fast, 0.0 disables proficiency training except for NPC training." ),
         0.0, 100.0, 1.0, 0.1
       );

    add_empty_line();

    add( "FOV_3D_Z_RANGE", "debug", to_translation( "Vertical range of 3D field of vision" ),
         to_translation(
             "How many levels up and down the 3D field of vision reaches.  (This many levels up, this many levels down.)  "
             "3D vision of the full height of the world can slow the game down a lot.  Seeing fewer Z-levels is faster.  "
             "Setting this to 0 disables vertical vision.  In tiles mode this also affects how many levels up and down are "
             "drawn on screen, and setting this to 0 displays only one level below with colored blocks instead." ),
         0, OVERMAP_LAYERS, 4
       );

    add_empty_line();

    add_option_group( "debug", Group( "occlusion_opts", to_translation( "Occlusion Options" ),
                                      to_translation( "Options regarding occlusion." ) ),
    [&]( const std::string & page_id ) {
        add( "PREVENT_OCCLUSION", page_id, to_translation( "Handle occlusion by high sprites" ),
        to_translation( "Draw walls normal (Off), retracted/transparent (On), or automatically retracting/transparent near player (Auto)." ), {
            { 0, to_translation( "Off" ) }, { 1, to_translation( "On" ) },
            { 2, to_translation( "Auto" ) }
        }, 2, 2
           );

        add( "PREVENT_OCCLUSION_TRANSP", page_id, to_translation( "Prevent occlusion via transparency" ),
             to_translation( "Prevent occlusion by using semi-transparent sprites." ),
             true
           );

        add( "PREVENT_OCCLUSION_RETRACT", page_id, to_translation( "Prevent occlusion via retraction" ),
             to_translation( "Prevent occlusion by retracting high sprites." ),
             true
           );

        add( "PREVENT_OCCLUSION_MIN_DIST", page_id,
             to_translation( "Minimum distance for auto occlusion handling" ),
             to_translation( "Minimum distance for auto occlusion handling.  Values above zero overwrite tileset settings." ),
             0.0, 60.0, 0.0, 0.1
           );

        add( "PREVENT_OCCLUSION_MAX_DIST", page_id,
             to_translation( "Maximum distance for auto occlusion handling" ),
             to_translation( "Maximum distance for auto occlusion handling.  Values above zero overwrite tileset settings." ),
             0.0, 60.0, 0.0, 0.1
           );
    } );

    add_empty_line();

    add( "SKIP_VERIFICATION", "debug", to_translation( "Skip verification step during loading" ),
         to_translation( "If enabled, this skips the JSON verification step during loading.  This may give a faster loading time, but risks JSON errors not being caught until runtime." ),
#if defined(EMSCRIPTEN)
         true
#else
         false
#endif
       );
}

void options_manager::add_options_android()
{
#if defined(__ANDROID__)
    const auto add_empty_line = [&]() {
        this->add_empty_line( "android" );
    };

    add( "ANDROID_QUICKSAVE", "android", to_translation( "Quicksave on app lose focus" ),
         to_translation( "If true, quicksave whenever the app loses focus (screen locked, app moved into background etc.)  WARNING: Experimental.  This may result in corrupt save games." ),
         false
       );

    add_empty_line();

    add_option_group( "android", Group( "android_keyboard_opts",
                                        to_translation( "Android Keyboard Options" ),
                                        to_translation( "Options regarding Android keyboard." ) ),
    [&]( const std::string & page_id ) {
        add( "ANDROID_TRAP_BACK_BUTTON", page_id, to_translation( "Trap Back button" ),
             to_translation( "If true, the back button will NOT back out of the app and will be passed to the application as SDL_SCANCODE_AC_BACK.  Requires restart." ),
             // take default setting from pre-game settings screen - important as there are issues with Back button on Android 9 with specific devices
             android_get_default_setting( "Trap Back button", true )
           );

        add( "ANDROID_NATIVE_UI", page_id, to_translation( "Use native Android UI menus" ),
             to_translation( "If true, native Android dialogs are used for some in-game menus, "
                             "such as popup messages and yes/no dialogs." ),
             android_get_default_setting( "Native Android UI", true )
           );

        add( "ANDROID_AUTO_KEYBOARD", page_id, to_translation( "Auto-manage virtual keyboard" ),
             to_translation( "If true, automatically show/hide the virtual keyboard when necessary based on context.  If false, virtual keyboard must be toggled manually." ),
             true
           );

        add( "ANDROID_KEYBOARD_SCREEN_SCALE", page_id,
             to_translation( "Virtual keyboard screen scale" ),
             to_translation( "If true, when the virtual keyboard is visible, scale the screen to prevent overlapping.  Useful for text entry so you can see what you're typing." ),
             true
           );
    } );

    add_empty_line();

    add( "ANDROID_VIBRATION", "android", to_translation( "Vibration duration" ),
         to_translation( "If non-zero, vibrate the device for this long on input, in milliseconds.  Ignored if hardware keyboard connected." ),
         0, 200, 10
       );

    add_empty_line();

    add_option_group( "android", Group( "joystick_android_opts",
                                        to_translation( "Android Joystick Options" ),
                                        to_translation( "Options regarding Android Joystick." ) ),
    [&]( const std::string & page_id ) {
        add( "ANDROID_SHOW_VIRTUAL_JOYSTICK", page_id, to_translation( "Show virtual joystick" ),
             to_translation( "If true, show the virtual joystick when touching and holding the screen.  Gives a visual indicator of deadzone and stick deflection." ),
             true
           );

        add( "ANDROID_VIRTUAL_JOYSTICK_OPACITY", page_id, to_translation( "Virtual joystick opacity" ),
             to_translation( "The opacity of the on-screen virtual joystick, as a percentage." ),
             0, 100, 20
           );

        add( "ANDROID_DEADZONE_RANGE", page_id, to_translation( "Virtual joystick deadzone size" ),
             to_translation( "While using the virtual joystick, deflecting the stick beyond this distance will trigger directional input.  Specified as a percentage of longest screen edge." ),
             0.01f, 0.2f, 0.03f, 0.001f, COPT_NO_HIDE, "%.3f"
           );

        add( "ANDROID_REPEAT_DELAY_RANGE", page_id, to_translation( "Virtual joystick size" ),
             to_translation( "While using the virtual joystick, deflecting the stick by this much will repeat input at the deflected rate (see below).  Specified as a percentage of longest screen edge." ),
             0.05f, 0.5f, 0.10f, 0.001f, COPT_NO_HIDE, "%.3f"
           );

        add( "ANDROID_VIRTUAL_JOYSTICK_FOLLOW", page_id,
             to_translation( "Virtual joystick follows finger" ),
             to_translation( "If true, the virtual joystick will follow when sliding beyond its range." ),
             false
           );

        add( "ANDROID_REPEAT_DELAY_MAX", page_id,
             to_translation( "Virtual joystick repeat rate (centered)" ),
             to_translation( "When the virtual joystick is centered, how fast should input events repeat, in milliseconds." ),
             50, 1000, 500
           );

        add( "ANDROID_REPEAT_DELAY_MIN", page_id,
             to_translation( "Virtual joystick repeat rate (deflected)" ),
             to_translation( "When the virtual joystick is fully deflected, how fast should input events repeat, in milliseconds." ),
             50, 1000, 100
           );

        add( "ANDROID_SENSITIVITY_POWER", page_id,
             to_translation( "Virtual joystick repeat rate sensitivity" ),
             to_translation( "As the virtual joystick moves from centered to fully deflected, this value is an exponent that controls the blend between the two repeat rates defined above.  1.0 = linear." ),
             0.1f, 5.0f, 0.75f, 0.05f, COPT_NO_HIDE, "%.2f"
           );

        add( "ANDROID_INITIAL_DELAY", page_id, to_translation( "Input repeat delay" ),
             to_translation( "While touching the screen, wait this long before showing the virtual joystick and repeating input, in milliseconds.  Also used to determine tap/double-tap detection, flick detection and toggling quick shortcuts." ),
             150, 1000, 300
           );

        add( "ANDROID_HIDE_HOLDS", page_id, to_translation( "Virtual joystick hides shortcuts" ),
             to_translation( "If true, hides on-screen keyboard shortcuts while using the virtual joystick.  Helps keep the view uncluttered while traveling long distances and navigating menus." ),
             true
           );
    } );

    add_empty_line();

    add_option_group( "android", Group( "shortcut_android_opts",
                                        to_translation( "Android Shortcut Options" ),
                                        to_translation( "Options regarding Android shortcut." ) ),
    [&]( const std::string & page_id ) {
        add( "ANDROID_SHORTCUT_DEFAULTS", page_id, to_translation( "Default gameplay shortcuts" ),
             to_translation( "The default set of gameplay shortcuts to show.  Used on starting a new game and whenever all gameplay shortcuts are removed." ),
             "0mi", 30
           );

        add( "ANDROID_ACTIONMENU_AUTOADD", page_id,
             to_translation( "Add shortcuts for action menu selections" ),
             to_translation( "If true, automatically add a shortcut for actions selected via the in-game action menu." ),
             true
           );

        add( "ANDROID_INVENTORY_AUTOADD", page_id,
             to_translation( "Add shortcuts for inventory selections" ),
             to_translation( "If true, automatically add a shortcut for items selected via the inventory." ),
             true
           );
    } );

    add_empty_line();

    add_option_group( "android", Group( "gestures_android_opts",
                                        to_translation( "Android Gestures Options" ),
                                        to_translation( "Options regarding Android gestures." ) ),
    [&]( const std::string & page_id ) {
        add( "ANDROID_TAP_KEY", page_id, to_translation( "Tap key (in-game)" ),
             to_translation( "The key to press when tapping during gameplay." ),
             ".", 1
           );

        add( "ANDROID_2_TAP_KEY", page_id, to_translation( "Two-finger tap key (in-game)" ),
             to_translation( "The key to press when tapping with two fingers during gameplay." ),
             "i", 1
           );

        add( "ANDROID_2_SWIPE_UP_KEY", page_id, to_translation( "Two-finger swipe up key (in-game)" ),
             to_translation( "The key to press when swiping up with two fingers during gameplay." ),
             "K", 1
           );

        add( "ANDROID_2_SWIPE_DOWN_KEY", page_id,
             to_translation( "Two-finger swipe down key (in-game)" ),
             to_translation( "The key to press when swiping down with two fingers during gameplay." ),
             "J", 1
           );

        add( "ANDROID_2_SWIPE_LEFT_KEY", page_id,
             to_translation( "Two-finger swipe left key (in-game)" ),
             to_translation( "The key to press when swiping left with two fingers during gameplay." ),
             "L", 1
           );

        add( "ANDROID_2_SWIPE_RIGHT_KEY", page_id,
             to_translation( "Two-finger swipe right key (in-game)" ),
             to_translation( "The key to press when swiping right with two fingers during gameplay." ),
             "H", 1
           );

        add( "ANDROID_PINCH_IN_KEY", page_id, to_translation( "Pinch in key (in-game)" ),
             to_translation( "The key to press when pinching in during gameplay." ),
             "Z", 1
           );

        add( "ANDROID_PINCH_OUT_KEY", page_id, to_translation( "Pinch out key (in-game)" ),
             to_translation( "The key to press when pinching out during gameplay." ),
             "z", 1
           );
    } );

    add_empty_line();

    add_option_group( "android", Group( "shortcut_android_in_game_opts",
                                        to_translation( "Android Shortcut Options" ),
                                        to_translation( "Options regarding In-game shortcut." ) ),
    [&]( const std::string & page_id ) {
        add( "ANDROID_SHORTCUT_AUTOADD", page_id,
             to_translation( "Auto-manage contextual gameplay shortcuts" ),
             to_translation( "If true, contextual in-game shortcuts are added and removed automatically as needed: examine, close, butcher, move up/down, control vehicle, pickup, toggle enemy + safe mode, sleep." ),
             true
           );

        add( "ANDROID_SHORTCUT_AUTOADD_FRONT", page_id,
             to_translation( "Move contextual gameplay shortcuts to front" ),
             to_translation( "If the option above is enabled, specifies whether contextual in-game shortcuts will be added to the front or back of the shortcuts list.  If true, makes them easier to reach.  If false, reduces shuffling of shortcut positions." ),
             false
           );

        add( "ANDROID_SHORTCUT_MOVE_FRONT", page_id, to_translation( "Move used shortcuts to front" ),
             to_translation( "If true, using an existing shortcut will always move it to the front of the shortcuts list.  If false, only shortcuts typed via keyboard will move to the front." ),
             false
           );

        add( "ANDROID_SHORTCUT_ZONE", page_id,
             to_translation( "Separate shortcuts for No Auto Pickup zones" ),
             to_translation( "If true, separate gameplay shortcuts will be used within No Auto Pickup zones.  Useful for keeping home base actions separate from exploring actions." ),
             true
           );

        add( "ANDROID_SHORTCUT_REMOVE_TURNS", page_id,
             to_translation( "Turns to remove unused gameplay shortcuts" ),
             to_translation( "If non-zero, unused gameplay shortcuts will be removed after this many turns (as in discrete player actions, not world calendar turns)." ),
             0, 1000, 0
           );

        add( "ANDROID_SHORTCUT_PERSISTENCE", page_id, to_translation( "Shortcuts persistence" ),
             to_translation( "If true, shortcuts are saved/restored with each save game.  If false, shortcuts reset between sessions." ),
             true
           );
    } );

    add_empty_line();

    add_option_group( "android", Group( "shortc_screen_opts",
                                        to_translation( "Shortcut Screen Options" ),
                                        to_translation( "Options regarding shortcut screen." ) ),
    [&]( const std::string & page_id ) {
        add( "ANDROID_SHORTCUT_POSITION", page_id, to_translation( "Shortcuts position" ),
             to_translation( "Switch between shortcuts on the left or on the right side of the screen." ),
        { { "left", to_translation( "Left" ) }, { "right", to_translation( "Right" ) } }, "left"
           );

        add( "ANDROID_SHORTCUT_SCREEN_PERCENTAGE", page_id,
             to_translation( "Shortcuts screen percentage" ),
             to_translation( "How much of the screen can shortcuts occupy, as a percentage of total screen width." ),
             10, 100, 100
           );

        add( "ANDROID_SHORTCUT_OVERLAP", page_id, to_translation( "Shortcuts overlap screen" ),
             to_translation( "If true, shortcuts will be drawn transparently overlapping the game screen.  If false, the game screen size will be reduced to fit the shortcuts below." ),
             true
           );

        add( "ANDROID_SHORTCUT_OPACITY_BG", page_id, to_translation( "Shortcut opacity (background)" ),
             to_translation( "The background opacity of on-screen keyboard shortcuts, as a percentage." ),
             0, 100, 75
           );

        add( "ANDROID_SHORTCUT_OPACITY_SHADOW", page_id, to_translation( "Shortcut opacity (shadow)" ),
             to_translation( "The shadow opacity of on-screen keyboard shortcuts, as a percentage." ),
             0, 100, 100
           );

        add( "ANDROID_SHORTCUT_OPACITY_FG", page_id, to_translation( "Shortcut opacity (text)" ),
             to_translation( "The foreground opacity of on-screen keyboard shortcuts, as a percentage." ),
             0, 100, 100
           );

        add( "ANDROID_SHORTCUT_COLOR", page_id, to_translation( "Shortcut color" ),
             to_translation( "The color of on-screen keyboard shortcuts." ),
             0, 15, 15
           );

        add( "ANDROID_SHORTCUT_BORDER", page_id, to_translation( "Shortcut border" ),
             to_translation( "The border of each on-screen keyboard shortcut in pixels." ),
             0, 16, 0
           );

        add( "ANDROID_SHORTCUT_WIDTH_MIN", page_id, to_translation( "Shortcut width (min)" ),
             to_translation( "The minimum width of each on-screen keyboard shortcut in pixels.  Only relevant when lots of shortcuts are visible at once." ),
             20, 1000, 50
           );

        add( "ANDROID_SHORTCUT_WIDTH_MAX", page_id, to_translation( "Shortcut width (max)" ),
             to_translation( "The maximum width of each on-screen keyboard shortcut in pixels." ),
             50, 1000, 160
           );

        add( "ANDROID_SHORTCUT_HEIGHT", page_id, to_translation( "Shortcut height" ),
             to_translation( "The height of each on-screen keyboard shortcut in pixels." ),
             50, 1000, 130
           );
    } );

#endif
}

#if defined(TILES)
// Helper method to isolate #ifdeffed tiles code.
static void refresh_tiles( bool used_tiles_changed, bool pixel_minimap_height_changed, bool ingame )
{
    if( used_tiles_changed ) {
        // Disable UIs below to avoid accessing the tile context during loading.
        ui_adaptor dummy( ui_adaptor::disable_uis_below {} );
        //try and keep SDL calls limited to source files that deal specifically with them
        try {
            closetilecontext->reinit();
            closetilecontext->load_tileset( get_option<std::string>( "TILES" ),
                                            /*precheck=*/false, /*force=*/false,
                                            /*pump_events=*/true );
            //game_ui::init_ui is called when zoom is changed
            g->reset_zoom();
            g->mark_main_ui_adaptor_resize();
            closetilecontext->do_tile_loading_report();
        } catch( const std::exception &err ) {
            popup( _( "Loading the tileset failed: %s" ), err.what() );
            use_tiles = false;
            use_tiles_overmap = false;
        }
        if( use_far_tiles ) {
            try {
                if( fartilecontext->is_valid() ) {
                    fartilecontext->reinit();
                }
                fartilecontext->load_tileset( get_option<std::string>( "DISTANT_TILES" ),
                                              /*precheck=*/false, /*force=*/false,
                                              /*pump_events=*/true );
                //game_ui::init_ui is called when zoom is changed
                g->reset_zoom();
                g->mark_main_ui_adaptor_resize();
                fartilecontext->do_tile_loading_report();
            } catch( const std::exception &err ) {
                popup( _( "Loading the far tileset failed: %s" ), err.what() );
                use_tiles = false;
                use_tiles_overmap = false;
            }
        }
        try {
            overmap_tilecontext->reinit();
            overmap_tilecontext->load_tileset( get_option<std::string>( "OVERMAP_TILES" ),
                                               /*precheck=*/false, /*force=*/false,
                                               /*pump_events=*/true );
            //game_ui::init_ui is called when zoom is changed
            g->reset_zoom();
            g->mark_main_ui_adaptor_resize();
            overmap_tilecontext->do_tile_loading_report();
        } catch( const std::exception &err ) {
            popup( _( "Loading the overmap tileset failed: %s" ), err.what() );
            use_tiles = false;
            use_tiles_overmap = false;
        }
    } else if( ingame && pixel_minimap_option && pixel_minimap_height_changed ) {
        g->mark_main_ui_adaptor_resize();
    }
}
#else
static void refresh_tiles( bool, bool, bool )
{
}
#endif // TILES

static void draw_borders_external(
    const catacurses::window &w, int horizontal_level, const std::set<int> &vert_lines,
    const bool world_options_only )
{
    if( !world_options_only ) {
        draw_border( w, BORDER_COLOR, _( "Options" ) );
    }
    // intersections
    mvwputch( w, point( 0, horizontal_level ), BORDER_COLOR, LINE_XXXO ); // |-
    mvwputch( w, point( getmaxx( w ) - 1, horizontal_level ), BORDER_COLOR, LINE_XOXX ); // -|
    for( const int &x : vert_lines ) {
        mvwputch( w, point( x + 1, getmaxy( w ) - 1 ), BORDER_COLOR, LINE_XXOX ); // _|_
    }
    wnoutrefresh( w );
}

static void draw_borders_internal( const catacurses::window &w, std::set<int> &vert_lines )
{
    for( int i = 0; i < getmaxx( w ); ++i ) {
        if( vert_lines.count( i ) != 0 ) {
            // intersection
            mvwputch( w, point( i, 0 ), BORDER_COLOR, LINE_OXXX );
        } else {
            // regular line
            mvwputch( w, point( i, 0 ), BORDER_COLOR, LINE_OXOX );
        }
    }
    wnoutrefresh( w );
}

std::string
options_manager::PageItem::fmt_tooltip( const Group &group,
                                        const options_manager::options_container &cont ) const
{
    switch( type ) {
        case ItemType::BlankLine:
            return "";
        case ItemType::GroupHeader: {
            return group.tooltip_.translated();
        }
        case ItemType::Option: {
            const std::string &opt_name = data;
            const cOpt &opt = cont.find( opt_name )->second;
            std::string ret = string_format( "%s #%s",
                                             opt.getTooltip(),
                                             opt.getDefaultText() );
#if defined(TILES) || defined(_WIN32)
            if( opt_name == "TERMINAL_X" ) {
                int new_window_width = 0;
                new_window_width = projected_window_width();

                ret += " -- ";
                ret += string_format(
                           n_gettext( "The window will be %d pixel wide with the selected value.",
                                      "The window will be %d pixels wide with the selected value.",
                                      new_window_width ), new_window_width );
            } else if( opt_name == "TERMINAL_Y" ) {
                int new_window_height = 0;
                new_window_height = projected_window_height();

                ret += " -- ";
                ret += string_format(
                           n_gettext( "The window will be %d pixel tall with the selected value.",
                                      "The window will be %d pixels tall with the selected value.",
                                      new_window_height ), new_window_height );
            }
#endif
            return ret;
        }
        default:
            cata_fatal( "invalid ItemType" );
    }
}

/** String with color */
struct string_col {
    std::string s;
    nc_color col;

    string_col() : col( c_black ) { }
    string_col( const std::string &s, nc_color col ) : s( s ), col( col ) { }
};

std::string options_manager::show( bool ingame, const bool world_options_only, bool with_tabs )
{
    const int iWorldOptPage = std::find_if( pages_.begin(), pages_.end(), [&]( const Page & p ) {
        return p.id_ == "world_default";
    } ) - pages_.begin();

    // temporary alias so the code below does not need to be changed
    options_container &OPTIONS = options;
    options_container &ACTIVE_WORLD_OPTIONS = world_options.has_value() ?
            *world_options.value() :
            OPTIONS;

    options_container OPTIONS_OLD = OPTIONS;
    options_container WOPTIONS_OLD = ACTIVE_WORLD_OPTIONS;
    if( world_generator->active_world == nullptr ) {
        ingame = false;
    }

    size_t sel_worldgen_tab = 1;
    std::map<size_t, inclusive_rectangle<point>> worldgen_tab_map;
    std::map<int, inclusive_rectangle<point>> opt_tab_map;
    std::map<int, inclusive_rectangle<point>> opt_line_map;
    std::set<int> vert_lines;
    vert_lines.insert( 4 );
    vert_lines.insert( 60 );

    int iCurrentPage = world_options_only ? iWorldOptPage : 0;
    int iCurrentLine = 0;
    int iStartPos = 0;

    std::unordered_map<std::string, bool> groups_state;
    groups_state.emplace( "", true ); // Non-existent group
    for( const Group &g : groups_ ) {
        // Start collapsed
        groups_state.emplace( g.id_, false );
    }

    input_context ctxt( "OPTIONS" );
    ctxt.register_cardinal();
    ctxt.register_action( "PAGE_UP", to_translation( "Fast scroll up" ) );
    ctxt.register_action( "PAGE_DOWN", to_translation( "Fast scroll down" ) );
    ctxt.register_action( "QUIT" );
    if( with_tabs || !world_options_only ) {
        ctxt.register_action( "NEXT_TAB" );
        ctxt.register_action( "PREV_TAB" );
    }
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    // for mouse selection
    ctxt.register_action( "SELECT" );
    ctxt.register_action( "MOUSE_MOVE" );
    ctxt.register_action( "SCROLL_UP" );
    ctxt.register_action( "SCROLL_DOWN" );

    const int iWorldOffset = world_options_only ? 2 : 0;
    int iMinScreenWidth = 0;
    const int iTooltipHeight = 7;
    int iContentHeight = 0;
    bool recalc_startpos = false;

    catacurses::window w_options_border;
    catacurses::window w_options_tooltip;
    catacurses::window w_options_header;
    catacurses::window w_options;

    const auto init_windows = [&]( ui_adaptor & ui ) {
        recalc_startpos = true;
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
        werase( w_options_header );
        werase( w_options_border );
        werase( w_options_tooltip );
        werase( w_options );

        opt_line_map.clear();
        opt_tab_map.clear();
        if( world_options_only ) {
            if( with_tabs ) {
                worldgen_tab_map = worldfactory::draw_worldgen_tabs( w_options_border, sel_worldgen_tab );
            } else {
                werase( w_options_border );
                draw_border( w_options_border );
            }
        }

        draw_borders_external( w_options_border, iTooltipHeight + 1 + iWorldOffset, vert_lines,
                               world_options_only );
        draw_borders_internal( w_options_header, vert_lines );

        options_manager::options_container &cOPTIONS = ( ingame || world_options_only ) &&
                iCurrentPage == iWorldOptPage ?
                ACTIVE_WORLD_OPTIONS : OPTIONS;

        const Page &page = pages_[iCurrentPage];
        const std::vector<PageItem> &page_items = page.items_;

        // Cache visible entries
        std::vector<int> visible_items;
        visible_items.reserve( page_items.size() );
        int curr_line_visible = 0;
        const auto is_visible = [&]( int i ) -> bool {
            const PageItem &it = page_items[i];
            switch( it.type )
            {
                case ItemType::GroupHeader:
                    return true;
                case ItemType::BlankLine:
                case ItemType::Option:
                    return groups_state[it.group];
                default:
                    cata_fatal( "invalid ItemType" );
            }
        };
        for( int i = 0; i < static_cast<int>( page_items.size() ); i++ ) {
            if( is_visible( i ) ) {
                if( i == iCurrentLine ) {
                    curr_line_visible = static_cast<int>( visible_items.size() );
                }
                visible_items.push_back( i );
            }
        }

        // Format name & value strings for given entry
        const auto fmt_name_value = [&]( const PageItem & it, bool is_selected )
        -> std::pair<string_col, string_col> {
            const char *IN_GROUP_PREFIX = ": ";
            switch( it.type )
            {
                case ItemType::BlankLine: {
                    std::string name = it.group.empty() ? "" : IN_GROUP_PREFIX;
                    return { string_col( name, c_white ), string_col() };
                }
                case ItemType::GroupHeader: {
                    bool expanded = groups_state[it.group];
                    std::string name = expanded ? "- " : "+ ";
                    name += find_group( it.group ).name_.translated();
                    return std::make_pair( string_col( name, c_white ), string_col() );
                }
                case options_manager::ItemType::Option: {
                    const cOpt &opt = cOPTIONS.find( it.data )->second;
                    const bool hasPrerequisite = opt.hasPrerequisite();
                    const bool hasPrerequisiteFulfilled = opt.checkPrerequisite();

                    std::string name_prefix = it.group.empty() ? "" : IN_GROUP_PREFIX;
                    string_col name( name_prefix + opt.getMenuText(), !hasPrerequisite ||
                                     hasPrerequisiteFulfilled ? c_white : c_light_gray );

                    nc_color cLineColor;
                    if( hasPrerequisite && !hasPrerequisiteFulfilled ) {
                        cLineColor = c_light_gray;
                    } else if( opt.getValue() == "false" || opt.getValue() == "disabled" || opt.getValue() == "off" ) {
                        cLineColor = c_light_red;
                    } else {
                        cLineColor = c_light_green;
                    }

                    string_col value( opt.getValueName(), is_selected ? hilite( cLineColor ) : cLineColor );

                    return std::make_pair( name, value );
                }
                default:
                    cata_fatal( "invalid ItemType" );
            }
        };

        // Draw separation lines
        for( int x : vert_lines ) {
            for( int y = 0; y < iContentHeight; y++ ) {
                mvwputch( w_options, point( x, y ), BORDER_COLOR, LINE_XOXO );
            }
        }

        if( recalc_startpos ) {
            // Update scroll position
            calcStartPos( iStartPos, curr_line_visible, iContentHeight,
                          static_cast<int>( visible_items.size() ) );
        }

        // where the column with the names starts
        const size_t name_col = 5;
        // where the column with the values starts
        const size_t value_col = 62;
        // 2 for the space between name and value column, 3 for the ">> "
        const size_t name_width = value_col - name_col - 2 - 3;
        const size_t value_width = getmaxx( w_options ) - value_col;
        //Draw options
        for( int i = iStartPos;
             i < iStartPos + ( iContentHeight > static_cast<int>( visible_items.size() ) ?
                               static_cast<int>( visible_items.size() ) : iContentHeight ); i++ ) {

            int line_pos = i - iStartPos; // Current line position in window.

            mvwprintz( w_options, point( 1, line_pos ), c_white, "%d", visible_items[i] + 1 );

            bool is_selected = visible_items[i] == iCurrentLine;
            if( is_selected ) {
                mvwprintz( w_options, point( name_col, line_pos ), c_yellow, ">>" );
            }

            const PageItem &curr_item = page_items[visible_items[i]];
            auto name_value = fmt_name_value( curr_item, is_selected );

            const std::string name = utf8_truncate( name_value.first.s, name_width );
            mvwprintz( w_options, point( name_col + 3, line_pos ), name_value.first.col, name );

            trim_and_print( w_options, point( value_col, line_pos ), value_width,
                            name_value.second.col, name_value.second.s );

            opt_line_map.emplace( visible_items[i], inclusive_rectangle<point>( point( name_col, line_pos ),
                                  point( value_col + value_width - 1, line_pos ) ) );
        }

        scrollbar()
        .offset_x( 0 )
        .offset_y( iTooltipHeight + 2 + iWorldOffset )
        .content_size( static_cast<int>( visible_items.size() ) )
        .viewport_pos( iStartPos )
        .viewport_size( iContentHeight )
        .apply( w_options_border );

        wnoutrefresh( w_options_border );

        //Draw Tabs
        int tab_x = 0;
        if( !world_options_only ) {
            mvwprintz( w_options_header, point( 7, 0 ), c_white, "" );
            for( int i = 0; i < static_cast<int>( pages_.size() ); i++ ) {
                wprintz( w_options_header, c_white, "[" );
                if( ingame && i == iWorldOptPage ) {
                    wprintz( w_options_header, iCurrentPage == i ? hilite( c_light_green ) : c_light_green,
                             _( "Current world" ) );
                } else {
                    wprintz( w_options_header, iCurrentPage == i ? hilite( c_light_green ) : c_light_green,
                             "%s", pages_[i].name_ );
                }
                wprintz( w_options_header, c_white, "]" );
                wputch( w_options_header, BORDER_COLOR, LINE_OXOX );
                tab_x++;
                int tab_w = utf8_width( pages_[i].name_.translated(), true );
                opt_tab_map.emplace( i, inclusive_rectangle<point>( point( 7 + tab_x, 0 ),
                                     point( 6 + tab_x + tab_w, 0 ) ) );
                tab_x += tab_w + 2;
            }
        }

        wnoutrefresh( w_options_header );

        const PageItem &curr_item = page_items[iCurrentLine];
        std::string tooltip = curr_item.fmt_tooltip( find_group( curr_item.group ), cOPTIONS );
        fold_and_print( w_options_tooltip, point_zero, iMinScreenWidth - 2, c_white, tooltip );

        if( ingame && iCurrentPage == iWorldOptPage ) {
            mvwprintz( w_options_tooltip, point( 3, 5 ), c_light_red, "%s", _( "Note: " ) );
            wprintz( w_options_tooltip, c_white, "%s",
                     _( "Some of these options may produce unexpected results if changed." ) );
        }
        wnoutrefresh( w_options_tooltip );

        wnoutrefresh( w_options );
    } );

    while( true ) {
        ui_manager::redraw();

        recalc_startpos = false;
        Page &page = pages_[iCurrentPage];
        auto &page_items = page.items_;

        options_manager::options_container &cOPTIONS = ( ingame || world_options_only ) &&
                iCurrentPage == iWorldOptPage ?
                ACTIVE_WORLD_OPTIONS : OPTIONS;

        std::string action = ctxt.handle_input();

        if( world_options_only && ( action == "NEXT_TAB" || action == "PREV_TAB" || action == "QUIT" ) ) {
            return action;
        }

        const PageItem &curr_item = page_items[iCurrentLine];

        const auto on_select_option = [&]() {
            cOpt &current_opt = cOPTIONS[curr_item.data];

#if defined(LOCALIZE)
            if( current_opt.getName() == "USE_LANG" ) {
                current_opt.setValue( select_language() );
                return;
            }
#endif

            bool hasPrerequisite = current_opt.hasPrerequisite();
            bool hasPrerequisiteFulfilled = current_opt.checkPrerequisite();

            if( hasPrerequisite && !hasPrerequisiteFulfilled ) {
                popup( _( "Prerequisite for this option not met!\n(%s)" ),
                       get_options().get_option( current_opt.getPrerequisite() ).getMenuText() );
                return;
            }

            if( action == "LEFT" ) {
                current_opt.setPrev();
            } else if( action == "RIGHT" ) {
                current_opt.setNext();
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
            }
        };

        const auto is_selectable = [&]( int i ) -> bool {
            const PageItem &curr_item = page_items[i];
            switch( curr_item.type )
            {
                case ItemType::BlankLine:
                    return false;
                case ItemType::GroupHeader:
                    return true;
                case ItemType::Option:
                    return groups_state[curr_item.group];
                default:
                    cata_fatal( "invalid ItemType" );
            }
        };

        if( action == "MOUSE_MOVE" || action == "SELECT" ) {
            bool found_opt = false;
            sel_worldgen_tab = 1;
            std::optional<point> coord = ctxt.get_coordinates_text( w_options_border );
            if( world_options_only && with_tabs && coord.has_value() ) {
                // worldgen tabs
                found_opt = run_for_point_in<size_t, point>( worldgen_tab_map, *coord,
                [&sel_worldgen_tab]( const std::pair<size_t, inclusive_rectangle<point>> &p ) {
                    sel_worldgen_tab = p.first;
                } ) > 0;
                if( found_opt && action == "SELECT" && sel_worldgen_tab != 1 ) {
                    return sel_worldgen_tab == 0 ? "PREV_TAB" : "NEXT_TAB";
                }
            }
            coord = ctxt.get_coordinates_text( w_options_header );
            if( !found_opt && coord.has_value() ) {
                // option category tabs
                bool new_val = false;
                const int psize = pages_.size();
                found_opt = run_for_point_in<int, point>( opt_tab_map, *coord,
                [&iCurrentPage, &new_val, &psize]( const std::pair<int, inclusive_rectangle<point>> &p ) {
                    if( p.first != iCurrentPage ) {
                        new_val = true;
                        iCurrentPage = clamp<int>( p.first, 0, psize - 1 );
                    }
                } ) > 0;
                if( new_val ) {
                    iCurrentLine = 0;
                    iStartPos = 0;
                    recalc_startpos = true;
                    sfx::play_variant_sound( "menu_move", "default", 100 );
                }
            }
            coord = ctxt.get_coordinates_text( w_options );
            if( !found_opt && coord.has_value() ) {
                // option lines
                const int psize = page_items.size();
                found_opt = run_for_point_in<int, point>( opt_line_map, *coord,
                [&iCurrentLine, &psize]( const std::pair<int, inclusive_rectangle<point>> &p ) {
                    iCurrentLine = clamp<int>( p.first, 0, psize - 1 );
                } ) > 0;
                if( found_opt && action == "SELECT" ) {
                    action = "CONFIRM";
                }
            }
        }

        const int recmax = static_cast<int>( page_items.size() );
        const int scroll_rate = recmax > 20 ? 10 : 3;

        if( action == "DOWN" || action == "SCROLL_DOWN" ) {
            do {
                iCurrentLine++;
                if( iCurrentLine >= recmax ) {
                    iCurrentLine = 0;
                }
            } while( !is_selectable( iCurrentLine ) );
            recalc_startpos = true;
        } else if( action == "UP" || action == "SCROLL_UP" ) {
            do {
                iCurrentLine--;
                if( iCurrentLine < 0 ) {
                    iCurrentLine = page_items.size() - 1;
                }
            } while( !is_selectable( iCurrentLine ) );
            recalc_startpos = true;
        } else if( action == "PAGE_DOWN" ) {
            if( iCurrentLine == recmax - 1 ) {
                iCurrentLine = 0;
            } else if( iCurrentLine + scroll_rate >= recmax ) {
                iCurrentLine = recmax - 1;
            } else {
                iCurrentLine += +scroll_rate;
                while( iCurrentLine < recmax && !is_selectable( iCurrentLine ) ) {
                    iCurrentLine++;
                }
            }
            recalc_startpos = true;
        } else if( action == "PAGE_UP" ) {
            if( iCurrentLine == 0 ) {
                iCurrentLine = recmax - 1;
            } else if( iCurrentLine <= scroll_rate ) {
                iCurrentLine = 0;
            } else {
                iCurrentLine += -scroll_rate;
                while( iCurrentLine > 0 && !is_selectable( iCurrentLine ) ) {
                    iCurrentLine--;
                }
            }
            recalc_startpos = true;
        } else if( action == "NEXT_TAB" ) {
            iCurrentLine = 0;
            iStartPos = 0;
            recalc_startpos = true;
            iCurrentPage++;
            if( iCurrentPage >= static_cast<int>( pages_.size() ) ) {
                iCurrentPage = 0;
            }
            sfx::play_variant_sound( "menu_move", "default", 100 );
        } else if( action == "PREV_TAB" ) {
            iCurrentLine = 0;
            iStartPos = 0;
            recalc_startpos = true;
            iCurrentPage--;
            if( iCurrentPage < 0 ) {
                iCurrentPage = pages_.size() - 1;
            }
            sfx::play_variant_sound( "menu_move", "default", 100 );
        } else if( action == "RIGHT" || action == "LEFT" || action == "CONFIRM" ) {
            switch( curr_item.type ) {
                case ItemType::Option: {
                    on_select_option();
                    break;
                }
                case ItemType::GroupHeader: {
                    bool &state = groups_state[curr_item.data];
                    state = !state;
                    recalc_startpos = true;
                    break;
                }
                case ItemType::BlankLine: {
                    // Should never happen
                    break;
                }
                default:
                    cata_fatal( "invalid ItemType" );
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

            } else if( iter.first == "TILES" || iter.first == "USE_TILES" || iter.first == "DISTANT_TILES" ||
                       iter.first == "USE_DISTANT_TILES" || iter.first == "OVERMAP_TILES" ) {
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
        set_language_from_options();
    }
    calendar::set_eternal_season( ::get_option<bool>( "ETERNAL_SEASON" ) );
    calendar::set_season_length( ::get_option<int>( "SEASON_LENGTH" ) );

    calendar::set_eternal_night( ::get_option<std::string>( "ETERNAL_TIME_OF_DAY" ) == "night" );
    calendar::set_eternal_day( ::get_option<std::string>( "ETERNAL_TIME_OF_DAY" ) == "day" );

#if !defined(EMSCRIPTEN) && !defined(__ANDROID__) && (defined(TILES) || defined(_WIN32))
    if( terminal_size_changed ) {
        int scaling_factor = get_scaling_factor();
        point TERM( ::get_option<int>( "TERMINAL_X" ), ::get_option<int>( "TERMINAL_Y" ) );
        TERM.x -= TERM.x % scaling_factor;
        TERM.y -= TERM.y % scaling_factor;
        const point set_term( std::max( EVEN_MINIMUM_TERM_WIDTH * scaling_factor, TERM.x ),
                              std::max( EVEN_MINIMUM_TERM_HEIGHT * scaling_factor, TERM.y ) );
        get_option( "TERMINAL_X" ).setValue( set_term.x );
        get_option( "TERMINAL_Y" ).setValue( set_term.y );
        save();

        resize_term( ::get_option<int>( "TERMINAL_X" ), ::get_option<int>( "TERMINAL_Y" ) );
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
        for( const PageItem &it : p.items_ ) {
            if( it.type != ItemType::Option ) {
                continue;
            }
            const auto iter = options.find( it.data );
            if( iter != options.end() ) {
                const options_manager::cOpt &opt = iter->second;

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

void options_manager::deserialize( const JsonArray &ja )
{
    for( JsonObject joOptions : ja ) {
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
    const auto iter = get_migrated_options().find( name );
    return iter != get_migrated_options().end() ? iter->second.first : name;
}

std::string options_manager::migrateOptionValue( const std::string &name,
        const std::string &val ) const
{
    const auto iter = get_migrated_options().find( name );
    if( iter == get_migrated_options().end() ) {
        return val;
    }

    const auto iter_val = iter->second.second.find( val );
    return iter_val != iter->second.second.end() ? iter_val->second : val;
}

void options_manager::update_options_cache()
{
    // cache to global due to heavy usage.
    trigdist = ::get_option<bool>( "CIRCLEDIST" );
    use_tiles = ::get_option<bool>( "USE_TILES" );

    prevent_occlusion = ::get_option<int>( "PREVENT_OCCLUSION" );
    prevent_occlusion_retract = ::get_option<bool>( "PREVENT_OCCLUSION_RETRACT" );
    prevent_occlusion_transp = ::get_option<bool>( "PREVENT_OCCLUSION_TRANSP" );
    prevent_occlusion_min_dist = ::get_option<float>( "PREVENT_OCCLUSION_MIN_DIST" );
    prevent_occlusion_max_dist = ::get_option<float>( "PREVENT_OCCLUSION_MAX_DIST" );

    // if the tilesets are identical don't duplicate
    use_far_tiles = ::get_option<bool>( "USE_DISTANT_TILES" ) ||
                    ::get_option<std::string>( "TILES" ) == ::get_option<std::string>( "DISTANT_TILES" );
    use_tiles_overmap = ::get_option<bool>( "USE_OVERMAP_TILES" );
    log_from_top = ::get_option<std::string>( "LOG_FLOW" ) == "new_top";
    message_ttl = ::get_option<int>( "MESSAGE_TTL" );
    message_cooldown = ::get_option<int>( "MESSAGE_COOLDOWN" );
    fov_3d_z_range = ::get_option<int>( "FOV_3D_Z_RANGE" );
    keycode_mode = ::get_option<std::string>( "SDL_KEYBOARD_MODE" ) == "keycode";
    use_pinyin_search = ::get_option<bool>( "USE_PINYIN_SEARCH" );

    cata::options::damage_indicators.clear();
    for( int i = 0; i < 6; i++ ) {
        const std::string opt_name = "DAMAGE_INDICATOR_LEVEL_" + std::to_string( i );
        const std::string val = ::has_option( opt_name )
                                ? ::get_option<std::string>( opt_name )
                                : "<color_pink>\?\?</color>";
        cata::options::damage_indicators.emplace_back( val );
    }
}

bool options_manager::save() const
{
    const cata_path savefile = PATH_INFO::options();

    update_options_cache();

    update_music_volume();

    return write_to_file( savefile, [&]( std::ostream & fout ) {
        JsonOut jout( fout, true );
        serialize( jout );
    }, _( "options" ) );
}

void options_manager::load()
{
    const cata_path file = PATH_INFO::options();
    read_from_file_optional_json( file, [&]( const JsonArray & jsin ) {
        deserialize( jsin );
    } );

    update_global_locale();
    update_options_cache();

#if defined(SDL_SOUND)
    sounds::sound_enabled = ::get_option<bool>( "SOUND_ENABLED" );
#endif
}

bool options_manager::has_option( const std::string &name ) const
{
    return options.count( name );
}

options_manager::cOpt &options_manager::get_option( const std::string &name )
{
    std::unordered_map<std::string, cOpt>::iterator opt = options.find( name );
    if( opt == options.end() ) {
        debugmsg( "requested non-existing option %s", name );
        static cOpt nullopt;
        return nullopt;
    }
    if( !world_options.has_value() ) {
        // Global options contains the default for new worlds, which is good enough here.
        return opt->second;
    }
    std::unordered_map<std::string, cOpt>::iterator wopt = ( *world_options )->find( name );
    if( wopt == ( *world_options )->end() ) {
        if( opt->second.getPage() != "world_default" ) {
            // Requested a non-world option, deliver it.
            return opt->second;
        }
        // May be a new option and an old world - import default from global options.
        cOpt &new_world_option = ( **world_options )[name];
        new_world_option = opt->second;
        return new_world_option;
    }
    return wopt->second;
}

options_manager::options_container options_manager::get_world_defaults() const
{
    std::unordered_map<std::string, cOpt> result;
    for( const auto &elem : options ) {
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
        } else if( lang == "ar" ) {
            std::locale::global( std::locale( "ar_SA.UTF-8" ) );
        } else if( lang == "cs" ) {
            std::locale::global( std::locale( "cs_CZ.UTF-8" ) );
        } else if( lang == "da" ) {
            std::locale::global( std::locale( "da_DK.UTF-8" ) );
        } else if( lang == "de" ) {
            std::locale::global( std::locale( "de_DE.UTF-8" ) );
        } else if( lang == "el" ) {
            std::locale::global( std::locale( "el_GR.UTF-8" ) );
        } else if( lang == "es_AR" ) {
            std::locale::global( std::locale( "es_AR.UTF-8" ) );
        } else if( lang == "es_ES" ) {
            std::locale::global( std::locale( "es_ES.UTF-8" ) );
        } else if( lang == "fr" ) {
            std::locale::global( std::locale( "fr_FR.UTF-8" ) );
        } else if( lang == "hu" ) {
            std::locale::global( std::locale( "hu_HU.UTF-8" ) );
        } else if( lang == "id" ) {
            std::locale::global( std::locale( "id_ID.UTF-8" ) );
        } else if( lang == "is" ) {
            std::locale::global( std::locale( "is_IS.UTF-8" ) );
        } else if( lang == "it_IT" ) {
            std::locale::global( std::locale( "it_IT.UTF-8" ) );
        } else if( lang == "ja" ) {
            std::locale::global( std::locale( "ja_JP.UTF-8" ) );
        } else if( lang == "ko" ) {
            std::locale::global( std::locale( "ko_KR.UTF-8" ) );
        } else if( lang == "nb" ) {
            std::locale::global( std::locale( "no_NO.UTF-8" ) );
        } else if( lang == "nl" ) {
            std::locale::global( std::locale( "nl_NL.UTF-8" ) );
        } else if( lang == "pl" ) {
            std::locale::global( std::locale( "pl_PL.UTF-8" ) );
        } else if( lang == "pt_BR" ) {
            std::locale::global( std::locale( "pt_BR.UTF-8" ) );
        } else if( lang == "ru" ) {
            std::locale::global( std::locale( "ru_RU.UTF-8" ) );
        } else if( lang == "sr" ) {
            std::locale::global( std::locale( "sr_CS.UTF-8" ) );
        } else if( lang == "tr" ) {
            std::locale::global( std::locale( "tr_TR.UTF-8" ) );
        } else if( lang == "uk_UA" ) {
            std::locale::global( std::locale( "uk_UA.UTF-8" ) );
        } else if( lang == "zh_CN" ) {
            std::locale::global( std::locale( "zh_CN.UTF-8" ) );
        } else if( lang == "zh_TW" ) {
            std::locale::global( std::locale( "zh_TW.UTF-8" ) );
        }
    } catch( std::runtime_error & ) {
        std::locale::global( std::locale() );
    }

    DebugLog( D_INFO, DC_ALL ) << "[options] C locale set to " << setlocale( LC_ALL, nullptr );
    DebugLog( D_INFO, DC_ALL ) << "[options] C++ locale set to " << std::locale().name();
}
