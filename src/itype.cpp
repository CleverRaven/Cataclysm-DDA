#include "debug.h"
#include "itype.h"
#include "ammo.h"
#include "game.h"
#include "item_factory.h"
#include "translations.h"

#include <fstream>
#include <stdexcept>
#include <algorithm>

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
    if( !has_use() ) {
        return nullptr;
    }

    const use_function *func = item_controller->get_iuse( iuse_name );
    if( func != nullptr ) {
        if( std::find( use_methods.cbegin(), use_methods.cend(),
                       *func ) != use_methods.cend() ) {
            return func;
        }
    }

    for( const auto &method : use_methods ) {
        const auto ptr = method.get_actor_ptr();
        if( ptr != nullptr && ptr->type == iuse_name ) {
            return &method;
        }
    }

    return nullptr;
}

long itype::tick( player *p, item *it, const tripoint &pos ) const
{
    // Note: can go higher than current charge count
    // Maybe should move charge decrementing here?
    int charges_to_use = 0;
    for( auto &method : use_methods ) {
        int val = method.call( p, it, true, pos );
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

    return use_methods.front().call( p, it, false, pos );
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

std::string ammo_name( std::string const &t )
{
    std::string ret = ammunition_type::find_ammunition_type( t ).name();
    if( ret != "none" ) {
        ret = _( ret.c_str() );
    }
    return ret;
}

itype_id const &default_ammo( std::string const &t )
{
    return ammunition_type::find_ammunition_type( t ).default_ammotype();
}

// 1 nutr per 5 minutes = 12 * 24 = 288 per day
// Assuming 2500 kcal per day, 1 nutr ~= 8.7 kcal
constexpr float kcal_per_nutr = 2500.0f / ( 12 * 24 );

int it_comest::get_nutrition() const
{
    if( nutr >= 0 ) {
        return nutr;
    }

    return ( int )( kcal / kcal_per_nutr );
}

int it_comest::get_calories() const
{
    if( nutr >= 0 ) {
        return ( int )( nutr * kcal_per_nutr );
    }

    return kcal;
}
