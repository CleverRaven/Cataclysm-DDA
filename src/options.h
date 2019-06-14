#pragma once

#ifndef OPTIONS_H
#define OPTIONS_H

#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "translations.h"
#include "json.h"

/** A mapping(string:string) that stores all tileset values.
 * Firsts string is tileset NAME from config.
 * Second string is directory that contain tileset.
 */
extern std::map <std::string, std::string> TILESETS;
/** A mapping(string:string) that stores all soundpack values.
 * Firsts string is soundpack NAME from config.
 * Second string is directory that contains soundpack.
 */
extern std::map <std::string, std::string> SOUNDPACKS;

enum ETypeOption
{
    TYPE_UNKNOWN,
    TYPE_BOOL,
    TYPE_FLOAT,
    TYPE_STRING_SELECT,
    TYPE_STRING_INPUT,
    TYPE_INT_MAP,
    TYPE_INT,
    TYPE_VOID
};

enum copt_hide_t
{
    /** Don't hide this option */
            COPT_NO_HIDE,
    /** Hide this option in SDL build */
            COPT_SDL_HIDE,
    /** Show this option in SDL builds only */
            COPT_CURSES_HIDE,
    /** Hide this option in non-Windows Curses builds */
            COPT_POSIX_CURSES_HIDE,
    /** Hide this option in builds without sound support */
            COPT_NO_SOUND_HIDE,
    /** Hide this option always, it should not be changed by user directly through UI. **/
            COPT_ALWAYS_HIDE
};

class id_and_option : public std::pair <std::string, translation>
{

public:

    id_and_option( const std::string &first, const std::string &second )
            : std::pair <std::string, translation>( first, second )
    {
    }

    id_and_option( const std::string &first, const translation &second )
            : std::pair <std::string, translation>( first, second )
    {
    }
};

class OptionsContainer
{

public:

    // ---- Fields ----

    std::string sName;
    std::string sPage;
    // The *untranslated* displayed option name ( short string ).
    std::string sMenuText;
    // The *untranslated* displayed option tool tip ( longer string ).
    std::string sTooltip;
    std::string sType;
    std::string sPrerequisite;
    std::string sDefault;
    std::string format;
    //sType == "string"
    std::string sSet;

    int iSortPos;
    int iMaxLength;

    int iSet;
    int iMin;
    int iMax;
    int iDefault;

    float fSet;
    float fMin;
    float fMax;
    float fDefault;
    float fStep;

    bool bSet;
    bool bDefault;
    bool verbose;

    std::vector <std::string> sPrerequisiteAllowedValues;
    // first is internal value, second is untranslated text
    std::vector< std::tuple<int, std::string> > mIntValues;
    std::vector <id_and_option> vItems;

    copt_hide_t hide;
    ETypeOption eType;

    // ---- Constructs ----

    OptionsContainer( );

    // ---- Static Functions ----

    static std::vector <std::string> getPrerequisiteSupportedTypes( )
    {
        return { "bool", "string", "string_select", "string_input" };
    };

    // ---- Functions ----

    std::string getPage( ) const;

    /// The translated displayed option name.
    std::string getMenuText( ) const;

    /// The translated displayed option tool tip.
    std::string getTooltip( ) const;

    std::string getType( ) const;

    std::string getPrerequisite( ) const;

    std::string getDefaultText( const bool bTranslated = true ) const;

    std::string getValue( bool classis_locale = false ) const;

    /// The translated currently selected option value.
    std::string getValueName( ) const;

    //helper functions
    int getSortPos( ) const;

    int getItemPos( const std::string &sSearch ) const;

    int getIntPos( const int iSearch ) const;

    /**
     * Option should be hidden in current build.
     * @return true if option should be hidden, false if not.
     */
    bool isHidden( ) const;

    bool hasPrerequisite( ) const;

    bool checkPrerequisite( ) const;

    void setSortPos( const std::string &sPageIn );

    //set to next item
    void setNextItem( );

    void setPreviousItem( );

    void setValue( std::string sSetIn );

    void setValue( float fSetIn );

    void setValue( int iSetIn );

    void setPrerequisites( const std::string &sOption, const std::vector <std::string> &sAllowedValues );

    void setPrerequisite( const std::string &sOption, const std::string &sAllowedValue = "true" )
    {
        setPrerequisites( sOption, { sAllowedValue } );
    }

    std::vector <id_and_option> getItems( ) const;

    cata::optional< std::tuple<int, std::string> > findInt( const int iSearch ) const;

    // ---- Operators ----

    bool operator==( const OptionsContainer &rhs ) const;

    bool operator!=( const OptionsContainer &rhs ) const
    {
        return !operator==( rhs );
    }

    // ---- Templates ----

    template <typename T>
    T value_as( ) const;
};

class OptionsManager
{

private:

    // ---- Static Fields ----

    static OptionsManager *instance;

    // ---- Fields ----

    int iWorldOptPage;

    /*
     * First is page id, second is untranslated page name
     */
    std::vector <std::pair <std::string, std::string>> vectorPages;

    std::map <std::string, std::pair <std::string, std::map <std::string, std::string> > > mMigrateOption;

    std::map <int, std::vector <std::string>> mPageItems;

    std::map <std::string, std::string> post_json_verify;

    std::unordered_map <std::string, OptionsContainer> options;

    // ---- Static Functions ----

    static std::vector <id_and_option> build_tilesets_list( );

    static std::vector <id_and_option> build_soundpacks_list( );

    static std::vector <id_and_option> load_soundpack_from(
    const std::string &path );

    // ---- Functions ----

    void fillVectorWithOptionsGeneral( );

    void fillVectorWithOptionsInterface( );

    void fillVectorWithoptionsGraphics( );

    void fillVectorWithOptionsDebug( );

    void fillVectorWithOptionsWorldDefault( );

    void fillVectorWithOptionsAndroid( );

    void enable_json( const std::string &var );

    void add_retry( const std::string &var, const std::string &val );

    // ---- Constructs ----

    OptionsManager( );

public:

    // ---- Fields ----

    std::unordered_map <std::string, OptionsContainer> *world_options;

    // ---- Static Functions ----

    static OptionsManager *getInstance( );

    // ---- Functions ----

    void init( );

    void load( );

    void add_value( const std::string &myoption, const std::string &myval,
                    const translation &myvaltxt );

    void serialize( JsonOut &json ) const;

    void deserialize( JsonIn &jsin );

    bool save( );

    std::string show( bool ingame = false, bool world_options_only = false );

    std::string migrateOptionName( const std::string &name ) const;

    std::string migrateOptionValue( const std::string &name, const std::string &val ) const;

    /**
     * Returns a copy of the options in the "world default" page. The options have their
     * current value, which acts as the default for new worlds.
     */
    std::unordered_map <std::string, OptionsContainer> get_world_defaults( ) const;

    /** Check if an option exists? */
    bool has_option( const std::string &name ) const;

    OptionsContainer &get_option( const std::string &name );

    //add hidden external option with value
    void add_external( const std::string &sNameIn, const std::string &sPageIn,
            const std::string &sType, const std::string &sMenuTextIn,
            const std::string &sTooltipIn );

    //add string select option
    void add( const std::string &sNameIn, const std::string &sPageIn,
              const std::string &sMenuTextIn, const std::string &sTooltipIn,
            // first is option value, second is display name of that value
              const std::vector <id_and_option> &sItemsIn, std::string sDefaultIn,
              copt_hide_t opt_hide = COPT_NO_HIDE );

    //add string input option
    void add( const std::string &sNameIn, const std::string &sPageIn,
              const std::string &sMenuTextIn, const std::string &sTooltipIn,
              const std::string &sDefaultIn, int iMaxLengthIn,
              copt_hide_t opt_hide = COPT_NO_HIDE );

    //add bool option
    void add( const std::string &sNameIn, const std::string &sPageIn,
              const std::string &sMenuTextIn, const std::string &sTooltipIn,
              bool bDefaultIn, copt_hide_t opt_hide = COPT_NO_HIDE );

    //add int option
    void add( const std::string &sNameIn, const std::string &sPageIn,
              const std::string &sMenuTextIn, const std::string &sTooltipIn,
              int iMinIn, int iMaxIn, int iDefaultIn,
              copt_hide_t opt_hide = COPT_NO_HIDE,
              const std::string &format = "%i" );

    //add int map option
    void add( const std::string &sNameIn, const std::string &sPageIn,
              const std::string &sMenuTextIn, const std::string &sTooltipIn,
              const std::map <int, std::string> &mIntValuesIn, int iInitialIn,
              int iDefaultIn, copt_hide_t opt_hide = COPT_NO_HIDE );
        //add int map option
        void add( const std::string &sNameIn, const std::string &sPageIn,
                  const std::string &sMenuTextIn, const std::string &sTooltipIn,
                  const std::vector< std::tuple<int, std::string> > &mIntValuesIn,
                  int iInitialIn, int iDefaultIn, copt_hide_t opt_hide = COPT_NO_HIDE,
                  bool verbose = false );

    //add float option
    void add( const std::string &sNameIn, const std::string &sPageIn,
              const std::string &sMenuTextIn, const std::string &sTooltipIn,
              float fMinIn, float fMaxIn, float fDefaultIn, float fStepIn,
              copt_hide_t opt_hide = COPT_NO_HIDE,
              const std::string &format = "%.2f" );

};

template <typename T>
inline T get_option( const std::string &name )
{
    return OptionsManager::getInstance()->get_option( name ).value_as <T>( );
}

#endif
