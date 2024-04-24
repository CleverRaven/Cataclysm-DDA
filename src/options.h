#pragma once
#ifndef CATA_SRC_OPTIONS_H
#define CATA_SRC_OPTIONS_H

#include <algorithm>
#include <functional>
#include <iosfwd>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "translations.h"
#include "type_id.h"

class cata_path;
class JsonArray;
class JsonOut;
class JsonObject;

class options_manager
{
    public:
        // first is internal value, second is text
        using id_and_option = std::pair<std::string, translation>;
        using int_and_option = std::pair<int, translation>;
        static std::vector<id_and_option> get_lang_options();
    private:
        /**
         * Search for resources.
         * @p storage is the resource map (e.g. SOUNDPACKS) to fill from resources found. It will be cleared.
         * @p option_list is the option list corresponding to the resources found. It will be cleared.
         * @p search_paths are the paths to search for resources in, in order of priority.
         * @p resource_name is the type of resource being searched for (e.g. "sound").
         * @p resource_filename is the name of the config file for the type of resource (e.g. path_info::soundpack_conf()).
         */
        static void search_resource(
            std::map<std::string, std::string> &storage, std::vector<id_and_option> &option_list,
            const std::vector<std::string> &search_paths, const std::string &resource_name,
            const std::string &resource_filename );
        static void search_resource(
            std::map<std::string, cata_path> &storage, std::vector<id_and_option> &option_list,
            const std::vector<cata_path> &search_paths, const std::string &resource_name,
            const std::string &resource_filename );
        static std::vector<id_and_option> build_tilesets_list();
        static std::vector<id_and_option> build_soundpacks_list();
        static std::unordered_set<std::string> get_langs_with_translation_files();

        void enable_json( const std::string &var );
        void add_retry( const std::string &var, const std::string &val );

        void update_global_locale();

        std::map<std::string, std::string> post_json_verify; // NOLINT(cata-serialize)

        friend options_manager &get_options();
        options_manager();

        void addOptionToPage( const std::string &name, const std::string &page );

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
                std::optional<int_and_option> findInt( int iSearch ) const;

                int getMaxLength() const;

                //set to next item
                void setNext();
                //set to previous item
                void setPrev();
                //set value
                void setValue( const std::string &sSetIn );
                void setValue( float fSetIn );
                void setValue( int iSetIn );

                template<typename T>
                T value_as( bool convert = false ) const;

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
                // The displayed option name ( short string ).
                translation sMenuText;
                // The displayed option tool tip ( longer string ).
                translation sTooltip;
                std::string sType;
                bool verbose = false;

                std::string format;

                std::string sPrerequisite;
                std::vector<std::string> sPrerequisiteAllowedValues;

                copt_hide_t hide;

                COPT_VALUE_TYPE eType;

                //sType == "string"
                std::string sSet;
                std::vector<id_and_option> vItems;
                std::string sDefault;

                int iMaxLength = 0;

                //sType == "bool"
                bool bSet = false;
                bool bDefault = false;

                //sType == "int"
                int iSet = 0;
                int iMin = 0;
                int iMax = 0;
                int iDefault = 0;
                std::vector<int_and_option> mIntValues;

                //sType == "float"
                float fSet = 0.0f;
                float fMin = 0.0f;
                float fMax = 0.0f;
                float fDefault = 0.0f;
                float fStep = 0.0f;

                template<typename T>
                std::optional<T> _convert() const;
        };

        using options_container = std::unordered_map<std::string, cOpt>;

        void init();
        void add_options_general();
        void add_options_interface();
        void add_options_graphics();
        void add_options_world_default();
        void add_options_debug();
        void add_options_android();
        void load();
        bool save() const;
        std::string show( bool ingame = false, bool world_options_only = false, bool with_tabs = true );

        void add_value( const std::string &lvar, const std::string &lval,
                        const translation &lvalname );

        void serialize( JsonOut &json ) const;
        void deserialize( const JsonArray &ja );

        std::string migrateOptionName( const std::string &name ) const;
        std::string migrateOptionValue( const std::string &name, const std::string &val ) const;

        // updates the caches in options_cache.h
        static void update_options_cache();

        /**
         * Returns a copy of the options in the "world default" page. The options have their
         * current value, which acts as the default for new worlds.
         */
        options_container get_world_defaults() const;

        void set_world_options( options_container *options );

        /** Check if an option exists? */
        bool has_option( const std::string &name ) const;

        cOpt &get_option( const std::string &name );

        //add hidden external option with value
        void add_external( const std::string &sNameIn, const std::string &sPageIn, const std::string &sType,
                           const translation &sMenuTextIn, const translation &sTooltipIn );

        //add string select option
        void add( const std::string &sNameIn, const std::string &sPageIn,
                  const translation &sMenuTextIn, const translation &sTooltipIn,
                  // first is option value, second is display name of that value
                  const std::vector<id_and_option> &sItemsIn, std::string sDefaultIn,
                  copt_hide_t opt_hide = COPT_NO_HIDE );

        //add string input option
        void add( const std::string &sNameIn, const std::string &sPageIn,
                  const translation &sMenuTextIn, const translation &sTooltipIn,
                  const std::string &sDefaultIn, int iMaxLengthIn,
                  copt_hide_t opt_hide = COPT_NO_HIDE );

        //add bool option
        void add( const std::string &sNameIn, const std::string &sPageIn,
                  const translation &sMenuTextIn, const translation &sTooltipIn,
                  bool bDefaultIn, copt_hide_t opt_hide = COPT_NO_HIDE );

        //add int option
        void add( const std::string &sNameIn, const std::string &sPageIn,
                  const translation &sMenuTextIn, const translation &sTooltipIn,
                  int iMinIn, int iMaxIn, int iDefaultIn,
                  copt_hide_t opt_hide = COPT_NO_HIDE,
                  const std::string &format = "%i" );

        //add int map option
        void add( const std::string &sNameIn, const std::string &sPageIn,
                  const translation &sMenuTextIn, const translation &sTooltipIn,
                  const std::vector<int_and_option> &mIntValuesIn,
                  int iInitialIn, int iDefaultIn, copt_hide_t opt_hide = COPT_NO_HIDE,
                  bool verbose = false );

        //add float option
        void add( const std::string &sNameIn, const std::string &sPageIn,
                  const translation &sMenuTextIn, const translation &sTooltipIn,
                  float fMinIn, float fMaxIn,
                  float fDefaultIn, float fStepIn,
                  copt_hide_t opt_hide = COPT_NO_HIDE,
                  const std::string &format = "%.2f" );

    private:
        options_container options;
        std::optional<options_container *> world_options; // NOLINT(cata-serialize)

        /** Option group. */
        class Group
        {
            public:
                /** Group identifier. Should be unique across all pages. */
                std::string id_;
                /** Group name */
                translation name_;
                /** Tooltip with description */
                translation tooltip_;

                Group() = default;
                Group( const std::string &id, const translation &name, const translation &tooltip )
                    : id_( id ), name_( name ), tooltip_( tooltip ) { }
        };

        /** Page item type. */
        enum class ItemType {
            BlankLine,
            GroupHeader,
            Option,
        };

        /** Single page item (entry). */
        class PageItem
        {
            public:
                ItemType type;
                std::string data;
                /** Empty if not assigned to any group. */
                std::string group;

                PageItem() : type( ItemType::BlankLine ) { }
                PageItem( ItemType type, const std::string &data, const std::string &group )
                    : type( type ), data( data ), group( group ) { }

                std::string fmt_tooltip( const Group &group, const options_container &cont ) const;
        };

        /**
         * A page (or tab) to be displayed in the options UI.
         * It contains a @ref id that is used to detect what options should go into this
         * page (see @ref cOpt::getPage).
         * It also has a name that will be translated and displayed.
         * And it has items, each item is either nothing (will be represented as empty line
         * in the UI) or the name of an option.
         */
        class Page
        {
            public:
                /** Page identifier */
                std::string id_;
                /** Page name */
                translation name_;
                /** Page items (entries) */
                std::vector<PageItem> items_;

                void removeRepeatedEmptyLines();

                Page( const std::string &id, const translation &name ) : id_( id ), name_( name ) { }
        };

        std::vector<Page> pages_; // NOLINT(cata-serialize)
        std::string adding_to_group_; // NOLINT(cata-serialize)
        std::vector<Group> groups_; // NOLINT(cata-serialize)

        /**
        * Specify option group.
        *
        * Option groups are used for visual separation of options on pages,
        * and allow some additional UI functionality (i.e. collapse/expand).
        *
        * @param page_id Page to create group at.
        * @param group Group to create.
        * @param entries Page entries added within this closure will be assigned to the group.
        *                Receives "page_id" as it's only argument.
        */
        void add_option_group( const std::string &page_id, const Group &group,
                               const std::function<void( const std::string & )> &entries );

        /** Add empty line to page. */
        void add_empty_line( const std::string &sPageIn );

        /** Find group by id. */
        const Group &find_group( const std::string &id ) const;
};

struct option_slider {
    private:
        struct option_slider_level {
            private:
                struct opt_slider_option {
                    std::string _opt;
                    std::string _type;
                    std::string _val;
                    opt_slider_option( const std::string &opt, const std::string &type, const std::string &val ) :
                        _opt( opt ), _type( type ), _val( val ) {}
                };
                translation _name;
                translation _desc;
                int _level = 0;
                std::vector<opt_slider_option> _opts;

            public:
                option_slider_level() = default;
                option_slider_level( const translation &name, const translation &desc, int level ) :
                    _name( name ), _desc( desc ), _level( level ) {}

                const translation &name() const {
                    return _name;
                }

                const translation &desc() const {
                    return _desc;
                }

                int level() const {
                    return _level;
                }

                void add( const std::string &opt, const std::string &type, const std::string &val ) {
                    _opts.emplace_back( opt, type, val );
                }

                bool remove( const std::string &opt );
                void apply_opts( options_manager::options_container &OPTIONS ) const;
                void deserialize( const JsonObject &jo );
        };

        translation _name;
        std::vector<option_slider_level> _levels;
        std::string _context;
        int _default_level = 0;

    public:
        option_slider_id id;
        bool was_loaded = false;

        static void load_option_sliders( const JsonObject &jo, const std::string &src );
        static void reset();
        static void finalize_all();
        static void check_consistency();
        void load( const JsonObject &jo, std::string_view src );
        void check() const;
        static const std::vector<option_slider> &get_all();

        void reorder_opts() {
            std::sort( _levels.begin(), _levels.end(),
            []( const option_slider_level & a, const option_slider_level & b ) {
                return a.level() < b.level();
            } );
        }

        const translation &name() const {
            return _name;
        }

        const std::string &context() const {
            return _context;
        }

        int default_level() const {
            return _default_level;
        }

        int count() const {
            return _levels.size();
        }

        void apply_opts( int level, options_manager::options_container &OPTIONS ) const {
            if( level >= 0 && level < static_cast<int>( _levels.size() ) ) {
                _levels[level].apply_opts( OPTIONS );
            }
        }

        const translation &level_name( int level ) const {
            static const translation none;
            if( level >= 0 && level < static_cast<int>( _levels.size() ) ) {
                return _levels[level].name();
            }
            return none;
        }

        const translation &level_desc( int level ) const {
            static const translation none;
            if( level >= 0 && level < static_cast<int>( _levels.size() ) ) {
                return _levels[level].desc();
            }
            return none;
        }

        int random_level() const;
};

/** A mapping(string:string) that stores all tileset values.
 * Firsts string is tileset NAME from config.
 * Second string is directory that contain tileset.
 */
extern std::map<std::string, cata_path> TILESETS;
/** A mapping(string:string) that stores all soundpack values.
 * Firsts string is soundpack NAME from config.
 * Second string is directory that contains soundpack.
 */
extern std::map<std::string, cata_path> SOUNDPACKS;

options_manager &get_options();

inline bool has_option( const std::string &name )
{
    return get_options().has_option( name );
}

template<typename T>
inline T get_option( const std::string &name, bool convert = false )
{
    return get_options().get_option( name ).value_as<T>( convert );
}

#endif // CATA_SRC_OPTIONS_H
