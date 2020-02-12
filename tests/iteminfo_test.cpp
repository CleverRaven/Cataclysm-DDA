#include <memory>
#include <string>
#include <vector>

#include "avatar.h"
#include "catch/catch.hpp"
#include "game.h"
#include "item.h"
#include "iteminfo_query.h"
#include "recipe_dictionary.h"

static void iteminfo_test( const item &i, const iteminfo_query &q, const std::string &reference )
{
    g->u.empty_traits();
    std::vector<iteminfo> info_v;
    std::string info = i.info( info_v, &q, 1 );
    CHECK( info == reference );
}

TEST_CASE( "armor_info", "[item][iteminfo]" )
{
    // Just a generic typical piece of clothing
    iteminfo_query q( { iteminfo_parts::ARMOR_BODYPARTS, iteminfo_parts::ARMOR_LAYER,
                        iteminfo_parts::ARMOR_COVERAGE, iteminfo_parts::ARMOR_WARMTH,
                        iteminfo_parts::ARMOR_ENCUMBRANCE, iteminfo_parts::ARMOR_PROTECTION
                      } );
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

TEST_CASE( "if_covers_nothing_omit_irreelevant_info", "[item][iteminfo]" )
{
    iteminfo_query q( { iteminfo_parts::ARMOR_BODYPARTS, iteminfo_parts::ARMOR_LAYER,
                        iteminfo_parts::ARMOR_COVERAGE, iteminfo_parts::ARMOR_WARMTH,
                        iteminfo_parts::ARMOR_ENCUMBRANCE, iteminfo_parts::ARMOR_PROTECTION
                      } );
    iteminfo_test(
        item( "ear_plugs" ), q,
        "--\n"
        "Covers: <color_c_cyan>Nothing</color>.\n" );
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

