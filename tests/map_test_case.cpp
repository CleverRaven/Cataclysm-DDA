#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <string>
#include <utility>

#include "field.h"
#include "level_cache.h"
#include "map.h"
#include "map_test_case.h"
#include "mdarray.h"
#include "shadowcasting.h"

tripoint_bub_ms map_test_case::get_origin()
{
    if( origin ) {
        return *origin;
    }

    std::optional<point_bub_ms> res = std::nullopt;

    if( anchor_char ) {
        for_each_tile( tripoint_bub_ms::zero, [&]( map_test_case::tile & t ) {
            if( t.setup_c == *anchor_char ) {
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
        res = point_bub_ms::zero;
    }

    origin = anchor_map_pos - tripoint_bub_ms( *res, 0 ).raw();
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

void map_test_case::for_each_tile( const tripoint_bub_ms &tmp_origin,
                                   const std::function<void( map_test_case::tile & )> &callback ) const
{
    int width = get_width();
    int height = get_height();

    tile tile;

    for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
            tile.setup_c = setup[y][x];
            tile.expect_c = expected_results[y][x];
            tile.p = tmp_origin + point( x, y );
            tile.p_local = point( x, y );
            callback( tile );
        }
    }
}

void map_test_case::for_each_tile( const std::function<void( tile )> &callback )
{
    do_internal_checks();
    if( !origin ) {
        origin = get_origin();
    }
    for_each_tile( *origin, callback );
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
        CAPTURE( line );
        REQUIRE( line.size() == static_cast<size_t>( width ) );
    }

    for( const std::string &line : expected_results ) {
        CAPTURE( line );
        REQUIRE( line.size() == static_cast<size_t>( width ) );
    }

    checks_complete = true;
}

void map_test_case::transpose()
{
    origin = std::nullopt;
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
}

void map_test_case::reflect_x()
{
    origin = std::nullopt;
    checks_complete = false;

    for( std::string &s : setup ) {
        std::reverse( s.begin(), s.end() );
    }
    for( std::string &s : expected_results ) {
        std::reverse( s.begin(), s.end() );
    }
}

void map_test_case::reflect_y()
{
    origin = std::nullopt;
    checks_complete = false;

    std::reverse( setup.begin(), setup.end() );
    std::reverse( expected_results.begin(), expected_results.end() );
}

void map_test_case::set_anchor_char_from( const std::set<char> &chars )
{
    for_each_tile( tripoint_bub_ms::zero, [&]( tile t ) {
        if( chars.count( t.setup_c ) ) {
            anchor_char = t.setup_c;
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

void map_test_case::validate_anchor_point( const tripoint_bub_ms &p )
{
    INFO( "checking point: " << p );
    tripoint_bub_ms origin_p = get_origin();
    REQUIRE( origin_p.z() == p.z() );
    REQUIRE( p == anchor_map_pos );
    // offset is anchor_map_pos in `map_test_case` local coords
    tripoint_rel_ms offset = p - origin_p;
    REQUIRE( offset.y() >= 0 );
    REQUIRE( offset.y() < get_height() );
    REQUIRE( offset.x() >= 0 );
    REQUIRE( offset.x() < get_width() );

    char setup_anchor_char = setup[offset.y()][offset.x()];
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

static std::string print_and_format_helper( map_test_case &t, int zshift,
        std::function<void( const tripoint &p, std::ostringstream &out )> print_tile )
{
    tripoint shift = tripoint( point::zero, zshift );
    return map_test_case_common::printers::format_2d_array(
    t.map_tiles_str( [&]( map_test_case::tile t, std::ostringstream & out ) {
        print_tile( ( t.p + shift ).raw(), out );
    } ) );
}

static std::string print_and_format_helper( map_test_case &t, int zshift,
        std::function<void( const tripoint_bub_ms &p, std::ostringstream &out )> print_tile )
{
    tripoint shift = tripoint( point::zero, zshift );
    return map_test_case_common::printers::format_2d_array(
    t.map_tiles_str( [&]( map_test_case::tile t, std::ostringstream & out ) {
        print_tile( ( t.p + shift ), out );
    } ) );
}

std::string map_test_case_common::printers::fields( map_test_case &t, int zshift )
{
    map &here = get_map();
    return print_and_format_helper( t, zshift, [&]( tripoint_bub_ms p, auto & out ) {
        bool first = true;
        for( auto &pr : here.field_at( p ) ) {
            out << ( first ? " " : "," ) << pr.second.name();
            first = false;
        }
    } );
}

std::string map_test_case_common::printers::transparency( map_test_case &t, int zshift )
{
    const level_cache &cache = get_map().access_cache( t.get_origin().z() + zshift );
    return print_and_format_helper( t, zshift, [&]( tripoint p, auto & out ) {
        out << std::setprecision( 3 ) << cache.transparency_cache[p.x][p.y] << ' ';
    } );
}

std::string map_test_case_common::printers::seen( map_test_case &t, int zshift )
{
    const auto &cache = get_map().access_cache( t.get_origin().z() + zshift ).seen_cache;
    return print_and_format_helper( t, zshift, [&]( tripoint p, auto & out ) {
        out << std::setprecision( 3 ) << cache[p.x][p.y] << ' ';
    } );
}

std::string map_test_case_common::printers::lm( map_test_case &t, int zshift )
{
    const level_cache &cache = get_map().access_cache( t.get_origin().z() + zshift );
    return print_and_format_helper( t, zshift, [&]( tripoint p, auto & out ) {
        out << cache.lm[p.x][p.y].to_string() << ' ';
    } );
}

std::string map_test_case_common::printers::apparent_light( map_test_case &t, int zshift )
{
    const level_cache &cache = get_map().access_cache( t.get_origin().z() + zshift );
    return print_and_format_helper( t, zshift, [&]( tripoint_bub_ms p, auto & out ) {
        out << std::setprecision( 3 ) << map::apparent_light_helper( cache, p ).apparent_light << ' ';
    } );
}

std::string map_test_case_common::printers::obstructed( map_test_case &t, int zshift )
{
    const level_cache &cache = get_map().access_cache( t.get_origin().z() + zshift );
    return print_and_format_helper( t, zshift, [&]( tripoint_bub_ms p, auto & out ) {
        bool obs = map::apparent_light_helper( cache, p ).obstructed;
        out << ( obs ? '#' : '.' );
    } );
}

std::string map_test_case_common::printers::floor( map_test_case &t, int zshift )
{
    const level_cache &cache = get_map().access_cache( t.get_origin().z() + zshift );
    return print_and_format_helper( t, zshift, [&]( tripoint p, auto & out ) {
        out << ( cache.floor_cache[p.x][p.y] ? '#' : '.' );
    } );
}

std::string map_test_case_common::printers::expected( map_test_case &t )
{
    return format_2d_array( t.map_tiles_str( [&]( map_test_case::tile t, std::ostringstream & out ) {
        out << t.expect_c;
    } ) );
}

// common helpers, used together with map_test_case
namespace map_test_case_common
{

tile_predicate operator+(
    const std::function<void( map_test_case::tile )> &f,
    const std::function<void( map_test_case::tile )> &g )
{
    return [ = ]( map_test_case::tile t ) {
        f( t );
        g( t );
        return true;
    };
}

tile_predicate operator&&( const tile_predicate &f, const tile_predicate &g )
{
    return [ = ]( map_test_case::tile t ) {
        return f( t ) && g( t );
    };
}

tile_predicate operator||( const tile_predicate &f, const tile_predicate &g )
{
    return [ = ]( map_test_case::tile t ) {
        return f( t ) || g( t );
    };
}

namespace tiles
{

tile_predicate ifchar( char c, const tile_predicate &f )
{
    return [ = ]( map_test_case::tile t ) {
        if( t.setup_c == c ) {
            f( t );
            return true;
        }
        return false;
    };
}

tile_predicate ter_set(
    ter_str_id ter,
    tripoint shift
)
{
    return [ = ]( map_test_case::tile t ) {
        REQUIRE( ter.is_valid() );
        tripoint_bub_ms p( t.p + shift );
        get_map().ter_set( p, ter );
        return true;
    };
}

} // namespace tiles

} // namespace map_test_case_common
