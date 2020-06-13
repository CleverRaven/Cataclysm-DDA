#include <cstdlib>
#include <utility>

#include "avatar.h"
#include "calendar.h"
#include "creature.h"
#include "game.h"
#include "item.h"
#include "melee.h"
#include "player.h"
#include "type_id.h"

#include "catch/catch.hpp"
#include "player_helpers.h"

// Run a large number of trials of a player attacking a monster with a given weapon,
// and return the average damage done per second.
static double weapon_dps_trials( avatar &attacker, monster &defender, item &weapon )
{
    constexpr int trials = 1000;

    int total_damage = 0;
    int total_moves = 0;

    clear_character( attacker );
    REQUIRE( attacker.can_wield( weapon ).success() );

    melee::clear_stats();
    melee_statistic_data melee_stats = melee::get_stats();
    // rerun the trials in groups of 1000 until 100 crits occur
    for( int i = 0; i < 10 && melee_stats.actual_crit_count < 100;
         i++, melee_stats = melee::get_stats() ) {
        for( int j = 0; j < trials; j++ ) {
            // Reset and re-wield weapon before each attack to prevent skill-up during trials
            clear_character( attacker );
            attacker.wield( weapon );
            // Verify that wielding worked (and not e.g. using martial arts
            // instead)
            REQUIRE( attacker.used_weapon().type == weapon.type );

            int before_moves = attacker.get_moves();

            // Keep the defender at maximum health
            const int starting_hp = defender.get_hp_max();
            defender.set_hp( starting_hp );

            // Attack once
            attacker.melee_attack( defender, false );

            // Tally total damage and moves
            total_damage += std::max( 0, starting_hp - defender.get_hp() );
            total_moves += std::abs( attacker.get_moves() - before_moves );
        }
    }

    // Scale damage-per-move to damage-per-second
    // 1 second == 100 moves
    return 100.0f * total_damage / total_moves;
}

// Compare actual DPS with estimated effective DPS for an attacker/defender/weapon combo.
static void check_actual_dps( avatar &attacker, monster &defender, item &weapon )
{
    clear_character( attacker );
    double expect_dps = weapon.effective_dps( attacker, defender );
    double actual_dps = weapon_dps_trials( attacker, defender, weapon );
    CHECK( actual_dps == Approx( expect_dps ).epsilon( 0.2f ) );
}

static void check_accuracy_dps( avatar &attacker, monster &defender, item &wpn1, item &wpn2,
                                item &wpn3 )
{
    clear_character( attacker );
    melee::clear_stats();
    double dps_wpn1 = weapon_dps_trials( attacker, defender, wpn1 );
    const melee_statistic_data wpn1_stats = melee::get_stats();
    melee::clear_stats();
    double dps_wpn2 = weapon_dps_trials( attacker, defender, wpn2 );
    const melee_statistic_data wpn2_stats = melee::get_stats();
    melee::clear_stats();
    double dps_wpn3 = weapon_dps_trials( attacker, defender, wpn3 );
    const melee_statistic_data wpn3_stats = melee::get_stats();
    REQUIRE( wpn1_stats.hit_count > 0 );
    REQUIRE( wpn2_stats.hit_count > 0 );
    REQUIRE( wpn3_stats.hit_count > 0 );
    CAPTURE( wpn1_stats.attack_count );
    CAPTURE( wpn2_stats.attack_count );
    CAPTURE( wpn3_stats.attack_count );
    double wpn1_hit_rate = static_cast<double>( wpn1_stats.hit_count ) / wpn1_stats.attack_count;
    double wpn2_hit_rate = static_cast<double>( wpn2_stats.hit_count ) / wpn2_stats.attack_count;
    double wpn3_hit_rate = static_cast<double>( wpn3_stats.hit_count ) / wpn3_stats.attack_count;
    CHECK( wpn2_hit_rate > wpn1_hit_rate );
    CHECK( wpn3_hit_rate > wpn2_hit_rate );
    CHECK( dps_wpn2 > dps_wpn1 );
    CHECK( dps_wpn3 > dps_wpn2 );
}
TEST_CASE( "effective damage per second", "[effective][dps]" )
{
    avatar &dummy = g->u;
    clear_character( dummy );

    item clumsy_sword( "test_clumsy_sword" );
    item normal_sword( "test_normal_sword" );
    item good_sword( "test_balanced_sword" );

    SECTION( "against a debug monster with no armor or dodge" ) {
        monster mummy( mtype_id( "debug_mon" ) );

        CHECK( clumsy_sword.effective_dps( dummy, mummy ) == Approx( 25.0f ).epsilon( 0.15f ) );
        CHECK( good_sword.effective_dps( dummy, mummy ) == Approx( 38.0f ).epsilon( 0.15f ) );
    }

    SECTION( "against an agile target" ) {
        monster smoker( mtype_id( "mon_zombie_smoker" ) );
        REQUIRE( smoker.get_dodge() >= 4 );

        CHECK( clumsy_sword.effective_dps( dummy, smoker ) == Approx( 11.0f ).epsilon( 0.15f ) );
        CHECK( good_sword.effective_dps( dummy, smoker ) == Approx( 25.0f ).epsilon( 0.15f ) );
    }

    SECTION( "against an armored target" ) {
        monster soldier( mtype_id( "mon_zombie_soldier" ) );

        CHECK( clumsy_sword.effective_dps( dummy, soldier ) == Approx( 8.0f ).epsilon( 0.15f ) );
        CHECK( good_sword.effective_dps( dummy, soldier ) == Approx( 15.0f ).epsilon( 0.15f ) );
    }

    SECTION( "effect of STR and DEX on damage per second" ) {
        monster mummy( mtype_id( "debug_mon" ) );

        SECTION( "STR 6, DEX 6" ) {
            dummy.str_max = 6;
            dummy.dex_max = 6;

            CHECK( clumsy_sword.effective_dps( dummy, mummy ) == Approx( 20.0f ).epsilon( 0.15f ) );
            CHECK( normal_sword.effective_dps( dummy, mummy ) == Approx( 26.0f ).epsilon( 0.15f ) );
            CHECK( good_sword.effective_dps( dummy, mummy ) == Approx( 33.0f ).epsilon( 0.15f ) );
        }

        SECTION( "STR 8, DEX 10" ) {
            dummy.str_max = 8;
            dummy.dex_max = 10;

            CHECK( clumsy_sword.effective_dps( dummy, mummy ) == Approx( 25.0f ).epsilon( 0.15f ) );
            CHECK( normal_sword.effective_dps( dummy, mummy ) == Approx( 32.0f ).epsilon( 0.15f ) );
            CHECK( good_sword.effective_dps( dummy, mummy ) == Approx( 38.0f ).epsilon( 0.15f ) );
        }

        SECTION( "STR 10, DEX 10" ) {
            dummy.str_max = 10;
            dummy.dex_max = 10;

            CHECK( clumsy_sword.effective_dps( dummy, mummy ) == Approx( 27.0f ).epsilon( 0.15f ) );
            CHECK( normal_sword.effective_dps( dummy, mummy ) == Approx( 34.0f ).epsilon( 0.15f ) );
            CHECK( good_sword.effective_dps( dummy, mummy ) == Approx( 41.0f ).epsilon( 0.15f ) );
        }
    }
}

TEST_CASE( "effective vs actual damage per second", "[actual][dps][!mayfail]" )
{
    avatar &dummy = g->u;
    clear_character( dummy );

    monster soldier( mtype_id( "mon_zombie_soldier" ) );
    monster smoker( mtype_id( "mon_zombie_smoker" ) );
    monster survivor( mtype_id( "mon_zombie_survivor" ) );

    item clumsy_sword( "test_clumsy_sword" );
    item normal_sword( "test_normal_sword" );
    item good_sword( "test_balanced_sword" );

    SECTION( "soldier zombie" ) {
        check_actual_dps( dummy, soldier, clumsy_sword );
        check_actual_dps( dummy, soldier, normal_sword );
        check_actual_dps( dummy, soldier, good_sword );
    }

    SECTION( "smoker zombie" ) {
        check_actual_dps( dummy, smoker, clumsy_sword );
        check_actual_dps( dummy, smoker, normal_sword );
        check_actual_dps( dummy, smoker, good_sword );
    }

    SECTION( "survivor zombie" ) {
        check_actual_dps( dummy, survivor, clumsy_sword );
        check_actual_dps( dummy, survivor, normal_sword );
        check_actual_dps( dummy, survivor, good_sword );
    }
}

TEST_CASE( "accuracy increases success", "[accuracy][dps]" )
{
    avatar &dummy = g->u;
    clear_character( dummy );

    monster soldier( mtype_id( "mon_zombie_soldier" ) );
    monster smoker( mtype_id( "mon_zombie_smoker" ) );
    monster survivor( mtype_id( "mon_zombie_survivor" ) );

    item clumsy_sword( "test_clumsy_sword" );
    item normal_sword( "test_normal_sword" );
    item good_sword( "test_balanced_sword" );

    SECTION( "soldier zombie" ) {
        check_accuracy_dps( dummy, soldier, clumsy_sword, normal_sword, good_sword );
    }

    SECTION( "smoker zombie" ) {
        check_accuracy_dps( dummy, smoker, clumsy_sword, normal_sword, good_sword );
    }

    SECTION( "survivor zombie" ) {
        check_accuracy_dps( dummy, survivor, clumsy_sword, normal_sword, good_sword );
    }
}

static void make_experienced_tester( avatar &test_guy )
{
    clear_character( test_guy );
    test_guy.str_max = 10;
    test_guy.dex_max = 10;
    test_guy.int_max = 10;
    test_guy.per_max = 10;
    test_guy.set_str_bonus( 0 );
    test_guy.set_dex_bonus( 0 );
    test_guy.set_int_bonus( 0 );
    test_guy.set_per_bonus( 0 );
    test_guy.reset_bonuses();
    test_guy.set_speed_base( 100 );
    test_guy.set_speed_bonus( 0 );
    test_guy.hp_cur.fill( test_guy.get_hp_max() );
    test_guy.set_skill_level( skill_id( "bashing" ), 4 );
    test_guy.set_skill_level( skill_id( "cutting" ), 4 );
    test_guy.set_skill_level( skill_id( "stabbing" ), 4 );
    test_guy.set_skill_level( skill_id( "unarmed" ), 4 );
    test_guy.set_skill_level( skill_id( "melee" ), 4 );

    REQUIRE( test_guy.get_str() == 10 );
    REQUIRE( test_guy.get_dex() == 10 );
    REQUIRE( test_guy.get_per() == 10 );
    REQUIRE( test_guy.get_skill_level( skill_id( "bashing" ) ) == 4 );
    REQUIRE( test_guy.get_skill_level( skill_id( "cutting" ) ) == 4 );
    REQUIRE( test_guy.get_skill_level( skill_id( "stabbing" ) ) == 4 );
    REQUIRE( test_guy.get_skill_level( skill_id( "unarmed" ) ) == 4 );
    REQUIRE( test_guy.get_skill_level( skill_id( "melee" ) ) == 4 );
}
static void calc_expected_dps( avatar &test_guy, const std::string &weapon_id, double target )
{
    item weapon( weapon_id );
    CHECK( test_guy.melee_value( weapon ) == Approx( target ).margin( 0.75 ) );
    if( test_guy.melee_value( weapon ) != Approx( target ).margin( 0.75 ) ) {
        std::cout << weapon_id << " out of range, expected: " << target;
        std::cout << " got " << test_guy.melee_value( weapon ) << std::endl;
    }
}
/*
 * A super tedious set of test cases to make sure that weapon values do not drift too far out
 * of range without anyone noticing them and adjusting them.
 * Used expected_dps(), which should make actual dps because of the calculations above.
 */
TEST_CASE( "expected weapon dps", "[expected][dps]" )
{
    avatar &test_guy = g->u;
    make_experienced_tester( test_guy );

    SECTION( "staves" ) { // typical value around 18
        calc_expected_dps( test_guy, "i_staff", 18.0 );
        calc_expected_dps( test_guy, "q_staff", 17.0 );
        calc_expected_dps( test_guy, "l-stick_on", 18.0 );
        calc_expected_dps( test_guy, "l-stick", 18.0 );
        calc_expected_dps( test_guy, "shock_staff", 17.0 );
        calc_expected_dps( test_guy, "hockey_stick", 13.0 );
        calc_expected_dps( test_guy, "pool_cue", 10.0 );
        calc_expected_dps( test_guy, "broom", 4.0 );
    }
    SECTION( "spear" ) { // typical value around 24
        calc_expected_dps( test_guy, "spear_steel", 24.0 );
        calc_expected_dps( test_guy, "pike", 23.0 );
        calc_expected_dps( test_guy, "qiang", 23.0 );
        calc_expected_dps( test_guy, "spear_dory", 23 );
        calc_expected_dps( test_guy, "spear_homemade_halfpike", 20.0 );
        calc_expected_dps( test_guy, "spear_copper", 19.0 );
        calc_expected_dps( test_guy, "spear_pipe", 19.0 );
        calc_expected_dps( test_guy, "spear_knife_superior", 18.0 );
        calc_expected_dps( test_guy, "spear_knife", 18.0 );
        calc_expected_dps( test_guy, "pike_inferior", 17.0 );
        calc_expected_dps( test_guy, "spear_wood", 15.0 );
        calc_expected_dps( test_guy, "pitchfork", 15.0 );
        calc_expected_dps( test_guy, "spear_stone", 14.0 );
        calc_expected_dps( test_guy, "spear_forked", 14.0 );
        calc_expected_dps( test_guy, "pike_fake", 10.0 );
    }
    SECTION( "polearm" ) { // typical value around 35
        calc_expected_dps( test_guy, "halberd", 36.0 );
        calc_expected_dps( test_guy, "halberd_fake", 15.0 );
        calc_expected_dps( test_guy, "ji", 35.0 );
        calc_expected_dps( test_guy, "glaive", 35.0 );
        calc_expected_dps( test_guy, "naginata", 35.0 );
        calc_expected_dps( test_guy, "naginata_inferior", 21.0 );
        calc_expected_dps( test_guy, "naginata_fake", 10.0 );
        calc_expected_dps( test_guy, "lucern_hammer", 36.0 );
        calc_expected_dps( test_guy, "lucern_hammerfake", 14.0 );
        calc_expected_dps( test_guy, "spear_survivor", 26.0 );
        calc_expected_dps( test_guy, "long_pole", 13.0 );
    }
    SECTION( "two-handed axe" ) { // typical value around 29
        calc_expected_dps( test_guy, "battleaxe", 29.0 );
        calc_expected_dps( test_guy, "battleaxe_fake", 11.0 );
        calc_expected_dps( test_guy, "battleaxe_inferior", 20.0 );
        calc_expected_dps( test_guy, "fire_ax", 25.0 );
        calc_expected_dps( test_guy, "lobotomizer", 24.0 );
        calc_expected_dps( test_guy, "ax", 21.0 );
        calc_expected_dps( test_guy, "copper_ax", 12.0 );
        calc_expected_dps( test_guy, "e_combatsaw_on", 28.0 );
        calc_expected_dps( test_guy, "combatsaw_on", 28.0 );
        calc_expected_dps( test_guy, "chainsaw_on", 16.0 );
        calc_expected_dps( test_guy, "cs_lajatang_on", 17.0 );
        calc_expected_dps( test_guy, "ecs_lajatang_on", 17.0 );
        calc_expected_dps( test_guy, "circsaw_on", 18.0 );
        calc_expected_dps( test_guy, "e_combatsaw_off", 3.0 );
        calc_expected_dps( test_guy, "ecs_lajatang_off", 3.0 );
        calc_expected_dps( test_guy, "combatsaw_off", 3.0 );
        calc_expected_dps( test_guy, "chainsaw_off", 2.0 );
        calc_expected_dps( test_guy, "cs_lajatang_off", 2.5 );
        calc_expected_dps( test_guy, "circsaw_off", 2.0 );
    }
    SECTION( "two-handed club/hammer" ) { // expected value ideally around 28
        calc_expected_dps( test_guy, "warhammer", 28.0 );
        calc_expected_dps( test_guy, "hammer_sledge", 20.0 );
        calc_expected_dps( test_guy, "halligan", 17.0 );
        calc_expected_dps( test_guy, "stick_long", 6.0 );
    }
    SECTION( "two-handed flails" ) { // expected value ideally around 28
        calc_expected_dps( test_guy, "2h_flail_steel", 25.0 );
        calc_expected_dps( test_guy, "2h_flail_wood", 20.0 );
        calc_expected_dps( test_guy, "homewrecker", 13.0 );
    }
    SECTION( "fist weapons" ) { // expected value around 10 but wide variation
        calc_expected_dps( test_guy, "bio_claws_weapon", 18.0 ); // basically a knife
        calc_expected_dps( test_guy, "bagh_nakha", 14.5 );
        calc_expected_dps( test_guy, "punch_dagger", 11.0 );
        calc_expected_dps( test_guy, "knuckle_katar", 10.5 );
        calc_expected_dps( test_guy, "knuckle_steel", 4.0 );
        calc_expected_dps( test_guy, "knuckle_brass", 4.0 );
        calc_expected_dps( test_guy, "knuckle_nail", 4.0 );
        calc_expected_dps( test_guy, "cestus", 3.0 );
    }
    SECTION( "ax" ) { // expected value around 27 but no dedicated weapons
        calc_expected_dps( test_guy, "hatchet", 24.0 );
        calc_expected_dps( test_guy, "crash_axe", 24.0 );
        calc_expected_dps( test_guy, "iceaxe", 19.0 );
        calc_expected_dps( test_guy, "throwing_axe", 14.0 );
        calc_expected_dps( test_guy, "carver_on", 22.5 );
        calc_expected_dps( test_guy, "pickaxe", 10.5 );
        calc_expected_dps( test_guy, "primitive_adze", 10.0 ); // rock on a stick
        calc_expected_dps( test_guy, "primitive_axe", 10.0 ); // rock on a stick
        calc_expected_dps( test_guy, "makeshift_axe", 9.0 ); // chunk of sharp steel
        calc_expected_dps( test_guy, "hand_axe", 8.5 ); // chunk of sharp rock
    }
    SECTION( "club" ) { // expected value around 24 but most aren't dedicated weapons
        calc_expected_dps( test_guy, "mace", 24.0 );
        calc_expected_dps( test_guy, "morningstar", 23.0 );
        calc_expected_dps( test_guy, "shillelagh_weighted", 22.0 );
        calc_expected_dps( test_guy, "bwirebat", 22.0 );
        calc_expected_dps( test_guy, "baton-extended", 21.0 );
        calc_expected_dps( test_guy, "bat_metal", 21.0 );
        calc_expected_dps( test_guy, "nailbat", 21.0 );
        calc_expected_dps( test_guy, "bat", 20.0 );
        calc_expected_dps( test_guy, "shillelagh", 20.0 );
        calc_expected_dps( test_guy, "bokken", 20.0 );
        calc_expected_dps( test_guy, "PR24-extended", 20.0 );
        calc_expected_dps( test_guy, "mace_inferior", 18.0 );
        calc_expected_dps( test_guy, "tonfa", 17.0 );
        calc_expected_dps( test_guy, "tonfa_wood", 16.0 );
        calc_expected_dps( test_guy, "shocktonfa_off", 16.0 );
        calc_expected_dps( test_guy, "shocktonfa_on", 16.0 );
        calc_expected_dps( test_guy, "crowbar", 15.0 );
        calc_expected_dps( test_guy, "morningstar_inferior", 15.0 );
        calc_expected_dps( test_guy, "bokken_inferior", 13.0 );
        calc_expected_dps( test_guy, "golf_club", 14.0 );
        calc_expected_dps( test_guy, "mace_fake", 13.0 );
        calc_expected_dps( test_guy, "claw_bar", 11.0 );
        calc_expected_dps( test_guy, "shovel", 11.0 );
        calc_expected_dps( test_guy, "e_tool", 11.0 );
        calc_expected_dps( test_guy, "sword_nail", 11.0 );
        calc_expected_dps( test_guy, "sword_wood", 10.5 );
        calc_expected_dps( test_guy, "cane", 10.5 );
        calc_expected_dps( test_guy, "cudgel", 10.5 );
        calc_expected_dps( test_guy, "primitive_hammer", 10.0 );
        calc_expected_dps( test_guy, "bokken_fake", 10.5 );
        calc_expected_dps( test_guy, "shillelagh_fake", 9.5 );
        calc_expected_dps( test_guy, "morningstar_fake", 8.0 );
        calc_expected_dps( test_guy, "wrench", 7.0 );
        calc_expected_dps( test_guy, "hammer", 6.0 );
        calc_expected_dps( test_guy, "rebar", 7.0 );
        calc_expected_dps( test_guy, "primitive_shovel", 7.0 );
        calc_expected_dps( test_guy, "heavy_flashlight", 7.0 );
        calc_expected_dps( test_guy, "rock", 6.0 );
    }
    SECTION( "two-handed sword" ) { // expected value around 27, 25 for long swords
        calc_expected_dps( test_guy, "nodachi", 27.0 );
        calc_expected_dps( test_guy, "zweihander", 27.0 );
        calc_expected_dps( test_guy, "estoc", 27.0 );
        calc_expected_dps( test_guy, "longsword", 25.0 );
        calc_expected_dps( test_guy, "katana", 25.0 );
        calc_expected_dps( test_guy, "longsword_inferior", 18.5 );
        calc_expected_dps( test_guy, "zweihander_inferior", 16.5 );
        calc_expected_dps( test_guy, "katana_inferior", 16.0 );
        calc_expected_dps( test_guy, "nodachi_inferior", 17.0 );
        calc_expected_dps( test_guy, "estoc_inferior", 16.0 );
        calc_expected_dps( test_guy, "estoc_fake", 11.0 );
        calc_expected_dps( test_guy, "zweihander_fake", 10.0 );
        calc_expected_dps( test_guy, "longsword_fake", 9.0 );
        calc_expected_dps( test_guy, "nodachi_fake", 9.0 );
        calc_expected_dps( test_guy, "katana_fake", 8.0 );
    }
    SECTION( "sword" ) { // expected value 24, does not include shortswords
        calc_expected_dps( test_guy, "broadsword", 24.0 );
        calc_expected_dps( test_guy, "rapier", 24.0 );
        calc_expected_dps( test_guy, "arming_sword", 24.0 ); // heavier than a broadsword
        calc_expected_dps( test_guy, "jian", 24.0 );
        calc_expected_dps( test_guy, "broadsword_inferior", 20.0 );
        calc_expected_dps( test_guy, "arming_sword_inferior", 18.0 );
        calc_expected_dps( test_guy, "jian_inferior", 19.0 );
        calc_expected_dps( test_guy, "broadsword_fake", 10.0 );
        calc_expected_dps( test_guy, "rapier_fake", 8.0 );
        calc_expected_dps( test_guy, "arming_sword_fake", 13.0 );
        calc_expected_dps( test_guy, "jian_fake", 8.0 );
        calc_expected_dps( test_guy, "glass_macuahuitl", 11.0 );
        calc_expected_dps( test_guy, "blade_scythe", 6.0 );
    }
    SECTION( "shortsword" ) { // expected value 22
        calc_expected_dps( test_guy, "scimitar", 22.0 );
        calc_expected_dps( test_guy, "butterfly_swords", 22.0 );
        calc_expected_dps( test_guy, "cutlass", 22.0 );
        calc_expected_dps( test_guy, "sword_bayonet", 22.0 );
        calc_expected_dps( test_guy, "kukri", 22.0 );
        calc_expected_dps( test_guy, "wakizashi", 22.0 );
        calc_expected_dps( test_guy, "sword_xiphos", 22.0 );
        calc_expected_dps( test_guy, "khopesh", 21.0 );
        calc_expected_dps( test_guy, "survivor_machete", 21.0 );
        calc_expected_dps( test_guy, "cavalry_sabre", 20.0 );
        calc_expected_dps( test_guy, "machete", 20.0 );
        calc_expected_dps( test_guy, "dao", 18.0 );
        calc_expected_dps( test_guy, "sword_cane", 17.5 );
        calc_expected_dps( test_guy, "cutlass_inferior", 15.0 );
        calc_expected_dps( test_guy, "scimitar_inferior", 14.5 );
        calc_expected_dps( test_guy, "sword_crude", 14.0 );
        calc_expected_dps( test_guy, "wakizashi_inferior", 13.0 );
        calc_expected_dps( test_guy, "makeshift_machete", 11.0 );
        calc_expected_dps( test_guy, "cavalry_sabre_fake", 8.0 );
        calc_expected_dps( test_guy, "cutlass_fake", 7.0 );
        calc_expected_dps( test_guy, "scimitar_fake", 7.0 );
        calc_expected_dps( test_guy, "wakizashi_fake", 7.0 );
        calc_expected_dps( test_guy, "blade", 7.0 );
        calc_expected_dps( test_guy, "fencing_epee", 5.0 );
        calc_expected_dps( test_guy, "fencing_sabre", 4.0 );
        calc_expected_dps( test_guy, "fencing_foil", 2.0 );
    }
    SECTION( "knife" ) { // expected value 19
        calc_expected_dps( test_guy, "bio_blade_weapon", 25.0 ); // much better than any other knife
        calc_expected_dps( test_guy, "knife_trench", 19.0 );
        calc_expected_dps( test_guy, "kirpan", 18.0 );
        calc_expected_dps( test_guy, "knife_combat", 19.0 );
        calc_expected_dps( test_guy, "tanto", 18.0 );
        calc_expected_dps( test_guy, "kris", 18.0 );
        calc_expected_dps( test_guy, "knife_rambo", 17.0 );
        calc_expected_dps( test_guy, "tanto_inferior", 15.0 );
        calc_expected_dps( test_guy, "knife_hunting", 11.0 );
        calc_expected_dps( test_guy, "kirpan_cheap", 11.0 );
        calc_expected_dps( test_guy, "switchblade", 10.5 );
        calc_expected_dps( test_guy, "knife_meat_cleaver", 8.5 );
        calc_expected_dps( test_guy, "diveknife", 8.0 );
        calc_expected_dps( test_guy, "copper_knife", 8.0 );
        calc_expected_dps( test_guy, "knife_butcher", 7.5 );
        calc_expected_dps( test_guy, "throwing_knife", 7.0 );
        calc_expected_dps( test_guy, "tanto_fake", 7.0 );
        calc_expected_dps( test_guy, "pockknife", 4.5 );
        calc_expected_dps( test_guy, "spike", 4.0 );
        calc_expected_dps( test_guy, "kris_fake", 2.5 );
        calc_expected_dps( test_guy, "primitive_knife", 2.5 );
    }
}
