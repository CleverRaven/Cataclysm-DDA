#include <iosfwd>
#include <memory>
#include <set>
#include <string>

#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "flag.h"
#include "item.h"
#include "item_pocket.h"
#include "itype.h"
#include "options_helpers.h"
#include "ret_val.h"
#include "type_id.h"
#include "value_ptr.h"

static const fault_id fault_gun_dirt( "fault_gun_dirt" );

static const item_category_id item_category_veh_parts( "veh_parts" );

static const itype_id itype_backpack_hiking( "backpack_hiking" );
static const itype_id itype_purse( "purse" );
static const itype_id itype_rock( "rock" );
static const itype_id itype_test_rock( "test_rock" );

static const skill_id skill_survival( "survival" );

// Test cases focused on item::tname

// TODO: Add test cases to cover other aspects of tname such as:
//
// - clothing with +1 suffix
// - ethereal (X turns)
// - is_bionic (sterile), (packed)
// - (UPS) for UPS tool
// - (faulty) for faults
// - "burnt" or "badly burnt"
// - (dirty)
// - (rotten)
// - (mushy)
// - (old)
// - (fresh)
// - Radio-mod with signals (Red, Blue, Green)
// - used, lit, plugged in, active, sawn-off
// - favorite *

TEST_CASE( "food_with_hidden_effects", "[item][tname][hidden]" )
{
    Character &player_character = get_player_character();
    player_character.clear_mutations();

    GIVEN( "food with hidden poison" ) {
        item coffee = item( "coffee_pod" );
        REQUIRE( coffee.is_food() );
        REQUIRE( coffee.has_flag( flag_HIDDEN_POISON ) );

        WHEN( "avatar has level 2 survival skill" ) {
            player_character.set_skill_level( skill_survival, 2 );
            REQUIRE( static_cast<int>( player_character.get_skill_level( skill_survival ) ) == 2 );

            THEN( "they cannot see it is poisonous" ) {
                CHECK( coffee.tname() == "Kentucky coffee pod" );
            }
        }

        WHEN( "avatar has level 3 survival skill" ) {
            player_character.set_skill_level( skill_survival, 3 );
            REQUIRE( static_cast<int>( player_character.get_skill_level( skill_survival ) ) == 3 );

            THEN( "they see it is poisonous" ) {
                CHECK( coffee.tname() == "Kentucky coffee pod (poisonous)" );
            }
        }
    }

    GIVEN( "food with hidden hallucinogen" ) {
        item mushroom = item( "mushroom" );
        mushroom.set_flag( flag_HIDDEN_HALLU );
        REQUIRE( mushroom.is_food() );
        REQUIRE( mushroom.has_flag( flag_HIDDEN_HALLU ) );

        WHEN( "avatar has level 4 survival skill" ) {
            player_character.set_skill_level( skill_survival, 4 );
            REQUIRE( static_cast<int>( player_character.get_skill_level( skill_survival ) ) == 4 );

            THEN( "they cannot see it is hallucinogenic" ) {
                CHECK( mushroom.tname() == "mushroom (fresh)" );
            }
        }

        WHEN( "avatar has level 5 survival skill" ) {
            player_character.set_skill_level( skill_survival, 5 );
            REQUIRE( static_cast<int>( player_character.get_skill_level( skill_survival ) ) == 5 );

            THEN( "they see it is hallucinogenic" ) {
                CHECK( mushroom.tname() == "mushroom (hallucinogenic) (fresh)" );
            }
        }
    }
}

TEST_CASE( "items_with_a_temperature_flag", "[item][tname][temperature]" )
{
    GIVEN( "food that can melt" ) {
        item shake( "milkshake" );
        REQUIRE( shake.is_food() );
        REQUIRE( shake.has_flag( flag_MELTS ) );

        WHEN( "frozen" ) {
            shake.set_flag( flag_FROZEN );
            REQUIRE( shake.has_flag( flag_FROZEN ) );

            THEN( "it appears frozen and not melted" ) {
                CHECK( shake.tname() == "milkshake (fresh) (frozen)" );
            }
        }
        WHEN( "cold" ) {
            shake.set_flag( flag_COLD );
            REQUIRE( shake.has_flag( flag_COLD ) );

            THEN( "it appears cold and melted" ) {
                CHECK( shake.tname() == "milkshake (fresh) (cold) (melted)" );
            }
        }
        WHEN( "hot" ) {
            shake.set_flag( flag_HOT );
            REQUIRE( shake.has_flag( flag_HOT ) );

            THEN( "it appears hot and melted" ) {
                CHECK( shake.tname() == "milkshake (fresh) (hot) (melted)" );
            }
        }
    }

    GIVEN( "food that cannot melt" ) {
        item nut( "pine_nuts" );
        REQUIRE( nut.is_food() );
        REQUIRE_FALSE( nut.has_flag( flag_MELTS ) );

        WHEN( "frozen" ) {
            nut.set_flag( flag_FROZEN );
            REQUIRE( nut.has_flag( flag_FROZEN ) );

            THEN( "it appears frozen" ) {
                CHECK( nut.tname() == "pine nuts (fresh) (frozen)" );
            }
        }
        WHEN( "cold" ) {
            nut.set_flag( flag_COLD );
            REQUIRE( nut.has_flag( flag_COLD ) );

            THEN( "it appears cold" ) {
                CHECK( nut.tname() == "pine nuts (fresh) (cold)" );
            }
        }

        WHEN( "hot" ) {
            nut.set_flag( flag_HOT );
            REQUIRE( nut.has_flag( flag_HOT ) );

            THEN( "it appears hot" ) {
                CHECK( nut.tname() == "pine nuts (fresh) (hot)" );
            }
        }
    }

    GIVEN( "a human corpse" ) {
        item corpse = item::make_corpse( mtype_id::NULL_ID(), calendar::turn, "" );
        REQUIRE( corpse.is_corpse() );

        WHEN( "frozen" ) {
            corpse.set_flag( flag_FROZEN );
            REQUIRE( corpse.has_flag( flag_FROZEN ) );

            THEN( "it appears frozen" ) {
                CHECK( corpse.tname() == "corpse of a human (fresh) (frozen)" );
            }
        }

        WHEN( "cold" ) {
            corpse.set_flag( flag_COLD );
            REQUIRE( corpse.has_flag( flag_COLD ) );

            THEN( "it appears cold" ) {
                CHECK( corpse.tname() == "corpse of a human (fresh) (cold)" );
            }
        }

        WHEN( "hot" ) {
            corpse.set_flag( flag_HOT );
            REQUIRE( corpse.has_flag( flag_HOT ) );

            THEN( "it appears hot" ) {
                CHECK( corpse.tname() == "corpse of a human (fresh) (hot)" );
            }
        }
    }

    GIVEN( "an item that is not food or a corpse" ) {
        item hammer( "hammer" );
        REQUIRE_FALSE( hammer.is_food() );
        REQUIRE_FALSE( hammer.is_corpse() );

        WHEN( "it is hot" ) {
            hammer.set_flag( flag_HOT );
            REQUIRE( hammer.has_flag( flag_HOT ) );

            THEN( "it does not appear hot" ) {
                CHECK( hammer.tname() == "hammer" );
            }
        }

        WHEN( "it is cold" ) {
            hammer.set_flag( flag_COLD );
            REQUIRE( hammer.has_flag( flag_COLD ) );

            THEN( "it does not appear cold" ) {
                CHECK( hammer.tname() == "hammer" );
            }
        }

        WHEN( "it is frozen" ) {
            hammer.set_flag( flag_FROZEN );
            REQUIRE( hammer.has_flag( flag_FROZEN ) );

            THEN( "it does not appear frozen" ) {
                CHECK( hammer.tname() == "hammer" );
            }
        }
    }
}

TEST_CASE( "wet_item", "[item][tname][wet]" )
{
    item rag( "rag" );
    rag.set_flag( flag_WET );
    REQUIRE( rag.has_flag( flag_WET ) );

    CHECK( rag.tname() == "rag (wet)" );
}

TEST_CASE( "filthy_item", "[item][tname][filthy]" )
{
    item rag( "rag" );
    rag.set_flag( flag_FILTHY );
    REQUIRE( rag.is_filthy() );

    CHECK( rag.tname() == "rag (filthy)" );
}

TEST_CASE( "diamond_item", "[item][tname][diamond]" )
{
    item katana( "katana" );
    katana.set_flag( flag_DIAMOND );
    REQUIRE( katana.has_flag( flag_DIAMOND ) );

    CHECK( katana.tname() == "diamond katana" );
}

TEST_CASE( "truncated_item_name", "[item][tname][truncate]" )
{
    SECTION( "plain item name can be truncated" ) {
        item katana( "katana" );

        CHECK( katana.tname() == "katana" );
        CHECK( katana.tname( 1, false, 5 ) == "katan" );
    }

    // TODO: color-coded or otherwise embellished item name can be truncated
}

TEST_CASE( "engine_displacement_volume", "[item][tname][engine]" )
{
    item vtwin = item( "v2_combustion" );
    item v12diesel = item( "v12_diesel" );
    item turbine = item( "small_turbine_engine" );

    REQUIRE( vtwin.engine_displacement() == 60 );
    REQUIRE( v12diesel.engine_displacement() == 700 );
    REQUIRE( turbine.engine_displacement() == 2700 );

    CHECK( vtwin.tname() == "0.6L V2 engine" );
    CHECK( v12diesel.tname() == "7L V12 diesel engine" );
    CHECK( turbine.tname() == "27L 1,350 HP gas turbine engine" );
}

TEST_CASE( "wheel_diameter", "[item][tname][wheel]" )
{
    item wheel17 = item( "wheel" );
    item wheel24 = item( "wheel_wide" );
    item wheel32 = item( "wheel_armor" );

    REQUIRE( wheel17.type->wheel->diameter == 17 );
    REQUIRE( wheel24.type->wheel->diameter == 24 );
    REQUIRE( wheel32.type->wheel->diameter == 32 );

    CHECK( wheel17.tname() == "17\" wheel" );
    CHECK( wheel24.tname() == "24\" wide wheel" );
    CHECK( wheel32.tname() == "32\" armored wheel" );
}

TEST_CASE( "item_health_or_damage_bar", "[item][tname][health][damage]" )
{
    GIVEN( "some clothing" ) {
        item shirt( "longshirt" );
        item deg_test( "test_baseball" );
        REQUIRE( shirt.is_armor() );
        REQUIRE( deg_test.type->category_force == item_category_veh_parts );

        // Ensure the item health option is set to bars
        override_option opt( "ITEM_HEALTH", "bars" );

        // Damage bar uses a scale of 0 `||` to 4 `XX`, in increments of 25%
        int dam25 = shirt.max_damage() / 4;
        int deg20 = shirt.max_damage() / 5;

        WHEN( "it is undamaged" ) {
            shirt.set_damage( 0 );
            REQUIRE( shirt.damage() == 0 );
            REQUIRE( shirt.damage_level() == 0 );

            // green `++`
            THEN( "it appears undamaged" ) {
                CHECK( shirt.tname() == "<color_c_green>++</color>\u00A0long-sleeved shirt (poor fit)" );
            }
        }

        WHEN( "it is minimally damaged" ) {
            shirt.set_damage( 1 );
            REQUIRE( shirt.damage() == 1 );
            REQUIRE( shirt.damage_level() == 1 );

            // light_green `||`
            THEN( "it appears damaged" ) {
                CHECK( shirt.tname() == "<color_c_light_green>||</color>\u00A0long-sleeved shirt (poor fit)" );
            }
        }

        WHEN( "it is one-quarter damaged" ) {
            shirt.set_damage( dam25 );
            REQUIRE( shirt.damage() == dam25 );
            REQUIRE( shirt.damage_level() == 2 );

            // yellow `|\`
            THEN( "it appears slightly damaged" ) {
                CHECK( shirt.tname() == "<color_c_yellow>|\\</color>\u00A0long-sleeved shirt (poor fit)" );
            }
        }

        WHEN( "it is half damaged" ) {
            shirt.set_damage( dam25 * 2 );
            REQUIRE( shirt.damage() == dam25 * 2 );
            REQUIRE( shirt.damage_level() == 3 );

            // light_red `|.`
            THEN( "it appears moderately damaged" ) {
                CHECK( shirt.tname() == "<color_c_light_red>|.</color>\u00A0long-sleeved shirt (poor fit)" );
            }
        }

        WHEN( "it is three-quarters damaged" ) {
            shirt.set_damage( dam25 * 3 );
            REQUIRE( shirt.damage() == dam25 * 3 );
            REQUIRE( shirt.damage_level() == 4 );

            // red `\.`
            THEN( "it appears heavily damaged" ) {
                CHECK( shirt.tname() == "<color_c_red>\\.</color>\u00A0long-sleeved shirt (poor fit)" );
            }
        }

        WHEN( "it is totally damaged" ) {
            shirt.set_damage( dam25 * 4 );
            REQUIRE( shirt.damage() == dam25 * 4 );
            REQUIRE( shirt.damage_level() == 5 );

            // dark gray `XX`
            THEN( "it appears almost destroyed" ) {
                CHECK( shirt.tname() == "<color_c_dark_gray>XX</color>\u00A0long-sleeved shirt (poor fit)" );
            }
        }

        WHEN( "it is one quarter degraded" ) {
            deg_test.set_degradation( deg20 );
            REQUIRE( deg_test.degradation() == deg20 );
            REQUIRE( deg_test.damage_level() == 1 );

            // yellow bar
            THEN( "it appears slightly degraded" ) {
                CHECK( deg_test.tname() ==
                       "<color_c_light_green>||</color><color_c_yellow>\u2587</color>\u00A0baseball" );
            }
        }

        WHEN( "it is half degraded" ) {
            deg_test.set_degradation( deg20 * 2 );
            REQUIRE( deg_test.degradation() == deg20 * 2 );
            REQUIRE( deg_test.damage_level() == 2 );

            // magenta bar
            THEN( "it appears slightly more degraded" ) {
                CHECK( deg_test.tname() ==
                       "<color_c_yellow>|\\</color><color_c_magenta>\u2585</color>\u00A0baseball" );
            }
        }

        WHEN( "it is three quarters degraded" ) {
            deg_test.set_degradation( deg20 * 3 );
            REQUIRE( deg_test.degradation() == deg20 * 3 );
            REQUIRE( deg_test.damage_level() == 3 );

            // light red bar
            THEN( "it appears very degraded" ) {
                CHECK( deg_test.tname() ==
                       "<color_c_light_red>|.</color><color_c_light_red>\u2583</color>\u00A0baseball" );
            }
        }

        WHEN( "it is totally degraded" ) {
            deg_test.set_degradation( deg20 * 4 );
            REQUIRE( deg_test.degradation() == deg20 * 4 );
            REQUIRE( deg_test.damage_level() == 4 );

            // short red bar
            THEN( "it appears extremely degraded" ) {
                CHECK( deg_test.tname() ==
                       "<color_c_red>\\.</color><color_c_red>\u2581</color>\u00A0baseball" );
            }
        }
    }

    GIVEN( "ITEM_HEALTH option set to 'desc'" ) {
        override_option opt( "ITEM_HEALTH", "descriptions" );
        item shirt( "longshirt" );
        item corpse( "corpse" );
        REQUIRE( shirt.is_armor() );
        int dam25 = shirt.max_damage() / 4;

        WHEN( "it is undamaged" ) {
            shirt.set_damage( 0 );
            REQUIRE( shirt.damage() == 0 );
            REQUIRE( shirt.damage_level() == 0 );

            corpse.set_damage( 0 );
            REQUIRE( corpse.damage() == 0 );
            REQUIRE( corpse.damage_level() == 0 );

            THEN( "it appears undamaged" ) {
                CHECK( shirt.tname() == "long-sleeved shirt (poor fit)" );
                CHECK( corpse.tname() == "corpse of a human (fresh)" );
            }
        }

        WHEN( "it is minimally damaged" ) {
            shirt.set_damage( 1 );
            REQUIRE( shirt.damage() == 1 );
            REQUIRE( shirt.damage_level() == 1 );

            corpse.set_damage( 1 );
            REQUIRE( corpse.damage() == 1 );
            REQUIRE( corpse.damage_level() == 1 );

            THEN( "it appears damaged" ) {
                CHECK( shirt.tname() == "long-sleeved shirt (poor fit)" );
                CHECK( corpse.tname() == "bruised corpse of a human (fresh)" );
            }
        }

        WHEN( "it is one-quarter damaged" ) {
            shirt.set_damage( dam25 );
            REQUIRE( shirt.damage() == dam25 );
            REQUIRE( shirt.damage_level() == 2 );

            corpse.set_damage( dam25 );
            REQUIRE( corpse.damage() == dam25 );
            REQUIRE( corpse.damage_level() == 2 );

            THEN( "it appears slightly damaged" ) {
                CHECK( shirt.tname() == "ripped long-sleeved shirt (poor fit)" );
                CHECK( corpse.tname() == "damaged corpse of a human (fresh)" );
            }
        }

        WHEN( "it is half damaged" ) {
            shirt.set_damage( dam25 * 2 );
            REQUIRE( shirt.damage() == dam25 * 2 );
            REQUIRE( shirt.damage_level() == 3 );

            corpse.set_damage( dam25 * 2 );
            REQUIRE( corpse.damage() == dam25 * 2 );
            REQUIRE( corpse.damage_level() == 3 );


            THEN( "it appears moderately damaged" ) {
                CHECK( shirt.tname() == "torn long-sleeved shirt (poor fit)" );
                CHECK( corpse.tname() == "mangled corpse of a human (fresh)" );
            }
        }

        WHEN( "it is three-quarters damaged" ) {
            shirt.set_damage( dam25 * 3 );
            REQUIRE( shirt.damage() == dam25 * 3 );
            REQUIRE( shirt.damage_level() == 4 );

            corpse.set_damage( dam25 * 3 );
            REQUIRE( corpse.damage() == dam25 * 3 );
            REQUIRE( corpse.damage_level() == 4 );

            THEN( "it appears heavily damaged" ) {
                CHECK( shirt.tname() == "shredded long-sleeved shirt (poor fit)" );
                CHECK( corpse.tname() == "pulped corpse of a human (fresh)" );
            }
        }

        WHEN( "it is totally damaged" ) {
            shirt.set_damage( dam25 * 4 );
            REQUIRE( shirt.damage() == dam25 * 4 );
            REQUIRE( shirt.damage_level() == 5 );

            corpse.set_damage( dam25 * 4 );
            REQUIRE( corpse.damage() == dam25 * 4 );
            REQUIRE( corpse.damage_level() == 5 );

            THEN( "it appears almost destroyed" ) {
                CHECK( shirt.tname() == "tattered long-sleeved shirt (poor fit)" );
                CHECK( corpse.tname() == "pulped corpse of a human (fresh)" );
            }
        }
    }

    GIVEN( "ITEM_HEALTH option set to 'both'" ) {
        override_option opt( "ITEM_HEALTH", "both" );
        item shirt( "longshirt" );
        REQUIRE( shirt.is_armor() );
        int dam25 = shirt.max_damage() / 4;

        WHEN( "it is undamaged" ) {
            shirt.set_damage( 0 );
            REQUIRE( shirt.damage() == 0 );
            REQUIRE( shirt.damage_level() == 0 );

            THEN( "it appears undamaged" ) {
                CHECK( shirt.tname() == "<color_c_green>++</color>\u00A0long-sleeved shirt (poor fit)" );
            }
        }

        WHEN( "it is minimally damaged" ) {
            shirt.set_damage( 1 );
            REQUIRE( shirt.damage() == 1 );
            REQUIRE( shirt.damage_level() == 1 );

            THEN( "it appears damaged" ) {
                CHECK( shirt.tname() == "<color_c_light_green>||</color>\u00A0long-sleeved shirt (poor fit)" );
            }
        }

        WHEN( "it is one-quarter damaged" ) {
            shirt.set_damage( dam25 );
            REQUIRE( shirt.damage() == dam25 );
            REQUIRE( shirt.damage_level() == 2 );

            THEN( "it appears slightly damaged" ) {
                CHECK( shirt.tname() == "<color_c_yellow>|\\</color>\u00A0ripped long-sleeved shirt (poor fit)" );
            }
        }

        WHEN( "it is half damaged" ) {
            shirt.set_damage( dam25 * 2 );
            REQUIRE( shirt.damage() == dam25 * 2 );
            REQUIRE( shirt.damage_level() == 3 );

            THEN( "it appears moderately damaged" ) {
                CHECK( shirt.tname() == "<color_c_light_red>|.</color>\u00A0torn long-sleeved shirt (poor fit)" );
            }
        }

        WHEN( "it is three-quarters damaged" ) {
            shirt.set_damage( dam25 * 3 );
            REQUIRE( shirt.damage() == dam25 * 3 );
            REQUIRE( shirt.damage_level() == 4 );

            THEN( "it appears heavily damaged" ) {
                CHECK( shirt.tname() ==
                       "<color_c_red>\\.</color>\u00A0shredded long-sleeved shirt (poor fit)" );
            }
        }

        WHEN( "it is totally damaged" ) {
            shirt.set_damage( dam25 * 4 );
            REQUIRE( shirt.damage() == dam25 * 4 );
            REQUIRE( shirt.damage_level() == 5 );

            THEN( "it appears almost destroyed" ) {
                CHECK( shirt.tname() ==
                       "<color_c_dark_gray>XX</color>\u00A0tattered long-sleeved shirt (poor fit)" );
            }
        }
    }
}

TEST_CASE( "weapon_fouling", "[item][tname][fouling][dirt]" )
{
    GIVEN( "a gun with potential fouling" ) {
        item gun( "hk_mp5" );

        Character &player_character = get_player_character();
        // Ensure the player and gun are normal size to prevent "too big" or "too small" suffix in tname
        player_character.clear_mutations();
        REQUIRE( gun.get_sizing( player_character ) == item::sizing::ignore );
        REQUIRE_FALSE( gun.has_flag( flag_OVERSIZE ) );
        REQUIRE_FALSE( gun.has_flag( flag_UNDERSIZE ) );

        WHEN( "it is perfectly clean" ) {
            gun.set_var( "dirt", 0 );
            CHECK( gun.tname() == "H&K MP5A2" );
        }

        WHEN( "it is fouled" ) {
            gun.faults.insert( fault_gun_dirt );
            REQUIRE( gun.has_fault( fault_gun_dirt ) );

            // Max dirt is 10,000

            THEN( "minimal fouling is not indicated" ) {
                gun.set_var( "dirt", 1000 );
                CHECK( gun.tname() == "H&K MP5A2" );
            }

            // U+2581 'Lower one eighth block'
            THEN( "20%% fouling is indicated with a thin white bar" ) {
                gun.set_var( "dirt", 2000 );
                CHECK( gun.tname() == "<color_white>\u2581</color>H&K MP5A2" );
            }

            // U+2583 'Lower three eighths block'
            THEN( "40%% fouling is indicated with a slight gray bar" ) {
                gun.set_var( "dirt", 4000 );
                CHECK( gun.tname() == "<color_light_gray>\u2583</color>H&K MP5A2" );
            }

            // U+2585 'Lower five eighths block'
            THEN( "60%% fouling is indicated with a medium gray bar" ) {
                gun.set_var( "dirt", 6000 );
                CHECK( gun.tname() == "<color_light_gray>\u2585</color>H&K MP5A2" );
            }

            // U+2585 'Lower seven eighths block'
            THEN( "80%% fouling is indicated with a tall dark gray bar" ) {
                gun.set_var( "dirt", 8000 );
                CHECK( gun.tname() == "<color_dark_gray>\u2587</color>H&K MP5A2" );
            }

            // U+2588 'Full block'
            THEN( "100%% fouling is indicated with a full brown bar" ) {
                gun.set_var( "dirt", 10000 );
                CHECK( gun.tname() == "<color_brown>\u2588</color>H&K MP5A2" );
            }
        }
    }
}

// make sure ordering still works with pockets
TEST_CASE( "molle_vest_additional_pockets", "[item][tname]" )
{
    item addition_vest( "test_load_bearing_vest" );
    addition_vest.get_contents().add_pocket( item( "holster" ) );

    CHECK( addition_vest.tname( 1 ) ==
           "<color_c_green>++</color>\u00A0load bearing vest+1" );
}

TEST_CASE( "nested_items_tname", "[item][tname]" )
{
    item backpack_hiking( itype_backpack_hiking );
    item purse( itype_purse );
    item rock( itype_test_rock );
    item rock2( itype_rock );
    const std::string color_pref =
        "<color_c_green>++</color>\u00A0";

    const std::string nesting_sym = ">";

    SECTION( "single stack inside" ) {

        backpack_hiking.put_in( rock, item_pocket::pocket_type::CONTAINER );

        std::string const rock_nested_tname = colorize( rock.tname(), rock.color_in_inventory() );
        std::string const rocks_nested_tname = colorize( rock.tname( 2 ), rock.color_in_inventory() );
        REQUIRE( rock_nested_tname == "<color_c_light_gray>TEST rock</color>" );
        SECTION( "single rock" ) {
            CHECK( backpack_hiking.tname( 1 ) == color_pref + "hiking backpack " + nesting_sym + " " +
                   rock_nested_tname );
        }
        SECTION( "several rocks" ) {
            backpack_hiking.put_in( rock, item_pocket::pocket_type::CONTAINER );
            CHECK( backpack_hiking.tname( 1 ) == color_pref + "hiking backpack " + nesting_sym +
                   " " + rocks_nested_tname + " (2)" );
        }
        SECTION( "several stacks" ) {
            backpack_hiking.put_in( rock, item_pocket::pocket_type::CONTAINER );
            backpack_hiking.put_in( rock2, item_pocket::pocket_type::CONTAINER );
            CHECK( backpack_hiking.tname( 1 ) == color_pref + "hiking backpack " + nesting_sym + " 2 items" );
        }
        SECTION( "container has whitelist" ) {
            std::string const wlmark = "⁺";
            REQUIRE( backpack_hiking.get_all_contained_pockets().size() >= 2 );
            backpack_hiking.get_all_contained_pockets().front()->settings.whitelist_item(
                itype_rock );
            CHECK( backpack_hiking.tname( 1 ) == color_pref + "hiking backpack" +
                   colorize( wlmark, c_dark_gray ) + " " + nesting_sym + " " + rock_nested_tname );
            backpack_hiking.get_all_contained_pockets()[1]->settings.set_was_edited();
            // different pocket was edited
            CHECK( backpack_hiking.tname( 1 ) == color_pref + "hiking backpack" +
                   colorize( wlmark, c_dark_gray ) + " " + nesting_sym + " " + rock_nested_tname );
            backpack_hiking.get_all_contained_pockets()[0]->settings.set_was_edited();
            CHECK( backpack_hiking.tname( 1 ) == color_pref + "hiking backpack" +
                   colorize( wlmark, c_light_gray ) + " " + nesting_sym + " " + rock_nested_tname );
        }
    }

    std::string const purse_color = get_tag_from_color( purse.color_in_inventory() );
    std::string const color_end_tag = "</color>";
    SECTION( "multi-level nesting" ) {
        purse.put_in( rock, item_pocket::pocket_type::CONTAINER );

        SECTION( "single rock" ) {
            backpack_hiking.put_in( purse, item_pocket::pocket_type::CONTAINER );
            CHECK( backpack_hiking.tname( 1 ) ==
                   color_pref + "hiking backpack " +
                   nesting_sym + " " + purse_color + color_pref + "purse " +
                   nesting_sym + " 1 item" +
                   color_end_tag );
        }

        SECTION( "several rocks" ) {
            purse.put_in( rock2, item_pocket::pocket_type::CONTAINER );

            backpack_hiking.put_in( purse, item_pocket::pocket_type::CONTAINER );

            CHECK( backpack_hiking.tname( 1 ) ==
                   color_pref + "hiking backpack " +
                   nesting_sym + " " + purse_color + color_pref + "purse " +
                   nesting_sym + " 2 items" +
                   color_end_tag );
        }

        SECTION( "several purses" ) {
            backpack_hiking.put_in( purse, item_pocket::pocket_type::CONTAINER );
            backpack_hiking.put_in( purse, item_pocket::pocket_type::CONTAINER );

            CHECK( backpack_hiking.tname( 1 ) == color_pref + "hiking backpack " + nesting_sym +
                   " " + purse_color + color_pref + "purses" + color_end_tag + " (2)" );
        }
    }

    SECTION( "non-standard pocket: software" ) {
        item usb_drive( "usb_drive" );
        item medisoft( "software_medical" );
        std::string const medisoft_nested_tname = colorize( medisoft.tname(),
                medisoft.color_in_inventory() );
        REQUIRE( usb_drive.is_software_storage() );
        REQUIRE( medisoft.is_software() );
        REQUIRE( medisoft_nested_tname == "<color_c_light_gray>MediSoft</color>" );
        usb_drive.put_in( medisoft, item_pocket::pocket_type::SOFTWARE );
        CHECK( usb_drive.tname( 1 ) == "USB drive " + nesting_sym + " " + medisoft_nested_tname );
    }
}
