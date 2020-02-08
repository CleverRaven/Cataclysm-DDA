#include <memory>
#include <string>
#include <vector>

#include "avatar.h"
#include "catch/catch.hpp"
#include "game.h"
#include "item.h"
#include "iteminfo_query.h"

static void iteminfo_test( const item &i, const iteminfo_query &q, const std::string &reference )
{
    g->u.empty_traits();
    std::vector<iteminfo> info_v;
    std::string info = i.info( info_v, &q, 1 );
    CHECK( info == reference );
}

TEST_CASE( "item description and physical attributes", "[item][iteminfo]" )
{
    iteminfo_query q( { iteminfo_parts::DESCRIPTION, iteminfo_parts::BASE_CATEGORY,
                        iteminfo_parts::BASE_PRICE, iteminfo_parts::BASE_VOLUME,
                        iteminfo_parts::BASE_WEIGHT, iteminfo_parts::BASE_MATERIAL } );
    iteminfo_test(
        item( "jug_plastic" ), q,
        "A standard plastic jug used for milk and household cleaning chemicals.\n"
        "--\n"
        "Category: <color_c_magenta>CONTAINERS</color>  Price: $<color_c_yellow>0.00</color>\n"
        "<color_c_white>Volume</color>: <color_c_yellow>3.750</color> L"
        "  Weight: <color_c_yellow>0.42</color> lbs\n"
        "--\n"
        "Material: <color_c_light_blue>Plastic</color>\n" );

}

TEST_CASE( "armor info", "[item][iteminfo]" )
{
    iteminfo_query q( { iteminfo_parts::ARMOR_BODYPARTS, iteminfo_parts::ARMOR_LAYER,
                        iteminfo_parts::ARMOR_COVERAGE, iteminfo_parts::ARMOR_WARMTH,
                        iteminfo_parts::ARMOR_ENCUMBRANCE, iteminfo_parts::ARMOR_PROTECTION
                      } );

    SECTION( "shows coverage and protection values" ) {
        iteminfo_test(
            item( "longshirt" ), q,
            "--\n"
            // NOLINTNEXTLINE(cata-text-style)
            "Covers: The <color_c_cyan>torso</color>. The <color_c_cyan>arms</color>. \n"
            // NOLINTNEXTLINE(cata-text-style)
            "Layer: <color_c_light_blue>Normal</color>. \n"
            "Coverage: <color_c_yellow>90</color>%  Warmth: <color_c_yellow>5</color>\n"
            "--\n"
            "<color_c_white>Encumbrance</color>: <color_c_yellow>3</color> <color_c_red>(poor fit)</color>\n"
            "<color_c_white>Protection</color>: Bash: <color_c_yellow>1</color>  Cut: <color_c_yellow>1</color>\n"
            "  Acid: <color_c_yellow>0</color>  Fire: <color_c_yellow>0</color>  Environmental: <color_c_yellow>0</color>\n" );
    }

    SECTION( "omits irrelevant info if it covers nothing" ) {
        iteminfo_test(
            item( "ear_plugs" ), q,
            "--\n"
            "Covers: <color_c_cyan>Nothing</color>.\n" );
    }
}


TEST_CASE( "gun_lists_default_ammo", "[item][iteminfo]" )
{
    iteminfo_query q( { iteminfo_parts::GUN_DEFAULT_AMMO } );
    iteminfo_test(
        item( "compbow" ), q,
        "--\n"
        "Gun is not loaded, so stats below assume the default ammo: <color_c_light_blue>wooden broadhead arrow</color>\n" );
}

TEST_CASE( "gun_damage_multiplier_not_integer", "[item][iteminfo]" )
{
    iteminfo_query q( { iteminfo_parts::GUN_DAMAGE, iteminfo_parts::GUN_DAMAGE_AMMOPROP,
                        iteminfo_parts::GUN_DAMAGE_TOTAL
                      } );
    iteminfo_test(
        item( "compbow" ), q,
        "--\n"
        "Damage: <color_c_yellow>18</color>*<color_c_yellow>1.25</color> = <color_c_yellow>22</color>\n" );
}

TEST_CASE( "nutrients_in_regular_item", "[item][iteminfo]" )
{
    iteminfo_query q( { iteminfo_parts::FOOD_NUTRITION, iteminfo_parts::FOOD_VITAMINS,
                        iteminfo_parts::FOOD_QUENCH
                      } );
    item i( "icecream" );
    iteminfo_test(
        i, q,
        "--\n"
        "<color_c_white>Calories (kcal)</color>: <color_c_yellow>325</color>  "
        "Quench: <color_c_yellow>0</color>\n"
        "Vitamins (RDA): Calcium (9%), Vitamin A (9%), and Vitamin B12 (11%)\n" );
}

TEST_CASE( "nutrient_ranges_for_recipe_exemplars", "[item][iteminfo]" )
{
    iteminfo_query q( { iteminfo_parts::FOOD_NUTRITION, iteminfo_parts::FOOD_VITAMINS,
                        iteminfo_parts::FOOD_QUENCH
                      } );
    item i( "icecream" );
    i.set_var( "recipe_exemplar", "icecream" );
    iteminfo_test(
        i, q,
        "--\n"
        "Nutrition will <color_cyan>vary with chosen ingredients</color>.\n"
        "<color_c_white>Calories (kcal)</color>: <color_c_yellow>317</color>-"
        "<color_c_yellow>469</color>  Quench: <color_c_yellow>0</color>\n"
        "Vitamins (RDA): Calcium (7-28%), Iron (0-83%), "
        "Vitamin A (3-11%), Vitamin B12 (2-6%), and Vitamin C (1-85%)\n" );
}

TEST_CASE( "item conductivity", "[item][iteminfo]" )
{
    iteminfo_query q( { iteminfo_parts::DESCRIPTION_CONDUCTIVITY } );

    SECTION( "non-conductive items" ) {
        iteminfo_test(
            item( "2x4" ), q,
            "--\n"
            "* This item <color_c_green>does not conduct</color> electricity.\n" );
        iteminfo_test(
            item( "fire_ax" ), q,
            "--\n"
            "* This item <color_c_green>does not conduct</color> electricity.\n" );
    }

    SECTION( "conductive items" ) {
        iteminfo_test(
            item( "pipe" ), q,
            "--\n"
            "* This item <color_c_red>conducts</color> electricity.\n" );
        iteminfo_test(
            item( "halligan" ), q,
            "--\n"
            "* This item <color_c_red>conducts</color> electricity.\n" );
    }
}

TEST_CASE( "list of item qualities", "[item][iteminfo]" )
{
    iteminfo_query q( { iteminfo_parts::QUALITIES } );

    SECTION( "screwdriver" ) {
        iteminfo_test(
            item( "screwdriver" ), q,
            "--\n"
            "Has level <color_c_cyan>1 screw driving</color> quality.\n" );
    }

    SECTION( "screwdriver_set" ) {
        iteminfo_test(
            item( "screwdriver_set" ), q,
            "--\n"
            "Has level <color_c_cyan>1 screw driving</color> quality.\n"
            "Has level <color_c_cyan>1 fine screw driving</color> quality.\n" );
    }
}

