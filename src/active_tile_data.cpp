#include "active_tile_data.h"
#include "coordinate_conversions.h"
#include "debug.h"
#include "electric_grid.h"
#include "item.h"
#include "itype.h"
#include "json.h"
#include "map.h"
#include "weather.h"

static const std::string flag_RECHARGE( "RECHARGE" );
static const std::string flag_USE_UPS( "USE_UPS" );

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

void solar_tile::update( time_point from, time_point to, map &m, const tripoint &p )
{
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
    m.electric_grid_at( ms_to_omt_copy( p ) )->mod_energy( produced );
}

active_tile_data *solar_tile::clone() const
{
    return new solar_tile( *this );
}

const std::string &solar_tile::get_type() const
{
    static const std::string type( "solar" );
    return type;
}

void solar_tile::store( JsonOut &jsout ) const
{
    jsout.member( "power", power );
}

void solar_tile::load( JsonObject &jo )
{
    // Can't use generic_factory because we don't have unique ids
    jo.read( "power", power );
    // TODO: Remove all of this, it's a hack around a mistake
    int dummy;
    jo.read( "stored_energy", dummy, false );
    jo.read( "max_energy", dummy, false );

}

void battery_tile::update( time_point, time_point, map &, const tripoint & )
{
    // TODO: Shouldn't have this function!
}

active_tile_data *battery_tile::clone() const
{
    return new battery_tile( *this );
}

const std::string &battery_tile::get_type() const
{
    static const std::string type( "battery" );
    return type;
}

void battery_tile::store( JsonOut &jsout ) const
{
    jsout.member( "stored", stored );
    jsout.member( "max_stored", max_stored );
}
void battery_tile::load( JsonObject &jo )
{
    jo.read( "stored", stored );
    jo.read( "max_stored", max_stored );
}

int battery_tile::mod_energy( int amt )
{
    if( amt > 0 ) {
        if( stored + amt >= max_stored ) {
            amt = stored + amt - max_stored;
            stored = max_stored;
            return amt;
        } else {
            stored += max_stored;
            return 0;
        }
    } else {
        if( stored - amt <= 0 ) {
            amt += stored;
            stored = 0;
            return amt;
        } else {
            stored += amt;
            return 0;
        }
    }
}

void charger_tile::update( time_point from, time_point to, map &m, const tripoint &p )
{
    if( !m.has_items( p ) ) {
        return;
    }
    electric_grid &grid = *m.electric_grid_at( p );
    int power = this->power * to_seconds<int>( to - from );
    // TODO: Make not a copy from map.cpp
    for( item &outer : m.i_at( p ) ) {
        outer.visit_items( [&power, &grid]( item * it ) {
            item &n = *it;
            if( !n.has_flag( flag_RECHARGE ) && !n.has_flag( flag_USE_UPS ) ) {
                return VisitResponse::NEXT;
            }
            if( n.ammo_capacity() > n.ammo_remaining() ||
                ( n.type->battery && n.type->battery->max_capacity > n.energy_remaining() ) ) {
                while( power >= 1000 || x_in_y( power, 1000 ) ) {
                    const int missing = grid.mod_energy( -1 );
                    if( missing == 0 ) {
                        if( n.is_battery() ) {
                            n.set_energy( 1_kJ );
                        } else {
                            n.ammo_set( "battery", n.ammo_remaining() + 1 );
                        }
                    }
                    power -= 1000;
                }
                return VisitResponse::ABORT;
            }

            return VisitResponse::SKIP;
        } );
    }
}

active_tile_data *charger_tile::clone() const
{
    return new charger_tile( *this );
}

const std::string &charger_tile::get_type() const
{
    static const std::string type( "charger" );
    return type;
}

void charger_tile::store( JsonOut &jsout ) const
{
    jsout.member( "power", power );
}

void charger_tile::load( JsonObject &jo )
{
    jo.read( "power", power );
}

active_tile_data *active_tile_data::create( const std::string &id )
{
    if( id == "solar" ) {
        return new solar_tile();
    } else if( id == "battery" ) {
        return new battery_tile();
    } else if( id == "charger" ) {
        return new charger_tile();
    }

    debugmsg( "Invalid active_tile_data id %s", id.c_str() );
    return new null_tile_data();
}
