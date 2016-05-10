#include "player.h"
#include "action.h"
#include "game.h"
#include "input.h"
#include "bionics.h"
#include "bodypart.h"
#include "translations.h"
#include "catacharset.h"
#include "output.h"

#include <algorithm> //std::min
#include <sstream>

// '!', '-' and '=' are uses as default bindings in the menu
const invlet_wrapper
bionic_chars( "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\"#&()*+./:;@[\\]^_{|}" );

namespace
{
enum bionic_tab_mode {
    TAB_ACTIVE,
    TAB_PASSIVE
};
enum bionic_menu_mode {
    ACTIVATING,
    EXAMINING,
    REASSIGNING,
    REMOVING
};
} // namespace

bionic *player::bionic_by_invlet( const long ch )
{
    for( auto &elem : my_bionics ) {
        if( elem.invlet == ch ) {
            return &elem;
        }
    }
    return nullptr;
}

char get_free_invlet( player &p )
{
    for( auto &inv_char : bionic_chars ) {
        if( p.bionic_by_invlet( inv_char ) == nullptr ) {
            return inv_char;
            break;
        }
    }
    return ' ';
}

void draw_bionics_titlebar( WINDOW *window, player *p, bionic_menu_mode mode )
{
    werase( window );

    const std::string pwr = string_format( _( "Power: %i/%i" ), int( p->power_level ),
                                           int( p->max_power_level ) );
    const int pwr_str_pos = getmaxx( window ) - utf8_width( pwr ) - 1;
    mvwprintz( window, 0, pwr_str_pos, c_white, "%s", pwr.c_str() );

    std::string desc;
    if( mode == REASSIGNING ) {
        desc = _( "Reassigning.\nSelect a bionic to reassign or press SPACE to cancel." );
    } else if( mode == ACTIVATING ) {
        desc = _( "<color_green>Activating</color>  <color_yellow>!</color> to examine, <color_yellow>-</color> to remove, <color_yellow>=</color> to reassign, <color_yellow>TAB</color> to switch tabs." );
    } else if( mode == REMOVING ) {
        desc = _( "<color_red>Removing</color>  <color_yellow>!</color> to activate, <color_yellow>-</color> to remove, <color_yellow>=</color> to reassign, <color_yellow>TAB</color> to switch tabs." );
    } else if( mode == EXAMINING ) {
        desc = _( "<color_ltblue>Examining</color>  <color_yellow>!</color> to activate, <color_yellow>-</color> to remove, <color_yellow>=</color> to reassign, <color_yellow>TAB</color> to switch tabs." );
    }
    fold_and_print( window, 0, 1, pwr_str_pos, c_white, desc );

    wrefresh( window );
}

const auto separator = []( std::ostringstream &s )
{
    return s.tellp() != 0 ? ", " : "";
};

//builds the power usage string of a given bionic
std::string build_bionic_poweronly_string( bionic const &bio )
{
    std::ostringstream power_desc;
    if( bionic_info( bio.id ).power_over_time > 0 && bionic_info( bio.id ).charge_time > 0 ) {
        power_desc << (
                       bionic_info( bio.id ).charge_time == 1
                       ? string_format( _( "%d PU / turn" ),
                                        bionic_info( bio.id ).power_over_time )
                       : string_format( _( "%d PU / %d turns" ),
                                        bionic_info( bio.id ).power_over_time,
                                        bionic_info( bio.id ).charge_time ) );
    }
    if( bionic_info( bio.id ).power_activate > 0 && !bionic_info( bio.id ).charge_time ) {
        power_desc << separator( power_desc ) << string_format( _( "%d PU act" ),
                   bionic_info( bio.id ).power_activate );
    }
    if( bionic_info( bio.id ).power_deactivate > 0 && !bionic_info( bio.id ).charge_time ) {
        power_desc << separator( power_desc ) << string_format( _( "%d PU deact" ),
                   bionic_info( bio.id ).power_deactivate );
    }
    if( bionic_info( bio.id ).toggled ) {
        power_desc << separator( power_desc ) << ( bio.powered ? _( "ON" ) : _( "OFF" ) );
    }

    return power_desc.str();
}

//generates the string that show how much power a bionic uses
std::string build_bionic_powerdesc_string( bionic const &bio )
{
    std::ostringstream power_desc;
    const std::string power_string = build_bionic_poweronly_string( bio );
    power_desc << bionic_info( bio.id ).name;
    if( !power_string.empty() ) {
        power_desc << ", " << power_string;
    }
    return power_desc.str();
}

void draw_bionics_tabs( WINDOW *win, const size_t active_num, const size_t passive_num,
                        const bionic_tab_mode current_mode )
{
    werase( win );

    const int width = getmaxx( win );
    mvwhline( win, 2, 0, LINE_OXOX, width );

    const std::string active_tab_name = string_format( _( "ACTIVE (%i)" ), active_num );
    const std::string passive_tab_name = string_format( _( "PASSIVE (%i)" ), passive_num );
    const int tab_step = 3;
    int tab_x = 1;
    draw_tab( win, tab_x, active_tab_name, current_mode == TAB_ACTIVE );
    tab_x += tab_step + utf8_width( active_tab_name );
    draw_tab( win, tab_x, passive_tab_name, current_mode == TAB_PASSIVE );

    wrefresh( win );
}

void draw_description( WINDOW *win, bionic const &bio )
{
    werase( win );
    const int width = getmaxx( win );
    const std::string poweronly_string = build_bionic_poweronly_string( bio );
    int ypos = fold_and_print( win, 0, 0, width, c_white, bionic_info( bio.id ).name );
    if( !poweronly_string.empty() ) {
        ypos += fold_and_print( win, ypos, 0, width, c_ltgray,
                                _( "Power usage: %s" ), poweronly_string.c_str() );
    }
    ypos += 1 + fold_and_print( win, ypos, 0, width, c_ltblue, bionic_info( bio.id ).description );

    const bool each_bp_on_new_line = ypos + ( int )num_bp + 1 < getmaxy( win );
    ypos += fold_and_print( win, ypos, 0, width, c_ltgray,
                            list_occupied_bps( bio.id, _( "This bionic occupies the following body parts:" ),
                                    each_bp_on_new_line ) );
    wrefresh( win );
}

//get a text color depending on the power/powering state of the bionic
nc_color get_bionic_text_color( bionic const &bio, bool const isHighlightedBionic )
{
    nc_color type = c_white;
    if( bionic_info( bio.id ).activated ) {
        if( isHighlightedBionic ) {
            if( bio.powered && !bionic_info( bio.id ).power_source ) {
                type = h_red;
            } else if( bionic_info( bio.id ).power_source && !bio.powered ) {
                type = h_ltcyan;
            } else if( bionic_info( bio.id ).power_source && bio.powered ) {
                type = h_ltgreen;
            } else {
                type = h_ltred;
            }
        } else {
            if( bio.powered && !bionic_info( bio.id ).power_source ) {
                type = c_red;
            } else if( bionic_info( bio.id ).power_source && !bio.powered ) {
                type = c_ltcyan;
            } else if( bionic_info( bio.id ).power_source && bio.powered ) {
                type = c_ltgreen;
            } else {
                type = c_ltred;
            }
        }
    } else {
        if( isHighlightedBionic ) {
            if( bionic_info( bio.id ).power_source ) {
                type = h_ltcyan;
            } else {
                type = h_cyan;
            }
        } else {
            if( bionic_info( bio.id ).power_source ) {
                type = c_ltcyan;
            } else {
                type = c_cyan;
            }
        }
    }
    return type;
}

std::vector< bionic *>filtered_bionics( std::vector<bionic> &all_bionics, bionic_tab_mode mode )
{
    std::vector< bionic *>filtered_entries;
    for( auto &elem : all_bionics ) {
        if( ( mode == TAB_ACTIVE ) == bionic_info( elem.id ).activated ) {
            filtered_entries.push_back( &elem );
        }
    }
    return filtered_entries;
}

void player::power_bionics()
{
    std::vector <bionic *> passive = filtered_bionics( my_bionics, TAB_PASSIVE );
    std::vector <bionic *> active = filtered_bionics( my_bionics, TAB_ACTIVE );
    bionic *bio_last = NULL;
    bionic_tab_mode tab_mode = TAB_ACTIVE;

    //added title_tab_height for the tabbed bionic display
    int TITLE_HEIGHT = 2;
    int TITLE_TAB_HEIGHT = 3;

    // Main window
    /** Total required height is:
     * top frame line:                                         + 1
     * height of title window:                                 + TITLE_HEIGHT
     * height of tabs:                                         + TITLE_TAB_HEIGHT
     * height of the biggest list of active/passive bionics:   + bionic_count
     * bottom frame line:                                      + 1
     * TOTAL: TITLE_HEIGHT + TITLE_TAB_HEIGHT + bionic_count + 2
     */
    const int HEIGHT = std::min( TERMY,
                                 std::max( FULL_SCREEN_HEIGHT,
                                           TITLE_HEIGHT + TITLE_TAB_HEIGHT +
                                           ( int )my_bionics.size() + 2 ) );
    const int WIDTH = FULL_SCREEN_WIDTH + ( TERMX - FULL_SCREEN_WIDTH ) / 2;
    const int START_X = ( TERMX - WIDTH ) / 2;
    const int START_Y = ( TERMY - HEIGHT ) / 2;
    //wBio is the entire bionic window
    WINDOW *wBio = newwin( HEIGHT, WIDTH, START_Y, START_X );
    WINDOW_PTR wBioptr( wBio );

    const int LIST_HEIGHT = HEIGHT - TITLE_HEIGHT - TITLE_TAB_HEIGHT - 2;

    const int DESCRIPTION_WIDTH = WIDTH - 2 - 40;
    const int DESCRIPTION_START_Y = START_Y + TITLE_HEIGHT + TITLE_TAB_HEIGHT + 1;
    const int DESCRIPTION_START_X = START_X + 1 + 40;
    //w_description is the description panel that is controlled with ! key
    WINDOW *w_description = newwin( LIST_HEIGHT, DESCRIPTION_WIDTH,
                                    DESCRIPTION_START_Y, DESCRIPTION_START_X );
    WINDOW_PTR w_descriptionptr( w_description );

    // Title window
    const int TITLE_START_Y = START_Y + 1;
    const int HEADER_LINE_Y = TITLE_HEIGHT + TITLE_TAB_HEIGHT + 1;
    WINDOW *w_title = newwin( TITLE_HEIGHT, WIDTH - 2, TITLE_START_Y, START_X + 1 );
    WINDOW_PTR w_titleptr( w_title );

    const int TAB_START_Y = TITLE_START_Y + 2;
    //w_tabs is the tab bar for passive and active bionic groups
    WINDOW *w_tabs = newwin( TITLE_TAB_HEIGHT, WIDTH - 2, TAB_START_Y, START_X + 1 );
    WINDOW_PTR w_tabsptr( w_tabs );

    int scroll_position = 0;
    int cursor = 0;

    //generate the tab title string and a count of the bionics owned
    bionic_menu_mode menu_mode = ACTIVATING;
    // offset for display: bionic with index i is drawn at y=list_start_y+i
    // drawing the bionics starts with bionic[scroll_position]
    const int list_start_y = HEADER_LINE_Y;// - scroll_position;
    int half_list_view_location = LIST_HEIGHT / 2;
    int max_scroll_position = std::max( 0, ( tab_mode == TAB_ACTIVE ?
                                        ( int )active.size() :
                                        ( int )passive.size() ) - LIST_HEIGHT );

    input_context ctxt( "BIONICS" );
    ctxt.register_updown();
    ctxt.register_action( "ANY_INPUT" );
    ctxt.register_action( "TOGGLE_EXAMINE" );
    ctxt.register_action( "REASSIGN" );
    ctxt.register_action( "REMOVE" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    bool recalc = false;
    bool redraw = true;

    for( ;; ) {
        if( recalc ) {
            passive = filtered_bionics( my_bionics, TAB_PASSIVE );
            active = filtered_bionics( my_bionics, TAB_ACTIVE );

            if( active.empty() && !passive.empty() ) {
                tab_mode = TAB_PASSIVE;
            }

            if( --cursor < 0 ) {
                cursor = 0;
            }
            if( scroll_position > max_scroll_position &&
                cursor - scroll_position < LIST_HEIGHT - half_list_view_location ) {
                scroll_position--;
            }

            recalc = false;
            // bionics were modified, so it's necessary to redraw the screen
            redraw = true;
        }

        //track which list we are looking at
        std::vector<bionic *> *current_bionic_list = ( tab_mode == TAB_ACTIVE ? &active : &passive );
        max_scroll_position = std::max( 0, ( int )current_bionic_list->size() - LIST_HEIGHT );

        if( redraw ) {
            redraw = false;

            werase( wBio );
            draw_border( wBio, BORDER_COLOR, _( " BIONICS " ) );
            // Draw symbols to connect additional lines to border
            mvwputch( wBio, HEADER_LINE_Y - 1, 0, BORDER_COLOR, LINE_XXXO ); // |-
            mvwputch( wBio, HEADER_LINE_Y - 1, WIDTH - 1, BORDER_COLOR, LINE_XOXX ); // -|

            if( current_bionic_list->empty() ) {
                std::string msg;
                switch( tab_mode ) {
                    case TAB_ACTIVE:
                        msg = _( "No activatable bionics installed." );
                        break;
                    case TAB_PASSIVE:
                        msg = _( "No passive bionics installed." );
                        break;
                }
                fold_and_print( wBio, list_start_y, 2, WIDTH - 3, c_ltgray, msg );
            } else {
                for( size_t i = scroll_position; i < current_bionic_list->size(); i++ ) {
                    if( list_start_y + static_cast<int>( i ) - scroll_position == HEIGHT - 1 ) {
                        break;
                    }
                    const bool is_highlighted = cursor == static_cast<int>( i );
                    const nc_color col = get_bionic_text_color( *( *current_bionic_list )[i],
                                         is_highlighted );
                    const std::string desc = build_bionic_powerdesc_string( *( *current_bionic_list )[i] );
                    trim_and_print( wBio, list_start_y + i - scroll_position, 2, WIDTH - 3, col,
                                    "%c %s", ( *current_bionic_list )[i]->invlet, desc.c_str() );
                }
            }

            draw_scrollbar( wBio, cursor, LIST_HEIGHT, current_bionic_list->size(), list_start_y );
        }
        wrefresh( wBio );
        draw_bionics_tabs( w_tabs, active.size(), passive.size(), tab_mode );
        draw_bionics_titlebar( w_title, this, menu_mode );
        if( menu_mode == EXAMINING && !current_bionic_list->empty() ) {
            draw_description( w_description, *( *current_bionic_list )[cursor] );
        }

        const std::string action = ctxt.handle_input();
        const long ch = ctxt.get_raw_input().get_first_input();
        bionic *tmp = NULL;
        bool confirmCheck = false;
        if( menu_mode == REASSIGNING ) {
            menu_mode = ACTIVATING;
            tmp = bionic_by_invlet( ch );
            if( tmp == nullptr ) {
                // Selected an non-existing bionic (or escape, or ...)
                continue;
            }
            redraw = true;
            const long newch = popup_getkey( _( "%s; enter new letter." ),
                                             bionic_info( tmp->id ).name.c_str() );
            wrefresh( wBio );
            if( newch == ch || newch == ' ' || newch == KEY_ESCAPE ) {
                continue;
            }
            if( !bionic_chars.valid( newch ) ) {
                popup( _( "Invalid bionic letter. Only those characters are valid:\n\n%s" ),
                       bionic_chars.get_allowed_chars().c_str() );
                continue;
            }
            bionic *otmp = bionic_by_invlet( newch );
            if( otmp != nullptr ) {
                std::swap( tmp->invlet, otmp->invlet );
            } else {
                tmp->invlet = newch;
            }
            // TODO: show a message like when reassigning a key to an item?
        } else if( action == "NEXT_TAB" ) {
            redraw = true;
            scroll_position = 0;
            cursor = 0;
            if( tab_mode == TAB_ACTIVE ) {
                tab_mode = TAB_PASSIVE;
            } else {
                tab_mode = TAB_ACTIVE;
            }
        } else if( action == "PREV_TAB" ) {
            redraw = true;
            scroll_position = 0;
            cursor = 0;
            if( tab_mode == TAB_PASSIVE ) {
                tab_mode = TAB_ACTIVE;
            } else {
                tab_mode = TAB_PASSIVE;
            }
        } else if( action == "DOWN" ) {
            redraw = true;
            if( static_cast<size_t>( cursor ) < current_bionic_list->size() - 1 ) {
                cursor++;
            }
            if( scroll_position < max_scroll_position &&
                cursor - scroll_position > LIST_HEIGHT - half_list_view_location ) {
                scroll_position++;
            }
        } else if( action == "UP" ) {
            redraw = true;
            if( cursor > 0 ) {
                cursor--;
            }
            if( scroll_position > 0 && cursor - scroll_position < half_list_view_location ) {
                scroll_position--;
            }
        } else if( action == "REASSIGN" ) {
            menu_mode = REASSIGNING;
        } else if( action == "TOGGLE_EXAMINE" ) { // switches between activation and examination
            menu_mode = menu_mode == ACTIVATING ? EXAMINING : ACTIVATING;
            redraw = true;
        } else if( action == "REMOVE" ) {
            menu_mode = REMOVING;
            redraw = true;
        } else if( action == "HELP_KEYBINDINGS" ) {
            redraw = true;
        } else if( action == "CONFIRM" ) {
            confirmCheck = true;
        } else {
            confirmCheck = true;
        }
        //confirmation either occurred by pressing enter where the bionic cursor is, or the hotkey was selected
        if( confirmCheck ) {
            auto &bio_list = tab_mode == TAB_ACTIVE ? active : passive;
            if( action == "CONFIRM" && !current_bionic_list->empty() ) {
                tmp = bio_list[cursor];
            } else {
                tmp = bionic_by_invlet( ch );
                if( tmp && tmp != bio_last ) {
                    // new bionic selected, update cursor and scroll position
                    int temp_cursor = 0;
                    for( temp_cursor = 0; temp_cursor < ( int )bio_list.size(); temp_cursor++ ) {
                        if( bio_list[temp_cursor] == tmp ) {
                            break;
                        }
                    }
                    // if bionic is not found in current list, ignore the attempt to view/activate
                    if( temp_cursor >= ( int )bio_list.size() ) {
                        continue;
                    }
                    //relocate cursor to the bionic that was found
                    cursor = temp_cursor;
                    scroll_position = 0;
                    while( scroll_position < max_scroll_position &&
                           cursor - scroll_position > LIST_HEIGHT - half_list_view_location ) {
                        scroll_position++;
                    }
                }
            }
            if( !tmp ) {
                // entered a key that is not mapped to any bionic,
                // -> leave screen
                break;
            }
            bio_last = tmp;
            const std::string &bio_id = tmp->id;
            const bionic_data &bio_data = bionic_info( bio_id );
            if( menu_mode == REMOVING ) {
                recalc = uninstall_bionic( bio_id );
                redraw = true;
                continue;
            }
            if( menu_mode == ACTIVATING ) {
                if( bio_data.activated ) {
                    int b = tmp - &my_bionics[0];
                    if( tmp->powered ) {
                        deactivate_bionic( b );
                    } else {
                        activate_bionic( b );
                    }
                    // update message log and the menu
                    g->refresh_all();
                    redraw = true;
                    continue;
                } else {
                    popup( _( "You can not activate %s!\n"
                              "To read a description of %s, press '!', then '%c'." ), bio_data.name.c_str(),
                           bio_data.name.c_str(), tmp->invlet );
                    redraw = true;
                }
            } else if( menu_mode == EXAMINING ) { // Describing bionics, allow user to jump to description key
                redraw = true;
                if( action != "CONFIRM" ) {
                    for( size_t i = 0; i < active.size(); i++ ) {
                        if( active[i] == tmp ) {
                            tab_mode = TAB_ACTIVE;
                            cursor = static_cast<int>( i );
                            int max_scroll_check = std::max( 0, ( int )active.size() - LIST_HEIGHT );
                            if( static_cast<int>( i ) > max_scroll_check ) {
                                scroll_position = max_scroll_check;
                            } else {
                                scroll_position = i;
                            }
                            break;
                        }
                    }
                    for( size_t i = 0; i < passive.size(); i++ ) {
                        if( passive[i] == tmp ) {
                            tab_mode = TAB_PASSIVE;
                            cursor = static_cast<int>( i );
                            int max_scroll_check = std::max( 0, ( int )passive.size() - LIST_HEIGHT );
                            if( static_cast<int>( i ) > max_scroll_check ) {
                                scroll_position = max_scroll_check;
                            } else {
                                scroll_position = i;
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
}
