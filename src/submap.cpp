#include "submap.h"

#include <algorithm>
#include <array>
#include <iterator>
#include <memory>
#include <utility>

#include "basecamp.h"
#include "int_id.h"
#include "mapdata.h"
#include "tileray.h"
#include "trap.h"
#include "vehicle.h"

template<int sx, int sy>
void maptile_soa<sx, sy>::swap_soa_tile( const point &p1, const point &p2 )
{
    std::swap( ter[p1.x][p1.y], ter[p2.x][p2.y] );
    std::swap( frn[p1.x][p1.y], frn[p2.x][p2.y] );
    std::swap( lum[p1.x][p1.y], lum[p2.x][p2.y] );
    std::swap( itm[p1.x][p1.y], itm[p2.x][p2.y] );
    std::swap( fld[p1.x][p1.y], fld[p2.x][p2.y] );
    std::swap( trp[p1.x][p1.y], trp[p2.x][p2.y] );
    std::swap( rad[p1.x][p1.y], rad[p2.x][p2.y] );
}

template<int sx, int sy>
void maptile_soa<sx, sy>::swap_soa_tile( const point &p, maptile_soa<1, 1> &other )
{
    std::swap( ter[p.x][p.y], **other.ter );
    std::swap( frn[p.x][p.y], **other.frn );
    std::swap( lum[p.x][p.y], **other.lum );
    std::swap( itm[p.x][p.y], **other.itm );
    std::swap( fld[p.x][p.y], **other.fld );
    std::swap( trp[p.x][p.y], **other.trp );
    std::swap( rad[p.x][p.y], **other.rad );
}

submap::submap()
{
    std::uninitialized_fill_n( &ter[0][0], elements, t_null );
    std::uninitialized_fill_n( &frn[0][0], elements, f_null );
    std::uninitialized_fill_n( &lum[0][0], elements, 0 );
    std::uninitialized_fill_n( &trp[0][0], elements, tr_null );
    std::uninitialized_fill_n( &rad[0][0], elements, 0 );

    is_uniform = false;
}

submap::submap( submap && ) = default;
submap::~submap() = default;

submap &submap::operator=( submap && ) = default;

static const std::string COSMETICS_GRAFFITI( "GRAFFITI" );
static const std::string COSMETICS_SIGNAGE( "SIGNAGE" );
// Handle GCC warning: 'warning: returning reference to temporary'
static const std::string STRING_EMPTY;

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
    const std::vector<submap::cosmetic_t> &cosmetics, const point &p, const std::string &type )
{
    for( size_t i = 0; i < cosmetics.size(); ++i ) {
        if( cosmetics[i].pos == p && cosmetics[i].type == type ) {
            return make_result( true, i );
        }
    }
    return make_result( false, -1 );
}

bool submap::has_graffiti( const point &p ) const
{
    return find_cosmetic( cosmetics, p, COSMETICS_GRAFFITI ).result;
}

const std::string &submap::get_graffiti( const point &p ) const
{
    const auto fresult = find_cosmetic( cosmetics, p, COSMETICS_GRAFFITI );
    if( fresult.result ) {
        return cosmetics[ fresult.ndx ].str;
    }
    return STRING_EMPTY;
}

void submap::set_graffiti( const point &p, const std::string &new_graffiti )
{
    is_uniform = false;
    // Find signage at p if available
    const auto fresult = find_cosmetic( cosmetics, p, COSMETICS_GRAFFITI );
    if( fresult.result ) {
        cosmetics[ fresult.ndx ].str = new_graffiti;
    } else {
        insert_cosmetic( p, COSMETICS_GRAFFITI, new_graffiti );
    }
}

void submap::delete_graffiti( const point &p )
{
    is_uniform = false;
    const auto fresult = find_cosmetic( cosmetics, p, COSMETICS_GRAFFITI );
    if( fresult.result ) {
        cosmetics[ fresult.ndx ] = cosmetics.back();
        cosmetics.pop_back();
    }
}
bool submap::has_signage( const point &p ) const
{
    if( frn[p.x][p.y].obj().has_flag( "SIGN" ) ) {
        return find_cosmetic( cosmetics, p, COSMETICS_SIGNAGE ).result;
    }

    return false;
}
std::string submap::get_signage( const point &p ) const
{
    if( frn[p.x][p.y].obj().has_flag( "SIGN" ) ) {
        const auto fresult = find_cosmetic( cosmetics, p, COSMETICS_SIGNAGE );
        if( fresult.result ) {
            return cosmetics[ fresult.ndx ].str;
        }
    }

    return STRING_EMPTY;
}
void submap::set_signage( const point &p, const std::string &s )
{
    is_uniform = false;
    // Find signage at p if available
    const auto fresult = find_cosmetic( cosmetics, p, COSMETICS_SIGNAGE );
    if( fresult.result ) {
        cosmetics[ fresult.ndx ].str = s;
    } else {
        insert_cosmetic( p, COSMETICS_SIGNAGE, s );
    }
}
void submap::delete_signage( const point &p )
{
    is_uniform = false;
    const auto fresult = find_cosmetic( cosmetics, p, COSMETICS_SIGNAGE );
    if( fresult.result ) {
        cosmetics[ fresult.ndx ] = cosmetics.back();
        cosmetics.pop_back();
    }
}

void submap::update_legacy_computer()
{
    if( legacy_computer ) {
        for( int x = 0; x < SEEX; ++x ) {
            for( int y = 0; y < SEEY; ++y ) {
                if( ter[x][y] == t_console ) {
                    computers.emplace( point( x, y ), *legacy_computer );
                }
            }
        }
        legacy_computer.reset();
    }
}

bool submap::has_computer( const point &p ) const
{
    return computers.find( p ) != computers.end() || ( legacy_computer && ter[p.x][p.y] == t_console );
}

const computer *submap::get_computer( const point &p ) const
{
    // the returned object will not get modified (should not, at least), so we
    // don't yet need to update to std::map
    const auto it = computers.find( p );
    if( it != computers.end() ) {
        return &it->second;
    }
    if( legacy_computer && ter[p.x][p.y] == t_console ) {
        return legacy_computer.get();
    }
    return nullptr;
}

computer *submap::get_computer( const point &p )
{
    // need to update to std::map first so modifications to the returned object
    // only affects the exact point p
    update_legacy_computer();
    const auto it = computers.find( p );
    if( it != computers.end() ) {
        return &it->second;
    }
    return nullptr;
}

void submap::set_computer( const point &p, const computer &c )
{
    update_legacy_computer();
    const auto it = computers.find( p );
    if( it != computers.end() ) {
        it->second = c;
    } else {
        computers.emplace( p, c );
    }
}

void submap::delete_computer( const point &p )
{
    update_legacy_computer();
    computers.erase( p );
}

bool submap::contains_vehicle( vehicle *veh )
{
    const auto match = std::find_if(
                           begin( vehicles ), end( vehicles ),
    [veh]( const std::unique_ptr<vehicle> &v ) {
        return v.get() == veh;
    } );
    return match != vehicles.end();
}

void submap::rotate( int turns )
{
    turns = turns % 4;

    if( turns == 0 ) {
        return;
    }

    const auto rotate_point = [turns]( const point & p ) {
        return p.rotate( turns, { SEEX, SEEY } );
    };

    if( turns == 2 ) {
        // Swap horizontal stripes.
        for( int j = 0, je = SEEY / 2; j < je; ++j ) {
            for( int i = j, ie = SEEX - j; i < ie; ++i ) {
                swap_soa_tile( { i, j }, rotate_point( { i, j } ) );
            }
        }
        // Swap vertical stripes so that they don't overlap with
        // the already swapped horizontals.
        for( int i = 0, ie = SEEX / 2; i < ie; ++i ) {
            for( int j = i + 1, je = SEEY - i - 1; j < je; ++j ) {
                swap_soa_tile( { i, j }, rotate_point( { i, j } ) );
            }
        }
    } else {
        maptile_soa<1, 1> tmp;

        for( int j = 0, je = SEEY / 2; j < je; ++j ) {
            for( int i = j, ie = SEEX - j - 1; i < ie; ++i ) {
                auto p = point{ i, j };

                swap_soa_tile( p, tmp );

                for( int k = 0; k < 4; ++k ) {
                    p = rotate_point( p );
                    swap_soa_tile( p, tmp );
                }
            }
        }
    }

    active_items.rotate_locations( turns, { SEEX, SEEY } );

    for( auto &elem : cosmetics ) {
        elem.pos = rotate_point( elem.pos );
    }

    for( auto &elem : spawns ) {
        elem.pos = rotate_point( elem.pos );
    }

    for( auto &elem : vehicles ) {
        const auto new_pos = rotate_point( elem->pos );

        elem->pos = new_pos;
        // turn the steering wheel, vehicle::turn does not actually
        // move the vehicle.
        elem->turn( turns * 90 );
        // The facing direction and recalculate the positions of the parts
        elem->face = elem->turn_dir;
        elem->precalc_mounts( 0, elem->turn_dir, elem->pivot_anchor[0] );
    }

    std::map<point, computer> rot_comp;
    for( auto &elem : computers ) {
        rot_comp.emplace( rotate_point( elem.first ), elem.second );
    }
    computers = rot_comp;
}
