#pragma once
#ifndef CATA_SRC_AUTO_NOTE_H
#define CATA_SRC_AUTO_NOTE_H

#include <iosfwd>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "map_extras.h"
#include "string_id.h"

namespace auto_notes
{

class custom_symbol
{
    private:
        cata::optional<uint32_t> symbol;

        cata::optional<nc_color> color;

    public:
        std::string get_symbol_string() const {
            return symbol ? utf32_to_utf8( *symbol ) : "";
        };

        std::string get_color_string() const {
            return color ? get_all_colors().get_name( *color ) : "";
        };

        uint32_t get_symbol() const {
            return symbol ? *symbol : UTF8_getch( "N" );
        };

        nc_color get_color() const {
            return color ? *color : c_yellow;
        };

        void set_symbol( const std::string &str ) {
            if( !str.empty() ) {
                symbol = UTF8_getch( str );
            } else {
                symbol.reset();
            }
        };

        void set_color( const std::string &col ) {
            if( !col.empty() ) {
                color = get_all_colors().name_to_color( col );
            } else {
                color.reset();
            }
        };
};

/**
 * Class implementing the user interface for the auto note configuration.
 */
class auto_note_manager_gui
{
    public:
        auto_note_manager_gui();

        void show();

        bool was_changed() const;

        void set_cached_custom_symbol( const string_id<map_extra> &mapExtId, const custom_symbol &symbol );

    private:
        /// The map extra type cache. This is initialized with all known map extra types
        /// and their auto note status with every call of initialize(). All changes to this
        /// will be applied to the per-character auto notes settings object after the user
        /// closes the GUI.
        std::unordered_map<string_id<map_extra>, std::pair<const map_extra, bool>> mapExtraCache;

        /// All map extra types that will be displayed in the GUI.
        std::vector<string_id<map_extra>> displayCache;

        std::unordered_map<string_id<map_extra>, custom_symbol> custom_symbol_cache;

        void fill_custom_symbols_cache();

        bool wasChanged{false};
};

/**
 * A class storing auto note settings for all map features for a given player.
 * Since auto note is something that is very dependent on in which stage of the game
 * a player is (f.e., dead college kids are useful in the early game, but not so much
 * in the late game), the settings are only available tied to a specific game save, meaning
 * there is no global setting like with the auto-pickup manager.
 */
class auto_note_settings
{
        // Allow private member access to the auto note manager GUI
        friend class auto_note_manager_gui;

    public:
        bool has_auto_note_enabled( const string_id<map_extra> &mapExtId ) const;

        void set_auto_note_status( const string_id<map_extra> &mapExtId, bool enabled );

        void set_discovered( const string_id<map_extra> &mapExtId );

        bool was_discovered( const string_id<map_extra> &mapExtId ) const;

    public:
        cata::optional<custom_symbol> get_custom_symbol( const string_id<map_extra> &mapExtId ) const;

        void set_custom_symbol( const string_id<map_extra> &mapExtId, const custom_symbol &symbol );

        void clear_all_custom_symbols();

    public:
        void load();

        bool save();

        void clear();

        void show_gui();

        /// Perform default initialization. This will use the auto note property of
        /// registered map extras in order to determine their enable status.
        void default_initialize();

    private:
        /// Build string containing path to the auto notes save file for the active player.
        std::string build_save_path() const;

    private:
        /// This set contains the ID strings of all map extras that have auto note enabled.
        std::unordered_set<string_id<map_extra>> autoNoteEnabled;

        /// This set contains the ID strings of all map extras that were already encountered by the player.
        /// This is used in order to avoid spoilers in the GUI.
        std::unordered_set<string_id<map_extra>> discovered;

        /// User-defined symbols and colors for the auto notes.
        std::unordered_map<string_id<map_extra>, custom_symbol> custom_symbols;
};
} // namespace auto_notes

auto_notes::auto_note_settings &get_auto_notes_settings();

#endif // CATA_SRC_AUTO_NOTE_H
