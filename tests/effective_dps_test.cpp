#include <algorithm>
#include <cstdlib>
#include <iostream>

#include "avatar.h"
#include "cata_catch.h"
#include "item.h"
#include "melee.h"
#include "monster.h"
#include "player_helpers.h"
#include "sounds.h"
#include "ret_val.h"
#include "type_id.h"

static const mtype_id debug_mon( "debug_mon" );
static const mtype_id mon_zombie_smoker( "mon_zombie_smoker" );
static const mtype_id mon_zombie_soldier_no_weakpoints( "mon_zombie_soldier_no_weakpoints" );
static const mtype_id mon_zombie_survivor_no_weakpoints( "mon_zombie_survivor_no_weakpoints" );

static const skill_id skill_bashing( "bashing" );
static const skill_id skill_cutting( "cutting" );
static const skill_id skill_melee( "melee" );
static const skill_id skill_stabbing( "stabbing" );
static const skill_id skill_unarmed( "unarmed" );

struct itype;

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
            // Verify that wielding worked (and not e.g. using martial arts instead)
            REQUIRE( !!attacker.used_weapon() );
            REQUIRE( attacker.used_weapon()->type == weapon.type );

            int before_moves = attacker.get_moves();

            // Keep the defender at maximum health
            const int starting_hp = defender.get_hp_max();
            defender.set_hp( starting_hp );

            // Attack once
            attacker.melee_attack_abstract( defender, false, matec_id( "" ) );

            // Tally total damage and moves
            total_damage += std::max( 0, starting_hp - defender.get_hp() );
            total_moves += std::abs( attacker.get_moves() - before_moves );

            // Every hit or miss enqueues a new sound
            // Ideally, we'd have sound vector get cleared after every test, but it's not easy
            sounds::reset_sounds();
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
    avatar &dummy = get_avatar();
    clear_character( dummy );

    item clumsy_sword( "test_clumsy_sword" );
    item normal_sword( "test_normal_sword" );
    item good_sword( "test_balanced_sword" );

    SECTION( "against a debug monster with no armor or dodge" ) {
        monster mummy( debug_mon );

        CHECK( clumsy_sword.effective_dps( dummy, mummy ) == Approx( 25.0f ).epsilon( 0.15f ) );
        CHECK( good_sword.effective_dps( dummy, mummy ) == Approx( 38.0f ).epsilon( 0.15f ) );
    }

    SECTION( "against an agile target" ) {
        monster smoker( mon_zombie_smoker );
        REQUIRE( smoker.get_dodge() >= 4 );

        CHECK( clumsy_sword.effective_dps( dummy, smoker ) == Approx( 11.0f ).epsilon( 0.15f ) );
        CHECK( good_sword.effective_dps( dummy, smoker ) == Approx( 25.0f ).epsilon( 0.15f ) );
    }

    SECTION( "against an armored target" ) {
        monster soldier( mon_zombie_soldier_no_weakpoints );

        CHECK( clumsy_sword.effective_dps( dummy, soldier ) == Approx( 8.0f ).epsilon( 0.15f ) );
        CHECK( good_sword.effective_dps( dummy, soldier ) == Approx( 15.0f ).epsilon( 0.15f ) );
    }

    SECTION( "effect of STR and DEX on damage per second" ) {
        monster mummy( debug_mon );

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
    avatar &dummy = get_avatar();
    clear_character( dummy );

    monster soldier( mon_zombie_soldier_no_weakpoints );
    monster smoker( mon_zombie_smoker );
    monster survivor( mon_zombie_survivor_no_weakpoints );

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
    avatar &dummy = get_avatar();
    clear_character( dummy );

    monster soldier( mon_zombie_soldier_no_weakpoints );
    monster smoker( mon_zombie_smoker );
    monster survivor( mon_zombie_survivor_no_weakpoints );

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
    test_guy.set_body();
    test_guy.set_all_parts_hp_to_max();

    test_guy.set_skill_level( skill_bashing, 4 );
    test_guy.set_skill_level( skill_cutting, 4 );
    test_guy.set_skill_level( skill_stabbing, 4 );
    test_guy.set_skill_level( skill_unarmed, 4 );
    test_guy.set_skill_level( skill_melee, 4 );

    REQUIRE( test_guy.get_str() == 10 );
    REQUIRE( test_guy.get_dex() == 10 );
    REQUIRE( test_guy.get_per() == 10 );
    REQUIRE( test_guy.get_skill_level( skill_bashing ) == 4 );
    REQUIRE( test_guy.get_skill_level( skill_cutting ) == 4 );
    REQUIRE( test_guy.get_skill_level( skill_stabbing ) == 4 );
    REQUIRE( test_guy.get_skill_level( skill_unarmed ) == 4 );
    REQUIRE( test_guy.get_skill_level( skill_melee ) == 4 );
}
/*
 * A super tedious set of test cases to make sure that weapon values do not drift too far out
 * of range without anyone noticing them and adjusting them.
 * Used expected_dps(), which should make actual dps because of the calculations above.
 */
static void check_staves( const std::function<Approx( const std::string & )> &calc_expected_dps )
{
    SECTION( "staves" ) { // typical value around 18
        CHECK( calc_expected_dps( "i_staff" ) == 20.62 );
        CHECK( calc_expected_dps( "staff_sling" ) == 11.56 );
        CHECK( calc_expected_dps( "q_staff" ) == 15.74 );
        CHECK( calc_expected_dps( "l-stick_on" ) == 17.5 );
        CHECK( calc_expected_dps( "l-stick" ) == 17.5 );
        CHECK( calc_expected_dps( "shock_staff" ) == 19.61 );
        CHECK( calc_expected_dps( "hockey_stick" ) == 13.75 );
        CHECK( calc_expected_dps( "pool_cue" ) == 7.76 );
        CHECK( calc_expected_dps( "broom" ) == 3.25 );
    }
}

static void check_spears( const std::function<Approx( const std::string & )> &calc_expected_dps )
{
    SECTION( "spears" ) { // typical value around 24
        CHECK( calc_expected_dps( "spear_steel" ) == 23.19 );
        CHECK( calc_expected_dps( "pike" ) == 23.0 );
        CHECK( calc_expected_dps( "qiang" ) == 22.1 );
        CHECK( calc_expected_dps( "spear_dory" ) == 19.97 );
        CHECK( calc_expected_dps( "spear_homemade_halfpike" ) == 18.4 );
        CHECK( calc_expected_dps( "spear_copper" ) == 19.0 );
        CHECK( calc_expected_dps( "spear_pipe" ) == 17.37 );
        CHECK( calc_expected_dps( "spear_knife_superior" ) == 17.9 );
        CHECK( calc_expected_dps( "spear_knife" ) == 17.9 );
        CHECK( calc_expected_dps( "pike_inferior" ) == 16.0 );
        CHECK( calc_expected_dps( "spear_wood" ) == 10.95 );
        CHECK( calc_expected_dps( "pitchfork" ) == 12.48 );
        CHECK( calc_expected_dps( "spear_stone" ) == 10.42 );
        CHECK( calc_expected_dps( "spear_forked" ) == 12.15 );
        CHECK( calc_expected_dps( "pike_fake" ) == 7.55 );
    }
}

static void check_polearms( const std::function<Approx( const std::string & )> &calc_expected_dps )
{
    SECTION( "polearms" ) { // typical value around 35
        CHECK( calc_expected_dps( "halberd" ) == 36.28 );
        CHECK( calc_expected_dps( "halberd_fake" ) == 12.77 );
        CHECK( calc_expected_dps( "ji" ) == 35.82 );
        CHECK( calc_expected_dps( "glaive" ) == 33.87 );
        CHECK( calc_expected_dps( "poleaxe" ) == 34.5 );
        CHECK( calc_expected_dps( "makeshift_halberd" ) == 20.5 );
        CHECK( calc_expected_dps( "naginata" ) == 33.97 );
        CHECK( calc_expected_dps( "naginata_inferior" ) == 19.35 );
        CHECK( calc_expected_dps( "naginata_fake" ) == 5.38 );
        CHECK( calc_expected_dps( "lucern_hammer" ) == 36.0 );
        CHECK( calc_expected_dps( "lucern_hammerfake" ) == 13.0 );
        CHECK( calc_expected_dps( "spear_survivor" ) == 29.74 );
        CHECK( calc_expected_dps( "long_pole" ) == 13.0 );
        CHECK( calc_expected_dps( "scythe_war" ) == 34.59 );
        CHECK( calc_expected_dps( "makeshift_scythe_war" ) == 24.5 );
    }
}

static void check_two_handed_axes( const std::function<Approx( const std::string & )>
                                   &calc_expected_dps )
{
    SECTION( "two-handed axes" ) { // typical value around 29
        CHECK( calc_expected_dps( "battleaxe" ) == 29.0 );
        CHECK( calc_expected_dps( "battleaxe_fake" ) == 12.0 );
        CHECK( calc_expected_dps( "battleaxe_inferior" ) == 19.25 );
        CHECK( calc_expected_dps( "fire_ax" ) == 25.0 );
        CHECK( calc_expected_dps( "lobotomizer" ) == 24.0 );
        CHECK( calc_expected_dps( "ax" ) == 20.25 );
        CHECK( calc_expected_dps( "copper_ax" ) == 13.75 );
        CHECK( calc_expected_dps( "e_combatsaw_on" ) == 29 );
        CHECK( calc_expected_dps( "combatsaw_on" ) == 29 );
        CHECK( calc_expected_dps( "chainsaw_on" ) == 16.0 );
        CHECK( calc_expected_dps( "circsaw_on" ) == 18.0 );
        CHECK( calc_expected_dps( "e_combatsaw_off" ) == 3.0 );
        CHECK( calc_expected_dps( "combatsaw_off" ) == 3.0 );
        CHECK( calc_expected_dps( "chainsaw_off" ) == 2.0 );
        CHECK( calc_expected_dps( "circsaw_off" ) == 1.25 );
    }
}

static void check_two_handed_clubs_hammers( const std::function<Approx( const std::string & )>
        &calc_expected_dps )
{
    SECTION( "two-handed clubs/hammers" ) { // expected value ideally around 28
        CHECK( calc_expected_dps( "warhammer" ) == 35.77 );
        CHECK( calc_expected_dps( "hammer_sledge" ) == 20.0 );
        CHECK( calc_expected_dps( "halligan" ) == 16.5 );
        CHECK( calc_expected_dps( "stick_long" ) == 6.0 );
    }
}

static void check_two_handed_flails( const std::function<Approx( const std::string & )>
                                     &calc_expected_dps )
{
    SECTION( "two-handed flails" ) { // expected value ideally around 20
        CHECK( calc_expected_dps( "2h_flail_steel" ) == 21.0 );
        CHECK( calc_expected_dps( "2h_flail_wood" ) == 20.0 );
        CHECK( calc_expected_dps( "homewrecker" ) == 13.0 );
    }
}

static void check_fist_weapons( const std::function<Approx( const std::string & )>
                                &calc_expected_dps )
{
    SECTION( "fist weapons" ) { // expected value around 10 but wide variation
        CHECK( calc_expected_dps( "bio_claws_weapon" ) == 16.5 ); // basically a knife
        CHECK( calc_expected_dps( "bagh_nakha" ) == 14.0 );
        CHECK( calc_expected_dps( "punch_dagger" ) == 11.0 );
        CHECK( calc_expected_dps( "knuckle_katar" ) == 10.5 );
    }
}

static void check_axes( const std::function<Approx( const std::string & )> &calc_expected_dps )
{
    SECTION( "axes" ) { // expected value around 27 but no dedicated weapons
        CHECK( calc_expected_dps( "hatchet" ) == 18.0 );
        CHECK( calc_expected_dps( "crash_axe" ) == 24.0 );
        CHECK( calc_expected_dps( "iceaxe" ) == 19.0 );
        CHECK( calc_expected_dps( "throwing_axe" ) == 14.0 );
        CHECK( calc_expected_dps( "carver_on" ) == 22.5 );
        CHECK( calc_expected_dps( "pickaxe" ) == 10.5 );
        CHECK( calc_expected_dps( "primitive_adze" ) == 10.0 ); // rock on a stick
        CHECK( calc_expected_dps( "primitive_axe" ) == 10.0 ); // rock on a stick
        CHECK( calc_expected_dps( "makeshift_axe" ) == 10.0 ); // chunk of sharp steel
        CHECK( calc_expected_dps( "hand_axe" ) == 8.5 ); // chunk of sharp rock
    }
}

static void check_clubs( const std::function<Approx( const std::string & )> &calc_expected_dps )
{
    SECTION( "clubs" ) { // expected value around 24 but most aren't dedicated weapons
        CHECK( calc_expected_dps( "mace" ) == 24.0 );
        CHECK( calc_expected_dps( "morningstar" ) == 26.12 );
        CHECK( calc_expected_dps( "shillelagh_weighted" ) == 24.17 );
        CHECK( calc_expected_dps( "bwirebat" ) == 22.0 );
        CHECK( calc_expected_dps( "baton-extended" ) == 12.0 );
        CHECK( calc_expected_dps( "bat_metal" ) == 21.0 );
        CHECK( calc_expected_dps( "nailbat" ) == 21.0 );
        CHECK( calc_expected_dps( "bat" ) == 20.0 );
        CHECK( calc_expected_dps( "shillelagh" ) == 22.21 );
        CHECK( calc_expected_dps( "bokken" ) == 20.0 );
        CHECK( calc_expected_dps( "PR24-extended" ) == 13.0 );
        CHECK( calc_expected_dps( "mace_inferior" ) == 18.5 );
        CHECK( calc_expected_dps( "tonfa" ) == 12.0 );
        CHECK( calc_expected_dps( "tonfa_wood" ) == 11.0 );
        CHECK( calc_expected_dps( "shocktonfa_off" ) == 11.1 );
        CHECK( calc_expected_dps( "shocktonfa_on" ) == 12.42 );
        CHECK( calc_expected_dps( "crowbar" ) == 15.0 );
        CHECK( calc_expected_dps( "morningstar_inferior" ) == 19.17 );
        CHECK( calc_expected_dps( "bokken_inferior" ) == 14.0 );
        CHECK( calc_expected_dps( "golf_club" ) == 14.0 );
        CHECK( calc_expected_dps( "mace_fake" ) == 13.0 );
        CHECK( calc_expected_dps( "claw_bar" ) == 11.0 );
        CHECK( calc_expected_dps( "shovel" ) == 10.25 );
        CHECK( calc_expected_dps( "e_tool" ) == 11.0 );
        CHECK( calc_expected_dps( "sword_nail" ) == 9.06 );
        CHECK( calc_expected_dps( "sword_wood" ) == 8.5 );
        CHECK( calc_expected_dps( "cane" ) == 10.5 );
        CHECK( calc_expected_dps( "cudgel" ) == 10.5 );
        CHECK( calc_expected_dps( "primitive_hammer" ) == 10.0 );
        CHECK( calc_expected_dps( "bokken_fake" ) == 9.5 );
        CHECK( calc_expected_dps( "shillelagh_fake" ) == 12.11 );
        CHECK( calc_expected_dps( "morningstar_fake" ) == 10.74 );
        CHECK( calc_expected_dps( "wrench" ) == 8.0 );
        CHECK( calc_expected_dps( "hammer" ) == 7.0 );
        CHECK( calc_expected_dps( "rebar" ) == 7.0 );
        CHECK( calc_expected_dps( "primitive_shovel" ) == 7.0 );
        CHECK( calc_expected_dps( "heavy_flashlight" ) == 7.5 );
        CHECK( calc_expected_dps( "rock" ) == 6.0 );
    }
}

static void check_two_handed_swords( const std::function<Approx( const std::string & )>
                                     &calc_expected_dps )
{
    SECTION( "two-handed swords" ) { // expected value around 27, 25 for long swords
        CHECK( calc_expected_dps( "nodachi" ) == 26.5 );
        CHECK( calc_expected_dps( "zweihander" ) == 25.78 );
        CHECK( calc_expected_dps( "estoc" ) == 27.0 );
        CHECK( calc_expected_dps( "longsword" ) == 25 );
        CHECK( calc_expected_dps( "katana" ) == 25.0 );
        CHECK( calc_expected_dps( "longsword_inferior" ) == 15.5 );
        CHECK( calc_expected_dps( "zweihander_inferior" ) == 11.64 );
        CHECK( calc_expected_dps( "katana_inferior" ) == 13.5 );
        CHECK( calc_expected_dps( "nodachi_inferior" ) == 16.25 );
        CHECK( calc_expected_dps( "estoc_inferior" ) == 15.5 );
        CHECK( calc_expected_dps( "estoc_fake" ) == 8.95 );
        CHECK( calc_expected_dps( "zweihander_fake" ) == 7.21 );
        CHECK( calc_expected_dps( "longsword_fake" ) == 6.25 );
        CHECK( calc_expected_dps( "nodachi_fake" ) == 8.5 );
        CHECK( calc_expected_dps( "katana_fake" ) == 7.0 );
    }
}

static void check_swords( const std::function<Approx( const std::string & )> &calc_expected_dps )
{
    SECTION( "swords" ) { // expected value 24, does not include shortswords
        CHECK( calc_expected_dps( "broadsword" ) == 24.0 );
        CHECK( calc_expected_dps( "rapier" ) == 21.47 );
        CHECK( calc_expected_dps( "arming_sword" ) == 24.0 ); // heavier than a broadsword
        CHECK( calc_expected_dps( "jian" ) == 24.0 );
        CHECK( calc_expected_dps( "broadsword_inferior" ) == 20.0 );
        CHECK( calc_expected_dps( "arming_sword_inferior" ) == 17.0 );
        CHECK( calc_expected_dps( "jian_inferior" ) == 18.5 );
        CHECK( calc_expected_dps( "broadsword_fake" ) == 8.5 );
        CHECK( calc_expected_dps( "rapier_fake" ) == 6.98 );
        CHECK( calc_expected_dps( "arming_sword_fake" ) == 11.5 );
        CHECK( calc_expected_dps( "jian_fake" ) == 8.25 );
        CHECK( calc_expected_dps( "glass_macuahuitl" ) == 11.0 );
        CHECK( calc_expected_dps( "blade_scythe" ) == 5.25 );
    }
}

static void check_shortswords( const std::function<Approx( const std::string & )>
                               &calc_expected_dps )
{
    SECTION( "shortswords" ) { // expected value 22
        CHECK( calc_expected_dps( "scimitar" ) == 24.3 );
        CHECK( calc_expected_dps( "butterfly_swords" ) == 22.0 );
        CHECK( calc_expected_dps( "cutlass" ) == 22.0 );
        CHECK( calc_expected_dps( "sword_bayonet" ) == 22.75 );
        CHECK( calc_expected_dps( "kukri" ) == 19.9 );
        CHECK( calc_expected_dps( "wakizashi" ) == 22.0 );
        CHECK( calc_expected_dps( "sword_xiphos" ) == 22.0 );
        CHECK( calc_expected_dps( "khopesh" ) == 24.26 );
        CHECK( calc_expected_dps( "survivor_machete" ) == 21.0 );
        CHECK( calc_expected_dps( "cavalry_sabre" ) == 23.29 );
        CHECK( calc_expected_dps( "machete" ) == 20.0 );
        CHECK( calc_expected_dps( "dao" ) == 23.47 );
        CHECK( calc_expected_dps( "sword_cane" ) == 20.34 );
        CHECK( calc_expected_dps( "cutlass_inferior" ) == 17.5 );
        CHECK( calc_expected_dps( "scimitar_inferior" ) == 16.15 );
        CHECK( calc_expected_dps( "sword_crude" ) == 9.5 );
        CHECK( calc_expected_dps( "wakizashi_inferior" ) == 15.0 );
        CHECK( calc_expected_dps( "makeshift_machete" ) == 9.19 );
        CHECK( calc_expected_dps( "cavalry_sabre_fake" ) == 8.98 );
        CHECK( calc_expected_dps( "cutlass_fake" ) == 7.75 );
        CHECK( calc_expected_dps( "scimitar_fake" ) == 8.44 );
        CHECK( calc_expected_dps( "wakizashi_fake" ) == 7.0 );
        CHECK( calc_expected_dps( "blade" ) == 7.0 );
        CHECK( calc_expected_dps( "fencing_epee" ) == 5.93 );
        CHECK( calc_expected_dps( "fencing_sabre" ) == 4.63 );
        CHECK( calc_expected_dps( "fencing_foil" ) == 2.0 );
    }
}

static void check_knives( const std::function<Approx( const std::string & )> &calc_expected_dps )
{
    SECTION( "knives" ) { // expected value 19
        CHECK( calc_expected_dps( "bio_blade_weapon" ) == 24.5 ); // much better than any other knife
        CHECK( calc_expected_dps( "knife_combat" ) == 19.0 );
        CHECK( calc_expected_dps( "knife_trench" ) == 15.19 );
        CHECK( calc_expected_dps( "knife_baselard" ) == 15.87 );
        CHECK( calc_expected_dps( "kirpan" ) == 18.0 );
        CHECK( calc_expected_dps( "tanto" ) == 15.78 );
        CHECK( calc_expected_dps( "kris" ) == 14.08 );
        CHECK( calc_expected_dps( "knife_rambo" ) == 14.36 );
        CHECK( calc_expected_dps( "tanto_inferior" ) == 14.14 );
        CHECK( calc_expected_dps( "bone_knife" ) == 7.9 );
        CHECK( calc_expected_dps( "knife_hunting" ) == 11.0 );
        CHECK( calc_expected_dps( "kirpan_cheap" ) == 7.61 );
        CHECK( calc_expected_dps( "switchblade" ) == 10.5 );
        CHECK( calc_expected_dps( "knife_meat_cleaver" ) == 13.97 );
        CHECK( calc_expected_dps( "diveknife" ) == 8.0 );
        CHECK( calc_expected_dps( "copper_knife" ) == 8.0 );
        CHECK( calc_expected_dps( "knife_butcher" ) == 10 );
        CHECK( calc_expected_dps( "throwing_knife" ) == 7.0 );
        CHECK( calc_expected_dps( "tanto_fake" ) == 6.33 );
        CHECK( calc_expected_dps( "pockknife" ) == 4.5 );
        CHECK( calc_expected_dps( "spike" ) == 4.0 );
        CHECK( calc_expected_dps( "kris_fake" ) == 2.5 );
        CHECK( calc_expected_dps( "primitive_knife" ) == 2.5 );
    }
}

TEST_CASE( "expected weapon dps", "[expected][dps]" )
{
    avatar &test_guy = get_avatar();
    make_experienced_tester( test_guy );

    const auto calc_expected_dps = [&test_guy]( const std::string & weapon_id ) {
        item weapon( weapon_id );
        return Approx( test_guy.melee_value( weapon ) ).margin( 0.5 );
    };

    check_staves( calc_expected_dps );
    check_spears( calc_expected_dps );
    check_polearms( calc_expected_dps );
    check_two_handed_axes( calc_expected_dps );
    check_two_handed_clubs_hammers( calc_expected_dps );
    check_two_handed_flails( calc_expected_dps );
    check_fist_weapons( calc_expected_dps );
    check_axes( calc_expected_dps );
    check_clubs( calc_expected_dps );
    check_two_handed_swords( calc_expected_dps );
    check_swords( calc_expected_dps );
    check_shortswords( calc_expected_dps );
    check_knives( calc_expected_dps );
}
