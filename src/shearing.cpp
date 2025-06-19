#include "shearing.h"

#include <algorithm>
#include <vector>

#include "flexbuffer_json.h"
#include "generic_factory.h"
#include "monster.h"
#include "rng.h"
#include "units.h"

shearing_roll shearing_entry::roll( const monster &mon ) const
{
    shearing_roll roll;
    roll.result = result;

    roll.amount += amount;

    if( random_max ) {
        roll.amount += rng( random_min, random_max );
    }

    if( ratio_mass ) {
        float weight = units::to_kilogram( mon.get_weight() );
        roll.amount += static_cast<int>( ratio_mass * weight );
    }

    if( ratio_volume ) {
        float volume = units::to_liter( mon.get_volume() );
        roll.amount += static_cast<int>( ratio_volume * volume );
    }

    return roll;
}

void shearing_entry::load( const JsonObject &jo )
{
    mandatory( jo, was_loaded, "result", result );

    optional( jo, was_loaded, "ratio_mass", ratio_mass );
    ratio_mass = std::max( 0.00f, ratio_mass );

    optional( jo, was_loaded, "ratio_volume", ratio_volume );
    ratio_volume = std::max( 0.00f, ratio_volume );

    if( jo.has_int( "amount" ) ) {
        mandatory( jo, was_loaded, "amount", amount );
        amount = std::max( 0, amount );
    } else if( jo.has_array( "amount" ) ) {
        std::vector<int> amount_random = jo.get_int_array( "amount" );
        random_min = std::max( 0, amount_random[0] );
        random_max = std::max( 0, amount_random[1] );
        if( random_min > random_max ) {
            std::swap( random_min, random_max );
        }
    }
}

shearing_data::shearing_data( std::vector<shearing_entry> &shearing_entries )
{
    if( !shearing_entries.empty() ) {
        entries_ = shearing_entries;
        valid_ = true;
    }
}

std::vector<shearing_roll> shearing_data::roll_all( const monster &mon ) const
{
    std::vector<shearing_roll> rolls;
    rolls.reserve( entries_.size() );

    for( const shearing_entry &entry : entries_ ) {
        rolls.push_back( entry.roll( mon ) );
    }

    return rolls;
}
