#include <functional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include "cata_catch.h"
#include "cata_generators.h"
#include "coordinates.h"
#include "map_scale_constants.h"
#include "point.h"

static constexpr int num_trials = 5;

static_assert( point::dimension == 2 );
static_assert( tripoint::dimension == 3 );
static_assert( point_abs_omt::dimension == 2 );
static_assert( tripoint_abs_omt::dimension == 3 );

// Out of bounds coords can be implicitly constructed from inbounds ones. This is used
// to ensure a return type is NOT an inbounds one, before it can be converted.
#define assert_not_ib(p) \
    (([](auto&& _p) { \
        using _t = decltype(_p); \
        static_assert(!std::decay_t<_t>::is_inbounds); \
        return std::forward<_t>(_p); } \
     )(p))

// Code moved from the obsolete coodinates_conversions.cpp file, as its only remaining
// usage is in these tests.

static int divide( int v, int m )
{
    if( v >= 0 ) {
        return v / m;
    }
    return ( v - m + 1 ) / m;
}

static int divide( int v, int m, int &r )
{
    const int result = divide( v, m );
    r = v - result * m;
    return result;
}

static point omt_to_om_copy( const point &p )
{
    return point( divide( p.x, OMAPX ), divide( p.y, OMAPY ) );
}

static tripoint omt_to_om_copy( const tripoint &p )
{
    return tripoint( divide( p.x, OMAPX ), divide( p.y, OMAPY ), p.z );
}

static point omt_to_om_remain( int &x, int &y )
{
    return point( divide( x, OMAPX, x ), divide( y, OMAPY, y ) );
}

static point omt_to_om_remain( point &p )
{
    return omt_to_om_remain( p.x, p.y );
}

static point sm_to_omt_copy( const point &p )
{
    return point( divide( p.x, 2 ), divide( p.y, 2 ) );
}

static point sm_to_omt_remain( int &x, int &y )
{
    return point( divide( x, 2, x ), divide( y, 2, y ) );
}

static point sm_to_omt_remain( point &p )
{
    return sm_to_omt_remain( p.x, p.y );
}

static point sm_to_om_copy( const point &p )
{
    return point( divide( p.x, 2 * OMAPX ), divide( p.y, 2 * OMAPY ) );
}

static point sm_to_om_remain( int &x, int &y )
{
    return point( divide( x, 2 * OMAPX, x ), divide( y, 2 * OMAPY, y ) );
}

static point sm_to_om_remain( point &p )
{
    return sm_to_om_remain( p.x, p.y );
}

static point omt_to_sm_copy( const point &p )
{
    return point( p.x * 2, p.y * 2 );
}

static point om_to_sm_copy( const point &p )
{
    return point( p.x * 2 * OMAPX, p.y * 2 * OMAPX );
}

static point ms_to_sm_copy( const point &p )
{
    return point( divide( p.x, SEEX ), divide( p.y, SEEY ) );
}

// Note: this gives you the map square coordinates of the top-left corner
// of the given submap.
static point sm_to_ms_copy( const point &p )
{
    return point( p.x * SEEX, p.y * SEEY );
}

static point ms_to_omt_copy( const point &p )
{
    return point( divide( p.x, SEEX * 2 ), divide( p.y, SEEY * 2 ) );
}

static point ms_to_omt_remain( int &x, int &y )
{
    return point( divide( x, SEEX * 2, x ), divide( y, SEEY * 2, y ) );
}

static point ms_to_omt_remain( point &p )
{
    return ms_to_omt_remain( p.x, p.y );
}

static tripoint omt_to_seg_copy( const tripoint &p )
{
    return tripoint( divide( p.x, SEG_SIZE ), divide( p.y, SEG_SIZE ), p.z );
}

// End of moved code.

TEST_CASE( "coordinate_strings", "[point][coords][nogame]" )
{
    CHECK( point_abs_omt( point( 3, 4 ) ).to_string() == "(3,4)" );

    SECTION( "coord_point_matches_point" ) {
        point p = GENERATE( take( num_trials, random_points() ) );
        point_abs_ms cp( p );
        CHECK( p.to_string() == cp.to_string() );
    }
}

TEST_CASE( "coordinate_operations", "[point][coords][nogame]" )
{
    SECTION( "construct_from_raw_point" ) {
        point p = GENERATE( take( num_trials, random_points() ) );
        point_abs_ms cp( p );
        CHECK( cp.x() == p.x );
        CHECK( cp.y() == p.y );
    }

    SECTION( "construct_from_raw_tripoint" ) {
        tripoint p = GENERATE( take( num_trials, random_tripoints() ) );
        tripoint_abs_ms cp( p );
        CHECK( cp.x() == p.x );
        CHECK( cp.y() == p.y );
        CHECK( cp.z() == p.z );
    }

    SECTION( "construct_from_values" ) {
        tripoint p = GENERATE( take( num_trials, random_tripoints() ) );
        {
            point_abs_ms cp( p.x, p.y );
            CHECK( cp.x() == p.x );
            CHECK( cp.y() == p.y );
        }
        {
            tripoint_abs_ms cp( p.x, p.y, p.z );
            CHECK( cp.x() == p.x );
            CHECK( cp.y() == p.y );
            CHECK( cp.z() == p.z );
        }
    }

    SECTION( "addition" ) {
        tripoint t0 = GENERATE( take( num_trials, random_tripoints() ) );
        point p0 = t0.xy();
        tripoint t1 = GENERATE( take( num_trials, random_tripoints() ) );
        point p1 = t1.xy();
        CAPTURE( p0, p1 );
        tripoint_abs_ms abst0( t0 );
        tripoint_bub_ms_ib bub_ibt0 = tripoint_bub_ms_ib::make_unchecked( t0 );
        point_abs_ms abs0( p0 );
        point_bub_ms_ib bub_ib0 = point_bub_ms_ib::make_unchecked( p0 );
        point_rel_ms rel0( p0 );
        point_rel_ms rel1( p1 );
        SECTION( "rel + rel -> rel" ) {
            point_rel_ms sum = rel0 + rel1;
            CHECK( sum.raw() == p0 + p1 );
        }
        SECTION( "abs + rel -> abs" ) {
            point_abs_ms sum = abs0 + rel1;
            CHECK( sum.raw() == p0 + p1 );
            tripoint_abs_ms sum_t = abst0 + rel1;
            CHECK( sum_t.raw() == t0 + p1 );
        }
        SECTION( "bub_ib + rel -> bub" ) {
            point_bub_ms sum = assert_not_ib( bub_ib0 + rel1 );
            CHECK( sum.raw() == p0 + p1 );
            tripoint_bub_ms sum_t = assert_not_ib( bub_ibt0 + rel1 );
            CHECK( sum_t.raw() == t0 + p1 );
        }
        SECTION( "abs + raw -> abs" ) {
            point_abs_ms sum = abs0 + p1;
            CHECK( sum.raw() == p0 + p1 );
        }
        SECTION( "bub_ib + raw -> bub" ) {
            point_bub_ms sum = assert_not_ib( bub_ib0 + p1 );
            CHECK( sum.raw() == p0 + p1 );
        }
        SECTION( "rel + abs -> abs" ) {
            point_abs_ms sum = rel1 + abs0;
            CHECK( sum.raw() == p0 + p1 );
        }
        SECTION( "rel + bub_ib -> bub" ) {
            point_bub_ms sum = assert_not_ib( rel1 + bub_ib0 );
            CHECK( sum.raw() == p0 + p1 );
            tripoint_bub_ms sum_t = assert_not_ib( rel1 + bub_ibt0 );
            CHECK( sum_t.raw() == t0 + p1 );
        }
        SECTION( "raw + abs -> abs" ) {
            point_abs_ms sum = p1 + abs0;
            CHECK( sum.raw() == p0 + p1 );
            tripoint_abs_ms sum_t = t1 + abs0;
            CHECK( sum_t.raw() == p0 + t1 );
        }
        SECTION( " raw + bub_ib -> bub" ) {
            point_bub_ms sum = assert_not_ib( p1 + bub_ib0 );
            CHECK( sum.raw() == p0 + p1 );
            tripoint_bub_ms sum_t = assert_not_ib( t1 + bub_ib0 );
            CHECK( sum_t.raw() == p0 + t1 );
        }
        SECTION( "rel += rel" ) {
            rel0 += rel1;
            CHECK( rel0.raw() == p0 + p1 );
        }
        SECTION( "abs += rel" ) {
            abs0 += rel1;
            CHECK( abs0.raw() == p0 + p1 );
        }
        SECTION( "abs += raw" ) {
            abs0 += p1;
            CHECK( abs0.raw() == p0 + p1 );
        }
    }

    SECTION( "subtraction" ) {
        tripoint t0 = GENERATE( take( num_trials, random_tripoints() ) );
        point p0 = t0.xy();
        point p1 = GENERATE( take( num_trials, random_points() ) );
        CAPTURE( p0, p1 );
        tripoint_abs_ms abst0( t0 );
        point_abs_ms abs0( p0 );
        point_bub_ms_ib bub_ib0 = point_bub_ms_ib::make_unchecked( p0 );
        point_abs_ms abs1( p1 );
        point_rel_ms rel0( p0 );
        point_rel_ms rel1( p1 );
        SECTION( "rel - rel -> rel" ) {
            point_rel_ms diff = rel0 - rel1;
            CHECK( diff.raw() == p0 - p1 );
        }
        SECTION( "abs - rel -> abs" ) {
            point_abs_ms diff = abs0 - rel1;
            CHECK( diff.raw() == p0 - p1 );
        }
        SECTION( "bub_ib - rel -> bub" ) {
            point_bub_ms diff = assert_not_ib( bub_ib0 - rel1 );
            CHECK( diff.raw() == p0 - p1 );
        }
        SECTION( "abs - raw -> abs" ) {
            point_abs_ms diff = abs0 - p1;
            CHECK( diff.raw() == p0 - p1 );
        }
        SECTION( "bub_ib - raw -> bub" ) {
            point_bub_ms diff = assert_not_ib( bub_ib0 - p1 );
            CHECK( diff.raw() == p0 - p1 );
        }
        SECTION( "abs - abs -> rel" ) {
            point_rel_ms diff0 = abs0 - abs1;
            CHECK( diff0.raw() == p0 - p1 );
            tripoint_rel_ms diff1 = abst0 - abs1;
            CHECK( diff1.raw() == t0 - p1 );
        }
        SECTION( "rel -= rel" ) {
            rel0 -= rel1;
            CHECK( rel0.raw() == p0 - p1 );
        }
        SECTION( "abs -= rel" ) {
            abs0 -= rel1;
            CHECK( abs0.raw() == p0 - p1 );
        }
        SECTION( "abs -= raw" ) {
            abs0 -= p1;
            CHECK( abs0.raw() == p0 - p1 );
        }
    }
}

TEST_CASE( "coordinate_comparison", "[point][coords][nogame]" )
{
    SECTION( "compare_points" ) {
        point p0 = GENERATE( take( num_trials, random_points() ) );
        point p1 = GENERATE( take( num_trials, random_points() ) );
        CAPTURE( p0, p1 );
        point_rel_ms cp0( p0 );
        point_rel_ms cp1( p1 );
        CAPTURE( cp0, cp1 );

        CHECK( ( p0 < p1 ) == ( cp0 < cp1 ) );
        CHECK( ( p0 == p1 ) == ( cp0 == cp1 ) );
        CHECK( cp0 == cp0 );
        CHECK( !( cp0 != cp0 ) );
    }

    SECTION( "compare_tripoints" ) {
        tripoint p0 = GENERATE( take( num_trials, random_tripoints() ) );
        tripoint p1 = GENERATE( take( num_trials, random_tripoints() ) );
        CAPTURE( p0, p1 );
        tripoint_rel_ms cp0( p0 );
        tripoint_rel_ms cp1( p1 );
        CAPTURE( cp0, cp1 );

        CHECK( ( p0 < p1 ) == ( cp0 < cp1 ) );
        CHECK( ( p0 == p1 ) == ( cp0 == cp1 ) );
        CHECK( cp0 == cp0 );
        CHECK( !( cp0 != cp0 ) );
    }
}

TEST_CASE( "coordinate_hash", "[point][coords][nogame]" )
{
    SECTION( "point_hash" ) {
        point p = GENERATE( take( num_trials, random_points() ) );
        point_abs_ms cp( p );
        CHECK( std::hash<point_abs_ms>()( cp ) == std::hash<point>()( p ) );
    }

    SECTION( "tripoint_hash" ) {
        tripoint p = GENERATE( take( num_trials, random_tripoints() ) );
        tripoint_abs_ms cp( p );
        CHECK( std::hash<tripoint_abs_ms>()( cp ) == std::hash<tripoint>()( p ) );
    }
}

TEST_CASE( "coordinate_conversion_consistency", "[point][coords][nogame]" )
{
    // Verifies that the new coord_point-based conversions yield the same
    // results as the legacy conversion functions.
    SECTION( "omt_to_om_point" ) {
        point p = GENERATE( take( num_trials, random_points() ) );
        CAPTURE( p );
        point_abs_om new_conversion = project_to<coords::om>( point_abs_omt( p ) );
        point old_conversion = omt_to_om_copy( p );
        CHECK( old_conversion == new_conversion.raw() );
    }

    SECTION( "omt_to_om_tripoint" ) {
        tripoint p = GENERATE( take( num_trials, random_tripoints() ) );
        CAPTURE( p );
        tripoint_abs_om new_conversion = project_to<coords::om>( tripoint_abs_omt( p ) );
        tripoint old_conversion = omt_to_om_copy( p );
        CHECK( old_conversion == new_conversion.raw() );
    }

    SECTION( "omt_to_om_remain_point" ) {
        point p = GENERATE( take( num_trials, random_points() ) );
        CAPTURE( p );
        point_abs_om new_conversion;
        point_om_omt remainder;
        std::tie( new_conversion, remainder ) = project_remain<coords::om>( point_abs_omt( p ) );
        point old_conversion = omt_to_om_remain( p );
        CHECK( old_conversion == new_conversion.raw() );
        CHECK( p == remainder.raw() );
    }

    SECTION( "sm_to_omt_point" ) {
        point p = GENERATE( take( num_trials, random_points() ) );
        CAPTURE( p );
        point_abs_omt new_conversion = project_to<coords::omt>( point_abs_sm( p ) );
        point old_conversion = sm_to_omt_copy( p );
        CHECK( old_conversion == new_conversion.raw() );
    }

    SECTION( "sm_to_omt_remain_point" ) {
        point p = GENERATE( take( num_trials, random_points() ) );
        CAPTURE( p );
        point_abs_omt new_conversion;
        point_omt_sm remainder;
        std::tie( new_conversion, remainder ) = project_remain<coords::omt>( point_abs_sm( p ) );
        point old_conversion = sm_to_omt_remain( p );
        CHECK( old_conversion == new_conversion.raw() );
        CHECK( p == remainder.raw() );
    }

    SECTION( "sm_to_om_point" ) {
        point p = GENERATE( take( num_trials, random_points() ) );
        CAPTURE( p );
        point_abs_om new_conversion = project_to<coords::om>( point_abs_sm( p ) );
        point old_conversion = sm_to_om_copy( p );
        CHECK( old_conversion == new_conversion.raw() );
    }

    SECTION( "sm_to_om_remain_point" ) {
        point p = GENERATE( take( num_trials, random_points() ) );
        CAPTURE( p );
        point_abs_om new_conversion;
        point_om_sm remainder;
        std::tie( new_conversion, remainder ) = project_remain<coords::om>( point_abs_sm( p ) );
        point old_conversion = sm_to_om_remain( p );
        CHECK( old_conversion == new_conversion.raw() );
        CHECK( p == remainder.raw() );
    }

    SECTION( "omt_to_sm_point" ) {
        point p = GENERATE( take( num_trials, random_points() ) );
        CAPTURE( p );
        point_abs_sm new_conversion = project_to<coords::sm>( point_abs_omt( p ) );
        point old_conversion = omt_to_sm_copy( p );
        CHECK( old_conversion == new_conversion.raw() );
    }

    SECTION( "om_to_sm_point" ) {
        point p = GENERATE( take( num_trials, random_points() ) );
        CAPTURE( p );
        point_abs_sm new_conversion = project_to<coords::sm>( point_abs_om( p ) );
        point old_conversion = om_to_sm_copy( p );
        CHECK( old_conversion == new_conversion.raw() );
    }

    SECTION( "ms_to_sm_point" ) {
        point p = GENERATE( take( num_trials, random_points() ) );
        CAPTURE( p );
        point_abs_sm new_conversion = project_to<coords::sm>( point_abs_ms( p ) );
        point old_conversion = ms_to_sm_copy( p );
        CHECK( old_conversion == new_conversion.raw() );
    }

    SECTION( "ms_to_sm_point_ib" ) {
        point p = GENERATE( take( num_trials, random_points() ) ).abs();
        CAPTURE( p );
        point_bub_sm_ib new_conversion = project_to<coords::sm>( point_bub_ms_ib::make_unchecked( p ) );
        point old_conversion = ms_to_sm_copy( p );
        CHECK( old_conversion == new_conversion.raw() );
    }

    SECTION( "sm_to_ms_point" ) {
        point p = GENERATE( take( num_trials, random_points() ) );
        CAPTURE( p );
        point_abs_ms new_conversion = project_to<coords::ms>( point_abs_sm( p ) );
        point old_conversion = sm_to_ms_copy( p );
        CHECK( old_conversion == new_conversion.raw() );
    }

    SECTION( "sm_to_ms_point_ib" ) {
        point p = GENERATE( take( num_trials, random_points() ) ).abs();
        CAPTURE( p );
        point_bub_ms_ib new_conversion = project_to<coords::ms>( point_bub_sm_ib::make_unchecked( p ) );
        point old_conversion = sm_to_ms_copy( p );
        CHECK( old_conversion == new_conversion.raw() );
    }

    SECTION( "ms_to_omt_point" ) {
        point p = GENERATE( take( num_trials, random_points() ) );
        CAPTURE( p );
        point_abs_omt new_conversion = project_to<coords::omt>( point_abs_ms( p ) );
        point old_conversion = ms_to_omt_copy( p );
        CHECK( old_conversion == new_conversion.raw() );
    }

    SECTION( "ms_to_omt_remain_point" ) {
        point p = GENERATE( take( num_trials, random_points() ) );
        CAPTURE( p );
        point_abs_omt new_conversion;
        point_omt_ms remainder;
        std::tie( new_conversion, remainder ) = project_remain<coords::omt>( point_abs_ms( p ) );
        point old_conversion = ms_to_omt_remain( p );
        CHECK( old_conversion == new_conversion.raw() );
        CHECK( p == remainder.raw() );
    }

    SECTION( "omt_to_seg_tripoint" ) {
        tripoint p = GENERATE( take( num_trials, random_tripoints() ) );
        CAPTURE( p );
        tripoint_abs_seg new_conversion = project_to<coords::seg>( tripoint_abs_omt( p ) );
        tripoint old_conversion = omt_to_seg_copy( p );
        CHECK( old_conversion == new_conversion.raw() );
    }
}

TEST_CASE( "combine_is_opposite_of_remain", "[point][coords][nogame]" )
{
    SECTION( "point_point" ) {
        point p = GENERATE( take( num_trials, random_points() ) );
        CAPTURE( p );
        point_abs_sm orig( p );
        point_abs_om quotient;
        point_om_sm remainder;
        std::tie( quotient, remainder ) = project_remain<coords::om>( orig );
        point_abs_sm recombined = project_combine( quotient, remainder );
        CHECK( recombined == orig );
    }
    SECTION( "tripoint_point" ) {
        tripoint p = GENERATE( take( num_trials, random_tripoints() ) );
        CAPTURE( p );
        tripoint_abs_sm orig( p );
        tripoint_abs_om quotient;
        point_om_sm remainder;
        std::tie( quotient, remainder ) = project_remain<coords::om>( orig );
        tripoint_abs_sm recombined = project_combine( quotient, remainder );
        CHECK( recombined == orig );
    }
    SECTION( "point_tripoint" ) {
        tripoint p = GENERATE( take( num_trials, random_tripoints() ) );
        CAPTURE( p );
        tripoint_abs_sm orig( p );
        point_abs_om quotient;
        tripoint_om_sm remainder;
        std::tie( quotient, remainder ) = project_remain<coords::om>( orig );
        tripoint_abs_sm recombined = project_combine( quotient, remainder );
        CHECK( recombined == orig );
    }
    SECTION( "tripoint_point_ib" ) {
        tripoint p = GENERATE( take( num_trials, random_tripoints() ) ).abs();
        CAPTURE( p );
        tripoint_bub_ms_ib orig = tripoint_bub_ms_ib::make_unchecked( p );
        tripoint_bub_sm_ib quotient;
        point_sm_ms_ib remainder;
        std::tie( quotient, remainder ) = project_remain<coords::sm>( orig );
        tripoint_bub_ms_ib recombined = project_combine( quotient, remainder );
        CHECK( recombined == orig );
    }
    SECTION( "tripoint_point_implicit_ib" ) {
        tripoint p = GENERATE( take( num_trials, random_tripoints() ) );
        CAPTURE( p );
        tripoint_bub_ms orig( p );
        tripoint_bub_sm quotient;
        point_sm_ms_ib remainder;
        std::tie( quotient, remainder ) = project_remain<coords::sm>( orig );
        tripoint_bub_ms recombined = assert_not_ib( project_combine( quotient, remainder ) );
        CHECK( recombined == orig );
    }
}

TEST_CASE( "coord_point_distances", "[point][coords][nogame]" )
{
    point_abs_omt p0;
    point_abs_omt p1( 10, 10 );
    tripoint_abs_omt t0;
    tripoint_abs_omt t1( 10, 10, 10 );

    SECTION( "square" ) {
        CHECK( square_dist( p0, p1 ) == 10 );
        CHECK( square_dist( t0, t1 ) == 10 );
    }

    SECTION( "trig" ) {
        CHECK( trig_dist( p0, p1 ) == 14 ); // int(10*sqrt(2))
        CHECK( trig_dist( t0, t1 ) == 17 ); // int(10*sqrt(3))
    }

    SECTION( "manhattan" ) {
        CHECK( manhattan_dist( p0, p1 ) == 20 );
    }
}

TEST_CASE( "coord_point_midpoint", "[point][coords][nogame]" )
{
    point_abs_omt p0( 2, 2 );
    point_abs_omt p1( 8, 17 );
    tripoint_abs_omt t0( 2, 2, 2 );
    tripoint_abs_omt t1( 8, 17, 5 );

    CHECK( midpoint( p0, p1 ) == point_abs_omt( 5, 9 ) );
    CHECK( midpoint( t0, t1 ) == tripoint_abs_omt( 5, 9, 3 ) );
}
