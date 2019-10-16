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
using bvector = std::vector <bionic *>;

enum bionic_columns : int {
    column_status,
    column_act_cost,
    column_turn_cost,
    column_num_entries
};
struct bionic_col_data {
    private:
        int width;
        std::string name;
        std::string description;
    public:
        bionic_col_data( int width, std::string name, std::string description ) :
            width( width ), name( name ), description( description ) {
        }
        int get_width() {
            return std::max( utf8_width( _( name ) ) + 1, width );
        }
        std::string get_name() {
            return _( name );
        }
        std::string get_description() {
            return _( description );
        }
};
bionic_col_data get_col_data( int col_idx )
{
    static std::array< bionic_col_data, column_num_entries> col_data = { {
            {
                3, translate_marker( "Status" ),
                translate_marker( "CBM Status:\n\t<color_c_green>+</color> - activated\n\t<color_c_red>x</color> - incapacitated.  This CBM cannot be used for some time." )
            },
            {8, translate_marker( "Activation" ), translate_marker( "Amount of Bionic Power required to activeate this CBM." )},
            {8, translate_marker( "Turn Cost" ), translate_marker( "Amount of Bionic Power this CBM consumes every turn while activated.  Values like 1 /600 mean that 1 Power will be consumed every 600 turns." ) }
        }
    };
    return col_data[col_idx];
}

void print_columnns( bionic *bio, bool is_selected, bool name_only, const catacurses::window &w,
                     int right_bound,
                     int cur_print_y )
{
    const bionic_data &bio_data = bio->id.obj();
    nc_color print_col;

    if( is_selected ) {
        print_col = h_white;
    } else if( bio->powered ) {
        print_col = c_light_cyan;
    } else {
        if( bio_data.gun_bionic ) {
            print_col = c_light_green;
        } else {
            print_col = c_dark_gray;
        }
    }

    if( !name_only ) {
        for( size_t col_idx = column_num_entries; col_idx-- > 0; ) {
            std::string print_str;
            switch( col_idx ) {
                case    column_status: {
                    int col_w = get_col_data( col_idx ).get_width();
                    if( is_selected ) { //fill whole line with color
                        mvwprintz( w, point( right_bound - col_w, cur_print_y ), print_col, "%*s", col_w, "" );
                    }
                    int pos_x = right_bound - 1;
                    if( bio_data.toggled && bio->powered ) {
                        nc_color print_col2 = is_selected ? h_green : c_green;
                        mvwprintz( w, point( pos_x, cur_print_y ), print_col2, "+" );
                    }
                    --pos_x;
                    if( bio->incapacitated_time > 0_turns ) {
                        nc_color print_col2 = is_selected ? h_red : c_red;
                        mvwprintz( w, point( pos_x, cur_print_y ), print_col2, "x" );
                    }
                    --pos_x;

                    right_bound -= col_w;
                    continue; //special printing case; skip generic handling
                }
                break;
                case    column_act_cost:
                    if( bio_data.power_activate > 0_kJ ) {
                        print_str = string_format( "%d", units::to_kilojoule( bio_data.power_activate ) );
                    }
                    break;
                case    column_turn_cost:
                    if( bio_data.charge_time > 0 && bio_data.power_over_time > 0_kJ ) {
                        print_str = bio_data.charge_time == 1
                                    ? string_format( "%d", units::to_kilojoule( bio_data.power_over_time ) )
                                    : string_format( "%d /%d", units::to_kilojoule( bio_data.power_over_time ),
                                                     bio_data.charge_time );
                    }
                    break;
                default:
                    break;
            }
            int col_w = get_col_data( col_idx ).get_width();
            right_bound -= col_w;
            mvwprintz( w, point( right_bound, cur_print_y ), print_col, "%*s", col_w, print_str );
        }
    }

    if( is_selected ) { //fill whole line with color
        mvwprintz( w, point( 0, cur_print_y ), print_col, "%*s", right_bound, "" );
    }
    trim_and_print( w, point( 1, cur_print_y ), right_bound, print_col, "%c %s", bio->invlet,
                    bio_data.name );
}
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

static void draw_bionics_titlebar( const catacurses::window &window, player *p )
{
    werase( window );
    std::ostringstream fuel_stream;
    fuel_stream << _( "Available Fuel: " );
    for( const bionic &bio : *p->my_bionics ) {
        for( const itype_id fuel : p->get_fuel_available( bio.id ) ) {
            fuel_stream << item( fuel ).tname() << ": " << "<color_green>" << p->get_value(
                            fuel ) << "</color>" << "/" << p->get_total_fuel_capacity( fuel ) << " ";
        }
    }
    fold_and_print( window, point_east, getmaxx( window ), c_white, fuel_stream.str() );
    wrefresh( window );
}

static void draw_bionics_tabs( const catacurses::window &win, std::vector<bvector> bionics,
                               int current_mode )
{
    werase( win );

    std::vector<std::pair<bionics_displayType_id, std::string>> tabs;
    for( size_t i = 0; i < BionicsDisplayType::displayTypes.size(); i++ ) {
        if( bionics[i].empty() ) {
            continue;
        }
        BionicsDisplayType type = BionicsDisplayType::displayTypes[i];
        tabs.emplace_back( type.ident(), type.display_string() );
    }
    bionics_displayType_id cur_type_id = BionicsDisplayType::displayTypes[current_mode].ident();
    draw_tabs( win, tabs, cur_type_id );

    // Draw symbols to connect additional lines to border
    int width = getmaxx( win );
    int height = getmaxy( win );
    for( int i = 0; i < height - 1; ++i ) {
        mvwputch( win, point( 0, i ), BORDER_COLOR, LINE_XOXO ); // |
        mvwputch( win, point( width - 1, i ), BORDER_COLOR, LINE_XOXO ); // |
    }
    mvwputch( win, point( 0, height - 1 ), BORDER_COLOR, LINE_XXXO ); // |-
    mvwputch( win, point( width - 1, height - 1 ), BORDER_COLOR, LINE_XOXX ); // -|

    wrefresh( win );
}

static void draw_connectors( const catacurses::window &win, const int start_y, const int start_x,
                             const int last_x, const bionic_id &bio_id )
{
    const int LIST_START_Y = 5;
    // first: pos_y, second: occupied slots
    std::vector<std::pair<int, size_t>> pos_and_num;
    for( const auto &elem : bio_id->occupied_bodyparts ) {
        pos_and_num.emplace_back( static_cast<int>( elem.first ) + LIST_START_Y, elem.second );
    }
    if( pos_and_num.empty() ) {
        return;
    }

    // draw horizontal line from selected bionic
    const int width = last_x - start_x;
    const int turn_x = start_x + ( width > 20 ? width * 2 / 3 : width / 3 );
    mvwputch( win, point( start_x, start_y ), BORDER_COLOR, '>' );
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

        mvwputch( win, point( turn_x, y ), BORDER_COLOR, bp_chr );

        // draw horizontal line to bodypart title
        mvwhline( win, point( turn_x + 1, y ), LINE_OXOX, last_x - turn_x - 1 );
        mvwputch( win, point( last_x, y ), BORDER_COLOR, '<' );

        // draw amount of consumed slots by this CBM
        const std::string fmt_num = string_format( "(%d)", elem.second );
        mvwprintz( win, point( turn_x + std::max( 1, ( last_x - turn_x - utf8_width( fmt_num ) ) / 2 ), y ),
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
    mvwputch( win, point( turn_x, start_y ), BORDER_COLOR, bionic_chr );
}

void player::power_bionics()
{
    bool with_slots = get_option < bool >( "CBM_SLOTS_ENABLED" );
    std::vector<bvector> bionics_by_type( BionicsDisplayType::displayTypes.size() );
    size_t max_list_size = with_slots ? num_bp : 4; //start with min list size
    int max_name_length = 0;
    for( auto &b : *my_bionics ) {
        int name_w = utf8_width( b.id->name.translated() );
        if( name_w > max_name_length ) {
            max_name_length = name_w;
        }
        bionics_displayType_id dType = b.id->display_type;
        for( size_t i = 0; i < BionicsDisplayType::displayTypes.size(); i++ ) {
            if( BionicsDisplayType::displayTypes[i].ident() == dType ) {
                bionics_by_type[i].emplace_back( &b );
                if( bionics_by_type[i].size() > max_list_size ) {
                    max_list_size = bionics_by_type[i].size();
                }
                break;
            }
        }
    }
    const int SLOTS_WIDTH = 29;

    max_name_length += 2; //invlet
    size_t col_width = with_slots ? SLOTS_WIDTH : 0;
    for( size_t i = 0; i < column_num_entries; i++ ) {
        col_width += get_col_data( i ).get_width();
    }

    size_t tabs_width = 1;
    for( size_t i = 0; i < bionics_by_type.size(); i++ ) {
        if( !bionics_by_type.empty() ) {
            tabs_width += utf8_width( BionicsDisplayType::displayTypes[i].display_string() ) + 3;
        }
    }

    //added title_tab_height for the tabbed bionic display
    const int TITLE_HEIGHT = 1;
    const int TITLE_TAB_HEIGHT = 3;
    const int FOOTER_HEIGHT = 5;

    // Main window
    // +4 = top + bottom + footer line + column names
    const int requested_height = TITLE_HEIGHT + TITLE_TAB_HEIGHT + FOOTER_HEIGHT + max_list_size + 4;
    const int HEIGHT = std::min( TERMY, requested_height );

    int requested_width = std::max( max_name_length + col_width + 3, tabs_width );
    requested_width = std::max( requested_width, 80 );

    const int WIDTH = std::min( requested_width, TERMX );
    const int MAIN_WINDOW_X = ( TERMX - WIDTH ) / 2;
    const int MAIN_WINDOW_Y = ( TERMY - HEIGHT ) / 2;

    //wBio is the entire bionic window
    catacurses::window wBio = catacurses::newwin( HEIGHT, WIDTH, point( MAIN_WINDOW_X,
                              MAIN_WINDOW_Y ) );

    // Title window
    const int TITLE_WINDOW_Y = MAIN_WINDOW_Y + 1;
    catacurses::window w_title = catacurses::newwin( TITLE_HEIGHT, WIDTH - 2, point( MAIN_WINDOW_X + 1,
                                 TITLE_WINDOW_Y ) );

    //Tabs bar
    const int TAB_WINDOW_Y = TITLE_WINDOW_Y + TITLE_HEIGHT;
    catacurses::window w_tabs = catacurses::newwin( TITLE_TAB_HEIGHT, WIDTH,
                                point( MAIN_WINDOW_X,
                                       TAB_WINDOW_Y ) );

    const int header_line_y = TITLE_HEIGHT + TITLE_TAB_HEIGHT + 1;
    const int footer_start_y = HEIGHT - FOOTER_HEIGHT - 1;

    const int col_right_bound = WIDTH - ( with_slots ? SLOTS_WIDTH : 3 );

    int scroll_position = 0;
    int cursor = 0;

    const int list_start_y = header_line_y + 1;
    const int LIST_HEIGHT = footer_start_y - list_start_y - 1; //-1 is footer separator
    const int half_list_view_location = LIST_HEIGHT / 2;

    input_context ctxt( "BIONICS" );
    ctxt.register_updown();
    ctxt.register_action( "ANY_INPUT" );
    ctxt.register_action( "REASSIGN" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "USAGE_HELP" );

    int cur_tab_idx = 0;
    for( ; static_cast<size_t>( cur_tab_idx ) < bionics_by_type.size() &&
         bionics_by_type[cur_tab_idx].empty(); cur_tab_idx++ ); //try to find non-empty list
    if( static_cast<size_t>( cur_tab_idx ) >= bionics_by_type.size() ) { //if all lists are empty
        cur_tab_idx = 0;
    }

    bool redraw = true;
    for( ;; ) {
        //track which list we are looking at
        const bvector &current_bionic_list = bionics_by_type[cur_tab_idx];
        const int max_scroll_position = std::max( 0,
                                        static_cast<int>( current_bionic_list.size() ) - LIST_HEIGHT );

        if( redraw ) {
            redraw = false;

            werase( wBio );
            draw_border( wBio, BORDER_COLOR );
            nc_color col = c_white;
            print_colored_text( wBio, point_east, col, col,
                                string_format( _( "< Bionic Power: <color_light_blue>%i</color>/<color_light_blue>%i</color> >" ),
                                               units::to_kilojoule( get_power_level() ), units::to_kilojoule( get_max_power_level() ) ) );
            std::string help_str = string_format( _( "< [%s] Columns Info >" ), ctxt.get_desc( "USAGE_HELP" ) );
            mvwprintz( wBio, point( WIDTH - 1 - utf8_width( help_str ), 0 ), c_white, help_str );

            mvwputch( wBio, point( 0, footer_start_y - 1 ), BORDER_COLOR, LINE_XXXO ); // |-
            mvwputch( wBio, point( WIDTH - 1, footer_start_y - 1 ), BORDER_COLOR, LINE_XOXX ); // -|
            mvwhline( wBio, point( 1, footer_start_y - 1 ), LINE_OXOX, WIDTH - 2 );
            help_str = string_format( _( "<Press %s to reassign.  You can use invlets from any tab.>" ),
                                      ctxt.get_desc( "REASSIGN" ) );
            mvwprintz( wBio, point( 2, footer_start_y - 1 ), c_white, help_str );

            const bool name_only = current_bionic_list.empty() ||
                                   BionicsDisplayType::displayTypes[cur_tab_idx].is_hide_columns();
            if( !name_only ) {
                for( size_t col_idx = column_num_entries, right_bound = col_right_bound; col_idx-- > 0; ) {
                    bionic_col_data col_data = get_col_data( col_idx );
                    int col_w = col_data.get_width();
                    right_bound -= col_w;
                    mvwprintz( wBio, point( right_bound, header_line_y ), c_light_gray, "%*s", col_w,
                               col_data.get_name() );
                }
            }

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
            if( with_slots ) {
                for( size_t i = 0; i < bps.size(); ++i ) {
                    mvwprintz( wBio, point( pos_x, i + list_start_y - 1 ), c_light_gray, bps[i] );
                }
            }

            if( current_bionic_list.empty() ) {
                fold_and_print( wBio, point( 2, list_start_y ), pos_x - 1, c_light_gray,
                                _( "No bionics installed." ) );
            } else {
                for( size_t i = scroll_position; i < current_bionic_list.size(); i++ ) {
                    if( static_cast<int>( i ) - scroll_position == LIST_HEIGHT ) {
                        break;
                    }
                    const bool is_highlighted = cursor == static_cast<int>( i );
                    int y_pos = list_start_y + i - scroll_position;
                    print_columnns( current_bionic_list[i], is_highlighted, name_only, wBio, col_right_bound, y_pos );
                    if( is_highlighted && with_slots ) {
                        const bionic_id bio_id = current_bionic_list[i]->id;
                        draw_connectors( wBio, y_pos, col_right_bound,
                                         pos_x - 2, bio_id );

                        // redraw highlighted (occupied) body parts
                        for( auto &elem : bio_id->occupied_bodyparts ) {
                            const int i = static_cast<int>( elem.first );
                            mvwprintz( wBio, point( pos_x, i + list_start_y - 1 ), c_yellow, bps[i] );
                        }
                    }

                }
            }

            draw_scrollbar( wBio, cursor, LIST_HEIGHT, current_bionic_list.size(), point( 0, list_start_y ) );

            if( !current_bionic_list.empty() && current_bionic_list[cursor] != nullptr ) {
                fold_and_print( wBio, point( 1, footer_start_y ), WIDTH - 2, c_light_blue, "%s",
                                current_bionic_list[cursor]->id->description );
            }
        }
        wrefresh( wBio );
        draw_bionics_tabs( w_tabs, bionics_by_type, cur_tab_idx );
        draw_bionics_titlebar( w_title, this );

        const std::string action = ctxt.handle_input();
        const int ch = ctxt.get_raw_input().get_first_input();
        bionic *tmp = nullptr;

        bool need_activate = false;
        if( action == "DOWN" ) {
            redraw = true;
            if( static_cast<size_t>( cursor ) < current_bionic_list.size() - 1 ) {
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
                cursor = current_bionic_list.size() - 1;
            }
            if( scroll_position > 0 && cursor - scroll_position < half_list_view_location ) {
                scroll_position--;
            }
            if( scroll_position < max_scroll_position &&
                cursor - scroll_position > LIST_HEIGHT - half_list_view_location ) {
                scroll_position =
                    std::max( std::min<int>( current_bionic_list.size() - LIST_HEIGHT,
                                             cursor - half_list_view_location ), 0 );
            }
        } else if( action == "REASSIGN" ) {
            tmp = current_bionic_list[cursor];
            while( true ) {
                const int invlet = popup_getkey(
                                       _( "%s\n\nEnter new letter.  Press SPACE to clear a manually assigned letter, ESCAPE to cancel." ),
                                       bionic_chars.get_allowed_chars() );

                if( invlet == KEY_ESCAPE ) {
                    break;
                } else if( invlet == ' ' ) {
                    tmp->invlet = invlet;
                    break;
                } else if( bionic_chars.valid( invlet ) ) {
                    bionic *otmp = bionic_by_invlet( invlet );
                    if( otmp ) {
                        std::swap( tmp->invlet, otmp->invlet );
                    } else {
                        tmp->invlet = invlet;
                    }
                    break;
                }
            }
            redraw = true;
        } else if( action == "NEXT_TAB" ) {
            redraw = true;
            scroll_position = 0;
            cursor = 0;
            if( !current_bionic_list.empty() ) {
                do {
                    cur_tab_idx = ( cur_tab_idx + 1 ) % BionicsDisplayType::displayTypes.size();
                } while( bionics_by_type[cur_tab_idx].empty() );
            }
        } else if( action == "PREV_TAB" ) {
            redraw = true;
            scroll_position = 0;
            cursor = 0;
            if( !current_bionic_list.empty() ) {
                do {
                    if( --cur_tab_idx < 0 ) {
                        cur_tab_idx = BionicsDisplayType::displayTypes.size() - 1;
                    }
                } while( bionics_by_type[cur_tab_idx].empty() );
            }
        } else if( action == "HELP_KEYBINDINGS" ) {
            redraw = true;
        } else if( action == "CONFIRM" ) {
            if( !current_bionic_list.empty() ) {
                tmp = current_bionic_list[cursor];
            }
            need_activate = true;
        } else if( action == "USAGE_HELP" ) {
            std::string help_str;
            for( size_t i = 0; i < column_num_entries; i++ ) {
                bionic_col_data col_data = get_col_data( i );
                help_str += colorize( col_data.get_name(), c_white );
                help_str += " - ";
                help_str += colorize( col_data.get_description(), c_light_gray );
                help_str += "\n";
            }
            popup( help_str );
            redraw = true;
        } else {
            if( ch == ' ' || ch == KEY_ESCAPE ) {
                break;
            }
            tmp = bionic_by_invlet( ch );

            //jump to bionic
            if( tmp != nullptr ) {
                bool is_found = false;
                for( size_t i = 0; i < bionics_by_type.size(); i++ ) {
                    const bvector &cur_list = bionics_by_type[i];
                    cur_tab_idx = i;
                    for( size_t j = 0; j < cur_list.size(); j++ ) {
                        if( cur_list[j] == tmp ) {
                            cursor = j;
                            scroll_position = 0;
                            while( scroll_position < max_scroll_position &&
                                   cursor - scroll_position > LIST_HEIGHT - half_list_view_location ) {
                                scroll_position++;
                            }
                            is_found = true;
                            break;
                        }
                    }
                    if( is_found ) {
                        break;
                    }
                }
            }
            need_activate = true;
            redraw = true;
        }
        if( tmp == nullptr ) {
            //Do nothing
        } else if( need_activate ) {
            const bionic_id &bio_id = tmp->id;
            const bionic_data &bio_data = bio_id.obj();
            if( bio_data.activated ) {
                int bionic_idx = tmp - &( *my_bionics )[0];
                if( tmp->powered ) {
                    deactivate_bionic( bionic_idx );
                } else {
                    activate_bionic( bionic_idx );
                    if( !bio_data.toggled && !bio_data.gun_bionic ) {
                        popup( _( "You activate your %s." ), bio_data.name );
                    }
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
            } else {
                popup( _( "You can not activate %s." ), bio_data.name );
            }
        }
    }
}
