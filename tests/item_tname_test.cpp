#include <memory>
#include <set>
#include <string>

#include "avatar.h"
#include "calendar.h"
#include "catch/catch.hpp"
#include "game.h"
#include "item.h"
#include "itype.h"
#include "options_helpers.h"
#include "type_id.h"
#include "value_ptr.h"

static const std::string flag_COLD( "COLD" );
static const std::string flag_DIAMOND( "DIAMOND" );
static const std::string flag_FILTHY( "FILTHY" );
static const std::string flag_FROZEN( "FROZEN" );
static const std::string flag_HIDDEN_HALLU( "HIDDEN_HALLU" );
static const std::string flag_HIDDEN_POISON( "HIDDEN_POISON" );
static const std::string flag_HOT( "HOT" );
static const std::string flag_MELTS( "MELTS" );
static const std::string flag_WET( "WET" );

static const fault_id fault_gun_dirt( "fault_gun_dirt" );

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

TEST_CASE( "food with hidden effects", "[item][tname][hidden]" )
{
    g->u.clear_mutations();

    GIVEN( "food with hidden poison" ) {
        item coffee = item( "coffee_pod" );
        REQUIRE( coffee.is_food() );
        REQUIRE( coffee.has_flag( flag_HIDDEN_POISON ) );

        WHEN( "avatar has level 2 survival skill" ) {
            g->u.set_skill_level( skill_survival, 2 );
            REQUIRE( g->u.get_skill_level( skill_survival ) == 2 );

            THEN( "they cannot see it is poisonous" ) {
                CHECK( coffee.tname() == "Kentucky coffee pod" );
            }
        }

        WHEN( "avatar has level 3 survival skill" ) {
            g->u.set_skill_level( skill_survival, 3 );
            REQUIRE( g->u.get_skill_level( skill_survival ) == 3 );

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
            g->u.set_skill_level( skill_survival, 4 );
            REQUIRE( g->u.get_skill_level( skill_survival ) == 4 );

            THEN( "they cannot see it is hallucinogenic" ) {
                CHECK( mushroom.tname() == "mushroom (fresh)" );
            }
        }

        WHEN( "avatar has level 5 survival skill" ) {
            g->u.set_skill_level( skill_survival, 5 );
            REQUIRE( g->u.get_skill_level( skill_survival ) == 5 );

            THEN( "they see it is hallucinogenic" ) {
                CHECK( mushroom.tname() == "mushroom (hallucinogenic) (fresh)" );
            }
        }
    }
}

TEST_CASE( "wet item", "[item][tname][wet]" )
{
    item rag( "rag" );
    rag.set_flag( flag_WET );
    REQUIRE( rag.has_flag( flag_WET ) );

    CHECK( rag.tname() == "rag (wet)" );
}

TEST_CASE( "filthy item", "[item][tname][filthy]" )
{
    override_option opt( "FILTHY_MORALE", "true" );
    item rag( "rag" );
    rag.set_flag( flag_FILTHY );
    REQUIRE( rag.is_filthy() );

    CHECK( rag.tname() == "rag (filthy)" );
}

TEST_CASE( "diamond item", "[item][tname][diamond]" )
{
    item katana( "katana" );
    katana.set_flag( flag_DIAMOND );
    REQUIRE( katana.has_flag( flag_DIAMOND ) );

    CHECK( katana.tname() == "diamond katana" );
}

TEST_CASE( "truncated item name", "[item][tname][truncate]" )
{
    SECTION( "plain item name can be truncated" ) {
        item katana( "katana" );

        CHECK( katana.tname() == "katana" );
        CHECK( katana.tname( 1, false, 5 ) == "katan" );
    }

    // TODO: color-coded or otherwise embellished item name can be truncated
}

TEST_CASE( "engine displacement volume", "[item][tname][engine]" )
{
    item vtwin = item( "v2_combustion" );
    item v12diesel = item( "v12_diesel" );
    item turbine = item( "small_turbine_engine" );

    REQUIRE( vtwin.engine_displacement() == 100 );
    REQUIRE( v12diesel.engine_displacement() == 700 );
    REQUIRE( turbine.engine_displacement() == 2700 );

    CHECK( vtwin.tname() == "1.0L V-twin engine" );
    CHECK( v12diesel.tname() == "7.0L V12 diesel engine" );
    CHECK( turbine.tname() == "27.0L 1350 hp gas turbine engine" );
}

TEST_CASE( "wheel diameter", "[item][tname][wheel]" )
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

TEST_CASE( "item health or damage bar", "[item][tname][health][damage]" )
{
    GIVEN( "some clothing" ) {
        item shirt( "longshirt" );
        REQUIRE( shirt.is_armor() );

        // Ensure the health bar option is enabled
        override_option opt( "ITEM_HEALTH_BAR", "true" );

        // Damage bar uses a scale of 0 `||` to 4 `XX`, in increments of 25%
        int dam25 = shirt.max_damage() / 4;

        WHEN( "it is undamaged" ) {
            shirt.set_damage( 0 );
            REQUIRE( shirt.damage() == 0 );
            REQUIRE( shirt.damage_level( 4 ) == 0 );

            // green `||`
            THEN( "it appears undamaged" ) {
                CHECK( shirt.tname() == "<color_c_light_green>||\u00A0</color>long-sleeved shirt (poor fit)" );
            }
        }

        WHEN( "is is one-quarter damaged" ) {
            shirt.set_damage( dam25 );
            REQUIRE( shirt.damage() == dam25 );
            REQUIRE( shirt.damage_level( 4 ) == 1 );

            // yellow `|\`
            THEN( "it appears slightly damaged" ) {
                CHECK( shirt.tname() == "<color_c_yellow>|\\\u00A0</color>long-sleeved shirt (poor fit)" );
            }
        }

        WHEN( "it is half damaged" ) {
            shirt.set_damage( dam25 * 2 );
            REQUIRE( shirt.damage() == dam25 * 2 );
            REQUIRE( shirt.damage_level( 4 ) == 2 );

            // magenta `|.`
            THEN( "it appears moderately damaged" ) {
                CHECK( shirt.tname() == "<color_c_magenta>|.\u00A0</color>long-sleeved shirt (poor fit)" );
            }
        }

        WHEN( "it is three-quarters damaged" ) {
            shirt.set_damage( dam25 * 3 );
            REQUIRE( shirt.damage() == dam25 * 3 );
            REQUIRE( shirt.damage_level( 4 ) == 3 );

            // red `\.`
            THEN( "it appears heavily damaged" ) {
                CHECK( shirt.tname() == "<color_c_light_red>\\.\u00A0</color>long-sleeved shirt (poor fit)" );
            }
        }

        WHEN( "it is totally damaged" ) {
            shirt.set_damage( dam25 * 4 );
            REQUIRE( shirt.damage() == dam25 * 4 );
            REQUIRE( shirt.damage_level( 4 ) == 4 );

            // dark gray `XX`
            THEN( "it appears almost destroyed" ) {
                CHECK( shirt.tname() == "<color_c_dark_gray>XX\u00A0</color>long-sleeved shirt (poor fit)" );
            }
        }
    }

    GIVEN( "ITEM_HEALTH_BAR option is disabled" ) {
        override_option opt( "ITEM_HEALTH_BAR", "false" );

        THEN( "clothing health bars are hidden" ) {
            item shirt( "longshirt" );
            REQUIRE( shirt.is_armor() );

            CHECK( shirt.tname() == "long-sleeved shirt (poor fit)" );
        }
    }
}

TEST_CASE( "weapon fouling", "[item][tname][fouling][dirt]" )
{
    GIVEN( "a gun with potential fouling" ) {
        item gun( "hk_mp5" );

        // Ensure the player and gun are normal size to prevent "too big" or "too small" suffix in tname
        g->u.clear_mutations();
        REQUIRE( gun.get_sizing( g-> u, true ) == item::sizing::human_sized_human_char );
        REQUIRE_FALSE( gun.has_flag( "OVERSIZE" ) );
        REQUIRE_FALSE( gun.has_flag( "UNDERSIZE" ) );

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
