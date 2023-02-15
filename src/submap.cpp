#include "submap.h"

#include <algorithm>
#include <array>
#include <iterator>
#include <memory>
#include <utility>

#include "basecamp.h"
#include "mapdata.h"
#include "tileray.h"
#include "trap.h"
#include "units.h"
#include "vehicle.h"

static const furn_str_id furn_f_console( "f_console" );

void maptile_soa::swap_soa_tile( const point &p1, const point &p2 )
{
    std::swap( ter[p1.x][p1.y], ter[p2.x][p2.y] );
    std::swap( frn[p1.x][p1.y], frn[p2.x][p2.y] );
    std::swap( lum[p1.x][p1.y], lum[p2.x][p2.y] );
    std::swap( itm[p1.x][p1.y], itm[p2.x][p2.y] );
    std::swap( fld[p1.x][p1.y], fld[p2.x][p2.y] );
    std::swap( trp[p1.x][p1.y], trp[p2.x][p2.y] );
    std::swap( rad[p1.x][p1.y], rad[p2.x][p2.y] );
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

submap::submap( submap && ) noexcept( map_is_noexcept ) = default;
submap::~submap() = default;

submap &submap::operator=( submap && ) noexcept = default;

void submap::clear_fields( const point &p )
{
    field &f = get_field( p );
    field_count -= f.field_count();
    f.clear();
}

static const std::string COSMETICS_GRAFFITI( "GRAFFITI" );
static const std::string COSMETICS_SIGNAGE( "SIGNAGE" );
// Handle GCC warning: 'warning: returning reference to temporary'
static const std::string STRING_EMPTY;

struct cosmetic_find_result {
    bool result = false;
    int ndx = 0;
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
    const cosmetic_find_result fresult = find_cosmetic( cosmetics, p, COSMETICS_GRAFFITI );
    if( fresult.result ) {
        return cosmetics[ fresult.ndx ].str;
    }
    return STRING_EMPTY;
}

void submap::set_graffiti( const point &p, const std::string &new_graffiti )
{
    is_uniform = false;
    // Find signage at p if available
    const cosmetic_find_result fresult = find_cosmetic( cosmetics, p, COSMETICS_GRAFFITI );
    if( fresult.result ) {
        cosmetics[ fresult.ndx ].str = new_graffiti;
    } else {
        insert_cosmetic( p, COSMETICS_GRAFFITI, new_graffiti );
    }
}

void submap::delete_graffiti( const point &p )
{
    is_uniform = false;
    const cosmetic_find_result fresult = find_cosmetic( cosmetics, p, COSMETICS_GRAFFITI );
    if( fresult.result ) {
        cosmetics[ fresult.ndx ] = cosmetics.back();
        cosmetics.pop_back();
    }
}
bool submap::has_signage( const point &p ) const
{
    if( frn[p.x][p.y].obj().has_flag( ter_furn_flag::TFLAG_SIGN ) ) {
        return find_cosmetic( cosmetics, p, COSMETICS_SIGNAGE ).result;
    }

    return false;
}
std::string submap::get_signage( const point &p ) const
{
    if( frn[p.x][p.y].obj().has_flag( ter_furn_flag::TFLAG_SIGN ) ) {
        const cosmetic_find_result fresult = find_cosmetic( cosmetics, p, COSMETICS_SIGNAGE );
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
    const cosmetic_find_result fresult = find_cosmetic( cosmetics, p, COSMETICS_SIGNAGE );
    if( fresult.result ) {
        cosmetics[ fresult.ndx ].str = s;
    } else {
        insert_cosmetic( p, COSMETICS_SIGNAGE, s );
    }
}
void submap::delete_signage( const point &p )
{
    is_uniform = false;
    const cosmetic_find_result fresult = find_cosmetic( cosmetics, p, COSMETICS_SIGNAGE );
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
                if( frn[x][y] == furn_f_console ) {
                    computers.emplace( point( x, y ), *legacy_computer );
                }
            }
        }
        legacy_computer.reset();
    }
}

bool submap::has_computer( const point &p ) const
{
    return computers.find( p ) != computers.end() || ( legacy_computer && frn[p.x][p.y]
            == furn_f_console );
}

const computer *submap::get_computer( const point &p ) const
{
    // the returned object will not get modified (should not, at least), so we
    // don't yet need to update to std::map
    const auto it = computers.find( p );
    if( it != computers.end() ) {
        return &it->second;
    }
    if( legacy_computer && frn[p.x][p.y] == furn_f_console ) {
        return legacy_computer.get();
    }
    return nullptr;
}

computer *submap::get_computer( const point &p )
{
    // need to update to std::map first so modifications to the returned object
    // only affects the exact point p
    //update_legacy_computer();
    const auto it = computers.find( p );
    if( it != computers.end() ) {
        return &it->second;
    }
    return nullptr;
}

void submap::set_computer( const point &p, const computer &c )
{
    //update_legacy_computer();
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

bool submap::is_open_air( const point &p ) const
{
    ter_id t = get_ter( p );
    return t->trap == tr_ledge;
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
    const auto rotate_point_ccw = [turns]( const point & p ) {
        return p.rotate( 4 - turns, { SEEX, SEEY } );
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
        for( int j = 0, je = SEEY / 2; j < je; ++j ) {
            for( int i = j, ie = SEEX - j - 1; i < ie; ++i ) {
                point p = point{ i, j };
                point pp = p;
                // three swaps are enough to perform the circular shift of four elements:
                // 0123 -> 3120 -> 3102 -> 3012
                for( int k = 0; k < 3; ++k ) {
                    p = pp;
                    pp = rotate_point_ccw( pp );
                    swap_soa_tile( p, pp );
                }
            }
        }
    }

    active_items.rotate_locations( turns, { SEEX, SEEY } );

    for( submap::cosmetic_t &elem : cosmetics ) {
        elem.pos = rotate_point( elem.pos );
    }

    for( spawn_point &elem : spawns ) {
        elem.pos = rotate_point( elem.pos );
    }

    for( auto &elem : vehicles ) {
        const point new_pos = rotate_point( elem->pos );

        elem->pos = new_pos;
        // turn the steering wheel, vehicle::turn does not actually
        // move the vehicle.
        elem->turn( turns * 90_degrees );
        // The facing direction and recalculate the positions of the parts
        elem->face = tileray( elem->turn_dir );
        elem->precalc_mounts( 0, elem->turn_dir, elem->pivot_anchor[0] );
    }

    std::map<point, computer> rot_comp;
    for( auto &elem : computers ) {
        rot_comp.emplace( rotate_point( elem.first ), elem.second );
    }
    computers = rot_comp;
}

void submap::mirror( bool horizontally )
{
    std::map<point, computer> mirror_comp;

    if( horizontally ) {
        for( int i = 0, ie = SEEX / 2; i < ie; i++ ) {
            for( int k = 0; k < SEEY; k++ ) {
                swap_soa_tile( { i, k }, { SEEX - 1 - i, k } );
            }
        }

        for( submap::cosmetic_t &elem : cosmetics ) {
            elem.pos = point( -elem.pos.x, elem.pos.y ) + point( SEEX - 1, 0 );
        }

        active_items.mirror( { SEEX, SEEY }, true );

        for( auto &elem : computers ) {
            mirror_comp.emplace( point( -elem.first.x, elem.first.y ) + point( SEEX - 1, 0 ), elem.second );
        }
        computers = mirror_comp;
    } else {
        for( int k = 0, ke = SEEY / 2; k < ke; k++ ) {
            for( int i = 0; i < SEEX; i++ ) {
                swap_soa_tile( { i, k }, { i, SEEY - 1 - k } );
            }
        }

        for( submap::cosmetic_t &elem : cosmetics ) {
            elem.pos = point( elem.pos.x, -elem.pos.y ) + point( 0, SEEY - 1 );
        }

        active_items.mirror( { SEEX, SEEY }, false );

        for( auto &elem : computers ) {
            mirror_comp.emplace( point( elem.first.x, -elem.first.y ) + point( 0, SEEY - 1 ), elem.second );
        }
        computers = mirror_comp;
    }
}

void submap::revert_submap( submap_revert &sr )
{
    for( int x = 0; x < SEEX; x++ ) {
        for( int y = 0; y < SEEY; y++ ) {
            point pt( x, y );
            frn[x][y] = sr.get_furn( pt );
            ter[x][y] = sr.get_ter( pt );
            trp[x][y] = sr.get_trap( pt );
            itm[x][y] = sr.get_items( pt );
            for( item &itm : itm[x][y] ) {
                if( itm.is_emissive() ) {
                    this->update_lum_add( pt, itm );
                }
                if( itm.needs_processing() ) {
                    active_items.add( itm, pt );
                }
            }
        }
    }
}

submap_revert submap::get_revert_submap() const
{
    submap_revert ret;
    for( int x = 0; x < SEEX; x++ ) {
        for( int y = 0; y < SEEY; y++ ) {
            point pt( x, y );
            ret.set_furn( pt, frn[x][y] );
            ret.set_ter( pt, ter[x][y] );
            ret.set_trap( pt, trp[x][y] );
            ret.set_items( pt, itm[x][y] );
        }
    }
    return ret;
}

void submap::update_lum_rem( const point &p, const item &i )
{
    is_uniform = false;
    if( !i.is_emissive() ) {
        return;
    } else if( lum[p.x][p.y] && lum[p.x][p.y] < 255 ) {
        lum[p.x][p.y]--;
        return;
    }

    // Have to scan through all items to be sure removing i will actually lower
    // the count below 255.
    int count = 0;
    for( const item &it : itm[p.x][p.y] ) {
        if( it.is_emissive() ) {
            count++;
        }
    }

    if( count <= 256 ) {
        lum[p.x][p.y] = static_cast<uint8_t>( count - 1 );
    }
}
