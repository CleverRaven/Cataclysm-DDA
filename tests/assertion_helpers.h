#pragma once
#ifndef CATA_TESTS_ASSERTION_HELPERS_H
#define CATA_TESTS_ASSERTION_HELPERS_H

#include "cata_catch.h"

#include <algorithm>

template<typename Container1, typename Container2>
void check_containers_equal( const Container1 &c1, const Container2 &c2 )
{
    CAPTURE( c1 );
    CAPTURE( c2 );
    REQUIRE( c1.size() == c2.size() );
    CHECK( std::equal( c1.begin(), c1.end(), c2.begin() ) );
}

#endif // CATA_TESTS_ASSERTION_HELPERS_H
