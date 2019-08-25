#pragma once
#ifndef AUTO_NOTE_H
#define AUTO_NOTE_H

#include <string>
#include <unordered_set>

namespace auto_notes
{
	/**
	 * Class implementing the user interface for the auto note configuration.
	 */
 	class auto_note_manager_gui
	{
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
			/// Check whether the user has enabled auto note for given map extra ID string.
			bool has_auto_note_enabled(const std::string& mapExtId) const;
			
			/// Update the auto note enabled status for given map extra ID string.
			void set_auto_note_status(const std::string& mapExtId, const bool enabled);
		
		public:
			/// Load auto note settings state from player save.
			void load();
			
			/// Save auto note settings state to player save.
			bool save();
			
			/// Clear all stored data
			void clear();
			
		private:
			/// Build string containing path to the auto notes save file for the active player.
			std::string build_save_path() const;
		
		private:
			/// This set contains the ID strings of all map extras that have auto note enabled.
			std::unordered_set<std::string> autoNoteEnabled;
	};	
} // namespace auto_notes

/// Function providing current auto note settings object
auto_notes::auto_note_settings& get_auto_notes_settings();

#endif // AUTO_NOTE_H
