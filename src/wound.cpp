#include "wound.h"

#include "damage.h"
#include "generic_factory.h"

std::map<damage_type, std::vector<wound_id>> wound_type::wound_lookup;

namespace
{
generic_factory<wound_type> wound_factory( "wound" );
} // namespace

const std::vector<wound_type> &wound_type::get_all()
{
    return wound_factory.get_all();
}

template<>
const wound_type &string_id<wound_type>::obj() const
{
    return wound_factory.obj( *this );
}

template<>
bool string_id<wound_type>::is_valid() const
{
    return wound_factory.is_valid( *this );
}

void wound_type::load_wound( const JsonObject &jo, const std::string &src )
{
    wound_factory.load( jo, src );
}

void wound_type::finalize()
{
    wound_type::wound_lookup.clear();
    for( const wound_type &cur_wound : get_all() ) {
        wound_lookup[cur_wound.dmg_type].push_back( cur_wound.id );
    }
}

void wound_type::load( const JsonObject &jo, const std::string & )
{
    optional( jo, was_loaded, "name", name );
    optional( jo, was_loaded, "description", description );
    optional( jo, was_loaded, "size", size, 0 );
    optional( jo, was_loaded, "sunder", sunder, 0 );
    optional( jo, was_loaded, "pain", pain );
    optional( jo, was_loaded, "min_damage", min_damage, 1 );
    optional( jo, was_loaded, "max_damage", max_damage, INT_MAX );
    mandatory( jo, was_loaded, "damage_type", dmg_type );
    mandatory( jo, was_loaded, "limbs", limbs );
    optional( jo, was_loaded, "bleed", bleed, volume_reader{}, 0_ml );
    optional( jo, was_loaded, "heal_time", heal_time );
    optional( jo, was_loaded, "heals_into", heals_into );
    optional( jo, was_loaded, "infects_into", infects_into );
}

wound::wound( const damage_unit &damage )
{
    const auto lookup_iter = wound_type::wound_lookup.find( damage.type );

    if( lookup_iter == wound_type::wound_lookup.cend() ) {
        debugmsg( string_format( "Error: No wound found that has damage type %s",
                                 io::enum_to_string( damage.type ) ) );
        // no point looking through empty vectors
        return;
    }

    // damage unit lookup here
    std::vector<wound_id> potential_hurt;
    for( const wound_id &wnd : lookup_iter->second ) {
        if( wnd->min_damage <= damage.amount && wnd->max_damage >= damage.amount ) {
            potential_hurt.push_back( wnd );
        }
    }
    int pick_random = rng( 0, static_cast<int>( potential_hurt.size() ) - 1 );
    id = potential_hurt[pick_random];
    intensity_multiplier = 1.0 + static_cast<double>( damage.amount - id->min_damage ) /
                           static_cast<double>( id->max_damage - id->min_damage );
}

std::string wound::name() const
{
    return id->name.translated();
}

std::string wound::description() const
{
    return id->description.translated();
}

wound wound::wound_healed() const
{
    if( !heals_into() ) {
        debugmsg( "Error: wound trying to heal into nonexistent wound" );
    }
    wound healed_wound;
    healed_wound.id = *heals_into();
    healed_wound.intensity_multiplier = intensity_multiplier;
    healed_wound.contamination = contamination;

    return healed_wound;
}

wound wound::wound_infected() const
{
    if( !infects_into() ) {
        debugmsg( "Error: wound trying to infect into nonexistent wound" );
    }
    wound infected_wound;
    infected_wound.id = *infects_into();
    infected_wound.intensity_multiplier = intensity_multiplier;
    infected_wound.contamination = contamination;

    return infected_wound;
}

bool wound::process( const time_duration &t, double healing_factor )
{
    age += t * healing_factor * ( 1 - infection );
    // stand-in sentinel value
    infection += 0.01 * contamination * to_seconds<double>( t );

    return id->heal_time && age >= *id->heal_time || is_infected();
}

std::optional<wound_id> wound::heals_into() const
{
    return id->heals_into;
}

std::optional<wound_id> wound::infects_into() const
{
    return id->infects_into;
}

double wound::wound_progression() const
{
    if( !id->heal_time ) {
        // this is a permanent wound
        return 0.0;
    }
    return to_turns<double>( age ) / to_turns<double>( *id->heal_time );
}

double wound::infection_progression() const
{
    return infection;
}

bool wound::is_infected() const
{
    return infection_progression() >= 1.0;
}

bool wound::overlaps( int target ) const
{
    if( location + id->size <= 100 ) {
        return location >= target && location + id->size <= target;
    } else {
        return location >= target || location + id->size - 100 <= target;
    }
}

int wound::sunder() const
{
    return id->sunder;
}

damage_type wound::damage_type() const
{
    return id->dmg_type;
}

const std::vector<wound> &limb_wounds::get_all_wounds() const
{
    return wounds;
}

int limb_wounds::sunder( damage_type dmg_type, int location ) const
{
    int total = 0;
    for( const wound &wnd : wounds ) {
        if( dmg_type == wnd.damage_type() && wnd.overlaps( location ) ) {
            total += wnd.sunder();
        }
    }
    return total;
}

void limb_wounds::add_wound( const damage_instance &damage, int location )
{
    for( damage_unit du : damage.damage_units ) {
        du.amount += sunder( du.type, location );
        add_wound( wound( du ) );
    }
}

void limb_wounds::add_wound( const wound &wnd )
{
    wounds.push_back( wnd );
}

// for this function, the bulk of it is actually the loop with the iterators.
// the actual processing is wound::process and any special rules should be put into there.
void limb_wounds::process( const time_duration &t, double healing_factor )
{
    // start at the ending because we'll be adding more to the real end as we go along
    for( auto wound_iter = wounds.rbegin(); wound_iter != wounds.rend(); ) {
        if( wound_iter->process( t, healing_factor ) ) {
            // the wound's age renews when it transforms
            if( wound_iter->is_infected() ) {
                // "infects" the wound
                add_wound( wound_iter->wound_infected() );
            } else if( wound_iter->heals_into() ) {
                // "heals" the wound
                add_wound( wound_iter->wound_healed() );
            }
            wound_iter = decltype( wound_iter )( wounds.erase( std::next( wound_iter ).base() ) );
        } else {
            ++wound_iter;
        }
    }
}
