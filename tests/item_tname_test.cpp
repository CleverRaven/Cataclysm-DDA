#include <memory>
#include <string>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "color.h"
#include "flag.h"
#include "item.h"
#include "item_category.h"
#include "item_contents.h"
#include "item_pocket.h"
#include "item_tname.h"
#include "itype.h"
#include "options_helpers.h"
#include "pocket_type.h"
#include "ret_val.h"
#include "type_id.h"
#include "value_ptr.h"

#if defined(LOCALIZE)
#include "translation_manager.h"
#include "translations.h"
#endif

static const fault_id fault_gun_dirt( "fault_gun_dirt" );

static const item_category_id item_category_veh_parts( "veh_parts" );

static const itype_id itype_backpack( "backpack" );
static const itype_id itype_backpack_hiking( "backpack_hiking" );
static const itype_id itype_bag_garbage( "bag_garbage" );
static const itype_id itype_bag_plastic( "bag_plastic" );
static const itype_id itype_carrot( "carrot" );
static const itype_id itype_coffee_pod( "coffee_pod" );
static const itype_id itype_corpse( "corpse" );
static const itype_id itype_hammer( "hammer" );
static const itype_id itype_hk_mp5( "hk_mp5" );
static const itype_id itype_holster( "holster" );
static const itype_id itype_jeans( "jeans" );
static const itype_id itype_juniper( "juniper" );
static const itype_id itype_katana( "katana" );
static const itype_id itype_longshirt( "longshirt" );
static const itype_id itype_milkshake( "milkshake" );
static const itype_id itype_mushroom( "mushroom" );
static const itype_id itype_pants( "pants" );
static const itype_id itype_pepper( "pepper" );
static const itype_id itype_pine_nuts( "pine_nuts" );
static const itype_id itype_protein_bar_evac( "protein_bar_evac" );
static const itype_id itype_purse( "purse" );
static const itype_id itype_rock( "rock" );
static const itype_id itype_salt( "salt" );
static const itype_id itype_sauerkraut( "sauerkraut" );
static const itype_id itype_sheet_cotton( "sheet_cotton" );
static const itype_id itype_test_baseball( "test_baseball" );
static const itype_id itype_test_load_bearing_vest( "test_load_bearing_vest" );
static const itype_id itype_test_rock( "test_rock" );
static const itype_id itype_wheel( "wheel" );
static const itype_id itype_wheel_armor( "wheel_armor" );
static const itype_id itype_wheel_wide( "wheel_wide" );

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
        item coffee = item( itype_coffee_pod );
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
        item mushroom = item( itype_mushroom );
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
        item shake( itype_milkshake );
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
        item nut( itype_pine_nuts );
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
        item hammer( itype_hammer );
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
    item sheet_cotton( itype_sheet_cotton );
    sheet_cotton.set_flag( flag_WET );
    REQUIRE( sheet_cotton.has_flag( flag_WET ) );

    CHECK( sheet_cotton.tname() == "cotton sheet (wet)" );
}

TEST_CASE( "filthy_item", "[item][tname][filthy]" )
{
    item sheet_cotton( itype_sheet_cotton );
    sheet_cotton.set_flag( flag_FILTHY );
    REQUIRE( sheet_cotton.is_filthy() );

    CHECK( sheet_cotton.tname() == "cotton sheet (filthy)" );
}

TEST_CASE( "diamond_item", "[item][tname][diamond]" )
{
    item katana( itype_katana );
    katana.set_flag( flag_DIAMOND );
    REQUIRE( katana.has_flag( flag_DIAMOND ) );

    CHECK( katana.tname() == "diamond katana" );
}

TEST_CASE( "wheel_diameter", "[item][tname][wheel]" )
{
    item wheel17 = item( itype_wheel );
    item wheel24 = item( itype_wheel_wide );
    item wheel32 = item( itype_wheel_armor );

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
        item shirt( itype_longshirt );
        item deg_test( itype_test_baseball );
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
        item shirt( itype_longshirt );
        item corpse( itype_corpse );
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
        item shirt( itype_longshirt );
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
        item gun( itype_hk_mp5 );

        Character &player_character = get_player_character();
        // Ensure the player and gun are normal size to prevent "too big" or "too small" suffix in tname
        player_character.clear_mutations();
        REQUIRE( gun.get_sizing( player_character ) == item::sizing::ignore );
        REQUIRE_FALSE( gun.has_flag( flag_OVERSIZE ) );
        REQUIRE_FALSE( gun.has_flag( flag_UNDERSIZE ) );

        WHEN( "it is perfectly clean" ) {
            gun.set_var( "dirt", 0 );
            CHECK( gun.tname() == "H&K MP5A2 submachine gun" );
        }

        WHEN( "it is fouled" ) {
            gun.set_fault( fault_gun_dirt );
            REQUIRE( gun.has_fault( fault_gun_dirt ) );

            // Max dirt is 10,000

            THEN( "minimal fouling is not indicated" ) {
                gun.set_var( "dirt", 1000 );
                CHECK( gun.tname() == "H&K MP5A2 submachine gun" );
            }

            // U+2581 'Lower one eighth block'
            THEN( "20%% fouling is indicated with a thin white bar" ) {
                gun.set_var( "dirt", 2000 );
                CHECK( gun.tname() == "<color_white>\u2581</color>H&K MP5A2 submachine gun" );
            }

            // U+2583 'Lower three eighths block'
            THEN( "40%% fouling is indicated with a slight gray bar" ) {
                gun.set_var( "dirt", 4000 );
                CHECK( gun.tname() == "<color_light_gray>\u2583</color>H&K MP5A2 submachine gun" );
            }

            // U+2585 'Lower five eighths block'
            THEN( "60%% fouling is indicated with a medium gray bar" ) {
                gun.set_var( "dirt", 6000 );
                CHECK( gun.tname() == "<color_light_gray>\u2585</color>H&K MP5A2 submachine gun" );
            }

            // U+2585 'Lower seven eighths block'
            THEN( "80%% fouling is indicated with a tall dark gray bar" ) {
                gun.set_var( "dirt", 8000 );
                CHECK( gun.tname() == "<color_dark_gray>\u2587</color>H&K MP5A2 submachine gun" );
            }

            // U+2588 'Full block'
            THEN( "100%% fouling is indicated with a full brown bar" ) {
                gun.set_var( "dirt", 10000 );
                CHECK( gun.tname() == "<color_brown>\u2588</color>H&K MP5A2 submachine gun" );
            }
        }
    }
}

// make sure ordering still works with pockets
TEST_CASE( "molle_vest_additional_pockets", "[item][tname]" )
{
    item addition_vest( itype_test_load_bearing_vest );
    addition_vest.get_contents().add_pocket( item( itype_holster ) );

    CHECK( addition_vest.tname( 1 ) ==
           "<color_c_green>++</color>\u00A0load bearing vest+1" );
}

TEST_CASE( "nested_items_tname", "[item][tname]" )
{
    item backpack_hiking( itype_backpack_hiking );
    item purse( itype_purse );
    item rock( itype_test_rock );
    item bag( itype_bag_plastic );
    item bag_garbage( itype_bag_garbage );
    item hammer( itype_hammer );
    rock.clear_itype_variant();
    item rock2( itype_rock );
    const std::string color_pref =
        "<color_c_green>++</color>\u00A0";

    const std::string nesting_sym = ">";

    SECTION( "single stack inside" ) {

        backpack_hiking.put_in( rock, pocket_type::CONTAINER );

        std::string const rock_nested_tname = colorize( rock.tname(), rock.color_in_inventory() );
        std::string const hammers_nested_tname = colorize( hammer.tname( 2 ), rock.color_in_inventory() );
        std::string const rocks_nested_tname = colorize( rock.tname( 2 ), rock.color_in_inventory() );
        REQUIRE( rock_nested_tname == "<color_c_light_gray>TEST rock</color>" );
        SECTION( "single rock" ) {
            CHECK( backpack_hiking.tname( 1 ) == color_pref + "hiking backpack " + nesting_sym + " " +
                   rock_nested_tname );
        }
        SECTION( "several rocks" ) {
            backpack_hiking.put_in( rock, pocket_type::CONTAINER );
            CHECK( backpack_hiking.tname( 1 ) == color_pref + "hiking backpack " + nesting_sym +
                   " 2 " + rocks_nested_tname );
        }
        SECTION( "several stacks" ) {
            REQUIRE( rock.get_category_shallow().id != purse.get_category_shallow().id );
            backpack_hiking.put_in( rock, pocket_type::CONTAINER );
            backpack_hiking.put_in( purse, pocket_type::CONTAINER );
            CHECK( backpack_hiking.tname( 1 ) == color_pref + "hiking backpack " + nesting_sym + " 2 items" );
        }
        SECTION( "several stacks of same category" ) {
            REQUIRE( rock.get_category_shallow().id == rock2.get_category_shallow().id );
            backpack_hiking.put_in( rock, pocket_type::CONTAINER );
            backpack_hiking.put_in( rock2, pocket_type::CONTAINER );
            CHECK( backpack_hiking.tname( 1 ) ==
                   color_pref + "hiking backpack " + nesting_sym + " 2 " +
                   colorize( rock.get_category_shallow().name_noun( 2 ), c_magenta ) );
        }
        SECTION( "several stacks of variants" ) {
            item rock_blue( itype_test_rock );
            item rock_green( itype_test_rock );
            rock_blue.set_itype_variant( "test_rock_blue" );
            rock_green.set_itype_variant( "test_rock_green" );
            backpack_hiking.put_in( rock_blue, pocket_type::CONTAINER );
            backpack_hiking.put_in( rock_green, pocket_type::CONTAINER );
            CHECK( backpack_hiking.tname( 1 ) == color_pref + "hiking backpack " + nesting_sym +
                   " 3 " + rocks_nested_tname );
        }
        SECTION( "dominant type" ) {
            backpack_hiking.clear_items();
            REQUIRE( backpack_hiking.put_in( hammer, pocket_type::CONTAINER ).success() );
            REQUIRE( backpack_hiking.put_in( hammer, pocket_type::CONTAINER ).success() );
            REQUIRE( backpack_hiking.put_in( bag, pocket_type::CONTAINER ).success() );
            CHECK( backpack_hiking.tname( 1 ) == color_pref + "hiking backpack " + nesting_sym + " 2 " +
                   hammers_nested_tname + " / 3 items" );
        }
        SECTION( "dominant category" ) {
            backpack_hiking.clear_items();
            REQUIRE( rock.get_category_shallow().id == rock2.get_category_shallow().id );
            REQUIRE( backpack_hiking.put_in( rock, pocket_type::CONTAINER ).success() );
            REQUIRE( backpack_hiking.put_in( rock2, pocket_type::CONTAINER ).success() );
            REQUIRE( rock.get_category_of_contents().id == rock2.get_category_of_contents().id );
            REQUIRE( rock.get_category_of_contents().id != purse.get_category_of_contents().id );
            REQUIRE( backpack_hiking.put_in( purse, pocket_type::CONTAINER ).success() );
            CHECK( backpack_hiking.tname( 1 ) ==
                   color_pref + "hiking backpack " + nesting_sym + " 2 " +
                   colorize( rock.get_category_shallow().name_noun( 2 ), c_magenta ) + " / 3 items" );
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
        purse.put_in( rock, pocket_type::CONTAINER );

        SECTION( "single rock" ) {
            backpack_hiking.put_in( purse, pocket_type::CONTAINER );
            CHECK( backpack_hiking.tname( 1 ) ==
                   color_pref + "hiking backpack " +
                   nesting_sym + " " + purse_color + color_pref + "purse " +
                   nesting_sym + " 1 item" +
                   color_end_tag );
        }

        SECTION( "several rocks" ) {
            purse.put_in( rock2, pocket_type::CONTAINER );

            backpack_hiking.put_in( purse, pocket_type::CONTAINER );

            CHECK( backpack_hiking.tname( 1 ) ==
                   color_pref + "hiking backpack " +
                   nesting_sym + " " + purse_color + color_pref + "purse " +
                   nesting_sym + " 2 items" +
                   color_end_tag );
        }

        SECTION( "several purses" ) {
            backpack_hiking.put_in( purse, pocket_type::CONTAINER );
            backpack_hiking.put_in( purse, pocket_type::CONTAINER );

            CHECK( backpack_hiking.tname( 1 ) == color_pref + "hiking backpack " + nesting_sym +
                   " 2 " + purse_color + color_pref + "purses" + color_end_tag );
        }
        SECTION( "dominant nested category" ) {
            REQUIRE( bag.put_in( rock, pocket_type::CONTAINER ).success() );
            REQUIRE( bag_garbage.put_in( rock, pocket_type::CONTAINER ).success() );
            REQUIRE( bag.get_category_of_contents().id == rock.get_category_of_contents().id );
            REQUIRE( bag_garbage.get_category_of_contents().id == rock.get_category_of_contents().id );
            REQUIRE( backpack_hiking.put_in( bag_garbage, pocket_type::CONTAINER ).success() );
            REQUIRE( backpack_hiking.put_in( bag, pocket_type::CONTAINER ).success() );
            item bag2( itype_bag_plastic );
            REQUIRE( bag2.put_in( hammer, pocket_type::CONTAINER ).success() );
            REQUIRE( bag.get_category_of_contents().id != bag2.get_category_of_contents().id );
            REQUIRE( backpack_hiking.put_in( bag2, pocket_type::CONTAINER ).success() );
            CHECK( backpack_hiking.tname( 1 ) ==
                   color_pref + "hiking backpack " + nesting_sym + " 2 " +
                   colorize( rock.get_category_shallow().name_noun( 2 ), c_magenta ) + " / 3 items" );
        }
    }
    tname::segment_bitset type_only;
    type_only.set( tname::segments::TYPE );
    SECTION( "aggregated food stats" ) {
        avatar &u = get_avatar();
        item salt( itype_salt );
        std::string const cat_food_str = salt.get_category_shallow().name_noun( 2 );
        item pepper( itype_pepper );
        item juniper( itype_juniper );
        item ration( itype_protein_bar_evac );
        item carrot( itype_carrot );
        item sauerkraut( itype_sauerkraut );
        item bag( itype_bag_plastic );
        std::string const carrots_tname = carrot.tname( 2, type_only );

        SECTION( "inedible" ) {
            REQUIRE( ( salt.is_food() && pepper.is_food() && juniper.is_food() && ration.is_food() &&
                       carrot.is_food() && sauerkraut.is_food() ) );

            REQUIRE( ( u.will_eat( salt ).value() == edible_rating::INEDIBLE &&
                       u.will_eat( pepper ).value() == edible_rating::INEDIBLE ) );
            REQUIRE( bag.put_in( salt, pocket_type::CONTAINER ).success() );
            REQUIRE( bag.put_in( pepper, pocket_type::CONTAINER ).success() );
            CHECK( bag.tname( 1 ) == "plastic bag > 2 " + colorize( cat_food_str, c_dark_gray ) );
        }

        SECTION( "non-perishable" ) {
            REQUIRE( ( !juniper.goes_bad() && !ration.goes_bad() ) );
            REQUIRE( ( u.will_eat( juniper ).value() == edible_rating::EDIBLE &&
                       u.will_eat( ration ).value() == edible_rating::EDIBLE ) );
            REQUIRE( bag.put_in( juniper, pocket_type::CONTAINER ).success() );
            REQUIRE( bag.put_in( ration, pocket_type::CONTAINER ).success() );
            CHECK( bag.tname( 1 ) == "plastic bag > 2 " + colorize( cat_food_str, c_cyan ) );
        }

        SECTION( "perishable" ) {
            bool same_item = GENERATE( true, false );
            bool rotten = GENERATE( true, false );
            bool both_rotten = rotten ? GENERATE( true, false ) : false;
            bool frozen = GENERATE( true, false );
            bool both_frozen = frozen ? GENERATE( true, false ) : false;
            bool mushy = !rotten ? GENERATE( true, false ) : false;
            bool both_mushy = mushy ? GENERATE( true, false ) : false;
            CAPTURE( same_item, rotten, both_rotten, mushy, both_mushy, frozen, both_frozen );

            nc_color color = c_light_cyan;
            item second_food( same_item ? carrot : sauerkraut );

            std::string fresh_str( " (fresh)" );
            std::string frozen_str;

            REQUIRE( ( carrot.goes_bad() && second_food.goes_bad() ) );
            REQUIRE( ( u.will_eat( carrot ).value() == edible_rating::EDIBLE &&
                       u.will_eat( second_food ).value() == edible_rating::EDIBLE ) );
            REQUIRE( ( carrot.is_fresh() && second_food.is_fresh() ) );

            if( rotten ) {
                carrot.set_relative_rot( 99 );
                REQUIRE_FALSE( carrot.is_fresh() );
                fresh_str = std::string{};
                if( both_rotten ) {
                    second_food.set_relative_rot( 99 );
                    fresh_str = " (rotten)";
                    color = c_brown;
                }
            } else if( mushy ) {
                carrot.set_flag( flag_MUSHY );
                fresh_str = std::string{};
                if( both_mushy ) {
                    second_food.set_flag( flag_MUSHY );
                    fresh_str = " (mushy)";
                }
            }
            if( frozen ) {
                carrot.set_flag( flag_FROZEN );
                if( both_frozen ) {
                    second_food.set_flag( flag_FROZEN );
                    frozen_str = " (frozen)";
                    color = c_dark_gray;
                }
            }
            REQUIRE( bag.put_in( carrot, pocket_type::CONTAINER ).success() );
            REQUIRE( bag.put_in( second_food, pocket_type::CONTAINER ).success() );
            if( same_item ) {
                CHECK( bag.tname( 1 ) ==
                       "plastic bag > 2 " +
                       colorize( carrots_tname + fresh_str + frozen_str, color ) );
            } else {
                CHECK( bag.tname( 1 ) == "plastic bag > 2 " +
                       colorize( cat_food_str, color ) + fresh_str +
                       frozen_str );
            }
        }
    }

    SECTION( "aggregated clothing stats" ) {
        item pants( itype_pants );
        pants.clear_itype_variant();
        pants.set_flag( flag_FIT );
        bool same_item = GENERATE( true, false );
        item second_item( same_item ? itype_pants : itype_jeans );
        second_item.clear_itype_variant();
        second_item.set_flag( flag_FIT );
        REQUIRE( ( pants.has_flag( flag_VARSIZE ) && second_item.has_flag( flag_VARSIZE ) ) );

        bool damaged = GENERATE( true, false );
        bool both_damaged = damaged ? GENERATE( true, false ) : false;
        bool unfit = GENERATE( true, false );
        bool both_unfit = unfit ? GENERATE( true, false ) : false;
        CAPTURE( same_item, damaged, both_damaged, unfit, both_unfit );

        std::string const cat_cl_str = pants.get_category_shallow().name_noun( 2 );
        std::string const pants_tname = pants.tname( 2, type_only );
        std::string dura_str = pants.durability_indicator();
        std::string fit_str;

        if( damaged ) {
            pants.inc_damage();
            dura_str = std::string{};
            if( both_damaged ) {
                second_item.inc_damage();
                dura_str = pants.durability_indicator();
            }
        }
        if( unfit ) {
            pants.unset_flag( flag_FIT );
            if( both_unfit ) {
                second_item.unset_flag( flag_FIT );
                fit_str = " (poor fit)";
            }
        }

        REQUIRE( bag.put_in( pants, pocket_type::CONTAINER ).success() );
        REQUIRE( bag.put_in( second_item, pocket_type::CONTAINER ).success() );

        if( same_item ) {
            CHECK( bag.tname( 1 ) == "plastic bag > 2 " +
                   colorize( dura_str + pants_tname + fit_str, c_light_gray ) );
        } else {
            CHECK( bag.tname( 1 ) == "plastic bag > 2 " + dura_str +
                   colorize( cat_cl_str, c_magenta ) + fit_str );
        }
    }
}

#ifdef LOCALIZE
TEST_CASE( "tname_i18n_order", "[item][tname][translations]" )
{
    item backpack( itype_backpack );
    backpack.burnt = 1;
    backpack.set_flag( flag_FILTHY );
    REQUIRE( backpack.tname() == "<color_c_green>++</color> burnt backpack (filthy)" );

    set_language( "ru" );
    TranslationManager::GetInstance().LoadDocuments( { "./data/mods/TEST_DATA/lang/mo/ru/LC_MESSAGES/TEST_DATA.mo" } );
    CHECK( backpack.tname() ==
           "<color_c_green>++</color> backpack (burnt) (filthy)" );

    set_language( "en" );
}
#endif
