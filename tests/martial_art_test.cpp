#include "catch/catch.hpp"
#include "player_helpers.h"

#include "avatar.h"
#include "character_martial_arts.h"
#include "martialarts.h"
#include "map_helpers.h"
#include "mtype.h"
#include "npc.h"

static const efftype_id effect_downed( "downed" );
static const efftype_id effect_grabbed( "grabbed" );
static const efftype_id effect_grabbing( "grabbing" );
static const efftype_id effect_stunned( "stunned" );

static const itype_id itype_club_wooden( "club_wooden" );
static const itype_id itype_sword_crude( "sword_crude" );
static const itype_id itype_test_weapon1( "test_weapon1" );
static const itype_id itype_test_weapon2( "test_weapon2" );

static const matec_id test_tech_condition_knockback( "test_tech_condition_knockback" );
static const matec_id test_tech_condition_stun( "test_tech_condition_stun" );
static const matec_id test_tech_condition_sweep( "test_tech_condition_sweep" );
static const matec_id test_technique( "test_technique" );
static const matype_id test_style_ma1( "test_style_ma1" );

static const species_id species_SLIME( "SLIME" );
static const species_id species_ZOMBIE( "ZOMBIE" );

static constexpr tripoint dude_pos( HALF_MAPSIZE_X, HALF_MAPSIZE_Y, 0 );

TEST_CASE( "martial_arts", "[martial_arts]" )
{
    SECTION( "martial art valid weapon" ) {
        GIVEN( "a weapon that fits the martial art" ) {
            CHECK( test_style_ma1->has_weapon( itype_test_weapon1 ) );
        }

        GIVEN( "a weapon that fits the martial art weapon category" ) {
            CHECK( test_style_ma1->has_weapon( itype_test_weapon2 ) );
        }
    }
}

TEST_CASE( "Martial_art_required_weapon_categories", "[martial_arts]" )
{
    SECTION( "Weapon categories required for buff" ) {
        REQUIRE( !test_style_ma1->onmiss_buffs.empty() );
        const mabuff_id &buff1 = test_style_ma1->onmiss_buffs[0];
        const mabuff_id &buff2 = test_style_ma1->onmiss_buffs[1];

        REQUIRE( buff1->reqs.weapon_categories_allowed[0] == *test_style_ma1->weapon_category.begin() );
        standard_npc dude( "TestCharacter", dude_pos, {}, 0, 8, 8, 8, 8 );
        item weap2( itype_test_weapon2 );
        clear_character( dude, true );
        CHECK( !buff1->is_valid_character( dude ) );
        CHECK( !buff2->is_valid_character( dude ) );
        dude.wield( weap2 );
        dude.martial_arts_data->add_martialart( test_style_ma1 );
        dude.martial_arts_data->set_style( test_style_ma1, false );
        CHECK( buff1->is_valid_character( dude ) );
        CHECK( !buff2->is_valid_character( dude ) );
    }

    SECTION( "Weapon categories required for technique" ) {
        REQUIRE( !test_style_ma1->techniques.empty() );
        const matec_id &tec = *test_style_ma1->techniques.find( test_technique );

        REQUIRE( tec->reqs.weapon_categories_allowed[0] == *test_style_ma1->weapon_category.begin() );
        standard_npc dude( "TestCharacter", dude_pos, {}, 0, 8, 8, 8, 8 );
        item weap2( itype_test_weapon2 );
        clear_character( dude, true );
        CHECK( !tec->is_valid_character( dude ) );
        dude.wield( weap2 );
        dude.martial_arts_data->add_martialart( test_style_ma1 );
        dude.martial_arts_data->set_style( test_style_ma1, false );
        CHECK( tec->is_valid_character( dude ) );
    }
}

TEST_CASE( "Martial_art_technique_conditionals", "[martial_arts]" )
{
    clear_map();
    standard_npc dude( "TestCharacter", dude_pos, {}, 0, 8, 8, 8, 8 );
    const tripoint target_1_pos = dude_pos + tripoint_east;
    const tripoint target_2_pos = dude_pos + tripoint_north;
    const tripoint target_3_pos = dude_pos + tripoint_west;
    clear_character( dude, true );
    dude.martial_arts_data->add_martialart( test_style_ma1 );
    dude.martial_arts_data->set_style( test_style_ma1, false );
    REQUIRE( dude.get_size() == 3 );
    SECTION( "Test sweep" ) {
        const matec_id &tec = *test_style_ma1->techniques.find( test_tech_condition_sweep );
        monster &target_1 = spawn_test_monster( "mon_zombie_fat", target_1_pos );
        monster &target_2 = spawn_test_monster( "mon_zombie_hulk", target_2_pos );
        monster &target_3 = spawn_test_monster( "mon_blob", target_3_pos );
        std::vector<matec_id> tech_1 = dude.evaluate_techniques( target_1, dude.used_weapon() );

        // test sweeping a zombie (succeed)
        REQUIRE( target_1.get_size() == 3 );
        CHECK( tech_1.size() == 1 );
        CHECK( std::find( tech_1.begin(), tech_1.end(), tec ) != tech_1.end() );
        // Being downed disables the attack
        target_1.add_effect( effect_downed, 1_days );
        CHECK( dude.evaluate_techniques( target_1, dude.used_weapon() ).empty() );
        // test sweeping a big zomb (fail)
        REQUIRE( target_2.get_size() == 5 );
        CHECK( dude.evaluate_techniques( target_2, dude.used_weapon() ).empty() );
        // test sweeping a slime (fail)
        REQUIRE( target_3.get_size() == 3 );
        REQUIRE( target_3.type->bodytype == "blob" );
        CHECK( dude.evaluate_techniques( target_3, dude.used_weapon() ).empty() );
    }

    SECTION( "Test stun" ) {
        const matec_id &tec = *test_style_ma1->techniques.find( test_tech_condition_stun );
        monster &target_1 = spawn_test_monster( "mon_feral_human_pipe", target_1_pos );
        monster &target_2 = spawn_test_monster( "mon_zombie_fat", target_2_pos );
        monster &target_3 = spawn_test_monster( "mon_blob", target_3_pos );
        item weap( itype_sword_crude );
        dude.wield( weap );
        // test stunning a feral (succeed)
        std::vector<matec_id> tech_1 = dude.evaluate_techniques( target_1, dude.used_weapon() );
        REQUIRE( target_1.get_size() == 3 );
        REQUIRE( !target_1.type->in_species( species_ZOMBIE ) );
        CHECK( std::find( tech_1.begin(), tech_1.end(), tec ) != tech_1.end() );
        // Being downed disables the attack
        target_1.add_effect( effect_stunned, 1_days );
        CHECK( dude.evaluate_techniques( target_1, dude.used_weapon() ).empty() );
        // test stunning a zombie (fail)
        REQUIRE( target_2.get_size() == 3 );
        REQUIRE( target_2.type->in_species( species_ZOMBIE ) );
        CHECK( dude.evaluate_techniques( target_2, dude.used_weapon() ).empty() );
        // test stunning a slime (fail)
        REQUIRE( target_3.get_size() == 3 );
        REQUIRE( target_3.type->in_species( species_SLIME ) );
        CHECK( dude.evaluate_techniques( target_3, dude.used_weapon() ).empty() );
    }
    SECTION( "Test knockback" ) {
        const matec_id &tec = *test_style_ma1->techniques.find( test_tech_condition_knockback );
        monster &target_1 = spawn_test_monster( "mon_feral_human_pipe", target_1_pos );
        monster &target_2 = spawn_test_monster( "mon_zombie_hulk", target_2_pos );
        monster &target_3 = spawn_test_monster( "mon_test_tech_grabber", target_3_pos );
        item weap( itype_club_wooden );
        dude.wield( weap );
        // test throwing a feral (succeed)
        std::vector<matec_id> tech_1 = dude.evaluate_techniques( target_1, dude.used_weapon() );
        REQUIRE( target_1.get_size() == 3 );
        CHECK( std::find( tech_1.begin(), tech_1.end(), tec ) != tech_1.end() );
        // Being downed disables the attack
        target_1.add_effect( effect_downed, 1_days );
        CHECK( dude.evaluate_techniques( target_1, dude.used_weapon() ).empty() );
        // test throwing a large target (fail)
        REQUIRE( target_2.get_size() == 5 );
        CHECK( dude.evaluate_techniques( target_2, dude.used_weapon() ).empty() );
        // test throwing a monster grabbing you (succeed)
        dude.add_effect( effect_grabbed, 1_days );
        target_3.add_effect( effect_grabbing, 1_days );
        REQUIRE( target_3.get_size() == 3 );
        REQUIRE( target_3.get_grab_strength() == 0 );
        CHECK( dude.evaluate_techniques( target_3, dude.used_weapon() ).size() == 1 );
    }
}
