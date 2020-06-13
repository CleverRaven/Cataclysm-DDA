#include "player.h" // IWYU pragma: associated

#include <algorithm> //std::min
#include <array>
#include <cstddef>
#include <memory>

#include "bionics.h"
#include "catacharset.h"
#include "compatibility.h"
#include "game.h"
#include "input.h"
#include "inventory.h"
#include "options.h"
#include "output.h"
#include "string_formatter.h"
#include "string_id.h"
#include "translations.h"
#include "ui_manager.h"
#include "units.h"

static const std::string flag_PERPETUAL( "PERPETUAL" );
static const std::string flag_SAFE_FUEL_OFF( "SAFE_FUEL_OFF" );

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
    // space is a special case for unassigned
    if( ch == ' ' ) {
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
    input_context ctxt( "BIONICS" );

    werase( window );
    std::string fuel_string;
    bool found_fuel = false;
    fuel_string = _( "Available Fuel: " );
    for( const bionic &bio : *p->my_bionics ) {
        for( const itype_id &fuel : p->get_fuel_available( bio.id ) ) {
            found_fuel = true;
            const item temp_fuel( fuel );
            if( temp_fuel.has_flag( flag_PERPETUAL ) ) {
                if( fuel == itype_id( "sunlight" ) && !g->is_in_sunlight( p->pos() ) ) {
                    continue;
                }
                fuel_string += colorize( temp_fuel.tname(), c_green ) + " ";
                continue;
            }
            fuel_string += temp_fuel.tname() + ": " + colorize( p->get_value( fuel.str() ),
                           c_green ) + "/" + std::to_string( p->get_total_fuel_capacity( fuel ) ) + " ";
        }
        if( bio.info().is_remote_fueled && p->has_active_bionic( bio.id ) ) {
            const itype_id rem_fuel = p->find_remote_fuel( true );
            if( !rem_fuel.is_empty() ) {
                const item tmp_rem_fuel( rem_fuel );
                if( tmp_rem_fuel.has_flag( flag_PERPETUAL ) ) {
                    fuel_string += colorize( tmp_rem_fuel.tname(), c_green ) + " ";
                } else {
                    fuel_string += tmp_rem_fuel.tname() + ": " + colorize( p->get_value( "rem_" + rem_fuel.str() ),
                                   c_green ) + " ";
                }
                found_fuel = true;
            }
        }
    }
    if( !found_fuel ) {
        fuel_string.clear();
    }
    std::string power_string;
    const int curr_power = units::to_millijoule( p->get_power_level() );
    const int kilo = curr_power / units::to_millijoule( 1_kJ );
    const int joule = ( curr_power % units::to_millijoule( 1_kJ ) ) / units::to_millijoule( 1_J );
    const int milli = curr_power % units::to_millijoule( 1_J );
    if( kilo > 0 ) {
        power_string = to_string( kilo );
        if( joule > 0 ) {
            power_string += pgettext( "decimal separator", "." ) + to_string( joule );
        }
        power_string += pgettext( "energy unit: kilojoule", "kJ" );
    } else if( joule > 0 ) {
        power_string = to_string( joule );
        if( milli > 0 ) {
            power_string += pgettext( "decimal separator", "." ) + to_string( milli );
        }
        power_string += pgettext( "energy unit: joule", "J" );
    } else {
        power_string = to_string( milli ) + pgettext( "energy unit: millijoule", "mJ" );
    }

    const int pwr_str_pos = right_print( window, 1, 1, c_white,
                                         string_format( _( "Bionic Power: <color_light_blue>%s</color>/<color_light_blue>%ikJ</color>" ),
                                                 power_string, units::to_kilojoule( p->get_max_power_level() ) ) );

    mvwputch( window, point( pwr_str_pos - 1, 1 ), BORDER_COLOR, LINE_XOXO ); // |
    mvwputch( window, point( pwr_str_pos - 1, 2 ), BORDER_COLOR, LINE_XXOO ); // |_
    for( int i = pwr_str_pos; i < getmaxx( window ); i++ ) {
        mvwputch( window, point( i, 2 ), BORDER_COLOR, LINE_OXOX ); // -
    }
    for( int i = 0; i < getmaxx( window ); i++ ) {
        mvwputch( window, point( i, 0 ), BORDER_COLOR, LINE_OXOX ); // -
    }
    mvwputch( window, point( pwr_str_pos - 1, 0 ), BORDER_COLOR, LINE_OXXX ); // ^|^
    center_print( window, 0, c_light_red, _( " BIONICS " ) );

    std::string desc_append = string_format(
                                  _( "[<color_yellow>%s</color>] Reassign, [<color_yellow>%s</color>] Switch tabs, "
                                     "[<color_yellow>%s</color>] Toggle fuel saving mode, "
                                     "[<color_yellow>%s</color>] Toggle auto start mode." ),
                                  ctxt.get_desc( "REASSIGN" ), ctxt.get_desc( "NEXT_TAB" ), ctxt.get_desc( "TOGGLE_SAFE_FUEL" ),
                                  ctxt.get_desc( "TOGGLE_AUTO_START" ) );
    std::string desc;
    if( mode == REASSIGNING ) {
        desc = _( "Reassigning.  Select a bionic to reassign or press [<color_yellow>SPACE</color>] to cancel." );
        fuel_string.clear();
    } else if( mode == ACTIVATING ) {
        desc = string_format( _( "<color_green>Activating</color>  "
                                 "[<color_yellow>%s</color>] Examine, %s" ),
                              ctxt.get_desc( "TOGGLE_EXAMINE" ), desc_append );
    } else if( mode == EXAMINING ) {
        desc = string_format( _( "<color_light_blue>Examining</color>  "
                                 "[<color_yellow>%s</color>] Activate, %s" ),
                              ctxt.get_desc( "TOGGLE_EXAMINE" ), desc_append );
    }

    // NOLINTNEXTLINE(cata-use-named-point-constants)
    int lines_count = fold_and_print( window, point( 1, 1 ), pwr_str_pos - 2, c_white, desc );
    fold_and_print( window, point( 1, ++lines_count ), pwr_str_pos - 2, c_white, fuel_string );
    wnoutrefresh( window );
}

//builds the power usage string of a given bionic
static std::string build_bionic_poweronly_string( const bionic &bio )
{
    const bionic_data &bio_data = bio.id.obj();
    std::vector<std::string> properties;

    if( bio_data.power_activate > 0_kJ ) {
        properties.push_back( string_format( _( "%s act" ),
                                             units::display( bio_data.power_activate ) ) );
    }
    if( bio_data.power_deactivate > 0_kJ ) {
        properties.push_back( string_format( _( "%s deact" ),
                                             units::display( bio_data.power_deactivate ) ) );
    }
    if( bio_data.charge_time > 0 && bio_data.power_over_time > 0_kJ ) {
        properties.push_back( bio_data.charge_time == 1
                              ? string_format( _( "%s/turn" ), units::display( bio_data.power_over_time ) )
                              : string_format( _( "%s/%d turns" ), units::display( bio_data.power_over_time ),
                                               bio_data.charge_time ) );
    }
    if( bio_data.has_flag( "BIONIC_TOGGLED" ) ) {
        properties.push_back( bio.powered ? _( "ON" ) : _( "OFF" ) );
    }
    if( bio.incapacitated_time > 0_turns ) {
        properties.push_back( _( "(incapacitated)" ) );
    }
    if( !bio.has_flag( flag_SAFE_FUEL_OFF ) && ( !bio.info().fuel_opts.empty() ||
            bio.info().is_remote_fueled ) ) {
        properties.push_back( _( "(fuel saving ON)" ) );
    }
    if( bio.is_auto_start_on() && ( !bio.info().fuel_opts.empty() || bio.info().is_remote_fueled ) ) {
        const std::string label = string_format( _( "(auto start < %d %%)" ),
                                  static_cast<int>( bio.get_auto_start_thresh() * 100 ) );
        properties.push_back( label );
    }

    return enumerate_as_string( properties, enumeration_conjunction::none );
}

//generates the string that show how much power a bionic uses
static std::string build_bionic_powerdesc_string( const bionic &bio )
{
    std::string power_desc;
    const std::string power_string = build_bionic_poweronly_string( bio );
    power_desc += bio.id->name.translated();
    if( !power_string.empty() ) {
        power_desc += ", " + power_string;
    }
    return power_desc;
}

static void draw_bionics_tabs( const catacurses::window &win, const size_t active_num,
                               const size_t passive_num, const bionic_tab_mode current_mode )
{
    werase( win );

    const std::vector<std::pair<bionic_tab_mode, std::string>> tabs = {
        { bionic_tab_mode::TAB_ACTIVE, string_format( _( "ACTIVE (%i)" ), active_num ) },
        { bionic_tab_mode::TAB_PASSIVE, string_format( _( "PASSIVE (%i)" ), passive_num ) },
    };
    draw_tabs( win, tabs, current_mode );

    // Draw symbols to connect additional lines to border
    int width = getmaxx( win );
    int height = getmaxy( win );
    for( int i = 0; i < height - 1; ++i ) {
        // |
        mvwputch( win, point( 0, i ), BORDER_COLOR, LINE_XOXO );
        // |
        mvwputch( win, point( width - 1, i ), BORDER_COLOR, LINE_XOXO );
    }
    // |-
    mvwputch( win, point( 0, height - 1 ), BORDER_COLOR, LINE_XXXO );
    // -|
    mvwputch( win, point( width - 1, height - 1 ), BORDER_COLOR, LINE_XOXX );

    wnoutrefresh( win );
}

static void draw_description( const catacurses::window &win, const bionic &bio )
{
    werase( win );
    const int width = getmaxx( win );
    const std::string poweronly_string = build_bionic_poweronly_string( bio );
    int ypos = fold_and_print( win, point_zero, width, c_white, "%s", bio.id->name );
    if( !poweronly_string.empty() ) {
        ypos += fold_and_print( win, point( 0, ypos ), width, c_light_gray,
                                _( "Power usage: %s" ), poweronly_string );
    }
    ypos += 1 + fold_and_print( win, point( 0, ypos ), width, c_light_blue, "%s", bio.id->description );

    // TODO: Unhide when enforcing limits
    if( get_option < bool >( "CBM_SLOTS_ENABLED" ) ) {
        const bool each_bp_on_new_line = ypos + static_cast<int>( num_bp ) + 1 < getmaxy( win );
        fold_and_print( win, point( 0, ypos ), width, c_light_gray, list_occupied_bps( bio.id,
                        _( "This bionic occupies the following body parts:" ), each_bp_on_new_line ) );
    }
    wnoutrefresh( win );
}

static void draw_connectors( const catacurses::window &win, const point &start,
                             int last_x, const bionic_id &bio_id )
{
    const int LIST_START_Y = 7;
    // first: pos_y, second: occupied slots
    std::vector<std::pair<int, size_t>> pos_and_num;
    for( const std::pair<const string_id<body_part_type>, size_t> &elem : bio_id->occupied_bodyparts ) {
        pos_and_num.emplace_back( static_cast<int>( elem.first->token ) + LIST_START_Y, elem.second );
    }
    if( pos_and_num.empty() || !get_option < bool >( "CBM_SLOTS_ENABLED" ) ) {
        return;
    }

    // draw horizontal line from selected bionic
    const int turn_x = start.x + ( last_x - start.x ) * 2 / 3;
    mvwputch( win, start, BORDER_COLOR, '>' );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwhline( win, start + point( 1, 0 ), LINE_OXOX, turn_x - start.x - 1 );

    int min_y = start.y;
    int max_y = start.y;
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
        if( !move_up && y < start.y ) {
            move_up = true;
        }
        if( !move_same && y == start.y ) {
            move_same = true;
        }
        if( !move_down && y > start.y ) {
            move_down = true;
        }

        // symbol is defined incorrectly for case ( y == start_y ) but
        // that's okay because it's overlapped by bionic_chr anyway
        int bp_chr = ( y > start.y ) ? LINE_XXOO : LINE_OXXO;
        if( ( max_y > y && y > start.y ) || ( min_y < y && y < start.y ) ) {
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
    // 001
    // '-'
    int bionic_chr = LINE_OXOX;
    if( move_up && !move_down && !move_same ) {
        // 100
        // '_|'
        bionic_chr = LINE_XOOX;
    } else if( move_up && move_down && !move_same ) {
        // 110
        // '-|'
        bionic_chr = LINE_XOXX;
    } else if( move_up && move_down && move_same ) {
        // 111
        // '-|-'
        bionic_chr = LINE_XXXX;
    } else if( move_up && !move_down && move_same ) {
        // 101
        // '_|_'
        bionic_chr = LINE_XXOX;
    } else if( !move_up && move_down && !move_same ) {
        // 010
        // '^|'
        bionic_chr = LINE_OOXX;
    } else if( !move_up && move_down && move_same ) {
        // 011
        // '^|^'
        bionic_chr = LINE_OXXX;
    }
    mvwputch( win, point( turn_x, start.y ), BORDER_COLOR, bionic_chr );
}

//get a text color depending on the power/powering state of the bionic
static nc_color get_bionic_text_color( const bionic &bio, const bool isHighlightedBionic )
{
    nc_color type = c_white;
    bool is_power_source = bio.id->has_flag( "BIONIC_POWER_SOURCE" );
    if( bio.id->activated ) {
        if( isHighlightedBionic ) {
            if( bio.powered && !is_power_source ) {
                type = h_red;
            } else if( is_power_source && !bio.powered ) {
                type = h_light_cyan;
            } else if( is_power_source && bio.powered ) {
                type = h_light_green;
            } else {
                type = h_light_red;
            }
        } else {
            if( bio.powered && !is_power_source ) {
                type = c_red;
            } else if( is_power_source && !bio.powered ) {
                type = c_light_cyan;
            } else if( is_power_source && bio.powered ) {
                type = c_light_green;
            } else {
                type = c_light_red;
            }
        }
    } else {
        if( isHighlightedBionic ) {
            if( is_power_source ) {
                type = h_light_cyan;
            } else {
                type = h_cyan;
            }
        } else {
            if( is_power_source ) {
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
    const int TITLE_HEIGHT = 4;
    const int TITLE_TAB_HEIGHT = 3;
    int HEIGHT = 0;
    int WIDTH = 0;
    int LIST_HEIGHT = 0;
    int list_start_y = 0;
    int half_list_view_location = 0;
    catacurses::window wBio;
    catacurses::window w_description;
    catacurses::window w_title;
    catacurses::window w_tabs;

    bool hide = false;
    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        if( hide ) {
            ui.position( point_zero, point_zero );
            return;
        }
        // Main window
        /** Total required height is:
         * top frame line:                                         + 1
         * height of title window:                                 + TITLE_HEIGHT
         * height of tabs:                                         + TITLE_TAB_HEIGHT
         * height of the biggest list of active/passive bionics:   + bionic_count
         * bottom frame line:                                      + 1
         * TOTAL: TITLE_HEIGHT + TITLE_TAB_HEIGHT + bionic_count + 2
         */
        HEIGHT = std::min( TERMY,
                           std::max( FULL_SCREEN_HEIGHT,
                                     TITLE_HEIGHT + TITLE_TAB_HEIGHT +
                                     static_cast<int>( my_bionics->size() ) + 2 ) );
        WIDTH = FULL_SCREEN_WIDTH + ( TERMX - FULL_SCREEN_WIDTH ) / 2;
        const int START_X = ( TERMX - WIDTH ) / 2;
        const int START_Y = ( TERMY - HEIGHT ) / 2;
        //wBio is the entire bionic window
        wBio = catacurses::newwin( HEIGHT, WIDTH, point( START_X, START_Y ) );

        LIST_HEIGHT = HEIGHT - TITLE_HEIGHT - TITLE_TAB_HEIGHT - 2;

        const int DESCRIPTION_WIDTH = WIDTH - 2 - 40;
        const int DESCRIPTION_START_Y = START_Y + TITLE_HEIGHT + TITLE_TAB_HEIGHT + 1;
        const int DESCRIPTION_START_X = START_X + 1 + 40;
        //w_description is the description panel that is controlled with ! key
        w_description = catacurses::newwin( LIST_HEIGHT, DESCRIPTION_WIDTH,
                                            point( DESCRIPTION_START_X, DESCRIPTION_START_Y ) );

        // Title window
        const int TITLE_START_Y = START_Y + 1;
        const int HEADER_LINE_Y = TITLE_HEIGHT + TITLE_TAB_HEIGHT;
        w_title = catacurses::newwin( TITLE_HEIGHT, WIDTH - 2, point( START_X + 1,
                                      START_Y ) );

        const int TAB_START_Y = TITLE_START_Y + 3;
        //w_tabs is the tab bar for passive and active bionic groups
        w_tabs = catacurses::newwin( TITLE_TAB_HEIGHT, WIDTH,
                                     point( START_X, TAB_START_Y ) );

        // offset for display: bionic with index i is drawn at y=list_start_y+i
        // drawing the bionics starts with bionic[scroll_position]
        // scroll_position;
        list_start_y = HEADER_LINE_Y;
        half_list_view_location = LIST_HEIGHT / 2;

        ui.position_from_window( wBio );
    } );
    ui.mark_resize();

    int scroll_position = 0;
    int cursor = 0;

    //generate the tab title string and a count of the bionics owned
    bionic_menu_mode menu_mode = ACTIVATING;
    int max_scroll_position = 0;

    input_context ctxt( "BIONICS" );
    ctxt.register_updown();
    ctxt.register_action( "ANY_INPUT" );
    ctxt.register_action( "TOGGLE_EXAMINE" );
    ctxt.register_action( "REASSIGN" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "TOGGLE_SAFE_FUEL" );
    ctxt.register_action( "TOGGLE_AUTO_START" );

    ui.on_redraw( [&]( const ui_adaptor & ) {
        if( hide ) {
            return;
        }

        std::vector<bionic *> *current_bionic_list = ( tab_mode == TAB_ACTIVE ? &active : &passive );

        werase( wBio );
        draw_border( wBio, BORDER_COLOR, _( " BIONICS " ) );
        mvwputch( wBio, point( getmaxx( wBio ) - 1, 2 ), BORDER_COLOR, LINE_XOXX ); // -|

        int max_width = 0;
        std::vector<std::string> bps;
        for( const bodypart_id &bp : get_all_body_parts() ) {
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
                mvwprintz( wBio, point( pos_x, i + list_start_y ), c_light_gray, bps[i] );
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
            fold_and_print( wBio, point( 2, list_start_y ), pos_x - 1, c_light_gray, msg );
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
                trim_and_print( wBio, point( 2, list_start_y + i - scroll_position ), WIDTH - 3, col,
                                desc );
                if( is_highlighted && menu_mode != EXAMINING && get_option < bool >( "CBM_SLOTS_ENABLED" ) ) {
                    const bionic_id bio_id = ( *current_bionic_list )[i]->id;
                    draw_connectors( wBio, point( utf8_width( desc ) + 3, list_start_y + i - scroll_position ),
                                     pos_x - 2, bio_id );

                    // redraw highlighted (occupied) body parts
                    for( const std::pair<const string_id<body_part_type>, size_t> &elem : bio_id->occupied_bodyparts ) {
                        const int i = static_cast<int>( elem.first->token );
                        mvwprintz( wBio, point( pos_x, i + list_start_y ), c_yellow, bps[i] );
                    }
                }

            }
        }

        draw_scrollbar( wBio, cursor, LIST_HEIGHT, current_bionic_list->size(), point( 0, list_start_y ) );

        wnoutrefresh( wBio );
        draw_bionics_tabs( w_tabs, active.size(), passive.size(), tab_mode );

        draw_bionics_titlebar( w_title, this, menu_mode );
        if( menu_mode == EXAMINING && !current_bionic_list->empty() ) {
            draw_description( w_description, *( *current_bionic_list )[cursor] );
        }
    } );

    for( ;; ) {
        ui_manager::redraw();

        //track which list we are looking at
        std::vector<bionic *> *current_bionic_list = ( tab_mode == TAB_ACTIVE ? &active : &passive );
        max_scroll_position = std::max( 0, static_cast<int>( current_bionic_list->size() ) - LIST_HEIGHT );
        scroll_position = clamp( scroll_position, 0, max_scroll_position );
        cursor = clamp<int>( cursor, 0, current_bionic_list->size() );

#if defined(__ANDROID__)
        ctxt.get_registered_manual_keys().clear();
        for( size_t i = 0; i < current_bionic_list->size(); i++ ) {
            ctxt.register_manual_key( ( *current_bionic_list )[i]->invlet,
                                      build_bionic_powerdesc_string( *( *current_bionic_list )[i] ).c_str() );
        }
#endif

        const std::string action = ctxt.handle_input();
        const int ch = ctxt.get_raw_input().get_first_input();
        bionic *tmp = nullptr;

        if( action == "DOWN" ) {
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
            const int newch = popup_getkey( _( "%s; enter new letter.  Space to clear.  Esc to cancel." ),
                                            tmp->id->name );
            if( newch == ch || newch == KEY_ESCAPE ) {
                continue;
            }
            if( newch == ' ' ) {
                tmp->invlet = ' ';
                continue;
            }
            if( !bionic_chars.valid( newch ) ) {
                popup( _( "Invalid bionic letter.  Only those characters are valid:\n\n%s" ),
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
            scroll_position = 0;
            cursor = 0;
            if( tab_mode == TAB_ACTIVE ) {
                tab_mode = TAB_PASSIVE;
            } else {
                tab_mode = TAB_ACTIVE;
            }
        } else if( action == "PREV_TAB" ) {
            scroll_position = 0;
            cursor = 0;
            if( tab_mode == TAB_PASSIVE ) {
                tab_mode = TAB_ACTIVE;
            } else {
                tab_mode = TAB_PASSIVE;
            }
        } else if( action == "REASSIGN" ) {
            menu_mode = REASSIGNING;
        } else if( action == "TOGGLE_EXAMINE" ) {
            // switches between activation and examination
            menu_mode = menu_mode == ACTIVATING ? EXAMINING : ACTIVATING;
        } else if( action == "TOGGLE_SAFE_FUEL" ) {
            auto &bio_list = tab_mode == TAB_ACTIVE ? active : passive;
            if( !current_bionic_list->empty() ) {
                tmp = bio_list[cursor];
                if( !tmp->info().fuel_opts.empty() || tmp->info().is_remote_fueled ) {
                    tmp->toggle_safe_fuel_mod();
                    g->invalidate_main_ui_adaptor();
                } else {
                    popup( _( "You can't toggle fuel saving mode on a non-fueled CBM." ) );
                }
            }
        } else if( action == "TOGGLE_AUTO_START" ) {
            auto &bio_list = tab_mode == TAB_ACTIVE ? active : passive;
            if( !current_bionic_list->empty() ) {
                tmp = bio_list[cursor];
                if( !tmp->info().fuel_opts.empty() || tmp->info().is_remote_fueled ) {
                    tmp->toggle_auto_start_mod();
                    g->invalidate_main_ui_adaptor();
                } else {
                    popup( _( "You can't toggle auto start mode on a non-fueled CBM." ) );
                }
            }
        } else if( action == "CONFIRM" || action == "ANY_INPUT" ) {
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
                // entered a key that is not mapped to any bionic
                continue;
            }
            bio_last = tmp;
            const bionic_id &bio_id = tmp->id;
            const bionic_data &bio_data = bio_id.obj();
            if( menu_mode == ACTIVATING ) {
                if( bio_data.activated ) {
                    int b = tmp - &( *my_bionics )[0];
                    hide = true;
                    ui.mark_resize();
                    if( tmp->powered ) {
                        deactivate_bionic( b );
                    } else {
                        bool close_ui = false;
                        activate_bionic( b, false, &close_ui );
                        // Exit this ui if we are firing a complex bionic
                        if( close_ui || tmp->ammo_count > 0 ) {
                            break;
                        }
                    }
                    hide = false;
                    ui.mark_resize();
                    g->invalidate_main_ui_adaptor();
                    if( moves < 0 ) {
                        return;
                    }
                    continue;
                } else {
                    popup( _( "You can not activate %s!\n"
                              "To read a description of %s, press '%s', then '%c'." ), bio_data.name,
                           bio_data.name, ctxt.get_desc( "TOGGLE_EXAMINE" ), tmp->invlet );
                }
            } else if( menu_mode == EXAMINING ) {
                // Describing bionics, allow user to jump to description key
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
        } else if( action == "QUIT" ) {
            break;
        }
    }
}
