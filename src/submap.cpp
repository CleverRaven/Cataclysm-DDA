#include "submap.h"

#include <algorithm>
#include <array>
#include <iterator>
#include <memory>
#include <utility>

#include "basecamp.h"
#include "debug.h"
#include "mapdata.h"
#include "tileray.h"
#include "trap.h"
#include "units.h"
#include "vehicle.h"

static furn_id f_null;

static const furn_str_id furn_f_console( "f_console" );

void maptile_soa::swap_soa_tile( const point_sm_ms &p1, const point_sm_ms &p2 )
{
    std::swap( ter[p1.x()][p1.y()], ter[p2.x()][p2.y()] );
    std::swap( frn[p1.x()][p1.y()], frn[p2.x()][p2.y()] );
    std::swap( lum[p1.x()][p1.y()], lum[p2.x()][p2.y()] );
    std::swap( itm[p1.x()][p1.y()], itm[p2.x()][p2.y()] );
    std::swap( fld[p1.x()][p1.y()], fld[p2.x()][p2.y()] );
    std::swap( trp[p1.x()][p1.y()], trp[p2.x()][p2.y()] );
    std::swap( rad[p1.x()][p1.y()], rad[p2.x()][p2.y()] );
}

submap::submap( submap && ) noexcept( map_is_noexcept ) = default;
submap::~submap() = default;

submap &submap::operator=( submap && ) noexcept = default;

void submap::clear_fields( const point_sm_ms &p )
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
    const std::vector<submap::cosmetic_t> &cosmetics, const point_sm_ms &p, const std::string &type )
{
    for( size_t i = 0; i < cosmetics.size(); ++i ) {
        if( cosmetics[i].pos == p && cosmetics[i].type == type ) {
            return make_result( true, i );
        }
    }
    return make_result( false, -1 );
}

bool submap::has_graffiti( const point_sm_ms &p ) const
{
    return find_cosmetic( cosmetics, p, COSMETICS_GRAFFITI ).result;
}

const std::string &submap::get_graffiti( const point_sm_ms &p ) const
{
    const cosmetic_find_result fresult = find_cosmetic( cosmetics, p, COSMETICS_GRAFFITI );
    if( fresult.result ) {
        return cosmetics[ fresult.ndx ].str;
    }
    return STRING_EMPTY;
}

void submap::set_graffiti( const point_sm_ms &p, const std::string &new_graffiti )
{
    ensure_nonuniform();
    // Find signage at p if available
    const cosmetic_find_result fresult = find_cosmetic( cosmetics, p, COSMETICS_GRAFFITI );
    if( fresult.result ) {
        cosmetics[ fresult.ndx ].str = new_graffiti;
    } else {
        insert_cosmetic( p, COSMETICS_GRAFFITI, new_graffiti );
    }
}

void submap::delete_graffiti( const point_sm_ms &p )
{
    const cosmetic_find_result fresult = find_cosmetic( cosmetics, p, COSMETICS_GRAFFITI );
    if( fresult.result ) {
        ensure_nonuniform();
        cosmetics[ fresult.ndx ] = cosmetics.back();
        cosmetics.pop_back();
    }
}
bool submap::has_signage( const point_sm_ms &p ) const
{
    if( !is_uniform() && m->frn[p.x()][p.y()].obj().has_flag( ter_furn_flag::TFLAG_SIGN ) ) {
        return find_cosmetic( cosmetics, p, COSMETICS_SIGNAGE ).result;
    }

    return false;
}
std::string submap::get_signage( const point_sm_ms &p ) const
{
    if( !is_uniform() && m->frn[p.x()][p.y()].obj().has_flag( ter_furn_flag::TFLAG_SIGN ) ) {
        const cosmetic_find_result fresult = find_cosmetic( cosmetics, p, COSMETICS_SIGNAGE );
        if( fresult.result ) {
            return cosmetics[ fresult.ndx ].str;
        }
    }

    return STRING_EMPTY;
}
void submap::set_signage( const point_sm_ms &p, const std::string &s )
{
    ensure_nonuniform();
    // Find signage at p if available
    const cosmetic_find_result fresult = find_cosmetic( cosmetics, p, COSMETICS_SIGNAGE );
    if( fresult.result ) {
        cosmetics[ fresult.ndx ].str = s;
    } else {
        insert_cosmetic( p, COSMETICS_SIGNAGE, s );
    }
}
void submap::delete_signage( const point_sm_ms &p )
{
    const cosmetic_find_result fresult = find_cosmetic( cosmetics, p, COSMETICS_SIGNAGE );
    if( fresult.result ) {
        ensure_nonuniform();
        cosmetics[ fresult.ndx ] = cosmetics.back();
        cosmetics.pop_back();
    }
}

bool submap::has_computer( const point_sm_ms &p ) const
{
    return !is_uniform() && computers.find( p ) != computers.end();
}

const computer *submap::get_computer( const point_sm_ms &p ) const
{
    // the returned object will not get modified (should not, at least), so we
    // don't yet need to update to std::map
    const auto it = computers.find( p );
    if( it != computers.end() ) {
        return &it->second;
    }
    return nullptr;
}

computer *submap::get_computer( const point_sm_ms &p )
{
    const auto it = computers.find( p );
    if( it != computers.end() ) {
        return &it->second;
    }
    return nullptr;
}

void submap::set_computer( const point_sm_ms &p, const computer &c )
{
    const auto it = computers.find( p );
    if( it != computers.end() ) {
        it->second = c;
    } else {
        computers.emplace( p, c );
    }
}

void submap::delete_computer( const point_sm_ms &p )
{
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

bool submap::is_open_air( const point_sm_ms &p ) const
{
    return get_ter( p ).obj().has_flag( ter_furn_flag::TFLAG_NO_FLOOR );
}

void submap::rotate( int turns )
{
    if( is_uniform() ) {
        return;
    }
    turns = turns % 4;

    if( turns == 0 ) {
        return;
    }

    const auto rotate_point = [turns]( const point_sm_ms & p ) {
        return p.rotate( turns, { SEEX, SEEY } );
    };
    const auto rotate_point_ccw = [turns]( const point_sm_ms & p ) {
        return p.rotate( 4 - turns, { SEEX, SEEY } );
    };

    if( turns == 2 ) {
        // Swap horizontal stripes.
        for( int j = 0, je = SEEY / 2; j < je; ++j ) {
            for( int i = j, ie = SEEX - j; i < ie; ++i ) {
                m->swap_soa_tile( { i, j }, rotate_point( { i, j } ) );
            }
        }
        // Swap vertical stripes so that they don't overlap with
        // the already swapped horizontals.
        for( int i = 0, ie = SEEX / 2; i < ie; ++i ) {
            for( int j = i + 1, je = SEEY - i - 1; j < je; ++j ) {
                m->swap_soa_tile( { i, j }, rotate_point( { i, j } ) );
            }
        }
    } else {
        for( int j = 0, je = SEEY / 2; j < je; ++j ) {
            for( int i = j, ie = SEEX - j - 1; i < ie; ++i ) {
                point_sm_ms p = point_sm_ms{ i, j };
                point_sm_ms pp = p;
                // three swaps are enough to perform the circular shift of four elements:
                // 0123 -> 3120 -> 3102 -> 3012
                for( int k = 0; k < 3; ++k ) {
                    p = pp;
                    pp = rotate_point_ccw( pp );
                    m->swap_soa_tile( p, pp );
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
        const point_sm_ms new_pos = rotate_point( elem->pos );

        elem->pos = new_pos;
        // turn the steering wheel, vehicle::turn does not actually
        // move the vehicle.
        elem->turn( turns * 90_degrees );
        // The facing direction and recalculate the positions of the parts
        elem->face = tileray( elem->turn_dir );
        elem->precalc_mounts( 0, elem->turn_dir, elem->pivot_anchor[0] );
    }

    std::map<point_sm_ms, computer> rot_comp;
    for( auto &elem : computers ) {
        rot_comp.emplace( rotate_point( elem.first ), elem.second );
    }
    computers = rot_comp;
}

void submap::mirror( bool horizontally )
{
    if( is_uniform() ) {
        return;
    }
    std::map<point_sm_ms, computer> mirror_comp;

    if( horizontally ) {
        for( int i = 0, ie = SEEX / 2; i < ie; i++ ) {
            for( int k = 0; k < SEEY; k++ ) {
                m->swap_soa_tile( { i, k }, { SEEX - 1 - i, k } );
            }
        }

        for( submap::cosmetic_t &elem : cosmetics ) {
            elem.pos = point_sm_ms( -elem.pos.x(), elem.pos.y() ) + point( SEEX - 1, 0 );
        }

        active_items.mirror( { SEEX, SEEY }, true );

        for( auto &elem : computers ) {
            mirror_comp.emplace( point( -elem.first.x(), elem.first.y() ) + point( SEEX - 1, 0 ), elem.second );
        }
        computers = mirror_comp;
    } else {
        for( int k = 0, ke = SEEY / 2; k < ke; k++ ) {
            for( int i = 0; i < SEEX; i++ ) {
                m->swap_soa_tile( { i, k }, { i, SEEY - 1 - k } );
            }
        }

        for( submap::cosmetic_t &elem : cosmetics ) {
            elem.pos = point_sm_ms( elem.pos.x(), -elem.pos.y() ) + point( 0, SEEY - 1 );
        }

        active_items.mirror( { SEEX, SEEY }, false );

        for( auto &elem : computers ) {
            mirror_comp.emplace( point( elem.first.x(), -elem.first.y() ) + point( 0, SEEY - 1 ), elem.second );
        }
        computers = mirror_comp;
    }
}

void submap::revert_submap( submap &sr )
{
    reverted = true;
    if( sr.is_uniform() ) {
        m.reset();
        set_all_ter( sr.get_ter( point_sm_ms::zero ), true );
        return;
    }

    ensure_nonuniform();
    for( int x = 0; x < SEEX; x++ ) {
        for( int y = 0; y < SEEY; y++ ) {
            point_sm_ms pt( x, y );
            m->frn[x][y] = sr.get_furn( pt );
            m->ter[x][y] = sr.get_ter( pt );
            m->trp[x][y] = sr.get_trap( pt );
            m->itm[x][y] = sr.get_items( pt );
            for( item &itm : m->itm[x][y] ) {
                if( itm.is_emissive() ) {
                    this->update_lum_add( pt, itm );
                }
                active_items.add( itm, point_sm_ms( pt ) );
            }
        }
    }
}

submap submap::get_revert_submap() const
{
    submap ret;
    ret.uniform_ter = uniform_ter;
    if( !is_uniform() ) {
        ret.m = std::make_unique<maptile_soa>( *m );
    }

    return ret;
}

void submap::update_lum_rem( const point_sm_ms &p, const item &i )
{
    ensure_nonuniform();
    if( !i.is_emissive() ) {
        return;
    } else if( m->lum[p.x()][p.y()] && m->lum[p.x()][p.y()] < 255 ) {
        m->lum[p.x()][p.y()]--;
        return;
    }

    // Have to scan through all items to be sure removing i will actually lower
    // the count below 255.
    int count = 0;
    for( const item &it : m->itm[p.x()][p.y()] ) {
        if( it.is_emissive() ) {
            count++;
        }
    }

    if( count <= 256 ) {
        m->lum[p.x()][p.y()] = static_cast<uint8_t>( count - 1 );
    }
}

void submap::merge_submaps( submap *copy_from, bool copy_from_is_overlay )
{
    this->field_count = 0;

    for( int x = 0; x < SEEX; x++ ) {
        for( int y = 0; y < SEEY; y++ ) {
            if( copy_from->m->ter[x][y] != t_null && ( copy_from_is_overlay ||
                    this->m->ter[x][y] == t_null ) ) {
                this->m->ter[x][y] = copy_from->m->ter[x][y];
                this->set_map_damage( { x, y }, copy_from->get_map_damage( { x, y } ) );
            }

            if( copy_from->m->frn[x][y] != f_null && ( copy_from_is_overlay ||
                    this->m->frn[x][y] == f_null ) ) {
                this->m->frn[x][y] = copy_from->m->frn[x][y];
            }

            this->m->lum[x][y] += copy_from->m->lum[x][y];

            for( const item &itm : copy_from->m->itm[x][y] ) {
                this->m->itm[x][y].emplace( itm );
            }

            for( std::map<field_type_id, field_entry>::iterator it = copy_from->m->fld[x][y].begin();
                 it != copy_from->m->fld[x][y].end(); it++ ) {
                if( !this->m->fld[x][y].find_field( it->first, false ) ) {
                    this->m->fld[x][y].add_field( it->first, it->second.get_field_intensity(),
                                                  it->second.get_field_age() );
                } else if( copy_from_is_overlay ) { // Modify the field to match
                    field_entry *fld = this->m->fld[x][y].find_field( it->first, false );
                    fld->set_field_intensity( it->second.get_field_intensity() );
                    fld->set_field_age( it->second.get_field_age() );
                }
            }

            for( std::map<field_type_id, field_entry>::iterator it = this->m->fld[x][y].begin();
                 it != this->m->fld[x][y].end(); it++ ) {
                this->field_count++;
            }

            if( copy_from->m->trp[x][y] != tr_null && ( copy_from_is_overlay ||
                    this->m->trp[x][y] == tr_null ) ) {
                this->m->trp[x][y] = copy_from->m->trp[x][y];
            }

            if( copy_from->m->rad[x][y] > 0 && ( copy_from_is_overlay || this->m->rad[x][y] == 0 ) ) {
                this->m->rad[x][y] = copy_from->m->rad[x][y];
            }
        }
    }

    for( const submap::cosmetic_t &cos : copy_from->cosmetics ) {
        bool found = false;

        for( submap::cosmetic_t &cosmetic : this->cosmetics ) {
            if( cosmetic.pos == cos.pos && cosmetic.type == cos.type ) {
                if( copy_from_is_overlay ) {
                    cosmetic.str = cos.str;
                }
                found = true;
                break;
            }
        }

        if( !found ) {
            this->insert_cosmetic( cos.pos, cos.type, cos.str );
        }
    }

    // TODO: Copy the active item cache
    if( !copy_from->active_items.empty() ) {
        debugmsg( "Active items found on copied submap which is not supported." );
    }

    if( copy_from->last_touched > this->last_touched ) {
        this->last_touched = copy_from->last_touched;
    }

    for( const spawn_point &spawn : copy_from->spawns ) {
        this->spawns.emplace_back( spawn );
    }

    for( const auto &vehicle : copy_from->vehicles ) {
        this->vehicles.emplace_back( vehicle.get() );
    }
    // Can't let that submap delete the vehicles when it's destroyed
    copy_from->vehicles.clear();

    if( !copy_from->partial_constructions.empty() ) {
        debugmsg( "Partial constructions found on copied submap when none are expected." );
    }

    if( copy_from->camp ) {
        debugmsg( "Camp found on copied submap when none is expected." );
    }

    for( const std::pair<const point_sm_ms, computer>  &comp : copy_from->computers ) {
        if( this->m->frn[comp.first.x()][comp.first.y()] == furn_f_console &&
            !this->get_computer( comp.first ) ) {
            this->set_computer( comp.first, comp.second );
        }
    }

    if( copy_from->temperature_mod != 0 && this->temperature_mod == 0 ) {
        this->temperature_mod = copy_from->temperature_mod;
    }
}
