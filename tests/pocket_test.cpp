#include "item.h"

#include "catch/catch.hpp"

TEST_CASE( "sealed containers", "[pocket][sealed]" )
{
    item sealed_can( "test_can_sealed" );
    CHECK( sealed_can.is_container() );
    CHECK( sealed_can.is_watertight_container() );
    CHECK_FALSE( sealed_can.will_spill() );
}

