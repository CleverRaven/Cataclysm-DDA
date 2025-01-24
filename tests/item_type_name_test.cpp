#include <string>

#include "calendar.h"
#include "cata_catch.h"
#include "item.h"
#include "type_id.h"

// Test cases focused on item::type_name
static const itype_id itype_blood( "blood" );
static const itype_id itype_shotgun_410( "shotgun_410" );
static const itype_id itype_test_block_of_cheese( "test_block_of_cheese" );
static const itype_id itype_test_book( "test_book" );
static const itype_id itype_test_food( "test_food" );
static const itype_id itype_test_item( "test_item" );
static const itype_id itype_test_lorry( "test_lorry" );
static const itype_id itype_test_mag( "test_mag" );
static const itype_id itype_test_mass( "test_mass" );
static const itype_id itype_test_oxygen( "test_oxygen" );
static const itype_id itype_test_pile_of_dirt( "test_pile_of_dirt" );
static const itype_id itype_test_test_item( "test_test_item" );

static const mtype_id mon_chicken( "mon_chicken" );
static const mtype_id mon_zombie( "mon_zombie" );

TEST_CASE( "item_name_pluralization", "[item][type_name][plural]" )
{
    SECTION( "singular and plural item names" ) {

        SECTION( "plural is the same as singular" ) {
            item food( itype_test_food );
            item oxygen( itype_test_oxygen );

            CHECK( food.type_name( 1 ) == "food" );
            CHECK( food.type_name( 2 ) == "food" );

            CHECK( oxygen.type_name( 1 ) == "oxygen" );
            CHECK( oxygen.type_name( 2 ) == "oxygen" );
        }

        SECTION( "pluralize the last part" ) {
            item test_item( itype_test_item );
            item test_mass( itype_test_mass );
            item test_lorry( itype_test_lorry );
            item test_test_item( itype_test_test_item );

            // just add +s
            CHECK( test_item.type_name( 1 ) == "item" );
            CHECK( test_item.type_name( 2 ) == "items" );
            CHECK( test_test_item.type_name( 1 ) == "test item" );
            CHECK( test_test_item.type_name( 2 ) == "test items" );

            // -y to +ies
            CHECK( test_lorry.type_name( 1 ) == "lorry" );
            CHECK( test_lorry.type_name( 2 ) == "lorries" );

            // -ss to +sses
            CHECK( test_mass.type_name( 1 ) == "mass" );
            CHECK( test_mass.type_name( 2 ) == "masses" );

        }

        SECTION( "pluralize the first part" ) {
            item test_block_of_cheese( itype_test_block_of_cheese );
            item test_pile_of_dirt( itype_test_pile_of_dirt );

            CHECK( test_block_of_cheese.type_name( 1 ) == "block of cheese" );
            CHECK( test_block_of_cheese.type_name( 2 ) == "blocks of cheese" );

            CHECK( test_pile_of_dirt.type_name( 1 ) == "pile of dirt" );
            CHECK( test_pile_of_dirt.type_name( 2 ) == "piles of dirt" );
        }

        SECTION( "pluralize by inserting a word" ) {
            item test_mag( itype_test_mag );
            item test_book( itype_test_book );

            CHECK( test_mag.type_name( 1 ) == "Journal of Testing" );
            CHECK( test_mag.type_name( 2 ) == "issues of Journal of Testing" );

            CHECK( test_book.type_name( 1 ) == "Unit Testing Principles" );
            CHECK( test_book.type_name( 2 ) == "copies of Unit Testing Principles" );
        }
    }
}

TEST_CASE( "custom_named_item", "[item][type_name][named]" )
{
    // Shop smart. Shop S-Mart.
    item shotgun( itype_shotgun_410 );
    shotgun.set_var( "name", "Boomstick" );
    REQUIRE( shotgun.get_var( "name" ) == "Boomstick" );

    CHECK( shotgun.type_name() == "Boomstick" );
}

TEST_CASE( "blood_item", "[item][type_name][blood]" )
{
    SECTION( "blood from a zombie corpse" ) {
        item corpse = item::make_corpse( mon_zombie );
        item blood( itype_blood );
        blood.set_mtype( corpse.get_mtype() );
        REQUIRE( blood.typeId() == itype_blood );
        REQUIRE_FALSE( blood.is_corpse() );

        CHECK( blood.type_name() == "zombie blood" );
    }

    SECTION( "blood from a chicken corpse" ) {
        item corpse = item::make_corpse( mon_chicken );
        item blood( itype_blood );
        blood.set_mtype( corpse.get_mtype() );
        REQUIRE( blood.typeId() == itype_blood );
        REQUIRE_FALSE( blood.is_corpse() );

        CHECK( blood.type_name() == "chicken blood" );
    }

    SECTION( "blood from an unknown corpse" ) {
        item blood( itype_blood );
        REQUIRE( blood.typeId() == itype_blood );
        REQUIRE_FALSE( blood.is_corpse() );

        CHECK( blood.type_name() == "human blood" );
    }
}

TEST_CASE( "corpse_item", "[item][type_name][corpse]" )
{
    // Anonymous corpses

    SECTION( "human corpse" ) {
        item corpse = item::make_corpse( mtype_id::NULL_ID(), calendar::turn, "" );
        REQUIRE( corpse.is_corpse() );
        REQUIRE( corpse.get_corpse_name().empty() );

        CHECK( corpse.type_name() == "corpse of a human" );
    }

    SECTION( "zombie corpse" ) {
        item corpse = item::make_corpse( mon_zombie, calendar::turn, "" );
        REQUIRE( corpse.is_corpse() );
        REQUIRE( corpse.get_corpse_name().empty() );

        CHECK( corpse.type_name() == "corpse of a zombie" );
    }

    SECTION( "chicken corpse" ) {
        item corpse = item::make_corpse( mon_chicken, calendar::turn, "" );
        REQUIRE( corpse.is_corpse() );
        REQUIRE( corpse.get_corpse_name().empty() );

        CHECK( corpse.type_name() == "corpse of a chicken" );
    }

    // Corpses with names

    SECTION( "human corpse with a name" ) {
        item corpse = item::make_corpse( mtype_id::NULL_ID(), calendar::turn, "Dead Dude" );
        REQUIRE( corpse.is_corpse() );
        REQUIRE_FALSE( corpse.get_corpse_name().empty() );

        CHECK( corpse.type_name() == "corpse of Dead Dude, human" );
    }

    SECTION( "zombie corpse with a name" ) {
        item corpse = item::make_corpse( mon_zombie, calendar::turn, "Deadite Jones" );
        REQUIRE( corpse.is_corpse() );
        REQUIRE_FALSE( corpse.get_corpse_name().empty() );

        CHECK( corpse.type_name() == "corpse of Deadite Jones, zombie" );
    }

    SECTION( "chicken corpse with a name" ) {
        item corpse = item::make_corpse( mon_chicken, calendar::turn, "Herb" );
        REQUIRE( corpse.is_corpse() );
        REQUIRE_FALSE( corpse.get_corpse_name().empty() );

        CHECK( corpse.type_name() == "corpse of Herb, chicken" );
    }
}

