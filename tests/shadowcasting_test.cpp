#include <chrono>
#include <cstdio>
#include <random>
#include <array>
#include <functional>
#include <memory>
#include <type_traits>
#include <vector>

#include "catch/catch.hpp"
#include "line.h" // For rl_dist.
#include "map.h"
#include "rng.h"
#include "shadowcasting.h"
#include "game_constants.h"
#include "lightmap.h"
#include "point.h"

// Constants setting the ratio of set to unset tiles.
constexpr unsigned int NUMERATOR = 1;
constexpr unsigned int DENOMINATOR = 10;

static void oldCastLight( float ( &output_cache )[MAPSIZE * SEEX][MAPSIZE * SEEY],
                          const float ( &input_array )[MAPSIZE * SEEX][MAPSIZE * SEEY],
                          const int xx, const int xy, const int yx, const int yy,
                          const int offsetX, const int offsetY, const int offsetDistance,
                          const int row = 1, float start = 1.0f, const float end = 0.0f )
{

    float newStart = 0.0f;
    const float radius = 60.0f - offsetDistance;
    if( start < end ) {
        return;
    }
    bool blocked = false;
    tripoint delta;
    for( int distance = row; distance <= radius && !blocked; distance++ ) {
        delta.y = -distance;
        for( delta.x = -distance; delta.x <= 0; delta.x++ ) {
            const int currentX = offsetX + delta.x * xx + delta.y * xy;
            const int currentY = offsetY + delta.x * yx + delta.y * yy;
            const float leftSlope = ( delta.x - 0.5f ) / ( delta.y + 0.5f );
            const float rightSlope = ( delta.x + 0.5f ) / ( delta.y - 0.5f );

            if( start < rightSlope ) {
                continue;
            } else if( end > leftSlope ) {
                break;
            }

            //check if it's within the visible area and mark visible if so
            if( rl_dist( tripoint_zero, delta ) <= radius ) {
                output_cache[currentX][currentY] = LIGHT_TRANSPARENCY_CLEAR;
            }

            if( blocked ) {
                //previous cell was a blocking one
                if( input_array[currentX][currentY] == LIGHT_TRANSPARENCY_SOLID ) {
                    //hit a wall
                    newStart = rightSlope;
                } else {
                    blocked = false;
                    start = newStart;
                }
            } else {
                if( input_array[currentX][currentY] == LIGHT_TRANSPARENCY_SOLID &&
                    distance < radius ) {
                    //hit a wall within sight line
                    blocked = true;
                    oldCastLight( output_cache, input_array, xx, xy, yx, yy,
                                  offsetX, offsetY, offsetDistance,  distance + 1, start, leftSlope );
                    newStart = rightSlope;
                }
            }
        }
    }
}

/*
 * This is checking whether bresenham visibility checks match shadowcasting (they don't).
 */
static bool bresenham_visibility_check(
    const int offsetX, const int offsetY, const int x, const int y,
    const float ( &transparency_cache )[MAPSIZE * SEEX][MAPSIZE * SEEY] )
{
    if( offsetX == x && offsetY == y ) {
        return true;
    }
    bool visible = true;
    const int junk = 0;
    bresenham( point( x, y ), point( offsetX, offsetY ), junk,
    [&transparency_cache, &visible]( const point & new_point ) {
        if( transparency_cache[new_point.x][new_point.y] <=
            LIGHT_TRANSPARENCY_SOLID ) {
            visible = false;
            return false;
        }
        return true;
    } );
    return visible;
}

static void randomly_fill_transparency(
    float ( &transparency_cache )[MAPSIZE * SEEX][MAPSIZE * SEEY],
    const unsigned int numerator = NUMERATOR, const unsigned int denominator = DENOMINATOR )
{
    // Construct a rng that produces integers in a range selected to provide the probability
    // we want, i.e. if we want 1/4 tiles to be set, produce numbers in the range 0-3,
    // with 0 indicating the bit is set.
    std::uniform_int_distribution<unsigned int> distribution( 0, denominator );
    auto rng = std::bind( distribution, rng_get_engine() );

    // Initialize the transparency value of each square to a random value.
    for( auto &inner : transparency_cache ) {
        for( float &square : inner ) {
            if( rng() < numerator ) {
                square = LIGHT_TRANSPARENCY_SOLID;
            } else {
                square = LIGHT_TRANSPARENCY_CLEAR;
            }
        }
    }
}

static bool is_nonzero( const float x )
{
    return x != 0;
}

static bool is_nonzero( const four_quadrants &x )
{
    return is_nonzero( x.max() );
}

template<typename Exp>
bool grids_are_equivalent( float control[MAPSIZE * SEEX][MAPSIZE * SEEY],
                           Exp experiment[MAPSIZE * SEEX][MAPSIZE * SEEY] )
{
    for( int x = 0; x < MAPSIZE * SEEX; ++x ) {
        for( int y = 0; y < MAPSIZE * SEEY; ++y ) {
            // Check that both agree on the outcome, but not necessarily the same values.
            if( is_nonzero( control[x][y] ) != is_nonzero( experiment[x][y] ) ) {
                return false;
            }
        }
    }

    return true;
}

template<typename Exp>
void print_grid_comparison( const int offsetX, const int offsetY,
                            float ( &transparency_cache )[MAPSIZE * SEEX][MAPSIZE * SEEY],
                            float control[MAPSIZE * SEEX][MAPSIZE * SEEY],
                            Exp experiment[MAPSIZE * SEEX][MAPSIZE * SEEY] )
{
    for( int x = 0; x < MAPSIZE * SEEX; ++x ) {
        for( int y = 0; y < MAPSIZE * SEEX; ++y ) {
            char output = ' ';
            const bool shadowcasting_disagrees =
                is_nonzero( control[x][y] ) != is_nonzero( experiment[x][y] );
            const bool bresenham_disagrees =
                bresenham_visibility_check( offsetX, offsetY, x, y, transparency_cache ) !=
                is_nonzero( experiment[x][y] );

            if( shadowcasting_disagrees && bresenham_disagrees ) {
                if( is_nonzero( experiment[x][y] ) ) {
                    output = 'R'; // Old shadowcasting and bresenham can't see.
                } else {
                    output = 'N'; // New shadowcasting can't see.
                }
            } else if( shadowcasting_disagrees ) {
                if( is_nonzero( control[x][y] ) ) {
                    output = 'C'; // New shadowcasting & bresenham can't see.
                } else {
                    output = 'O'; // Old shadowcasting can't see.
                }
            } else if( bresenham_disagrees ) {
                if( is_nonzero( experiment[x][y] ) ) {
                    output = 'B'; // Bresenham can't see it.
                } else {
                    output = 'S'; // Shadowcasting can't see it.
                }
            }
            if( transparency_cache[x][y] == LIGHT_TRANSPARENCY_SOLID ) {
                output = '#';
            }
            if( x == offsetX && y == offsetY ) {
                output = '@';
            }
            printf( "%c", output );
        }
        printf( "\n" );
    }
    for( int x = 0; x < MAPSIZE * SEEX; ++x ) {
        for( int y = 0; y < MAPSIZE * SEEX; ++y ) {
            char output = ' ';
            if( transparency_cache[x][y] == LIGHT_TRANSPARENCY_SOLID ) {
                output = '#';
            } else if( control[x][y] > LIGHT_TRANSPARENCY_SOLID ) {
                output = 'X';
            }
            printf( "%c", output );
        }
        printf( "    " );
        for( int y = 0; y < MAPSIZE * SEEX; ++y ) {
            char output = ' ';
            if( transparency_cache[x][y] == LIGHT_TRANSPARENCY_SOLID ) {
                output = '#';
            } else if( is_nonzero( experiment[x][y] ) ) {
                output = 'X';
            }
            printf( "%c", output );
        }
        printf( "\n" );
    }
}

static void shadowcasting_runoff( const int iterations, const bool test_bresenham = false )
{
    float seen_squares_control[MAPSIZE * SEEX][MAPSIZE * SEEY] = {{0}};
    float seen_squares_experiment[MAPSIZE * SEEX][MAPSIZE * SEEY] = {{0}};
    float transparency_cache[MAPSIZE * SEEX][MAPSIZE * SEEY] = {{0}};

    randomly_fill_transparency( transparency_cache );

    map dummy;

    const int offsetX = 65;
    const int offsetY = 65;

    const auto start1 = std::chrono::high_resolution_clock::now();
    for( int i = 0; i < iterations; i++ ) {
        // First the control algorithm.
        oldCastLight( seen_squares_control, transparency_cache, 0, 1, 1, 0, offsetX, offsetY, 0 );
        oldCastLight( seen_squares_control, transparency_cache, 1, 0, 0, 1, offsetX, offsetY, 0 );

        oldCastLight( seen_squares_control, transparency_cache, 0, -1, 1, 0, offsetX, offsetY, 0 );
        oldCastLight( seen_squares_control, transparency_cache, -1, 0, 0, 1, offsetX, offsetY, 0 );

        oldCastLight( seen_squares_control, transparency_cache, 0, 1, -1, 0, offsetX, offsetY, 0 );
        oldCastLight( seen_squares_control, transparency_cache, 1, 0, 0, -1, offsetX, offsetY, 0 );

        oldCastLight( seen_squares_control, transparency_cache, 0, -1, -1, 0, offsetX, offsetY, 0 );
        oldCastLight( seen_squares_control, transparency_cache, -1, 0, 0, -1, offsetX, offsetY, 0 );
    }
    const auto end1 = std::chrono::high_resolution_clock::now();

    const auto start2 = std::chrono::high_resolution_clock::now();
    for( int i = 0; i < iterations; i++ ) {
        // Then the current algorithm.
        castLightAll<float, float, sight_calc, sight_check, update_light, accumulate_transparency>(
            seen_squares_experiment, transparency_cache, point( offsetX, offsetY ) );
    }
    const auto end2 = std::chrono::high_resolution_clock::now();

    if( iterations > 1 ) {
        const long long diff1 = std::chrono::duration_cast<std::chrono::microseconds>
                                ( end1 - start1 ).count();
        const long long diff2 = std::chrono::duration_cast<std::chrono::microseconds>
                                ( end2 - start2 ).count();
        printf( "oldCastLight() executed %d times in %lld microseconds.\n",
                iterations, diff1 );
        printf( "castLight() executed %d times in %lld microseconds.\n",
                iterations, diff2 );
    }

    bool passed = grids_are_equivalent( seen_squares_control, seen_squares_experiment );
    for( int x = 0; test_bresenham && passed && x < MAPSIZE * SEEX; ++x ) {
        for( int y = 0; y < MAPSIZE * SEEX; ++y ) {
            // Check that both agree on the outcome, but not necessarily the same values.
            if( bresenham_visibility_check( offsetX, offsetY, x, y, transparency_cache ) !=
                ( seen_squares_experiment[x][y] > LIGHT_TRANSPARENCY_SOLID ) ) {
                passed = false;
                break;
            }
        }
    }

    if( !passed ) {
        print_grid_comparison( offsetX, offsetY, transparency_cache, seen_squares_control,
                               seen_squares_experiment );
    }

    REQUIRE( passed );
}

static void shadowcasting_float_quad(
    const int iterations, const unsigned int denominator = DENOMINATOR )
{
    float lit_squares_float[MAPSIZE * SEEX][MAPSIZE * SEEY] = {{0}};
    four_quadrants lit_squares_quad[MAPSIZE * SEEX][MAPSIZE * SEEY] = {{}};
    float transparency_cache[MAPSIZE * SEEX][MAPSIZE * SEEY] = {{0}};

    randomly_fill_transparency( transparency_cache, denominator );

    map dummy;

    const int offsetX = 65;
    const int offsetY = 65;

    const auto start1 = std::chrono::high_resolution_clock::now();
    for( int i = 0; i < iterations; i++ ) {
        castLightAll<float, four_quadrants, sight_calc, sight_check, update_light_quadrants,
                     accumulate_transparency>(
                         lit_squares_quad, transparency_cache, point( offsetX, offsetY ) );
    }
    const auto end1 = std::chrono::high_resolution_clock::now();

    const auto start2 = std::chrono::high_resolution_clock::now();
    for( int i = 0; i < iterations; i++ ) {
        // Then the current algorithm.
        castLightAll<float, float, sight_calc, sight_check, update_light,
                     accumulate_transparency>(
                         lit_squares_float, transparency_cache, point( offsetX, offsetY ) );
    }
    const auto end2 = std::chrono::high_resolution_clock::now();

    if( iterations > 1 ) {
        const long long diff1 = std::chrono::duration_cast<std::chrono::microseconds>
                                ( end1 - start1 ).count();
        const long long diff2 = std::chrono::duration_cast<std::chrono::microseconds>
                                ( end2 - start2 ).count();
        printf( "castLight on four_quadrants (denominator %u) "
                "executed %d times in %lld microseconds.\n",
                denominator, iterations, diff1 );
        printf( "castLight on floats (denominator %u) "
                "executed %d times in %lld microseconds.\n",
                denominator, iterations, diff2 );
    }

    bool passed = grids_are_equivalent( lit_squares_float, lit_squares_quad );

    if( !passed ) {
        print_grid_comparison( offsetX, offsetY, transparency_cache, lit_squares_float,
                               lit_squares_quad );
    }

    REQUIRE( passed );
}

static void shadowcasting_3d_2d( const int iterations )
{
    float seen_squares_control[MAPSIZE * SEEX][MAPSIZE * SEEY] = {{0}};
    float seen_squares_experiment[MAPSIZE * SEEX][MAPSIZE * SEEY] = {{0}};
    float transparency_cache[MAPSIZE * SEEX][MAPSIZE * SEEY] = {{0}};
    bool floor_cache[MAPSIZE * SEEX][MAPSIZE * SEEY] = {{false}};

    randomly_fill_transparency( transparency_cache );

    map dummy;

    const int offsetX = 65;
    const int offsetY = 65;
    const int offsetZ = 0;

    const auto start1 = std::chrono::high_resolution_clock::now();
    for( int i = 0; i < iterations; i++ ) {
        // First the control algorithm.
        castLightAll<float, float, sight_calc, sight_check, update_light, accumulate_transparency>(
            seen_squares_control, transparency_cache, point( offsetX, offsetY ) );
    }
    const auto end1 = std::chrono::high_resolution_clock::now();

    const tripoint origin( offsetX, offsetY, offsetZ );
    std::array<const float ( * )[MAPSIZE *SEEX][MAPSIZE *SEEY], OVERMAP_LAYERS> transparency_caches;
    std::array<float ( * )[MAPSIZE *SEEX][MAPSIZE *SEEY], OVERMAP_LAYERS> seen_caches;
    std::array<const bool ( * )[MAPSIZE *SEEX][MAPSIZE *SEEY], OVERMAP_LAYERS> floor_caches;
    for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; z++ ) {
        // TODO: Give some more proper values here
        transparency_caches[z + OVERMAP_DEPTH] = &transparency_cache;
        seen_caches[z + OVERMAP_DEPTH] = &seen_squares_experiment;
        floor_caches[z + OVERMAP_DEPTH] = &floor_cache;
    }

    const auto start2 = std::chrono::high_resolution_clock::now();
    for( int i = 0; i < iterations; i++ ) {
        // Then the newer algorithm.
        cast_zlight<float, sight_calc, sight_check, accumulate_transparency>(
            seen_caches, transparency_caches, floor_caches, origin, 0, 1.0 );
    }
    const auto end2 = std::chrono::high_resolution_clock::now();

    if( iterations > 1 ) {
        const long long diff1 =
            std::chrono::duration_cast<std::chrono::microseconds>( end1 - start1 ).count();
        const long long diff2 =
            std::chrono::duration_cast<std::chrono::microseconds>( end2 - start2 ).count();
        printf( "castLight() executed %d times in %lld microseconds.\n",
                iterations, diff1 );
        printf( "cast_zlight() executed %d times in %lld microseconds.\n",
                iterations, diff2 );
        printf( "new/old execution time ratio: %.02f.\n", static_cast<double>( diff2 ) / diff1 );
    }

    bool passed = grids_are_equivalent( seen_squares_control, seen_squares_experiment );

    if( !passed ) {
        print_grid_comparison( offsetX, offsetY, transparency_cache, seen_squares_control,
                               seen_squares_experiment );
    }

    REQUIRE( passed );
}


// T, O and V are 'T'ransparent, 'O'paque and 'V'isible.
// X marks the player location, which is not set to visible by this algorithm.
#define T LIGHT_TRANSPARENCY_CLEAR
#define O LIGHT_TRANSPARENCY_SOLID
#define V LIGHT_TRANSPARENCY_CLEAR
#define X LIGHT_TRANSPARENCY_SOLID

const point ORIGIN( 65, 65 );

struct grid_overlay {
    std::vector<std::vector<float>> data;
    point offset;
    float default_value;

    // origin_offset is specified as the coordinates of the "camera" within the overlay.
    grid_overlay( const point &origin_offset, const float default_value ) {
        this->offset = ORIGIN - origin_offset;
        this->default_value = default_value;
    }

    int height() const {
        return data.size();
    }
    int width() const {
        if( data.empty() ) {
            return 0;
        }
        return data[0].size();
    }

    float get_global( const int x, const int y ) const {
        if( y >= offset.y && y < offset.y + height() &&
            x >= offset.x && x < offset.x + width() ) {
            return data[ y - offset.y ][ x - offset.x ];
        }
        return default_value;
    }

    float get_local( const int x, const int y ) const {
        return data[ y ][ x ];
    }
};

static void run_spot_check( const grid_overlay &test_case, const grid_overlay &expected_result )
{
    float seen_squares[ MAPSIZE * SEEY ][ MAPSIZE * SEEX ] = {{ 0 }};
    float transparency_cache[ MAPSIZE * SEEY ][ MAPSIZE * SEEX ] = {{ 0 }};

    for( int y = 0; y < static_cast<int>( sizeof( transparency_cache ) /
                                          sizeof( transparency_cache[0] ) ); ++y ) {
        for( int x = 0; x < static_cast<int>( sizeof( transparency_cache[0] ) /
                                              sizeof( transparency_cache[0][0] ) ); ++x ) {
            transparency_cache[ y ][ x ] = test_case.get_global( x, y );
        }
    }

    castLightAll<float, float, sight_calc, sight_check, update_light, accumulate_transparency>(
        seen_squares, transparency_cache, ORIGIN );

    // Compares the whole grid, but out-of-bounds compares will de-facto pass.
    for( int y = 0; y < expected_result.height(); ++y ) {
        for( int x = 0; x < expected_result.width(); ++x ) {
            INFO( "x:" << x << " y:" << y << " expected:" << expected_result.data[y][x] << " actual:" <<
                  seen_squares[expected_result.offset.y + y][expected_result.offset.x + x] );
            if( V == expected_result.get_local( x, y ) ) {
                CHECK( seen_squares[expected_result.offset.y + y][expected_result.offset.x + x] > 0 );
            } else {
                CHECK( seen_squares[expected_result.offset.y + y][expected_result.offset.x + x] == 0 );
            }
        }
    }
}

TEST_CASE( "shadowcasting_slope_inversion_regression_test", "[shadowcasting]" )
{
    grid_overlay test_case( { 7, 8 }, LIGHT_TRANSPARENCY_CLEAR );
    test_case.data = {
        {T, T, T, T, T, T, T, T, T, T},
        {T, O, T, T, T, T, T, T, T, T},
        {T, O, T, T, T, T, T, T, T, T},
        {T, O, O, T, O, T, T, T, T, T},
        {T, T, T, T, T, T, T, T, T, T},
        {T, T, T, T, T, T, T, T, T, T},
        {T, T, T, T, T, T, T, T, T, T},
        {T, T, T, T, T, T, T, T, O, T},
        {T, T, T, T, T, T, O, T, O, T},
        {T, T, T, T, T, T, O, O, O, T},
        {T, T, T, T, T, T, T, T, T, T}
    };

    grid_overlay expected_results( { 7, 8 }, LIGHT_TRANSPARENCY_CLEAR );
    expected_results.data = {
        {O, O, O, V, V, V, V, V, V, V},
        {O, V, V, O, V, V, V, V, V, V},
        {O, O, V, V, V, V, V, V, V, V},
        {O, O, V, V, V, V, V, V, V, V},
        {O, O, V, V, V, V, V, V, V, V},
        {O, O, O, V, V, V, V, V, V, O},
        {O, O, O, O, V, V, V, V, V, O},
        {O, O, O, O, O, V, V, V, V, O},
        {O, O, O, O, O, O, V, X, V, O},
        {O, O, O, O, O, O, V, V, V, O},
        {O, O, O, O, O, O, O, O, O, O}
    };

    run_spot_check( test_case, expected_results );
}

TEST_CASE( "shadowcasting_pillar_behavior_cardinally_adjacent", "[shadowcasting]" )
{
    grid_overlay test_case( { 1, 4 }, LIGHT_TRANSPARENCY_CLEAR );
    test_case.data = {
        {T, T, T, T, T, T, T, T, T},
        {T, T, T, T, T, T, T, T, T},
        {T, T, T, T, T, T, T, T, T},
        {T, T, T, T, T, T, T, T, T},
        {T, T, O, T, T, T, T, T, T},
        {T, T, T, T, T, T, T, T, T},
        {T, T, T, T, T, T, T, T, T},
        {T, T, T, T, T, T, T, T, T},
        {T, T, T, T, T, T, T, T, T}
    };

    grid_overlay expected_results( { 1, 4 }, LIGHT_TRANSPARENCY_CLEAR );
    expected_results.data = {
        {V, V, V, V, V, V, V, O, O},
        {V, V, V, V, V, V, O, O, O},
        {V, V, V, V, V, O, O, O, O},
        {V, V, V, V, O, O, O, O, O},
        {V, X, V, O, O, O, O, O, O},
        {V, V, V, V, O, O, O, O, O},
        {V, V, V, V, V, O, O, O, O},
        {V, V, V, V, V, V, O, O, O},
        {V, V, V, V, V, V, V, O, O}
    };

    run_spot_check( test_case, expected_results );
}

TEST_CASE( "shadowcasting_pillar_behavior_2_1_diagonal_gap", "[shadowcasting]" )
{
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    grid_overlay test_case( { 1, 1 }, LIGHT_TRANSPARENCY_CLEAR );
    test_case.data = {
        {T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T},
        {T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T},
        {T, T, T, O, T, T, T, T, T, T, T, T, T, T, T, T, T, T},
        {T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T},
        {T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T},
        {T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T},
        {T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T},
        {T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T},
        {T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T}
    };

    // NOLINTNEXTLINE(cata-use-named-point-constants)
    grid_overlay expected_results( { 1, 1 }, LIGHT_TRANSPARENCY_CLEAR );
    expected_results.data = {
        {V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V},
        {V, X, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V},
        {V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V},
        {V, V, V, V, V, O, O, O, V, V, V, V, V, V, V, V, V, V},
        {V, V, V, V, V, V, O, O, O, O, O, O, O, V, V, V, V, V},
        {V, V, V, V, V, V, V, O, O, O, O, O, O, O, O, O, O, O},
        {V, V, V, V, V, V, V, V, O, O, O, O, O, O, O, O, O, O},
        {V, V, V, V, V, V, V, V, V, O, O, O, O, O, O, O, O, O},
        {V, V, V, V, V, V, V, V, V, V, O, O, O, O, O, O, O, O},
    };

    run_spot_check( test_case, expected_results );
}

TEST_CASE( "shadowcasting_vision_along_a_wall", "[shadowcasting]" )
{
    grid_overlay test_case( { 8, 2 }, LIGHT_TRANSPARENCY_CLEAR );
    test_case.data = {
        {T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T},
        {T, O, O, O, O, O, O, O, O, O, O, O, O, O, O, O, O, T},
        {T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T},
        {T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T},
        {T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T},
        {T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T},
        {T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T},
        {T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T},
        {T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T}
    };

    grid_overlay expected_results( { 8, 2 }, LIGHT_TRANSPARENCY_CLEAR );
    expected_results.data = {
        {O, O, O, O, O, O, O, O, O, O, O, O, O, O, O, O, O, O},
        {V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V},
        {V, V, V, V, V, V, V, V, X, V, V, V, V, V, V, V, V, V},
        {V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V},
        {V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V},
        {V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V},
        {V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V},
        {V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V}
    };

    run_spot_check( test_case, expected_results );
}

// Some random edge cases aren't matching.
TEST_CASE( "shadowcasting_runoff", "[.]" )
{
    shadowcasting_runoff( 1 );
}

TEST_CASE( "shadowcasting_performance", "[.]" )
{
    shadowcasting_runoff( 100000 );
}

TEST_CASE( "shadowcasting_3d_2d", "[.]" )
{
    shadowcasting_3d_2d( 1 );
}

TEST_CASE( "shadowcasting_3d_2d_performance", "[.]" )
{
    shadowcasting_3d_2d( 100000 );
}

TEST_CASE( "shadowcasting_float_quad_equivalence", "[shadowcasting]" )
{
    shadowcasting_float_quad( 1 );
}

TEST_CASE( "shadowcasting_float_quad_performance", "[.]" )
{
    shadowcasting_float_quad( 1000000 );
    shadowcasting_float_quad( 1000000, 100 );
}

// I'm not sure this will ever work.
TEST_CASE( "bresenham_vs_shadowcasting", "[.]" )
{
    shadowcasting_runoff( 1, true );
}
