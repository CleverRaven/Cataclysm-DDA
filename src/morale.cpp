#include "morale.h"

#include "cata_utility.h"
#include "debug.h"
#include "itype.h"
#include "output.h"
#include "bodypart.h"
#include "player.h"

#include <stdlib.h>
#include <algorithm>

const efftype_id effect_took_prozac( "took_prozac" );

namespace
{
static const std::string item_name_placeholder = "%i"; // Used to address an item name

const std::string &get_morale_data( const morale_type id )
{
    static const std::array<std::string, NUM_MORALE_TYPES> morale_data = { {
            { "This is a bug (player.cpp:moraledata)" },
            { _( "Enjoyed %i" ) },
            { _( "Enjoyed a hot meal" ) },
            { _( "Music" ) },
            { _( "Enjoyed honey" ) },
            { _( "Played Video Game" ) },
            { _( "Marloss Bliss" ) },
            { _( "Mutagenic Anticipation" ) },
            { _( "Good Feeling" ) },
            { _( "Supported" ) },
            { _( "Looked at photos" ) },

            { _( "Nicotine Craving" ) },
            { _( "Caffeine Craving" ) },
            { _( "Alcohol Craving" ) },
            { _( "Opiate Craving" ) },
            { _( "Speed Craving" ) },
            { _( "Cocaine Craving" ) },
            { _( "Crack Cocaine Craving" ) },
            { _( "Mutagen Craving" ) },
            { _( "Diazepam Craving" ) },
            { _( "Marloss Craving" ) },

            { _( "Disliked %i" ) },
            { _( "Ate Human Flesh" ) },
            { _( "Ate Meat" ) },
            { _( "Ate Vegetables" ) },
            { _( "Ate Fruit" ) },
            { _( "Lactose Intolerance" ) },
            { _( "Ate Junk Food" ) },
            { _( "Wheat Allergy" ) },
            { _( "Ate Indigestible Food" ) },
            { _( "Wet" ) },
            { _( "Dried Off" ) },
            { _( "Cold" ) },
            { _( "Hot" ) },
            { _( "Bad Feeling" ) },
            { _( "Killed Innocent" ) },
            { _( "Killed Friend" ) },
            { _( "Guilty about Killing" ) },
            { _( "Guilty about Mutilating Corpse" ) },
            { _( "Fey Mutation" ) },
            { _( "Chimerical Mutation" ) },
            { _( "Mutation" ) },

            { _( "Moodswing" ) },
            { _( "Read %i" ) },
            { _( "Got comfy" ) },

            { _( "Heard Disturbing Scream" ) },

            { _( "Masochism" ) },
            { _( "Hoarder" ) },
            { _( "Stylish" ) },
            { _( "Optimist" ) },
            { _( "Bad Tempered" ) },
            //~ You really don't like wearing the Uncomfy Gear
            { _( "Uncomfy Gear" ) },
            { _( "Found kitten <3" ) },

            { _( "Got a Haircut" ) },
            { _( "Freshly Shaven" ) },
        }
    };

    if( static_cast<size_t>( id ) >= morale_data.size() ) {
        debugmsg( "invalid morale type: %d", id );
        return morale_data[0];
    }

    return morale_data[id];
}
} // namespace

std::string morale_point::get_name() const
{
    std::string name = get_morale_data( type );

    if( item_type != nullptr ) {
        name = string_replace( name, item_name_placeholder, item_type->nname( 1 ) );
    } else if( name.find( item_name_placeholder ) != std::string::npos ) {
        debugmsg( "%s(): Morale #%d (%s) requires item_type to be specified.", __FUNCTION__, type,
                  name.c_str() );
    }

    return name;
}

int morale_point::get_net_bonus() const
{
    return bonus * ( ( age > decay_start ) ? logarithmic_range( decay_start, duration, age ) : 1 );
}

int morale_point::get_net_bonus( const morale_mult &mult ) const
{
    return get_net_bonus() * mult;
}

bool morale_point::is_expired() const
{
    return age >= duration; // Will show zero morale bonus
}

void morale_point::add( int new_bonus, int new_max_bonus, int new_duration, int new_decay_start,
                        bool new_cap )
{
    if( new_cap ) {
        duration = new_duration;
        decay_start = new_decay_start;
    } else {
        bool same_sign = ( bonus > 0 ) == ( new_max_bonus > 0 );

        duration = pick_time( duration, new_duration, same_sign );
        decay_start = pick_time( decay_start, new_decay_start, same_sign );
    }

    bonus = get_net_bonus() + new_bonus;

    if( abs( bonus ) > abs( new_max_bonus ) && ( new_max_bonus != 0 || new_cap ) ) {
        bonus = new_max_bonus;
    }

    age = 0; // Brand new. The assignment should stay below.
}

int morale_point::pick_time( int current_time, int new_time, bool same_sign ) const
{
    const int remaining_time = current_time - age;
    return ( remaining_time <= new_time && same_sign ) ? new_time : remaining_time;
}

void morale_point::proceed( int ticks )
{
    if( ticks < 0 ) {
        debugmsg( "%s(): Called with negative ticks %d.", __FUNCTION__, ticks );
        return;
    }

    age += ticks;
}

void player_morale::add( morale_type type, int bonus, int max_bonus,
                        int duration, int decay_start,
                        bool capped, const itype* item_type )
{
    // Search for a matching morale entry.
    for( auto &m : points ) {
        if( m.get_type() == type && m.get_item_type() == item_type ) {
            const int prev_bonus = m.get_net_bonus();

            m.add( bonus, max_bonus, duration, decay_start, capped );
            if ( m.get_net_bonus() != prev_bonus ) {
                invalidate_level();
            }
            return;
        }
    }

    morale_point new_morale( type, item_type, bonus, duration, decay_start );

    if( !new_morale.is_expired() ) {
        points.push_back( new_morale );
        invalidate_level();
    }
}

int player_morale::has( morale_type type, const itype *item_type ) const
{
    for( auto &m : points ) {
        if( m.get_type() == type && ( item_type == nullptr || m.get_item_type() == item_type ) ) {
            return m.get_net_bonus();
        }
    }
    return 0;
}

void player_morale::remove( morale_type type, const itype* item_type )
{
    for( size_t i = 0; i < points.size(); ++i ) {
        if( points[i].get_type() == type && points[i].get_item_type() == item_type ) {
            if( points[i].get_net_bonus() > 0 ) {
                invalidate_level();
            }
            points.erase( points.begin() + i );
            break;
        }
    }
}

morale_mult player_morale::get_traits_mult() const
{
    morale_mult ret;

    if( p->has_trait( "OPTIMISTIC" ) ) {
        ret *= morale_mults::optimistic;
    }

    if( p->has_trait( "BADTEMPER" ) ) {
        ret *= morale_mults::badtemper;
    }

    return ret;
}

morale_mult player_morale::get_effects_mult() const
{
    morale_mult ret;

    //TODO: Maybe add something here to cheer you up as well?
    if( p->has_effect( effect_took_prozac ) ) {
        ret *= morale_mults::prozac;
    }

    return ret;
}

int player_morale::get_level() const
{
    if ( !level_is_valid ) {
        const morale_mult mult = get_traits_mult();

        level = 0;
        for( auto &m : points ) {
            level += m.get_net_bonus( mult );
        }

        level *= get_effects_mult();
        level_is_valid = true;
    }

    return level;
}

void player_morale::update( const int ticks )
{
    const auto proceed = [ ticks ]( morale_point &m ) { m.proceed( ticks ); };
    const auto is_expired = []( const morale_point &m ) -> bool { return m.is_expired(); };

    std::for_each( points.begin(), points.end(), proceed );
    const auto new_end = std::remove_if( points.begin(), points.end(), is_expired );
    points.erase( new_end, points.end() );
    // Invalidate level to recalculate it on demand
    invalidate_level();
}

void player_morale::clear()
{
    points.clear();
    invalidate_level();
}

void player_morale::invalidate_level()
{
    level_is_valid = false;
}
