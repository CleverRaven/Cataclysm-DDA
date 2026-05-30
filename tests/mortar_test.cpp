#include "cata_catch.h"

#include "mortar.h"

static const mortar_type_id mortar_type_m224( "m224" );

TEST_CASE( "mortar_minimum_range_and_deflection_error", "[mortar]" )
{
    const mortar_type &mortar = mortar_type_m224.obj();

    mortar_error error = mortar.minimum_error( 1000 );
    CHECK( error.range == Approx( 15.0 ) );
    CHECK( error.deflection == Approx( 2.0 ) );

    error = mortar.minimum_error( 3500 );
    CHECK( error.range == Approx( 52.5 ) );
    CHECK( error.deflection == Approx( 7.0 ) );
}
