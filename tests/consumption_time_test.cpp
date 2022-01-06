#include "avatar.h"
#include "item.h"
#include "cata_catch.h"

TEST_CASE( "characters with no mutations take at least 1 second to consume comestibles",
           "[character][item][food][time]" )
{
    GIVEN( "a character with no mutations and a comestible" ) {
        avatar character;
        REQUIRE( character.my_mutations.empty() );

        item mustard( "mustard" );
        REQUIRE( mustard.is_comestible() );

        WHEN( "character wants to consume it" ) {
            time_duration time_to_consume = character.get_consume_time( mustard );
            THEN( "it should take the character at least 1 second to do so" ) {
                CHECK( time_to_consume >= 1_seconds );
            }
        }
    }
}
