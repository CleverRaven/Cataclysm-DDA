#include "vehicle.h" // IWYU pragma: associated

#include <algorithm>
#include <cmath>
#include <memory>
#include <set>
#include <string>

#include "ammo.h"
#include "cata_assert.h"
#include "character.h"
#include "color.h"
#include "debug.h"
#include "enums.h"
#include "fault.h"
#include "flag.h"
#include "game.h"
#include "item.h"
#include "itype.h"
#include "iuse_actor.h"
#include "map.h"
#include "messages.h"
#include "npc.h"
#include "pocket_type.h"
#include "ret_val.h"
#include "string_formatter.h"
#include "translations.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_type.h"
#include "vpart_position.h"
#include "weather.h"

static const ammotype ammo_battery( "battery" );

static const itype_id fuel_type_battery( "battery" );
static const itype_id fuel_type_none( "null" );

static const itype_id itype_battery( "battery" );

/*-----------------------------------------------------------------------------
 *                              VEHICLE_PART
 *-----------------------------------------------------------------------------*/
vehicle_part::vehicle_part()
    : vehicle_part( vpart_id::NULL_ID(), item( vpart_id::NULL_ID()->base_item ) ) { }

vehicle_part::vehicle_part( const vpart_id &type, item &&base )
    : info_( &type.obj() )
{
    set_base( std::move( base ) );
    variant = info_->variant_default;
}

vehicle_part::vehicle_part( const vpart_id &type, item &&base, std::vector<item> &installed_with )
    : info_( &type.obj() )
{
    set_base( std::move( base ) );
    variant = info_->variant_default;
    for( item &it : installed_with ) {
        if( !it.has_flag( flag_UNRECOVERABLE ) ) {
            salvageable.push_back( it );
        }
    }
}

const item &vehicle_part::get_base() const
{
    return base;
}

std::vector<item> vehicle_part::get_salvageable() const
{
    std::vector<item> tmp;
    if( has_flag( vp_flag::unsalvageable_flag ) ) {
        return tmp;
    }
    if( !salvageable.empty() ) {
        return salvageable;
    }
    // Get default install component
    const requirement_data reqs = info().install_requirements();
    for( const auto &altercomps : reqs.get_components() ) {
        const item_comp &comp = altercomps.front();
        item newit( comp.type, calendar::turn );
        if( base.typeId() != comp.type && !newit.has_flag( flag_UNRECOVERABLE ) ) {
            int compcount = comp.count;
            const bool is_liquid = newit.made_of( phase_id::LIQUID );
            if( newit.count_by_charges() || is_liquid ) {
                newit.charges = compcount;
                compcount = 1;
            } else if( !newit.craft_has_charges() && newit.charges > 0 ) {
                newit.charges = 0;
            }
            for( ; compcount > 0; compcount-- ) {
                tmp.push_back( newit );
            }
        }
    }
    return tmp;
}

void vehicle_part::set_base( item &&new_base )
{
    if( new_base.typeId() != info().base_item ) {
        debugmsg( "new base '%s' doesn't match part type '%s', this is a bug",
                  new_base.typeId().str(), info().id.str() );
        return;
    }
    base = std::move( new_base );
    base.set_flag( flag_VEHICLE );
}

std::string vehicle_part::name( bool with_prefix ) const
{
    std::string res;
    if( with_prefix ) {
        res += base.damage_indicator() + base.degradation_symbol() + " ";
        if( !base.type->degrade_increments() ) {
            res += " "; // aligns names when printing degrading and non-degrading parts with prefixes
        }
    }
    if( base.is_wheel() ) {
        res += string_format( _( "%d\" " ), base.type->wheel->diameter );
    }
    res += info().name();
    if( base.has_var( "contained_name" ) ) {
        res += string_format( _( " holding %s" ), base.get_var( "contained_name" ) );
    }
    for( const fault_id &f : base.faults ) {
        const std::string prefix = f->item_prefix();
        if( !prefix.empty() ) {
            res += " (" + prefix + ")";
            break;
        }
    }
    if( health_percent() < floating_leak_threshold() && info().has_flag( VPFLAG_FLOATS ) &&
        !info().has_flag( VPFLAG_NO_LEAK ) ) {
        res += _( " (leaking)" );
    }
    if( is_leaking() ) {
        res += _( " (draining)" );
    }
    if( debug_mode ) {
        res += "{" + variant + "}";
    }
    return res;
}

int vehicle_part::hp() const
{
    const int dur = info().durability;
    if( base.max_damage() > 0 ) {
        return dur - dur * damage_percent();
    } else {
        return dur;
    }
}

int vehicle_part::damage() const
{
    return base.damage();
}

int vehicle_part::degradation() const
{
    return base.degradation();
}

int vehicle_part::max_damage() const
{
    return base.max_damage();
}

int vehicle_part::damage_level() const
{
    return base.damage_level();
}

bool vehicle_part::is_repairable() const
{
    return !is_broken() && base.repairable_levels() > 0 && info().is_repairable();
}

double vehicle_part::health_percent() const
{
    return 1.0 - damage_percent();
}

double vehicle_part::floating_leak_threshold() const
{
    return 0.5;
}

double vehicle_part::damage_percent() const
{
    return static_cast<double>( damage() ) / max_damage();
}

/** parts are considered broken at zero health */
bool vehicle_part::is_broken() const
{
    return base.damage() >= base.max_damage();
}

bool vehicle_part::is_cleaner_on() const
{
    const bool is_cleaner = info().has_flag( VPFLAG_AUTOCLAVE ) ||
                            info().has_flag( VPFLAG_DISHWASHER ) ||
                            info().has_flag( VPFLAG_WASHING_MACHINE );
    return is_cleaner && enabled;
}

bool vehicle_part::is_unavailable( const bool carried ) const
{
    return is_broken() || ( has_flag( vp_flag::carried_flag ) && carried );
}

bool vehicle_part::is_available( const bool carried ) const
{
    return !is_unavailable( carried );
}

itype_id vehicle_part::fuel_current() const
{
    if( !ammo_pref.is_null() ) {
        return ammo_pref;
    }
    return info().fuel_type;
}

bool vehicle_part::fuel_set( const itype_id &fuel )
{
    if( is_engine() ) {
        for( const itype_id &avail : info().engine_info->fuel_opts ) {
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
        return itype_battery;
    }

    if( is_tank() && !base.empty() ) {
        return base.legacy_front().typeId();
    }

    if( is_fuel_store( false ) || is_turret() ) {
        return base.ammo_current() != itype_id::NULL_ID() ? base.ammo_current() : base.ammo_default();
    }

    return itype_id::NULL_ID();
}

int vehicle_part::ammo_capacity( const ammotype &ammo ) const
{
    if( is_tank() ) {
        const itype *ammo_type = item::find_type( ammo->default_ammotype() );
        const int max_charges_volume = ammo_type->charges_per_volume( base.get_total_capacity() );
        const int max_charges_weight = ammo_type->weight == 0_gram ? INT_MAX :
                                       static_cast<int>( base.get_total_weight_capacity() / ammo_type->weight );
        return std::min( max_charges_volume, max_charges_weight );
    }

    if( is_fuel_store( false ) || is_turret() ) {
        return base.ammo_capacity( ammo );
    }

    return 0;
}

int vehicle_part::ammo_remaining() const
{
    if( is_tank() ) {
        return base.empty() ? 0 : base.legacy_front().charges;
    }
    if( is_fuel_store( false ) || is_turret() ) {
        return base.ammo_remaining();
    }

    return 0;
}

int vehicle_part::remaining_ammo_capacity() const
{
    return base.remaining_ammo_capacity();
}

int vehicle_part::ammo_set( const itype_id &ammo, int qty )
{
    // We often check if ammo is set to see if tank is empty, if qty == 0 don't set ammo
    if( is_tank() && qty != 0 ) {
        const itype *ammo_itype = item::find_type( ammo );
        if( ammo_itype && ammo_itype->ammo && ammo_itype->phase >= phase_id::LIQUID ) {
            base.clear_items();
            const int limit = ammo_capacity( ammo_itype->ammo->type );
            // assuming "ammo" isn't really going into a magazine as this is a vehicle part
            const int amount = qty > 0 ? std::min( qty, limit ) : limit;
            base.put_in( item( ammo, calendar::turn, amount ), pocket_type::CONTAINER );
            return amount;
        }
    }

    if( is_turret() ) {
        if( base.is_magazine() ) {
            return base.ammo_set( ammo, qty ).ammo_remaining();
        }
        itype_id mag_type = base.magazine_default();
        if( mag_type ) {
            item mag( mag_type );
            mag.ammo_set( ammo, qty );
            base.put_in( mag, pocket_type::MAGAZINE_WELL );
            return base.ammo_remaining();
        }
    }

    if( is_fuel_store() ) {
        const itype *ammo_itype = item::find_type( ammo );
        if( ammo_itype && ammo_itype->ammo ) {
            base.ammo_set( ammo, qty >= 0 ? qty : ammo_capacity( ammo_itype->ammo->type ) );
            return base.ammo_remaining();
        }
    }

    return -1;
}

void vehicle_part::ammo_unset()
{
    if( is_tank() ) {
        base.clear_items();
    } else if( is_fuel_store() ) {
        base.ammo_unset();
    }
}

int vehicle_part::ammo_consume( int qty, const tripoint &pos )
{
    if( is_tank() && !base.empty() ) {
        const int res = std::min( ammo_remaining(), qty );
        item &liquid = base.legacy_front();
        liquid.charges -= res;
        if( liquid.charges == 0 ) {
            base.clear_items();
        }
        return res;
    }
    return base.ammo_consume( qty, pos, nullptr );
}

units::energy vehicle_part::consume_energy( const itype_id &ftype, units::energy wanted_energy )
{
    if( !is_fuel_store() ) {
        return 0_J;
    }

    for( item *const fuel : base.all_items_top() ) {
        if( fuel->typeId() != ftype || !fuel->is_fuel() ) {
            continue;
        }
        const units::energy energy_per_charge = fuel->fuel_energy();
        const int charges_wanted = static_cast<int>( wanted_energy / energy_per_charge );
        const int charges_to_use = std::min( charges_wanted, fuel->charges );
        fuel->charges -= charges_to_use;
        if( fuel->charges == 0 ) {
            base.remove_item( *fuel );
        }

        return charges_to_use * energy_per_charge;
    }
    return 0_J;
}

bool vehicle_part::can_reload( const item &obj ) const
{
    // first check part is not destroyed and can contain ammo
    if( !is_fuel_store() ) {
        return false;
    }

    if( is_battery() ) {
        return false;
    }

    if( !obj.is_null() ) {
        const itype_id obj_type = obj.typeId();
        if( is_reactor() ) {
            return base.can_reload_with( obj, true );
        }

        // forbid filling tanks with solids or non-material things
        if( is_tank() && ( obj.made_of( phase_id::SOLID ) || obj.made_of( phase_id::PNULL ) ) ) {
            return false;
        }
        // forbid putting liquids, gasses, and plasma in things that aren't tanks
        if( !obj.made_of( phase_id::SOLID ) && !is_tank() ) {
            return false;
        }
        // prevent mixing of different ammo
        if( !ammo_current().is_null() && ammo_current() != obj_type ) {
            return false;
        }
        // For storage with set type, prevent filling with different types
        if( info().fuel_type != fuel_type_none && info().fuel_type != obj_type ) {
            return false;
        }
        // don't fill magazines with inappropriate fuel
        if( !is_tank() && !base.can_reload_with( obj, true ) ) {
            return false;
        }
    }
    if( base.is_gun() ) {
        return false;
    }

    if( is_reactor() ) {
        return true;
    }

    if( ammo_current().is_null() ) {
        return true; // empty tank
    }

    // Despite checking for an empty tank, item::find_type can still turn up with an empty ammo pointer
    if( cata::value_ptr<islot_ammo> a_val = item::find_type( ammo_current() )->ammo ) {
        return ammo_remaining() < ammo_capacity( a_val->type );
    }

    // Nothing in tank
    return ammo_capacity( obj.ammo_type() ) > 0;
}

void vehicle_part::process_contents( map &here, const tripoint &pos, const bool e_heater )
{
    // for now we only care about processing food containers since things like
    // fuel don't care about temperature yet
    temperature_flag flag = temperature_flag::NORMAL;
    if( e_heater ) {
        flag = temperature_flag::HEATER;
    }
    if( enabled && info().has_flag( VPFLAG_FRIDGE ) ) {
        flag = temperature_flag::FRIDGE;
    } else if( enabled && info().has_flag( VPFLAG_FREEZER ) ) {
        flag = temperature_flag::FREEZER;
    } else if( enabled && info().has_flag( VPFLAG_HEATED_TANK ) ) {
        flag = temperature_flag::HEATER;
    }
    base.process( here, nullptr, pos, 1, flag );
}

bool vehicle_part::fill_with( item &liquid, int qty )
{
    if( ( is_tank() && !liquid.made_of( phase_id::LIQUID ) ) || !can_reload( liquid ) ) {
        return false;
    }

    int charges_max = 0;
    if( cata::value_ptr<islot_ammo> a_val = item::find_type( ammo_current() )->ammo ) {
        charges_max = ammo_capacity( a_val->type ) - ammo_remaining();
    } else {
        // Nothing in tank
        charges_max = ammo_capacity( liquid.ammo_type() );
    }
    qty = qty < liquid.charges ? qty : liquid.charges;

    if( charges_max < liquid.charges ) {
        qty = charges_max;
    }

    liquid.charges -= base.fill_with( liquid, qty );

    return true;
}

const std::set<fault_id> &vehicle_part::faults() const
{
    return base.faults;
}

bool vehicle_part::has_fault_flag( const std::string &searched_flag ) const
{
    return base.has_fault_flag( searched_flag );
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
    const vpart_info &vp = info();
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

bool vehicle_part::contains_liquid() const
{
    return is_tank() && !base.empty() &&
           base.only_item().made_of( phase_id::LIQUID );
}

bool vehicle_part::is_battery() const
{
    return info().has_flag( VPFLAG_BATTERY ) ||
           ( base.is_magazine() && base.ammo_types().count( ammo_battery ) );
}

bool vehicle_part::is_reactor() const
{
    return info().has_flag( VPFLAG_REACTOR );
}

bool vehicle_part::is_leaking() const
{
    return  health_percent() <= 0.5 && ( is_tank() || is_battery() || is_reactor() );
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
    return *info_;
}

void vehicle::set_hp( vehicle_part &pt, int qty, bool keep_degradation, int new_degradation )
{
    int dur = pt.info().durability;
    if( qty == dur || dur <= 0 ) {
        pt.base.set_damage( keep_degradation ? pt.base.degradation() : 0 );

    } else if( qty == 0 ) {
        pt.base.set_damage( pt.base.max_damage() );

    } else {
        int amt = pt.base.max_damage() - pt.base.max_damage() * qty / dur;
        amt = std::max( amt, pt.base.degradation() );
        pt.base.set_damage( amt );
    }
    if( !keep_degradation ) {
        if( new_degradation >= 0 ) {
            pt.base.set_degradation( new_degradation );
        } else {
            pt.base.rand_degradation();
        }
    } else if( new_degradation >= 0 ) {
        pt.base.set_degradation( new_degradation );
    }
}

bool vehicle::mod_hp( vehicle_part &pt, int qty )
{
    const int dur = pt.info().durability;
    if( dur <= 0 ) {
        return false;
    }
    return pt.base.mod_damage( -qty * pt.base.max_damage() / dur );
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

    if( pt.info().has_flag( "PLANTER" ) && !warm_enough_to_plant( get_player_character().pos() ) ) {
        if( alert ) {
            add_msg( m_bad, _( "It is too cold to plant anything now." ) );
        }
        return false;
    }

    // TODO: check fuel for combustion engines

    if( pt.info().epower < 0_W && fuel_left( fuel_type_battery ) <= 0 ) {
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
    for( vehicle_part &e : parts ) {
        if( &e == &pt ) {
            // skip this part
            continue;
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

std::string vehicle_part::carried_name() const
{
    return carried_stack.empty()
           ? std::string()
           : carried_stack.top().veh_name;
}
