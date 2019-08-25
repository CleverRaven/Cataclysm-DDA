#include "auto_note.h"

#include <iostream>


#include "game.h"
#include "json.h"
#include "string_formatter.h"
#include "color.h"
#include "cursesdef.h"
#include "input.h"
#include "options.h"
#include "cata_utility.h"
#include "debug.h"
#include "filesystem.h"
#include "avatar.h"

namespace auto_notes
{
	std::string auto_note_settings::build_save_path() const
	{
		return g->get_player_base_save_path() + ".ano.json";
	}

	void auto_note_settings::clear()
	{
		this->autoNoteEnabled.clear();
	}

	bool auto_note_settings::save()
	{	
		// Check if the player is already saved.
		if(!file_exist(g->get_player_base_save_path() + ".sav"))
			return true; // Nothing to do here
			
		// Serialize contents of map ext set as array of strings
		return write_to_file(this->build_save_path(), [&](std::ostream& fstr) {
			JsonOut jout{ fstr, true };
			
			jout.start_array();		
			for(const std::string& entry: this->autoNoteEnabled)
			{
				jout.write(entry);
			}	
			jout.end_array();
			
		}, _("auto notes configuration"));
	}
	
	void auto_note_settings::load()
	{
		// Clear all remaining settings
		this->clear();
	
		// Lambda used inside the read from file call
		const auto parseJson = [&](JsonIn& jin) {
			jin.start_array();
			
			while(!jin.end_array()) {
				const std::string entry = jin.get_string();
				this->autoNoteEnabled.insert(entry);
			}	
		};
		
		// Actually read save file
		read_from_file_optional_json(this->build_save_path(), parseJson);
	}
	
	bool auto_note_settings::has_auto_note_enabled(const std::string& mapExtId) const
	{
		return this->autoNoteEnabled.count(mapExtId) != 0;
	}
					
	void auto_note_settings::set_auto_note_status(const std::string& mapExtId, const bool enabled)
	{
		if(enabled)
		{
			this->autoNoteEnabled.insert(mapExtId);
		}
		else if(this->has_auto_note_enabled(mapExtId))
		{
			this->autoNoteEnabled.erase(mapExtId);
		}
	}
}

auto_notes::auto_note_settings& get_auto_notes_settings()
{
	static auto_notes::auto_note_settings staticSettings;
	return staticSettings;
}
