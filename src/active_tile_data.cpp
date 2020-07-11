#include "active_tile_data.h"
#include "debug.h"
#include "map.h"
#include "weather.h"

active_tile_data::~active_tile_data() {}

class null_tile_data : public active_tile_data
{
        void update( time_point, time_point, map &, const tripoint & ) override
        {}
        active_tile_data *clone() const override {
            return new null_tile_data( *this );
        }

        const std::string &get_type() const override {
            static const std::string type( "null" );
            return type;
        }
        void store( JsonOut & ) const override
        {}
        void load( JsonObject & ) override
        {}
};

// Copypasted from character.cpp
// TODO: Move somewhere (calendar?)
inline int ticks_between( const time_point &from, const time_point &to,
                          const time_duration &tick_length )
{
    return ( to_turn<int>( to ) / to_turns<int>( tick_length ) ) - ( to_turn<int>
            ( from ) / to_turns<int>( tick_length ) );
}

class solar_tile : public active_tile_data
{
        /* In kJ */
        int stored_energy;
        int max_energy;
        /* In W */
        int power;

        void update( time_point from, time_point to, map &m, const tripoint &p ) override {
            constexpr time_duration tick_length = 1_minutes;
            int ticks = ticks_between( from, to, tick_length );
            // This is to cut down on sum_conditions
            if( ticks <= 0 ) {
                return;
            }
            // TODO: Use something that doesn't calc a ton of worthless crap
            int sunlight = sum_conditions( from, to, m.getabs( p ) ).sunlight;
            // int64 because we can have years in here
            int produced = to_turns<std::int64_t>( tick_length ) * sunlight * ticks / 1000;
            stored_energy = std::min( stored_energy + produced, max_energy );
        }
        active_tile_data *clone() const override {
            return new solar_tile( *this );
        }

        const std::string &get_type() const override {
            static const std::string type( "solar" );
            return type;
        }

        void store( JsonOut &jsout ) const override {
            jsout.member( "stored_energy", stored_energy );
            jsout.member( "max_energy", max_energy );
            jsout.member( "power", power );
        }
        void load( JsonObject &jo ) override {
            // Can't use generic_factory because we don't have unique ids
            jo.read( "stored_energy", stored_energy );
            jo.read( "max_energy", max_energy );
            jo.read( "power", power );
        }
};

active_tile_data *active_tile_data::create( const std::string &id )
{
    if( id == "solar" ) {
        return new solar_tile();
    }

    debugmsg( "Invalid active_tile_data id %s", id.c_str() );
    return new null_tile_data();
}
