#include <string>

#include "cata_catch.h"
#include "character.h"
#include "item.h"
#include "map_helpers.h"
#include "player_helpers.h"
#include "recipe.h"
#include "type_id.h"

static const ammotype ammo_battery( "battery" );

static const itype_id itype_chest_freezer( "chest_freezer" );
static const itype_id itype_medium_battery_cell( "medium_battery_cell" );
static const itype_id itype_rock( "rock" );
static const itype_id itype_stick( "stick" );

static const recipe_id
recipe_test_rock_need_full_magazine_mixed( "test_rock_need_full_magazine_mixed" );
static const recipe_id
recipe_test_rock_ordinary_stick_and_battery( "test_rock_ordinary_stick_and_battery" );
static const recipe_id
recipe_test_rock_ordinary_stick_and_rock( "test_rock_ordinary_stick_and_rock" );
static const skill_id skill_fabrication( "fabrication" );

static item battery_with_charge( const int charges )
{
    item battery( itype_medium_battery_cell );
    battery.ammo_set( battery.ammo_default(), charges );
    return battery;
}

static Character &prepare_crafter()
{
    clear_avatar();
    clear_map();
    Character &guy = get_player_character();
    guy.set_skill_level( skill_fabrication, 10 );
    return guy;
}

TEST_CASE( "ordinary_recipes_do_not_require_full_magazines", "[recipe][components][magazine]" )
{
    SECTION( "an incomplete magazine is accepted without the flag" ) {
        Character &guy = prepare_crafter();
        item battery( itype_medium_battery_cell );
        const int capacity = battery.ammo_capacity( ammo_battery );
        REQUIRE( capacity > 1 );
        battery.ammo_set( battery.ammo_default(), capacity / 2 );

        guy.i_add( item( itype_stick ) );
        guy.i_add( battery );
        guy.invalidate_crafting_inventory();

        CHECK( guy.can_start_craft( &recipe_test_rock_ordinary_stick_and_battery.obj(),
                                    recipe_filter_flags::none,
                                    1 ) );
    }

    SECTION( "ordinary non-magazine components are unaffected" ) {
        Character &guy = prepare_crafter();
        guy.i_add( item( itype_stick ) );
        guy.i_add( item( itype_rock ) );
        guy.invalidate_crafting_inventory();

        CHECK( guy.can_start_craft( &recipe_test_rock_ordinary_stick_and_rock.obj(),
                                    recipe_filter_flags::none, 1 ) );
    }
}

TEST_CASE( "NEED_FULL_MAGAZINE_only_filters_magazine_components", "[recipe][components][magazine]" )
{
    Character &guy = prepare_crafter();
    const item battery( itype_medium_battery_cell );
    const int capacity = battery.ammo_capacity( ammo_battery );
    REQUIRE( capacity > 1 );

    guy.i_add( item( itype_chest_freezer ) );

    SECTION( "a full magazine and an ordinary component satisfy the recipe" ) {
        guy.i_add( battery_with_charge( capacity ) );
        guy.invalidate_crafting_inventory();

        CHECK( guy.can_start_craft( &recipe_test_rock_need_full_magazine_mixed.obj(),
                                    recipe_filter_flags::none, 1 ) );
    }

    SECTION( "a half-full magazine does not satisfy the recipe" ) {
        guy.i_add( battery_with_charge( capacity / 2 ) );
        guy.invalidate_crafting_inventory();

        CHECK_FALSE( guy.can_start_craft( &recipe_test_rock_need_full_magazine_mixed.obj(),
                                          recipe_filter_flags::none, 1 ) );
    }

    SECTION( "an empty magazine does not satisfy the recipe" ) {
        guy.i_add( battery_with_charge( 0 ) );
        guy.invalidate_crafting_inventory();

        CHECK_FALSE( guy.can_start_craft( &recipe_test_rock_need_full_magazine_mixed.obj(),
                                          recipe_filter_flags::none, 1 ) );
    }
}
