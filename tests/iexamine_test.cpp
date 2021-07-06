#include "cata_catch.h"

#include "iexamine.h"
#include "mapdata.h"

TEST_CASE( "mapdata_examine" )
{
    map_data_common_t data;
    data.set_examine( iexamine::water_source );

    CHECK( data.has_examine( iexamine::water_source ) );
    CHECK_FALSE( data.has_examine( iexamine::fungus ) );
    CHECK_FALSE( data.has_examine( iexamine::dirtmound ) );
    CHECK_FALSE( data.has_examine( iexamine::none ) );
}
