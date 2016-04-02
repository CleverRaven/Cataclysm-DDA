#include "scent.h"
#include "debug.h"
#include "enums.h"

// @todo It only needs map to get scent blockers and game to get the map...
#include "game.h"
#include "map.h"

#include <string>

scent_layer::scent_layer()
{
    clear();
}

scent_cache::scent_cache()
{
    for( auto &ptr : scents ) {
        ptr = std::unique_ptr<scent_layer>( new scent_layer() );
    }
}

scent_cache::~scent_cache()
{
}

inline bool inbounds( const tripoint &p )
{
    return p.x >= 0 && p.x < MAPSIZE * SEEX &&
           p.y >= 0 && p.y < MAPSIZE * SEEY &&
           p.z >= -OVERMAP_DEPTH && p.z <= OVERMAP_HEIGHT;
}


int scent_cache::get( const tripoint &p ) const
{
    if( inbounds( p ) ) {
        return scents[p.z + OVERMAP_DEPTH]->values[p.x][p.y];
    }

    debugmsg( "Tried to get scent outside bounds: %d, %d, %d", p.x, p.y, p.z );
    return 0;
}

void scent_cache::set( const tripoint &p, int value )
{
    if( inbounds( p ) ) {
        scents[p.z + OVERMAP_DEPTH]->values[p.x][p.y] = value;
    } else {
        debugmsg( "Tried to set scent outside bounds: %d, %d, %d", p.x, p.y, p.z );
    }
}

scent_layer &scent_cache::get_layer( int zlev )
{
    static scent_layer nullayer;
    if( zlev >= -OVERMAP_DEPTH && zlev <= OVERMAP_HEIGHT ) {
        return *scents[zlev + OVERMAP_DEPTH];
    }

    debugmsg( "Tried to get scent layer outside allowed z-level range: %d", zlev );
    return nullayer;
}

// @todo Remove the recursion, make it spread in 3D
void scent_cache::update( int minz, int maxz )
{
    if( minz > maxz ) {
        debugmsg( "minz (%d) > maxz (%d)", minz, maxz );
        return;
    }

    const int zlev = minz;
    scent_array &grscent = get_layer( zlev ).values;
    // These two matrices are transposed so that x addresses are contiguous in memory
    int sum_3_scent_y[SEEY * MAPSIZE][SEEX * MAPSIZE];  //intermediate variable
    int squares_used_y[SEEY * MAPSIZE][SEEX * MAPSIZE]; //intermediate variable
    std::fill_n( &sum_3_scent_y[0][0], SEEX * MAPSIZE * SEEY * MAPSIZE, 0 );
    std::fill_n( &squares_used_y[0][0], SEEX * MAPSIZE * SEEY * MAPSIZE, 0 );

    constexpr int scentmap_minx = 0;
    constexpr int scentmap_maxx = MAPSIZE * SEEX;
    constexpr int scentmap_miny = 0;
    constexpr int scentmap_maxy = MAPSIZE * SEEY;

    // Decrease this to reduce gas spread. Keep it under 125 for stability.
    // This is essentially a decimal number / 1000.
    constexpr int diffusivity = 100;

    const auto &ch = g->m.get_cache_ref( zlev );
    const auto &blocks_scent = ch.blocks_scent;
    const auto &reduces_scent = ch.reduces_scent;
    // Sum neighbors in the y direction.
    // This way, each square gets called 3 times instead of 9 times.
    for( size_t x = scentmap_minx + 1; x < scentmap_maxx - 1; ++x ) {
        // @todo Can be done with 2 additions per tile instead of 3 by
        // only adding/subtracting the newest/oldest value
        for( size_t y = scentmap_miny + 1; y < scentmap_maxy - 1; ++y ) {
            // Sum of the scent val for the 3 neighboring squares that can diffuse into
            for( size_t i = y - 1; i <= y + 1; ++i ) {
                if( !blocks_scent[x][i] ) {
                    // only 20% of scent can diffuse on REDUCE_SCENT squares
                    const int diff_mod = reduces_scent[x][i] ? 2 : 10;
                    sum_3_scent_y[y][x] += diff_mod * grscent[x][i];
                    squares_used_y[y][x] += diff_mod;
                }
            }
        }
    }

    // Rest of the scent map
    for( size_t x = scentmap_minx + 1; x < scentmap_maxx - 1; ++x ) {
        // @todo Can be done with 2 additions per tile instead of 3 by
        // keeping accumulators for both arrays
        for( size_t y = scentmap_miny; y < scentmap_maxy; ++y ) {
            if( blocks_scent[x][y] ) {
                // This cell blocks scent
                grscent[x][y] = 0;
                continue;
            }

            // To how many neighboring squares do we diffuse out? (include our own square
            // since we also include our own square when diffusing in)
            int squares_used = squares_used_y[y][x - 1]
                               + squares_used_y[y][x]
                               + squares_used_y[y][x + 1];

            // Less air movement for REDUCE_SCENT square
            int this_diffusivity = reduces_scent[x][y] ? diffusivity / 5 : diffusivity;
            // Take the old scent and subtract what diffuses out
            int temp_scent = grscent[x][y] * ( 10 * 1000 - squares_used * this_diffusivity );
            // Neighboring walls and reduce_scent squares absorb some scent
            temp_scent -= grscent[x][y] * this_diffusivity * ( 90 - squares_used ) / 5;
            // We've already summed neighboring scent values in the y direction in the previous
            // loop. Now we do it for the x direction, multiply by diffusion, and this is what
            // diffuses into our current square.
            grscent[x][y] =
                ( temp_scent
                  + this_diffusivity * ( sum_3_scent_y[y][x - 1]
                                         + sum_3_scent_y[y][x]
                                         + sum_3_scent_y[y][x + 1] )
                ) / ( 1000 * 10 );

            if( grscent[x][y] > 10000 ) {
                debugmsg( "Wacky scent at %d, %d, %d, (%d)", x, y, zlev, grscent[x][y] );
                grscent[x][y] = 0; // Scent should never be higher
            }
        }
    }

    // @todo Remove, make it properly spread in 3D
    if( minz < maxz ) {
        update( minz + 1, maxz );
    }
}

void scent_cache::decay( int zlev, int amount )
{
    constexpr int scentmap_minx = 0;
    constexpr int scentmap_maxx = MAPSIZE * SEEX;
    constexpr int scentmap_miny = 0;
    constexpr int scentmap_maxy = MAPSIZE * SEEY;

    scent_array &grscent = get_layer( zlev ).values;

    for( size_t x = scentmap_minx; x < scentmap_maxx; ++x ) {
        for( size_t y = scentmap_miny; y < scentmap_maxy; ++y ) {
            grscent[x][y] = std::max( 0, grscent[x][y] - amount );
        }
    }
}

void scent_cache::shift( int dx, int dy )
{
    for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; z++ ) {
        shift( dx, dy, z );
    }
}

void scent_cache::shift( int dx, int dy, int zlev )
{
    scent_layer &layer = get_layer( zlev );
    // Copy the old array
    scent_array old_scent = layer.values;
    layer.clear();

    scent_array &new_scent = layer.values;

    // Clip the area to intersection of new and shifted old
    // We want x+dx>0 and x+dx<MAPSIZE*SEEX for all x
    int scentmap_minx = std::max( 0, -dx );
    int scentmap_maxx = std::min( MAPSIZE * SEEX, MAPSIZE * SEEX - dx );
    int scentmap_miny = std::max( 0, -dy );
    int scentmap_maxy = std::min( MAPSIZE * SEEY, MAPSIZE * SEEY - dy );

    for( int x = scentmap_minx; x < scentmap_maxx; ++x ) {
        for( int y = scentmap_miny; y < scentmap_maxy; ++y ) {
            new_scent[x][y] = std::max( 0, old_scent[x + dx][y + dy] );
        }
    }
}

void scent_cache::clear()
{
    for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; z++ ) {
        clear( z );
    }
}

void scent_cache::clear( int zlev )
{
    get_layer( zlev ).clear();
}

void scent_layer::clear()
{
    std::fill_n( &values[0][0], SEEX * MAPSIZE * SEEY * MAPSIZE, 0 );
}
