#include <array>
#include <iosfwd>

#include "cached_options.h"
#include "calendar.h"
#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "character.h"
#include "line.h"
#include "map.h"
#include "map_helpers.h"
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

static const itype_id itype_rock( "rock" );

static constexpr tripoint attacker_location{ 65, 65, 0 };

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
    you.dodges_left = 1;
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
            you.dodges_left = 1;
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
