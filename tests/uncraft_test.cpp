#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "item.h"
#include "item_location.h"
#include "map.h"
#include "map_helpers.h"
#include "player_helpers.h"
#include "recipe_dictionary.h"

// Tests for disassembling items from an "uncraft" recipe.
//
// Covers the Character::complete_disassemble function, including:
//
// - Uncraft recipe "difficulty" affects component recovery success using character skill
// - Damage to the original item reduces the chance of successful component recovery

static const itype_id itype_cotton_patchwork( "cotton_patchwork" );
static const itype_id itype_debug_backpack( "debug_backpack" );
static const itype_id itype_string_6( "string_6" );
static const itype_id itype_test_knotted_string_ball( "test_knotted_string_ball" );
static const itype_id itype_test_multitool( "test_multitool" );
static const itype_id itype_test_rag_bundle( "test_rag_bundle" );

static time_point midday = calendar::turn_zero + 12_hours;

// Prepare for crafting with storage, tools, and light, and return the player character
static Character &setup_uncraft_character()
{
    Character &they = get_player_character();
    clear_avatar();
    clear_map();
    set_time( midday );
    // Backpack for storage, and multi-tool for qualities
    they.worn.wear_item( they, item( itype_debug_backpack ), false, false );
    they.i_add( item( itype_test_multitool ) );
    return they;
}

// Repeatedly disassemble an item of a given type according to the given recipe, and return
// a tally of all of the yielded item types.
static std::map<itype_id, int> repeat_uncraft( Character &they, const itype_id &dis_itype,
        const recipe &dis_recip, int repeat = 1, int damage = 0 )
{
    // Accumulate all items from recipe disassembly
    std::map<itype_id, int> yield_items;

    // Locations for real item and disassembly item
    item_location it_loc;
    item_location it_dis_loc;

    for( int rep = 0; rep < repeat; ++rep ) {
        // Add another instance of the disassembly item
        item_location it = they.i_add( item( dis_itype ) );
        if( damage > 0 ) {
            it->set_damage( damage );
        }
        it_dis_loc = they.create_in_progress_disassembly( it );

        // Clear away bits
        clear_map();
        // Get lit
        set_time( midday );
        // Disassemble it
        they.complete_disassemble( it_dis_loc, dis_recip );

        // Count how many of each type of item are here
        for( item &it : get_map().i_at( they.pos() ) ) {
            if( yield_items.count( it.typeId() ) == 0 ) {
                yield_items[it.typeId()] = it.count();
            } else {
                yield_items[it.typeId()] += it.count();
            }
        }
    }

    return yield_items;
}

// Return the number of part_itype items yielded by character with the given skill level
// when disassembling whole_itype items the given number of times.
static int uncraft_yield( Character &they, const itype_id whole_itype, const itype_id part_itype,
                          const int difficulty, const int skill_level )
{
    recipe uncraft_recipe = recipe_dictionary::get_uncraft( whole_itype );
    uncraft_recipe.difficulty = difficulty;
    they.set_skill_level( uncraft_recipe.skill_used, skill_level );

    std::map<itype_id, int> yield = repeat_uncraft( they, whole_itype, uncraft_recipe, 1 );
    return yield[part_itype];
}


// When a "recipe" or "uncraft" has a "difficulty" and "skill_used", recovery of components when
// disassembling from that recipe/uncraft may fail, depending on the skill level of the character
// doing disassembly.
//
// For each of the recipe or uncraft "components", two dice rolls are made:
//
//           Difficulty roll:  #dice = difficulty  #sides = 24
//      Character skill roll:  #dice = 4*SKILL+2   #sides = INT+16
//
// For example, if recipe difficulty is 1, character skill is 0, and INT is the normal 8, the
// character is at an advantage in the dice rolls:
//
//           Difficulty roll:  1 d 24  (difficulty# dice, 24 sides)
//      Character skill roll:  2 d 24  (4*0+2 dice, 8+16 sides)
//
// For a more difficult recipe, say difficulty 4, with character still having 0 skill, and 8 INT,
// they are at a disadvantage:
//
//           Difficulty roll:  4 d 24  (difficulty# dice, 24 sides)
//      Character skill roll:  2 d 24  (4*0+2 dice, 8+16 sides)
//
// The character can increase their dice by increasing skill level; each level gives 4 additional
// dice to the character, so if they can increase their skill to 1, then attempt the same recipe,
// they will be at a slight advantage again:
//
//           Difficulty roll:  4 d 24  (difficulty# dice, 24 sides)
//      Character skill roll:  6 d 24  (4*1+2 dice, 8+16 sides)
//
// For each component, if the character skill roll *exceeds* the difficulty roll, recovery succeeds.
// Recipes with 0 difficulty do not require any skill to succeed; all components are recovered.
//
// Recovery may still fail if the original item was damaged; for that, see the [damage] test.
//
TEST_CASE( "uncraft difficulty and character skill", "[uncraft][difficulty][skill]" )
{
    Character &they = setup_uncraft_character();

    // The knotted string ball yields 1,000 short strings when successfully desconstructed.
    const itype_id decon_it = itype_test_knotted_string_ball;
    const itype_id part_it = itype_string_6;

    std::map<itype_id, int> yield;

    SECTION( "uncraft recipe with difficulty 0" ) {
        // Full yield at any skill level
        CHECK( uncraft_yield( they, decon_it, part_it, 0, 0 ) == 1000 );
        CHECK( uncraft_yield( they, decon_it, part_it, 0, 1 ) == 1000 );
        CHECK( uncraft_yield( they, decon_it, part_it, 0, 2 ) == 1000 );
        CHECK( uncraft_yield( they, decon_it, part_it, 0, 3 ) == 1000 );
        CHECK( uncraft_yield( they, decon_it, part_it, 0, 4 ) == 1000 );
    }

    SECTION( "uncraft recipe with difficulty 1" ) {
        CHECK( uncraft_yield( they, decon_it, part_it, 1, 0 ) == Approx( 830 ).margin( 50 ) );
        CHECK( uncraft_yield( they, decon_it, part_it, 1, 1 ) == Approx( 1000 ).margin( 50 ) );
        CHECK( uncraft_yield( they, decon_it, part_it, 1, 2 ) == Approx( 1000 ).margin( 50 ) );
        CHECK( uncraft_yield( they, decon_it, part_it, 1, 3 ) == Approx( 1000 ).margin( 50 ) );
        CHECK( uncraft_yield( they, decon_it, part_it, 1, 4 ) == Approx( 1000 ).margin( 50 ) );
    }

    SECTION( "uncraft recipe with difficulty 5" ) {
        CHECK( uncraft_yield( they, decon_it, part_it, 5, 0 ) == Approx( 20 ).margin( 50 ) );
        CHECK( uncraft_yield( they, decon_it, part_it, 5, 1 ) == Approx( 700 ).margin( 50 ) );
        CHECK( uncraft_yield( they, decon_it, part_it, 5, 2 ) == Approx( 990 ).margin( 50 ) );
        CHECK( uncraft_yield( they, decon_it, part_it, 5, 3 ) == Approx( 1000 ).margin( 50 ) );
        CHECK( uncraft_yield( they, decon_it, part_it, 5, 4 ) == Approx( 1000 ).margin( 50 ) );
    }

    SECTION( "uncraft recipe with difficulty 10" ) {
        CHECK( uncraft_yield( they, decon_it, part_it, 10, 0 ) == Approx( 0 ).margin( 50 ) );
        CHECK( uncraft_yield( they, decon_it, part_it, 10, 1 ) == Approx( 30 ).margin( 50 ) );
        CHECK( uncraft_yield( they, decon_it, part_it, 10, 2 ) == Approx( 500 ).margin( 50 ) );
        CHECK( uncraft_yield( they, decon_it, part_it, 10, 3 ) == Approx( 930 ).margin( 50 ) );
        CHECK( uncraft_yield( they, decon_it, part_it, 10, 4 ) == Approx( 1000 ).margin( 50 ) );
    }
}


// Item damage_level (0-4) affects chance of successfully recovering components from disassembly.
// The rate of successful component recovery for each damage level is:
//
// level : success (0.8^level)
//     0 : 100%
//     1 : 80%
//     2 : 64%
//     3 : 51%
//     4 : 41%
//
// Internally, damage is applied in a range from (0 - 4000) as described in src/item.h.
// Roughly, set_damage(1000) yields damage_level() == 1, and 2000 yields level 2, etc.
//
TEST_CASE( "uncraft from a damaged item", "[uncraft][damage]" )
{
    Character &they = setup_uncraft_character();

    // This uncraft has 0 difficulty and cannot fail because of skills
    recipe uncraft_rags = recipe_dictionary::get_uncraft( itype_test_rag_bundle );
    REQUIRE( uncraft_rags.difficulty == 0 );

    // The rag bundle should yield 100 rags and 1 long string (string_36)
    // We will only count the rags, since they are more likely to be affected by damage
    std::map<itype_id, int> yield;

    // 80% chance for each component
    // Chance of recovering all 100 is about 1 in 5 billion
    SECTION( "1 damage_level: 80 percent chance to recover components" ) {
        yield = repeat_uncraft( they, itype_test_rag_bundle, uncraft_rags, 1, 1000 );
        CHECK( yield[itype_cotton_patchwork] == Approx( 80 ).margin( 20 ) );
    }

    // 64% chance for each component
    // Recovering more than 90 is billlions-to-one
    SECTION( "2 damage_level: 64 percent chance to recover components" ) {
        yield = repeat_uncraft( they, itype_test_rag_bundle, uncraft_rags, 1, 2000 );
        CHECK( yield[itype_cotton_patchwork] == Approx( 64 ).margin( 30 ) );
    }

    // 51% chance for each component
    // Recovering more than 80 is billions-to-one
    SECTION( "3 damage_level: 51 percent chance to recover components" ) {
        yield = repeat_uncraft( they, itype_test_rag_bundle, uncraft_rags, 1, 3000 );
        CHECK( yield[itype_cotton_patchwork] == Approx( 51 ).margin( 40 ) );
    }

    // 41% chance for each component
    // Recovering more than 70 is billions-to-one
    SECTION( "4 damage_level: 41 percent chance to recover components" ) {
        yield = repeat_uncraft( they, itype_test_rag_bundle, uncraft_rags, 1, 4000 );
        CHECK( yield[itype_cotton_patchwork] == Approx( 41 ).margin( 40 ) );
    }
}

