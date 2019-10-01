#include "vehicle.h" // IWYU pragma: associated

#include <algorithm>
#include <cassert>
#include <cmath>
#include <set>
#include <memory>
#include <list>

#include "avatar.h"
#include "debug.h"
#include "game.h"
#include "item.h"
#include "itype.h"
#include "map.h"
#include "messages.h"
#include "npc.h"
#include "string_formatter.h"
#include "translations.h"
#include "veh_type.h"
#include "vpart_position.h"
#include "weather.h"
#include "optional.h"
#include "color.h"
#include "enums.h"
#include "flat_set.h"

static const itype_id fuel_type_none( "null" );
static const itype_id fuel_type_battery( "battery" );
/*-----------------------------------------------------------------------------
 *                              VEHICLE_PART
 *-----------------------------------------------------------------------------*/
vehicle_part::vehicle_part()
    : id( vpart_id::NULL_ID() ) {}

vehicle_part::vehicle_part( const vpart_id &vp, const point &dp, item &&obj )
    : mount( dp ), id( vp ), base( std::move( obj ) )
{
    // Mark base item as being installed as a vehicle part
    base.item_tags.insert( "VEHICLE" );

    if( base.typeId() != vp->item ) {
        debugmsg( "incorrect vehicle part item, expected: %s, received: %s",
                  vp->item.c_str(), base.typeId().c_str() );
    }
}

vehicle_part::operator bool() const
{
    return id != vpart_id::NULL_ID();
}

const item &vehicle_part::get_base() const
{
    return base;
}

void vehicle_part::set_base( const item &new_base )
{
    base = new_base;
}

item vehicle_part::properties_to_item() const
{
    item tmp = base;
    tmp.item_tags.erase( "VEHICLE" );

    // Cables get special handling: their target coordinates need to remain
    // stored, and if a cable actually drops, it should be half-connected.
    if( tmp.has_flag( "CABLE_SPOOL" ) ) {
        const tripoint local_pos = g->m.getlocal( target.first );
        if( !g->m.veh_at( local_pos ) ) {
            tmp.item_tags.insert( "NO_DROP" ); // That vehicle ain't there no more.
        }

        tmp.set_var( "source_x", target.first.x );
        tmp.set_var( "source_y", target.first.y );
        tmp.set_var( "source_z", target.first.z );
        tmp.set_var( "state", "pay_out_cable" );
        tmp.active = true;
    }

    return tmp;
}

std::string vehicle_part::name( bool with_prefix ) const
{
    auto res = info().name();

    if( base.engine_displacement() > 0 ) {
        res.insert( 0, string_format( _( "%2.1fL " ), base.engine_displacement() / 100.0 ) );

    } else if( wheel_diameter() > 0 ) {
        res.insert( 0, string_format( _( "%d\" " ), wheel_diameter() ) );
    }

    if( base.is_faulty() ) {
        res += _( " (faulty)" );
    }

    if( base.has_var( "contained_name" ) ) {
        res += string_format( _( " holding %s" ), base.get_var( "contained_name" ) );
    }

    if( with_prefix ) {
        res.insert( 0, colorize( base.damage_symbol(), base.damage_color() ) + " " );
    }
    return res;
}

int vehicle_part::hp() const
{
    const int dur = info().durability;
    if( base.max_damage() > 0 ) {
        return dur - dur * base.damage() / base.max_damage();
    } else {
        return dur;
    }
}

int vehicle_part::damage() const
{
    return base.damage();
}

int vehicle_part::max_damage() const
{
    return base.max_damage();
}

int vehicle_part::damage_level( int max ) const
{
    return base.damage_level( max );
}

double vehicle_part::health_percent() const
{
    return 1.0 - static_cast<double>( base.damage() ) / base.max_damage();
}

double vehicle_part::damage_percent() const
{
    return static_cast<double>( base.damage() ) / base.max_damage();
}

/** parts are considered broken at zero health */
bool vehicle_part::is_broken() const
{
    return base.damage() >= base.max_damage();
}

bool vehicle_part::is_unavailable( const bool carried ) const
{
    return is_broken() || ( has_flag( carried_flag ) && carried );
}

bool vehicle_part::is_available( const bool carried ) const
{
    return !is_unavailable( carried );
}

itype_id vehicle_part::fuel_current() const
{
    if( is_engine() ) {
        if( ammo_pref == "null" ) {
            return info().fuel_type != "muscle" ? info().fuel_type : "null";
        } else {
            return ammo_pref;
        }
    }

    return "null";
}

bool vehicle_part::fuel_set( const itype_id &fuel )
{
    if( is_engine() ) {
        for( const itype_id &avail : info().engine_fuel_opts() ) {
            if( fuel == avail ) {
                ammo_pref = fuel;
                return true;
            }
        }
    }
    return false;
}

itype_id vehicle_part::ammo_current() const
{
    if( is_battery() ) {
        return "battery";
    }

    if( is_tank() && !base.contents.empty() ) {
        return base.contents.front().typeId();
    }

    if( is_fuel_store( false ) || is_turret() ) {
        return base.ammo_current();
    }

    return "null";
}

int vehicle_part::ammo_capacity() const
{
    if( is_tank() ) {
        return item::find_type( ammo_current() )->charges_per_volume( base.get_container_capacity() );
    }

    if( is_fuel_store( false ) || is_turret() ) {
        return base.ammo_capacity();
    }

    return 0;
}

int vehicle_part::ammo_remaining() const
{
    if( is_tank() ) {
        return base.contents.empty() ? 0 : base.contents.back().charges;
    }

    if( is_fuel_store( false ) || is_turret() ) {
        return base.ammo_remaining();
    }

    return 0;
}

int vehicle_part::ammo_set( const itype_id &ammo, int qty )
{
    const itype *liquid = item::find_type( ammo );

    // We often check if ammo is set to see if tank is empty, if qty == 0 don't set ammo
    if( is_tank() && liquid->phase >= LIQUID && qty != 0 ) {
        base.contents.clear();
        const auto stack = units::legacy_volume_factor / std::max( liquid->stack_size, 1 );
        const int limit = units::from_milliliter( ammo_capacity() ) / stack;
        base.emplace_back( ammo, calendar::turn, qty > 0 ? std::min( qty, limit ) : limit );
        return qty;
    }

    if( is_turret() ) {
        return base.ammo_set( ammo, qty ).ammo_remaining();
    }

    if( is_fuel_store() ) {
        base.ammo_set( ammo, qty >= 0 ? qty : ammo_capacity() );
        return base.ammo_remaining();
    }

    return -1;
}

void vehicle_part::ammo_unset()
{
    if( is_tank() ) {
        base.contents.clear();
    } else if( is_fuel_store() ) {
        base.ammo_unset();
    }
}

int vehicle_part::ammo_consume( int qty, const tripoint &pos )
{
    if( is_tank() && !base.contents.empty() ) {
        const int res = std::min( ammo_remaining(), qty );
        item &liquid = base.contents.back();
        liquid.charges -= res;
        if( liquid.charges == 0 ) {
            base.contents.clear();
        }
        return res;
    }
    return base.ammo_consume( qty, pos );
}

double vehicle_part::consume_energy( const itype_id &ftype, double energy_j )
{
    if( base.contents.empty() || !is_fuel_store() ) {
        return 0.0f;
    }

    item &fuel = base.contents.back();
    if( fuel.typeId() == ftype ) {
        assert( fuel.is_fuel() );
        // convert energy density in MJ/L to J/ml
        const double energy_p_mL = fuel.fuel_energy() * 1000;
        const int ml_to_use = static_cast<int>( std::floor( energy_j / energy_p_mL ) );
        int charges_to_use = fuel.charges_per_volume( ml_to_use * 1_ml );

        if( !charges_to_use ) {
            return 0.0;
        }
        if( charges_to_use > fuel.charges ) {
            charges_to_use = fuel.charges;
            base.contents.clear();
        } else {
            fuel.charges -= charges_to_use;
        }
        item fuel_consumed( ftype, calendar::turn, charges_to_use );
        return energy_p_mL * units::to_milliliter<int>( fuel_consumed.volume( true ) );
    }
    return 0.0;
}

bool vehicle_part::can_reload( const item &obj ) const
{
    // first check part is not destroyed and can contain ammo
    if( !is_fuel_store() ) {
        return false;
    }

    if( !obj.is_null() ) {
        const itype_id obj_type = obj.typeId();
        if( is_reactor() ) {
            return base.is_reloadable_with( obj_type );
        }

        // forbid filling tanks with solids or non-material things
        if( is_tank() && ( obj.made_of( SOLID ) || obj.made_of( PNULL ) ) ) {
            return false;
        }
        // forbid putting liquids, gasses, and plasma in things that aren't tanks
        else if( !obj.made_of( SOLID ) && !is_tank() ) {
            return false;
        }
        // prevent mixing of different ammo
        if( ammo_current() != "null" && ammo_current() != obj_type ) {
            return false;
        }
        // For storage with set type, prevent filling with different types
        if( info().fuel_type != fuel_type_none && info().fuel_type != obj_type ) {
            return false;
        }
        // don't fill magazines with inappropriate fuel
        if( !is_tank() && !base.is_reloadable_with( obj_type ) ) {
            return false;
        }
    }

    return ammo_remaining() < ammo_capacity();
}

void vehicle_part::process_contents( const tripoint &pos, const bool e_heater )
{
    // for now we only care about processing food containers since things like
    // fuel don't care about temperature yet
    if( base.is_food_container() ) {
        temperature_flag flag = temperature_flag::TEMP_NORMAL;
        if( e_heater ) {
            flag = temperature_flag::TEMP_HEATER;
        }
        base.process( nullptr, pos, false, 1, flag );
    }
}

bool vehicle_part::fill_with( item &liquid, int qty )
{
    if( !is_tank() || !can_reload( liquid ) ) {
        return false;
    }

    base.fill_with( liquid, qty );
    return true;
}

const std::set<fault_id> &vehicle_part::faults() const
{
    return base.faults;
}

std::set<fault_id> vehicle_part::faults_potential() const
{
    return base.faults_potential();
}

bool vehicle_part::fault_set( const fault_id &f )
{
    if( !faults_potential().count( f ) ) {
        return false;
    }
    base.faults.insert( f );
    return true;
}

int vehicle_part::wheel_area() const
{
    return info().wheel_area();
}

/** Get wheel diameter (inches) or return 0 if part is not wheel */
int vehicle_part::wheel_diameter() const
{
    return base.is_wheel() ? base.type->wheel->diameter : 0;
}

/** Get wheel width (inches) or return 0 if part is not wheel */
int vehicle_part::wheel_width() const
{
    return base.is_wheel() ? base.type->wheel->width : 0;
}

npc *vehicle_part::crew() const
{
    if( is_broken() || !crew_id.is_valid() ) {
        return nullptr;
    }

    npc *const res = g->critter_by_id<npc>( crew_id );
    if( !res ) {
        return nullptr;
    }
    return res->is_player_ally() ? res : nullptr;
}

bool vehicle_part::set_crew( const npc &who )
{
    if( who.is_dead_state() || !( who.is_walking_with() || who.is_player_ally() ) ) {
        return false;
    }
    if( is_broken() || ( !is_seat() && !is_turret() ) ) {
        return false;
    }
    crew_id = who.getID();
    return true;
}

void vehicle_part::unset_crew()
{
    crew_id = character_id();
}

void vehicle_part::reset_target( const tripoint &pos )
{
    target.first = pos;
    target.second = pos;
}

bool vehicle_part::is_engine() const
{
    return info().has_flag( VPFLAG_ENGINE );
}

bool vehicle_part::is_light() const
{
    const auto &vp = info();
    return vp.has_flag( VPFLAG_CONE_LIGHT ) ||
           vp.has_flag( VPFLAG_WIDE_CONE_LIGHT ) ||
           vp.has_flag( VPFLAG_HALF_CIRCLE_LIGHT ) ||
           vp.has_flag( VPFLAG_CIRCLE_LIGHT ) ||
           vp.has_flag( VPFLAG_AISLE_LIGHT ) ||
           vp.has_flag( VPFLAG_DOME_LIGHT ) ||
           vp.has_flag( VPFLAG_ATOMIC_LIGHT );
}

bool vehicle_part::is_fuel_store( bool skip_broke ) const
{
    if( skip_broke && is_broken() ) {
        return false;
    }
    return is_tank() || base.is_magazine() || is_reactor();
}

bool vehicle_part::is_tank() const
{
    return base.is_watertight_container();
}

bool vehicle_part::is_battery() const
{
    return base.is_magazine() && base.ammo_types().count( ammotype( "battery" ) );
}

bool vehicle_part::is_reactor() const
{
    return info().has_flag( VPFLAG_REACTOR );
}

bool vehicle_part::is_turret() const
{
    return base.is_gun();
}

bool vehicle_part::is_seat() const
{
    return info().has_flag( "SEAT" );
}

const vpart_info &vehicle_part::info() const
{
    if( !info_cache ) {
        info_cache = &id.obj();
    }
    return *info_cache;
}

void vehicle::set_hp( vehicle_part &pt, int qty )
{
    if( qty == pt.info().durability || pt.info().durability <= 0 ) {
        pt.base.set_damage( 0 );

    } else if( qty == 0 ) {
        pt.base.set_damage( pt.base.max_damage() );

    } else {
        pt.base.set_damage( pt.base.max_damage() - pt.base.max_damage() * qty / pt.info().durability );
    }
}

bool vehicle::mod_hp( vehicle_part &pt, int qty, damage_type dt )
{
    if( pt.info().durability > 0 ) {
        return pt.base.mod_damage( -( pt.base.max_damage() * qty / pt.info().durability ), dt );
    } else {
        return false;
    }
}

bool vehicle::can_enable( const vehicle_part &pt, bool alert ) const
{
    if( std::none_of( parts.begin(), parts.end(), [&pt]( const vehicle_part & e ) {
    return &e == &pt;
} ) || pt.removed ) {
        debugmsg( "Cannot enable removed or non-existent part" );
    }

    if( pt.is_broken() ) {
        return false;
    }

    if( pt.info().has_flag( "PLANTER" ) && !warm_enough_to_plant( g->u.pos() ) ) {
        if( alert ) {
            add_msg( m_bad, _( "It is too cold to plant anything now." ) );
        }
        return false;
    }

    // TODO: check fuel for combustion engines

    if( pt.info().epower < 0 && fuel_left( fuel_type_battery, true ) <= 0 ) {
        if( alert ) {
            add_msg( m_bad, _( "Insufficient power to enable %s" ), pt.name() );
        }
        return false;
    }

    return true;
}

bool vehicle::assign_seat( vehicle_part &pt, const npc &who )
{
    if( !pt.is_seat() || !pt.set_crew( who ) ) {
        return false;
    }

    // NPC's can only be assigned to one seat in the vehicle
    for( auto &e : parts ) {
        if( &e == &pt ) {
            continue; // skip this part
        }

        if( e.is_seat() ) {
            const npc *n = e.crew();
            if( n && n->getID() == who.getID() ) {
                e.unset_crew();
            }
        }
    }

    return true;
}
