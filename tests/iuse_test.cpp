#include <climits>
#include <list>
#include <memory>

#include "avatar.h"
#include "catch/catch.hpp"
#include "game.h"
#include "monster.h"
#include "mtype.h"
#include "player.h"
#include "bodypart.h"
#include "calendar.h"
#include "inventory.h"
#include "item.h"
#include "string_id.h"
#include "type_id.h"
#include "point.h"

static player &get_sanitized_player( )
{
    player &dummy = g->u;

    // Remove first worn item until there are none left.
    std::list<item> temp;
    while( dummy.takeoff( dummy.i_at( -2 ), &temp ) );
    dummy.inv.clear();
    dummy.remove_weapon();

    return dummy;
}

TEST_CASE( "eyedrops", "[iuse][eyedrops]" )
{
    player &dummy = get_sanitized_player();

    item &test_item = dummy.i_add( item( "saline", 0, item::default_charges_tag{} ) );

    REQUIRE( test_item.charges == 5 );

    dummy.add_env_effect( efftype_id( "boomered" ), bp_eyes, 3, 12_turns );

    item_location loc = item_location( dummy, &test_item );
    REQUIRE( loc );
    int test_item_pos = dummy.inv.position_by_item( &test_item );
    REQUIRE( test_item_pos != INT_MIN );

    dummy.consume( loc );

    test_item_pos = dummy.inv.position_by_item( &test_item );
    REQUIRE( test_item_pos != INT_MIN );
    REQUIRE( test_item.charges == 4 );
    REQUIRE( !dummy.has_effect( efftype_id( "boomered" ) ) );

    dummy.consume( loc );
    dummy.consume( loc );
    dummy.consume( loc );
    dummy.consume( loc );

    test_item_pos = dummy.inv.position_by_item( &test_item );
    REQUIRE( test_item_pos == INT_MIN );
}

static monster *find_adjacent_monster( const tripoint &pos )
{
    tripoint target = pos;
    for( target.x = pos.x - 1; target.x <= pos.x + 1; target.x++ ) {
        for( target.y = pos.y - 1; target.y <= pos.y + 1; target.y++ ) {
            if( target == pos ) {
                continue;
            }
            if( monster *const candidate = g->critter_at<monster>( target ) ) {
                return candidate;
            }
        }
    }
    return nullptr;
}

TEST_CASE( "manhack", "[iuse][manhack]" )
{
    player &dummy = get_sanitized_player();

    g->clear_zombies();
    item &test_item = dummy.i_add( item( "bot_manhack", 0, item::default_charges_tag{} ) );

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

TEST_CASE( "antifungal", "[iuse][antifungal]" )
{
    player &dummy = get_sanitized_player();
    item &antifungal = dummy.i_add( item( "antifungal", 0, item::default_charges_tag{} ) );

    GIVEN( "player has a fungal infection" ) {
        dummy.add_effect( efftype_id( "fungus" ), 1_hours );
        REQUIRE( dummy.has_effect( efftype_id( "fungus" ) ) );
        WHEN( "they take an antifungal drug" ) {
            dummy.invoke_item( &antifungal );
            THEN( "it cures the fungal infection" ) {
                REQUIRE_FALSE( dummy.has_effect( efftype_id( "fungus" ) ) );
            }
        }
    }

    GIVEN( "player has fungal spores" ) {
        dummy.add_effect( efftype_id( "spores" ), 1_hours );
        REQUIRE( dummy.has_effect( efftype_id( "spores" ) ) );
        WHEN( "they take an antifungal drug" ) {
            dummy.invoke_item( &antifungal );
            THEN( "it has no effect on the spores" ) {
                REQUIRE( dummy.has_effect( efftype_id( "spores" ) ) );
            }
        }
    }
}

TEST_CASE( "antiparasitic", "[iuse][antiparasitic]" )
{
    player &dummy = get_sanitized_player();
    item &antiparasitic = dummy.i_add( item( "antiparasitic", 0, item::default_charges_tag{} ) );

    GIVEN( "player has parasite infections" ) {
        dummy.add_effect( efftype_id( "dermatik" ), 1_hours );
        dummy.add_effect( efftype_id( "tapeworm" ), 1_hours );
        dummy.add_effect( efftype_id( "bloodworms" ), 1_hours );
        dummy.add_effect( efftype_id( "brainworms" ), 1_hours );
        dummy.add_effect( efftype_id( "paincysts" ), 1_hours );

        REQUIRE( dummy.has_effect( efftype_id( "dermatik" ) ) );
        REQUIRE( dummy.has_effect( efftype_id( "tapeworm" ) ) );
        REQUIRE( dummy.has_effect( efftype_id( "bloodworms" ) ) );
        REQUIRE( dummy.has_effect( efftype_id( "brainworms" ) ) );
        REQUIRE( dummy.has_effect( efftype_id( "paincysts" ) ) );

        WHEN( "they use an antiparasitic drug" ) {
            dummy.invoke_item( &antiparasitic );

            THEN( "it cures all parasite infections" ) {
                CHECK_FALSE( dummy.has_effect( efftype_id( "dermatik" ) ) );
                CHECK_FALSE( dummy.has_effect( efftype_id( "tapeworm" ) ) );
                CHECK_FALSE( dummy.has_effect( efftype_id( "bloodworms" ) ) );
                CHECK_FALSE( dummy.has_effect( efftype_id( "brainworms" ) ) );
                CHECK_FALSE( dummy.has_effect( efftype_id( "paincysts" ) ) );
            }
        }
    }

    GIVEN( "player has a fungal infection" ) {
        dummy.add_effect( efftype_id( "fungus" ), 1_hours );
        REQUIRE( dummy.has_effect( efftype_id( "fungus" ) ) );

        WHEN( "they use an antiparasitic drug" ) {
            dummy.invoke_item( &antiparasitic );

            THEN( "it has no effect on the fungal infection" ) {
                REQUIRE( dummy.has_effect( efftype_id( "fungus" ) ) );
            }
        }
    }
}

TEST_CASE( "anticonvulsant", "[iuse][anticonvulsant][!mayfail]" )
{
    player &dummy = get_sanitized_player();
    item &anticonvulsant = dummy.i_add( item( "diazepam", 0, item::default_charges_tag{} ) );

    GIVEN( "player has the shakes" ) {
        //REQUIRE( efftype_id( "shakes" ).is_valid() );
        //REQUIRE_FALSE( dummy.is_immune_effect( efftype_id( "shakes" ) ) );
        dummy.add_effect( efftype_id( "shakes" ), 1_hours );

        // FIXME: Why is this failing?
        REQUIRE( dummy.has_effect( efftype_id( "shakes" ) ) );

        WHEN( "they use an anticonvulsant drug" ) {
            dummy.invoke_item( &anticonvulsant );

            THEN( "it cures the shakes" ) {
                REQUIRE_FALSE( dummy.has_effect( efftype_id( "shakes" ) ) );
            }

            AND_THEN( "it has side-effects that wear off after a while" ) {
                REQUIRE( dummy.has_effect( efftype_id( "valium" ) ) );
                REQUIRE( dummy.has_effect( efftype_id( "high" ) ) );
                REQUIRE( dummy.has_effect( efftype_id( "took_anticonvulsant_visible" ) ) );

                // Actual duration depends on strength, tolerance, and lightweight attributes
                // with some randomness, but always less than 10 hours total
                dummy.moves -= to_moves<int>( 24_hours );
                REQUIRE_FALSE( dummy.has_effect( efftype_id( "valium" ) ) );
                REQUIRE_FALSE( dummy.has_effect( efftype_id( "high" ) ) );
                REQUIRE_FALSE( dummy.has_effect( efftype_id( "took_anticonvulsant_visible" ) ) );
            }
        }
    }
}

