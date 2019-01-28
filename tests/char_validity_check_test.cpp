#include "catch/catch.hpp"
#include "char_validity_check.h"

TEST_CASE( "char_validity_check" )
{
    CHECK( is_char_allowed( '\0' ) == false );
    CHECK( is_char_allowed( '\b' ) == false );
    CHECK( is_char_allowed( '\t' ) == false );
    CHECK( is_char_allowed( '\n' ) == false );
    CHECK( is_char_allowed( '\r' ) == false );
    CHECK( is_char_allowed( '\xa0' ) == false );
    CHECK( is_char_allowed( '\\' ) == false );
    CHECK( is_char_allowed( '/' ) == false );
    CHECK( is_char_allowed( ' ' ) == true );
    CHECK( is_char_allowed( '?' ) == true );
    CHECK( is_char_allowed( '!' ) == true );
}
