#include "cata_catch.h"

#include "cartesian_product.h"

TEST_CASE( "cartesian_product", "[nogame]" )
{
    std::vector<int> empty;
    std::vector<int> singleton = { 0 };
    std::vector<int> pair = { 1, 2 };

    using Lists = std::vector<std::vector<int>>;

    CHECK( cata::cartesian_product( Lists{} ) == Lists{ {} } );
    CHECK( cata::cartesian_product( Lists{ empty } ).empty() );
    CHECK( cata::cartesian_product( Lists{ empty, singleton } ).empty() );
    CHECK( cata::cartesian_product( Lists{ empty, pair } ).empty() );
    CHECK( cata::cartesian_product( Lists{ singleton, singleton } ) == Lists{ { 0, 0 } } );
    CHECK( cata::cartesian_product( Lists{ singleton, pair } ) == Lists{ { 0, 1 }, { 0, 2 } } );
    CHECK( cata::cartesian_product( Lists{ pair, pair } ) == Lists{ { 1, 1 }, { 2, 1 }, { 1, 2 }, { 2, 2 } } );
}
