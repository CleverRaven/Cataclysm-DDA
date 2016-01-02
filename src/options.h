#ifndef OPTIONS_H
#define OPTIONS_H

#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <algorithm> //atoi
#include "json.h"

class options_data
{
        friend struct regional_settings;
    public:
        void add_retry( const std::string &var, const std::string &val );
        void add_value( const std::string &myoption, const std::string &myval, std::string myvaltxt = "" );
        options_data();
    private:
        void enable_json( const std::string &var );
        std::map<std::string, std::string> post_json_verify;
};

extern options_data optionsdata;

class options_manager : public JsonSerializer, public JsonDeserializer
{
    private:
        static std::string build_tilesets_list();
        static std::string build_soundpacks_list();
        bool bIngame;

        enum copt_hide_t {
            /** Don't hide this option */
            COPT_NO_HIDE,
            /** Hide this option in SDL build */
            COPT_SDL_HIDE,
            /** Show this option in SDL builds only */
            COPT_CURSES_HIDE,
            /** Hide this option in non-Windows Curses builds */
            COPT_POSIX_CURSES_HIDE,
            /** Hide this option in builds without sound support */
            COPT_NO_SOUND_HIDE
        };

        bool load_legacy();

    public:
        class cOpt
        {
                friend class options_data;
            public:
                //Default constructor
                cOpt();

                //string select constructor
                cOpt( const std::string sPageIn, const std::string sMenuTextIn, const std::string sTooltipIn,
                      const std::string sItemsIn, std::string sDefaultIn, copt_hide_t opt_hide = COPT_NO_HIDE );

                //string input constructor
                cOpt( const std::string sPageIn, const std::string sMenuTextIn, const std::string sTooltipIn,
                      const std::string sDefaultIn, const int iMaxLengthIn, copt_hide_t opt_hide = COPT_NO_HIDE );

                //bool constructor
                cOpt( const std::string sPageIn, const std::string sMenuTextIn, const std::string sTooltipIn,
                      const bool bDefaultIn, copt_hide_t opt_hide = COPT_NO_HIDE );

                //int constructor
                cOpt( const std::string sPageIn, const std::string sMenuTextIn, const std::string sTooltipIn,
                      const int iMinIn, int iMaxIn, int iDefaultIn, copt_hide_t opt_hide = COPT_NO_HIDE );

                //float constructor
                cOpt( const std::string sPageIn, const std::string sMenuTextIn, const std::string sTooltipIn,
                      const float fMinIn, float fMaxIn, float fDefaultIn, float fStepIn,
                      copt_hide_t opt_hide = COPT_NO_HIDE );

                //Default deconstructor
                ~cOpt() {};

                void setSortPos( const std::string sPageIn );

                //helper functions
                int getSortPos();

                /**
                 * Option should be hidden in current build.
                 * @return true if option should be hidden, false if not.
                 */
                bool is_hidden();

                std::string getPage();
                std::string getMenuText();
                std::string getTooltip();
                std::string getType();

                std::string getValue();
                std::string getValueName();
                std::string getDefaultText( const bool bTranslated = true );

                int getItemPos( const std::string sSearch );

                int getMaxLength();

                //set to next item
                void setNext();
                //set to prev item
                void setPrev();
                //set value
                void setValue( std::string sSetIn );
                void setValue( float fSetIn );

                //Set default class behaviour to float
                operator float() const;
                // return integer via int
                explicit operator int() const;
                //allow (explicit) boolean conversions
                explicit operator bool() const;
                // if (class == "string")
                bool operator==( const std::string sCompare ) const;
                // if (class != "string")
                bool operator!=( const std::string sCompare ) const;

            private:
                std::string sPage;
                std::string sMenuText;
                std::string sTooltip;
                std::string sType;

                copt_hide_t hide;
                int iSortPos;

                //sType == "string"
                std::string sSet;
                std::vector<std::string> vItems;
                std::string sDefault;

                int iMaxLength;

                //sType == "bool"
                bool bSet;
                bool bDefault;

                //sType == "int"
                int iSet;
                int iMin;
                int iMax;
                int iDefault;

                //sType == "float"
                float fSet;
                float fMin;
                float fMax;
                float fDefault;
                float fStep;
        };

        void init();
        void load();
        bool save( bool ingame = false );
        void show( bool ingame = false );

        using JsonSerializer::serialize;
        void serialize( JsonOut &json ) const override;
        void deserialize( JsonIn &jsin ) override;
};

bool use_narrow_sidebar(); // short-circuits to on if terminal is too small

/** A mapping(string:string) that stores all tileset values.
 * Firsts string is tileset NAME from config.
 * Second string is directory that contain tileset.
 */
extern std::map<std::string, std::string> TILESETS;
/** A mapping(string:string) that stores all soundpack values.
 * Firsts string is soundpack NAME from config.
 * Second string is directory that contains soundpack.
 */
extern std::map<std::string, std::string> SOUNDPACKS;
extern std::unordered_map<std::string, options_manager::cOpt> OPTIONS;
extern std::unordered_map<std::string, options_manager::cOpt> ACTIVE_WORLD_OPTIONS;
extern std::map<int, std::vector<std::string> > mPageItems;
extern int iWorldOptPage;

options_manager &get_options();

std::string trim( const std::string &s ); // Remove spaces from the start and the end of a string

#endif
