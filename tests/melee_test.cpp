#include <cstddef>
#include <sstream>
#include <string>

#include "calendar.h"
#include "cata_catch.h"
#include "creature.h"
#include "game_constants.h"
#include "item.h"
#include "map_helpers.h"
#include "monattack.h"
#include "monster.h"
#include "mtype.h"
#include "npc.h"
#include "point.h"
#include "skill.h"
#include "type_id.h"

static const damage_type_id damage_test_fire( "test_fire" );

static const efftype_id effect_sleep( "sleep" );

static const move_mode_id move_mode_prone( "prone" );

static const mtype_id mon_manhack( "mon_manhack" );
static const mtype_id mon_zombie( "mon_zombie" );
static const mtype_id mon_zombie_hulk( "mon_zombie_hulk" );
static const mtype_id pseudo_debug_mon( "pseudo_debug_mon" );

static const skill_id skill_melee( "melee" );

static float brute_probability( Creature &attacker, Creature &target, const size_t iters )
{
    // Note: not using deal_melee_attack because it trains dodge, which causes problems here
    size_t hits = 0;
    for( size_t i = 0; i < iters; i++ ) {
        const int spread = attacker.hit_roll() - target.dodge_roll();
        if( spread > 0 ) {
            hits++;
        }
    }

    return static_cast<float>( hits ) / iters;
}

static float brute_special_probability( monster &attacker, Creature &target, const size_t iters )
{
    size_t hits = 0;
    for( size_t i = 0; i < iters; i++ ) {
        if( !target.dodge_check( &attacker ) ) {
            hits++;
        }
    }

    return static_cast<float>( hits ) / iters;
}

static std::string full_attack_details( const Character &dude )
{
    std::stringstream ss;
    ss << "Details for " << dude.disp_name() << std::endl;
    ss << "get_hit() == " << dude.get_hit() << std::endl;
    ss << "get_melee_hit_base() == " << dude.get_melee_hit_base() << std::endl;
    ss << "get_hit_weapon() == " << dude.get_hit_weapon( *dude.get_wielded_item() ) << std::endl;
    return ss.str();
}

static std::string percent_string( const float f )
{
    // Using stringstream for prettier precision printing
    std::stringstream ss;
    ss << 100.0f * f << "%";
    return ss.str();
}

static void check_near( float prob, const float expected, const float tolerance )
{
    const float low = expected - tolerance;
    const float high = expected + tolerance;
    THEN( "The chance to hit is between " + percent_string( low ) +
          " and " + percent_string( high ) ) {
        REQUIRE( prob > low );
        REQUIRE( prob < high );
    }
}

static const int num_iters = 10000;

static constexpr tripoint dude_pos( HALF_MAPSIZE_X, HALF_MAPSIZE_Y, 0 );

TEST_CASE( "Character_attacking_a_zombie", "[.melee]" )
{
    monster zed( mon_zombie );
    INFO( "Zombie has get_dodge() == " + std::to_string( zed.get_dodge() ) );

    SECTION( "8/8/8/8, no skills, unarmed" ) {
        standard_npc dude( "TestCharacter", dude_pos, {}, 0, 8, 8, 8, 8 );
        const float prob = brute_probability( dude, zed, num_iters );
        INFO( full_attack_details( dude ) );
        check_near( prob, 0.6f, 0.1f );
    }

    SECTION( "8/8/8/8, 3 all skills, plank" ) {
        standard_npc dude( "TestCharacter", dude_pos, {}, 3, 8, 8, 8, 8 );
        dude.set_wielded_item( item( "2x4" ) );
        const float prob = brute_probability( dude, zed, num_iters );
        INFO( full_attack_details( dude ) );
        check_near( prob, 0.8f, 0.05f );
    }

    SECTION( "10/10/10/10, 8 all skills, katana" ) {
        standard_npc dude( "TestCharacter", dude_pos, {}, 8, 10, 10, 10, 10 );
        dude.set_wielded_item( item( "katana" ) );
        const float prob = brute_probability( dude, zed, num_iters );
        INFO( full_attack_details( dude ) );
        check_near( prob, 0.975f, 0.025f );
    }
}

TEST_CASE( "Character_attacking_a_manhack", "[.melee]" )
{
    monster manhack( mon_manhack );
    INFO( "Manhack has get_dodge() == " + std::to_string( manhack.get_dodge() ) );

    SECTION( "8/8/8/8, no skills, unarmed" ) {
        standard_npc dude( "TestCharacter", dude_pos, {}, 0, 8, 8, 8, 8 );
        const float prob = brute_probability( dude, manhack, num_iters );
        INFO( full_attack_details( dude ) );
        check_near( prob, 0.2f, 0.05f );
    }

    SECTION( "8/8/8/8, 3 all skills, plank" ) {
        standard_npc dude( "TestCharacter", dude_pos, {}, 3, 8, 8, 8, 8 );
        dude.set_wielded_item( item( "2x4" ) );
        const float prob = brute_probability( dude, manhack, num_iters );
        INFO( full_attack_details( dude ) );
        check_near( prob, 0.4f, 0.05f );
    }

    SECTION( "10/10/10/10, 8 all skills, katana" ) {
        standard_npc dude( "TestCharacter", dude_pos, {}, 8, 10, 10, 10, 10 );
        dude.set_wielded_item( item( "katana" ) );
        const float prob = brute_probability( dude, manhack, num_iters );
        INFO( full_attack_details( dude ) );
        check_near( prob, 0.7f, 0.05f );
    }
}

TEST_CASE( "Zombie_attacking_a_character", "[.melee]" )
{
    monster zed( mon_zombie );
    INFO( "Zombie has get_hit() == " + std::to_string( zed.get_hit() ) );

    SECTION( "8/8/8/8, no skills, unencumbered" ) {
        standard_npc dude( "TestCharacter", dude_pos, {}, 0, 8, 8, 8, 8 );
        const float prob = brute_probability( zed, dude, num_iters );
        INFO( "Has get_dodge() == " + std::to_string( dude.get_dodge() ) );
        THEN( "Character has no significant dodge bonus or penalty" ) {
            REQUIRE( dude.get_dodge_bonus() < 0.5f );
            REQUIRE( dude.get_dodge_bonus() > -0.5f );
        }

        THEN( "Character's dodge skill is roughly equal to zombie's attack skill" ) {
            REQUIRE( dude.get_dodge() < zed.get_hit() + 0.5f );
            REQUIRE( dude.get_dodge() > zed.get_hit() - 0.5f );
        }

        check_near( prob, 0.5f, 0.05f );
    }

    SECTION( "10/10/10/10, 3 all skills, good cotton armor" ) {
        standard_npc dude( "TestCharacter", dude_pos,
        { "hoodie", "jeans", "long_underpants", "long_undertop", "longshirt" },
        3, 10, 10, 10, 10 );
        const float prob = brute_probability( zed, dude, num_iters );
        INFO( "Has get_dodge() == " + std::to_string( dude.get_dodge() ) );
        check_near( prob, 0.2f, 0.05f );
    }

    SECTION( "10/10/10/10, 8 all skills, survivor suit" ) {
        standard_npc dude( "TestCharacter", dude_pos, { "survivor_suit" }, 8, 10, 10, 10, 10 );
        const float prob = brute_probability( zed, dude, num_iters );
        INFO( "Has get_dodge() == " + std::to_string( dude.get_dodge() ) );
        check_near( prob, 0.025f, 0.0125f );
    }
}

TEST_CASE( "Manhack_attacking_a_character", "[.melee]" )
{
    monster manhack( mon_manhack );
    INFO( "Manhack has get_hit() == " + std::to_string( manhack.get_hit() ) );

    SECTION( "8/8/8/8, no skills, unencumbered" ) {
        standard_npc dude( "TestCharacter", dude_pos, {}, 0, 8, 8, 8, 8 );
        const float prob = brute_probability( manhack, dude, num_iters );
        INFO( "Has get_dodge() == " + std::to_string( dude.get_dodge() ) );
        THEN( "Character has no significant dodge bonus or penalty" ) {
            REQUIRE( dude.get_dodge_bonus() < 0.5f );
            REQUIRE( dude.get_dodge_bonus() > -0.5f );
        }

        check_near( prob, 0.9f, 0.05f );
    }

    SECTION( "10/10/10/10, 3 all skills, good cotton armor" ) {
        standard_npc dude( "TestCharacter", dude_pos,
        { "hoodie", "jeans", "long_underpants", "long_undertop", "longshirt" },
        3, 10, 10, 10, 10 );
        const float prob = brute_probability( manhack, dude, num_iters );
        INFO( "Has get_dodge() == " + std::to_string( dude.get_dodge() ) );
        check_near( prob, 0.6f, 0.05f );
    }

    SECTION( "10/10/10/10, 8 all skills, survivor suit" ) {
        standard_npc dude( "TestCharacter", dude_pos, { "survivor_suit" }, 8, 10, 10, 10, 10 );
        const float prob = brute_probability( manhack, dude, num_iters );
        INFO( "Has get_dodge() == " + std::to_string( dude.get_dodge() ) );
        check_near( prob, 0.25f, 0.05f );
    }
}

TEST_CASE( "Hulk_smashing_a_character", "[.], [melee], [monattack]" )
{
    monster zed( mon_zombie_hulk );
    INFO( "Hulk has get_hit() == " + std::to_string( zed.get_hit() ) );

    SECTION( "8/8/8/8, no skills, unencumbered" ) {
        standard_npc dude( "TestCharacter", dude_pos, {}, 0, 8, 8, 8, 8 );
        const float prob = brute_special_probability( zed, dude, num_iters );
        INFO( "Has get_dodge() == " + std::to_string( dude.get_dodge() ) );
        THEN( "Character has no significant dodge bonus or penalty" ) {
            REQUIRE( dude.get_dodge_bonus() < 0.5f );
            REQUIRE( dude.get_dodge_bonus() > -0.5f );
        }

        check_near( prob, 0.95f, 0.05f );
    }

    SECTION( "10/10/10/10, 3 all skills, good cotton armor" ) {
        standard_npc dude( "TestCharacter", dude_pos,
        { "hoodie", "jeans", "long_underpants", "long_undertop", "longshirt" },
        3, 10, 10, 10, 10 );
        const float prob = brute_special_probability( zed, dude, num_iters );
        INFO( "Has get_dodge() == " + std::to_string( dude.get_dodge() ) );
        check_near( prob, 0.75f, 0.05f );
    }

    SECTION( "10/10/10/10, 8 all skills, survivor suit" ) {
        standard_npc dude( "TestCharacter", dude_pos, { "survivor_suit" }, 8, 10, 10, 10, 10 );
        const float prob = brute_special_probability( zed, dude, num_iters );
        INFO( "Has get_dodge() == " + std::to_string( dude.get_dodge() ) );
        check_near( prob, 0.2f, 0.05f );
    }
}

TEST_CASE( "Charcter_can_dodge" )
{
    standard_npc dude( "TestCharacter", dude_pos, {}, 0, 8, 8, 8, 8 );
    monster zed( mon_zombie );

    dude.clear_effects();
    REQUIRE( dude.get_dodge() > 0.0 );

    const int dodges_left = dude.get_dodges_left();
    for( int i = 0; i < 10000; ++i ) {
        dude.deal_melee_attack( &zed, 1 );
        if( dodges_left < dude.get_dodges_left() ) {
            CHECK( dodges_left < dude.get_dodges_left() );
            break;
        }
    }
}

TEST_CASE( "Incapacited_character_can_not_dodge" )
{
    standard_npc dude( "TestCharacter", dude_pos, {}, 0, 8, 8, 8, 8 );
    monster zed( mon_zombie );

    dude.clear_effects();
    dude.add_effect( effect_sleep, 1_hours );
    REQUIRE( dude.get_dodge() == 0.0 );

    const int dodges_left = dude.get_dodges_left();
    for( int i = 0; i < 10000; ++i ) {
        dude.deal_melee_attack( &zed, 1 );
        CHECK( dodges_left == dude.get_dodges_left() );
    }
}

TEST_CASE( "Melee_skill_training_caps", "[melee], [melee_training_cap], [skill]" )
{
    standard_npc dude( "TestCharacter", dude_pos, {} );
    monster dummy_1( pseudo_debug_mon );
    monster zed( mon_zombie );
    SkillLevel &level = dude.get_skill_level_object( skill_melee );
    REQUIRE( level.knowledgeLevel() == 4 );
    REQUIRE( level.knowledgeExperience( true ) == 0 );

    SECTION( "Monster's melee training cap is calculated correctly" ) {
        CHECK( dummy_1.get_melee() == 0 );
        CHECK( dummy_1.type->melee_training_cap == 2 );
        CHECK( zed.get_melee() == 4 );
        CHECK( zed.type->melee_training_cap == 6 );
    }

    SECTION( "Attacking a monster when above its traing cap will not cause further skill gain" ) {
        dude.melee_attack_abstract( dummy_1, false, matec_id( "" ) );
        CHECK( level.knowledgeLevel() == 4 );
        CHECK( level.knowledgeExperience( true ) == 0 );
    }
    SECTION( "Attacking a monster when below the traing cap will train the skill up to the cap" ) {
        dude.melee_attack_abstract( zed, false, matec_id( "" ) );
        CHECK( level.knowledgeLevel() == 4 );
        CHECK( level.knowledgeExperience( true ) > 0 );
        // Training stops if we get above the cap
        dude.set_skill_level( skill_melee, 7 );
        int prev_xp = level.knowledgeExperience( true );
        dude.melee_attack_abstract( zed, false, matec_id( "" ) );
        CHECK( level.knowledgeExperience( true ) == prev_xp );
    }
}

static void check_damage_from_test_fire( const std::string &mon_id, int expected_resist,
        bool is_immune, float expected_avg_dmg )
{
    int total_dmg = 0;
    int total_hits = 0;
    int set_on_fire = 0;
    for( int i = 0; i < 1000; i++ ) {
        clear_creatures();
        standard_npc dude( "TestCharacter", dude_pos, {}, 8, 10, 10, 10, 10 );
        monster &mon = spawn_test_monster( mon_id, dude.pos() + tripoint_east );
        REQUIRE( mon.pos() == dude.pos() + tripoint_east );
        REQUIRE( mon.get_armor_type( damage_test_fire, body_part_bp_null ) == expected_resist );
        REQUIRE( mon.is_immune_damage( damage_test_fire ) == is_immune );
        REQUIRE( mon.get_hp() == mon.get_hp_max() );
        REQUIRE( dude.get_value( "npctalk_var_general_dmg_type_test_test_fire" ).empty() );
        REQUIRE( mon.get_value( "npctalk_var_general_dmg_type_test_test_fire" ).empty() );
        dude.set_wielded_item( item( "test_fire_sword" ) );
        dude.melee_attack( mon, false );
        if( mon.get_hp() < mon.get_hp_max() ) {
            total_hits++;
            total_dmg += mon.get_hp_max() - mon.get_hp();
            if( mon.get_value( "npctalk_var_general_dmg_type_test_test_fire" ) == "target" ) {
                REQUIRE( dude.get_value( "npctalk_var_general_dmg_type_test_test_fire" ) == "source" );
                set_on_fire++;
            }
        }
    }
    Messages::clear_messages();
    CHECK( total_hits == Approx( 1000 ).margin( 50 ) );
    CHECK( set_on_fire == total_hits );
    CHECK( total_dmg / static_cast<float>( total_hits ) == Approx( expected_avg_dmg ).epsilon( 0.05 ) );
}

static void check_eocs_from_test_fire( const std::string &mon_id )
{
    int eoc_total_dmg = 0;
    clear_creatures();
    standard_npc dude( "TestCharacter", dude_pos, {}, 8, 10, 10, 10, 10 );
    monster &mon = spawn_test_monster( mon_id, dude.pos() + tripoint_east );
    REQUIRE( mon.pos() == dude.pos() + tripoint_east );
    REQUIRE( mon.get_hp() == mon.get_hp_max() );
    REQUIRE( dude.get_value( "npctalk_var_general_dmg_type_test_test_fire" ).empty() );
    REQUIRE( mon.get_value( "npctalk_var_general_dmg_type_test_test_fire" ).empty() );
    item firesword( "test_fire_sword" );
    dude.set_wielded_item( firesword );
    for( int i = 0; i < 1000; ++i ) {
        if( dude.melee_attack( mon, false ) && !dude.get_value( "npctalk_var_test_bp" ).empty() ) {
            break;
        }
    }
    CHECK( !dude.get_value( "npctalk_var_test_bp" ).empty() );
    if( mon.get_value( "npctalk_var_general_dmg_type_test_test_fire" ) == "target" ) {
        REQUIRE( dude.get_value( "npctalk_var_general_dmg_type_test_test_fire" ) == "source" );
    }

    eoc_total_dmg = std::round( std::stod(
                                    dude.get_value( "npctalk_var_test_damage_taken" ) ) );

    Messages::clear_messages();
    CHECK( eoc_total_dmg == firesword.damage_melee( damage_test_fire ) );
}

static void check_damage_from_test_fire( const std::vector<std::string> &armor_items,
        const bodypart_id &checked_bp, int expected_resist, bool is_immune, float expected_avg_dmg )
{
    int total_dmg = 0;
    int total_hits = 0;
    int set_on_fire = 0;
    for( int i = 0; i < 1000; i++ ) {
        clear_creatures();
        standard_npc dude( "TestCharacter", dude_pos, {}, 8, 10, 10, 10, 10 );
        standard_npc dude2( "TestCharacter2", dude_pos + tripoint_east, {}, 0, 0, 0, 0, 0 );
        for( const std::string &itm : armor_items ) {
            REQUIRE( dude2.wear_item( item( itm ), false ).has_value() );
        }
        dude2.set_movement_mode( move_mode_prone ); // no dodging allowed :)
        REQUIRE( dude2.pos() == dude.pos() + tripoint_east );
        REQUIRE( dude2.get_armor_type( damage_test_fire, checked_bp ) == expected_resist );
        REQUIRE( dude2.is_immune_damage( damage_test_fire ) == is_immune );
        REQUIRE( dude2.get_hp() == dude2.get_hp_max() );
        REQUIRE( dude.get_value( "npctalk_var_general_dmg_type_test_test_fire" ).empty() );
        REQUIRE( dude2.get_value( "npctalk_var_general_dmg_type_test_test_fire" ).empty() );
        dude.set_wielded_item( item( "test_fire_sword" ) );
        dude.melee_attack( dude2, false );
        if( dude2.get_hp() < dude2.get_hp_max() ) {
            total_hits++;
            total_dmg += dude2.get_hp_max() - dude2.get_hp();
            if( dude2.get_value( "npctalk_var_general_dmg_type_test_test_fire" ) == "target" ) {
                REQUIRE( dude.get_value( "npctalk_var_general_dmg_type_test_test_fire" ) == "source" );
                set_on_fire++;
            }
        }
    }
    Messages::clear_messages();
    CHECK( total_hits == Approx( 1000 ).margin( 100 ) );
    CHECK( set_on_fire == total_hits );
    const float avg_dmg = total_dmg / static_cast<float>( total_hits );
    CHECK( avg_dmg == Approx( expected_avg_dmg ).epsilon( 0.075 ) );
}

TEST_CASE( "Damage_type_effectiveness_vs_monster_resistance", "[melee][damage][eoc]" )
{
    clear_map();

    SECTION( "Attacking a monster with no resistance to test_fire" ) {
        check_damage_from_test_fire( "mon_test_zombie", 0, false, 16.f );
    }

    SECTION( "Attacking a monster with some resistance to test_fire" ) {
        check_damage_from_test_fire( "mon_test_fire_resist", 5, false, 11.f );
    }

    SECTION( "Attacking a monster that is very resistant to test_fire" ) {
        check_damage_from_test_fire( "mon_test_fire_vresist", 50, false, 8.f );
    }

    SECTION( "Attacking a monster that is immune to test_fire" ) {
        check_damage_from_test_fire( "mon_test_fire_immune", 0, true, 8.f );
    }

    SECTION( "Attacking an NPC with no resistance to test_fire" ) {
        check_damage_from_test_fire( std::vector<std::string> { "test_zentai" },
                                     body_part_torso, 0, false, 14.84f );
    }

    SECTION( "Attacking an NPC that is resistant to test_fire" ) {
        check_damage_from_test_fire( std::vector<std::string> { "test_zentai_resist_test_fire" },
                                     body_part_torso, 2, false, 11.5f );
    }

    SECTION( "Attacking an NPC that is immune to test_fire" ) {
        check_damage_from_test_fire( std::vector<std::string> { "test_zentai_immune_test_fire" },
                                     body_part_torso, 0, true, 6.87f );
    }
}

TEST_CASE( "Damage_type_EOCs", "[damage][eoc]" )
{
    clear_map();

    SECTION( "Attacking a monster" ) {
        check_eocs_from_test_fire( "mon_test_zombie_only_fire" );
    }

}
