#include "scent_map.h"

#include <algorithm>
#include <cstdlib>

#include "assign.h"
#include "calendar.h"
#include "cata_assert.h"
#include "color.h"
#include "cuboid_rectangle.h"
#include "cursesdef.h"
#include "debug.h"
#include "generic_factory.h"
#include "map.h"
#include "output.h"
#include "point.h"

static constexpr int SCENT_RADIUS = 40;

static nc_color sev( const size_t level )
{
    static const std::array<nc_color, 22> colors = { {
            c_cyan,
            c_light_cyan,
            c_light_blue,
            c_blue,
            c_light_green,
            c_green,
            c_yellow,
            c_pink,
            c_light_red,
            c_red,
            c_magenta,
            c_brown,
            c_cyan_red,
            c_light_cyan_red,
            c_light_blue_red,
            c_blue_red,
            c_light_green_red,
            c_green_red,
            c_yellow_red,
            c_pink_red,
            c_magenta_red,
            c_brown_red,
        }
    };
    return level < colors.size() ? colors[level] : c_dark_gray;
}

void scent_map::reset()
{
    for( auto &elem : grscent ) {
        for( int &val : elem ) {
            val = 0;
        }
    }
    typescent = scenttype_id();
}

void scent_map::decay()
{
    for( auto &elem : grscent ) {
        for( int &val : elem ) {
            val = std::max( 0, val - 1 );
        }
    }
}

void scent_map::draw( const catacurses::window &win, const int div,
                      const tripoint_bub_ms &center ) const
{
    cata_assert( div != 0 );
    const point max( getmaxx( win ), getmaxy( win ) );
    for( int x = 0; x < max.x; ++x ) {
        for( int y = 0; y < max.y; ++y ) {
            const int sn = get( center + point( -max.x / 2 + x, -max.y / 2 + y ) ) / div;
            mvwprintz( win, point( x, y ), sev( sn / 10 ), "%d", sn % 10 );
        }
    }
}

void scent_map::shift( const point_rel_ms &sm_shift )
{
    // Set up iteration such that we ensure data has been moved before
    // it's overwritten.
    const int x_start = sm_shift.x() >= 0 ? 0 : MAPSIZE_X - 1;
    const int x_stop = sm_shift.x() >= 0 ? MAPSIZE_X : -1;
    const int x_step = sm_shift.x() >= 0 ? 1 : -1;
    const int y_start = sm_shift.y() >= 0 ? 0 : MAPSIZE_Y - 1;
    const int y_stop = sm_shift.y() >= 0 ? MAPSIZE_Y : -1;
    const int y_step = sm_shift.y() >= 0 ? 1 : -1;

    for( int x = x_start; x != x_stop; x += x_step ) {
        for( int y = y_start; y != y_stop; y += y_step ) {
            const point_bub_ms p = point_bub_ms( x, y ) + sm_shift;
            grscent[x][y] = inbounds( p ) ? grscent[p.x()][p.y()] : 0;
        }
    }
}

int scent_map::get( const tripoint_bub_ms &p ) const
{
    if( inbounds( p ) && grscent[p.x()][p.y()] > 0 ) {
        return get_unsafe( p );
    }
    return 0;
}

void scent_map::set( const tripoint_bub_ms &p, int value, const scenttype_id &type )
{
    if( inbounds( p ) ) {
        set_unsafe( p, value, type );
    }
}

void scent_map::set_unsafe( const tripoint_bub_ms &p, int value, const scenttype_id &type )
{
    grscent[p.x()][p.y()] = value;
    if( !type.is_empty() ) {
        typescent = type;
    }
}
int scent_map::get_unsafe( const tripoint_bub_ms &p ) const
{
    return grscent[p.x()][p.y()] - std::abs( get_map().get_abs_sub().z() - p.z() );
}

scenttype_id scent_map::get_type() const
{
    return typescent;
}

scenttype_id scent_map::get_type( const tripoint_bub_ms &p ) const
{
    scenttype_id id;
    if( inbounds( p ) && grscent[p.x()][p.y()] > 0 ) {
        id = typescent;
    }
    return id;
}

bool scent_map::inbounds( const tripoint_bub_ms &p ) const
{
    map &here = get_map();

    // HACK: This weird long check here is a hack around the fact that scentmap is 2D
    // A z-level can access scentmap if it is within SCENT_MAP_Z_REACH flying z-level move from player's z-level
    // That is, if a flying critter could move directly up or down (or stand still) and be on same z-level as player
    const int levz = here.get_abs_sub().z();
    const bool scent_map_z_level_inbounds = ( p.z() == levz ) ||
                                            ( std::abs( p.z() - levz ) == SCENT_MAP_Z_REACH &&
                                                    here.valid_move( p, tripoint_bub_ms( p.xy(), levz ), false, true ) );
    if( !scent_map_z_level_inbounds ) {
        return false;
    }
    return inbounds( p.xy() );
}

bool scent_map::inbounds( const point_bub_ms &p ) const
{
    static constexpr point_bub_ms scent_map_boundary_min{};
    static constexpr point_bub_ms scent_map_boundary_max( MAPSIZE_X, MAPSIZE_Y );

    static constexpr half_open_rectangle<point_bub_ms> scent_map_boundaries(
        scent_map_boundary_min, scent_map_boundary_max );

    return scent_map_boundaries.contains( p );
}

void scent_map::update( const tripoint_bub_ms &center, map &m )
{
    // Stop updating scent after X turns of the player not moving.
    // Once wind is added, need to reset this on wind shifts as well.
    if( !player_last_position || center != *player_last_position ) {
        player_last_position = center;
        player_last_moved = calendar::turn;
    } else if( player_last_moved + 1000_turns < calendar::turn ) {
        return;
    }

    // note: the next four intermediate matrices need to be at least
    // [2*SCENT_RADIUS+3][2*SCENT_RADIUS+1] in size to hold enough data
    // The code I'm modifying used [MAPSIZE_X]. I'm staying with that to avoid new bugs.

    // These two matrices are transposed so that x addresses are contiguous in memory
    scent_array<int> sum_3_scent_y;
    scent_array<int> squares_used_y;

    // these are for caching flag lookups
    scent_array<bool> blocks_scent; // currently only ter_furn_flag::TFLAG_NO_SCENT blocks scent
    scent_array<bool> reduces_scent;

    // for loop constants
    const int scentmap_minx = center.x() - SCENT_RADIUS;
    const int scentmap_maxx = center.x() + SCENT_RADIUS;
    const int scentmap_miny = center.y() - SCENT_RADIUS;
    const int scentmap_maxy = center.y() + SCENT_RADIUS;

    // decrease this to reduce gas spread. Keep it under 125 for
    // stability. This is essentially a decimal number * 1000.
    const int diffusivity = 100;

    // The new scent flag searching function. Should be wayyy faster than the old one.
    m.scent_blockers( blocks_scent, reduces_scent, point_bub_ms( scentmap_minx - 1, scentmap_miny - 1 ),
                      point_bub_ms( scentmap_maxx + 1, scentmap_maxy + 1 ) );
    // Sum neighbors in the y direction.  This way, each square gets called 3 times instead of 9
    // times. This cost us an extra loop here, but it also eliminated a loop at the end, so there
    // is a net performance improvement over the old code. Could probably still be better.
    // note: this method needs an array that is one square larger on each side in the x direction
    // than the final scent matrix. I think this is fine since SCENT_RADIUS is less than
    // MAPSIZE_X, but if that changes, this may need tweaking.
    for( int x = scentmap_minx - 1; x <= scentmap_maxx + 1; ++x ) {
        for( int y = scentmap_miny; y <= scentmap_maxy; ++y ) {
            // remember the sum of the scent val for the 3 neighboring squares that can defuse into
            sum_3_scent_y[y][x] = 0;
            squares_used_y[y][x] = 0;
            for( int i = y - 1; i <= y + 1; ++i ) {
                if( !blocks_scent[x][i] ) {
                    if( reduces_scent[x][i] ) {
                        // only 20% of scent can diffuse on REDUCE_SCENT squares
                        sum_3_scent_y[y][x] += 2 * grscent[x][i];
                        squares_used_y[y][x] += 2;
                    } else {
                        sum_3_scent_y[y][x] += 10 * grscent[x][i];
                        squares_used_y[y][x] += 10;
                    }
                }
            }
        }
    }

    // Rest of the scent map
    for( int x = scentmap_minx; x <= scentmap_maxx; ++x ) {
        for( int y = scentmap_miny; y <= scentmap_maxy; ++y ) {
            int &scent_here = grscent[x][y];
            if( !blocks_scent[x][y] ) {
                // to how many neighboring squares do we diffuse out? (include our own square
                // since we also include our own square when diffusing in)
                const int squares_used = squares_used_y[y][x - 1]
                                         + squares_used_y[y][x]
                                         + squares_used_y[y][x + 1];

                int this_diffusivity;
                if( !reduces_scent[x][y] ) {
                    this_diffusivity = diffusivity;
                } else {
                    this_diffusivity = diffusivity / 5; //less air movement for REDUCE_SCENT square
                }
                // take the old scent and subtract what diffuses out
                int temp_scent = scent_here * ( 10 * 1000 - squares_used * this_diffusivity );
                // neighboring REDUCE_SCENT squares absorb some scent
                temp_scent -= scent_here * this_diffusivity * ( 90 - squares_used ) / 5;
                // we've already summed neighboring scent values in the y direction in the previous
                // loop. Now we do it for the x direction, multiply by diffusion, and this is what
                // diffuses into our current square.
                scent_here =
                    ( temp_scent
                      + this_diffusivity * ( sum_3_scent_y[y][x - 1]
                                             + sum_3_scent_y[y][x]
                                             + sum_3_scent_y[y][x + 1] )
                    ) / ( 1000 * 10 );
            } else {
                // this cell blocks scent via NO_SCENT (in json)
                scent_here = 0;
            }
        }
    }
}

namespace
{
generic_factory<scent_type> scent_factory( "scent_type" );
} // namespace

template<>
const scent_type &string_id<scent_type>::obj() const
{
    return scent_factory.obj( *this );
}

template<>
bool string_id<scent_type>::is_valid() const
{
    return scent_factory.is_valid( *this );
}

void scent_type::load_scent_type( const JsonObject &jo, const std::string &src )
{
    scent_factory.load( jo, src );
}

void scent_type::load( const JsonObject &jo, std::string_view )
{
    assign( jo, "id", id );
    assign( jo, "receptive_species", receptive_species );
}

const std::vector<scent_type> &scent_type::get_all()
{
    return scent_factory.get_all();
}

void scent_type::check_scent_consistency()
{
    for( const scent_type &styp : get_all() ) {
        for( const species_id &spe : styp.receptive_species ) {
            if( !spe.is_valid() ) {
                debugmsg( "scent_type %s has invalid species_id %s in receptive_species", styp.id.c_str(),
                          spe.c_str() );
            }
        }
    }
}

void scent_type::reset()
{
    scent_factory.reset();
}
