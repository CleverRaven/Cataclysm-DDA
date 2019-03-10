#include "vehicle.h" // IWYU pragma: associated
#include "vpart_position.h" // IWYU pragma: associated
#include "vpart_range.h" // IWYU pragma: associated
#include "vpart_reference.h" // IWYU pragma: associated

#include <algorithm>
#include <array>
#include <cassert>
#include <complex>
#include <cmath>
#include <cstdlib>
#include <numeric>
#include <queue>
#include <set>
#include <sstream>
#include <unordered_map>

#include "ammo.h"
#include "cata_utility.h"
#include "coordinate_conversions.h"
#include "debug.h"
#include "game.h"
#include "item.h"
#include "item_group.h"
#include "itype.h"
#include "json.h"
#include "map.h"
#include "map_iterator.h"
#include "mapbuffer.h"
#include "mapdata.h"
#include "messages.h"
#include "output.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "sounds.h"
#include "string_formatter.h"
#include "submap.h"
#include "translations.h"
#include "veh_interact.h"
#include "veh_type.h"
#include "vehicle_selector.h"
#include "weather.h"

/*
 * Speed up all those if ( blarg == "structure" ) statements that are used everywhere;
 *   assemble "structure" once here instead of repeatedly later.
 */
static const itype_id fuel_type_battery( "battery" );
static const itype_id fuel_type_muscle( "muscle" );
static const itype_id fuel_type_plutonium_cell( "plut_cell" );
static const std::string part_location_structure( "structure" );
static const std::string part_location_center( "center" );
static const std::string part_location_onroof( "on_roof" );

static const fault_id fault_belt( "fault_engine_belt_drive" );
static const fault_id fault_immobiliser( "fault_engine_immobiliser" );
static const fault_id fault_filter_air( "fault_engine_filter_air" );
static const fault_id fault_filter_fuel( "fault_engine_filter_fuel" );

const skill_id skill_mechanics( "mechanics" );

// 1 kJ per battery charge
const int bat_energy_j = 1000;

inline int modulo( int v, int m );
//
// Point dxs for the adjacent cardinal tiles.
point vehicles::cardinal_d[5] = { point( -1, 0 ), point( 1, 0 ), point( 0, -1 ), point( 0, 1 ), point_zero };

// Vehicle stack methods.
std::list<item>::iterator vehicle_stack::erase( std::list<item>::iterator it )
{
    return myorigin->remove_item( part_num, it );
}

void vehicle_stack::push_back( const item &newitem )
{
    myorigin->add_item( part_num, newitem );
}

void vehicle_stack::insert_at( std::list<item>::iterator index,
                               const item &newitem )
{
    myorigin->add_item_at( part_num, index, newitem );
}

units::volume vehicle_stack::max_volume() const
{
    if( myorigin->part_flag( part_num, "CARGO" ) && myorigin->parts[part_num].is_available() ) {
        // Set max volume for vehicle cargo to prevent integer overflow
        return std::min( myorigin->parts[part_num].info().size, 10000000_ml );
    }
    return 0_ml;
}

// Vehicle class methods.

vehicle::vehicle( const vproto_id &type_id, int init_veh_fuel,
                  int init_veh_status ): type( type_id )
{
    turn_dir = 0;
    face.init( 0 );
    move.init( 0 );
    of_turn_carry = 0;

    if( !type.str().empty() && type.is_valid() ) {
        const vehicle_prototype &proto = type.obj();
        // Copy the already made vehicle. The blueprint is created when the json data is loaded
        // and is guaranteed to be valid (has valid parts etc.).
        *this = *proto.blueprint;
        init_state( init_veh_fuel, init_veh_status );
    }
    precalc_mounts( 0, pivot_rotation[0], pivot_anchor[0] );
    refresh();
}

vehicle::vehicle() : vehicle( vproto_id() )
{
    smx = 0;
    smy = 0;
    smz = 0;
}

vehicle::~vehicle() = default;

bool vehicle::player_in_control( const player &p ) const
{
    // Debug switch to prevent vehicles from skidding
    // without having to place the player in them.
    if( tags.count( "IN_CONTROL_OVERRIDE" ) ) {
        return true;
    }

    const optional_vpart_position vp = g->m.veh_at( p.pos() );
    if( vp && &vp->vehicle() == this &&
        part_with_feature( vp->part_index(), VPFLAG_CONTROLS, false ) >= 0 && p.controlling_vehicle ) {
        return true;
    }

    return remote_controlled( p );
}

bool vehicle::remote_controlled( const player &p ) const
{
    vehicle *veh = g->remoteveh();
    if( veh != this ) {
        return false;
    }

    for( const vpart_reference &vp : get_avail_parts( "REMOTE_CONTROLS" ) ) {
        if( rl_dist( p.pos(), vp.pos() ) <= 40 ) {
            return true;
        }
    }

    add_msg( m_bad, _( "Lost connection with the vehicle due to distance!" ) );
    g->setremoteveh( nullptr );
    return false;
}

/** Checks all parts to see if frames are missing (as they might be when
 * loading from a game saved before the vehicle construction rules overhaul). */
void vehicle::add_missing_frames()
{
    static const vpart_id frame_id( "frame_vertical" );
    //No need to check the same spot more than once
    std::set<point> locations_checked;
    for( auto &i : parts ) {
        if( locations_checked.count( i.mount ) != 0 ) {
            continue;
        }
        locations_checked.insert( i.mount );

        bool found = false;
        for( auto &elem : parts_at_relative( i.mount, false ) ) {
            if( part_info( elem ).location == part_location_structure ) {
                found = true;
                break;
            }
        }
        if( !found ) {
            // Install missing frame
            parts.emplace_back( frame_id, i.mount, item( frame_id->item ) );
        }
    }
}

// Called when loading a vehicle that predates steerable wheels.
// Tries to convert some wheels to steerable versions on the front axle.
void vehicle::add_steerable_wheels()
{
    int axle = INT_MIN;
    std::vector< std::pair<int, vpart_id> > wheels;

    // Find wheels that have steerable versions.
    // Convert the wheel(s) with the largest x value.
    for( const vpart_reference &vp : get_all_parts() ) {
        if( vp.has_feature( "STEERABLE" ) || vp.has_feature( "TRACKED" ) ) {
            // Has a wheel that is inherently steerable
            // (e.g. unicycle, casters), this vehicle doesn't
            // need conversion.
            return;
        }

        if( vp.mount().x < axle ) {
            // there is another axle in front of this
            continue;
        }

        if( vp.has_feature( VPFLAG_WHEEL ) ) {
            vpart_id steerable_id( vp.info().get_id().str() + "_steerable" );
            if( steerable_id.is_valid() ) {
                // We can convert this.
                if( vp.mount().x != axle ) {
                    // Found a new axle further forward than the
                    // existing one.
                    wheels.clear();
                    axle = vp.mount().x;
                }

                wheels.push_back( std::make_pair( static_cast<int>( vp.part_index() ), steerable_id ) );
            }
        }
    }

    // Now convert the wheels to their new types.
    for( auto &wheel : wheels ) {
        parts[ wheel.first ].id = wheel.second;
    }
}

void vehicle::init_state( int init_veh_fuel, int init_veh_status )
{
    // vehicle parts excluding engines are by default turned off
    for( auto &pt : parts ) {
        pt.enabled = pt.base.is_engine();
    }

    bool destroySeats = false;
    bool destroyControls = false;
    bool destroyTank = false;
    bool destroyEngine = false;
    bool destroyTires = false;
    bool blood_covered = false;
    bool blood_inside = false;
    bool has_no_key = false;
    bool destroyAlarm = false;

    // More realistically it should be -5 days old
    last_update = 0;

    // veh_fuel_multiplier is percentage of fuel
    // 0 is empty, 100 is full tank, -1 is random 7% to 35%
    int veh_fuel_mult = init_veh_fuel;
    if( init_veh_fuel == - 1 ) {
        veh_fuel_mult = rng( 1, 7 );
    }
    if( init_veh_fuel > 100 ) {
        veh_fuel_mult = 100;
    }

    // veh_status is initial vehicle damage
    // -1 = light damage (DEFAULT)
    //  0 = undamaged
    //  1 = disabled, destroyed tires OR engine
    int veh_status = -1;
    if( init_veh_status == 0 ) {
        veh_status = 0;
    }
    if( init_veh_status == 1 ) {
        int rand = rng( 1, 100 );
        veh_status = 1;

        if( rand <= 5 ) {          //  seats are destroyed 5%
            destroySeats = true;
        } else if( rand <= 15 ) {  // controls are destroyed 10%
            destroyControls = true;
            veh_fuel_mult += rng( 0, 7 );   // add 0-7% more fuel if controls are destroyed
        } else if( rand <= 23 ) {  // battery, minireactor or gasoline tank are destroyed 8%
            destroyTank = true;
        } else if( rand <= 29 ) {  // engine are destroyed 6%
            destroyEngine = true;
            veh_fuel_mult += rng( 3, 12 );  // add 3-12% more fuel if engine is destroyed
        } else if( rand <= 66 ) {  // tires are destroyed 37%
            destroyTires = true;
            veh_fuel_mult += rng( 0, 18 );  // add 0-18% more fuel if tires are destroyed
        } else {                   // vehicle locked 34%
            has_no_key = true;
        }
    }
    // if locked, 16% chance something damaged
    if( one_in( 6 ) && has_no_key ) {
        if( one_in( 3 ) ) {
            destroyTank = true;
        } else if( one_in( 2 ) ) {
            destroyEngine = true;
        } else {
            destroyTires = true;
        }
    } else if( !one_in( 3 ) ) {
        //most cars should have a destroyed alarm
        destroyAlarm = true;
    }

    //Provide some variety to non-mint vehicles
    if( veh_status != 0 ) {
        //Leave engine running in some vehicles, if the engine has not been destroyed
        if( veh_fuel_mult > 0 && !empty( get_avail_parts( "ENGINE" ) ) &&
            one_in( 8 ) && !destroyEngine && !has_no_key && has_engine_type_not( fuel_type_muscle, true ) ) {
            engine_on = true;
        }

        auto light_head  = one_in( 20 );
        auto light_whead  = one_in( 20 ); // wide-angle headlight
        auto light_dome  = one_in( 16 );
        auto light_aisle = one_in( 8 );
        auto light_hoverh = one_in( 4 ); // half circle overhead light
        auto light_overh = one_in( 4 );
        auto light_atom  = one_in( 2 );
        for( auto &pt : parts ) {
            if( pt.has_flag( VPFLAG_CONE_LIGHT ) ) {
                pt.enabled = light_head;
            } else if( pt.has_flag( VPFLAG_WIDE_CONE_LIGHT ) ) {
                pt.enabled = light_whead;
            } else if( pt.has_flag( VPFLAG_DOME_LIGHT ) ) {
                pt.enabled = light_dome;
            } else if( pt.has_flag( VPFLAG_AISLE_LIGHT ) ) {
                pt.enabled = light_aisle;
            } else if( pt.has_flag( VPFLAG_HALF_CIRCLE_LIGHT ) ) {
                pt.enabled = light_hoverh;
            } else if( pt.has_flag( VPFLAG_CIRCLE_LIGHT ) ) {
                pt.enabled = light_overh;
            } else if( pt.has_flag( VPFLAG_ATOMIC_LIGHT ) ) {
                pt.enabled = light_atom;
            }
        }

        if( one_in( 10 ) ) {
            blood_covered = true;
        }

        if( one_in( 8 ) ) {
            blood_inside = true;
        }

        for( const vpart_reference &vp : get_parts_including_carried( "FRIDGE" ) ) {
            vp.part().enabled = true;
        }

        for( const vpart_reference &vp : get_parts_including_carried( "FREEZER" ) ) {
            vp.part().enabled = true;
        }

        for( const vpart_reference &vp : get_parts_including_carried( "WATER_PURIFIER" ) ) {
            vp.part().enabled = true;
        }
    }

    cata::optional<point> blood_inside_pos;
    for( const vpart_reference &vp : get_all_parts() ) {
        const size_t p = vp.part_index();
        vehicle_part &pt = vp.part();

        if( vp.has_feature( VPFLAG_REACTOR ) ) {
            // De-hardcoded reactors. Should always start active
            pt.enabled = true;
        }

        if( pt.is_battery() ) {
            if( veh_fuel_mult == 100 ) { // Mint condition vehicle
                pt.ammo_set( "battery", pt.ammo_capacity() );
            } else if( one_in( 2 ) && veh_fuel_mult > 0 ) { // Randomize battery ammo a bit
                pt.ammo_set( "battery", pt.ammo_capacity() * ( veh_fuel_mult + rng( 0, 10 ) ) / 100 );
            } else if( one_in( 2 ) && veh_fuel_mult > 0 ) {
                pt.ammo_set( "battery", pt.ammo_capacity() * ( veh_fuel_mult - rng( 0, 10 ) ) / 100 );
            } else {
                pt.ammo_set( "battery", pt.ammo_capacity() * veh_fuel_mult / 100 );
            }
        }

        if( pt.is_tank() && type->parts[p].fuel != "null" ) {
            int qty = pt.ammo_capacity() * veh_fuel_mult / 100;
            qty *= std::max( item::find_type( type->parts[p].fuel )->stack_size, 1 );
            qty /= to_milliliter( units::legacy_volume_factor );
            pt.ammo_set( type->parts[ p ].fuel, qty );
        } else if( pt.is_fuel_store() && type->parts[p].fuel != "null" ) {
            int qty = pt.ammo_capacity() * veh_fuel_mult / 100;
            pt.ammo_set( type->parts[ p ].fuel, qty );
        }

        if( vp.has_feature( "OPENABLE" ) ) { // doors are closed
            if( !pt.open && one_in( 4 ) ) {
                open( p );
            }
        }
        if( vp.has_feature( "BOARDABLE" ) ) {   // no passengers
            pt.remove_flag( vehicle_part::passenger_flag );
        }

        // initial vehicle damage
        if( veh_status == 0 ) {
            // Completely mint condition vehicle
            set_hp( pt, vp.info().durability );
        } else {
            //a bit of initial damage :)
            //clamp 4d8 to the range of [8,20]. 8=broken, 20=undamaged.
            int broken = 8;
            int unhurt = 20;
            int roll = dice( 4, 8 );
            if( roll < unhurt ) {
                if( roll <= broken ) {
                    set_hp( pt, 0 );
                    pt.ammo_unset(); //empty broken batteries and fuel tanks
                } else {
                    set_hp( pt, ( roll - broken ) / double( unhurt - broken ) * vp.info().durability );
                }
            } else {
                set_hp( pt, vp.info().durability );
            }

            if( vp.has_feature( VPFLAG_ENGINE ) ) {
                // If possible set an engine fault rather than destroying the engine outright
                if( destroyEngine && pt.faults_potential().empty() ) {
                    set_hp( pt, 0 );
                } else if( destroyEngine || one_in( 3 ) ) {
                    do {
                        pt.fault_set( random_entry( pt.faults_potential() ) );
                    } while( one_in( 3 ) );
                }

            } else if( ( destroySeats && ( vp.has_feature( "SEAT" ) || vp.has_feature( "SEATBELT" ) ) ) ||
                       ( destroyControls && ( vp.has_feature( "CONTROLS" ) || vp.has_feature( "SECURITY" ) ) ) ||
                       ( destroyAlarm && vp.has_feature( "SECURITY" ) ) ) {
                set_hp( pt, 0 );
            }

            // Fuel tanks should be emptied as well
            if( destroyTank && pt.is_fuel_store() ) {
                set_hp( pt, 0 );
                pt.ammo_unset();
            }

            //Solar panels have 25% of being destroyed
            if( vp.has_feature( "SOLAR_PANEL" ) && one_in( 4 ) ) {
                set_hp( pt, 0 );
            }

            /* Bloodsplatter the front-end parts. Assume anything with x > 0 is
            * the "front" of the vehicle (since the driver's seat is at (0, 0).
            * We'll be generous with the blood, since some may disappear before
            * the player gets a chance to see the vehicle. */
            if( blood_covered && vp.mount().x > 0 ) {
                if( one_in( 3 ) ) {
                    //Loads of blood. (200 = completely red vehicle part)
                    pt.blood = rng( 200, 600 );
                } else {
                    //Some blood
                    pt.blood = rng( 50, 200 );
                }
            }

            if( blood_inside ) {
                // blood is splattered around (blood_inside_pos),
                // coordinates relative to mount point; the center is always a seat
                if( blood_inside_pos ) {
                    const int distSq = std::pow( blood_inside_pos->x - vp.mount().x, 2 ) +
                                       std::pow( blood_inside_pos->y - vp.mount().y, 2 );
                    if( distSq <= 1 ) {
                        pt.blood = rng( 200, 400 ) - distSq * 100;
                    }
                } else if( vp.has_feature( "SEAT" ) ) {
                    // Set the center of the bloody mess inside
                    blood_inside_pos.emplace( vp.mount() );
                }
            }
        }
        //sets the vehicle to locked, if there is no key and an alarm part exists
        if( vp.has_feature( "SECURITY" ) && has_no_key && pt.is_available() ) {
            is_locked = true;

            if( one_in( 2 ) ) {
                // if vehicle has immobilizer 50% chance to add additional fault
                pt.fault_set( fault_immobiliser );
            }
        }
    }
    // destroy tires until the vehicle is not drivable
    if( destroyTires && !wheelcache.empty() ) {
        int tries = 0;
        while( valid_wheel_config() && tries < 100 ) {
            // wheel config is still valid, destroy the tire.
            set_hp( parts[random_entry( wheelcache )], 0 );
            tries++;
        }
    }

    invalidate_mass();
}

/**
 * Smashes up a vehicle that has already been placed; used for generating
 * very damaged vehicles. Additionally, any spot where two vehicles overlapped
 * (ie, any spot with multiple frames) will be completely destroyed, as that
 * was the collision point.
 */
void vehicle::smash( float hp_percent_loss_min, float hp_percent_loss_max,
                     float percent_of_parts_to_affect, point damage_origin, float damage_size )
{
    for( auto &part : parts ) {
        //Skip any parts already mashed up or removed.
        if( part.is_broken() || part.removed ) {
            continue;
        }

        std::vector<int> parts_in_square = parts_at_relative( part.mount, true );
        int structures_found = 0;
        for( auto &square_part_index : parts_in_square ) {
            if( part_info( square_part_index ).location == part_location_structure ) {
                structures_found++;
            }
        }

        if( structures_found > 1 ) {
            //Destroy everything in the square
            for( int idx : parts_in_square ) {
                mod_hp( parts[ idx ], 0 - parts[ idx ].hp(), DT_BASH );
                parts[ idx ].ammo_unset();
            }
            continue;
        }

        int roll = dice( 1, 1000 );
        int pct_af = ( percent_of_parts_to_affect * 1000.0f );
        if( roll < pct_af ) {
            float dist = 1.0f - trig_dist( damage_origin.x, damage_origin.y, part.precalc[0].x,
                                           part.precalc[0].y ) / damage_size;
            dist = clamp( dist, 0.0f, 1.0f );
            if( damage_size == 0 ) {
                dist = 1.0f;
            }
            //Everywhere else, drop by 10-120% of max HP (anything over 100 = broken)
            if( mod_hp( part, 0 - ( rng_float( hp_percent_loss_min * dist,
                                               hp_percent_loss_max * dist ) * part.info().durability ), DT_BASH ) ) {
                part.ammo_unset();
            }
        }
    }
    // clear out any duplicated locations
    for( int p = static_cast<int>( parts.size() ) - 1; p >= 0; p-- ) {
        vehicle_part &part = parts[ p ];
        if( part.removed ) {
            continue;
        }
        std::vector<int> parts_here = parts_at_relative( part.mount, true );
        for( int other_i = static_cast<int>( parts_here.size() ) - 1; other_i >= 0; other_i -- ) {
            int other_p = parts_here[ other_i ];
            if( p == other_p ) {
                continue;
            }
            if( ( part_info( p ).location.empty() &&
                  part_info( p ).get_id() == part_info( other_p ).get_id() ) ||
                ( part_info( p ).location == part_info( other_p ).location ) ) {
                remove_part( other_p );
            }
        }
    }
}

int vehicle::lift_strength() const
{
    units::mass mass = total_mass();
    return std::max( mass / 10000_gram, 1 );
}

void vehicle::toggle_specific_engine( int e, bool on )
{
    toggle_specific_part( engines[e], on );
}
void vehicle::toggle_specific_part( int p, bool on )
{
    parts[p].enabled = on;
}
bool vehicle::is_engine_type_on( int e, const itype_id &ft ) const
{
    return is_engine_on( e ) && is_engine_type( e, ft );
}

bool vehicle::has_engine_type( const itype_id &ft, const bool enabled ) const
{
    for( size_t e = 0; e < engines.size(); ++e ) {
        if( is_engine_type( e, ft ) && ( !enabled || is_engine_on( e ) ) ) {
            return true;
        }
    }
    return false;
}
bool vehicle::has_engine_type_not( const itype_id &ft, const bool enabled ) const
{
    for( size_t e = 0; e < engines.size(); ++e ) {
        if( !is_engine_type( e, ft ) && ( !enabled || is_engine_on( e ) ) ) {
            return true;
        }
    }
    return false;
}

bool vehicle::has_engine_conflict( const vpart_info *possible_conflict,
                                   std::string &conflict_type ) const
{
    std::vector<std::string> new_excludes = possible_conflict->engine_excludes();
    // skip expensive string comparisons if there are no exclusions
    if( new_excludes.empty() ) {
        return false;
    }

    bool has_conflict = false;

    for( int engine : engines ) {
        std::vector<std::string> install_excludes = part_info( engine ).engine_excludes();
        std::vector<std::string> conflicts;
        std::set_intersection( new_excludes.begin(), new_excludes.end(), install_excludes.begin(),
                               install_excludes.end(), back_inserter( conflicts ) );
        if( !conflicts.empty() ) {
            has_conflict = true;
            conflict_type = conflicts.front();
            break;
        }
    }
    return has_conflict;
}

bool vehicle::is_engine_type( const int e, const itype_id  &ft ) const
{
    return parts[engines[e]].ammo_current() == "null" ? parts[engines[e]].fuel_current() == ft :
           parts[engines[e]].ammo_current() == ft;
}

bool vehicle::is_perpetual_type( const int e ) const
{
    const itype_id  &ft = part_info( engines[e] ).fuel_type;
    return item( ft ).has_flag( "PERPETUAL" );
}

bool vehicle::is_engine_on( const int e ) const
{
    return parts[ engines[ e ] ].is_available() && is_part_on( engines[ e ] );
}

bool vehicle::is_part_on( const int p ) const
{
    return parts[p].enabled;
}

bool vehicle::is_alternator_on( const int a ) const
{
    auto alt = parts[ alternators [ a ] ];
    if( alt.is_unavailable() ) {
        return false;
    }

    return std::any_of( engines.begin(), engines.end(), [this, &alt]( int idx ) {
        auto &eng = parts [ idx ];
        return eng.enabled && eng.is_available() && eng.mount == alt.mount &&
               !eng.faults().count( fault_belt );
    } );
}

bool vehicle::has_security_working() const
{
    bool found_security = false;
    for( int s : speciality ) {
        if( part_flag( s, "SECURITY" ) && parts[ s ].is_available() ) {
            found_security = true;
            break;
        }
    }
    return found_security;
}

void vehicle::backfire( const int e ) const
{
    const int power = part_vpower_w( engines[e], true );
    const tripoint pos = global_part_pos3( engines[e] );
    //~ backfire sound
    sounds::ambient_sound( pos, 40 + power / 10000, sounds::sound_t::movement,
                           string_format( _( "a loud BANG! from the %s" ),
                                          parts[ engines[ e ] ].name() ) );
}

const vpart_info &vehicle::part_info( int index, bool include_removed ) const
{
    if( index < static_cast<int>( parts.size() ) ) {
        if( !parts[index].removed || include_removed ) {
            return parts[index].info();
        }
    }
    return vpart_id::NULL_ID().obj();
}

// engines & alternators all have power.
// engines provide, whilst alternators consume.
int vehicle::part_vpower_w( const int index, const bool at_full_hp ) const
{
    const vehicle_part &vp = parts[ index ];

    int pwr = vp.info().power;
    if( part_flag( index, VPFLAG_ENGINE ) ) {
        if( pwr == 0 ) {
            pwr = vhp_to_watts( vp.base.engine_displacement() );
        }
        ///\EFFECT_STR increases power produced for MUSCLE_* vehicles
        pwr += ( g->u.str_cur - 8 ) * part_info( index ).engine_muscle_power_factor();
    }

    if( pwr < 0 ) {
        return pwr; // Consumers always draw full power, even if broken
    }
    if( at_full_hp ) {
        return pwr; // Assume full hp
    }
    // Damaged engines give less power, but some engines handle it better
    double health = parts[index].health_percent();
    // dpf is 0 for engines that scale power linearly with damage and
    // provides a floor otherwise
    float dpf = part_info( index ).engine_damaged_power_factor();
    double effective_percent = dpf + ( ( 1 - dpf ) * health );
    return static_cast<int>( pwr * effective_percent );
}

// alternators, solar panels, reactors, and accessories all have epower.
// alternators, solar panels, and reactors provide, whilst accessories consume.
// for motor consumption see @ref vpart_info::energy_consumption instead
int vehicle::part_epower_w( const int index ) const
{
    int e = part_info( index ).epower;
    if( e < 0 ) {
        return e; // Consumers always draw full power, even if broken
    }
    return e * parts[ index ].health_percent();
}

int vehicle::power_to_energy_bat( const int power_w, const int t_seconds ) const
{
    // Integrate constant epower (watts) over time to get units of battery energy
    int energy_j = power_w * t_seconds;
    int energy_bat = energy_j / bat_energy_j;
    int sign = power_w >= 0 ? 1 : -1;
    // energy_bat remainder results in chance at additional charge/discharge
    energy_bat += x_in_y( abs( energy_j % bat_energy_j ), bat_energy_j ) ? sign : 0;
    return energy_bat;
}

int vehicle::vhp_to_watts( const int power_vhp )
{
    // Convert vhp units (0.5 HP ) to watts
    // Used primarily for calculating battery charge/discharge
    // TODO: convert batteries to use energy units based on watts (watt-ticks?)
    constexpr int conversion_factor = 373; // 373 watts == 1 power_vhp == 0.5 HP
    return power_vhp * conversion_factor;
}

bool vehicle::has_structural_part( const point &dp ) const
{
    for( const int elem : parts_at_relative( dp, false ) ) {
        if( part_info( elem ).location == part_location_structure &&
            !part_info( elem ).has_flag( "PROTRUSION" ) ) {
            return true;
        }
    }
    return false;
}

/**
 * Returns whether or not the vehicle has a structural part queued for removal,
 * @return true if a structural is queue for removal, false if not.
 * */
bool vehicle::is_structural_part_removed() const
{
    for( const vpart_reference &vp : get_all_parts() ) {
        if( vp.part().removed && vp.info().location == part_location_structure ) {
            return true;
        }
    }
    return false;
}

/**
 * Returns whether or not the vehicle part with the given id can be mounted in
 * the specified square.
 * @param dp The local coordinate to mount in.
 * @param id The id of the part to install.
 * @return true if the part can be mounted, false if not.
 */
bool vehicle::can_mount( const point &dp, const vpart_id &id ) const
{
    //The part has to actually exist.
    if( !id.is_valid() ) {
        return false;
    }

    //It also has to be a real part, not the null part
    const vpart_info &part = id.obj();
    if( part.has_flag( "NOINSTALL" ) ) {
        return false;
    }

    const std::vector<int> parts_in_square = parts_at_relative( dp, false );

    //First part in an empty square MUST be a structural part
    if( parts_in_square.empty() && part.location != part_location_structure ) {
        return false;
    }

    //No other part can be placed on a protrusion
    if( !parts_in_square.empty() && part_info( parts_in_square[0] ).has_flag( "PROTRUSION" ) ) {
        return false;
    }

    //No part type can stack with itself, or any other part in the same slot
    for( const auto &elem : parts_in_square ) {
        const vpart_info &other_part = parts[elem].info();

        //Parts with no location can stack with each other (but not themselves)
        if( part.get_id() == other_part.get_id() ||
            ( !part.location.empty() && part.location == other_part.location ) ) {
            return false;
        }
        // Until we have an interface for handling multiple components with CARGO space,
        // exclude them from being mounted in the same tile.
        if( part.has_flag( "CARGO" ) && other_part.has_flag( "CARGO" ) ) {
            return false;
        }

    }

    // All parts after the first must be installed on or next to an existing part
    // the exception is when a single tile only structural object is being repaired
    if( !parts.empty() ) {
        if( !is_structural_part_removed() &&
            !has_structural_part( dp ) &&
            !has_structural_part( dp + point( +1,  0 ) ) &&
            !has_structural_part( dp + point( 0, +1 ) ) &&
            !has_structural_part( dp + point( -1,  0 ) ) &&
            !has_structural_part( dp + point( 0, -1 ) ) ) {
            return false;
        }
    }

    // only one exclusive engine allowed
    std::string empty;
    if( has_engine_conflict( &part, empty ) ) {
        return false;
    }

    // Alternators must be installed on a gas engine
    if( part.has_flag( VPFLAG_ALTERNATOR ) ) {
        bool anchor_found = false;
        for( const auto &elem : parts_in_square ) {
            if( part_info( elem ).has_flag( "E_ALTERNATOR" ) ) {
                anchor_found = true;
            }
        }
        if( !anchor_found ) {
            return false;
        }
    }

    //Seatbelts must be installed on a seat
    if( part.has_flag( "SEATBELT" ) ) {
        bool anchor_found = false;
        for( const auto &elem : parts_in_square ) {
            if( part_info( elem ).has_flag( "BELTABLE" ) ) {
                anchor_found = true;
            }
        }
        if( !anchor_found ) {
            return false;
        }
    }

    //Internal must be installed into a cargo area.
    if( part.has_flag( "INTERNAL" ) ) {
        bool anchor_found = false;
        for( const auto &elem : parts_in_square ) {
            if( part_info( elem ).has_flag( "CARGO" ) ) {
                anchor_found = true;
            }
        }
        if( !anchor_found ) {
            return false;
        }
    }

    // curtains must be installed on (reinforced)windshields
    // TODO: do this automatically using "location":"on_mountpoint"
    if( part.has_flag( "WINDOW_CURTAIN" ) ) {
        bool anchor_found = false;
        for( const auto &elem : parts_in_square ) {
            if( part_info( elem ).has_flag( "WINDOW" ) ) {
                anchor_found = true;
            }
        }
        if( !anchor_found ) {
            return false;
        }
    }

    // Security system must be installed on controls
    if( part.has_flag( "ON_CONTROLS" ) ) {
        bool anchor_found = false;
        for( auto it : parts_in_square ) {
            if( part_info( it ).has_flag( "CONTROLS" ) ) {
                anchor_found = true;
            }
        }
        if( !anchor_found ) {
            return false;
        }
    }

    // Cargo locks must go on lockable cargo containers
    // TODO: do this automatically using "location":"on_mountpoint"
    if( part.has_flag( "CARGO_LOCKING" ) ) {
        bool anchor_found = false;
        for( auto it : parts_in_square ) {
            if( part_info( it ).has_flag( "LOCKABLE_CARGO" ) ) {
                anchor_found = true;
            }
        }
        if( !anchor_found ) {
            return false;
        }
    }

    //Swappable storage battery must be installed on a BATTERY_MOUNT
    if( part.has_flag( "NEEDS_BATTERY_MOUNT" ) ) {
        bool anchor_found = false;
        for( const auto &elem : parts_in_square ) {
            if( part_info( elem ).has_flag( "BATTERY_MOUNT" ) ) {
                anchor_found = true;
            }
        }
        if( !anchor_found ) {
            return false;
        }
    }

    //Door motors need OPENABLE
    if( part.has_flag( "DOOR_MOTOR" ) ) {
        bool anchor_found = false;
        for( const auto &elem : parts_in_square ) {
            if( part_info( elem ).has_flag( "OPENABLE" ) ) {
                anchor_found = true;
            }
        }
        if( !anchor_found ) {
            return false;
        }
    }

    //Mirrors cannot be mounted on OPAQUE parts
    if( part.has_flag( "VISION" ) && !part.has_flag( "CAMERA" ) ) {
        for( const auto &elem : parts_in_square ) {
            if( part_info( elem ).has_flag( "OPAQUE" ) ) {
                return false;
            }
        }
    }
    //and vice versa
    if( part.has_flag( "OPAQUE" ) ) {
        for( const auto &elem : parts_in_square ) {
            if( part_info( elem ).has_flag( "VISION" ) &&
                !part_info( elem ).has_flag( "CAMERA" ) ) {
                return false;
            }
        }
    }

    //Turrets must be installed on a turret mount
    if( part.has_flag( "TURRET" ) ) {
        bool anchor_found = false;
        for( const auto &elem : parts_in_square ) {
            if( part_info( elem ).has_flag( "TURRET_MOUNT" ) ) {
                anchor_found = true;
                break;
            }
        }
        if( !anchor_found ) {
            return false;
        }
    }

    //Turret mounts must NOT be installed on other (moded) turret mounts
    if( part.has_flag( "TURRET_MOUNT" ) ) {
        for( const auto &elem : parts_in_square ) {
            if( part_info( elem ).has_flag( "TURRET_MOUNT" ) ) {
                return false;
            }
        }
    }

    //Anything not explicitly denied is permitted
    return true;
}

bool vehicle::can_unmount( const int p ) const
{
    std::string no_reason;
    return can_unmount( p, no_reason );
}

bool vehicle::can_unmount( const int p, std::string &reason ) const
{
    if( p < 0 || p > static_cast<int>( parts.size() ) ) {
        return false;
    }

    // Can't remove an engine if there's still an alternator there
    if( part_flag( p, VPFLAG_ENGINE ) && part_with_feature( p, VPFLAG_ALTERNATOR, true ) >= 0 ) {
        reason = _( "Remove attached alternator first." );
        return false;
    }

    //Can't remove a seat if there's still a seatbelt there
    if( part_flag( p, "BELTABLE" ) && part_with_feature( p, "SEATBELT", true ) >= 0 ) {
        reason = _( "Remove attached seatbelt first." );
        return false;
    }

    // Can't remove a window with curtains still on it
    if( part_flag( p, "WINDOW" ) && part_with_feature( p, "CURTAIN", true ) >= 0 ) {
        reason = _( "Remove attached curtains first." );
        return false;
    }

    //Can't remove controls if there's something attached
    if( part_flag( p, "CONTROLS" ) && part_with_feature( p, "ON_CONTROLS", true ) >= 0 ) {
        reason = _( "Remove attached part first." );
        return false;
    }

    //Can't remove a battery mount if there's still a battery there
    if( part_flag( p, "BATTERY_MOUNT" ) && part_with_feature( p, "NEEDS_BATTERY_MOUNT", true ) >= 0 ) {
        reason = _( "Remove battery from mount first." );
        return false;
    }

    //Can't remove a turret mount if there's still a turret there
    if( part_flag( p, "TURRET_MOUNT" ) && part_with_feature( p, "TURRET", true ) >= 0 ) {
        reason = _( "Remove attached mounted weapon first." );
        return false;
    }

    //Can't remove an animal part if the animal is still contained
    if( parts[p].has_flag( vehicle_part::animal_flag ) ) {
        reason = _( "Remove carried animal first." );
        return false;
    }

    //Structural parts have extra requirements
    if( part_info( p ).location == part_location_structure ) {

        std::vector<int> parts_in_square = parts_at_relative( parts[p].mount, false );
        /* To remove a structural part, there can be only structural parts left
         * in that square (might be more than one in the case of wreckage) */
        for( auto &elem : parts_in_square ) {
            if( part_info( elem ).location != part_location_structure ) {
                reason = _( "Remove all other attached parts first." );
                return false;
            }
        }

        //If it's the last part in the square...
        if( parts_in_square.size() == 1 ) {

            /* This is the tricky part: We can't remove a part that would cause
             * the vehicle to 'break into two' (like removing the middle section
             * of a quad bike, for instance). This basically requires doing some
             * breadth-first searches to ensure previously connected parts are
             * still connected. */

            //First, find all the squares connected to the one we're removing
            std::vector<vehicle_part> connected_parts;

            for( int i = 0; i < 4; i++ ) {
                const point next = parts[p].mount + point( i < 2 ? ( i == 0 ? -1 : 1 ) : 0,
                                   i < 2 ? 0 : ( i == 2 ? -1 : 1 ) );
                std::vector<int> parts_over_there = parts_at_relative( next, false );
                //Ignore empty squares
                if( !parts_over_there.empty() ) {
                    //Just need one part from the square to track the x/y
                    connected_parts.push_back( parts[parts_over_there[0]] );
                }
            }

            /* If size = 0, it's the last part of the whole vehicle, so we're OK
             * If size = 1, it's one protruding part (ie, bicycle wheel), so OK
             * Otherwise, it gets complicated... */
            if( connected_parts.size() > 1 ) {

                /* We'll take connected_parts[0] to be the target part.
                 * Every other part must have some path (that doesn't involve
                 * the part about to be removed) to the target part, in order
                 * for the part to be legally removable. */
                for( const auto &next_part : connected_parts ) {
                    if( !is_connected( connected_parts[0], next_part, parts[p] ) ) {
                        //Removing that part would break the vehicle in two
                        reason = _( "Removing this part would split the vehicle." );
                        return false;
                    }
                }

            }

        }
    }
    //Anything not explicitly denied is permitted
    return true;
}

/**
 * Performs a breadth-first search from one part to another, to see if a path
 * exists between the two without going through the excluded part. Used to see
 * if a part can be legally removed.
 * @param to The part to reach.
 * @param from The part to start the search from.
 * @param excluded_part The part that is being removed and, therefore, should not
 *        be included in the path.
 * @return true if a path exists without the excluded part, false otherwise.
 */
bool vehicle::is_connected( const vehicle_part &to, const vehicle_part &from,
                            const vehicle_part &excluded_part ) const
{
    const auto target = to.mount;
    const auto excluded = excluded_part.mount;

    //Breadth-first-search components
    std::list<vehicle_part> discovered;
    vehicle_part current_part;
    std::list<vehicle_part> searched;

    //We begin with just the start point
    discovered.push_back( from );

    while( !discovered.empty() ) {
        current_part = discovered.front();
        discovered.pop_front();
        auto current = current_part.mount;

        for( int i = 0; i < 4; i++ ) {
            point next = current + vehicles::cardinal_d[i];

            if( next == target ) {
                //Success!
                return true;
            } else if( next == excluded ) {
                //There might be a path, but we're not allowed to go that way
                continue;
            }

            std::vector<int> parts_there = parts_at_relative( next, true );

            if( !parts_there.empty() && !parts[ parts_there[ 0 ] ].removed &&
                part_info( parts_there[ 0 ] ).location == "structure" &&
                !part_info( parts_there[ 0 ] ).has_flag( "PROTRUSION" ) ) {
                //Only add the part if we haven't been here before
                bool found = false;
                for( auto &elem : discovered ) {
                    if( elem.mount == next ) {
                        found = true;
                        break;
                    }
                }
                if( !found ) {
                    for( auto &elem : searched ) {
                        if( elem.mount == next ) {
                            found = true;
                            break;
                        }
                    }
                }
                if( !found ) {
                    vehicle_part next_part = parts[parts_there[0]];
                    discovered.push_back( next_part );
                }
            }
        }
        //Now that that's done, we've finished exploring here
        searched.push_back( current_part );
    }
    //If we completely exhaust the discovered list, there's no path
    return false;
}

/**
 * Installs a part into this vehicle.
 * @param dp The coordinate of where to install the part.
 * @param id The string ID of the part to install. (see vehicle_parts.json)
 * @param force Skip check of whether we can mount the part here.
 * @return false if the part could not be installed, true otherwise.
 */
int vehicle::install_part( const point &dp, const vpart_id &id, bool force )
{
    if( !( force || can_mount( dp, id ) ) ) {
        return -1;
    }
    return install_part( dp, vehicle_part( id, dp, item( id.obj().item ) ) );
}

int vehicle::install_part( const point &dp, const vpart_id &id, item &&obj, bool force )
{
    if( !( force || can_mount( dp, id ) ) ) {
        return -1;
    }
    return install_part( dp, vehicle_part( id, dp, std::move( obj ) ) );
}

int vehicle::install_part( const point &dp, const vehicle_part &new_part )
{
    // Should be checked before installing the part
    bool enable = false;
    if( new_part.is_engine() ) {
        enable = true;
    } else {
        // @todo: read toggle groups from JSON
        static const std::vector<std::string> enable_like = {{
                "CONE_LIGHT",
                "CIRCLE_LIGHT",
                "AISLE_LIGHT",
                "DOME_LIGHT",
                "ATOMIC_LIGHT",
                "STEREO",
                "CHIMES",
                "FRIDGE",
                "FREEZER",
                "RECHARGE",
                "PLOW",
                "REAPER",
                "PLANTER",
                "SCOOP",
                "WATER_PURIFIER",
                "ROCKWHEEL"
            }
        };

        for( const std::string &flag : enable_like ) {
            if( new_part.info().has_flag( flag ) ) {
                enable = has_part( flag, true );
                break;
            }
        }
    }

    parts.push_back( new_part );
    auto &pt = parts.back();

    pt.enabled = enable;

    pt.mount = dp;

    refresh();
    coeff_air_changed = true;
    return parts.size() - 1;
}

bool vehicle::try_to_rack_nearby_vehicle( const std::vector<std::vector<int>> &list_of_racks )
{
    for( auto this_bike_rack : list_of_racks ) {
        std::vector<vehicle *> carry_vehs;
        carry_vehs.assign( 4, nullptr );
        vehicle *test_veh = nullptr;
        std::set<tripoint> veh_partial_match;
        std::vector<std::set<tripoint>> partial_matches;
        partial_matches.assign( 4, veh_partial_match );
        for( auto rack_part : this_bike_rack ) {
            tripoint rack_pos = global_part_pos3( rack_part );
            for( int i = 0; i < 4; i++ ) {
                tripoint search_pos( rack_pos + vehicles::cardinal_d[ i ] );
                test_veh = veh_pointer_or_null( g->m.veh_at( search_pos ) );
                if( test_veh == nullptr || test_veh == this ) {
                    continue;
                } else if( test_veh != carry_vehs[ i ] ) {
                    carry_vehs[ i ] = test_veh;
                    partial_matches[ i ].clear();
                }
                partial_matches[ i ].insert( search_pos );
                if( partial_matches[ i ] == test_veh->get_points() ) {
                    return merge_rackable_vehicle( test_veh, this_bike_rack );
                }
            }
        }
    }
    return false;
}

bool vehicle::merge_rackable_vehicle( vehicle *carry_veh, const std::vector<int> &rack_parts )
{
    // Mapping between the old vehicle and new vehicle mounting points
    struct mapping {
        // All the parts attached to this mounting point
        std::vector<int> carry_parts_here;

        // the index where the racking part is on the vehicle with the rack
        int rack_part;

        // the mount point we are going to add to the vehicle with the rack
        point carry_mount;

        // the mount point on the old vehicle (carry_veh) that will be destroyed
        point old_mount;
    };

    // By structs, we mean all the parts of the carry vehicle that are at the structure location
    // of the vehicle (i.e. frames)
    std::vector<int> carry_veh_structs = carry_veh->all_parts_at_location( part_location_structure );
    std::vector<mapping> carry_data;
    carry_data.reserve( carry_veh_structs.size() );

    //X is forward/backward, Y is left/right
    std::string axis;
    if( carry_veh_structs.size() == 1 ) {
        axis = "X";
    } else {
        for( auto carry_part : carry_veh_structs ) {
            if( carry_veh->parts[ carry_part ].mount.x || carry_veh->parts[ carry_part ].mount.y ) {
                axis = carry_veh->parts[ carry_part ].mount.x ? "X" : "Y";
            }
        }
    }

    int relative_dir = modulo( carry_veh->face.dir() - face.dir(), 360 );
    int relative_180 = modulo( relative_dir, 180 );
    int face_dir_180 = modulo( face.dir(), 180 );

    // if the carrier is skewed N/S and the carried vehicle isn't aligned with
    // the carrier, force the carried vehicle to be at a right angle
    if( face_dir_180 >= 45 && face_dir_180 <= 135 ) {
        if( relative_180 >= 45 && relative_180 <= 135 ) {
            if( relative_dir < 180 ) {
                relative_dir = 90;
            } else {
                relative_dir = 270;
            }
        }
    }

    // We look at each of the structure parts (mount points, i.e. frames) for the
    // carry vehicle and then find a rack part adjacent to it. If we dont find a rack part,
    // then we cant merge.
    bool found_all_parts = true;
    for( auto carry_part : carry_veh_structs ) {

        // The current position on the original vehicle for this part
        tripoint carry_pos = carry_veh->global_part_pos3( carry_part );

        bool merged_part = false;
        for( int rack_part : rack_parts ) {
            size_t j = 0;
            // There's no mathematical transform from global pos3 to vehicle mount, so search for the
            // carry part in global pos3 after translating
            point carry_mount;
            for( j = 0; j < 4; j++ ) {
                carry_mount = parts[ rack_part ].mount + vehicles::cardinal_d[ j ];
                tripoint possible_pos = mount_to_tripoint( carry_mount );
                if( possible_pos == carry_pos ) {
                    break;
                }
            }

            // We checked the adjacent points from the mounting rack and managed
            // to find the current structure part were looking for nearby. If the part was not
            // near this particular rack, we would look at each in the list of rack_parts
            const bool carry_part_next_to_this_rack = j < 4;
            if( carry_part_next_to_this_rack ) {
                mapping carry_map;
                point old_mount = carry_veh->parts[ carry_part ].mount;
                carry_map.carry_parts_here = carry_veh->parts_at_relative( old_mount, true );
                carry_map.rack_part = rack_part;
                carry_map.carry_mount = carry_mount;
                carry_map.old_mount = old_mount;
                carry_data.push_back( carry_map );
                merged_part = true;
                break;
            }
        }

        // We checked all the racks and could not find a place for this structure part.
        if( !merged_part ) {
            found_all_parts = false;
            break;
        }
    }

    // Now that we have mapped all the parts of the carry vehicle to the vehicle with the rack
    // we can go ahead and merge
    const point mount_zero = point_zero;
    if( found_all_parts ) {
        decltype( loot_zones ) new_zones;
        for( auto carry_map : carry_data ) {
            std::string offset = string_format( "%s%3d", carry_map.old_mount == mount_zero ? axis : " ",
                                                axis == "X" ? carry_map.old_mount.x : carry_map.old_mount.y );
            std::string unique_id = string_format( "%s%3d%s", offset, relative_dir, carry_veh->name );
            for( auto carry_part : carry_map.carry_parts_here ) {
                parts.push_back( carry_veh->parts[ carry_part ] );
                vehicle_part &carried_part = parts.back();
                carried_part.mount = carry_map.carry_mount;
                carried_part.carry_names.push( unique_id );
                carried_part.enabled = false;
                carried_part.set_flag( vehicle_part::carried_flag );
                parts[ carry_map.rack_part ].set_flag( vehicle_part::carrying_flag );
            }

            const std::pair<std::unordered_multimap<point, zone_data>::iterator, std::unordered_multimap<point, zone_data>::iterator>
            zones_on_point = carry_veh->loot_zones.equal_range( carry_map.old_mount );
            for( std::unordered_multimap<point, zone_data>::const_iterator it = zones_on_point.first;
                 it != zones_on_point.second; ++it ) {
                new_zones.emplace( carry_map.carry_mount, it->second );
            }
        }

        for( std::unordered_multimap<point, zone_data>::iterator it = new_zones.begin();
             it != new_zones.end(); ++it ) {
            zone_manager::get_manager().create_vehicle_loot_zone( *this, it->first, it->second );
        }

        // Now that we've added zones to this vehicle, we need to make sure their positions
        // update when we next interact with them
        zones_dirty = true;

        //~ %1$s is the vehicle being loaded onto the bicycle rack
        add_msg( _( "You load the %1$s on the rack" ), carry_veh->name );
        g->m.destroy_vehicle( carry_veh );
        g->m.dirty_vehicle_list.insert( this );
        g->m.set_transparency_cache_dirty( smz );
        refresh();
    } else {
        //~ %1$s is the vehicle being loaded onto the bicycle rack
        add_msg( m_bad, _( "You can't get the %1$s on the rack" ), carry_veh->name );
    }
    return found_all_parts;
}

/**
 * Mark a part as removed from the vehicle.
 * @return bool true if the vehicle's 0,0 point shifted.
 */
bool vehicle::remove_part( int p )
{
    if( p >= static_cast<int>( parts.size() ) ) {
        debugmsg( "Tried to remove part %d but only %d parts!", p, parts.size() );
        return false;
    }
    if( parts[p].removed ) {
        /* This happens only when we had to remove part, because it was depending on
         * other part (using recursive remove_part() call) - currently curtain
         * depending on presence of window and seatbelt depending on presence of seat.
         */
        return false;
    }

    const tripoint part_loc = global_part_pos3( p );

    // If `p` has flag `parent_flag`, remove child with flag `child_flag`
    // Returns true if removal occurs
    const auto remove_dependent_part = [&]( const std::string & parent_flag,
    const std::string & child_flag ) {
        if( part_flag( p, parent_flag ) ) {
            int dep = part_with_feature( p, child_flag, false );
            if( dep >= 0 ) {
                item it = parts[dep].properties_to_item();
                g->m.add_item_or_charges( part_loc, it );
                remove_part( dep );
                return true;
            }
        }
        return false;
    };

    // if a windshield is removed (usually destroyed) also remove curtains
    // attached to it.
    if( remove_dependent_part( "WINDOW", "CURTAIN" ) || part_flag( p, VPFLAG_OPAQUE ) ) {
        g->m.set_transparency_cache_dirty( smz );
    }

    remove_dependent_part( "SEAT", "SEATBELT" );
    remove_dependent_part( "BATTERY_MOUNT", "NEEDS_BATTERY_MOUNT" );

    // Unboard any entities standing on removed boardable parts
    if( part_flag( p, "BOARDABLE" ) ) {
        std::vector<int> bp = boarded_parts();
        for( auto &elem : bp ) {
            if( elem == p ) {
                g->m.unboard_vehicle( part_loc );
            }
        }
    }

    // Release any animal held by the part
    if( parts[p].has_flag( vehicle_part::animal_flag ) ) {
        tripoint target = part_loc;
        bool spawn = true;
        if( !g->is_empty( target ) ) {
            std::vector<tripoint> valid;
            for( const tripoint &dest : g->m.points_in_radius( target, 1 ) ) {
                if( g->is_empty( dest ) ) {
                    valid.push_back( dest );
                }
            }
            if( valid.empty() ) {
                spawn = false;
            } else {
                target = random_entry( valid );
            }
        }
        item base = item( parts[p].get_base() );
        base.release_monster( target, spawn );
        parts[p].set_base( base );
        parts[p].remove_flag( vehicle_part::animal_flag );
    }

    // Update current engine configuration if needed
    if( part_flag( p, "ENGINE" ) && engines.size() > 1 ) {
        bool any_engine_on = false;

        for( auto &e : engines ) {
            if( e != p && is_part_on( e ) ) {
                any_engine_on = true;
                break;
            }
        }

        if( !any_engine_on ) {
            engine_on = false;
            for( auto &e : engines ) {
                toggle_specific_part( e, true );
            }
        }
    }

    //Remove loot zone if Cargo was removed.
    const auto lz_iter = loot_zones.find( parts[p].mount );
    const bool no_zone = lz_iter != loot_zones.end();

    if( no_zone && part_flag( p, "CARGO" ) ) {
        // Using the key here (instead of the iterator) will remove all zones on
        // this mount points regardless of how many there are
        loot_zones.erase( parts[p].mount );
        zones_dirty = true;
    }

    parts[p].removed = true;
    removed_part_count++;

    // If the player is currently working on the removed part, stop them as it's futile now.
    const player_activity &act = g->u.activity;
    if( act.id() == activity_id( "ACT_VEHICLE" ) && act.moves_left > 0 && act.values.size() > 6 ) {
        if( veh_pointer_or_null( g->m.veh_at( tripoint( act.values[0], act.values[1],
                                              g->u.posz() ) ) ) == this ) {
            if( act.values[6] >= p ) {
                g->u.cancel_activity();
                add_msg( m_info, _( "The vehicle part you were working on has gone!" ) );
            }
        }
    }

    const point &vp_mount = parts[p].mount;
    const auto iter = labels.find( label( vp_mount ) );
    const bool no_label = iter != labels.end();
    const bool grab_found = g->u.get_grab_type() == OBJECT_VEHICLE && g->u.grab_point == part_loc;
    // Checking these twice to avoid calling the relatively expensive parts_at_relative() unnecessarily.
    if( no_label || grab_found ) {
        if( parts_at_relative( vp_mount, false ).empty() ) {
            if( no_label ) {
                labels.erase( iter );
            }
            if( grab_found ) {
                add_msg( m_info, _( "The vehicle part you were holding has been destroyed!" ) );
                g->u.grab( OBJECT_NONE );
            }
        }
    }

    for( auto &i : get_items( p ) ) {
        // Note: this can spawn items on the other side of the wall!
        tripoint dest( part_loc.x + rng( -3, 3 ), part_loc.y + rng( -3, 3 ), part_loc.z );
        g->m.add_item_or_charges( dest, i );
    }
    g->m.dirty_vehicle_list.insert( this );
    refresh();
    coeff_air_changed = true;
    return shift_if_needed();
}

void vehicle::part_removal_cleanup()
{
    bool changed = false;
    for( std::vector<vehicle_part>::iterator it = parts.begin(); it != parts.end(); /* noop */ ) {
        if( it->removed ) {
            auto items = get_items( std::distance( parts.begin(), it ) );
            while( !items.empty() ) {
                items.erase( items.begin() );
            }
            it = parts.erase( it );
            changed = true;
        } else {
            ++it;
        }
    }
    removed_part_count = 0;
    if( changed || parts.empty() ) {
        refresh();
        if( parts.empty() ) {
            g->m.destroy_vehicle( this );
            return;
        } else {
            g->m.update_vehicle_cache( this, smz );
        }
    }
    shift_if_needed();
    refresh(); // Rebuild cached indices
    coeff_air_dirty = coeff_air_changed;
    coeff_air_changed = false;
}

void vehicle::remove_carried_flag()
{
    for( vehicle_part &part : parts ) {
        part.carry_names.pop();
        if( part.carry_names.empty() ) {
            part.remove_flag( vehicle_part::carried_flag );
        }
    }
}

bool vehicle::remove_carried_vehicle( const std::vector<int> &carried_parts )
{
    if( carried_parts.empty() ) {
        return false;
    }
    std::string veh_record;
    tripoint new_pos3;
    bool x_aligned = false;
    for( int carried_part : carried_parts ) {
        std::string id_string = parts[ carried_part ].carry_names.top().substr( 0, 1 );
        if( id_string == "X" || id_string == "Y" ) {
            veh_record = parts[ carried_part ].carry_names.top();
            new_pos3 = global_part_pos3( carried_part );
            x_aligned = id_string == "X";
            break;
        }
    }
    if( veh_record.empty() ) {
        return false;
    }
    int new_dir = modulo( std::stoi( veh_record.substr( 4, 3 ) ) + face.dir(), 360 );
    int host_dir = modulo( face.dir(), 180 );
    // if the host is skewed N/S, and the carried vehicle is going to come at an angle,
    // force it to east/west instead
    if( host_dir >= 45 && host_dir <= 135 ) {
        if( new_dir <= 45 || new_dir >= 315 ) {
            new_dir = 0;
        } else if( new_dir >= 135 && new_dir <= 225 ) {
            new_dir = 180;
        }
    }
    vehicle *new_vehicle = g->m.add_vehicle( vproto_id( "none" ), new_pos3, new_dir );
    if( new_vehicle == nullptr ) {
        add_msg( m_debug, "Unable to unload bike rack, host face %d, new_dir %d!", face.dir(), new_dir );
        return false;
    }

    std::vector<point> new_mounts;
    new_vehicle->name = veh_record.substr( vehicle_part::name_offset );
    for( auto carried_part : carried_parts ) {
        std::string mount_str = parts[ carried_part ].carry_names.top().substr( 1, 3 );
        point new_mount;
        if( x_aligned ) {
            new_mount.x = std::stoi( mount_str );
        } else {
            new_mount.y = std::stoi( mount_str );
        }
        new_mounts.push_back( new_mount );
    }

    std::vector<vehicle *> new_vehicles;
    new_vehicles.push_back( new_vehicle );
    std::vector<std::vector<int>> carried_vehicles;
    carried_vehicles.push_back( carried_parts );
    std::vector<std::vector<point>> carried_mounts;
    carried_mounts.push_back( new_mounts );
    bool success = split_vehicles( carried_vehicles, new_vehicles, carried_mounts );
    if( success ) {
        //~ %s is the vehicle being loaded onto the bicycle rack
        add_msg( _( "You unload the %s from the bike rack. " ), new_vehicle->name );
        new_vehicle->remove_carried_flag();
        g->m.dirty_vehicle_list.insert( this );
        part_removal_cleanup();
    } else {
        //~ %s is the vehicle being loaded onto the bicycle rack
        add_msg( m_bad, _( "You can't unload the %s from the bike rack. " ), new_vehicle->name );
    }
    return success;
}

// split the current vehicle into up to 3 new vehicles that do not connect to each other
bool vehicle::find_and_split_vehicles( int exclude )
{
    std::vector<int> valid_parts = all_parts_at_location( part_location_structure );
    std::set<int> checked_parts;
    checked_parts.insert( exclude );

    std::vector<std::vector <int>> all_vehicles;

    for( size_t cnt = 0 ; cnt < 4 ; cnt++ ) {
        int test_part = -1;
        for( auto p : valid_parts ) {
            if( parts[ p ].removed ) {
                continue;
            }
            if( checked_parts.find( p ) == checked_parts.end() ) {
                test_part = p;
                break;
            }
        }
        if( test_part == -1 || static_cast<size_t>( test_part ) > parts.size() ) {
            break;
        }

        std::queue<std::pair<int, std::vector<int>>> search_queue;

        const auto push_neighbor = [&]( int p, std::vector<int> with_p ) {
            std::pair<int, std::vector<int>> data( p, with_p );
            search_queue.push( data );
        };
        auto pop_neighbor = [&]() {
            std::pair<int, std::vector<int>> result = search_queue.front();
            search_queue.pop();
            return result;
        };

        std::vector<int> veh_parts;
        push_neighbor( test_part, parts_at_relative( parts[ test_part ].mount, true ) );
        while( !search_queue.empty() ) {
            std::pair<int, std::vector<int>> test_set = pop_neighbor();
            test_part = test_set.first;
            if( checked_parts.find( test_part ) != checked_parts.end() ) {
                continue;
            }
            for( auto p : test_set.second ) {
                veh_parts.push_back( p );
            }
            checked_parts.insert( test_part );
            for( size_t i = 0; i < 4; i++ ) {
                const point dp = parts[test_part].mount + vehicles::cardinal_d[ i ];
                std::vector<int> all_neighbor_parts = parts_at_relative( dp, true );
                int neighbor_struct_part = -1;
                for( int p : all_neighbor_parts ) {
                    if( parts[ p ].removed ) {
                        continue;
                    }
                    if( part_info( p ).location == part_location_structure ) {
                        neighbor_struct_part = p;
                        break;
                    }
                }
                if( neighbor_struct_part != -1 ) {
                    push_neighbor( neighbor_struct_part, all_neighbor_parts );
                }
            }
        }
        // don't include the first vehicle's worth of parts
        if( cnt > 0 ) {
            all_vehicles.push_back( veh_parts );
        }
    }

    if( !all_vehicles.empty() ) {
        bool success = split_vehicles( all_vehicles );
        if( success ) {
            // update the active cache
            shift_parts( point_zero );
            return true;
        }
    }
    return false;
}

void vehicle::relocate_passengers( const std::vector<player *> &passengers )
{
    const auto boardables = get_avail_parts( "BOARDABLE" );
    for( player *passenger : passengers ) {
        for( const vpart_reference &vp : boardables ) {
            if( vp.part().passenger_id == passenger->getID() ) {
                passenger->setpos( vp.pos() );
            }
        }
    }
}

// Split a vehicle into an old vehicle and one or more new vehicles by moving vehicle_parts
// from one the old vehicle to the new vehicles.
// some of the logic borrowed from remove_part
// skipped the grab, curtain, player activity, and engine checks because they deal
// with pos, not a vehicle pointer
// @param new_vehs vector of vectors of part indexes to move to new vehicles
// @param new_vehicles vector of vehicle pointers containing the new vehicles; if empty, new
// vehicles will be created
// @param new_mounts vector of vector of mount points. must have one vector for every vehicle*
// in new_vehicles, and forces the part indices in new_vehs to be mounted on the new vehicle
// at those mount points
bool vehicle::split_vehicles( const std::vector<std::vector <int>> &new_vehs,
                              const std::vector<vehicle *> &new_vehicles,
                              const std::vector<std::vector <point>> &new_mounts )
{
    bool did_split = false;
    size_t i = 0;
    for( i = 0; i < new_vehs.size(); i ++ ) {
        std::vector<int> split_parts = new_vehs[ i ];
        if( split_parts.empty() ) {
            continue;
        }
        std::vector<point> split_mounts = new_mounts[ i ];
        did_split = true;

        vehicle *new_vehicle = nullptr;
        if( i < new_vehicles.size() ) {
            new_vehicle = new_vehicles[ i ];
        }
        int split_part0 = split_parts.front();
        tripoint new_v_pos3;
        point mnt_offset;

        decltype( labels ) new_labels;
        decltype( loot_zones ) new_zones;
        if( new_vehicle == nullptr ) {
            // make sure the split_part0 is a legal 0,0 part
            if( split_parts.size() > 1 ) {
                for( size_t sp = 0; sp < split_parts.size(); sp++ ) {
                    int p = split_parts[ sp ];
                    if( part_info( p ).location == part_location_structure &&
                        !part_info( p ).has_flag( "PROTRUSION" ) ) {
                        split_part0 = sp;
                        break;
                    }
                }
            }
            new_v_pos3 = global_part_pos3( parts[ split_part0 ] );
            mnt_offset = parts[ split_part0 ].mount;
            new_vehicle = g->m.add_vehicle( vproto_id( "none" ), new_v_pos3, face.dir() );
            new_vehicle->name = name;
            new_vehicle->move = move;
            new_vehicle->turn_dir = turn_dir;
            new_vehicle->velocity = velocity;
            new_vehicle->vertical_velocity = vertical_velocity;
            new_vehicle->cruise_velocity = cruise_velocity;
            new_vehicle->cruise_on = cruise_on;
            new_vehicle->engine_on = engine_on;
            new_vehicle->tracking_on = tracking_on;
            new_vehicle->camera_on = camera_on;
        }
        new_vehicle->last_fluid_check = last_fluid_check;

        std::vector<player *> passengers;
        for( size_t new_part = 0; new_part < split_parts.size(); new_part++ ) {
            int mov_part = split_parts[ new_part ];
            point cur_mount = parts[ mov_part ].mount;
            point new_mount = cur_mount;
            if( !split_mounts.empty() ) {
                new_mount = split_mounts[ new_part ];
            } else {
                new_mount -= mnt_offset;
            }

            player *passenger = nullptr;
            // Unboard any entities standing on any transferred part
            if( part_flag( mov_part, "BOARDABLE" ) ) {
                passenger = get_passenger( mov_part );
                if( passenger ) {
                    passengers.push_back( passenger );
                }
            }
            // transfer the vehicle_part to the new vehicle
            new_vehicle->parts.emplace_back( parts[ mov_part ] );
            new_vehicle->parts.back().mount = new_mount;

            // remove labels associated with the mov_part
            const auto iter = labels.find( label( cur_mount ) );
            if( iter != labels.end() ) {
                std::string label_str = iter->text;
                labels.erase( iter );
                new_labels.insert( label( new_mount, label_str ) );
            }
            // Prepare the zones to be moved to the new vehicle
            const std::pair<std::unordered_multimap<point, zone_data>::iterator, std::unordered_multimap<point, zone_data>::iterator>
            zones_on_point = loot_zones.equal_range( cur_mount );
            for( std::unordered_multimap<point, zone_data>::const_iterator lz_iter = zones_on_point.first;
                 lz_iter != zones_on_point.second; ++lz_iter ) {
                new_zones.emplace( new_mount, lz_iter->second );
            }

            // Erasing on the key removes all the zones from the point at once
            loot_zones.erase( cur_mount );

            // The zone manager will be updated when we next interact with it through get_vehicle_zones
            zones_dirty = true;

            // remove the passenger from the old vehicle
            if( passenger ) {
                parts[ mov_part ].remove_flag( vehicle_part::passenger_flag );
                parts[ mov_part ].passenger_id = 0;
            }
            // indicate the part needs to be removed from the old vehicle
            parts[ mov_part].removed = true;
            removed_part_count++;
        }

        // We want to create the vehicle zones after we've setup the parts
        // because we need only to move the zone once per mount, not per part. If we move per
        // part, we will end up with duplicates of the zone per part on the same mount
        for( std::pair<point, zone_data> zone : new_zones ) {
            zone_manager::get_manager().create_vehicle_loot_zone( *new_vehicle, zone.first, zone.second );
        }

        // create_vehicle_loot_zone marks the vehicle as not dirty but since we got these zones
        // in an unknown state from the previous vehicle, we need to let the cache rebuild next
        // time we interact with them
        new_vehicle->zones_dirty = true;

        g->m.dirty_vehicle_list.insert( new_vehicle );
        g->m.set_transparency_cache_dirty( smz );
        if( !new_labels.empty() ) {
            new_vehicle->labels = new_labels;
        }

        if( split_mounts.empty() ) {
            new_vehicle->refresh();
        } else {
            // include refresh
            new_vehicle->shift_parts( point_zero - mnt_offset );
        }

        // update the precalc points
        new_vehicle->precalc_mounts( 1, new_vehicle->skidding ?
                                     new_vehicle->turn_dir : new_vehicle->face.dir(),
                                     new_vehicle->pivot_point() );
        if( !passengers.empty() ) {
            new_vehicle->relocate_passengers( passengers );
        }
    }
    return did_split;
}

bool vehicle::split_vehicles( const std::vector<std::vector <int>> &new_vehs )
{
    std::vector<vehicle *> null_vehicles;
    std::vector<std::vector <point>> null_mounts;
    std::vector<point> nothing;
    null_vehicles.assign( new_vehs.size(), nullptr );
    null_mounts.assign( new_vehs.size(), nothing );
    return split_vehicles( new_vehs, null_vehicles, null_mounts );
}

item_location vehicle::part_base( int p )
{
    return item_location( vehicle_cursor( *this, p ), &parts[ p ].base );
}

int vehicle::find_part( const item &it ) const
{
    auto idx = std::find_if( parts.begin(), parts.end(), [&it]( const vehicle_part & e ) {
        return &e.base == &it;
    } );
    return idx != parts.end() ? std::distance( parts.begin(), idx ) : INT_MIN;
}

item_group::ItemList vehicle_part::pieces_for_broken_part() const
{
    const std::string &group = info().breaks_into_group;
    // @todo make it optional? Or use id of empty item group?
    if( group.empty() ) {
        return {};
    }

    return item_group::items_from( group, calendar::turn );
}

std::vector<int> vehicle::parts_at_relative( const point &dp,
        const bool use_cache ) const
{
    if( !use_cache ) {
        std::vector<int> res;
        for( const vpart_reference &vp : get_all_parts() ) {
            if( vp.mount() == dp && !vp.part().removed ) {
                res.push_back( static_cast<int>( vp.part_index() ) );
            }
        }
        return res;
    } else {
        const auto &iter = relative_parts.find( dp );
        if( iter != relative_parts.end() ) {
            return iter->second;
        } else {
            std::vector<int> res;
            return res;
        }
    }
}

cata::optional<vpart_reference> vpart_position::obstacle_at_part() const
{
    const cata::optional<vpart_reference> part = part_with_feature( VPFLAG_OBSTACLE, true );
    if( !part ) {
        return cata::nullopt; // No obstacle here
    }

    if( part->has_feature( VPFLAG_OPENABLE ) && part->part().open ) {
        return cata::nullopt; // Open door here
    }

    return part;
}

cata::optional<vpart_reference> vpart_position::part_displayed() const
{
    int part_id = vehicle().part_displayed_at( mount() );
    if( part_id == -1 ) {
        return cata::nullopt;
    }
    return vpart_reference( vehicle(), part_id );
}

cata::optional<vpart_reference> vpart_position::part_with_feature( const std::string &f,
        const bool unbroken ) const
{
    const int i = vehicle().part_with_feature( part_index(), f, unbroken );
    if( i < 0 ) {
        return cata::nullopt;
    }
    return vpart_reference( vehicle(), i );
}

cata::optional<vpart_reference> vpart_position::part_with_feature( const vpart_bitflags f,
        const bool unbroken ) const
{
    const int i = vehicle().part_with_feature( part_index(), f, unbroken );
    if( i < 0 ) {
        return cata::nullopt;
    }
    return vpart_reference( vehicle(), i );
}

cata::optional<vpart_reference> optional_vpart_position::part_with_feature( const std::string &f,
        const bool unbroken ) const
{
    return has_value() ? value().part_with_feature( f, unbroken ) : cata::nullopt;
}

cata::optional<vpart_reference> optional_vpart_position::part_with_feature( const vpart_bitflags f,
        const bool unbroken ) const
{
    return has_value() ? value().part_with_feature( f, unbroken ) : cata::nullopt;
}

cata::optional<vpart_reference> optional_vpart_position::obstacle_at_part() const
{
    return has_value() ? value().obstacle_at_part() : cata::nullopt;
}

cata::optional<vpart_reference> optional_vpart_position::part_displayed() const
{
    return has_value() ? value().part_displayed() : cata::nullopt;
}

int vehicle::part_with_feature( int part, vpart_bitflags const flag, bool unbroken ) const
{
    if( part_flag( part, flag ) && ( !unbroken || !parts[part].is_broken() ) ) {
        return part;
    }
    const auto it = relative_parts.find( parts[part].mount );
    if( it != relative_parts.end() ) {
        const std::vector<int> &parts_here = it->second;
        for( auto &i : parts_here ) {
            if( part_flag( i, flag ) && ( !unbroken || !parts[i].is_broken() ) ) {
                return i;
            }
        }
    }
    return -1;
}

int vehicle::part_with_feature( int part, const std::string &flag, bool unbroken ) const
{
    return part_with_feature( parts[part].mount, flag, unbroken );
}

int vehicle::part_with_feature( const point &pt, const std::string &flag, bool unbroken ) const
{
    std::vector<int> parts_here = parts_at_relative( pt, false );
    for( auto &elem : parts_here ) {
        if( part_flag( elem, flag ) && ( !unbroken || !parts[ elem ].is_broken() ) ) {
            return elem;
        }
    }
    return -1;
}

int vehicle::avail_part_with_feature( int part, vpart_bitflags const flag, bool unbroken ) const
{
    int part_a = part_with_feature( part, flag, unbroken );
    if( ( part_a >= 0 ) && parts[ part_a ].is_available() ) {
        return part_a;
    }
    return -1;
}

int vehicle::avail_part_with_feature( int part, const std::string &flag, bool unbroken ) const
{
    return avail_part_with_feature( parts[ part ].mount, flag, unbroken );
}

int vehicle::avail_part_with_feature( const point &pt, const std::string &flag,
                                      bool unbroken ) const
{
    int part_a = part_with_feature( pt, flag, unbroken );
    if( ( part_a >= 0 ) && parts[ part_a ].is_available() ) {
        return part_a;
    }
    return -1;
}

bool vehicle::has_part( const std::string &flag, bool enabled ) const
{
    return std::any_of( parts.begin(), parts.end(), [&flag, &enabled]( const vehicle_part & e ) {
        return !e.removed && ( !enabled || e.enabled ) && !e.is_broken() && e.info().has_flag( flag );
    } );
}

bool vehicle::has_part( const tripoint &pos, const std::string &flag, bool enabled ) const
{
    const tripoint relative_pos = pos - global_pos3();

    for( const auto &e : parts ) {
        if( e.precalc[0].x != relative_pos.x || e.precalc[0].y != relative_pos.y ) {
            continue;
        }
        if( !e.removed && ( !enabled || e.enabled ) && !e.is_broken() && e.info().has_flag( flag ) ) {
            return true;
        }
    }
    return false;
}

std::vector<vehicle_part *> vehicle::get_parts_at( const tripoint &pos, const std::string &flag,
        const part_status_flag condition )
{
    const tripoint relative_pos = pos - global_pos3();
    std::vector<vehicle_part *> res;
    for( auto &e : parts ) {
        if( e.precalc[ 0 ].x != relative_pos.x || e.precalc[ 0 ].y != relative_pos.y ) {
            continue;
        }
        if( !e.removed &&
            ( flag.empty() || e.info().has_flag( flag ) ) &&
            ( !( condition & part_status_flag::enabled ) || e.enabled ) &&
            ( !( condition & part_status_flag::working ) || !e.is_broken() ) ) {
            res.push_back( &e );
        }
    }
    return res;
}

std::vector<const vehicle_part *> vehicle::get_parts_at( const tripoint &pos,
        const std::string &flag,
        const part_status_flag condition ) const
{
    const tripoint relative_pos = pos - global_pos3();
    std::vector<const vehicle_part *> res;
    for( const auto &e : parts ) {
        if( e.precalc[ 0 ].x != relative_pos.x || e.precalc[ 0 ].y != relative_pos.y ) {
            continue;
        }
        if( !e.removed &&
            ( flag.empty() || e.info().has_flag( flag ) ) &&
            ( !( condition & part_status_flag::enabled ) || e.enabled ) &&
            ( !( condition & part_status_flag::working ) || !e.is_broken() ) ) {
            res.push_back( &e );
        }
    }
    return res;
}

cata::optional<std::string> vpart_position::get_label() const
{
    const auto it = vehicle().labels.find( label( mount() ) );
    if( it == vehicle().labels.end() ) {
        return cata::nullopt;
    }
    if( it->text.empty() ) {
        // legacy support @todo change labels into a map and keep track of deleted labels
        return cata::nullopt;
    }
    return it->text;
}

void vpart_position::set_label( const std::string &text ) const
{
    auto &labels = vehicle().labels;
    const auto it = labels.find( label( mount() ) );
    //@todo empty text should remove the label instead of just storing an empty string, see get_label
    if( it == labels.end() ) {
        labels.insert( label( mount(), text ) );
    } else {
        // labels should really be a map
        labels.insert( labels.erase( it ), label( mount(), text ) );
    }
}

int vehicle::next_part_to_close( int p, bool outside ) const
{
    std::vector<int> parts_here = parts_at_relative( parts[p].mount, true );

    // We want reverse, since we close the outermost thing first (curtains), and then the innermost thing (door)
    for( std::vector<int>::reverse_iterator part_it = parts_here.rbegin();
         part_it != parts_here.rend();
         ++part_it ) {

        if( part_flag( *part_it, VPFLAG_OPENABLE )
            && parts[ *part_it ].is_available()
            && parts[*part_it].open == 1
            && ( !outside || !part_flag( *part_it, "OPENCLOSE_INSIDE" ) ) ) {
            return *part_it;
        }
    }
    return -1;
}

int vehicle::next_part_to_open( int p, bool outside ) const
{
    std::vector<int> parts_here = parts_at_relative( parts[p].mount, true );

    // We want forwards, since we open the innermost thing first (curtains), and then the innermost thing (door)
    for( auto &elem : parts_here ) {
        if( part_flag( elem, VPFLAG_OPENABLE ) && parts[ elem ].is_available() && parts[elem].open == 0 &&
            ( !outside || !part_flag( elem, "OPENCLOSE_INSIDE" ) ) ) {
            return elem;
        }
    }
    return -1;
}

vehicle_part_with_feature_range<std::string> vehicle::get_avail_parts( std::string feature ) const
{
    return vehicle_part_with_feature_range<std::string>( const_cast<vehicle &>( *this ),
            std::move( feature ),
            static_cast<part_status_flag>( part_status_flag::working |
                                           part_status_flag::available ) );
}

vehicle_part_with_feature_range<vpart_bitflags> vehicle::get_avail_parts(
    const vpart_bitflags feature ) const
{
    return vehicle_part_with_feature_range<vpart_bitflags>( const_cast<vehicle &>( *this ), feature,
            static_cast<part_status_flag>( part_status_flag::working |
                                           part_status_flag::available ) );
}

vehicle_part_with_feature_range<std::string> vehicle::get_parts_including_carried(
    std::string feature ) const
{
    return vehicle_part_with_feature_range<std::string>( const_cast<vehicle &>( *this ),
            std::move( feature ), part_status_flag::working );
}

vehicle_part_with_feature_range<vpart_bitflags> vehicle::get_parts_including_carried(
    const vpart_bitflags feature ) const
{
    return vehicle_part_with_feature_range<vpart_bitflags>( const_cast<vehicle &>( *this ), feature,
            part_status_flag::working );
}

vehicle_part_with_feature_range<std::string> vehicle::get_any_parts( std::string feature ) const
{
    return vehicle_part_with_feature_range<std::string>( const_cast<vehicle &>( *this ),
            std::move( feature ), part_status_flag::any );
}

vehicle_part_with_feature_range<vpart_bitflags> vehicle::get_any_parts(
    const vpart_bitflags feature ) const
{
    return vehicle_part_with_feature_range<vpart_bitflags>( const_cast<vehicle &>( *this ), feature,
            part_status_flag::any );
}

vehicle_part_with_feature_range<std::string> vehicle::get_enabled_parts( std::string feature ) const
{
    return vehicle_part_with_feature_range<std::string>( const_cast<vehicle &>( *this ),
            std::move( feature ),
            static_cast<part_status_flag>( part_status_flag::enabled |
                                           part_status_flag::working |
                                           part_status_flag::available ) );
}

vehicle_part_with_feature_range<vpart_bitflags> vehicle::get_enabled_parts(
    const vpart_bitflags feature ) const
{
    return vehicle_part_with_feature_range<vpart_bitflags>( const_cast<vehicle &>( *this ), feature,
            static_cast<part_status_flag>( part_status_flag::enabled |
                                           part_status_flag::working |
                                           part_status_flag::available ) );
}

/**
 * Returns all parts in the vehicle that exist in the given location slot. If
 * the empty string is passed in, returns all parts with no slot.
 * @param location The location slot to get parts for.
 * @return A list of indices to all parts with the specified location.
 */
std::vector<int> vehicle::all_parts_at_location( const std::string &location ) const
{
    std::vector<int> parts_found;
    for( size_t part_index = 0; part_index < parts.size(); ++part_index ) {
        if( part_info( part_index ).location == location && !parts[part_index].removed ) {
            parts_found.push_back( part_index );
        }
    }
    return parts_found;
}

/**
 * Returns all parts in the vehicle that have the specified flag in their vpinfo and
 * are on the same X-axis or Y-axis as the input part and are contiguous with each other.
 * @param part The part to find adjacent parts to
 * @param flag The flag to match
 * @return A list of lists of indices of all parts sharing the flag and contiguous with the part
 * on the X or Y axis. Returns 0, 1, or 2 lists of indices.
 */
std::vector<std::vector<int>> vehicle::find_lines_of_parts( int part, const std::string &flag )
{
    const auto possible_parts = get_avail_parts( flag );
    std::vector<std::vector<int>> ret_parts;
    if( empty( possible_parts ) ) {
        return ret_parts;
    }

    std::vector<int> x_parts;
    std::vector<int> y_parts;
    vpart_id part_id = part_info( part ).get_id();
    // create vectors of parts on the same X or Y axis
    point target = parts[ part ].mount;
    for( const vpart_reference &vp : possible_parts ) {
        if( vp.part().is_unavailable() ||
            !vp.has_feature( "MULTISQUARE" ) ||
            vp.info().get_id() != part_id )  {
            continue;
        }
        if( vp.mount().x == target.x ) {
            x_parts.push_back( vp.part_index() );
        }
        if( vp.mount().y == target.y ) {
            y_parts.push_back( vp.part_index() );
        }
    }

    if( x_parts.size() > 1 ) {
        std::vector<int> x_ret;
        // sort by Y-axis, since they're all on the same X-axis
        const auto x_sorter = [&]( const int lhs, const int rhs ) {
            return( parts[lhs].mount.y > parts[rhs].mount.y );
        };
        std::sort( x_parts.begin(), x_parts.end(), x_sorter );
        int first_part = 0;
        int prev_y = parts[ x_parts[ 0 ] ].mount.y;
        int i;
        bool found_part = x_parts[ 0 ] == part;
        for( i = 1; static_cast<size_t>( i ) < x_parts.size(); i++ ) {
            // if the Y difference is > 1, there's a break in the run
            if( std::abs( parts[ x_parts[ i ] ].mount.y - prev_y )  > 1 ) {
                // if we found the part, this is the run we wanted
                if( found_part ) {
                    break;
                }
                first_part = i;
            }
            found_part |= x_parts[ i ] == part;
            prev_y = parts[ x_parts[ i ] ].mount.y;
        }
        for( size_t j = first_part; j < static_cast<size_t>( i ); j++ ) {
            x_ret.push_back( x_parts[ j ] );
        }
        ret_parts.push_back( x_ret );
    }
    if( y_parts.size() > 1 ) {
        std::vector<int> y_ret;
        const auto y_sorter = [&]( const int lhs, const int rhs ) {
            return( parts[lhs].mount.x > parts[rhs].mount.x );
        };
        std::sort( y_parts.begin(), y_parts.end(), y_sorter );
        int first_part = 0;
        int prev_x = parts[ y_parts[ 0 ] ].mount.x;
        int i;
        bool found_part = y_parts[ 0 ] == part;
        for( i = 1; static_cast<size_t>( i ) < y_parts.size(); i++ ) {
            if( std::abs( parts[ y_parts[ i ] ].mount.x - prev_x )  > 1 ) {
                if( found_part ) {
                    break;
                }
                first_part = i;
            }
            found_part |= y_parts[ i ] == part;
            prev_x = parts[ y_parts[ i ] ].mount.x;
        }
        for( size_t j = first_part; j < static_cast<size_t>( i ); j++ ) {
            y_ret.push_back( y_parts[ j ] );
        }
        ret_parts.push_back( y_ret );
    }
    if( y_parts.size() == 1 && x_parts.size() == 1 ) {
        ret_parts.push_back( x_parts );
    }
    return ret_parts;
}

bool vehicle::part_flag( int part, const std::string &flag ) const
{
    if( part < 0 || part >= static_cast<int>( parts.size() ) || parts[part].removed ) {
        return false;
    } else {
        return part_info( part ).has_flag( flag );
    }
}

bool vehicle::part_flag( int part, const vpart_bitflags flag ) const
{
    if( part < 0 || part >= static_cast<int>( parts.size() ) || parts[part].removed ) {
        return false;
    } else {
        return part_info( part ).has_flag( flag );
    }
}

int vehicle::part_at( const point &dp ) const
{
    for( const vpart_reference &vp : get_all_parts() ) {
        if( vp.part().precalc[0] == dp && !vp.part().removed ) {
            return static_cast<int>( vp.part_index() );
        }
    }
    return -1;
}

/**
 * Given a vehicle part which is inside of this vehicle, returns the index of
 * that part. This exists solely because activities relating to vehicle editing
 * require the index of the vehicle part to be passed around.
 * @param part The part to find.
 * @param check_removed Check whether this part can be removed
 * @return The part index, -1 if it is not part of this vehicle.
 */
int vehicle::index_of_part( const vehicle_part *const part, const bool check_removed ) const
{
    if( part != nullptr ) {
        for( const vpart_reference &vp : get_all_parts() ) {
            const vehicle_part &next_part = vp.part();
            if( !check_removed && next_part.removed ) {
                continue;
            }
            if( part->id == next_part.id && part->mount == vp.mount() ) {
                return vp.part_index();
            }
        }
    }
    return -1;
}

/**
 * Returns which part (as an index into the parts list) is the one that will be
 * displayed for the given square. Returns -1 if there are no parts in that
 * square.
 * @param dp The local coordinate.
 * @return The index of the part that will be displayed.
 */
int vehicle::part_displayed_at( const point &dp ) const
{
    // Z-order is implicitly defined in game::load_vehiclepart, but as
    // numbers directly set on parts rather than constants that can be
    // used elsewhere. A future refactor might be nice but this way
    // it's clear where the magic number comes from.
    const int ON_ROOF_Z = 9;

    std::vector<int> parts_in_square = parts_at_relative( dp, true );

    if( parts_in_square.empty() ) {
        return -1;
    }

    bool in_vehicle = g->u.in_vehicle;
    if( in_vehicle ) {
        // They're in a vehicle, but are they in /this/ vehicle?
        std::vector<int> psg_parts = boarded_parts();
        in_vehicle = false;
        for( auto &psg_part : psg_parts ) {
            if( get_passenger( psg_part ) == &( g->u ) ) {
                in_vehicle = true;
                break;
            }
        }
    }

    int hide_z_at_or_above = ( in_vehicle ) ? ( ON_ROOF_Z ) : INT_MAX;

    int top_part = 0;
    for( size_t index = 1; index < parts_in_square.size(); index++ ) {
        if( ( part_info( parts_in_square[top_part] ).z_order <
              part_info( parts_in_square[index] ).z_order ) &&
            ( part_info( parts_in_square[index] ).z_order <
              hide_z_at_or_above ) ) {
            top_part = index;
        }
    }

    return parts_in_square[top_part];
}

int vehicle::roof_at_part( const int part ) const
{
    std::vector<int> parts_in_square = parts_at_relative( parts[part].mount, true );
    for( const int p : parts_in_square ) {
        if( part_info( p ).location == "on_roof" || part_flag( p, "ROOF" ) ) {
            return p;
        }
    }

    return -1;
}

point vehicle::coord_translate( const point &p ) const
{
    point q;
    coord_translate( pivot_rotation[0], pivot_anchor[0], p, q );
    return q;
}

void vehicle::coord_translate( int dir, const point &pivot, const point &p, point &q ) const
{
    tileray tdir( dir );
    tdir.advance( p.x - pivot.x );
    q.x = tdir.dx() + tdir.ortho_dx( p.y - pivot.y );
    q.y = tdir.dy() + tdir.ortho_dy( p.y - pivot.y );
}

void vehicle::coord_translate( tileray tdir, const point &pivot, const point &p, point &q ) const
{
    tdir.clear_advance();
    tdir.advance( p.x - pivot.x );
    q.x = tdir.dx() + tdir.ortho_dx( p.y - pivot.y );
    q.y = tdir.dy() + tdir.ortho_dy( p.y - pivot.y );
}

point vehicle::rotate_mount( int old_dir, int new_dir, const point &pivot, const point &p ) const
{
    point q;
    coord_translate( new_dir - old_dir, pivot, p, q );
    return q;
}

tripoint vehicle::mount_to_tripoint( const point &mount ) const
{
    point offset_zero( 0, 0 );
    return mount_to_tripoint( mount, offset_zero );
}

tripoint vehicle::mount_to_tripoint( const point &mount, const point &offset ) const
{
    point mnt_translated;
    coord_translate( pivot_rotation[0], pivot_anchor[ 0 ], mount + offset, mnt_translated );
    return global_pos3() + mnt_translated;
}

void vehicle::precalc_mounts( int idir, int dir, const point &pivot )
{
    if( idir < 0 || idir > 1 ) {
        idir = 0;
    }
    tileray tdir( dir );
    std::unordered_map<point, point> mount_to_precalc;
    for( auto &p : parts ) {
        if( p.removed ) {
            continue;
        }
        auto q = mount_to_precalc.find( p.mount );
        if( q == mount_to_precalc.end() ) {
            coord_translate( tdir, pivot, p.mount, p.precalc[idir] );
            mount_to_precalc.insert( { p.mount, p.precalc[idir] } );
        } else {
            p.precalc[idir] = q->second;
        }
    }
    pivot_anchor[idir] = pivot;
    pivot_rotation[idir] = dir;
}

std::vector<int> vehicle::boarded_parts() const
{
    std::vector<int> res;
    for( const vpart_reference &vp : get_avail_parts( VPFLAG_BOARDABLE ) ) {
        if( vp.part().has_flag( vehicle_part::passenger_flag ) ) {
            res.push_back( static_cast<int>( vp.part_index() ) );
        }
    }
    return res;
}

player *vehicle::get_passenger( int p ) const
{
    p = part_with_feature( p, VPFLAG_BOARDABLE, false );
    if( p >= 0 && parts[p].has_flag( vehicle_part::passenger_flag ) ) {
        return g->critter_by_id<player>( parts[p].passenger_id );
    }
    return nullptr;
}

tripoint vehicle::global_pos3() const
{
    return tripoint( smx * SEEX + posx, smy * SEEY + posy, smz );
}

tripoint vehicle::global_part_pos3( const int &index ) const
{
    return global_part_pos3( parts[ index ] );
}

tripoint vehicle::global_part_pos3( const vehicle_part &pt ) const
{
    return global_pos3() + pt.precalc[ 0 ];
}

void vehicle::set_submap_moved( int x, int y )
{
    const point old_msp = g->m.getabs( global_pos3().x, global_pos3().y );
    smx = x;
    smy = y;
    if( !tracking_on ) {
        return;
    }
    overmap_buffer.move_vehicle( this, old_msp );
}

units::mass vehicle::total_mass() const
{
    if( mass_dirty ) {
        refresh_mass();
    }

    return mass_cache;
}

units::volume vehicle::total_folded_volume() const
{
    units::volume m = 0_ml;
    for( const vpart_reference &vp : get_all_parts() ) {
        if( vp.part().removed ) {
            continue;
        }
        m += vp.info().folded_volume;
    }
    return m;
}

const point &vehicle::rotated_center_of_mass() const
{
    // @todo: Bring back caching of this point
    calc_mass_center( true );

    return mass_center_precalc;
}

const point &vehicle::local_center_of_mass() const
{
    if( mass_center_no_precalc_dirty ) {
        calc_mass_center( false );
    }

    return mass_center_no_precalc;
}

point vehicle::pivot_displacement() const
{
    // precalc_mounts always produces a result that puts the pivot point at (0,0).
    // If the pivot point changes, this artificially moves the vehicle, as the position
    // of the old pivot point will appear to move from (posx+0, posy+0) to some other point
    // (posx+dx,posy+dy) even if there is no change in vehicle position or rotation.
    // This method finds that movement so it can be canceled out when actually moving
    // the vehicle.

    // rotate the old pivot point around the new pivot point with the old rotation angle
    point dp;
    coord_translate( pivot_rotation[0], pivot_anchor[1], pivot_anchor[0], dp );
    return dp;
}

int vehicle::fuel_left( const itype_id &ftype, bool recurse ) const
{
    int fl = std::accumulate( parts.begin(), parts.end(), 0, [&ftype]( const int &lhs,
    const vehicle_part & rhs ) {
        // don't count frozen liquid
        if( rhs.is_tank() && rhs.base.contents_made_of( SOLID ) ) {
            return static_cast<long>( lhs );
        }
        return lhs + ( rhs.ammo_current() == ftype ? rhs.ammo_remaining() : 0 );
    } );

    if( recurse && ftype == fuel_type_battery ) {
        auto fuel_counting_visitor = [&]( vehicle const * veh, int amount, int ) {
            return amount + veh->fuel_left( ftype, false );
        };

        // HAX: add 1 to the initial amount so traversal doesn't immediately stop just
        // 'cause we have 0 fuel left in the current vehicle. Subtract the 1 immediately
        // after traversal.
        fl = traverse_vehicle_graph( this, fl + 1, fuel_counting_visitor ) - 1;
    }

    //muscle engines have infinite fuel
    if( ftype == fuel_type_muscle ) {
        // @todo: Allow NPCs to power those
        const optional_vpart_position vp = g->m.veh_at( g->u.pos() );
        bool player_controlling = player_in_control( g->u );

        //if the engine in the player tile is a muscle engine, and player is controlling vehicle
        if( vp && &vp->vehicle() == this && player_controlling ) {
            const int p = avail_part_with_feature( vp->part_index(), VPFLAG_ENGINE, true );
            if( p >= 0 && is_part_on( p ) && part_info( p ).fuel_type == fuel_type_muscle ) {
                //Broken limbs prevent muscle engines from working
                if( ( part_info( p ).has_flag( "MUSCLE_LEGS" ) && g->u.hp_cur[hp_leg_l] > 0 &&
                      g->u.hp_cur[hp_leg_r] > 0 ) || ( part_info( p ).has_flag( "MUSCLE_ARMS" ) &&
                                                       g->u.hp_cur[hp_arm_l] > 0 &&
                                                       g->u.hp_cur[hp_arm_r] > 0 ) ) {
                    fl += 10;
                }
            }
        }
        // As do any other engine flagged as perpetual
    } else if( item( ftype ).has_flag( "PERPETUAL" ) ) {
        fl += 10;
    }

    return fl;
}
int vehicle::fuel_left( const int p, bool recurse ) const
{
    return fuel_left( parts[ p ].fuel_current(), recurse );
}

int vehicle::engine_fuel_left( const int e, bool recurse ) const
{
    if( static_cast<size_t>( e ) < engines.size() ) {
        return fuel_left( parts[ engines[ e ] ].fuel_current(), recurse );
    }
    return 0;
}

int vehicle::fuel_capacity( const itype_id &ftype ) const
{
    return std::accumulate( parts.begin(), parts.end(), 0, [&ftype]( const int &lhs,
    const vehicle_part & rhs ) {
        return lhs + ( rhs.ammo_current() == ftype ? rhs.ammo_capacity() : 0 );
    } );
}

int vehicle::drain( const itype_id &ftype, int amount )
{
    if( ftype == fuel_type_battery ) {
        // Batteries get special handling to take advantage of jumper
        // cables -- discharge_battery knows how to recurse properly
        // (including taking cable power loss into account).
        int remnant = discharge_battery( amount, true );

        // discharge_battery returns amount of charges that were not
        // found anywhere in the power network, whereas this function
        // returns amount of charges consumed; simple subtraction.
        return amount - remnant;
    }

    int drained = 0;
    for( auto &p : parts ) {
        if( amount <= 0 ) {
            break;
        }
        if( p.ammo_current() == ftype ) {
            int qty = p.ammo_consume( amount, global_part_pos3( p ) );
            drained += qty;
            amount -= qty;
        }
    }

    invalidate_mass();
    return drained;
}

int vehicle::drain( const int index, int amount )
{
    if( index < 0 || index >= static_cast<int>( parts.size() ) ) {
        debugmsg( "Tried to drain an invalid part index: %d", index );
        return 0;
    }
    vehicle_part &pt = parts[index];
    if( pt.ammo_current() == fuel_type_battery ) {
        return drain( fuel_type_battery, amount );
    }
    if( !pt.is_tank() || !pt.ammo_remaining() ) {
        debugmsg( "Tried to drain something without any liquid: %s amount: %d ammo: %d",
                  pt.name(), amount, pt.ammo_remaining() );
        return 0;
    }

    const int drained = pt.ammo_consume( amount, global_part_pos3( pt ) );
    invalidate_mass();
    return drained;
}

int vehicle::basic_consumption( const itype_id &ftype ) const
{
    int fcon = 0;
    for( size_t e = 0; e < engines.size(); ++e ) {
        if( is_engine_type_on( e, ftype ) ) {
            if( parts[ engines[e] ].ammo_current() == fuel_type_battery &&
                part_epower_w( engines[e] ) >= 0 ) {
                // Electric engine - use epower instead
                fcon -= part_epower_w( engines[e] );

            } else if( !is_perpetual_type( e ) ) {
                fcon += part_vpower_w( engines[e] );
                if( parts[ e ].faults().count( fault_filter_air ) ) {
                    fcon *= 2;
                }
            }
        }
    }
    return fcon;
}

int vehicle::consumption_per_hour( const itype_id &ftype, int fuel_rate_w ) const
{
    item fuel = item( ftype );
    if( fuel_rate_w == 0 || fuel.has_flag( "PERPETUAL" ) ) {
        return 0;
    }
    // consume this fuel type's share of alternator load for 3600 seconds
    int amount_pct = 3600 * alternator_load / 1000;

    // calculate fuel consumption for the lower of safe speed or 70 mph
    // or 0 if the vehicle is idling
    if( is_moving() ) {
        int target_v = std::min( safe_velocity(), 70 * 100 );
        int vslowdown = slowdown( target_v );
        // add 3600 seconds worth of fuel consumption for the engine
        // HACK - engines consume 1 second worth of fuel per turn, even though a turn is 6 seconds
        if( vslowdown > 0 ) {
            amount_pct += 600 * vslowdown / acceleration( true, target_v );
        }
    }
    int energy_j_per_mL = fuel.fuel_energy() * 1000;
    return -amount_pct * fuel_rate_w / energy_j_per_mL;
}

int vehicle::total_power_w( const bool fueled, const bool safe ) const
{
    int pwr = 0;
    int cnt = 0;

    for( size_t e = 0; e < engines.size(); e++ ) {
        int p = engines[e];
        if( is_engine_on( e ) && ( !fueled || engine_fuel_left( e ) ) ) {
            int m2c = safe ? part_info( engines[e] ).engine_m2c() : 100;
            if( parts[ engines[e] ].faults().count( fault_filter_fuel ) ) {
                m2c *= 0.6;
            }
            pwr += part_vpower_w( p ) * m2c / 100;
            cnt++;
        }
    }

    for( size_t a = 0; a < alternators.size(); a++ ) {
        int p = alternators[a];
        if( is_alternator_on( a ) ) {
            pwr += part_vpower_w( p ); // alternators have negative power
        }
    }
    pwr = std::max( 0, pwr );

    if( cnt > 1 ) {
        pwr = pwr * 4 / ( 4 + cnt - 1 );
    }
    return pwr;
}

bool vehicle::is_moving() const
{
    return velocity != 0;
}

int vehicle::ground_acceleration( const bool fueled, int at_vel_in_vmi ) const
{
    if( !( engine_on || skidding ) ) {
        return 0;
    }
    int target_vmiph = std::max( at_vel_in_vmi, std::max( 1000, max_velocity( fueled ) / 4 ) );
    int cmps = vmiph_to_cmps( target_vmiph );
    int engine_power_ratio = total_power_w( fueled ) / to_kilogram( total_mass() );
    int accel_at_vel = 100 * 100 * engine_power_ratio / cmps;
    add_msg( m_debug, "%s: accel at %d vimph is %d", name, target_vmiph,
             cmps_to_vmiph( accel_at_vel ) );
    return cmps_to_vmiph( accel_at_vel );
}

int vehicle::water_acceleration( const bool fueled, int at_vel_in_vmi ) const
{
    if( !( engine_on || skidding ) ) {
        return 0;
    }
    int target_vmiph = std::max( at_vel_in_vmi, std::max( 1000,
                                 max_water_velocity( fueled ) / 4 ) );
    int cmps = vmiph_to_cmps( target_vmiph );
    int engine_power_ratio = total_power_w( fueled ) / to_kilogram( total_mass() );
    int accel_at_vel = 100 * 100 * engine_power_ratio / cmps;
    add_msg( m_debug, "%s: water accel at %d vimph is %d", name, target_vmiph,
             cmps_to_vmiph( accel_at_vel ) );
    return cmps_to_vmiph( accel_at_vel );
}


// cubic equation solution
// don't use complex numbers unless necessary and it's usually not
// see https://math.vanderbilt.edu/schectex/courses/cubic/ for the gory details
double simple_cubic_solution( double a, double b, double c, double d )
{
    double p = -b / ( 3 * a );
    double q = p * p * p + ( b * c - 3 * a * d ) / ( 6 * a * a );
    double r = c / ( 3 * a );
    double t = r - p * p;
    double tricky_bit = q * q + t * t * t;
    if( tricky_bit < 0 ) {
        double cr = 1.0 / 3.0; // approximate the cube root of a complex number
        std::complex<double> q_complex( q );
        std::complex<double> tricky_complex( std::sqrt( std::complex<double>( tricky_bit ) ) );
        std::complex<double> term1( std::pow( q_complex + tricky_complex, cr ) );
        std::complex<double> term2( std::pow( q_complex - tricky_complex, cr ) );
        std::complex<double> term_sum( term1 + term2 );

        if( imag( term_sum ) < 2 ) {
            return p + real( term_sum ) ;
        } else {
            debugmsg( "cubic solution returned imaginary values" );
            return 0;
        }
    } else {
        double tricky_final = std::sqrt( tricky_bit );
        double term1_part = q + tricky_final;
        double term2_part = q - tricky_final;
        double term1 = std::cbrt( term1_part );
        double term2 = std::cbrt( term2_part );
        return p + term1 + term2;
    }
}

int vehicle::acceleration( const bool fueled, int at_vel_in_vmi ) const
{
    if( is_floating ) {
        return water_acceleration( fueled, at_vel_in_vmi );
    }
    return ground_acceleration( fueled, at_vel_in_vmi );
}

int vehicle::current_acceleration( const bool fueled ) const
{
    return acceleration( fueled, std::abs( velocity ) );
}

// Ugly physics below:
// maximum speed occurs when all available thrust is used to overcome air/rolling resistance
// sigma F = 0 as we were taught in Engineering Mechanics 301
// engine power is torque * rotation rate (in rads for simplicity)
// torque / wheel radius = drive force at where the wheel meets the road
// velocity is wheel radius * rotation rate (in rads for simplicity)
// air resistance is -1/2 * air density * drag coeff * cross area * v^2
//        and c_air_drag is -1/2 * air density * drag coeff * cross area
// rolling resistance is mass * accel_g * rolling coeff * 0.000225 * ( 33.3 + v )
//        and c_rolling_drag is mass * accel_g * rolling coeff * 0.000225
//        and rolling_constant_to_variable is 33.3
// or by formula:
// max velocity occurs when F_drag = F_wheel
// F_wheel = engine_power / rotation_rate / wheel_radius
// velocity = rotation_rate * wheel_radius
// F_wheel * velocity = engine_power * rotation_rate * wheel_radius / rotation_rate / wheel_radius
// F_wheel * velocity = engine_power
// F_wheel = engine_power / velocity
// F_drag = F_air_drag + F_rolling_drag
// F_air_drag = c_air_drag * velocity^2
// F_rolling_drag = c_rolling_drag * velocity + rolling_constant_to_variable * c_rolling_drag
// engine_power / v = c_air_drag * v^2 + c_rolling_drag * v + 33 * c_rolling_drag
// c_air_drag * v^3 + c_rolling_drag * v^2 + c_rolling_drag * 33.3 * v - engine power = 0
// solve for v with the simplified cubic equation solver
// got it? quiz on Wednesday.
int vehicle::max_ground_velocity( const bool fueled ) const
{
    int total_engine_w = total_power_w( fueled );
    double c_rolling_drag = coeff_rolling_drag();
    double max_in_mps = simple_cubic_solution( coeff_air_drag(), c_rolling_drag,
                        c_rolling_drag * vehicles::rolling_constant_to_variable,
                        -total_engine_w );
    add_msg( m_debug, "%s: power %d, c_air %3.2f, c_rolling %3.2f, max_in_mps %3.2f",
             name, total_engine_w, coeff_air_drag(), c_rolling_drag, max_in_mps );
    return mps_to_vmiph( max_in_mps );
}

// the same physics as ground velocity, but there's no rolling resistance so the math is easy
// F_drag = F_water_drag + F_air_drag
// F_drag = c_water_drag * velocity^2 + c_air_drag * velocity^2
// F_drag = ( c_water_drag + c_air_drag ) * velocity^2
// F_prop = engine_power / velocity
// F_prop = F_drag
// engine_power / velocity = ( c_water_drag + c_air_drag ) * velocity^2
// engine_power = ( c_water_drag + c_air_drag ) * velocity^3
// velocity^3 = engine_power / ( c_water_drag + c_air_drag )
// velocity = cube root( engine_power / ( c_water_drag + c_air_drag ) )
int vehicle::max_water_velocity( const bool fueled ) const
{
    int total_engine_w = total_power_w( fueled );
    double total_drag = coeff_water_drag() + coeff_air_drag();
    double max_in_mps = std::cbrt( total_engine_w / total_drag );
    add_msg( m_debug, "%s: power %d, c_air %3.2f, c_water %3.2f, water max_in_mps %3.2f",
             name, total_engine_w, coeff_air_drag(), coeff_water_drag(), max_in_mps );
    return mps_to_vmiph( max_in_mps );
}

int vehicle::max_velocity( const bool fueled ) const
{
    return is_floating ? max_water_velocity( fueled ) : max_ground_velocity( fueled );
}

// the same physics as max_ground_velocity, but with a smaller engine power
int vehicle::safe_ground_velocity( const bool fueled ) const
{
    int effective_engine_w = total_power_w( fueled, true );
    double c_rolling_drag = coeff_rolling_drag();
    double safe_in_mps = simple_cubic_solution( coeff_air_drag(), c_rolling_drag,
                         c_rolling_drag * vehicles::rolling_constant_to_variable,
                         -effective_engine_w );
    return mps_to_vmiph( safe_in_mps );
}

// the same physics as max_water_velocity, but with a smaller engine power
int vehicle::safe_water_velocity( const bool fueled ) const
{
    int total_engine_w = total_power_w( fueled, true );
    double total_drag = coeff_water_drag() + coeff_air_drag();
    double safe_in_mps = std::cbrt( total_engine_w / total_drag );
    return mps_to_vmiph( safe_in_mps );
}

int vehicle::safe_velocity( const bool fueled ) const
{
    return is_floating ? safe_water_velocity( fueled ) : safe_ground_velocity( fueled );
}

bool vehicle::do_environmental_effects()
{
    bool needed = false;
    // check for smoking parts
    for( const vpart_reference &vp : get_all_parts() ) {
        /* Only lower blood level if:
         * - The part is outside.
         * - The weather is any effect that would cause the player to be wet. */
        if( vp.part().blood > 0 && g->m.is_outside( vp.pos() ) ) {
            needed = true;
            if( g->weather >= WEATHER_DRIZZLE && g->weather <= WEATHER_ACID_RAIN ) {
                vp.part().blood--;
            }
        }
    }
    return needed;
}

void vehicle::spew_smoke( double joules, int part, int density )
{
    if( rng( 1, 10000 ) > joules ) {
        return;
    }
    point p = parts[part].mount;
    density = std::max( joules / 10000, double( density ) );
    // Move back from engine/muffler until we find an open space
    while( relative_parts.find( p ) != relative_parts.end() ) {
        p.x += ( velocity < 0 ? 1 : -1 );
    }
    point q = coord_translate( p );
    const tripoint dest = global_pos3() + tripoint( q.x, q.y, 0 );
    g->m.adjust_field_strength( dest, fd_smoke, density );
}

/**
 * Generate noise or smoke from a vehicle with engines turned on
 * load = how hard the engines are working, from 0.0 until 1.0
 * time = how many seconds to generated smoke for
 */
void vehicle::noise_and_smoke( int load, time_duration time )
{
    const std::array<int, 8> sound_levels = {{ 0, 15, 30, 60, 100, 140, 180, INT_MAX }};
    const std::array<std::string, 8> sound_msgs = {{
            _( "hmm" ), _( "hummm!" ), _( "whirrr!" ), _( "vroom!" ), _( "roarrr!" ),
            _( "ROARRR!" ), _( "BRRROARRR!" ), _( "BRUMBRUMBRUMBRUM!" )
        }
    };
    double noise = 0.0;
    double mufflesmoke = 0.0;
    double muffle = 1.0;
    double m = 0.0;
    int exhaust_part = -1;
    for( const vpart_reference &vp : get_avail_parts( "MUFFLER" ) ) {
        m = 1.0 - ( 1.0 - vp.info().bonus / 100.0 ) * vp.part().health_percent();
        if( m < muffle ) {
            muffle = m;
            exhaust_part = int( vp.part_index() );
        }
    }

    bool bad_filter = false;

    for( size_t e = 0; e < engines.size(); e++ ) {
        int p = engines[e];
        if( is_engine_on( e ) &&  engine_fuel_left( e ) ) {
            // convert current engine load to units of watts/40K
            // then spew more smoke and make more noise as the engine load increases
            int part_watts = part_vpower_w( p, true );
            double max_stress = static_cast<double>( part_watts / 40000.0 );
            double cur_stress = load / 1000.0 * max_stress;
            double part_noise = cur_stress * part_info( p ).engine_noise_factor();

            if( part_info( p ).has_flag( "E_COMBUSTION" ) ) {
                double health = parts[p].health_percent();
                if( parts[ p ].base.faults.count( fault_filter_fuel ) ) {
                    health = 0.0;
                }
                if( health < part_info( p ).engine_backfire_threshold() && one_in( 50 + 150 * health ) ) {
                    backfire( e );
                }
                double j = cur_stress * 6 * to_turns<int>( time ) * muffle * 1000;

                if( parts[ p ].base.faults.count( fault_filter_air ) ) {
                    bad_filter = true;
                    j *= j;
                }

                if( ( exhaust_part == -1 ) && engine_on ) {
                    spew_smoke( j, p, bad_filter ? MAX_FIELD_DENSITY : 1 );
                } else {
                    mufflesmoke += j;
                }
                part_noise = ( part_noise + max_stress * 3 + 5 ) * muffle;
            }
            noise = std::max( noise, part_noise ); // Only the loudest engine counts.
        }
    }

    if( ( exhaust_part != -1 ) && engine_on &&
        has_engine_type_not( fuel_type_muscle, true ) ) { // No engine, no smoke
        spew_smoke( mufflesmoke, exhaust_part, bad_filter ? MAX_FIELD_DENSITY : 1 );
    }
    // Cap engine noise to avoid deafening.
    noise = std::min( noise, 100.0 );
    // Even a vehicle with engines off will make noise traveling at high speeds
    noise = std::max( noise, double( fabs( velocity / 500.0 ) ) );
    int lvl = 0;
    if( one_in( 4 ) && rng( 0, 30 ) < noise && has_engine_type_not( fuel_type_muscle, true ) ) {
        while( noise > sound_levels[lvl] ) {
            lvl++;
        }
    }
    sounds::ambient_sound( global_pos3(), noise, sounds::sound_t::movement, sound_msgs[lvl] );
}

int vehicle::wheel_area() const
{
    int total_area = 0;
    for( const int &wheel_index : wheelcache ) {
        total_area += parts[ wheel_index ].wheel_area();
    }

    return total_area;
}

float vehicle::average_or_rating() const
{
    if( wheelcache.empty() ) {
        return 0.0f;
    }
    float total_rating = 0;
    for( const int &wheel_index : wheelcache ) {
        total_rating += part_info( wheel_index ).wheel_or_rating();
    }
    return total_rating / wheelcache.size();
}

static double tile_to_width( int tiles )
{
    if( tiles < 1 ) {
        return 0.1;
    } else if( tiles < 6 ) {
        return 0.5 + 0.4 * tiles;
    } else {
        return 2.5 + 0.15 * ( tiles - 5 );
    }
}

static constexpr int minrow = -122;
static constexpr int maxrow = 122;
struct drag_column {
    int pro = minrow;
    int hboard = minrow;
    int fboard = minrow;
    int aisle = minrow;
    int seat = minrow;
    int exposed = minrow;
    int roof = minrow;
    int shield = minrow;
    int turret = minrow;
    int panel = minrow;
    int windmill = minrow;
    int last = maxrow;
};

double vehicle::coeff_air_drag() const
{
    if( !coeff_air_dirty ) {
        return coefficient_air_resistance;
    }
    constexpr double c_air_base = 0.25;
    constexpr double c_air_mod = 0.1;
    constexpr double base_height = 1.4;
    constexpr double aisle_height = 0.6;
    constexpr double fullboard_height = 0.5;
    constexpr double roof_height = 0.1;
    constexpr double windmill_height = 0.7;

    std::vector<int> structure_indices = all_parts_at_location( part_location_structure );
    int min_x = 0;
    int max_x = 0;
    int min_y = 0;
    int max_y = 0;
    // find how many rows and columns the vehicle has
    for( int p : structure_indices ) {
        min_x = std::min( min_x, parts[p].mount.x );
        max_x = std::max( max_x, parts[p].mount.x );
        min_y = std::min( min_y, parts[p].mount.y );
        max_y = std::max( max_y, parts[p].mount.y );
    }
    int width = max_y - min_y + 1;

    // a mess of lambdas to make the next bit slightly easier to read
    const auto d_exposed = [&]( const vehicle_part & p ) {
        // if it's not inside, it's a center location, and it doesn't need a roof, it's exposed
        if( p.info().location != part_location_center ) {
            return false;
        }
        return !( p.inside || p.info().has_flag( "NO_ROOF_NEEDED" ) ||
                  p.info().has_flag( "WINDSHIELD" ) ||
                  p.info().has_flag( "OPENABLE" ) );
    };

    const auto d_protrusion = [&]( std::vector<int> parts_at ) {
        if( parts_at.size() > 1 ) {
            return false;
        } else {
            return parts[ parts_at.front() ].info().has_flag( "PROTRUSION" );
        }
    };
    const auto d_check_min = [&]( int &value, const vehicle_part & p, bool test ) {
        value = std::min( value, test ? p.mount.x - min_x : maxrow );
    };
    const auto d_check_max = [&]( int &value, const vehicle_part & p, bool test ) {
        value = std::max( value, test ? p.mount.x - min_x : minrow );
    };

    // raycast down each column. the least drag vehicle has halfboard, windshield, seat with roof,
    // windshield, halfboard and is twice as long as it is wide.
    // find the first instance of each item and compare against the ideal configuration.
    std::vector<drag_column> drag( width );
    for( int p : structure_indices ) {
        if( parts[ p ].removed ) {
            continue;
        }
        int col = parts[ p ].mount.y - min_y;
        std::vector<int> parts_at = parts_at_relative( parts[ p ].mount, true );
        d_check_min( drag[ col ].pro, parts[ p ], d_protrusion( parts_at ) );
        for( int pa_index : parts_at ) {
            const vehicle_part &pa = parts[ pa_index ];
            d_check_max( drag[ col ].hboard, pa, pa.info().has_flag( "HALF_BOARD" ) );
            d_check_max( drag[ col ].fboard, pa, pa.info().has_flag( "FULL_BOARD" ) );
            d_check_max( drag[ col ].aisle, pa, pa.info().has_flag( "AISLE" ) );
            d_check_max( drag[ col ].shield, pa, pa.info().has_flag( "WINDSHIELD" ) &&
                         pa.is_available() );
            d_check_max( drag[ col ].seat, pa, pa.info().has_flag( "SEAT" ) ||
                         pa.info().has_flag( "BED" ) );
            d_check_max( drag[ col ].turret, pa, pa.info().location == part_location_onroof &&
                         !pa.info().has_flag( "SOLAR_PANEL" ) );
            d_check_max( drag[ col ].roof, pa, pa.info().has_flag( "ROOF" ) );
            d_check_max( drag[ col ].panel, pa, pa.info().has_flag( "SOLAR_PANEL" ) );
            d_check_max( drag[ col ].windmill, pa, pa.info().has_flag( "WIND_TURBINE" ) );
            d_check_max( drag[ col ].exposed, pa, d_exposed( pa ) );
            d_check_min( drag[ col ].last, pa, pa.info().has_flag( "LOW_FINAL_AIR_DRAG" ) ||
                         pa.info().has_flag( "HALF_BOARD" ) );
        }
    }
    double height = 1;
    double c_air_drag = c_air_base;
    // tally the results of each row, and take the worst height and worst c_air_drag
    for( drag_column &dc : drag ) {
        // even as m_debug you rarely want to see this
        // add_msg( m_debug, "veh %: pro %d, hboard %d, fboard %d, shield %d, seat %d, roof %d, aisle %d, turret %d, panel %d, exposed %d, last %d\n", name, dc.pro, dc.hboard, dc.fboard, dc.shield, dc.seat, dc.roof, dc.aisle, dc.turret, dc.panel, dc.exposed, dc.last );

        double c_air_drag_c = c_air_base;
        // rams in front of the vehicle mildly worsens air drag
        c_air_drag_c += ( dc.pro > dc.hboard ) ? c_air_mod : 0;
        // not having halfboards in front of any windshields or fullboards moderately worsens
        // air drag
        c_air_drag_c += ( std::max( std::max( dc.hboard, dc.fboard ),
                                    dc.shield ) != dc.hboard ) ? 2 * c_air_mod : 0;
        // not having windshields in front of seats severely worsens air drag
        c_air_drag_c += ( dc.shield < dc.seat ) ? 3 * c_air_mod : 0;
        // missing roofs and open doors severely worsen air drag
        c_air_drag_c += ( dc.exposed > minrow ) ? 3 * c_air_mod : 0;
        // being twice as long as wide mildly reduces air drag
        c_air_drag_c -= ( 2 * ( max_x - min_x ) > width ) ? c_air_mod : 0;
        // trunk doors and halfboards at the tail mildly reduce air drag
        c_air_drag_c -= ( dc.last == min_x ) ? c_air_mod : 0;
        // turrets severely worsen air drag
        c_air_drag_c += ( dc.turret > minrow ) ? 3 * c_air_mod : 0;
        // having a windmill is terrible for your drag
        c_air_drag_c += ( dc.windmill > minrow ) ? 5 * c_air_mod : 0;
        c_air_drag = std::max( c_air_drag, c_air_drag_c );
        // vehicles are 1.4m tall
        double c_height = base_height;
        // plus a bit for a roof
        c_height += ( dc.roof > minrow ) ? roof_height : 0;
        // plus a lot for an aisle
        c_height += ( dc.aisle > minrow ) ?  aisle_height : 0;
        // or fullboards
        c_height += ( dc.fboard > minrow ) ? fullboard_height : 0;
        // and a little for anything on the roof
        c_height += ( dc.turret > minrow ) ? 2 * roof_height : 0;
        // solar panels are better than turrets or floodlights, though
        c_height += ( dc.panel > minrow ) ? roof_height : 0;
        // windmills are tall, too
        c_height += ( dc.windmill > minrow ) ? windmill_height : 0;
        height = std::max( height, c_height );
    }
    constexpr double air_density = 1.29; // kg/m^3
    double area = height * tile_to_width( width );
    add_msg( m_debug, "%s: height %3.2fm, width %3.2fm (%d tiles), c_air %3.2f\n", name, height,
             tile_to_width( width ), width, c_air_drag );
    // F_air_drag = c_air_drag * cross_area * 1/2 * air_density * v^2
    // coeff_air_resistance = c_air_drag * cross_area * 1/2 * air_density
    coefficient_air_resistance = std::max( 0.1, c_air_drag * area * 0.5 * air_density );
    coeff_air_dirty = false;
    return coefficient_air_resistance;
}

double vehicle::coeff_rolling_drag() const
{
    if( !coeff_rolling_dirty ) {
        return coefficient_rolling_resistance;
    }
    constexpr double wheel_ratio = 1.25;
    constexpr double base_wheels = 4.0;
    // SAE J2452 measurements are in F_rr = N * C_rr * 0.000225 * ( v + 33.33 )
    // Don't ask me why, but it's the numbers we have. We want N * C_rr * 0.000225 here,
    // and N is mass * accel from gravity (aka weight)
    constexpr double sae_ratio = 0.000225;
    constexpr double newton_ratio = accel_g * sae_ratio;
    double wheel_factor = 0;
    if( wheelcache.empty() ) {
        wheel_factor = 50;
    } else {
        // should really sum the each wheel's c_rolling_resistance * it's share of vehicle mass
        for( auto wheel : wheelcache ) {
            wheel_factor += parts[ wheel ].info().wheel_rolling_resistance();
        }
        // mildly increasing rolling resistance for vehicles with more than 4 wheels and mildly
        // decrease it for vehicles with less
        wheel_factor *=  wheel_ratio /
                         ( base_wheels * wheel_ratio - base_wheels + wheelcache.size() );
    }
    coefficient_rolling_resistance = newton_ratio * wheel_factor * to_kilogram( total_mass() );
    coeff_rolling_dirty = false;
    return coefficient_rolling_resistance;
}

double vehicle::water_draft() const
{
    if( coeff_water_dirty ) {
        coeff_water_drag();
    }
    return draft_m;
}

bool vehicle::can_float() const
{
    if( coeff_water_dirty ) {
        coeff_water_drag();
    }
    // Someday I'll deal with submarines, but now, you can only float if you have freeboard
    return draft_m < hull_height;
}

bool vehicle::is_in_water() const
{
    return is_floating;
}

double vehicle::coeff_water_drag() const
{
    if( !coeff_water_dirty ) {
        return coefficient_water_resistance;
    }
    std::vector<int> structure_indices = all_parts_at_location( part_location_structure );
    if( structure_indices.empty() ) {
        // huh?
        coeff_water_dirty = false;
        hull_height = 0.3;
        draft_m = 1.0;
        return 1250.0;
    }
    double hull_coverage = static_cast<double>( floating.size() ) / structure_indices.size();

    int min_x = 0;
    int max_x = 0;
    int min_y = 0;
    int max_y = 0;
    // find how many rows and columns the vehicle has
    for( int p : structure_indices ) {
        min_x = std::min( min_x, parts[p].mount.x );
        max_x = std::max( max_x, parts[p].mount.x );
        min_y = std::min( min_y, parts[p].mount.y );
        max_y = std::max( max_y, parts[p].mount.y );
    }
    // assume a rectangular footprint instead of doing a stepwise integration by row
    double width = tile_to_width( max_y - min_y + 1 );
    // only count board board tiles to determine area.
    double area =  width * ( max_x - min_x + 1 ) * std::max( 0.1, hull_coverage );
    // treat the hullform as a tetrahedron for half it's length, and a rectangular block
    // for the rest.  the mass of the water displaced by those shapes is equal to the mass
    // of the vehicle (Archimedes principle, eh?) and the volume of that water is the volume
    // of the hull below the waterline divided by the density of water.  apply math to get
    // depth.
    // volume of the block = width * length / 2 * depth
    // volume of the tetrahedron = 1/3 * area of the triangle * depth
    // area of the triangle = 1/2 triangle length * width = 1/2 * length/2 * width
    // volume of the tetrahedron = 1/3 * 1/4 * length * width * depth
    // hull volume underwater = 1/2 * width * length * depth + 1/12 * length * width * depth
    // 7/12 * length * width * depth = hull_volume = water_mass / water density
    // water_mass = vehicle_mass
    // 7/12 * length * width * depth = vehicle_mass / water_density
    // depth = 12/7 * vehicle_mass / water_density / ( length * width )
    constexpr double water_density = 1000.0; // kg/m^3
    draft_m = 12 / 7 * to_kilogram( total_mass() ) / water_density / area;
    // increase the streamlining as more of the boat is covered in boat boards
    double c_water_drag = 1.25 - hull_coverage;
    // hull height starts at 0.3m and goes up as you add more boat boards
    hull_height = 0.3 + 0.5 * hull_coverage;
    // F_water_drag = c_water_drag * cross_area * 1/2 * water_density * v^2
    // coeff_water_resistance = c_water_drag * cross_area * 1/2 * water_density
    coefficient_water_resistance = c_water_drag * width * draft_m * 0.5 * water_density;
    coeff_water_dirty = false;
    return coefficient_water_resistance;
}

float vehicle::k_traction( float wheel_traction_area ) const
{
    if( is_floating ) {
        return can_float() ? 1.0f : -1.0f;
    }

    const float mass_penalty = ( 1.0f - wheel_traction_area / wheel_area() ) *
                               to_kilogram( total_mass() );

    float traction = std::min( 1.0f, wheel_traction_area / mass_penalty );
    add_msg( m_debug, "%s has traction %.2f", name, traction );

    // For now make it easy until it gets properly balanced: add a low cap of 0.1
    return std::max( 0.1f, traction );
}

int vehicle::static_drag( bool actual ) const
{
    return extra_drag / 1000 + ( actual && !engine_on ? 300 : 0 );
}

float vehicle::strain() const
{
    int mv = max_velocity();
    int sv = safe_velocity();
    if( mv <= sv ) {
        mv = sv + 1;
    }
    if( velocity < sv && velocity > -sv ) {
        return 0;
    } else {
        return static_cast<float>( abs( velocity ) - sv ) / static_cast<float>( mv - sv );
    }
}

bool vehicle::sufficient_wheel_config() const
{
    if( wheelcache.empty() ) {
        // No wheels!
        return false;
    } else if( wheelcache.size() == 1 ) {
        //Has to be a stable wheel, and one wheel can only support a 1-3 tile vehicle
        if( !part_info( wheelcache.front() ).has_flag( "STABLE" ) ||
            all_parts_at_location( part_location_structure ).size() > 3 ) {
            return false;
        }
    }
    return true;
}

bool vehicle::balanced_wheel_config() const
{
    int xmin = INT_MAX;
    int ymin = INT_MAX;
    int xmax = INT_MIN;
    int ymax = INT_MIN;
    // find the bounding box of the wheels
    for( auto &w : wheelcache ) {
        const auto &pt = parts[ w ].mount;
        xmin = std::min( xmin, pt.x );
        ymin = std::min( ymin, pt.y );
        xmax = std::max( xmax, pt.x );
        ymax = std::max( ymax, pt.y );
    }

    const point &com = local_center_of_mass();
    if( com.x < xmin || com.x > xmax || com.y < ymin || com.y > ymax ) {
        return false; // center of mass not inside support of wheels (roughly)
    }
    return true;
}

bool vehicle::valid_wheel_config() const
{
    return sufficient_wheel_config() && balanced_wheel_config();
}

float vehicle::steering_effectiveness() const
{
    if( is_floating ) {
        // I'M ON A BOAT
        return can_float() ? 1.0 : 0.0;
    }

    if( steering.empty() ) {
        return -1.0; // No steering installed
    }

    // For now, you just need one wheel working for 100% effective steering.
    // TODO: return something less than 1.0 if the steering isn't so good
    // (unbalanced, long wheelbase, back-heavy vehicle with front wheel steering,
    // etc)
    for( int p : steering ) {
        if( parts[ p ].is_available() ) {
            return 1.0;
        }
    }

    // We have steering, but it's all broken.
    return 0.0;
}

float vehicle::handling_difficulty() const
{
    const float steer = std::max( 0.0f, steering_effectiveness() );
    const float ktraction = k_traction( g->m.vehicle_wheel_traction( *this ) );
    const float aligned = std::max( 0.0f, 1.0f - ( face_vec() - dir_vec() ).magnitude() );

    // TestVehicle: perfect steering, moving on road at 100 mph (25 tiles per turn) = 0.0
    // TestVehicle but on grass (0.75 friction) = 2.5
    // TestVehicle but with bad steering (0.5 steer) = 5
    // TestVehicle but on fungal bed (0.5 friction) and bad steering = 10
    // TestVehicle but turned 90 degrees during this turn (0 align) = 10
    const float diff_mod = ( ( 1.0f - steer ) + ( 1.0f - ktraction ) + ( 1.0f - aligned ) );
    return velocity * diff_mod / vehicles::vmiph_per_tile;
}

std::map<itype_id, int> vehicle::fuel_usage() const
{
    std::map<itype_id, int> ret;
    for( size_t i = 0; i < engines.size(); i++ ) {
        // Note: functions with "engine" in name do NOT take part indices
        // @todo: Use part indices and not engine vector indices
        if( !is_engine_on( i ) ) {
            continue;
        }

        const size_t e = engines[ i ];
        const auto &info = part_info( e );
        static const itype_id null_fuel_type( "null" );
        const itype_id &cur_fuel = parts[ e ].fuel_current();
        if( cur_fuel  == null_fuel_type ) {
            continue;
        }

        if( !is_perpetual_type( i ) ) {
            int usage = info.energy_consumption;
            if( parts[ e ].faults().count( fault_filter_air ) ) {
                usage *= 2;
            }

            ret[ cur_fuel ] += usage;
        }
    }

    return ret;
}

double vehicle::drain_energy( const itype_id &ftype, double energy_j )
{
    double drained = 0.0f;
    for( auto &p : parts ) {
        if( energy_j <= 0.0f ) {
            break;
        }

        double consumed = p.consume_energy( ftype, energy_j );
        drained += consumed;
        energy_j -= consumed;
    }

    invalidate_mass();
    return drained;
}

void vehicle::consume_fuel( int load, const int t_seconds, bool skip_electric )
{
    double st = strain();
    for( auto &fuel_pr : fuel_usage() ) {
        auto &ft = fuel_pr.first;
        if( skip_electric && ft == fuel_type_battery ) {
            continue;
        }

        double amnt_precise_j = static_cast<double>( fuel_pr.second ) * t_seconds;
        amnt_precise_j *= load / 1000.0 * ( 1.0 + st * st * 100.0 );
        double remainder = fuel_remainder[ ft ];
        amnt_precise_j -= remainder;

        if( amnt_precise_j > 0.0f ) {
            fuel_remainder[ ft ] = drain_energy( ft, amnt_precise_j ) - amnt_precise_j;
        } else {
            fuel_remainder[ ft ] = -amnt_precise_j;
        }
    }
    //do this with chance proportional to current load
    // But only if the player is actually there!
    if( load > 0 && one_in( 1000 / load ) && fuel_left( fuel_type_muscle ) > 0 ) {
        //cost proportional to strain
        int mod = 1 + 4 * st;
        //charge bionics when using muscle engine
        if( g->u.has_active_bionic( bionic_id( "bio_torsionratchet" ) ) ) {
            g->u.charge_power( 2 );
            mod = mod * 2;
        }
        if( g->u.has_bionic( bionic_id( "bio_torsionratchet" ) ) ) {
            if( one_in( 20 ) ) {
                g->u.charge_power( 1 );
            }
        }
        if( one_in( 10 ) ) {
            g->u.mod_hunger( mod );
            g->u.mod_thirst( mod );
            g->u.mod_fatigue( mod );
        }
        if( g->u.has_active_bionic( bionic_id( "bio_torsionratchet" ) ) ) {
            g->u.mod_stat( "stamina", -mod * 30 );
        } else {
            g->u.mod_stat( "stamina", -mod * 20 );
        }
    }
}

std::vector<vehicle_part *> vehicle::lights( bool active )
{
    std::vector<vehicle_part *> res;
    for( auto &e : parts ) {
        if( ( !active || e.enabled ) && e.is_available() && e.is_light() ) {
            res.push_back( &e );
        }
    }
    return res;
}

int vehicle::total_epower_w()
{
    int engine_epower = 0;
    return total_epower_w( engine_epower, false );
}

int vehicle::total_epower_w( int &engine_epower, bool skip_solar )
{
    int epower = 0;

    for( const vpart_reference &vp : get_enabled_parts( VPFLAG_ENABLED_DRAINS_EPOWER ) ) {
        epower += vp.info().epower;
    }

    // Engines: can both produce (plasma) or consume (gas, diesel)
    // Gas engines require epower to run for ignition system, ECU, etc.
    // Electric motor consumption not included, see @ref vpart_info::energy_consumption
    if( engine_on ) {
        int engine_vpower = 0;
        for( size_t e = 0; e < engines.size(); ++e ) {
            if( is_engine_on( e ) ) {
                engine_epower += part_epower_w( engines[e] );
                engine_vpower += part_vpower_w( engines[e] );
            }
        }

        epower += engine_epower;

        // Producers of epower

        // If the engine is on, the alternators are working.
        int alternators_epower = 0;
        int alternators_power = 0;
        for( size_t p = 0; p < alternators.size(); ++p ) {
            if( is_alternator_on( p ) ) {
                alternators_epower += part_epower_w( alternators[p] );
                alternators_power += part_vpower_w( alternators[p] );
            }
        }
        if( engine_vpower ) {
            alternator_load = 1000 * abs( alternators_power + extra_drag ) / engine_vpower;
        } else {
            alternator_load = 0;
        }
        // could check if alternator_load > 1000 and then reduce alternator epower,
        // but that's a lot of work for a corner case.
        if( alternators_epower > 0 ) {
            epower += alternators_epower;
        }
    }

    if( skip_solar ) {
        return epower;
    }
    for( int part : solar_panels ) {
        if( parts[ part ].is_unavailable() ) {
            continue;
        }
        epower += part_epower_w( part );
    }
    return epower;
}

int vehicle::total_reactor_epower_w() const
{
    int epower_w = 0;
    for( int elem : reactors ) {
        epower_w += is_part_on( elem ) ? part_epower_w( elem ) : 0;
    }
    return epower_w;
}

void vehicle::power_parts()
{
    int engine_epower = 0;
    int epower = total_epower_w( engine_epower );

    bool reactor_online = false;
    int delta_energy_bat = power_to_energy_bat( epower, 6 * to_turns<int>( 1_turns ) );
    int storage_deficit_bat = std::max( 0, fuel_capacity( fuel_type_battery ) -
                                        fuel_left( fuel_type_battery ) - delta_energy_bat );
    if( !reactors.empty() && storage_deficit_bat > 0 ) {
        // Still not enough surplus epower to fully charge battery
        // Produce additional epower from any reactors
        bool reactor_working = false;
        for( auto &elem : reactors ) {
            // Check whether the reactor is on. If not, move on.
            if( !is_part_on( elem ) ) {
                continue;
            }
            // Keep track whether or not the vehicle has any reactors activated
            reactor_online = true;
            // the amount of energy the reactor generates each turn
            const int gen_energy_bat = power_to_energy_bat( part_epower_w( elem ),
                                       6 * to_turns<int>( 1_turns ) );
            if( parts[ elem ].is_unavailable() ) {
                continue;
            } else if( parts[ elem ].info().has_flag( "PERPETUAL" ) ) {
                reactor_working = true;
                delta_energy_bat += std::min( storage_deficit_bat, gen_energy_bat );
            } else if( parts[elem].ammo_remaining() > 0 ) {
                // Efficiency: one unit of fuel is this many units of battery
                // Note: One battery is 1 kJ
                const int efficiency = part_info( elem ).power;
                const int avail_fuel = parts[elem].ammo_remaining() * efficiency;
                const int elem_energy_bat = std::min( gen_energy_bat, avail_fuel );
                // Cap output at what we can achieve and utilize
                const int reactors_output_bat = std::min( elem_energy_bat, storage_deficit_bat );
                // Fuel consumed in actual units of the resource
                int fuel_consumed = reactors_output_bat / efficiency;
                // Remainder has a chance of resulting in more fuel consumption
                fuel_consumed += x_in_y( reactors_output_bat % efficiency, efficiency ) ? 1 : 0;
                parts[ elem ].ammo_consume( fuel_consumed, global_part_pos3( elem ) );
                reactor_working = true;
                delta_energy_bat += reactors_output_bat;
            }
        }

        if( !reactor_working && reactor_online ) {
            // All reactors out of fuel or destroyed
            for( auto &elem : reactors ) {
                parts[ elem ].enabled = false;
            }
            if( player_in_control( g->u ) || g->u.sees( global_pos3() ) ) {
                add_msg( _( "The %s's reactor dies!" ), name );
            }
        }
    }

    int battery_deficit = 0;
    if( delta_energy_bat > 0 ) {
        // store epower surplus in battery
        charge_battery( delta_energy_bat );
    } else if( epower < 0 ) {
        // draw epower deficit from battery
        battery_deficit = discharge_battery( abs( delta_energy_bat ) );
    }

    if( battery_deficit != 0 ) {
        // Scoops need a special case since they consume power during actual use
        for( const vpart_reference &vp : get_enabled_parts( "SCOOP" ) ) {
            vp.part().enabled = false;
        }
        // Rechargers need special case since they consume power on demand
        for( const vpart_reference &vp : get_enabled_parts( "RECHARGE" ) ) {
            vp.part().enabled = false;
        }

        for( const vpart_reference &vp : get_enabled_parts( VPFLAG_ENABLED_DRAINS_EPOWER ) ) {
            vehicle_part &pt = vp.part();
            if( pt.info().epower < 0 ) {
                pt.enabled = false;
            }
        }

        is_alarm_on = false;
        camera_on = false;
        if( player_in_control( g->u ) || g->u.sees( global_pos3() ) ) {
            add_msg( _( "The %s's battery dies!" ), name );
        }
        if( engine_epower < 0 ) {
            // Not enough epower to run gas engine ignition system
            engine_on = false;
            if( player_in_control( g->u ) || g->u.sees( global_pos3() ) ) {
                add_msg( _( "The %s's engine dies!" ), name );
            }
        }
    }
}

vehicle *vehicle::find_vehicle( const tripoint &where )
{
    // Is it in the reality bubble?
    tripoint veh_local = g->m.getlocal( where );
    if( const optional_vpart_position vp = g->m.veh_at( veh_local ) ) {
        return &vp->vehicle();
    }

    // Nope. Load up its submap...
    point veh_in_sm = point( where.x, where.y );
    point veh_sm = ms_to_sm_remain( veh_in_sm );

    auto sm = MAPBUFFER.lookup_submap( veh_sm.x, veh_sm.y, where.z );
    if( sm == nullptr ) {
        return nullptr;
    }

    for( auto &elem : sm->vehicles ) {
        vehicle *found_veh = elem.get();
        point veh_location( found_veh->posx, found_veh->posy );

        if( veh_in_sm == veh_location ) {
            return found_veh;
        }
    }

    return nullptr;
}

template <typename Func, typename Vehicle>
int vehicle::traverse_vehicle_graph( Vehicle *start_veh, int amount, Func action )
{
    // Breadth-first search! Initialize the queue with a pointer to ourselves and go!
    std::queue< std::pair<Vehicle *, int> > connected_vehs;
    std::set<Vehicle *> visited_vehs;
    connected_vehs.push( std::make_pair( start_veh, 0 ) );

    while( amount > 0 && !connected_vehs.empty() ) {
        auto current_node = connected_vehs.front();
        Vehicle *current_veh = current_node.first;
        int current_loss = current_node.second;

        visited_vehs.insert( current_veh );
        connected_vehs.pop();

        g->u.add_msg_if_player( m_debug, "Traversing graph with %d power", amount );

        for( auto &p : current_veh->loose_parts ) {
            if( !current_veh->part_info( p ).has_flag( "POWER_TRANSFER" ) ) {
                continue; // ignore loose parts that aren't power transfer cables
            }

            auto target_veh = vehicle::find_vehicle( current_veh->parts[p].target.second );
            if( target_veh == nullptr || visited_vehs.count( target_veh ) > 0 ) {
                // Either no destination here (that vehicle's rolled away or off-map) or
                // we've already looked at that vehicle.
                continue;
            }

            // Add this connected vehicle to the queue of vehicles to search next,
            // but only if we haven't seen this one before.
            if( visited_vehs.count( target_veh ) < 1 ) {
                int target_loss = current_loss + current_veh->part_info( p ).epower;
                connected_vehs.push( std::make_pair( target_veh, target_loss ) );

                float loss_amount = ( static_cast<float>( amount ) * static_cast<float>( target_loss ) ) / 100;
                g->u.add_msg_if_player( m_debug, "Visiting remote %p with %d power (loss %f, which is %d percent)",
                                        ( void * )target_veh, amount, loss_amount, target_loss );

                amount = action( target_veh, amount, static_cast<int>( loss_amount ) );
                g->u.add_msg_if_player( m_debug, "After remote %p, %d power", ( void * )target_veh, amount );

                if( amount < 1 ) {
                    break; // No more charge to donate away.
                }
            }
        }
    }
    return amount;
}

int vehicle::charge_battery( int amount, bool include_other_vehicles )
{
    // Key parts by percentage charge level.
    std::multimap<int, vehicle_part *> chargeable_parts;
    for( vehicle_part &p : parts ) {
        if( p.is_available() && p.is_battery() && p.ammo_capacity() > p.ammo_remaining() ) {
            chargeable_parts.insert( { ( p.ammo_remaining() * 100 ) / p.ammo_capacity(), &p } );
        }
    }
    while( amount > 0 && !chargeable_parts.empty() ) {
        // Grab first part, charge until it reaches the next %, then re-insert with new % key.
        auto iter = chargeable_parts.begin();
        int charge_level = iter->first;
        vehicle_part *p = iter->second;
        chargeable_parts.erase( iter );
        // Calculate number of charges to reach the next %, but insure it's at least
        // one more than current charge.
        long next_charge_level = ( ( charge_level + 1 ) * p->ammo_capacity() ) / 100;
        next_charge_level = std::max( next_charge_level, p->ammo_remaining() + 1 );
        long qty = std::min( static_cast<long>( amount ), next_charge_level - p->ammo_remaining() );
        p->ammo_set( fuel_type_battery, p->ammo_remaining() + qty );
        amount -= qty;
        if( p->ammo_capacity() > p->ammo_remaining() ) {
            chargeable_parts.insert( { ( p->ammo_remaining() * 100 ) / p->ammo_capacity(), p } );
        }
    }

    auto charge_visitor = []( vehicle * veh, int amount, int lost ) {
        g->u.add_msg_if_player( m_debug, "CH: %d", amount - lost );
        return veh->charge_battery( amount - lost, false );
    };

    if( amount > 0 && include_other_vehicles ) { // still a bit of charge we could send out...
        amount = traverse_vehicle_graph( this, amount, charge_visitor );
    }

    return amount;
}

int vehicle::discharge_battery( int amount, bool recurse )
{
    // Key parts by percentage charge level.
    std::multimap<int, vehicle_part *> dischargeable_parts;
    for( vehicle_part &p : parts ) {
        if( p.is_available() && p.is_battery() && p.ammo_remaining() > 0 ) {
            dischargeable_parts.insert( { ( p.ammo_remaining() * 100 ) / p.ammo_capacity(), &p } );
        }
    }
    while( amount > 0 && !dischargeable_parts.empty() ) {
        // Grab first part, discharge until it reaches the next %, then re-insert with new % key.
        auto iter = std::prev( dischargeable_parts.end() );
        int charge_level = iter->first;
        vehicle_part *p = iter->second;
        dischargeable_parts.erase( iter );
        // Calculate number of charges to reach the previous %.
        long prev_charge_level = ( ( charge_level - 1 ) * p->ammo_capacity() ) / 100;
        int amount_to_discharge = std::min( p->ammo_remaining() - prev_charge_level,
                                            static_cast<long>( amount ) );
        p->ammo_consume( amount_to_discharge, global_part_pos3( *p ) );
        amount -= amount_to_discharge;
        if( p->ammo_remaining() > 0 ) {
            dischargeable_parts.insert( { ( p->ammo_remaining() * 100 ) / p->ammo_capacity(), p } );
        }
    }

    auto discharge_visitor = []( vehicle * veh, int amount, int lost ) {
        g->u.add_msg_if_player( m_debug, "CH: %d", amount + lost );
        return veh->discharge_battery( amount + lost, false );
    };
    if( amount > 0 && recurse ) { // need more power!
        amount = traverse_vehicle_graph( this, amount, discharge_visitor );
    }

    return amount; // non-zero if we weren't able to fulfill demand.
}

void vehicle::do_engine_damage( size_t e, int strain )
{
    strain = std::min( 25, strain );
    if( is_engine_on( e ) && !is_perpetual_type( e ) &&
        engine_fuel_left( e ) &&  rng( 1, 100 ) < strain ) {
        int dmg = rng( 0, strain * 4 );
        damage_direct( engines[e], dmg );
        if( one_in( 2 ) ) {
            add_msg( _( "Your engine emits a high pitched whine." ) );
        } else {
            add_msg( _( "Your engine emits a loud grinding sound." ) );
        }
    }
}

void vehicle::idle( bool on_map )
{
    power_parts();
    if( engine_on && total_power_w() > 0 ) {
        int idle_rate = alternator_load;
        if( idle_rate < 10 ) {
            idle_rate = 10;    // minimum idle is 1% of full throttle
        }
        consume_fuel( idle_rate, 6 * to_turns<int>( 1_turns ), true );

        if( on_map ) {
            noise_and_smoke( idle_rate, 1_turns );
        }
    } else {
        if( engine_on && g->u.sees( global_pos3() ) &&
            has_engine_type_not( fuel_type_muscle, true ) ) {
            add_msg( _( "The %s's engine dies!" ), name );
        }
        engine_on = false;
    }

    if( !warm_enough_to_plant() ) {
        for( const vpart_reference &vp : get_enabled_parts( "PLANTER" ) ) {
            if( g->u.sees( global_pos3() ) ) {
                add_msg( _( "The %s's planter turns off due to low temperature." ), name );
            }
            vp.part().enabled = false;
        }
    }

    if( !on_map ) {
        return;
    } else {
        update_time( calendar::turn );
    }

    if( has_part( "STEREO", true ) ) {
        play_music();
    }

    if( has_part( "CHIMES", true ) ) {
        play_chimes();
    }

    if( is_alarm_on ) {
        alarm();
    }
}

void vehicle::on_move()
{
    if( has_part( "SCOOP", true ) ) {
        operate_scoop();
    }
    if( has_part( "PLANTER", true ) ) {
        operate_planter();
    }
    if( has_part( "PLOW", true ) ) {
        operate_plow();
    }
    if( has_part( "REAPER", true ) ) {
        operate_reaper();
    }
    if( has_part( "ROCKWHEEL", true ) ) {
        operate_rockwheel();
    }

    occupied_cache_time = calendar::before_time_starts;
}

void vehicle::slow_leak()
{
    // for each badly damaged tanks (lower than 50% health), leak a small amount
    for( auto &p : parts ) {
        auto health = p.health_percent();
        if( health > 0.5 || p.ammo_remaining() <= 0 ) {
            continue;
        }

        auto fuel = p.ammo_current();
        int qty = std::max( ( 0.5 - health ) * ( 0.5 - health ) * p.ammo_remaining() / 10, 1.0 );
        point q = coord_translate( p.mount );
        const tripoint dest = global_pos3() + tripoint( q.x, q.y, 0 );

        // damaged batteries self-discharge without leaking, plutonium leaks slurry
        if( fuel != fuel_type_battery && fuel != fuel_type_plutonium_cell ) {
            item leak( fuel, calendar::turn, qty );
            g->m.add_item_or_charges( dest, leak );
            p.ammo_consume( qty, global_part_pos3( p ) );
        } else if( fuel == fuel_type_plutonium_cell ) {
            item leak( "plut_slurry_dense", calendar::turn, qty );
            if( p.ammo_remaining() >= PLUTONIUM_CHARGES / 10 ) {
                g->m.add_item_or_charges( dest, leak );
                p.ammo_consume( qty * PLUTONIUM_CHARGES / 10, global_part_pos3( p ) );
            } else {
                p.ammo_consume( p.ammo_remaining(), global_part_pos3( p ) );
            }
        } else {
            p.ammo_consume( qty, global_part_pos3( p ) );
        }
    }
}

// total volume of all the things
units::volume vehicle::stored_volume( const int part ) const
{
    return get_items( part ).stored_volume();
}

units::volume vehicle::max_volume( const int part ) const
{
    return get_items( part ).max_volume();
}

units::volume vehicle::free_volume( const int part ) const
{
    return get_items( part ).free_volume();
}

void vehicle::make_active( item_location &loc )
{
    item *target = loc.get_item();
    if( !target->needs_processing() ) {
        return;
    }
    auto cargo_parts = get_parts_at( loc.position(), "CARGO", part_status_flag::any );
    if( cargo_parts.empty() ) {
        return;
    }
    // System insures that there is only one part in this vector.
    vehicle_part *cargo_part = cargo_parts.front();
    auto &item_stack = cargo_part->items;
    auto item_index = std::find_if( item_stack.begin(), item_stack.end(),
    [&target]( const item & i ) {
        return &i == target;
    } );
    active_items.add( item_index, cargo_part->mount );
}

long vehicle::add_charges( int part, const item &itm )
{
    if( !itm.count_by_charges() ) {
        debugmsg( "Add charges was called for an item not counted by charges!" );
        return 0;
    }
    const long ret = get_items( part ).amount_can_fit( itm );
    if( ret == 0 ) {
        return 0;
    }

    item itm_copy = itm;
    itm_copy.charges = ret;
    return add_item( part, itm_copy ) ? ret : 0;
}

bool vehicle::add_item( int part, const item &itm )
{
    if( part < 0 || part >= static_cast<int>( parts.size() ) ) {
        debugmsg( "int part (%d) is out of range", part );
        return false;
    }
    // const int max_weight = ?! // TODO: weight limit, calculation per vpart & vehicle stats, not a hard user limit.
    // add creaking sounds and damage to overloaded vpart, outright break it past a certain point, or when hitting bumps etc

    if( parts[ part ].base.is_gun() ) {
        if( !itm.is_ammo() || itm.ammo_type() != parts[ part ].base.ammo_type() ) {
            return false;
        }
    }
    bool charge = itm.count_by_charges();
    vehicle_stack istack = get_items( part );
    const long to_move = istack.amount_can_fit( itm );
    if( to_move == 0 || ( charge && to_move < itm.charges ) ) {
        return false; // @add_charges should be used in the latter case
    }
    if( charge ) {
        item *here = istack.stacks_with( itm );
        if( here ) {
            invalidate_mass();
            return here->merge_charges( itm );
        }
    }
    return add_item_at( part, parts[part].items.end(), itm );
}

bool vehicle::add_item( vehicle_part &pt, const item &obj )
{
    int idx = index_of_part( &pt );
    if( idx < 0 ) {
        debugmsg( "Tried to add item to invalid part" );
        return false;
    }
    return add_item( idx, obj );
}

bool vehicle::add_item_at( int part, std::list<item>::iterator index, item itm )
{
    if( itm.is_bucket_nonempty() ) {
        for( auto &elem : itm.contents ) {
            g->m.add_item_or_charges( global_part_pos3( part ), elem );
        }

        itm.contents.clear();
    }

    const auto new_pos = parts[part].items.insert( index, itm );
    if( itm.needs_processing() ) {
        active_items.add( new_pos, parts[part].mount );
    }

    invalidate_mass();
    return true;
}

bool vehicle::remove_item( int part, int itemdex )
{
    if( itemdex < 0 || itemdex >= static_cast<int>( parts[part].items.size() ) ) {
        return false;
    }

    remove_item( part, std::next( parts[part].items.begin(), itemdex ) );
    return true;
}

bool vehicle::remove_item( int part, const item *it )
{
    bool rc = false;
    std::list<item> &veh_items = parts[part].items;

    for( auto iter = veh_items.begin(); iter != veh_items.end(); iter++ ) {
        //delete the item if the pointer memory addresses are the same
        if( it == &*iter ) {
            remove_item( part, iter );
            rc = true;
            break;
        }
    }
    return rc;
}

std::list<item>::iterator vehicle::remove_item( int part, std::list<item>::iterator it )
{
    std::list<item> &veh_items = parts[part].items;

    if( active_items.has( it, parts[part].mount ) ) {
        active_items.remove( it, parts[part].mount );
    }

    invalidate_mass();
    return veh_items.erase( it );
}

vehicle_stack vehicle::get_items( const int part )
{
    const tripoint pos = global_part_pos3( part );
    return vehicle_stack( &parts[part].items, point( pos.x, pos.y ), this, part );
}

vehicle_stack vehicle::get_items( const int part ) const
{
    // HACK: callers could modify items through this
    // TODO: a const version of vehicle_stack is needed
    return const_cast<vehicle *>( this )->get_items( part );
}

void vehicle::place_spawn_items()
{
    if( !type.is_valid() ) {
        return;
    }

    for( const auto &pt : type->parts ) {
        if( pt.with_ammo ) {
            int turret = part_with_feature( pt.pos, "TURRET", true );
            if( turret >= 0 && x_in_y( pt.with_ammo, 100 ) ) {
                parts[ turret ].ammo_set( random_entry( pt.ammo_types ), rng( pt.ammo_qty.first,
                                          pt.ammo_qty.second ) );
            }
        }
    }

    for( const auto &spawn : type.obj().item_spawns ) {
        if( rng( 1, 100 ) <= spawn.chance ) {
            int part = part_with_feature( spawn.pos, "CARGO", false );
            if( part < 0 ) {
                debugmsg( "No CARGO parts at (%d, %d) of %s!", spawn.pos.x, spawn.pos.y, name.c_str() );

            } else {
                // if vehicle part is broken only 50% of items spawn and they will be variably damaged
                bool broken = parts[ part ].is_broken();
                if( broken && one_in( 2 ) ) {
                    continue;
                }

                std::vector<item> created;
                for( const itype_id &e : spawn.item_ids ) {
                    created.emplace_back( item( e ).in_its_container() );
                }
                for( const std::string &e : spawn.item_groups ) {
                    item_group::ItemList group_items = item_group::items_from( e, calendar::time_of_cataclysm );
                    for( auto spawn_item : group_items ) {
                        created.emplace_back( spawn_item );
                    }
                }

                for( item &e : created ) {
                    if( e.is_null() ) {
                        continue;
                    }
                    if( broken && e.mod_damage( rng( 1, e.max_damage() ) ) ) {
                        continue; // we destroyed the item
                    }
                    if( e.is_tool() || e.is_gun() || e.is_magazine() ) {
                        bool spawn_ammo = rng( 0, 99 ) < spawn.with_ammo && e.ammo_remaining() == 0;
                        bool spawn_mag  = rng( 0, 99 ) < spawn.with_magazine && !e.magazine_integral() &&
                                          !e.magazine_current();

                        if( spawn_mag ) {
                            e.contents.emplace_back( e.magazine_default(), e.birthday() );
                        }
                        if( spawn_ammo ) {
                            e.ammo_set( e.ammo_type()->default_ammotype() );
                        }
                    }
                    add_item( part, e );
                }
            }
        }
    }
}

void vehicle::gain_moves()
{
    check_falling_or_floating();
    if( is_moving() || is_falling ) {
        if( !loose_parts.empty() ) {
            shed_loose_parts();
        }
        of_turn = 1 + of_turn_carry;
        const int vslowdown = slowdown( velocity );
        if( vslowdown > abs( velocity ) ) {
            if( cruise_on && cruise_velocity ) {
                velocity = velocity > 0 ? 1 : -1;
            } else {
                stop();
            }
        } else if( velocity < 0 ) {
            velocity += vslowdown;
        } else {
            velocity -= vslowdown;
        }
    } else {
        of_turn = .001;
    }
    of_turn_carry = 0;

    // cruise control TODO: enable for NPC?
    if( player_in_control( g->u ) && cruise_on && cruise_velocity != velocity ) {
        thrust( ( cruise_velocity ) > velocity ? 1 : -1 );
    }

    // Force off-map vehicles to load by visiting them every time we gain moves.
    // Shouldn't be too expensive if there aren't fifty trillion vehicles in the graph...
    // ...and if there are, it's the player's fault for putting them there.
    auto nil_visitor = []( vehicle *, int amount, int ) {
        return amount;
    };
    traverse_vehicle_graph( this, 1, nil_visitor );

    if( check_environmental_effects ) {
        check_environmental_effects = do_environmental_effects();
    }

    // turrets which are enabled will try to reload and then automatically fire
    // Turrets which are disabled but have targets set are a special case
    for( auto e : turrets() ) {
        if( e->enabled || e->target.second != e->target.first ) {
            automatic_fire_turret( *e );
        }
    }

    if( velocity < 0 ) {
        beeper_sound();
    }
}

/**
 * Refreshes all caches and refinds all parts. Used after the vehicle has had a part added or removed.
 * Makes indices of different part types so they're easy to find. Also calculates power drain.
 */
void vehicle::refresh()
{
    alternators.clear();
    engines.clear();
    reactors.clear();
    solar_panels.clear();
    wind_turbines.clear();
    funnels.clear();
    relative_parts.clear();
    loose_parts.clear();
    wheelcache.clear();
    steering.clear();
    speciality.clear();
    floating.clear();
    alternator_load = 0;
    extra_drag = 0;
    // Used to sort part list so it displays properly when examining
    struct sort_veh_part_vector {
        vehicle *veh;
        inline bool operator()( const int p1, const int p2 ) {
            return veh->part_info( p1 ).list_order < veh->part_info( p2 ).list_order;
        }
    } svpv = { this };
    std::vector<int>::iterator vii;

    // Main loop over all vehicle parts.
    for( const vpart_reference &vp : get_all_parts() ) {
        const size_t p = vp.part_index();
        const vpart_info &vpi = vp.info();
        if( vp.part().removed ) {
            continue;
        }

        // Build map of point -> all parts in that point
        const point pt = vp.mount();
        // This will keep the parts at point pt sorted
        vii = std::lower_bound( relative_parts[pt].begin(), relative_parts[pt].end(),
                                static_cast<int>( p ), svpv );
        relative_parts[pt].insert( vii, p );

        if( vp.part().is_unavailable() ) {
            continue;
        }
        if( vpi.has_flag( VPFLAG_ALTERNATOR ) ) {
            alternators.push_back( p );
        }
        if( vpi.has_flag( VPFLAG_ENGINE ) ) {
            engines.push_back( p );
        }
        if( vpi.has_flag( VPFLAG_REACTOR ) ) {
            reactors.push_back( p );
        }
        if( vpi.has_flag( VPFLAG_SOLAR_PANEL ) ) {
            solar_panels.push_back( p );
        }
        if( vpi.has_flag( "WIND_TURBINE" ) ) {
            wind_turbines.push_back( p );
        }
        if( vpi.has_flag( "FUNNEL" ) ) {
            funnels.push_back( p );
        }
        if( vpi.has_flag( "UNMOUNT_ON_MOVE" ) ) {
            loose_parts.push_back( p );
        }
        if( vpi.has_flag( VPFLAG_WHEEL ) ) {
            wheelcache.push_back( p );
        }
        if( vpi.has_flag( "STEERABLE" ) || vpi.has_flag( "TRACKED" ) ) {
            // TRACKED contributes to steering effectiveness but
            //  (a) doesn't count as a steering axle for install difficulty
            //  (b) still contributes to drag for the center of steering calculation
            steering.push_back( p );
        }
        if( vpi.has_flag( "SECURITY" ) ) {
            speciality.push_back( p );
        }
        if( vpi.has_flag( VPFLAG_FLOATS ) ) {
            floating.push_back( p );
        }
        if( vp.part().enabled && vpi.has_flag( "EXTRA_DRAG" ) ) {
            extra_drag += vpi.power;
        }
        if( camera_on && vpi.has_flag( "CAMERA" ) ) {
            vp.part().enabled = true;
        } else if( !camera_on && vpi.has_flag( "CAMERA" ) ) {
            vp.part().enabled = false;
        }
    }

    // NB: using the _old_ pivot point, don't recalc here, we only do that when moving!
    precalc_mounts( 0, pivot_rotation[0], pivot_anchor[0] );
    check_environmental_effects = true;
    insides_dirty = true;
    zones_dirty = true;
    invalidate_mass();
}

const point &vehicle::pivot_point() const
{
    if( pivot_dirty ) {
        refresh_pivot();
    }

    return pivot_cache;
}

void vehicle::refresh_pivot() const
{
    // Const method, but messes with mutable fields
    pivot_dirty = false;

    if( wheelcache.empty() || !valid_wheel_config() ) {
        // No usable wheels, use CoM (dragging)
        pivot_cache = local_center_of_mass();
        return;
    }

    // The model here is:
    //
    //  We are trying to rotate around some point (xc,yc)
    //  This produces a friction force / moment from each wheel resisting the
    //  rotation. We want to find the point that minimizes that resistance.
    //
    //  For a given wheel w at (xw,yw), find:
    //   weight(w): a scaling factor for the friction force based on wheel
    //              size, brokenness, steerability/orientation
    //   center_dist: the distance from (xw,yw) to (xc,yc)
    //   centerline_angle: the angle between the X axis and a line through
    //                     (xw,yw) and (xc,yc)
    //
    //  Decompose the force into two components, assuming that the wheel is
    //  aligned along the X axis and we want to apply different weightings to
    //  the in-line vs perpendicular parts of the force:
    //
    //   Resistance force in line with the wheel (X axis)
    //    Fi = weightI(w) * center_dist * sin(centerline_angle)
    //   Resistance force perpendicular to the wheel (Y axis):
    //    Fp = weightP(w) * center_dist * cos(centerline_angle);
    //
    //  Then find the moment that these two forces would apply around (xc,yc)
    //    moment(w) = center_dist * cos(centerline_angle) * Fi +
    //                center_dist * sin(centerline_angle) * Fp
    //
    //  Note that:
    //    cos(centerline_angle) = (xw-xc) / center_dist
    //    sin(centerline_angle) = (yw-yc) / center_dist
    // -> moment(w) = weightP(w)*(xw-xc)^2 + weightI(w)*(yw-yc)^2
    //              = weightP(w)*xc^2 - 2*weightP(w)*xc*xw + weightP(w)*xw^2 +
    //                weightI(w)*yc^2 - 2*weightI(w)*yc*yw + weightI(w)*yw^2
    //
    //  which happily means that the X and Y axes can be handled independently.
    //  We want to minimize sum(moment(w)) due to wheels w=0,1,..., which
    //  occurs when:
    //
    //    sum( 2*xc*weightP(w) - 2*weightP(w)*xw ) = 0
    //     -> xc = (weightP(0)*x0 + weightP(1)*x1 + ...) /
    //             (weightP(0) + weightP(1) + ...)
    //    sum( 2*yc*weightI(w) - 2*weightI(w)*yw ) = 0
    //     -> yc = (weightI(0)*y0 + weightI(1)*y1 + ...) /
    //             (weightI(0) + weightI(1) + ...)
    //
    // so it turns into a fairly simple weighted average of the wheel positions.

    float xc_numerator = 0;
    float xc_denominator = 0;
    float yc_numerator = 0;
    float yc_denominator = 0;

    for( int p : wheelcache ) {
        const auto &wheel = parts[p];

        // @todo: load on tire?
        int contact_area = wheel.wheel_area();
        float weight_i;  // weighting for the in-line part
        float weight_p;  // weighting for the perpendicular part
        if( wheel.is_broken() ) {
            // broken wheels don't roll on either axis
            weight_i = contact_area * 2.0;
            weight_p = contact_area * 2.0;
        } else if( wheel.info().has_flag( "STEERABLE" ) ) {
            // Unbroken steerable wheels can handle motion on both axes
            // (but roll a little more easily inline)
            weight_i = contact_area * 0.1;
            weight_p = contact_area * 0.2;
        } else {
            // Regular wheels resist perpendicular motion
            weight_i = contact_area * 0.1;
            weight_p = contact_area;
        }

        xc_numerator += weight_p * wheel.mount.x;
        yc_numerator += weight_i * wheel.mount.y;
        xc_denominator += weight_p;
        yc_denominator += weight_i;
    }

    if( xc_denominator < 0.1 || yc_denominator < 0.1 ) {
        debugmsg( "vehicle::refresh_pivot had a bad weight: xc=%.3f/%.3f yc=%.3f/%.3f",
                  xc_numerator, xc_denominator, yc_numerator, yc_denominator );
        pivot_cache = local_center_of_mass();
    } else {
        pivot_cache.x = round( xc_numerator / xc_denominator );
        pivot_cache.y = round( yc_numerator / yc_denominator );
    }
}

void vehicle::remove_remote_part( int part_num )
{
    auto veh = find_vehicle( parts[part_num].target.second );

    // If the target vehicle is still there, ask it to remove its part
    if( veh != nullptr ) {
        const tripoint local_abs = g->m.getabs( global_part_pos3( part_num ) );

        for( size_t j = 0; j < veh->loose_parts.size(); j++ ) {
            int remote_partnum = veh->loose_parts[j];
            auto remote_part = &veh->parts[remote_partnum];

            if( veh->part_flag( remote_partnum, "POWER_TRANSFER" ) && remote_part->target.first == local_abs ) {
                veh->remove_part( remote_partnum );
                return;
            }
        }
    }
}

void vehicle::shed_loose_parts()
{
    // remove_part rebuilds the loose_parts vector, when all of those parts have been removed,
    // it will stay empty.
    while( !loose_parts.empty() ) {
        const int elem = loose_parts.front();
        if( part_flag( elem, "POWER_TRANSFER" ) ) {
            remove_remote_part( elem );
        }

        auto part = &parts[elem];
        item drop = part->properties_to_item();
        g->m.add_item_or_charges( global_part_pos3( *part ), drop );

        remove_part( elem );
    }
}

void vehicle::refresh_insides()
{
    if( !insides_dirty ) {
        return;
    }
    insides_dirty = false;
    for( const vpart_reference &vp : get_all_parts() ) {
        const size_t p = vp.part_index();
        if( vp.part().removed ) {
            continue;
        }
        /* If there's no roof, or there is a roof but it's broken, it's outside.
         * (Use short-circuiting && so broken frames don't screw this up) */
        if( !( part_with_feature( p, "ROOF", true ) >= 0 && vp.part().is_available() ) ) {
            vp.part().inside = false;
            continue;
        }

        parts[p].inside = true; // inside if not otherwise
        for( int i = 0; i < 4; i++ ) { // let's check four neighbor parts
            point near_mount = parts[ p ].mount + vehicles::cardinal_d[ i ];
            std::vector<int> parts_n3ar = parts_at_relative( near_mount, true );
            bool cover = false; // if we aren't covered from sides, the roof at p won't save us
            for( auto &j : parts_n3ar ) {
                // another roof -- cover
                if( part_flag( j, "ROOF" ) && parts[ j ].is_available() ) {
                    cover = true;
                    break;
                } else if( part_flag( j, "OBSTACLE" ) && parts[ j ].is_available() ) {
                    // found an obstacle, like board or windshield or door
                    if( parts[j].inside || ( part_flag( j, "OPENABLE" ) && parts[j].open ) ) {
                        continue; // door and it's open -- can't cover
                    }
                    cover = true;
                    break;
                }
                //Otherwise keep looking, there might be another part in that square
            }
            if( !cover ) {
                vp.part().inside = false;
                break;
            }
        }
    }
}

bool vpart_position::is_inside() const
{
    // TODO: this is a bit of a hack as refresh_insides has side effects
    // this should be called elsewhere and not in a function that intends to just query
    // it's also a no-op if the insides are up to date.
    vehicle().refresh_insides();
    return vehicle().parts[part_index()].inside;
}

void vehicle::unboard_all()
{
    std::vector<int> bp = boarded_parts();
    for( auto &i : bp ) {
        g->m.unboard_vehicle( global_part_pos3( i ) );
    }
}

int vehicle::damage( int p, int dmg, damage_type type, bool aimed )
{
    if( dmg < 1 ) {
        return dmg;
    }

    std::vector<int> pl = parts_at_relative( parts[p].mount, true );
    if( pl.empty() ) {
        // We ran out of non removed parts at this location already.
        return dmg;
    }

    if( !aimed ) {
        bool found_obs = false;
        for( auto &i : pl ) {
            if( part_flag( i, "OBSTACLE" ) &&
                ( !part_flag( i, "OPENABLE" ) || !parts[i].open ) ) {
                found_obs = true;
                break;
            }
        }

        if( !found_obs ) { // not aimed at this tile and no obstacle here -- fly through
            return dmg;
        }
    }

    int target_part = random_entry( pl );

    // door motor mechanism is protected by closed doors
    if( part_flag( target_part, "DOOR_MOTOR" ) ) {
        // find the most strong openable that is not open
        int strongest_door_part = -1;
        int strongest_door_durability = INT_MIN;
        for( int part : pl ) {
            if( part_flag( part, "OPENABLE" ) && !parts[part].open ) {
                int door_durability = part_info( part ).durability;
                if( door_durability > strongest_door_durability ) {
                    strongest_door_part = part;
                    strongest_door_durability = door_durability;
                }
            }
        }

        // if we found a closed door, target it instead of the door_motor
        if( strongest_door_part != -1 ) {
            target_part = strongest_door_part;
        }
    }

    int damage_dealt;

    int armor_part = part_with_feature( p, "ARMOR", true );
    if( armor_part < 0 ) {
        // Not covered by armor -- damage part
        damage_dealt = damage_direct( target_part, dmg, type );
    } else {
        // Covered by armor -- hit both armor and part, but reduce damage by armor's reduction
        int protection = part_info( armor_part ).damage_reduction[ type ];
        // Parts on roof aren't protected
        bool overhead = part_flag( target_part, "ROOF" ) || part_info( target_part ).location == "on_roof";
        // Calling damage_direct may remove the damaged part
        // completely, therefore the other index (target_part) becomes
        // wrong if target_part > armor_part.
        // Damaging the part with the higher index first is save,
        // as removing a part only changes indices after the
        // removed part.
        if( armor_part < target_part ) {
            damage_direct( target_part, overhead ? dmg : dmg - protection, type );
            damage_dealt = damage_direct( armor_part, dmg, type );
        } else {
            damage_dealt = damage_direct( armor_part, dmg, type );
            damage_direct( target_part, overhead ? dmg : dmg - protection, type );
        }
    }

    return damage_dealt;
}

void vehicle::damage_all( int dmg1, int dmg2, damage_type type, const point &impact )
{
    if( dmg2 < dmg1 ) {
        std::swap( dmg1, dmg2 );
    }

    if( dmg1 < 1 ) {
        return;
    }

    for( const vpart_reference &vp : get_all_parts() ) {
        const size_t p = vp.part_index();
        int distance = 1 + square_dist( vp.mount().x, vp.mount().y, impact.x, impact.y );
        if( distance > 1 && part_info( p ).location == part_location_structure &&
            !part_info( p ).has_flag( "PROTRUSION" ) ) {
            damage_direct( p, rng( dmg1, dmg2 ) / ( distance * distance ), type );
        }
    }
}

/**
 * Shifts all parts of the vehicle by the given amounts, and then shifts the
 * vehicle itself in the opposite direction. The end result is that the vehicle
 * appears to have not moved. Useful for re-zeroing a vehicle to ensure that a
 * (0, 0) part is always present.
 * @param delta How much to shift along each axis
 */
void vehicle::shift_parts( const point &delta )
{
    // Don't invalidate the active item cache's location!
    active_items.subtract_locations( delta );
    for( auto &elem : parts ) {
        elem.mount -= delta;
    }

    decltype( labels ) new_labels;
    for( auto &l : labels ) {
        new_labels.insert( label( l - delta, l.text ) );
    }
    labels = new_labels;

    decltype( loot_zones ) new_zones;
    for( auto const &z : loot_zones ) {
        new_zones.emplace( z.first - delta, z.second );
    }
    loot_zones = new_zones;

    pivot_anchor[0] -= delta;
    refresh();
    //Need to also update the map after this
    g->m.reset_vehicle_cache( smz );
}

/**
 * Detect if the vehicle is currently missing a 0,0 part, and
 * adjust if necessary.
 * @return bool true if the shift was needed.
 */
bool vehicle::shift_if_needed()
{
    std::vector<int> vehicle_origin = parts_at_relative( point_zero, true );
    if( !vehicle_origin.empty() && !parts[ vehicle_origin[ 0 ] ].removed ) {
        // Shifting is not needed.
        return false;
    }
    //Find a frame, any frame, to shift to
    for( const vpart_reference &vp : get_all_parts() ) {
        if( vp.info().location == "structure"
            && !vp.has_feature( "PROTRUSION" )
            && !vp.part().removed ) {
            shift_parts( vp.mount() );
            refresh();
            return true;
        }
    }
    // There are only parts with PROTRUSION left, choose one of them.
    for( const vpart_reference &vp : get_all_parts() ) {
        if( !vp.part().removed ) {
            shift_parts( vp.mount() );
            refresh();
            return true;
        }
    }
    return false;
}

int vehicle::break_off( int p, int dmg )
{
    /* Already-destroyed part - chance it could be torn off into pieces.
     * Chance increases with damage, and decreases with part max durability
     * (so lights, etc are easily removed; frames and plating not so much) */
    if( rng( 0, part_info( p ).durability / 10 ) >= dmg ) {
        return dmg;
    }

    const tripoint pos = global_part_pos3( p );
    const auto scatter_parts = [&]( const vehicle_part & pt ) {
        for( const item &piece : pt.pieces_for_broken_part() ) {
            // inside the loop, so each piece goes to a different place
            // @todo this may spawn items behind a wall
            const tripoint where = random_entry( g->m.points_in_radius( pos, SCATTER_DISTANCE ) );
            // TODO: balance audit, ensure that less pieces are generated than one would need
            // to build the component (smash a vehicle box that took 10 lumps of steel,
            // find 12 steel lumps scattered after atom-smashing it with a tree trunk)
            g->m.add_item_or_charges( where, piece );
        }
    };
    if( part_info( p ).location == part_location_structure ) {
        // For structural parts, remove other parts first
        std::vector<int> parts_in_square = parts_at_relative( parts[p].mount, true );
        for( int index = parts_in_square.size() - 1; index >= 0; index-- ) {
            // Ignore the frame being destroyed
            if( parts_in_square[index] == p ) {
                continue;
            }

            if( parts[ parts_in_square[ index ] ].is_broken() ) {
                // Tearing off a broken part - break it up
                if( g->u.sees( pos ) ) {
                    add_msg( m_bad, _( "The %s's %s breaks into pieces!" ), name,
                             parts[ parts_in_square[ index ] ].name() );
                }
                scatter_parts( parts[parts_in_square[index]] );
            } else {
                // Intact (but possibly damaged) part - remove it in one piece
                if( g->u.sees( pos ) ) {
                    add_msg( m_bad, _( "The %1$s's %2$s is torn off!" ), name,
                             parts[ parts_in_square[ index ] ].name() );
                }
                item part_as_item = parts[parts_in_square[index]].properties_to_item();
                g->m.add_item_or_charges( pos, part_as_item );
            }
            remove_part( parts_in_square[index] );
        }
        // After clearing the frame, remove it.
        if( g->u.sees( pos ) ) {
            add_msg( m_bad, _( "The %1$s's %2$s is destroyed!" ), name, parts[ p ].name() );
        }
        scatter_parts( parts[p] );
        remove_part( p );
        find_and_split_vehicles( p );
    } else {
        //Just break it off
        if( g->u.sees( pos ) ) {
            add_msg( m_bad, _( "The %1$s's %2$s is destroyed!" ), name, parts[ p ].name() );
        }

        scatter_parts( parts[p] );
        remove_part( p );
    }

    return dmg;
}

bool vehicle::explode_fuel( int p, damage_type type )
{
    const itype_id &ft = part_info( p ).fuel_type;
    item fuel = item( ft );
    if( !fuel.has_explosion_data() ) {
        return false;
    }
    fuel_explosion data = fuel.get_explosion_data();

    if( parts[ p ].is_broken() ) {
        leak_fuel( parts[ p ] );
    }

    int explosion_chance = type == DT_HEAT ? data.explosion_chance_hot : data.explosion_chance_cold;
    if( one_in( explosion_chance ) ) {
        g->u.add_memorial_log( pgettext( "memorial_male", "The fuel tank of the %s exploded!" ),
                               pgettext( "memorial_female", "The fuel tank of the %s exploded!" ),
                               name.c_str() );
        const int pow = 120 * ( 1 - exp( data.explosion_factor / -5000 *
                                         ( parts[p].ammo_remaining() * data.fuel_size_factor ) ) );
        //debugmsg( "damage check dmg=%d pow=%d amount=%d", dmg, pow, parts[p].amount );

        g->explosion( global_part_pos3( p ), pow, 0.7, data.fiery_explosion );
        mod_hp( parts[p], 0 - parts[ p ].hp(), DT_HEAT );
        parts[p].ammo_unset();
    }

    return true;
}

int vehicle::damage_direct( int p, int dmg, damage_type type )
{
    // Make sure p is within range and hasn't been removed already
    if( ( static_cast<size_t>( p ) >= parts.size() ) || parts[p].removed ) {
        return dmg;
    }
    g->m.set_memory_seen_cache_dirty( global_part_pos3( p ) );
    if( parts[p].is_broken() ) {
        return break_off( p, dmg );
    }

    int tsh = std::min( 20, part_info( p ).durability / 10 );
    if( dmg < tsh && type != DT_TRUE ) {
        if( type == DT_HEAT && parts[p].is_fuel_store() ) {
            explode_fuel( p, type );
        }

        return dmg;
    }

    dmg -= std::min<int>( dmg, part_info( p ).damage_reduction[ type ] );
    int dres = dmg - parts[p].hp();
    if( mod_hp( parts[ p ], 0 - dmg, type ) ) {
        insides_dirty = true;
        pivot_dirty = true;

        // destroyed parts lose any contained fuels, battery charges or ammo
        leak_fuel( parts [ p ] );

        for( const auto &e : parts[p].items ) {
            g->m.add_item_or_charges( global_part_pos3( p ), e );
        }
        parts[p].items.clear();

        invalidate_mass();
        coeff_air_changed = true;
    }

    if( parts[p].is_fuel_store() ) {
        explode_fuel( p, type );
    } else if( parts[ p ].is_broken() && part_flag( p, "UNMOUNT_ON_DAMAGE" ) ) {
        g->m.spawn_item( global_part_pos3( p ), part_info( p ).item, 1, 0, calendar::turn );
        remove_part( p );
    }

    return std::max( dres, 0 );
}

void vehicle::leak_fuel( vehicle_part &pt )
{
    // only liquid fuels from non-empty tanks can leak out onto map tiles
    if( !pt.is_tank() || pt.ammo_remaining() <= 0 ) {
        return;
    }

    // leak in random directions but prefer closest tiles and avoid walls or other obstacles
    auto tiles = closest_tripoints_first( 1, global_part_pos3( pt ) );
    tiles.erase( std::remove_if( tiles.begin(), tiles.end(), []( const tripoint & e ) {
        return !g->m.passable( e );
    } ), tiles.end() );

    // leak up to 1/3 of remaining fuel per iteration and continue until the part is empty
    auto *fuel = item::find_type( pt.ammo_current() );
    while( !tiles.empty() && pt.ammo_remaining() ) {
        int qty = pt.ammo_consume( rng( 0, std::max( pt.ammo_remaining() / 3, 1L ) ),
                                   global_part_pos3( pt ) );
        if( qty > 0 ) {
            g->m.add_item_or_charges( random_entry( tiles ), item( fuel, calendar::turn, qty ) );
        }
    }

    pt.ammo_unset();
}

std::map<itype_id, long> vehicle::fuels_left() const
{
    std::map<itype_id, long> result;
    for( const auto &p : parts ) {
        if( p.is_fuel_store() && p.ammo_current() != "null" ) {
            result[ p.ammo_current() ] += p.ammo_remaining();
        }
    }
    return result;
}

bool vehicle::is_foldable() const
{
    for( const vpart_reference &vp : get_all_parts() ) {
        if( !vp.has_feature( "FOLDABLE" ) ) {
            return false;
        }
    }
    return true;
}

bool vehicle::restore( const std::string &data )
{
    std::istringstream veh_data( data );
    try {
        JsonIn json( veh_data );
        parts.clear();
        json.read( parts );
    } catch( const JsonError &e ) {
        debugmsg( "Error restoring vehicle: %s", e.c_str() );
        return false;
    }
    refresh();
    face.init( 0 );
    turn_dir = 0;
    turn( 0 );
    precalc_mounts( 0, pivot_rotation[0], pivot_anchor[0] );
    precalc_mounts( 1, pivot_rotation[1], pivot_anchor[1] );
    last_update = calendar::turn;
    return true;
}

std::set<tripoint> &vehicle::get_points( const bool force_refresh )
{
    if( force_refresh || occupied_cache_time != calendar::turn ) {
        occupied_cache_time = calendar::turn;
        occupied_points.clear();
        for( const auto &p : parts ) {
            occupied_points.insert( global_part_pos3( p ) );
        }
    }

    return occupied_points;
}

vehicle_part &vpart_reference::part() const
{
    assert( part_index() < vehicle().parts.size() );
    return vehicle().parts[part_index()];
}

const vpart_info &vpart_reference::info() const
{
    return part().info();
}

point vpart_position::mount() const
{
    return vehicle().parts[part_index()].mount;
}

tripoint vpart_position::pos() const
{
    return vehicle().global_part_pos3( part_index() );
}

bool vpart_reference::has_feature( const std::string &f ) const
{
    return info().has_flag( f );
}

bool vpart_reference::has_feature( const vpart_bitflags f ) const
{
    return info().has_flag( f );
}

inline int modulo( int v, int m )
{
    // C++11: negative v and positive m result in negative v%m (or 0),
    // but this is supposed to be mathematical modulo: 0 <= v%m < m,
    const int r = v % m;
    // Adding m in that (and only that) case.
    return r >= 0 ? r : r + m;
}

bool is_sm_tile_outside( const tripoint &real_global_pos )
{
    const tripoint smp = ms_to_sm_copy( real_global_pos );
    const int px = modulo( real_global_pos.x, SEEX );
    const int py = modulo( real_global_pos.y, SEEY );
    auto sm = MAPBUFFER.lookup_submap( smp );
    if( sm == nullptr ) {
        debugmsg( "is_sm_tile_outside(): couldn't find submap %d,%d,%d", smp.x, smp.y, smp.z );
        return false;
    }

    if( px < 0 || px >= SEEX || py < 0 || py >= SEEY ) {
        debugmsg( "err %d,%d", px, py );
        return false;
    }

    return !( sm->ter[px][py].obj().has_flag( TFLAG_INDOORS ) ||
              sm->get_furn( { px, py } ).obj().has_flag( TFLAG_INDOORS ) );
}

void vehicle::update_time( const time_point &update_to )
{
    if( smz < 0 ) {
        return;
    }

    const time_point update_from = last_update;
    if( update_to < update_from ) {
        // Special case going backwards in time - that happens
        last_update = update_to;
        return;
    }

    if( update_to >= update_from && update_to - update_from < 1_minutes ) {
        // We don't need to check every turn
        return;
    }
    time_duration elapsed = update_to - last_update;
    last_update = update_to;

    // Weather stuff, only for z-levels >= 0
    // TODO: Have it wash cars from blood?
    if( funnels.empty() && solar_panels.empty() && wind_turbines.empty() ) {
        return;
    }

    // Get one weather data set per vehicle, they don't differ much across vehicle area
    auto accum_weather = sum_conditions( update_from, update_to, g->m.getabs( global_pos3() ) );
    // make some reference objects to use to check for reload
    const item water( "water" );
    const item water_clean( "water_clean" );

    for( int idx : funnels ) {
        const auto &pt = parts[idx];

        // we need an unbroken funnel mounted on the exterior of the vehicle
        if( pt.is_unavailable() || !is_sm_tile_outside( g->m.getabs( global_part_pos3( pt ) ) ) ) {
            continue;
        }

        // we need an empty tank (or one already containing water) below the funnel
        auto tank = std::find_if( parts.begin(), parts.end(), [&]( const vehicle_part & e ) {
            return pt.mount == e.mount && e.is_tank() &&
                   ( e.can_reload( water ) || e.can_reload( water_clean ) );
        } );

        if( tank == parts.end() ) {
            continue;
        }

        double area = pow( pt.info().size / units::legacy_volume_factor, 2 ) * M_PI;
        int qty = divide_roll_remainder( funnel_charges_per_turn( area, accum_weather.rain_amount ), 1.0 );
        int c_qty = qty + ( tank->can_reload( water_clean ) ?  tank->ammo_remaining() : 0 );
        int cost_to_purify = c_qty * item::find_type( "water_purifier" )->charges_to_use();

        if( qty > 0 ) {
            if( has_part( global_part_pos3( pt ), "WATER_PURIFIER", true ) &&
                ( fuel_left( "battery" ) > cost_to_purify ) ) {
                tank->ammo_set( "water_clean", c_qty );
                discharge_battery( cost_to_purify );
            } else {
                tank->ammo_set( "water", tank->ammo_remaining() + qty );
            }
            invalidate_mass();
        }
    }

    if( !solar_panels.empty() ) {
        int epower_w = 0;
        for( int part : solar_panels ) {
            if( parts[ part ].is_unavailable() ) {
                continue;
            }

            if( !is_sm_tile_outside( g->m.getabs( global_part_pos3( part ) ) ) ) {
                continue;
            }

            epower_w += part_epower_w( part );
        }
        double intensity = accum_weather.sunlight / DAYLIGHT_LEVEL / to_turns<double>( elapsed );
        int energy_bat = power_to_energy_bat( epower_w * intensity, 6 * to_turns<int>( elapsed ) );
        if( energy_bat > 0 ) {
            add_msg( m_debug, "%s got %d kJ energy from solar panels", name, energy_bat );
            charge_battery( energy_bat );
        }
    }
    if( !wind_turbines.empty() ) {
        const oter_id &cur_om_ter = overmap_buffer.ter( g->m.getabs( global_pos3() ) );
        const w_point weatherPoint = *g->weather_precise;
        double basewindpower = weatherPoint.windpower;
        double windpower;
        int epower_w = 0;
        for( int part : wind_turbines ) {
            if( parts[ part ].is_unavailable() ) {
                continue;
            }

            if( !is_sm_tile_outside( g->m.getabs( global_part_pos3( part ) ) ) ) {
                continue;
            }

            windpower = get_local_windpower( basewindpower, cur_om_ter, global_part_pos3( part ),
                                             weatherPoint.winddirection, false );
            if( windpower <= ( basewindpower / 10 ) ) {
                continue;
            }
            epower_w += part_epower_w( part ) * windpower;
        }
        int energy_bat = power_to_energy_bat( epower_w, 6 * to_turns<int>( elapsed ) );
        if( energy_bat > 0 ) {
            add_msg( m_debug, "%s got %d kJ energy from wind turbines", name, energy_bat );
            charge_battery( energy_bat );
        }
    }
}

void vehicle::invalidate_mass()
{
    mass_dirty = true;
    mass_center_precalc_dirty = true;
    mass_center_no_precalc_dirty = true;
    // Anything that affects mass will also affect the pivot
    pivot_dirty = true;
    coeff_rolling_dirty = true;
    coeff_water_dirty = true;
}

void vehicle::refresh_mass() const
{
    calc_mass_center( true );
}

void vehicle::calc_mass_center( bool use_precalc ) const
{
    units::quantity<float, units::mass::unit_type> xf = 0;
    units::quantity<float, units::mass::unit_type> yf = 0;
    units::mass m_total = 0_gram;
    for( const vpart_reference &vp : get_all_parts() ) {
        const size_t i = vp.part_index();
        if( vp.part().removed ) {
            continue;
        }

        units::mass m_part = 0_gram;
        m_part += vp.part().base.weight();
        for( const auto &j : get_items( i ) ) {
            //m_part += j.type->weight;
            // Change back to the above if it runs too slowly
            m_part += j.weight();
        }

        if( vp.has_feature( VPFLAG_BOARDABLE ) && vp.part().has_flag( vehicle_part::passenger_flag ) ) {
            const player *p = get_passenger( i );
            // Sometimes flag is wrongly set, don't crash!
            m_part += p != nullptr ? p->get_weight() : 0_gram;
        }

        if( vp.part().has_flag( vehicle_part::animal_flag ) ) {
            int animal_mass = vp.part().base.get_var( "weight", 0 );
            m_part += units::from_gram( animal_mass );
        }

        if( use_precalc ) {
            xf += vp.part().precalc[0].x * m_part;
            yf += vp.part().precalc[0].y * m_part;
        } else {
            xf += vp.mount().x * m_part;
            yf += vp.mount().y * m_part;
        }

        m_total += m_part;
    }

    mass_cache = m_total;
    mass_dirty = false;

    const float x = xf / mass_cache;
    const float y = yf / mass_cache;
    if( use_precalc ) {
        mass_center_precalc.x = round( x );
        mass_center_precalc.y = round( y );
        mass_center_precalc_dirty = false;
    } else {
        mass_center_no_precalc.x = round( x );
        mass_center_no_precalc.y = round( y );
        mass_center_no_precalc_dirty = false;
    }
}

bounding_box vehicle::get_bounding_box()
{
    int min_x = INT_MAX;
    int max_x = INT_MIN;
    int min_y = INT_MAX;
    int max_y = INT_MIN;

    face.init( turn_dir );

    precalc_mounts( 0, turn_dir, point() );

    int i_use = 0;
    for( const tripoint &p : get_points( true ) ) {
        const point pt = parts[part_at( point( p.x, p.y ) )].precalc[i_use];
        if( pt.x < min_x ) {
            min_x = pt.x;
        }
        if( pt.x > max_x ) {
            max_x = pt.x;
        }
        if( pt.y < min_y ) {
            min_y = pt.y;
        }
        if( pt.y > max_y ) {
            max_y = pt.y;
        }
    }
    bounding_box b;
    b.p1 = point( min_x, min_y );
    b.p2 = point( max_x, max_y );
    return b;
}

vehicle_part_range vehicle::get_all_parts() const
{
    return vehicle_part_range( const_cast<vehicle &>( *this ) );
}

bool vehicle::refresh_zones()
{
    if( zones_dirty ) {
        decltype( loot_zones ) new_zones;
        for( auto const &z : loot_zones ) {
            zone_data zone = z.second;
            //Get the global position of the first cargo part at the relative coordinate

            const int part_idx = part_with_feature( z.first, "CARGO", false );
            if( part_idx == -1 ) {
                debugmsg( "Could not find cargo part at %d,%d on vehicle %s for loot zone. Removing loot zone.",
                          z.first.x, z.first.y, this->name.c_str() );

                // If this loot zone refers to a part that no longer exists at this location, then its unattached somehow.
                // By continuing here and not adding to new_zones, we effectively remove it
                continue;
            }
            tripoint zone_pos = global_part_pos3( part_idx );
            zone_pos = g->m.getabs( zone_pos );
            //Set the position of the zone to that part
            zone.set_position( std::pair<tripoint, tripoint>( zone_pos, zone_pos ), false );
            new_zones.emplace( z.first, zone );
        }
        loot_zones = new_zones;
        zones_dirty = false;
        return true;
    }
    return false;
}

template<>
bool vehicle_part_with_feature_range<std::string>::matches( const size_t part ) const
{
    const vehicle_part &vp = this->vehicle().parts[part];
    return vp.info().has_flag( feature_ ) &&
           !vp.removed &&
           ( !( part_status_flag::working & required_ ) || !vp.is_broken() ) &&
           ( !( part_status_flag::available & required_ ) || vp.is_available() ) &&
           ( !( part_status_flag::enabled & required_ ) || vp.enabled );
}

template<>
bool vehicle_part_with_feature_range<vpart_bitflags>::matches( const size_t part ) const
{
    const vehicle_part &vp = this->vehicle().parts[part];
    return vp.info().has_flag( feature_ ) &&
           !vp.removed &&
           ( !( part_status_flag::working & required_ ) || !vp.is_broken() ) &&
           ( !( part_status_flag::available & required_ ) || vp.is_available() ) &&
           ( !( part_status_flag::enabled & required_ ) || vp.enabled );
}
