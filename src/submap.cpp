#include "submap.h"
#include "mapdata.h"
#include "trap.h"
#include "vehicle.h"

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

bool submap::has_graffiti( int x, int y ) const
{
    return cosmetics[x][y].count( COSMETICS_GRAFFITI ) > 0;
}

const std::string &submap::get_graffiti( int x, int y ) const
{
    const auto it = cosmetics[x][y].find( COSMETICS_GRAFFITI );
    if( it == cosmetics[x][y].end() ) {
        static const std::string empty_string;
        return empty_string;
    }
    return it->second;
}

void submap::set_graffiti( int x, int y, const std::string &new_graffiti )
{
    is_uniform = false;
    cosmetics[x][y][COSMETICS_GRAFFITI] = new_graffiti;
}

void submap::delete_graffiti( int x, int y )
{
    is_uniform = false;
    cosmetics[x][y].erase( COSMETICS_GRAFFITI );
}
