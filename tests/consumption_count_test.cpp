#include <list>
#include <memory>
#include <string>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "dialogue.h"
#include "item.h"
#include "math_parser.h"
#include "player_helpers.h"
#include "talker.h"
#include "type_id.h"

static const itype_id itype_butter( "butter" );

TEST_CASE( "consumption_count_math_function", "[math_parser][consumption]" )
{
    clear_avatar();
    avatar &guy = get_avatar();
    math_exp testexp;

    dialogue d( get_talker_for( guy ), std::make_unique<talker>() );

    SECTION( "returns 0 with no consumption history" ) {
        guy.consumption_history.clear();
        REQUIRE( testexp.parse( "u_consumption_count('butter')" ) );
        CHECK( testexp.eval( d ) == Approx( 0 ) );
    }

    SECTION( "counts matching items in history" ) {
        guy.consumption_history.clear();
        item butter( itype_butter );
        // Add 3 butter consumption events at current time
        guy.consumption_history.emplace_back( butter );
        guy.consumption_history.emplace_back( butter );
        guy.consumption_history.emplace_back( butter );

        REQUIRE( testexp.parse( "u_consumption_count('butter')" ) );
        CHECK( testexp.eval( d ) == Approx( 3 ) );

        // Different item should not count
        REQUIRE( testexp.parse( "u_consumption_count('apple')" ) );
        CHECK( testexp.eval( d ) == Approx( 0 ) );
    }

    SECTION( "hours kwarg filters by time window" ) {
        guy.consumption_history.clear();
        item butter( itype_butter );

        // Add a recent event (current turn)
        guy.consumption_history.emplace_back( butter );

        // Add an old event (10 hours ago)
        consumption_event old_event( butter );
        old_event.time = calendar::turn - 10_hours;
        guy.consumption_history.push_back( old_event );

        // Default 48h window should see both
        REQUIRE( testexp.parse( "u_consumption_count('butter')" ) );
        CHECK( testexp.eval( d ) == Approx( 2 ) );

        // 6h window should see only the recent one
        REQUIRE( testexp.parse( "u_consumption_count('butter', 'hours': 6)" ) );
        CHECK( testexp.eval( d ) == Approx( 1 ) );

        // 12h window should see both
        REQUIRE( testexp.parse( "u_consumption_count('butter', 'hours': 12)" ) );
        CHECK( testexp.eval( d ) == Approx( 2 ) );
    }
}
