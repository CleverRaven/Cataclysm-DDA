#pragma once
#ifndef OPTIONS_H
#define OPTIONS_H

#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <tuple>

#include "translations.h"
#include "optional.h"

class JsonIn;
class JsonOut;

class options_manager
{
    public:
        class id_and_option : public std::pair<std::string, translation>
        {
            public:
                id_and_option( const std::string &first, const std::string &second )
                    : std::pair<std::string, translation>( first, translation( second ) ) {
                }
                id_and_option( const std::string &first, const translation &second )
                    : std::pair<std::string, translation>( first, second ) {
                }
        };
    private:
        static std::vector<id_and_option> build_tilesets_list();
        static std::vector<id_and_option> build_soundpacks_list();
        static std::vector<id_and_option> load_soundpack_from(
            const std::string &path );

        bool load_legacy();

        void enable_json( const std::string &var );
        void add_retry( const std::string &var, const std::string &val );

        void update_global_locale();

        std::map<std::string, std::string> post_json_verify;

        std::map<std::string, std::pair<std::string, std::map<std::string, std::string> > > mMigrateOption;

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
            /** Hide this option always, it should not be changed by user directly through UI. **/
            COPT_ALWAYS_HIDE
        };

        class cOpt
        {
                friend class options_manager;
            public:
                cOpt();

                void setSortPos( const std::string &sPageIn );

                //helper functions
                int getSortPos() const;

                /**
                 * Option should be hidden in current build.
                 * @return true if option should be hidden, false if not.
                 */
                bool is_hidden() const;

                std::string getName() const;
                std::string getPage() const;
                /// The translated displayed option name.
                std::string getMenuText() const;
                /// The translated displayed option tool tip.
                std::string getTooltip() const;
                std::string getType() const;

                std::string getValue( bool classis_locale = false ) const;
                /// The translated currently selected option value.
                std::string getValueName() const;
                std::string getDefaultText( bool bTranslated = true ) const;

                int getItemPos( const std::string &sSearch ) const;
                std::vector<id_and_option> getItems() const;

                int getIntPos( int iSearch ) const;
                cata::optional< std::tuple<int, std::string> > findInt( int iSearch ) const;

                int getMaxLength() const;

                //set to next item
                void setNext();
                //set to previous item
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

                static std::vector<std::string> getPrerequisiteSupportedTypes() {
                    return { "bool", "string", "string_select", "string_input" };
                }

                void setPrerequisites( const std::string &sOption, const std::vector<std::string> &sAllowedValues );
                void setPrerequisite( const std::string &sOption, const std::string &sAllowedValue = "true" ) {
                    setPrerequisites( sOption, { sAllowedValue } );
                }
                std::string getPrerequisite() const;
                bool hasPrerequisite() const;
                bool checkPrerequisite() const;

                enum COPT_VALUE_TYPE {
                    CVT_UNKNOWN = 0,
                    CVT_BOOL = 1,
                    CVT_STRING = 2,
                    CVT_FLOAT = 3,
                    CVT_INT = 4,
                    CVT_VOID = 5
                };

            private:
                std::string sName;
                std::string sPage;
                // The *untranslated* displayed option name ( short string ).
                std::string sMenuText;
                // The *untranslated* displayed option tool tip ( longer string ).
                std::string sTooltip;
                std::string sType;
                bool verbose;

                std::string format;

                std::string sPrerequisite;
                std::vector<std::string> sPrerequisiteAllowedValues;

                copt_hide_t hide;
                int iSortPos;

                COPT_VALUE_TYPE eType;

                //sType == "string"
                std::string sSet;
                // first is internal value, second is untranslated text
                std::vector<id_and_option> vItems;
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
                std::vector< std::tuple<int, std::string> > mIntValues;

                //sType == "float"
                float fSet;
                float fMin;
                float fMax;
                float fDefault;
                float fStep;
        };

        using options_container = std::unordered_map<std::string, cOpt>;

        void init();
        void add_options_general();
        void add_options_interface();
        void add_options_graphics();
        void add_options_debug();
        void add_options_world_default();
        void add_options_android();
        void load();
        bool save();
        std::string show( bool ingame = false, bool world_options_only = false );

        void add_value( const std::string &lvar, const std::string &lval,
                        const translation &lvalname );

        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );

        std::string migrateOptionName( const std::string &name ) const;
        std::string migrateOptionValue( const std::string &name, const std::string &val ) const;

        /**
         * Returns a copy of the options in the "world default" page. The options have their
         * current value, which acts as the default for new worlds.
         */
        options_container get_world_defaults() const;
        std::vector<std::string> getWorldOptPageItems() const;

        options_container *world_options;

        /** Check if an option exists? */
        bool has_option( const std::string &name ) const;

        cOpt &get_option( const std::string &name );

        //add hidden external option with value
        void add_external( const std::string &sNameIn, const std::string &sPageIn, const std::string &sType,
                           const std::string &sMenuTextIn, const std::string &sTooltipIn );

        //add string select option
        void add( const std::string &sNameIn, const std::string &sPageIn,
                  const std::string &sMenuTextIn, const std::string &sTooltipIn,
                  // first is option value, second is display name of that value
                  const std::vector<id_and_option> &sItemsIn, std::string sDefaultIn,
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
                  const std::vector< std::tuple<int, std::string> > &mIntValuesIn,
                  int iInitialIn, int iDefaultIn, copt_hide_t opt_hide = COPT_NO_HIDE,
                  bool verbose = false );

        //add float option
        void add( const std::string &sNameIn, const std::string &sPageIn,
                  const std::string &sMenuTextIn, const std::string &sTooltipIn,
                  float fMinIn, float fMaxIn,
                  float fDefaultIn, float fStepIn,
                  copt_hide_t opt_hide = COPT_NO_HIDE,
                  const std::string &format = "%.2f" );

    private:
        options_container options;
        // first is page id, second is untranslated page name
        std::vector<std::pair<std::string, std::string>> vPages;
        std::map<int, std::vector<std::string>> mPageItems;
        int iWorldOptPage;
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

options_manager &get_options();

template<typename T>
inline T get_option( const std::string &name )
{
    return get_options().get_option( name ).value_as<T>();
}

#endif
