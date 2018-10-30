#include "catch/catch.hpp"

#include "item.h"
#include "units.h"

TEST_CASE( "item_volume", "[item]" )
{
    item i( "battery", 0, item::default_charges_tag() );
    // Would be better with Catch2 generators
    units::volume big_volume = units::from_milliliter( std::numeric_limits<int>::max() / 2 );
    for( units::volume v : {
             0_ml, 1_ml, i.volume(), big_volume
         } ) {
        INFO( "checking batteries that fit in " << v );
        auto charges_that_should_fit = i.charges_per_volume( v );
        i.charges = charges_that_should_fit;
        CHECK( i.volume() <= v );
        i.charges++;
        CHECK( i.volume() > v );
    }
}
