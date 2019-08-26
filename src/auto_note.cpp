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
		if(!read_from_file_optional_json(this->build_save_path(), parseJson)) {
			// If loading from JSON failed, fall back to default initialization
			this->default_initialize();
			this->save();
		}
	}
	
	void auto_note_settings::default_initialize()
	{
		// Perform clear for good measure
		this->clear();
	
		// Fetch all know map extra types and iterate over them
		for(auto extra: MapExtras::mapExtraFactory().get_all())
		{
			if(extra.autonote) {
				this->autoNoteEnabled.insert(extra.id.str());
			}
		}
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
		for(auto extra: MapExtras::mapExtraFactory().get_all())
		{
			// Ignore all extras that have autonote disabled in the JSON.
			// This filters out lots of extras users shouldnt see (like "normal")
			if(!extra.autonote) {
				continue;
			}
		
			// Check if auto note is enabled for this map extra type
			bool isAutoNoteEnabled = settings.has_auto_note_enabled(extra.id);
		
			// Insert into cache
			this->mapExtraCache.push_back(std::make_pair(extra, isAutoNoteEnabled));
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
        mvwputch( w_border, point( 0, 2 ), c_light_gray, LINE_XXXO );
        mvwputch( w_border, point( 79, 2 ), c_light_gray, LINE_XOXX );
        mvwputch( w_border, point( 61, FULL_SCREEN_HEIGHT - 1 ), c_light_gray, LINE_XXOX );
        wrefresh( w_border );
        
        // == Draw header
        int tmpx = 0;  
        tmpx += shortcut_print( w_header, point( tmpx, 0 ), c_white, c_light_green, _( "<E>nable" ) ) + 2;
        tmpx += shortcut_print( w_header, point( tmpx, 0 ), c_white, c_light_green, _( "<D>isable" ) ) + 2;
        tmpx += shortcut_print( w_header, point( tmpx, 0 ), c_white, c_light_green, _( "<Enter>-Toggle" ) ) + 2;
        
        // Draw horizontal line and corner pieces of the table
        for( int x = 0; x < 78; x++ ) {
			if( x == 60 ) {
				// T-piece and start of table column separator
				mvwputch( w_header, point( x, 1 ), c_light_gray, LINE_OXXX );
				mvwputch( w_header, point( x, 2 ), c_light_gray, LINE_XOXO );
			} else {
				// Horizontal line
				mvwputch( w_header, point( x, 1 ), c_light_gray, LINE_OXOX );
			}
		}
		
		// Draw table headers
		mvwprintz( w_header, point( 1, 2 ), c_white, _( "Map Extra" ) );
        mvwprintz( w_header, point( 62, 2 ), c_white, _( "Enabled" ) );
        
        wrefresh( w_header );
        
        // ===========================================================================
        
        
        // The currently selected line
        int currentLine = 0;
        
        // At which index we begin drawing cache entries
        int startPosition = 0;
        
        // At which index we should stop drawing cache entries
        int endPosition = 0;
        
        // Setup input context with the actions applicable to this GUI
        input_context ctx{ "AUTO_NOTES" };
        ctx.register_cardinal();
        ctx.register_action("CONFIRM");
        ctx.register_action("QUIT");
        ctx.register_action("ENABLE_MAPEXTRA_NOTE");
        ctx.register_action("DISABLE_MAPEXTRA_NOTE");
        ctx.register_action("SWITCH_AUTO_NOTE_OPTION");
        
        while(true) {
        	// Draw info about auto notes option
        	mvwprintz( w_header, point( 39, 0 ), c_white, _( "Auto notes enabled:" ) );
        	
        	int currentX = 60;
        	currentX += shortcut_print( w_header, point( currentX, 0 ),
                                get_option<bool>( "AUTO_NOTES" ) ? c_light_green : c_light_red, c_white,
                                get_option<bool>( "AUTO_NOTES" ) ? _( "True" ) : _( "False" ) );
             
            currentX += shortcut_print( w_header, point( currentX, 0 ), c_white, c_light_green, "  " );
        	currentX += shortcut_print( w_header, point( currentX, 0 ), c_white, c_light_green, _( "<S>witch " ) );                   
                               
        
        	// Clear table
        	for( int y = 0; y < iContentHeight; y++ ) {
            	for( int x = 0; x < 79; x++ ) {
            		// The middle beam needs special treatment
            		if(x == 60) {
            			mvwputch( w, point( x, y ), c_light_gray, LINE_XOXO );
            		} else {
            			mvwputch( w, point( x, y ), c_black, ' ' );
            		}
            	}
        	}
        	
        	// Retrieve number of cache entries
        	const int cacheSize = static_cast<int>(this->mapExtraCache.size());
        	
        	// Draw the scroll bar
        	draw_scrollbar( w_border, currentLine, iContentHeight, cacheSize, point( 0, 4 ) );
        	
        	// Calculate start and stop indices for map extra table display
        	calcStartPos( startPosition, currentLine, iContentHeight, this->mapExtraCache.size() );
        	endPosition = startPosition + ( iContentHeight > cacheSize ? cacheSize : iContentHeight );
        	
        	// Actually draw table entries
        	for(int i = startPosition; i < endPosition; ++i) {
        		
        		// Retrieve current cache entry
        		const auto& cacheEntry = this->mapExtraCache[i];
        		
        		// Determine line text style based on selection status
        		const auto lineColor = (i == currentLine) ? hilite(c_white) : c_white;
        		const auto statusColor = (cacheEntry.second) ? c_green : c_red;
        		const auto statusString = (cacheEntry.second) ? "yes   " : "no    ";
        		
        		// Move into position to draw this line
        		mvwprintz( w, point( 1, i - startPosition ), lineColor, "" );
        		
        		// Draw chevrons (>>) at currently selected item
        		if(i == currentLine) {
        			wprintz( w, c_yellow, ">> " );
        		} else {
        			wprintz( w, c_yellow, "   " );
        		}
        		
        		// Draw the two column entries
        		wprintz( w, lineColor, "%s", cacheEntry.first.name ); 		
        		mvwprintz( w, point( 64, i - startPosition ), statusColor, "%s", statusString );
        	}
        	
        	wrefresh( w_header );
        	wrefresh( w_border );
        	wrefresh( w );
        
        	// Retrieve pending input action and process it
        	const std::string currentAction = ctx.handle_input();
        	
        	// Retrieve currently selected cache entry. This is used in multiple input action handlers.
    		std::pair<const map_extra, bool>& entry = this->mapExtraCache[currentLine];

        	
        	if( currentAction == "UP" ) {
        		if(currentLine > 0) {
        			--currentLine;
        		} else {
        			currentLine = cacheSize - 1;
        		}
        	} else if( currentAction == "DOWN" ) {
        		if(currentLine == cacheSize - 1) {
        			currentLine = 0;
        		} else {
        			++currentLine;
        		}
        	} else if( currentAction == "QUIT" ) {
        		break;
        	} else if( currentAction == "ENABLE_MAPEXTRA_NOTE" ) {
        		// Update cache entry
        		entry.second = true;
        		
        		// Set dirty flag
        		this->wasChanged = true;
        	} else if( currentAction == "DISABLE_MAPEXTRA_NOTE" ) {
        		// Update cache entry
        		entry.second = false;
	
        		// Set dirty flag
        		this->wasChanged = true;
        	} else if( currentAction == "CONFIRM" ) {  	
        		// Update cache entry
        		entry.second = !entry.second;
        		
        		// Set dirty flag
        		this->wasChanged = true;
        	} else if( currentAction == "SWITCH_AUTO_NOTE_OPTION" ) {
        		get_options().get_option( "AUTO_NOTES" ).setNext();
        	
        		// If we just enabled auto notes and auto notes extras is disabled, 
        		// enable that as well
        		if( get_option<bool>( "AUTO_NOTES" ) && !get_option<bool>( "AUTO_NOTES_MAP_EXTRAS" )) {
        			get_options().get_option( "AUTO_NOTES_MAP_EXTRAS" ).setNext();
        		}
        		
        		get_options().save();
        	}
        }
        
        // If nothing was changed, we are done here
        if( !this->was_changed() ) {
        	return;
        }
        
        // Ask the user if they want to save the changes they made.
        if( query_yn( _( "Save changes?" ) ) ) {
        	// Retrieve auto note settings object
        	auto_notes::auto_note_settings& settings = get_auto_notes_settings();
        
        	// Apply changes
        	for(const std::pair<const map_extra&, bool>& entry: this->mapExtraCache) {
        		if(entry.second)
        			std::cout << "Setting TRUE for " << entry.first.id.str() << std::endl;
        		settings.set_auto_note_status(entry.first.id, entry.second);
        	}
    	}
	}
}

auto_notes::auto_note_settings& get_auto_notes_settings()
{
	static auto_notes::auto_note_settings staticSettings;
	return staticSettings;
}
