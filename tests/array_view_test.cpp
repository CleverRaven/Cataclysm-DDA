#include "catch/catch.hpp"
#include "array_view.h"
#include "string_view.h"
#include <string>

TEST_CASE( "array_view", "[views]" )
{
    std::string s = "CataclysmDDA";
    string_view sv = s;
    GIVEN( "A string and a string view constructed from that string" ) {
        std::string const &sr = s;
        WHEN( "string is a const&" ) {
            REQUIRE( sv.at( 5 ) == sr.at( 5 ) );
            REQUIRE_THROWS( sv.at( 999 ) );
            REQUIRE_THROWS( sr.at( 999 ) );
            REQUIRE( sv[4] == sr[4] );
            REQUIRE( sv.front() == sr.front() );
            REQUIRE( sv.back() == sr.back() );
            REQUIRE( bool( sv.data() == sr.data() ) );
            REQUIRE( bool( &*sv.begin() == &*sr.begin() ) );
            REQUIRE( bool( &*sv.end() == &*sr.end() ) );
            REQUIRE( bool( &*sv.rbegin() == &*sr.rbegin() ) );
            REQUIRE( bool( &*sv.rend() == &*sr.rend() ) );
            REQUIRE( sv.empty() == sr.empty() );
            REQUIRE( sv.size() == sr.size() );
            REQUIRE( sv.length() == sr.length() );
            REQUIRE( sv.compare( "CataclysmDDR" ) == sr.compare( "CataclysmDDR" ) );
            REQUIRE( sv.substr( 0, 3 ) == sr.substr( 0, 3 ) );
            REQUIRE( sv.find( 'D' ) == sr.find( 'D' ) );
            REQUIRE( sv.rfind( 'D' ) == sr.rfind( 'D' ) );
            REQUIRE( sv.find_first_of( "ay" ) == sr.find_first_of( "ay" ) );
            REQUIRE( sv.find_last_of( "ay" ) == sr.find_last_of( "ay" ) );
            REQUIRE( ( sv + std::string( " is fun" ) ) == ( sr + std::string( " is fun" ) ) );
            REQUIRE( sv == sr );
            REQUIRE_FALSE( sv != sr );
            REQUIRE( ( sv < "Dogaclysm" ) == ( sr < "Dogaclysm" ) );
        }
        WHEN( "printing into a stream" ) {
            std::ostringstream ss1, ss2;
            ss1 << sv;
            ss2 << sr;
            REQUIRE( ss1.str() == ss2.str() );
        }
        WHEN( "string is mutable" ) {
            sv.pop_back();
            s.pop_back();
            REQUIRE( sv == s );
            sv.clear();
            s.clear();
            REQUIRE( sv == s );
        }
    }
}

