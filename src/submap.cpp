#include "submap.h"
#include "mapdata.h"
#include "trap.h"
#include "vehicle.h"
#include "computer.h"

#include <memory>

submap::submap()
{
    constexpr size_t elements = SEEX * SEEY;

    std::uninitialized_fill_n( &ter[0][0], elements, t_null );
    std::uninitialized_fill_n( &frn[0][0], elements, f_null );
    std::uninitialized_fill_n( &lum[0][0], elements, 0 );
    std::uninitialized_fill_n( &trp[0][0], elements, tr_null );
    std::uninitialized_fill_n( &rad[0][0], elements, 0 );

    is_uniform = false;
}

submap::~submap()
{
    delete_vehicles();
}

void submap::delete_vehicles()
{
    for( vehicle *veh : vehicles ) {
        delete veh;
    }
    vehicles.clear();
}

static const std::string COSMETICS_GRAFFITI( "GRAFFITI" );
static const std::string COSMETICS_SIGNAGE( "SIGNAGE" );
// Handle GCC warning: 'warning: returning reference to temporary'
static const std::string STRING_EMPTY( "" );

struct cosmetic_find_result {
    bool result;
    int ndx;
};
static cosmetic_find_result make_result( bool b, int ndx )
{
    cosmetic_find_result result;
    result.result = b;
    result.ndx = ndx;
    return result;
}
static cosmetic_find_result find_cosmetic(
    const std::vector<submap::cosmetic_t> &cosmetics, const point p, const std::string &type )
{
    for( size_t i = 0; i < cosmetics.size(); ++i ) {
        if( cosmetics[i].p == p && cosmetics[i].type == type ) {
            return make_result( true, i );
        }
    }
    return make_result( false, -1 );
}

bool submap::has_graffiti( int x, int y ) const
{
    point tp( x, y );
    return find_cosmetic( cosmetics, tp, COSMETICS_GRAFFITI ).result;
}

const std::string &submap::get_graffiti( int x, int y ) const
{
    point tp( x, y );
    auto fresult = find_cosmetic( cosmetics, tp, COSMETICS_GRAFFITI );
    if( fresult.result ) {
        return cosmetics[ fresult.ndx ].str;
    }
    return STRING_EMPTY;
}

void submap::set_graffiti( int x, int y, const std::string &new_graffiti )
{
    is_uniform = false;
    point tp( x, y );
    // Find signage at tp if available
    auto fresult = find_cosmetic( cosmetics, tp, COSMETICS_GRAFFITI );
    if( fresult.result ) {
        cosmetics[ fresult.ndx ].str = new_graffiti;
    } else {
        insert_cosmetic( tp, COSMETICS_GRAFFITI, new_graffiti );
    }
}

void submap::delete_graffiti( int x, int y )
{
    is_uniform = false;
    point tp( x, y );
    auto fresult = find_cosmetic( cosmetics, tp, COSMETICS_GRAFFITI );
    if( fresult.result ) {
        cosmetics[ fresult.ndx ] = cosmetics.back();
        cosmetics.pop_back();
    }
}
bool submap::has_signage( const int x, const int y ) const
{
    if( frn[x][y] == furn_id( "f_sign" ) ) {
        point tp( x, y );
        return find_cosmetic( cosmetics, tp, COSMETICS_SIGNAGE ).result;
    }

    return false;
}
const std::string submap::get_signage( const int x, const int y ) const
{
    if( frn[x][y] == furn_id( "f_sign" ) ) {
        point tp( x, y );
        const auto fresult = find_cosmetic( cosmetics, tp, COSMETICS_SIGNAGE );
        if( fresult.result ) {
            return cosmetics[ fresult.ndx ].str;
        }
    }

    return STRING_EMPTY;
}
void submap::set_signage( const int x, const int y, const std::string &s )
{
    is_uniform = false;
    point tp( x, y );
    // Find signage at tp if available
    auto fresult = find_cosmetic( cosmetics, tp, COSMETICS_SIGNAGE );
    if( fresult.result ) {
        cosmetics[ fresult.ndx ].str = s;
    } else {
        insert_cosmetic( tp, COSMETICS_SIGNAGE, s );
    }
}
void submap::delete_signage( const int x, const int y )
{
    is_uniform = false;
    point tp( x, y );
    auto fresult = find_cosmetic( cosmetics, tp, COSMETICS_SIGNAGE );
    if( fresult.result ) {
        cosmetics[ fresult.ndx ] = cosmetics.back();
        cosmetics.pop_back();
    }
}
