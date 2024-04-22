#pragma once
#ifndef CATA_SRC_AUTO_NOTE_H
#define CATA_SRC_AUTO_NOTE_H

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cata_path.h"
#include "catacharset.h"
#include "color.h"
#include "map_extras.h"  // IWYU pragma: keep
#include "type_id.h"

namespace auto_notes
{

class custom_symbol
{
    private:
        std::optional<uint32_t> symbol;

        std::optional<nc_color> color;

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

        bool was_changed( bool bCharacter ) const;

        void set_cached_custom_symbol( const map_extra_id &mapExtId, const custom_symbol &symbol,
                                       bool bCharacter );

    private:
        /// The map extra type cache. This is initialized with all known map extra types
        /// and their auto note status with every call of initialize(). All changes to this
        /// will be applied to the per-character or global auto notes settings object after the user
        /// closes the GUI.
        std::unordered_map<map_extra_id, std::pair<const map_extra, bool>> char_mapExtraCache;
        std::unordered_map<map_extra_id, std::pair<const map_extra, bool>> global_mapExtraCache;

        /// All map extra types for the character that will be displayed in the GUI.
        std::vector<map_extra_id> char_displayCache;

        /// All map extra types for global types that will be displayed in the GUI.
        std::vector<map_extra_id> global_displayCache;

        std::unordered_map<map_extra_id, custom_symbol> char_custom_symbol_cache;
        std::unordered_map<map_extra_id, custom_symbol> global_custom_symbol_cache;

        void fill_custom_symbols_cache();

        bool charwasChanged{false};
        bool globalwasChanged{false};
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
        bool has_auto_note_enabled( const map_extra_id &mapExtId, bool bCharacter ) const;

        void set_auto_note_status( const map_extra_id &mapExtId, bool enabled, bool bCharacter );

        void set_discovered( const map_extra_id &mapExtId );

        bool was_discovered( const map_extra_id &mapExtId ) const;

        std::optional<custom_symbol> get_custom_symbol( const map_extra_id &mapExtId ) const;

        void set_custom_symbol( const map_extra_id &mapExtId, const custom_symbol &symbol,
                                bool bCharacter );

        void clear_all_custom_symbols( bool bCharacter );

        void load( bool bCharacter );

        bool save( bool bCharacter );

        void clear();

        void show_gui();

        /// Perform default initialization. This will use the auto note property of
        /// registered map extras in order to determine their enable status.
        void default_initialize();

    private:
        /// Build string containing path to the auto notes save file for the active player.
        cata_path build_save_path() const;

        /// This set contains the ID strings of all map extras that have auto note enabled for a character.
        std::unordered_set<map_extra_id> character_autoNoteEnabled;

        /// This set contains the ID strings of all map extras that have auto note disabled globally.
        std::unordered_set<map_extra_id> global_autoNoteDisabled;

        /// This set contains the ID strings of all map extras that were already encountered by the player.
        /// This is used in order to avoid spoilers in the GUI.
        std::unordered_set<map_extra_id> discovered;

        /// User-defined symbols and colors for the auto notes for this character.
        std::unordered_map<map_extra_id, custom_symbol> character_custom_symbols;

        /// User-defined symbols and colors for the auto notes globably.
        std::unordered_map<map_extra_id, custom_symbol> global_custom_symbols;
};
} // namespace auto_notes

auto_notes::auto_note_settings &get_auto_notes_settings();

#endif // CATA_SRC_AUTO_NOTE_H
