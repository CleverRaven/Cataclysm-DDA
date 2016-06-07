#include "scent_map.h"
#include "calendar.h"
#include "color.h"
#include "map.h"
#include "output.h"

static constexpr int SCENT_RADIUS = 40;

nc_color sev( const int a )
{
    switch( a ) {
        case 0:
            return c_cyan;
        case 1:
            return c_ltcyan;
        case 2:
            return c_ltblue;
        case 3:
            return c_blue;
        case 4:
            return c_ltgreen;
        case 5:
            return c_green;
        case 6:
            return c_yellow;
        case 7:
            return c_pink;
        case 8:
            return c_ltred;
        case 9:
            return c_red;
        case 10:
            return c_magenta;
        case 11:
            return c_brown;
        case 12:
            return c_cyan_red;
        case 13:
            return c_ltcyan_red;
        case 14:
            return c_ltblue_red;
        case 15:
            return c_blue_red;
        case 16:
            return c_ltgreen_red;
        case 17:
            return c_green_red;
        case 18:
            return c_yellow_red;
        case 19:
            return c_pink_red;
        case 20:
            return c_magenta_red;
        case 21:
            return c_brown_red;
    }
    return c_dkgray;
}

scent_map::scent_map() = default;

void scent_map::reset()
{
    for( auto &elem : grscent ) {
        for( auto &val : elem ) {
            val = 0;
        }
    }
}

void scent_map::decay()
{
    for( auto &elem : grscent ) {
        for( auto &val : elem ) {
            val = std::max( 0, val - 1 );
        }
    }
}

void scent_map::draw( WINDOW *const win, const int div, const tripoint &center ) const
{
    const int maxx = getmaxx( win );
    const int maxy = getmaxy( win );
    for( int x = 0; x < maxx; ++x ) {
        for( int y = 0; y < maxy; ++y ) {
            const int sn = operator()( x + center.x - maxx / 2, y + center.y - maxy / 2 ) / div;
            mvwprintz( win, y, x, sev( sn / 10 ), "%d", sn % 10 );
        }
    }
}

void scent_map::shift( const int sm_shift_x, const int sm_shift_y )
{
    scent_array<int> new_scent;
    for( size_t x = 0; x < SEEX * MAPSIZE; ++x ) {
        for( size_t y = 0; y < SEEY * MAPSIZE; ++y ) {
            // operator() does bound checking and returns 0 upon invalid coordinates
            new_scent[x][y] = operator()( x + sm_shift_x, y + sm_shift_y );
        }
    }
    grscent = new_scent;
}

int scent_map::operator()( const size_t x, const size_t y ) const
{
    if( inbounds( x, y ) ) {
        return grscent[x][y];
    }
    return 0;
}

int &scent_map::operator()( const size_t x, const size_t y )
{
    if( inbounds( x, y ) ) {
        return grscent[x][y];
    }
    null_scent = 0;
    return null_scent;
}

void scent_map::update( const tripoint &center, map &m )
{
    // Stop updating scent after X turns of the player not moving.
    // Once wind is added, need to reset this on wind shifts as well.
    if( center == player_last_position && player_last_moved + 1000 < calendar::turn ) {
        return;
    }
    player_last_position = center;
    player_last_moved = calendar::turn;

    // note: the next four intermediate matrices need to be at least
    // [2*SCENT_RADIUS+3][2*SCENT_RADIUS+1] in size to hold enough data
    // The code I'm modifying used [SEEX * MAPSIZE]. I'm staying with that to avoid new bugs.

    // These two matrices are transposed so that x addresses are contiguous in memory
    scent_array<int> sum_3_scent_y;
    scent_array<int> squares_used_y;

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
    m.scent_blockers( blocks_scent, reduces_scent, scentmap_minx, scentmap_miny, scentmap_maxx,
                      scentmap_maxy );
    // Sum neighbors in the y direction.  This way, each square gets called 3 times instead of 9
    // times. This cost us an extra loop here, but it also eliminated a loop at the end, so there
    // is a net performance improvement over the old code. Could probably still be better.
    // note: this method needs an array that is one square larger on each side in the x direction
    // than the final scent matrix. I think this is fine since SCENT_RADIUS is less than
    // SEEX*MAPSIZE, but if that changes, this may need tweaking.
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
            auto &scent_here = grscent[x][y];
            if( !blocks_scent[x][y] ) {
                // to how many neighboring squares do we diffuse out? (include our own square
                // since we also include our own square when diffusing in)
                int squares_used = squares_used_y[y][x - 1]
                                   + squares_used_y[y][x]
                                   + squares_used_y[y][x + 1];

                int this_diffusivity;
                if( !reduces_scent[x][y] ) {
                    this_diffusivity = diffusivity;
                } else {
                    this_diffusivity = diffusivity / 5; //less air movement for REDUCE_SCENT square
                }
                int temp_scent;
                // take the old scent and subtract what diffuses out
                temp_scent = scent_here * ( 10 * 1000 - squares_used * this_diffusivity );
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
            } else {
                // this cell blocks scent
                scent_here = 0;
            }
        }
    }
}
