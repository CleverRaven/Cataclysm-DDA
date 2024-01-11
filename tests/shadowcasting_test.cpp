#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <functional>
#include <sstream>
#include <type_traits>
#include <vector>

#include "cata_catch.h"
#include "cuboid_rectangle.h"
#include "game_constants.h"
#include "level_cache.h"
#include "lightmap.h"
#include "line.h" // For rl_dist.
#include "map.h"
#include "mdarray.h"
#include "point.h"
#include "rng.h"
#include "shadowcasting.h"

// Constants setting the ratio of set to unset tiles.
static constexpr unsigned int NUMERATOR = 1;
static constexpr unsigned int DENOMINATOR = 10;

// NOLINTNEXTLINE(cata-xy)
static void oldCastLight(
    cata::mdarray<float, point_bub_ms> &output_cache,
    const cata::mdarray<float, point_bub_ms> &input_array,
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
            const point current( offsetX + delta.x * xx + delta.y * xy, offsetY + delta.x * yx + delta.y * yy );
            const float leftSlope = ( delta.x - 0.5f ) / ( delta.y + 0.5f );
            const float rightSlope = ( delta.x + 0.5f ) / ( delta.y - 0.5f );

            if( start < rightSlope ) {
                continue;
            } else if( end > leftSlope ) {
                break;
            }

            //check if it's within the visible area and mark visible if so
            if( rl_dist( tripoint_zero, delta ) <= radius ) {
                output_cache[current.x][current.y] = VISIBILITY_FULL;
            }

            if( blocked ) {
                //previous cell was a blocking one
                if( input_array[current.x][current.y] == LIGHT_TRANSPARENCY_SOLID ) {
                    //hit a wall
                    newStart = rightSlope;
                } else {
                    blocked = false;
                    start = newStart;
                }
            } else {
                if( input_array[current.x][current.y] == LIGHT_TRANSPARENCY_SOLID &&
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
    const point &offset, const point &p,
    const cata::mdarray<float, point_bub_ms> &transparency_cache )
{
    if( offset == p ) {
        return true;
    }
    bool visible = true;
    const int junk = 0;
    bresenham( p, offset, junk,
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
    cata::mdarray<float, point_bub_ms> &transparency_cache,
    const unsigned int numerator = NUMERATOR, const unsigned int denominator = DENOMINATOR )
{
    // Construct a rng that produces integers in a range selected to provide the probability
    // we want, i.e. if we want 1/4 tiles to be set, produce numbers in the range 0-3,
    // with 0 indicating the bit is set.
    std::uniform_int_distribution<unsigned int> distribution( 0, denominator );
    auto rng = [&distribution] { return distribution( rng_get_engine() ); };

    // Initialize the transparency value of each square to a random value.
    transparency_cache.fill_from_callable( [numerator, &rng]() {
        if( rng() < numerator ) {
            return LIGHT_TRANSPARENCY_SOLID;
        } else {
            return LIGHT_TRANSPARENCY_OPEN_AIR;
        }
    } );
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
bool grids_are_equivalent(
    const cata::mdarray<float, point_bub_ms> &control,
    const cata::mdarray<Exp, point_bub_ms> &experiment )
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
void print_grid_comparison(
    const point &offset,
    cata::mdarray<float, point_bub_ms> &transparency_cache,
    const cata::mdarray<float, point_bub_ms> &control,
    const cata::mdarray<Exp, point_bub_ms> &experiment )
{
    for( int x = 0; x < MAPSIZE * SEEX; ++x ) {
        for( int y = 0; y < MAPSIZE * SEEX; ++y ) {
            char output = ' ';
            const bool shadowcasting_disagrees =
                is_nonzero( control[x][y] ) != is_nonzero( experiment[x][y] );
            const bool bresenham_disagrees =
                bresenham_visibility_check( offset, point( x, y ), transparency_cache ) !=
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
            if( x == offset.x && y == offset.y ) {
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
    cata::mdarray<float, point_bub_ms> seen_squares_control = {};
    cata::mdarray<float, point_bub_ms> seen_squares_experiment = {};
    cata::mdarray<float, point_bub_ms> transparency_cache = {};

    randomly_fill_transparency( transparency_cache );

    map dummy;

    const point offset( 65, 65 );

    const std::chrono::high_resolution_clock::time_point start1 =
        std::chrono::high_resolution_clock::now();
    for( int i = 0; i < iterations; i++ ) {
        // First the control algorithm.
        oldCastLight( seen_squares_control, transparency_cache, 0, 1, 1, 0, offset.x, offset.y, 0 );
        oldCastLight( seen_squares_control, transparency_cache, 1, 0, 0, 1, offset.x, offset.y, 0 );

        oldCastLight( seen_squares_control, transparency_cache, 0, -1, 1, 0, offset.x, offset.y, 0 );
        oldCastLight( seen_squares_control, transparency_cache, -1, 0, 0, 1, offset.x, offset.y, 0 );

        oldCastLight( seen_squares_control, transparency_cache, 0, 1, -1, 0, offset.x, offset.y, 0 );
        oldCastLight( seen_squares_control, transparency_cache, 1, 0, 0, -1, offset.x, offset.y, 0 );

        oldCastLight( seen_squares_control, transparency_cache, 0, -1, -1, 0, offset.x, offset.y, 0 );
        oldCastLight( seen_squares_control, transparency_cache, -1, 0, 0, -1, offset.x, offset.y, 0 );
    }
    const std::chrono::high_resolution_clock::time_point end1 =
        std::chrono::high_resolution_clock::now();

    const std::chrono::high_resolution_clock::time_point start2 =
        std::chrono::high_resolution_clock::now();
    for( int i = 0; i < iterations; i++ ) {
        // Then the current algorithm.
        castLightAll<float, float, sight_calc, sight_check, update_light, accumulate_transparency>(
            seen_squares_experiment, transparency_cache, offset );
    }
    const std::chrono::high_resolution_clock::time_point end2 =
        std::chrono::high_resolution_clock::now();

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
            if( bresenham_visibility_check( offset, point( x, y ), transparency_cache ) !=
                ( seen_squares_experiment[x][y] > LIGHT_TRANSPARENCY_SOLID ) ) {
                passed = false;
                break;
            }
        }
    }

    if( !passed ) {
        print_grid_comparison( offset, transparency_cache, seen_squares_control,
                               seen_squares_experiment );
    }

    REQUIRE( passed );
}

static void shadowcasting_float_quad(
    const int iterations, const unsigned int denominator = DENOMINATOR )
{
    struct test_grids {
        cata::mdarray<float, point_bub_ms> lit_squares_float = {};
        cata::mdarray<four_quadrants, point_bub_ms> lit_squares_quad = {};
        cata::mdarray<float, point_bub_ms> transparency_cache = {};
    };

    std::unique_ptr<test_grids> grids = std::make_unique<test_grids>();
    cata::mdarray<float, point_bub_ms> &lit_squares_float = grids->lit_squares_float;
    cata::mdarray<four_quadrants, point_bub_ms> &lit_squares_quad = grids->lit_squares_quad;
    cata::mdarray<float, point_bub_ms> &transparency_cache = grids->transparency_cache;

    randomly_fill_transparency( transparency_cache, denominator );

    map dummy;

    const point offset( 65, 65 );

    const std::chrono::high_resolution_clock::time_point start1 =
        std::chrono::high_resolution_clock::now();
    for( int i = 0; i < iterations; i++ ) {
        castLightAll<float, four_quadrants, sight_calc, sight_check, update_light_quadrants,
                     accumulate_transparency>(
                         lit_squares_quad, transparency_cache, offset );
    }
    const std::chrono::high_resolution_clock::time_point end1 =
        std::chrono::high_resolution_clock::now();

    const std::chrono::high_resolution_clock::time_point start2 =
        std::chrono::high_resolution_clock::now();
    for( int i = 0; i < iterations; i++ ) {
        // Then the current algorithm.
        castLightAll<float, float, sight_calc, sight_check, update_light,
                     accumulate_transparency>(
                         lit_squares_float, transparency_cache, offset );
    }
    const std::chrono::high_resolution_clock::time_point end2 =
        std::chrono::high_resolution_clock::now();

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
        print_grid_comparison( offset, transparency_cache, lit_squares_float,
                               lit_squares_quad );
    }

    REQUIRE( passed );
}

static void do_3d_benchmark(
    const array_of_grids_of<const float> &transparency_caches,
    const int iterations )
{
    struct test_grids {
        std::array<cata::mdarray<float, point_bub_ms>, OVERMAP_LAYERS> seen_squares = {};
        std::array<cata::mdarray<bool, point_bub_ms>, OVERMAP_LAYERS> floor_cache = {};
    };

    std::unique_ptr<test_grids> grids = std::make_unique<test_grids>();

    const tripoint origin( 65, 65, 0 );
    array_of_grids_of<float> seen_caches;
    array_of_grids_of<const bool> floor_caches;

    for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; z++ ) {
        seen_caches[z + OVERMAP_DEPTH] = &grids->seen_squares[z + OVERMAP_DEPTH];
        floor_caches[z + OVERMAP_DEPTH] = &grids->floor_cache[z + OVERMAP_DEPTH];
    }

    const std::chrono::high_resolution_clock::time_point start =
        std::chrono::high_resolution_clock::now();
    for( int i = 0; i < iterations; i++ ) {
        cast_zlight<float, sight_calc, sight_check, accumulate_transparency>(
            seen_caches, transparency_caches, floor_caches, origin, 0, 1.0 );
    }
    const std::chrono::high_resolution_clock::time_point end =
        std::chrono::high_resolution_clock::now();

    if( iterations > 1 ) {
        const long long diff =
            std::chrono::duration_cast<std::chrono::microseconds>( end - start ).count();
        printf( "cast_zlight() executed %d times in %lld microseconds.\n",
                iterations, diff );
    }
}

static void shadowcasting_3d_benchmark( const int iterations )
{
    struct test_grids {
        std::array<cata::mdarray<float, point_bub_ms>, OVERMAP_LAYERS> transparency_cache = {};
    };

    std::unique_ptr<test_grids> grids = std::make_unique<test_grids>();
    std::array<cata::mdarray<float, point_bub_ms>, OVERMAP_LAYERS> &transparency_cache =
        grids->transparency_cache;
    array_of_grids_of<const float> transparency_caches;
    for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; z++ ) {
        randomly_fill_transparency( transparency_cache[z + OVERMAP_DEPTH] );
        transparency_caches[z + OVERMAP_DEPTH] = &transparency_cache[z + OVERMAP_DEPTH];
    }
    do_3d_benchmark( transparency_caches, iterations );

    // Flat plain
    // TODO: add roofs
    for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; z++ ) {
        float value_to_set = LIGHT_TRANSPARENCY_SOLID;
        if( z < 0 ) {
            value_to_set = LIGHT_TRANSPARENCY_SOLID;
        } else {
            value_to_set = LIGHT_TRANSPARENCY_OPEN_AIR;
        }
        transparency_cache[z + OVERMAP_DEPTH].fill( value_to_set );
    }
    do_3d_benchmark( transparency_caches, iterations );

    // Add some obstacles, a ring at distance 5
    cata::mdarray<float, point_bub_ms> &ground_level = transparency_cache[OVERMAP_DEPTH];
    ground_level[60][65] = LIGHT_TRANSPARENCY_SOLID;
    ground_level[63][63] = LIGHT_TRANSPARENCY_SOLID;
    ground_level[65][60] = LIGHT_TRANSPARENCY_SOLID;
    ground_level[68][63] = LIGHT_TRANSPARENCY_SOLID;
    ground_level[65][70] = LIGHT_TRANSPARENCY_SOLID;
    ground_level[68][68] = LIGHT_TRANSPARENCY_SOLID;
    ground_level[70][65] = LIGHT_TRANSPARENCY_SOLID;
    ground_level[63][68] = LIGHT_TRANSPARENCY_SOLID;
    do_3d_benchmark( transparency_caches, iterations );
}

static void shadowcasting_3d_2d( const int iterations )
{
    cata::mdarray<float, point_bub_ms> seen_squares_control = {};
    cata::mdarray<float, point_bub_ms> seen_squares_experiment = {};
    cata::mdarray<float, point_bub_ms> transparency_cache = {};
    cata::mdarray<bool, point_bub_ms> floor_cache = {};

    randomly_fill_transparency( transparency_cache );

    map dummy;

    const tripoint offset( 65, 65, 0 );

    const std::chrono::high_resolution_clock::time_point start1 =
        std::chrono::high_resolution_clock::now();
    for( int i = 0; i < iterations; i++ ) {
        // First the control algorithm.
        castLightAll<float, float, sight_calc, sight_check, update_light, accumulate_transparency>(
            seen_squares_control, transparency_cache, offset.xy() );
    }
    const std::chrono::high_resolution_clock::time_point end1 =
        std::chrono::high_resolution_clock::now();

    const tripoint origin( offset );
    array_of_grids_of<const float> transparency_caches;
    array_of_grids_of<float> seen_caches;
    array_of_grids_of<const bool> floor_caches;
    for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; z++ ) {
        // TODO: Give some more proper values here
        transparency_caches[z + OVERMAP_DEPTH] = &transparency_cache;
        seen_caches[z + OVERMAP_DEPTH] = &seen_squares_experiment;
        floor_caches[z + OVERMAP_DEPTH] = &floor_cache;
    }

    const std::chrono::high_resolution_clock::time_point start2 =
        std::chrono::high_resolution_clock::now();
    for( int i = 0; i < iterations; i++ ) {
        // Then the newer algorithm.
        cast_zlight<float, sight_calc, sight_check, accumulate_transparency>(
            seen_caches, transparency_caches, floor_caches, origin, 0, 1.0 );
    }
    const std::chrono::high_resolution_clock::time_point end2 =
        std::chrono::high_resolution_clock::now();

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
        print_grid_comparison( offset.xy(), transparency_cache, seen_squares_control,
                               seen_squares_experiment );
    }

    REQUIRE( passed );
}

// T, O and V are 'T'ransparent, 'O'paque and 'V'isible.
// X marks the player location, which is not set to visible by this algorithm.
static constexpr float T = LIGHT_TRANSPARENCY_OPEN_AIR;
static constexpr float O = LIGHT_TRANSPARENCY_SOLID;
static constexpr float V = LIGHT_TRANSPARENCY_OPEN_AIR;
static constexpr float X = LIGHT_TRANSPARENCY_SOLID;

static const tripoint ORIGIN( 65, 65, 11 );

struct grid_overlay {
    std::vector<std::vector<std::vector<float>>> data;
    std::vector<std::vector<std::vector<bool>>> floor;
    tripoint offset;
    float default_value;
    bool default_floor = true;

    // origin_offset is specified as the coordinates of the "camera" within the overlay.
    grid_overlay( const point &origin_offset, const float default_value ) {
        this->offset = ORIGIN - origin_offset;
        this->default_value = default_value;
    }
    grid_overlay( const tripoint &origin_offset, const float default_value ) {
        this->offset = ORIGIN - origin_offset;
        this->default_value = default_value;
    }

    int depth() const {
        return data.size();
    }
    int height() const {
        if( data.empty() ) {
            return 0;
        }
        return data[0].size();
    }
    int width() const {
        if( data.empty() || data[0].empty() ) {
            return 0;
        }
        return data[0][0].size();
    }
    tripoint get_max() const {
        return offset + tripoint( width(), height(), depth() );
    }

    float get_transparency_global( const tripoint &p ) const {
        const half_open_cuboid<tripoint> bounds( offset, get_max() );
        if( bounds.contains( p ) ) {
            return data[ p.z - offset.z ][ p.y - offset.y ][ p.x - offset.x ];
        }
        return default_value;
    }
    bool get_floor_global( const tripoint &p ) const {
        if( floor.empty() ) {
            return default_floor;
        }
        const half_open_cuboid<tripoint> bounds( offset, get_max() );
        if( bounds.contains( p ) ) {
            return data[ p.z - offset.z ][ p.y - offset.y ][ p.x - offset.x ];
        }
        return default_floor;
    }
};

static void run_spot_check( const grid_overlay &test_case, const grid_overlay &expected,
                            bool fov_3d )
{
    // Reminder to not trigger 2D shadowcasting on 3D use cases.
    if( !fov_3d ) {
        REQUIRE( test_case.depth() == 1 );
    }
    std::array<level_cache *, OVERMAP_LAYERS> caches;
    array_of_grids_of<float> seen_squares;
    array_of_grids_of<const float> transparency_cache;
    array_of_grids_of<const bool> floor_cache;

    const int upper_bound = fov_3d ? OVERMAP_LAYERS : 12;
    const int lower_bound = fov_3d ? 0 : 11;
    for( int z = lower_bound; z < upper_bound; ++z ) {
        caches[z] = new level_cache();
        seen_squares[z] = &caches[z]->seen_cache;
        transparency_cache[z] = &caches[z]->transparency_cache;
        floor_cache[z] = &caches[z]->floor_cache;
        for( int y = 0; y < MAPSIZE * SEEY; ++y ) {
            for( int x = 0; x < MAPSIZE * SEEX; ++x ) {
                caches[z]->transparency_cache[x][y] = test_case.get_transparency_global( { x, y, z } );
                caches[z]->floor_cache[x][y] = test_case.get_floor_global( { x, y, z } );
            }
        }
    }

    if( fov_3d ) {
        cast_zlight<float, sight_calc, sight_check, accumulate_transparency>( seen_squares,
                transparency_cache, floor_cache, ORIGIN - tripoint( 0, 0, OVERMAP_DEPTH ), 0, 1.0 );
        get_map().seen_cache_process_ledges( seen_squares, floor_cache, ORIGIN - tripoint( 0, 0,
                                             OVERMAP_DEPTH ) );
    } else {
        castLightAll<float, float, sight_calc, sight_check, update_light, accumulate_transparency>(
            *seen_squares[11], *transparency_cache[11], ORIGIN.xy() );

    }
    bool passed = true;
    std::ostringstream trans_grid;
    std::ostringstream expected_grid;
    std::ostringstream actual_grid;
    for( int gz = expected.offset.z; gz < expected.get_max().z; ++gz ) {
        for( int gy = expected.offset.y; gy < expected.get_max().y; ++gy ) {
            for( int gx = expected.offset.x; gx < expected.get_max().x; ++gx ) {
                trans_grid << caches[gz]->transparency_cache[gx][gy];
                expected_grid << ( expected.get_transparency_global( { gx, gy, gz } ) > 0 ? 'V' : 'O' );
                actual_grid << ( ( *seen_squares[gz] )[gx][gy] > 0 ? 'V' : 'O' );
                const float expected_trans = expected.get_transparency_global( { gx, gy, gz } );
                const float seen = ( *seen_squares[gz] )[gx][gy];
                if( ( V == expected_trans && seen == 0 ) || ( O == expected_trans && seen > 0 ) ) {
                    passed = false;
                }
            }
            trans_grid << '\n';
            expected_grid << '\n';
            actual_grid << '\n';
        }
        trans_grid << '\n';
        expected_grid << '\n';
        actual_grid << '\n';
    }

    for( int z = lower_bound; z < upper_bound; ++z ) {
        delete caches[z];
    }

    CAPTURE( fov_3d );
    INFO( "transparency:\n" << trans_grid.str() );
    INFO( "actual:\n" << actual_grid.str() );
    INFO( "expected:\n" << expected_grid.str() );
    CHECK( passed );
}

TEST_CASE( "shadowcasting_slope_inversion_regression_test", "[shadowcasting]" )
{
    grid_overlay test_case( { 7, 8 }, LIGHT_TRANSPARENCY_OPEN_AIR );
    test_case.data = { {
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
        }
    };

    grid_overlay expected_results( { 7, 8 }, LIGHT_TRANSPARENCY_OPEN_AIR );
    expected_results.data = { {
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
        }
    };

    bool fov_3d = GENERATE( true, false );
    run_spot_check( test_case, expected_results, fov_3d );
}

TEST_CASE( "shadowcasting_pillar_behavior_cardinally_adjacent", "[shadowcasting]" )
{
    grid_overlay test_case( { 1, 4 }, LIGHT_TRANSPARENCY_OPEN_AIR );
    test_case.data = { {
            {T, T, T, T, T, T, T, T, T},
            {T, T, T, T, T, T, T, T, T},
            {T, T, T, T, T, T, T, T, T},
            {T, T, T, T, T, T, T, T, T},
            {T, T, O, T, T, T, T, T, T},
            {T, T, T, T, T, T, T, T, T},
            {T, T, T, T, T, T, T, T, T},
            {T, T, T, T, T, T, T, T, T},
            {T, T, T, T, T, T, T, T, T}
        }
    };

    grid_overlay expected_results( { 1, 4 }, LIGHT_TRANSPARENCY_OPEN_AIR );
    expected_results.data = { {
            {V, V, V, V, V, V, V, O, O},
            {V, V, V, V, V, V, O, O, O},
            {V, V, V, V, V, O, O, O, O},
            {V, V, V, V, O, O, O, O, O},
            {V, X, V, O, O, O, O, O, O},
            {V, V, V, V, O, O, O, O, O},
            {V, V, V, V, V, O, O, O, O},
            {V, V, V, V, V, V, O, O, O},
            {V, V, V, V, V, V, V, O, O}
        }
    };

    bool fov_3d = GENERATE( true, false );
    run_spot_check( test_case, expected_results, fov_3d );
}

TEST_CASE( "shadowcasting_pillar_behavior_2_1_diagonal_gap", "[shadowcasting]" )
{
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    grid_overlay test_case( { 1, 1 }, LIGHT_TRANSPARENCY_OPEN_AIR );
    test_case.data = { {
            {T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T},
            {T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T},
            {T, T, T, O, T, T, T, T, T, T, T, T, T, T, T, T, T, T},
            {T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T},
            {T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T},
            {T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T},
            {T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T},
            {T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T},
            {T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T}
        }
    };

    // NOLINTNEXTLINE(cata-use-named-point-constants)
    grid_overlay expected_results( { 1, 1 }, LIGHT_TRANSPARENCY_OPEN_AIR );
    expected_results.data = { {
            {V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V},
            {V, X, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V},
            {V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V},
            {V, V, V, V, V, O, O, O, V, V, V, V, V, V, V, V, V, V},
            {V, V, V, V, V, V, O, O, O, O, O, O, O, V, V, V, V, V},
            {V, V, V, V, V, V, V, O, O, O, O, O, O, O, O, O, O, O},
            {V, V, V, V, V, V, V, V, O, O, O, O, O, O, O, O, O, O},
            {V, V, V, V, V, V, V, V, V, O, O, O, O, O, O, O, O, O},
            {V, V, V, V, V, V, V, V, V, V, O, O, O, O, O, O, O, O},
        }
    };

    bool fov_3d = GENERATE( true, false );
    run_spot_check( test_case, expected_results, fov_3d );
}

TEST_CASE( "shadowcasting_vision_along_a_wall", "[shadowcasting]" )
{
    grid_overlay test_case( { 8, 2 }, LIGHT_TRANSPARENCY_OPEN_AIR );
    test_case.data = { {
            {T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T},
            {T, O, O, O, O, O, O, O, O, O, O, O, O, O, O, O, O, T},
            {T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T},
            {T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T},
            {T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T},
            {T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T},
            {T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T},
            {T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T},
            {T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T}
        }
    };

    grid_overlay expected_results( { 8, 2 }, LIGHT_TRANSPARENCY_OPEN_AIR );
    expected_results.data = { {
            {O, O, O, O, O, O, O, O, O, O, O, O, O, O, O, O, O, O},
            {V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V},
            {V, V, V, V, V, V, V, V, X, V, V, V, V, V, V, V, V, V},
            {V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V},
            {V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V},
            {V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V},
            {V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V},
            {V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V, V}
        }
    };

    bool fov_3d = GENERATE( true, false );
    run_spot_check( test_case, expected_results, fov_3d );
}

TEST_CASE( "shadowcasting_edgewise_wall_view", "[shadowcasting]" )
{
    grid_overlay test_case( { 1, 2 }, LIGHT_TRANSPARENCY_OPEN_AIR );
    test_case.data = { {
            {T, T, O, T, T, T, T},
            {T, T, O, T, T, T, T},
            {T, T, O, O, O, T, T},
            {T, T, T, T, T, T, T},
            {T, T, T, T, T, T, T}
        }
    };

    grid_overlay expected_results( { 1, 2 }, LIGHT_TRANSPARENCY_OPEN_AIR );
    expected_results.data = { {
            {V, V, V, O, O, O, O},
            {V, V, V, O, O, O, O},
            {V, X, V, O, O, O, O},
            {V, V, V, V, O, O, O},
            {V, V, V, V, V, O, O},
            {V, V, V, V, V, V, O}
        }
    };

    bool fov_3d = GENERATE( true, false );
    run_spot_check( test_case, expected_results, fov_3d );
}

TEST_CASE( "shadowcasting_opaque_floors", "[shadowcasting]" )
{
    grid_overlay test_case( { 2, 2, 1 }, LIGHT_TRANSPARENCY_OPEN_AIR );
    test_case.data = {
        {
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T}
        },
        {
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T}
        },
        {
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T}
        }
    };

    grid_overlay expected_results( { 2, 2, 1 }, LIGHT_TRANSPARENCY_OPEN_AIR );
    expected_results.data = {
        {
            {O, O, O, O, O},
            {O, O, O, O, O},
            {O, O, O, O, O},
            {O, O, O, O, O},
            {O, O, O, O, O}
        },
        {
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, X, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V}
        },
        {
            {O, O, O, O, O},
            {O, O, O, O, O},
            {O, O, O, O, O},
            {O, O, O, O, O},
            {O, O, O, O, O}
        }
    };

    run_spot_check( test_case, expected_results, true );
}

TEST_CASE( "shadowcasting_transparent_floors", "[shadowcasting]" )
{
    grid_overlay test_case( { 2, 2, 1 }, LIGHT_TRANSPARENCY_OPEN_AIR );
    test_case.data = {
        {
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T}
        },
        {
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T}
        },
        {
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T}
        }
    };
    test_case.default_floor = false;

    grid_overlay expected_results( { 2, 2, 1 }, LIGHT_TRANSPARENCY_OPEN_AIR );
    expected_results.data = {
        {
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V}
        },
        {
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, X, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V}
        },
        {
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V}
        }
    };

    run_spot_check( test_case, expected_results, true );
}

// From origin looking out, we should check every combination of 2x2 transparent and opaque patterns.
// T T  O O
// T T  O O
//
// T T  T T  O T  T O
// O T  T O  T T  T T
//
// T T  O T  T O  O T  T O  O O
// O O  O T  O T  T O  T O  T T
//
// O T  T O  O O  O O
// O O  O O  O T  T O

TEST_CASE( "shadowcasting_floating_wall", "[shadowcasting]" )
{
    grid_overlay test_case( { 2, 16, 3 }, LIGHT_TRANSPARENCY_OPEN_AIR );
    test_case.data = {
        {
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T}
        },
        {
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T}
        },
        {
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, O, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T}
        },
        {
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, T, T, T},
            {T, T, X, T, T}
        }
    };
    test_case.default_floor = false;

    grid_overlay expected_results( { 2, 16, 3 }, LIGHT_TRANSPARENCY_OPEN_AIR );
    expected_results.data = {
        {
            {O, O, O, O, O},
            {O, O, O, O, O},
            {O, O, O, O, O},
            {V, O, O, O, V},
            {V, O, O, O, V},
            {V, O, O, O, V},
            {V, O, O, O, V},
            {V, O, O, O, V},
            {V, V, O, V, V},
            {V, V, O, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V}
        },
        {
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, O, O, O, V},
            {V, V, O, V, V},
            {V, V, O, V, V},
            {V, V, O, V, V},
            {V, V, O, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V}
        },
        {
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V}
        },
        {
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, V, V, V},
            {V, V, X, V, V}
        }
    };

    run_spot_check( test_case, expected_results, true );
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

TEST_CASE( "shadowcasting_3d_performance", "[.]" )
{
    shadowcasting_3d_benchmark( 10000 );
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
