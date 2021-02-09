#include "vehicle.h" // IWYU pragma: associated
#include "vpart_position.h" // IWYU pragma: associated
#include "vpart_range.h" // IWYU pragma: associated

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstdint>
#include <cstdlib>
#include <list>
#include <memory>
#include <numeric>
#include <queue>
#include <set>
#include <sstream>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

#include "activity_type.h"
#include "avatar.h"
#include "bionics.h"
#include "cata_assert.h"
#include "cata_utility.h"
#include "character.h"
#include "clzones.h"
#include "colony.h"
#include "coordinate_conversions.h"
#include "creature.h"
#include "cuboid_rectangle.h"
#include "debug.h"
#include "enum_traits.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "explosion.h"
#include "faction.h"
#include "field_type.h"
#include "flag.h"
#include "game.h"
#include "item.h"
#include "item_contents.h"
#include "item_group.h"
#include "item_pocket.h"
#include "itype.h"
#include "json.h"
#include "make_static.h"
#include "map.h"
#include "map_iterator.h"
#include "mapbuffer.h"
#include "mapdata.h"
#include "material.h"
#include "math_defines.h"
#include "messages.h"
#include "monster.h"
#include "move_mode.h"
#include "npc.h"
#include "options.h"
#include "output.h"
#include "overmapbuffer.h"
#include "pimpl.h"
#include "player.h"
#include "player_activity.h"
#include "ret_val.h"
#include "rng.h"
#include "sounds.h"
#include "string_formatter.h"
#include "submap.h"
#include "translations.h"
#include "units_utility.h"
#include "value_ptr.h"
#include "veh_type.h"
#include "vehicle_selector.h"
#include "weather.h"
#include "weather_gen.h"
#include "weather_type.h"

/*
 * Speed up all those if ( blarg == "structure" ) statements that are used everywhere;
 *   assemble "structure" once here instead of repeatedly later.
 */
static const std::string part_location_structure( "structure" );
static const std::string part_location_center( "center" );
static const std::string part_location_onroof( "on_roof" );

static const itype_id fuel_type_animal( "animal" );
static const itype_id fuel_type_battery( "battery" );
static const itype_id fuel_type_muscle( "muscle" );
static const itype_id fuel_type_plutonium_cell( "plut_cell" );
static const itype_id fuel_type_wind( "wind" );
static const itype_id fuel_type_mana( "mana" );

static const fault_id fault_engine_immobiliser( "fault_engine_immobiliser" );

static const activity_id ACT_VEHICLE( "ACT_VEHICLE" );

static const bionic_id bio_jointservo( "bio_jointservo" );

static const proficiency_id proficiency_prof_aircraft_mechanic( "prof_aircraft_mechanic" );

static const efftype_id effect_harnessed( "harnessed" );
static const efftype_id effect_winded( "winded" );

static const itype_id itype_battery( "battery" );
static const itype_id itype_plut_cell( "plut_cell" );
static const itype_id itype_water( "water" );
static const itype_id itype_water_clean( "water_clean" );

static const itype_id itype_water_purifier( "water_purifier" );

static const std::string flag_E_COMBUSTION( "E_COMBUSTION" );

static bool is_sm_tile_outside( const tripoint &real_global_pos );
static bool is_sm_tile_over_water( const tripoint &real_global_pos );

// 1 kJ per battery charge
static const int bat_energy_j = 1000;

// For reference what each function is supposed to do, see their implementation in
// @ref DefaultRemovePartHandler. Add compatible code for it into @ref MapgenRemovePartHandler,
// if needed.
class RemovePartHandler
{
    public:
        virtual ~RemovePartHandler() = default;

        virtual void unboard( const tripoint &loc ) = 0;
        virtual void add_item_or_charges( const tripoint &loc, item it, bool permit_oob ) = 0;
        virtual void set_transparency_cache_dirty( int z ) = 0;
        virtual void set_floor_cache_dirty( int z ) = 0;
        virtual void removed( vehicle &veh, int part ) = 0;
        virtual void spawn_animal_from_part( item &base, const tripoint &loc ) = 0;
};

class DefaultRemovePartHandler : public RemovePartHandler
{
    public:
        ~DefaultRemovePartHandler() override = default;

        void unboard( const tripoint &loc ) override {
            get_map().unboard_vehicle( loc );
        }
        void add_item_or_charges( const tripoint &loc, item it, bool /*permit_oob*/ ) override {
            get_map().add_item_or_charges( loc, std::move( it ) );
        }
        void set_transparency_cache_dirty( const int z ) override {
            map &here = get_map();
            here.set_transparency_cache_dirty( z );
            here.set_seen_cache_dirty( tripoint_zero );
        }
        void set_floor_cache_dirty( const int z ) override {
            get_map().set_floor_cache_dirty( z );
        }
        void removed( vehicle &veh, const int part ) override {
            avatar &player_character = get_avatar();
            // If the player is currently working on the removed part, stop them as it's futile now.
            const player_activity &act = player_character.activity;
            map &here = get_map();
            if( act.id() == ACT_VEHICLE && act.moves_left > 0 && act.values.size() > 6 ) {
                if( veh_pointer_or_null( here.veh_at( tripoint( act.values[0], act.values[1],
                                                      player_character.posz() ) ) ) == &veh ) {
                    if( act.values[6] >= part ) {
                        player_character.cancel_activity();
                        add_msg( m_info, _( "The vehicle part you were working on has gone!" ) );
                    }
                }
            }
            // TODO: maybe do this for all the nearby NPCs as well?

            if( player_character.get_grab_type() == object_type::VEHICLE &&
                player_character.pos() + player_character.grab_point == veh.global_part_pos3( part ) ) {
                if( veh.parts_at_relative( veh.part( part ).mount, false ).empty() ) {
                    add_msg( m_info, _( "The vehicle part you were holding has been destroyed!" ) );
                    player_character.grab( object_type::NONE );
                }
            }

            here.dirty_vehicle_list.insert( &veh );
        }
        void spawn_animal_from_part( item &base, const tripoint &loc ) override {
            base.release_monster( loc, 1 );
        }
};

class MapgenRemovePartHandler : public RemovePartHandler
{
    private:
        map &m;

    public:
        explicit MapgenRemovePartHandler( map &m ) : m( m ) { }

        ~MapgenRemovePartHandler() override = default;

        void unboard( const tripoint &/*loc*/ ) override {
            debugmsg( "Tried to unboard during mapgen!" );
            // Ignored. Will almost certainly not be called anyway, because
            // there are no creatures that could have been mounted during mapgen.
        }
        void add_item_or_charges( const tripoint &loc, item it, bool permit_oob ) override {
            if( !m.inbounds( loc ) ) {
                if( !permit_oob ) {
                    debugmsg( "Tried to put item %s on invalid tile %s during mapgen!",
                              it.tname(), loc.to_string() );
                }
                tripoint copy = loc;
                m.clip_to_bounds( copy );
                cata_assert( m.inbounds( copy ) ); // prevent infinite recursion
                add_item_or_charges( copy, std::move( it ), false );
                return;
            }
            m.add_item_or_charges( loc, std::move( it ) );
        }
        void set_transparency_cache_dirty( const int /*z*/ ) override {
            // Ignored for now. We don't initialize the transparency cache in mapgen anyway.
        }
        void set_floor_cache_dirty( const int /*z*/ ) override {
            // Ignored for now. We don't initialize the floor cache in mapgen anyway.
        }
        void removed( vehicle &veh, const int /*part*/ ) override {
            // TODO: check if this is necessary, it probably isn't during mapgen
            m.dirty_vehicle_list.insert( &veh );
        }
        void spawn_animal_from_part( item &/*base*/, const tripoint &/*loc*/ ) override {
            debugmsg( "Tried to spawn animal from vehicle part during mapgen!" );
            // Ignored. The base item will not be changed and will spawn as is:
            // still containing the animal.
            // This should not happend during mapgen anyway.
            // TODO: *if* this actually happens: create a spawn point for the animal instead.
        }
};

// Vehicle stack methods.
vehicle_stack::iterator vehicle_stack::erase( vehicle_stack::const_iterator it )
{
    return myorigin->remove_item( part_num, it );
}

void vehicle_stack::insert( const item &newitem )
{
    myorigin->add_item( part_num, newitem );
}

units::volume vehicle_stack::max_volume() const
{
    if( myorigin->part_flag( part_num, "CARGO" ) && !myorigin->part( part_num ).is_broken() ) {
        // Set max volume for vehicle cargo to prevent integer overflow
        return std::min( myorigin->part( part_num ).info().size, 10000_liter );
    }
    return 0_ml;
}

// Vehicle class methods.

vehicle::vehicle( const vproto_id &type_id, int init_veh_fuel,
                  int init_veh_status ): type( type_id )
{
    turn_dir = 0_degrees;
    face.init( 0_degrees );
    move.init( 0_degrees );
    of_turn_carry = 0;

    if( !type.str().empty() && type.is_valid() ) {
        const vehicle_prototype &proto = type.obj();
        // Copy the already made vehicle. The blueprint is created when the json data is loaded
        // and is guaranteed to be valid (has valid parts etc.).
        *this = *proto.blueprint;
        // The game language may have changed after the blueprint was created,
        // so translated the prototype name again.
        name = proto.name.translated();
        init_state( init_veh_fuel, init_veh_status );
    }
    precalc_mounts( 0, pivot_rotation[0], pivot_anchor[0] );
    refresh();
}

vehicle::vehicle() : vehicle( vproto_id() )
{
    sm_pos = tripoint_zero;
}

vehicle::~vehicle() = default;

bool vehicle::player_in_control( const Character &p ) const
{
    // Debug switch to prevent vehicles from skidding
    // without having to place the player in them.
    if( tags.count( "IN_CONTROL_OVERRIDE" ) ) {
        return true;
    }

    const optional_vpart_position vp = get_map().veh_at( p.pos() );
    if( vp && &vp->vehicle() == this &&
        p.controlling_vehicle &&
        ( ( part_with_feature( vp->part_index(), "CONTROL_ANIMAL", true ) >= 0 &&
            has_engine_type( fuel_type_animal, false ) && has_harnessed_animal() ) ||
          ( part_with_feature( vp->part_index(), VPFLAG_CONTROLS, false ) >= 0 ) )
      ) {
        return true;
    }

    return remote_controlled( p );
}

bool vehicle::remote_controlled( const Character &p ) const
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

void vehicle::init_state( int init_veh_fuel, int init_veh_status )
{
    // vehicle parts excluding engines are by default turned off
    for( vehicle_part &pt : parts ) {
        pt.enabled = pt.is_engine();
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
    last_update = calendar::turn_zero;

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
        //chance decays from 1 in 4 vehicles on day 0 to 1 in (day + 4) in the future.
        int current_day = std::max( to_days<int>( calendar::turn - calendar::turn_zero ), 0 );
        if( veh_fuel_mult > 0 && !empty( get_avail_parts( "ENGINE" ) ) &&
            one_in( current_day + 4 ) && !destroyEngine && !has_no_key &&
            has_engine_type_not( fuel_type_muscle, true ) ) {
            engine_on = true;
        }

        bool light_head  = one_in( 20 );
        bool light_whead  = one_in( 20 ); // wide-angle headlight
        bool light_dome  = one_in( 16 );
        bool light_aisle = one_in( 8 );
        bool light_hoverh = one_in( 4 ); // half circle overhead light
        bool light_overh = one_in( 4 );
        bool light_atom  = one_in( 2 );
        for( vehicle_part &pt : parts ) {
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

        if( pt.is_reactor() ) {
            const ammotype plut( "plutonium" );
            if( veh_fuel_mult == 100 ) { // Mint condition vehicle
                pt.ammo_set( itype_plut_cell );
            } else if( one_in( 2 ) && veh_fuel_mult > 0 ) { // Randomize charge a bit
                pt.ammo_set( itype_plut_cell, pt.ammo_capacity( plut ) * ( veh_fuel_mult + rng( 0, 10 ) ) / 100 );
            } else if( one_in( 2 ) && veh_fuel_mult > 0 ) {
                pt.ammo_set( itype_plut_cell, pt.ammo_capacity( plut ) * ( veh_fuel_mult - rng( 0, 10 ) ) / 100 );
            } else {
                pt.ammo_set( itype_plut_cell, pt.ammo_capacity( plut ) * veh_fuel_mult / 100 );
            }
        }

        if( pt.is_battery() ) {
            const ammotype battery( "battery" );
            if( veh_fuel_mult == 100 ) { // Mint condition vehicle
                pt.ammo_set( itype_battery );
            } else if( one_in( 2 ) && veh_fuel_mult > 0 ) { // Randomize battery ammo a bit
                pt.ammo_set( itype_battery, pt.ammo_capacity( battery ) * ( veh_fuel_mult + rng( 0, 10 ) ) / 100 );
            } else if( one_in( 2 ) && veh_fuel_mult > 0 ) {
                pt.ammo_set( itype_battery, pt.ammo_capacity( battery ) * ( veh_fuel_mult - rng( 0, 10 ) ) / 100 );
            } else {
                pt.ammo_set( itype_battery, pt.ammo_capacity( battery ) * veh_fuel_mult / 100 );
            }
        }

        if( !type->parts[p].fuel.is_null() ) {
            const itype *loaded = item::find_type( type->parts[p].fuel );
            const ammotype loaded_ammotype = loaded->ammo->type;
            if( pt.is_tank() ) {
                int qty = pt.ammo_capacity( loaded_ammotype ) * veh_fuel_mult / 100;
                qty *= std::max( loaded->stack_size, 1 );
                qty /= to_milliliter( units::legacy_volume_factor );
                pt.ammo_set( type->parts[p].fuel, qty );
            } else if( pt.is_fuel_store() ) {
                int qty = pt.ammo_capacity( loaded_ammotype ) * veh_fuel_mult / 100;
                pt.ammo_set( type->parts[p].fuel, qty );
            }
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
                    set_hp( pt, ( roll - broken ) / static_cast<double>( unhurt - broken ) * vp.info().durability );
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
                pt.fault_set( fault_engine_immobiliser );
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

    for( size_t i = 0; i < engines.size(); i++ ) {
        auto_select_fuel( i );
    }

    invalidate_mass();
}

void vehicle::activate_magical_follow()
{
    for( vehicle_part &vp : parts ) {
        if( vp.info().fuel_type == fuel_type_mana ) {
            vp.enabled = true;
            is_following = true;
            engine_on = true;
        } else {
            vp.enabled = true;
        }
    }
    refresh();
}

void vehicle::activate_animal_follow()
{
    for( size_t e = 0; e < parts.size(); e++ ) {
        vehicle_part &vp = parts[ e ];
        if( vp.info().fuel_type == fuel_type_animal ) {
            monster *mon = get_monster( e );
            if( mon && mon->has_effect( effect_harnessed ) ) {
                vp.enabled = true;
                is_following = true;
                engine_on = true;
            }
        } else {
            vp.enabled = true;
        }
    }
    refresh();
}

void vehicle::autopilot_patrol()
{
    /** choose one single zone ( multiple zones too complex for now )
     * choose a point at the far edge of the zone
     * the edge chosen is the edge that is smaller, therefore the longer side
     * of the rectangle is the one the vehicle drives mostly parallel too.
     * if its  perfect square then choose a point that is on any edge that the
     * vehicle is not currently at
     * drive to that point.
     * then once arrived, choose a random opposite point of the zone.
     * this should ( in a simple fashion ) cause a patrolling behavior
     * in a criss-cross fashion.
     * in an auto-tractor, this would eventually cover the entire rectangle.
     */
    map &here = get_map();
    // if we are close to a waypoint, then return to come back to this function next turn.
    if( autodrive_local_target != tripoint_zero ) {
        if( rl_dist( here.getabs( global_pos3() ), autodrive_local_target ) <= 3 ) {
            autodrive_local_target = tripoint_zero;
            return;
        }
        if( !here.inbounds( here.getlocal( autodrive_local_target ) ) ) {
            autodrive_local_target = tripoint_zero;
            is_patrolling = false;
            return;
        }
        drive_to_local_target( autodrive_local_target, false );
        return;
    }
    zone_manager &mgr = zone_manager::get_manager();
    const auto &zone_src_set = mgr.get_near( zone_type_id( "VEHICLE_PATROL" ),
                               here.getabs( global_pos3() ), 60 );
    if( zone_src_set.empty() ) {
        is_patrolling = false;
        return;
    }
    // get corners.
    tripoint min;
    tripoint max;
    for( const tripoint &box : zone_src_set ) {
        if( min == tripoint_zero ) {
            min = box;
            max = box;
            continue;
        }
        min.x = std::min( box.x, min.x );
        min.y = std::min( box.y, min.y );
        min.z = std::min( box.z, min.z );
        max.x = std::max( box.x, max.x );
        max.y = std::max( box.y, max.y );
        max.z = std::max( box.z, max.z );
    }
    const bool x_side = ( max.x - min.x ) < ( max.y - min.y );
    const int point_along = x_side ? rng( min.x, max.x ) : rng( min.y, max.y );
    const tripoint max_tri = x_side ? tripoint( point_along, max.y, min.z ) : tripoint( max.x,
                             point_along,
                             min.z );
    const tripoint min_tri = x_side ? tripoint( point_along, min.y, min.z ) : tripoint( min.x,
                             point_along,
                             min.z );
    tripoint chosen_tri = min_tri;
    if( rl_dist( max_tri, here.getabs( global_pos3() ) ) >= rl_dist( min_tri,
            here.getabs( global_pos3() ) ) ) {
        chosen_tri = max_tri;
    }
    autodrive_local_target = chosen_tri;
    drive_to_local_target( autodrive_local_target, false );
}

std::set<point> vehicle::immediate_path( const units::angle &rotate )
{
    std::set<point> points_to_check;
    const int distance_to_check = 10 + ( velocity / 800 );
    units::angle adjusted_angle = normalize( face.dir() + rotate );
    // clamp to multiples of 15.
    adjusted_angle = round_to_multiple_of( adjusted_angle, 15_degrees );
    tileray collision_vector;
    collision_vector.init( adjusted_angle );
    map &here = get_map();
    point top_left_actual = global_pos3().xy() + coord_translate( front_left );
    point top_right_actual = global_pos3().xy() + coord_translate( front_right );
    std::vector<point> front_row = line_to( here.getabs( top_left_actual ),
                                            here.getabs( top_right_actual ) );
    for( const point &elem : front_row ) {
        for( int i = 0; i < distance_to_check; ++i ) {
            collision_vector.advance( i );
            point point_to_add = elem + point( collision_vector.dx(), collision_vector.dy() );
            points_to_check.emplace( point_to_add );
        }
    }
    collision_check_points = points_to_check;
    return points_to_check;
}

static int get_turn_from_angle( const units::angle angle, const tripoint &vehpos,
                                const tripoint &target, bool reverse = false )
{
    if( angle > 10.0_degrees && angle <= 45.0_degrees ) {
        return reverse ? 4 : 1;
    } else if( angle > 45.0_degrees && angle <= 90.0_degrees ) {
        return 3;
    } else if( angle > 90.0_degrees && angle < 180.0_degrees ) {
        return reverse ? 1 : 4;
    } else if( angle < -10.0_degrees && angle >= -45.0_degrees ) {
        return reverse ? -4 : -1;
    } else if( angle < -45.0_degrees && angle >= -90.0_degrees ) {
        return -3;
    } else if( angle < -90.0_degrees && angle > -180.0_degrees ) {
        return reverse ? -1 : -4;
        // edge case of being exactly on the button for the target.
        // just keep driving, the next path point will be picked up.
    } else if( ( angle == 180_degrees || angle == -180_degrees ) && vehpos == target ) {
        return 0;
    }
    return 0;
}

void vehicle::stop_autodriving()
{
    if( !is_autodriving && !is_patrolling && !is_following ) {
        return;
    }
    if( velocity > 0 ) {
        if( is_patrolling || is_following ) {
            autodrive( point( 0, 10 ) );
        } else {
            pldrive( point( 0, 10 ) );
        }
    }
    is_autodriving = false;
    is_patrolling = false;
    is_following = false;
    autopilot_on = false;
    autodrive_local_target = tripoint_zero;
    collision_check_points.clear();
}

void vehicle::drive_to_local_target( const tripoint &target, bool follow_protocol )
{
    Character &player_character = get_player_character();
    if( follow_protocol && player_character.in_vehicle ) {
        stop_autodriving();
        return;
    }
    refresh();
    map &here = get_map();
    tripoint vehpos = here.getabs( global_pos3() );
    units::angle angle = get_angle_from_targ( target );
    // now we got the angle to the target, we can work out when we are heading towards disaster.
    // Check the tileray in the direction we need to head towards.
    std::set<point> points_to_check = immediate_path( angle );
    bool stop = false;
    for( const point &pt_elem : points_to_check ) {
        point elem = here.getlocal( pt_elem );
        if( stop ) {
            break;
        }
        const optional_vpart_position ovp = here.veh_at( tripoint( elem, sm_pos.z ) );
        if( here.impassable_ter_furn( tripoint( elem, sm_pos.z ) ) || ( ovp &&
                &ovp->vehicle() != this ) ) {
            stop = true;
            break;
        }
        if( elem == player_character.pos().xy() ) {
            if( follow_protocol || player_character.in_vehicle ) {
                continue;
            } else {
                stop = true;
                break;
            }
        }
        bool its_a_pet = false;
        if( g->critter_at( tripoint( elem, sm_pos.z ) ) ) {
            npc *guy = g->critter_at<npc>( tripoint( elem, sm_pos.z ) );
            if( guy && !guy->in_vehicle ) {
                stop = true;
                break;
            }
            for( const vehicle_part &p : parts ) {
                monster *mon = get_monster( index_of_part( &p ) );
                if( mon && mon->pos().xy() == elem ) {
                    its_a_pet = true;
                    break;
                }
            }
            if( !its_a_pet ) {
                stop = true;
                break;
            }
        }
    }
    if( stop ) {
        if( autopilot_on ) {
            sounds::sound( global_pos3(), 30, sounds::sound_t::alert,
                           string_format( _( "the %s emitting a beep and saying \"Obstacle detected!\"" ),
                                          name ) );
        }
        if( velocity > 0 ) {
            follow_protocol ||
            is_patrolling ? autodrive( point( 0, 10 ) ) : pldrive( point( 0, 10 ) );
        }
        stop_autodriving();
        return;
    }
    int turn_x = get_turn_from_angle( angle, vehpos, target );
    int accel_y = 0;
    // best to cruise around at a safe velocity or 40mph, whichever is lowest
    // accelerate when it doesn't need to turn.
    // when following player, take distance to player into account.
    // we really want to avoid running the player over.
    // If its a helicopter, we dont need to worry about airborne obstacles so much
    // And fuel efficiency is terrible at low speeds.
    const int safe_player_follow_speed = 400 *
                                         player_character.current_movement_mode()->move_speed_mult();
    if( follow_protocol ) {
        if( ( ( turn_x > 0 || turn_x < 0 ) && velocity > safe_player_follow_speed ) ||
            rl_dist( vehpos, here.getabs( player_character.pos() ) ) < 7 + ( ( mount_max.y * 3 ) + 4 ) ) {
            accel_y = 1;
        }
        if( ( velocity < std::min( safe_velocity(), safe_player_follow_speed ) && turn_x == 0 &&
              rl_dist( vehpos, here.getabs( player_character.pos() ) ) > 8 + ( ( mount_max.y * 3 ) + 4 ) ) ||
            velocity < 100 ) {
            accel_y = -1;
        }
    } else {
        if( ( turn_x > 0 || turn_x < 0 ) && velocity > 1000 ) {
            accel_y = 1;
        }
        if( ( velocity < std::min( safe_velocity(), is_rotorcraft() &&
                                   is_flying_in_air() ? 12000 : 32 * 100 ) && turn_x == 0 ) || velocity < 500 ) {
            accel_y = -1;
        }
        if( is_patrolling && velocity > 400 ) {
            accel_y = 1;
        }
    }
    follow_protocol ||
    is_patrolling ? autodrive( point( turn_x, accel_y ) ) : pldrive( point( turn_x, accel_y ) );
}

units::angle vehicle::get_angle_from_targ( const tripoint &targ )
{
    tripoint vehpos = get_map().getabs( global_pos3() );
    rl_vec2d facevec = face_vec();
    point rel_pos_target = targ.xy() - vehpos.xy();
    rl_vec2d targetvec = rl_vec2d( rel_pos_target.x, rel_pos_target.y );
    // cross product
    double crossy = ( facevec.x * targetvec.y ) - ( targetvec.x * facevec.y );
    // dot product.
    double dotx = ( facevec.x * targetvec.x ) + ( facevec.y * targetvec.y );

    return units::atan2( crossy, dotx );
}

void vehicle::do_autodrive()
{
    if( omt_path.empty() ) {
        stop_autodriving();
    }
    Character &player_character = get_player_character();
    map &here = get_map();
    tripoint vehpos = global_pos3();
    // TODO: fix point types
    tripoint_abs_omt veh_omt_pos( ms_to_omt_copy( here.getabs( vehpos ) ) );
    // we're at or close to the waypoint, pop it out and look for the next one.
    if( ( is_autodriving && !player_character.omt_path.empty() && !omt_path.empty() ) &&
        veh_omt_pos == omt_path.back() ) {
        player_character.omt_path.pop_back();
        omt_path.pop_back();
    }
    if( omt_path.empty() ) {
        stop_autodriving();
        return;
    }

    point_rel_omt omt_diff = omt_path.back().xy() - veh_omt_pos.xy();
    if( omt_diff.x() > 3 || omt_diff.x() < -3 || omt_diff.y() > 3 || omt_diff.y() < -3 ) {
        // we've gone walkabout somehow, call off the whole thing
        stop_autodriving();
        return;
    }
    point side;
    if( omt_diff.x() > 0 ) {
        side.x = 2 * SEEX - 1;
    } else if( omt_diff.x() < 0 ) {
        side.x = 0;
    } else {
        side.x = SEEX;
    }
    if( omt_diff.y() > 0 ) {
        side.y = 2 * SEEY - 1;
    } else if( omt_diff.y() < 0 ) {
        side.y = 0;
    } else {
        side.y = SEEY;
    }
    // get the shared border mid-point of the next path omt
    tripoint_abs_ms global_a = project_to<coords::ms>( veh_omt_pos );
    // TODO: fix point types
    tripoint autodrive_temp_target = ( global_a.raw() + tripoint( side,
                                       sm_pos.z ) - here.getabs( vehpos ) ) + vehpos;
    autodrive_local_target = here.getabs( autodrive_temp_target );
    drive_to_local_target( autodrive_local_target, false );
}

/**
 * Smashes up a vehicle that has already been placed; used for generating
 * very damaged vehicles. Additionally, any spot where two vehicles overlapped
 * (i.e., any spot with multiple frames) will be completely destroyed, as that
 * was the collision point.
 */
void vehicle::smash( map &m, float hp_percent_loss_min, float hp_percent_loss_max,
                     float percent_of_parts_to_affect, point damage_origin, float damage_size )
{
    for( vehicle_part &part : parts ) {
        //Skip any parts already mashed up or removed.
        if( part.is_broken() || part.removed ) {
            continue;
        }

        std::vector<int> parts_in_square = parts_at_relative( part.mount, true );
        int structures_found = 0;
        for( int &square_part_index : parts_in_square ) {
            if( part_info( square_part_index ).location == part_location_structure ) {
                structures_found++;
            }
        }

        if( structures_found > 1 ) {
            //Destroy everything in the square
            for( int idx : parts_in_square ) {
                mod_hp( parts[ idx ], 0 - parts[ idx ].hp(), damage_type::BASH );
                parts[ idx ].ammo_unset();
            }
            continue;
        }

        int roll = dice( 1, 1000 );
        int pct_af = ( percent_of_parts_to_affect * 1000.0f );
        if( roll < pct_af ) {
            double dist =  damage_size == 0.0f ? 1.0f :
                           clamp( 1.0f - trig_dist( damage_origin, part.precalc[0].xy() ) /
                                  damage_size, 0.0f, 1.0f );
            //Everywhere else, drop by 10-120% of max HP (anything over 100 = broken)
            if( mod_hp( part, 0 - ( rng_float( hp_percent_loss_min * dist,
                                               hp_percent_loss_max * dist ) *
                                    part.info().durability ), damage_type::BASH ) ) {
                part.ammo_unset();
            }
        }
    }

    std::unique_ptr<RemovePartHandler> handler_ptr;
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
            const vpart_info &p_info = part_info( p );
            const vpart_info &other_p_info = part_info( other_p );

            if( p_info.get_id() == other_p_info.get_id() ||
                ( !p_info.location.empty() && p_info.location == other_p_info.location ) ) {
                // Deferred creation of the handler to here so it is only created when actually needed.
                if( !handler_ptr ) {
                    // This is a heuristic: we just assume the default handler is good enough when called
                    // on the main game map. And assume that we run from some mapgen code if called on
                    // another instance.
                    if( g && &get_map() == &m ) {
                        handler_ptr = std::make_unique<DefaultRemovePartHandler>();
                    } else {
                        handler_ptr = std::make_unique<MapgenRemovePartHandler>( m );
                    }
                }
                remove_part( other_p, *handler_ptr );
            }
        }
    }
}

int vehicle::lift_strength() const
{
    units::mass mass = total_mass();
    return std::max<std::int64_t>( mass / 10000_gram, 1 );
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
    return parts[engines[e]].ammo_current().is_null() ? parts[engines[e]].fuel_current() == ft :
           parts[engines[e]].ammo_current() == ft;
}

bool vehicle::is_combustion_engine_type( const int e ) const
{
    return parts[engines[e]].info().has_flag( flag_E_COMBUSTION );
}

bool vehicle::is_perpetual_type( const int e ) const
{
    const itype_id  &ft = part_info( engines[e] ).fuel_type;
    return item( ft ).has_flag( flag_PERPETUAL );
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
    vehicle_part alt = parts[ alternators [ a ] ];
    if( alt.is_unavailable() ) {
        return false;
    }

    return std::any_of( engines.begin(), engines.end(), [this, &alt]( int idx ) {
        const vehicle_part &eng = parts [ idx ];
        //fuel_left checks that the engine can produce power to be absorbed
        return eng.mount == alt.mount && eng.is_available() && eng.enabled &&
               fuel_left( eng.fuel_current() ) &&
               !eng.has_fault_flag( "NO_ALTERNATOR_CHARGE" );
    } );
}

bool vehicle::has_security_working() const
{
    bool found_security = false;
    if( fuel_left( fuel_type_battery ) > 0 ) {
        for( int s : speciality ) {
            if( part_flag( s, "SECURITY" ) && parts[ s ].is_available() ) {
                found_security = true;
                break;
            }
        }
    }
    return found_security;
}

void vehicle::backfire( const int e ) const
{
    const int power = part_vpower_w( engines[e], true );
    const tripoint pos = global_part_pos3( engines[e] );
    sounds::sound( pos, 40 + power / 10000, sounds::sound_t::movement,
                   // single space after the exclamation mark because it does not end the sentence
                   //~ backfire sound
                   string_format( _( "a loud BANG! from the %s" ), // NOLINT(cata-text-style)
                                  parts[ engines[ e ] ].name() ), true, "vehicle", "engine_backfire" );
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
        if( vp.info().fuel_type == fuel_type_animal ) {
            monster *mon = get_monster( index );
            if( mon != nullptr && mon->has_effect( effect_harnessed ) ) {
                // An animal that can carry twice as much weight, can pull a cart twice as hard.
                pwr = mon->get_speed() * ( mon->get_size() - 1 ) * 3
                      * ( mon->get_mountable_weight_ratio() * 5 );
            } else {
                pwr = 0;
            }
        }
        // Weary multiplier
        const float weary_mult = get_player_character().exertion_adjusted_move_multiplier();
        ///\EFFECT_STR increases power produced for MUSCLE_* vehicles
        pwr += ( get_player_character().str_cur - 8 ) * part_info( index ).engine_muscle_power_factor() *
               weary_mult;
        /// wind-powered vehicles have differing power depending on wind direction
        if( vp.info().fuel_type == fuel_type_wind ) {
            weather_manager &weather = get_weather();
            int windpower = weather.windspeed;
            rl_vec2d windvec;
            double raddir = ( ( weather.winddirection + 180 ) % 360 ) * ( M_PI / 180 );
            windvec = windvec.normalized();
            windvec.y = -std::cos( raddir );
            windvec.x = std::sin( raddir );
            rl_vec2d fv = face_vec();
            double dot = windvec.dot_product( fv );
            if( dot <= 0 ) {
                dot = std::min( -0.1, dot );
            } else {
                dot = std::max( 0.1, dot );
            }
            int windeffectint = static_cast<int>( ( windpower * dot ) * 200 );
            pwr = std::max( 1, pwr + windeffectint );
        }
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

int vehicle::power_to_energy_bat( const int power_w, const time_duration &d ) const
{
    // Integrate constant epower (watts) over time to get units of battery energy
    // Thousands of watts over millions of seconds can happen, so 32-bit int
    // insufficient.
    int64_t energy_j = power_w * to_seconds<int64_t>( d );
    int energy_bat = energy_j / bat_energy_j;
    int sign = power_w >= 0 ? 1 : -1;
    // energy_bat remainder results in chance at additional charge/discharge
    energy_bat += x_in_y( std::abs( energy_j % bat_energy_j ), bat_energy_j ) ? sign : 0;
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
    // If its a part that harnesses animals that don't allow placing on it.
    if( !parts_in_square.empty() && part_info( parts_in_square[0] ).has_flag( "ANIMAL_CTRL" ) ) {
        return false;
    }
    //No other part can be placed on a protrusion
    if( !parts_in_square.empty() && part_info( parts_in_square[0] ).has_flag( "PROTRUSION" ) ) {
        return false;
    }

    //No part type can stack with itself, or any other part in the same slot
    for( const int &elem : parts_in_square ) {
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
            !has_structural_part( dp + point_east ) &&
            !has_structural_part( dp + point_south ) &&
            !has_structural_part( dp + point_west ) &&
            !has_structural_part( dp + point_north ) ) {
            return false;
        }
    }

    // only one exclusive engine allowed
    std::string empty;
    if( has_engine_conflict( &part, empty ) ) {
        return false;
    }

    // Check all the flags of the part to see if they require other flags
    // If other flags are required check if those flags are present
    for( const std::string &flag : part.get_flags() ) {
        if( !json_flag::get( flag ).requires_flag().empty() ) {
            bool anchor_found = false;
            for( const int &elem : parts_in_square ) {
                if( part_info( elem ).has_flag( json_flag::get( flag ).requires_flag() ) ) {
                    anchor_found = true;
                }
            }
            if( !anchor_found ) {
                return false;
            }
        }
    }

    //Mirrors cannot be mounted on OPAQUE parts
    if( part.has_flag( "VISION" ) && !part.has_flag( "CAMERA" ) ) {
        for( const int &elem : parts_in_square ) {
            if( part_info( elem ).has_flag( "OPAQUE" ) ) {
                return false;
            }
        }
    }
    //Opaque parts cannot be mounted on mirrors parts
    if( part.has_flag( "OPAQUE" ) ) {
        for( const int &elem : parts_in_square ) {
            if( part_info( elem ).has_flag( "VISION" ) &&
                !part_info( elem ).has_flag( "CAMERA" ) ) {
                return false;
            }
        }
    }

    //Turret mounts must NOT be installed on other (modded) turret mounts
    if( part.has_flag( "TURRET_MOUNT" ) ) {
        for( const int &elem : parts_in_square ) {
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

    // Find all the flags on parts in this tile that require other flags
    const point pt = parts[p].mount;
    std::vector<int> parts_here = parts_at_relative( pt, false );

    for( const int &elem : parts_here ) {
        for( const std::string &flag : part_info( elem ).get_flags() ) {
            if( part_info( p ).has_flag( json_flag::get( flag ).requires_flag() ) ) {
                reason = string_format( _( "Remove the attached %s first." ), part_info( elem ).name() );
                return false;
            }
        }
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
        for( const int &elem : parts_in_square ) {
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
             * If size = 1, it's one protruding part (i.e., bicycle wheel), so OK
             * Otherwise, it gets complicated... */
            if( connected_parts.size() > 1 ) {

                /* We'll take connected_parts[0] to be the target part.
                 * Every other part must have some path (that doesn't involve
                 * the part about to be removed) to the target part, in order
                 * for the part to be legally removable. */
                for( const vehicle_part &next_part : connected_parts ) {
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
    const point target = to.mount;
    const point excluded = excluded_part.mount;

    //Breadth-first-search components
    std::list<vehicle_part> discovered;
    std::list<vehicle_part> searched;

    //We begin with just the start point
    discovered.push_back( from );

    while( !discovered.empty() ) {
        vehicle_part current_part = discovered.front();
        discovered.pop_front();
        point current = current_part.mount;

        for( const point &offset : four_adjacent_offsets ) {
            point next = current + offset;

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
                for( const vehicle_part &elem : discovered ) {
                    if( elem.mount == next ) {
                        found = true;
                        break;
                    }
                }
                if( !found ) {
                    for( const vehicle_part &elem : searched ) {
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
int vehicle::install_part( const point &dp, const vpart_id &id, const std::string &variant_id,
                           bool force )
{
    if( !( force || can_mount( dp, id ) ) ) {
        return -1;
    }
    return install_part( dp, vehicle_part( id, variant_id, dp, item( id.obj().base_item ) ) );
}

int vehicle::install_part( const point &dp, const vpart_id &id, item &&obj,
                           const std::string &variant_id, bool force )
{
    if( !( force || can_mount( dp, id ) ) ) {
        return -1;
    }
    return install_part( dp, vehicle_part( id, variant_id, dp, std::move( obj ) ) );
}

int vehicle::install_part( const point &dp, const vehicle_part &new_part )
{
    // Should be checked before installing the part
    bool enable = false;
    if( new_part.is_engine() ) {
        enable = true;
    } else {
        // TODO: read toggle groups from JSON
        static const std::vector<std::string> enable_like = {{
                "CONE_LIGHT",
                "CIRCLE_LIGHT",
                "AISLE_LIGHT",
                "AUTOPILOT",
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
                "SPACE_HEATER",
                "COOLER",
                "WATER_PURIFIER",
                "ROCKWHEEL",
                "ROADHEAD"
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
    vehicle_part &pt = parts.back();

    pt.enabled = enable;

    pt.mount = dp;

    refresh();
    coeff_air_changed = true;
    return parts.size() - 1;
}

bool vehicle::try_to_rack_nearby_vehicle( std::vector<std::vector<int>> &list_of_racks )
{
    map &here = get_map();
    for( std::vector<int> &this_bike_rack : list_of_racks ) {
        std::vector<vehicle *> carry_vehs;
        carry_vehs.assign( 4, nullptr );
        vehicle *test_veh = nullptr;
        std::set<tripoint> veh_partial_match;
        std::vector<std::set<tripoint>> partial_matches;
        partial_matches.assign( 4, veh_partial_match );

        //do not attempt to rack onto already filled bike racks
        this_bike_rack.erase( std::remove_if( this_bike_rack.begin(),
        this_bike_rack.end(), [this]( const int &rack ) {
            return parts[rack].has_flag( vehicle_part::carrying_flag );
        } ), this_bike_rack.end() );

        for( int &rack_part : this_bike_rack ) {
            tripoint rack_pos = global_part_pos3( rack_part );
            int i = 0;
            for( const point &offset : four_cardinal_directions ) {
                tripoint search_pos( rack_pos + offset );
                test_veh = veh_pointer_or_null( here.veh_at( search_pos ) );
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
                ++i;
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
        int rack_part = 0;

        // the mount point we are going to add to the vehicle with the rack
        point carry_mount;

        // the mount point on the old vehicle (carry_veh) that will be destroyed
        point old_mount;
    };
    invalidate_towing( true );
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
        for( const int &carry_part : carry_veh_structs ) {
            if( carry_veh->parts[ carry_part ].mount.x || carry_veh->parts[ carry_part ].mount.y ) {
                axis = carry_veh->parts[ carry_part ].mount.x ? "X" : "Y";
            }
        }
    }

    units::angle relative_dir = normalize( carry_veh->face.dir() - face.dir() );
    units::angle relative_180 = units::fmod( relative_dir, 180_degrees );
    units::angle face_dir_180 = normalize( face.dir(), 180_degrees );

    // if the carrier is skewed N/S and the carried vehicle isn't aligned with
    // the carrier, force the carried vehicle to be at a right angle
    if( face_dir_180 >= 45_degrees && face_dir_180 <= 135_degrees ) {
        if( relative_180 >= 45_degrees && relative_180 <= 135_degrees ) {
            if( relative_dir < 180_degrees ) {
                relative_dir = 90_degrees;
            } else {
                relative_dir = 270_degrees;
            }
        }
    }

    // We look at each of the structure parts (mount points, i.e. frames) for the
    // carry vehicle and then find a rack part adjacent to it. If we don't find a rack part,
    // then we can't merge.
    bool found_all_parts = true;
    for( const int &carry_part : carry_veh_structs ) {

        // The current position on the original vehicle for this part
        tripoint carry_pos = carry_veh->global_part_pos3( carry_part );

        bool merged_part = false;
        for( int rack_part : rack_parts ) {
            size_t j = 0;
            // There's no mathematical transform from global pos3 to vehicle mount, so search for the
            // carry part in global pos3 after translating
            point carry_mount;
            for( const point &offset : four_cardinal_directions ) {
                carry_mount = parts[ rack_part ].mount + offset;
                tripoint possible_pos = mount_to_tripoint( carry_mount );
                if( possible_pos == carry_pos ) {
                    break;
                }
                ++j;
            }

            // We checked the adjacent points from the mounting rack and managed
            // to find the current structure part were looking for nearby. If the part was not
            // near this particular rack, we would look at each in the list of rack_parts
            const bool carry_part_next_to_this_rack = j < four_adjacent_offsets.size();
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
    const point mount_zero{};
    if( found_all_parts ) {
        decltype( loot_zones ) new_zones;
        for( const mapping &carry_map : carry_data ) {
            std::string offset = string_format( "%s%3d", carry_map.old_mount == mount_zero ? axis : " ",
                                                axis == "X" ? carry_map.old_mount.x : carry_map.old_mount.y );
            std::string unique_id = string_format( "%s%3d%s", offset,
                                                   static_cast<int>( to_degrees( relative_dir ) ),
                                                   carry_veh->name );
            for( const int &carry_part : carry_map.carry_parts_here ) {
                parts.push_back( carry_veh->parts[ carry_part ] );
                vehicle_part &carried_part = parts.back();
                carried_part.mount = carry_map.carry_mount;
                carried_part.carry_names.push( unique_id );
                carried_part.enabled = false;
                carried_part.set_flag( vehicle_part::carried_flag );
                //give each carried part a tracked_flag so that we can re-enable overmap tracking on unloading if necessary
                if( carry_veh->tracking_on ) {
                    carried_part.set_flag( vehicle_part::tracked_flag );
                }

                if( carried_part.has_flag( vehicle_part::passenger_flag ) ) {
                    carried_part.remove_flag( vehicle_part::passenger_flag );
                    carried_part.passenger_id = character_id();
                }

                parts[ carry_map.rack_part ].set_flag( vehicle_part::carrying_flag );
            }

            const std::pair<std::unordered_multimap<point, zone_data>::iterator, std::unordered_multimap<point, zone_data>::iterator>
            zones_on_point = carry_veh->loot_zones.equal_range( carry_map.old_mount );
            for( std::unordered_multimap<point, zone_data>::const_iterator it = zones_on_point.first;
                 it != zones_on_point.second; ++it ) {
                new_zones.emplace( carry_map.carry_mount, it->second );
            }
        }

        for( auto &new_zone : new_zones ) {
            zone_manager::get_manager().create_vehicle_loot_zone(
                *this, new_zone.first, new_zone.second );
        }

        // Now that we've added zones to this vehicle, we need to make sure their positions
        // update when we next interact with them
        zones_dirty = true;

        map &here = get_map();
        //~ %1$s is the vehicle being loaded onto the bicycle rack
        add_msg( _( "You load the %1$s on the rack." ), carry_veh->name );
        here.destroy_vehicle( carry_veh );
        here.dirty_vehicle_list.insert( this );
        here.set_transparency_cache_dirty( sm_pos.z );
        here.set_seen_cache_dirty( tripoint_zero );
        refresh();
    } else {
        //~ %1$s is the vehicle being loaded onto the bicycle rack
        add_msg( m_bad, _( "You can't get the %1$s on the rack." ), carry_veh->name );
    }
    return found_all_parts;
}

/**
 * Mark a part as removed from the vehicle.
 * @return bool true if the vehicle's 0,0 point shifted.
 */
bool vehicle::remove_part( const int p )
{
    DefaultRemovePartHandler handler;
    return remove_part( p, handler );
}

bool vehicle::remove_part( const int p, RemovePartHandler &handler )
{
    // NOTE: Don't access g or g->m or anything from it directly here.
    // Forward all access to the handler.
    // There are currently two implementations of it:
    // - one for normal game play (vehicle is on the main map g->m),
    // - one for mapgen (vehicle is on a temporary map used only during mapgen).
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

    // Unboard any entities standing on removed boardable parts
    if( part_flag( p, "BOARDABLE" ) && parts[p].has_flag( vehicle_part::passenger_flag ) ) {
        handler.unboard( part_loc );
    }

    // If `p` has flag `parent_flag`, remove child with flag `child_flag`
    // Returns true if removal occurs
    const auto remove_dependent_part = [&]( const std::string & parent_flag,
    const std::string & child_flag ) {
        if( part_flag( p, parent_flag ) ) {
            int dep = part_with_feature( p, child_flag, false );
            if( dep >= 0 && !magic ) {
                handler.add_item_or_charges( part_loc, parts[dep].properties_to_item(), false );
                remove_part( dep, handler );
                return true;
            }
        }
        return false;
    };

    // if a windshield is removed (usually destroyed) also remove curtains
    // attached to it.
    if( remove_dependent_part( "WINDOW", "CURTAIN" ) || part_flag( p, VPFLAG_OPAQUE ) ) {
        handler.set_transparency_cache_dirty( sm_pos.z );
    }

    if( part_flag( p, VPFLAG_ROOF ) || part_flag( p, VPFLAG_OPAQUE ) ) {
        handler.set_floor_cache_dirty( sm_pos.z + 1 );
    }

    remove_dependent_part( "SEAT", "SEATBELT" );
    remove_dependent_part( "BATTERY_MOUNT", "NEEDS_BATTERY_MOUNT" );

    // Release any animal held by the part
    if( parts[p].has_flag( vehicle_part::animal_flag ) ) {
        item base = parts[p].get_base();
        handler.spawn_animal_from_part( base, part_loc );
        parts[p].set_base( base );
        parts[p].remove_flag( vehicle_part::animal_flag );
    }

    // Update current engine configuration if needed
    if( part_flag( p, "ENGINE" ) && engines.size() > 1 ) {
        bool any_engine_on = false;

        for( const int &e : engines ) {
            if( e != p && is_part_on( e ) ) {
                any_engine_on = true;
                break;
            }
        }

        if( !any_engine_on ) {
            engine_on = false;
            for( const int &e : engines ) {
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

    handler.removed( *this, p );

    const point &vp_mount = parts[p].mount;
    const auto iter = labels.find( label( vp_mount ) );
    if( iter != labels.end() && parts_at_relative( vp_mount, false ).empty() ) {
        labels.erase( iter );
    }

    for( item &i : get_items( p ) ) {
        // Note: this can spawn items on the other side of the wall!
        // TODO: fix this ^^
        if( !magic ) {
            tripoint dest( part_loc + point( rng( -3, 3 ), rng( -3, 3 ) ) );
            // This new point might be out of the map bounds.  It's not
            // reasonable to try to spawn it outside the currently valid map,
            // so we pass true here to cause such points to be clamped to the
            // valid bounds without printing an error (as would normally
            // occur).
            handler.add_item_or_charges( dest, i, true );
        }
    }
    refresh();
    coeff_air_changed = true;
    return shift_if_needed();
}

void vehicle::part_removal_cleanup()
{
    bool changed = false;
    map &here = get_map();
    for( std::vector<vehicle_part>::iterator it = parts.begin(); it != parts.end(); /* noop */ ) {
        if( it->removed ) {
            vehicle_stack items = get_items( std::distance( parts.begin(), it ) );
            while( !items.empty() ) {
                items.erase( items.begin() );
            }
            const tripoint &pt = global_part_pos3( *it );
            here.clear_vehicle_point_from_cache( this, pt );
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
            here.destroy_vehicle( this );
            return;
        } else {
            here.add_vehicle_to_cache( this );
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
        if( part.carry_names.empty() ) {
            // note: we get here if the part was added while the vehicle was carried / mounted. This is not expected.
            // still try to remove the carried flag, if any.
            part.remove_flag( vehicle_part::carried_flag );
        } else {
            part.carry_names.pop();
            if( part.carry_names.empty() ) {
                part.remove_flag( vehicle_part::carried_flag );
            }
        }
    }
}

void vehicle::remove_tracked_flag()
{
    for( vehicle_part &part : parts ) {
        if( part.carry_names.empty() ) {
            part.remove_flag( vehicle_part::tracked_flag );
        } else {
            part.carry_names.pop();
            if( part.carry_names.empty() ) {
                part.remove_flag( vehicle_part::tracked_flag );
            }
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
    bool tracked_parts =
        false; // we will set this to true if any of the vehicle parts carry a tracked_flag
    for( int carried_part : carried_parts ) {
        //check if selected part carries tracking flag
        if( parts[carried_part].has_flag(
                vehicle_part::tracked_flag ) ) { //this should only need to run once
            tracked_parts = true;
        }
        const auto &carry_names = parts[carried_part].carry_names;
        if( !carry_names.empty() ) {
            std::string id_string = carry_names.top().substr( 0, 1 );
            if( id_string == "X" || id_string == "Y" ) {
                veh_record = carry_names.top();
                new_pos3 = global_part_pos3( carried_part );
                x_aligned = id_string == "X";
                break;
            }
        }
    }
    if( veh_record.empty() ) {
        return false;
    }
    units::angle new_dir =
        normalize( units::from_degrees( std::stoi( veh_record.substr( 4, 3 ) ) ) + face.dir() );
    units::angle host_dir = normalize( face.dir(), 180_degrees );
    // if the host is skewed N/S, and the carried vehicle is going to come at an angle,
    // force it to east/west instead
    if( host_dir >= 45_degrees && host_dir <= 135_degrees ) {
        if( new_dir <= 45_degrees || new_dir >= 315_degrees ) {
            new_dir = 0_degrees;
        } else if( new_dir >= 135_degrees && new_dir <= 225_degrees ) {
            new_dir = 180_degrees;
        }
    }
    map &here = get_map();
    vehicle *new_vehicle = here.add_vehicle( vproto_id( "none" ), new_pos3, new_dir );
    if( new_vehicle == nullptr ) {
        add_msg_debug( "Unable to unload bike rack, host face %d, new_dir %d!",
                       to_degrees( face.dir() ), to_degrees( new_dir ) );
        return false;
    }

    std::vector<point> new_mounts;
    new_vehicle->name = veh_record.substr( vehicle_part::name_offset );
    for( int carried_part : carried_parts ) {
        point new_mount;
        std::string mount_str;
        if( !parts[carried_part].carry_names.empty() ) {
            // the mount string should be something like "X 0 0" for ex. We get the first number out of it.
            mount_str = parts[carried_part].carry_names.top().substr( 1, 3 );
        } else {
            // FIX #28712; if we get here it means that a part was added to the bike while the latter was a carried vehicle.
            // This part didn't get a carry_names because those are assigned when the carried vehicle is loaded.
            // We can't be sure to which vehicle it really belongs to, so it will be detached from the vehicle.
            // We can at least inform the player that there's something wrong.
            add_msg( m_warning,
                     _( "A part of the vehicle ('%s') has no containing vehicle's name.  It will be detached from the %s vehicle." ),
                     parts[carried_part].name(),  new_vehicle->name );

            // check if any other parts at the same location have a valid carry name so we can still have a valid mount location.
            for( const int &local_part : parts_at_relative( parts[carried_part].mount, true ) ) {
                if( !parts[local_part].carry_names.empty() ) {
                    mount_str = parts[local_part].carry_names.top().substr( 1, 3 );
                    break;
                }
            }
        }
        if( mount_str.empty() ) {
            add_msg( m_bad,
                     _( "There's not viable mount location on this vehicle: %s. It can't be unloaded from the rack." ),
                     new_vehicle->name );
            return false;
        }
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
    const bool success = split_vehicles( carried_vehicles, new_vehicles, carried_mounts );
    if( success ) {
        //~ %s is the vehicle being loaded onto the bicycle rack
        add_msg( _( "You unload the %s from the bike rack." ), new_vehicle->name );
        new_vehicle->remove_carried_flag();
        if( tracked_parts ) {
            new_vehicle->toggle_tracking(); //turn on tracking for our newly created vehicle
            new_vehicle->remove_tracked_flag(); //remove our tracking flags now that the vehicle isn't carried
        }
        here.dirty_vehicle_list.insert( this );
        part_removal_cleanup();
        new_vehicle->enable_refresh();
        for( int idx : new_vehicle->engines ) {
            if( !new_vehicle->parts[idx].is_broken() ) {
                new_vehicle->parts[idx].enabled = true;
            }
        }
    } else {
        //~ %s is the vehicle being loaded onto the bicycle rack
        add_msg( m_bad, _( "You can't unload the %s from the bike rack." ), new_vehicle->name );
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
        for( const int &p : valid_parts ) {
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

        const auto push_neighbor = [&]( int p, const std::vector<int> &with_p ) {
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
            for( const int &p : test_set.second ) {
                veh_parts.push_back( p );
            }
            checked_parts.insert( test_part );
            for( const point &offset : four_adjacent_offsets ) {
                const point dp = parts[test_part].mount + offset;
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
    map &here = get_map();
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
            new_vehicle = here.add_vehicle( vproto_id( "none" ), new_v_pos3, face.dir() );
            if( new_vehicle == nullptr ) {
                // the split part was out of the map bounds.
                continue;
            }
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
            // if this part is a towing part, transfer the tow_data to the new vehicle.
            if( part_flag( mov_part, "TOW_CABLE" ) ) {
                if( is_towed() ) {
                    tow_data.get_towed_by()->tow_data.set_towing( tow_data.get_towed_by(), new_vehicle );
                    tow_data.clear_towing();
                } else if( is_towing() ) {
                    tow_data.get_towed()->tow_data.set_towing( new_vehicle, tow_data.get_towed() );
                    tow_data.clear_towing();
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
                parts[ mov_part ].passenger_id = character_id();
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

        here.dirty_vehicle_list.insert( new_vehicle );
        here.set_transparency_cache_dirty( sm_pos.z );
        here.set_seen_cache_dirty( tripoint_zero );
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
    const item_group_id &group = info().breaks_into_group;
    // TODO: make it optional? Or use id of empty item group?
    if( group.is_empty() ) {
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

cata::optional<vpart_reference> vpart_position::part_with_tool( const itype_id &tool_type ) const
{
    for( const int idx : vehicle().parts_at_relative( mount(), false ) ) {
        const vpart_reference vp( vehicle(), idx );
        if( vp.part().is_available() && vp.info().has_tool( tool_type ) ) {
            return vp;
        }
    }
    return cata::optional<vpart_reference>();
}

std::vector<std::pair<itype_id, int>> vpart_position::get_tools() const
{
    std::set<std::pair<itype_id, int>> tools;
    for( const int part_idx : this->vehicle().parts_at_relative( this->mount(), false ) ) {
        const vpart_reference vp( this->vehicle(), part_idx );
        if( vp.part().is_broken() ) {
            continue;
        }
        std::set<std::pair<itype_id, int>> items = vp.part().info().get_pseudo_tools();
        std::copy( items.cbegin(), items.cend(), std::inserter( tools, tools.end() ) );
    }

    return std::vector<std::pair<itype_id, int>>( tools.cbegin(), tools.cend() );
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

cata::optional<vpart_reference> vpart_position::avail_part_with_feature(
    const std::string &f ) const
{
    const int i = vehicle().avail_part_with_feature( part_index(), f );
    return i >= 0 ? vpart_reference( vehicle(), i ) : cata::optional<vpart_reference>();
}

cata::optional<vpart_reference> vpart_position::avail_part_with_feature( vpart_bitflags f ) const
{
    const int i = vehicle().avail_part_with_feature( part_index(), f );
    return i >= 0 ? vpart_reference( vehicle(), i ) : cata::optional<vpart_reference>();
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

cata::optional<vpart_reference> optional_vpart_position::avail_part_with_feature(
    const std::string &f ) const
{
    return has_value() ? value().avail_part_with_feature( f ) : cata::nullopt;
}

cata::optional<vpart_reference> optional_vpart_position::avail_part_with_feature(
    vpart_bitflags f ) const
{
    return has_value() ? value().avail_part_with_feature( f ) : cata::nullopt;
}

cata::optional<vpart_reference> optional_vpart_position::obstacle_at_part() const
{
    return has_value() ? value().obstacle_at_part() : cata::nullopt;
}

cata::optional<vpart_reference> optional_vpart_position::part_displayed() const
{
    return has_value() ? value().part_displayed() : cata::nullopt;
}

cata::optional<vpart_reference> optional_vpart_position::part_with_tool(
    const itype_id &tool_type ) const
{
    return has_value() ? value().part_with_tool( tool_type ) : cata::nullopt;
}

int vehicle::part_with_feature( int part, vpart_bitflags const flag, bool unbroken ) const
{
    if( part_flag( part, flag ) && ( !unbroken || !parts[part].is_broken() ) ) {
        return part;
    }
    const auto it = relative_parts.find( parts[part].mount );
    if( it != relative_parts.end() ) {
        const std::vector<int> &parts_here = it->second;
        for( const int &i : parts_here ) {
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

std::vector<std::pair<itype_id, int>> optional_vpart_position::get_tools() const
{
    return has_value() ? value().get_tools() : std::vector<std::pair<itype_id, int>>();
}

int vehicle::part_with_feature( const point &pt, const std::string &flag, bool unbroken ) const
{
    std::vector<int> parts_here = parts_at_relative( pt, false );
    for( const int &elem : parts_here ) {
        if( part_flag( elem, flag ) && ( !unbroken || !parts[ elem ].is_broken() ) ) {
            return elem;
        }
    }
    return -1;
}

int vehicle::avail_part_with_feature( int part, vpart_bitflags const flag ) const
{
    int part_a = part_with_feature( part, flag, true );
    if( ( part_a >= 0 ) && parts[ part_a ].is_available() ) {
        return part_a;
    }
    return -1;
}

int vehicle::avail_part_with_feature( int part, const std::string &flag ) const
{
    return avail_part_with_feature( parts[ part ].mount, flag );
}

int vehicle::avail_part_with_feature( const point &pt, const std::string &flag ) const
{
    int part_a = part_with_feature( pt, flag, true );
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

    for( const vehicle_part &e : parts ) {
        if( e.precalc[0] != relative_pos ) {
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
    for( vehicle_part &e : parts ) {
        if( e.precalc[ 0 ] != relative_pos ) {
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
    for( const vehicle_part &e : parts ) {
        if( e.precalc[ 0 ] != relative_pos ) {
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
        // legacy support TODO: change labels into a map and keep track of deleted labels
        return cata::nullopt;
    }
    return it->text;
}

void vpart_position::set_label( const std::string &text ) const
{
    std::set<label> &labels = vehicle().labels;
    const auto it = labels.find( label( mount() ) );
    // TODO: empty text should remove the label instead of just storing an empty string, see get_label
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
    for( const int &elem : parts_here ) {
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

// another NPC probably removed a part in the time it took to walk here and start the activity.
// as the part index was first "chosen" before the NPC started traveling here.
// therefore the part index is now invalid shifted by one or two ( depending on how many other NPCs working on this vehicle )
// so loop over the part indexes in reverse order to get the next one down that matches the part type we wanted to remove
int vehicle::get_next_shifted_index( int original_index, player &p )
{
    int ret_index = original_index;
    bool found_shifted_index = false;
    for( std::vector<vehicle_part>::reverse_iterator it = parts.rbegin(); it != parts.rend(); ++it ) {
        if( p.get_value( "veh_index_type" ) == it->info().name() ) {
            ret_index = index_of_part( &*it );
            found_shifted_index = true;
            break;
        }
    }
    if( !found_shifted_index ) {
        // we are probably down to a few parts left, and things get messy here, so an alternative index maybe can't be found
        // if loads of npcs are all removing parts at the same time.
        // if that's the case, just bail out and give up, somebody else is probably doing the job right now anyway.
        return -1;
    } else {
        return ret_index;
    }
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
        if( vp.part().precalc[0].xy() == dp && !vp.part().removed ) {
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

    Character &player_character = get_player_character();
    bool in_vehicle = player_character.in_vehicle;
    if( in_vehicle ) {
        // They're in a vehicle, but are they in /this/ vehicle?
        std::vector<int> psg_parts = boarded_parts();
        in_vehicle = false;
        for( const int &psg_part : psg_parts ) {
            if( get_passenger( psg_part ) == &player_character ) {
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
    tripoint q;
    coord_translate( pivot_rotation[0], pivot_anchor[0], p, q );
    return q.xy();
}

void vehicle::coord_translate( const units::angle &dir, const point &pivot, const point &p,
                               tripoint &q ) const
{
    tileray tdir( dir );
    tdir.advance( p.x - pivot.x );
    q.x = tdir.dx() + tdir.ortho_dx( p.y - pivot.y );
    q.y = tdir.dy() + tdir.ortho_dy( p.y - pivot.y );
}

void vehicle::coord_translate( tileray tdir, const point &pivot, const point &p, tripoint &q ) const
{
    tdir.clear_advance();
    tdir.advance( p.x - pivot.x );
    q.x = tdir.dx() + tdir.ortho_dx( p.y - pivot.y );
    q.y = tdir.dy() + tdir.ortho_dy( p.y - pivot.y );
}

point vehicle::rotate_mount( units::angle old_dir, units::angle new_dir, const point &pivot,
                             const point &p ) const
{
    tripoint q;
    coord_translate( new_dir - old_dir, pivot, p, q );
    return q.xy();
}

tripoint vehicle::mount_to_tripoint( const point &mount ) const
{
    return mount_to_tripoint( mount, point_zero );
}

tripoint vehicle::mount_to_tripoint( const point &mount, const point &offset ) const
{
    tripoint mnt_translated;
    coord_translate( pivot_rotation[0], pivot_anchor[ 0 ], mount + offset, mnt_translated );
    return global_pos3() + mnt_translated;
}

void vehicle::precalc_mounts( int idir, const units::angle &dir, const point &pivot )
{
    if( idir < 0 || idir > 1 ) {
        idir = 0;
    }
    tileray tdir( dir );
    std::unordered_map<point, tripoint> mount_to_precalc;
    for( vehicle_part &p : parts ) {
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

std::vector<rider_data> vehicle::get_riders() const
{
    std::vector<rider_data> res;
    for( const vpart_reference &vp : get_avail_parts( VPFLAG_BOARDABLE ) ) {
        Creature *rider = g->critter_at( vp.pos() );
        if( rider ) {
            rider_data r;
            r.prt = vp.part_index();
            r.psg = rider;
            res.emplace_back( r );
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

monster *vehicle::get_monster( int p ) const
{
    p = part_with_feature( p, VPFLAG_BOARDABLE, false );
    if( p >= 0 ) {
        return g->critter_at<monster>( global_part_pos3( p ), true );
    }
    return nullptr;
}

tripoint vehicle::global_pos3() const
{
    return sm_to_ms_copy( sm_pos ) + pos;
}

tripoint vehicle::global_part_pos3( const int &index ) const
{
    return global_part_pos3( parts[ index ] );
}

tripoint vehicle::global_part_pos3( const vehicle_part &pt ) const
{
    return global_pos3() + pt.precalc[ 0 ];
}

void vehicle::set_submap_moved( const tripoint &p )
{
    const point old_msp = get_map().getabs( global_pos3().xy() );
    sm_pos = p;
    if( !tracking_on ) {
        return;
    }
    // TODO: fix point types
    overmap_buffer.move_vehicle( this, point_abs_ms( old_msp ) );
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
    // TODO: Bring back caching of this point
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
    tripoint dp;
    coord_translate( pivot_rotation[0], pivot_anchor[1], pivot_anchor[0], dp );
    return dp.xy();
}

int vehicle::fuel_left( const itype_id &ftype, bool recurse ) const
{
    int fl = 0;

    for( const int i : fuel_containers ) {
        const vehicle_part &part = parts[i];
        if( part.ammo_current() != ftype ||
            // don't count frozen liquid
            ( !part.base.contents.empty() && part.is_tank() &&
              part.base.contents.legacy_front().made_of( phase_id::SOLID ) ) ) {
            continue;
        }
        fl += part.ammo_remaining();
    }

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
        Character &player_character = get_player_character();
        // TODO: Allow NPCs to power those
        const optional_vpart_position vp = get_map().veh_at( player_character.pos() );
        bool player_controlling = player_in_control( player_character );

        //if the engine in the player tile is a muscle engine, and player is controlling vehicle
        if( vp && &vp->vehicle() == this && player_controlling ) {
            const int p = avail_part_with_feature( vp->part_index(), VPFLAG_ENGINE );
            if( p >= 0 && is_part_on( p ) && part_info( p ).fuel_type == fuel_type_muscle ) {
                //Broken limbs prevent muscle engines from working
                if( ( part_info( p ).has_flag( "MUSCLE_LEGS" ) &&
                      ( player_character.get_working_leg_count() >= 2 ) ) ||
                    ( part_info( p ).has_flag( "MUSCLE_ARMS" ) &&
                      ( player_character.get_working_arm_count() >= 2 ) ) ) {
                    fl += 10;
                }
            }
        }
        // As do any other engine flagged as perpetual
    } else if( item( ftype ).has_flag( flag_PERPETUAL ) ) {
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
        return lhs + ( ( rhs.is_available() &&
                         rhs.ammo_current() == ftype ) ? rhs.ammo_capacity( item::find_type(
                                     ftype )->ammo->type ) : 0 );
    } );
}

float vehicle::fuel_specific_energy( const itype_id &ftype ) const
{
    float total_energy = 0.0f;
    float total_mass = 0.0f;
    for( const vehicle_part &vp : parts ) {
        if( vp.is_tank() && vp.ammo_current() == ftype &&
            vp.base.contents.legacy_front().made_of( phase_id::LIQUID ) ) {
            float energy = vp.base.contents.legacy_front().specific_energy;
            float mass = to_gram( vp.base.contents.legacy_front().weight() );
            total_energy += energy * mass;
            total_mass += mass;
        }
    }
    return total_energy / total_mass;
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
    for( vehicle_part &p : parts ) {
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
                if( parts[ e ].has_fault_flag( "DOUBLE_FUEL_CONSUMPTION" ) ) {
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
    if( fuel_rate_w == 0 || fuel.has_flag( flag_PERPETUAL ) || !engine_on ) {
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
        // HACK: engines consume 1 second worth of fuel per turn, even though a turn is 6 seconds
        if( vslowdown > 0 ) {
            int accel = acceleration( true, target_v );
            if( accel == 0 ) {
                // FIXME: Long-term plan is to change the fuel consumption
                // computation entirely; for now just warn if this would
                // otherwise have been division-by-zero
                debugmsg( "Vehicle unexpectedly has zero acceleration" );
            } else {
                amount_pct += 600 * vslowdown / accel;
            }
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
            if( parts[ engines[e] ].has_fault_flag( "REDUCE_ENG_POWER" ) ) {
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

bool vehicle::can_use_rails() const
{
    // do not allow vehicles without rail wheels or with mixed wheels
    bool can_use = !rail_wheelcache.empty() && wheelcache.size() == rail_wheelcache.size();
    if( !can_use ) {
        return false;
    }
    map &here = get_map();
    bool is_wheel_on_rail = false;
    for( int part_index : rail_wheelcache ) {
        // at least one wheel should be on track
        if( here.has_flag_ter_or_furn( TFLAG_RAIL, global_part_pos3( part_index ) ) ) {
            is_wheel_on_rail = true;
            break;
        }
    }
    return is_wheel_on_rail;
}

int vehicle::ground_acceleration( const bool fueled, int at_vel_in_vmi ) const
{
    if( !( engine_on || skidding ) ) {
        return 0;
    }
    int target_vmiph = std::max( at_vel_in_vmi, std::max( 1000, max_velocity( fueled ) / 4 ) );
    int cmps = vmiph_to_cmps( target_vmiph );
    double weight = to_kilogram( total_mass() );
    if( is_towing() ) {
        vehicle *other_veh = tow_data.get_towed();
        if( other_veh ) {
            weight = weight + to_kilogram( other_veh->total_mass() );
        }
    }
    int engine_power_ratio = total_power_w( fueled ) / weight;
    int accel_at_vel = 100 * 100 * engine_power_ratio / cmps;
    add_msg_debug( "%s: accel at %d vimph is %d", name, target_vmiph,
                   cmps_to_vmiph( accel_at_vel ) );
    return cmps_to_vmiph( accel_at_vel );
}

int vehicle::rotor_acceleration( const bool fueled, int at_vel_in_vmi ) const
{
    ( void )at_vel_in_vmi;
    if( !( engine_on || is_flying ) ) {
        return 0;
    }
    const int accel_at_vel = 100 * lift_thrust_of_rotorcraft( fueled ) / to_kilogram( total_mass() );
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
    double weight = to_kilogram( total_mass() );
    if( is_towing() ) {
        vehicle *other_veh = tow_data.get_towed();
        if( other_veh ) {
            weight = weight + to_kilogram( other_veh->total_mass() );
        }
    }
    int engine_power_ratio = total_power_w( fueled ) / weight;
    int accel_at_vel = 100 * 100 * engine_power_ratio / cmps;
    add_msg_debug( "%s: water accel at %d vimph is %d", name, target_vmiph,
                   cmps_to_vmiph( accel_at_vel ) );
    return cmps_to_vmiph( accel_at_vel );
}

// cubic equation solution
// don't use complex numbers unless necessary and it's usually not
// see https://math.vanderbilt.edu/schectex/courses/cubic/ for the gory details
static double simple_cubic_solution( double a, double b, double c, double d )
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
            return p + real( term_sum );
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
    if( is_watercraft() ) {
        return water_acceleration( fueled, at_vel_in_vmi );
    } else if( is_rotorcraft() && is_flying ) {
        return rotor_acceleration( fueled, at_vel_in_vmi );
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
    add_msg_debug( "%s: power %d, c_air %3.2f, c_rolling %3.2f, max_in_mps %3.2f",
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
    add_msg_debug( "%s: power %d, c_air %3.2f, c_water %3.2f, water max_in_mps %3.2f",
                   name, total_engine_w, coeff_air_drag(), coeff_water_drag(), max_in_mps );
    return mps_to_vmiph( max_in_mps );
}

int vehicle::max_rotor_velocity( const bool fueled ) const
{
    const double max_air_mps = std::sqrt( lift_thrust_of_rotorcraft( fueled ) / coeff_air_drag() );
    // helicopters just cannot go over 250mph at very maximum
    // weird things start happening to their rotors if they do.
    // due to the rotor tips going supersonic.
    return std::min( 25501, mps_to_vmiph( max_air_mps ) );
}

int vehicle::max_velocity( const bool fueled ) const
{
    if( is_flying && is_rotorcraft() ) {
        return max_rotor_velocity( fueled );
    } else if( is_watercraft() ) {
        return max_water_velocity( fueled );
    } else {
        return max_ground_velocity( fueled );
    }
}

int vehicle::max_reverse_velocity( const bool fueled ) const
{
    int max_vel = max_velocity( fueled );
    if( has_engine_type( fuel_type_battery, true ) ) {
        // Electric motors can go in reverse as well as forward
        return -max_vel;
    } else {
        // All other motive powers do poorly in reverse
        return -max_vel / 4;
    }
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

int vehicle::safe_rotor_velocity( const bool fueled ) const
{
    const double max_air_mps = std::sqrt( lift_thrust_of_rotorcraft( fueled,
                                          true ) / coeff_air_drag() );
    return std::min( 22501, mps_to_vmiph( max_air_mps ) );
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
    if( is_flying && is_rotorcraft() ) {
        return safe_rotor_velocity( fueled );
    } else if( is_watercraft() ) {
        return safe_water_velocity( fueled );
    } else {
        return safe_ground_velocity( fueled );
    }
}

bool vehicle::do_environmental_effects()
{
    bool needed = false;
    map &here = get_map();
    // check for smoking parts
    for( const vpart_reference &vp : get_all_parts() ) {
        /* Only lower blood level if:
         * - The part is outside.
         * - The weather is any effect that would cause the player to be wet. */
        if( vp.part().blood > 0 && here.is_outside( vp.pos() ) ) {
            needed = true;
            if( get_weather().weather_id->rains &&
                get_weather().weather_id->precip != precip_class::very_light ) {
                vp.part().blood--;
            }
        }
    }
    return needed;
}

void vehicle::spew_field( double joules, int part, field_type_id type, int intensity )
{
    if( rng( 1, 10000 ) > joules ) {
        return;
    }
    intensity = std::max( joules / 10000, static_cast<double>( intensity ) );
    const tripoint dest = exhaust_dest( part );
    get_map().mod_field_intensity( dest, type, intensity );
}

/**
 * Generate noise or smoke from a vehicle with engines turned on
 * load = how hard the engines are working, from 0.0 until 1.0
 * time = how many seconds to generated smoke for
 */
void vehicle::noise_and_smoke( int load, time_duration time )
{
    static const std::array<std::pair<std::string, int>, 8> sounds = { {
            { translate_marker( "hmm" ), 0 }, { translate_marker( "hummm!" ), 15 },
            { translate_marker( "whirrr!" ), 30 }, { translate_marker( "vroom!" ), 60 },
            { translate_marker( "roarrr!" ), 100 }, { translate_marker( "ROARRR!" ), 140 },
            { translate_marker( "BRRROARRR!" ), 180 }, { translate_marker( "BRUMBRUMBRUMBRUM!" ), INT_MAX }
        }
    };
    const std::string heli_noise = translate_marker( "WUMPWUMPWUMP!" );
    double noise = 0.0;
    double mufflesmoke = 0.0;
    double muffle;
    int exhaust_part;
    std::tie( exhaust_part, muffle ) = get_exhaust_part();

    bool bad_filter = false;
    bool combustion = false;

    for( size_t e = 0; e < engines.size(); e++ ) {
        int p = engines[e];
        if( is_engine_on( e ) &&  engine_fuel_left( e ) ) {
            // convert current engine load to units of watts/40K
            // then spew more smoke and make more noise as the engine load increases
            int part_watts = part_vpower_w( p, true );
            double max_stress = static_cast<double>( part_watts / 40000.0 );
            double cur_stress = load / 1000.0 * max_stress;
            // idle stress = 1.0 resulting in nominal working engine noise = engine_noise_factor()
            // and preventing noise = 0
            cur_stress = std::max( cur_stress, 1.0 );
            double part_noise = cur_stress * part_info( p ).engine_noise_factor();

            if( part_info( p ).has_flag( "E_COMBUSTION" ) ) {
                combustion = true;
                double health = parts[p].health_percent();
                if( parts[ p ].has_fault_flag( "ENG_BACKFIRE" ) ) {
                    health = 0.0;
                }
                if( health < part_info( p ).engine_backfire_threshold() && one_in( 50 + 150 * health ) ) {
                    backfire( e );
                }
                double j = cur_stress * to_turns<int>( time ) * muffle * 1000;

                if( parts[ p ].has_fault_flag( "EXTRA_EXHAUST" ) ) {
                    bad_filter = true;
                    j *= j;
                }

                if( ( exhaust_part == -1 ) && engine_on ) {
                    spew_field( j, p, fd_smoke, bad_filter ? fd_smoke->get_max_intensity() : 1 );
                } else {
                    mufflesmoke += j;
                }
                part_noise = ( part_noise + max_stress * 3 + 5 ) * muffle;
            }
            noise = std::max( noise, part_noise ); // Only the loudest engine counts.
        }
    }
    if( !combustion ) {
        return;
    }
    /// TODO: handle other engine types: muscle / animal / wind / coal / ...

    if( exhaust_part != -1 && engine_on ) {
        spew_field( mufflesmoke, exhaust_part, fd_smoke,
                    bad_filter ? fd_smoke->get_max_intensity() : 1 );
    }
    if( is_rotorcraft() ) {
        noise *= 2;
    }
    // Cap engine noise to avoid deafening.
    noise = std::min( noise, 100.0 );
    // Even a vehicle with engines off will make noise traveling at high speeds
    noise = std::max( noise, static_cast<double>( std::fabs( velocity / 500.0 ) ) );
    int lvl = 0;
    if( one_in( 4 ) && rng( 0, 30 ) < noise ) {
        while( noise > sounds[lvl].second ) {
            lvl++;
        }
    }
    add_msg_debug( "VEH NOISE final: %d", static_cast<int>( noise ) );
    vehicle_noise = static_cast<unsigned char>( noise );
    sounds::sound( global_pos3(), noise, sounds::sound_t::movement,
                   _( is_rotorcraft() ? heli_noise : sounds[lvl].first ), true );
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
    float total_rating = 0.0f;
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
    int sail = minrow;
    int rotor = minrow;
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
    constexpr double sail_height = 0.8;
    constexpr double rotor_height = 0.6;

    std::vector<int> structure_indices = all_parts_at_location( part_location_structure );
    int width = mount_max.y - mount_min.y + 1;

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
        value = std::min( value, test ? p.mount.x - mount_min.x : maxrow );
    };
    const auto d_check_max = [&]( int &value, const vehicle_part & p, bool test ) {
        value = std::max( value, test ? p.mount.x - mount_min.x : minrow );
    };

    // raycast down each column. the least drag vehicle has halfboard, windshield, seat with roof,
    // windshield, halfboard and is twice as long as it is wide.
    // find the first instance of each item and compare against the ideal configuration.
    std::vector<drag_column> drag( width );
    for( int p : structure_indices ) {
        if( parts[ p ].removed ) {
            continue;
        }
        int col = parts[ p ].mount.y - mount_min.y;
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
            d_check_max( drag[ col ].rotor, pa, pa.info().has_flag( "ROTOR" ) );
            d_check_max( drag[ col ].rotor, pa, pa.info().has_flag( "ROTOR_SIMPLE" ) );
            d_check_max( drag[ col ].sail, pa, pa.info().has_flag( "WIND_POWERED" ) );
            d_check_max( drag[ col ].exposed, pa, d_exposed( pa ) );
            d_check_min( drag[ col ].last, pa, pa.info().has_flag( "LOW_FINAL_AIR_DRAG" ) ||
                         pa.info().has_flag( "HALF_BOARD" ) );
        }
    }
    double height = 0;
    double c_air_drag = 0;
    // tally the results of each row and prorate them relative to vehicle width
    for( drag_column &dc : drag ) {
        // even as m_debug you rarely want to see this
        // add_msg_debug( "veh %: pro %d, hboard %d, fboard %d, shield %d, seat %d, roof %d, aisle %d, turret %d, panel %d, exposed %d, last %d\n", name, dc.pro, dc.hboard, dc.fboard, dc.shield, dc.seat, dc.roof, dc.aisle, dc.turret, dc.panel, dc.exposed, dc.last );

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
        c_air_drag_c -= ( 2 * ( mount_max.x - mount_min.x ) > width ) ? c_air_mod : 0;
        // trunk doors and halfboards at the tail mildly reduce air drag
        c_air_drag_c -= ( dc.last == mount_min.x ) ? c_air_mod : 0;
        // turrets severely worsen air drag
        c_air_drag_c += ( dc.turret > minrow ) ? 3 * c_air_mod : 0;
        // having a windmill is terrible for your drag
        c_air_drag_c += ( dc.windmill > minrow ) ? 5 * c_air_mod : 0;
        // rotors are not great for drag!
        c_air_drag_c += ( dc.rotor > minrow ) ? 6 * c_air_mod : 0;
        // having a sail is terrible for your drag
        c_air_drag_c += ( dc.sail > minrow ) ? 7 * c_air_mod : 0;
        c_air_drag += c_air_drag_c;
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
        c_height += ( dc.rotor > minrow ) ? rotor_height : 0;
        // sails are tall, too
        c_height += ( dc.sail > minrow ) ? sail_height : 0;
        height += c_height;
    }
    constexpr double air_density = 1.29; // kg/m^3
    // prorate per row height and c_air_drag
    height /= width;
    c_air_drag /= width;
    double cross_area = height * tile_to_width( width );
    add_msg_debug( "%s: height %3.2fm, width %3.2fm (%d tiles), c_air %3.2f\n", name, height,
                   tile_to_width( width ), width, c_air_drag );
    // F_air_drag = c_air_drag * cross_area * 1/2 * air_density * v^2
    // coeff_air_resistance = c_air_drag * cross_area * 1/2 * air_density
    coefficient_air_resistance = std::max( 0.1, c_air_drag * cross_area * 0.5 * air_density );
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
        for( int wheel : wheelcache ) {
            wheel_factor += parts[ wheel ].info().wheel_rolling_resistance();
        }
        // mildly increasing rolling resistance for vehicles with more than 4 wheels and mildly
        // decrease it for vehicles with less
        wheel_factor *= wheel_ratio /
                        ( base_wheels * wheel_ratio - base_wheels + wheelcache.size() );
    }
    coefficient_rolling_resistance = newton_ratio * wheel_factor * to_kilogram( total_mass() );
    coeff_rolling_dirty = false;
    return coefficient_rolling_resistance;
}

double vehicle::water_hull_height() const
{
    if( coeff_water_dirty ) {
        coeff_water_drag();
    }
    return hull_height;
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

// apologies for the imperial measurements, they'll get converted before used finally in the vehicle speed at the end of the function.
// sources for the equations to calculate rotor lift thrust were only available in imperial, and the constants used are designed for that.
// r= radius or d = diameter of rotor blades.
// area A [ft^2] = Pi * r^2 -or- A [ft^2] = (Pi/4) * D^2
// Power loading [hp/ft^2] = power( in hp ) / A
// thrust loading [lb/hp]= 8.6859 * Power loading^(-0.3107)
// Lift = Thrust loading * power >>>[lb] = [lb/hp] * [hp]

double vehicle::lift_thrust_of_rotorcraft( const bool fuelled, const bool safe ) const
{
    int total_diameter = 0;
    for( const int rotor : rotors ) {
        total_diameter += parts[ rotor ].info().rotor_diameter();
    }
    int total_engine_w = total_power_w( fuelled, safe );
    // take off 15 % due to the imaginary tail rotor power.
    double engine_power_in_hp = total_engine_w * 0.00134102;
    int rotor_area_in_feet = ( M_PI / 4 ) * std::pow( total_diameter * 3.28084, 2 );
    // lift_thrust in lbthrust
    double lift_thrust = ( 8.8658 * std::pow( engine_power_in_hp / rotor_area_in_feet,
                           -0.3107 ) ) * engine_power_in_hp;
    add_msg_debug(
        "lift thrust in lbs of %s = %f, rotor area in feet : %d, engine power in hp %f, thrust in newtons : %f",
        name, lift_thrust, rotor_area_in_feet, engine_power_in_hp, engine_power_in_hp * 4.45 );
    // convert to newtons.
    return lift_thrust * 4.45;
}

bool vehicle::has_sufficient_rotorlift() const
{
    // comparison of newton to newton - convert kg to newton.
    return lift_thrust_of_rotorcraft( true ) > to_kilogram( total_mass() ) * 9.8;
}

bool vehicle::is_rotorcraft() const
{
    return !rotors.empty() && player_in_control( get_player_character() ) &&
           has_sufficient_rotorlift();
}

bool vehicle::is_flyable() const
{
    return flyable;
}

void vehicle::set_flyable( bool val )
{
    flyable = val;
}

int vehicle::get_z_change() const
{
    return requested_z_change;
}

bool vehicle::would_install_prevent_flyable( const vpart_info &vpinfo, Character &pc ) const
{
    if( flyable && !rotors.empty() && !( vpinfo.has_flag( "SIMPLE_PART" ) ||
                                         vpinfo.has_flag( "AIRCRAFT_REPAIRABLE_NOPROF" ) ) ) {
        return !pc.has_proficiency( proficiency_prof_aircraft_mechanic );
    } else {
        return false;
    }
}

bool vehicle::would_repair_prevent_flyable( vehicle_part &vp, Character &pc ) const
{
    if( flyable && !rotors.empty() ) {
        if( vp.info().has_flag( "SIMPLE_PART" ) ||
            vp.info().has_flag( "AIRCRAFT_REPAIRABLE_NOPROF" ) ) {
            vpart_position vppos = vpart_position( const_cast<vehicle &>( *this ),
                                                   index_of_part( const_cast<vehicle_part *>( &vp ) ) );
            return !vppos.is_inside();
        } else {
            return !pc.has_proficiency( proficiency_prof_aircraft_mechanic );
        }
    } else {
        return false;
    }
}

bool vehicle::would_removal_prevent_flyable( vehicle_part &vp, Character &pc ) const
{
    if( flyable && !rotors.empty() && !vp.info().has_flag( "SIMPLE_PART" ) ) {
        return !pc.has_proficiency( proficiency_prof_aircraft_mechanic );
    } else {
        return false;
    }
}

bool vehicle::is_flying_in_air() const
{
    return is_flying;
}

void vehicle::set_flying( bool new_flying_value )
{
    is_flying = new_flying_value;
}

bool vehicle::is_watercraft() const
{
    return is_floating || ( in_water && wheelcache.empty() );
}

bool vehicle::is_in_water( bool deep_water ) const
{
    return deep_water ? is_floating : in_water;
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

    int tile_width = mount_max.y - mount_min.y + 1;
    double width_m = tile_to_width( tile_width );

    // actual area of the hull in m^2 (handles non-rectangular shapes)
    // footprint area in tiles = tile width * tile length
    // effective footprint percent = # of structure tiles / footprint area in tiles
    // actual hull area in m^2 = footprint percent * length in meters * width in meters
    // length in meters = length in tiles
    // actual area in m = # of structure tiles * length in tiles * width in meters /
    //                    ( length in tiles * width in tiles )
    // actual area in m = # of structure tiles * width in meters / width in tiles
    double actual_area_m = width_m * structure_indices.size() / tile_width;

    // effective hull area is actual hull area * hull coverage
    double hull_area_m   = actual_area_m * std::max( 0.1, hull_coverage );
    // Treat the hullform as a simple cuboid to calculate displaced depth of
    // water.
    // Apply Archimedes' principle (mass of water displaced is mass of vehicle).
    // area * depth = hull_volume = water_mass / water density
    // water_mass = vehicle_mass
    // area * depth = vehicle_mass / water_density
    // depth = vehicle_mass / water_density / area
    constexpr double water_density = 1000.0; // kg/m^3
    draft_m = to_kilogram( total_mass() ) / water_density / hull_area_m;
    // increase the streamlining as more of the boat is covered in boat boards
    double c_water_drag = 1.25 - hull_coverage;
    // hull height starts at 0.3m and goes up as you add more boat boards
    hull_height = 0.3 + 0.5 * hull_coverage;
    // F_water_drag = c_water_drag * cross_area * 1/2 * water_density * v^2
    // coeff_water_resistance = c_water_drag * cross_area * 1/2 * water_density
    coefficient_water_resistance = c_water_drag * width_m * draft_m * 0.5 * water_density;
    coeff_water_dirty = false;
    return coefficient_water_resistance;
}

float vehicle::k_traction( float wheel_traction_area ) const
{
    if( is_floating ) {
        return can_float() ? 1.0f : -1.0f;
    }
    if( is_flying ) {
        return is_rotorcraft() ? 1.0f : -1.0f;
    }
    if( is_watercraft() && can_float() ) {
        return 1.0f;
    }

    const float fraction_without_traction = 1.0f - wheel_traction_area / wheel_area();
    if( fraction_without_traction == 0 ) {
        return 1.0f;
    }
    const float mass_penalty = fraction_without_traction * to_kilogram( total_mass() );
    float traction = std::min( 1.0f, wheel_traction_area / mass_penalty );
    add_msg_debug( "%s has traction %.2f", name, traction );

    // For now make it easy until it gets properly balanced: add a low cap of 0.1
    return std::max( 0.1f, traction );
}

int vehicle::static_drag( bool actual ) const
{
    return extra_drag + ( actual && !engine_on && !is_towed() ? -1500 : 0 );
}

float vehicle::strain() const
{
    if( velocity == 0.0 ) {
        return 0.0f;
    }
    int mv = max_velocity();
    int sv = safe_velocity();
    if( mv <= sv ) {
        mv = sv + 1;
    }
    if( velocity < sv && velocity > -sv ) {
        return 0;
    } else {
        return static_cast<float>( std::abs( velocity ) - sv ) / static_cast<float>( mv - sv );
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

bool vehicle::is_owned_by( const Character &c, bool available_to_take ) const
{
    if( owner.is_null() ) {
        return available_to_take;
    }
    if( !c.get_faction() ) {
        debugmsg( "vehicle::is_owned_by() player %s has no faction", c.disp_name() );
        return false;
    }
    return c.get_faction()->id == get_owner();
}

bool vehicle::is_old_owner( const Character &c, bool available_to_take ) const
{
    if( old_owner.is_null() ) {
        return available_to_take;
    }
    if( !c.get_faction() ) {
        debugmsg( "vehicle::is_old_owner() player %s has no faction", c.disp_name() );
        return false;
    }
    return c.get_faction()->id == get_old_owner();
}

std::string vehicle::get_owner_name() const
{
    if( !g->faction_manager_ptr->get( owner ) ) {
        debugmsg( "vehicle::get_owner_name() vehicle %s has no valid nor null faction id ", disp_name() );
        return _( "no owner" );
    }
    return _( g->faction_manager_ptr->get( owner )->name );
}

void vehicle::set_owner( const Character &c )
{
    if( !c.get_faction() ) {
        debugmsg( "vehicle::set_owner() player %s has no valid faction", c.disp_name() );
        return;
    }
    owner = c.get_faction()->id;
}

bool vehicle::handle_potential_theft( player &p, bool check_only, bool prompt )
{
    const bool is_owned_by_player = is_owned_by( p );
    std::vector<npc *> witnesses;
    for( npc &elem : g->all_npcs() ) {
        if( rl_dist( elem.pos(), p.pos() ) < MAX_VIEW_DISTANCE && has_owner() &&
            !is_owned_by_player && elem.sees( p.pos() ) ) {
            witnesses.push_back( &elem );
        }
    }
    // the vehicle is yours, that's fine.
    if( is_owned_by_player ) {
        return true;
        // if There is no owner
        // handle transfer of ownership
    } else if( !has_owner() ) {
        set_owner( p.get_faction()->id );
        remove_old_owner();
        return true;
        // if there is a marker for having been stolen, but 15 minutes have passed, then officially transfer ownership
    } else if( witnesses.empty() && has_old_owner() && !is_old_owner( p ) && theft_time &&
               calendar::turn - *theft_time > 15_minutes ) {
        set_owner( p.get_faction()->id );
        remove_old_owner();
        return true;
        // No witnesses? then don't need to prompt, we assume the player is in process of stealing it.
        // Ownership transfer checking is handled above, and warnings handled below.
        // This is just to perform interaction with the vehicle without a prompt.
        // It will prompt first-time, even with no witnesses, to inform player it is owned by someone else
        // subsequently, no further prompts, the player should know by then.
    } else if( witnesses.empty() && old_owner ) {
        return true;
    }
    // if we are just checking if we could continue without problems, then the rest is assumed false
    if( check_only ) {
        return false;
    }
    // if we got here, there's some theft occurring
    if( prompt ) {
        if( !query_yn(
                _( "This vehicle belongs to: %s, there may be consequences if you are observed interacting with it, continue?" ),
                _( get_owner_name() ) ) ) {
            return false;
        }
    }
    // set old owner so that we can restore ownership if there are witnesses.
    set_old_owner( get_owner() );
    for( npc *elem : witnesses ) {
        elem->say( "<witnessed_thievery>", 7 );
    }
    if( !witnesses.empty() ) {
        if( p.add_faction_warning( get_owner() ) ) {
            for( npc *elem : witnesses ) {
                elem->make_angry();
            }
        }
        // remove the temporary marker for a successful theft, as it was witnessed.
        remove_old_owner();
    }
    // if we got here, then the action will proceed after the previous warning
    return true;
}

bool vehicle::balanced_wheel_config() const
{
    point min = point_max;
    point max = point_min;
    // find the bounding box of the wheels
    for( const int &w : wheelcache ) {
        const point &pt = parts[ w ].mount;
        min.x = std::min( min.x, pt.x );
        min.y = std::min( min.y, pt.y );
        max.x = std::max( max.x, pt.x );
        max.y = std::max( max.y, pt.y );
    }

    // Check center of mass inside support of wheels (roughly)
    const point &com = local_center_of_mass();
    const inclusive_rectangle<point> support( min, max );
    return support.contains( com );
}

bool vehicle::valid_wheel_config() const
{
    return sufficient_wheel_config() && balanced_wheel_config();
}

float vehicle::steering_effectiveness() const
{
    if( is_floating ) {
        // I'M ON A BOAT
        return can_float() ? 1.0f : 0.0f;
    }
    if( is_flying ) {
        // I'M IN THE AIR
        return is_rotorcraft() ? 1.0f : 0.0f;
    }
    // irksome special case for boats in shallow water
    if( is_watercraft() && can_float() ) {
        return 1.0f;
    }

    if( steering.empty() ) {
        return -1.0f; // No steering installed
    }
    // If the only steering part is an animal harness, with no animal in, it
    // is not steerable.
    const vehicle_part &vp = parts[ steering[0] ];
    if( steering.size() == 1 && vp.info().fuel_type == fuel_type_animal ) {
        monster *mon = get_monster( steering[0] );
        if( mon == nullptr || !mon->has_effect( effect_harnessed ) ) {
            return -2.0f;
        }
    }
    // For now, you just need one wheel working for 100% effective steering.
    // TODO: return something less than 1.0 if the steering isn't so good
    // (unbalanced, long wheelbase, back-heavy vehicle with front wheel steering,
    // etc)
    for( int p : steering ) {
        if( parts[ p ].is_available() ) {
            return 1.0f;
        }
    }

    // We have steering, but it's all broken.
    return 0.0f;
}

float vehicle::handling_difficulty() const
{
    const float steer = std::max( 0.0f, steering_effectiveness() );
    const float ktraction = k_traction( get_map().vehicle_wheel_traction( *this ) );
    const float aligned = std::max( 0.0f, 1.0f - ( face_vec() - dir_vec() ).magnitude() );

    // TestVehicle: perfect steering, moving on road at 100 mph (25 tiles per turn) = 0.0
    // TestVehicle but on grass (0.75 friction) = 2.5
    // TestVehicle but with bad steering (0.5 steer) = 5
    // TestVehicle but on fungal bed (0.5 friction) and bad steering = 10
    // TestVehicle but turned 90 degrees during this turn (0 align) = 10
    const float diff_mod = ( ( 1.0f - steer ) + ( 1.0f - ktraction ) + ( 1.0f - aligned ) );
    return velocity * diff_mod / vehicles::vmiph_per_tile;
}

int vehicle::engine_fuel_usage( int e ) const
{
    if( !is_engine_on( e ) ) {
        return 0;
    }

    static const itype_id null_fuel_type( "null" );
    const itype_id &cur_fuel = parts[engines[e]].fuel_current();
    if( cur_fuel  == null_fuel_type ) {
        return 0;
    }

    if( is_perpetual_type( e ) ) {
        return 0;
    }
    const auto &info = part_info( engines[ e ] );

    int usage = info.energy_consumption;
    if( parts[ engines[ e ] ].has_fault_flag( "DOUBLE_FUEL_CONSUMPTION" ) ) {
        usage *= 2;
    }

    return usage;
}

std::map<itype_id, int> vehicle::fuel_usage() const
{
    std::map<itype_id, int> ret;
    for( size_t i = 0; i < engines.size(); i++ ) {
        // Note: functions with "engine" in name do NOT take part indices
        // TODO: Use part indices and not engine vector indices
        if( !is_engine_on( i ) ) {
            continue;
        }

        const size_t e = engines[ i ];
        static const itype_id null_fuel_type( "null" );
        const itype_id &cur_fuel = parts[ e ].fuel_current();
        if( cur_fuel  == null_fuel_type ) {
            continue;
        }

        if( !is_perpetual_type( i ) ) {
            ret[cur_fuel] += engine_fuel_usage( i );
        }
    }

    return ret;
}

double vehicle::drain_energy( const itype_id &ftype, double energy_j )
{
    double drained = 0.0f;
    for( vehicle_part &p : parts ) {
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
    for( const auto &fuel_pr : fuel_usage() ) {
        const itype_id &ft = fuel_pr.first;
        if( skip_electric && ft == fuel_type_battery ) {
            continue;
        }

        double amnt_precise_j = static_cast<double>( fuel_pr.second ) * t_seconds;
        amnt_precise_j *= load / 1000.0 * ( 1.0 + st * st * 100.0 );
        auto inserted = fuel_used_last_turn.insert( { ft, 0.0f } );
        inserted.first->second += amnt_precise_j;
        double remainder = fuel_remainder[ ft ];
        amnt_precise_j -= remainder;

        if( amnt_precise_j > 0.0f ) {
            fuel_remainder[ ft ] = drain_energy( ft, amnt_precise_j ) - amnt_precise_j;
        } else {
            fuel_remainder[ ft ] = -amnt_precise_j;
        }
    }
    Character &player_character = get_player_character();
    if( load > 0 && fuel_left( fuel_type_muscle ) > 0 &&
        player_character.has_effect( effect_winded ) ) {
        cruise_velocity = 0;
        if( velocity == 0 ) {
            stop();
        }
    }
    // we want this to update the activity level whenever the engine is running
    if( load > 0 && fuel_left( fuel_type_muscle ) > 0 ) {
        player_character.increase_activity_level( ACTIVE_EXERCISE );
        //do this as a function of current load
        // But only if the player is actually there!
        int eff_load = load / 10;
        int mod = 4 * st; // strain
        int base_burn = static_cast<int>( get_option<float>( "PLAYER_BASE_STAMINA_REGEN_RATE" ) ) -
                        3;
        base_burn = std::max( eff_load / 3, base_burn );
        //charge bionics when using muscle engine
        const item muscle( "muscle" );
        for( const bionic_id &bid : player_character.get_bionic_fueled_with( muscle ) ) {
            if( player_character.has_active_bionic( bid ) ) { // active power gen
                // more pedaling = more power
                player_character.mod_power_level( units::from_kilojoule( muscle.fuel_energy() ) *
                                                  bid->fuel_efficiency *
                                                  ( load / 1000 ) );
                mod += eff_load / 5;
            } else { // passive power gen
                player_character.mod_power_level( units::from_kilojoule( muscle.fuel_energy() ) *
                                                  bid->passive_fuel_efficiency *
                                                  ( load / 1000 ) );
                mod += eff_load / 10;
            }
        }
        // decreased stamina burn scalable with load
        if( player_character.has_active_bionic( bio_jointservo ) ) {
            player_character.mod_power_level( units::from_kilojoule( -std::max( eff_load / 20, 1 ) ) );
            mod -= std::max( eff_load / 5, 5 );
        }
        if( one_in( 1000 / load ) && one_in( 10 ) ) {
            player_character.mod_thirst( 1 );
            player_character.mod_fatigue( 1 );
        }
        player_character.mod_stamina( -( base_burn + mod ) );
        add_msg_debug( "Load: %d", load );
        add_msg_debug( "Mod: %d", mod );
        add_msg_debug( "Burn: %d", -( base_burn + mod ) );
    }
}

std::vector<vehicle_part *> vehicle::lights( bool active )
{
    std::vector<vehicle_part *> res;
    for( vehicle_part &e : parts ) {
        if( ( !active || e.enabled ) && e.is_available() && e.is_light() ) {
            res.push_back( &e );
        }
    }
    return res;
}

int vehicle::total_accessory_epower_w() const
{
    int epower = 0;
    for( int part : accessories ) {
        const vehicle_part &vp = parts[part];
        if( vp.enabled ) {
            epower += vp.info().epower;
        }
    }
    return epower;
}

std::pair<int, int> vehicle::battery_power_level() const
{
    int total_epower_capacity = 0;
    int remaining_epower = 0;

    for( const int bi : batteries ) {
        const vehicle_part &b = parts[bi];
        if( b.is_available() ) {
            remaining_epower += b.ammo_remaining();
            total_epower_capacity += b.ammo_capacity( ammotype( "battery" ) );
        }
    }

    return std::make_pair( remaining_epower, total_epower_capacity );
}

bool vehicle::start_engine( int e, bool turn_on )
{
    if( parts[engines[e]].enabled == turn_on ) {
        return false;
    }
    bool res = false;
    if( turn_on ) {
        toggle_specific_engine( e, true );
        // prevent starting of the faulty engines
        if( ! start_engine( e ) ) {
            toggle_specific_engine( e, false );
        } else {
            res = true;
        }
    } else {
        toggle_specific_engine( e, false );
        res = true;
    }
    return res;
}

int vehicle::total_alternator_epower_w() const
{
    int epower = 0;
    if( engine_on ) {
        // If the engine is on, the alternators are working.
        for( size_t p = 0; p < alternators.size(); ++p ) {
            if( is_alternator_on( p ) ) {
                epower += part_epower_w( alternators[p] );
            }
        }
    }
    return epower;
}

int vehicle::total_engine_epower_w() const
{
    int epower = 0;

    // Engines: can both produce (plasma) or consume (gas, diesel) epower.
    // Gas engines require epower to run for ignition system, ECU, etc.
    // Electric motor consumption not included, see @ref vpart_info::energy_consumption
    if( engine_on ) {
        for( size_t e = 0; e < engines.size(); ++e ) {
            if( is_engine_on( e ) ) {
                epower += part_epower_w( engines[e] );
            }
        }
    }

    return epower;
}

int vehicle::total_solar_epower_w() const
{
    int epower_w = 0;
    map &here = get_map();
    for( int part : solar_panels ) {
        if( parts[ part ].is_unavailable() ) {
            continue;
        }

        if( !is_sm_tile_outside( here.getabs( global_part_pos3( part ) ) ) ) {
            continue;
        }

        epower_w += part_epower_w( part );
    }
    // Weather doesn't change much across the area of the vehicle, so just
    // sample it once.
    weather_type_id wtype = current_weather( global_pos3() );
    const float tick_sunlight = incident_sunlight( wtype, calendar::turn );
    double intensity = tick_sunlight / default_daylight_level();
    return epower_w * intensity;
}

int vehicle::total_wind_epower_w() const
{
    map &here = get_map();
    // TODO: fix point types
    const oter_id &cur_om_ter =
        overmap_buffer.ter( tripoint_abs_omt( ms_to_omt_copy( here.getabs( global_pos3() ) ) ) );
    weather_manager &weather = get_weather();
    const w_point weatherPoint = *weather.weather_precise;
    int epower_w = 0;
    for( int part : wind_turbines ) {
        if( parts[ part ].is_unavailable() ) {
            continue;
        }

        if( !is_sm_tile_outside( here.getabs( global_part_pos3( part ) ) ) ) {
            continue;
        }

        double windpower = get_local_windpower( weather.windspeed, cur_om_ter, global_part_pos3( part ),
                                                weather.winddirection, false );
        if( windpower <= ( weather.windspeed / 10.0 ) ) {
            continue;
        }
        epower_w += part_epower_w( part ) * windpower;
    }
    return epower_w;
}

int vehicle::total_water_wheel_epower_w() const
{
    int epower_w = 0;
    map &here = get_map();
    for( int part : water_wheels ) {
        if( parts[ part ].is_unavailable() ) {
            continue;
        }

        if( !is_sm_tile_over_water( here.getabs( global_part_pos3( part ) ) ) ) {
            continue;
        }

        epower_w += part_epower_w( part );
    }
    // TODO: river current intensity changes power - flat for now.
    return epower_w;
}

int vehicle::net_battery_charge_rate_w() const
{
    return total_engine_epower_w() + total_alternator_epower_w() + total_accessory_epower_w() +
           total_solar_epower_w() + total_wind_epower_w() + total_water_wheel_epower_w();
}

int vehicle::max_reactor_epower_w() const
{
    int epower_w = 0;
    for( int elem : reactors ) {
        epower_w += is_part_on( elem ) ? part_epower_w( elem ) : 0;
    }
    return epower_w;
}

void vehicle::update_alternator_load()
{
    // Update alternator load
    if( engine_on ) {
        int engine_vpower = 0;
        for( size_t e = 0; e < engines.size(); ++e ) {
            if( is_engine_on( e ) && parts[engines[e]].info().has_flag( "E_ALTERNATOR" ) ) {
                engine_vpower += part_vpower_w( engines[e] );
            }
        }
        int alternators_power = 0;
        for( size_t p = 0; p < alternators.size(); ++p ) {
            if( is_alternator_on( p ) ) {
                alternators_power += part_vpower_w( alternators[p] );
            }
        }
        alternator_load =
            engine_vpower
            ? 1000 * ( std::abs( alternators_power ) + std::abs( extra_drag ) ) / engine_vpower
            : 0;
    } else {
        alternator_load = 0;
    }
}

void vehicle::power_parts()
{
    update_alternator_load();
    // Things that drain energy: engines and accessories.
    int engine_epower = total_engine_epower_w();
    int epower = engine_epower + total_accessory_epower_w() + total_alternator_epower_w();

    int delta_energy_bat = power_to_energy_bat( epower, 1_turns );
    int battery_left, battery_capacity;
    std::tie( battery_left, battery_capacity ) = battery_power_level();
    int storage_deficit_bat = std::max( 0, battery_capacity - battery_left - delta_energy_bat );
    Character &player_character = get_player_character();
    // Reactors trigger only on demand. If we'd otherwise run out of power, see
    // if we can spin up the reactors.
    if( !reactors.empty() && storage_deficit_bat > 0 ) {
        // Still not enough surplus epower to fully charge battery
        // Produce additional epower from any reactors
        bool reactor_working = false;
        bool reactor_online = false;
        for( int elem : reactors ) {
            // Check whether the reactor is on. If not, move on.
            if( !is_part_on( elem ) ) {
                continue;
            }
            // Keep track whether or not the vehicle has any reactors activated
            reactor_online = true;
            // the amount of energy the reactor generates each turn
            const int gen_energy_bat = power_to_energy_bat( part_epower_w( elem ), 1_turns );
            if( parts[ elem ].is_unavailable() ) {
                continue;
            } else if( parts[ elem ].info().has_flag( STATIC( std::string( "PERPETUAL" ) ) ) ) {
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
            for( int elem : reactors ) {
                parts[ elem ].enabled = false;
            }
            if( player_in_control( player_character ) || player_character.sees( global_pos3() ) ) {
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
        battery_deficit = discharge_battery( std::abs( delta_energy_bat ) );
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
        if( player_in_control( player_character ) || player_character.sees( global_pos3() ) ) {
            add_msg( _( "The %s's battery dies!" ), name );
        }
        if( engine_epower < 0 ) {
            // Not enough epower to run gas engine ignition system
            engine_on = false;
            if( player_in_control( player_character ) || player_character.sees( global_pos3() ) ) {
                add_msg( _( "The %s's engine dies!" ), name );
            }
        }
    }
}

vehicle *vehicle::find_vehicle( const tripoint &where )
{
    map &here = get_map();
    // Is it in the reality bubble?
    tripoint veh_local = here.getlocal( where );
    if( const optional_vpart_position vp = here.veh_at( veh_local ) ) {
        return &vp->vehicle();
    }

    // Nope. Load up its submap...
    tripoint veh_in_sm = where;
    tripoint veh_sm = ms_to_sm_remain( veh_in_sm );

    const submap *sm = MAPBUFFER.lookup_submap( veh_sm );
    if( sm == nullptr ) {
        return nullptr;
    }

    for( const auto &elem : sm->vehicles ) {
        vehicle *found_veh = elem.get();
        if( veh_in_sm.xy() == found_veh->pos ) {
            return found_veh;
        }
    }

    return nullptr;
}

void vehicle::enumerate_vehicles( std::map<vehicle *, bool> &connected_vehicles,
                                  std::set<vehicle *> &vehicle_list )
{
    auto enumerate_visitor = [&connected_vehicles]( vehicle * veh, int amount, int ) {
        // Only emplaces if element is not present already.
        connected_vehicles.emplace( veh, false );
        return amount;
    };
    for( vehicle *veh : vehicle_list ) {
        // This autovivifies, and also overwrites the value if already present.
        connected_vehicles[veh] = true;
        traverse_vehicle_graph( veh, 1, enumerate_visitor );
    }
}

template <typename Func, typename Vehicle>
int vehicle::traverse_vehicle_graph( Vehicle *start_veh, int amount, Func action )
{
    if( start_veh->loose_parts.empty() ) {
        return amount;
    }
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

        add_msg_debug( "Traversing graph with %d power", amount );

        for( int p : current_veh->loose_parts ) {
            if( !current_veh->part_info( p ).has_flag( "POWER_TRANSFER" ) ) {
                continue; // ignore loose parts that aren't power transfer cables
            }

            vehicle *target_veh = vehicle::find_vehicle( current_veh->parts[p].target.second );
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

                float loss_amount = ( static_cast<float>( amount ) * static_cast<float>( target_loss ) ) / 100.0f;
                add_msg_debug( "Visiting remote %p with %d power (loss %f, which is %d percent)",
                               static_cast<void *>( target_veh ), amount, loss_amount, target_loss );

                amount = action( target_veh, amount, static_cast<int>( loss_amount ) );
                add_msg_debug( "After remote %p, %d power", static_cast<void *>( target_veh ), amount );

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
        if( p.is_available() && p.is_battery() &&
            p.ammo_capacity( ammotype( "battery" ) ) > p.ammo_remaining() ) {
            chargeable_parts.insert( { ( p.ammo_remaining() * 100 ) / p.ammo_capacity( ammotype( "battery" ) ), &p } );
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
        int next_charge_level = ( ( charge_level + 1 ) * p->ammo_capacity( ammotype( "battery" ) ) ) / 100;
        next_charge_level = std::max( next_charge_level, p->ammo_remaining() + 1 );
        int qty = std::min( amount, next_charge_level - p->ammo_remaining() );
        p->ammo_set( fuel_type_battery, p->ammo_remaining() + qty );
        amount -= qty;
        if( p->ammo_capacity( ammotype( "battery" ) ) > p->ammo_remaining() ) {
            chargeable_parts.insert( { ( p->ammo_remaining() * 100 ) / p->ammo_capacity( ammotype( "battery" ) ), p } );
        }
    }

    auto charge_visitor = []( vehicle * veh, int amount, int lost ) {
        add_msg_debug( "CH: %d", amount - lost );
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
            dischargeable_parts.insert( { ( p.ammo_remaining() * 100 ) / p.ammo_capacity( ammotype( "battery" ) ), &p } );
        }
    }
    while( amount > 0 && !dischargeable_parts.empty() ) {
        // Grab first part, discharge until it reaches the next %, then re-insert with new % key.
        auto iter = std::prev( dischargeable_parts.end() );
        int charge_level = iter->first;
        vehicle_part *p = iter->second;
        dischargeable_parts.erase( iter );
        // Calculate number of charges to reach the previous %.
        int prev_charge_level = ( ( charge_level - 1 ) * p->ammo_capacity( ammotype( "battery" ) ) ) / 100;
        int amount_to_discharge = std::min( p->ammo_remaining() - prev_charge_level, amount );
        p->ammo_consume( amount_to_discharge, global_part_pos3( *p ) );
        amount -= amount_to_discharge;
        if( p->ammo_remaining() > 0 ) {
            dischargeable_parts.insert( { ( p->ammo_remaining() * 100 ) / p->ammo_capacity( ammotype( "battery" ) ), p } );
        }
    }

    auto discharge_visitor = []( vehicle * veh, int amount, int lost ) {
        add_msg_debug( "CH: %d", amount + lost );
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
        engine_fuel_left( e ) && rng( 1, 100 ) < strain ) {
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
    avg_velocity = ( velocity + avg_velocity ) / 2;

    power_parts();
    Character &player_character = get_player_character();
    if( engine_on && total_power_w() > 0 ) {
        int idle_rate = alternator_load;
        if( idle_rate < 10 ) {
            idle_rate = 10;    // minimum idle is 1% of full throttle
        }
        // helicopters use basically nearly all of their power just to hover.
        // it becomes more efficient the closer they reach their safe cruise speed.
        if( is_rotorcraft() && is_flying_in_air() ) {
            idle_rate = 1000;
        }
        if( has_engine_type_not( fuel_type_muscle, true ) ) {
            consume_fuel( idle_rate, to_turns<int>( 1_turns ), true );
        }

        if( on_map ) {
            noise_and_smoke( idle_rate, 1_turns );
        }
    } else {
        if( engine_on &&
            ( has_engine_type_not( fuel_type_muscle, true ) && has_engine_type_not( fuel_type_animal, true ) &&
              has_engine_type_not( fuel_type_wind, true ) && has_engine_type_not( fuel_type_mana, true ) ) ) {
            add_msg_if_player_sees( global_pos3(), _( "The %s's engine dies!" ), name );
        }
        engine_on = false;
    }

    if( !warm_enough_to_plant( player_character.pos() ) ) {
        for( int i : planters ) {
            vehicle_part &vp = parts[ i ];
            if( vp.enabled ) {
                add_msg_if_player_sees( global_pos3(), _( "The %s's planter turns off due to low temperature." ),
                                        name );
                vp.enabled = false;
            }
        }
    }

    smart_controller_handle_turn();

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

    if( has_part( "CRASH_TERRAIN_AROUND", true ) ) {
        crash_terrain_around();
    }

    if( is_alarm_on ) {
        alarm();
    }
}

void vehicle::on_move()
{
    if( has_part( "TRANSFORM_TERRAIN", true ) ) {
        transform_terrain();
    }
    if( has_part( "SCOOP", true ) ) {
        operate_scoop();
    }
    if( has_part( "PLANTER", true ) ) {
        operate_planter();
    }
    if( has_part( "REAPER", true ) ) {
        operate_reaper();
    }
}

void vehicle::slow_leak()
{
    map &here = get_map();
    // for each badly damaged tanks (lower than 50% health), leak a small amount
    for( int part : fuel_containers ) {
        vehicle_part &p = parts[part];
        if( !p.is_leaking() || p.ammo_remaining() <= 0 ) {
            continue;
        }

        double health = p.health_percent();
        itype_id fuel = p.ammo_current();
        int qty = std::max( ( 0.5 - health ) * ( 0.5 - health ) * p.ammo_remaining() / 10, 1.0 );
        point q = coord_translate( p.mount );
        const tripoint dest = global_pos3() + tripoint( q, 0 );

        // damaged batteries self-discharge without leaking, plutonium leaks slurry
        if( fuel != fuel_type_battery && fuel != fuel_type_plutonium_cell ) {
            item leak( fuel, calendar::turn, qty );
            here.add_item_or_charges( dest, leak );
            p.ammo_consume( qty, global_part_pos3( p ) );
        } else if( fuel == fuel_type_plutonium_cell ) {
            if( p.ammo_remaining() >= PLUTONIUM_CHARGES / 10 ) {
                item leak( "plut_slurry_dense", calendar::turn, qty );
                here.add_item_or_charges( dest, leak );
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
    item &target = *loc;
    if( !target.needs_processing() ) {
        return;
    }
    auto cargo_parts = get_parts_at( loc.position(), "CARGO", part_status_flag::any );
    if( cargo_parts.empty() ) {
        return;
    }
    // System insures that there is only one part in this vector.
    vehicle_part *cargo_part = cargo_parts.front();
    active_items.add( target, cargo_part->mount );
}

int vehicle::add_charges( int part, const item &itm )
{
    if( !itm.count_by_charges() ) {
        debugmsg( "Add charges was called for an item not counted by charges!" );
        return 0;
    }
    const int ret = get_items( part ).amount_can_fit( itm );
    if( ret == 0 ) {
        return 0;
    }

    item itm_copy = itm;
    itm_copy.charges = ret;
    return add_item( part, itm_copy ) ? ret : 0;
}

cata::optional<vehicle_stack::iterator> vehicle::add_item( vehicle_part &pt, const item &obj )
{
    int idx = index_of_part( &pt );
    if( idx < 0 ) {
        debugmsg( "Tried to add item to invalid part" );
        return cata::nullopt;
    }
    return add_item( idx, obj );
}

cata::optional<vehicle_stack::iterator> vehicle::add_item( int part, const item &itm )
{
    if( part < 0 || part >= static_cast<int>( parts.size() ) ) {
        debugmsg( "int part (%d) is out of range", part );
        return cata::nullopt;
    }
    // const int max_weight = ?! // TODO: weight limit, calculation per vpart & vehicle stats, not a hard user limit.
    // add creaking sounds and damage to overloaded vpart, outright break it past a certain point, or when hitting bumps etc
    vehicle_part &p = parts[ part ];
    if( p.is_broken() ) {
        return cata::nullopt;
    }

    if( p.base.is_gun() ) {
        if( !itm.is_ammo() || !p.base.ammo_types().count( itm.ammo_type() ) ) {
            return cata::nullopt;
        }
    }
    bool charge = itm.count_by_charges();
    vehicle_stack istack = get_items( part );
    const int to_move = istack.amount_can_fit( itm );
    if( to_move == 0 || ( charge && to_move < itm.charges ) ) {
        return cata::nullopt; // @add_charges should be used in the latter case
    }
    if( charge ) {
        item *here = istack.stacks_with( itm );
        if( here ) {
            invalidate_mass();
            if( !here->merge_charges( itm ) ) {
                return cata::nullopt;
            } else {
                return cata::optional<vehicle_stack::iterator>( istack.get_iterator_from_pointer( here ) );
            }
        }
    }

    item itm_copy = itm;

    if( itm_copy.is_bucket_nonempty() ) {
        // this is a vehicle, so there is only one pocket.
        // so if it will spill, spill all of it
        itm_copy.contents.spill_contents( global_part_pos3( part ) );
    }

    const vehicle_stack::iterator new_pos = p.items.insert( itm_copy );
    if( itm_copy.needs_processing() ) {
        active_items.add( *new_pos, p.mount );
    }

    invalidate_mass();
    return cata::optional<vehicle_stack::iterator>( new_pos );
}

bool vehicle::remove_item( int part, item *it )
{
    const cata::colony<item> &veh_items = parts[part].items;
    const cata::colony<item>::const_iterator iter = veh_items.get_iterator_from_pointer( it );
    if( iter == veh_items.end() ) {
        return false;
    }
    remove_item( part, iter );
    return true;
}

vehicle_stack::iterator vehicle::remove_item( int part, const vehicle_stack::const_iterator &it )
{
    cata::colony<item> &veh_items = parts[part].items;

    // remove from the active items cache (if it isn't there does nothing)
    active_items.remove( &*it );

    invalidate_mass();
    return veh_items.erase( it );
}

vehicle_stack vehicle::get_items( const int part )
{
    const tripoint pos = global_part_pos3( part );
    return vehicle_stack( &parts[part].items, pos.xy(), this, part );
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

    for( const vehicle_prototype::part_def &pt : type->parts ) {
        if( pt.with_ammo ) {
            int turret = part_with_feature( pt.pos, "TURRET", true );
            if( turret >= 0 && x_in_y( pt.with_ammo, 100 ) ) {
                parts[ turret ].ammo_set( random_entry( pt.ammo_types ), rng( pt.ammo_qty.first,
                                          pt.ammo_qty.second ) );
            }
        }
    }

    const float spawn_rate = get_option<float>( "ITEM_SPAWNRATE" );
    for( const vehicle_item_spawn &spawn : type.obj().item_spawns ) {
        int part = part_with_feature( spawn.pos, "CARGO", false );
        if( part < 0 ) {
            debugmsg( "No CARGO parts at (%d, %d) of %s!", spawn.pos.x, spawn.pos.y, name );
        } else {
            bool broken = parts[ part ].is_broken();

            std::vector<item> created;
            const int spawn_count = roll_remainder( spawn.chance * std::max( spawn_rate, 1.0f ) / 100.0f );
            for( int i = 0; i < spawn_count; ++i ) {
                // if vehicle part is broken only 50% of items spawn and they will be variably damaged
                if( broken && one_in( 2 ) ) {
                    continue;
                }

                for( const itype_id &e : spawn.item_ids ) {
                    if( rng_float( 0, 1 ) < spawn_rate ) {
                        created.emplace_back( item( e ).in_its_container() );
                    }
                }
                for( const item_group_id &e : spawn.item_groups ) {
                    item_group::ItemList group_items = item_group::items_from( e, calendar::start_of_cataclysm,
                                                       spawn_flags::use_spawn_rate );
                    created.insert( created.end(), group_items.begin(), group_items.end() );
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
                        item mag( e.magazine_default(), e.birthday() );
                        if( spawn_ammo ) {
                            mag.ammo_set( mag.ammo_default() );
                        }
                        e.put_in( mag, item_pocket::pocket_type::MAGAZINE_WELL );
                    } else if( spawn_ammo && e.is_magazine() ) {
                        e.ammo_set( e.ammo_default() );
                    }
                }
                add_item( part, e );
            }
        }
    }
}

void vehicle::gain_moves()
{
    fuel_used_last_turn.clear();
    check_falling_or_floating();
    const bool pl_control = player_in_control( get_player_character() );
    if( is_moving() || is_falling ) {
        if( !loose_parts.empty() ) {
            shed_loose_parts();
        }
        of_turn = 1 + of_turn_carry;
        const int vslowdown = slowdown( velocity );
        if( vslowdown > std::abs( velocity ) ) {
            if( cruise_on && cruise_velocity && pl_control ) {
                velocity = velocity > 0 ? 1 : -1;
            } else {
                stop();
            }
        } else if( velocity < 0 ) {
            velocity += vslowdown;
        } else {
            velocity -= vslowdown;
        }
        is_on_ramp = false;
    } else {
        of_turn = .001;
    }
    of_turn_carry = 0;
    // cruise control TODO: enable for NPC?
    if( ( pl_control || is_following || is_patrolling ) && cruise_on && cruise_velocity != velocity ) {
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
    for( vehicle_part *e : turrets() ) {
        if( e->enabled || e->target.second != e->target.first ) {
            automatic_fire_turret( *e );
        }
    }

    if( velocity < 0 ) {
        beeper_sound();
    }
}

void vehicle::dump_items_from_part( const size_t index )
{
    map &here = get_map();
    vehicle_part &vp = parts[ index ];
    for( item &e : vp.items ) {
        here.add_item_or_charges( global_part_pos3( vp ), e );
    }
    vp.items.clear();
}

bool vehicle::decrement_summon_timer()
{
    if( !summon_time_limit ) {
        return false;
    }
    if( *summon_time_limit <= 0_turns ) {
        for( const vpart_reference &vp : get_all_parts() ) {
            const size_t p = vp.part_index();
            dump_items_from_part( p );
        }
        add_msg_if_player_sees( global_pos3(), m_info, _( "Your %s winks out of existence." ), name );
        get_map().destroy_vehicle( this );
        return true;
    } else {
        *summon_time_limit -= 1_turns;
    }
    return false;
}

void vehicle::suspend_refresh()
{
    // disable refresh and cache recalculation
    no_refresh = true;
    mass_dirty = false;
    mass_center_precalc_dirty = false;
    mass_center_no_precalc_dirty = false;
    coeff_rolling_dirty = false;
    coeff_air_dirty = false;
    coeff_water_dirty = false;
    coeff_air_changed = false;
}

void vehicle::enable_refresh()
{
    // force all caches to recalculate
    no_refresh = false;
    mass_dirty = true;
    mass_center_precalc_dirty = true;
    mass_center_no_precalc_dirty = true;
    coeff_rolling_dirty = true;
    coeff_air_dirty = true;
    coeff_water_dirty = true;
    coeff_air_changed = true;
    refresh();
}

/**
 * Refreshes all caches and refinds all parts. Used after the vehicle has had a part added or removed.
 * Makes indices of different part types so they're easy to find. Also calculates power drain.
 */
void vehicle::refresh()
{
    if( no_refresh ) {
        return;
    }

    alternators.clear();
    engines.clear();
    reactors.clear();
    solar_panels.clear();
    wind_turbines.clear();
    sails.clear();
    water_wheels.clear();
    funnels.clear();
    emitters.clear();
    relative_parts.clear();
    loose_parts.clear();
    wheelcache.clear();
    rail_wheelcache.clear();
    rotors.clear();
    steering.clear();
    speciality.clear();
    floating.clear();
    batteries.clear();
    fuel_containers.clear();
    turret_locations.clear();
    mufflers.clear();
    planters.clear();
    accessories.clear();

    alternator_load = 0;
    extra_drag = 0;
    all_wheels_on_one_axis = true;
    int first_wheel_y_mount = INT_MAX;

    // Used to sort part list so it displays properly when examining
    struct sort_veh_part_vector {
        vehicle *veh;
        inline bool operator()( const int p1, const int p2 ) {
            return veh->part_info( p1 ).list_order < veh->part_info( p2 ).list_order;
        }
    } svpv = { this };

    mount_min.x = 123;
    mount_min.y = 123;
    mount_max.x = -123;
    mount_max.y = -123;

    int railwheel_xmin = INT_MAX;
    int railwheel_ymin = INT_MAX;
    int railwheel_xmax = INT_MIN;
    int railwheel_ymax = INT_MIN;

    has_enabled_smart_controller = false;
    smart_controller_state = cata::nullopt;

    bool refresh_done = false;

    // Main loop over all vehicle parts.
    for( const vpart_reference &vp : get_all_parts() ) {
        const size_t p = vp.part_index();
        const vpart_info &vpi = vp.info();
        if( vp.part().removed ) {
            continue;
        }
        refresh_done = true;

        // Build map of point -> all parts in that point
        const point pt = vp.mount();
        mount_min.x = std::min( mount_min.x, pt.x );
        mount_min.y = std::min( mount_min.y, pt.y );
        mount_max.x = std::max( mount_max.x, pt.x );
        mount_max.y = std::max( mount_max.y, pt.y );

        // This will keep the parts at point pt sorted
        std::vector<int>::iterator vii = std::lower_bound( relative_parts[pt].begin(),
                                         relative_parts[pt].end(),
                                         static_cast<int>( p ), svpv );
        relative_parts[pt].insert( vii, p );

        if( vpi.has_flag( VPFLAG_FLOATS ) ) {
            floating.push_back( p );
        }

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
        if( vpi.has_flag( VPFLAG_ROTOR ) || vpi.has_flag( VPFLAG_ROTOR_SIMPLE ) ) {
            rotors.push_back( p );
        }
        if( vp.part().is_battery() ) {
            batteries.push_back( p );
        }
        if( vp.part().is_fuel_store( false ) ) {
            fuel_containers.push_back( p );
        }
        if( vp.part().is_turret() ) {
            turret_locations.push_back( p );
        }
        if( vpi.has_flag( "WIND_TURBINE" ) ) {
            wind_turbines.push_back( p );
        }
        if( vpi.has_flag( "WIND_POWERED" ) ) {
            sails.push_back( p );
        }
        if( vpi.has_flag( "WATER_WHEEL" ) ) {
            water_wheels.push_back( p );
        }
        if( vpi.has_flag( "FUNNEL" ) ) {
            funnels.push_back( p );
        }
        if( vpi.has_flag( "UNMOUNT_ON_MOVE" ) ) {
            loose_parts.push_back( p );
        }
        if( !vpi.emissions.empty() || !vpi.exhaust.empty() ) {
            emitters.push_back( p );
        }
        if( vpi.has_flag( VPFLAG_WHEEL ) ) {
            wheelcache.push_back( p );
        }
        if( vpi.has_flag( "SMART_ENGINE_CONTROLLER" ) && vp.part().enabled ) {
            has_enabled_smart_controller = true;
        }
        if( vpi.has_flag( VPFLAG_WHEEL ) && vpi.has_flag( VPFLAG_RAIL ) ) {
            rail_wheelcache.push_back( p );
            if( first_wheel_y_mount == INT_MAX ) {
                first_wheel_y_mount = vp.part().mount.y;
            }
            if( first_wheel_y_mount != vp.part().mount.y ) {
                // vehicle have wheels on different axis
                all_wheels_on_one_axis = false;
            }

            railwheel_xmin = std::min( railwheel_xmin, pt.x );
            railwheel_ymin = std::min( railwheel_ymin, pt.y );
            railwheel_xmax = std::max( railwheel_xmax, pt.x );
            railwheel_ymax = std::max( railwheel_ymax, pt.y );
        }
        if( ( vpi.has_flag( "STEERABLE" ) && part_with_feature( pt, "STEERABLE", true ) != -1 ) ||
            vpi.has_flag( "TRACKED" ) ) {
            // TRACKED contributes to steering effectiveness but
            //  (a) doesn't count as a steering axle for install difficulty
            //  (b) still contributes to drag for the center of steering calculation
            steering.push_back( p );
        }
        if( vpi.has_flag( "SECURITY" ) ) {
            speciality.push_back( p );
        }
        if( vp.part().enabled && vpi.has_flag( "EXTRA_DRAG" ) ) {
            extra_drag += vpi.power;
        }
        if( vpi.has_flag( "EXTRA_DRAG" ) && ( vpi.has_flag( "WIND_TURBINE" ) ||
                                              vpi.has_flag( "WATER_WHEEL" ) ) ) {
            extra_drag += vpi.power;
        }
        if( camera_on && vpi.has_flag( "CAMERA" ) ) {
            vp.part().enabled = true;
        } else if( !camera_on && vpi.has_flag( "CAMERA" ) ) {
            vp.part().enabled = false;
        }
        if( vpi.has_flag( "TURRET" ) && !has_part( global_part_pos3( vp.part() ), "TURRET_CONTROLS" ) ) {
            vp.part().enabled = false;
        }
        if( vpi.has_flag( "MUFFLER" ) ) {
            mufflers.push_back( p );
        }
        if( vpi.has_flag( "PLANTER" ) ) {
            planters.push_back( p );
        }
        if( vpi.has_flag( VPFLAG_ENABLED_DRAINS_EPOWER ) ) {
            accessories.push_back( p );
        }
    }

    rail_wheel_bounding_box.p1 = point( railwheel_xmin, railwheel_ymin );
    rail_wheel_bounding_box.p2 = point( railwheel_xmax, railwheel_ymax );
    front_left.x = mount_max.x;
    front_left.y = mount_min.y;
    front_right = mount_max;

    if( !refresh_done ) {
        mount_min = mount_max = point_zero;
        rail_wheel_bounding_box.p1 = point_zero;
        rail_wheel_bounding_box.p2 = point_zero;
    }

    // NB: using the _old_ pivot point, don't recalc here, we only do that when moving!
    precalc_mounts( 0, pivot_rotation[0], pivot_anchor[0] );
    check_environmental_effects = true;
    insides_dirty = true;
    zones_dirty = true;
    invalidate_mass();
    occupied_cache_pos = { -1, -1, -1 };
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

    float xc_numerator = 0.0f;
    float xc_denominator = 0.0f;
    float yc_numerator = 0.0f;
    float yc_denominator = 0.0f;

    for( int p : wheelcache ) {
        const vehicle_part &wheel = parts[p];

        // TODO: load on tire?
        int contact_area = wheel.wheel_area();
        float weight_i;  // weighting for the in-line part
        float weight_p;  // weighting for the perpendicular part
        if( wheel.is_broken() ) {
            // broken wheels don't roll on either axis
            weight_i = contact_area * 2.0;
            weight_p = contact_area * 2.0;
        } else if( part_with_feature( wheel.mount, "STEERABLE", true ) != -1 ) {
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
        pivot_cache.x = std::round( xc_numerator / xc_denominator );
        pivot_cache.y = std::round( yc_numerator / yc_denominator );
    }
}

void vehicle::do_towing_move()
{
    if( !no_towing_slack() || velocity <= 0 ) {
        return;
    }
    bool invalidate = false;
    if( !tow_data.get_towed() ) {
        debugmsg( "tried to do towing move, but no towed vehicle!" );
        invalidate = true;
    }
    const int tow_index = get_tow_part();
    if( tow_index == -1 ) {
        debugmsg( "tried to do towing move, but no tow part" );
        invalidate = true;
    }
    vehicle *towed_veh = tow_data.get_towed();
    if( !towed_veh ) {
        debugmsg( "tried to do towing move, but towed vehicle doesn't exist." );
        invalidate_towing();
        return;
    }
    const int other_tow_index = towed_veh->get_tow_part();
    if( other_tow_index == -1 ) {
        debugmsg( "tried to do towing move but towed vehicle has no towing part" );
        invalidate = true;
    }
    if( towed_veh->global_pos3().z != global_pos3().z ) {
        // how the hellicopter did this happen?
        // yes, this can happen when towing over a bridge (see #47293)
        invalidate = true;
        add_msg( m_info, _( "A towing cable snaps off of %s." ), tow_data.get_towed()->disp_name() );
    }
    if( invalidate ) {
        invalidate_towing( true );
        return;
    }
    map &here = get_map();
    const tripoint tower_tow_point = here.getabs( global_part_pos3( tow_index ) );
    const tripoint towed_tow_point = here.getabs( towed_veh->global_part_pos3( other_tow_index ) );
    // same as above, but where the pulling vehicle is pulling from
    units::angle towing_veh_angle = towed_veh->get_angle_from_targ( tower_tow_point );
    const bool reverse = towed_veh->tow_data.tow_direction == TOW_BACK;
    int accel_y = 0;
    tripoint vehpos = here.getabs( towed_veh->global_pos3() );
    int turn_x = get_turn_from_angle( towing_veh_angle, vehpos, tower_tow_point, reverse );
    if( rl_dist( towed_tow_point, tower_tow_point ) < 6 ) {
        accel_y = reverse ? -1 : 1;
    }
    if( towed_veh->velocity <= velocity && rl_dist( towed_tow_point, tower_tow_point ) >= 7 ) {
        accel_y = reverse ? 1 : -1;
    }
    if( rl_dist( towed_tow_point, tower_tow_point ) >= 12 ) {
        towed_veh->velocity = velocity * 1.8;
        if( reverse ) {
            towed_veh->velocity = -towed_veh->velocity;
        }
    } else {
        towed_veh->velocity = reverse ? -velocity : velocity;
    }
    if( towed_veh->tow_data.tow_direction == TOW_FRONT ) {
        towed_veh->autodrive( point( turn_x, accel_y ) );
    } else if( towed_veh->tow_data.tow_direction == TOW_BACK ) {
        accel_y = 10;
        towed_veh->autodrive( point( turn_x, accel_y ) );
    } else {
        towed_veh->skidding = true;
        std::vector<tripoint> lineto = line_to( here.getlocal( towed_tow_point ),
                                                here.getlocal( tower_tow_point ) );
        tripoint nearby_destination;
        if( lineto.size() >= 2 ) {
            nearby_destination = lineto[1];
        } else {
            nearby_destination = tower_tow_point;
        }
        const int destination_delta_x = here.getlocal( tower_tow_point ).x - nearby_destination.x;
        const int destination_delta_y = here.getlocal( tower_tow_point ).y - nearby_destination.y;
        const int destination_delta_z = towed_veh->global_pos3().z;
        const tripoint move_destination( clamp( destination_delta_x, -1, 1 ),
                                         clamp( destination_delta_y, -1, 1 ),
                                         clamp( destination_delta_z, -1, 1 ) );
        here.move_vehicle( *towed_veh, move_destination, towed_veh->face );
        towed_veh->move = tileray( point( destination_delta_x, destination_delta_y ) );
    }

}

bool vehicle::is_external_part( const tripoint &part_pt ) const
{
    map &here = get_map();
    for( const tripoint &elem : here.points_in_radius( part_pt, 1 ) ) {
        const optional_vpart_position vp = here.veh_at( elem );
        if( !vp ) {
            return true;
        }
        if( &vp->vehicle() != this ) {
            return true;
        }
    }
    return false;
}

bool vehicle::is_towing() const
{
    bool ret = false;
    if( !tow_data.get_towed() ) {
        return ret;
    } else {
        if( !tow_data.get_towed()->tow_data.get_towed_by() ) {
            debugmsg( "vehicle %s is towing, but the towed vehicle has no tower defined", name );
            return ret;
        }
        ret = true;
    }
    return ret;
}

bool vehicle::is_towed() const
{
    bool ret = false;
    if( !tow_data.get_towed_by() ) {
        return ret;
    } else {
        if( !tow_data.get_towed_by()->tow_data.get_towed() ) {
            debugmsg( "vehicle %s is marked as towed, but the tower vehicle has no towed defined", name );
            return ret;
        }
        ret = true;
    }
    return ret;
}

int vehicle::get_tow_part() const
{
    for( const vpart_reference &vp : get_all_parts() ) {
        const size_t p = vp.part_index();
        if( vp.part().removed ) {
            continue;
        }

        if( part_with_feature( p, "TOW_CABLE", true ) >= 0 && vp.part().is_available() ) {
            return p;
        }
    }
    return -1;
}

bool vehicle::has_tow_attached() const
{
    bool ret = false;
    for( const vpart_reference &vp : get_all_parts() ) {
        const size_t p = vp.part_index();
        if( vp.part().removed ) {
            continue;
        }

        if( part_with_feature( p, "TOW_CABLE", true ) >= 0 && vp.part().is_available() ) {
            ret = true;
            break;
        }
    }
    return ret;
}

void vehicle::set_tow_directions()
{
    const int length = mount_max.x - mount_min.x + 1;
    const point mount_of_tow = parts[get_tow_part()].mount;
    const point normalized_tow_mount = point( std::abs( mount_of_tow.x - mount_min.x ),
                                       std::abs( mount_of_tow.y - mount_min.y ) );
    if( length >= 3 ) {
        const int trisect = length / 3;
        if( normalized_tow_mount.x <= trisect ) {
            tow_data.tow_direction = TOW_BACK;
        } else if( normalized_tow_mount.x > trisect && normalized_tow_mount.x <= trisect * 2 ) {
            tow_data.tow_direction = TOW_SIDE;
        } else {
            tow_data.tow_direction = TOW_FRONT;
        }
    } else {
        // its a small vehicle, no danger if it flips around.
        tow_data.tow_direction = TOW_FRONT;
    }
}

bool towing_data::set_towing( vehicle *tower_veh, vehicle *towed_veh )
{
    if( !towed_veh || !tower_veh ) {
        return false;
    }
    towed_veh->tow_data.towed_by = tower_veh;
    tower_veh->tow_data.towing = towed_veh;
    tower_veh->set_tow_directions();
    towed_veh->set_tow_directions();
    return true;
}

void vehicle::invalidate_towing( bool first_vehicle )
{
    if( !is_towing() && !is_towed() ) {
        return;
    }
    vehicle *other_veh = nullptr;
    if( is_towing() ) {
        other_veh = tow_data.get_towed();
    } else if( is_towed() ) {
        other_veh = tow_data.get_towed_by();
    }
    if( other_veh && first_vehicle ) {
        other_veh->invalidate_towing();
    }
    map &here = get_map();
    for( const vpart_reference &vp : get_all_parts() ) {
        const size_t p = vp.part_index();
        if( vp.part().removed ) {
            continue;
        }

        if( part_with_feature( p, "TOW_CABLE", true ) >= 0 ) {
            if( first_vehicle ) {
                vehicle_part *part = &parts[part_with_feature( p, "TOW_CABLE", true )];
                item drop = part->properties_to_item();
                here.add_item_or_charges( global_part_pos3( *part ), drop );
            }
            remove_part( part_with_feature( p, "TOW_CABLE", true ) );
            break;
        }
    }
    tow_data.clear_towing();
}

// to be called on the towed vehicle
bool vehicle::tow_cable_too_far() const
{
    if( !tow_data.get_towed_by() ) {
        debugmsg( "checking tow cable length on a vehicle that has no towing vehicle" );
        return false;
    }
    int index = get_tow_part();
    if( index == -1 ) {
        debugmsg( "towing data exists but no towing part" );
        return false;
    }
    map &here = get_map();
    tripoint towing_point = here.getabs( global_part_pos3( index ) );
    if( !tow_data.get_towed_by()->tow_data.get_towed() ) {
        debugmsg( "vehicle %s has data for a towing vehicle, but that towing vehicle does not have %s listed as towed",
                  disp_name(), disp_name() );
        return false;
    }
    int other_index = tow_data.get_towed_by()->get_tow_part();
    if( other_index == -1 ) {
        debugmsg( "towing data exists but no towing part" );
        return false;
    }
    tripoint towed_point = here.getabs( tow_data.get_towed_by()->global_part_pos3( other_index ) );
    if( towing_point == tripoint_zero || towed_point == tripoint_zero ) {
        debugmsg( "towing data exists but no towing part" );
        return false;
    }
    return rl_dist( towing_point, towed_point ) >= 25;
}

// the towing cable only starts pulling at a certain distance between the vehicles
// to be called on the towing vehicle
bool vehicle::no_towing_slack() const
{
    if( !tow_data.get_towed() ) {
        return false;
    }
    int index = get_tow_part();
    if( index == -1 ) {
        debugmsg( "towing data exists but no towing part" );
        return false;
    }
    map &here = get_map();
    tripoint towing_point = here.getabs( global_part_pos3( index ) );
    if( !tow_data.get_towed()->tow_data.get_towed_by() ) {
        debugmsg( "vehicle %s has data for a towed vehicle, but that towed vehicle does not have %s listed as tower",
                  disp_name(), disp_name() );
        return false;
    }
    int other_index = tow_data.get_towed()->get_tow_part();
    if( other_index == -1 ) {
        debugmsg( "towing data exists but no towing part" );
        return false;
    }
    tripoint towed_point = here.getabs( tow_data.get_towed()->global_part_pos3( other_index ) );
    if( towing_point == tripoint_zero || towed_point == tripoint_zero ) {
        debugmsg( "towing data exists but no towing part" );
        return false;
    }
    return rl_dist( towing_point, towed_point ) >= 8;

}

void vehicle::remove_remote_part( int part_num )
{
    vehicle *veh = find_vehicle( parts[part_num].target.second );

    // If the target vehicle is still there, ask it to remove its part
    if( veh != nullptr ) {
        const tripoint local_abs = get_map().getabs( global_part_pos3( part_num ) );

        for( size_t j = 0; j < veh->loose_parts.size(); j++ ) {
            int remote_partnum = veh->loose_parts[j];
            const vehicle_part *remote_part = &veh->parts[remote_partnum];

            if( veh->part_flag( remote_partnum, "POWER_TRANSFER" ) && remote_part->target.first == local_abs ) {
                veh->remove_part( remote_partnum );
                return;
            }
        }
    }
}

void vehicle::shed_loose_parts()
{
    map &here = get_map();
    // remove_part rebuilds the loose_parts vector, when all of those parts have been removed,
    // it will stay empty.
    while( !loose_parts.empty() ) {
        const int elem = loose_parts.front();
        if( part_flag( elem, "POWER_TRANSFER" ) ) {
            remove_remote_part( elem );
        }
        if( is_towing() || is_towed() ) {
            vehicle *other_veh = is_towing() ? tow_data.get_towed() : tow_data.get_towed_by();
            if( other_veh ) {
                other_veh->remove_part( other_veh->part_with_feature( other_veh->get_tow_part(), "TOW_CABLE",
                                        true ) );
                other_veh->tow_data.clear_towing();
            }
            tow_data.clear_towing();
        }
        const vehicle_part *part = &parts[elem];
        if( !magic ) {
            item drop = part->properties_to_item();
            here.add_item_or_charges( global_part_pos3( *part ), drop );
        }

        remove_part( elem );
    }
}

bool vehicle::enclosed_at( const tripoint &pos )
{
    refresh_insides();
    std::vector<vehicle_part *> parts_here = get_parts_at( pos, "BOARDABLE",
            part_status_flag::working );
    if( !parts_here.empty() ) {
        return parts_here.front()->inside;
    }
    return false;
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

        // inside if not otherwise
        parts[p].inside = true;
        // let's check four neighbor parts
        for( const point &offset : four_adjacent_offsets ) {
            point near_mount = parts[ p ].mount + offset;
            std::vector<int> parts_n3ar = parts_at_relative( near_mount, true );
            // if we aren't covered from sides, the roof at p won't save us
            bool cover = false;
            for( const int &j : parts_n3ar ) {
                // another roof -- cover
                if( part_flag( j, "ROOF" ) && parts[ j ].is_available() ) {
                    cover = true;
                    break;
                } else if( part_flag( j, "OBSTACLE" ) && parts[ j ].is_available() ) {
                    // found an obstacle, like board or windshield or door
                    if( parts[j].inside || ( part_flag( j, "OPENABLE" ) && parts[j].open ) ) {
                        // door and it's open -- can't cover
                        continue;
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
    return vehicle().part( part_index() ).inside;
}

void vehicle::unboard_all()
{
    map &here = get_map();
    std::vector<int> bp = boarded_parts();
    for( const int &i : bp ) {
        here.unboard_vehicle( global_part_pos3( i ) );
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
        for( const int &i : pl ) {
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

    int target_part = part_info( p ).rotor_diameter() ? p : random_entry( pl );

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
        int protection = part_info( armor_part ).damage_reduction[ static_cast<int>( type )];
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
        int distance = 1 + square_dist( vp.mount(), impact );
        if( distance > 1 ) {
            int net_dmg = rng( dmg1, dmg2 ) / ( distance * distance );
            if( part_info( p ).location != part_location_structure ||
                !part_info( p ).has_flag( "PROTRUSION" ) ) {
                int shock_absorber = part_with_feature( p, "SHOCK_ABSORBER", true );
                if( shock_absorber >= 0 ) {
                    net_dmg = std::max( 0, net_dmg - parts[ shock_absorber ].info().bonus );
                }
            }
            damage_direct( p, net_dmg, type );
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
    for( vehicle_part &elem : parts ) {
        elem.mount -= delta;
    }

    decltype( labels ) new_labels;
    for( const label &l : labels ) {
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
    get_map().rebuild_vehicle_level_caches();
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
    map &here = get_map();
    const tripoint pos = global_part_pos3( p );
    const auto scatter_parts = [&]( const vehicle_part & pt ) {
        for( const item &piece : pt.pieces_for_broken_part() ) {
            // inside the loop, so each piece goes to a different place
            // TODO: this may spawn items behind a wall
            const tripoint where = random_entry( here.points_in_radius( pos, SCATTER_DISTANCE ) );
            // TODO: balance audit, ensure that less pieces are generated than one would need
            // to build the component (smash a vehicle box that took 10 lumps of steel,
            // find 12 steel lumps scattered after atom-smashing it with a tree trunk)
            if( !magic ) {
                here.add_item_or_charges( where, piece );
            }
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
                add_msg_if_player_sees( pos, m_bad, _( "The %s's %s breaks into pieces!" ), name,
                                        parts[ parts_in_square[ index ] ].name() );
                scatter_parts( parts[parts_in_square[index]] );
            } else {
                // Intact (but possibly damaged) part - remove it in one piece
                add_msg_if_player_sees( pos, m_bad, _( "The %1$s's %2$s is torn off!" ), name,
                                        parts[ parts_in_square[ index ] ].name() );
                if( !magic ) {
                    item part_as_item = parts[parts_in_square[index]].properties_to_item();
                    here.add_item_or_charges( pos, part_as_item );
                }
            }
            remove_part( parts_in_square[index] );
        }
        // After clearing the frame, remove it.
        add_msg_if_player_sees( pos, m_bad, _( "The %1$s's %2$s is destroyed!" ), name, parts[ p ].name() );
        scatter_parts( parts[p] );
        remove_part( p );
        find_and_split_vehicles( p );
    } else {
        //Just break it off
        add_msg_if_player_sees( pos, m_bad, _( "The %1$s's %2$s is destroyed!" ), name, parts[ p ].name() );

        scatter_parts( parts[p] );
        const point position = parts[p].mount;
        remove_part( p );

        // remove parts for which required flags are not present anymore
        if( !part_info( p ).get_flags().empty() ) {
            const std::vector<int> parts_here = parts_at_relative( position, false );
            for( const auto &part : parts_here ) {
                bool remove = false;
                for( const std::string &flag : part_info( part ).get_flags() ) {
                    if( !json_flag::get( flag ).requires_flag().empty() ) {
                        remove = true;
                        for( const auto &elem : parts_here ) {
                            if( part_info( elem ).has_flag( json_flag::get( flag ).requires_flag() ) ) {
                                remove = false;
                                continue;
                            }
                        }
                    }
                }
                if( remove ) {
                    item part_as_item = parts[part].properties_to_item();
                    here.add_item_or_charges( pos, part_as_item );
                    remove_part( part );
                }
            }
        }
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
    const fuel_explosion_data &data = fuel.get_explosion_data();

    if( parts[ p ].is_broken() ) {
        leak_fuel( parts[ p ] );
    }

    int explosion_chance = type == damage_type::HEAT ? data.explosion_chance_hot :
                           data.explosion_chance_cold;
    if( one_in( explosion_chance ) ) {
        get_event_bus().send<event_type::fuel_tank_explodes>( name );
        const int pow = 120 * ( 1 - std::exp( data.explosion_factor / -5000 *
                                              ( parts[p].ammo_remaining() * data.fuel_size_factor ) ) );
        //debugmsg( "damage check dmg=%d pow=%d amount=%d", dmg, pow, parts[p].amount );

        explosion_handler::explosion( global_part_pos3( p ), pow, 0.7, data.fiery_explosion );
        mod_hp( parts[p], 0 - parts[ p ].hp(), damage_type::HEAT );
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
    // If auto-driving and damage happens, bail out
    if( is_autodriving ) {
        stop_autodriving();
    }
    map &here = get_map();
    here.set_memory_seen_cache_dirty( global_part_pos3( p ) );
    if( parts[p].is_broken() ) {
        return break_off( p, dmg );
    }

    int tsh = std::min( 20, part_info( p ).durability / 10 );
    if( dmg < tsh && type != damage_type::PURE ) {
        if( type == damage_type::HEAT && parts[p].is_fuel_store() ) {
            explode_fuel( p, type );
        }

        return dmg;
    }

    dmg -= std::min<int>( dmg, part_info( p ).damage_reduction[ static_cast<int>( type ) ] );
    int dres = dmg - parts[p].hp();
    if( mod_hp( parts[ p ], 0 - dmg, type ) ) {
        if( is_flyable() && !rotors.empty() && !parts[p].has_flag( VPFLAG_SIMPLE_PART ) ) {
            // If we break a part, we can no longer fly the vehicle.
            set_flyable( false );
        }

        insides_dirty = true;
        pivot_dirty = true;

        // destroyed parts lose any contained fuels, battery charges or ammo
        leak_fuel( parts [ p ] );

        for( const item &e : parts[p].items ) {
            here.add_item_or_charges( global_part_pos3( p ), e );
        }
        parts[p].items.clear();

        invalidate_mass();
        coeff_air_changed = true;

        // refresh cache in case the broken part has changed the status
        refresh();
    }

    if( parts[p].is_fuel_store() ) {
        explode_fuel( p, type );
    } else if( parts[ p ].is_broken() && part_flag( p, "UNMOUNT_ON_DAMAGE" ) ) {
        here.spawn_item( global_part_pos3( p ), part_info( p ).base_item, 1, 0, calendar::turn );
        monster *mon = get_monster( p );
        if( mon != nullptr && mon->has_effect( effect_harnessed ) ) {
            mon->remove_effect( effect_harnessed );
        }
        if( part_flag( p, "TOW_CABLE" ) ) {
            invalidate_towing( true );
        } else {
            remove_part( p );
        }
    }

    return std::max( dres, 0 );
}

void vehicle::leak_fuel( vehicle_part &pt )
{
    // only liquid fuels from non-empty tanks can leak out onto map tiles
    if( !pt.is_tank() || pt.ammo_remaining() <= 0 ) {
        return;
    }

    map &here = get_map();
    // leak in random directions but prefer closest tiles and avoid walls or other obstacles
    std::vector<tripoint> tiles = closest_points_first( global_part_pos3( pt ), 1 );
    tiles.erase( std::remove_if( tiles.begin(), tiles.end(), [&here]( const tripoint & e ) {
        return !here.passable( e );
    } ), tiles.end() );

    // leak up to 1/3 of remaining fuel per iteration and continue until the part is empty
    const itype *fuel = item::find_type( pt.ammo_current() );
    while( !tiles.empty() && pt.ammo_remaining() ) {
        int qty = pt.ammo_consume( rng( 0, std::max( pt.ammo_remaining() / 3, 1 ) ),
                                   global_part_pos3( pt ) );
        if( qty > 0 ) {
            here.add_item_or_charges( random_entry( tiles ), item( fuel, calendar::turn, qty ) );
        }
    }

    pt.ammo_unset();
}

std::map<itype_id, int> vehicle::fuels_left() const
{
    std::map<itype_id, int> result;
    for( const vehicle_part &p : parts ) {
        if( p.is_fuel_store() && !p.ammo_current().is_null() ) {
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
    face.init( 0_degrees );
    turn_dir = 0_degrees;
    turn( 0_degrees );
    precalc_mounts( 0, pivot_rotation[0], pivot_anchor[0] );
    precalc_mounts( 1, pivot_rotation[1], pivot_anchor[1] );
    last_update = calendar::turn;
    return true;
}

const std::set<tripoint> &vehicle::get_points( const bool force_refresh ) const
{
    if( force_refresh || occupied_cache_pos != global_pos3() ||
        occupied_cache_direction != face.dir() ) {
        occupied_cache_pos = global_pos3();
        occupied_cache_direction = face.dir();
        occupied_points.clear();
        for( const std::pair<const point, std::vector<int>> &part_location : relative_parts ) {
            occupied_points.insert( global_part_pos3( part_location.second.front() ) );
        }
    }

    return occupied_points;
}

std::list<item> vehicle::use_charges( const vpart_position &vp, const itype_id &type,
                                      int &quantity,
                                      const std::function<bool( const item & )> &filter )
{
    std::list<item> ret;
    // HACK: water_faucet pseudo tool gives access to liquids in tanks
    const itype_id veh_tool_type = type.obj().phase > phase_id::SOLID
                                   ? itype_id( "water_faucet" )
                                   : type;
    const cata::optional<vpart_reference> tool_vp = vp.part_with_tool( veh_tool_type );
    const cata::optional<vpart_reference> cargo_vp = vp.part_with_feature( "CARGO", true );

    const auto tool_wants_battery = []( const itype_id & type ) {
        item tool( type, calendar::turn_zero ), mag( tool.magazine_default() );
        mag.contents.clear_items();

        return tool.contents.insert_item( mag, item_pocket::pocket_type::MAGAZINE_WELL ).success() &&
               tool.ammo_capacity( ammotype( "battery" ) ) > 0;
    };

    if( tool_vp ) { // handle vehicle tools
        itype_id fuel_type = tool_wants_battery( type ) ? itype_battery : type;
        item tmp( type, calendar::turn_zero ); // TODO: add a sane birthday arg
        // TODO: Handle water poison when crafting starts respecting it
        tmp.charges = tool_vp->vehicle().drain( fuel_type, quantity );
        quantity -= tmp.charges;
        ret.push_back( tmp );

        if( quantity == 0 ) {
            return ret;
        }
    }

    if( cargo_vp ) {
        vehicle_stack veh_stack = get_items( cargo_vp->part_index() );
        std::list<item> tmp = veh_stack.use_charges( type, quantity, vp.pos(), filter );
        ret.splice( ret.end(), tmp );
        if( quantity <= 0 ) {
            return ret;
        }
    }

    return ret;
}

vehicle_part &vpart_reference::part() const
{
    cata_assert( part_index() < static_cast<size_t>( vehicle().part_count() ) );
    return vehicle().part( part_index() );
}

const vpart_info &vpart_reference::info() const
{
    return part().info();
}

player *vpart_reference::get_passenger() const
{
    return vehicle().get_passenger( part_index() );
}

point vpart_position::mount() const
{
    return vehicle().part( part_index() ).mount;
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

static bool is_sm_tile_over_water( const tripoint &real_global_pos )
{

    const tripoint smp = ms_to_sm_copy( real_global_pos );
    const point p( modulo( real_global_pos.x, SEEX ), modulo( real_global_pos.y, SEEY ) );
    const submap *sm = MAPBUFFER.lookup_submap( smp );
    if( sm == nullptr ) {
        debugmsg( "is_sm_tile_outside(): couldn't find submap %d,%d,%d", smp.x, smp.y, smp.z );
        return false;
    }

    if( p.x < 0 || p.x >= SEEX || p.y < 0 || p.y >= SEEY ) {
        debugmsg( "err %d,%d", p.x, p.y );
        return false;
    }

    return ( sm->get_ter( p ).obj().has_flag( TFLAG_CURRENT ) ||
             sm->get_furn( p ).obj().has_flag( TFLAG_CURRENT ) );
}

static bool is_sm_tile_outside( const tripoint &real_global_pos )
{

    const tripoint smp = ms_to_sm_copy( real_global_pos );
    const point p( modulo( real_global_pos.x, SEEX ), modulo( real_global_pos.y, SEEY ) );
    const submap *sm = MAPBUFFER.lookup_submap( smp );
    if( sm == nullptr ) {
        debugmsg( "is_sm_tile_outside(): couldn't find submap %d,%d,%d", smp.x, smp.y, smp.z );
        return false;
    }

    if( p.x < 0 || p.x >= SEEX || p.y < 0 || p.y >= SEEY ) {
        debugmsg( "err %d,%d", p.x, p.y );
        return false;
    }

    return !( sm->get_ter( p ).obj().has_flag( TFLAG_INDOORS ) ||
              sm->get_furn( p ).obj().has_flag( TFLAG_INDOORS ) );
}

void vehicle::update_time( const time_point &update_to )
{
    double muffle;
    int exhaust_part;
    std::tie( exhaust_part, muffle ) = get_exhaust_part();

    map &here = get_map();
    // Parts emitting fields
    for( int idx : emitters ) {
        const vehicle_part &pt = parts[idx];
        if( pt.is_unavailable() || !pt.enabled ) {
            continue;
        }
        for( const emit_id &e : pt.info().emissions ) {
            here.emit_field( global_part_pos3( pt ), e );
        }
        for( const emit_id &e : pt.info().exhaust ) {
            if( exhaust_part == -1 ) {
                here.emit_field( global_part_pos3( pt ), e );
            } else {
                here.emit_field( exhaust_dest( exhaust_part ), e );
            }
        }
        discharge_battery( pt.info().epower );
    }

    if( sm_pos.z < 0 ) {
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
    if( funnels.empty() && solar_panels.empty() && wind_turbines.empty() && water_wheels.empty() ) {
        return;
    }
    // Get one weather data set per vehicle, they don't differ much across vehicle area
    const weather_sum accum_weather = sum_conditions( update_from, update_to,
                                      here.getabs( global_pos3() ) );
    // make some reference objects to use to check for reload
    const item water( "water" );
    const item water_clean( "water_clean" );

    for( int idx : funnels ) {
        const vehicle_part &pt = parts[idx];

        // we need an unbroken funnel mounted on the exterior of the vehicle
        if( pt.is_unavailable() || !is_sm_tile_outside( here.getabs( global_part_pos3( pt ) ) ) ) {
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

        double area = std::pow( pt.info().size / units::legacy_volume_factor, 2 ) * M_PI;
        int qty = roll_remainder( funnel_charges_per_turn( area, accum_weather.rain_amount ) );
        int c_qty = qty + ( tank->can_reload( water_clean ) ?  tank->ammo_remaining() : 0 );
        int cost_to_purify = c_qty * item::find_type( itype_water_purifier )->charges_to_use();

        if( qty > 0 ) {
            const cata::optional<vpart_reference> vp_purifier = vpart_position( *this, idx )
                    .part_with_tool( itype_water_purifier );

            if( vp_purifier && ( fuel_left( itype_battery, true ) > cost_to_purify ) ) {
                tank->ammo_set( itype_water_clean, c_qty );
                discharge_battery( cost_to_purify );
            } else {
                tank->ammo_set( itype_water, tank->ammo_remaining() + qty );
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

            if( !is_sm_tile_outside( here.getabs( global_part_pos3( part ) ) ) ) {
                continue;
            }

            epower_w += part_epower_w( part );
        }
        double intensity = accum_weather.sunlight / default_daylight_level() / to_turns<double>( elapsed );
        int energy_bat = power_to_energy_bat( epower_w * intensity, elapsed );
        if( energy_bat > 0 ) {
            add_msg_debug( "%s got %d kJ energy from solar panels", name, energy_bat );
            charge_battery( energy_bat );
        }
    }
    if( !wind_turbines.empty() ) {
        // TODO: use accum_weather wind data to backfill wind turbine
        // generation capacity.
        int epower_w = total_wind_epower_w();
        int energy_bat = power_to_energy_bat( epower_w, elapsed );
        if( energy_bat > 0 ) {
            add_msg_debug( "%s got %d kJ energy from wind turbines", name, energy_bat );
            charge_battery( energy_bat );
        }
    }
    if( !water_wheels.empty() ) {
        int epower_w = total_water_wheel_epower_w();
        int energy_bat = power_to_energy_bat( epower_w, elapsed );
        if( energy_bat > 0 ) {
            add_msg_debug( "%s got %d kJ energy from water wheels", name, energy_bat );
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
    units::quantity<float, units::mass::unit_type> xf;
    units::quantity<float, units::mass::unit_type> yf;
    units::mass m_total = 0_gram;
    for( const vpart_reference &vp : get_all_parts() ) {
        const size_t i = vp.part_index();
        if( vp.part().removed ) {
            continue;
        }

        units::mass m_part = 0_gram;
        units::mass m_part_items = 0_gram;
        m_part += vp.part().base.weight();
        for( const item &j : get_items( i ) ) {
            m_part_items += j.weight();
        }
        if( vp.part().info().cargo_weight_modifier != 100 ) {
            m_part_items *= vp.part().info().cargo_weight_modifier / 100;
        }
        m_part += m_part_items;

        if( vp.has_feature( VPFLAG_BOARDABLE ) && vp.part().has_flag( vehicle_part::passenger_flag ) ) {
            const player *p = get_passenger( i );
            // Sometimes flag is wrongly set, don't crash!
            m_part += p != nullptr ? p->get_weight() : 0_gram;
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
        mass_center_precalc.x = std::round( x );
        mass_center_precalc.y = std::round( y );
        mass_center_precalc_dirty = false;
    } else {
        mass_center_no_precalc.x = std::round( x );
        mass_center_no_precalc.y = std::round( y );
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
        const point pt = parts[part_at( p.xy() )].precalc[i_use].xy();
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

int vehicle::part_count() const
{
    return static_cast<int>( parts.size() );
}

vehicle_part &vehicle::part( int part_num )
{
    return parts[part_num];
}

const vehicle_part &vehicle::cpart( int part_num ) const
{
    return const_cast<vehicle_part &>( parts[part_num] );
}

bool vehicle::valid_part( int part_num ) const
{
    return part_num >= 0 && part_num < static_cast<int>( parts.size() );
}

void vehicle::force_erase_part( int part_num )
{
    parts.erase( parts.begin() + part_num );
}

std::set<int> vehicle::advance_precalc_mounts( const point &new_pos, const tripoint &src,
        const tripoint &dp, int ramp_offset, const bool adjust_pos,
        std::set<int> parts_to_move )
{
    map &here = get_map();
    std::set<int> smzs;
    // when a vehicle part enters the low end of a down ramp, or the high end of an up ramp,
    // it immediately translates down or up a z-level, respectively, ending up on the low
    // end of an up ramp or high end of a down ramp, respectively.  The two ends are set
    // past each other, like so:
    // (side view)  z+1   Rdh RDl
    //              z+0   RUh Rul
    // A vehicle moving left to right on z+1 drives down to z+0 by entering the ramp down low end.
    // A vehicle moving right to left on z+0 drives up to z+1 by entering the ramp up high end.
    // A vehicle moving left to right on z+0 should ideally collide into a wall before entering
    //   the ramp up high end, but even if it does, it briefly transitions to z+1 before returning
    //   to z0 by entering the ramp down low end.
    // A vehicle moving right to left on z+1 drives down to z+0 by entering the ramp down low end,
    //   then immediately returns to z+1 by entering the ramp up high end.
    // When a vehicle's pivot point transitions a z-level via a ramp, all other pre-calc points
    // make the opposite transition, so that points that were above an ascending pivot point are
    // now level with it, and parts that were level with an ascending pivot point are now below
    // it.
    // parts that enter the translation portion of a ramp on the same displacement as the
    // pivot point stay at the same relative z to the pivot point, as the ramp_offset adjustments
    // cancel out.
    // if a vehicle manages move partially up or down a ramp and then veers off course, it
    // can get split across the z-levels and continue moving, enough though large parts of the
    // vehicle are unsupported.  In that case, move the unsupported parts down until they are
    // supported.
    int index = -1;
    for( vehicle_part &prt : parts ) {
        index += 1;
        here.clear_vehicle_point_from_cache( this, src + prt.precalc[0] );
        // no parts means this is a normal horizontal or vertical move
        if( parts_to_move.empty() ) {
            prt.precalc[0] = prt.precalc[1];
            // partial part movement means we're zero-ing out after missing a ramp
        } else if( adjust_pos && parts_to_move.find( index ) == parts_to_move.end() ) {
            prt.precalc[0].z -= dp.z;
        } else if( !adjust_pos &&  parts_to_move.find( index ) != parts_to_move.end() ) {
            prt.precalc[0].z += dp.z;
        }
        if( here.has_flag( TFLAG_RAMP_UP, src + dp + prt.precalc[0] ) ) {
            prt.precalc[0].z += 1;
        } else if( here.has_flag( TFLAG_RAMP_DOWN, src + dp + prt.precalc[0] ) ) {
            prt.precalc[0].z -= 1;
        }
        prt.precalc[0].z -= ramp_offset;
        prt.precalc[1].z = prt.precalc[0].z;
        smzs.insert( prt.precalc[0].z );
    }
    if( adjust_pos ) {
        if( parts_to_move.empty() ) {
            pivot_anchor[0] = pivot_anchor[1];
            pivot_rotation[0] = pivot_rotation[1];
        }
        pos = new_pos;
    }
    occupied_cache_pos = { -1, -1, -1 };
    return smzs;
}

bool vehicle::refresh_zones()
{
    if( zones_dirty ) {
        map &here = get_map();
        decltype( loot_zones ) new_zones;
        for( auto const &z : loot_zones ) {
            zone_data zone = z.second;
            //Get the global position of the first cargo part at the relative coordinate

            const int part_idx = part_with_feature( z.first, "CARGO", false );
            if( part_idx == -1 ) {
                debugmsg( "Could not find cargo part at %d,%d on vehicle %s for loot zone.  Removing loot zone.",
                          z.first.x, z.first.y, this->name );

                // If this loot zone refers to a part that no longer exists at this location, then its unattached somehow.
                // By continuing here and not adding to new_zones, we effectively remove it
                continue;
            }
            tripoint zone_pos = global_part_pos3( part_idx );
            zone_pos = here.getabs( zone_pos );
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

std::pair<int, double> vehicle::get_exhaust_part() const
{
    double muffle = 1.0;
    double m = 0.0;
    int exhaust_part = -1;
    for( int part : mufflers ) {
        const vehicle_part &vp = parts[ part ];
        m = 1.0 - ( 1.0 - vp.info().bonus / 100.0 ) * vp.health_percent();
        if( m < muffle ) {
            muffle = m;
            exhaust_part = part;
        }
    }
    return std::make_pair( exhaust_part, muffle );
}

tripoint vehicle::exhaust_dest( int part ) const
{
    point p = parts[part].mount;
    // Move back from engine/muffler until we find an open space
    while( relative_parts.find( p ) != relative_parts.end() ) {
        p.x += ( velocity < 0 ? 1 : -1 );
    }
    point q = coord_translate( p );
    return global_pos3() + tripoint( q, 0 );
}

template<>
bool vehicle_part_with_feature_range<std::string>::matches( const size_t part ) const
{
    const vehicle_part &vp = this->vehicle().part( part );
    return vp.info().has_flag( feature_ ) &&
           !vp.removed &&
           ( !( part_status_flag::working & required_ ) || !vp.is_broken() ) &&
           ( !( part_status_flag::available & required_ ) || vp.is_available() ) &&
           ( !( part_status_flag::enabled & required_ ) || vp.enabled );
}

template<>
bool vehicle_part_with_feature_range<vpart_bitflags>::matches( const size_t part ) const
{
    const vehicle_part &vp = this->vehicle().part( part );
    return vp.info().has_flag( feature_ ) &&
           !vp.removed &&
           ( !( part_status_flag::working & required_ ) || !vp.is_broken() ) &&
           ( !( part_status_flag::available & required_ ) || vp.is_available() ) &&
           ( !( part_status_flag::enabled & required_ ) || vp.enabled );
}
