#include "game.h"
#include "options.h"
#include "output.h"
#include "debug.h"
#include "translations.h"
#include "filesystem.h"
#include "cursesdef.h"
#include "path_info.h"
#include "mapsharing.h"
#include "cata_utility.h"
#include "input.h"
#include "worldfactory.h"
#include "catacharset.h"
#include "game_constants.h"
#include "string_input_popup.h"

#ifdef TILES
#include "cata_tiles.h"
#endif // TILES

#include <stdlib.h>
#include <string>
#include <locale>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <memory>

bool trigdist;
bool use_tiles;
bool log_from_top;
int message_ttl;
bool fov_3d;
bool tile_iso;

#ifdef TILES
extern std::unique_ptr<cata_tiles> tilecontext;
#endif // TILES

std::map<std::string, std::string> TILESETS; // All found tilesets: <name, tileset_dir>
std::map<std::string, std::string> SOUNDPACKS; // All found soundpacks: <name, soundpack_dir>
std::vector<std::pair<std::string, std::string> > vPages;
std::map<int, std::vector<std::string> > mPageItems;
std::map<std::string, int> mOptionsSort;
std::map<std::string, std::string> optionNames;
int iWorldOptPage;

options_manager &get_options()
{
    static options_manager single_instance;
    return single_instance;
}

options_manager::options_manager()
{
    enable_json("DEFAULT_REGION");
    // to allow class based init_data functions to add values to a 'string' type option, add:
    //   enable_json("OPTION_KEY_THAT_GETS_STRING_ENTRIES_ADDED_VIA_JSON");
    // then, in the my_class::load_json (or post-json setup) method:
    //   get_options().add_value("OPTION_KEY_THAT_GETS_STRING_ENTRIES_ADDED_VIA_JSON", "thisvalue");
}

static const std::string blank_value( 1, 001 ); // because "" might be valid

void options_manager::enable_json(const std::string &lvar)
{
    post_json_verify[ lvar ] = blank_value;
}

void options_manager::add_retry(const std::string &lvar, const::std::string &lval)
{
    std::map<std::string, std::string>::const_iterator it = post_json_verify.find(lvar);
    if ( it != post_json_verify.end() && it->second == blank_value ) {
        // initialized with impossible value: valid
        post_json_verify[ lvar ] = lval;
    }
}

void options_manager::add_value( const std::string &lvar, const std::string &lval,
                                 const std::string &lvalname )
{
    std::map<std::string, std::string>::const_iterator it = post_json_verify.find(lvar);
    if ( it != post_json_verify.end() ) {
        auto ot = global_options.find(lvar);
        if ( ot != global_options.end() && ot->second.sType == "string_select" ) {
            for(std::vector<std::string>::const_iterator eit = ot->second.vItems.begin();
                eit != ot->second.vItems.end(); ++eit) {
                if ( *eit == lval ) { // already in
                    return;
                }
            }
            ot->second.vItems.push_back(lval);
            if ( optionNames.find(lval) == optionNames.end() ) {
                optionNames[ lval ] = ( lvalname == "" ? lval : lvalname );
            }
            // our value was saved, then set to default, so set it again.
            if ( it->second == lval ) {
                global_options[ lvar ].setValue( lval );
            }
        }

    }
}

options_manager::cOpt::cOpt()
{
    sType = "VOID";
    sPage = "";
    hide = COPT_NO_HIDE;
}

//add string select option
void options_manager::add(const std::string sNameIn, const std::string sPageIn,
                            const std::string sMenuTextIn, const std::string sTooltipIn,
                            const std::string sItemsIn, std::string sDefaultIn,
                            copt_hide_t opt_hide)
{
    cOpt thisOpt;

    thisOpt.sName = sNameIn;
    thisOpt.sPage = sPageIn;
    thisOpt.sMenuText = sMenuTextIn;
    thisOpt.sTooltip = sTooltipIn;
    thisOpt.sType = "string_select";

    thisOpt.hide = opt_hide;

    std::stringstream ssTemp(sItemsIn);
    std::string sItem;
    while (std::getline(ssTemp, sItem, ',')) {
        thisOpt.vItems.push_back(sItem);
    }

    if (thisOpt.getItemPos(sDefaultIn) == -1) {
        sDefaultIn = thisOpt.vItems[0];
    }

    thisOpt.sDefault = sDefaultIn;
    thisOpt.sSet = sDefaultIn;

    thisOpt.setSortPos(sPageIn);

    global_options[sNameIn] = thisOpt;
}

//add string input option
void options_manager::add(const std::string sNameIn, const std::string sPageIn,
                            const std::string sMenuTextIn, const std::string sTooltipIn,
                            const std::string sDefaultIn, const int iMaxLengthIn,
                            copt_hide_t opt_hide)
{
    cOpt thisOpt;

    thisOpt.sName = sNameIn;
    thisOpt.sPage = sPageIn;
    thisOpt.sMenuText = sMenuTextIn;
    thisOpt.sTooltip = sTooltipIn;
    thisOpt.sType = "string_input";

    thisOpt.hide = opt_hide;

    thisOpt.iMaxLength = iMaxLengthIn;
    thisOpt.sDefault = (thisOpt.iMaxLength > 0) ? sDefaultIn.substr(0, thisOpt.iMaxLength) : sDefaultIn;
    thisOpt.sSet = thisOpt.sDefault;

    thisOpt.setSortPos(sPageIn);

    global_options[sNameIn] = thisOpt;
}

//add bool option
void options_manager::add(const std::string sNameIn, const std::string sPageIn,
                            const std::string sMenuTextIn, const std::string sTooltipIn,
                            const bool bDefaultIn, copt_hide_t opt_hide)
{
    cOpt thisOpt;

    thisOpt.sName = sNameIn;
    thisOpt.sPage = sPageIn;
    thisOpt.sMenuText = sMenuTextIn;
    thisOpt.sTooltip = sTooltipIn;
    thisOpt.sType = "bool";

    thisOpt.hide = opt_hide;

    thisOpt.bDefault = bDefaultIn;
    thisOpt.bSet = bDefaultIn;

    thisOpt.setSortPos(sPageIn);

    global_options[sNameIn] = thisOpt;
}

//add int option
void options_manager::add(const std::string sNameIn, const std::string sPageIn,
                            const std::string sMenuTextIn, const std::string sTooltipIn,
                            const int iMinIn, int iMaxIn, int iDefaultIn,
                            copt_hide_t opt_hide, const std::string &format )
{
    cOpt thisOpt;

    thisOpt.sName = sNameIn;
    thisOpt.sPage = sPageIn;
    thisOpt.sMenuText = sMenuTextIn;
    thisOpt.sTooltip = sTooltipIn;
    thisOpt.sType = "int";

    thisOpt.format = format;

    thisOpt.hide = opt_hide;

    if (iMinIn > iMaxIn) {
        iMaxIn = iMinIn;
    }

    thisOpt.iMin = iMinIn;
    thisOpt.iMax = iMaxIn;

    if (iDefaultIn < iMinIn || iDefaultIn > iMaxIn) {
        iDefaultIn = iMinIn ;
    }

    thisOpt.iDefault = iDefaultIn;
    thisOpt.iSet = iDefaultIn;

    thisOpt.setSortPos(sPageIn);

    global_options[sNameIn] = thisOpt;
}

//add int map option
void options_manager::add(const std::string sNameIn, const std::string sPageIn,
                            const std::string sMenuTextIn, const std::string sTooltipIn,
                            const std::map<int, std::string> mIntValuesIn, int iInitialIn,
                            int iDefaultIn, copt_hide_t opt_hide)
{
    cOpt thisOpt;

    thisOpt.sName = sNameIn;
    thisOpt.sPage = sPageIn;
    thisOpt.sMenuText = sMenuTextIn;
    thisOpt.sTooltip = sTooltipIn;
    thisOpt.sType = "int_map";

    thisOpt.hide = opt_hide;

    thisOpt.mIntValues = mIntValuesIn;

    auto item = mIntValuesIn.find( iInitialIn );
    if ( item == mIntValuesIn.cend() ) {
        iInitialIn = mIntValuesIn.cbegin()->first;
    }

    item = mIntValuesIn.find( iDefaultIn );
    if ( item == mIntValuesIn.cend() ) {
        iDefaultIn = mIntValuesIn.cbegin()->first;
    }

    thisOpt.iDefault = iDefaultIn;
    thisOpt.iSet = iInitialIn;

    thisOpt.setSortPos(sPageIn);

    global_options[sNameIn] = thisOpt;
}

//add float option
void options_manager::add(const std::string sNameIn, const std::string sPageIn,
                            const std::string sMenuTextIn, const std::string sTooltipIn,
                            const float fMinIn, float fMaxIn, float fDefaultIn,
                            float fStepIn, copt_hide_t opt_hide, const std::string &format )
{
    cOpt thisOpt;

    thisOpt.sName = sNameIn;
    thisOpt.sPage = sPageIn;
    thisOpt.sMenuText = sMenuTextIn;
    thisOpt.sTooltip = sTooltipIn;
    thisOpt.sType = "float";

    thisOpt.format = format;

    thisOpt.hide = opt_hide;

    if (fMinIn > fMaxIn) {
        fMaxIn = fMinIn;
    }

    thisOpt.fMin = fMinIn;
    thisOpt.fMax = fMaxIn;
    thisOpt.fStep = fStepIn;

    if (fDefaultIn < fMinIn || fDefaultIn > fMaxIn) {
        fDefaultIn = fMinIn ;
    }

    thisOpt.fDefault = fDefaultIn;
    thisOpt.fSet = fDefaultIn;

    thisOpt.setSortPos(sPageIn);

    global_options[sNameIn] = thisOpt;
}

//helper functions
bool options_manager::cOpt::is_hidden() const
{
    switch( hide ) {
    case COPT_NO_HIDE:
        return false;

    case COPT_SDL_HIDE:
#ifdef TILES
        return true;
#else
        return false;
#endif

    case COPT_CURSES_HIDE:
#ifndef TILES // If not defined.  it's curses interface.
        return true;
#else
        return false;
#endif

    case COPT_POSIX_CURSES_HIDE:
        // Check if we on windows and using wincuses.
#if (defined TILES || defined _WIN32 || defined WINDOWS)
        return false;
#else
        return true;
#endif

    case COPT_NO_SOUND_HIDE:
#ifndef SDL_SOUND // If not defined, we have no sound support.
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

void options_manager::cOpt::setSortPos(const std::string sPageIn)
{
    if (!is_hidden()) {
        mOptionsSort[sPageIn]++;
        iSortPos = mOptionsSort[sPageIn] - 1;

    } else {
        iSortPos = -1;
    }
}

int options_manager::cOpt::getSortPos() const
{
    return iSortPos;
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
    return sMenuText;
}

std::string options_manager::cOpt::getTooltip() const
{
    return sTooltip;
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
        debugmsg( "unknown option type %s", sType.c_str() );
        return false;
    }
}

std::string options_manager::cOpt::getValue() const
{
    if (sType == "string_select" || sType == "string_input") {
        return sSet;

    } else if (sType == "bool") {
        return (bSet) ? "true" : "false";

    } else if (sType == "int" || sType == "int_map") {
        return string_format( format, iSet );

    } else if (sType == "float") {
        std::ostringstream ssTemp;
        ssTemp.imbue( std::locale::classic() );
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
    if( sType != "string_select" && sType != "string_input" ) {
        debugmsg( "%s tried to get string value from option of type %s", sName.c_str(), sType.c_str() );
    }
    return sSet;
}

template<>
bool options_manager::cOpt::value_as<bool>() const
{
    if( sType != "bool" ) {
        debugmsg( "%s tried to get boolean value from option of type %s", sName.c_str(), sType.c_str() );
    }
    return bSet;
}

template<>
float options_manager::cOpt::value_as<float>() const
{
    if( sType != "float" ) {
        debugmsg( "%s tried to get float value from option of type %s", sName.c_str(), sType.c_str() );
    }
    return fSet;
}

template<>
int options_manager::cOpt::value_as<int>() const
{
    if( sType != "int" && sType != "int_map" ) {
        debugmsg( "%s tried to get integer value from option of type %s", sName.c_str(), sType.c_str() );
    }
    return iSet;
}

std::string options_manager::cOpt::getValueName() const
{
    if (sType == "string_select") {
        return optionNames[sSet];

    } else if (sType == "bool") {
        return (bSet) ? _("True") : _("False");

    } else if ( sType == "int_map" ) {
        return string_format(_("%d: %s"), iSet, mIntValues.find( iSet )->second.c_str());
    }

    return getValue();
}

std::string options_manager::cOpt::getDefaultText(const bool bTranslated) const
{
    if (sType == "string_select") {
        const std::string sItems = enumerate_as_string( vItems.begin(), vItems.end(),
        [ this, bTranslated ]( const std::string &elem ) {
            return bTranslated ? optionNames[elem] : elem;
        }, false );
        return string_format(_("Default: %s - Values: %s"),
                             (bTranslated) ? optionNames[sDefault].c_str() : sDefault.c_str(), sItems.c_str());

    } else if (sType == "string_input") {
        return string_format(_("Default: %s"), sDefault.c_str());

    } else if (sType == "bool") {
        return (bDefault) ? _("Default: True") : _("Default: False");

    } else if (sType == "int") {
        return string_format(_("Default: %d - Min: %d, Max: %d"), iDefault, iMin, iMax);

    } else if (sType == "int_map") {
        return string_format( _( "Default: %d: %s" ), iDefault, mIntValues.find( iDefault )->second.c_str() );

    } else if (sType == "float") {
        return string_format(_("Default: %.2f - Min: %.2f, Max: %.2f"), fDefault, fMin, fMax);
    }

    return "";
}

int options_manager::cOpt::getItemPos(const std::string sSearch) const
{
    if (sType == "string_select") {
        for (size_t i = 0; i < vItems.size(); i++) {
            if (vItems[i] == sSearch) {
                return i;
            }
        }
    }

    return -1;
}

int options_manager::cOpt::getMaxLength() const
{
    if (sType == "string_input") {
        return iMaxLength;
    }

    return 0;
}

//set to next item
void options_manager::cOpt::setNext()
{
    if (sType == "string_select") {
        int iNext = getItemPos(sSet) + 1;
        if (iNext >= (int)vItems.size()) {
            iNext = 0;
        }

        sSet = vItems[iNext];

    } else if (sType == "string_input") {
        int iMenuTextLength = sMenuText.length();
        string_input_popup()
        .width( ( iMaxLength > 80 ) ? 80 : ( ( iMaxLength < iMenuTextLength ) ? iMenuTextLength : iMaxLength + 1) )
        .description( sMenuText )
        .max_length( iMaxLength )
        .edit( sSet );

    } else if (sType == "bool") {
        bSet = !bSet;

    } else if (sType == "int") {
        iSet++;
        if (iSet > iMax) {
            iSet = iMin;
        }

    } else if (sType == "int_map") {
        auto next = std::next( mIntValues.find( iSet ) );
        if ( next == mIntValues.cend() ) {
            iSet = mIntValues.cbegin()->first;
        } else {
            iSet = next->first;
        }

    } else if (sType == "float") {
        fSet += fStep;
        if (fSet > fMax) {
            fSet = fMin;
        }
    }
}

//set to prev item
void options_manager::cOpt::setPrev()
{
    if (sType == "string_select") {
        int iPrev = getItemPos(sSet) - 1;
        if (iPrev < 0) {
            iPrev = vItems.size() - 1;
        }

        sSet = vItems[iPrev];

    } else if (sType == "string_select") {
        setNext();

    } else if (sType == "bool") {
        bSet = !bSet;

    } else if (sType == "int") {
        iSet--;
        if (iSet < iMin) {
            iSet = iMax;
        }

    } else if (sType == "int_map") {
        auto item = mIntValues.find( iSet );
        if ( item == mIntValues.cbegin() ) {
            auto prev = std::prev( mIntValues.cend() );
            iSet = prev->first;
        } else {
            auto prev = std::prev( item );
            iSet = prev->first;
        }

    } else if (sType == "float") {
        fSet -= fStep;
        if (fSet < fMin) {
            fSet = fMax;
        }
    }
}

//set value
void options_manager::cOpt::setValue(float fSetIn)
{
    if (sType != "float") {
        debugmsg("tried to set a float value to a %s option", sType.c_str());
        return;
    }
    fSet = fSetIn;
    if ( fSet < fMin || fSet > fMax ) {
        fSet = fDefault;
    }
}

//set value
void options_manager::cOpt::setValue( int iSetIn )
{
    if( sType != "int" ) {
        debugmsg( "tried to set an int value to a %s option", sType.c_str() );
        return;
    }
    iSet = iSetIn;
    if( iSet < iMin || iSet > iMax ) {
        iSet = iDefault;
    }
}

//set value
void options_manager::cOpt::setValue(std::string sSetIn)
{
    if (sType == "string_select") {
        if (getItemPos(sSetIn) != -1) {
            sSet = sSetIn;
        }

    } else if (sType == "string_input") {
        sSet = (iMaxLength > 0) ? sSetIn.substr(0, iMaxLength) : sSetIn;

    } else if (sType == "bool") {
        bSet = (sSetIn == "True" || sSetIn == "true" || sSetIn == "T" || sSetIn == "t");

    } else if (sType == "int") {
        iSet = atoi(sSetIn.c_str());

        if ( iSet < iMin || iSet > iMax ) {
            iSet = iDefault;
        }

    } else if (sType == "int_map") {
        iSet = atoi(sSetIn.c_str());

        auto item = mIntValues.find( iSet );
        if ( item == mIntValues.cend() ) {
            iSet = iDefault;
        }

    } else if (sType == "float") {
        std::istringstream ssTemp(sSetIn);
        ssTemp.imbue(std::locale::classic());
        float tmpFloat;
        ssTemp >> tmpFloat;
        if(ssTemp) {
            setValue(tmpFloat);
        } else {
            debugmsg("invalid floating point option: %s", sSetIn.c_str());
        }
    }
}

/** Fill a mapping with values.
 * Scans all directores in FILENAMES[dirname_label] directory for
 * a file named FILENAMES[filename_label].
 * All found values added to resource_option as name, resource_dir.
 * Furthermore, it builds possible values list for cOpt class.
 * @return string containing all found resources in form "resource1,resource2,resource3,..."
 */
static std::string build_resource_list(
    std::map<std::string, std::string> &resource_option, std::string operation_name,
    std::string dirname_label, std::string filename_label ) {
    std::string resource_names;

    resource_option.clear();
    auto const resource_dirs = get_directories_with( FILENAMES[filename_label],
                                                     FILENAMES[dirname_label], true );

    for( auto &resource_dir : resource_dirs ) {
        read_from_file( resource_dir + "/" + FILENAMES[filename_label], [&]( std::istream &fin ) {
            std::string resource_name;
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
                        resource_name = "";
                        getline( fin, resource_name );
                        resource_name.erase( std::remove( resource_name.begin(), resource_name.end(), ',' ), resource_name.end() );
                        resource_name = trim( resource_name );
                        if( resource_names.empty() ) {
                            resource_names += resource_name;
                        } else {
                            resource_names += std::string( "," );
                            resource_names += resource_name;
                        }
                    } else if( sOption.find( "VIEW" ) != std::string::npos ) {
                        std::string viewName = "";
                        getline( fin, viewName );
                        viewName = trim( viewName );
                        optionNames[resource_name] = viewName;
                        break;
                    }
                }
            }
            if( resource_option.count( resource_name ) != 0 ) {
                DebugLog( D_ERROR, DC_ALL ) << "Found " << operation_name << " duplicate with name " << resource_name;
            } else {
                resource_option.insert( std::pair<std::string,std::string>( resource_name, resource_dir ) );
            }
        } );
    }

    return resource_names;
}

std::string options_manager::build_tilesets_list()
{
    std::string tileset_names = build_resource_list( TILESETS, "tileset",
                                                     "gfxdir", "tileset-conf");

    if( tileset_names.empty() ) {
        optionNames["deon"] = _("Deon's");
        optionNames["hoder"] = _("Hoder's");
        return "hoder,deon";
    }
    return tileset_names;
}

std::string options_manager::build_soundpacks_list()
{
    const std::string soundpack_names = build_resource_list( SOUNDPACKS, "soundpack",
                                                             "sounddir", "soundpack-conf");
    if( soundpack_names.empty() ) {
        optionNames["basic"] = _("Basic");
        return "basic";
    }
    return soundpack_names;
}

void options_manager::init()
{
    global_options.clear();
    vPages.clear();
    mPageItems.clear();
    mOptionsSort.clear();
    optionNames.clear();

    vPages.push_back(std::make_pair("general", _("General")));
    vPages.push_back(std::make_pair("interface", _("Interface")));
    vPages.push_back(std::make_pair("graphics", _("Graphics")));
    // when sharing maps only admin is allowed to change these.
    if(!MAP_SHARING::isCompetitive() || MAP_SHARING::isAdmin()) {
        vPages.push_back(std::make_pair("debug", _("Debug")));
    }
    iWorldOptPage = vPages.size();
    // when sharing maps only admin is allowed to change these.
    if(!MAP_SHARING::isCompetitive() || MAP_SHARING::isAdmin()) {
        vPages.push_back(std::make_pair("world_default", _("World Defaults")));
    }

#ifdef __ANDROID__
    vPages.push_back(std::make_pair("android", _("Android")));
#endif

    std::string tileset_names;
    tileset_names = build_tilesets_list(); //get the tileset names and set the optionNames

    std::string soundpack_names;
    soundpack_names = build_soundpacks_list(); //get the soundpack names and set the optionNames

    ////////////////////////////GENERAL//////////////////////////
    add("DEF_CHAR_NAME", "general", _("Default character name"),
        _("Set a default character name that will be used instead of a random name on character creation."),
        "", 30
        );

    mOptionsSort["general"]++;

    add("AUTO_PICKUP", "general", _("Auto pickup enabled"),
        _("Enable item auto pickup.  Change pickup rules with the Auto Pickup Manager in the Help Menu ?3"),
        false
        );

    add("AUTO_PICKUP_ADJACENT", "general", _("Auto pickup adjacent"),
        _("If true, will enable to pickup items one tile around to the player.  You can assign No Auto Pickup zones with the Zones Manager 'Y' key for eg.  your homebase."),
        false
        );

    add("AUTO_PICKUP_WEIGHT_LIMIT", "general", _("Auto pickup light items"),
        _("Auto pickup items with weight less than or equal to [option] * 50 grams.  You must also set the small items option.  '0' disables this option"),
        0, 20, 0
        );

    add("AUTO_PICKUP_VOL_LIMIT", "general", _("Auto pickup small items"),
        _("Auto pickup items with volume less than or equal to [option] * 50 milliliters.  You must also set the light items option.  '0' disables this option"),
        0, 20, 0
        );

    add("AUTO_PICKUP_SAFEMODE", "general", _("Auto pickup safe mode"),
        _("Auto pickup is disabled as long as you can see monsters nearby.  This is affected by 'Safe Mode proximity distance'."),
        false
        );

    mOptionsSort["general"]++;

    add("DANGEROUS_PICKUPS", "general", _("Dangerous pickups"),
        _("If false, will cause player to drop new items that cause them to exceed the weight limit."),
        false
        );

    mOptionsSort["general"]++;

    add("AUTOSAFEMODE", "general", _("Auto-safe mode"),
        _("If true, turns safemode automatically back on after it being disabled beforehand.  See option 'Turns to re-enable safe mode'"),
        false
        );

    add("AUTOSAFEMODETURNS", "general", _("Turns to re-enable safe mode"),
        _("Number of turns after safe mode is re-enabled if no hostiles are in 'Safe Mode proximity distance'."),
        1, 100, 50
        );

    add("SAFEMODE", "general", _("Safe Mode"),
        _("If true, will hold the game and display a warning if a hostile monster/npc is approaching."),
        true
        );

    add("SAFEMODEPROXIMITY", "general", _("Safe Mode proximity distance"),
        _("If safe mode is enabled, distance to hostiles at which safe mode should show a warning.  0 = Max player viewdistance."),
        0, MAX_VIEW_DISTANCE, 0
        );

    add("SAFEMODEVEH", "general", _("Safe Mode when driving"),
        _("When true, safe mode will alert you of hostiles while you are driving a vehicle."),
        false
        );

    mOptionsSort["general"]++;

    add("TURN_DURATION", "general", _("Realtime turn progression"),
        _("If enabled, monsters will take periodic gameplay turns.  This value is the delay between each turn, in seconds.  Works best with Safe Mode disabled.  0 = disabled."),
        0.0, 10.0, 0.0, 0.05
        );

    mOptionsSort["general"]++;

    add("AUTOSAVE", "general", _("Periodically autosave"),
        _("If true, game will periodically save the map.  Autosaves occur based on in-game turns or real-time minutes, whichever is larger."),
        false
        );

    add("AUTOSAVE_TURNS", "general", _("Game turns between autosaves"),
        _("Number of game turns between autosaves"),
        10, 1000, 50
        );

    add("AUTOSAVE_MINUTES", "general", _("Real minutes between autosaves"),
        _("Number of real time minutes between autosaves"),
        0, 127, 5
        );

    mOptionsSort["general"]++;

    add("CIRCLEDIST", "general", _("Circular distances"),
        _("If true, the game will calculate range in a realistic way: light sources will be circles, diagonal movement will cover more ground and take longer.  If disabled, everything is square: moving to the northwest corner of a building takes as long as moving to the north wall."),
        false
        );

    optionNames["no"] = _("No");
    //~ containers
    optionNames["watertight"] = _("Watertight");
    optionNames["all"] = _("All");
    add("DROP_EMPTY", "general", _("Drop empty containers"),
        _("Set to drop empty containers after use.  No: Don't drop any. - Watertight: All except watertight containers. - All: Drop all containers."),
        "no,watertight,all", "no"
        );

    add("AUTO_NOTES", "general", _("Auto notes"),
        _("If true, automatically sets notes on places that have stairs that go up or down"),
        true
        );

    optionNames["ask"]      = _("Ask");
    optionNames["always"]   = _("Always");
    optionNames["never"]    = _("Never");
    add("DEATHCAM", "general", _("DeathCam"),
        _("Always: Always start deathcam.  Ask: Query upon death.  Never: Never show deathcam."),
        "always,ask,never", "ask"
        );

    mOptionsSort["general"]++;

    add("SOUNDPACKS", "general", _("Choose soundpack"),
        _("Choose the soundpack you want to use."),
        soundpack_names, "basic", COPT_NO_SOUND_HIDE
        ); // populate the options dynamically

    add("MUSIC_VOLUME", "general", _("Music Volume"),
        _("Adjust the volume of the music being played in the background."),
        0, 200, 100, COPT_NO_SOUND_HIDE
        );

    add("SOUND_EFFECT_VOLUME", "general", _("Sound Effect Volume"),
        _("Adjust the volume of sound effects being played by the game."),
        0, 200, 100, COPT_NO_SOUND_HIDE
        );

    ////////////////////////////INTERFACE////////////////////////
    // TODO: scan for languages like we do for tilesets.
    optionNames[""] = _("System language");
    // Note: language names are in their own language and are *not* translated at all.
    // Note: Somewhere in github PR was better link to msdn.microsoft.com with language names.
    // http://en.wikipedia.org/wiki/List_of_language_names
    optionNames["en"] = R"(English)";
    optionNames["fr"] =  R"(Français)";
    optionNames["de"] = R"(Deutsch)";
    optionNames["it_IT"] = R"(Italiano)";
    optionNames["es_AR"] = R"(Español (Argentina))";
    optionNames["es_ES"] = R"(Español (España))";
    optionNames["ja"] = R"(日本語)";
    optionNames["ko"] = R"(한국어)";
    optionNames["pt_BR"] = R"(Português (Brasil))";
    optionNames["pt_PT"] = R"(Português (Portugal))";
    optionNames["ru"] = R"(Русский)";
    optionNames["zh_CN"] = R"(中文(天朝))";
    optionNames["zh_TW"] = R"(中文(台灣))";
    add("USE_LANG", "interface", _("Language"), _("Switch Language."),
        ",en,fr,de,it_IT,es_AR,es_ES,ja,ko,pt_BR,pt_PT,ru,zh_CN,zh_TW",
        ""
        );

    mOptionsSort["interface"]++;

    optionNames["fahrenheit"] = _("Fahrenheit");
    optionNames["celsius"] = _("Celsius");
    add("USE_CELSIUS", "interface", _("Temperature units"),
        _("Switch between Celsius and Fahrenheit."),
        "fahrenheit,celsius", "fahrenheit"
        );

    optionNames["mph"] = _("mph");
    optionNames["km/h"] = _("km/h");
    add("USE_METRIC_SPEEDS", "interface", _("Speed units"),
        _("Switch between km/h and mph."),
        "mph,km/h", "mph"
        );

    optionNames["lbs"] = _("lbs");
    optionNames["kg"] = _("kg");
    add("USE_METRIC_WEIGHTS", "interface", _("Mass units"),
        _("Switch between kg and lbs."),
        "lbs,kg", "lbs"
        );

    optionNames["c"] = _("Cup");
    optionNames["l"] = _("Liter");
    optionNames["qt"] = _("Quart");
    add("VOLUME_UNITS", "interface", _("Volume units"),
        _("Switch between the Cup (c), Liter (L) or Quart (qt)."),
        "c,l,qt", "l"
        );

    //~ 12h time, e.g.  11:59pm
    optionNames["12h"] = _("12h");
    //~ Military time, e.g.  2359
    optionNames["military"] = _("Military");
    //~ 24h time, e.g.  23:59
    optionNames["24h"] = _("24h");
    add("24_HOUR", "interface", _("Time format"),
        _("12h: AM/PM, eg: 7:31 AM - Military: 24h Military, eg: 0731 - 24h: Normal 24h, eg: 7:31"),
        "12h,military,24h", "12h"
        );

    mOptionsSort["interface"]++;

    add("FORCE_CAPITAL_YN", "interface", _("Force Y/N in prompts"),
        _("If true, Y/N prompts are case-sensitive and y and n are not accepted."),
        true
        );

    add("SNAP_TO_TARGET", "interface", _("Snap to target"),
        _("If true, automatically follow the crosshair when firing/throwing."),
        false
        );

    add("SAVE_SLEEP", "interface", _("Ask to save before sleeping"),
        _("If true, game will ask to save the map before sleeping."),
        false
        );

    add("QUERY_DISASSEMBLE", "interface", _("Query on disassembly"),
        _("If true, will query before disassembling items."),
        true
        );

    add("QUERY_KEYBIND_REMOVAL", "interface", _("Query on keybinding removal"),
        _("If true, will query before removing a keybinding from a hotkey."),
        true
        );

    add("CLOSE_ADV_INV", "interface", _("Close advanced inventory on move all"),
        _("If true, will close the advanced inventory when the move all items command is used."),
        false
        );

    add( "INV_USE_ACTION_NAMES", "interface", _( "Display actions in Use Item menu" ),
        _( "If true, actions (like \"Read\", \"Smoke\", \"Wrap tighter\") will be displayed next to the corresponding items." ),
        true
        );

    mOptionsSort["interface"]++;

    add("VEHICLE_ARMOR_COLOR", "interface", _("Vehicle plating changes part color"),
        _("If true, vehicle parts will change color if they are armor plated"),
        true
        );

    add("DRIVING_VIEW_OFFSET", "interface", _("Auto-shift the view while driving"),
        _("If true, view will automatically shift towards the driving direction"),
        true
        );

    add("VEHICLE_DIR_INDICATOR", "interface", _("Draw vehicle facing indicator"),
        _("If true, when controlling a vehicle, a white 'X' (in curses version) or a crosshair (in tiles version) at distance 10 from the center will display its current facing."),
        true
        );

    mOptionsSort["interface"]++;

    //~ sidebar position
    optionNames["left"] = _("Left");
    optionNames["right"] = _("Right");
    add("SIDEBAR_POSITION", "interface", _("Sidebar position"),
        _("Switch between sidebar on the left or on the right side.  Requires restart."),
        "left,right", "right"
        );

    //~ sidebar style
    optionNames["wider"] = _("Wider");
    optionNames["narrow"] = _("Narrow");
    add("SIDEBAR_STYLE", "interface", _("Sidebar style"),
        _("Switch between a narrower or wider sidebar.  Requires restart."),
        "wider,narrow", "narrow"
        );

    //~ sidebar message log flow direction
    optionNames["new_top"] = _("Top");
    optionNames["new_bottom"] = _("Bottom");
    add("SIDEBAR_LOG_FLOW", "interface", _("Sidebar log flow"),
        _("Where new sidebar log messages should show."),
        "new_top,new_bottom", "new_bottom"
        );

    add("MESSAGE_TTL", "interface", _("Sidebar log message display duration"),
        _("Number of turns after which a message will be removed from the sidebar log.  '0' disables this option."),
        0, 1000, 0
        );

    //~ aim bar style - bars or numbers
    optionNames["numbers"] = _("Numbers");
    optionNames["bars"] = _("Bars");
    add("ACCURACY_DISPLAY", "interface", _("Aim window display style"),
        _("How should confidence and steadiness be communicated to the player."),
        "numbers,bars", "bars"
        );

    mOptionsSort["interface"]++;

    add("MOVE_VIEW_OFFSET", "interface", _("Move view offset"),
        _("Move view by how many squares per keypress."),
        1, 50, 1
        );

    add("MENU_SCROLL", "interface", _("Centered menu scrolling"),
        _("If true, menus will start scrolling in the center of the list, and keep the list centered."),
        true
        );

    optionNames["false"] = _("False");
    optionNames["centered"] = _("Centered");
    optionNames["edge"] = _("To edge");
    add("SHIFT_LIST_ITEM_VIEW", "interface", _("Shift list item view"),
        _("Centered or to edge, shift the view toward the selected item if it is outside of your current viewport."),
        "false,centered,edge",  "centered"
        );

    add("AUTO_INV_ASSIGN", "interface", _("Auto inventory letters"),
        _("If false, new inventory items will only get letters assigned if they had one before."),
        true
        );

    add("ITEM_HEALTH_BAR", "interface", _("Show item health bars"),
        _("If true, show item health bars instead of reinforced, scratched etc. text."),
        true
        );

    add("ITEM_SYMBOLS", "interface", _("Show item symbols"),
        _("If true, show item symbols in inventory and pick up menu."),
        false
        );

    mOptionsSort["interface"]++;

    add("ENABLE_JOYSTICK", "interface", _("Enable Joystick"),
        _("Enable input from joystick."),
        true, COPT_CURSES_HIDE
        );

    //~ show mouse cursor
    optionNames["show"] = _("Show");
    //~ hide mouse cursor
    optionNames["hide"] = _("Hide");
    //~ hide mouse cursor when keyboard is used
    optionNames["hidekb"] = _("HideKB");
    add("HIDE_CURSOR", "interface", _("Hide mouse cursor"),
        _("Show: Cursor is always shown.  Hide: Cursor is hidden.  HideKB: Cursor is hidden on keyboard input and unhidden on mouse movement."),
        "show,hide,hidekb", "show", COPT_CURSES_HIDE
        );

    ////////////////////////////GRAPHICS/////////////////////////
    add("ANIMATIONS", "graphics", _("Animations"),
        _("If true, will display enabled animations."),
        true
        );

    add("ANIMATION_RAIN", "graphics", _("Rain animation"),
        _("If true, will display weather animations."),
        true
        );

    add("ANIMATION_SCT", "graphics", _("SCT animation"),
        _("If true, will display scrolling combat text animations."),
        true
        );

    add("ANIMATION_DELAY", "graphics", _("Animation delay"),
        _("The amount of time to pause between animation frames in ms."),
        0, 100, 10
        );

    add("FORCE_REDRAW", "graphics", _("Force redraw"),
        _("If true, forces the game to redraw at least once per turn."),
        true
        );

    mOptionsSort["graphics"]++;

    add("TERMINAL_X", "graphics", _("Terminal width"),
        _("Set the size of the terminal along the X axis.  Requires restart."),
        80, 242, 80, COPT_POSIX_CURSES_HIDE
        );

    add("TERMINAL_Y", "graphics", _("Terminal height"),
        _("Set the size of the terminal along the Y axis.  Requires restart."),
        24, 187, 24, COPT_POSIX_CURSES_HIDE
        );

    mOptionsSort["graphics"]++;

    add("USE_TILES", "graphics", _("Use tiles"),
        _("If true, replaces some TTF rendered text with tiles."),
#ifdef __ANDROID__
        // On Android, we default to not using tiles, so the game will run on more lower spec devices by default.
        // Users can try turning tiles on, but if that doesn't work, they can just delete game data and reload.
        // Seems like a more user-friendly option than asking them to delete/mess around with the installed 'gfx' folder.
        false, COPT_CURSES_HIDE
#else
        true, COPT_CURSES_HIDE
#endif
        );

    add("TILES", "graphics", _("Choose tileset"),
        _("Choose the tileset you want to use."),
        tileset_names, "ChestHole", COPT_CURSES_HIDE
        ); // populate the options dynamically

    add("PIXEL_MINIMAP", "graphics", _("Pixel Minimap"),
        _("If true, shows the pixel-detail minimap in game after the save is loaded.  Use the 'Toggle Pixel Minimap' action key to change its visibility during gameplay."),
        true, COPT_CURSES_HIDE
        );

    add("PIXEL_MINIMAP_HEIGHT", "graphics", _("Pixel Minimap height"),
        _("Height of pixel-detail minimap, measured in terminal rows.  Set to 0 for default spacing."),
        0, 100, 0, COPT_CURSES_HIDE
        );

    add("PIXEL_MINIMAP_RATIO", "graphics", _("Maintain Pixel Minimap aspect ratio"),
        _("Preserves the square shape of tiles shown on the pixel minimap."),
        true, COPT_CURSES_HIDE
        );

    add("PIXEL_MINIMAP_BLINK", "graphics", _("Enemy beacon blink speed"),
        _("Controls how fast the enemy beacons blink on the pixel minimap.  Value is multiplied by 200 ms.  Set to 0 to disable."),
        0, 50, 10, COPT_CURSES_HIDE
        );

    mOptionsSort["graphics"]++;


    add("DISPLAY", "graphics", _("Display"),
        _("Sets which video display will be used to show the game.  Requires restart."),
        0, 10000, 0, COPT_CURSES_HIDE
        );

+#ifndef __ANDROID__
    optionNames["fullscreen"] = _("Fullscreen");
    optionNames["windowedbl"] = _("Windowed borderless");
    add("FULLSCREEN", "graphics", _("Fullscreen"),
        _("Starts Cataclysm in one of the fullscreen modes.  Requires restart."),
        "no,fullscreen,windowedbl", "no", COPT_CURSES_HIDE
        );
+#endif

    add("SOFTWARE_RENDERING", "graphics", _("Software rendering"),
        _("Use software renderer instead of graphics card acceleration."),
        false, COPT_CURSES_HIDE
        );

    //~ Do not scale the game image to the window size.
    optionNames["none"] = _("No scaling");
    //~ An algorithm for image scaling.
    optionNames["nearest"] = _("Nearest neighbor");
    //~ An algorithm for image scaling.
    optionNames["linear"] = _("Linear filtering");
    add("SCALING_MODE", "graphics", _("Scaling mode"),
        _("Sets the scaling mode, 'none' (default) displays at the game's native resolution, 'nearest'  uses low-quality but fast scaling, and 'linear' provides high-quality scaling."),
        "none,nearest,linear", "none", COPT_CURSES_HIDE
        );

    ////////////////////////////DEBUG////////////////////////////
    add("DISTANCE_INITIAL_VISIBILITY", "debug", _("Distance initial visibility"),
        _("Determines the scope, which is known in the beginning of the game."),
        3, 20, 15
        );

    mOptionsSort["debug"]++;

    add("INITIAL_STAT_POINTS", "debug", _("Initial stat points"),
        _("Initial points available to spend on stats on character generation."),
        0, 1000, 6
        );

    add("INITIAL_TRAIT_POINTS", "debug", _("Initial trait points"),
        _("Initial points available to spend on traits on character generation."),
        0, 1000, 0
        );

    add("INITIAL_SKILL_POINTS", "debug", _("Initial skill points"),
        _("Initial points available to spend on skills on character generation."),
        0, 1000, 2
        );

    add("MAX_TRAIT_POINTS", "debug", _("Maximum trait points"),
            _("Maximum trait points available for character generation."),
            0, 1000, 12
            );

    mOptionsSort["debug"]++;

    add("SKILL_TRAINING_SPEED", "debug", _("Skill training speed"),
        _("Scales experience gained from practicing skills and reading books.  0.5 is half as fast as default, 2.0 is twice as fast, 0.0 disables skill training except for NPC training."),
        0.0, 100.0, 1.0, 0.1
        );

    mOptionsSort["debug"]++;

    //~ plain, default, normal
    optionNames["vanilla"] = _("Vanilla");
    //~ capped at a value
    optionNames["capped"] = _("Capped");
    //~ based on intelligence
    optionNames["int"] = _("Int");
    //~ based on intelligence and capped
    optionNames["intcap"] = _("IntCap");
    optionNames["off"] = _("Off");
    add("SKILL_RUST", "debug", _("Skill rust"),
        _("Set the level of skill rust.  Vanilla: Vanilla Cataclysm - Capped: Capped at skill levels 2 - Int: Intelligence dependent - IntCap: Intelligence dependent, capped - Off: None at all."),
        "vanilla,capped,int,intcap,off", "off"
        );

    mOptionsSort["debug"]++;

    add("FOV_3D", "debug", _("Experimental 3D Field of Vision"),
        _("If false, vision is limited to current z-level.  If true and the world is in z-level mode, the vision will extend beyond current z-level.  Currently very bugged!"),
        false
        );

    add("ENCODING_CONV", "debug", _("Experimental path name encoding conversion"),
        _("If true, file path names are going to be transcoded from system encoding to UTF-8 when reading and will be transcoded back when writing.  Mainly for CJK Windows users."),
        true
        );
    
    mOptionsSort["debug"]++;
    
    add("OVERMAP_GENERATION_TRIES", "debug", _("Overmap generation attempt count"),
        _("Maximum number of retries in overmap generation due to inability to place mandatory special locations.  High numbers and strange world settings will lead to VERY slow generation!"),
        1, 20, 2
        );

    //~ allow invalid (bugged, bad) maps without asking user
    optionNames["allow_invalid"] = _("Any");
    //~ allow any valid map, even if it's "bad"
    optionNames["ask_invalid"] = _("Valid");
    //~ ask for lifting restrictions
    optionNames["ask_unlimited"] = _("Ask");
    add("ALLOW_INVALID_OVERMAPS", "debug", _("Allow invalid overmaps"),
        _("What to do if world settings/mods prevent valid overmaps.  Invalid maps are BUGGED and while playable, may cause errors during missions.  Unlimited maps will look ugly, but are fully functional."),
        "allow_invalid,ask_invalid,ask_unlimited", "ask_invalid"
        );

    ////////////////////////////WORLD DEFAULT////////////////////
    add("CORE_VERSION", "world_default", _("Core version data"),
        _("Controls what migrations are applied for legacy worlds"),
        1, core_version, core_version, COPT_ALWAYS_HIDE
        );

    mOptionsSort["world_default"]++;

    optionNames["no"] = _("No");
    optionNames["yes"] = _("Yes");
    optionNames["query"] = _("Query");
    add("DELETE_WORLD", "world_default", _("Delete world"),
        _("Delete the world when the last active character dies."),
        "no,yes,query", "no"
        );

    mOptionsSort["world_default"]++;

    add("CITY_SIZE", "world_default", _("Size of cities"),
        _("A number determining how large cities are.  0 disables cities and roads."),
        0, 16, 4
        );

    add("CITY_SPACING", "world_default", _("City spacing"),
        _("A number determining how far apart cities are.  Warning, small numbers lead to very slow mapgen."),
        0, 8, 4
        );

    add("SPAWN_DENSITY", "world_default", _("Spawn rate scaling factor"),
        _("A scaling factor that determines density of monster spawns."),
        0.0, 50.0, 1.0, 0.1
        );

    add("ITEM_SPAWNRATE", "world_default", _("Item spawn scaling factor"),
        _("A scaling factor that determines density of item spawns."),
        0.01, 10.0, 1.0, 0.01
        );

    add("NPC_DENSITY", "world_default", _("NPC spawn rate scaling factor"),
        _("A scaling factor that determines density of dynamic NPC spawns."),
        0.0, 100.0, 0.1, 0.01
        );

    add("MONSTER_UPGRADE_FACTOR", "world_default", _("Monster evolution scaling factor"),
        _("A scaling factor that determines the time between monster upgrades.  A higher number means slower evolution.  Set to 0.00 to turn off monster upgrades."),
        0.0, 100, 4.0, 0.01
        );

    mOptionsSort["world_default"]++;

    add("MONSTER_SPEED", "world_default", _("Monster speed"),
        _("Determines the movement rate of monsters.  A higher value increases monster speed and a lower reduces it."),
        1, 1000, 100, COPT_NO_HIDE, "%i%%"
        );

    add("MONSTER_RESILIENCE", "world_default", _("Monster resilience"),
        _("Determines how much damage monsters can take.  A higher value makes monsters more resilient and a lower makes them more flimsy."),
        1, 1000, 100, COPT_NO_HIDE, "%i%%"
        );

    mOptionsSort["world_default"]++;

    std::string region_ids("default");
    optionNames["default"] = "default";
    add("DEFAULT_REGION", "world_default", _("Default region type"),
        _("(WIP feature) Determines terrain, shops, plants, and more."),
        region_ids, "default"
        );

    mOptionsSort["world_default"]++;

    add("INITIAL_TIME", "world_default", _("Initial time"),
        _("Initial starting time of day on character generation."),
        0, 23, 8
        );

    optionNames["spring"] = _("Spring");
    optionNames["summer"] = _("Summer");
    optionNames["autumn"] = _("Autumn");
    optionNames["winter"] = _("Winter");
    add("INITIAL_SEASON", "world_default", _("Initial season"),
        _("Season the player starts in.  Options other than the default delay spawn of the character, so food decay and monster spawns will have advanced."),
        "spring,summer,autumn,winter", "spring"
        );

    add("SEASON_LENGTH", "world_default", _("Season length"),
        _("Season length, in days."),
        14, 127, 14
        );

    add("CONSTRUCTION_SCALING", "world_default", _("Construction scaling"),
        _("Sets the time of construction in percents.  '50' is two times faster than default, '200' is two times longer.  '0' automatically scales construction time to match the world's season length."),
        0, 1000, 100
        );

    add("ETERNAL_SEASON", "world_default", _("Eternal season"),
        _("Keep the initial season for ever."),
        false
        );

    mOptionsSort["world_default"]++;

    add("WANDER_SPAWNS", "world_default", _("Wander spawns"),
        _("Emulation of zombie hordes.  Zombie spawn points wander around cities and may go to noise.  Must reset world directory after changing for it to take effect."),
        false
        );

    add("CLASSIC_ZOMBIES", "world_default", _("Classic zombies"),
        _("Only spawn classic zombies and natural wildlife.  Requires a reset of save folder to take effect.  This disables certain buildings."),
        false
        );

    add("BLACK_ROAD", "world_default", _("Surrounded start"),
        _("If true, spawn zombies at shelters.  Makes the starting game a lot harder."),
        false
        );

    mOptionsSort["world_default"]++;

    add("STATIC_NPC", "world_default", _("Static npcs"),
        _("If true, the game will spawn static NPC at the start of the game, requires world reset."),
        false
        );

    add("RANDOM_NPC", "world_default", _("Random npcs"),
        _("If true, the game will randomly spawn NPC during gameplay."),
        false
        );

    mOptionsSort["world_default"]++;

    add("RAD_MUTATION", "world_default", _("Mutations by radiation"),
        _("If true, radiation causes the player to mutate."),
        true
        );

    mOptionsSort["world_default"]++;

    add("ZLEVELS", "world_default", _("Experimental z-levels"),
        _("If true, experimental z-level maps will be enabled.  This feature is not finished yet and turning it on will only slow the game down."),
        false
        );

    mOptionsSort["world_default"]++;

    add("ALIGN_STAIRS", "world_default", _("Align up and down stairs"),
        _("If true, downstairs will be placed directly above upstairs, even if this results in uglier maps."),
        false
        );

    mOptionsSort["world_default"]++;

    add("NO_FAULTS", "world_default", _("Disables vehicle part faults."),
        _("If true, disables vehicle part faults, vehicle parts will be totally reliable unless destroyed, and can only be repaired via replacement."),
        false, COPT_ALWAYS_HIDE
        );

    mOptionsSort["world_default"]++;

    add("FILTHY_MORALE", "world_default", _("Morale penalty for filthy clothing."),
        _("If true, wearing filthy clothing will cause morale penalties."),
        false, COPT_ALWAYS_HIDE
        );

    mOptionsSort["world_default"]++;

    add("FILTHY_WOUNDS", "world_default", _("Infected wounds from filthy clothing."),
        _("If true, getting hit in a body part covered in filthy clothing may cause infections."),
        false, COPT_ALWAYS_HIDE
        );

    mOptionsSort["world_default"]++;

    add("NO_VITAMINS", "world_default", _("Disables tracking vitamins in food items."),
        _("If true, disables vitamin tracking and vitamin disorders."),
        false, COPT_ALWAYS_HIDE
        );

    mOptionsSort["world_default"]++;

    add("NO_NPC_FOOD", "world_default", _("Disables tracking food, thirst and (partially) fatigue for NPCs."),
        _("If true, NPCs won't need to eat or drink and will only get tired enough to sleep, not to get penalties."),
        false, COPT_ALWAYS_HIDE
        );

#ifdef __ANDROID__
    add("ANDROID_SKIP_SPLASH", "android", _("Skip welcome screen"),
        _("If true, skips the Android welcome screen on app load."),
        false
        );

    add("ANDROID_QUICKSAVE", "android", _("Quicksave on app lose focus"),
        _("If true, quicksave whenever the app loses focus (screen locked, app moved into background etc.) WARNING: Experimental. This may result in corrupt save games."),
        false
        );

    mOptionsSort["android"]++;

    add("ANDROID_AUTO_KEYBOARD", "android", _("Auto-manage virtual keyboard"),
        _("If true, automatically show/hide the virtual keyboard when necessary based on context. If false, virtual keyboard must be toggled manually."),
        true
        );

    add("ANDROID_KEYBOARD_SCREEN_SCALE", "android", _("Virtual keyboard screen scale"),
        _("When the virtual keyboard is visible, scale the screen to prevent overlapping. Useful for text entry so you can see what you're typing."),
        true
        );

    mOptionsSort["android"]++;

    add("ANDROID_VIBRATION", "android", _("Vibration duration"),
        _("If non-zero, vibrate the device for this long on input, in millisconds. Ignored if hardware keyboard connected."),
        0, 200, 10
        );

    optionNames["slSensor"] = _("Sensor");
    optionNames["slPortrait"] = _("Portrait");
    optionNames["slLandscapeLeft"] = _("Landscape");
    optionNames["slLandscapeRight"] = _("Rev Landscape");
    add("ANDROID_SCREEN_ORIENTATION", "android", _("Screen orientation"),
        _("Use the device's sensor to orient the screen automatically, or force a specific orientation."),
        "slSensor,slPortrait,slLandscapeLeft,slLandscapeRight", "slSensor", COPT_CURSES_HIDE
        );

    mOptionsSort["android"]++;

    add("ANDROID_SHOW_VIRTUAL_JOYSTICK", "android", _("Show virtual joystick"),
        _("If true, show the virtual joystick when touching and holding the screen. Gives a visual indicator of deadzone and stick deflection."),
        true
        );

    add("ANDROID_VIRTUAL_JOYSTICK_OPACITY", "android", _("Virtual joystick opacity"),
        _("The opacity of the on-screen virtual joystick, as a percentage."),
        0, 100, 20
        );

    add("ANDROID_DEADZONE_RANGE", "android", _("Virtual joystick deadzone size"),
        _("While using the virtual joystick, deflecting the stick beyond this distance will trigger directional input. Specified as a percentage of longest screen edge."),
        0.01f, 0.2f, 0.03f, 0.001f, COPT_NO_HIDE, "%.3f"
        );

    add("ANDROID_REPEAT_DELAY_RANGE", "android", _("Virtual joystick size"),
        _("While using the virtual joystick, deflecting the stick by this much will repeat input at the deflected rate (see below). Specified as a percentage of longest screen edge."),
        0.05f, 0.5f, 0.10f, 0.001f, COPT_NO_HIDE, "%.3f"
        );

    add("ANDROID_VIRTUAL_JOYSTICK_FOLLOW", "android", _("Virtual joystick follows finger"),
        _("If true, the virtual joystick will follow when sliding beyond its range."),
        true
        );

    add("ANDROID_REPEAT_DELAY_MAX", "android", _("Virtual joystick repeat rate (centered)"),
        _("When the virtual joystick is centered, how fast should input events repeat, in milliseconds."),
        50, 1000, 500
        );

    add("ANDROID_REPEAT_DELAY_MIN", "android", _("Virtual joystick repeat rate (deflected)"),
        _("When the virtual joystick is fully deflected, how fast should input events repeat, in milliseconds."),
        50, 1000, 100
        );

    add("ANDROID_SENSITIVITY_POWER", "android", _("Virtual joystick repeat rate sensitivity"),
        _("As the virtual joystick moves from centered to fully deflected, this value is an exponent that controls the blend between the two repeat rates defined above. 1.0 = linear."),
        0.1f, 5.0f, 0.75f, 0.05f, COPT_NO_HIDE, "%.2f"
        );

    add("ANDROID_INITIAL_DELAY", "android", _("Input repeat delay"),
        _("While touching the screen, wait this long before showing the virtual joystick and repeating input, in milliseconds. Also used to determine tap/double-tap detection, flick detection and toggling quick shortcuts."),
        150, 1000, 300
        );

    add("ANDROID_HIDE_HOLDS", "android", _("Virtual joystick hides shortcuts"),
        _("If true, hides on-screen keyboard shortcuts while using the virtual joystick. Helps keep the view uncluttered while travelling long distances and navigating menus."),
        true
        );

    mOptionsSort["android"]++;

    add("ANDROID_SHORTCUT_DEFAULTS", "android", _("Default gameplay shortcuts"),
        _("The default set of gameplay shortcuts to show. Used on starting a new game and whenever all gameplay shortcuts are removed."),
        "?mi", 30
        );

    add("ANDROID_ACTIONMENU_AUTOADD", "android", _("Add shortcuts for action menu selections"),
        _("If true, automatically add a shortcut for actions selected via the in-game action menu."),
        true
        );

    add("ANDROID_INVENTORY_AUTOADD", "android", _("Add shortcuts for inventory selections"),
        _("If true, automatically add a shortcut for items selected via the inventory."),
        true
        );

    mOptionsSort["android"]++;

    add("ANDROID_TAP_KEY", "android", _("Tap key (in-game)"),
        _("The key to press when tapping during gameplay."),
        ".", 1
        );

    add("ANDROID_2_TAP_KEY", "android", _("Two-finger tap key (in-game)"),
        _("The key to press when tapping with two fingers during gameplay."),
        "i", 1
        );

    add("ANDROID_2_SWIPE_UP_KEY", "android", _("Two-finger swipe up key (in-game)"),
        _("The key to press when swiping up with two fingers during gameplay."),
        "K", 1
        );

    add("ANDROID_2_SWIPE_DOWN_KEY", "android", _("Two-finger swipe down key (in-game)"),
        _("The key to press when swiping down with two fingers during gameplay."),
        "J", 1
        );

    add("ANDROID_2_SWIPE_LEFT_KEY", "android", _("Two-finger swipe left key (in-game)"),
        _("The key to press when swiping left with two fingers during gameplay."),
        "L", 1
        );

    add("ANDROID_2_SWIPE_RIGHT_KEY", "android", _("Two-finger swipe right key (in-game)"),
        _("The key to press when swiping right with two fingers during gameplay."),
        "H", 1
        );

    add("ANDROID_PINCH_IN_KEY", "android", _("Pinch in key (in-game)"),
        _("The key to press when pinching in during gameplay."),
        "z", 1
        );

    add("ANDROID_PINCH_OUT_KEY", "android", _("Pinch out key (in-game)"),
        _("The key to press when pinching out during gameplay."),
        "Z", 1
        );

    mOptionsSort["android"]++;

    add("ANDROID_SHORTCUT_AUTOADD", "android", _("Auto-manage contextual gameplay shortcuts"),
        _("If true, contextual in-game shortcuts are added and removed automatically as needed: examine, close, butcher, move up/down, control vehicle, pickup, toggle enemy + safe mode, sleep."),
        true
        );

    add("ANDROID_SHORTCUT_AUTOADD_FRONT", "android", _("Move contextual gameplay shortcuts to front"),
        _("If the above option is enabled, specifies whether contextual in-game shortcuts will be added to the front or back of the shortcuts list. True makes them easier to reach, False reduces shuffling of shortcut positions."),
        false
        );

    add("ANDROID_SHORTCUT_MOVE_FRONT", "android", _("Move used shortcuts to front"),
        _("If true, using an existing shortcut will always move it to the front of the shortcuts list. If false, only shortcuts typed via keyboard will move to the front."),
        false
        );

    add("ANDROID_SHORTCUT_ZONE", "android", _("Separate shortcuts for No Auto Pickup zones"),
        _("If true, separate gameplay shortcuts will be used within No Auto Pickup zones. Useful for keeping home base actions separate from exploring actions."),
        true
        );

    add("ANDROID_SHORTCUT_REMOVE_TURNS", "android", _("Turns to remove unused gameplay shortcuts"),
        _("If non-zero, unused gameplay shortcuts will be removed after this many turns (as in discrete player actions, not world calendar turns)."),
        0, 1000, 0
        );

    add("ANDROID_SHORTCUT_PERSISTENCE", "android", _("Shortcuts persistence"),
        _("If true, shortcuts are saved/restored with each save game. If false, shortcuts reset between sessions."),
        true
        );

    mOptionsSort["android"]++;

    add("ANDROID_SHORTCUT_POSITION", "android", _("Shortcuts position"),
        _("Switch between shortcuts on the left or on the right side of the screen."),
        "left,right", "left"
        );

    add("ANDROID_SHORTCUT_SCREEN_PERCENTAGE", "android", _("Shortcuts screen percentage"),
        _("How much of the screen can shortcuts occupy, as a percentage of total screen width."),
        10, 100, 100
        );

    add("ANDROID_SHORTCUT_OVERLAP", "android", _("Shortcuts overlap screen"),
        _("If true, shortcuts will be drawn transparently overlapping the game screen. If false, the game screen size will be reduced to fit the shortcuts below."),
        true
        );

    add("ANDROID_SHORTCUT_OPACITY_BG", "android", _("Shortcut opacity (background)"),
        _("The background opacity of on-screen keyboard shortcuts, as a percentage."),
        0, 100, 75
        );

    add("ANDROID_SHORTCUT_OPACITY_SHADOW", "android", _("Shortcut opacity (shadow)"),
        _("The shadow opacity of on-screen keyboard shortcuts, as a percentage."),
        0, 100, 100
        );

    add("ANDROID_SHORTCUT_OPACITY_FG", "android", _("Shortcut opacity (text)"),
        _("The foreground opacity of on-screen keyboard shortcuts, as a percentage."),
        0, 100, 100
        );

    add("ANDROID_SHORTCUT_COLOR", "android", _("Shortcut color"),
        _("The color of on-screen keyboard shortcuts."),
        0, 15, 15
        );

    add("ANDROID_SHORTCUT_BORDER", "android", _("Shortcut border"),
        _("The border of each on-screen keyboard shortcut in pixels. ."),
        0, 16, 0
        );

    add("ANDROID_SHORTCUT_WIDTH_MIN", "android", _("Shortcut width (min)"),
        _("The minimum width of each on-screen keyboard shortcut in pixels. Only relevant when lots of shortcuts are visible at once."),
        20, 1000, 50
        );

    add("ANDROID_SHORTCUT_WIDTH_MAX", "android", _("Shortcut width (max)"),
        _("The maximum width of each on-screen keyboard shortcut in pixels."),
        50, 1000, 160
        );

    add("ANDROID_SHORTCUT_HEIGHT", "android", _("Shortcut height"),
        _("The height of each on-screen keyboard shortcut in pixels."),
        50, 1000, 130
        );

#endif

    for (unsigned i = 0; i < vPages.size(); ++i) {
        mPageItems[i].resize(mOptionsSort[vPages[i].first]);
    }

    for( auto &elem : global_options ) {
        for (unsigned i = 0; i < vPages.size(); ++i) {
            if( vPages[i].first == ( elem.second ).getPage() &&
                ( elem.second ).getSortPos() > -1 ) {
                mPageItems[i][( elem.second ).getSortPos()] = elem.first;
                break;
            }
        }
    }

    //Sort out possible double empty lines after options are hidden
    for (unsigned i = 0; i < vPages.size(); ++i) {
        bool bLastLineEmpty = false;
        while (mPageItems[i][0] == "") {
            //delete empty lines at the beginning
            mPageItems[i].erase(mPageItems[i].begin());
        }

        while (mPageItems[i][mPageItems[i].size() - 1] == "") {
            //delete empty lines at the end
            mPageItems[i].erase(mPageItems[i].end() - 1);
        }

        for (unsigned j = mPageItems[i].size() - 1; j > 0; --j) {
            bool bThisLineEmpty = (mPageItems[i][j] == "");

            if (bLastLineEmpty == true && bThisLineEmpty == true) {
                //delete empty lines in between
                mPageItems[i].erase(mPageItems[i].begin() + j);
            }

            bLastLineEmpty = bThisLineEmpty;
        }
    }
}

#ifdef TILES
// Helper method to isolate #ifdeffed tiles code.
static void refresh_tiles( bool used_tiles_changed, bool pixel_minimap_height_changed, bool ingame )
{
    if( used_tiles_changed ) {
        //try and keep SDL calls limited to source files that deal specifically with them
        try {
            tilecontext->reinit();
            //g->init_ui is called when zoom is changed
            g->reset_zoom();
            if( ingame ) {
                if( g->pixel_minimap_option ) {
                    wrefresh(g->w_pixel_minimap);
                }
                g->refresh_all();
            }
            tilecontext->do_tile_loading_report();
        } catch( const std::exception &err ) {
            popup( _( "Loading the tileset failed: %s" ), err.what() );
            use_tiles = false;
        }
    } else if( ingame && g->pixel_minimap_option && pixel_minimap_height_changed ) {
        tilecontext->reinit_minimap();
        g->init_ui();
        wrefresh( g->w_pixel_minimap );
        g->refresh_all();
    }
}
#else
static void refresh_tiles( bool, bool, bool ) {
}
#endif // TILES

void draw_borders_external( WINDOW *w, int horizontal_level, std::map<int, bool> &mapLines )
{
    draw_border( w, BORDER_COLOR, _( " OPTIONS " ) );
    // intersections
    mvwputch( w, horizontal_level, 0, BORDER_COLOR, LINE_XXXO ); // |-
    mvwputch( w, horizontal_level, getmaxx( w ) - 1, BORDER_COLOR, LINE_XOXX ); // -|
    for( auto &mapLine : mapLines ) {
        mvwputch( w, getmaxy( w ) - 1, mapLine.first + 1, BORDER_COLOR, LINE_XXOX ); // _|_
    }
    wrefresh( w );
}

void draw_borders_internal( WINDOW *w, std::map<int, bool> &mapLines )
{
    for( int i = 0; i < getmaxx( w ); ++i ) {
        if( mapLines[i] ) {
            // intersection
            mvwputch( w, 0, i, BORDER_COLOR, LINE_OXXX );
        } else {
            // regular line
            mvwputch( w, 0, i, BORDER_COLOR, LINE_OXOX );
        }
    }
    wrefresh( w );
}

void options_manager::show(bool ingame)
{
    // temporary alias so the code below does not need to be changed
    auto &OPTIONS = global_options;
    auto &ACTIVE_WORLD_OPTIONS = world_generator->active_world ? world_generator->active_world->WORLD_OPTIONS : OPTIONS;

    auto OPTIONS_OLD = OPTIONS;
    auto WOPTIONS_OLD = ACTIVE_WORLD_OPTIONS;
    if ( world_generator->active_world == NULL ) {
        ingame = false;
    }

    const int iTooltipHeight = 4;
    const int iContentHeight = FULL_SCREEN_HEIGHT - 3 - iTooltipHeight;

    const int iOffsetX = (TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0;
    const int iOffsetY = (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0;

    std::map<int, bool> mapLines;
    mapLines[4] = true;
    mapLines[60] = true;

    WINDOW *w_options_border = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH, iOffsetY, iOffsetX);

    WINDOW *w_options_tooltip = newwin(iTooltipHeight, FULL_SCREEN_WIDTH - 2, 1 + iOffsetY,
                                       1 + iOffsetX);
    WINDOW *w_options_header = newwin(1, FULL_SCREEN_WIDTH - 2, 1 + iTooltipHeight + iOffsetY,
                                      1 + iOffsetX);
    WINDOW *w_options = newwin(iContentHeight, FULL_SCREEN_WIDTH - 2,
                               iTooltipHeight + 2 + iOffsetY, 1 + iOffsetX);

    draw_borders_external( w_options_border, iTooltipHeight + 1, mapLines );
    draw_borders_internal( w_options_header, mapLines );

    int iCurrentPage = 0;
    int iLastPage = 0;
    int iCurrentLine = 0;
    int iStartPos = 0;

    input_context ctxt("OPTIONS");
    ctxt.register_cardinal();
    ctxt.register_action("QUIT");
    ctxt.register_action("NEXT_TAB");
    ctxt.register_action("PREV_TAB");
    ctxt.register_action("CONFIRM");
    ctxt.register_action("HELP_KEYBINDINGS");

    std::stringstream sTemp;

    while(true) {
        auto &cOPTIONS = ( ingame && iCurrentPage == iWorldOptPage ?
                           ACTIVE_WORLD_OPTIONS : OPTIONS );

        //Clear the lines
        for (int i = 0; i < iContentHeight; i++) {
            for (int j = 0; j < 79; j++) {
                if (mapLines[j]) {
                    mvwputch(w_options, i, j, BORDER_COLOR, LINE_XOXO);
                } else {
                    mvwputch(w_options, i, j, c_black, ' ');
                }

                if (i < iTooltipHeight) {
                    mvwputch(w_options_tooltip, i, j, c_black, ' ');
                }
            }
        }

        calcStartPos(iStartPos, iCurrentLine, iContentHeight, mPageItems[iCurrentPage].size());

        // where the column with the names starts
        const size_t name_col = 5;
        // where the column with the values starts
        const size_t value_col = 62;
        // 2 for the space between name and value column, 3 for the ">> "
        const size_t name_width = value_col - name_col - 2 - 3;
        const size_t value_width = getmaxx( w_options ) - value_col;
        //Draw options
        size_t iBlankOffset = 0; // Offset when blank line is printed.
        for (int i = iStartPos; i < iStartPos + ((iContentHeight > (int)mPageItems[iCurrentPage].size()) ?
                (int)mPageItems[iCurrentPage].size() : iContentHeight); i++) {

            int line_pos; // Current line position in window.
            nc_color cLineColor = c_light_green;
            cOpt *current_opt = &(cOPTIONS[mPageItems[iCurrentPage][i]]);

            line_pos = i - iStartPos;

            sTemp.str("");
            sTemp << i + 1 - iBlankOffset;
            mvwprintz(w_options, line_pos, 1, c_white, sTemp.str().c_str());

            if (iCurrentLine == i) {
                mvwprintz(w_options, line_pos, name_col, c_yellow, ">> ");
            } else {
                mvwprintz(w_options, line_pos, name_col, c_yellow, "   ");
            }
            const std::string name = utf8_truncate( current_opt->getMenuText(), name_width );
            mvwprintz(w_options, line_pos, name_col + 3, c_white, "%s", name.c_str());

            if (current_opt->getValue() == "false") {
                cLineColor = c_light_red;
            }

            const std::string value = utf8_truncate( current_opt->getValueName(), value_width );
            mvwprintz(w_options, line_pos, value_col, (iCurrentLine == i) ? hilite(cLineColor) :
                      cLineColor, "%s", value.c_str());
        }

        draw_scrollbar(w_options_border, iCurrentLine, iContentHeight,
                       mPageItems[iCurrentPage].size(), iTooltipHeight + 2, 0, BORDER_COLOR);
        wrefresh(w_options_border);

        //Draw Tabs
        mvwprintz(w_options_header, 0, 7, c_white, "");
        for (int i = 0; i < (int)vPages.size(); i++) {
            if (!mPageItems[i].empty()) { //skip empty pages
                wprintz(w_options_header, c_white, "[");
                if ( ingame && i == iWorldOptPage ) {
                    wprintz(w_options_header,
                            (iCurrentPage == i) ? hilite(c_light_green) : c_light_green, _("Current world"));
                } else {
                    wprintz(w_options_header, (iCurrentPage == i) ?
                            hilite(c_light_green) : c_light_green, (vPages[i].second).c_str());
                }
                wprintz(w_options_header, c_white, "]");
                wputch(w_options_header, BORDER_COLOR, LINE_OXOX);
            }
        }

        wrefresh(w_options_header);

#if (defined TILES || defined _WIN32 || defined WINDOWS)
        if (mPageItems[iCurrentPage][iCurrentLine] == "TERMINAL_X") {
            int new_terminal_x, new_window_width;
            std::stringstream value_conversion(OPTIONS[mPageItems[iCurrentPage][iCurrentLine]].getValueName());

            value_conversion >> new_terminal_x;
            new_window_width = projected_window_width(new_terminal_x);

            fold_and_print(w_options_tooltip, 0, 0, 78, c_white,
                           ngettext("%s #%s -- The window will be %d pixel wide with the selected value.",
                                    "%s #%s -- The window will be %d pixels wide with the selected value.",
                                    new_window_width),
                           OPTIONS[mPageItems[iCurrentPage][iCurrentLine]].getTooltip().c_str(),
                           OPTIONS[mPageItems[iCurrentPage][iCurrentLine]].getDefaultText().c_str(),
                           new_window_width);
        } else if (mPageItems[iCurrentPage][iCurrentLine] == "TERMINAL_Y") {
            int new_terminal_y, new_window_height;
            std::stringstream value_conversion(OPTIONS[mPageItems[iCurrentPage][iCurrentLine]].getValueName());

            value_conversion >> new_terminal_y;
            new_window_height = projected_window_height(new_terminal_y);

            fold_and_print(w_options_tooltip, 0, 0, 78, c_white,
                           ngettext("%s #%s -- The window will be %d pixel tall with the selected value.",
                                    "%s #%s -- The window will be %d pixels tall with the selected value.",
                                    new_window_height),
                           OPTIONS[mPageItems[iCurrentPage][iCurrentLine]].getTooltip().c_str(),
                           OPTIONS[mPageItems[iCurrentPage][iCurrentLine]].getDefaultText().c_str(),
                           new_window_height);
        } else
#endif
        {
            fold_and_print(w_options_tooltip, 0, 0, 78, c_white, "%s #%s",
                           OPTIONS[mPageItems[iCurrentPage][iCurrentLine]].getTooltip().c_str(),
                           OPTIONS[mPageItems[iCurrentPage][iCurrentLine]].getDefaultText().c_str());
        }

        if ( iCurrentPage != iLastPage ) {
            iLastPage = iCurrentPage;
            if ( ingame && iCurrentPage == iWorldOptPage ) {
                mvwprintz( w_options_tooltip, 3, 3, c_light_red, "%s", _("Note: ") );
                wprintz(  w_options_tooltip, c_white, "%s",
                          _("Some of these options may produce unexpected results if changed."));
            }
        }
        wrefresh(w_options_tooltip);

        wrefresh(w_options);

        const std::string action = ctxt.handle_input();

        if (action == "DOWN") {
            do {
                iCurrentLine++;
                if (iCurrentLine >= (int)mPageItems[iCurrentPage].size()) {
                    iCurrentLine = 0;
                }
            } while( (cOPTIONS[mPageItems[iCurrentPage][iCurrentLine]].getMenuText() == ""));
        } else if (action == "UP") {
            do {
                iCurrentLine--;
                if (iCurrentLine < 0) {
                    iCurrentLine = mPageItems[iCurrentPage].size() - 1;
                }
            } while( (cOPTIONS[mPageItems[iCurrentPage][iCurrentLine]].getMenuText() == "")
                   );
        } else if (!mPageItems[iCurrentPage].empty() && action == "RIGHT") {
            cOPTIONS[mPageItems[iCurrentPage][iCurrentLine]].setNext();
        } else if (!mPageItems[iCurrentPage].empty() && action == "LEFT") {
            cOPTIONS[mPageItems[iCurrentPage][iCurrentLine]].setPrev();
        } else if (action == "NEXT_TAB") {
            iCurrentLine = 0;
            iStartPos = 0;
            iCurrentPage++;
            if (iCurrentPage >= (int)vPages.size()) {
                iCurrentPage = 0;
            }
        } else if (action == "PREV_TAB") {
            iCurrentLine = 0;
            iStartPos = 0;
            iCurrentPage--;
            if (iCurrentPage < 0) {
                iCurrentPage = vPages.size() - 1;
            }
        } else if (!mPageItems[iCurrentPage].empty() && action == "CONFIRM") {
            cOpt &cur_opt = cOPTIONS[mPageItems[iCurrentPage][iCurrentLine]];
            if (cur_opt.getType() == "bool" || cur_opt.getType() == "string_select" || cur_opt.getType() == "string_input" ) {
                cur_opt.setNext();
            } else {
                const bool is_int = cur_opt.getType() == "int";
                const bool is_float = cur_opt.getType() == "float";
                const std::string old_opt_val = cur_opt.getValueName();
                const std::string opt_val = string_input_popup()
                                            .title( cur_opt.getMenuText() )
                                            .width( 10 )
                                            .text( old_opt_val )
                                            .only_digits( is_int )
                                            .query_string();
                if (!opt_val.empty() && opt_val != old_opt_val) {
                    if (is_float) {
                        std::istringstream ssTemp(opt_val);
                        ssTemp.imbue(std::locale(""));
                        // This uses the current locale, to allow the users
                        // to use their own decimal format.
                        float tmpFloat;
                        ssTemp >> tmpFloat;
                        if (ssTemp) {
                            cur_opt.setValue(tmpFloat);

                        } else {
                            popup(_("Invalid input: not a number"));
                        }
                    } else {
                        // option is of type "int": string_input_popup
                        // has taken care that the string contains
                        // only digits, parsing is done in setValue
                        cur_opt.setValue(opt_val);
                    }
                }
            }
        } else if( action == "HELP_KEYBINDINGS" ) {
            // keybinding screen erased the internal borders of main menu, restore it:
            draw_borders_internal( w_options_header, mapLines );
        } else if (action == "QUIT") {
            break;
        }
    }

    //Look for changes
    bool options_changed = false;
    bool world_options_changed = false;
    bool lang_changed = false;
    bool used_tiles_changed = false;
    bool pixel_minimap_height_changed = false;

    for (auto &iter : OPTIONS_OLD) {
        if ( iter.second != OPTIONS[iter.first] ) {
            options_changed = true;

            if ( iter.second.getPage() == "world_default" ) {
                world_options_changed = true;
            }

            if ( iter.first == "PIXEL_MINIMAP_HEIGHT" || iter.first == "PIXEL_MINIMAP_RATIO" ) {
                pixel_minimap_height_changed = true;
            }

            if ( iter.first == "TILES" || iter.first == "USE_TILES" ) {
                used_tiles_changed = true;

            } else if ( iter.first == "USE_LANG" ) {
                lang_changed = true;
            }
        }
    }
    for( auto &iter : WOPTIONS_OLD ) {
        if( iter.second != ACTIVE_WORLD_OPTIONS[iter.first] ) {
            options_changed = true;
            world_options_changed = true;
        }
    }

    if (options_changed) {
        if(query_yn(_("Save changes?"))) {
            save();
            if( ingame && world_options_changed ) {
                world_generator->active_world->WORLD_OPTIONS = ACTIVE_WORLD_OPTIONS;
                world_generator->save_world( world_generator->active_world, false );
            }
        } else {
            used_tiles_changed = false;
            OPTIONS = OPTIONS_OLD;
            if (ingame && world_options_changed) {
                ACTIVE_WORLD_OPTIONS = WOPTIONS_OLD;
            }
        }
    }
    if( lang_changed ) {
        set_language();
    }

    refresh_tiles( used_tiles_changed, pixel_minimap_height_changed, ingame );

    delwin(w_options);
    delwin(w_options_border);
    delwin(w_options_header);
    delwin(w_options_tooltip);
}

void options_manager::serialize(JsonOut &json) const
{
    json.start_array();

    for( size_t j = 0; j < vPages.size(); ++j ) {
        for( auto &elem : mPageItems[j] ) {
            // Skip blanks between option groups
            // to avoid empty json entries being stored
            if( elem.empty() ) {
                continue;
            }
            const auto iter = global_options.find( elem );
            if( iter != global_options.end() ) {
                const auto &opt = iter->second;
                json.start_object();

                json.member( "info", opt.getTooltip() );
                json.member( "default", opt.getDefaultText( false ) );
                json.member( "name", elem );
                json.member( "value", opt.getValue() );

                json.end_object();
            }
        }
    }

    json.end_array();
}

void options_manager::deserialize(JsonIn &jsin)
{
    jsin.start_array();
    while (!jsin.end_array()) {
        JsonObject joOptions = jsin.get_object();

        const std::string name = joOptions.get_string("name");
        const std::string value = joOptions.get_string("value");

        add_retry(name, value);
        global_options[ name ].setValue( value );
    }
}

bool options_manager::save()
{
    const auto savefile = FILENAMES["options"];

    // cache to global due to heavy usage.
    trigdist = ::get_option<bool>( "CIRCLEDIST" );
    use_tiles = ::get_option<bool>( "USE_TILES" );
    log_from_top = ::get_option<std::string>( "SIDEBAR_LOG_FLOW" ) == "new_top";
    message_ttl = ::get_option<int>( "MESSAGE_TTL" );
    fov_3d = ::get_option<bool>( "FOV_3D" );

    return write_to_file( savefile, [&]( std::ostream &fout ) {
        JsonOut jout( fout, true );
        serialize(jout);
    }, _( "options" ) );
}

void options_manager::load()
{
    const auto file = FILENAMES["options"];

    if( !read_from_file_optional( file, *this ) ) {
        if (load_legacy()) {
            if (save()) {
                remove_file(FILENAMES["legacy_options"]);
                remove_file(FILENAMES["legacy_options2"]);
            }
        }
    }

    // cache to global due to heavy usage.
    trigdist = ::get_option<bool>( "CIRCLEDIST" );
    use_tiles = ::get_option<bool>( "USE_TILES" );
    log_from_top = ::get_option<std::string>( "SIDEBAR_LOG_FLOW" ) == "new_top";
    message_ttl = ::get_option<int>( "MESSAGE_TTL" );
    fov_3d = ::get_option<bool>( "FOV_3D" );
}

bool options_manager::load_legacy()
{
    const auto reader = [&]( std::istream &fin ) {
        std::string sLine;
        while(!fin.eof()) {
            getline(fin, sLine);

            if(sLine != "" && sLine[0] != '#' && std::count(sLine.begin(), sLine.end(), ' ') == 1) {
                int iPos = sLine.find(' ');
                const std::string loadedvar = sLine.substr(0, iPos);
                const std::string loadedval = sLine.substr(iPos + 1, sLine.length());
                // option with values from post init() might get clobbered
                add_retry(loadedvar, loadedval); // stash it until update();

                global_options[ loadedvar ].setValue( loadedval );
            }
        }
    };

    return read_from_file_optional( FILENAMES["legacy_options"], reader ) ||
           read_from_file_optional( FILENAMES["legacy_options2"], reader );
}

bool use_narrow_sidebar()
{
    return TERMY < 25 || g->narrow_sidebar;
}

bool options_manager::has_option( const std::string &name ) const
{
    return global_options.count( name );
}

options_manager::cOpt &options_manager::get_option( const std::string &name )
{
    if( global_options.count( name ) == 0 ) {
        debugmsg( "requested non-existing option %s", name.c_str() );
    }
    return global_options[name];
}

options_manager::cOpt &options_manager::get_world_option( const std::string &name )
{
    if( !world_generator->active_world ) {
        // Global options contains the default for new worlds, which is good enough here.
        return get_option( name );
    }
    auto &wopts = world_generator->active_world->WORLD_OPTIONS;
    if( wopts.count( name ) == 0 ) {
        // May be a new option and an old world - import default from global options.
        wopts[name] = get_option( name );
    }
    return wopts[name];
}

std::unordered_map<std::string, options_manager::cOpt> options_manager::get_world_defaults() const
{
    std::unordered_map<std::string, cOpt> result;
    for( auto &elem : global_options ) {
        if( elem.second.getPage() == "world_default" ) {
            result.insert( elem );
        }
    }
    return result;
}
