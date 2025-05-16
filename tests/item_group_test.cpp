#include <algorithm>
#include <list>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "cata_catch.h"
#include "cata_utility.h"
#include "enums.h"
#include "flag.h"
#include "item.h"
#include "item_group.h"
#include "options.h"
#include "options_helpers.h"
#include "type_id.h"

static const itype_id itype_40x46mm_m1006( "40x46mm_m1006" );
static const itype_id itype_glock_19( "glock_19" );
static const itype_id itype_longbow( "longbow" );
static const itype_id itype_match( "match" );
static const itype_id itype_matches( "matches" );
static const itype_id itype_rock( "rock" );
static const itype_id itype_test_balloon( "test_balloon" );
static const itype_id itype_test_rock( "test_rock" );

TEST_CASE( "truncate_spawn_when_items_dont_fit", "[item_group]" )
{
    // This item group is a 10L container with three 4L objects.  We should
    // always see exactly 2 of the 3.
    // Moreover, we should see all possible pairs chosen from amongst those 3.
    item_group_id truncate_test_id( "test_truncating_to_container" );
    std::set<std::pair<itype_id, itype_id>> observed_pairs;

    for( int i = 0; i < 100; ++i ) {
        const item_group::ItemList items = item_group::items_from( truncate_test_id );
        REQUIRE( items.size() == 1 );
        REQUIRE( items[0].typeId() == itype_test_balloon );
        std::list<const item *> contents = items[0].all_items_top();
        REQUIRE( contents.size() == 2 );
        observed_pairs.emplace( contents.front()->typeId(), contents.back()->typeId() );
    }

    CAPTURE( observed_pairs );

    CHECK( observed_pairs.size() == 6 );
}

TEST_CASE( "spill_when_items_dont_fit", "[item_group]" )
{
    // This item group is a 10L container with three 4L objects.  We should
    // always see 2 in the container and one outside.
    // Moreover, we should see all possible combinations chosen from amongst those 3.
    item_group_id truncate_test_id( "test_spilling_from_container" );
    std::set<std::pair<itype_id, itype_id>> observed_pairs_inside;
    std::set<itype_id> observed_outside;

    for( int i = 0; i < 100; ++i ) {
        const item_group::ItemList items = item_group::items_from( truncate_test_id );
        REQUIRE( items.size() == 2 );
        const item *container;
        const item *other;
        if( items[0].typeId() == itype_test_balloon ) {
            container = &items[0];
            other = &items[1];
        } else {
            container = &items[1];
            other = &items[0];
        }
        REQUIRE( container->typeId() == itype_test_balloon );
        std::list<const item *> contents = container->all_items_top();
        REQUIRE( contents.size() == 2 );
        observed_pairs_inside.emplace( contents.front()->typeId(), contents.back()->typeId() );
        observed_outside.emplace( other->typeId() );
    }

    CAPTURE( observed_pairs_inside );
    CAPTURE( observed_outside );

    CHECK( observed_pairs_inside.size() == 6 );
    CHECK( observed_outside.size() == 3 );
}

TEST_CASE( "spawn_with_default_charges_and_with_ammo", "[item_group]" )
{
    Item_modifier default_charges;
    default_charges.with_ammo = 100;
    SECTION( "tools without ammo" ) {
        item matches( itype_matches );
        REQUIRE( matches.ammo_default() == itype_match );
        default_charges.modify( matches, "modifier test (matches ammo)" );
        CHECK( matches.remaining_ammo_capacity() == 0 );
    }

    SECTION( "gun with ammo type" ) {
        item glock( itype_glock_19 );
        REQUIRE( !glock.magazine_default().is_null() );
        default_charges.modify( glock, "modifier test (glock ammo)" );
        CHECK( glock.remaining_ammo_capacity() == 0 );
    }
}

TEST_CASE( "Item_modifier_damages_item", "[item_group]" )
{
    Item_modifier damaged;
    damaged.damage.first = 1;
    damaged.damage.second = 1;
    SECTION( "except when it's an ammunition" ) {
        item rock( itype_rock );
        REQUIRE( rock.damage() == 0 );
        REQUIRE( rock.max_damage() == 0 );
        damaged.modify( rock, "modifier test (rock damage)" );
        CHECK( rock.damage() == 0 );
    }
    SECTION( "when it can be damaged" ) {
        item glock( itype_glock_19 );
        REQUIRE( glock.damage() == 0 );
        REQUIRE( glock.max_damage() > 0 );
        damaged.modify( glock, "modifier test (glock damage)" );
        CHECK( glock.damage() == 1 );
    }
}

TEST_CASE( "Item_modifier_gun_fouling", "[item_group]" )
{
    Item_modifier fouled;
    fouled.dirt.first = 1;
    SECTION( "guns can be fouled" ) {
        item glock( itype_glock_19 );
        REQUIRE( !glock.has_flag( flag_PRIMITIVE_RANGED_WEAPON ) );
        REQUIRE( !glock.has_var( "dirt" ) );
        fouled.modify( glock, "modifier test (glock fouling)" );
        CHECK( glock.get_var( "dirt", 0.0 ) > 0.0 );
    }
    SECTION( "bows can't be fouled" ) {
        item bow( itype_longbow );
        REQUIRE( !bow.has_var( "dirt" ) );
        REQUIRE( bow.has_flag( flag_PRIMITIVE_RANGED_WEAPON ) );
        fouled.modify( bow, "modifier test (bow fouling)" );
        CHECK( !bow.has_var( "dirt" ) );
    }
}

TEST_CASE( "item_modifier_modifies_charges_for_item", "[item_group]" )
{
    GIVEN( "an ammo item that uses charges" ) {
        const std::string &item_id = itype_40x46mm_m1006.c_str();
        item subject( itype_40x46mm_m1006 );

        const int default_charges = 6;

        REQUIRE( subject.is_ammo() );
        REQUIRE( subject.count_by_charges() );
        REQUIRE( subject.count() == default_charges );
        REQUIRE( subject.charges == default_charges );

        AND_GIVEN( "a modifier that does not modify charges" ) {
            Item_modifier modifier;

            WHEN( "the item is modified" ) {
                modifier.modify( subject, "modifier test (" + item_id + " ammo default)" );

                THEN( "charges should be unchanged" ) {
                    CHECK( subject.charges == default_charges );
                }
            }
        }

        AND_GIVEN( "a modifier that sets min and max charges to 0" ) {
            const int min_charges = 0;
            const int max_charges = 0;
            Item_modifier modifier;
            modifier.charges = { min_charges, max_charges };

            WHEN( "the item is modified" ) {
                modifier.modify( subject, "modifier test (" + item_id + " ammo set to 1)" );

                THEN( "charges are set to 1" ) {
                    CHECK( subject.charges == 1 );
                }
            }
        }

        AND_GIVEN( "a modifier that sets min and max charges to -1" ) {
            const int min_charges = -1;
            const int max_charges = -1;
            Item_modifier modifier;
            modifier.charges = { min_charges, max_charges };

            WHEN( "the item is modified" ) {
                modifier.modify( subject, "modifier test (" + item_id + " ammo explicitly default)" );

                THEN( "charges should be unchanged" ) {
                    CHECK( subject.charges == default_charges );
                }
            }
        }

        AND_GIVEN( "a modifier that sets min and max charges to a range [1, 4]" ) {
            const int min_charges = 1;
            const int max_charges = 4;
            Item_modifier modifier;
            modifier.charges = { min_charges, max_charges };

            WHEN( "the item is modified" ) {
                // We have to repeat this a bunch because the charges assignment
                // actually does rng between min and max, and if we only checked once
                // we might get a false success.
                std::vector<int> results;
                results.reserve( 100 );
                for( int i = 0; i < 100; i++ ) {
                    modifier.modify( subject, "modifier test (" + item_id + " ammo between 1 and 4)" );
                    results.emplace_back( subject.charges );
                }

                THEN( "charges are set to the expected range of values" ) {
                    CHECK( std::all_of( results.begin(), results.end(), [&]( int v ) {
                        return v >= min_charges && v <= max_charges;
                    } ) );
                }
            }
        }

        AND_GIVEN( "a modifier that sets min and max charges to 4" ) {
            const int expected = 4;
            const int min_charges = expected;
            const int max_charges = expected;
            Item_modifier modifier;
            modifier.charges = { min_charges, max_charges };

            WHEN( "the item is modified" ) {
                // We have to repeat this a bunch because the charges assignment
                // actually does rng between min and max, and if we only checked once
                // we might get a false success.
                std::vector<int> results;
                results.reserve( 100 );
                for( int i = 0; i < 100; i++ ) {
                    modifier.modify( subject, "modifier test (" + item_id + " ammo set to 4)" );
                    results.emplace_back( subject.charges );
                }

                THEN( "charges are set to the expected value" ) {
                    CHECK( std::all_of( results.begin(), results.end(), [&]( int v ) {
                        return v == expected;
                    } ) );
                }
            }
        }
    }
}

TEST_CASE( "Event-based_item_spawns_do_not_spawn_outside_event", "[item_group]" )
{
    override_option ev_spawn_opt( "EVENT_SPAWNS", "items" );
    REQUIRE( get_option<std::string>( "EVENT_SPAWNS" ) == "items" );

    item_group_id event_test_id( "test_event_item_spawn" );
    itype_id test_rock( itype_test_rock );
    const item_group::ItemList items = item_group::items_from( event_test_id );
    REQUIRE( item_group::every_possible_item_from( event_test_id ).size() == 6 );
    holiday cur_event = get_holiday_from_time();

    if( cur_event == holiday::christmas ) {
        CHECK( items.size() == 4 );
    } else if( cur_event == holiday::halloween ) {
        CHECK( items.size() == 3 );
    } else {
        CHECK( items.size() == 1 );
        CHECK( items[0].typeId() == test_rock );
    }
}
