#include "morale.h"
#include "itype.h"
#include "cata_utility.h"
#include "debug.h"

namespace {
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
        } };

        if( static_cast<size_t>( id ) >= morale_data.size() ) {
            debugmsg( "invalid morale type: %d", id );
            return morale_data[0];
        }

        return morale_data[id];
    }
} // namespace

std::string morale_point::name() const
{
    // Start with the morale type's description.
    std::string ret = get_morale_data( type );

    // Get the name of the referenced item (if any).
    std::string item_name = "";
    if( item_type != nullptr ) {
        item_name = item_type->nname( 1 );
    }

    // Replace each instance of %i with the item's name.
    size_t it = ret.find( "%i" );
    while( it != std::string::npos ) {
        ret.replace( it, 2, item_name );
        it = ret.find( "%i" );
    }

    return ret;
}

void morale_point::add( int bonus, int max_bonus, int duration, int decay_start, bool cap_existing )
{
    // If we're capping the existing effect, we can use the new duration
    // and decay start.
    if( cap_existing ) {
        this->duration = duration;
        this->decay_start = decay_start;
    } else {
        // Otherwise, we need to figure out whether the existing effect had
        // more remaining duration and decay-resistance than the new one does.
        // Only takes the new duration if new bonus and old are the same sign.
        if( this->duration - this->age <= duration && ( ( this->bonus > 0 ) == ( max_bonus > 0 ) ) ) {
            this->duration = duration;
        } else {
            this->duration -= this->age; // This will give a longer duration than above.
        }

        if( this->decay_start - this->age <= decay_start && ( ( this->bonus > 0 ) == ( max_bonus > 0 ) ) ) {
            this->decay_start = decay_start;
        } else {
            this->decay_start -= this->age; // This will give a later decay start than above.
        }
    }

    // Now that we've finished using it, reset the age to 0.
    this->age = 0;

    // Is the current morale level for this entry below its cap, if any?
    if( abs( this->bonus ) < abs( max_bonus ) || max_bonus == 0) {
        this->bonus += bonus; // Add the requested morale boost.
        // If we passed the cap, pull back to it.
        if( abs( this->bonus ) > abs( max_bonus ) && max_bonus != 0 ) {
            this->bonus = max_bonus;
        }
    } else if( cap_existing ) {
        // The existing bonus is above the new cap.  Reduce it.
        this->bonus = max_bonus;
    }
}

bool morale_point::is_expired()
{
    return age >= duration;
}

void morale_point::proceed( int ticks )
{
    if( ticks < 0 ) {
        debugmsg( "One can't turn back time." );
        return;
    }

    age += ticks;

    if( is_expired() ) {
        bonus = 0;
    } else if( age > decay_start ) {
        bonus *= logarithmic_range( decay_start, duration, age );
    }
}
