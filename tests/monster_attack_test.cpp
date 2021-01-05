#include "catch/catch.hpp"

#include "character.h"
#include "monster.h"

#include "map.h"

#include "map_helpers.h"

static constexpr tripoint attacker_location{ 65, 65, 0 };

static void test_monster_attack( const tripoint &target_offset, bool expected )
{
    clear_creatures();
    // Monster adjacent to target.
    const std::string monster_type = "mon_zombie";
    const tripoint target_location = attacker_location + target_offset;
    Character &you = get_player_character();
    you.setpos( target_location );
    monster &test_monster = spawn_test_monster( monster_type, attacker_location );
    // Trigger basic attack.
    CAPTURE( attacker_location );
    CAPTURE( target_location );
    CHECK( test_monster.attack_at( target_location ) == expected );
    // Then test the reverse.
    clear_creatures();
    you.setpos( attacker_location );
    monster &target_monster = spawn_test_monster( monster_type, target_location );
    CHECK( you.melee_attack( target_monster, false ) == expected );
}

static void monster_attack_zlevel( const std::string &title, const tripoint &offset,
                                   const std::string &monster_ter, const std::string &target_ter,
                                   bool expected )
{
    map &here = get_map();
    SECTION( title ) {
        here.ter_set( attacker_location, ter_id( monster_ter ) );
        here.ter_set( attacker_location + offset, ter_id( target_ter ) );
        test_monster_attack( offset, expected );
        for( const tripoint &more_offset : eight_horizontal_neighbors ) {
            here.ter_set( attacker_location + offset + more_offset, ter_id( "t_floor" ) );
            test_monster_attack( offset + more_offset, false );
        }
    }
}

TEST_CASE( "monster_attack" )
{
    clear_map();
    SECTION( "attacking on open ground" ) {
        // Adjacent can attack of course.
        for( const tripoint &offset : eight_horizontal_neighbors ) {
            test_monster_attack( offset, true );
        }
        // Too far away cannot.
        test_monster_attack( { 2, 2, 0 }, false );
        test_monster_attack( { 2, 1, 0 }, false );
        test_monster_attack( { 2, 0, 0 }, false );
        test_monster_attack( { 2, -1, 0 }, false );
        test_monster_attack( { 2, -2, 0 }, false );
        test_monster_attack( { 1, 2, 0 }, false );
        test_monster_attack( { 1, -2, 0 }, false );
        test_monster_attack( { 0, 2, 0 }, false );
        test_monster_attack( { 0, -2, 0 }, false );
        test_monster_attack( { -1, 2, 0 }, false );
        test_monster_attack( { -1, -2, 0 }, false );
        test_monster_attack( { -2, 2, 0 }, false );
        test_monster_attack( { -2, 1, 0 }, false );
        test_monster_attack( { -2, 0, 0 }, false );
        test_monster_attack( { -2, -1, 0 }, false );
        test_monster_attack( { -2, -2, 0 }, false );
    }

    monster_attack_zlevel( "attack_up_stairs", tripoint_above, "t_stairs_up", "t_stairs_down", true );
    monster_attack_zlevel( "attack_down_stairs", tripoint_below, "t_stairs_down", "t_stairs_up", true );
    monster_attack_zlevel( "attack through ceiling", tripoint_above, "t_floor", "t_floor", false );
    monster_attack_zlevel( "attack through floor", tripoint_below, "t_floor", "t_floor", false );

    monster_attack_zlevel( "attack up legde", tripoint_above, "t_floor", "t_floor", false );
    monster_attack_zlevel( "attack down legde", tripoint_below, "t_floor", "t_floor", false );
}
