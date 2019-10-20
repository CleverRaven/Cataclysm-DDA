#include "scent_map.h"

#include <cstdlib>
#include <cassert>
#include <algorithm>

#include "calendar.h"
#include "color.h"
#include "game.h"
#include "map.h"
#include "output.h"
#include "cursesdef.h"

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
        for( auto &val : elem ) {
            val = 0;
        }
    }
    for( auto &elem : typescent ) {
        for( std::string &val : elem ) {
            val.clear();
        }
    }
}

void scent_map::decay()
{
    int x = 0;
    for( auto &elem : grscent ) {
        int y = 0;
        for( auto &val : elem ) {
            val = std::max( 0, val - 1 );
            if( val == 0 ) {
                typescent[x][y].clear();
            }
            y ++;
        }
        x++;
    }
}

void scent_map::draw( const catacurses::window &win, const int div, const tripoint &center ) const
{
    assert( div != 0 );
    const int maxx = getmaxx( win );
    const int maxy = getmaxy( win );
    for( int x = 0; x < maxx; ++x ) {
        for( int y = 0; y < maxy; ++y ) {
            const int sn = get( center + point( -maxx / 2 + x, -maxy / 2 + y ) ) / div;
            mvwprintz( win, point( x, y ), sev( sn / 10 ), "%d", sn % 10 );
        }
    }
}

void scent_map::shift( const int sm_shift_x, const int sm_shift_y )
{
    scent_array<int> new_scent;
    scent_array<std::string> new_scent_type;
    for( size_t x = 0; x < MAPSIZE_X; ++x ) {
        for( size_t y = 0; y < MAPSIZE_Y; ++y ) {
            const point p( x + sm_shift_x, y + sm_shift_y );
            new_scent[x][y] = inbounds( p ) ? grscent[ p.x ][ p.y ] : 0;
            new_scent_type[x][y] = inbounds( p ) ? typescent[p.x][p.y] : "";
        }
    }
    grscent = new_scent;
    typescent = new_scent_type;
}

int scent_map::get( const tripoint &p ) const
{
    if( inbounds( p ) && grscent[p.x][p.y] > 0 ) {
        return get_unsafe( p );
    }
    return 0;
}

void scent_map::set( const tripoint &p, int value, std::string type )
{
    if( inbounds( p ) ) {
        set_unsafe( p, value, type );
    }
}

void scent_map::set_unsafe( const tripoint &p, int value, std::string type )
{
    grscent[p.x][p.y] = value;
    if( !type.empty() ) {
        typescent[p.x][p.y] = type;
    }
}
int scent_map::get_unsafe( const tripoint &p ) const
{
    return grscent[p.x][p.y] - std::abs( gm.get_levz() - p.z );
}

std::string scent_map::get_type( const tripoint &p ) const
{
    std::string scent_type;
    if( inbounds( p ) && grscent[p.x][p.y] > 0 ) {
        scent_type = get_type_unsafe( p );
    }
    return scent_type;
}

std::string scent_map::get_type_unsafe( const tripoint &p ) const
{
    return typescent[p.x][p.y];
}

bool scent_map::inbounds( const tripoint &p ) const
{
    // This weird long check here is a hack around the fact that scentmap is 2D
    // A z-level can access scentmap if it is within SCENT_MAP_Z_REACH flying z-level move from player's z-level
    // That is, if a flying critter could move directly up or down (or stand still) and be on same z-level as player
    const int levz = gm.get_levz();
    const bool scent_map_z_level_inbounds = ( p.z == levz ) ||
                                            ( std::abs( p.z - levz ) == SCENT_MAP_Z_REACH &&
                                                    gm.m.valid_move( p, tripoint( p.xy(), levz ), false, true ) );
    if( !scent_map_z_level_inbounds ) {
        return false;
    }
    static constexpr point scent_map_boundary_min( point_zero );
    static constexpr point scent_map_boundary_max( MAPSIZE_X, MAPSIZE_Y );

    static constexpr rectangle scent_map_boundaries(
        scent_map_boundary_min, scent_map_boundary_max );

    return scent_map_boundaries.contains_half_open( p.xy() );
}

void scent_map::update( const tripoint &center, map &m )
{
    // Stop updating scent after X turns of the player not moving.
    // Once wind is added, need to reset this on wind shifts as well.
    if( !player_last_position || center != *player_last_position ) {
        player_last_position.emplace( center );
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

    scent_array<std::string> sum_3_type_y;

    // these are for caching flag lookups
    scent_array<bool> blocks_scent; // currently only TFLAG_WALL blocks scent
    scent_array<bool> reduces_scent;

    // for loop constants
    const int scentmap_minx = center.x - SCENT_RADIUS;
    const int scentmap_maxx = center.x + SCENT_RADIUS;
    const int scentmap_miny = center.y - SCENT_RADIUS;
    const int scentmap_maxy = center.y + SCENT_RADIUS;

    // decrease this to reduce gas spread. Keep it under 125 for
    // stability. This is essentially a decimal number * 1000.
    const int diffusivity = 100;

    // The new scent flag searching function. Should be wayyy faster than the old one.
    m.scent_blockers( blocks_scent, reduces_scent, point( scentmap_minx - 1, scentmap_miny - 1 ),
                      point( scentmap_maxx + 1, scentmap_maxy + 1 ) );
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
                    if( !typescent[x][i].empty() ) {
                        sum_3_type_y[y][x] = typescent[x][i];
                    }
                }
            }
        }
    }

    // Rest of the scent map
    for( int x = scentmap_minx; x <= scentmap_maxx; ++x ) {
        for( int y = scentmap_miny; y <= scentmap_maxy; ++y ) {
            int &scent_here = grscent[x][y];
            std::string &type_here = typescent[x][y];
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
                // neighboring walls and reduce_scent squares absorb some scent
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
                for( int ki = x - 1; ki < x + 2; ki++ ) {
                    if( !sum_3_type_y[y][ki].empty() ) {
                        type_here = sum_3_type_y[y][ki];
                    }
                }

            } else {
                // this cell blocks scent
                scent_here = 0;
                type_here.clear();
            }
        }
    }
}
