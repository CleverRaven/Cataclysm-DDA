#include "catch/catch.hpp"

#include "output.h"
#include <chrono>

void string_format_benchmark()
{
    const char message[] = "Hello, the solutions of equation %s is %.2lf\n";
    const char arg1[] = "2x+3=0";
    const double arg2 = -1.5;

    auto start = std::chrono::high_resolution_clock::now();

    for( int i = 0; i < 100*1000; i++ ) {
        string_format(message, arg1, arg2);
    }
    auto end = std::chrono::high_resolution_clock::now();
    long diff = std::chrono::duration_cast<std::chrono::microseconds>( end - start ).count();
    printf( "100k calls of string_format() executed in %ld microseconds.\n", diff );
}

TEST_CASE("string_format_benchmark", "[.]") {
    string_format_benchmark();
}

TEST_CASE("string_format") {
    std::string formatted = string_format( "I'll give %s %d cookies.", "you", 42 );
    std::string expected = "I'll give you 42 cookies.";
    REQUIRE( formatted.size() == expected.size() );
    REQUIRE( formatted == expected );
}

