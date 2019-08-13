#include "player.h" // IWYU pragma: associated

#include <algorithm> //std::min
#include <sstream>
#include <cstddef>

#include "bionics.h"
#include "catacharset.h"
#include "game.h"
#include "input.h"
#include "output.h"
#include "string_formatter.h"
#include "translations.h"
#include "options.h"
#include "string_id.h"

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
    REASSIGNING
};
} // namespace

bionic *player::bionic_by_invlet( const int ch )
{
    if( ch == ' ' ) {  // space is a special case for unassigned
        return nullptr;
    }

    for( auto &elem : *my_bionics ) {
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
        }
    }
    return ' ';
}

static void draw_bionics_titlebar( const catacurses::window &window, player *p,
                                   bionic_menu_mode mode )
{
    werase( window );

    const int pwr_str_pos = right_print( window, 0, 1, c_white,
                                         string_format( _( "Bionic Power: <color_light_blue>%i</color>/<color_light_blue>%i</color>" ),
                                                 p->power_level, p->max_power_level ) );
    std::string desc;
    if( mode == REASSIGNING ) {
        desc = _( "Reassigning.\nSelect a bionic to reassign or press SPACE to cancel." );
    } else if( mode == ACTIVATING ) {
        desc = _( "<color_green>Activating</color>  <color_yellow>!</color> to examine, <color_yellow>=</color> to reassign, <color_yellow>TAB</color> to switch tabs." );
    } else if( mode == EXAMINING ) {
        desc = _( "<color_light_blue>Examining</color>  <color_yellow>!</color> to activate, <color_yellow>=</color> to reassign, <color_yellow>TAB</color> to switch tabs." );
    }
    fold_and_print( window, 0, 1, pwr_str_pos, c_white, desc );

    wrefresh( window );
}

//builds the power usage string of a given bionic
static std::string build_bionic_poweronly_string( const bionic &bio )
{
    const bionic_data &bio_data = bio.id.obj();
    std::vector<std::string> properties;

    if( bio_data.power_activate > 0 ) {
        properties.push_back( string_format( _( "%d PU act" ), bio_data.power_activate ) );
    }
    if( bio_data.power_deactivate > 0 ) {
        properties.push_back( string_format( _( "%d PU deact" ), bio_data.power_deactivate ) );
    }
    if( bio_data.charge_time > 0 && bio_data.power_over_time > 0 ) {
        properties.push_back( bio_data.charge_time == 1
                              ? string_format( _( "%d PU/turn" ), bio_data.power_over_time )
                              : string_format( _( "%d PU/%d turns" ), bio_data.power_over_time,
                                               bio_data.charge_time ) );
    }
    if( bio_data.toggled ) {
        properties.push_back( bio.powered ? _( "ON" ) : _( "OFF" ) );
    }
    if( bio.incapacitated_time > 0_turns ) {
        properties.push_back( _( "(incapacitated)" ) );
    }

    return enumerate_as_string( properties, enumeration_conjunction::none );
}

//generates the string that show how much power a bionic uses
static std::string build_bionic_powerdesc_string( const bionic &bio )
{
    std::ostringstream power_desc;
    const std::string power_string = build_bionic_poweronly_string( bio );
    power_desc << bio.id->name;
    if( !power_string.empty() ) {
        power_desc << ", " << power_string;
    }
    return power_desc.str();
}

static void draw_bionics_tabs( const catacurses::window &win, const size_t active_num,
                               const size_t passive_num, const bionic_tab_mode current_mode )
{
    werase( win );

    const int width = getmaxx( win );
    mvwhline( win, point( 0, 2 ), LINE_OXOX, width );

    const std::string active_tab_name = string_format( _( "ACTIVE (%i)" ), active_num );
    const std::string passive_tab_name = string_format( _( "PASSIVE (%i)" ), passive_num );
    const int tab_step = 3;
    int tab_x = 1;
    draw_tab( win, tab_x, active_tab_name, current_mode == TAB_ACTIVE );
    tab_x += tab_step + utf8_width( active_tab_name );
    draw_tab( win, tab_x, passive_tab_name, current_mode == TAB_PASSIVE );

    wrefresh( win );
}

static void draw_description( const catacurses::window &win, const bionic &bio )
{
    werase( win );
    const int width = getmaxx( win );
    const std::string poweronly_string = build_bionic_poweronly_string( bio );
    int ypos = fold_and_print( win, 0, 0, width, c_white, bio.id->name );
    if( !poweronly_string.empty() ) {
        ypos += fold_and_print( win, ypos, 0, width, c_light_gray,
                                _( "Power usage: %s" ), poweronly_string );
    }
    ypos += 1 + fold_and_print( win, ypos, 0, width, c_light_blue, bio.id->description );

    // TODO: Unhide when enforcing limits
    if( get_option < bool >( "CBM_SLOTS_ENABLED" ) ) {
        const bool each_bp_on_new_line = ypos + static_cast<int>( num_bp ) + 1 < getmaxy( win );
        // NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
        ypos += fold_and_print( win, ypos, 0, width, c_light_gray,
                                list_occupied_bps( bio.id, _( "This bionic occupies the following body parts:" ),
                                        each_bp_on_new_line ) );
    }
    wrefresh( win );
}

static void draw_connectors( const catacurses::window &win, const int start_y, const int start_x,
                             const int last_x, const bionic_id &bio_id )
{
    const int LIST_START_Y = 6;
    // first: pos_y, second: occupied slots
    std::vector<std::pair<int, size_t>> pos_and_num;
    for( const auto &elem : bio_id->occupied_bodyparts ) {
        pos_and_num.emplace_back( static_cast<int>( elem.first ) + LIST_START_Y, elem.second );
    }
    if( pos_and_num.empty() || !get_option < bool >( "CBM_SLOTS_ENABLED" ) ) {
        return;
    }

    // draw horizontal line from selected bionic
    const int turn_x = start_x + ( last_x - start_x ) * 2 / 3;
    mvwputch( win, start_y, start_x, BORDER_COLOR, '>' );
    mvwhline( win, point( start_x + 1, start_y ), LINE_OXOX, turn_x - start_x - 1 );

    int min_y = start_y;
    int max_y = start_y;
    for( const auto &elem : pos_and_num ) {
        min_y = std::min( min_y, elem.first );
        max_y = std::max( max_y, elem.first );
    }
    if( max_y - min_y > 1 ) {
        mvwvline( win, point( turn_x, min_y + 1 ), LINE_XOXO, max_y - min_y - 1 );
    }

    bool move_up   = false;
    bool move_same = false;
    bool move_down = false;
    for( const auto &elem : pos_and_num ) {
        const int y = elem.first;
        if( !move_up && y < start_y ) {
            move_up = true;
        }
        if( !move_same && y == start_y ) {
            move_same = true;
        }
        if( !move_down && y > start_y ) {
            move_down = true;
        }

        // symbol is defined incorrectly for case ( y == start_y ) but
        // that's okay because it's overlapped by bionic_chr anyway
        int bp_chr = ( y > start_y ) ? LINE_XXOO : LINE_OXXO;
        if( ( max_y > y && y > start_y ) || ( min_y < y && y < start_y ) ) {
            bp_chr = LINE_XXXO;
        }

        mvwputch( win, y, turn_x, BORDER_COLOR, bp_chr );

        // draw horizontal line to bodypart title
        mvwhline( win, point( turn_x + 1, y ), LINE_OXOX, last_x - turn_x - 1 );
        mvwputch( win, y, last_x, BORDER_COLOR, '<' );

        // draw amount of consumed slots by this CBM
        const std::string fmt_num = string_format( "(%d)", elem.second );
        mvwprintz( win, y, turn_x + std::max( 1, ( last_x - turn_x - utf8_width( fmt_num ) ) / 2 ),
                   c_yellow, fmt_num );
    }

    // define and draw a proper intersection character
    int bionic_chr = LINE_OXOX; // '-'                // 001
    if( move_up && !move_down && !move_same ) {        // 100
        bionic_chr = LINE_XOOX;  // '_|'
    } else if( move_up && move_down && !move_same ) {  // 110
        bionic_chr = LINE_XOXX;  // '-|'
    } else if( move_up && move_down && move_same ) {   // 111
        bionic_chr = LINE_XXXX;  // '-|-'
    } else if( move_up && !move_down && move_same ) {  // 101
        bionic_chr = LINE_XXOX;  // '_|_'
    } else if( !move_up && move_down && !move_same ) { // 010
        bionic_chr = LINE_OOXX;  // '^|'
    } else if( !move_up && move_down && move_same ) {  // 011
        bionic_chr = LINE_OXXX;  // '^|^'
    }
    mvwputch( win, start_y, turn_x, BORDER_COLOR, bionic_chr );
}

//get a text color depending on the power/powering state of the bionic
static nc_color get_bionic_text_color( const bionic &bio, const bool isHighlightedBionic )
{
    nc_color type = c_white;
    if( bio.id->activated ) {
        if( isHighlightedBionic ) {
            if( bio.powered && !bio.id->power_source ) {
                type = h_red;
            } else if( bio.id->power_source && !bio.powered ) {
                type = h_light_cyan;
            } else if( bio.id->power_source && bio.powered ) {
                type = h_light_green;
            } else {
                type = h_light_red;
            }
        } else {
            if( bio.powered && !bio.id->power_source ) {
                type = c_red;
            } else if( bio.id->power_source && !bio.powered ) {
                type = c_light_cyan;
            } else if( bio.id->power_source && bio.powered ) {
                type = c_light_green;
            } else {
                type = c_light_red;
            }
        }
    } else {
        if( isHighlightedBionic ) {
            if( bio.id->power_source ) {
                type = h_light_cyan;
            } else {
                type = h_cyan;
            }
        } else {
            if( bio.id->power_source ) {
                type = c_light_cyan;
            } else {
                type = c_cyan;
            }
        }
    }
    return type;
}

static std::vector<bionic *> filtered_bionics( bionic_collection &all_bionics,
        bionic_tab_mode mode )
{
    std::vector< bionic *>filtered_entries;
    for( auto &elem : all_bionics ) {
        if( ( mode == TAB_ACTIVE ) == elem.id->activated ) {
            filtered_entries.push_back( &elem );
        }
    }
    return filtered_entries;
}

void player::power_bionics()
{
    std::vector <bionic *> passive = filtered_bionics( *my_bionics, TAB_PASSIVE );
    std::vector <bionic *> active = filtered_bionics( *my_bionics, TAB_ACTIVE );
    bionic *bio_last = nullptr;
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
                                           static_cast<int>( my_bionics->size() ) + 2 ) );
    const int WIDTH = FULL_SCREEN_WIDTH + ( TERMX - FULL_SCREEN_WIDTH ) / 2;
    const int START_X = ( TERMX - WIDTH ) / 2;
    const int START_Y = ( TERMY - HEIGHT ) / 2;
    //wBio is the entire bionic window
    catacurses::window wBio = catacurses::newwin( HEIGHT, WIDTH, point( START_X, START_Y ) );

    const int LIST_HEIGHT = HEIGHT - TITLE_HEIGHT - TITLE_TAB_HEIGHT - 2;

    const int DESCRIPTION_WIDTH = WIDTH - 2 - 40;
    const int DESCRIPTION_START_Y = START_Y + TITLE_HEIGHT + TITLE_TAB_HEIGHT + 1;
    const int DESCRIPTION_START_X = START_X + 1 + 40;
    //w_description is the description panel that is controlled with ! key
    catacurses::window w_description = catacurses::newwin( LIST_HEIGHT, DESCRIPTION_WIDTH,
                                       point( DESCRIPTION_START_X, DESCRIPTION_START_Y ) );

    // Title window
    const int TITLE_START_Y = START_Y + 1;
    const int HEADER_LINE_Y = TITLE_HEIGHT + TITLE_TAB_HEIGHT + 1;
    catacurses::window w_title = catacurses::newwin( TITLE_HEIGHT, WIDTH - 2, point( START_X + 1,
                                 TITLE_START_Y ) );

    const int TAB_START_Y = TITLE_START_Y + 2;
    //w_tabs is the tab bar for passive and active bionic groups
    catacurses::window w_tabs = catacurses::newwin( TITLE_TAB_HEIGHT, WIDTH - 2, point( START_X + 1,
                                TAB_START_Y ) );

    int scroll_position = 0;
    int cursor = 0;

    //generate the tab title string and a count of the bionics owned
    bionic_menu_mode menu_mode = ACTIVATING;
    // offset for display: bionic with index i is drawn at y=list_start_y+i
    // drawing the bionics starts with bionic[scroll_position]
    // scroll_position;
    const int list_start_y = HEADER_LINE_Y;
    int half_list_view_location = LIST_HEIGHT / 2;
    int max_scroll_position = std::max( 0, static_cast<int>( active.size() ) );

    input_context ctxt( "BIONICS" );
    ctxt.register_updown();
    ctxt.register_action( "ANY_INPUT" );
    ctxt.register_action( "TOGGLE_EXAMINE" );
    ctxt.register_action( "REASSIGN" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    bool recalc = false;
    bool redraw = true;

    for( ;; ) {
        if( recalc ) {
            passive = filtered_bionics( *my_bionics, TAB_PASSIVE );
            active = filtered_bionics( *my_bionics, TAB_ACTIVE );

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
        max_scroll_position = std::max( 0, static_cast<int>( current_bionic_list->size() ) - LIST_HEIGHT );

        if( redraw ) {
            redraw = false;

            werase( wBio );
            draw_border( wBio, BORDER_COLOR, _( " BIONICS " ) );
            // Draw symbols to connect additional lines to border
            mvwputch( wBio, HEADER_LINE_Y - 1, 0, BORDER_COLOR, LINE_XXXO ); // |-
            mvwputch( wBio, HEADER_LINE_Y - 1, WIDTH - 1, BORDER_COLOR, LINE_XOXX ); // -|

            int max_width = 0;
            std::vector<std::string>bps;
            for( const body_part bp : all_body_parts ) {
                const int total = get_total_bionics_slots( bp );
                const std::string s = string_format( "%s: %d/%d",
                                                     body_part_name_as_heading( bp, 1 ),
                                                     total - get_free_bionics_slots( bp ), total );
                bps.push_back( s );
                max_width = std::max( max_width, utf8_width( s ) );
            }
            const int pos_x = WIDTH - 2 - max_width;
            if( get_option < bool >( "CBM_SLOTS_ENABLED" ) ) {
                for( size_t i = 0; i < bps.size(); ++i ) {
                    mvwprintz( wBio, i + list_start_y, pos_x, c_light_gray, bps[i] );
                }
            }

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
                fold_and_print( wBio, list_start_y, 2, pos_x - 1, c_light_gray, msg );
            } else {
                for( size_t i = scroll_position; i < current_bionic_list->size(); i++ ) {
                    if( list_start_y + static_cast<int>( i ) - scroll_position == HEIGHT - 1 ) {
                        break;
                    }
                    const bool is_highlighted = cursor == static_cast<int>( i );
                    const nc_color col = get_bionic_text_color( *( *current_bionic_list )[i],
                                         is_highlighted );
                    const std::string desc = string_format( "%c %s", ( *current_bionic_list )[i]->invlet,
                                                            build_bionic_powerdesc_string(
                                                                    *( *current_bionic_list )[i] ).c_str() );
                    trim_and_print( wBio, list_start_y + i - scroll_position, 2, WIDTH - 3, col,
                                    desc );
                    if( is_highlighted && menu_mode != EXAMINING && get_option < bool >( "CBM_SLOTS_ENABLED" ) ) {
                        const bionic_id bio_id = ( *current_bionic_list )[i]->id;
                        draw_connectors( wBio, list_start_y + i - scroll_position, utf8_width( desc ) + 3,
                                         pos_x - 2, bio_id );

                        // redraw highlighted (occupied) body parts
                        for( auto &elem : bio_id->occupied_bodyparts ) {
                            const int i = static_cast<int>( elem.first );
                            mvwprintz( wBio, i + list_start_y, pos_x, c_yellow, bps[i] );
                        }
                    }

                }
            }

            draw_scrollbar( wBio, cursor, LIST_HEIGHT, current_bionic_list->size(), list_start_y );

#if defined(__ANDROID__)
            ctxt.get_registered_manual_keys().clear();
            for( size_t i = 0; i < current_bionic_list->size(); i++ ) {
                ctxt.register_manual_key( ( *current_bionic_list )[i]->invlet,
                                          build_bionic_powerdesc_string( *( *current_bionic_list )[i] ).c_str() );
            }
#endif

        }
        wrefresh( wBio );
        draw_bionics_tabs( w_tabs, active.size(), passive.size(), tab_mode );
        draw_bionics_titlebar( w_title, this, menu_mode );
        if( menu_mode == EXAMINING && !current_bionic_list->empty() ) {
            draw_description( w_description, *( *current_bionic_list )[cursor] );
        }

        const std::string action = ctxt.handle_input();
        const int ch = ctxt.get_raw_input().get_first_input();
        bionic *tmp = nullptr;
        bool confirmCheck = false;

        if( action == "DOWN" ) {
            redraw = true;
            if( static_cast<size_t>( cursor ) < current_bionic_list->size() - 1 ) {
                cursor++;
            } else {
                cursor = 0;
            }
            if( scroll_position < max_scroll_position &&
                cursor - scroll_position > LIST_HEIGHT - half_list_view_location ) {
                scroll_position++;
            }
            if( scroll_position > 0 && cursor - scroll_position < half_list_view_location ) {
                scroll_position = std::max( cursor - half_list_view_location, 0 );
            }
        } else if( action == "UP" ) {
            redraw = true;
            if( cursor > 0 ) {
                cursor--;
            } else {
                cursor = current_bionic_list->size() - 1;
            }
            if( scroll_position > 0 && cursor - scroll_position < half_list_view_location ) {
                scroll_position--;
            }
            if( scroll_position < max_scroll_position &&
                cursor - scroll_position > LIST_HEIGHT - half_list_view_location ) {
                scroll_position =
                    std::max( std::min<int>( current_bionic_list->size() - LIST_HEIGHT,
                                             cursor - half_list_view_location ), 0 );
            }
        } else if( menu_mode == REASSIGNING ) {
            menu_mode = ACTIVATING;

            if( action == "CONFIRM" && !current_bionic_list->empty() ) {
                auto &bio_list = tab_mode == TAB_ACTIVE ? active : passive;
                tmp = bio_list[cursor];
            } else {
                tmp = bionic_by_invlet( ch );
            }

            if( tmp == nullptr ) {
                // Selected an non-existing bionic (or Escape, or ...)
                continue;
            }
            redraw = true;
            const int newch = popup_getkey( _( "%s; enter new letter. Space to clear. Esc to cancel." ),
                                            tmp->id->name );
            wrefresh( wBio );
            if( newch == ch || newch == KEY_ESCAPE ) {
                continue;
            }
            if( newch == ' ' ) {
                tmp->invlet = ' ';
                continue;
            }
            if( !bionic_chars.valid( newch ) ) {
                popup( _( "Invalid bionic letter. Only those characters are valid:\n\n%s" ),
                       bionic_chars.get_allowed_chars() );
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
        } else if( action == "REASSIGN" ) {
            menu_mode = REASSIGNING;
        } else if( action == "TOGGLE_EXAMINE" ) { // switches between activation and examination
            menu_mode = menu_mode == ACTIVATING ? EXAMINING : ACTIVATING;
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
                    for( temp_cursor = 0; temp_cursor < static_cast<int>( bio_list.size() ); temp_cursor++ ) {
                        if( bio_list[temp_cursor] == tmp ) {
                            break;
                        }
                    }
                    // if bionic is not found in current list, ignore the attempt to view/activate
                    if( temp_cursor >= static_cast<int>( bio_list.size() ) ) {
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
            const bionic_id &bio_id = tmp->id;
            const bionic_data &bio_data = bio_id.obj();
            if( menu_mode == ACTIVATING ) {
                if( bio_data.activated ) {
                    int b = tmp - &( *my_bionics )[0];
                    if( tmp->powered ) {
                        deactivate_bionic( b );
                    } else {
                        activate_bionic( b );
                        // Clear the menu if we are firing a bionic gun
                        if( tmp->info().gun_bionic || tmp->ammo_count > 0 ) {
                            break;
                        }
                    }
                    // update message log and the menu
                    g->refresh_all();
                    redraw = true;
                    if( moves < 0 ) {
                        return;
                    }
                    continue;
                } else {
                    popup( _( "You can not activate %s!\n"
                              "To read a description of %s, press '!', then '%c'." ), bio_data.name,
                           bio_data.name, tmp->invlet );
                    redraw = true;
                }
            } else if( menu_mode == EXAMINING ) { // Describing bionics, allow user to jump to description key
                redraw = true;
                if( action != "CONFIRM" ) {
                    for( size_t i = 0; i < active.size(); i++ ) {
                        if( active[i] == tmp ) {
                            tab_mode = TAB_ACTIVE;
                            cursor = static_cast<int>( i );
                            int max_scroll_check = std::max( 0, static_cast<int>( active.size() ) - LIST_HEIGHT );
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
                            int max_scroll_check = std::max( 0, static_cast<int>( passive.size() ) - LIST_HEIGHT );
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
