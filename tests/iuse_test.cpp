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
    avatar dummy;
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
    avatar dummy;
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

TEST_CASE( "anticonvulsant", "[iuse][anticonvulsant]" )
{
    avatar dummy;
    item &anticonvulsant = dummy.i_add( item( "diazepam", 0, item::default_charges_tag{} ) );

    GIVEN( "player has the shakes" ) {
        dummy.add_effect( efftype_id( "shakes" ), 1_hours );
        REQUIRE( dummy.has_effect( efftype_id( "shakes" ) ) );

        WHEN( "they use an anticonvulsant drug" ) {
            dummy.invoke_item( &anticonvulsant );

            THEN( "it cures the shakes" ) {
                REQUIRE_FALSE( dummy.has_effect( efftype_id( "shakes" ) ) );
            }

            AND_THEN( "it has side-effects" ) {
                REQUIRE( dummy.has_effect( efftype_id( "valium" ) ) );
                REQUIRE( dummy.has_effect( efftype_id( "high" ) ) );
                REQUIRE( dummy.has_effect( efftype_id( "took_anticonvulsant_visible" ) ) );
            }
        }
    }
}

TEST_CASE( "oxygen tank", "[iuse][oxygen]" )
{
    avatar dummy;
    item &oxygen = dummy.i_add( item( "oxygen_tank", 0, item::default_charges_tag{} ) );

    SECTION( "oxygen relieves smoke inhalation effects" ) {
        dummy.add_effect( efftype_id( "smoke" ), 1_hours );
        REQUIRE( dummy.has_effect( efftype_id( "smoke" ) ) );

        dummy.invoke_item( &oxygen );
        CHECK_FALSE( dummy.has_effect( efftype_id( "smoke" ) ) );
        CHECK( dummy.get_painkiller() == 2 );
    }

    SECTION( "oxygen relieves tear gas effects" ) {
        dummy.add_effect( efftype_id( "teargas" ), 1_hours );
        REQUIRE( dummy.has_effect( efftype_id( "teargas" ) ) );

        dummy.invoke_item( &oxygen );
        CHECK_FALSE( dummy.has_effect( efftype_id( "teargas" ) ) );
        CHECK( dummy.get_painkiller() == 2 );
    }

    SECTION( "oxygen relieves asthma effects" ) {
        dummy.add_effect( efftype_id( "asthma" ), 1_hours );
        REQUIRE( dummy.has_effect( efftype_id( "asthma" ) ) );

        dummy.invoke_item( &oxygen );
        CHECK_FALSE( dummy.has_effect( efftype_id( "asthma" ) ) );
        CHECK( dummy.get_painkiller() == 2 );
    }

    GIVEN( "player has no effects for the oxygen to treat" ) {
        REQUIRE_FALSE( dummy.has_effect( efftype_id( "smoke" ) ) );
        REQUIRE_FALSE( dummy.has_effect( efftype_id( "teargas" ) ) );
        REQUIRE_FALSE( dummy.has_effect( efftype_id( "asthma" ) ) );

        AND_GIVEN( "is not very stimulated yet" ) {
            int stim = dummy.get_stim();
            REQUIRE( dummy.get_painkiller() == 0 );
            REQUIRE( stim < 16 );

            THEN( "oxygen has a stimulant and extra painkiller effect" ) {
                dummy.invoke_item( &oxygen );
                CHECK( dummy.get_stim() >= stim + 8 );
                CHECK( dummy.get_painkiller() == 4 );
            }
        }
    }
}

