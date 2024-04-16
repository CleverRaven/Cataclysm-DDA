#include "auto_note.h"

#include <cstddef>
#include <iosfwd>
#include <iterator>
#include <string>
#include <type_traits>

#include "cata_utility.h"
#include "color.h"
#include "cursesdef.h"
#include "filesystem.h"
#include "flexbuffer_json-inl.h"
#include "flexbuffer_json.h"
#include "generic_factory.h"
#include "input_context.h"
#include "json.h"
#include "map_extras.h"
#include "options.h"
#include "output.h"
#include "path_info.h"
#include "point.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translation.h"
#include "translations.h"
#include "ui.h"
#include "ui_manager.h"

namespace auto_notes
{
cata_path auto_note_settings::build_save_path() const
{
    return PATH_INFO::player_base_save_path_path() + ".ano.json";
}

void auto_note_settings::clear()
{
    character_autoNoteEnabled.clear();
    discovered.clear();
}

bool auto_note_settings::save( bool bCharacter )
{
    if( bCharacter && !file_exist( PATH_INFO::player_base_save_path() + ".sav" ) ) {
        return true;
    }
    cata_path sGlobalFile = PATH_INFO::autonote();
    return write_to_file( bCharacter ? build_save_path() : sGlobalFile, [ &,
    bCharacter]( std::ostream & fstr ) {
        JsonOut jout{ fstr, true };

        jout.start_object();

        if( bCharacter ) {
            jout.member( "enabled" );

            jout.start_array();
            for( const map_extra_id &entry : character_autoNoteEnabled ) {
                jout.write( entry.str() );
            }
            jout.end_array();

            jout.member( "discovered" );

            jout.start_array();
            for( const map_extra_id &entry : discovered ) {
                jout.write( entry.str() );
            }
            jout.end_array();

        } else {
            jout.member( "disabled" );

            jout.start_array();
            for( const map_extra_id &entry : global_autoNoteDisabled ) {
                jout.write( entry.str() );
            }
            jout.end_array();
        }

        if( !( bCharacter ?  character_custom_symbols : global_custom_symbols ).empty() ) {
            jout.member( "custom_symbols" );
            jout.start_array();
            for( const std::pair<const map_extra_id &, const auto_notes::custom_symbol &> symbol_entry :
                 bCharacter ?  character_custom_symbols : global_custom_symbols ) {
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

void auto_note_settings::load( bool bCharacter )
{
    clear();

    const auto parseJson = [ &, bCharacter ]( const JsonValue & jv ) {
        JsonObject jo = jv;

        for( JsonMember member : jo ) {
            const std::string name = member.name();

            if( name == "enabled" ) {
                JsonArray enabled_notes = member;
                for( std::string entry : enabled_notes ) {
                    character_autoNoteEnabled.insert( map_extra_id{ entry } );
                }
            } else if( name == "disabled" ) {
                JsonArray disabled_notes = member;
                for( std::string entry : disabled_notes ) {
                    global_autoNoteDisabled.insert( map_extra_id{ entry } );
                }
            } else if( name == "discovered" ) {
                JsonArray discovered_array = member;
                for( std::string entry : discovered_array ) {
                    discovered.insert( map_extra_id {std::move( entry )} );
                }
            } else if( name == "custom_symbols" ) {
                JsonArray symbols_json = member;
                for( JsonObject joSymbols : symbols_json ) {
                    const std::string entry = joSymbols.get_string( "map_extra" );
                    const std::string custom_symbol_str = joSymbols.get_string( "symbol" );
                    const std::string custom_color = joSymbols.get_string( "color" );
                    custom_symbol sym;
                    sym.set_symbol( custom_symbol_str );
                    sym.set_color( custom_color );
                    ( bCharacter ? character_custom_symbols : global_custom_symbols ).insert( std::make_pair(
                                map_extra_id {entry}, sym ) );
                }
            }
        }
    };

    if( bCharacter ) {
        if( !read_from_file_optional_json( build_save_path(), parseJson ) ) {
            default_initialize();
            save( true );
        }
    } else {
        cata_path sGlobalFile = PATH_INFO::autonote();
        read_from_file_optional_json( sGlobalFile, parseJson );
    }
}

void auto_note_settings::default_initialize()
{
    clear();
    load( false );

    for( const map_extra &extra : MapExtras::mapExtraFactory().get_all() ) {
        if( has_auto_note_enabled( extra.id, false ) && extra.autonote ) {
            character_autoNoteEnabled.insert( extra.id );
        }
    }
    character_custom_symbols = global_custom_symbols;
}

void auto_note_settings::set_discovered( const map_extra_id &mapExtId )
{
    discovered.insert( mapExtId );
}

bool auto_note_settings::was_discovered( const map_extra_id &mapExtId ) const
{
    return discovered.count( mapExtId ) != 0;
}

void auto_note_settings::show_gui()
{
    auto_note_manager_gui gui{ };
    gui.show();

    if( gui.was_changed( true ) ) {
        save( true );
    }
    if( gui.was_changed( false ) ) {
        save( false );
    }
}

bool auto_note_settings::has_auto_note_enabled( const map_extra_id &mapExtId,
        bool bCharacter ) const
{
    if( bCharacter ) {
        return character_autoNoteEnabled.count( mapExtId ) != 0;
    } else {
        return global_autoNoteDisabled.count( mapExtId ) == 0;
    }
}

void auto_note_settings::set_auto_note_status( const map_extra_id &mapExtId,
        const bool enabled, bool bCharacter )
{
    if( enabled && bCharacter ) {
        character_autoNoteEnabled.insert( mapExtId );
    } else if( enabled && !bCharacter ) {
        global_autoNoteDisabled.erase( mapExtId );
    } else if( has_auto_note_enabled( mapExtId, bCharacter ) ) {
        if( bCharacter ) {
            character_autoNoteEnabled.erase( mapExtId );
        } else {
            global_autoNoteDisabled.insert( mapExtId );
        }
    }
}

std::optional<custom_symbol> auto_note_settings::get_custom_symbol(
    const map_extra_id &mapExtId ) const
{
    auto entry = character_custom_symbols.find( mapExtId );
    return entry == character_custom_symbols.end() ? std::nullopt
           : std::optional<custom_symbol>( entry->second );
}

void auto_note_settings::set_custom_symbol( const map_extra_id &mapExtId,
        const custom_symbol &symbol, bool bCharacter )
{
    ( bCharacter ?  character_custom_symbols : global_custom_symbols ).emplace( mapExtId, symbol );
}

void auto_note_settings::clear_all_custom_symbols( bool bCharacter )
{
    ( bCharacter ?  character_custom_symbols : global_custom_symbols ).clear();
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

        char_mapExtraCache.emplace( extra.id, std::make_pair( extra,
                                    settings.has_auto_note_enabled( extra.id, true ) ) );
        global_mapExtraCache.emplace( extra.id, std::make_pair( extra,
                                      settings.has_auto_note_enabled( extra.id, false ) ) );

        if( settings.was_discovered( extra.id ) ) {
            char_displayCache.push_back( extra.id );
            global_displayCache.push_back( extra.id );
        }
    }

    fill_custom_symbols_cache();
}

bool auto_note_manager_gui::was_changed( bool bCharacter ) const
{
    return bCharacter ? charwasChanged : globalwasChanged;
}

void auto_note_manager_gui::fill_custom_symbols_cache()
{
    const auto_note_settings &settings = get_auto_notes_settings();
    char_custom_symbol_cache.clear();
    global_custom_symbol_cache.clear();
    for( const std::pair<const map_extra_id &, const auto_notes::custom_symbol &> symbol_entry :
         settings.character_custom_symbols ) {
        char_custom_symbol_cache.emplace( symbol_entry.first, symbol_entry.second );
    }
    for( const std::pair<const map_extra_id &, const auto_notes::custom_symbol &> symbol_entry :
         settings.global_custom_symbols ) {
        global_custom_symbol_cache.emplace( symbol_entry.first, symbol_entry.second );
    }
}

void auto_note_manager_gui::set_cached_custom_symbol( const map_extra_id &mapExtId,
        const custom_symbol &symbol, bool bCharacter )
{
    auto found = ( bCharacter ? char_custom_symbol_cache : global_custom_symbol_cache ).find(
                     mapExtId );
    if( found == ( bCharacter ? char_custom_symbol_cache : global_custom_symbol_cache ).end() ) {
        ( bCharacter ? char_custom_symbol_cache : global_custom_symbol_cache ).emplace( mapExtId, symbol );
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
    bool bCharacter = true;
    const bool char_emptyMode = char_displayCache.empty();
    const bool global_emptyMode = global_displayCache.empty();
    const int char_cacheSize = static_cast<int>( char_displayCache.size() );
    const int global_cacheSize = static_cast<int>( global_displayCache.size() );

    int currentLine = 0;
    int startPosition = 0;
    int endPosition = 0;

    input_context ctxt{ "AUTO_NOTES" };
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "SWITCH_AUTO_NOTE_OPTION" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "NEXT_TAB" );
    if( !char_emptyMode || !global_emptyMode ) {
        ctxt.register_navigate_ui_list();
        ctxt.register_action( "CONFIRM" );
        ctxt.register_action( "QUIT" );
        ctxt.register_action( "ENABLE_MAPEXTRA_NOTE" );
        ctxt.register_action( "DISABLE_MAPEXTRA_NOTE" );
        ctxt.register_action( "CHANGE_MAPEXTRA_CHARACTER" );
    }

    ui.on_redraw( [&]( const ui_adaptor & ) {
        // == Draw border
        draw_border( w_border, BORDER_COLOR, _( "Auto notes manager" ) );
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
        tmpx = 17;
        tmpx += shortcut_print( w_header, point( tmpx, iHeaderHeight - 2 ),
                                bCharacter ? hilite( c_white ) : c_white, c_light_green, _( "Character" ) ) + 2;
        shortcut_print( w_header, point( tmpx, iHeaderHeight - 2 ),
                        !bCharacter ? hilite( c_white ) : c_white, c_light_green, _( "Global" ) );

        mvwprintz( w_header, point( 1, iHeaderHeight - 1 ), c_white, _( "Map Extra" ) );
        mvwprintz( w_header, point( 53, iHeaderHeight - 1 ), c_white, _( "Symbol" ) );
        mvwprintz( w_header, point( 62, iHeaderHeight - 1 ), c_white, _( "Enabled" ) );

        wnoutrefresh( w_header );

        // TODO: Show info about custom symbols (hotkey, hint, state)
        std::string header_info_custom_symbols = string_format( _( "<color_light_green>%1$s</color> %2$s" ),
                ctxt.get_desc( "CHANGE_MAPEXTRA_CHARACTER" ), _( "Change a symbol for map extra" ) );
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
        currentX = 39;
        mvwprintz( w_header, point( currentX, 0 ), c_white, std::string( FULL_SCREEN_WIDTH - 2 - currentX,
                   ' ' ) );
        shortcut_print( w_header, point( currentX, 0 ), c_white, c_light_green,
                        _( "<Tab> to change pages." ) );

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
        int cacheSize = bCharacter ? char_cacheSize : global_cacheSize;
        draw_scrollbar( w_border, currentLine, iContentHeight, cacheSize, point( 0, iHeaderHeight + 1 ) );

        if( bCharacter ? char_emptyMode : global_emptyMode ) {
            // NOLINTNEXTLINE(cata-use-named-point-constants)
            mvwprintz( w, point( 1, 0 ), c_light_gray,
                       _( "Discover more special encounters to populate this list" ) );
        } else {
            calcStartPos( startPosition, currentLine, iContentHeight,
                          ( bCharacter ? char_displayCache : global_displayCache ).size() );
            endPosition = startPosition + ( iContentHeight > cacheSize ? cacheSize : iContentHeight );

            for( int i = startPosition; i < endPosition; ++i ) {
                const map_extra_id &displayCacheEntry = ( bCharacter ? char_displayCache : global_displayCache )[i];
                const auto &cacheEntry = ( bCharacter ? char_mapExtraCache :
                                           global_mapExtraCache )[displayCacheEntry];

                const nc_color lineColor = ( i == currentLine ) ? hilite( c_white ) : c_white;
                const nc_color statusColor = enabled_auto_notes_ME ? ( cacheEntry.second ? c_green : c_red ) :
                                             c_dark_gray;
                const std::string statusString = cacheEntry.second ? _( "yes" ) : _( "no" );
                auto found_custom_symbol = ( bCharacter ? char_custom_symbol_cache :
                                             global_custom_symbol_cache ).find( displayCacheEntry );
                const bool has_custom_symbol = ( found_custom_symbol != ( bCharacter ? char_custom_symbol_cache :
                                                 global_custom_symbol_cache ).end() );
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

        const std::string action = ctxt.handle_input();

        // Actions that also work with no items to display
        if( action == "SWITCH_AUTO_NOTE_OPTION" ) {
            get_options().get_option( "AUTO_NOTES_MAP_EXTRAS" ).setNext();
            get_options().save();
        } else if( action == "QUIT" ) {
            break;
        } else if( action == "NEXT_TAB" ) {
            bCharacter = !bCharacter;
        }

        if( bCharacter ? char_emptyMode : global_emptyMode ) {
            continue;
        }

        int cacheSize = bCharacter ? char_cacheSize : global_cacheSize;
        const map_extra_id &currentItem = ( bCharacter ? char_displayCache :
                                            global_displayCache )[currentLine];
        std::pair<const map_extra, bool> &entry = ( bCharacter ? char_mapExtraCache :
                global_mapExtraCache )[currentItem];
        const int scroll_rate = cacheSize > 20 ? 10 : 3;

        if( navigate_ui_list( action, currentLine, scroll_rate, cacheSize, true ) ) {
        } else if( action == "ENABLE_MAPEXTRA_NOTE" ) {
            entry.second = true;
            ( bCharacter ? charwasChanged : globalwasChanged ) = true;
        } else if( action == "DISABLE_MAPEXTRA_NOTE" ) {
            entry.second = false;
            ( bCharacter ? charwasChanged : globalwasChanged ) = true;
        } else if( action == "CHANGE_MAPEXTRA_CHARACTER" ) {
            string_input_popup custom_symbol_popup;
            custom_symbol_popup
            .title( _( "Enter a map extra custom symbol (empty to unset):" ) )
            .width( 2 )
            .query_string();

            if( !custom_symbol_popup.canceled() ) {
                const std::string &custom_symbol_str = custom_symbol_popup.text();
                if( custom_symbol_str.empty() ) {

                    ( bCharacter ? char_custom_symbol_cache : global_custom_symbol_cache ).erase( currentItem );
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
                    set_cached_custom_symbol( currentItem, value_to_set, bCharacter );
                }
                ( bCharacter ? charwasChanged : globalwasChanged ) = true;
            }
        } else if( action == "CONFIRM" ) {
            entry.second = !entry.second;
            ( bCharacter ? charwasChanged : globalwasChanged ) = true;
        }
    }

    if( !get_option<bool>( "AUTO_NOTES" ) &&
        get_option<bool>( "AUTO_NOTES_MAP_EXTRAS" ) &&
        query_yn( _( "Auto notes are disabled globally.\nDo you want to enable?" ) ) ) {
        get_options().get_option( "AUTO_NOTES" ).setNext();
        get_options().save();
    }

    if( !was_changed( false ) && !was_changed( true ) ) {
        return;
    }

    if( query_yn( _( "Save changes?" ) ) ) {
        auto_notes::auto_note_settings &settings = get_auto_notes_settings();

        for( const auto &entry : char_mapExtraCache ) {
            settings.set_auto_note_status( entry.second.first.id, entry.second.second, true );
        }
        for( const auto &entry : global_mapExtraCache ) {
            settings.set_auto_note_status( entry.second.first.id, entry.second.second, false );
        }
        settings.clear_all_custom_symbols( true );
        settings.clear_all_custom_symbols( false );
        for( const auto &entry : char_custom_symbol_cache ) {
            settings.set_custom_symbol( entry.first, entry.second, true );
        }
        for( const auto &entry : global_custom_symbol_cache ) {
            settings.set_custom_symbol( entry.first, entry.second, false );
        }
    }
}
} // namespace auto_notes

auto_notes::auto_note_settings &get_auto_notes_settings()
{
    static auto_notes::auto_note_settings staticSettings;
    return staticSettings;
}
