#ifndef OPTIONS_H
#define OPTIONS_H

#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include "json.h"

class options_manager : public JsonSerializer, public JsonDeserializer
{
    private:
        static std::string build_tilesets_list();
        static std::string build_soundpacks_list();

        bool load_legacy();

        void enable_json( const std::string &var );
        void add_retry( const std::string &var, const std::string &val );

        std::map<std::string, std::string> post_json_verify;

        friend options_manager &get_options();
        options_manager();

    public:
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
            COPT_NO_SOUND_HIDE,
            /** Hide this option always, it is set as a mod. **/
            COPT_ALWAYS_HIDE
        };

        class cOpt
        {
                friend class options_manager;
            public:
                //Default constructor
                cOpt();

                //Default deconstructor
                ~cOpt() {};

                void setSortPos( const std::string sPageIn );

                //helper functions
                int getSortPos() const;

                /**
                 * Option should be hidden in current build.
                 * @return true if option should be hidden, false if not.
                 */
                bool is_hidden() const;

                std::string getName() const;
                std::string getPage() const;
                std::string getMenuText() const;
                std::string getTooltip() const;
                std::string getType() const;

                std::string getValue() const;
                std::string getValueName() const;
                std::string getDefaultText( const bool bTranslated = true ) const;

                int getItemPos( const std::string sSearch ) const;

                int getMaxLength() const;

                //set to next item
                void setNext();
                //set to prev item
                void setPrev();
                //set value
                void setValue( std::string sSetIn );
                void setValue( float fSetIn );
                void setValue( int iSetIn );

                template<typename T>
                T value_as() const;

                bool operator==( const cOpt &rhs ) const;
                bool operator!=( const cOpt &rhs ) const {
                    return !operator==( rhs );
                }

            private:
                std::string sName;
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
                std::map<int, std::string> mIntValues;

                //sType == "float"
                float fSet;
                float fMin;
                float fMax;
                float fDefault;
                float fStep;
        };

        void init();
        void load();
        bool save();
        void show( bool ingame = false );

        void add_value( const std::string &myoption, const std::string &myval,
                        const std::string &myvaltxt = "" );

        using JsonSerializer::serialize;
        void serialize( JsonOut &json ) const override;
        void deserialize( JsonIn &jsin ) override;

        /**
         * Returns a copy of the options in the "world default" page. The options have their
         * current value, which acts as the default for new worlds.
         */
        std::unordered_map<std::string, cOpt> get_world_defaults() const;

        /** Check if an option exists? */
        bool has_option( const std::string &name ) const;

        cOpt &get_option( const std::string &name );
        cOpt &get_world_option( const std::string &name );

        //add string select option
        void add( const std::string sNameIn, const std::string sPageIn,
                  const std::string sMenuTextIn, const std::string sTooltipIn,
                  const std::string sItemsIn, std::string sDefaultIn,
                  copt_hide_t opt_hide = COPT_NO_HIDE );

        //add string input option
        void add( const std::string sNameIn, const std::string sPageIn,
                  const std::string sMenuTextIn, const std::string sTooltipIn,
                  const std::string sDefaultIn, const int iMaxLengthIn,
                  copt_hide_t opt_hide = COPT_NO_HIDE );

        //add bool option
        void add( const std::string sNameIn, const std::string sPageIn,
                  const std::string sMenuTextIn, const std::string sTooltipIn,
                  const bool bDefaultIn, copt_hide_t opt_hide = COPT_NO_HIDE );

        //add int option
        void add( const std::string sNameIn, const std::string sPageIn,
                  const std::string sMenuTextIn, const std::string sTooltipIn,
                  const int iMinIn, int iMaxIn, int iDefaultIn,
                  copt_hide_t opt_hide = COPT_NO_HIDE );

        //add int map option
        void add( const std::string sNameIn, const std::string sPageIn,
                  const std::string sMenuTextIn, const std::string sTooltipIn,
                  const std::map<int, std::string> mIntValuesIn, int iInitialIn,
                  int iDefaultIn, copt_hide_t opt_hide = COPT_NO_HIDE );

        //add float option
        void add( const std::string sNameIn, const std::string sPageIn,
                  const std::string sMenuTextIn, const std::string sTooltipIn,
                  const float fMinIn, float fMaxIn,
                  float fDefaultIn, float fStepIn,
                  copt_hide_t opt_hide = COPT_NO_HIDE );

    private:
        std::unordered_map<std::string, cOpt> global_options;
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
extern std::map<int, std::vector<std::string> > mPageItems;
extern int iWorldOptPage;

options_manager &get_options();

template<typename T>
inline T get_option( const std::string &name )
{
    return get_options().get_option( name ).value_as<T>();
}

template<typename T>
inline T get_world_option( const std::string &name )
{
    return get_options().get_world_option( name ).value_as<T>();
}

#endif
