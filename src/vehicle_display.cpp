#include "vehicle.h" // IWYU pragma: associated

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <optional>
#include <set>
#include <string>

#include "calendar.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "color.h"
#include "cursesdef.h"
#include "debug.h"
#include "itype.h"
#include "options.h"
#include "output.h"
#include "string_formatter.h"
#include "translations.h"
#include "units.h"
#include "units_utility.h"
#include "veh_type.h"
#include "vpart_position.h"
#include "vpart_range.h"

static const std::string part_location_structure( "structure" );
static const ammotype ammo_battery( "battery" );

static const itype_id fuel_type_muscle( "muscle" );
static const itype_id itype_battery( "battery" );

std::string vehicle::disp_name() const
{
    return string_format( _( "the %s" ), name );
}

vpart_display::vpart_display()
    : id( vpart_id::NULL_ID() )
    , variant( vpart_id::NULL_ID()->variants.begin()->second ) {}

vpart_display::vpart_display( const vehicle_part &vp )
    : id( vp.info().id )
    , variant( vp.info().variants.at( vp.variant ) ) {}

std::string vpart_display::get_tileset_id() const
{
    std::string res;
    const size_t variant_size = variant.id.size();
    res.reserve( 4 + id.str().size() + variant_size );
    res.append( "vp_" );
    res.append( id.str() );
    if( variant_size > 0 ) {
        res.append( 1, vehicles::variant_separator );
        res.append( variant.id );
    }
    return res;
}

vpart_display vehicle::get_display_of_tile( const point_rel_ms &dp, bool rotate, bool include_fake,
        bool below_roof, bool roof ) const
{
    const int part_idx = part_displayed_at( dp, include_fake, below_roof, roof );
    if( part_idx == -1 ) {
        debugmsg( "no display part at mount (%d, %d)", dp.x(), dp.y() );
        return {};
    }

    const vehicle_part &vp = part( part_idx );
    if( vp.hidden ) {
        return {};
    }
    const vpart_info &vpi = vp.info();

    auto variant_it = vpi.variants.find( vp.variant );
    if( variant_it == vpi.variants.end() ) {
        // shouldn't happen but just in case...
        variant_it = vpi.variants.begin();
        const std::string fallback_variant = variant_it->first;
        debugmsg( "part '%s' uses non-existent variant '%s' setting to '%s'",
                  vpi.id.str(), vp.variant, fallback_variant );
        vehicle_part &vp_cast = const_cast<vehicle_part &>( vp );
        vp_cast.variant = fallback_variant;
    }
    const vpart_variant &vv = variant_it->second;

    vpart_display ret( vp );
    ret.is_broken = vp.is_broken();
    ret.is_open = vp.open && vpi.has_flag( VPFLAG_OPENABLE );
    ret.color = ret.is_broken ? vpi.color_broken : vpi.color;

    if( ret.is_open ) { // open door (or similar OPENABLE part)
        ret.symbol = '\'';
        ret.symbol_curses = '\'';
    } else {
        const units::angle direction = rotate ? 270_degrees - face.dir() : 0_degrees;
        ret.symbol = vv.get_symbol( direction, ret.is_broken );
        ret.symbol_curses = vpart_variant::get_symbol_curses( ret.symbol );
    }

    // curtain color override
    const int curtains = part_with_feature( dp, VPFLAG_CURTAIN, true );
    if( curtains >= 0 ) {
        const vehicle_part &vp_curtain = parts[curtains];
        if( !vp_curtain.open && part_with_feature( dp, VPFLAG_WINDOW, true ) >= 0 ) {
            ret.color = vp_curtain.info().color;
        }
    }

    // armor color override
    if( get_option<bool>( "VEHICLE_ARMOR_COLOR" ) ) {
        const int parm = part_with_feature( dp, VPFLAG_ARMOR, true );
        if( parm != -1 ) {
            const vehicle_part &vp_armor = parts[parm];
            ret.color = vp_armor.info().color;
        }
    }

    // blood color override
    if( vp.blood > 200 ) {
        ret.color = c_red;
    } else if( vp.blood > 0 ) {
        ret.color = c_light_red;
    }

    // if cargo has items color is inverted
    const int cargo_part = part_with_feature( dp, VPFLAG_CARGO, true );
    if( cargo_part >= 0 && !get_items( part( cargo_part ) ).empty() ) {
        ret.has_cargo = true;
        ret.color = invert_color( ret.color );
    }

    return ret;
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
 * @param detail Whether or not to show detailed contents for fuel components.
 */
int vehicle::print_part_list( const catacurses::window &win, int y1,
                              const int max_y, int width,
                              int p, int hl /*= -1*/, bool detail, bool include_fakes ) const
{
    if( p < 0 || p >= static_cast<int>( parts.size() ) ) {
        return y1;
    }
    std::vector<int> pl = this->parts_at_relative( parts[p].mount, true, include_fakes );
    int y = y1;
    for( size_t i = 0; i < pl.size(); i++ ) {
        const vehicle_part &vp = parts[pl[i]];
        const vpart_info &vpi = vp.info();
        if( !vp.is_real_or_active_fake() ) {
            continue;
        }
        if( y >= max_y ) {
            mvwprintz( win, point( 1, y ), c_yellow, _( "More parts here…" ) );
            ++y;
            break;
        }

        std::string partname = vp.name();

        if( vp.is_fuel_store() && !vp.ammo_current().is_null() ) {
            if( detail ) {
                if( vp.ammo_current() == itype_battery ) {
                    partname += string_format( _( " (%s/%s charge)" ), vp.ammo_remaining( ),
                                               vp.ammo_capacity( ammo_battery ) );
                } else if( vp.ammo_current()->stack_size > 0 ) {
                    const itype *pt_ammo_cur = item::find_type( vp.ammo_current() );
                    auto stack = 250_ml / pt_ammo_cur->stack_size;
                    partname += string_format( _( " (%.1fL %s)" ),
                                               round_up( units::to_liter( vp.ammo_remaining( ) * stack ), 1 ),
                                               item::nname( vp.ammo_current() ) );
                }
            } else {
                partname += string_format( " (%s)", item::nname( vp.ammo_current() ) );
            }
        }

        if( vpi.has_flag( VPFLAG_CARGO ) ) {
            const vehicle_stack vs = get_items( vp );
            //~ used/total volume of a cargo vehicle part
            partname += string_format( _( " (vol: %s/%s %s)" ),
                                       format_volume( vs.stored_volume() ),
                                       format_volume( vs.max_volume() ),
                                       volume_units_abbr() );
        }

        const bool armor = vpi.has_flag( VPFLAG_ARMOR );
        std::string left_sym;
        std::string right_sym;
        if( armor ) {
            left_sym = "(";
            right_sym = ")";
        } else if( vpi.location == part_location_structure ) {
            left_sym = "[";
            right_sym = "]";
        } else {
            left_sym = "-";
            right_sym = "-";
        }
        nc_color sym_color = static_cast<int>( i ) == hl ? hilite( c_light_gray ) : c_light_gray;
        mvwprintz( win, point( 1, y ), sym_color, left_sym );
        trim_and_print( win, point( 2, y ), getmaxx( win ) - 4,
                        static_cast<int>( i ) == hl ? hilite( c_light_gray ) : c_light_gray, partname );
        wprintz( win, sym_color, right_sym );

        if( i == 0 && vpart_position( const_cast<vehicle &>( *this ), pl[i] ).is_inside() ) {
            //~ indicates that a vehicle part is inside
            mvwprintz( win, point( width - 2 - utf8_width( _( "Interior" ) ), y ), c_light_gray,
                       _( "Interior" ) );
        } else if( i == 0 ) {
            //~ indicates that a vehicle part is outside
            mvwprintz( win, point( width - 2 - utf8_width( _( "Exterior" ) ), y ), c_light_gray,
                       _( "Exterior" ) );
        }
        y++;
    }

    // print the label for this location
    const std::optional<std::string> label = vpart_position( const_cast<vehicle &>( *this ),
            p ).get_label();
    if( label && y <= max_y ) {
        mvwprintz( win, point( 1, y++ ), c_light_red, _( "Label: %s" ), label->c_str() );
    }

    return y;
}

/**
 * Prints a list of descriptions for all parts to the screen inside of a boxed window
 * @param win The window to draw in.
 * @param max_y Draw no further than this y-coordinate.
 * @param width The width of the window.
 * @param p The index of the part being examined.
 * @param start_at Which vehicle part to start printing at.
 * @param start_limit the part index beyond which the display is full
 */
void vehicle::print_vparts_descs( const catacurses::window &win, int max_y, int width, int p,
                                  int &start_at, int &start_limit ) const
{
    if( p < 0 || p >= static_cast<int>( parts.size() ) ) {
        return;
    }

    std::vector<int> pl = this->parts_at_relative( parts[p].mount, true );
    std::string msg;

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
        msg += std::string( "<color_yellow>" ) + "<  " + _( "More parts here…" ) + "</color>\n";
        lines += 1;
    }
    for( size_t i = start_at; i < pl.size(); i++ ) {
        const vehicle_part &vp = parts[ pl [ i ] ];
        std::string possible_msg;
        const nc_color name_color = vp.is_broken() ? c_dark_gray : c_light_green;
        possible_msg += colorize( vp.name(), name_color ) + "\n";
        const nc_color desc_color = vp.is_broken() ? c_dark_gray : c_light_gray;
        // -4 = -2 for left & right padding + -2 for "> "
        int new_lines = 2 + vp.info().format_description( possible_msg, desc_color, width - 4 );
        if( vp.has_flag( vp_flag::carrying_flag ) ) {
            possible_msg += _( "  Carrying a vehicle on a rack.\n" );
            new_lines += 1;
        }
        if( vp.has_flag( vp_flag::carried_flag ) ) {
            possible_msg += string_format( _( "  Part of a %s carried on a rack.\n" ),
                                           vp.carried_name() );
            new_lines += 1;
        }

        possible_msg += "</color>\n";
        if( lines + new_lines <= max_y ) {
            msg += possible_msg;
            lines += new_lines;
            start_limit = start_at;
        } else {
            msg += std::string( "<color_yellow>" ) + _( "More parts here…" ) + "  >" + "</color>\n";
            start_limit = i;
            break;
        }
    }
    werase( win );
    // -2 for left & right padding
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    fold_and_print( win, point( 1, 0 ), width - 2, c_light_gray, msg );
    wnoutrefresh( win );
}

/**
 * Returns an array of fuel types that can be printed
 * @return An array of printable fuel type ids
 */
std::vector<itype_id> vehicle::get_printable_fuel_types( map &here ) const
{
    std::set<itype_id> opts;
    for( const vpart_reference &vpr : get_all_parts() ) {
        const vehicle_part &pt = vpr.part();
        if( !pt.has_flag( vp_flag::carried_flag ) && pt.is_fuel_store() &&
            !pt.ammo_current().is_null() ) {
            opts.emplace( pt.ammo_current() );
        }
    }

    std::vector<itype_id> res( opts.begin(), opts.end() );

    std::sort( res.begin(), res.end(), [&]( const itype_id & lhs, const itype_id & rhs ) {
        return basic_consumption( here, rhs ) < basic_consumption( here, lhs );
    } );

    return res;
}

/**
 * Prints all of the fuel indicators of the vehicle
 * @param win Pointer to the window to draw in.
 * @param p location to draw at.
 * @param start_index Starting index in array of fuel gauges to start reading from
 * @param fullsize true if it's expected to print multiple rows
 * @param verbose true if there should be anything after the gauge (either the %, or number)
 * @param desc true if the name of the fuel should be at the end
 * @param isHorizontal true if the menu is not vertical
 */
void vehicle::print_fuel_indicators( map &here, const catacurses::window &win, const point &p,
                                     int start_index,
                                     bool fullsize, bool verbose, bool desc, bool isHorizontal )
{
    auto fuels = get_printable_fuel_types( here );
    if( fuels.empty() ) {
        return;
    }
    if( !fullsize ) {
        for( const int part_idx : engines ) {
            const vehicle_part &vp = parts[part_idx];
            // if only one display, print the first engine that's on and consumes power
            if( is_engine_on( vp ) && !is_perpetual_type( vp ) && !is_engine_type( vp, fuel_type_muscle ) ) {
                print_fuel_indicator( here, win, p, vp.fuel_current(), verbose, desc );
                return;
            }
        }
        // or print the first fuel if no engines
        print_fuel_indicator( here, win, p, fuels.front(), verbose, desc );
        return;
    }

    int yofs = 0;
    int max_gauge = ( isHorizontal ? 12 : 5 ) + start_index;
    int max_size = std::min( static_cast<int>( fuels.size() ), max_gauge );

    for( int i = start_index; i < max_size; i++ ) {
        const itype_id &f = fuels[i];
        print_fuel_indicator( here, win, p + point( 0, yofs ), f, fuel_used_last_turn, verbose, desc );
        yofs++;
    }

    // check if the current index is less than the max size minus 12 or 5, to indicate that there's more
    if( start_index < static_cast<int>( fuels.size() ) - ( isHorizontal ? 12 : 5 ) ) {
        mvwprintz( win, p + point( 0, yofs ), c_light_green, ">" );
        wprintz( win, c_light_gray, " for more" );
    }
}

/**
 * Prints a fuel gauge for a vehicle
 * @param win Pointer to the window to draw in.
 * @param p location to draw at.
 * @param fuel_type ID of the fuel type to draw
 * @param verbose true if there should be anything after the gauge (either the %, or number)
 * @param desc true if the name of the fuel should be at the end
 * @param fuel_usages map of fuel types to consumption for verbose
 */
void vehicle::print_fuel_indicator( map &here, const catacurses::window &win, const point &p,
                                    const itype_id &fuel_type, bool verbose, bool desc )
{
    std::map<itype_id, units::energy> fuel_usages;
    print_fuel_indicator( here, win, p, fuel_type, fuel_usages, verbose, desc );
}

void vehicle::print_fuel_indicator( map &here, const catacurses::window &win, const point &p,
                                    const itype_id &fuel_type,
                                    std::map<itype_id, units::energy> fuel_usages,
                                    bool verbose, bool desc )
{
    static constexpr std::array<char, 5> fsyms = { 'E', '\\', '|', '/', 'F' };
    nc_color col_indf1 = c_light_gray;
    int cap = fuel_capacity( here, fuel_type );
    int f_left = fuel_left( here, fuel_type );
    nc_color f_color = item::find_type( fuel_type )->color;
    // NOLINTNEXTLINE(cata-text-style): not an ellipsis
    mvwprintz( win, p, col_indf1, "E...F" );
    int amnt = cap > 0 ? f_left * 99 / cap : 0;
    int indf = ( amnt / 20 ) % 5;
    mvwprintz( win, p + point( indf, 0 ), f_color, "%c", fsyms[indf] );
    if( verbose ) {
        if( debug_mode || cap == 0 ) {
            mvwprintz( win, p + point( 6, 0 ), f_color, "%d/%d", f_left, cap );
        } else {
            mvwprintz( win, p + point( 6, 0 ), f_color, "%d", f_left * 100 / cap );
            wprintz( win, c_light_gray, "%c", 045 );
        }
    }
    if( desc ) {
        wprintz( win, c_light_gray, " - %s", item::nname( fuel_type ) );
    }
    if( verbose ) {
        auto fuel_data = fuel_usages.find( fuel_type );
        int rate = 0;
        std::string units;
        if( fuel_data != fuel_usages.end() ) {
            rate = consumption_per_hour( fuel_type, fuel_data->second );
            units = _( "mL" );
        }
        if( fuel_type == itype_battery ) {
            rate += power_to_energy_bat( net_battery_charge_rate( here,/* include_reactors = */ true ),
                                         1_hours );
            units = _( "kJ" );
        }
        if( rate != 0 && cap != 0 ) {
            int tank_use = 0;
            nc_color tank_color = c_light_green;
            std::string tank_goal = _( "full" );
            if( rate > 0 ) {
                tank_use = cap - f_left;
                if( !tank_use ) {
                    return;
                }
            } else {
                if( !f_left ) {
                    return;
                }
                tank_use = f_left;
                tank_color = c_light_red;
                tank_goal = _( "empty" );
            }

            // promote to double so esimate doesn't overflow for high fuel values
            // 3600 * tank_use overflows signed 32 bit when tank_use is over ~596523
            double turns = to_turns<double>( 60_minutes );
            time_duration estimate = time_duration::from_turns( turns * tank_use / std::abs( rate ) );

            if( debug_mode ) {
                wprintz( win, tank_color, _( ", %d %s(%4.2f%%)/hour, %s until %s" ),
                         rate, units, 100.0 * rate / cap, to_string_clipped( estimate ), tank_goal );
            } else {
                wprintz( win, tank_color, _( ", %3.1f%% / hour, %s until %s" ),
                         100.0 * rate  / cap, to_string_clipped( estimate ), tank_goal );
            }
        }
    }
}

void vehicle::print_speed_gauge( map &here, const catacurses::window &win, const point &p,
                                 int spacing ) const
{
    if( spacing < 0 ) {
        spacing = 0;
    }

    // Color is based on how much vehicle is straining beyond its safe velocity
    const float strain = this->strain( here );
    nc_color col_vel = strain <= 0 ? c_light_blue :
                       ( strain <= 0.2 ? c_yellow :
                         ( strain <= 0.4 ? c_light_red : c_red ) );
    // Get cruising (target) velocity, and current (actual) velocity
    int t_speed = static_cast<int>( convert_velocity( cruise_velocity, VU_VEHICLE ) );
    int c_speed = static_cast<int>( convert_velocity( velocity, VU_VEHICLE ) );
    auto ndigits = []( int value ) {
        return value == 0 ? 1 :
               ( value > 0 ?
                 static_cast<int>( std::log10( static_cast<double>( std::abs( value ) ) ) ) + 1 :
                 static_cast<int>( std::log10( static_cast<double>( std::abs( value ) ) ) ) + 2 );
    };
    const std::string type = get_option<std::string> ( "USE_METRIC_SPEEDS" );
    int t_offset = ndigits( t_speed );
    int c_offset = ndigits( c_speed );

    // Target cruising velocity in green
    mvwprintz( win, p, c_light_green, "%d", t_speed );
    mvwprintz( win, p + point( t_offset + spacing, 0 ), c_light_gray, "<" );
    // Current velocity in color indicating engine strain
    mvwprintz( win, p + point( t_offset + 1 + 2 * spacing, 0 ), col_vel, "%d", c_speed );
    // Units of speed (mph, km/h, t/t)
    mvwprintz( win, p + point( t_offset  + c_offset + 1 + 3 * spacing, 0 ), c_light_gray, type );
}
