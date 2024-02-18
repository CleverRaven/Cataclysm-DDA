#include <array>
#include <iosfwd>

#include "cached_options.h"
#include "calendar.h"
#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "character.h"
#include "character_martial_arts.h"
#include "creature.h"
#include "line.h"
#include "map.h"
#include "map_helpers.h"
#include "martialarts.h"
#include "mattack_actors.h"
#include "mattack_common.h"
#include "messages.h"
#include "monattack.h"
#include "monster.h"
#include "mtype.h"
#include "options_helpers.h"
#include "player_helpers.h"
#include "point.h"
#include "test_statistics.h"
#include "type_id.h"
#include "weather.h"
#include "weather_type.h"
#include "game.h"
#include "type_id.h"
#include "units.h"

static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_grabbed( "grabbed" );
static const efftype_id effect_took_flumed( "took_flumed" );

static const itype_id itype_rock( "rock" );
static const json_character_flag json_flag_GRAB( "GRAB" );
static const json_character_flag json_flag_TOUGH_FEET( "TOUGH_FEET" );
static const matype_id style_brawling( "style_brawling" );
static const skill_id skill_unarmed( "unarmed" );
static const trait_id trait_TOUGH_FEET( "TOUGH_FEET" );

static constexpr tripoint attacker_location{ 65, 65, 0 };
static constexpr int test_margin = 75; // 7.5% margin on test outcomes

static void reset_caches( int a_zlev, int t_zlev )
{
    Character &you = get_player_character();
    map &here = get_map();
    g->reset_light_level();
    // Why twice? See vision_test.cpp
    here.update_visibility_cache( a_zlev );
    here.invalidate_map_cache( a_zlev );
    here.build_map_cache( a_zlev );
    here.update_visibility_cache( a_zlev );
    here.invalidate_map_cache( a_zlev );
    here.build_map_cache( a_zlev );
    if( a_zlev != t_zlev ) {
        here.update_visibility_cache( t_zlev );
        here.invalidate_map_cache( t_zlev );
        here.build_map_cache( t_zlev );
        here.update_visibility_cache( t_zlev );
        here.invalidate_map_cache( t_zlev );
        here.build_map_cache( t_zlev );
    }
    you.recalc_sight_limits();
}

static void test_monster_attack( const tripoint &target_offset, bool expect_attack,
                                 bool expect_vision, bool( *special_attack )( monster *x ) = nullptr )
{
    int day_hour = hour_of_day<int>( calendar::turn );
    CAPTURE( day_hour );
    REQUIRE( is_day( calendar::turn ) );
    clear_creatures();
    // Monster adjacent to target.
    const std::string monster_type = "mon_zombie";
    const tripoint target_location = attacker_location + target_offset;
    int distance = rl_dist( attacker_location, target_location );
    CAPTURE( distance );
    int a_zlev = attacker_location.z;
    int t_zlev = target_location.z;
    Character &you = get_player_character();
    clear_avatar();
    you.setpos( target_location );
    monster &test_monster = spawn_test_monster( monster_type, attacker_location );
    test_monster.set_dest( you.get_location() );
    reset_caches( a_zlev, t_zlev );
    // Trigger basic attack.
    CAPTURE( attacker_location );
    CAPTURE( target_location );
    CAPTURE( fov_3d );
    CHECK( test_monster.sees( target_location ) == expect_vision );
    if( special_attack == nullptr ) {
        CHECK( test_monster.attack_at( target_location ) == expect_attack );
    } else {
        CHECK( special_attack( &test_monster ) == expect_attack );
    }
    // Then test the reverse.
    clear_creatures();
    clear_avatar();
    you.setpos( attacker_location );
    monster &target_monster = spawn_test_monster( monster_type, target_location );
    reset_caches( a_zlev, t_zlev );
    CHECK( you.sees( target_monster ) == expect_vision );
    if( special_attack == nullptr ) {
        CHECK( you.melee_attack( target_monster, false ) == expect_attack );
    }
}

static void monster_attack_zlevel( const std::string &title, const tripoint &offset,
                                   const std::string &monster_ter, const std::string &target_ter,
                                   bool expected )
{
    clear_map();
    map &here = get_map();
    restore_on_out_of_scope<bool> restore_fov_3d( fov_3d );
    fov_3d = GENERATE( false, true );
    override_option opt( "FOV_3D", fov_3d ? "true" : "false" );

    std::stringstream section_name;
    section_name << title;
    section_name << " " << ( fov_3d ? "3d" : "2d" );

    SECTION( section_name.str() ) {
        here.ter_set( attacker_location, ter_id( monster_ter ) );
        here.ter_set( attacker_location + offset, ter_id( target_ter ) );
        test_monster_attack( offset, expected && fov_3d, fov_3d );
        for( const tripoint &more_offset : eight_horizontal_neighbors ) {
            here.ter_set( attacker_location + offset + more_offset, ter_id( "t_floor" ) );
            test_monster_attack( offset + more_offset, false, expected && fov_3d );
        }
    }
}

TEST_CASE( "monster_attack", "[vision][reachability]" )
{
    clear_map();
    restore_on_out_of_scope<time_point> restore_calendar_turn( calendar::turn );
    calendar::turn = daylight_time( calendar::turn ) + 2_hours;
    scoped_weather_override weather_clear( WEATHER_CLEAR );
    SECTION( "attacking on open ground" ) {
        // Adjacent can attack of course.
        for( const tripoint &offset : eight_horizontal_neighbors ) {
            test_monster_attack( offset, true, true );
        }
        clear_map();
        // Too far away cannot.
        test_monster_attack( { 2, 2, 0 }, false, true );
        test_monster_attack( { 2, 1, 0 }, false, true );
        test_monster_attack( { 2, 0, 0 }, false, true );
        test_monster_attack( { 2, -1, 0 }, false, true );
        test_monster_attack( { 2, -2, 0 }, false, true );
        test_monster_attack( { 1, 2, 0 }, false, true );
        test_monster_attack( { 1, -2, 0 }, false, true );
        test_monster_attack( { 0, 2, 0 }, false, true );
        test_monster_attack( { 0, -2, 0 }, false, true );
        test_monster_attack( { -1, 2, 0 }, false, true );
        test_monster_attack( { -1, -2, 0 }, false, true );
        test_monster_attack( { -2, 2, 0 }, false, true );
        test_monster_attack( { -2, 1, 0 }, false, true );
        test_monster_attack( { -2, 0, 0 }, false, true );
        test_monster_attack( { -2, -1, 0 }, false, true );
        test_monster_attack( { -2, -2, 0 }, false, true );
    }

    monster_attack_zlevel( "attack_up_stairs", tripoint_above, "t_stairs_up", "t_stairs_down", true );
    monster_attack_zlevel( "attack_down_stairs", tripoint_below, "t_stairs_down", "t_stairs_up", true );
    monster_attack_zlevel( "attack through ceiling", tripoint_above, "t_floor", "t_floor", false );
    monster_attack_zlevel( "attack through floor", tripoint_below, "t_floor", "t_floor", false );

    monster_attack_zlevel( "attack up ledge", tripoint_above, "t_floor", "t_floor", false );
    monster_attack_zlevel( "attack down ledge", tripoint_below, "t_floor", "t_floor", false );
}

TEST_CASE( "monster_special_attack", "[vision][reachability]" )
{
    clear_map();
    restore_on_out_of_scope<time_point> restore_calendar_turn( calendar::turn );
    calendar::turn = daylight_time( calendar::turn ) + 2_hours;
    scoped_weather_override weather_clear( WEATHER_CLEAR );
    restore_on_out_of_scope<bool> restore_fov_3d( fov_3d );
    fov_3d = GENERATE( false, true );
    override_option opt( "FOV_3D", fov_3d ? "true" : "false" );
    get_map().ter_set( attacker_location + tripoint{ 2, 0, 0 }, ter_id( "t_wall" ) );
    get_map().ter_set( attacker_location + tripoint{ 2, 0, 1 }, ter_id( "t_floor" ) );
    get_map().ter_set( attacker_location + tripoint_east, ter_id( "t_wall" ) );
    get_map().ter_set( attacker_location + tripoint{ 1, 0, 1 }, ter_id( "t_floor" ) );
    // Adjacent should be visible if 3d vision is on, but it's too close to attack.
    //test_monster_attack( { 1, 0, 1 },  false, fov_3d, mattack::stretch_attack );
    // At a distance of 2, the ledge should block los and line of attack.
    test_monster_attack( { 2, 0, 1 },  false, false, mattack::stretch_attack );
}

TEST_CASE( "monster_throwing_sanity_test", "[throwing],[balance]" )
{
    std::array<float, 6> expected_average_damage_at_range = { 0, 0, 8.5, 6.5, 5, 3.25 };
    clear_map();
    map &here = get_map();
    restore_on_out_of_scope<time_point> restore_calendar_turn( calendar::turn );
    calendar::turn = sunrise( calendar::turn );
    scoped_weather_override weather_clear( WEATHER_CLEAR );
    const tripoint target_location = { 65, 65, 0 };
    // You got a player
    Character &you = get_player_character();
    clear_avatar();
    you.set_dodges_left( 1 ) ;
    REQUIRE( Approx( you.get_dodge() ) == 4.0 );
    you.setpos( target_location );
    const tripoint_abs_ms abs_target_location = you.get_location();
    reset_caches( target_location.z, target_location.z );
    REQUIRE( g->natural_light_level( 0 ) > 50.0 );
    CHECK( here.ambient_light_at( target_location ) > 50.0 );
    const std::string monster_type = "mon_feral_human_pipe";
    for( int distance = 2; distance <= 5; ++distance ) {
        float expected_damage = expected_average_damage_at_range[ distance ];
        // and you got a monster
        const tripoint attacker_location = target_location + tripoint_east * distance;
        monster &test_monster = spawn_test_monster( monster_type, attacker_location );
        test_monster.set_dest( you.get_location() );
        const mtype_special_attack &attack = test_monster.type->special_attacks.at( "gun" );
        REQUIRE( test_monster.get_dest() == abs_target_location );
        REQUIRE( test_monster.sees( target_location ) );
        Creature *target = test_monster.attack_target();
        REQUIRE( target );
        REQUIRE( test_monster.sees( *target ) );
        REQUIRE( rl_dist( test_monster.pos(), target->pos() ) <= 5 );
        statistics<int> damage_dealt;
        statistics<bool> hits;
        epsilon_threshold threshold{ expected_damage, 2.5 };
        do {
            you.set_all_parts_hp_to_max();
            you.set_stamina( you.get_stamina_max() ); // Resets stamina so dummy can keep dodging
            // Remove stagger/winded effects
            you.clear_effects();
            you.set_dodges_left( 1 );
            int prev_hp = you.get_hp();
            // monster shoots the player
            REQUIRE( attack->call( test_monster ) == true );
            // how much damage did it do?
            // Player-centric test in throwing_test.cpp ranges from 2 - 8 damage at point-blank range.
            int current_hp = you.get_hp();
            hits.add( current_hp < prev_hp );
            damage_dealt.add( prev_hp - current_hp );
            test_monster.ammo[ itype_rock ]++;
        } while( damage_dealt.n() < 100 || damage_dealt.uncertain_about( threshold ) );
        clear_creatures();
        CAPTURE( expected_damage );
        CAPTURE( distance );
        INFO( "Num hits: " << damage_dealt.n() );
        INFO( "Hit rate: " << hits.avg() );
        INFO( "Avg total damage: " << damage_dealt.avg() );
        INFO( "Dmg Lower: " << damage_dealt.lower() << " Dmg Upper: " << damage_dealt.upper() );
        CHECK( damage_dealt.test_threshold( threshold ) );
    }
}

TEST_CASE( "Mattack_dialog_condition_test", "[mattack]" )
{
    clear_map();
    clear_creatures();
    const tripoint target_location = attacker_location + tripoint_east;
    Character &you = get_player_character();
    clear_avatar();
    you.setpos( target_location );
    const std::string monster_type = "mon_test_mattack_dialog";
    monster &test_monster = spawn_test_monster( monster_type, attacker_location );
    test_monster.set_dest( you.get_location() );
    const mtype_special_attack &attack = test_monster.type->special_attacks.at( "test_conditions_1" );

    // Fail at first
    CHECK( !attack->call( test_monster ) );
    // Check the easier arm of the OR
    test_monster.add_effect( effect_took_flumed, 1_days );
    REQUIRE( test_monster.has_effect( effect_took_flumed ) );
    CHECK( attack->call( test_monster ) );

    // Reset the test, attack fails again
    test_monster.remove_effect( effect_took_flumed );
    REQUIRE( !attack->call( test_monster ) );

    // Check the nested AND arm
    REQUIRE( is_creature_outside( test_monster ) );
    you.set_mutation( trait_TOUGH_FEET );
    REQUIRE( you.has_flag( json_flag_TOUGH_FEET ) );
    CHECK( !attack->call( test_monster ) );
    test_monster.add_effect( effect_bleed, 1_days );
    REQUIRE( test_monster.has_effect( effect_bleed ) );
    // We now have all conditions specified in the attack
    CHECK( attack->call( test_monster ) );

    // Add disqualifying condition to target
    you.add_effect( effect_took_flumed, 1_days );
    REQUIRE( you.has_effect( effect_took_flumed ) );
    // Attack fails
    CHECK( !attack->call( test_monster ) );
}

TEST_CASE( "Targeted_grab_removal_test", "[mattack][grab]" )
{

    const std::string grabber_left = "mon_debug_grabber_left";
    const std::string grabber_right = "mon_debug_grabber_right";
    const tripoint target_location = attacker_location + tripoint_east;
    const tripoint attacker_location_e = target_location + tripoint_east;

    clear_map();
    clear_creatures();
    Character &you = get_player_character();
    clear_avatar();
    you.setpos( target_location );

    monster &test_monster_left = spawn_test_monster( grabber_left, attacker_location_e );
    monster &test_monster_right = spawn_test_monster( grabber_right, attacker_location );
    test_monster_left.set_dest( you.get_location() );
    test_monster_right.set_dest( you.get_location() );
    const mattack_actor &attack_left = test_monster_left.type->special_attacks.at( "grab" ).operator
                                       * ();
    const mattack_actor &attack_right = test_monster_right.type->special_attacks.at( "grab" ).operator
                                        * ();

    // Grabbed by both
    REQUIRE( attack_left.call( test_monster_left ) );
    REQUIRE( attack_right.call( test_monster_right ) );

    //Have two grabs, the monsters have the right filter effects
    REQUIRE( you.has_effect( effect_grabbed, body_part_arm_r ) );
    REQUIRE( you.has_effect( effect_grabbed, body_part_arm_l ) );
    REQUIRE( test_monster_right.has_effect( body_part_arm_r->grabbing_effect ) );
    REQUIRE( test_monster_left.has_effect( body_part_arm_l->grabbing_effect ) );

    // Kill the left grabber
    test_monster_left.die( nullptr );

    // Now we only have the one
    REQUIRE( you.has_effect( effect_grabbed, body_part_arm_r ) );
    REQUIRE( !you.has_effect( effect_grabbed, body_part_arm_l ) );
}

TEST_CASE( "Ranged_pull_tests", "[mattack][grab]" )
{
    // Set up further from the target
    const tripoint target_location = attacker_location + tripoint{ 4, 0, 0 };
    clear_map();
    restore_on_out_of_scope<time_point> restore_calendar_turn( calendar::turn );
    calendar::turn = daylight_time( calendar::turn ) + 2_hours;
    scoped_weather_override weather_clear( WEATHER_CLEAR );
    clear_creatures();
    Character &you = get_player_character();
    clear_avatar();
    you.setpos( target_location );
    REQUIRE( units::to_gram<int>( you.get_weight() ) > 50000 );

    SECTION( "Weak puller" ) {
        const std::string monster_type = "mon_debug_puller_weak";
        monster &test_monster = spawn_test_monster( monster_type, attacker_location );
        test_monster.set_dest( you.get_location() );
        REQUIRE( test_monster.sees( you ) );
        const mattack_actor &attack = test_monster.type->special_attacks.at( "ranged_pull" ).operator * ();
        REQUIRE( units::to_gram<int>( test_monster.get_weight() ) == 100000 );
        // Fail to pull our too-chonky survivor
        // Pull fails loudly so we can't count on it returning false
        REQUIRE( attack.call( test_monster ) );
        REQUIRE( you.pos() == target_location );
        // Reduce weight
        you.set_stored_kcal( 5000 );
        REQUIRE( units::to_gram<int>( you.get_weight() ) < 50000 );
        // Attack succeeds
        REQUIRE( attack.call( test_monster ) );
        REQUIRE( you.pos() == attacker_location + tripoint_east );
    }
    SECTION( "Strong puller" ) {
        const std::string monster_type = "mon_debug_puller_strong";
        monster &test_monster = spawn_test_monster( monster_type, attacker_location );
        test_monster.set_dest( you.get_location() );
        REQUIRE( test_monster.sees( you ) );
        const mattack_actor &attack = test_monster.type->special_attacks.at( "ranged_pull" ).operator * ();
        REQUIRE( units::to_gram<int>( test_monster.get_weight() ) == 100000 );
        // Pull on the first try
        REQUIRE( attack.call( test_monster ) );
        REQUIRE( you.pos() == attacker_location + tripoint_east );
    }
    SECTION( "Incompetent puller" ) {
        const std::string monster_type = "mon_debug_puller_incompetent";
        monster &test_monster = spawn_test_monster( monster_type, attacker_location );
        test_monster.set_dest( you.get_location() );
        REQUIRE( test_monster.sees( you ) );
        const mattack_actor &attack = test_monster.type->special_attacks.at( "ranged_pull" ).operator * ();
        // Can't pull, fail silently
        REQUIRE( !attack.call( test_monster ) );
        REQUIRE( you.pos() == target_location );
    }
    SECTION( "Pulls vs existing grabs" ) {
        const std::string monster_type = "mon_debug_puller_strong";
        const std::string grabber_type = "mon_debug_grabber_strong";
        monster &test_monster = spawn_test_monster( monster_type, attacker_location );
        monster &test_grabber = spawn_test_monster( grabber_type, target_location + tripoint_south );
        const mattack_actor &pull = test_monster.type->special_attacks.at( "ranged_pull" ).operator * ();
        const mattack_actor &grab = test_grabber.type->special_attacks.at( "grab" ).operator * ();
        test_monster.set_dest( you.get_location() );
        test_grabber.set_dest( you.get_location() );
        REQUIRE( test_monster.sees( you ) );
        REQUIRE( grab.call( test_grabber ) );
        int counter = 0;
        // Pull until the grabber lets go, it should eventually do so
        while( you.pos() == target_location && counter < 101 ) {
            REQUIRE( pull.call( test_monster ) );
            counter++;
        }
        REQUIRE( counter < 100 );
    }
}


TEST_CASE( "Grab_breaks_against_weak_grabbers", "[mattack][grab]" )
{
    const tripoint target_location = attacker_location + tripoint_east;
    const tripoint attacker_location_n = target_location + tripoint_north;
    const tripoint attacker_location_e = target_location + tripoint_east;
    clear_map();
    clear_creatures();
    Character &you = get_player_character();
    clear_avatar();
    you.setpos( target_location );

    // Setup monsters adjacently
    const std::string monster_type = "mon_debug_grabber";
    monster &test_monster = spawn_test_monster( monster_type, attacker_location );
    monster &test_monster_e = spawn_test_monster( monster_type, attacker_location_e );
    monster &test_monster_n = spawn_test_monster( monster_type, attacker_location_n );
    test_monster.set_dest( you.get_location() );
    test_monster_e.set_dest( you.get_location() );
    test_monster_n.set_dest( you.get_location() );
    const mattack_actor &attack = test_monster.type->special_attacks.at( "grab" ).operator * ();
    const mattack_actor &attack_e = test_monster_e.type->special_attacks.at( "grab" ).operator * ();
    const mattack_actor &attack_n = test_monster.type->special_attacks.at( "grab" ).operator * ();

    REQUIRE( test_monster.get_grab_strength() == 20 );
    REQUIRE( you.get_str() == 8 );
    REQUIRE( you.get_skill_level( skill_unarmed ) == 0 );

    SECTION( "Starter character vs one weak grabber" ) {
        int success = 0;
        // Start grabbed
        REQUIRE( attack.call( test_monster ) );
        // Safety check to ensure grab strength is read and applied correctly
        for( const effect &grab : you.get_effects_with_flag( json_flag_GRAB ) ) {
            CHECK( grab.get_intensity() == 20 );
        }
        for( int i = 0; i < 1000; i++ ) {
            if( you.try_remove_grab() ) {
                // Record success and reapply grab
                success++;
                you.clear_effects();
                you.set_stamina( you.get_stamina_max() );
                REQUIRE( attack.call( test_monster ) );
            }
        }
        Messages::clear_messages();
        // Single zombie grabbing a starting survivor has about 50% chance to release on a turn
        // Actually higher for non-arm/torso grabs, but bundled works well enough
        INFO( "You should have about 40% chance to break a weak grab every turn as a starting character" );
        CHECK( success == Approx( 400 ).margin( test_margin ) );
    }
    SECTION( "Starter character vs 3 weak grabs at the same time" ) {
        // Setup 3v1 test
        you.clear_effects();
        test_monster.clear_effects();
        you.set_stamina( you.get_stamina_max() );

        int success = 0;
        // Start grabbed
        REQUIRE( attack.call( test_monster ) );
        REQUIRE( attack_e.call( test_monster_e ) );
        REQUIRE( attack_n.call( test_monster_n ) );
        for( int i = 0; i < 1000; i++ ) {
            REQUIRE( you.get_effects_with_flag( json_flag_GRAB ).size() == 3 );
            if( you.try_remove_grab() ) {
                // Record success if all grabs are broken
                success++;
            }
            // Reapply grabs
            you.clear_effects();
            test_monster.clear_effects();
            test_monster_e.clear_effects();
            test_monster_n.clear_effects();
            you.set_stamina( you.get_stamina_max() );
            REQUIRE( attack.call( test_monster ) );
            REQUIRE( attack_e.call( test_monster_e ) );
            REQUIRE( attack_n.call( test_monster_n ) );
        }
        Messages::clear_messages();
        INFO( "You have about 5% chance to break three weak grabs on a single turn" );
        CHECK( success == Approx( 50 ).margin( test_margin ) );
    }
    // Setup lategame character
    you.set_skill_level( skill_unarmed, 8 );
    you.martial_arts_data->set_style( style_brawling, true );
    REQUIRE( you.get_skill_level( skill_unarmed ) == 8 );
    REQUIRE( you.has_grab_break_tec() );

    you.clear_effects();
    you.set_stamina( you.get_stamina_max() );
    test_monster.clear_effects();
    test_monster_e.clear_effects();
    test_monster_n.clear_effects();
    SECTION( "Lategame character against 1 weak grabber" ) {
        int success = 0;
        REQUIRE( attack.call( test_monster ) );
        for( int i = 0; i < 1000; i++ ) {
            if( you.try_remove_grab() ) {
                // Record success and reapply grab
                success++;
                you.clear_effects();
                you.set_stamina( you.get_stamina_max() );
                REQUIRE( attack.call( test_monster ) );
            }
        }
        Messages::clear_messages();
        INFO( "100% chance to break a weak grab as a lategame character with grab breaks" );
        CHECK( success == Approx( 1000 ).margin( test_margin ) );
    }
    SECTION( "Lategame character vs 3 weak grabs at the same time" ) {
        // Setup 3v1 test
        you.clear_effects();
        test_monster.clear_effects();
        you.set_stamina( you.get_stamina_max() );

        int success = 0;
        // Start grabbed
        REQUIRE( attack.call( test_monster ) );
        REQUIRE( attack_e.call( test_monster_e ) );
        REQUIRE( attack_n.call( test_monster_n ) );
        for( int i = 0; i < 1000; i++ ) {
            REQUIRE( you.get_effects_with_flag( json_flag_GRAB ).size() == 3 );
            if( you.try_remove_grab() ) {
                // Record success if all grabs are broken
                success++;
            }
            // Reapply grabs
            you.clear_effects();
            you.set_stamina( you.get_stamina_max() );
            test_monster.clear_effects();
            test_monster_e.clear_effects();
            test_monster_n.clear_effects();
            REQUIRE( attack.call( test_monster ) );
            REQUIRE( attack_e.call( test_monster_e ) );
            REQUIRE( attack_n.call( test_monster_n ) );
        }
        Messages::clear_messages();
        INFO( "99% chance to break three weak grabs on a single turn" );
        CHECK( success == Approx( 990 ).margin( test_margin ) );
    }
}

TEST_CASE( "Grab_breaks_against_midline_grabbers", "[mattack][grab]" )
{
    const tripoint target_location = attacker_location + tripoint_east;
    const tripoint attacker_location_n = target_location + tripoint_north;
    const tripoint attacker_location_e = target_location + tripoint_east;
    clear_map();
    clear_creatures();
    Character &you = get_player_character();
    clear_avatar();
    you.setpos( target_location );

    // Setup monsters adjacently
    const std::string monster_type = "mon_debug_grabber_mid";
    monster &test_monster = spawn_test_monster( monster_type, attacker_location );
    monster &test_monster_e = spawn_test_monster( monster_type, attacker_location_e );
    monster &test_monster_n = spawn_test_monster( monster_type, attacker_location_n );
    test_monster.set_dest( you.get_location() );
    test_monster_e.set_dest( you.get_location() );
    test_monster_n.set_dest( you.get_location() );
    const mattack_actor &attack = test_monster.type->special_attacks.at( "grab" ).operator * ();
    const mattack_actor &attack_e = test_monster_e.type->special_attacks.at( "grab" ).operator * ();
    const mattack_actor &attack_n = test_monster.type->special_attacks.at( "grab" ).operator * ();

    REQUIRE( test_monster.get_grab_strength() == 50 );
    REQUIRE( you.get_str() == 8 );
    REQUIRE( you.get_skill_level( skill_unarmed ) == 0 );

    SECTION( "Starter character vs one midline grabber" ) {
        int success = 0;
        // Start grabbed
        REQUIRE( attack.call( test_monster ) );
        // Safety check to ensure grab strength is read and applied correctly
        for( const effect &grab : you.get_effects_with_flag( json_flag_GRAB ) ) {
            CHECK( grab.get_intensity() == 50 );
        }
        for( int i = 0; i < 1000; i++ ) {
            if( you.try_remove_grab() ) {
                success++;
                you.clear_effects();
                you.set_stamina( you.get_stamina_max() );
                REQUIRE( attack.call( test_monster ) );
            }
        }
        Messages::clear_messages();
        INFO( "You should have about 15% chance to break a mid-strength grab every turn as a starting character" );
        CHECK( success == Approx( 150 ).margin( test_margin ) );
    }
    SECTION( "Starter character vs 3 mid grabs at the same time" ) {
        // Setup 3v1 test
        you.clear_effects();
        test_monster.clear_effects();
        you.set_stamina( you.get_stamina_max() );

        int success = 0;
        // Start grabbed
        REQUIRE( attack.call( test_monster ) );
        REQUIRE( attack_e.call( test_monster_e ) );
        REQUIRE( attack_n.call( test_monster_n ) );
        for( int i = 0; i < 1000; i++ ) {
            REQUIRE( you.get_effects_with_flag( json_flag_GRAB ).size() == 3 );
            if( you.try_remove_grab() ) {
                // Record success if all grabs are broken
                success++;
            }
            // Reapply grabs
            you.clear_effects();
            test_monster.clear_effects();
            test_monster_e.clear_effects();
            test_monster_n.clear_effects();
            you.set_stamina( you.get_stamina_max() );
            REQUIRE( attack.call( test_monster ) );
            REQUIRE( attack_e.call( test_monster_e ) );
            REQUIRE( attack_n.call( test_monster_n ) );
        }
        Messages::clear_messages();
        INFO( "You have about 5% chance to break three midline grabs on a single turn" );
        CHECK( success == Approx( 50 ).margin( test_margin ) );
    }
    // Setup lategame character
    you.set_skill_level( skill_unarmed, 8 );
    you.martial_arts_data->set_style( style_brawling, true );
    REQUIRE( you.get_skill_level( skill_unarmed ) == 8 );
    REQUIRE( you.has_grab_break_tec() );

    you.clear_effects();
    you.set_stamina( you.get_stamina_max() );
    test_monster.clear_effects();
    test_monster_e.clear_effects();
    test_monster_n.clear_effects();
    SECTION( "Lategame character against 1 midline grabber" ) {
        int success = 0;
        REQUIRE( attack.call( test_monster ) );
        for( int i = 0; i < 1000; i++ ) {
            if( you.try_remove_grab() ) {
                // Record success and reapply grab
                success++;
                you.clear_effects();
                you.set_stamina( you.get_stamina_max() );
                REQUIRE( attack.call( test_monster ) );
            }
        }
        Messages::clear_messages();
        INFO( "80% chance to break a midline grab as a lategame character with grab breaks" );
        CHECK( success == Approx( 800 ).margin( test_margin ) );
    }
    SECTION( "Lategame character vs 3 mid grabs at the same time" ) {
        // Setup 3v1 test
        you.clear_effects();
        test_monster.clear_effects();
        you.set_stamina( you.get_stamina_max() );

        int success = 0;
        // Start grabbed
        REQUIRE( attack.call( test_monster ) );
        REQUIRE( attack_e.call( test_monster_e ) );
        REQUIRE( attack_n.call( test_monster_n ) );
        for( int i = 0; i < 1000; i++ ) {
            REQUIRE( you.get_effects_with_flag( json_flag_GRAB ).size() == 3 );
            if( you.try_remove_grab() ) {
                // Record success if all grabs are broken
                success++;
            }
            // Reapply grabs
            you.clear_effects();
            you.set_stamina( you.get_stamina_max() );
            test_monster.clear_effects();
            test_monster_e.clear_effects();
            test_monster_n.clear_effects();
            REQUIRE( attack.call( test_monster ) );
            REQUIRE( attack_e.call( test_monster_e ) );
            REQUIRE( attack_n.call( test_monster_n ) );
        }
        Messages::clear_messages();
        INFO( "25% chance to break three mid grabs on a single turn" );
        CHECK( success == Approx( 250 ).margin( test_margin ) );
    }
}

TEST_CASE( "Grab_breaks_against_strong_grabbers", "[mattack][grab]" )
{
    const tripoint target_location = attacker_location + tripoint_east;
    const tripoint attacker_location_n = target_location + tripoint_north;
    const tripoint attacker_location_e = target_location + tripoint_east;
    clear_map();
    clear_creatures();
    Character &you = get_player_character();
    clear_avatar();
    you.setpos( target_location );

    const std::string monster_type = "mon_debug_grabber_strong";
    monster &test_monster = spawn_test_monster( monster_type, attacker_location );
    monster &test_monster_e = spawn_test_monster( monster_type, attacker_location_e );
    monster &test_monster_n = spawn_test_monster( monster_type, attacker_location_n );
    test_monster.set_dest( you.get_location() );
    test_monster_e.set_dest( you.get_location() );
    test_monster_n.set_dest( you.get_location() );
    const mattack_actor &attack = test_monster.type->special_attacks.at( "grab" ).operator * ();
    const mattack_actor &attack_e = test_monster_e.type->special_attacks.at( "grab" ).operator * ();
    const mattack_actor &attack_n = test_monster.type->special_attacks.at( "grab" ).operator * ();

    REQUIRE( test_monster.get_grab_strength() == 100 );
    REQUIRE( you.get_str() == 8 );
    REQUIRE( you.get_skill_level( skill_unarmed ) == 0 );

    SECTION( "Starter character vs one strong grabber" ) {
        int success = 0;
        // Start grabbed
        REQUIRE( attack.call( test_monster ) );
        // Safety check to ensure grab strength is read and applied correctly
        for( const effect &grab : you.get_effects_with_flag( json_flag_GRAB ) ) {
            CHECK( grab.get_intensity() == 100 );
        }
        for( int i = 0; i < 1000; i++ ) {
            if( you.try_remove_grab() ) {
                success++;
                you.clear_effects();
                you.set_stamina( you.get_stamina_max() );
                REQUIRE( attack.call( test_monster ) );
            }
        }
        Messages::clear_messages();
        INFO( "You should have about 7,5% chance to break a max-strength grab every turn as a starting character" );
        CHECK( success == Approx( 75 ).margin( test_margin ) );
    }
    SECTION( "Starter character vs 3 max-strength grabs at the same time" ) {
        you.clear_effects();
        test_monster.clear_effects();
        you.set_stamina( you.get_stamina_max() );

        int success = 0;
        // Start grabbed
        REQUIRE( attack.call( test_monster ) );
        REQUIRE( attack_e.call( test_monster_e ) );
        REQUIRE( attack_n.call( test_monster_n ) );
        for( int i = 0; i < 1000; i++ ) {
            REQUIRE( you.get_effects_with_flag( json_flag_GRAB ).size() == 3 );
            if( you.try_remove_grab() ) {
                success++;
            }
            you.clear_effects();
            test_monster.clear_effects();
            test_monster_e.clear_effects();
            test_monster_n.clear_effects();
            you.set_stamina( you.get_stamina_max() );
            REQUIRE( attack.call( test_monster ) );
            REQUIRE( attack_e.call( test_monster_e ) );
            REQUIRE( attack_n.call( test_monster_n ) );
        }
        Messages::clear_messages();
        INFO( "You have about 0% chance to break three max-strength grabs on a single turn" );
        CHECK( success == Approx( 0 ).margin( test_margin / 2 ) );
    }

    you.set_skill_level( skill_unarmed, 8 );
    you.martial_arts_data->set_style( style_brawling, true );
    REQUIRE( you.get_skill_level( skill_unarmed ) == 8 );
    REQUIRE( you.has_grab_break_tec() );

    you.clear_effects();
    you.set_stamina( you.get_stamina_max() );
    test_monster.clear_effects();
    test_monster_e.clear_effects();
    test_monster_n.clear_effects();
    SECTION( "Lategame character against 1 max-strength grabber" ) {
        int success = 0;
        REQUIRE( attack.call( test_monster ) );
        for( int i = 0; i < 1000; i++ ) {
            if( you.try_remove_grab() ) {
                success++;
                you.clear_effects();
                you.set_stamina( you.get_stamina_max() );
                REQUIRE( attack.call( test_monster ) );
            }
        }
        Messages::clear_messages();
        INFO( "40% chance to break a max-strength grab as a lategame character with grab breaks" );
        CHECK( success == Approx( 400 ).margin( test_margin ) );
    }
    SECTION( "Lategame character vs 3 max-strength grabs at the same time" ) {
        you.clear_effects();
        test_monster.clear_effects();
        you.set_stamina( you.get_stamina_max() );

        int success = 0;
        REQUIRE( attack.call( test_monster ) );
        REQUIRE( attack_e.call( test_monster_e ) );
        REQUIRE( attack_n.call( test_monster_n ) );
        for( int i = 0; i < 1000; i++ ) {
            REQUIRE( you.get_effects_with_flag( json_flag_GRAB ).size() == 3 );
            if( you.try_remove_grab() ) {
                success++;
            }
            you.clear_effects();
            you.set_stamina( you.get_stamina_max() );
            test_monster.clear_effects();
            test_monster_e.clear_effects();
            test_monster_n.clear_effects();
            REQUIRE( attack.call( test_monster ) );
            REQUIRE( attack_e.call( test_monster_e ) );
            REQUIRE( attack_n.call( test_monster_n ) );
        }
        Messages::clear_messages();
        INFO( "5% chance to break three max-strength grabs on a single turn" );
        CHECK( success == Approx( 50 ).margin( test_margin ) );
    }
}
