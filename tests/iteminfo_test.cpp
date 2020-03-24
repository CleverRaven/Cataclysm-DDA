#include <memory>
#include <string>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "catch/catch.hpp"
#include "game.h"
#include "item.h"
#include "iteminfo_query.h"
#include "itype.h"
#include "options_helpers.h"
#include "recipe_dictionary.h"

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
                        iteminfo_parts::BASE_WEIGHT, iteminfo_parts::BASE_MATERIAL
                      } );
    override_option opt( "USE_METRIC_WEIGHTS", "lbs" );
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

TEST_CASE( "weapon attack ratings and moves", "[item][iteminfo]" )
{
    iteminfo_query q( { iteminfo_parts::BASE_DAMAGE, iteminfo_parts::BASE_TOHIT,
                        iteminfo_parts::BASE_MOVES
                      } );

    iteminfo_test(
        item( "halligan" ), q,
        "Bash: <color_c_yellow>20</color>"
        "  Cut: <color_c_yellow>5</color>"
        "  To-hit bonus: <color_c_yellow>+2</color>\n"
        "Moves per attack: <color_c_yellow>145</color>\n" );

}

TEST_CASE( "techniques when wielded", "[item][iteminfo]" )
{
    iteminfo_query q( { iteminfo_parts::DESCRIPTION_TECHNIQUES } );

    iteminfo_test(
        item( "halligan" ), q,
        "--\n"
        "<color_c_white>Techniques when wielded</color>:"
        " <color_c_light_blue>Brutal Strike:</color> <color_c_cyan>Stun 1 turn, knockback 1 tile, crit only</color>,"
        " <color_c_light_blue>Sweep Attack:</color> <color_c_cyan>Down 2 turns</color>, and"
        " <color_c_light_blue>Block:</color> <color_c_cyan>Medium blocking ability</color>\n" );

}

TEST_CASE( "armor coverage and protection values", "[item][iteminfo]" )
{
    iteminfo_query q( { iteminfo_parts::ARMOR_BODYPARTS, iteminfo_parts::ARMOR_LAYER,
                        iteminfo_parts::ARMOR_COVERAGE, iteminfo_parts::ARMOR_WARMTH,
                        iteminfo_parts::ARMOR_ENCUMBRANCE, iteminfo_parts::ARMOR_PROTECTION
                      } );

    SECTION( "shows coverage, encumbrance, and protection for armor with coverage" ) {
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

TEST_CASE( "ranged weapon attributes", "[item][iteminfo]" )
{
    SECTION( "ammo capacity" ) {
        iteminfo_query q( { iteminfo_parts::GUN_CAPACITY } );
        iteminfo_test(
            item( "compbow" ), q,
            "--\n"
            "<color_c_white>Capacity:</color> <color_c_yellow>1</color> round of arrows\n" );
    }

    SECTION( "default ammo when unloaded" ) {
        iteminfo_query q( { iteminfo_parts::GUN_DEFAULT_AMMO } );
        iteminfo_test(
            item( "compbow" ), q,
            "--\n"
            "Gun is not loaded, so stats below assume the default ammo:"
            " <color_c_light_blue>wooden broadhead arrow</color>\n" );
    }

    SECTION( "damage including floating-point multiplier" ) {
        iteminfo_query q( { iteminfo_parts::GUN_DAMAGE, iteminfo_parts::GUN_DAMAGE_AMMOPROP,
                            iteminfo_parts::GUN_DAMAGE_TOTAL
                          } );
        iteminfo_test(
            item( "compbow" ), q,
            "--\n"
            "Damage: <color_c_yellow>50</color>*<color_c_yellow>1.50</color> = <color_c_yellow>75</color>\n" );
    }

    SECTION( "time to reload" ) {
        iteminfo_query q( { iteminfo_parts::GUN_RELOAD_TIME } );
        iteminfo_test(
            item( "compbow" ), q,
            "--\n"
            "Reload time: <color_c_yellow>100</color> moves \n" ); // NOLINT(cata-text-style)
    }

    SECTION( "firing modes" ) {
        iteminfo_query q( { iteminfo_parts::GUN_FIRE_MODES } );
        iteminfo_test(
            item( "compbow" ), q,
            "--\n"
            "<color_c_white>Fire modes:</color> manual (1)\n" );
    }

    SECTION( "weapon mods" ) {
        iteminfo_query q( { iteminfo_parts::DESCRIPTION_GUN_MODS } );
        iteminfo_test(
            item( "compbow" ), q,
            "--\n"
            "<color_c_white>Mods:</color> <color_c_white>0/2</color> accessories;"
            " <color_c_white>0/1</color> dampening; <color_c_white>0/1</color> sights;"
            " <color_c_white>0/1</color> stabilizer; <color_c_white>0/1</color> underbarrel.\n" );
    }

}

TEST_CASE( "nutrients in food", "[item][iteminfo]" )
{
    iteminfo_query q( { iteminfo_parts::FOOD_NUTRITION, iteminfo_parts::FOOD_VITAMINS,
                        iteminfo_parts::FOOD_QUENCH
                      } );
    SECTION( "fixed nutrient values in regular item" ) {
        item i( "icecream" );
        iteminfo_test(
            i, q,
            "--\n"
            "<color_c_white>Calories (kcal)</color>: <color_c_yellow>325</color>  "
            "Quench: <color_c_yellow>0</color>\n"
            "Vitamins (RDA): Calcium (9%), Vitamin A (9%), and Vitamin B12 (11%)\n" );
    }

    SECTION( "nutrient ranges for recipe exemplars", "[item][iteminfo]" ) {
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
}

TEST_CASE( "food freshness and lifetime", "[item][iteminfo]" )
{
    iteminfo_query q( { iteminfo_parts::FOOD_ROT } );

    // Ensure test character has no skill estimating spoilage
    g->u.empty_skills();
    REQUIRE_FALSE( g->u.can_estimate_rot() );

    SECTION( "food is fresh" ) {
        iteminfo_test(
            item( "pine_nuts" ), q,
            "--\n"
            "* This food is <color_c_yellow>perishable</color>, and at room temperature has"
            " an estimated nominal shelf life of <color_c_cyan>6 weeks</color>.\n"
            "* This food looks as <color_c_green>fresh</color> as it can be.\n" );
    }

    SECTION( "food is old" ) {
        item nuts( "pine_nuts" );
        nuts.mod_rot( nuts.type->comestible->spoils );
        iteminfo_test(
            nuts, q,
            "--\n"
            "* This food is <color_c_yellow>perishable</color>, and at room temperature has"
            " an estimated nominal shelf life of <color_c_cyan>6 weeks</color>.\n"
            "* This food looks <color_c_red>old</color>.  It's on the brink of becoming inedible.\n" );
    }

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

    SECTION( "screwdriver set" ) {
        iteminfo_test(
            item( "screwdriver_set" ), q,
            "--\n"
            "Has level <color_c_cyan>1 screw driving</color> quality.\n"
            "Has level <color_c_cyan>1 fine screw driving</color> quality.\n" );
    }

    SECTION( "Halligan bar" ) {
        iteminfo_test(
            item( "halligan" ), q,
            "--\n"
            "Has level <color_c_cyan>1 digging</color> quality.\n"
            "Has level <color_c_cyan>2 hammering</color> quality.\n"
            "Has level <color_c_cyan>4 prying</color> quality.\n" );
    }
}

TEST_CASE( "repairable and with what tools", "[item][iteminfo]" )
{
    iteminfo_query q( { iteminfo_parts::DESCRIPTION_REPAIREDWITH } );

    iteminfo_test(
        item( "halligan" ), q,
        "--\n"
        "<color_c_white>Repaired with</color>: extended toolset, arc welder, or makeshift arc welder\n" );

    iteminfo_test(
        item( "hazmat_suit" ), q,
        "--\n"
        "<color_c_white>Repaired with</color>: soldering iron or extended toolset\n" );

    iteminfo_test(
        item( "rock" ), q,
        "--\n"
        "* This item is <color_c_red>not repairable</color>.\n" );
}

TEST_CASE( "item description flags", "[item][iteminfo]" )
{
    iteminfo_query q( { iteminfo_parts::DESCRIPTION_FLAGS } );

    iteminfo_test(
        item( "halligan" ), q,
        "--\n"
        "* This item can be clipped on to a <color_c_cyan>belt loop</color> of the appropriate size.\n"
        "* As a weapon, this item is <color_c_green>well-made</color> and will"
        " <color_c_cyan>withstand the punishment of combat</color>.\n" );

    iteminfo_test(
        item( "hazmat_suit" ), q,
        "--\n"
        "* This gear <color_c_green>completely protects</color> you from"
        " <color_c_cyan>electric discharges</color>.\n"
        "* This gear <color_c_green>completely protects</color> you from"
        " <color_c_cyan>any gas</color>.\n"
        "* This gear is generally <color_c_cyan>worn over</color> clothing.\n"
        "* This clothing <color_c_green>completely protects</color> you from"
        " <color_c_cyan>radiation</color>.\n"
        "* This piece of clothing is designed to keep you <color_c_cyan>dry</color> in the rain.\n"
        "* This clothing <color_c_cyan>won't let water through</color>."
        "  Unless you jump in the river or something like that.\n" );
}

TEST_CASE( "show available recipes with item as an ingredient", "[item][iteminfo][recipes]" )
{
    iteminfo_query q( { iteminfo_parts::DESCRIPTION_APPLICABLE_RECIPES } );
    const recipe *purtab = &recipe_id( "pur_tablets" ).obj();
    g->u.empty_traits();

    GIVEN( "character has a potassium iodide tablet and no skill" ) {
        item &iodine = g->u.i_add( item( "iodine" ) );
        g->u.empty_skills();

        THEN( "nothing is craftable from it" ) {
            iteminfo_test(
                iodine, q,
                "--\nYou know of nothing you could craft with it.\n" );
        }

        WHEN( "they acquire the needed skills" ) {
            g->u.set_skill_level( purtab->skill_used, purtab->difficulty );
            REQUIRE( g->u.get_skill_level( purtab->skill_used ) == purtab->difficulty );

            THEN( "still nothing is craftable from it" ) {
                iteminfo_test(
                    iodine, q,
                    "--\nYou know of nothing you could craft with it.\n" );
            }

            WHEN( "they have no book, but have the recipe memorized" ) {
                g->u.learn_recipe( purtab );
                REQUIRE( g->u.knows_recipe( purtab ) );

                THEN( "they can use potassium iodide tablets to craft it" ) {
                    iteminfo_test(
                        iodine, q,
                        "--\n"
                        "You could use it to craft: "
                        "<color_c_dark_gray>water purification tablet</color>\n" );
                }
            }

            WHEN( "they have the recipe in a book, but not memorized" ) {
                g->u.i_add( item( "textbook_chemistry" ) );

                THEN( "they can use potassium iodide tablets to craft it" ) {
                    iteminfo_test(
                        iodine, q,
                        "--\n"
                        "You could use it to craft: "
                        "<color_c_dark_gray>water purification tablet</color>\n" );
                }
            }
        }
    }
}
