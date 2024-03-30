#include <iosfwd>
#include <memory>
#include <string>

#include "avatar.h"
#include "cata_catch.h"
#include "creature_tracker.h"
#include "game.h"
#include "magic.h"
#include "map_helpers.h"
#include "monster.h"
#include "pimpl.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"

static const spell_id spell_test_spell_box( "test_spell_box" );
static const spell_id spell_test_spell_tp_ghost( "test_spell_tp_ghost" );
static const spell_id spell_test_spell_tp_mummy( "test_spell_tp_mummy" );

// Magic Spell tests
// -----------------
//
// Each test case relates to some spell feature, in terms of:
//
// - JSON spell content, from data/json/*.json, as documented in doc/MAGIC.md
// - C++ spell functions, defined in src/magic.cpp and src/magic_spell_effect.cpp
//
// To run all tests in this file:
//
//      tests/cata_test [magic][spell]
//
// Other tags used include: [level], [damage], [duration], [range], [aoe]
//
// All these test cases use spells from data/mods/TEST_DATA/magic.json, to have predictable data
// unaffected by in-game balance and mods.

// Spell name
// ----------
//
// Related JSON fields:
// "name"
//
// Functions:
// spell::name
//
TEST_CASE( "spell_name", "[magic][spell][name]" )
{
    // Test spells from data/mods/TEST_DATA/magic.json
    spell_id pew_id( "test_spell_pew" );
    spell_id lava_id( "test_spell_lava" );
    spell_id kiss_id( "test_spell_kiss" );
    spell_id montage_id( "test_spell_montage" );

    spell pew_spell( pew_id );
    spell lava_spell( lava_id );
    spell kiss_spell( kiss_id );
    spell montage_spell( montage_id );

    CHECK( pew_spell.name() == "Pew, Pew" );
    CHECK( lava_spell.name() == "The Floor is Lava" );
    CHECK( kiss_spell.name() == "Kiss the Owie" );
    CHECK( montage_spell.name() == "Sports Training Montage" );
}

// Spell level
// -----------
//
// Related JSON fields:
// "max_level"
//
// Functions:
// spell::get_level
// spell::set_level
// spell::exp_to_next_level

TEST_CASE( "spell_level", "[magic][spell][level]" )
{
    npc guy;
    spell_id pew_id( "test_spell_pew" );

    const spell_type &pew_type = pew_id.obj();
    REQUIRE( pew_type.max_level.min.dbl_val.value() == 10 );

    GIVEN( "spell level 0" ) {
        spell pew_spell( pew_id );
        REQUIRE( pew_spell.get_level() == 0 );

        WHEN( "spell levels up once" ) {
            pew_spell.gain_level( guy );

            THEN( "it is level 1" ) {
                CHECK( pew_spell.get_level() == 1 );
            }

            AND_WHEN( "spell levels up to max_level" ) {
                pew_spell.set_level( guy, pew_type.max_level.min.dbl_val.value() );

                THEN( "it is maximum level" ) {
                    CHECK( pew_spell.get_level() == pew_type.max_level.min.dbl_val.value() );
                }

                AND_THEN( "it cannot level up beyond max_level" ) {
                    pew_spell.set_level( guy, pew_type.max_level.min.dbl_val.value() + 1 );
                    CHECK( pew_spell.get_level() == pew_type.max_level.min.dbl_val.value() );
                }
            }
        }
    }
}

// Return experience points needed to level up a spell, starting at from_level
static int spell_xp_to_next_level( const spell_id &sp_id, const int from_level )
{
    npc guy;
    spell test_spell( sp_id );
    test_spell.set_level( guy, from_level );
    return test_spell.exp_to_next_level();
}

TEST_CASE( "experience_to_gain_spell_levels", "[magic][spell][level][xp]" )
{
    npc guy;
    spell_id pew_id( "test_spell_pew" );
    spell_id lava_id( "test_spell_lava" );
    int level_1_xp = 0;
    int level_2_xp = 0;
    int current_xp = 0;

    spell lava_spell( lava_id );

    GIVEN( "spell is level 0" ) {
        lava_spell.set_level( guy, 0 );
        REQUIRE( lava_spell.get_level() == 0 );

        THEN( "exp_to_next_level returns experience needed to reach level 1" ) {
            level_1_xp = lava_spell.exp_to_next_level();
            CHECK( level_1_xp > 1000 );

            // These sections all make the same assertion in different ways, basically:
            // - Starting from 0, exp_to_next_level is the total XP to reach level 1
            // - As spell experience accumulates, exp_to_next_level diminishes toward 0
            // - Upon reaching level 1, exp_to_next_level returns XP needed to reach level 2

            WHEN( "some experience is gained" ) {
                current_xp = 500;
                lava_spell.set_exp( current_xp );

                THEN( "exp_to_next_level is that much closer to level 1" ) {
                    CHECK( lava_spell.exp_to_next_level() == level_1_xp - current_xp );
                }
            }

            WHEN( "experience is half way to level 1" ) {
                current_xp = level_1_xp / 2;
                lava_spell.set_exp( current_xp );

                THEN( "exp_to_next_level is half way to zero" ) {
                    CHECK( lava_spell.exp_to_next_level() == level_1_xp - current_xp );
                }
            }

            WHEN( "experience is 10 points away from level 1" ) {
                current_xp = level_1_xp - 10;
                lava_spell.set_exp( current_xp );

                THEN( "exp_to_next_level is 10" ) {
                    CHECK( lava_spell.exp_to_next_level() == 10 );
                }
            }

            WHEN( "experience is just enough to reach level 1" ) {
                current_xp = level_1_xp;
                lava_spell.set_exp( current_xp );

                THEN( "spell is level 1" ) {
                    CHECK( lava_spell.get_level() == 1 );
                }

                THEN( "exp_to_next_level returns experience needed to reach level 2" ) {
                    level_2_xp = lava_spell.exp_to_next_level();
                    CHECK( level_2_xp > 1000 );
                }
            }
        }
    }

    SECTION( "experience needed to level up spells" ) {
        SECTION( "same for all spells" ) {
            CHECK( spell_xp_to_next_level( lava_id, 0 ) == spell_xp_to_next_level( pew_id, 0 ) );
            CHECK( spell_xp_to_next_level( lava_id, 1 ) == spell_xp_to_next_level( pew_id, 1 ) );
            CHECK( spell_xp_to_next_level( lava_id, 2 ) == spell_xp_to_next_level( pew_id, 2 ) );
        }
        SECTION( "getting from level 0 to 1 is the initial XP hurdle" ) {
            CHECK( spell_xp_to_next_level( lava_id, 0 ) == 4881 );
        }
        SECTION( "levels 2-8 take progressively more XP, but still less XP than 0-1" ) {
            CHECK( spell_xp_to_next_level( lava_id, 1 ) == 1751 );
            CHECK( spell_xp_to_next_level( lava_id, 2 ) == 2027 );
            CHECK( spell_xp_to_next_level( lava_id, 3 ) == 2347 );
            CHECK( spell_xp_to_next_level( lava_id, 4 ) == 2717 );
            CHECK( spell_xp_to_next_level( lava_id, 5 ) == 3147 );
            CHECK( spell_xp_to_next_level( lava_id, 6 ) == 3644 );
            CHECK( spell_xp_to_next_level( lava_id, 7 ) == 4220 );
        }
        SECTION( "getting from level 8 to 9 takes about as much XP as getting from 0 to 1" ) {
            CHECK( spell_xp_to_next_level( lava_id, 8 ) == 4886 );
        }
        SECTION( "level 9 and above take progressively more XP to level up" ) {
            CHECK( spell_xp_to_next_level( lava_id, 9 ) == 5659 );
            CHECK( spell_xp_to_next_level( lava_id, 10 ) == 6552 );
            CHECK( spell_xp_to_next_level( lava_id, 11 ) == 7587 );
            CHECK( spell_xp_to_next_level( lava_id, 12 ) == 8785 );
            CHECK( spell_xp_to_next_level( lava_id, 13 ) == 10173 );
            CHECK( spell_xp_to_next_level( lava_id, 14 ) == 11780 );
            CHECK( spell_xp_to_next_level( lava_id, 15 ) == 13641 );
            CHECK( spell_xp_to_next_level( lava_id, 16 ) == 15795 );
            CHECK( spell_xp_to_next_level( lava_id, 17 ) == 18291 );
            CHECK( spell_xp_to_next_level( lava_id, 18 ) == 21180 );
            CHECK( spell_xp_to_next_level( lava_id, 19 ) == 24525 );
            CHECK( spell_xp_to_next_level( lava_id, 20 ) == 28400 );
        }
    }
}

// Spell damage
// ------------
//
// Related JSON fields:
// "min_damage"
// "max_damage"
// "damage_increment"
// "max_level"
//
// Functions:
// spell::damage

// Return spell damage at a given level
static int spell_damage( const spell_id &sp_id, const int spell_level )
{
    npc guy;
    spell test_spell( sp_id );
    test_spell.set_level( guy, spell_level );
    return test_spell.damage( guy );
}

TEST_CASE( "spell_damage", "[magic][spell][damage]" )
{
    spell_id pew_id( "test_spell_pew" );
    const spell_type &pew_type = pew_id.obj();

    // Level 0 damage for this spell is 1
    REQUIRE( pew_type.min_damage.min.dbl_val.value() == 1 );
    // and 1 damage is added at each level
    REQUIRE( pew_type.damage_increment.min.dbl_val.value() == 1 );
    // however, maximum damage is 5
    REQUIRE( pew_type.max_damage.min.dbl_val.value() == 5 );
    // so maximum damage will be reached at level 4, when
    //
    // Lv4 damage = 1 + 4 * 1 = 5
    //            min lvl inc max
    // Because this spell has a maximum level of 10
    REQUIRE( pew_type.max_level.min.dbl_val.value() == 10 );
    // damage from level 5-10 remains at 5.

    SECTION( "spell damage varies from min_damage to max_damage as level increases" ) {
        CHECK( spell_damage( pew_id, 0 ) == 1 );
        CHECK( spell_damage( pew_id, 1 ) == 2 );
        CHECK( spell_damage( pew_id, 2 ) == 3 );
        CHECK( spell_damage( pew_id, 3 ) == 4 );
        CHECK( spell_damage( pew_id, 4 ) == 5 );
        // Max damage reached
        CHECK( spell_damage( pew_id, 5 ) == 5 );
        CHECK( spell_damage( pew_id, 9 ) == 5 );
        CHECK( spell_damage( pew_id, 10 ) == 5 );
    }
}

// Spell duration
// --------------
//
// Related JSON fields:
// "min_duration"
// "max_duration"
// "duration_increment"
// "max_level"
//
// Functions:
// spell::duration_string

// Return spell duration at a given level
static std::string spell_duration_string( const spell_id &sp_id, const int spell_level )
{
    npc guy;
    spell test_spell( sp_id );
    test_spell.set_level( guy, spell_level );
    return test_spell.duration_string( guy );
}

TEST_CASE( "spell_duration", "[magic][spell][duration]" )
{
    spell_id lava_id( "test_spell_lava" );
    const spell_type &lava_type = lava_id.obj();

    // Level 0 duration for this spell is 100 seconds
    REQUIRE( lava_type.min_duration.min.dbl_val.value() == 10000 );
    // and 10 seconds are added at each level
    REQUIRE( lava_type.duration_increment.min.dbl_val.value() == 1000 );
    // however, maximum duration is 250 seconds
    REQUIRE( lava_type.max_duration.min.dbl_val.value() == 25000 );
    // maximum duration will be reached at level 15, when
    //
    // Lv15 duration = 100 + 15 * 10 = 250
    //                 min  lvl  inc   max
    // Because this spell has a maximum level of 20
    REQUIRE( lava_type.max_level.min.dbl_val.value() == 20 );
    // duration from level 16-20 remains 250 seconds.

    SECTION( "spell duration varies from min_duration to max_duration as level increases" ) {
        // At level 0, duration is 100 seconds = 1m 40s
        CHECK( spell_duration_string( lava_id, 0 ) == "1 minute and 40 seconds" );
        // Gain 10 seconds per level
        CHECK( spell_duration_string( lava_id, 1 ) == "1 minute and 50 seconds" );
        CHECK( spell_duration_string( lava_id, 2 ) == "2 minutes" );
        // Gain 6 more levels for 60 more seconds
        CHECK( spell_duration_string( lava_id, 8 ) == "3 minutes" );
        // Keep gaining 10 seconds per level
        CHECK( spell_duration_string( lava_id, 9 ) == "3 minutes and 10 seconds" );
        CHECK( spell_duration_string( lava_id, 10 ) == "3 minutes and 20 seconds" );
        CHECK( spell_duration_string( lava_id, 11 ) == "3 minutes and 30 seconds" );
        CHECK( spell_duration_string( lava_id, 12 ) == "3 minutes and 40 seconds" );
        CHECK( spell_duration_string( lava_id, 13 ) == "3 minutes and 50 seconds" );
        CHECK( spell_duration_string( lava_id, 14 ) == "4 minutes" );
        // Maximum duration is 250 seconds = 4m 10s
        CHECK( spell_duration_string( lava_id, 15 ) == "4 minutes and 10 seconds" );
        // At levels beyond, max_duration remains the same
        CHECK( spell_duration_string( lava_id, 16 ) == "4 minutes and 10 seconds" );
        CHECK( spell_duration_string( lava_id, 17 ) == "4 minutes and 10 seconds" );
        CHECK( spell_duration_string( lava_id, 18 ) == "4 minutes and 10 seconds" );
        CHECK( spell_duration_string( lava_id, 19 ) == "4 minutes and 10 seconds" );
        CHECK( spell_duration_string( lava_id, 20 ) == "4 minutes and 10 seconds" );
    }

    // TODO: Random duration
}

// Spells with the PERMANENT flag have behavior that depends on what kind of spell it is
// - If spell has "effect": "spawn_item", the spawned item only has permanent duration at maximum level
// - If spell has "effect": "summon", the summoned monster can have permanent duration at any level
TEST_CASE( "permanent_spell_duration_depends_on_effect_and_level", "[magic][spell][permanent]" )
{
    npc guy;
    GIVEN( "spell with spawn_item effect, nonzero duration, and PERMANENT flag" ) {
        const spell_type &box_type = spell_test_spell_box.obj();
        const spell box_spell( spell_test_spell_box );
        REQUIRE( box_type.effect_name == "spawn_item" );
        REQUIRE( box_type.duration_increment.min.dbl_val.value() > 0 );
        REQUIRE( box_type.min_duration.min.dbl_val.value() > 0 );
        REQUIRE( box_type.max_duration.min.dbl_val.value() > 0 );
        REQUIRE( box_spell.has_flag( spell_flag::PERMANENT ) );
        REQUIRE( box_spell.get_max_level( guy ) > 9 );

        THEN( "spell has increasing duration before reaching max level" ) {
            CHECK( spell_duration_string( spell_test_spell_box, 0 ) == "10 minutes" );
            CHECK( spell_duration_string( spell_test_spell_box, 1 ) == "15 minutes" );
            CHECK( spell_duration_string( spell_test_spell_box, 2 ) == "20 minutes" );
            CHECK( spell_duration_string( spell_test_spell_box, 3 ) == "25 minutes" );
            CHECK( spell_duration_string( spell_test_spell_box, 4 ) == "30 minutes" );
            CHECK( spell_duration_string( spell_test_spell_box, 5 ) == "35 minutes" );
            CHECK( spell_duration_string( spell_test_spell_box, 6 ) == "40 minutes" );
            CHECK( spell_duration_string( spell_test_spell_box, 7 ) == "45 minutes" );
            CHECK( spell_duration_string( spell_test_spell_box, 8 ) == "50 minutes" );
            CHECK( spell_duration_string( spell_test_spell_box, 9 ) == "55 minutes" );
        }

        THEN( "spell is permanent at max level" ) {
            CHECK( spell_duration_string( spell_test_spell_box,
                                          box_spell.get_max_level( guy ) ) == "Permanent" );
        }
    }

    GIVEN( "spell with summon effect, zero duration, and PERMANENT flag" ) {
        const spell_type &mummy_type = spell_test_spell_tp_mummy.obj();
        const spell mummy_spell( spell_test_spell_tp_mummy );
        REQUIRE( mummy_type.effect_name == "summon" );
        REQUIRE( mummy_type.min_duration.min.dbl_val.value() == 0 );
        REQUIRE( mummy_type.max_duration.min.dbl_val.value() == 0 );
        REQUIRE( mummy_spell.has_flag( spell_flag::PERMANENT ) );
        REQUIRE( mummy_spell.get_max_level( guy ) > 0 );

        THEN( "spell has permanent duration at every level" ) {
            CHECK( spell_duration_string( spell_test_spell_tp_mummy, 0 ) == "Permanent" );
            CHECK( spell_duration_string( spell_test_spell_tp_mummy, 1 ) == "Permanent" );
            CHECK( spell_duration_string( spell_test_spell_tp_mummy, 2 ) == "Permanent" );
            CHECK( spell_duration_string( spell_test_spell_tp_mummy,
                                          mummy_spell.get_max_level( guy ) ) == "Permanent" );
        }
    }
}

// Spell range
// -----------
//
// Related JSON fields:
// "min_range"
// "max_range"
// "range_increment"
// "max_level"
//
// Functions:
// spell::range

// Return spell range at a given level
static int spell_range( const spell_id &sp_id, const int spell_level )
{
    npc guy;
    spell test_spell( sp_id );
    test_spell.set_level( guy, spell_level );
    return test_spell.range( guy );
}

TEST_CASE( "spell_range", "[magic][spell][range]" )
{
    spell_id pew_id( "test_spell_pew" );
    const spell_type &pew_type = pew_id.obj();

    // Level 0 range for this spell is 10
    REQUIRE( pew_type.min_range.min.dbl_val.value() == 10 );
    // with 2.0 added at each level
    REQUIRE( pew_type.range_increment.min.dbl_val.value() == 2 );
    // reaching a maximum range of 30
    REQUIRE( pew_type.max_range.min.dbl_val.value() == 30 );
    // maximum range will be reached at level 10, when
    //
    // Lv10 range = 10 + 10 * 2.0 = 30
    //             min  lvl   inc  max
    // This coincides with the maximum level of the spell
    REQUIRE( pew_type.max_level.min.dbl_val.value() == 10 );
    // giving an even spread of ranges across all levels.

    SECTION( "spell range varies from min_range to max_range as level increases" ) {
        CHECK( spell_range( pew_id, 0 ) == 10 );
        CHECK( spell_range( pew_id, 1 ) == 12 );
        CHECK( spell_range( pew_id, 2 ) == 14 );
        CHECK( spell_range( pew_id, 3 ) == 16 );
        CHECK( spell_range( pew_id, 4 ) == 18 );
        CHECK( spell_range( pew_id, 5 ) == 20 );
        CHECK( spell_range( pew_id, 6 ) == 22 );
        CHECK( spell_range( pew_id, 7 ) == 24 );
        CHECK( spell_range( pew_id, 8 ) == 26 );
        CHECK( spell_range( pew_id, 9 ) == 28 );
        CHECK( spell_range( pew_id, 10 ) == 30 );
    }
}

// Spell area of effect
// --------------------
//
// Related JSON fields:
// "min_aoe"
// "max_aoe"
// "aoe_increment"
// "max_level"
//
// Functions:
// spell::aoe

// Return spell AOE at a given level
static int spell_aoe( const spell_id &sp_id, const int spell_level )
{
    npc guy;
    spell test_spell( sp_id );
    test_spell.set_level( guy, spell_level );
    return test_spell.aoe( guy );
}

TEST_CASE( "spell_area_of_effect", "[magic][spell][aoe]" )
{
    spell_id lava_id( "test_spell_lava" );
    const spell_type &lava_type = lava_id.obj();

    // Level 0 AOE for this spell is 4
    REQUIRE( lava_type.min_aoe.min.dbl_val.value() == 4 );
    // with 1.0 added at each level
    REQUIRE( lava_type.aoe_increment.min.dbl_val.value() == 1 );
    // however, maximum AOE is 15
    REQUIRE( lava_type.max_aoe.min.dbl_val.value() == 15 );
    // so maximum AOE will be reached at level 11, when
    //
    // Lv11 aoe = 4 + 11 * 1.0 = 15
    //          min  lvl   inc  max
    // Because this spell has a maximum level of 20
    REQUIRE( lava_type.max_level.min.dbl_val.value() == 20 );
    // AOE from level 12-20 remains 15.

    SECTION( "spell area of effect varies from min_aoe to max_aoe as level increases" ) {
        CHECK( spell_aoe( lava_id, 0 ) == 4 );
        CHECK( spell_aoe( lava_id, 1 ) == 5 );
        CHECK( spell_aoe( lava_id, 2 ) == 6 );
        // One, two, skip a few
        CHECK( spell_aoe( lava_id, 10 ) == 14 );
        CHECK( spell_aoe( lava_id, 11 ) == 15 );
        CHECK( spell_aoe( lava_id, 12 ) == 15 );
        // 99, 100
        CHECK( spell_aoe( lava_id, 19 ) == 15 );
        CHECK( spell_aoe( lava_id, 20 ) == 15 );
    }
}

// Spell effects
// -------------
//
// Related JSON fields/values:
// "effect"
// "effect_str"
// "base_energy_cost"
// "energy_source"
//
// Functions:
// spell::cast_spell_effect
//
// TODO:
// spell_effect::spawn_item
// spell_effect::projectile_attack
// spell_effect::cone_attack

// spell_effect::target_attack
TEST_CASE( "spell_effect_-_target_attack", "[magic][spell][effect][target_attack]" )
{
    // World setup
    clear_map();

    // Locations for avatar and monster
    const tripoint dummy_loc = { 60, 60, 0 };
    const tripoint mummy_loc = { 62, 60, 0 };

    // For tracking spell damage
    int before_hp = 0;
    int after_hp = 0;

    creature_tracker &creatures = get_creature_tracker();
    // Avatar/spellcaster
    avatar &dummy = get_avatar();
    clear_character( dummy );
    dummy.setpos( dummy_loc );
    REQUIRE( dummy.pos() == dummy_loc );
    REQUIRE( creatures.creature_at( dummy_loc ) );
    REQUIRE( g->num_creatures() == 1 );

    // Monster/defender
    monster &mummy = spawn_test_monster( "mon_zombie", mummy_loc );
    REQUIRE( mummy.pos() == mummy_loc );
    REQUIRE( creatures.creature_at( mummy_loc ) );
    REQUIRE( g->num_creatures() == 2 );

    // Spell with ranged target_attack effect
    spell_id pew_id( "test_spell_pew" );

    // Ensure the spell has the needed attributes
    const spell_type &pew_type = pew_id.obj();
    REQUIRE( pew_type.effect_name == "attack" );
    REQUIRE( pew_type.min_damage.min.dbl_val.value() > 0 );
    REQUIRE( pew_type.min_range.min.dbl_val.value() >= 2 );

    // The spell itself
    spell pew_spell( pew_id );
    pew_spell.set_level( dummy, 5 );
    REQUIRE( pew_spell.damage( dummy ) > 0 );
    REQUIRE( pew_spell.range( dummy ) >= 2 );

    // Ensure avatar has enough mana to cast
    REQUIRE( dummy.magic->has_enough_energy( dummy, pew_spell ) );

    // Cast the spell and measure the defender's change in HP
    before_hp = mummy.get_hp();
    pew_spell.cast_spell_effect( dummy, mummy_loc );
    after_hp = mummy.get_hp();

    // Should do approximately the expected damage
    CHECK( before_hp - pew_spell.damage( dummy ) == Approx( after_hp ).margin( 1 ) );
}

// spell_effect::spawn_summoned_monster
TEST_CASE( "spell_effect_-_summon", "[magic][spell][effect][summon]" )
{
    clear_map();

    // Avatar/spellcaster and summoned mummy locations
    const tripoint dummy_loc = { 60, 60, 0 };
    const tripoint mummy_loc = { 61, 60, 0 };

    avatar &dummy = get_avatar();
    creature_tracker &creatures = get_creature_tracker();
    clear_character( dummy );
    dummy.setpos( dummy_loc );
    REQUIRE( dummy.pos() == dummy_loc );
    REQUIRE( creatures.creature_at( dummy_loc ) );
    REQUIRE( g->num_creatures() == 1 );

    spell ghost_spell( spell_test_spell_tp_ghost );
    REQUIRE( dummy.magic->has_enough_energy( dummy, ghost_spell ) );

    // Summon the ghost in the adjacent space
    ghost_spell.cast_spell_effect( dummy, mummy_loc );

    CHECK( creatures.creature_at( mummy_loc ) );
    CHECK( g->num_creatures() == 2 );

    //kill the ghost
    creatures.creature_at( mummy_loc )->die( nullptr );
    g->cleanup_dead();

    //a corpse was not created
    CHECK( get_map().i_at( mummy_loc ).empty() );

    spell_id mummy_id( "test_spell_tp_mummy" );

    spell mummy_spell( mummy_id );
    REQUIRE( dummy.magic->has_enough_energy( dummy, mummy_spell ) );

    // Summon the mummy in the adjacent space
    mummy_spell.cast_spell_effect( dummy, mummy_loc );

    CHECK( creatures.creature_at( mummy_loc ) );
    CHECK( g->num_creatures() == 2 );

    //kill the mummy
    creatures.creature_at( mummy_loc )->die( nullptr );
    g->cleanup_dead();

    //a corpse was created
    CHECK( !get_map().i_at( mummy_loc ).empty() );

}

// spell_effect::recover_energy
TEST_CASE( "spell_effect_-_recover_energy", "[magic][spell][effect][recover_energy]" )
{
    // Takes recovery amount from sp.damage
    // Takes energy source from sp.effect_data

    // For these effects, positive "damage" is good (adding to internal reservoirs):
    // MANA: p.magic.mod_mana
    // STAMINA: p.mod_stamina
    // HEALTH: p.mod_livestyle (hidden health stat)
    // BIONIC: p.mod_power_level (positive) OR p.mod_stamina (negative)
    //
    // For these effects, negative "damage" is good (reducing the amount of a bad thing)
    // sleepiness: p.mod_sleepiness
    // PAIN: p.mod_pain_resist or p_mod_pain

    // NOTE: This spell effect cannot be used for healing HP.
    // For that, "target_attack" with a negative damage is used.

    // Yer a wizard, ya dummy
    avatar &dummy = get_avatar();
    clear_character( dummy );
    clear_map();

    SECTION( "recover stamina" ) {
        spell_id montage_id( "test_spell_montage" );
        const spell_type &montage_type = montage_id.obj();

        // This spell recovers stamina
        REQUIRE( montage_type.effect_name == "recover_energy" );
        REQUIRE( montage_type.effect_str == "STAMINA" );
        // at the cost of a substantial amount of mana
        REQUIRE( montage_type.base_energy_cost.min.dbl_val.value() == 800 );
        REQUIRE( montage_type.energy_source == magic_energy_type::mana );

        // At level 0, recovers 1000 stamina (10% of maximum)
        REQUIRE( montage_type.min_damage.min.dbl_val.value() == 1000 );
        // and at level 10, recovers 10000 stamina (all of it)
        REQUIRE( montage_type.max_damage.min.dbl_val.value() == 10000 );

        // Ensure avatar needs some stamina
        int start_stamina = dummy.get_stamina_max() / 2;
        dummy.set_stamina( start_stamina );
        REQUIRE( dummy.get_stamina() == start_stamina );

        // Cast montage spell on avatar
        spell montage_spell( montage_id );
        montage_spell.cast_spell_effect( dummy, dummy.pos() );

        // Get stamina back equal to min_damage (at level 0)
        CHECK( dummy.get_stamina() == start_stamina + montage_type.min_damage.min.dbl_val.value() );
    }

    SECTION( "reduce pain" ) {
        spell_id kiss_id( "test_spell_kiss" );
        const spell_type &kiss_type = kiss_id.obj();

        REQUIRE( kiss_type.effect_name == "recover_energy" );
        REQUIRE( kiss_type.effect_str == "PAIN" );
        // Positive "damage" for pain gives relief from pain
        REQUIRE( kiss_type.min_damage.min.dbl_val.value() == 1 );
        REQUIRE( kiss_type.max_damage.min.dbl_val.value() == 10 );
        REQUIRE( kiss_type.damage_increment.min.dbl_val.value() == 1 );

        spell kiss_spell( kiss_id );

        dummy.set_pain( 5 );
        REQUIRE( dummy.get_pain() == 5 );

        kiss_spell.cast_spell_effect( dummy, dummy.pos() );
        CHECK( dummy.get_pain() == 4 );

        kiss_spell.cast_spell_effect( dummy, dummy.pos() );
        CHECK( dummy.get_pain() == 3 );

        kiss_spell.cast_spell_effect( dummy, dummy.pos() );
        CHECK( dummy.get_pain() == 2 );
    }
}

