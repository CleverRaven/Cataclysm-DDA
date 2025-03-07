#include <map>
#include <string>
#include <vector>

#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "character_attire.h"
#include "coordinates.h"
#include "item.h"
#include "item_location.h"
#include "map.h"
#include "map_helpers.h"
#include "player_helpers.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "type_id.h"

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
        for( item &it : get_map().i_at( they.pos_bub() ) ) {
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
TEST_CASE( "uncraft_difficulty_and_character_skill", "[uncraft][difficulty][skill]" )
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

    constexpr int margin = 55;

    SECTION( "uncraft recipe with difficulty 1" ) {
        CHECK( uncraft_yield( they, decon_it, part_it, 1, 0 ) == Approx( 830 ).margin( margin ) );
        CHECK( uncraft_yield( they, decon_it, part_it, 1, 1 ) == Approx( 1000 ).margin( margin ) );
        CHECK( uncraft_yield( they, decon_it, part_it, 1, 2 ) == Approx( 1000 ).margin( margin ) );
        CHECK( uncraft_yield( they, decon_it, part_it, 1, 3 ) == Approx( 1000 ).margin( margin ) );
        CHECK( uncraft_yield( they, decon_it, part_it, 1, 4 ) == Approx( 1000 ).margin( margin ) );
    }

    SECTION( "uncraft recipe with difficulty 5" ) {
        CHECK( uncraft_yield( they, decon_it, part_it, 5, 0 ) == Approx( 20 ).margin( margin ) );
        CHECK( uncraft_yield( they, decon_it, part_it, 5, 1 ) == Approx( 700 ).margin( margin ) );
        CHECK( uncraft_yield( they, decon_it, part_it, 5, 2 ) == Approx( 990 ).margin( margin ) );
        CHECK( uncraft_yield( they, decon_it, part_it, 5, 3 ) == Approx( 1000 ).margin( margin ) );
        CHECK( uncraft_yield( they, decon_it, part_it, 5, 4 ) == Approx( 1000 ).margin( margin ) );
    }

    SECTION( "uncraft recipe with difficulty 10" ) {
        CHECK( uncraft_yield( they, decon_it, part_it, 10, 0 ) == Approx( 0 ).margin( margin ) );
        CHECK( uncraft_yield( they, decon_it, part_it, 10, 1 ) == Approx( 30 ).margin( margin ) );
        CHECK( uncraft_yield( they, decon_it, part_it, 10, 2 ) == Approx( 500 ).margin( margin ) );
        CHECK( uncraft_yield( they, decon_it, part_it, 10, 3 ) == Approx( 930 ).margin( margin ) );
        CHECK( uncraft_yield( they, decon_it, part_it, 10, 4 ) == Approx( 1000 ).margin( margin ) );
    }
}

// Item damage_level (0-5) affects chance of successfully recovering components from disassembly.
TEST_CASE( "uncraft_from_a_damaged_item", "[uncraft][damage]" )
{
    Character &they = setup_uncraft_character();

    recipe uncraft_rags = recipe_dictionary::get_uncraft( itype_test_rag_bundle );
    REQUIRE( uncraft_rags.difficulty == 0 ); // 0 difficulty makes sure it can't fail because of skills

    struct uncraft_test_preset {
        const int damage;       // value to assign as item's damage
        const int damage_level; // expected item damage level
        const int yield;        // expected approximate yield
        const int margin;       // expected approximate yield margin
    };

    // yield is based on pow( 0.8, damage_level ) roll, e.g. level 2 will get 0.8^2 = ~64% yield
    // uncraft repeated 10 times and the bundle yields 100 each = 1000 yield total for intact item
    // margin value of 55 derived from running 1000 iterations of this, add 5 for rng outliers
    const int margin = 60;
    const std::vector<uncraft_test_preset> presets = {
        {    0, 0, 1000,      0 }, // expect full yield from undamaged
        {  999, 1,  800, margin }, // ~80% from damage level 1
        { 1999, 2,  640, margin }, // ~64% from damage level 2
        { 2999, 3,  512, margin }, // ~51% from damage level 3
        { 3999, 4,  410, margin }, // ~41% from damage level 4
        { 4000, 5,  328, margin }, // ~33% from damage level 5 (XX)
    };

    for( const uncraft_test_preset &preset : presets ) {
        CAPTURE( preset.damage, preset.damage_level, preset.yield, preset.margin );

        item test_item( itype_test_rag_bundle, calendar::turn );
        test_item.set_damage( preset.damage );
        CHECK( test_item.damage_level() == preset.damage_level );

        const int yield = repeat_uncraft( they, itype_test_rag_bundle, uncraft_rags, 10, preset.damage )
                          .at( itype_cotton_patchwork );
        CHECK( yield == Approx( preset.yield ).margin( preset.margin ) );
    }
}

