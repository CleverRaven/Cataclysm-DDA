#include <iomanip>

#include "map.h"
#include "field.h"
#include "map_test_case.h"

tripoint map_test_case::get_origin()
{
    if( origin ) {
        return *origin;
    }

    cata::optional<point> res = cata::nullopt;

    if( anchor_char ) {
        for_each_tile( tripoint_zero, [&]( map_test_case::tile & t ) {
            if( t.sc == *anchor_char ) {
                if( res ) {
                    FAIL( "Origin char '" << *anchor_char << "' is found more than once in setup" );
                }
                res = t.p.xy();
            }
        } );

        if( !res.has_value() ) {
            FAIL( "Origin char '" << *anchor_char << "' is not found in setup" );
        }
    } else {
        res = point_zero;
    }

    origin = anchor_map_pos - tripoint( *res, 0 );
    return *origin;
}

int map_test_case::get_width() const
{
    return setup[0].size();
}

int map_test_case::get_height() const
{
    return setup.size();
}

void map_test_case::for_each_tile( tripoint tmp_origin,
                                   const std::function<void( map_test_case::tile & )> &callback ) const
{
    int width = get_width();
    int height = get_height();

    tile tile;

    for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
            tile.sc = setup[y][x];
            tile.ec = expected_results[y][x];
            tile.p = tmp_origin + point( x, y );
            tile.p_local = point( x, y );
            callback( tile );
        }
    }
}

void map_test_case::for_each_tile( const std::function<void( tile )> &callback )
{
    for_each_tile<void>( callback );
}

std::vector<std::vector<std::string>> map_test_case::map_tiles_str(
                                       const std::function<void( tile, std::ostringstream & )> &callback )
{
    return map_tiles<std::string>( [&]( tile t ) {
        std::ostringstream out;
        callback( t, out );
        return out.str();
    } );
}

void map_test_case::do_internal_checks()
{
    if( checks_complete ) {
        return;
    }

    int height = get_height();
    REQUIRE( height > 0 );
    REQUIRE( static_cast<size_t>( height ) == expected_results.size() );
    int width = get_width();
    REQUIRE( width > 0 );

    for( const std::string &line : setup ) {
        REQUIRE( line.size() == static_cast<size_t>( width ) );
    }

    for( const std::string &line : expected_results ) {
        REQUIRE( line.size() == static_cast<size_t>( width ) );
    }

    checks_complete = true;
}

void map_test_case::transpose()
{
    origin = cata::nullopt;
    checks_complete = false;

    auto transpose = []( std::vector<std::string> v ) {
        if( v.empty() ) {
            return;
        }
        std::vector<std::string> new_v( v[0].size() );

        for( const std::string &col : v ) {
            for( size_t y = 0; y < new_v.size(); ++y ) {
                new_v[y].push_back( col.at( y ) );
            }
        }

        v = new_v;
    };

    transpose( setup );
    transpose( expected_results );
};

void map_test_case::reflect_x()
{
    origin = cata::nullopt;
    checks_complete = false;

    for( std::string &s : setup ) {
        std::reverse( s.begin(), s.end() );
    }
    for( std::string &s : expected_results ) {
        std::reverse( s.begin(), s.end() );
    }
};

void map_test_case::reflect_y()
{
    origin = cata::nullopt;
    checks_complete = false;

    std::reverse( setup.begin(), setup.end() );
    std::reverse( expected_results.begin(), expected_results.end() );
}

void map_test_case::set_anchor_char_from( const std::set<char> &chars )
{
    for_each_tile( tripoint_zero, [&]( tile t ) {
        if( chars.count( t.sc ) ) {
            anchor_char = t.sc;
        }
    } );
}

std::string map_test_case::generate_transform_combinations()
{
    std::ostringstream out;
    if( GENERATE( false, true ) ) {
        out << "__transpose";
        transpose();
    }
    if( GENERATE( false, true ) ) {
        out << "__reflect_x";
        reflect_x();
    }
    if( GENERATE( false, true ) ) {
        out << "__reflect_y";
        reflect_y();
    }
    return out.str();
}

void map_test_case::validate_anchor_point( tripoint p )
{
    INFO( "checking point: " << p );
    tripoint origin_p = get_origin();
    REQUIRE( origin_p.z == p.z );
    REQUIRE( p == anchor_map_pos );
    // offset is anchor_map_pos in `map_test_case` local coords
    tripoint offset = p - origin_p;
    REQUIRE( offset.y >= 0 );
    REQUIRE( offset.y < get_height() );
    REQUIRE( offset.x >= 0 );
    REQUIRE( offset.x < get_width() );

    char setup_anchor_char = setup[offset.y][offset.x];
    REQUIRE( anchor_char );
    REQUIRE( setup_anchor_char == *anchor_char );
}

std::string map_test_case_common::printers::format_2d_array( const
        std::vector<std::vector<std::string>> &info )
{
    size_t max = 0;
    for( const auto &row : info ) {
        for( const std::string &el : row ) {
            max = std::max( max, el.size() );
        }
    }
    std::ostringstream buf;

    for( const auto &row : info ) {
        for( const std::string &el : row ) {
            buf << std::setw( max ) << el;
        }
        buf << '\n';
    }

    return buf.str();
}

std::string print_and_format_helper( map_test_case &t, int zshift,
                                     std::function<void( const tripoint &p, std::ostringstream &out )> print_tile )
{
    tripoint shift = tripoint( point_zero, zshift );
    return map_test_case_common::printers::format_2d_array(
    t.map_tiles_str( [&]( map_test_case::tile t, std::ostringstream & out ) {
        print_tile( t.p + shift, out );
    } ) );
}

std::string map_test_case_common::printers::fields( map_test_case &t, int zshift )
{
    map &here = get_map();
    return print_and_format_helper( t, zshift, [&]( auto p, auto & out ) {
        bool first = true;
        for( auto &pr : here.field_at( p ) ) {
            out << ( first ? " " : "," ) << pr.second.name();
            first = false;
        }
    } );
}

std::string map_test_case_common::printers::transparency( map_test_case &t, int zshift )
{
    const level_cache &cache = get_map().access_cache( t.get_origin().z + zshift );
    return print_and_format_helper( t, zshift, [&]( auto p, auto & out ) {
        out << std::setprecision( 3 ) << cache.transparency_cache[p.x][p.y] << ' ';
    } );
}

std::string map_test_case_common::printers::seen( map_test_case &t, int zshift )
{
    const auto &cache = get_map().access_cache( t.get_origin().z + zshift ).seen_cache;
    return print_and_format_helper( t, zshift, [&]( auto p, auto & out ) {
        out << std::setprecision( 3 ) << cache[p.x][p.y] << ' ';
    } );
}

std::string map_test_case_common::printers::lm( map_test_case &t, int zshift )
{
    const level_cache &cache = get_map().access_cache( t.get_origin().z + zshift );
    return print_and_format_helper( t, zshift, [&]( auto p, auto & out ) {
        out << cache.lm[p.x][p.y].to_string() << ' ';
    } );
}

std::string map_test_case_common::printers::apparent_light( map_test_case &t, int zshift )
{
    const level_cache &cache = get_map().access_cache( t.get_origin().z + zshift );
    return print_and_format_helper( t, zshift, [&]( auto p, auto & out ) {
        out << std::setprecision( 3 ) << map::apparent_light_helper( cache, p ).apparent_light << ' ';
    } );
}

std::string map_test_case_common::printers::obstructed( map_test_case &t, int zshift )
{
    const level_cache &cache = get_map().access_cache( t.get_origin().z + zshift );
    return print_and_format_helper( t, zshift, [&]( auto p, auto & out ) {
        bool obs = map::apparent_light_helper( cache, p ).obstructed;
        out << ( obs ? '#' : '.' );
    } );
}

std::string map_test_case_common::printers::floor( map_test_case &t, int zshift )
{
    const level_cache &cache = get_map().access_cache( t.get_origin().z + zshift );
    return print_and_format_helper( t, zshift, [&]( auto p, auto & out ) {
        out << ( cache.floor_cache[p.x][p.y] ? '#' : '.' );
    } );
}

std::string map_test_case_common::printers::expected( map_test_case &t )
{
    return format_2d_array( t.map_tiles_str( [&]( map_test_case::tile t, std::ostringstream & out ) {
        out << t.ec;
    } ) );
}

// test some of the map_test_case helpers
TEST_CASE( "map_test_case_common", "[map_test_case]" )
{
    using namespace map_test_case_common;
    SECTION( "|| and &&" ) {
        std::function<bool( int )> a = []( int arg ) {
            return arg > 0;
        };
        std::function<bool( int )> b = []( int arg ) {
            return arg < 5;
        };

        auto a_and_b = a && b;
        auto a_or_b = a || b;

        CHECK( a_and_b( 1 ) );
        CHECK_FALSE( a_and_b( -1 ) );

        CHECK( a_or_b( -1 ) );
        CHECK( a_or_b( 1 ) );
        CHECK( a_or_b( 6 ) );
    }
    SECTION( "a && a && a" ) {
        std::function<bool()> a = []() {
            return true;
        };
        std::function<bool()> b = []() {
            return false;
        };
        auto aaa = a && a && a;
        auto aab = a && a && b;
        CHECK( aaa() );
        CHECK_FALSE( aab() );
    }
    SECTION( "inc + inc + inc" ) {
        int counter = 0;
        std::function<void()> inc = [&]() {
            counter++;
        };
        auto f = inc + inc + inc;
        CHECK( counter == 0 );
        f();
        CHECK( counter == 3 );
    }
}

// checks the consistency of all transformations
// and valid coordinate calculation
TEST_CASE( "map_test_case_transform_consistency", "[map_test_case]" )
{
    using namespace map_test_case_common;
    using mtc = map_test_case;

    mtc tst;
    tst.anchor_char = '5';
    tst.setup = {
        "123",
        "456",
        "789",
    };
    tst.expected_results = {
        "123",
        "456",
        "789",
    };

    SECTION( tst.generate_transform_combinations() ) {
    }

    // assumes tst.anchor_map_pos == tripoint_zero
    CHECK( tst.get_origin() == tripoint{-1, -1, 0} );
    tst.validate_anchor_point( {0, 0, 0} );

    std::set<tripoint> tiles;
    tst.for_each_tile( [&]( mtc::tile t ) {
        tiles.emplace( t.p );
    } );

    CHECK( tiles.size() == 9 );

    std::vector<tripoint> neigh_vec(
        std::begin( eight_horizontal_neighbors ),
        std::end( eight_horizontal_neighbors )
    );
    neigh_vec.push_back( tripoint_zero );
    CHECK_THAT( std::vector<tripoint>( tiles.begin(), tiles.end() ),
                Catch::UnorderedEquals( neigh_vec ) );

    tst.for_each_tile( [&]( mtc::tile t ) {
        CHECK( t.ec == t.sc );
        CHECK( tiles.count( t.p ) == 1 );
    } );
}

// checks transformations for non-square field
TEST_CASE( "map_test_case_transform_non_square", "[map_test_case]" )
{
    using namespace map_test_case_common;
    using mtc = map_test_case;

    mtc tst;
    tst.anchor_char = '1';
    tst.setup = {
        "12345"
    };
    tst.expected_results = {
        "abcde"
    };

    SECTION( tst.generate_transform_combinations() ) {
    }
    tst.validate_anchor_point( {0, 0, 0} );

    CAPTURE( tst.get_width(), tst.get_height() );
    CHECK(
        ( ( tst.get_width() == 5 && tst.get_height() == 1 ) ||
          ( tst.get_width() == 1 && tst.get_height() == 5 ) )
    );

    CAPTURE( tst.get_origin() );
    CHECK( std::set<tripoint> {
        tripoint_zero, {-4, 0, 0}, {0, -4, 0}
    } .count( tst.get_origin() ) );

    tst.for_each_tile( [&]( mtc::tile t ) {
        CHECK( t.ec - 'a' == t.sc - '1' );
    } );
}
