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
#include "output.h"
#include "debug.h"
#include "filesystem.h"
#include "translations.h"
#include "avatar.h"
#include "map_extras.h"

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
	
	void auto_note_settings::show_gui()
	{
		// Create local GUI instance and show
		auto_note_manager_gui gui{ };
		gui.show();
		
		// If settings were changed, we need to do a save
		if(gui.was_changed())
			this->save();
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
	
	bool auto_note_settings::has_auto_note_enabled(const string_id<map_extra>& mapExtId) const
	{
		return this->has_auto_note_enabled(mapExtId.str());
	}
	
	void auto_note_settings::set_auto_note_status(const string_id<map_extra>& mapExtId, const bool enabled)
	{
		this->set_auto_note_status(mapExtId.str(), enabled);
	}
	
	void auto_note_manager_gui::initialize()
	{
		// Clear all data
		this->mapExtraCache.clear();
		
		// Fetch reference to current auto note settings
		const auto_note_settings& settings = get_auto_notes_settings(); 
		
		// Fetch all know map extra types and iterate over them
		for(auto& extra: MapExtras::mapExtraFactory().get_all())
		{
			// Check if auto note is enabled for this map extra type
			bool isAutoNoteEnabled = settings.has_auto_note_enabled(extra.id);
		
			// Insert into cache
			this->mapExtraCache.emplace(std::make_pair(extra.id.str(), std::make_pair(extra, isAutoNoteEnabled)));
		}
	}
	
	auto_note_manager_gui::auto_note_manager_gui()
	{
		// Initialize GUI state
		this->initialize();
	}
	
	bool auto_note_manager_gui::was_changed()
	{
		return this->wasChanged;
	}
	
	void auto_note_manager_gui::show()
	{
		const int iHeaderHeight = 3;
		const int iContentHeight = FULL_SCREEN_HEIGHT - 2 - iHeaderHeight;

		const int iOffsetX = TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0;
		const int iOffsetY = TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0;

		catacurses::window w_help = catacurses::newwin( FULL_SCREEN_HEIGHT / 2 + 2,
				                FULL_SCREEN_WIDTH * 3 / 4,
				                point( iOffsetX + 19 / 2, 7 + iOffsetY + FULL_SCREEN_HEIGHT / 2 / 2 ) );

		catacurses::window w_border = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
				                  point( iOffsetX, iOffsetY ) );
				                  
		catacurses::window w_header = catacurses::newwin( iHeaderHeight, FULL_SCREEN_WIDTH - 2,
				                  point( 1 + iOffsetX, 1 + iOffsetY ) );
				                  
		catacurses::window w = catacurses::newwin( iContentHeight, FULL_SCREEN_WIDTH - 2,
				           point( 1 + iOffsetX, iHeaderHeight + 1 + iOffsetY ) );
		
		// ===========================================================================
		// Perform initial draw. This includes things like the window border that do 
		// not need to be refreshed more than once.
		
		// == Draw border
		draw_border( w_border, BORDER_COLOR, _( " AUTO NOTES MANAGER " ) );
        mvwputch( w_border, point( 0, 2 ), c_light_gray, LINE_XXXO ) ; // |-
        mvwputch( w_border, point( 79, 2 ), c_light_gray, LINE_XOXX ); // -|
        wrefresh( w_border );
        
        // == Draw header
        int tmpx = 0;  
        tmpx += shortcut_print( w_header, point( tmpx, 0 ), c_white, c_light_green, _( "<E>nable" ) ) + 2;
        tmpx += shortcut_print( w_header, point( tmpx, 0 ), c_white, c_light_green, _( "<D>isable" ) ) + 2;
        tmpx += shortcut_print( w_header, point( tmpx, 0 ), c_white, c_light_green, _( "<Enter>-Toggle" ) ) + 2;
        
        // Draw horizontal line and corner pieces of the table
        for( int i = 0; i < 78; i++ ) {
			if( i == 60 ) {
				// T-piece and start of table column separator
				mvwputch( w_header, point( i, 1 ), c_light_gray, LINE_OXXX );
				mvwputch( w_header, point( i, 2 ), c_light_gray, LINE_XOXO );
			} else {
				// Horizontal line
				mvwputch( w_header, point( i, 1 ), c_light_gray, LINE_OXOX );
			}
		}
		
		// Draw table headers
		mvwprintz( w_header, point( 1, 2 ), c_white, _( "Map Extra" ) );
        mvwprintz( w_header, point( 62, 2 ), c_white, _( "Enabled" ) );
        
        wrefresh( w_header );
        
        // ===========================================================================
        
        
        input_context ctx{ "AUTO_NOTES" };
        ctx.register_cardinal();
        ctx.register_action("CONFIRM");
        ctx.register_action("QUIT");
        ctx.register_action("ENABLE_MAPEXTRA_NOTE");
        ctx.register_action("DISABLE_MAPEXTRA_NOTE");
        
        while(true) { 
        	const std::string currentAction = ctx.handle_input();
        	
        	if(currentAction == "CONFIRM")
        		std::cout << "TOGGLE" << std::endl;
        	else if(currentAction == "ENABLE_MAPEXTRA_NOTE")
        		std::cout << "ENABLE_MAPEXTRA_NOTE" << std::endl;
        	else if(currentAction == "DISABLE_MAPEXTRA_NOTE")
        		std::cout << "DISABLE_MAPEXTRA_NOTE" << std::endl;
        }	
	}
}

auto_notes::auto_note_settings& get_auto_notes_settings()
{
	static auto_notes::auto_note_settings staticSettings;
	return staticSettings;
}
