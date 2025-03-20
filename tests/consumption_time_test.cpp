#include <unordered_map>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "item.h"
#include "type_id.h"

static const itype_id itype_mustard( "mustard" );

TEST_CASE( "characters_with_no_mutations_take_at_least_1_second_to_consume_comestibles",
           "[character][item][food][time]" )
{
    GIVEN( "a character with no mutations and a comestible" ) {
        avatar character;
        REQUIRE( character.cached_mutations.empty() );

        item mustard( itype_mustard );
        REQUIRE( mustard.is_comestible() );

        WHEN( "character wants to consume it" ) {
            time_duration time_to_consume = character.get_consume_time( mustard );
            THEN( "it should take the character at least 1 second to do so" ) {
                CHECK( time_to_consume >= 1_seconds );
            }
        }
    }
}
