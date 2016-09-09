#include "catch/catch.hpp"

#include "game.h"
#include "iuse.h"
#include "monster.h"
#include "mtype.h"
#include "player.h"


player &get_sanitized_player(  ) {
    player &dummy = g->u;

    // Remove first worn item until there are none left.
    std::list<item> temp;
    while( dummy.takeoff( dummy.i_at( -2 ), &temp ) );
    dummy.inv.clear();
    dummy.remove_weapon();

    return dummy;
}

TEST_CASE( "use_eyedrops" ) {
    player &dummy = get_sanitized_player();

    item &test_item = dummy.i_add( item( "saline", 0, item::default_charges_tag{} ) );

    REQUIRE( test_item.charges == 5 );

    dummy.add_env_effect( efftype_id( "boomered" ), bp_eyes, 3, 12 );

    int test_item_pos = dummy.inv.position_by_item( &test_item );
    REQUIRE( test_item_pos != INT_MIN );

    dummy.invoke_item( &test_item );

    test_item_pos = dummy.inv.position_by_item( &test_item );
    REQUIRE( test_item_pos != INT_MIN );
    REQUIRE( test_item.charges == 4 );
    REQUIRE( !dummy.has_effect( efftype_id( "boomered" ) ) );

    dummy.invoke_item( &test_item );
    dummy.invoke_item( &test_item );
    dummy.invoke_item( &test_item );
    dummy.invoke_item( &test_item );

    test_item_pos = dummy.inv.position_by_item( &test_item );
    REQUIRE( test_item_pos == INT_MIN );
}

monster *find_adjacent_monster( const tripoint &pos ) {
    tripoint target = pos;
    monster *candidate = nullptr;
    for( target.x = pos.x - 1; target.x <= pos.x + 1; target.x++ ) {
        for( target.y = pos.y - 1; target.y <= pos.y + 1; target.y++ ) {
            if( target == pos ) {
                continue;
            }
            if( g->mon_at( target ) != -1 ) {
                candidate = g->monster_at( target );
                if( candidate != nullptr ) {
                    return candidate;
                }
            }
        }
    }
    return candidate;
}

TEST_CASE( "use_manhack" ) {
    player &dummy = get_sanitized_player();

    g->clear_zombies();
    item &test_item = dummy.i_add( item( "bot_manhack", 0, false ) );

    int test_item_pos = dummy.inv.position_by_item( &test_item );
    REQUIRE( test_item_pos != INT_MIN );

    monster *new_manhack = find_adjacent_monster( dummy.pos() );
    REQUIRE( new_manhack == nullptr );

    dummy.invoke_item( &test_item );

    test_item_pos = dummy.inv.position_by_item( &test_item );
    REQUIRE( test_item_pos == INT_MIN );

    new_manhack = find_adjacent_monster( dummy.pos() );
    REQUIRE( new_manhack != nullptr );
    REQUIRE( new_manhack->type->id == mtype_id( "mon_manhack" ) );
    g->clear_zombies();
}
