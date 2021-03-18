#include <iosfwd>
#include <list>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "catch/catch.hpp"
#include "character.h"
#include "flag.h"
#include "item.h"
#include "item_contents.h"
#include "iteminfo_query.h"
#include "itype.h"
#include "options_helpers.h"
#include "output.h"
#include "player_helpers.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"

// ITEM INFO
// =========
//
// When looking at the description of any item in the game, the information you see comes from the
// item::info function, which calls functions like basic_info, armor_info, food_info and many others
// to build a complete string describing the item.
//
// The included info depends on:
//
// - Relevancy to current item (damage for weapons, coverage for armor, vitamins for food, etc.)
// - Including one or more appropriate iteminfo_part::PART_NAME flags in the `info` function call
//
// Color-highlighted text in item info uses a semantic markup in the code (ex. "good", "bad") that
// becomes translated to color codes for output:
//
//   <bold>   <color_c_white>
//   <good>   <color_c_green>
//   <bad>    <color_c_red>
//   <info>   <color_c_cyan>
//   <stat>   <color_c_light_blue>
//   <num>    <color_c_yellow>
//   <header> <color_c_magenta>
//
// Each xxx_info function takes a std::vector<iteminfo> reference, where each line or snippet of
// info will be appended. The main item::info function assembles all these into a string.
//
// The test cases here mostly test item::info directly, using std::vector<iteminfo_parts> flags to
// request only the parts relevant to the current test.
//
// To run all the tests in this file:
//
//      tests/cata_test [iteminfo]
//
// Other tags: [book], [food], [pocket], [quality], [weapon], [volume], [weight], and many others

// Call the info() function on an item with given flags, and return the formatted string.
static std::string item_info_str( const item &it, const std::vector<iteminfo_parts> &part_flags )
{
    // Old captures from test_info_equals/contains, in case needed
    //int encumber = i.type->armor ? i.type->armor->encumber : -1;
    //int max_encumber = i.type->armor ? i.type->armor->max_encumber : -1;
    //CAPTURE( encumber );
    //CAPTURE( max_encumber );
    //CAPTURE( i.typeId() );
    //CAPTURE( i.has_flag( "FIT" ) );
    //CAPTURE( i.has_flag( "VARSIZE" ) );
    //CAPTURE( i.get_clothing_mod_val( clothing_mod_type_encumbrance ) );
    //CAPTURE( i.get_sizing( get_avatar(), true ) );

    std::vector<iteminfo> info_v;
    const iteminfo_query query_v( part_flags );
    it.info( info_v, &query_v, 1 );
    return format_item_info( info_v, {} );
}

// Related JSON fields:
// "volume"
// "weight"
// "longest_side"
//
// Functions:
// item::basic_info
TEST_CASE( "item volume and weight", "[iteminfo][volume][weight]" )
{
    clear_avatar();

    item plank( "test_2x4" );

    // Volume and weight are shown together, though the units may differ
    std::vector<iteminfo_parts> vol_weight = { iteminfo_parts::BASE_VOLUME, iteminfo_parts::BASE_WEIGHT };

    SECTION( "metric volume" ) {
        override_option opt_volume( "VOLUME_UNITS", "l" );
        SECTION( "metric weight" ) {
            override_option opt_weight( "USE_METRIC_WEIGHTS", "kg" );
            CHECK( item_info_str( plank, vol_weight ) ==
                   "Volume: <color_c_yellow>4.40</color> L  Weight: <color_c_yellow>2.20</color> kg\n" );
        }
        SECTION( "imperial weight" ) {
            override_option opt_weight( "USE_METRIC_WEIGHTS", "lbs" );
            CHECK( item_info_str( plank, vol_weight ) ==
                   "Volume: <color_c_yellow>4.40</color> L  Weight: <color_c_yellow>4.85</color> lbs\n" );
        }
    }

    SECTION( "imperial volume" ) {
        override_option opt_volume( "VOLUME_UNITS", "qt" );
        SECTION( "metric weight" ) {
            override_option opt_weight( "USE_METRIC_WEIGHTS", "kg" );
            CHECK( item_info_str( plank, vol_weight ) ==
                   "Volume: <color_c_yellow>4.65</color> qt  Weight: <color_c_yellow>2.20</color> kg\n" );
        }
        SECTION( "imperial weight" ) {
            override_option opt_weight( "USE_METRIC_WEIGHTS", "lbs" );
            CHECK( item_info_str( plank, vol_weight ) ==
                   "Volume: <color_c_yellow>4.65</color> qt  Weight: <color_c_yellow>4.85</color> lbs\n" );
        }
    }

    SECTION( "imperial length" ) {
        override_option opt_dist( "DISTANCE_UNITS", "imperial" );
        CHECK( item_info_str( plank, { iteminfo_parts::BASE_LENGTH } ) ==
               "Length: <color_c_yellow>51</color> in.\n" );
    }

    SECTION( "metric length" ) {
        override_option opt_dist( "DISTANCE_UNITS", "metric" );
        CHECK( item_info_str( plank, { iteminfo_parts::BASE_LENGTH } ) ==
               "Length: <color_c_yellow>130</color> cm\n" );
    }
}

// Related JSON fields:
// "material"
// "category"
// "description"
//
// Functions:
// item::basic_info
TEST_CASE( "item material, category, description", "[iteminfo][material][category][description]" )
{
    clear_avatar();

    // TODO: Test magical focus, in-progress crafts (also part of DESCRIPTION)

    std::vector<iteminfo_parts> material = { iteminfo_parts::BASE_MATERIAL };
    std::vector<iteminfo_parts> category = { iteminfo_parts::BASE_CATEGORY };
    std::vector<iteminfo_parts> description = { iteminfo_parts::DESCRIPTION };

    SECTION( "fire ax" ) {
        item axe( "test_fire_ax" );
        CHECK( item_info_str( axe, material ) ==
               "Material: <color_c_light_blue>Steel</color>, <color_c_light_blue>Wood</color>\n" );

        CHECK( item_info_str( axe, category ) ==
               "Category: <color_c_magenta>TOOLS</color>\n" );

        CHECK( item_info_str( axe, description ) ==
               "--\n"
               "This is a large, two-handed pickhead axe normally used by firefighters."
               "  It makes a powerful melee weapon, but is a bit slow to recover between swings.\n" );
    }

    SECTION( "plank" ) {
        item plank( "test_2x4" );

        CHECK( item_info_str( plank, material ) ==
               "Material: <color_c_light_blue>Wood</color>\n" );

        CHECK( item_info_str( plank, category ) ==
               "Category: <color_c_magenta>SPARE PARTS</color>\n" );

        CHECK( item_info_str( plank, description ) ==
               "--\n"
               "A narrow, thick plank of wood, like a 2 by 4 or similar piece of dimensional lumber."
               "  Makes a decent melee weapon, and can be used for all kinds of construction.\n" );
    }
}

// Related JSON fields:
// none
//
// Functions:
// item::basic_info
TEST_CASE( "item owner", "[iteminfo][owner]" )
{
    clear_avatar();

    SECTION( "item owned by player" ) {
        item my_rock( "test_rock" );
        my_rock.set_owner( get_player_character() );
        REQUIRE_FALSE( my_rock.get_owner().is_null() );
        CHECK( item_info_str( my_rock, { iteminfo_parts::BASE_OWNER } ) == "Owner: Your Followers\n" );
    }

    SECTION( "item with no owner" ) {
        item nobodys_rock( "test_rock" );
        REQUIRE( nobodys_rock.get_owner().is_null() );
        CHECK( item_info_str( nobodys_rock, { iteminfo_parts::BASE_OWNER } ).empty() );
    }
}

// Related JSON fields:
// "min_skills"
// "min_strength"
// "min_intelligence"
// "min_perception"
//
// Functions:
// item::basic_info
TEST_CASE( "item requirements", "[iteminfo][requirements]" )
{
    // TODO:
    // - get_min_str() - type->min_str with special gun/gunmod handling

    // NOTE: BOOK_REQUIREMENTS_INT is separate
    // NOTE: There are presently no in-game items with min_dex, min_int, or min_per

    clear_avatar();

    std::vector<iteminfo_parts> reqs = { iteminfo_parts::BASE_REQUIREMENTS };

    item compbow( "test_compbow" );
    item sonic( "test_sonic_screwdriver" );

    REQUIRE( compbow.type->min_str == 6 );
    CHECK( item_info_str( compbow, reqs ) ==
           "--\n"
           "<color_c_white>Minimum requirements</color>:\n"
           "strength 6\n" );

    REQUIRE( sonic.type->min_int == 9 );
    REQUIRE( sonic.type->min_per == 5 );
    CHECK( item_info_str( sonic, reqs ) ==
           "--\n"
           "<color_c_white>Minimum requirements</color>:\n"
           "intelligence 9, perception 5, electronics 3, and devices 2\n" );
}

// Functions:
// item::basic_info
TEST_CASE( "item contents", "[iteminfo][contents]" )
{
    clear_avatar();

    // TODO: Test "Contains:", if it is still possible (couldn't find any items with it)
    //std::vector<iteminfo_parts> contents = { iteminfo_parts::BASE_CONTENTS };

    // Amount is shown for items having count_by_charges(), and are not food or medication
    // This includes all kinds of ammo and arrows, thread, and some chemicals like sulfur.
    item ammo( "test_9mm_ammo" );
    std::vector<iteminfo_parts> amount = { iteminfo_parts::BASE_AMOUNT };
    CHECK( item_info_str( ammo, amount ) == "--\nAmount: <color_c_yellow>50</color>\n" );
}

// Related JSON fields:
// "comestible_type": "MED":
// "quench"
// "fun"
// "stim"
// "charges" (portions)
// "addiction_potential"
//
// Functions:
// item::med_info
TEST_CASE( "med_info", "[iteminfo][med]" )
{
    clear_avatar();

    // Parts to test
    std::vector<iteminfo_parts> quench = { iteminfo_parts::MED_QUENCH };
    std::vector<iteminfo_parts> joy = { iteminfo_parts::MED_JOY };
    std::vector<iteminfo_parts> stimulation = { iteminfo_parts::MED_STIMULATION };
    std::vector<iteminfo_parts> portions = { iteminfo_parts::MED_PORTIONS };
    std::vector<iteminfo_parts> consume_time = { iteminfo_parts::MED_CONSUME_TIME };
    std::vector<iteminfo_parts> addicting = { iteminfo_parts::DESCRIPTION_MED_ADDICTING };

    // Items with comestible_type "MED"
    SECTION( "item that is medication shows medicinal attributes in med_info" ) {
        item gum( "test_gum" );
        REQUIRE( gum.is_medication() );

        CHECK( item_info_str( gum, quench ) ==
               "--\nQuench: <color_c_yellow>50</color>\n" );

        CHECK( item_info_str( gum, joy ) ==
               "--\nEnjoyability: <color_c_yellow>5</color>\n" );

        CHECK( item_info_str( gum, stimulation ) ==
               "--\nStimulation: <color_c_light_blue>Upper</color>\n" );

        CHECK( item_info_str( gum, portions ) ==
               "--\nPortions: <color_c_yellow>10</color>\n" );

        CHECK( item_info_str( gum, consume_time ) ==
               "--\nConsume time: 5 seconds\n" );

        CHECK( item_info_str( gum, addicting ) ==
               "--\n* Consuming this item is <color_c_red>addicting</color>.\n" );
    }

    SECTION( "item that is not medication does not show med_info" ) {
        item apple( "test_apple" );
        REQUIRE_FALSE( apple.is_medication() );

        CHECK( item_info_str( apple, quench ).empty() );
        CHECK( item_info_str( apple, joy ).empty() );
        CHECK( item_info_str( apple, stimulation ).empty() );
        CHECK( item_info_str( apple, portions ).empty() );
        CHECK( item_info_str( apple, consume_time ).empty() );
        CHECK( item_info_str( apple, addicting ).empty() );
    }
}

// Related JSON fields:
// "price"
// "price_postapoc"
//
// Functions:
// item::final_info
TEST_CASE( "item price and barter value", "[iteminfo][price]" )
{
    clear_avatar();

    // Price and barter value are displayed together on a single line
    std::vector<iteminfo_parts> price_barter = { iteminfo_parts::BASE_PRICE, iteminfo_parts::BASE_BARTER };

    SECTION( "item with different price and barter value" ) {
        item pipe( "test_pipe" );
        REQUIRE( pipe.price( false ) == 7500 );
        REQUIRE( pipe.price( true ) == 300 );

        CHECK( item_info_str( pipe, price_barter ) ==
               "--\n"
               "Price: $<color_c_yellow>75.00</color>  Barter value: $<color_c_yellow>3.00</color>\n" );
    }

    SECTION( "item with same price and barter value shows only price" ) {
        item nuts( "test_pine_nuts" );
        REQUIRE( nuts.price( false ) == 136 );
        REQUIRE( nuts.price( true ) == 136 );

        CHECK( item_info_str( nuts, price_barter ) ==
               "--\n"
               "Price: $<color_c_yellow>1.36</color>" );
    }

    SECTION( "item with no price or barter value" ) {
        item rock( "test_rock" );
        REQUIRE( rock.price( false ) == 0 );
        REQUIRE( rock.price( true ) == 0 );

        CHECK( item_info_str( rock, price_barter ) ==
               "--\n"
               "Price: $<color_c_yellow>0.00</color>" );
    }
}

// Related JSON fields:
// "encumbrance"
// "max_encumbrance"
// "pocket_data"
// "coverage"
// - "rigid"
// - "max_contains_volume"
//
// Functions:
// item::armor_info
TEST_CASE( "item rigidity", "[iteminfo][rigidity]" )
{
    clear_avatar();

    // Rigidity and encumbrance are related; non-rigid items have flexible encumbrance.
    std::vector<iteminfo_parts> rigidity = { iteminfo_parts::BASE_RIGIDITY };
    std::vector<iteminfo_parts> encumbrance = { iteminfo_parts::ARMOR_ENCUMBRANCE };

    SECTION( "items with rigid pockets have a single encumbrance value" ) {
        item briefcase( "test_briefcase" );
        REQUIRE( briefcase.contents.all_pockets_rigid() );
        CHECK( item_info_str( briefcase, encumbrance ) ==
               "--\n"
               "<color_c_white>Encumbrance</color>:\n"
               "L. Arm:  <color_c_yellow>30</color>  "
               "Coverage:  <color_c_yellow>10</color>\n"
               "R. Arm:  <color_c_yellow>30</color>  "
               "Coverage:  <color_c_yellow>10</color>\n"
               "L. Hand:  <color_c_yellow>30</color>  "
               "Coverage:  <color_c_yellow>10</color>\n"
               "R. Hand:  <color_c_yellow>30</color>  "
               "Coverage:  <color_c_yellow>10</color>\n" );
    }

    SECTION( "non-rigid items indicate their flexible volume/encumbrance" ) {
        item waterskin( "test_waterskin" );
        item backpack( "test_backpack" );
        item quiver( "test_quiver" );

        SECTION( "rigidity indicator" ) {
            REQUIRE_FALSE( waterskin.contents.all_pockets_rigid() );
            REQUIRE_FALSE( backpack.contents.all_pockets_rigid() );
            REQUIRE_FALSE( quiver.contents.all_pockets_rigid() );

            CHECK( item_info_str( waterskin, rigidity ) ==
                   "--\n"
                   "* This item is <color_c_cyan>not rigid</color>."
                   "  Its volume and encumbrance increase with contents.\n" );

            CHECK( item_info_str( backpack, rigidity ) ==
                   "--\n"
                   "* This item is <color_c_cyan>not rigid</color>."
                   "  Its volume and encumbrance increase with contents.\n" );

            CHECK( item_info_str( quiver, rigidity ) ==
                   "--\n"
                   "* This item is <color_c_cyan>not rigid</color>."
                   "  Its volume and encumbrance increase with contents.\n" );
        }

        SECTION( "encumbrance when empty and full" ) {
            // test_waterskin does not define "encumbrance" or "max_encumbrance", so base
            // encumbrance is 0, and max_encumbrance is set by the item factory (finalize_post)
            // based on the pocket "max_contains_volume" (1 encumbrance per 250 ml).
            CHECK( item_info_str( waterskin, encumbrance ) ==
                   "--\n"
                   "<color_c_white>Encumbrance</color>:\n"
                   "L. Leg:  <color_c_yellow>0</color>  "
                   "When full:  <color_c_yellow>6</color>  "
                   "Coverage:  <color_c_yellow>5</color>\n"
                   "R. Leg:  <color_c_yellow>0</color>  "
                   "When full:  <color_c_yellow>6</color>  "
                   "Coverage:  <color_c_yellow>5</color>\n" );

            // test_backpack has an explicit "encumbrance" and "max_encumbrance"
            CHECK( item_info_str( backpack, encumbrance ) ==
                   "--\n"
                   "<color_c_white>Encumbrance</color>:\n"
                   "Torso:  <color_c_yellow>2</color>  "
                   "When full:  <color_c_yellow>15</color>  "
                   "Coverage:  <color_c_yellow>30</color>\n" );

            // quiver has no volume, only an implicit volume via ammo
            CHECK( item_info_str( quiver, encumbrance ) ==
                   "--\n"
                   "<color_c_white>Encumbrance</color>:\n"
                   "L. Leg:  <color_c_yellow>3</color>  "
                   "When full:  <color_c_yellow>11</color>  "
                   "Coverage:  <color_c_yellow>10</color>\n"
                   "R. Leg:  <color_c_yellow>3</color>  "
                   "When full:  <color_c_yellow>11</color>  "
                   "Coverage:  <color_c_yellow>10</color>\n" );
        }
    }
}

// Related JSON fields:
// "bashing"
// "cutting"
// "to_hit"
//
// Functions:
// item::combat_info
TEST_CASE( "weapon attack ratings and moves", "[iteminfo][weapon]" )
{
    clear_avatar();
    Character &player_character = get_player_character();
    // new DPS calculations depend on the avatar's stats, so make sure they're consistent
    REQUIRE( player_character.get_str() == 8 );
    REQUIRE( player_character.get_dex() == 8 );

    item rag( "test_rag" );
    item rock( "test_rock" );
    item halligan( "test_halligan" );
    item mr_pointy( "test_pointy_stick" );
    item arrow( "test_arrow_wood" );

    SECTION( "melee damage" ) {
        // Melee damage comes from the "bashing" and "cutting" attributes in JSON

        // NOTE: BASE_DAMAGE info has no newline, since BASE_TOHIT always follows it
        std::vector<iteminfo_parts> damage = { iteminfo_parts::BASE_DAMAGE };

        SECTION( "no damage" ) {
            CHECK( item_info_str( rag, damage ).empty() );
        }

        SECTION( "bash" ) {
            CHECK( item_info_str( rock, damage ) ==
                   "--\n"
                   "<color_c_white>Melee damage</color>:"
                   " Bash: <color_c_yellow>7</color>" );
        }
        SECTION( "bash and cut" ) {
            CHECK( item_info_str( halligan, damage ) ==
                   "--\n"
                   "<color_c_white>Melee damage</color>:"
                   " Bash: <color_c_yellow>20</color>"
                   "  Cut: <color_c_yellow>5</color>" );
        }
        SECTION( "bash and pierce" ) {
            // Pierce damage comes from "cut" value, if item has the STAB or SPEAR flag
            REQUIRE( mr_pointy.has_flag( flag_SPEAR ) );
            CHECK( item_info_str( mr_pointy, damage ) ==
                   "--\n"
                   "<color_c_white>Melee damage</color>:"
                   " Bash: <color_c_yellow>5</color>"
                   "  Pierce: <color_c_yellow>9</color>" );
        }
        SECTION( "bash and cut (ranged ammo)" ) {
            CHECK( item_info_str( arrow, damage ) ==
                   "--\n"
                   "<color_c_white>Melee damage</color>:"
                   " Bash: <color_c_yellow>2</color>"
                   "  Cut: <color_c_yellow>1</color>" );
        }
    }

    SECTION( "to-hit rating" ) {
        // To-hit rating comes from the "to_hit" attribute in the item's JSON.

        std::vector<iteminfo_parts> to_hit = { iteminfo_parts::BASE_TOHIT };

        CHECK( item_info_str( rock, to_hit ) ==
               "--\n"
               "  To-hit bonus: <color_c_yellow>-2</color>\n" );

        CHECK( item_info_str( halligan, to_hit ) ==
               "--\n"
               "  To-hit bonus: <color_c_yellow>+2</color>\n" );

        CHECK( item_info_str( mr_pointy, to_hit ) ==
               "--\n"
               "  To-hit bonus: <color_c_yellow>-1</color>\n" );

        CHECK( item_info_str( arrow, to_hit ) ==
               "--\n"
               "  To-hit bonus: <color_c_yellow>+0</color>\n" );
    }

    SECTION( "base moves" ) {
        // Moves are calculated in item::attack_time based on item volume, weight, and count.
        // Those calculations are outside the scope of these tests, but we can at least ensure
        // they have expected values before checking the item info string.
        // If one of these fails, it suggests attack_time() changed:
        REQUIRE( rock.attack_time() == 79 );
        REQUIRE( halligan.attack_time() == 145 );
        REQUIRE( mr_pointy.attack_time() == 100 );
        REQUIRE( arrow.attack_time() == 65 );

        std::vector<iteminfo_parts> moves = { iteminfo_parts::BASE_MOVES };

        CHECK( item_info_str( rock, moves ) ==
               "--\n"
               "Moves per attack: <color_c_yellow>79</color>\n" );

        CHECK( item_info_str( halligan, moves ) ==
               "--\n"
               "Moves per attack: <color_c_yellow>145</color>\n" );

        CHECK( item_info_str( mr_pointy, moves ) ==
               "--\n"
               "Moves per attack: <color_c_yellow>100</color>\n" );

        CHECK( item_info_str( arrow, moves ) ==
               "--\n"
               "Moves per attack: <color_c_yellow>65</color>\n" );
    }

    SECTION( "base damage per second" ) {
        // Damage per second is dynamically calculated in item::dps and item::effective_dps based on
        // many different factors, all outside the scope of these tests. Here we just hope they
        // have the expected values in the item info summary.

        std::vector<iteminfo_parts> dps = { iteminfo_parts::BASE_DPS };

        CHECK( item_info_str( rock, dps ) ==
               "--\n"
               "Typical damage per second:\n"
               "Best: <color_c_yellow>4.92</color>"
               "  Vs. Agile: <color_c_yellow>2.05</color>"
               "  Vs. Armored: <color_c_yellow>0.15</color>\n" );

        CHECK( item_info_str( halligan, dps ) ==
               "--\n"
               "Typical damage per second:\n"
               "Best: <color_c_yellow>9.38</color>"
               "  Vs. Agile: <color_c_yellow>5.74</color>"
               "  Vs. Armored: <color_c_yellow>2.84</color>\n" );

        CHECK( item_info_str( mr_pointy, dps ) ==
               "--\n"
               "Typical damage per second:\n"
               "Best: <color_c_yellow>6.87</color>"
               "  Vs. Agile: <color_c_yellow>3.20</color>"
               "  Vs. Armored: <color_c_yellow>0.12</color>\n" );

        CHECK( item_info_str( arrow, dps ) ==
               "--\n"
               "Typical damage per second:\n"
               "Best: <color_c_yellow>4.90</color>"
               "  Vs. Agile: <color_c_yellow>2.46</color>"
               "  Vs. Armored: <color_c_yellow>0.00</color>\n" );
    }
}

// Related JSON fields:
// "techniques" - list of technique ids, ex. [ "WBLOCK_1", "BRUTAL", "SWEEP" ]
//
// Technique descriptions are defined in data/json/techniques.json
//
// Functions:
// item::combat_info
TEST_CASE( "techniques when wielded", "[iteminfo][weapon][techniques]" )
{
    clear_avatar();

    item halligan( "test_halligan" );
    CHECK( item_info_str( halligan, { iteminfo_parts::DESCRIPTION_TECHNIQUES } ) ==
           "--\n"
           "<color_c_white>Techniques when wielded</color>:"
           " <color_c_light_blue>Brutal Strike</color>:"
           " <color_c_cyan>Stun 1 turn, knockback 1 tile, crit only</color>,"
           " <color_c_light_blue>Sweep Attack</color>:"
           " <color_c_cyan>Down 2 turns</color>, and"
           " <color_c_light_blue>Block</color>:"
           " <color_c_cyan>Medium blocking ability</color>\n" );

    item plank( "test_2x4" );
    CHECK( item_info_str( plank, { iteminfo_parts::DESCRIPTION_TECHNIQUES } ) ==
           "--\n"
           "<color_c_white>Techniques when wielded</color>:"
           " <color_c_light_blue>Block</color>:"
           " <color_c_cyan>Medium blocking ability</color>\n" );
}

// Related JSON fields:
// "covers"
// "coverage"
// "warmth"
// "encumbrance"
//
// Functions:
// item::armor_info
TEST_CASE( "armor coverage, warmth, and encumbrance", "[iteminfo][armor][coverage]" )
{
    clear_avatar();

    SECTION( "armor with coverage shows covered body parts, warmth, encumbrance, and protection values" ) {
        // Long-sleeved shirt covering torso and arms
        item longshirt( "test_longshirt" );
        REQUIRE( longshirt.get_covered_body_parts().any() );
        REQUIRE( longshirt.get_coverage( bodypart_id( "torso" ) ) == 90 );
        REQUIRE( longshirt.get_coverage( bodypart_id( "arm_l" ) ) == 90 );
        REQUIRE( longshirt.get_coverage( bodypart_id( "arm_r" ) ) == 90 );
        REQUIRE( longshirt.get_coverage( bodypart_id( "leg_l" ) ) == 0 );
        REQUIRE( longshirt.get_coverage( bodypart_id( "leg_r" ) ) == 0 );
        REQUIRE( longshirt.get_coverage( bodypart_id( "hand_l" ) ) == 0 );
        REQUIRE( longshirt.get_coverage( bodypart_id( "hand_r" ) ) == 0 );
        REQUIRE( longshirt.get_coverage( bodypart_id( "head" ) ) == 0 );
        REQUIRE( longshirt.get_coverage( bodypart_id( "mouth" ) ) == 0 );
        REQUIRE( longshirt.get_coverage( bodypart_id( "foot_l" ) ) == 0 );
        REQUIRE( longshirt.get_coverage( bodypart_id( "foot_r" ) ) == 0 );

        CHECK( item_info_str( longshirt, { iteminfo_parts::ARMOR_BODYPARTS } ) ==
               "--\n"
               "<color_c_white>Covers</color>:"
               " The <color_c_cyan>torso</color>."
               " The <color_c_cyan>arms</color>.\n" );

        CHECK( item_info_str( longshirt, { iteminfo_parts::ARMOR_LAYER } ) ==
               "--\n"
               "Layer: <color_c_light_blue>Normal</color>.\n" );

        // Coverage and warmth are displayed together on a single line
        std::vector<iteminfo_parts> cov_warm_shirt = { iteminfo_parts::ARMOR_COVERAGE, iteminfo_parts::ARMOR_WARMTH };
        REQUIRE( longshirt.get_avg_coverage() == 90 );
        REQUIRE( longshirt.get_warmth() == 5 );
        CHECK( item_info_str( longshirt, cov_warm_shirt )
               ==
               "--\n"
               "Average Coverage: <color_c_yellow>90</color>%  Warmth: <color_c_yellow>5</color>\n" );

        REQUIRE( longshirt.get_avg_encumber( get_player_character() ) == 3 );
        REQUIRE( longshirt.get_encumber( get_player_character(), bodypart_id( "torso" ) ) == 3 );
        REQUIRE( longshirt.get_encumber( get_player_character(), bodypart_id( "arm_l" ) ) == 3 );
        REQUIRE( longshirt.get_encumber( get_player_character(), bodypart_id( "arm_r" ) ) == 3 );
        REQUIRE( longshirt.get_encumber( get_player_character(), bodypart_id( "leg_l" ) ) == 0 );
        REQUIRE( longshirt.get_encumber( get_player_character(), bodypart_id( "leg_r" ) ) == 0 );
        REQUIRE( longshirt.get_encumber( get_player_character(), bodypart_id( "hand_l" ) ) == 0 );
        REQUIRE( longshirt.get_encumber( get_player_character(), bodypart_id( "hand_r" ) ) == 0 );
        REQUIRE( longshirt.get_encumber( get_player_character(), bodypart_id( "head" ) ) == 0 );
        REQUIRE( longshirt.get_encumber( get_player_character(), bodypart_id( "mouth" ) ) == 0 );
        REQUIRE( longshirt.get_encumber( get_player_character(), bodypart_id( "foot_l" ) ) == 0 );
        REQUIRE( longshirt.get_encumber( get_player_character(), bodypart_id( "foot_r" ) ) == 0 );

        REQUIRE( longshirt.get_avg_encumber( get_player_character(),
                                             item::encumber_flags::assume_full ) == 3 );
        REQUIRE( longshirt.get_encumber( get_player_character(), bodypart_id( "torso" ),
                                         item::encumber_flags::assume_full ) == 3 );
        REQUIRE( longshirt.get_encumber( get_player_character(), bodypart_id( "arm_l" ),
                                         item::encumber_flags::assume_full ) == 3 );
        REQUIRE( longshirt.get_encumber( get_player_character(), bodypart_id( "arm_r" ),
                                         item::encumber_flags::assume_full ) == 3 );
        REQUIRE( longshirt.get_encumber( get_player_character(), bodypart_id( "leg_l" ),
                                         item::encumber_flags::assume_full ) == 0 );
        REQUIRE( longshirt.get_encumber( get_player_character(), bodypart_id( "leg_r" ),
                                         item::encumber_flags::assume_full ) == 0 );
        REQUIRE( longshirt.get_encumber( get_player_character(), bodypart_id( "hand_l" ),
                                         item::encumber_flags::assume_full ) == 0 );
        REQUIRE( longshirt.get_encumber( get_player_character(), bodypart_id( "hand_r" ),
                                         item::encumber_flags::assume_full ) == 0 );
        REQUIRE( longshirt.get_encumber( get_player_character(), bodypart_id( "head" ),
                                         item::encumber_flags::assume_full ) == 0 );
        REQUIRE( longshirt.get_encumber( get_player_character(), bodypart_id( "mouth" ),
                                         item::encumber_flags::assume_full ) == 0 );
        REQUIRE( longshirt.get_encumber( get_player_character(), bodypart_id( "foot_l" ),
                                         item::encumber_flags::assume_full ) == 0 );
        REQUIRE( longshirt.get_encumber( get_player_character(), bodypart_id( "foot_r" ),
                                         item::encumber_flags::assume_full ) == 0 );

        CHECK( item_info_str( longshirt, { iteminfo_parts::ARMOR_ENCUMBRANCE } ) ==
               "--\n"
               "<color_c_white>Encumbrance</color>: <color_c_red>(poor fit)</color>\n"
               "Arms:  <color_c_yellow>3</color>  "
               "Coverage:  <color_c_yellow>90</color>\n"
               "Torso:  <color_c_yellow>3</color>  "
               "Coverage:  <color_c_yellow>90</color>\n" );

        item swat_armor( "test_swat_armor" );
        REQUIRE( swat_armor.get_covered_body_parts().any() );

        CHECK( item_info_str( swat_armor, { iteminfo_parts::ARMOR_BODYPARTS } ) ==
               "--\n"
               "<color_c_white>Covers</color>:"
               " The <color_c_cyan>torso</color>."
               " The <color_c_cyan>arms</color>."
               " The <color_c_cyan>legs</color>.\n" );

        CHECK( item_info_str( swat_armor, { iteminfo_parts::ARMOR_LAYER } ) ==
               "--\n"
               "Layer: <color_c_light_blue>Normal</color>.\n" );

        std::vector<iteminfo_parts> cov_warm_swat = { iteminfo_parts::ARMOR_COVERAGE, iteminfo_parts::ARMOR_WARMTH };
        REQUIRE( swat_armor.get_avg_coverage() == 95 );
        REQUIRE( swat_armor.get_warmth() == 35 );
        CHECK( item_info_str( swat_armor, cov_warm_swat )
               ==
               "--\n"
               "Average Coverage: <color_c_yellow>95</color>%  Warmth: <color_c_yellow>35</color>\n" );

        REQUIRE( swat_armor.get_coverage( bodypart_id( "torso" ) ) == 95 );
        REQUIRE( swat_armor.get_coverage( bodypart_id( "leg_l" ) ) == 95 );
        REQUIRE( swat_armor.get_coverage( bodypart_id( "leg_r" ) ) == 95 );
        REQUIRE( swat_armor.get_coverage( bodypart_id( "arm_l" ) ) == 95 );
        REQUIRE( swat_armor.get_coverage( bodypart_id( "arm_r" ) ) == 95 );
        REQUIRE( swat_armor.get_coverage( bodypart_id( "head" ) ) == 0 );
        REQUIRE( swat_armor.get_coverage( bodypart_id( "foot_l" ) ) == 0 );
        REQUIRE( swat_armor.get_coverage( bodypart_id( "foot_r" ) ) == 0 );
        REQUIRE( swat_armor.get_coverage( bodypart_id( "eyes" ) ) == 0 );
        REQUIRE( swat_armor.get_coverage( bodypart_id( "mouth" ) ) == 0 );
        REQUIRE( swat_armor.get_coverage( bodypart_id( "hand_r" ) ) == 0 );
        REQUIRE( swat_armor.get_coverage( bodypart_id( "hand_l" ) ) == 0 );

        REQUIRE( swat_armor.get_avg_encumber( get_player_character() ) == 12 );
        REQUIRE( swat_armor.get_encumber( get_player_character(), bodypart_id( "torso" ) ) == 12 );
        REQUIRE( swat_armor.get_encumber( get_player_character(), bodypart_id( "leg_l" ) ) == 12 );
        REQUIRE( swat_armor.get_encumber( get_player_character(), bodypart_id( "leg_r" ) ) == 12 );
        REQUIRE( swat_armor.get_encumber( get_player_character(), bodypart_id( "arm_l" ) ) == 12 );
        REQUIRE( swat_armor.get_encumber( get_player_character(), bodypart_id( "arm_r" ) ) == 12 );
        REQUIRE( swat_armor.get_encumber( get_player_character(), bodypart_id( "head" ) ) == 0 );
        REQUIRE( swat_armor.get_encumber( get_player_character(), bodypart_id( "foot_l" ) ) == 0 );
        REQUIRE( swat_armor.get_encumber( get_player_character(), bodypart_id( "foot_r" ) ) == 0 );
        REQUIRE( swat_armor.get_encumber( get_player_character(), bodypart_id( "eyes" ) ) == 0 );
        REQUIRE( swat_armor.get_encumber( get_player_character(), bodypart_id( "mouth" ) ) == 0 );
        REQUIRE( swat_armor.get_encumber( get_player_character(), bodypart_id( "hand_l" ) ) == 0 );
        REQUIRE( swat_armor.get_encumber( get_player_character(), bodypart_id( "hand_r" ) ) == 0 );

        REQUIRE( swat_armor.get_avg_encumber( get_player_character(),
                                              item::encumber_flags::assume_full ) == 25 );
        REQUIRE( swat_armor.get_encumber( get_player_character(), bodypart_id( "torso" ),
                                          item::encumber_flags::assume_full ) == 25 );
        REQUIRE( swat_armor.get_encumber( get_player_character(), bodypart_id( "leg_l" ),
                                          item::encumber_flags::assume_full ) == 25 );
        REQUIRE( swat_armor.get_encumber( get_player_character(), bodypart_id( "leg_r" ),
                                          item::encumber_flags::assume_full ) == 25 );
        REQUIRE( swat_armor.get_encumber( get_player_character(), bodypart_id( "arm_l" ),
                                          item::encumber_flags::assume_full ) == 25 );
        REQUIRE( swat_armor.get_encumber( get_player_character(), bodypart_id( "arm_r" ),
                                          item::encumber_flags::assume_full ) == 25 );
        REQUIRE( swat_armor.get_encumber( get_player_character(), bodypart_id( "foot_l" ),
                                          item::encumber_flags::assume_full ) == 0 );
        REQUIRE( swat_armor.get_encumber( get_player_character(), bodypart_id( "foot_r" ),
                                          item::encumber_flags::assume_full ) == 0 );
        REQUIRE( swat_armor.get_encumber( get_player_character(), bodypart_id( "head" ),
                                          item::encumber_flags::assume_full ) == 0 );
        REQUIRE( swat_armor.get_encumber( get_player_character(), bodypart_id( "eyes" ),
                                          item::encumber_flags::assume_full ) == 0 );
        REQUIRE( swat_armor.get_encumber( get_player_character(), bodypart_id( "mouth" ),
                                          item::encumber_flags::assume_full ) == 0 );
        REQUIRE( swat_armor.get_encumber( get_player_character(), bodypart_id( "hand_l" ),
                                          item::encumber_flags::assume_full ) == 0 );
        REQUIRE( swat_armor.get_encumber( get_player_character(), bodypart_id( "hand_r" ),
                                          item::encumber_flags::assume_full ) == 0 );

        CHECK( item_info_str( swat_armor, { iteminfo_parts::ARMOR_ENCUMBRANCE } ) ==
               "--\n"
               "<color_c_white>Encumbrance</color>:\n"
               "Arms:  <color_c_yellow>12</color>  "
               "When full:  <color_c_yellow>25</color>  "
               "Coverage:  <color_c_yellow>95</color>\n"
               "Legs:  <color_c_yellow>12</color>  "
               "When full:  <color_c_yellow>25</color>  "
               "Coverage:  <color_c_yellow>95</color>\n"
               "Torso:  <color_c_yellow>12</color>  "
               "When full:  <color_c_yellow>25</color>  "
               "Coverage:  <color_c_yellow>95</color>\n" );

        // Test copy-from
        item faux_fur_pants( "test_pants_faux_fur" );
        REQUIRE( faux_fur_pants.get_covered_body_parts().any() );

        CHECK( item_info_str( faux_fur_pants, { iteminfo_parts::ARMOR_BODYPARTS } ) ==
               "--\n"
               "<color_c_white>Covers</color>:"
               " The <color_c_cyan>legs</color>.\n" );

        CHECK( item_info_str( faux_fur_pants, { iteminfo_parts::ARMOR_LAYER } ) ==
               "--\n"
               "Layer: <color_c_light_blue>Normal</color>.\n" );

        std::vector<iteminfo_parts> cov_warm_pants = { iteminfo_parts::ARMOR_COVERAGE, iteminfo_parts::ARMOR_WARMTH };
        REQUIRE( faux_fur_pants.get_avg_coverage() == 95 );
        REQUIRE( faux_fur_pants.get_warmth() == 70 );
        CHECK( item_info_str( faux_fur_pants, cov_warm_pants )
               ==
               "--\n"
               "Average Coverage: <color_c_yellow>95</color>%  Warmth: <color_c_yellow>70</color>\n" );

        REQUIRE( faux_fur_pants.get_avg_coverage() == 95 );
        REQUIRE( faux_fur_pants.get_coverage( bodypart_id( "leg_l" ) ) == 95 );
        REQUIRE( faux_fur_pants.get_coverage( bodypart_id( "leg_r" ) ) == 95 );
        REQUIRE( faux_fur_pants.get_coverage( bodypart_id( "arm_l" ) ) == 0 );
        REQUIRE( faux_fur_pants.get_coverage( bodypart_id( "arm_r" ) ) == 0 );
        REQUIRE( faux_fur_pants.get_coverage( bodypart_id( "foot_r" ) ) == 0 );
        REQUIRE( faux_fur_pants.get_coverage( bodypart_id( "foot_l" ) ) == 0 );
        REQUIRE( faux_fur_pants.get_coverage( bodypart_id( "head" ) ) == 0 );
        REQUIRE( faux_fur_pants.get_coverage( bodypart_id( "eyes" ) ) == 0 );
        REQUIRE( faux_fur_pants.get_coverage( bodypart_id( "mouth" ) ) == 0 );
        REQUIRE( faux_fur_pants.get_coverage( bodypart_id( "hand_l" ) ) == 0 );
        REQUIRE( faux_fur_pants.get_coverage( bodypart_id( "hand_r" ) ) == 0 );

        REQUIRE( faux_fur_pants.get_avg_encumber( get_player_character() ) == 16 );
        REQUIRE( faux_fur_pants.get_encumber( get_player_character(), bodypart_id( "leg_l" ) ) == 16 );
        REQUIRE( faux_fur_pants.get_encumber( get_player_character(), bodypart_id( "leg_r" ) ) == 16 );
        REQUIRE( faux_fur_pants.get_encumber( get_player_character(), bodypart_id( "arm_l" ) ) == 0 );
        REQUIRE( faux_fur_pants.get_encumber( get_player_character(), bodypart_id( "arm_r" ) ) == 0 );
        REQUIRE( faux_fur_pants.get_encumber( get_player_character(), bodypart_id( "foot_r" ) ) == 0 );
        REQUIRE( faux_fur_pants.get_encumber( get_player_character(), bodypart_id( "foot_l" ) ) == 0 );
        REQUIRE( faux_fur_pants.get_encumber( get_player_character(), bodypart_id( "head" ) ) == 0 );
        REQUIRE( faux_fur_pants.get_encumber( get_player_character(), bodypart_id( "eyes" ) ) == 0 );
        REQUIRE( faux_fur_pants.get_encumber( get_player_character(), bodypart_id( "mouth" ) ) == 0 );
        REQUIRE( faux_fur_pants.get_encumber( get_player_character(), bodypart_id( "hand_l" ) ) == 0 );
        REQUIRE( faux_fur_pants.get_encumber( get_player_character(), bodypart_id( "hand_r" ) ) == 0 );

        REQUIRE( faux_fur_pants.get_avg_encumber( get_player_character(),
                 item::encumber_flags::assume_full ) == 20 );
        REQUIRE( faux_fur_pants.get_encumber( get_player_character(), bodypart_id( "leg_l" ),
                                              item::encumber_flags::assume_full ) == 20 );
        REQUIRE( faux_fur_pants.get_encumber( get_player_character(), bodypart_id( "leg_r" ),
                                              item::encumber_flags::assume_full ) == 20 );
        REQUIRE( faux_fur_pants.get_encumber( get_player_character(), bodypart_id( "arm_l" ),
                                              item::encumber_flags::assume_full ) == 0 );
        REQUIRE( faux_fur_pants.get_encumber( get_player_character(), bodypart_id( "arm_r" ),
                                              item::encumber_flags::assume_full ) == 0 );
        REQUIRE( faux_fur_pants.get_encumber( get_player_character(), bodypart_id( "foot_r" ),
                                              item::encumber_flags::assume_full ) == 0 );
        REQUIRE( faux_fur_pants.get_encumber( get_player_character(), bodypart_id( "foot_l" ),
                                              item::encumber_flags::assume_full ) == 0 );
        REQUIRE( faux_fur_pants.get_encumber( get_player_character(), bodypart_id( "head" ),
                                              item::encumber_flags::assume_full ) == 0 );
        REQUIRE( faux_fur_pants.get_encumber( get_player_character(), bodypart_id( "eyes" ),
                                              item::encumber_flags::assume_full ) == 0 );
        REQUIRE( faux_fur_pants.get_encumber( get_player_character(), bodypart_id( "mouth" ),
                                              item::encumber_flags::assume_full ) == 0 );
        REQUIRE( faux_fur_pants.get_encumber( get_player_character(), bodypart_id( "hand_l" ),
                                              item::encumber_flags::assume_full ) == 0 );
        REQUIRE( faux_fur_pants.get_encumber( get_player_character(), bodypart_id( "hand_r" ),
                                              item::encumber_flags::assume_full ) == 0 );

        item faux_fur_suit( "test_portion_faux_fur_pants_suit" );
        REQUIRE( faux_fur_suit.get_covered_body_parts().any() );

        CHECK( item_info_str( faux_fur_suit, { iteminfo_parts::ARMOR_BODYPARTS } ) ==
               "--\n"
               "<color_c_white>Covers</color>:"
               " The <color_c_cyan>head</color>."
               " The <color_c_cyan>torso</color>."
               " The <color_c_cyan>arms</color>."
               " The <color_c_cyan>legs</color>.\n" );

        CHECK( item_info_str( faux_fur_suit, { iteminfo_parts::ARMOR_LAYER } ) ==
               "--\n"
               "Layer: <color_c_light_blue>Normal</color>.\n" );

        std::vector<iteminfo_parts> cov_warm_suit = { iteminfo_parts::ARMOR_COVERAGE, iteminfo_parts::ARMOR_WARMTH };
        REQUIRE( faux_fur_suit.get_avg_coverage() == 75 );
        REQUIRE( faux_fur_suit.get_warmth() == 5 );
        CHECK( item_info_str( faux_fur_suit, cov_warm_suit )
               ==
               "--\n"
               "Average Coverage: <color_c_yellow>75</color>%  Warmth: <color_c_yellow>5</color>\n" );

        REQUIRE( faux_fur_suit.get_avg_coverage() == 75 );
        REQUIRE( faux_fur_suit.get_coverage( bodypart_id( "torso" ) ) == 100 );
        REQUIRE( faux_fur_suit.get_coverage( bodypart_id( "leg_l" ) ) == 50 );
        REQUIRE( faux_fur_suit.get_coverage( bodypart_id( "leg_r" ) ) == 100 );
        REQUIRE( faux_fur_suit.get_coverage( bodypart_id( "arm_l" ) ) == 50 );
        REQUIRE( faux_fur_suit.get_coverage( bodypart_id( "arm_r" ) ) == 100 );
        REQUIRE( faux_fur_suit.get_coverage( bodypart_id( "head" ) ) == 50 );
        REQUIRE( faux_fur_suit.get_coverage( bodypart_id( "eyes" ) ) == 0 );
        REQUIRE( faux_fur_suit.get_coverage( bodypart_id( "mouth" ) ) == 0 );
        REQUIRE( faux_fur_suit.get_coverage( bodypart_id( "hand_l" ) ) == 0 );
        REQUIRE( faux_fur_suit.get_coverage( bodypart_id( "hand_r" ) ) == 0 );
        REQUIRE( faux_fur_suit.get_coverage( bodypart_id( "foot_l" ) ) == 0 );
        REQUIRE( faux_fur_suit.get_coverage( bodypart_id( "foot_r" ) ) == 0 );

        REQUIRE( faux_fur_suit.get_avg_encumber( get_player_character() ) == 7 );
        REQUIRE( faux_fur_suit.get_encumber( get_player_character(), bodypart_id( "torso" ) ) == 10 );
        REQUIRE( faux_fur_suit.get_encumber( get_player_character(), bodypart_id( "leg_l" ) ) == 5 );
        REQUIRE( faux_fur_suit.get_encumber( get_player_character(), bodypart_id( "leg_r" ) ) == 10 );
        REQUIRE( faux_fur_suit.get_encumber( get_player_character(), bodypart_id( "arm_l" ) ) == 5 );
        REQUIRE( faux_fur_suit.get_encumber( get_player_character(), bodypart_id( "arm_r" ) ) == 10 );
        REQUIRE( faux_fur_suit.get_encumber( get_player_character(), bodypart_id( "head" ) ) == 5 );
        REQUIRE( faux_fur_suit.get_encumber( get_player_character(), bodypart_id( "eyes" ) ) == 0 );
        REQUIRE( faux_fur_suit.get_encumber( get_player_character(), bodypart_id( "mouth" ) ) == 0 );
        REQUIRE( faux_fur_suit.get_encumber( get_player_character(), bodypart_id( "foot_l" ) ) == 0 );
        REQUIRE( faux_fur_suit.get_encumber( get_player_character(), bodypart_id( "foot_r" ) ) == 0 );
        REQUIRE( faux_fur_suit.get_encumber( get_player_character(), bodypart_id( "hand_r" ) ) == 0 );
        REQUIRE( faux_fur_suit.get_encumber( get_player_character(), bodypart_id( "hand_r" ) ) == 0 );

        REQUIRE( faux_fur_suit.get_avg_encumber( get_player_character(),
                 item::encumber_flags::assume_full ) == 17 );
        REQUIRE( faux_fur_suit.get_encumber( get_player_character(), bodypart_id( "torso" ),
                                             item::encumber_flags::assume_full ) == 25 );
        REQUIRE( faux_fur_suit.get_encumber( get_player_character(), bodypart_id( "leg_l" ),
                                             item::encumber_flags::assume_full ) == 9 );
        REQUIRE( faux_fur_suit.get_encumber( get_player_character(), bodypart_id( "leg_r" ),
                                             item::encumber_flags::assume_full ) == 25 );
        REQUIRE( faux_fur_suit.get_encumber( get_player_character(), bodypart_id( "arm_l" ),
                                             item::encumber_flags::assume_full ) == 9 );
        REQUIRE( faux_fur_suit.get_encumber( get_player_character(), bodypart_id( "arm_r" ),
                                             item::encumber_flags::assume_full ) == 25 );
        REQUIRE( faux_fur_suit.get_encumber( get_player_character(), bodypart_id( "head" ),
                                             item::encumber_flags::assume_full ) == 9 );
        REQUIRE( faux_fur_suit.get_encumber( get_player_character(), bodypart_id( "eyes" ),
                                             item::encumber_flags::assume_full ) == 0 );
        REQUIRE( faux_fur_suit.get_encumber( get_player_character(), bodypart_id( "mouth" ),
                                             item::encumber_flags::assume_full ) == 0 );
        REQUIRE( faux_fur_suit.get_encumber( get_player_character(), bodypart_id( "hand_l" ),
                                             item::encumber_flags::assume_full ) == 0 );
        REQUIRE( faux_fur_suit.get_encumber( get_player_character(), bodypart_id( "hand_r" ),
                                             item::encumber_flags::assume_full ) == 0 );
        REQUIRE( faux_fur_suit.get_encumber( get_player_character(), bodypart_id( "foot_l" ),
                                             item::encumber_flags::assume_full ) == 0 );
        REQUIRE( faux_fur_suit.get_encumber( get_player_character(), bodypart_id( "foot_r" ),
                                             item::encumber_flags::assume_full ) == 0 );

        CHECK( item_info_str( faux_fur_suit, { iteminfo_parts::ARMOR_ENCUMBRANCE } ) ==
               "--\n"
               "<color_c_white>Encumbrance</color>: <color_c_red>(poor fit)</color>\n"
               "L. Arm:  <color_c_yellow>5</color>  "
               "When full:  <color_c_yellow>9</color>  "
               "Coverage:  <color_c_yellow>50</color>\n"
               "R. Arm:  <color_c_yellow>10</color>  "
               "When full:  <color_c_yellow>25</color>  "
               "Coverage:  <color_c_yellow>100</color>\n"
               "Head:  <color_c_yellow>5</color>  "
               "When full:  <color_c_yellow>9</color>  "
               "Coverage:  <color_c_yellow>50</color>\n"
               "L. Leg:  <color_c_yellow>5</color>  "
               "When full:  <color_c_yellow>9</color>  "
               "Coverage:  <color_c_yellow>50</color>\n"
               "R. Leg:  <color_c_yellow>10</color>  "
               "When full:  <color_c_yellow>25</color>  "
               "Coverage:  <color_c_yellow>100</color>\n"
               "Torso:  <color_c_yellow>10</color>  "
               "When full:  <color_c_yellow>25</color>  "
               "Coverage:  <color_c_yellow>100</color>\n" );
    }

    SECTION( "armor with no coverage omits irrelevant info" ) {
        // Ear plugs with no coverage, and no other info to display
        item ear_plugs( "test_ear_plugs" );
        REQUIRE_FALSE( ear_plugs.get_covered_body_parts().any() );

        CHECK( item_info_str( ear_plugs, { iteminfo_parts::ARMOR_BODYPARTS, iteminfo_parts::ARMOR_LAYER,
                                           iteminfo_parts::ARMOR_COVERAGE, iteminfo_parts::ARMOR_WARMTH,
                                           iteminfo_parts::ARMOR_ENCUMBRANCE, iteminfo_parts::ARMOR_PROTECTION
                                         } ) ==
               "--\n"
               "<color_c_white>Covers</color>: <color_c_cyan>Nothing</color>.\n" );
    }
}

// Related JSON fields:
// "covers"
// "flags"
// "power_armor"
//
// Functions:
// item::armor_fit_info
TEST_CASE( "armor fit and sizing", "[iteminfo][armor][fit]" )
{
    clear_avatar();

    std::vector<iteminfo_parts> varsize = { iteminfo_parts::DESCRIPTION_FLAGS_VARSIZE };
    std::vector<iteminfo_parts> sided = { iteminfo_parts::DESCRIPTION_FLAGS_SIDED };
    std::vector<iteminfo_parts> powerarmor = { iteminfo_parts::DESCRIPTION_FLAGS_POWERARMOR };

    // TODO: Test items with these
    //std::vector<iteminfo_parts> helmet_compat = { iteminfo_parts::DESCRIPTION_FLAGS_HELMETCOMPAT };
    //std::vector<iteminfo_parts> fits = { iteminfo_parts::DESCRIPTION_FLAGS_FITS };
    //std::vector<iteminfo_parts> irradiation = { iteminfo_parts::DESCRIPTION_IRRADIATION };
    //std::vector<iteminfo_parts> powerarmor_rad = { iteminfo_parts::DESCRIPTION_FLAGS_POWERARMOR_RADIATIONHINT };

    // Items with VARSIZE flag can be fitted
    item socks( "test_socks" );
    CHECK( item_info_str( socks, varsize ) ==
           "--\n"
           "* This clothing <color_c_cyan>can be refitted</color>.\n" );

    // Items with "covers" LEG_EITHER, ARM_EITHER, FOOT_EITHER, HAND_EITHER are "sided"
    item briefcase( "test_briefcase" );
    CHECK( item_info_str( briefcase, sided ) ==
           "--\n"
           "* This item can be worn on <color_c_cyan>either side</color> of the body.\n" );

    item power_armor( "test_power_armor" );
    CHECK( item_info_str( power_armor, powerarmor ) ==
           "--\n"
           "* This gear is a part of power armor.\n" );
}

static void expected_armor_values( const item &armor, float bash, float cut, float stab,
                                   float bullet,
                                   float acid = 0.0f, float fire = 0.0f, float env = 0.0f )
{
    CAPTURE( armor.typeId().str() );
    REQUIRE( armor.bash_resist() == Approx( bash ) );
    REQUIRE( armor.cut_resist() == Approx( cut ) );
    REQUIRE( armor.stab_resist() == Approx( stab ) );
    REQUIRE( armor.bullet_resist() == Approx( bullet ) );
    REQUIRE( armor.acid_resist() == Approx( acid ) );
    REQUIRE( armor.fire_resist() == Approx( fire ) );
    REQUIRE( armor.get_env_resist() == Approx( env ) );
}

TEST_CASE( "armor_stats", "[armor][protection]" )
{
    expected_armor_values( item( itype_id( "zentai" ) ), 0.2f, 0.2f, 0.16f, 0.2f );
    expected_armor_values( item( itype_id( "tshirt" ) ), 0.1f, 0.1f, 0.08f, 0.1f );
    expected_armor_values( item( itype_id( "dress_shirt" ) ), 0.1f, 0.1f, 0.08f, 0.1f );
}

// Armor protction is based on materials, thickness, and/or environmental protection rating.
// For armor defined in JSON:
//
// - "materials" determine the resistance factors (bash, cut, fire etc.)
// - "material_thickness" multiplies bash, cut, and bullet resistance
// - "environmental_protection" gives acid and fire resist (N/10 if less than 10)
//
// See doc/DEVELOPER_FAQ.md "How armor protection is calculated" for more.
//
// Materials and protection calculations are not tested here; only their display in item info.
//
// item::armor_protection_info
TEST_CASE( "armor protection", "[iteminfo][armor][protection]" )
{
    clear_avatar();

    std::vector<iteminfo_parts> protection = { iteminfo_parts::ARMOR_PROTECTION };

    // TODO:
    // - Air filtration or gas mask (inactive/active)
    // - Damaged armor reduces protection

    SECTION( "no protection from physical, no protection from environmental" ) {
        // Long-sleeved shirt, material:cotton, thickness:0.2
        // 1/1/1 bash/cut/bullet x 1 thickness
        // 0/0/0 acid/fire/env
        item longshirt( "test_longshirt" );
        expected_armor_values( longshirt, 0.2f, 0.2f, 0.16f, 0.2f );
        REQUIRE( longshirt.get_covered_body_parts().any() );

        // Protection info displayed on two lines
        CHECK( item_info_str( longshirt, protection ) ==
               "--\n"
               "<color_c_white>Protection</color>:"
               " Bash: <color_c_yellow>0.20</color>"
               "  Cut: <color_c_yellow>0.20</color>"
               "  Ballistic: <color_c_yellow>0.20</color>\n"
               "  Acid: <color_c_yellow>0.00</color>"
               "  Fire: <color_c_yellow>0.00</color>"
               "  Environmental: <color_c_yellow>0</color>\n" );
    }

    SECTION( "moderate protection from physical and environmental damage" ) {
        // Hazmat suit, material:plastic, thickness:2
        // 2/2/2 bash/cut/bullet x 2 thickness
        // 9/1/20 acid/fire/env
        item hazmat( "test_hazmat_suit" );
        REQUIRE( hazmat.get_covered_body_parts().any() );
        expected_armor_values( hazmat, 4, 4, 3.2, 4, 9, 1, 20 );

        // Protection info displayed on two lines
        CHECK( item_info_str( hazmat, protection ) ==
               "--\n"
               "<color_c_white>Protection</color>:"
               " Bash: <color_c_yellow>4.00</color>"
               "  Cut: <color_c_yellow>4.00</color>"
               "  Ballistic: <color_c_yellow>4.00</color>\n"
               "  Acid: <color_c_yellow>9.00</color>"
               "  Fire: <color_c_yellow>1.00</color>"
               "  Environmental: <color_c_yellow>20</color>\n" );
    }

    SECTION( "pet armor with good physical and environmental protection" ) {
        // Kevlar cat harness, for reasons
        // material:layered_kevlar, thickness:2
        // 2/3/5 bash/cut/bullet x 2 thickness
        // 5/3/10 acid/fire/env
        item meower_armor( "test_meower_armor" );
        expected_armor_values( meower_armor, 4, 6, 4.8, 10, 5, 3, 10 );

        CHECK( item_info_str( meower_armor, protection ) ==
               "--\n"
               "<color_c_white>Protection</color>:"
               " Bash: <color_c_yellow>4.00</color>"
               "  Cut: <color_c_yellow>6.00</color>"
               "  Ballistic: <color_c_yellow>10.00</color>\n"
               "  Acid: <color_c_yellow>5.00</color>"
               "  Fire: <color_c_yellow>3.00</color>"
               "  Environmental: <color_c_yellow>10</color>\n" );
    }
}

// Related JSON fields:
// "fun"
// "time"
// "skill"
// "required_level"
// "max_level"
// "intelligence"
//
// Functions:
// item::book_info
TEST_CASE( "book info", "[iteminfo][book]" )
{
    clear_avatar();

    // Parts to test
    std::vector<iteminfo_parts> summary = { iteminfo_parts::BOOK_SUMMARY };
    std::vector<iteminfo_parts> reqs_begin = { iteminfo_parts::BOOK_REQUIREMENTS_BEGINNER };
    std::vector<iteminfo_parts> skill_min = { iteminfo_parts::BOOK_SKILLRANGE_MIN };
    std::vector<iteminfo_parts> skill_max = { iteminfo_parts::BOOK_SKILLRANGE_MAX };
    std::vector<iteminfo_parts> reqs_int = { iteminfo_parts::BOOK_REQUIREMENTS_INT };
    std::vector<iteminfo_parts> morale = { iteminfo_parts::BOOK_MORALECHANGE };
    std::vector<iteminfo_parts> time_per_chapter = { iteminfo_parts::BOOK_TIMEPERCHAPTER };

    // TODO: include test for these:
    // std::vector<iteminfo_parts> num_unread = { iteminfo_parts::BOOK_NUMUNREADCHAPTERS };
    // std::vector<iteminfo_parts> included_recipes = { iteminfo_parts::BOOK_INCLUDED_RECIPES };

    item dragon( "test_dragon_book" );
    item cmdline( "test_cmdline_book" );
    // TODO: add martial arts book to test data

    REQUIRE( dragon.is_book() );
    REQUIRE( cmdline.is_book() );

    // summary is shown for martial arts books and "just for fun" books, but no others
    WHEN( "book has not been identified" ) {
        THEN( "some basic book info is shown" ) {
            // Some info is always shown
            CHECK( item_info_str( cmdline, summary ) ==
                   "--\n"
                   "Just for fun.\n" );

            CHECK( item_info_str( cmdline, reqs_begin ) ==
                   "--\n"
                   "It can be <color_c_cyan>understood by beginners</color>.\n" );
        }

        THEN( "other book info is hidden" ) {
            CHECK( item_info_str( cmdline, skill_min ).empty() );
            CHECK( item_info_str( cmdline, skill_max ).empty() );
            CHECK( item_info_str( cmdline, reqs_int ).empty() );
            CHECK( item_info_str( cmdline, morale ).empty() );
            CHECK( item_info_str( cmdline, time_per_chapter ).empty() );
        }
    }

    WHEN( "book has been identified" ) {
        avatar &player_character = get_avatar();
        player_character.do_read( cmdline );
        player_character.do_read( dragon );

        THEN( "some basic book info is shown" ) {
            CHECK( item_info_str( cmdline, summary ) ==
                   "--\n"
                   "Just for fun.\n" );

            CHECK( item_info_str( cmdline, reqs_begin ) ==
                   "--\n"
                   "It can be <color_c_cyan>understood by beginners</color>.\n" );

        }

        THEN( "morale (fun) info is shown" ) {
            // JSON field: "fun"
            CHECK( item_info_str( cmdline, morale ) ==
                   "--\n"
                   "Reading this book affects your morale by <color_c_yellow>+2</color>\n" );

            CHECK( item_info_str( dragon, morale ) ==
                   "--\n"
                   "Reading this book affects your morale by <color_c_yellow>-1</color>\n" );
        }

        THEN( "time per chapter is shown" ) {
            // JSON field: "time"
            CHECK( item_info_str( cmdline, time_per_chapter ) ==
                   "--\n"
                   "A chapter of this book takes <color_c_yellow>5</color>"
                   " <color_c_cyan>minutes to read</color>.\n" );

            CHECK( item_info_str( dragon, time_per_chapter ) ==
                   "--\n"
                   "A chapter of this book takes <color_c_yellow>50</color>"
                   " <color_c_cyan>minutes to read</color>.\n" );
        }

        THEN( "book requirement info is shown" ) {
            // Book with skill min/max and intelligence requirement
            // JSON fields: "required_level", "max_level", "intelligence"
            CHECK( item_info_str( dragon, skill_min ) ==
                   "--\n"
                   "<color_c_cyan>Requires computers level</color> <color_c_yellow>4</color> to understand.\n" );

            CHECK( item_info_str( dragon, skill_max ) ==
                   "--\n"
                   "Can bring your <color_c_cyan>computers skill to</color> <color_c_yellow>7</color>.\n"
                   "Your current <color_c_light_blue>computers skill</color> is <color_c_yellow>0</color>.\n" );

            CHECK( item_info_str( dragon, reqs_int ) ==
                   "--\n"
                   "Requires <color_c_cyan>intelligence of</color> <color_c_yellow>12</color> to easily read.\n" );
        }

        THEN( "no requirement info is shown if book has none" ) {
            // Book with no requirements
            CHECK( item_info_str( cmdline, skill_min ).empty() );
            CHECK( item_info_str( cmdline, skill_max ).empty() );
            CHECK( item_info_str( cmdline, reqs_int ).empty() );
        }
    }
}

// Related JSON fields:
// "range"
// "ranged_damage"
// - "amount"
// "skill"
// "critical_multiplier" (ammo)
// "magazines"
//
// Functions:
// item::gun_info
TEST_CASE( "gun or other ranged weapon attributes", "[iteminfo][weapon][gun]" )
{
    clear_avatar();

    item compbow( "test_compbow" );
    item glock( "test_glock" );
    item rag( "test_rag" );

    SECTION( "weapon damage including floating-point multiplier" ) {
        // Ranged damage info is displayed on a single line, in three parts:
        //
        // - base ranged damage (GUN_DAMAGE)
        // - floating-point multiplier (GUN_DAMAGE_AMMOPROP)
        // - total damage calculation (GUN_DAMAGE_TOTAL)
        //
        std::vector<iteminfo_parts> damage = { iteminfo_parts::GUN_DAMAGE,
                                               iteminfo_parts::GUN_DAMAGE_AMMOPROP,
                                               iteminfo_parts::GUN_DAMAGE_TOTAL
                                             };

        CHECK( item_info_str( compbow, damage ) ==
               "--\n"
               "<color_c_white>Ranged damage</color>:"
               " <color_c_yellow>18</color>*<color_c_yellow>1.50</color> = <color_c_yellow>27</color>\n" );
    }
    // TODO: Test glock damage with and without ammo (test_glock has -1 damage when unloaded)

    SECTION( "maximum range" ) {
        std::vector<iteminfo_parts> max_range = { iteminfo_parts::GUN_MAX_RANGE };

        CHECK( item_info_str( compbow, max_range ) ==
               "--\n"
               "Maximum range: <color_c_yellow>18</color>\n" );

        CHECK( item_info_str( glock, max_range ) ==
               "--\n"
               "Maximum range: <color_c_yellow>14</color>\n" );
    }

    SECTION( "skill used" ) {
        std::vector<iteminfo_parts> used_skill = { iteminfo_parts::GUN_USEDSKILL };

        CHECK( item_info_str( compbow, used_skill ) ==
               "--\n"
               "Skill used: <color_c_cyan>archery</color>\n" );

        CHECK( item_info_str( glock, used_skill ) ==
               "--\n"
               "Skill used: <color_c_cyan>handguns</color>\n" );
    }

    SECTION( "ammo capacity of weapon" ) {
        std::vector<iteminfo_parts> gun_capacity = { iteminfo_parts::GUN_CAPACITY };

        CHECK( item_info_str( compbow, gun_capacity ) ==
               "--\n"
               "Capacity: <color_c_yellow>1</color> round of arrows\n" );

        // FIXME: Why empty?
        CHECK( item_info_str( glock, gun_capacity ).empty() );
    }

    SECTION( "default ammo when weapon is unloaded" ) {
        std::vector<iteminfo_parts> default_ammo = { iteminfo_parts::GUN_DEFAULT_AMMO };

        CHECK( item_info_str( compbow, default_ammo ) ==
               "--\n"
               "Weapon is <color_c_red>not loaded</color>, so stats below assume the default ammo:"
               " <color_c_light_blue>wooden broadhead arrow</color>\n" );

        CHECK( item_info_str( glock, default_ammo ) ==
               "--\n"
               "Weapon is <color_c_red>not loaded</color>, so stats below assume the default ammo:"
               " <color_c_light_blue>Test 9mm ammo</color>\n" );
    }

    SECTION( "critical multiplier" ) {
        std::vector<iteminfo_parts> crit_multiplier = { iteminfo_parts::AMMO_DAMAGE_CRIT_MULTIPLIER };

        CHECK( item_info_str( compbow, crit_multiplier ) ==
               "--\n"
               "Critical multiplier: <color_c_yellow>10</color>\n" );

        CHECK( item_info_str( glock, crit_multiplier ) ==
               "--\n"
               "Critical multiplier: <color_c_yellow>2</color>\n" );
    }

    SECTION( "recoil" ) {
        std::vector<iteminfo_parts> recoil = { iteminfo_parts::GUN_RECOIL };

        CHECK( item_info_str( compbow, recoil ).empty() );

        CHECK( item_info_str( glock, recoil ) ==
               "--\n"
               "Effective recoil: <color_c_yellow>312</color>\n" );
    }

    SECTION( "gun type and current magazine" ) {
        std::vector<iteminfo_parts> magazine = { iteminfo_parts::GUN_MAGAZINE };

        // When unloaded, the "Magazine:" section should not be shown
        CHECK( item_info_str( glock, magazine ).empty() );

        // TODO: Test guns having "Type:" (not many exist; ex. ar15_retool_300blk)

        // TODO: Test with magazine inserted, ex. "Magazine: Glock 17 17-round magazine"
    }

    SECTION( "gun aiming stats" ) {
        std::vector<iteminfo_parts> aim_stats = { iteminfo_parts::GUN_AIMING_STATS };
        CHECK( item_info_str( glock, aim_stats ) ==
               "--\n"
               "<color_c_white>Base aim speed</color>: <color_c_yellow>104</color>\n"
               "<color_c_cyan>Regular</color>\n"
               "Even chance of good hit at range: <color_c_yellow>3</color>\n"
               "Time to reach aim level: <color_c_yellow>115</color> moves\n"
               "<color_c_cyan>Careful</color>\n"
               "Even chance of good hit at range: <color_c_yellow>4</color>\n"
               "Time to reach aim level: <color_c_yellow>145</color> moves\n"
               "<color_c_cyan>Precise</color>\n"
               "Even chance of good hit at range: <color_c_yellow>6</color>\n"
               "Time to reach aim level: <color_c_yellow>174</color> moves\n" );
    }

    SECTION( "compatible magazines" ) {
        std::vector<iteminfo_parts> allowed_mags = { iteminfo_parts::GUN_ALLOWED_MAGAZINES };

        // expect magazine_compatible().empty() if magazine_integral()

        // Compound bow has integral magazine, not compatible ones
        REQUIRE( compbow.magazine_integral() );
        REQUIRE( compbow.magazine_compatible().empty() );
        // No compatible magazine info should be shown
        CHECK( item_info_str( compbow, allowed_mags ).empty() );

        // Glock has compatible magazines, not integral
        REQUIRE_FALSE( glock.magazine_integral() );
        REQUIRE_FALSE( glock.magazine_compatible().empty() );
        // Should show compatible magazines
        CHECK( item_info_str( glock, allowed_mags ) ==
               "--\n"
               "<color_c_white>Compatible magazines</color>:"
               " Test Glock extended magazine and Test Glock magazine\n" );

        // Rag does not have integral or compatible magazines
        REQUIRE_FALSE( rag.magazine_integral() );
        REQUIRE( rag.magazine_compatible().empty() );
        // No magazine info should be shown
        CHECK( item_info_str( rag, allowed_mags ).empty() );
    }

    SECTION( "armor piercing" ) {
        CHECK( item_info_str( compbow, { iteminfo_parts::GUN_ARMORPIERCE } ) ==
               "--\n"
               "Armor-pierce: <color_c_yellow>0</color>\n" );
    }

    SECTION( "time to reload weapon" ) {
        CHECK( item_info_str( compbow, { iteminfo_parts::GUN_RELOAD_TIME } ) ==
               "--\n"
               "Reload time: <color_c_yellow>110</color> moves\n" );
    }

    SECTION( "weapon firing modes" ) {
        CHECK( item_info_str( compbow, { iteminfo_parts::GUN_FIRE_MODES } ) ==
               "--\n"
               "<color_c_white>Fire modes</color>: manual (1)\n" );
    }

    SECTION( "weapon mods" ) {
        CHECK( item_info_str( compbow, { iteminfo_parts::DESCRIPTION_GUN_MODS } ) ==
               "--\n"
               "<color_c_white>Mods</color>: <color_c_white>0/2</color> accessories;"
               " <color_c_white>0/1</color> dampening; <color_c_white>0/1</color> sights;"
               " <color_c_white>0/1</color> stabilizer; <color_c_white>0/1</color> underbarrel.\n" );
    }

    SECTION( "weapon dispersion" ) {
        CHECK( item_info_str( compbow, { iteminfo_parts::GUN_DISPERSION } ) ==
               "--\n"
               "Dispersion: <color_c_yellow>850</color>\n" );
    }

    SECTION( "needing two hands to fire" ) {
        REQUIRE( compbow.has_flag( flag_FIRE_TWOHAND ) );

        CHECK( item_info_str( compbow, { iteminfo_parts::DESCRIPTION_TWOHANDED } ) ==
               "--\n"
               "This weapon needs <color_c_cyan>two free hands</color> to fire.\n" );
    }
}

// Functions:
// item::gun_info
TEST_CASE( "gun armor piercing, dispersion and other stats", "[iteminfo][gun][misc]" )
{
    clear_avatar();

    std::vector<iteminfo_parts> dmg_loaded = { iteminfo_parts::GUN_DAMAGE_LOADEDAMMO };
    std::vector<iteminfo_parts> ap_loaded = { iteminfo_parts::GUN_ARMORPIERCE_LOADEDAMMO };
    std::vector<iteminfo_parts> ap_total = { iteminfo_parts::GUN_ARMORPIERCE_TOTAL };
    std::vector<iteminfo_parts> disp_loaded = { iteminfo_parts::GUN_DISPERSION_LOADEDAMMO };
    std::vector<iteminfo_parts> disp_total = { iteminfo_parts::GUN_DISPERSION_TOTAL };
    std::vector<iteminfo_parts> disp_sight = { iteminfo_parts::GUN_DISPERSION_SIGHT };

    // TODO: Test these
    //std::vector<iteminfo_parts> recoil_bipod = { iteminfo_parts::GUN_RECOIL_BIPOD };
    //std::vector<iteminfo_parts> ammo_remain = { iteminfo_parts::AMMO_REMAINING };
    //std::vector<iteminfo_parts> ammo_upscost = { iteminfo_parts::AMMO_UPSCOST };
    //std::vector<iteminfo_parts> gun_casings = { iteminfo_parts::DESCRIPTION_GUN_CASINGS };

    item glock( "test_glock" );

    CHECK( item_info_str( glock, dmg_loaded ) ==
           "--\n<color_c_yellow>+26</color>\n" );

    CHECK( item_info_str( glock, ap_loaded ) ==
           "--\n<color_c_yellow>+0</color>\n" );
    CHECK( item_info_str( glock, ap_total ) ==
           "--\n = <color_c_yellow>0</color>\n" );

    CHECK( item_info_str( glock, disp_loaded ) ==
           "--\n<color_c_yellow>+60</color>\n" );
    CHECK( item_info_str( glock, disp_total ) ==
           "--\n = <color_c_yellow>540</color>\n" );

    CHECK( item_info_str( glock, disp_sight ) ==
           "--\n"
           "Sight dispersion: <color_c_yellow>30</color>"
           "<color_c_yellow>+14</color>"
           " = <color_c_yellow>44</color>\n" );

    // TODO: Add a test gun with thest attributes
    //CHECK( item_info_str( glock, recoil_bipod ).empty() );
    //CHECK( item_info_str( glock, ammo_remain ).empty() );
    //CHECK( item_info_str( glock, ammo_upscost ).empty() );
    //CHECK( item_info_str( glock, gun_casings ).empty() );
}

// Related JSON fields:
// "sight_dispersion"
// "aim_speed"
// "handling_modifier"
// "damage_modifier"
// - "amount"
// "flags"
// "mod_targets"
// "location"
//
// Functions:
// item::gunmod_info
TEST_CASE( "gunmod info", "[iteminfo][gunmod]" )
{
    clear_avatar();

    std::vector<iteminfo_parts> reach = { iteminfo_parts::DESCRIPTION_GUNMOD_REACH };
    std::vector<iteminfo_parts> no_sights = { iteminfo_parts::DESCRIPTION_GUNMOD_DISABLESSIGHTS };
    std::vector<iteminfo_parts> consumable = { iteminfo_parts::DESCRIPTION_GUNMOD_CONSUMABLE };
    std::vector<iteminfo_parts> disp_sight = { iteminfo_parts::GUNMOD_DISPERSION_SIGHT };
    std::vector<iteminfo_parts> aim_speed = { iteminfo_parts::GUNMOD_AIMSPEED };
    std::vector<iteminfo_parts> damage = { iteminfo_parts::GUNMOD_DAMAGE };
    std::vector<iteminfo_parts> handling = { iteminfo_parts::GUNMOD_HANDLING };
    std::vector<iteminfo_parts> usedon = { iteminfo_parts::GUNMOD_USEDON };
    std::vector<iteminfo_parts> location = { iteminfo_parts::GUNMOD_LOCATION };

    // TODO for gunmod_info:
    //std::vector<iteminfo_parts> gunmod = { iteminfo_parts::DESCRIPTION_GUNMOD };
    //std::vector<iteminfo_parts> armorpierce = { iteminfo_parts::GUNMOD_ARMORPIERCE };
    //std::vector<iteminfo_parts> ammo = { iteminfo_parts::GUNMOD_AMMO };
    //std::vector<iteminfo_parts> reload = { iteminfo_parts::GUNMOD_RELOAD };
    //std::vector<iteminfo_parts> strength = { iteminfo_parts::GUNMOD_STRENGTH };
    //std::vector<iteminfo_parts> add_mod = { iteminfo_parts::GUNMOD_ADD_MOD };
    //std::vector<iteminfo_parts> blacklist_mod = { iteminfo_parts::GUNMOD_BLACKLIST_MOD };

    item supp( "test_crafted_suppressor" );
    REQUIRE( supp.is_gunmod() );

    /* FIXME: This only applies if is_gun() ??
    CHECK( item_info_str( supp, { iteminfo_parts::DESCRIPTION_GUNMOD } ) ==
           "--\n"
           "* This mod <color_c_red>must be attached to a gun</color>, it can not be fired separately.\n" );
    */

    SECTION( "gunmod flags" ) {
        REQUIRE( supp.has_flag( flag_REACH_ATTACK ) );
        CHECK( item_info_str( supp, reach ) ==
               "--\n"
               "When attached to a gun, <color_c_green>allows</color> making"
               " <color_c_cyan>reach melee attacks</color> with it.\n" );

        REQUIRE( supp.has_flag( flag_DISABLE_SIGHTS ) );
        CHECK( item_info_str( supp, no_sights ) ==
               "--\n"
               "This mod <color_c_red>obscures sights</color> of the base weapon.\n" );

        REQUIRE( supp.has_flag( flag_CONSUMABLE ) );
        CHECK( item_info_str( supp, consumable ) ==
               "--\n"
               "This mod might <color_c_red>suffer wear</color> when firing the base weapon.\n" );
    }

    CHECK( item_info_str( supp, disp_sight ) ==
           "--\n"
           "Sight dispersion: <color_c_yellow>11</color>\n" );

    CHECK( item_info_str( supp, aim_speed ) ==
           "--\n"
           "Aim speed: <color_c_yellow>4</color>\n" );

    CHECK( item_info_str( supp, damage ) ==
           "--\n"
           "Damage: <color_c_yellow>-5</color>\n" );

    CHECK( item_info_str( supp, handling ) ==
           "--\n"
           "Handling modifier: <color_c_yellow>+1</color>\n" );

    // FIXME: This is a different order than given in JSON, and not alphabetical either;
    // could be made more predictable by making enumerate_as_string do sorting
    /*
    CHECK( item_info_str( supp, usedon ) ==
           "--\n"
           "Used on: <color_c_cyan>rifle</color> and <color_c_cyan>pistol</color>\n" );
    */

    CHECK( item_info_str( supp, location ) ==
           "--\n"
           "Location: muzzle\n" );

}

// Functions:
// item::ammo_info
TEST_CASE( "ammunition", "[iteminfo][ammo]" )
{
    clear_avatar();

    std::vector<iteminfo_parts> ammo = { iteminfo_parts::AMMO_REMAINING_OR_TYPES,
                                         iteminfo_parts::AMMO_DAMAGE_VALUE,
                                         iteminfo_parts::AMMO_DAMAGE_PROPORTIONAL,
                                         iteminfo_parts::AMMO_DAMAGE_AP,
                                         iteminfo_parts::AMMO_DAMAGE_RANGE,
                                         iteminfo_parts::AMMO_DAMAGE_DISPERSION,
                                         iteminfo_parts::AMMO_DAMAGE_RECOIL,
                                         iteminfo_parts::AMMO_DAMAGE_CRIT_MULTIPLIER
                                       };

    SECTION( "simple item with ammo damage" ) {
        item rock( "test_rock" );

        CHECK( item_info_str( rock, ammo ) ==
               "--\n"
               "<color_c_white>Ammunition type</color>: rocks\n"
               "Damage: <color_c_yellow>7</color>  Armor-pierce: <color_c_yellow>0</color>\n"
               "Range: <color_c_yellow>10</color>  Dispersion: <color_c_yellow>14</color>\n"
               "Recoil: <color_c_yellow>0</color>  Critical multiplier: <color_c_yellow>2</color>\n" );
    }

    SECTION( "batteries" ) {
        item batt_dispose( "test_battery_disposable" );

        // FIXME: is_battery is only true if type = "BATTERY"
        // no items in the game have this property anymore
        //REQUIRE( batt_dispose.is_battery() );

        REQUIRE( batt_dispose.ammo_data() );
        REQUIRE( batt_dispose.ammo_data()->nname( 1 ) == "battery" );

        // No ammo info should be displayed
        CHECK( item_info_str( batt_dispose, ammo ).empty() );
    }
}

// Functions:
// item::food_info
TEST_CASE( "nutrients in food", "[iteminfo][food]" )
{
    clear_avatar();

    item ice_cream( "icecream" );

    SECTION( "fixed nutrient values in regular item" ) {
        CHECK( item_info_str( ice_cream, { iteminfo_parts::FOOD_NUTRITION, iteminfo_parts::FOOD_QUENCH } )
               ==
               "--\n"
               "<color_c_white>Calories (kcal)</color>: <color_c_yellow>325</color>"
               "  Quench: <color_c_yellow>0</color>\n" );

        CHECK( item_info_str( ice_cream, { iteminfo_parts::FOOD_VITAMINS } ) ==
               "--\n"
               "Vitamins (RDA): Calcium (9%), Vitamin A (9%), and Vitamin B12 (11%)\n" );
    }

    SECTION( "nutrient ranges for recipe exemplars", "[iteminfo]" ) {
        ice_cream.set_var( "recipe_exemplar", "icecream" );

        CHECK( item_info_str( ice_cream, { iteminfo_parts::FOOD_NUTRITION, iteminfo_parts::FOOD_QUENCH } )
               ==
               "--\n"
               "Nutrition will <color_cyan>vary with chosen ingredients</color>.\n"
               "<color_c_white>Calories (kcal)</color>:"
               " <color_c_yellow>126</color>-<color_c_yellow>467</color>"
               "  Quench: <color_c_yellow>0</color>\n" );

        CHECK( item_info_str( ice_cream, { iteminfo_parts::FOOD_VITAMINS } ) ==
               "--\n"
               "Nutrition will <color_cyan>vary with chosen ingredients</color>.\n"
               "Vitamins (RDA): Calcium (7-28%), Iron (0-83%), "
               "Vitamin A (3-11%), Vitamin B12 (2-6%), and Vitamin C (1-85%)\n" );
    }
}

// Related JSON fields:
// "spoils_in"
//
// Functions:
// item::food_info
TEST_CASE( "food freshness and lifetime", "[iteminfo][food]" )
{
    clear_avatar();

    Character &player_character = get_player_character();
    // Ensure test character has no skill estimating spoilage
    player_character.empty_skills();
    REQUIRE_FALSE( player_character.can_estimate_rot() );

    item nuts( "test_pine_nuts" );
    REQUIRE( nuts.goes_bad() );

    // TODO:
    // - flags FREEZERBURN, MUSHY, NO_PARASITES
    // - traits: SAPROVORE, bio_digestion
    // - with can_estimate_rot() == true

    SECTION( "food is fresh" ) {
        REQUIRE( nuts.is_fresh() );
        CHECK( item_info_str( nuts, { iteminfo_parts::FOOD_ROT } ) ==
               "--\n"
               "* This food is <color_c_yellow>perishable</color>, and at room temperature has"
               " an estimated nominal shelf life of <color_c_cyan>6 weeks</color>.\n"
               "* This food looks as <color_c_green>fresh</color> as it can be.\n" );
    }

    SECTION( "food is old" ) {
        nuts.mod_rot( nuts.type->comestible->spoils );
        REQUIRE( nuts.is_going_bad() );
        CHECK( item_info_str( nuts, { iteminfo_parts::FOOD_ROT } ) ==
               "--\n"
               "* This food is <color_c_yellow>perishable</color>, and at room temperature has"
               " an estimated nominal shelf life of <color_c_cyan>6 weeks</color>.\n"
               "* This food looks <color_c_red>old</color>.  It's on the brink of becoming inedible.\n" );
    }
}

// Related JSON fields:
// "fun"
// "charges"
//
// Functions:
// item::food_info
TEST_CASE( "basic food info", "[iteminfo][food]" )
{
    clear_avatar();

    std::vector<iteminfo_parts> joy = { iteminfo_parts::FOOD_JOY };
    std::vector<iteminfo_parts> portions = { iteminfo_parts::FOOD_PORTIONS };
    std::vector<iteminfo_parts> consume_time = { iteminfo_parts::FOOD_CONSUME_TIME };

    // TODO: Test these:
    //std::vector<iteminfo_parts> smell = { iteminfo_parts::FOOD_SMELL };
    //std::vector<iteminfo_parts> vit_effects = { iteminfo_parts::FOOD_VIT_EFFECTS };

    item apple( "test_apple" );
    item nuts( "test_pine_nuts" );
    item wine( "test_wine" );

    REQUIRE( apple.is_food() );
    REQUIRE( nuts.is_food() );
    REQUIRE( wine.is_food() );

    CHECK( item_info_str( apple, joy ) ==
           "--\nEnjoyability: <color_c_yellow>10</color>\n" );
    CHECK( item_info_str( apple, portions ) ==
           "--\nPortions: <color_c_yellow>1</color>\n" );
    CHECK( item_info_str( apple, consume_time ) ==
           "--\nConsume time: 50 seconds\n" );

    CHECK( item_info_str( nuts, joy ) ==
           "--\nEnjoyability: <color_c_yellow>2</color>\n" );
    CHECK( item_info_str( nuts, portions ) ==
           "--\nPortions: <color_c_yellow>4</color>\n" );
    CHECK( item_info_str( nuts, consume_time ) ==
           "--\nConsume time: 12 seconds\n" );

    CHECK( item_info_str( wine, joy ) ==
           "--\nEnjoyability: <color_c_yellow>-5</color>\n" );
    CHECK( item_info_str( wine, portions ) ==
           "--\nPortions: <color_c_yellow>7</color>\n" );
    CHECK( item_info_str( wine, consume_time ) ==
           "--\nConsume time: 2 seconds\n" );
}

// Related JSON fields:
// "material" (determines allergen)
//
// Functions:
// item::food_info
TEST_CASE( "food character is allergic to", "[iteminfo][food][allergy]" )
{
    clear_avatar();
    Character &player_character = get_player_character();

    std::vector<iteminfo_parts> allergen = { iteminfo_parts::FOOD_ALLERGEN };

    GIVEN( "character allergic to fruit" ) {
        player_character.toggle_trait( trait_id( "ANTIFRUIT" ) );
        REQUIRE( player_character.has_trait( trait_id( "ANTIFRUIT" ) ) );

        THEN( "fruit indicates an allergic reaction" ) {
            item apple( "test_apple" );
            REQUIRE( apple.has_flag( flag_ALLERGEN_FRUIT ) );
            CHECK( item_info_str( apple, allergen ) ==
                   "--\n"
                   "* This food will cause an <color_c_red>allergic reaction</color>.\n" );
        }

        THEN( "nuts do not indicate an allergic reaction" ) {
            item nuts( "test_pine_nuts" );
            REQUIRE_FALSE( nuts.has_flag( flag_ALLERGEN_FRUIT ) );
            CHECK( item_info_str( nuts, allergen ).empty() );
        }
    }
}

// Related JSON fields:
// "flags"
//
// Functions:
// item::food_info
TEST_CASE( "food with hidden poison or hallucinogen", "[iteminfo][food][poison][hallu]" )
{
    clear_avatar();

    // Test food with hidden effects
    item almond( "test_bitter_almond" );
    item nutmeg( "test_hallu_nutmeg" );

    // Ensure they are food
    REQUIRE( almond.is_food() );
    REQUIRE( nutmeg.is_food() );

    // Ensure they have the expected flags
    REQUIRE( almond.has_flag( flag_HIDDEN_POISON ) );
    REQUIRE( nutmeg.has_flag( flag_HIDDEN_HALLU ) );

    // Parts flags for display
    std::vector<iteminfo_parts> poison = { iteminfo_parts::FOOD_POISON };
    std::vector<iteminfo_parts> hallu = { iteminfo_parts::FOOD_HALLUCINOGENIC };

    Character &player_character = get_player_character();
    // Seeing hidden poison or hallucinogen depends on character survival skill
    // At low level, no info is shown
    GIVEN( "survival 2" ) {
        player_character.set_skill_level( skill_id( "survival" ), 2 );
        REQUIRE( player_character.get_skill_level( skill_id( "survival" ) ) == 2 );

        THEN( "cannot see hidden poison or hallucinogen" ) {
            CHECK( item_info_str( almond, poison ).empty() );
            CHECK( item_info_str( nutmeg, hallu ).empty() );
        }
    }

    // Hidden poison is visible at survival level 3
    GIVEN( "survival 3" ) {
        player_character.set_skill_level( skill_id( "survival" ), 3 );
        REQUIRE( player_character.get_skill_level( skill_id( "survival" ) ) == 3 );

        THEN( "can see hidden poison" ) {
            CHECK( item_info_str( almond, poison ) ==
                   "--\n"
                   "* On closer inspection, this appears to be"
                   " <color_c_red>poisonous</color>.\n" );
        }

        THEN( "cannot see hidden hallucinogen" ) {
            CHECK( item_info_str( nutmeg, hallu ).empty() );
        }
    }

    // Hidden hallucinogen is not visible until survival level 5
    GIVEN( "survival 4" ) {
        player_character.set_skill_level( skill_id( "survival" ), 4 );
        REQUIRE( player_character.get_skill_level( skill_id( "survival" ) ) == 4 );

        THEN( "still cannot see hidden hallucinogen" ) {
            CHECK( item_info_str( nutmeg, hallu ).empty() );
        }
    }

    GIVEN( "survival 5" ) {
        player_character.set_skill_level( skill_id( "survival" ), 5 );
        REQUIRE( player_character.get_skill_level( skill_id( "survival" ) ) == 5 );

        THEN( "can see hidden hallucinogen" ) {
            CHECK( item_info_str( nutmeg, hallu ) ==
                   "--\n"
                   "* On closer inspection, this appears to be"
                   " <color_c_yellow>hallucinogenic</color>.\n" );
        }
    }
}

// Related JSON fields:
// "material" (human flesh is "hflesh")
//
// Functions:
// item::food_info
TEST_CASE( "food that is made of human flesh", "[iteminfo][food][cannibal]" )
{
    // TODO: Test food that is_tainted(): "This food is *tainted* and will poison you"

    clear_avatar();
    Character &player_character = get_player_character();

    std::vector<iteminfo_parts> cannibal = { iteminfo_parts::FOOD_CANNIBALISM };

    item thumb( "test_thumb" );
    REQUIRE( thumb.has_flag( flag_CANNIBALISM ) );

    GIVEN( "character is not a cannibal" ) {
        REQUIRE_FALSE( player_character.has_trait( trait_id( "CANNIBAL" ) ) );
        THEN( "human flesh is indicated as bad" ) {
            // red highlight
            CHECK( item_info_str( thumb, cannibal ) ==
                   "--\n"
                   "* This food contains <color_c_red>human flesh</color>.\n" );
        }
    }

    GIVEN( "character is a cannibal" ) {
        player_character.toggle_trait( trait_id( "CANNIBAL" ) );
        REQUIRE( player_character.has_trait( trait_id( "CANNIBAL" ) ) );

        THEN( "human flesh is indicated as good" ) {
            // green highlight
            CHECK( item_info_str( thumb, cannibal ) ==
                   "--\n"
                   "* This food contains <color_c_green>human flesh</color>.\n" );
        }
    }
}

// Related JSON fields:
// "flags"
//
// Functions:
// item::final_info
// FIXME: Move conducivity out of final_info
TEST_CASE( "item conductivity", "[iteminfo][conductivity]" )
{
    clear_avatar();

    std::vector<iteminfo_parts> conductivity = { iteminfo_parts::DESCRIPTION_CONDUCTIVITY };

    SECTION( "non-conductive items" ) {
        item plank( "test_2x4" );
        REQUIRE_FALSE( plank.conductive() );
        CHECK( item_info_str( plank, conductivity ) ==
               "--\n"
               "* This item <color_c_green>does not conduct</color> electricity.\n" );

        item axe( "test_fire_ax" );
        REQUIRE_FALSE( axe.conductive() );
        CHECK( item_info_str( axe, conductivity ) ==
               "--\n"
               "* This item <color_c_green>does not conduct</color> electricity.\n" );
    }

    SECTION( "conductive items" ) {
        // Pipe is made of conductive material (steel)
        item pipe( "test_pipe" );
        REQUIRE( pipe.conductive() );
        CHECK( item_info_str( pipe, conductivity ) ==
               "--\n"
               "* This item <color_c_red>conducts</color> electricity.\n" );

        // Halligan bar is made of conductive material (steel)
        item halligan( "test_halligan" );
        REQUIRE( halligan.conductive() );
        CHECK( item_info_str( halligan, conductivity ) ==
               "--\n"
               "* This item <color_c_red>conducts</color> electricity.\n" );

        // Balloon is made of non-conductive rubber, but has CONDUCTIVE flag
        item balloon( "test_balloon" );
        REQUIRE( balloon.conductive() );
        CHECK( item_info_str( balloon, conductivity ) ==
               "--\n"
               "* This item effectively <color_c_red>conducts</color>"
               " electricity, as it has no guard.\n" );
    }
}

// Related JSON fields:
// "qualities"
//
// Functions:
// item::qualities_info
TEST_CASE( "list of item qualities", "[iteminfo][quality]" )
{
    clear_avatar();

    std::vector<iteminfo_parts> qualities = { iteminfo_parts::QUALITIES };

    SECTION( "Halligan bar" ) {
        item halligan( "test_halligan" );
        CHECK( item_info_str( halligan, qualities ) ==
               "--\n"
               "Has level <color_c_cyan>1 digging</color> quality.\n"
               "Has level <color_c_cyan>2 hammering</color> quality.\n"
               "Has level <color_c_cyan>4 prying</color> quality.\n" );
    }

    SECTION( "bottle jack" ) {
        item jack( "test_jack_small" );

        SECTION( "metric units" ) {
            override_option opt_kg( "USE_METRIC_WEIGHTS", "kg" );
            CHECK( item_info_str( jack, qualities ) ==
                   "--\n"
                   "Has level <color_c_cyan>4 jacking</color> quality and is rated at"
                   " <color_c_cyan>2000</color> kg\n" );
        }
        SECTION( "imperial units" ) {
            override_option opt_lbs( "USE_METRIC_WEIGHTS", "lbs" );
            CHECK( item_info_str( jack, qualities ) ==
                   "--\n"
                   "Has level <color_c_cyan>4 jacking</color> quality and is rated at"
                   " <color_c_cyan>4409</color> lbs\n" );
        }
    }

    SECTION( "sonic screwdriver" ) {
        item sonic( "test_sonic_screwdriver" );

        CHECK( item_info_str( sonic, qualities ) ==
               "--\n"
               "Has level <color_c_cyan>30 lockpicking</color> quality.\n"
               "Has level <color_c_cyan>2 prying</color> quality.\n"
               "Has level <color_c_cyan>2 screw driving</color> quality.\n"
               "Has level <color_c_cyan>1 fine screw driving</color> quality.\n"
               "Has level <color_c_cyan>1 bolt turning</color> quality.\n" );
    }
}

// Related JSON fields:
// "flags" (USE_UPS, RECHARGE)
//
// Functions:
// item::tool_info
TEST_CASE( "tool info", "[iteminfo][tool]" )
{
    // TODO: Find a tool using this
    //std::vector<iteminfo_parts> mag_current = { iteminfo_parts::TOOL_MAGAZINE_CURRENT };

    clear_avatar();

    SECTION( "maximum charges" ) {
        std::vector<iteminfo_parts> capacity = { iteminfo_parts::TOOL_CAPACITY };

        item matches( "test_matches" );
        CHECK( item_info_str( matches, capacity ) ==
               "--\n"
               "Maximum <color_c_yellow>20</color> charges of match.\n" );
    }

    SECTION( "tool with charges" ) {
        std::vector<iteminfo_parts> charges = { iteminfo_parts::TOOL_CHARGES };

        item matches( "test_matches" );
        matches.ammo_set( itype_id( "match" ) );
        REQUIRE( matches.ammo_remaining() > 0 );

        CHECK( item_info_str( matches, charges ) ==
               "--\n"
               "<color_c_white>Charges</color>: 20\n" );
    }

    SECTION( "candle with feedback on burnout" ) {
        std::vector<iteminfo_parts> burnout = { iteminfo_parts::TOOL_BURNOUT };

        item candle( "candle" );
        candle.ammo_set( itype_id( "candle_wax" ) );
        REQUIRE( candle.ammo_remaining() > 0 );

        CHECK( item_info_str( candle, burnout ) ==
               "--\n"
               "<color_c_white>Fuel</color>: It's new, and ready to burn.\n" );

        candle.ammo_set( itype_id( "candle_wax" ), ( candle.type->maximum_charges() / 2 ) - 1 );
        CHECK( item_info_str( candle, burnout ) ==
               "--\n"
               "<color_c_white>Fuel</color>: More than half has burned away.\n" );
    }

    SECTION( "UPS charged tool" ) {
        std::vector<iteminfo_parts> recharge_ups = { iteminfo_parts::DESCRIPTION_RECHARGE_UPSMODDED };

        item smartphone( "test_smart_phone" );
        REQUIRE( smartphone.has_flag( flag_USE_UPS ) );

        CHECK( item_info_str( smartphone, recharge_ups ) ==
               "--\n"
               "* This tool has been modified to use a"
               " <color_c_cyan>universal power supply</color> and is"
               " <color_c_yellow>not compatible</color> with"
               " <color_c_cyan>standard batteries</color>.\n" );
    }

    SECTION( "compatible magazines" ) {
        std::vector<iteminfo_parts> magazine_compat = { iteminfo_parts::TOOL_MAGAZINE_COMPATIBLE };

        // Rag has no magazine capacity
        item rag( "test_rag" );
        REQUIRE_FALSE( rag.magazine_integral() );
        REQUIRE( rag.magazine_compatible().empty() );

        CHECK( item_info_str( rag, magazine_compat ).empty() );

        // Acetylene torch is a tool with compatible magazines
        // Other tools with "Compatible magazine": electric hair trimmer, circular saw
        item oxy_torch( "oxy_torch" );
        REQUIRE_FALSE( oxy_torch.magazine_integral() );
        REQUIRE_FALSE( oxy_torch.magazine_compatible().empty() );

        CHECK( item_info_str( oxy_torch, magazine_compat ) ==
               "--\n"
               "Compatible magazines: small welding tank and welding tank\n" );
    }
}

// Related JSON fields:
// "type": "bionic"
// "capacity"
// "occupied_bodyparts"
// "fuel_capacity"
// "env_protec"
//
// Functions:
// item::bionic_info
TEST_CASE( "bionic info", "[iteminfo][bionic]" )
{
    // TODO: Add a test for this
    //std::vector<iteminfo_parts> slots = { iteminfo_parts::DESCRIPTION_CBM_SLOTS };
    // FIXME: bionic_info has NO OTHER iteminfo_parts filters!

    // TODO: Add tests for:
    // - bash protection
    // - cut protection
    // - balliastic protection
    // - stat bonus
    // - weight capacity modifier
    // - weight capacity bonus

    clear_avatar();

    item burner( "bio_ethanol" );
    item power( "bio_power_storage" );
    item nostril( "bio_nostril" );
    item purifier( "bio_purifier" );

    REQUIRE( burner.is_bionic() );
    REQUIRE( power.is_bionic() );
    REQUIRE( nostril.is_bionic() );
    REQUIRE( purifier.is_bionic() );

    CHECK( item_info_str( burner, {} ) ==
           "--\n"
           "* This bionic can produce power from the following fuel:"
           " <color_c_cyan>Alcohol</color>\n" );

    // NOTE: No trailing newline
    CHECK( item_info_str( power, {} ) ==
           "--\n"
           "<color_c_white>Power Capacity</color>:"
           " <color_c_yellow>100000000</color> mJ" );

    // NOTE: Funky trailing space
    CHECK( item_info_str( nostril, {} ) ==
           "--\n"
           "<color_c_white>Encumbrance</color>: "
           "Mouth <color_c_yellow>10</color>" );

    CHECK( item_info_str( purifier, {} ) ==
           "--\n"
           "<color_c_white>Environmental Protection</color>: "
           "Mouth <color_c_yellow>7</color>" );
}

// Functions:
// item::repair_info
TEST_CASE( "repairable and with what tools", "[iteminfo][repair]" )
{
    clear_avatar();

    item halligan( "test_halligan" );
    item hazmat( "test_hazmat_suit" );
    item rock( "test_rock" );

    std::vector<iteminfo_parts> repaired = { iteminfo_parts::DESCRIPTION_REPAIREDWITH };

    // TODO: Move reinforcement to a different part flag ? Repair tools interfere here (especially
    // with Magiclysm, which has enchanted tailor's kit)
    /*
    item socks( "test_socks" );
    CHECK( item_info_str( socks, repaired ) ==
           "--\n"
           "* This item can be <color_c_green>reinforced</color>.\n" );
    */

    CHECK( item_info_str( halligan, repaired ) ==
           "--\n"
           "<color_c_white>Repair</color> using extended toolset, arc welder, or makeshift arc welder.\n" );

    // FIXME: Use an item that can only be repaired by test tools
    CHECK( item_info_str( hazmat, repaired ) ==
           "--\n"
           "<color_c_white>Repair</color> using soldering iron, TEST soldering iron, or extended toolset.\n" );

    CHECK( item_info_str( rock, repaired ) ==
           "--\n"
           "* This item is <color_c_red>not repairable</color>.\n" );
}

// Functions:
// item::disassembly_info
TEST_CASE( "disassembly time and yield", "[iteminfo][disassembly]" )
{
    clear_avatar();

    std::vector<iteminfo_parts> disassemble = { iteminfo_parts::DESCRIPTION_COMPONENTS_DISASSEMBLE };

    item iron( "test_soldering_iron" );
    item metal( "test_sheet_metal" );

    CHECK( item_info_str( iron, disassemble ) ==
           "--\n"
           "<color_c_white>Disassembly</color> takes about 20 minutes, requires 1 tool"
           " with <color_c_cyan>cutting of 1</color> or more and 1 tool with"
           " <color_c_cyan>screw driving of 1</color> or more and <color_c_white>might"
           " yield</color>: 2 electronic scraps, copper (1), scrap metal (1), and copper"
           " wire (5).\n" );

    CHECK( item_info_str( metal, disassemble ) ==
           "--\n"
           "<color_c_white>Disassembly</color> takes about 2 minutes, requires 1 tool"
           " with <color_c_cyan>metal sawing of 2</color> or more and <color_c_white>might"
           " yield</color>: TEST small metal sheet (24).\n" );
}

// Related JSON fields:
// "flags"
//
// The DESCRIPTION_FLAGS part shows descriptions of item "flags" from data/json/flags.json.
//
// Functions:
// item::final_info
// FIXME: Factor out of final_info
TEST_CASE( "item description flags", "[iteminfo][flags]" )
{
    clear_avatar();

    std::vector<iteminfo_parts> flags = { iteminfo_parts::DESCRIPTION_FLAGS };

    item halligan( "test_halligan" );
    item hazmat( "test_hazmat_suit" );

    // Halligan bar has a couple flags
    REQUIRE( halligan.has_flag( flag_BELT_CLIP ) );
    REQUIRE( halligan.has_flag( flag_DURABLE_MELEE ) );
    CHECK( item_info_str( halligan, flags ) ==
           "--\n"
           "* This item can be <color_c_cyan>clipped onto a belt loop</color> of the appropriate size.\n"
           "* As a weapon, this item is <color_c_green>well-made</color> and will"
           " <color_c_cyan>withstand the punishment of combat</color>.\n" );

    // Hazmat suit has a lot of flags
    REQUIRE( hazmat.has_flag( flag_ELECTRIC_IMMUNE ) );
    REQUIRE( hazmat.has_flag( flag_GAS_PROOF ) );
    REQUIRE( hazmat.has_flag( flag_OUTER ) );
    REQUIRE( hazmat.has_flag( flag_RAD_PROOF ) );
    REQUIRE( hazmat.has_flag( flag_RAINPROOF ) );
    REQUIRE( hazmat.has_flag( flag_WATERPROOF ) );
    CHECK( item_info_str( hazmat, flags ) ==
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

// Functions:
// item::final_info
TEST_CASE( "show available recipes with item as an ingredient", "[iteminfo][recipes]" )
{
    clear_avatar();
    avatar &player_character = get_avatar();

    const recipe *purtab = &recipe_id( "pur_tablets" ).obj();
    recipe_subset &known_recipes = const_cast<recipe_subset &>
                                   ( player_character.get_learned_recipes() );
    known_recipes.clear();

    // FIXME: Factor out of final_info
    std::vector<iteminfo_parts> crafting = { iteminfo_parts::DESCRIPTION_APPLICABLE_RECIPES };

    GIVEN( "character has a potassium iodide tablet and no skill" ) {
        player_character.worn.push_back( item( "backpack" ) );
        item &iodine = player_character.i_add( item( "iodine" ) );
        player_character.empty_skills();
        REQUIRE( !player_character.knows_recipe( purtab ) );

        THEN( "nothing is craftable from it" ) {
            CHECK( item_info_str( iodine, crafting ) ==
                   "--\nYou know of nothing you could craft with it.\n" );
        }

        WHEN( "they acquire the needed skills" ) {
            player_character.set_skill_level( purtab->skill_used, purtab->difficulty );
            REQUIRE( player_character.get_skill_level( purtab->skill_used ) == purtab->difficulty );

            THEN( "still nothing is craftable from it" ) {
                CHECK( item_info_str( iodine, crafting ) ==
                       "--\nYou know of nothing you could craft with it.\n" );
            }

            WHEN( "they have no book, but have the recipe memorized" ) {
                player_character.learn_recipe( purtab );
                REQUIRE( player_character.knows_recipe( purtab ) );

                THEN( "they can use potassium iodide tablets to craft it" ) {
                    CHECK( item_info_str( iodine, crafting ) ==
                           "--\n"
                           "You could use it to craft: "
                           "<color_c_dark_gray>water purification tablet</color>\n" );
                }
            }

            WHEN( "they have the recipe in a book, but not memorized" ) {
                item &textbook = player_character.i_add( item( "textbook_chemistry" ) );
                player_character.do_read( textbook );
                REQUIRE( player_character.has_identified( itype_id( "textbook_chemistry" ) ) );
                // update the crafting inventory cache
                player_character.moves++;

                THEN( "they can use potassium iodide tablets to craft it" ) {
                    CHECK( item_info_str( iodine, crafting ) ==
                           "--\n"
                           "You could use it to craft: "
                           "<color_c_dark_gray>antiseptic powder</color>"
                           " and <color_c_dark_gray>water purification tablet</color>\n" );
                }
            }
        }
    }
}

// Pocket tests below relate to te JSON "pocket_data" fields:
//
// - "max_contains_volume"
// - "max_contains_weight"
// - "max_item_length"
// - "max_item_volume"
// - "watertight"
// - "moves"
// - "rigid"

// Functions:
// item_contents::info
// item_pocket::general_info
TEST_CASE( "pocket info for a simple container", "[iteminfo][pocket][container]" )
{
    clear_avatar();

    item test_waterskin( "test_waterskin" );
    std::vector<iteminfo_parts> pockets = { iteminfo_parts::DESCRIPTION_POCKETS };

    override_option opt_vol( "VOLUME_UNITS", "l" );
    override_option opt_weight( "USE_METRIC_WEIGHTS", "kg" );
    override_option opt_dist( "DISTANCE_UNITS", "metric" );

    // Simple containers with only one pocket show a "Total capacity" section
    // with all the pocket's restrictions and other attributes
    CHECK( item_info_str( test_waterskin, pockets ) ==
           "--\n"
           "<color_c_white>Total capacity</color>:\n"
           "Volume: <color_c_yellow>1.50</color> L  Weight: <color_c_yellow>3.00</color> kg\n"
           "Maximum item length: <color_c_yellow>12</color> cm\n"
           "Maximum item volume: <color_c_yellow>0.015 L</color>\n"
           "Base moves to remove item: <color_c_yellow>220</color>\n"
           "This pocket can <color_c_cyan>contain a liquid</color>.\n" );
}

// Functions:
// item_contents::info
// item_pocket::general_info
TEST_CASE( "pocket info units - imperial or metric", "[iteminfo][pocket][units]" )
{
    clear_avatar();

    item test_jug( "test_jug_plastic" );
    std::vector<iteminfo_parts> pockets = { iteminfo_parts::DESCRIPTION_POCKETS };

    GIVEN( "metric units" ) {
        override_option opt_vol( "VOLUME_UNITS", "l" );
        override_option opt_weight( "USE_METRIC_WEIGHTS", "kg" );
        override_option opt_dist( "DISTANCE_UNITS", "metric" );

        THEN( "pocket capacity is shown in liters, kilograms, and millimeters" ) {
            CHECK( item_info_str( test_jug, pockets ) ==
                   "--\n"
                   "<color_c_white>Total capacity</color>:\n"
                   "Volume: <color_c_yellow>3.75</color> L  Weight: <color_c_yellow>5.00</color> kg\n"
                   "Maximum item length: <color_c_yellow>226</color> mm\n"
                   "Maximum item volume: <color_c_yellow>0.015 L</color>\n"
                   "Base moves to remove item: <color_c_yellow>100</color>\n"
                   "This pocket is <color_c_cyan>rigid</color>.\n"
                   "This pocket can <color_c_cyan>contain a liquid</color>.\n" );
        }
    }

    GIVEN( "imperial units" ) {
        override_option opt_vol( "VOLUME_UNITS", "qt" );
        override_option opt_weight( "USE_METRIC_WEIGHTS", "lbs" );
        override_option opt_dist( "DISTANCE_UNITS", "imperial" );

        THEN( "pocket capacity is shown in quarts, pounds, and inches" ) {
            CHECK( item_info_str( test_jug, pockets ) ==
                   "--\n"
                   "<color_c_white>Total capacity</color>:\n"
                   "Volume: <color_c_yellow>3.97</color> qt  Weight: <color_c_yellow>11.02</color> lbs\n"
                   "Maximum item length: <color_c_yellow>8</color> in.\n"
                   "Maximum item volume: <color_c_yellow>0.016 qt</color>\n"
                   "Base moves to remove item: <color_c_yellow>100</color>\n"
                   "This pocket is <color_c_cyan>rigid</color>.\n"
                   "This pocket can <color_c_cyan>contain a liquid</color>.\n" );
        }
    }
}

// Functions:
// item_contents::info
// item_pocket::general_info
TEST_CASE( "pocket info for a multi-pocket item", "[iteminfo][pocket][multiple]" )
{
    clear_avatar();

    item test_belt( "test_tool_belt" );
    std::vector<iteminfo_parts> pockets = { iteminfo_parts::DESCRIPTION_POCKETS };

    override_option opt_vol( "VOLUME_UNITS", "l" );
    override_option opt_weight( "USE_METRIC_WEIGHTS", "kg" );
    override_option opt_dist( "DISTANCE_UNITS", "metric" );

    // When two pockets have the same attributes, they are combined with a heading like:
    //
    //  2 Pockets with capacity:
    //  Volume: ...  Weight: ...
    //
    // The "Total capacity" indicates the sum Volume/Weight capacity of all pockets.
    CHECK( item_info_str( test_belt, pockets ) ==
           "--\n"
           "<color_c_white>Total capacity</color>:\n"
           "Volume: <color_c_yellow>6.00</color> L  Weight: <color_c_yellow>4.80</color> kg\n"
           "--\n"
           "<color_c_white>4 Pockets</color> with capacity:\n"
           "Volume: <color_c_yellow>1.50</color> L  Weight: <color_c_yellow>1.20</color> kg\n"
           "Maximum item length: <color_c_yellow>70</color> cm\n"
           "Minimum item volume: <color_c_yellow>0.050 L</color>\n"
           "Base moves to remove item: <color_c_yellow>50</color>\n"
           "<color_c_white>Restrictions</color>:\n"
           "* Item must clip onto a belt loop\n"
           "* <color_c_white>or</color> Item must fit in a sheath\n" );
}

TEST_CASE( "ammo restriction info", "[iteminfo][ammo_restriction]" )
{
    SECTION( "container pocket with ammo restriction" ) {
        // For non-MAGAZINE pockets with ammo_restriction, pocket info shows what it can hold
        std::vector<iteminfo_parts> pockets = { iteminfo_parts::DESCRIPTION_POCKETS };

        // Quiver is a CONTAINER with ammo_restriction "arrow" or "bolt"
        item quiver( "test_quiver" );
        // Not a magazine, but it should have ammo_types
        REQUIRE_FALSE( quiver.is_magazine() );
        REQUIRE_FALSE( quiver.ammo_types().empty() );

        CHECK( item_info_str( quiver, pockets ) ==
               "--\n"
               "<color_c_white>Total capacity</color>:\n"
               "Holds: <color_c_yellow>20</color> rounds of arrows\n"
               "Holds: <color_c_yellow>20</color> rounds of bolts\n"
               "Base moves to remove item: <color_c_yellow>20</color>\n" );
    }

    SECTION( "magazine pocket with ammo restriction" ) {
        // For MAGAZINE pockets, ammo_restriction is shown in magazine info
        std::vector<iteminfo_parts> mag_cap = { iteminfo_parts::MAGAZINE_CAPACITY };

        // Matches are TOOL with MAGAZINE pocket, and ammo_restriction "match"
        item matches( "test_matches" );
        REQUIRE( matches.is_magazine() );
        REQUIRE_FALSE( matches.ammo_types().empty() );
        // But they have the NO_RELOAD flag, so their capacity should not be displayed
        REQUIRE( matches.has_flag( flag_NO_RELOAD ) );
        CHECK( item_info_str( matches, mag_cap ).empty() );

        // Compound bow is a GUN with integral MAGAZINE pocket, ammo_restriction "arrow"
        item compbow( "test_compbow" );
        REQUIRE( compbow.is_magazine() );
        REQUIRE_FALSE( compbow.ammo_types().empty() );
        // It can be reloaded, so its magazine capacity should be displayed
        REQUIRE_FALSE( compbow.has_flag( flag_NO_RELOAD ) );
        CHECK( item_info_str( compbow, mag_cap ) ==
               "--\n"
               "Capacity: <color_c_yellow>1</color> round of arrows\n" );

    }
}

// Functions:
// vol_to_info from item.cpp
TEST_CASE( "vol_to_info", "[iteminfo][volume]" )
{
    clear_avatar();

    override_option opt_vol( "VOLUME_UNITS", "l" );
    iteminfo vol = vol_to_info( "BASE", "Volume", 3_liter );
    // strings
    CHECK( vol.sType == "BASE" );
    CHECK( vol.sName == "Volume" );
    CHECK( vol.sFmt == "<num> L" );
    CHECK( vol.sValue == "3.00" );
    // doubles
    CHECK( vol.dValue == 3.0 );
    // booleans
    CHECK( vol.bDrawName == true );
    CHECK( vol.bLowerIsBetter == true );
    CHECK( vol.bNewLine == false );
    CHECK( vol.bShowPlus == false );
    CHECK( vol.is_int == false );
    CHECK( vol.three_decimal == false );
}

// Functions:
// weight_to_info from item.cpp
TEST_CASE( "weight_to_info", "[iteminfo][weight]" )
{
    clear_avatar();

    override_option opt( "USE_METRIC_WEIGHTS", "kg" );
    iteminfo wt = weight_to_info( "BASE", "Weight", 3_kilogram );
    // strings
    CHECK( wt.sType == "BASE" );
    CHECK( wt.sName == "Weight" );
    CHECK( wt.sFmt == "<num> kg" );
    CHECK( wt.sValue == "3.00" );
    // doubles
    CHECK( wt.dValue == 3.0 );
    // booleans
    CHECK( wt.bDrawName == true );
    CHECK( wt.bLowerIsBetter == true );
    CHECK( wt.bNewLine == false );
    CHECK( wt.bShowPlus == false );
    CHECK( wt.is_int == false );
    CHECK( wt.three_decimal == false );
}

// Functions:
// item::final_info
TEST_CASE( "final info", "[iteminfo][final]" )
{
    clear_avatar();
    Character &player_character = get_player_character();

    SECTION( "material allergy" ) {
        item socks( "test_socks" );
        REQUIRE( socks.made_of( material_id( "wool" ) ) );

        WHEN( "avatar has a wool allergy" ) {
            player_character.toggle_trait( trait_id( "WOOLALLERGY" ) );
            REQUIRE( player_character.has_trait( trait_id( "WOOLALLERGY" ) ) );

            CHECK( item_info_str( socks, { iteminfo_parts::DESCRIPTION_ALLERGEN } ) ==
                   "--\n"
                   "<color_c_red>Can't wear that, it's made of wool!</color>\n" );
        }
    }

    SECTION( "fermentation and brewing" ) {
        std::vector<iteminfo_parts> brew_duration = { iteminfo_parts::DESCRIPTION_BREWABLE_DURATION };
        std::vector<iteminfo_parts> brew_products = { iteminfo_parts::DESCRIPTION_BREWABLE_PRODUCTS };

        item wine_must( "test_brew_wine" );
        REQUIRE( wine_must.brewing_time() == 12_hours );

        // TODO: DESCRIPTION_ACTIVATABLE_TRANSFORMATION (sourdough?)

        CHECK( item_info_str( wine_must, brew_duration ) ==
               "--\n"
               "* Once set in a vat, this will ferment in around 12 hours.\n" );

        CHECK( item_info_str( wine_must, brew_products ) ==
               "--\n"
               "* Fermenting this will produce <color_c_yellow>test tennis ball wine</color>.\n"
               "* Fermenting this will produce <color_c_yellow>yeast</color>.\n" );
    }

    SECTION( "radioactivity" ) {
        std::vector<iteminfo_parts> radioactive = { iteminfo_parts::DESCRIPTION_RADIOACTIVITY_ALWAYS };

        item carafe( "test_nuclear_carafe" );
        REQUIRE( carafe.has_flag( flag_RADIOACTIVE ) );
        REQUIRE( carafe.has_flag( flag_LEAK_ALWAYS ) );

        CHECK( item_info_str( carafe, radioactive ) ==
               "--\n"
               "* This object is <color_c_yellow>surrounded</color>"
               " by a <color_c_cyan>sickly green glow</color>.\n" );
    }
}

// Each item::xxx_info function has a `debug` argument, though only a few use it. The main
// item::info function sets it based on whether the global game variable `debug_mode` is true.
//
// The item::debug_info function prints debug info for the item if iteminfo_parts::BASE_DEBUG is
// given, and the `debug` argument passed to the function is true. This function used to be part of
// item::basic_info, and as of this writing is now invoked just after that function.
//
// Functions:
// item::debug_info
// FIXME: This fails when run with other tests. May not be worth having a test on...
TEST_CASE( "item debug info", "[iteminfo][debug][!mayfail][.]" )
{
    clear_avatar();

    SECTION( "debug info displayed when debug_mode is true" ) {
        // Lightly aged pine nuts
        item nuts( "test_pine_nuts" );
        calendar::turn += 8_hours;
        // Quick-check a couple expected values for debug info
        REQUIRE( nuts.age() == 8_hours );
        REQUIRE( nuts.charges == 4 );

        // Debug info may be enabled with an iteminfo_parts flag
        std::vector<iteminfo_parts> debug_part = { iteminfo_parts::BASE_DEBUG };
        const iteminfo_query debug_query( debug_part );

        // Invoke item::debug_info directly, so we can pass debug = true
        // (instead of relying on item::info to use the debug_mode global setting)
        std::vector<iteminfo> info_vec;
        nuts.debug_info( info_vec, &debug_query, 1, true );

        // FIXME: "last rot" and "last temp" are expected to be 0, but may have values (ex. 43200)
        // Neex to figure out what processing to do before this check to make them predictable
        CHECK( format_item_info( info_vec, {} ) ==
               "age (hours): <color_c_yellow>8</color>\n"
               "charges: <color_c_yellow>4</color>\n"
               "damage: <color_c_yellow>0</color>\n"
               "active: <color_c_yellow>1</color>\n"
               "burn: <color_c_yellow>0</color>\n"
               "tags: \n" // NOLINT(cata-text-style)
               "age (turns): <color_c_yellow>28800</color>\n"
               "rot (turns): <color_c_yellow>0</color>\n"
               "  max rot (turns): <color_c_yellow>3888000</color>\n"
               "last rot: <color_c_yellow>0</color>\n"
               "last temp: <color_c_yellow>0</color>\n"
               "Temp: <color_c_yellow>0</color>\n"
               "Spec ener: <color_c_yellow>-10</color>\n"
               "Spec heat lq: <color_c_yellow>2200</color>\n"
               "Spec heat sld: <color_c_yellow>2200</color>\n"
               "latent heat: <color_c_yellow>20</color>\n"
               "Freeze point: <color_c_yellow>32</color>\n" );
    }
}
