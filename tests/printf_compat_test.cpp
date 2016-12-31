#include "catch/catch.hpp"

#include "output.h"

TEST_CASE( "printf_sizet", "[printf]" ) {
    // %zu formatting size_t
    CHECK( string_format( "%zu %d %zu %s %zu %s",
                          (size_t) 42,
                          11,
                          (size_t) 77,
                          "elbereth",
                          (size_t) 99,
                          "zooooo" ) == "42 11 77 elbereth 99 zooooo" );
}

#ifdef LOCALIZE
TEST_CASE( "printf_positional", "[printf]" ) {
    // $n positional formats
    CHECK( string_format( "%1$d %2$s", 24, "xyzzy" ) == "24 xyzzy" );
    CHECK( string_format( "%2$d %1$s", "xyzzy", 24 ) == "24 xyzzy" );
    CHECK( string_format( "%9$s %8$d %7$d %6$d %5$d %4$d %3$d %2$d %1$d", 1, 2, 3, 4, 5, 6, 7, 8, "xyzzy" ) == "xyzzy 8 7 6 5 4 3 2 1" );
}
#endif
