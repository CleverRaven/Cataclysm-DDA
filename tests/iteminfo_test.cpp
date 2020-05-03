#include <memory>
#include <string>
#include <vector>

#include "avatar.h"
#include "catch/catch.hpp"
#include "game.h"
#include "item.h"
#include "iteminfo_query.h"
#include "itype.h"
#include "player_helpers.h"
#include "options_helpers.h"
#include "recipe.h"
#include "type_id.h"
#include "value_ptr.h"

static void test_info_equals( const item &i, const iteminfo_query &q,
                              const std::string &reference )
{
    g->u.clear_mutations();
    std::vector<iteminfo> info_v;
    std::string info = i.info( info_v, &q, 1 );
    CHECK( info == reference );
}

static void test_info_contains( const item &i, const iteminfo_query &q,
                                const std::string &reference )
{
    g->u.clear_mutations();
    std::vector<iteminfo> info_v;
    std::string info = i.info( info_v, &q, 1 );
    using Catch::Matchers::Contains;
    REQUIRE_THAT( info, Contains( reference ) );
}

/*
 * Wrap the iteminfo_query() constructor to avoid MacOS clang compiler errors like this:
 *
 * iteminfo_test.cpp:NN: error: call to constructor of 'iteminfo_query' is ambiguous
 *     iteminfo_query q( { iteminfo_parts::BASE_RIGIDITY, iteminfo_parts::ARMOR_ENCUMBRANCE } );
 *                    ^  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  ../src/iteminfo_query.h:245:9: note: candidate constructor
 *     iteminfo_query( const std::string &bits );
 * ../src/iteminfo_query.h:246:9: note: candidate constructor
 *     iteminfo_query( const std::vector<iteminfo_parts> &setBits );
 *
 * Using this wrapper should force it to use the vector constructor.
 */
static iteminfo_query q_vec( const std::vector<iteminfo_parts> &part_flags )
{
    return iteminfo_query( part_flags );
}

TEST_CASE( "item description and physical attributes", "[item][iteminfo][primary][!mayfail]" )
{
    iteminfo_query q = q_vec( { iteminfo_parts::BASE_CATEGORY, iteminfo_parts::BASE_MATERIAL,
                                iteminfo_parts::BASE_VOLUME, iteminfo_parts::BASE_WEIGHT,
                                iteminfo_parts::DESCRIPTION
                              } );

    override_option opt( "USE_METRIC_WEIGHTS", "lbs" );
    SECTION( "volume, weight, category, material, description" ) {
        test_info_equals(
            item( "test_jug_plastic" ), q,
            "Material: <color_c_light_blue>Plastic</color>\n"
            "Volume: <color_c_yellow>3.750</color> L  Weight: <color_c_yellow>0.42</color> lbs\n"
            "Category: <color_c_magenta>CONTAINERS</color>\n"
            "--\n"
            "A standard plastic jug used for milk and household cleaning chemicals.\n" );
    }
}

TEST_CASE( "item owner, price, and barter value", "[item][iteminfo][price]" )
{
    iteminfo_query q = q_vec( std::vector<iteminfo_parts>( { iteminfo_parts::BASE_PRICE, iteminfo_parts::BASE_BARTER } ) );

    SECTION( "owner and price" ) {
        item my_rock( "test_rock" );
        my_rock.set_owner( g->u );
        test_info_equals(
            my_rock, q,
            "Owner: Your Followers\n"
            "--\n"
            "Price: $<color_c_yellow>0.00</color>" );
    }

    SECTION( "owner, price and barter value" ) {
        item my_pipe( "test_pipe" );
        my_pipe.set_owner( g->u );
        test_info_equals(
            my_pipe, q,
            "Owner: Your Followers\n"
            "--\n"
            "Price: $<color_c_yellow>75.00</color>  Barter value: $<color_c_yellow>3.00</color>\n" );
    }

    SECTION( "zero price item with no owner" ) {
        test_info_equals(
            item( "test_rock" ), q,
            "--\n"
            "Price: $<color_c_yellow>0.00</color>" );
    }
}

TEST_CASE( "item rigidity", "[item][iteminfo][rigidity][!mayfail]" )
{
    iteminfo_query q = q_vec( { iteminfo_parts::BASE_RIGIDITY, iteminfo_parts::ARMOR_ENCUMBRANCE } );

    SECTION( "non-rigid items indicate their flexible volume/encumbrance" ) {
        test_info_equals(
            item( "test_waterskin" ), q,
            "--\n"
            "<color_c_white>Encumbrance</color>: <color_c_yellow>0</color>"
            "  Encumbrance when full: <color_c_yellow>6</color>\n"
            "--\n"
            "* This item is <color_c_cyan>not rigid</color>."
            "  Its volume and encumbrance increase with contents.\n" );

        test_info_equals(
            item( "test_backpack" ), q,
            "--\n"
            "<color_c_white>Encumbrance</color>: <color_c_yellow>2</color>"
            "  Encumbrance when full: <color_c_yellow>15</color>\n"
            "--\n"
            "* This item is <color_c_cyan>not rigid</color>."
            "  Its volume and encumbrance increase with contents.\n" );
    }

    SECTION( "rigid items do not indicate they are rigid, since almost all items are" ) {
        test_info_equals(
            item( "test_briefcase" ), q,
            "--\n"
            "<color_c_white>Encumbrance</color>: <color_c_yellow>30</color>\n" );

        test_info_equals( item( "test_jug_plastic" ), q, "" );
        test_info_equals( item( "test_pipe" ), q, "" );
        test_info_equals( item( "test_pine_nuts" ), q, "" );
    }
}

TEST_CASE( "weapon attack ratings and moves", "[item][iteminfo][weapon]" )
{
    // new DPS calculations depend on the avatar's stats, so make sure they're consistent
    clear_avatar();
    REQUIRE( g->u.get_str() == 8 );
    REQUIRE( g->u.get_dex() == 8 );
    iteminfo_query q = q_vec( { iteminfo_parts::BASE_DAMAGE, iteminfo_parts::BASE_TOHIT,
                                iteminfo_parts::BASE_MOVES
                              } );

    SECTION( "bash damage" ) {
        test_info_equals(
            item( "test_rock" ), q,
            "--\n"
            "<color_c_white>Melee damage</color>: Bash: <color_c_yellow>7</color>"
            "  To-hit bonus: <color_c_yellow>-2</color>\n"
            "Moves per attack: <color_c_yellow>79</color>\n"
            "Typical damage per second:\n"
            "Best: <color_c_yellow>4.92</color>"
            "  Vs. Agile: <color_c_yellow>2.05</color>"
            "  Vs. Armored: <color_c_yellow>0.15</color>\n" );
    }

    SECTION( "bash and cut damage" ) {
        test_info_equals(
            item( "test_halligan" ), q,
            "--\n"
            "<color_c_white>Melee damage</color>: Bash: <color_c_yellow>20</color>"
            "  Cut: <color_c_yellow>5</color>"
            "  To-hit bonus: <color_c_yellow>+2</color>\n"
            "Moves per attack: <color_c_yellow>145</color>\n"
            "Typical damage per second:\n"
            "Best: <color_c_yellow>9.38</color>"
            "  Vs. Agile: <color_c_yellow>5.74</color>"
            "  Vs. Armored: <color_c_yellow>2.84</color>\n" );
    }

    SECTION( "bash and pierce damage" ) {
        test_info_equals(
            item( "pointy_stick" ), q,
            "--\n"
            "<color_c_white>Melee damage</color>: Bash: <color_c_yellow>5</color>"
            "  Pierce: <color_c_yellow>9</color>"
            "  To-hit bonus: <color_c_yellow>-1</color>\n"
            "Moves per attack: <color_c_yellow>100</color>\n"
            "Typical damage per second:\n"
            "Best: <color_c_yellow>6.87</color>"
            "  Vs. Agile: <color_c_yellow>3.20</color>"
            "  Vs. Armored: <color_c_yellow>0.12</color>\n" );
    }

    SECTION( "melee and ranged damaged" ) {
        test_info_equals(
            item( "arrow_wood" ), q,
            "--\n"
            "<color_c_white>Melee damage</color>: Bash: <color_c_yellow>2</color>"
            "  Cut: <color_c_yellow>1</color>"
            "  To-hit bonus: <color_c_yellow>+0</color>\n"
            "Moves per attack: <color_c_yellow>65</color>\n"
            "Typical damage per second:\n"
            "Best: <color_c_yellow>4.90</color>"
            "  Vs. Agile: <color_c_yellow>2.46</color>"
            "  Vs. Armored: <color_c_yellow>0.00</color>\n" );
    }

    SECTION( "no damage" ) {
        test_info_equals( item( "test_rag" ), q, "" );
    }
}

TEST_CASE( "techniques when wielded", "[item][iteminfo][weapon]" )
{
    iteminfo_query q = q_vec( { iteminfo_parts::DESCRIPTION_TECHNIQUES } );

    test_info_equals(
        item( "test_halligan" ), q,
        "--\n"
        "<color_c_white>Techniques when wielded</color>:"
        " <color_c_light_blue>Brutal Strike</color>: <color_c_cyan>Stun 1 turn, knockback 1 tile, crit only</color>,"
        " <color_c_light_blue>Sweep Attack</color>: <color_c_cyan>Down 2 turns</color>, and"
        " <color_c_light_blue>Block</color>: <color_c_cyan>Medium blocking ability</color>\n" );
}

TEST_CASE( "armor coverage and protection values", "[item][iteminfo][armor]" )
{
    iteminfo_query q = q_vec( { iteminfo_parts::ARMOR_BODYPARTS, iteminfo_parts::ARMOR_LAYER,
                                iteminfo_parts::ARMOR_COVERAGE, iteminfo_parts::ARMOR_WARMTH,
                                iteminfo_parts::ARMOR_ENCUMBRANCE, iteminfo_parts::ARMOR_PROTECTION
                              } );

    SECTION( "shows coverage, encumbrance, and protection for armor with coverage" ) {
        test_info_equals(
            item( "test_longshirt" ), q,
            "--\n"
            // NOLINTNEXTLINE(cata-text-style)
            "<color_c_white>Covers</color>: The <color_c_cyan>torso</color>. The <color_c_cyan>arms</color>. \n"
            // NOLINTNEXTLINE(cata-text-style)
            "Layer: <color_c_light_blue>Normal</color>. \n"
            "Coverage: <color_c_yellow>90</color>%  Warmth: <color_c_yellow>5</color>\n"
            "--\n"
            "<color_c_white>Encumbrance</color>: <color_c_yellow>3</color> <color_c_red>(poor fit)</color>\n"
            "<color_c_white>Protection</color>: Bash: <color_c_yellow>1</color>  Cut: <color_c_yellow>1</color>  Ballistic: <color_c_yellow>1</color>\n"
            "  Acid: <color_c_yellow>0</color>  Fire: <color_c_yellow>0</color>  Environmental: <color_c_yellow>0</color>\n" );
    }

    SECTION( "omits irrelevant info if it covers nothing" ) {
        test_info_equals(
            item( "test_ear_plugs" ), q,
            "--\n"
            "<color_c_white>Covers</color>: <color_c_cyan>Nothing</color>.\n" );
    }
}

TEST_CASE( "ranged weapon attributes", "[item][iteminfo][weapon][ranged][gun]" )
{

    SECTION( "skill used" ) {
        iteminfo_query q = q_vec( { iteminfo_parts::GUN_USEDSKILL } );
        test_info_equals(
            item( "test_compbow" ), q,
            "--\n"
            "Skill used: <color_c_cyan>archery</color>\n" );
    }

    SECTION( "ammo capacity of weapon" ) {
        iteminfo_query q = q_vec( { iteminfo_parts::GUN_CAPACITY } );
        test_info_equals(
            item( "test_compbow" ), q,
            "--\n"
            "Capacity: <color_c_yellow>1</color> round of arrows\n" );
    }

    SECTION( "default ammo when weapon is unloaded" ) {
        iteminfo_query q = q_vec( { iteminfo_parts::GUN_DEFAULT_AMMO } );
        test_info_equals(
            item( "test_compbow" ), q,
            "--\n"
            "Weapon is <color_c_red>not loaded</color>, so stats below assume the default ammo:"
            " <color_c_light_blue>wooden broadhead arrow</color>\n" );
    }

    SECTION( "weapon damage including floating-point multiplier" ) {
        iteminfo_query q = q_vec( { iteminfo_parts::GUN_DAMAGE, iteminfo_parts::GUN_DAMAGE_AMMOPROP,
                                    iteminfo_parts::GUN_DAMAGE_TOTAL, iteminfo_parts::GUN_ARMORPIERCE,
                                    iteminfo_parts::AMMO_DAMAGE_CRIT_MULTIPLIER
                                  } );
        test_info_equals(
            item( "test_compbow" ), q,
            "--\n"
            "<color_c_white>Ranged damage</color>:"
            " <color_c_yellow>18</color>*<color_c_yellow>1.50</color> = <color_c_yellow>27</color>\n"
            "Critical multiplier: <color_c_yellow>10</color>\n"
            "Armor-pierce: <color_c_yellow>0</color>\n" );
    }

    SECTION( "time to reload weapon" ) {
        iteminfo_query q = q_vec( { iteminfo_parts::GUN_RELOAD_TIME } );
        test_info_equals(
            item( "test_compbow" ), q,
            "--\n"
            "Reload time: <color_c_yellow>110</color> moves \n" ); // NOLINT(cata-text-style)
    }

    SECTION( "weapon firing modes" ) {
        iteminfo_query q = q_vec( { iteminfo_parts::GUN_FIRE_MODES } );
        test_info_equals(
            item( "test_compbow" ), q,
            "--\n"
            "<color_c_white>Fire modes</color>: manual (1)\n" );
    }

    SECTION( "weapon mods" ) {
        iteminfo_query q = q_vec( { iteminfo_parts::DESCRIPTION_GUN_MODS } );
        test_info_equals(
            item( "test_compbow" ), q,
            "--\n"
            "<color_c_white>Mods</color>: <color_c_white>0/2</color> accessories;"
            " <color_c_white>0/1</color> dampening; <color_c_white>0/1</color> sights;"
            " <color_c_white>0/1</color> stabilizer; <color_c_white>0/1</color> underbarrel.\n" );
    }

    SECTION( "weapon dispersion" ) {
        iteminfo_query q = q_vec( { iteminfo_parts::GUN_DISPERSION } );
        test_info_equals(
            item( "test_compbow" ), q,
            "--\n"
            "Dispersion: <color_c_yellow>850</color>\n" );
    }
}

TEST_CASE( "ammunition", "[item][iteminfo][ammo]" )
{
    iteminfo_query q = q_vec( { iteminfo_parts::AMMO_REMAINING_OR_TYPES, iteminfo_parts::AMMO_DAMAGE_VALUE,
                                iteminfo_parts::AMMO_DAMAGE_PROPORTIONAL, iteminfo_parts::AMMO_DAMAGE_AP,
                                iteminfo_parts::AMMO_DAMAGE_RANGE, iteminfo_parts::AMMO_DAMAGE_DISPERSION,
                                iteminfo_parts::AMMO_DAMAGE_RECOIL, iteminfo_parts::AMMO_DAMAGE_CRIT_MULTIPLIER
                              } );

    SECTION( "simple item with ammo damage" ) {
        test_info_equals(
            item( "test_rock" ), q,
            "--\n"
            "<color_c_white>Ammunition type</color>: rocks\n"
            "Damage: <color_c_yellow>7</color>  Armor-pierce: <color_c_yellow>0</color>\n"
            "Range: <color_c_yellow>10</color>  Dispersion: <color_c_yellow>14</color>\n"
            "Recoil: <color_c_yellow>0</color>  Critical multiplier: <color_c_yellow>2</color>\n" );
    }
}

TEST_CASE( "nutrients in food", "[item][iteminfo][food][!mayfail]" )
{
    iteminfo_query q = q_vec( { iteminfo_parts::FOOD_NUTRITION, iteminfo_parts::FOOD_VITAMINS,
                                iteminfo_parts::FOOD_QUENCH
                              } );
    SECTION( "fixed nutrient values in regular item" ) {
        item i( "icecream" );
        test_info_equals(
            i, q,
            "--\n"
            "<color_c_white>Calories (kcal)</color>: <color_c_yellow>325</color>  "
            "Quench: <color_c_yellow>0</color>\n"
            "Vitamins (RDA): Calcium (9%), Vitamin A (9%), and Vitamin B12 (11%)\n" );
    }

    SECTION( "nutrient ranges for recipe exemplars", "[item][iteminfo]" ) {
        item i( "icecream" );
        i.set_var( "recipe_exemplar", "icecream" );
        test_info_equals(
            i, q,
            "--\n"
            "Nutrition will <color_cyan>vary with chosen ingredients</color>.\n"
            "<color_c_white>Calories (kcal)</color>: <color_c_yellow>317</color>-"
            "<color_c_yellow>469</color>  Quench: <color_c_yellow>0</color>\n"
            "Vitamins (RDA): Calcium (7-28%), Iron (0-83%), "
            "Vitamin A (3-11%), Vitamin B12 (2-6%), and Vitamin C (1-85%)\n" );
    }
}

TEST_CASE( "food freshness and lifetime", "[item][iteminfo][food]" )
{
    iteminfo_query q = q_vec( { iteminfo_parts::FOOD_ROT } );

    // Ensure test character has no skill estimating spoilage
    g->u.empty_skills();
    REQUIRE_FALSE( g->u.can_estimate_rot() );

    SECTION( "food is fresh" ) {
        test_info_equals(
            item( "test_pine_nuts" ), q,
            "--\n"
            "* This food is <color_c_yellow>perishable</color>, and at room temperature has"
            " an estimated nominal shelf life of <color_c_cyan>6 weeks</color>.\n"
            "* This food looks as <color_c_green>fresh</color> as it can be.\n" );
    }

    SECTION( "food is old" ) {
        item nuts( "test_pine_nuts" );
        nuts.mod_rot( nuts.type->comestible->spoils );
        test_info_equals(
            nuts, q,
            "--\n"
            "* This food is <color_c_yellow>perishable</color>, and at room temperature has"
            " an estimated nominal shelf life of <color_c_cyan>6 weeks</color>.\n"
            "* This food looks <color_c_red>old</color>.  It's on the brink of becoming inedible.\n" );
    }
}

TEST_CASE( "item conductivity", "[item][iteminfo][conductivity]" )
{
    iteminfo_query q = q_vec( { iteminfo_parts::DESCRIPTION_CONDUCTIVITY } );

    SECTION( "non-conductive items" ) {
        test_info_equals(
            item( "test_2x4" ), q,
            "--\n"
            "* This item <color_c_green>does not conduct</color> electricity.\n" );
        test_info_equals(
            item( "test_fire_ax" ), q,
            "--\n"
            "* This item <color_c_green>does not conduct</color> electricity.\n" );
    }

    SECTION( "conductive items" ) {
        test_info_equals(
            item( "test_pipe" ), q,
            "--\n"
            "* This item <color_c_red>conducts</color> electricity.\n" );
        test_info_equals(
            item( "test_halligan" ), q,
            "--\n"
            "* This item <color_c_red>conducts</color> electricity.\n" );
    }
}

TEST_CASE( "list of item qualities", "[item][iteminfo][quality]" )
{
    iteminfo_query q = q_vec( { iteminfo_parts::QUALITIES } );

    SECTION( "Halligan bar" ) {
        test_info_equals(
            item( "test_halligan" ), q,
            "--\n"
            "Has level <color_c_cyan>1 digging</color> quality.\n"
            "Has level <color_c_cyan>2 hammering</color> quality.\n"
            "Has level <color_c_cyan>4 prying</color> quality.\n" );
    }

    SECTION( "bottle jack" ) {
        override_option opt( "USE_METRIC_WEIGHTS", "lbs" );
        test_info_equals(
            item( "test_jack_small" ), q,
            "--\n"
            "Has level <color_c_cyan>4 jacking</color> quality and is rated at <color_c_cyan>4409</color> lbs\n" );
    }

    SECTION( "sonic screwdriver" ) {
        test_info_equals(
            item( "test_sonic_screwdriver" ), q,
            "--\n"
            "Has level <color_c_cyan>2 prying</color> quality.\n"
            "Has level <color_c_cyan>2 screw driving</color> quality.\n"
            "Has level <color_c_cyan>1 fine screw driving</color> quality.\n"
            "Has level <color_c_cyan>1 bolt turning</color> quality.\n" );
    }
}

TEST_CASE( "repairable and with what tools", "[item][iteminfo][repair]" )
{
    iteminfo_query q = q_vec( { iteminfo_parts::DESCRIPTION_REPAIREDWITH } );

    test_info_contains(
        item( "test_halligan" ), q,
        "<color_c_white>Repair</color> using extended toolset, arc welder, or makeshift arc welder.\n" );

    test_info_contains(
        item( "test_hazmat_suit" ), q,
        "<color_c_white>Repair</color> using soldering iron, TEST soldering iron, or extended toolset.\n" );

    test_info_contains(
        item( "test_rock" ), q, "* This item is <color_c_red>not repairable</color>.\n" );

    test_info_contains(
        item( "test_socks" ), q, "* This item can be <color_c_green>reinforced</color>.\n" );
}

TEST_CASE( "disassembly time and yield", "[item][iteminfo][disassembly]" )
{
    iteminfo_query q = q_vec( { iteminfo_parts::DESCRIPTION_COMPONENTS_DISASSEMBLE } );

    test_info_equals(
        item( "test_soldering_iron" ), q,
        "--\n"
        "<color_c_white>Disassembly</color> takes about 20 minutes and might yield:"
        " 2 electronic scraps, copper (1), scrap metal (1), and copper wire (5).\n" );

    test_info_equals(
        item( "test_sheet_metal" ), q,
        "--\n"
        "<color_c_white>Disassembly</color> takes about 2 minutes and might yield: TEST small metal sheet (24).\n" );
}

TEST_CASE( "item description flags", "[item][iteminfo]" )
{
    iteminfo_query q = q_vec( { iteminfo_parts::DESCRIPTION_FLAGS } );

    test_info_equals(
        item( "test_halligan" ), q,
        "--\n"
        "* This item can be clipped on to a <color_c_cyan>belt loop</color> of the appropriate size.\n"
        "* As a weapon, this item is <color_c_green>well-made</color> and will"
        " <color_c_cyan>withstand the punishment of combat</color>.\n" );

    test_info_equals(
        item( "test_hazmat_suit" ), q,
        "--\n"
        "* This gear <color_c_green>completely protects</color> you from"
        " <color_c_cyan>electric discharges</color>.\n"
        "* This gear <color_c_green>completely protects</color> you from"
        " <color_c_cyan>any gas</color>.\n"
        "* This gear is generally <color_c_cyan>worn over</color> clothing.\n"
        "* This clothing <color_c_green>completely protects</color> you from"
        " <color_c_cyan>radiation</color>.\n"
        "* This clothing is designed to keep you <color_c_cyan>dry</color> in the rain.\n"
        "* This clothing <color_c_cyan>won't let water through</color>."
        "  Unless you jump in the river or something like that.\n" );
}

TEST_CASE( "show available recipes with item as an ingredient", "[item][iteminfo][recipes]" )
{
    iteminfo_query q = q_vec( { iteminfo_parts::DESCRIPTION_APPLICABLE_RECIPES } );
    const recipe *purtab = &recipe_id( "pur_tablets" ).obj();
    g->u.clear_mutations();

    GIVEN( "character has a potassium iodide tablet and no skill" ) {
        g->u.worn.push_back( item( "backpack" ) );
        item &iodine = g->u.i_add( item( "iodine" ) );
        g->u.empty_skills();

        THEN( "nothing is craftable from it" ) {
            test_info_equals(
                iodine, q,
                "--\nYou know of nothing you could craft with it.\n" );
        }

        WHEN( "they acquire the needed skills" ) {
            g->u.set_skill_level( purtab->skill_used, purtab->difficulty );
            REQUIRE( g->u.get_skill_level( purtab->skill_used ) == purtab->difficulty );

            THEN( "still nothing is craftable from it" ) {
                test_info_equals(
                    iodine, q,
                    "--\nYou know of nothing you could craft with it.\n" );
            }

            WHEN( "they have no book, but have the recipe memorized" ) {
                g->u.learn_recipe( purtab );
                REQUIRE( g->u.knows_recipe( purtab ) );

                THEN( "they can use potassium iodide tablets to craft it" ) {
                    test_info_equals(
                        iodine, q,
                        "--\n"
                        "You could use it to craft: "
                        "<color_c_dark_gray>water purification tablet</color>\n" );
                }
            }

            WHEN( "they have the recipe in a book, but not memorized" ) {
                g->u.i_add( item( "textbook_chemistry" ) );

                THEN( "they can use potassium iodide tablets to craft it" ) {
                    test_info_equals(
                        iodine, q,
                        "--\n"
                        "You could use it to craft: "
                        "<color_c_dark_gray>water purification tablet</color>\n" );
                }
            }
        }
    }
}
