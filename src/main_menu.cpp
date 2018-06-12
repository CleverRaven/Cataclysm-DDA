#include "main_menu.h"

#include "game.h"
#include "player.h"
#include "output.h"
#include "gamemode.h"
#include "debug.h"
#include "mapbuffer.h"
#include "overmapbuffer.h"
#include "translations.h"
#include "catacharset.h"
#include "get_version.h"
#include "help.h"
#include "worldfactory.h"
#include "filesystem.h"
#include "path_info.h"
#include "mapsharing.h"
#include "sounds.h"
#include "cata_utility.h"
#include "auto_pickup.h"
#include "safemode_ui.h"

#define dbg(x) DebugLog((DebugLevel)(x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

void main_menu::on_move() const
{
    sfx::play_variant_sound( "menu_move", "default", 100 );
}

void main_menu::print_menu_items( const catacurses::window &w_in, std::vector<std::string> vItems,
                                  size_t iSel, int iOffsetY, int iOffsetX, int spacing )
{
    wmove( w_in, iOffsetY, iOffsetX );
    for( size_t i = 0; i < vItems.size(); ++i ) {
        nc_color text_color;
        nc_color key_color;
        if( iSel == i ) {
            text_color = h_white;
            key_color = h_white;
        } else {
            text_color = c_light_gray;
            key_color = c_white;
        }
        wprintz( w_in, c_light_gray, "[" );
        shortcut_print( w_in, text_color, key_color, vItems[i] );
        wprintz( w_in, c_light_gray, "]" );
        // Don't print spaces after last item.
        if( i != ( vItems.size() - 1 ) ) {
            wprintz( w_in, c_light_gray, std::string( spacing, ' ' ).c_str() );
        }
    }
}

void main_menu::print_menu( const catacurses::window &w_open, int iSel, const int iMenuOffsetX,
                            int iMenuOffsetY, bool bShowDDA )
{
    // Clear Lines
    werase( w_open );

    // Define window size
    int window_width = getmaxx( w_open );
    int window_height = getmaxy( w_open );

    // Draw horizontal line
    for( int i = 1; i < window_width - 1; ++i ) {
        mvwputch( w_open, window_height - 2, i, c_white, LINE_OXOX );
    }

    center_print( w_open, window_height - 1, c_red,
                  _( "Please report bugs to kevin.granade@gmail.com or post on the forums." ) );

    int iLine = 0;
    const int iOffsetX = ( window_width - FULL_SCREEN_WIDTH ) / 2;

    const nc_color cColor1 = c_light_cyan;
    const nc_color cColor2 = c_light_blue;
    const nc_color cColor3 = c_light_blue;

    if( mmenu_title.size() > 1 ) {
        for( size_t i = 0; i < mmenu_title.size(); ++i ) {
            if( i == 6 ) {
                if( !bShowDDA ) {
                    break;
                }
                if( FULL_SCREEN_HEIGHT > 24 ) {
                    ++iLine;
                }
            }
            mvwprintz( w_open, iLine++, iOffsetX, i < 6 ? cColor1 : cColor2, mmenu_title[i].c_str() );
        }
    } else {
        center_print( w_open, iLine++, cColor1, mmenu_title[0] );
    }

    if( bShowDDA ) {
        iLine++;
        center_print( w_open, iLine++, cColor3, string_format( _( "Version: %s" ), getVersionString() ) );
    }

    int menu_length = 0;
    for( size_t i = 0; i < vMenuItems.size(); ++i ) {
        menu_length += utf8_width( vMenuItems[i], true ) + 2;
        if( !vMenuHotkeys[i].empty() ) {
            menu_length += utf8_width( vMenuHotkeys[i][0] );
        }
    }
    const int free_space = std::max( 0, window_width - menu_length - iMenuOffsetX );
    const int spacing = free_space / ( ( int )vMenuItems.size() + 1 );
    const int width_of_spacing = spacing * ( vMenuItems.size() + 1 );
    const int adj_offset = std::max( 0, ( free_space - width_of_spacing ) / 2 );
    const int final_offset = iMenuOffsetX + adj_offset + spacing;

    print_menu_items( w_open, vMenuItems, iSel, iMenuOffsetY, final_offset, spacing );

    catacurses::refresh();
    wrefresh( w_open );
    catacurses::refresh();
}

std::vector<std::string> main_menu::load_file( const std::string &path,
        const std::string &alt_text ) const
{
    std::vector<std::string> result;
    read_from_file_optional( path, [&result]( std::istream & fin ) {
        std::string line;
        while( std::getline( fin, line ) ) {
            if( !line.empty() && line[0] == '#' ) {
                continue;
            }
            result.push_back( line );
        }
    } );
    if( result.empty() ) {
        result.push_back( alt_text );
    }
    return result;
}

std::string main_menu::handle_input_timeout( input_context &ctxt )
{
    inp_mngr.set_timeout( 125 );

    std::string action = ctxt.handle_input();

    if( action == "TIMEOUT" ) {
        init_windows();
    }

    inp_mngr.reset_timeout();

    return action;
}

void main_menu::init_windows()
{
    if( LAST_TERMX == TERMX && LAST_TERMY == TERMY ) {
        return;
    }

    w_background = catacurses::newwin( TERMY, TERMX, 0, 0 );
    werase( w_background );
    wrefresh( w_background );

    // main window should also expand to use available display space.
    // expanding to evenly use up half of extra space, for now.
    extra_w = ( ( TERMX - FULL_SCREEN_WIDTH ) / 2 ) - 1;
    int extra_h = ( ( TERMY - FULL_SCREEN_HEIGHT ) / 2 ) - 1;
    extra_w = ( extra_w > 0 ? extra_w : 0 );
    extra_h = ( extra_h > 0 ? extra_h : 0 );
    const int total_w = FULL_SCREEN_WIDTH + extra_w;
    const int total_h = FULL_SCREEN_HEIGHT + extra_h;

    // position of window within main display
    const int x0 = ( TERMX - total_w ) / 2;
    const int y0 = ( TERMY - total_h ) / 2;

    w_open = catacurses::newwin( total_h, total_w, y0, x0 );

    iMenuOffsetY = total_h - 3;
    // note: if iMenuOffset is changed,
    // please update MOTD and credits to indicate how long they can be.

    LAST_TERMX = TERMX;
    LAST_TERMY = TERMY;
}

void main_menu::init_strings()
{
    // ASCII Art
    mmenu_title = load_file( PATH_INFO::find_translated_file( "titledir", ".title", "title" ),
                             _( "Cataclysm: Dark Days Ahead" ) );
    // MOTD
    mmenu_motd = load_file( PATH_INFO::find_translated_file( "motddir", ".motd", "motd" ),
                            _( "No message today." ) );
    // Credits
    mmenu_credits.clear();
    std::vector<std::string> buffer;
    read_from_file_optional( PATH_INFO::find_translated_file( "creditsdir", ".credits",
    "credits" ), [&buffer, this]( std::istream & stream ) {
        std::string line;
        while( std::getline( stream, line ) ) {
            if( line[0] == '#' ) {
                continue;
            } else {
                buffer.push_back( line );
            }
            if( buffer.size() > 14 || line.empty() ) {
                std::ostringstream ss;
                for( std::vector<std::string>::iterator it = buffer.begin(); it != buffer.end(); ++it ) {
                    ss << *it << std::endl;
                }
                mmenu_credits.push_back( ss.str() );
                buffer.clear();
            }
        }
    } );
    if( !buffer.empty() ) {
        std::ostringstream ss;
        for( std::vector<std::string>::iterator it = buffer.begin(); it != buffer.end(); ++it ) {
            ss << *it << std::endl;
        }
        mmenu_credits.push_back( ss.str() );
    }
    if( mmenu_credits.empty() ) {
        mmenu_credits.push_back( _( "No credits information found." ) );
    }

    // fill menu with translated menu items
    vMenuItems.clear();
    vMenuItems.push_back( pgettext( "Main Menu", "<M|m>OTD" ) );
    vMenuItems.push_back( pgettext( "Main Menu", "<N|n>ew Game" ) );
    vMenuItems.push_back( pgettext( "Main Menu", "Lo<a|A>d" ) );
    vMenuItems.push_back( pgettext( "Main Menu", "<W|w>orld" ) );
    vMenuItems.push_back( pgettext( "Main Menu", "<S|s>pecial" ) );
    vMenuItems.push_back( pgettext( "Main Menu", "Se<t|T>tings" ) );
    vMenuItems.push_back( pgettext( "Main Menu", "H<e|E|?>lp" ) );
    vMenuItems.push_back( pgettext( "Main Menu", "<C|c>redits" ) );
    vMenuItems.push_back( pgettext( "Main Menu", "<Q|q>uit" ) );

    // determine hotkeys from translated menu item text
    vMenuHotkeys.clear();
    for( const std::string &item : vMenuItems ) {
        vMenuHotkeys.push_back( get_hotkeys( item ) );
    }

    vWorldSubItems.clear();
    vWorldSubItems.push_back( pgettext( "Main Menu|World", "<D|d>elete World" ) );
    vWorldSubItems.push_back( pgettext( "Main Menu|World", "<R|r>eset World" ) );
    vWorldSubItems.push_back( pgettext( "Main Menu|World", "<S|s>how World Mods" ) );

    vWorldHotkeys.clear();
    for( const std::string &item : vWorldSubItems ) {
        vWorldHotkeys.push_back( get_hotkeys( item ) );
    }

    vSettingsSubItems.clear();
    vSettingsSubItems.push_back( pgettext( "Main Menu|Settings", "<O|o>ptions" ) );
    vSettingsSubItems.push_back( pgettext( "Main Menu|Settings", "K<e|E>ybindings" ) );
    vSettingsSubItems.push_back( pgettext( "Main Menu|Settings", "<A|a>utopickup" ) );
    vSettingsSubItems.push_back( pgettext( "Main Menu|Settings", "<S|s>afemode" ) );
    vSettingsSubItems.push_back( pgettext( "Main Menu|Settings", "<C|c>olors" ) );

    vSettingsHotkeys.clear();
    for( auto item : vSettingsSubItems ) {
        vSettingsHotkeys.push_back( get_hotkeys( item ) );
    }
}

std::vector<std::string> main_menu::get_hotkeys( const std::string &s )
{
    std::vector<std::string> hotkeys;
    size_t start = s.find_first_of( '<' );
    size_t end = s.find_first_of( '>' );
    if( start != std::string::npos && end != std::string::npos ) {
        // hotkeys separated by '|' inside '<' and '>', for example "<e|E|?>"
        size_t lastsep = start;
        size_t sep = s.find_first_of( '|', start );
        while( sep < end ) {
            hotkeys.push_back( s.substr( lastsep + 1, sep - lastsep - 1 ) );
            lastsep = sep;
            sep = s.find_first_of( '|', sep + 1 );
        }
        hotkeys.push_back( s.substr( lastsep + 1, end - lastsep - 1 ) );
    }
    return hotkeys;
}

void main_menu::display_credits()
{
    // AStyle got this redundant indent
    catacurses::window w_credits_border = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                          ( TERMY > FULL_SCREEN_HEIGHT ) ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0,
                                          ( TERMX > FULL_SCREEN_WIDTH ) ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0 );
    catacurses::window w_credits = catacurses::newwin( FULL_SCREEN_HEIGHT - 2, FULL_SCREEN_WIDTH - 2,
                                   1 + ( int )( ( TERMY > FULL_SCREEN_HEIGHT ) ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0 ),
                                   1 + ( int )( ( TERMX > FULL_SCREEN_WIDTH ) ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0 ) );
    draw_border( w_credits_border, BORDER_COLOR, _( " CREDITS " ) );
    wrefresh( w_credits_border );
    catacurses::refresh();
    multipage( w_credits, mmenu_credits );
    catacurses::refresh();
}

bool main_menu::opening_screen()
{
    // Play title music, whoo!
    play_music( "title" );

    world_generator->set_active_world( NULL );
    world_generator->init();

    init_windows();
    init_strings();
    print_menu( w_open, 0, iMenuOffsetX, iMenuOffsetY );

    if( !assure_dir_exist( FILENAMES["config_dir"] ) ) {
        popup( _( "Unable to make config directory. Check permissions." ) );
        return false;
    }

    if( !assure_dir_exist( FILENAMES["savedir"] ) ) {
        popup( _( "Unable to make save directory. Check permissions." ) );
        return false;
    }

    if( !assure_dir_exist( FILENAMES["templatedir"] ) ) {
        popup( _( "Unable to make templates directory. Check permissions." ) );
        return false;
    }

    for( std::string path : get_files_from_path( ".template", FILENAMES["templatedir"], false,
            true ) ) {
        path = native_to_utf8( path );
        path.erase( path.find( ".template" ), std::string::npos );
        path.erase( 0, path.find_last_of( "\\//" ) + 1 );
        templates.push_back( path );
    }
    std::sort( templates.begin(), templates.end(), std::greater<std::string>() );

    ctxt.register_cardinal();
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "DELETE_TEMPLATE" );
    // for the menu shortcuts
    ctxt.register_action( "ANY_INPUT" );
    bool start = false;

    g->u = player();

    // Make [Load Game] the default cursor position if there's game save available
    if( !world_generator->all_worldnames().empty() ) {
        sel1 = 2;
    }

    while( !start ) {
        print_menu( w_open, sel1, iMenuOffsetX, iMenuOffsetY, ( sel1 != 0 ) );

        if( layer == 1 ) {
            if( sel1 == 0 ) { // Print the MOTD.
                const int motdy = ( iMenuOffsetY - mmenu_motd.size() ) * 2 / 3;
                const int motdx = 8 + extra_w / 2;
                for( size_t i = 0; i < mmenu_motd.size(); i++ ) {
                    mvwprintz( w_open, motdy + i, motdx, c_light_red, mmenu_motd[i].c_str() );
                }

                wrefresh( w_open );
                catacurses::refresh();
            }

            std::string action = handle_input_timeout( ctxt );

            std::string sInput = ctxt.get_raw_input().text;
            // check automatic menu shortcuts
            for( size_t i = 0; i < vMenuHotkeys.size(); ++i ) {
                for( auto hotkey : vMenuHotkeys[i] ) {
                    if( sInput == hotkey ) {
                        sel1 = i;
                        action = "CONFIRM";
                    }
                }
            }
            // also check special keys
            if( action == "QUIT" ) {
                if( query_yn( _( "Really quit?" ) ) ) {
                    sel1 = 8;
                    action = "CONFIRM";
                }
            } else if( action == "LEFT" ) {
                if( sel1 > 0 ) {
                    sel1--;
                } else {
                    sel1 = 8;
                }
                on_move();
            } else if( action == "RIGHT" ) {
                if( sel1 < 8 ) {
                    sel1++;
                } else {
                    sel1 = 0;
                }
                on_move();
            }
            if( ( action == "UP" || action == "CONFIRM" ) && sel1 > 0 ) {
                if( sel1 == 6 ) {
                    display_help();
                } else if( sel1 == 7 ) {
                    display_credits();
                } else if( sel1 == 8 ) {
                    return false;
                } else {
                    sel2 = 0;
                    layer = 2;
                    print_menu( w_open, sel1, iMenuOffsetX, iMenuOffsetY, ( sel1 != 0 ) );

                    switch( sel1 ) {
                        case 1:
                            start = new_character_tab();
                            break;
                        case 2:
                            start = load_character_tab();
                            break;
                        case 3:
                            world_tab();
                            break;
                        default:
                            break;
                    }
                }
            }
        } else if( layer == 2 ) {
            if( sel1 == 4 ) { // Special game
                if( MAP_SHARING::isSharing() ) { // Thee can't save special games, therefore thee can't share them
                    layer = 1;
                    popup( _( "Special games don't work with shared maps." ) );
                    continue;
                }

                std::vector<std::string> special_names;
                int xoffset = 32 + iMenuOffsetX  + extra_w / 2;
                int yoffset = iMenuOffsetY - 2;
                int xlen = 0;
                for( int i = 1; i < NUM_SPECIAL_GAMES; i++ ) {
                    std::string spec_name = special_game_name( special_game_id( i ) );
                    special_names.push_back( spec_name );
                    xlen += spec_name.size() + 2;
                }
                xlen += special_names.size() - 1;
                print_menu_items( w_open, special_names, sel2, yoffset, xoffset - ( xlen / 4 ) );

                wrefresh( w_open );
                catacurses::refresh();
                std::string action = handle_input_timeout( ctxt );
                if( action == "LEFT" ) {
                    if( sel2 > 0 ) {
                        sel2--;
                    } else {
                        sel2 = NUM_SPECIAL_GAMES - 2;
                    }
                    on_move();
                } else if( action == "RIGHT" ) {
                    if( sel2 < NUM_SPECIAL_GAMES - 2 ) {
                        sel2++;
                    } else {
                        sel2 = 0;
                    }
                    on_move();
                } else if( action == "DOWN" || action == "QUIT" ) {
                    layer = 1;
                }
                if( action == "UP" || action == "CONFIRM" ) {
                    if( sel2 >= 0 && sel2 < NUM_SPECIAL_GAMES - 1 ) {
                        g->gamemode = get_special_game( special_game_id( sel2 + 1 ) );
                        // check world
                        WORLDPTR world = world_generator->make_new_world( special_game_id( sel2 + 1 ) );
                        if( world == NULL ) {
                            continue;
                        }
                        world_generator->set_active_world( world );
                        try {
                            g->setup();
                        } catch( const std::exception &err ) {
                            debugmsg( "Error: %s", err.what() );
                            g->gamemode.reset();
                            g->u = player();
                            continue;
                        }
                        if( !g->gamemode->init() ) {
                            g->gamemode.reset();
                            g->u = player();
                            continue;
                        }
                        start = true;
                    }
                }
            } else if( sel1 == 5 ) {  // Settings Menu
                int settings_subs_to_display = vSettingsSubItems.size();
                std::vector<std::string> settings_subs;
                int xoffset = 46 + iMenuOffsetX + extra_w / 2;
                int yoffset = iMenuOffsetY - 2;
                int xlen = 0;
                for( int i = 0; i < settings_subs_to_display; ++i ) {
                    settings_subs.push_back( vSettingsSubItems[i] );
                    xlen += vSettingsSubItems[i].size() + 2; // Open and close brackets added
                }
                xlen += settings_subs.size() - 1;
                if( settings_subs.size() > 1 ) {
                    xoffset -= 6;
                }
                print_menu_items( w_open, settings_subs, sel2, yoffset, xoffset - ( xlen / 4 ) );
                wrefresh( w_open );
                catacurses::refresh();
                std::string action = handle_input_timeout( ctxt );
                std::string sInput = ctxt.get_raw_input().text;
                for( int i = 0; i < settings_subs_to_display; ++i ) {
                    for( auto hotkey : vSettingsHotkeys[i] ) {
                        if( sInput == hotkey ) {
                            sel2 = i;
                            action = "CONFIRM";
                        }
                    }
                }

                if( action == "LEFT" ) {
                    if( sel2 > 0 ) {
                        --sel2;
                    } else {
                        sel2 = settings_subs_to_display - 1;
                    }
                    on_move();
                } else if( action == "RIGHT" ) {
                    if( sel2 < settings_subs_to_display - 1 ) {
                        ++sel2;
                    } else {
                        sel2 = 0;
                    }
                    on_move();
                } else if( action == "DOWN" || action == "QUIT" ) {
                    layer = 1;
                }

                if( action == "UP" || action == "CONFIRM" ) {
                    if( sel2 == 0 ) {
                        get_options().show( true );
                        // The language may have changed- gracefully handle this.
                        init_strings();
                        print_menu( w_open, sel1, iMenuOffsetX, iMenuOffsetY, ( sel1 != 0 ) );
                    } else if( sel2 == 1 ) {
                        input_context ctxt_default = get_default_mode_input_context();
                        ctxt_default.display_menu();
                    } else if( sel2 == 2 ) {
                        get_auto_pickup().show();
                    } else if( sel2 == 3 ) {
                        get_safemode().show();
                    } else if( sel2 == 4 ) {
                        all_colors.show_gui();
                    }
                }
            }
        }
    }
    if( start ) {
        g->refresh_all();
        g->draw();
    }
    return start;
}

bool main_menu::new_character_tab()
{
    std::vector<std::string> vSubItems;
    vSubItems.push_back( pgettext( "Main Menu|New Game", "<C|c>ustom Character" ) );
    vSubItems.push_back( pgettext( "Main Menu|New Game", "<P|p>reset Character" ) );
    vSubItems.push_back( pgettext( "Main Menu|New Game", "<R|r>andom Character" ) );
    if( !MAP_SHARING::isSharing() ) { // "Play Now" function doesn't play well together with shared maps
        vSubItems.push_back( pgettext( "Main Menu|New Game", "Play Now! (<F|f>ixed Scenario)" ) );
        vSubItems.push_back( pgettext( "Main Menu|New Game", "Play <N|n>ow!" ) );
    }
    std::vector<std::vector<std::string>> vNewGameHotkeys;
    for( auto item : vSubItems ) {
        vNewGameHotkeys.push_back( get_hotkeys( item ) );
    }

    bool start = false;
    while( !start && sel1 == 1 && ( layer == 2 || layer == 3 ) ) {
        print_menu( w_open, 1, iMenuOffsetX, iMenuOffsetY, true );
        if( layer == 2 && sel1 == 1 ) {
            // Then choose custom character, random character, preset, etc
            if( MAP_SHARING::isSharing() &&
                world_generator->all_worldnames().empty() ) { //don't show anything when there are no worlds (will not work if there are special maps)
                layer = 1;
                sel1 = 1;
                continue;
            }

            print_menu_items( w_open, vSubItems, sel2, iMenuOffsetY - 2, iMenuOffsetX );
            wrefresh( w_open );
            catacurses::refresh();

            std::string action = handle_input_timeout( ctxt );
            std::string sInput = ctxt.get_raw_input().text;
            for( size_t i = 0; i < vNewGameHotkeys.size(); ++i ) {
                for( auto hotkey : vNewGameHotkeys[i] ) {
                    if( sInput == hotkey ) {
                        sel2 = i;
                        action = "CONFIRM";
                    }
                }
            }
            if( action == "LEFT" ) {
                sel2--;
                if( sel2 < 0 ) {
                    sel2 = vSubItems.size() - 1;
                }
                on_move();
            } else if( action == "RIGHT" ) {
                sel2++;
                if( sel2 >= ( int )vSubItems.size() ) {
                    sel2 = 0;
                }
                on_move();
            } else if( action == "DOWN" || action == "QUIT" ) {
                layer = 1;
                sel1 = 1;
            }
            if( action == "UP" || action == "CONFIRM" ) {
                if( sel2 == 0 || sel2 == 2 || sel2 == 3 || sel2 == 4 ) {
                    // First load the mods, this is done by
                    // loading the world.
                    // Pick a world, suppressing prompts if it's "play now" mode.
                    WORLDPTR world = world_generator->pick_world( sel2 != 3 && sel2 != 4 );
                    if( world == NULL ) {
                        continue;
                    }
                    world_generator->set_active_world( world );
                    try {
                        g->setup();
                    } catch( const std::exception &err ) {
                        debugmsg( "Error: %s", err.what() );
                        g->u = player();
                        continue;
                    }
                    character_type play_type = PLTYPE_CUSTOM;
                    switch( sel2 ) {
                        case 0:
                            play_type = PLTYPE_CUSTOM;
                            break;
                        case 2:
                            play_type = PLTYPE_RANDOM;
                            break;
                        case 3:
                            play_type = PLTYPE_NOW;
                            break;
                        case 4:
                            play_type = PLTYPE_FULL_RANDOM;
                            break;
                    }
                    if( !g->u.create( play_type ) ) {
                        g->u = player();
                        werase( w_background );
                        wrefresh( w_background );
                        continue;
                    }

                    werase( w_background );
                    wrefresh( w_background );

                    if( !g->start_game() ) {
                        g->u = player();
                        continue;
                    }
                    start = true;
                } else if( sel2 == 1 ) {
                    layer = 3;
                    sel3 = 0;
                }
            }
        } else if( layer == 3 && sel1 == 1 ) {
            // Then view presets
            if( templates.empty() ) {
                mvwprintz( w_open, iMenuOffsetY - 4, iMenuOffsetX + 20 + extra_w / 2,
                           c_red, _( "No templates found!" ) );
                sfx::play_variant_sound( "menu_error", "default", 100 );
            } else {
                mvwprintz( w_open, iMenuOffsetY - 2, iMenuOffsetX + 20 + extra_w / 2,
                           c_white, _( "Press 'd' to delete a preset." ) );
                for( int i = 0; i < ( int )templates.size(); i++ ) {
                    int line = iMenuOffsetY - 4 - i;
                    mvwprintz( w_open, line, 20 + iMenuOffsetX + extra_w / 2,
                               ( sel3 == i ? h_white : c_white ), templates[i].c_str() );
                }
            }
            wrefresh( w_open );
            catacurses::refresh();
            std::string action = handle_input_timeout( ctxt );
            if( action == "DOWN" ) {
                if( sel3 > 0 ) {
                    sel3--;
                } else {
                    sel3 = templates.size() - 1;
                }
            } else if( templates.empty() && ( action == "UP" || action == "CONFIRM" ) ) {
                sel1 = 1;
                layer = 2;
                print_menu( w_open, sel1, iMenuOffsetX, iMenuOffsetY );
            } else if( action == "UP" ) {
                if( sel3 < ( int )templates.size() - 1 ) {
                    sel3++;
                } else {
                    sel3 = 0;
                }
            } else if( action == "LEFT"  || action == "QUIT" || templates.empty() ) {
                sel1 = 1;
                layer = 2;
                print_menu( w_open, sel1, iMenuOffsetX, iMenuOffsetY );
            } else if( !templates.empty() && action == "DELETE_TEMPLATE" ) {
                if( query_yn( _( "Are you sure you want to delete %s?" ),
                              templates[sel3].c_str() ) ) {
                    const auto path = FILENAMES["templatedir"] + utf8_to_native( templates[sel3] ) + ".template";
                    if( std::remove( path.c_str() ) != 0 ) {
                        popup( _( "Sorry, something went wrong." ) );
                    } else {
                        templates.erase( templates.begin() + sel3 );
                        if( ( size_t )sel3 > templates.size() - 1 ) {
                            sel3--;
                        }
                    }
                }
            } else if( action == "RIGHT" || action == "CONFIRM" ) {
                WORLDPTR world = world_generator->pick_world();
                if( world == NULL ) {
                    g->u = player();
                    continue;
                }
                world_generator->set_active_world( world );
                try {
                    g->setup();
                } catch( const std::exception &err ) {
                    debugmsg( "Error: %s", err.what() );
                    g->u = player();
                    continue;
                }
                if( !g->u.create( PLTYPE_TEMPLATE, templates[sel3] ) ) {
                    g->u = player();
                    werase( w_background );
                    wrefresh( w_background );
                    continue;
                }
                werase( w_background );
                wrefresh( w_background );
                if( !g->start_game() ) {
                    g->u = player();
                    continue;
                }
                start = true;
            }
        }
    } // end while
    return start;
}

bool main_menu::load_character_tab()
{
    bool start = false;
    while( !start && sel1 == 2 && ( layer == 2 || layer == 3 ) ) {
        print_menu( w_open, 2, iMenuOffsetX, iMenuOffsetY, true );
        const auto all_worldnames = world_generator->all_worldnames();
        if( layer == 2 && sel1 == 2 ) {
            if( all_worldnames.empty() ) {
                mvwprintz( w_open, iMenuOffsetY - 2, 15 + iMenuOffsetX + extra_w / 2,
                           c_red, _( "No Worlds found!" ) );
                sfx::play_variant_sound( "menu_error", "default", 100 );
            } else {
                for( int i = 0; i < ( int )all_worldnames.size(); ++i ) {
                    int line = iMenuOffsetY - 2 - i;
                    std::string world_name = all_worldnames[i];
                    int savegames_count = world_generator->get_world( world_name )->world_saves.size();
                    nc_color color1, color2;
                    if( world_name == "TUTORIAL" || world_name == "DEFENSE" ) {
                        color1 = c_light_cyan;
                        color2 = h_light_cyan;
                    } else {
                        if( world_generator->world_need_lua_build( world_name ) ) {
                            color1 = c_dark_gray;
                            color2 = h_dark_gray;
                        } else {
                            color1 = c_white;
                            color2 = h_white;
                        }
                    }
                    mvwprintz( w_open, line, 15 + iMenuOffsetX + extra_w / 2,
                               ( sel2 == i ? color2 : color1 ), "%s (%d)",
                               world_name.c_str(), savegames_count );
                }
            }
            wrefresh( w_open );
            catacurses::refresh();
            std::string action = handle_input_timeout( ctxt );
            if( all_worldnames.empty() && ( action == "DOWN" || action == "CONFIRM" ) ) {
                layer = 1;
            } else if( action == "DOWN" ) {
                if( sel2 > 0 ) {
                    sel2--;
                } else {
                    sel2 = all_worldnames.size() - 1;
                }
            } else if( action == "UP" ) {
                if( sel2 < ( int )all_worldnames.size() - 1 ) {
                    sel2++;
                } else {
                    sel2 = 0;
                }
            } else if( action == "LEFT" || action == "QUIT" ) {
                layer = 1;
            } else if( action == "RIGHT" || action == "CONFIRM" ) {
                if( sel2 >= 0 && sel2 < ( int )all_worldnames.size() ) {
                    layer = 3;
                    sel3 = 0;
                }
            }
        } else if( layer == 3 && sel1 == 2 ) {
            savegames = world_generator->get_world( all_worldnames[sel2] )->world_saves;
            const std::string &wn = all_worldnames[sel2];

            if( MAP_SHARING::isSharing() ) {
                auto new_end = std::remove_if( savegames.begin(), savegames.end(),
                []( const save_t &str ) {
                    return str.player_name() != MAP_SHARING::getUsername();
                } );
                savegames.erase( new_end, savegames.end() );
            }

            mvwprintz( w_open, iMenuOffsetY - 2 - sel2, 15 + iMenuOffsetX + extra_w / 2, h_white, wn.c_str() );

            if( ( wn != "TUTORIAL" && wn != "DEFENSE" ) && world_generator->world_need_lua_build( wn ) ) {
                savegames.clear();
                mvwprintz( w_open, iMenuOffsetY - 2 - sel2, 40 + iMenuOffsetX + extra_w / 2,
                           c_red, _( "This world requires the game to be compiled with Lua." ) );
                sfx::play_variant_sound( "menu_error", "default", 100 );
            } else if( savegames.empty() ) {
                mvwprintz( w_open, iMenuOffsetY - 2 - sel2, 40 + iMenuOffsetX + extra_w / 2,
                           c_red, _( "No save games found!" ) );
                sfx::play_variant_sound( "menu_error", "default", 100 );
            } else {
                int line = iMenuOffsetY - 2;

                for( const auto &savename : savegames ) {
                    const bool selected = sel3 + line == iMenuOffsetY - 2;
                    mvwprintz( w_open, line--, 40 + iMenuOffsetX + extra_w / 2,
                               selected ? h_white : c_white, savename.player_name().c_str() );
                }
            }
            wrefresh( w_open );
            catacurses::refresh();
            std::string action = handle_input_timeout( ctxt );
            if( savegames.empty() && ( action == "DOWN" || action == "CONFIRM" ) ) {
                layer = 2;
            } else if( action == "DOWN" ) {
                if( sel3 > 0 ) {
                    sel3--;
                } else {
                    sel3 = savegames.size() - 1;
                }
            } else if( action == "UP" ) {
                if( sel3 < ( int )savegames.size() - 1 ) {
                    sel3++;
                } else {
                    sel3 = 0;
                }
            } else if( action == "LEFT" || action == "QUIT" ) {
                layer = 2;
                sel3 = 0;
                print_menu( w_open, sel1, iMenuOffsetX, iMenuOffsetY );
            }
            if( action == "RIGHT" || action == "CONFIRM" ) {
                if( sel3 >= 0 && sel3 < ( int )savegames.size() ) {
                    werase( w_background );
                    wrefresh( w_background );
                    WORLDPTR world = world_generator->get_world( all_worldnames[sel2] );
                    world_generator->set_active_world( world );
                    try {
                        g->setup();
                    } catch( const std::exception &err ) {
                        debugmsg( "Error: %s", err.what() );
                        g->u = player();
                        continue;
                    }

                    g->load( savegames[sel3] );
                    start = true;
                }
            }
        }
    } // end while
    return start;
}

void main_menu::world_tab()
{
    while( sel1 == 3 && ( layer == 2 || layer == 3 ) ) {
        print_menu( w_open, 3, iMenuOffsetX, iMenuOffsetY, true );
        if( layer == 3 ) { // World Menu
            // Show options for Destroy, Reset worlds.
            // Reset and Destroy ask for world to modify.
            // Reset empties world of everything but options, then makes new world within it.
            // Destroy asks for confirmation, then destroys everything in world and then removes world folder.

            int xoffset = 40 + iMenuOffsetX + extra_w / 2;
            int yoffset = iMenuOffsetY - 2 - sel2;

            const auto all_worldnames = world_generator->all_worldnames();
            mvwprintz( w_open, yoffset, xoffset - 15, h_white, all_worldnames[sel2 - 1].c_str() );

            for( size_t i = 0; i < vWorldSubItems.size(); ++i ) {
                nc_color text_color;
                nc_color key_color;
                if( sel3 == static_cast<int>( i ) ) {
                    text_color = h_white;
                    key_color = h_white;
                } else {
                    text_color = c_light_gray;
                    key_color = c_white;
                }
                wmove( w_open, yoffset - i, xoffset );
                wprintz( w_open, c_light_gray, "[" );
                shortcut_print( w_open, text_color, key_color, vWorldSubItems[i] );
                wprintz( w_open, c_light_gray, "]" );
            }

            wrefresh( w_open );
            catacurses::refresh();
            std::string action = handle_input_timeout( ctxt );
            std::string sInput = ctxt.get_raw_input().text;
            for( size_t i = 0; i < vWorldSubItems.size(); ++i ) {
                for( auto hotkey : vWorldHotkeys[i] ) {
                    if( sInput == hotkey ) {
                        sel3 = i;
                        action = "CONFIRM";
                    }
                }
            }

            if( action == "DOWN" ) {
                if( sel3 > 0 ) {
                    --sel3;
                } else {
                    sel3 = vWorldSubItems.size() - 1;
                }
                on_move();
            } else if( action == "UP" ) {
                if( sel3 < static_cast<int>( vWorldSubItems.size() ) - 1 ) {
                    ++sel3;
                } else {
                    sel3 = 0;
                }
                on_move();
            } else if( action == "LEFT" || action == "QUIT" ) {
                layer = 2;
            }

            if( action == "RIGHT" || action == "CONFIRM" ) {
                if( sel3 == 2 ) { // Active World Mods
                    WORLDPTR world = world_generator->get_world( all_worldnames[sel2 - 1] );
                    world_generator->show_active_world_mods( world->active_mod_order );

                } else {
                    bool query_yes = false;
                    bool do_delete = false;
                    if( sel3 == 0 ) { // Delete World
                        if( query_yn( _( "Delete the world and all saves?" ) ) ) {
                            query_yes = true;
                            do_delete = true;
                        }
                    } else if( sel3 == 1 ) { // Reset World
                        if( query_yn( _( "Remove all saves and regenerate world?" ) ) ) {
                            query_yes = true;
                            do_delete = false;
                        }
                    }

                    if( query_yes ) {
                        layer = 2; // Go to world submenu, not list of worlds

                        world_generator->delete_world( all_worldnames[sel2 - 1], do_delete );

                        savegames.clear();
                        MAPBUFFER.reset();
                        overmap_buffer.clear();

                        if( do_delete ) {
                            sel2 = 0; // reset to create world selection
                        }
                    }
                }
            }
        } else if( layer == 2 ) { // Show world names
            if( MAP_SHARING::isSharing() && !MAP_SHARING::isWorldmenu() && !MAP_SHARING::isAdmin() ) {
                layer = 1;
                popup( _( "Only the admin can change worlds." ) );
                continue;
            }

            mvwprintz( w_open, iMenuOffsetY - 2, 25 + iMenuOffsetX + extra_w / 2,
                       ( sel2 == 0 ? h_white : c_white ), _( "Create World" ) );

            int i = 1;
            const auto all_worldnames = world_generator->all_worldnames();
            for( auto it = all_worldnames.begin(); it != all_worldnames.end(); ++it, i++ ) {
                int savegames_count = world_generator->get_world( *it )->world_saves.size();
                int line = iMenuOffsetY - 2 - i;
                nc_color color1, color2;
                if( *it == "TUTORIAL" || *it == "DEFENSE" ) {
                    color1 = c_light_cyan;
                    color2 = h_light_cyan;
                } else {
                    color1 = c_white;
                    color2 = h_white;
                }
                mvwprintz( w_open, line, 25 + iMenuOffsetX + extra_w / 2,
                           ( sel2 == i ? color2 : color1 ), "%s (%d)", ( *it ).c_str(), savegames_count );
            }

            wrefresh( w_open );
            catacurses::refresh();
            std::string action = handle_input_timeout( ctxt );

            if( action == "DOWN" ) {
                if( sel2 > 0 ) {
                    --sel2;
                } else {
                    sel2 = all_worldnames.size();
                }
            } else if( action == "UP" ) {
                if( sel2 < ( int )all_worldnames.size() ) {
                    ++sel2;
                } else {
                    sel2 = 0;
                }
            } else if( action == "LEFT" || action == "QUIT" ) {
                layer = 1;
            }
            if( action == "RIGHT" || action == "CONFIRM" ) {
                if( sel2 == 0 ) {
                    world_generator->make_new_world();

                } else {
                    layer = 3;
                    sel3 = 0;
                }
            }
        }
    } // end while layer == ...
}
