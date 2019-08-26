#pragma once
#ifndef AUTO_NOTE_H
#define AUTO_NOTE_H

#include <string>
#include <unordered_set>
#include <utility>
#include <unordered_map>
#include <vector>

#include "string_id.h"
#include "map_extras.h"


namespace auto_notes
{
/**
 * Class implementing the user interface for the auto note configuration.
 */
class auto_note_manager_gui
{
    public:
        auto_note_manager_gui();

        void show();

        bool was_changed() const;

    private:
        /// The map extra type cache. This is initialized with all known map extra types
        /// and their auto note status with every call of initialize(). All changes to this
        /// will be applied to the per-character auto notes settings object after the user
        /// closes the GUI.
        std::unordered_map<string_id<map_extra>, std::pair<const map_extra, bool>> mapExtraCache;

        /// All map extra types that will be displayed in the GUI.
        std::vector<string_id<map_extra>> displayCache;

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
};
} // namespace auto_notes

auto_notes::auto_note_settings &get_auto_notes_settings();

#endif // AUTO_NOTE_H
