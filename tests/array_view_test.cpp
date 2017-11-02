#include "catch/catch.hpp"
#include "array_view.h"
#include "string_view.h"

#include <vector>
#include <array>
#include <string>
#include <algorithm>
#include <initializer_list>
#include <sstream>

template<typename T>
void test_array(array_view<char> av, T const& container) {
    REQUIRE( std::equal(container.begin(), container.end(), av.begin()) );
    REQUIRE( av.size() == container.size() );
    REQUIRE( static_cast<void const*>(&av[0]) == static_cast<void const*>(&container[0]) );
    REQUIRE( static_cast<void const*>(av.data()) == static_cast<void const*>(container.data()) );
    REQUIRE( av.back() == container.back() );
    REQUIRE( av.empty() == container.empty() );
}
void test_initlist(array_view<char> av, std::initializer_list<char> ilist) {
    REQUIRE( std::equal(ilist.begin(), ilist.end(), av.begin()) );
    REQUIRE( av.size() == ilist.size() );
}

template<typename T>
void test_string(string_view sv, T const& container) {
    REQUIRE( sv == container );
    REQUIRE( sv.substr(1) != container );
}

TEST_CASE("array_view", "[views]" )
{
    std::vector<char> vector  = { 'c', 'u', 't', 'e', ' ' };
    std::array<char, 5> array = {{ 'c', 'a', 't', 's', ' ' }};
    std::string string        = "are ";
    auto init                 = {'t', 'h', 'e', ' '};
    const char* cstring       = "best!";

    GIVEN( "An array view constructed from a vector" ) {
        test_array(vector, vector);
    }
    GIVEN( "An array view constructed from an array" ) {
        test_array(array, array);
    }
    GIVEN( "An array view constructed from a string" ) {
        test_array(string, string);
    }
    GIVEN( "An array view constructed from an initializer list" ) {
        test_initlist(init, init);
    }
    GIVEN( "An string view constructed from c string" ) {
        test_string(cstring, cstring);
    }
    GIVEN( "An string view constructed from std::string" ) {
        test_string(string, string);
    }
    GIVEN( "A stringtream filled from a stringview" ) {
        std::ostringstream ss;
        string_view sv;
        ss << (sv = vector);
        ss << (sv = array);
        ss << (sv = string);
        ss << (sv = init);
        ss << (sv = cstring);
        REQUIRE( ss.str() == "cute cats are the best!" );
    }
    GIVEN( "A stringview \"za warudo\"" ) {
        string_view sv = "za warudo";
        REQUIRE( sv > "the world" );
        REQUIRE( sv.substr(3) == "warudo" );
        REQUIRE( std::string{sv.substr(3,3)} == "war" );
        REQUIRE( sv.substr(sv.find('r')) == "rudo" );
        REQUIRE( sv.substr(sv.find("war")) == "warudo" );
        REQUIRE( sv.find("dio") == sv.npos );
        REQUIRE_THROWS( sv.substr(sv.find("dio")) );
    }
    GIVEN( "An empty stringview" ) {
        string_view sv;
        REQUIRE_FALSE( sv == "the world" );
        REQUIRE_THROWS( sv.substr(3) );
        REQUIRE_THROWS( sv.at(4) ) ;
        REQUIRE( sv != string );
    }
}



