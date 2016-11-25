#include "debug.h"
#include "itype.h"
#include "ammo.h"
#include "game.h"
#include "item_factory.h"
#include "translations.h"

#include <stdexcept>
#include <algorithm>
#include <cmath>

double friction_loss = 0.05;

std::string itype::nname( unsigned int const quantity ) const
{
    return ngettext( name.c_str(), name_plural.c_str(), quantity );
}

// Members of iuse struct, which is slowly morphing into a class.
bool itype::has_use() const
{
    return !use_methods.empty();
}

bool itype::can_use( const std::string &iuse_name ) const
{
    return get_use( iuse_name ) != nullptr;
}

const use_function *itype::get_use( const std::string &iuse_name ) const
{
    auto iter = use_methods.find( iuse_name );
    return iter != use_methods.end() ? &iter->second : nullptr;
}

long itype::tick( player *p, item *it, const tripoint &pos ) const
{
    // Note: can go higher than current charge count
    // Maybe should move charge decrementing here?
    int charges_to_use = 0;
    for( auto &method : use_methods ) {
        int val = method.second.call( p, it, true, pos );
        if( charges_to_use < 0 || val < 0 ) {
            charges_to_use = -1;
        } else {
            charges_to_use += val;
        }
    }

    return charges_to_use;
}

long itype::invoke( player *p, item *it, const tripoint &pos ) const
{
    if( !has_use() ) {
        return 0;
    }

    return use_methods.begin()->second.call( p, it, false, pos );
}

long itype::invoke( player *p, item *it, const tripoint &pos, const std::string &iuse_name ) const
{
    const use_function *use = get_use( iuse_name );
    if( use == nullptr ) {
        debugmsg( "Tried to invoke %s on a %s, which doesn't have this use_function",
                  iuse_name.c_str(), nname( 1 ).c_str() );
        return 0;
    }

    return use->call( p, it, false, pos );
}

std::string ammo_name( const ammotype &t )
{
    return t.obj().name();
}

const itype_id &default_ammo( const ammotype &t )
{
    return t.obj().default_ammotype();
}

double islot_engine::velocity_max( int mass, float dynamics ) const
{
    // scale engine power (J/s) to amount where 100% of output is consumed replacing friction losses
    double e = power * dynamics / friction_loss;
    return sqrt( e / mass );
}

double islot_engine::velocity_optimal( int mass, float dynamics ) const
{
    auto limit = velocity_max( mass, dynamics );
    if( gears.empty() ) {
        return limit; // engines without discrete gearing are equally efficient at any velocity
    }
    return std::min( double( optimum ) / gears.back(), limit );
}

double islot_engine::velocity_safe( int mass, float dynamics ) const
{
    auto limit = velocity_max( mass, dynamics );
    if( gears.empty() ) {
        return limit; // engines without discrete gearing can operate safely at any velocity
    }
    return std::min( double( redline ) / gears.back(), limit );
}

int islot_engine::best_gear( double velocity ) const
{
    int best = INT_MAX;
    int sel = -1; // either no gears or stalled

    for( size_t idx = 0; idx != gears.size(); ++idx ) {
        // calculate engine RPM using this gear
        int eng_rpm = std::abs( velocity ) * gears[idx];

        // dont allow selection of any gear that would result in a stall
        if( eng_rpm < idle ) {
            continue;
        }

        // select best gear (closest to optimum rpm)
        int delta = std::abs( eng_rpm - optimum );
        if( delta < best ) {
            best = delta;
            sel = idx;
        }
    }

    return sel;
}

int islot_engine::effective_rpm( double velocity ) const
{
    int g = best_gear( velocity );
    return g >= 0 ? std::abs( velocity ) * gears[g] : 0;
}
