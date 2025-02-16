#include <memory>
#include <string>

#include "cata_catch.h"
#include "worldfactory.h"

TEST_CASE( "world_create_timestamp" )
{
    std::unique_ptr<WORLD> world = std::make_unique<WORLD>();
    REQUIRE( world->create_timestamp() );
    INFO( world->timestamp );
    CHECK( world->timestamp.size() + 1 == sizeof( "yyyymmddHHMMSS123456789" ) );
    for( const char ch : world->timestamp ) {
        CHECK( ch >= '0' );
        CHECK( ch <= '9' );
    }
}
