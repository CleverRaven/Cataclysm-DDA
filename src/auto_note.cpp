#include "auto_note.h"

#include <functional>
#include <string>

#include "cata_utility.h"
#include "color.h"
#include "cursesdef.h"
#include "filesystem.h"
#include "generic_factory.h"
#include "input.h"
#include "json.h"
#include "map_extras.h"
#include "options.h"
#include "output.h"
#include "path_info.h"
#include "point.h"
#include "string_input_popup.h"
#include "translations.h"
#include "ui.h"
#include "ui_manager.h"

namespace auto_notes
{
std::string auto_note_settings::build_save_path() const
{
    return PATH_INFO::player_base_save_path() + ".ano.json";
}

void auto_note_settings::clear()
{
    autoNoteEnabled.clear();
}

bool auto_note_settings::save()
{
    if( !file_exist( PATH_INFO::player_base_save_path() + ".sav" ) ) {
        return true;
    }

    return write_to_file( build_save_path(), [&]( std::ostream & fstr ) {
        JsonOut jout{ fstr, true };

        jout.start_object();

        jout.member( "enabled" );

        jout.start_array();
        for( const string_id<map_extra> &entry : autoNoteEnabled ) {
            jout.write( entry.str() );
        }
        jout.end_array();

        jout.member( "discovered" );

        jout.start_array();
        for( const string_id<map_extra> &entry : discovered ) {
            jout.write( entry.str() );
        }
        jout.end_array();

        if( !custom_symbols.empty() ) {
            jout.member( "custom_symbols" );
            jout.start_array();
            for( const std::pair<const string_id<map_extra> &, const auto_notes::custom_symbol &> symbol_entry :
                 custom_symbols ) {
                jout.start_object();

                jout.member( "map_extra" );
                jout.write( symbol_entry.first.str() );

                jout.member( "symbol" );
                jout.write( symbol_entry.second.get_symbol_string() );

                jout.member( "color" );
                jout.write( symbol_entry.second.get_color_string() );

                jout.end_object();
            }
            jout.end_array();
        }

        jout.end_object();

    }, _( "auto notes configuration" ) );
}

void auto_note_settings::load()
{
    clear();

    const auto parseJson = [&]( JsonIn & jin ) {
        jin.start_object();

        while( !jin.end_object() ) {
            const std::string name = jin.get_member_name();

            if( name == "enabled" ) {
                jin.start_array();
                while( !jin.end_array() ) {
                    const std::string entry = jin.get_string();
                    autoNoteEnabled.insert( string_id<map_extra> {entry} );
                }
            } else if( name == "discovered" ) {
                jin.start_array();
                while( !jin.end_array() ) {
                    const std::string entry = jin.get_string();
                    discovered.insert( string_id<map_extra> {entry} );
                }
            } else if( name == "custom_symbols" ) {
                jin.start_array();
                while( !jin.end_array() ) {
                    JsonObject joSymbols = jin.get_object();
                    const std::string entry = joSymbols.get_string( "map_extra" );
                    const std::string custom_symbol_str = joSymbols.get_string( "symbol" );
                    const std::string custom_color = joSymbols.get_string( "color" );
                    custom_symbol sym;
                    sym.set_symbol( custom_symbol_str );
                    sym.set_color( custom_color );
                    custom_symbols.insert( std::make_pair( string_id<map_extra> {entry}, sym ) );
                }
            } else {
                jin.skip_value();
            }

        }
    };

    if( !read_from_file_optional_json( build_save_path(), parseJson ) ) {
        default_initialize();
        save();
    }
}

void auto_note_settings::default_initialize()
{
    clear();

    for( const map_extra &extra : MapExtras::mapExtraFactory().get_all() ) {
        if( extra.autonote ) {
            autoNoteEnabled.insert( extra.id );
        }
    }
}

void auto_note_settings::set_discovered( const string_id<map_extra> &mapExtId )
{
    discovered.insert( mapExtId );
}

bool auto_note_settings::was_discovered( const string_id<map_extra> &mapExtId ) const
{
    return discovered.count( mapExtId ) != 0;
}

void auto_note_settings::show_gui()
{
    auto_note_manager_gui gui{ };
    gui.show();

    if( gui.was_changed() ) {
        save();
    }
}

bool auto_note_settings::has_auto_note_enabled( const string_id<map_extra> &mapExtId ) const
{
    return autoNoteEnabled.count( mapExtId ) != 0;
}

void auto_note_settings::set_auto_note_status( const string_id<map_extra> &mapExtId,
        const bool enabled )
{
    if( enabled ) {
        autoNoteEnabled.insert( mapExtId );
    } else if( has_auto_note_enabled( mapExtId ) ) {
        autoNoteEnabled.erase( mapExtId );
    }
}

cata::optional<custom_symbol> auto_note_settings::get_custom_symbol(
    const string_id<map_extra> &mapExtId ) const
{
    auto entry = custom_symbols.find( mapExtId );
    return entry == custom_symbols.end() ? cata::nullopt
           : cata::optional<custom_symbol>( entry->second );
}

void auto_note_settings::set_custom_symbol( const string_id<map_extra> &mapExtId,
        const custom_symbol &symbol )
{
    custom_symbols.emplace( mapExtId, symbol );
}

void auto_note_settings::clear_all_custom_symbols()
{
    custom_symbols.clear();
}

auto_note_manager_gui::auto_note_manager_gui()
{
    const auto_note_settings &settings = get_auto_notes_settings();

    for( const map_extra &extra : MapExtras::mapExtraFactory().get_all() ) {
        // Ignore all extras that have autonote disabled in the JSON.
        // This filters out lots of extras users shouldn't see (like "normal")
        if( !extra.autonote ) {
            continue;
        }

        bool is_auto_note_enabled = settings.has_auto_note_enabled( extra.id );

        mapExtraCache.emplace( std::make_pair( extra.id, std::make_pair( extra,
                                               is_auto_note_enabled ) ) );

        if( settings.was_discovered( extra.id ) ) {
            displayCache.push_back( extra.id );
        }
    }

    fill_custom_symbols_cache();
}

bool auto_note_manager_gui::was_changed() const
{
    return wasChanged;
}

void auto_note_manager_gui::fill_custom_symbols_cache()
{
    const auto_note_settings &settings = get_auto_notes_settings();
    custom_symbol_cache.clear();

    for( const std::pair<const string_id<map_extra> &, const auto_notes::custom_symbol &> symbol_entry :
         settings.custom_symbols ) {
        custom_symbol_cache.emplace( symbol_entry.first, symbol_entry.second );
    }
}

void auto_note_manager_gui::set_cached_custom_symbol( const string_id<map_extra> &mapExtId,
        const custom_symbol &symbol )
{
    auto found = custom_symbol_cache.find( mapExtId );
    if( found == custom_symbol_cache.end() ) {
        custom_symbol_cache.emplace( mapExtId, symbol );
    } else {
        found->second = symbol;
    }
}

void auto_note_manager_gui::show()
{
    const int iHeaderHeight = 4;
    int iContentHeight = 0;
    catacurses::window w_border;
    catacurses::window w_header;
    catacurses::window w;

    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        iContentHeight = FULL_SCREEN_HEIGHT - 2 - iHeaderHeight;

        const point iOffset( TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0,
                             TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0 );

        w_border = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                       iOffset );

        w_header = catacurses::newwin( iHeaderHeight, FULL_SCREEN_WIDTH - 2,
                                       iOffset + point_south_east );

        w = catacurses::newwin( iContentHeight, FULL_SCREEN_WIDTH - 2,
                                iOffset + point( 1, iHeaderHeight + 1 ) );

        ui.position_from_window( w_border );
    } );
    ui.mark_resize();

    // ===========================================================================
    // If the display cache contains no entries, the player might not have discovered any of
    // the map extras. In this case, we switch to a special state that alerts the user of this
    // in order to avoid confusion a completely empty GUI might normally create.
    const bool emptyMode = displayCache.empty();
    const int cacheSize = static_cast<int>( displayCache.size() );

    int currentLine = 0;
    int startPosition = 0;
    int endPosition = 0;

    input_context ctx{ "AUTO_NOTES" };
    ctx.register_action( "QUIT" );
    ctx.register_action( "SWITCH_AUTO_NOTE_OPTION" );
    ctx.register_action( "HELP_KEYBINDINGS" );

    if( !emptyMode ) {
        ctx.register_cardinal();
        ctx.register_action( "PAGE_UP", to_translation( "Fast scroll up" ) );
        ctx.register_action( "PAGE_DOWN", to_translation( "Fast scroll down" ) );
        ctx.register_action( "CONFIRM" );
        ctx.register_action( "QUIT" );
        ctx.register_action( "ENABLE_MAPEXTRA_NOTE" );
        ctx.register_action( "DISABLE_MAPEXTRA_NOTE" );
        ctx.register_action( "CHANGE_MAPEXTRA_CHARACTER" );
    }

    ui.on_redraw( [&]( const ui_adaptor & ) {
        // == Draw border
        draw_border( w_border, BORDER_COLOR, _( " AUTO NOTES MANAGER " ) );
        mvwputch( w_border, point( 0, iHeaderHeight - 1 ), c_light_gray, LINE_XXXO );
        mvwputch( w_border, point( 79, iHeaderHeight - 1 ), c_light_gray, LINE_XOXX );
        mvwputch( w_border, point( 52, FULL_SCREEN_HEIGHT - 1 ), c_light_gray, LINE_XXOX );
        mvwputch( w_border, point( 61, FULL_SCREEN_HEIGHT - 1 ), c_light_gray, LINE_XXOX );
        wnoutrefresh( w_border );

        // == Draw header
        int tmpx = 0;
        tmpx += shortcut_print( w_header, point( tmpx, 0 ), c_white, c_light_green, _( "<E>nable" ) ) + 2;
        tmpx += shortcut_print( w_header, point( tmpx, 0 ), c_white, c_light_green, _( "<D>isable" ) ) + 2;
        shortcut_print( w_header, point( tmpx, 0 ), c_white, c_light_green, _( "<Enter> - Toggle" ) );

        // Draw horizontal line and corner pieces of the table
        for( int x = 0; x < 78; x++ ) {
            if( x == 51 || x == 60 ) {
                mvwputch( w_header, point( x, iHeaderHeight - 2 ), c_light_gray, LINE_OXXX );
                mvwputch( w_header, point( x, iHeaderHeight - 1 ), c_light_gray, LINE_XOXO );
            } else {
                mvwputch( w_header, point( x, iHeaderHeight - 2 ), c_light_gray, LINE_OXOX );
            }
        }

        mvwprintz( w_header, point( 1, iHeaderHeight - 1 ), c_white, _( "Map Extra" ) );
        mvwprintz( w_header, point( 53, iHeaderHeight - 1 ), c_white, _( "Symbol" ) );
        mvwprintz( w_header, point( 62, iHeaderHeight - 1 ), c_white, _( "Enabled" ) );

        wnoutrefresh( w_header );

        // TODO: Show info about custom symbols (hotkey, hint, state)
        std::string header_info_custom_symbols = string_format( _( "<color_light_green>%1$s</color> %2$s" ),
                ctx.get_desc( "CHANGE_MAPEXTRA_CHARACTER" ), _( "Change a symbol for map extra" ) );
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        fold_and_print( w_header, point( 0, 1 ), FULL_SCREEN_WIDTH - 2, c_white,
                        header_info_custom_symbols );

        mvwprintz( w_header, point( 39, 1 ), c_white, _( "Auto notes enabled:" ) );

        int currentX = 60;
        mvwprintz( w_header, point( currentX, 1 ), c_white,
                   std::string( FULL_SCREEN_WIDTH - 2 - currentX, ' ' ) );
        const bool enabled_auto_notes_ME = get_option<bool>( "AUTO_NOTES_MAP_EXTRAS" );
        currentX += shortcut_print( w_header, point( currentX, 1 ),
                                    enabled_auto_notes_ME ? c_light_green : c_light_red, c_white,
                                    enabled_auto_notes_ME ? _( "True" ) : _( "False" ) );

        currentX += shortcut_print( w_header, point( currentX, 1 ), c_white, c_light_green, "  " );
        shortcut_print( w_header, point( currentX, 1 ), c_white, c_light_green, _( "<S>witch " ) );

        // Clear table
        for( int y = 0; y < iContentHeight; y++ ) {
            for( int x = 0; x < 79; x++ ) {
                // The middle beams needs special treatment
                if( x == 51 || x == 60 ) {
                    mvwputch( w, point( x, y ), c_light_gray, LINE_XOXO );
                } else {
                    mvwputch( w, point( x, y ), c_black, ' ' );
                }
            }
        }

        draw_scrollbar( w_border, currentLine, iContentHeight, cacheSize, point( 0, iHeaderHeight + 1 ) );

        if( emptyMode ) {
            // NOLINTNEXTLINE(cata-use-named-point-constants)
            mvwprintz( w, point( 1, 0 ), c_light_gray,
                       _( "Discover more special encounters to populate this list" ) );
        } else {
            calcStartPos( startPosition, currentLine, iContentHeight, displayCache.size() );
            endPosition = startPosition + ( iContentHeight > cacheSize ? cacheSize : iContentHeight );

            for( int i = startPosition; i < endPosition; ++i ) {
                const string_id<map_extra> &displayCacheEntry = displayCache[i];
                const auto &cacheEntry = mapExtraCache[displayCacheEntry];

                const nc_color lineColor = ( i == currentLine ) ? hilite( c_white ) : c_white;
                const nc_color statusColor = enabled_auto_notes_ME ? ( cacheEntry.second ? c_green : c_red ) :
                                             c_dark_gray;
                const std::string statusString = cacheEntry.second ? _( "yes" ) : _( "no" );
                auto found_custom_symbol = custom_symbol_cache.find( displayCacheEntry );
                const bool has_custom_symbol = ( found_custom_symbol != custom_symbol_cache.end() );
                const nc_color charColor = has_custom_symbol ? found_custom_symbol->second.get_color() :
                                           cacheEntry.first.color;
                const std::string displayChar = has_custom_symbol ? found_custom_symbol->second.get_symbol_string()
                                                :
                                                cacheEntry.first.get_symbol();

                mvwprintz( w, point( 1, i - startPosition ), lineColor, "" );

                if( i == currentLine ) {
                    wprintz( w, c_yellow, ">> " );
                } else {
                    wprintz( w, c_yellow, "   " );
                }

                wprintz( w, lineColor, "%s", cacheEntry.first.name() );

                // Print the character this map extra is indicated by on the map
                mvwprintz( w, point( 55, i - startPosition ), charColor, "%s", displayChar );
                if( has_custom_symbol ) {
                    mvwprintz( w, point( 56, i - startPosition ), c_white, "*" );
                }
                // Since yes is longer than no, we need to clear the space for the status string before
                // displaying the current text. Otherwise artifacts might occur.
                mvwprintz( w, point( 64, i - startPosition ), statusColor, "     " );
                mvwprintz( w, point( 64, i - startPosition ), statusColor, "%s", statusString );
            }
        }

        wnoutrefresh( w_header );
        wnoutrefresh( w_border );
        wnoutrefresh( w );
    } );

    while( true ) {
        ui_manager::redraw();

        const std::string currentAction = ctx.handle_input();

        // Actions that also work with no items to display
        if( currentAction == "SWITCH_AUTO_NOTE_OPTION" ) {
            get_options().get_option( "AUTO_NOTES_MAP_EXTRAS" ).setNext();
            get_options().save();
        } else if( currentAction == "QUIT" ) {
            break;
        }

        if( emptyMode ) {
            continue;
        }

        const string_id<map_extra> &currentItem = displayCache[currentLine];
        std::pair<const map_extra, bool> &entry = mapExtraCache[currentItem];
        const int scroll_rate = cacheSize > 20 ? 10 : 3;

        if( currentAction == "UP" ) {
            if( currentLine > 0 ) {
                --currentLine;
            } else {
                currentLine = cacheSize - 1;
            }
        } else if( currentAction == "DOWN" ) {
            if( currentLine == cacheSize - 1 ) {
                currentLine = 0;
            } else {
                ++currentLine;
            }
        } else if( currentAction == "PAGE_DOWN" ) {
            if( currentLine == cacheSize - 1 ) {
                currentLine = 0;
            } else if( currentLine + scroll_rate >= cacheSize ) {
                currentLine = cacheSize - 1;
            } else {
                currentLine += +scroll_rate;
            }
        } else if( currentAction == "PAGE_UP" ) {
            if( currentLine == 0 ) {
                currentLine = cacheSize - 1;
            } else if( currentLine <= scroll_rate ) {
                currentLine = 0;
            } else {
                currentLine += -scroll_rate;
            }
        }  else if( currentAction == "ENABLE_MAPEXTRA_NOTE" ) {
            entry.second = true;
            wasChanged = true;
        } else if( currentAction == "DISABLE_MAPEXTRA_NOTE" ) {
            entry.second = false;
            wasChanged = true;
        } else if( currentAction == "CHANGE_MAPEXTRA_CHARACTER" ) {
            string_input_popup custom_symbol_popup;
            custom_symbol_popup
            .title( _( "Enter a map extra custom symbol (empty to unset):" ) )
            .width( 2 )
            .query_string();

            if( !custom_symbol_popup.canceled() ) {
                const std::string &custom_symbol_str = custom_symbol_popup.text();
                if( custom_symbol_str.empty() ) {
                    custom_symbol_cache.erase( currentItem );
                } else {
                    uilist ui_colors;
                    ui_colors.text = _( "Pick a color:" );
                    int i = 0;
                    const std::unordered_map<std::string, note_color> &note_color_names = get_note_color_names();
                    for( const std::pair<const std::string, note_color> &color_pair : note_color_names ) {
                        ui_colors.addentry( string_format( pgettext( "note color", "%1$s:%2$s, " ), color_pair.first,
                                                           colorize( color_pair.second.name, color_pair.second.color ) ) );
                        if( entry.first.color == color_pair.second.color ) {
                            ui_colors.selected = i;
                        }
                        i++;
                    }
                    // Default note color is the last one
                    const nc_color color_default = c_yellow;
                    const color_manager &colors = get_all_colors();
                    const std::string color_default_name = colors.get_name( color_default );
                    const translation color_yellow = to_translation( "color", "yellow" );
                    ui_colors.addentry( string_format( _( "Default: %s" ),
                                                       colorize( color_yellow.translated(), color_default ) ) );

                    ui_colors.query();
                    std::string custom_symbol_color;
                    if( ui_colors.ret >= 0 && static_cast<size_t>( ui_colors.ret ) < note_color_names.size() ) {
                        auto iter = note_color_names.begin();
                        std::advance( iter, ui_colors.ret );

                        // selected color
                        custom_symbol_color = colors.get_name( iter->second.color );
                    }

                    custom_symbol value_to_set;
                    value_to_set.set_symbol( custom_symbol_str );
                    value_to_set.set_color( custom_symbol_color );
                    set_cached_custom_symbol( currentItem, value_to_set );
                }
                wasChanged = true;
            }
        } else if( currentAction == "CONFIRM" ) {
            entry.second = !entry.second;
            wasChanged = true;
        }
    }

    if( !get_option<bool>( "AUTO_NOTES" ) &&
        get_option<bool>( "AUTO_NOTES_MAP_EXTRAS" ) &&
        query_yn( _( "Auto notes are disabled globally.\nDo you want to enable?" ) ) ) {
        get_options().get_option( "AUTO_NOTES" ).setNext();
        get_options().save();
    }

    if( !was_changed() ) {
        return;
    }

    if( query_yn( _( "Save changes?" ) ) ) {
        auto_notes::auto_note_settings &settings = get_auto_notes_settings();

        for( const auto &entry : mapExtraCache ) {
            settings.set_auto_note_status( entry.second.first.id, entry.second.second );
        }
        settings.clear_all_custom_symbols();
        for( const auto &entry : custom_symbol_cache ) {
            settings.set_custom_symbol( entry.first, entry.second );
        }
    }
}
} // namespace auto_notes

auto_notes::auto_note_settings &get_auto_notes_settings()
{
    static auto_notes::auto_note_settings staticSettings;
    return staticSettings;
}
