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
    private:
        /// A type used to store each map extra type and its auto note enabled status
        using cache_t = std::map<std::string, std::pair<const map_extra, bool>>;

        /// A type used to store all map extra type ids that are to be displayed in the GUI.
        /// This might differ from the cache, since we might hide some extra types in order
        /// to avoid spoilers.
        using display_cache_t = std::vector<std::string>;

    public:
        auto_note_manager_gui();

        /// Initializes the internal GUI state. Should be called every time this GUI is opened.
        void initialize();

        /// Show the gui.
        void show();

        /// Returns flag indicating if any of the auto note settings were changed.
        bool was_changed();

    private:
        /// The map extra type cache. This is initialized with all known map extra types
        /// and their auto note status with every call of initialize(). All changes to this
        /// will be applied to the per-character auto notes settings object after the user
        /// closes the GUI.
        cache_t mapExtraCache;

        /// All map extra types that will be displayed in the GUI.
        display_cache_t displayCache;

        /// Flag indicating whether any of the auto note configuration data was changed using this GUI
        /// instance.
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
        /// Check whether the user has enabled auto note for given map extra ID
        bool has_auto_note_enabled( const string_id<map_extra> &mapExtId ) const;

        /// Update the auto note enabled status for given map extra ID
        void set_auto_note_status( const string_id<map_extra> &mapExtId, bool enabled );

        /// Set given map extra type to discovered.
        void set_discovered( const string_id<map_extra> &mapExtId );

        /// Check if given map extra type was already discovered by the player.
        bool was_discovered( const string_id<map_extra> &mapExtId ) const;

    protected:
        /// Check whether the user has enabled auto note for given map extra ID string.
        bool has_auto_note_enabled( const std::string &mapExtId ) const;

        /// Update the auto note enabled status for given map extra ID string.
        void set_auto_note_status( const std::string &mapExtId, bool enabled );

    public:
        /// Load auto note settings state from player save.
        void load();

        /// Save auto note settings state to player save.
        bool save();

        /// Clear all stored data
        void clear();

        /// Show the configuration GUI
        void show_gui();

        /// Perform default initialization. This will use the auto note property of
        /// registered map extras in order to determine their enable status.
        void default_initialize();

    private:
        /// Build string containing path to the auto notes save file for the active player.
        std::string build_save_path() const;

    private:
        /// This set contains the ID strings of all map extras that have auto note enabled.
        std::unordered_set<std::string> autoNoteEnabled;

        /// This set contains the ID strings of all map extras that were already encountered by the player.
        /// This is used in order to avoid spoilers in the GUI.
        std::unordered_set<std::string> discovered;
};
} // namespace auto_notes

/// Function providing current auto note settings object
auto_notes::auto_note_settings &get_auto_notes_settings();

#endif // AUTO_NOTE_H
