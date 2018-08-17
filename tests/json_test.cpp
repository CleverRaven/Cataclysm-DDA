#include "catch/catch.hpp"

#include "json.h"

#include <initializer_list>
#include <limits>
#include <sstream>
#include <typeinfo>

static void json_float_save_load( std::initializer_list<double> list )
{
    for( auto &&tosave : list ) {
        if( !std::isfinite( tosave ) ) {
            WARN( "json only supports finite float values" );
            continue;
        }

        std::stringstream info;
        info.precision( std::numeric_limits<double>::max_digits10 );
        info << "Saving double " << tosave << " and reloading it using json\n";

        std::stringstream saved;
        JsonOut jout( saved );
        jout.write( tosave );
        info << "Saved string: " << saved.str() << "\n";

        JsonIn jin( saved );
        auto &&loaded = jin.get_float();
        info << "Reloaded double: " << loaded;

        INFO( info.str() );
        CHECK( std::signbit( tosave ) == std::signbit( loaded ) ); // for +-0
        CHECK( tosave == loaded );
    }
}

static void json_float_load( std::initializer_list<std::pair<std::string, double>> list )
{
    for( auto &&pr : list ) {
        auto &&toload = pr.first;
        auto &&answer = pr.second;
        if( !std::isfinite( answer ) ) {
            WARN( "json only supports finite float values" );
            continue;
        }

        std::stringstream info;
        info.precision( std::numeric_limits<double>::max_digits10 );
        info << "Loading double string " << toload << " using json\n";
        info << "Result expected: " << answer << "\n";

        std::stringstream saved( toload );
        JsonIn jin( saved );
        auto &&loaded = jin.get_float();
        info << "Actual result: " << loaded;

        INFO( info.str() );
        CHECK( std::signbit( loaded ) == std::signbit( answer ) ); // for +-0
        CHECK( loaded == answer );
    }
}

TEST_CASE( "Json float number save and load", "[json]" )
{
    // test easy values
    json_float_save_load( {
        -2147483648., -2147483647., -65536, -65535, -32000, -100, -1, -0.5, 0, 0.5, 4, 127, 128,
            900, 3000, 4194410, 4294967295., 4294967296.
        } );
#define make( val ) {#val, val}
    json_float_load( {
        make( -2147483648 ), make( -2147483647 ), make( -1.024e4 ), make( -1e4 ),
        make( -1024 ), make( -1000 ), make( -3e0 ), make( -3.00e0 ), make( -1.00000 ),
        make( 0 ), make( 0.25 ), make( 1.000000000000000000000000000000 ), make( 3e3 ),
        make( 65535 ), make( 10e4 ), make( 4194410 ), make( 4000000000 ), make( 4e9 )
    } );
}
