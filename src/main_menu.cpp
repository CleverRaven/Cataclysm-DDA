#include "main_menu.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <sstream>

#include "auto_pickup.h"
#include "avatar.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "debug.h"
#include "filesystem.h"
#include "game.h"
#include "gamemode.h"
#include "get_version.h"
#include "help.h"
#include "ime.h"
#include "loading_ui.h"
#include "mapbuffer.h"
#include "mapsharing.h"
#include "output.h"
#include "overmapbuffer.h"
#include "path_info.h"
#include "safemode_ui.h"
#include "scenario.h"
#include "sdlsound.h"
#include "sounds.h"
#include "text_snippets.h"
#include "translations.h"
#include "worldfactory.h"
#include "color.h"
#include "enums.h"
#include "options.h"
#include "pldata.h"
#include "string_formatter.h"

#define dbg(x) DebugLog((DebugLevel)(x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

static const bool halloween_theme = false;

void main_menu::on_move() const
{
    sfx::play_variant_sound( "menu_move", "default", 100 );
}

void main_menu::on_error()
{
    if( errflag ) {
        return;
    }
    sfx::play_variant_sound( "menu_error", "default", 100 );
    errflag = true;
}

void main_menu::clear_error()
{
    errflag = false;
}

void main_menu::print_menu_items( const catacurses::window &w_in,
                                  const std::vector<std::string> &vItems,
                                  size_t iSel, point offset, int spacing )
{
    std::string text;
    for( size_t i = 0; i < vItems.size(); ++i ) {
        if( i > 0 ) {
            text += std::string( spacing, ' ' );
        }

        std::string temp = shortcut_text( c_white, vItems[i] );
        if( iSel == i ) {
            text += string_format( "[<color_%s>%s</color>]",
                                   string_from_color( h_white ),
                                   remove_color_tags( temp ) );
        } else {
            text += string_format( "[%s]", temp );
        }
    }

    if( utf8_width( remove_color_tags( text ) ) > getmaxx( w_in ) ) {
        offset.y -= std::ceil( utf8_width( remove_color_tags( text ) ) / getmaxx( w_in ) );
    }

    fold_and_print( w_in, offset, getmaxx( w_in ), c_light_gray, text, ']' );
}

void main_menu::print_menu( const catacurses::window &w_open, int iSel, const point &offset )
{
    // Clear Lines
    werase( w_open );

    // Define window size
    int window_width = getmaxx( w_open );
    int window_height = getmaxy( w_open );

    // Draw horizontal line
    for( int i = 1; i < window_width - 1; ++i ) {
        mvwputch( w_open, point( i, window_height - 4 ), c_white, LINE_OXOX );
    }

    center_print( w_open, window_height - 2, c_red,
                  _( "Bugs?  Suggestions?  Use links in MOTD to report them." ) );

    center_print( w_open, window_height - 1, c_light_cyan, string_format( _( "Tip of the day: %s" ),
                  vdaytip ) );

    int iLine = 0;
    const int iOffsetX = ( window_width - FULL_SCREEN_WIDTH ) / 2;

    const nc_color cColor1 = c_light_cyan;
    const nc_color cColor2 = c_light_blue;
    const nc_color cColor3 = c_light_blue;

    if( halloween_theme ) {
        fold_and_print_from( w_open, point_zero, 30, 0, c_white, halloween_spider() );
        fold_and_print_from( w_open, point( getmaxx( w_open ) - 25, offset.y - 8 ),
                             25, 0, c_white, halloween_graves() );
    }

    if( mmenu_title.size() > 1 ) {
        for( size_t i = 0; i < mmenu_title.size(); ++i ) {
            if( halloween_theme ) {
                static const std::string marker = "█";
                const utf8_wrapper text( mmenu_title[i] );
                for( size_t j = 0; j < text.size(); j++ ) {
                    std::string temp = text.substr_display( j, 1 ).str();
                    if( temp != " " ) {
                        mvwprintz( w_open, point( iOffsetX + j, iLine ),
                                   ( temp != marker ) ? c_red : ( i < 9  ? cColor1 : cColor2 ),
                                   "%s", ( temp == marker ) ? "▓" : temp );
                    }
                }
                iLine++;
            } else {
                mvwprintz( w_open, point( iOffsetX, iLine++ ), i < 6 ? cColor1 : cColor2, "%s", mmenu_title[i] );
            }
        }
    } else {
        center_print( w_open, iLine++, cColor1, mmenu_title[0] );
    }

    iLine++;
    center_print( w_open, iLine++, cColor3, string_format( _( "Version: %s" ), getVersionString() ) );

    int menu_length = 0;
    for( size_t i = 0; i < vMenuItems.size(); ++i ) {
        menu_length += utf8_width( vMenuItems[i], true ) + 2;
        if( !vMenuHotkeys[i].empty() ) {
            menu_length += utf8_width( vMenuHotkeys[i][0] );
        }
    }
    const int free_space = std::max( 0, window_width - menu_length - offset.x );
    const int spacing = free_space / ( static_cast<int>( vMenuItems.size() ) + 1 );
    const int width_of_spacing = spacing * ( vMenuItems.size() + 1 );
    const int adj_offset = std::max( 0, ( free_space - width_of_spacing ) / 2 );
    const int final_offset = offset.x + adj_offset + spacing;

    print_menu_items( w_open, vMenuItems, iSel, point( final_offset, offset.y ), spacing );

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
    std::string action = ctxt.handle_input( 125 );

    if( action == "TIMEOUT" ) {
        init_windows();
    }

    return action;
}

void main_menu::init_windows()
{
    if( LAST_TERMX == TERMX && LAST_TERMY == TERMY ) {
        return;
    }

    w_background = catacurses::newwin( TERMY, TERMX, point_zero );
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

    w_open = catacurses::newwin( total_h, total_w, point( x0, y0 ) );

    menu_offset.y = total_h - 3;
    // note: if iMenuOffset is changed,
    // please update MOTD and credits to indicate how long they can be.

    LAST_TERMX = TERMX;
    LAST_TERMY = TERMY;
}

void main_menu::init_strings()
{
    // ASCII Art
    mmenu_title = load_file( PATH_INFO::find_translated_file( "titledir",
                             halloween_theme ? ".halloween" : ".title",
                             halloween_theme ? "halloween" : "title" ),
                             _( "Cataclysm: Dark Days Ahead" ) );
    // MOTD
    auto motd = load_file( PATH_INFO::find_translated_file( "motddir", ".motd", "motd" ),
                           _( "No message today." ) );

    std::ostringstream buffer;
    mmenu_motd.clear();
    for( auto &line : motd ) {
        buffer << ( line.empty() ? " " : line ) << std::endl;
    }
    mmenu_motd = "<color_light_red>" + buffer.str() + "</color>";

    buffer.str( "" );

    // Credits
    mmenu_credits.clear();
    read_from_file_optional( PATH_INFO::find_translated_file( "creditsdir", ".credits",
    "credits" ), [&buffer]( std::istream & stream ) {
        std::string line;
        while( std::getline( stream, line ) ) {
            if( line[0] != '#' ) {
                buffer << ( line.empty() ? " " : line ) << std::endl;
            }
        }
    } );

    mmenu_credits = buffer.str();

    if( mmenu_credits.empty() ) {
        mmenu_credits = _( "No credits information found." );
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
    vWorldSubItems.push_back( pgettext( "Main Menu|World", "<C|c>opy World Settings" ) );

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
    for( const std::string &item : vSettingsSubItems ) {
        vSettingsHotkeys.push_back( get_hotkeys( item ) );
    }

    loading_ui ui( false );
    g->load_core_data( ui );
    vdaytip = SNIPPET.random_from_category( "tip" );
}

void main_menu::display_text( const std::string &text, const std::string &title, int &selected )
{
    catacurses::window w_border = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                  point( TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0,
                                         TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0 ) );

    catacurses::window w_text = catacurses::newwin( FULL_SCREEN_HEIGHT - 2, FULL_SCREEN_WIDTH - 2,
                                point( 1 + static_cast<int>( TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0 ),
                                       1 + static_cast<int>( TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0 ) ) );

    draw_border( w_border, BORDER_COLOR, title );

    int width = FULL_SCREEN_WIDTH - 2;
    int height = FULL_SCREEN_HEIGHT - 2;
    const auto vFolded = foldstring( text, width );
    int iLines = vFolded.size();

    if( selected < 0 ) {
        selected = 0;
    } else if( iLines < height ) {
        selected = 0;
    } else if( selected >= iLines - height ) {
        selected = iLines - height;
    }

    fold_and_print_from( w_text, point_zero, width, selected, c_light_gray, text );

    draw_scrollbar( w_border, selected, height, iLines, point_south, BORDER_COLOR, true );
    wrefresh( w_border );
    wrefresh( w_text );
    catacurses::refresh();
}

void main_menu::load_char_templates()
{
    templates.clear();

    for( std::string path : get_files_from_path( ".template", FILENAMES["templatedir"], false,
            true ) ) {
        path = native_to_utf8( path );
        path.erase( path.find( ".template" ), std::string::npos );
        path.erase( 0, path.find_last_of( "\\//" ) + 1 );
        templates.push_back( path );
    }
    std::sort( templates.begin(), templates.end(), std::greater<std::string>() );
}

bool main_menu::opening_screen()
{
    // Play title music, whoo!
    play_music( "title" );

    world_generator->set_active_world( nullptr );
    world_generator->init();

    get_help().load();
    init_windows();
    init_strings();
    print_menu( w_open, 0, menu_offset );

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

    if( !assure_dir_exist( FILENAMES["user_sound"] ) ) {
        popup( _( "Unable to make sound directory. Check permissions." ) );
        return false;
    }

    load_char_templates();

    ctxt.register_cardinal();
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "DELETE_TEMPLATE" );
    ctxt.register_action( "PAGE_UP" );
    ctxt.register_action( "PAGE_DOWN" );
    // for the menu shortcuts
    ctxt.register_action( "ANY_INPUT" );
    bool start = false;

    g->u = avatar();

    int sel_line = 0;

    // Make [Load Game] the default cursor position if there's game save available
    if( !world_generator->all_worldnames().empty() ) {
        sel1 = 2;
    }

    while( !start ) {
        // disable ime at program start
        // somehow this need to be here to actually work
        disable_ime();

        print_menu( w_open, sel1, menu_offset );

        if( layer == 1 ) {
            if( sel1 == 0 ) { // Print MOTD.
                display_text( mmenu_motd, "MOTD", sel_line );

            } else if( sel1 == 7 ) { // Print Credits.
                display_text( mmenu_credits, "Credits", sel_line );
            }

            std::string action = handle_input_timeout( ctxt );

            std::string sInput = ctxt.get_raw_input().text;
            // check automatic menu shortcuts
            for( size_t i = 0; i < vMenuHotkeys.size(); ++i ) {
                for( const std::string &hotkey : vMenuHotkeys[i] ) {
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
                sel_line = 0;
                if( sel1 > 0 ) {
                    sel1--;
                } else {
                    sel1 = 8;
                }
                on_move();
            } else if( action == "RIGHT" ) {
                sel_line = 0;
                if( sel1 < 8 ) {
                    sel1++;
                } else {
                    sel1 = 0;
                }
                on_move();
            }

            if( ( sel1 == 0 || sel1 == 7 ) && ( action == "UP" || action == "DOWN" ||
                                                action == "PAGE_UP" || action == "PAGE_DOWN" ) ) {
                if( action == "UP" || action == "PAGE_UP" ) {
                    sel_line--;
                } else if( action == "DOWN" || action == "PAGE_DOWN" ) {
                    sel_line++;
                }

            }
            if( ( action == "UP" || action == "CONFIRM" ) && sel1 != 0 && sel1 != 7 ) {
                if( sel1 == 6 ) {
                    get_help().display_help();
                } else if( sel1 == 8 ) {
                    return false;
                } else {
                    sel2 = 0;
                    layer = 2;
                    print_menu( w_open, sel1, menu_offset );

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
                int xlen = 0;
                for( int i = 1; i < NUM_SPECIAL_GAMES; i++ ) {
                    std::string spec_name = special_game_name( static_cast<special_game_id>( i ) );
                    special_names.push_back( spec_name );
                    xlen += spec_name.size() + 2;
                }
                xlen += special_names.size() - 1;
                point offset( menu_offset + point( -( xlen / 4 ) + 32 + extra_w / 2, -2 ) );
                print_menu_items( w_open, special_names, sel2, offset );

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
                        g->gamemode = get_special_game( static_cast<special_game_id>( sel2 + 1 ) );
                        // check world
                        WORLDPTR world = world_generator->make_new_world( static_cast<special_game_id>( sel2 + 1 ) );
                        if( world == nullptr ) {
                            continue;
                        }
                        world_generator->set_active_world( world );
                        try {
                            g->setup();
                        } catch( const std::exception &err ) {
                            debugmsg( "Error: %s", err.what() );
                            g->gamemode.reset();
                            g->u = avatar();
                            continue;
                        }
                        if( !g->gamemode->init() ) {
                            g->gamemode.reset();
                            g->u = avatar();
                            continue;
                        }
                        start = true;
                    }
                }
            } else if( sel1 == 5 ) {  // Settings Menu
                int settings_subs_to_display = vSettingsSubItems.size();
                std::vector<std::string> settings_subs;
                int xlen = 0;
                for( int i = 0; i < settings_subs_to_display; ++i ) {
                    settings_subs.push_back( vSettingsSubItems[i] );
                    xlen += vSettingsSubItems[i].size() + 2; // Open and close brackets added
                }
                xlen += settings_subs.size() - 1;
                point offset = menu_offset + point( 46 + extra_w / 2 - ( xlen / 4 ), -2 );
                if( settings_subs.size() > 1 ) {
                    offset.x -= 6;
                }
                print_menu_items( w_open, settings_subs, sel2, offset );
                wrefresh( w_open );
                catacurses::refresh();
                std::string action = handle_input_timeout( ctxt );
                std::string sInput = ctxt.get_raw_input().text;
                for( int i = 0; i < settings_subs_to_display; ++i ) {
                    for( const std::string &hotkey : vSettingsHotkeys[i] ) {
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
                        print_menu( w_open, sel1, menu_offset );
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
    vNewGameHotkeys.reserve( vSubItems.size() );
    for( const std::string &item : vSubItems ) {
        vNewGameHotkeys.push_back( get_hotkeys( item ) );
    }

    bool start = false;
    while( !start && sel1 == 1 && ( layer == 2 || layer == 3 ) ) {
        print_menu( w_open, 1, menu_offset );
        if( layer == 2 && sel1 == 1 ) {
            // Then choose custom character, random character, preset, etc
            if( MAP_SHARING::isSharing() &&
                world_generator->all_worldnames().empty() ) { //don't show anything when there are no worlds (will not work if there are special maps)
                layer = 1;
                sel1 = 1;
                continue;
            }

            print_menu_items( w_open, vSubItems, sel2, menu_offset + point( 0, -2 ) );
            wrefresh( w_open );
            catacurses::refresh();

            std::string action = handle_input_timeout( ctxt );
            std::string sInput = ctxt.get_raw_input().text;
            for( size_t i = 0; i < vNewGameHotkeys.size(); ++i ) {
                for( const std::string &hotkey : vNewGameHotkeys[i] ) {
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
                if( sel2 >= static_cast<int>( vSubItems.size() ) ) {
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
                    if( world == nullptr ) {
                        continue;
                    }
                    world_generator->set_active_world( world );
                    try {
                        g->setup();
                    } catch( const std::exception &err ) {
                        debugmsg( "Error: %s", err.what() );
                        g->u = avatar();
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
                        g->u = avatar();
                        load_char_templates();
                        werase( w_background );
                        wrefresh( w_background );
                        MAPBUFFER.reset();
                        overmap_buffer.clear();
                        continue;
                    }

                    werase( w_background );
                    wrefresh( w_background );

                    if( !g->start_game() ) {
                        g->u = avatar();
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
                mvwprintz( w_open, menu_offset + point( 20 + extra_w / 2, -4 ),
                           c_red, "%s", _( "No templates found!" ) );
                on_error();
            } else {
                mvwprintz( w_open, menu_offset + point( 20 + extra_w / 2, -2 ),
                           c_white, "%s", _( "Press 'd' to delete a preset." ) );
                for( int i = 0; i < static_cast<int>( templates.size() ); i++ ) {
                    int line = menu_offset.y - 4 - i;
                    mvwprintz( w_open, point( 20 + menu_offset.x + extra_w / 2, line ),
                               ( sel3 == i ? h_white : c_white ), "%s",
                               templates[i] );
                }
            }
            wrefresh( w_open );
            catacurses::refresh();
            std::string action = handle_input_timeout( ctxt );
            if( errflag && action != "TIMEOUT" ) {
                clear_error();
                sel1 = 1;
                layer = 2;
                print_menu( w_open, sel1, menu_offset );
            } else if( action == "DOWN" ) {
                if( sel3 > 0 ) {
                    sel3--;
                } else {
                    sel3 = templates.size() - 1;
                }
            } else if( action == "UP" ) {
                if( sel3 < static_cast<int>( templates.size() ) - 1 ) {
                    sel3++;
                } else {
                    sel3 = 0;
                }
            } else if( action == "LEFT"  || action == "QUIT" ) {
                sel1 = 1;
                layer = 2;
                print_menu( w_open, sel1, menu_offset );
            } else if( !templates.empty() && action == "DELETE_TEMPLATE" ) {
                if( query_yn( _( "Are you sure you want to delete %s?" ),
                              templates[sel3].c_str() ) ) {
                    const auto path = FILENAMES["templatedir"] + utf8_to_native( templates[sel3] ) + ".template";
                    if( std::remove( path.c_str() ) != 0 ) {
                        popup( _( "Sorry, something went wrong." ) );
                    } else {
                        templates.erase( templates.begin() + sel3 );
                        if( static_cast<size_t>( sel3 ) > templates.size() - 1 ) {
                            sel3--;
                        }
                    }
                }
            } else if( action == "RIGHT" || action == "CONFIRM" ) {
                WORLDPTR world = world_generator->pick_world();
                if( world == nullptr ) {
                    g->u = avatar();
                    continue;
                }
                world_generator->set_active_world( world );
                try {
                    g->setup();
                } catch( const std::exception &err ) {
                    debugmsg( "Error: %s", err.what() );
                    g->u = avatar();
                    continue;
                }
                if( !g->u.create( PLTYPE_TEMPLATE, templates[sel3] ) ) {
                    g->u = avatar();
                    load_char_templates();
                    werase( w_background );
                    wrefresh( w_background );
                    MAPBUFFER.reset();
                    overmap_buffer.clear();
                    continue;
                }
                werase( w_background );
                wrefresh( w_background );
                if( !g->start_game() ) {
                    g->u = avatar();
                    continue;
                }
                start = true;
            }
        }
    } // end while

    if( start ) {
        g->u.add_msg_if_player( g->scen->description( g->u.male ) );

        world_generator->last_world_name = world_generator->active_world->world_name;
        world_generator->last_character_name = g->u.name;
        world_generator->save_last_world_info();
    }
    return start;
}

bool main_menu::load_character_tab()
{
    bool start = false;
    const auto all_worldnames = world_generator->all_worldnames();

    const size_t last_world_pos = std::find( all_worldnames.begin(), all_worldnames.end(),
                                  world_generator->last_world_name ) - all_worldnames.begin();
    if( last_world_pos < all_worldnames.size() ) {
        sel2 = last_world_pos;
        savegames = world_generator->get_world( all_worldnames[sel2] )->world_saves;
    }
    const size_t last_character_pos = std::find_if( savegames.begin(), savegames.end(),
    []( const save_t &it ) {
        return it.player_name() == world_generator->last_character_name;
    } ) - savegames.begin();
    if( last_character_pos < savegames.size() ) {
        sel3 = last_character_pos;
    } else {
        sel3 = 0;
    }

    while( !start && sel1 == 2 && ( layer == 2 || layer == 3 ) ) {
        print_menu( w_open, 2, menu_offset );
        if( layer == 2 && sel1 == 2 ) {
            if( all_worldnames.empty() ) {
                mvwprintz( w_open, menu_offset + point( 15 + extra_w / 2, -2 ),
                           c_red, "%s", _( "No Worlds found!" ) );
                on_error();
            } else {
                for( int i = 0; i < static_cast<int>( all_worldnames.size() ); ++i ) {
                    int line = menu_offset.y - 2 - i;
                    std::string world_name = all_worldnames[i];
                    int savegames_count = world_generator->get_world( world_name )->world_saves.size();
                    nc_color color1, color2;
                    if( world_name == "TUTORIAL" || world_name == "DEFENSE" ) {
                        color1 = c_light_cyan;
                        color2 = h_light_cyan;
                    } else {
                        color1 = c_white;
                        color2 = h_white;
                    }
                    mvwprintz( w_open, point( 15 + menu_offset.x + extra_w / 2, line ),
                               ( sel2 == i ? color2 : color1 ), "%s (%d)",
                               world_name, savegames_count );
                }
            }
            wrefresh( w_open );
            catacurses::refresh();
            std::string action = handle_input_timeout( ctxt );
            if( errflag && action != "TIMEOUT" ) {
                clear_error();
                layer = 1;
            } else if( action == "DOWN" ) {
                if( sel2 > 0 ) {
                    sel2--;
                } else {
                    sel2 = all_worldnames.size() - 1;
                }
            } else if( action == "UP" ) {
                if( sel2 < static_cast<int>( all_worldnames.size() ) - 1 ) {
                    sel2++;
                } else {
                    sel2 = 0;
                }
            } else if( action == "LEFT" || action == "QUIT" ) {
                layer = 1;
            } else if( action == "RIGHT" || action == "CONFIRM" ) {
                if( sel2 >= 0 && sel2 < static_cast<int>( all_worldnames.size() ) ) {
                    layer = 3;
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

            mvwprintz( w_open, menu_offset + point( 15 + extra_w / 2, -2 - sel2 ), h_white,
                       "%s", wn );

            if( savegames.empty() ) {
                mvwprintz( w_open, menu_offset + point( 40 + extra_w / 2, -2 - sel2 ),
                           c_red, "%s", _( "No save games found!" ) );
                on_error();
            } else {
                int line = menu_offset.y - 2;

                for( const auto &savename : savegames ) {
                    const bool selected = sel3 + line == menu_offset.y - 2;
                    mvwprintz( w_open, point( 40 + menu_offset.x + extra_w / 2, line-- ),
                               selected ? h_white : c_white,
                               "%s", savename.player_name() );
                }
            }
            wrefresh( w_open );
            catacurses::refresh();
            std::string action = handle_input_timeout( ctxt );
            if( errflag && action != "TIMEOUT" ) {
                clear_error();
                layer = 2;
            } else if( action == "DOWN" ) {
                if( sel3 > 0 ) {
                    sel3--;
                } else {
                    sel3 = savegames.size() - 1;
                }
            } else if( action == "UP" ) {
                if( sel3 < static_cast<int>( savegames.size() - 1 ) ) {
                    sel3++;
                } else {
                    sel3 = 0;
                }
            } else if( action == "LEFT" || action == "QUIT" ) {
                layer = 2;
                sel3 = 0;
                print_menu( w_open, sel1, menu_offset );
            }
            if( action == "RIGHT" || action == "CONFIRM" ) {
                if( sel3 >= 0 && sel3 < static_cast<int>( savegames.size() ) ) {
                    werase( w_background );
                    wrefresh( w_background );
                    WORLDPTR world = world_generator->get_world( all_worldnames[sel2] );
                    world_generator->last_world_name = world->world_name;
                    world_generator->last_character_name = savegames[sel3].player_name();
                    world_generator->save_last_world_info();
                    world_generator->set_active_world( world );
                    try {
                        g->setup();
                    } catch( const std::exception &err ) {
                        debugmsg( "Error: %s", err.what() );
                        g->u = avatar();
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
        print_menu( w_open, 3, menu_offset );
        if( layer == 3 ) { // World Menu
            // Show options for Destroy, Reset worlds.
            // Reset and Destroy ask for world to modify.
            // Reset empties world of everything but options, then makes new world within it.
            // Destroy asks for confirmation, then destroys everything in world and then removes world folder.

            const point offset = menu_offset + point( 40 + extra_w / 2, -2 - sel2 );

            const auto all_worldnames = world_generator->all_worldnames();
            mvwprintz( w_open, offset + point( -15, 0 ), h_white, "%s", all_worldnames[sel2 - 1] );

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
                wmove( w_open, offset + point( 0, -i ) );
                wprintz( w_open, c_light_gray, "[" );
                shortcut_print( w_open, text_color, key_color, vWorldSubItems[i] );
                wprintz( w_open, c_light_gray, "]" );
            }

            wrefresh( w_open );
            catacurses::refresh();
            std::string action = handle_input_timeout( ctxt );
            std::string sInput = ctxt.get_raw_input().text;
            for( size_t i = 0; i < vWorldSubItems.size(); ++i ) {
                for( const std::string &hotkey : vWorldHotkeys[i] ) {
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
                    } else if( sel3 == 3 ) { // Copy World settings
                        layer = 2;
                        world_generator->make_new_world( true, all_worldnames[sel2 - 1] );
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

            mvwprintz( w_open, menu_offset + point( 25 + extra_w / 2, -2 ),
                       ( sel2 == 0 ? h_white : c_white ), "%s", _( "Create World" ) );

            int i = 1;
            const auto all_worldnames = world_generator->all_worldnames();
            for( auto it = all_worldnames.begin(); it != all_worldnames.end(); ++it, i++ ) {
                int savegames_count = world_generator->get_world( *it )->world_saves.size();
                int line = menu_offset.y - 2 - i;
                nc_color color1, color2;
                if( *it == "TUTORIAL" || *it == "DEFENSE" ) {
                    color1 = c_light_cyan;
                    color2 = h_light_cyan;
                } else {
                    color1 = c_white;
                    color2 = h_white;
                }
                mvwprintz( w_open, point( 25 + menu_offset.x + extra_w / 2, line ),
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
                if( sel2 < static_cast<int>( all_worldnames.size() ) ) {
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

std::string main_menu::halloween_spider()
{
    static const std::string spider =
        "\\ \\ \\/ / / / / / / /\n"
        " \\ \\/\\/ / / / / / /\n"
        "\\ \\/__\\/ / / / / /\n"
        " \\/____\\/ / / / /\n"
        "\\/______\\/ / / /\n"
        "/________\\/ / /\n"
        "__________\\/ /\n"
        "___________\\/\n"
        "        |\n"
        "        |\n"
        "        |\n"
        "        |\n"
        "        |\n"
        "        |\n"
        "        |\n"
        "        |\n"
        "        |\n"
        "        |\n"
        "  , .   |  . ,\n"
        "  { | ,--, | }\n"
        "   \\\\{~~~~}//\n"
        "  /_/ {<color_c_red>..</color>} \\_\\\n"
        "  { {      } }\n"
        "  , ,      , .";

    return spider;
}

std::string main_menu::halloween_graves()
{
    static const std::string graves =
        "                    _\n"
        "        -q       __(\")_\n"
        "         (\\      \\_  _/\n"
        " .-.   .-''\"'.     |/\n"
        "|RIP|  | RIP |   .-.\n"
        "|   |  |     |  |RIP|\n"
        ";   ;  |     | ,'---',";

    return graves;
}
