#include "vehicle.h"

#include "coordinate_conversions.h"
#include "output.h"
#include "game.h"
#include "veh_interact.h"
#include "cursesdef.h"
#include "catacharset.h"
#include "messages.h"
#include "vpart_position.h"
#include "vpart_reference.h"
#include "string_formatter.h"
#include "ui.h"
#include "debug.h"
#include "translations.h"
#include "options.h"
#include "veh_type.h"
#include "itype.h"
#include "cata_utility.h"

#include <sstream>
#include <stdlib.h>
#include <set>
#include <queue>
#include <math.h>
#include <array>
#include <numeric>
#include <algorithm>
#include <cassert>

static const std::string part_location_structure( "structure" );

const std::string vehicle::disp_name() const
{
    return string_format( _( "the %s" ), name.c_str() );
}

char vehicle::part_sym( const int p, const bool exact ) const
{
    if( p < 0 || p >= ( int )parts.size() || parts[p].removed ) {
        return ' ';
    }

    const int displayed_part = exact ? p : part_displayed_at( parts[p].mount.x, parts[p].mount.y );

    if( part_flag( displayed_part, VPFLAG_OPENABLE ) && parts[displayed_part].open ) {
        return '\''; // open door
    } else {
        return parts[ displayed_part ].is_broken() ?
               part_info( displayed_part ).sym_broken : part_info( displayed_part ).sym;
    }
}

// similar to part_sym(int p) but for use when drawing SDL tiles. Called only by cata_tiles during draw_vpart
// vector returns at least 1 element, max of 2 elements. If 2 elements the second denotes if it is open or damaged
vpart_id vehicle::part_id_string( int const p, char &part_mod ) const
{
    part_mod = 0;
    if( p < 0 || p >= ( int )parts.size() || parts[p].removed ) {
        return vpart_id::NULL_ID();
    }

    int displayed_part = part_displayed_at( parts[p].mount.x, parts[p].mount.y );
    const vpart_id idinfo = parts[displayed_part].id;

    if( part_flag( displayed_part, VPFLAG_OPENABLE ) && parts[displayed_part].open ) {
        part_mod = 1; // open
    } else if( parts[ displayed_part ].is_broken() ) {
        part_mod = 2; // broken
    }

    return idinfo;
}

nc_color vehicle::part_color( const int p, const bool exact ) const
{
    if( p < 0 || p >= ( int )parts.size() ) {
        return c_black;
    }

    nc_color col;

    int parm = -1;

    //If armoring is present and the option is set, it colors the visible part
    if( get_option<bool>( "VEHICLE_ARMOR_COLOR" ) ) {
        parm = part_with_feature( p, VPFLAG_ARMOR, false );
    }

    if( parm >= 0 ) {
        col = part_info( parm ).color;
    } else {
        const int displayed_part = exact ? p : part_displayed_at( parts[p].mount.x, parts[p].mount.y );

        if( displayed_part < 0 || displayed_part >= ( int )parts.size() ) {
            return c_black;
        }
        if( parts[displayed_part].blood > 200 ) {
            col = c_red;
        } else if( parts[displayed_part].blood > 0 ) {
            col = c_light_red;
        } else if( parts[displayed_part].is_broken() ) {
            col = part_info( displayed_part ).color_broken;
        } else {
            col = part_info( displayed_part ).color;
        }

    }

    if( exact ) {
        return col;
    }

    // curtains turn windshields gray
    int curtains = part_with_feature( p, VPFLAG_CURTAIN, false );
    if( curtains >= 0 ) {
        if( part_with_feature( p, VPFLAG_WINDOW, true ) >= 0 && !parts[curtains].open ) {
            col = part_info( curtains ).color;
        }
    }

    //Invert colors for cargo parts with stuff in them
    int cargo_part = part_with_feature( p, VPFLAG_CARGO, true );
    if( cargo_part > 0 && !get_items( cargo_part ).empty() ) {
        return invert_color( col );
    } else {
        return col;
    }
}

/**
 * Prints a list of all parts to the screen inside of a boxed window, possibly
 * highlighting a selected one.
 * @param win The window to draw in.
 * @param y1 The y-coordinate to start drawing at.
 * @param max_y Draw no further than this y-coordinate.
 * @param width The width of the window.
 * @param p The index of the part being examined.
 * @param hl The index of the part to highlight (if any).
 */
int vehicle::print_part_list( const catacurses::window &win, int y1, const int max_y, int width,
                              int p, int hl /*= -1*/ ) const
{
    if( p < 0 || p >= ( int )parts.size() ) {
        return y1;
    }
    std::vector<int> pl = this->parts_at_relative( parts[p].mount.x, parts[p].mount.y, true );
    int y = y1;
    for( size_t i = 0; i < pl.size(); i++ ) {
        if( y >= max_y ) {
            mvwprintz( win, y, 1, c_yellow, _( "More parts here..." ) );
            ++y;
            break;
        }

        const vehicle_part &vp = parts[ pl [ i ] ];
        nc_color col_cond = vp.is_broken() ? c_dark_gray : vp.base.damage_color();

        std::string partname = vp.name();

        if( vp.is_fuel_store() && vp.ammo_current() != "null" ) {
            partname += string_format( " (%s)", item::nname( vp.ammo_current() ).c_str() );
        }

        if( part_flag( pl[i], "CARGO" ) ) {
            //~ used/total volume of a cargo vehicle part
            partname += string_format( _( " (vol: %s/%s %s)" ),
                                       format_volume( stored_volume( pl[i] ) ).c_str(),
                                       format_volume( max_volume( pl[i] ) ).c_str(),
                                       volume_units_abbr() );
        }

        bool armor = part_flag( pl[i], "ARMOR" );
        std::string left_sym;
        std::string right_sym;
        if( armor ) {
            left_sym = "(";
            right_sym = ")";
        } else if( part_info( pl[i] ).location == part_location_structure ) {
            left_sym = "[";
            right_sym = "]";
        } else {
            left_sym = "-";
            right_sym = "-";
        }
        nc_color sym_color = ( int )i == hl ? hilite( c_light_gray ) : c_light_gray;
        mvwprintz( win, y, 1, sym_color, left_sym );
        trim_and_print( win, y, 2, getmaxx( win ) - 4,
                        ( int )i == hl ? hilite( col_cond ) : col_cond, partname );
        wprintz( win, sym_color, right_sym );

        if( i == 0 && vpart_position( const_cast<vehicle &>( *this ), pl[i] ).is_inside() ) {
            //~ indicates that a vehicle part is inside
            mvwprintz( win, y, width - 2 - utf8_width( _( "Interior" ) ), c_light_gray, _( "Interior" ) );
        } else if( i == 0 ) {
            //~ indicates that a vehicle part is outside
            mvwprintz( win, y, width - 2 - utf8_width( _( "Exterior" ) ), c_light_gray, _( "Exterior" ) );
        }
        y++;
    }

    // print the label for this location
    const cata::optional<std::string> label = vpart_position( const_cast<vehicle &>( *this ),
            p ).get_label();
    if( label && y <= max_y ) {
        mvwprintz( win, y++, 1, c_light_red, _( "Label: %s" ), label->c_str() );
    }

    return y;
}

/**
 * Prints a list of descriptions for all parts to the screen inside of a boxed window
 * @param win The window to draw in.
 * @param max_y Draw no further than this y-coordinate.
 * @param width The width of the window.
 * @param &p The index of the part being examined.
 * @param start_at Which vehicle part to start printing at.
 * @param start_limit the part index beyond which the display is full
 */
void vehicle::print_vparts_descs( const catacurses::window &win, int max_y, int width, int &p,
                                  int &start_at, int &start_limit ) const
{
    if( p < 0 || p >= ( int )parts.size() ) {
        return;
    }

    std::vector<int> pl = this->parts_at_relative( parts[p].mount.x, parts[p].mount.y, true );
    std::ostringstream msg;

    int lines = 0;
    /*
     * start_at and start_limit interaction is little tricky
     * start_at and start_limit start at 0 when moving to a new frame
     * if all the descriptions are displayed in the window, start_limit stays at 0 and
     *    start_at is capped at 0 - so no scrolling at all.
     * if all the descriptions aren't displayed, start_limit jumps to the last displayed part
     *    and the next scrollthrough can start there - so scrolling down happens.
     * when the scroll reaches the point where all the remaining descriptions are displayed in
     *    the window, start_limit is set to start_at again.
     * on the next attempted scrolldown, start_limit is set to the nth item, and start_at is
     *    capped to the nth item, so no more scrolling down.
     * start_at can always go down, but never below 0, so scrolling up is only possible after
     *    some scrolling down has occurred.
     * important! the calling function needs to track p, start_at, and start_limit, and set
     *    start_limit to 0 if p changes.
     */
    start_at = std::max( 0, std::min( start_at, start_limit ) );
    if( start_at ) {
        msg << "<color_yellow>" << "<  " << _( "More parts here..." ) << "</color>\n";
        lines += 1;
    }
    for( size_t i = start_at; i < pl.size(); i++ ) {
        const vehicle_part &vp = parts[ pl [ i ] ];
        std::ostringstream possible_msg;
        std::string name_color = string_format( "<color_%1$s>",
                                                string_from_color( vp.is_broken() ? c_dark_gray : c_light_green ) );
        possible_msg << name_color << vp.name() << "</color>\n";
        std::string desc_color = string_format( "<color_%1$s>",
                                                string_from_color( vp.is_broken() ? c_dark_gray : c_light_gray ) );
        int new_lines = 2 + vp.info().format_description( possible_msg, desc_color, width - 2 );
        if( vp.has_flag( vehicle_part::carrying_flag ) ) {
            possible_msg << "  Carrying a vehicle on a rack.\n";
            new_lines += 1;
        }
        if( vp.has_flag( vehicle_part::carried_flag ) ) {
            std::string carried_name = vp.carry_names.top();
            possible_msg << string_format( "  Part of a %s carried on a rack.\n",
                                           carried_name.substr( vehicle_part::name_offset ) );
            new_lines += 1;
        }

        possible_msg << "</color>\n";
        if( lines + new_lines <= max_y ) {
            msg << possible_msg.str();
            lines += new_lines;
            start_limit = start_at;
        } else {
            msg << "<color_yellow>" << _( "More parts here..." ) << "  >" << "</color>\n";
            start_limit = i;
            break;
        }
    }
    werase( win );
    fold_and_print( win, 0, 1, width, c_light_gray, msg.str() );
    wrefresh( win );
}

/**
 * Returns an array of fuel types that can be printed
 * @return An array of printable fuel type ids
 */
std::vector<itype_id> vehicle::get_printable_fuel_types() const
{
    std::set<itype_id> opts;
    for( const auto &pt : parts ) {
        if( ( pt.is_fuel_store() ) && pt.ammo_current() != "null" ) {
            opts.emplace( pt.ammo_current() );
        }
    }

    std::vector<itype_id> res( opts.begin(), opts.end() );

    std::sort( res.begin(), res.end(), [&]( const itype_id & lhs, const itype_id & rhs ) {
        return basic_consumption( rhs ) < basic_consumption( lhs );
    } );

    return res;
}

/**
 * Prints all of the fuel indicators of the vehicle
 * @param win Pointer to the window to draw in.
 * @param y Y location to draw at.
 * @param x X location to draw at.
 * @param start_index Starting index in array of fuel gauges to start reading from
 * @param fullsize true if it's expected to print multiple rows
 * @param verbose true if there should be anything after the gauge (either the %, or number)
 * @param desc true if the name of the fuel should be at the end
 * @param isHorizontal true if the menu is not vertical
 */
void vehicle::print_fuel_indicators( const catacurses::window &win, int y, int x, int start_index,
                                     bool fullsize, bool verbose, bool desc, bool isHorizontal ) const
{
    auto fuels = get_printable_fuel_types();

    if( !fullsize ) {
        if( !fuels.empty() ) {
            print_fuel_indicator( win, y, x, fuels.front(), verbose, desc );
        }
        return;
    }

    int yofs = 0;
    int max_gauge = ( ( isHorizontal ) ? 12 : 5 ) + start_index;
    int max_size = std::min( ( int )fuels.size(), max_gauge );

    for( int i = start_index; i < max_size; i++ ) {
        const itype_id &f = fuels[i];
        print_fuel_indicator( win, y + yofs, x, f, verbose, desc );
        yofs++;
    }

    // check if the current index is less than the max size minus 12 or 5, to indicate that there's more
    if( ( start_index < ( int )fuels.size() - ( ( isHorizontal ) ? 12 : 5 ) ) ) {
        mvwprintz( win, y + yofs, x, c_light_green, ">" );
        wprintz( win, c_light_gray, " for more" );
    }
}

/**
 * Prints a fuel gauge for a vehicle
 * @param w Pointer to the window to draw in.
 * @param y Y location to draw at.
 * @param x X location to draw at.
 * @param fuel_type ID of the fuel type to draw
 * @param verbose true if there should be anything after the gauge (either the %, or number)
 * @param desc true if the name of the fuel should be at the end
 */
void vehicle::print_fuel_indicator( const catacurses::window &win, int y, int x,
                                    const itype_id &fuel_type, bool verbose, bool desc ) const
{
    const char fsyms[5] = { 'E', '\\', '|', '/', 'F' };
    nc_color col_indf1 = c_light_gray;
    int cap = fuel_capacity( fuel_type );
    int f_left = fuel_left( fuel_type );
    nc_color f_color = item::find_type( fuel_type )->color;
    mvwprintz( win, y, x, col_indf1, "E...F" );
    int amnt = cap > 0 ? f_left * 99 / cap : 0;
    int indf = ( amnt / 20 ) % 5;
    mvwprintz( win, y, x + indf, f_color, "%c", fsyms[indf] );
    if( verbose ) {
        if( debug_mode ) {
            mvwprintz( win, y, x + 6, f_color, "%d/%d", f_left, cap );
        } else {
            mvwprintz( win, y, x + 6, f_color, "%d", ( f_left * 100 ) / cap );
            wprintz( win, c_light_gray, "%c", 045 );
        }
    }
    if( desc ) {
        wprintz( win, c_light_gray, " - %s", item::nname( fuel_type ).c_str() );
    }
}
