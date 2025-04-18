#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "cata_catch.h"
#include "character.h"
#include "character_attire.h"
#include "coordinates.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "mapdata.h"
#include "npc.h"
#include "player_helpers.h"
#include "point.h"
#include "string_formatter.h"
#include "test_data.h"
#include "type_id.h"

static const furn_str_id furn_test_f_bash_persist( "test_f_bash_persist" );
static const furn_str_id furn_test_f_eoc( "test_f_eoc" );

static const itype_id itype_backpack( "backpack" );
static const itype_id itype_mossberg_590( "mossberg_590" );
static const itype_id itype_shot_bird( "shot_bird" );

static const ter_str_id ter_t_floor( "t_floor" );
static const ter_str_id ter_test_t_bash_persist( "test_t_bash_persist" );
static const ter_str_id ter_test_t_pit_shallow( "test_t_pit_shallow" );

void bash_test_loadout::apply( Character &guy ) const
{
    clear_character( guy );

    guy.str_max = strength;
    guy.reset_stats();
    REQUIRE( guy.str_cur == strength );

    for( const itype_id &it : worn ) {
        REQUIRE( guy.wear_item( item( it ), false, false ).has_value() );
    }
    guy.calc_encumbrance();

    if( wielded.has_value() ) {
        item to_wield( wielded.value() );
        REQUIRE( guy.wield( to_wield ) );
    }

    REQUIRE( guy.smash_ability() == expected_smash_ability );
}

static void test_bash_set( const bash_test_set &set )
{
    map &here = get_map();
    Character &guy = get_player_character();
    // Arbitrary point on the map
    const tripoint_bub_ms test_pt( 40, 40, 0 );

    for( const single_bash_test &test : set.tests ) {
        test.loadout.apply( guy );

        constexpr int max_tries = 999;
        for( const furn_id &furn : set.tested_furn ) {
            INFO( string_format( "%s bashing %s", test.id, furn.id().str() ) );
            here.ter_set( test_pt, ter_t_floor );
            here.furn_set( test_pt, furn );
            int tries = 0;
            while( here.furn( test_pt ) == furn && tries < max_tries ) {
                ++tries;
                here.bash( test_pt, guy.smash_ability() );
            }
            auto it = test.furn_tries.find( furn );
            if( it == test.furn_tries.end() ) {
                CHECK( tries == max_tries );
            } else {
                CHECK( tries >= it->second.first );
                CHECK( tries <= it->second.second );
            }
        }
        for( const ter_id &ter : set.tested_ter ) {
            INFO( string_format( "%s bashing %s", test.id, ter.id().str() ) );
            here.ter_set( test_pt, ter );
            here.furn_set( test_pt, furn_str_id::NULL_ID() );
            int tries = 0;
            while( here.ter( test_pt ) == ter && tries < max_tries ) {
                ++tries;
                here.bash( test_pt, guy.smash_ability() );
            }
            auto it = test.ter_tries.find( ter );
            if( it == test.ter_tries.end() ) {
                CHECK( tries == max_tries );
            } else {
                CHECK( tries >= it->second.first );
                CHECK( tries <= it->second.second );
            }
        }
    }
}

TEST_CASE( "map_bash_chances", "[map][bash]" )
{
    clear_map();

    for( const bash_test_set &set : test_data::bash_tests ) {
        test_bash_set( set );
    }
}

TEST_CASE( "map_bash_ephemeral_persistence", "[map][bash]" )
{
    clear_map();
    map &here = get_map();

    const tripoint_bub_ms test_pt( 40, 40, 0 );

    // Assumptions
    REQUIRE( furn_test_f_bash_persist->bash->str_min == 4 );
    REQUIRE( furn_test_f_bash_persist->bash->str_max == 100 );
    REQUIRE( ter_test_t_bash_persist->bash->str_min == 4 );
    REQUIRE( ter_test_t_bash_persist->bash->str_max == 100 );

    SECTION( "bashing a furniture to completion leaves behind no map bash info" ) {
        here.furn_set( test_pt, furn_test_f_bash_persist );

        REQUIRE( here.furn( test_pt ) == furn_test_f_bash_persist );
        REQUIRE( here.get_map_damage( test_pt ) == 0 );
        // One above str_min, but well below str_max
        here.bash( test_pt, 5 );
        // Does not destroy it
        CHECK( here.furn( test_pt ) == furn_test_f_bash_persist );
        // There is any map damage
        CHECK( here.get_map_damage( test_pt ) > 0 );
        // Bash it again to destroy it
        here.bash( test_pt, 999 );
        CHECK( here.furn( test_pt ) != furn_test_f_bash_persist );
        // Then, it is gone and there is no map damage
        CHECK( here.furn( test_pt ) != furn_test_f_bash_persist );
        CHECK( here.get_map_damage( test_pt ) == 0 );
    }
    SECTION( "bashing a terrain to completion leaves behind no map bash info" ) {
        here.ter_set( test_pt, ter_test_t_bash_persist );

        REQUIRE( here.ter( test_pt ) == ter_test_t_bash_persist );
        REQUIRE( here.get_map_damage( test_pt ) == 0 );
        // One above str_min, but well below str_max
        here.bash( test_pt, 5 );
        // Does not destroy the terrain
        CHECK( here.ter( test_pt ) == ter_test_t_bash_persist );
        // There is any map damage
        CHECK( here.get_map_damage( test_pt ) > 0 );
        // Bash it again to destroy it
        here.bash( test_pt, 999 );
        CHECK( here.ter( test_pt ) != ter_test_t_bash_persist );
        // Then, it is gone and there is no map damage
        CHECK( here.ter( test_pt ) != ter_test_t_bash_persist );
        CHECK( here.get_map_damage( test_pt ) == 0 );
    }
    SECTION( "when a terrain changes, map damage is lost" ) {
        here.ter_set( test_pt, ter_test_t_bash_persist );

        REQUIRE( here.ter( test_pt ) == ter_test_t_bash_persist );
        REQUIRE( here.get_map_damage( test_pt ) == 0 );
        // One above str_min, but well below str_max
        here.bash( test_pt, 5 );
        // Does not destroy the terrain
        CHECK( here.ter( test_pt ) == ter_test_t_bash_persist );
        // There is any map damage
        CHECK( here.get_map_damage( test_pt ) > 0 );
        // Change the terrain
        here.ter_set( test_pt, ter_test_t_pit_shallow );
        CHECK( here.ter( test_pt ) == ter_test_t_pit_shallow );
        CHECK( here.get_map_damage( test_pt ) == 0 );
    }
    SECTION( "when a furniture changes, map damage is lost" ) {
        here.furn_set( test_pt, furn_test_f_bash_persist );

        REQUIRE( here.furn( test_pt ) == furn_test_f_bash_persist );
        REQUIRE( here.get_map_damage( test_pt ) == 0 );
        // One above str_min, but well below str_max
        here.bash( test_pt, 5 );
        // Does not destroy the terrain
        CHECK( here.furn( test_pt ) == furn_test_f_bash_persist );
        // There is any map damage
        CHECK( here.get_map_damage( test_pt ) > 0 );
        // Change the terrain
        here.furn_set( test_pt, furn_test_f_eoc );
        CHECK( here.furn( test_pt ) == furn_test_f_eoc );
        CHECK( here.get_map_damage( test_pt ) == 0 );
    }
}

static void shoot_at_terrain( npc &shooter, const std::string &ter_str, tripoint_bub_ms wall_pos,
                              bool expected_to_break, tripoint_bub_ms aim_pos = tripoint_bub_ms::zero )
{
    map &here = get_map();
    // Place a terrain
    ter_str_id id{ ter_str };
    if( aim_pos == tripoint_bub_ms::zero ) {
        aim_pos = wall_pos;
    }
    here.ter_set( wall_pos, id );
    REQUIRE( here.ter( wall_pos ) == id );
    // This is a workaround for nonsense where you can't shoot terrain or furniture unless it
    // obscures sight, specifically map::is_transparent() must return false.
    here.build_map_cache( 0, true );

    // Shoot it a bunch
    for( int i = 0; i < 9; ++i ) {
        shooter.recoil = 0;
        shooter.set_moves( 100 );
        shooter.fire_gun( aim_pos );
    }
    // is it gone?
    INFO( here.ter( wall_pos ).id().str() );
    CHECK( ( here.ter( wall_pos ) != id ) == expected_to_break );
}

TEST_CASE( "shooting_at_terrain", "[map][bash][ranged]" )
{
    clear_map();

    // Make a shooter
    standard_npc shooter( "Shooter", { 10, 10, 0 } );
    shooter.set_body();
    shooter.worn.wear_item( shooter, item( itype_backpack ), false, false );
    SECTION( "birdshot vs adobe wall point blank" ) {
        arm_shooter( shooter, itype_mossberg_590, {}, itype_shot_bird );
        shoot_at_terrain( shooter, "t_adobe_brick_wall", shooter.pos_bub() + point::east, false );
    }
    SECTION( "birdshot vs adobe wall near" ) {
        arm_shooter( shooter, itype_mossberg_590, {}, itype_shot_bird );
        shoot_at_terrain( shooter, "t_adobe_brick_wall", shooter.pos_bub() + point::east * 2, false );
    }
    SECTION( "birdshot vs opaque glass door point blank" ) {
        arm_shooter( shooter, itype_mossberg_590, {}, itype_shot_bird );
        shoot_at_terrain( shooter, "test_t_door_glass_opaque_c", shooter.pos_bub() + point::east, true );
    }
    SECTION( "birdshot vs opaque glass door near" ) {
        arm_shooter( shooter, itype_mossberg_590, {}, itype_shot_bird );
        shoot_at_terrain( shooter, "test_t_door_glass_opaque_c", shooter.pos_bub() + point::east * 2,
                          false );
    }
    SECTION( "birdshot vs door near" ) {
        arm_shooter( shooter, itype_mossberg_590, {}, itype_shot_bird );
        shoot_at_terrain( shooter, "t_door_c", shooter.pos_bub() + point::east * 2, false );
    }
    // I thought I saw some failures based on whether an unseen monster was present,
    // But I think it was just shooting at door wthout a 100% chance to break it and getting unlucky.
    SECTION( "birdshot through door at nothing" ) {
        arm_shooter( shooter, itype_mossberg_590, {}, itype_shot_bird );
        shoot_at_terrain( shooter, "t_door_c", shooter.pos_bub() + point::east, true,
                          shooter.pos_bub() + point::east * 2 );
    }
    SECTION( "birdshot through door at monster" ) {
        arm_shooter( shooter, itype_mossberg_590, {}, itype_shot_bird );
        spawn_test_monster( "mon_zombie", shooter.pos_bub() + point::east * 2 );
        shoot_at_terrain( shooter, "t_door_c", shooter.pos_bub() + point::east, true,
                          shooter.pos_bub() + point::east * 2 );
    }
    // TODO: If we get a feature where damage accumulates a test for it would go here.
    // These are failing because you can't shoot transparent terrain.
    /*
    SECTION( "birdshot vs glass door point blank" ) {
        arm_shooter( shooter, itype_mossberg_590, {}, itype_shot_bird );
        shoot_at_terrain( shooter, "t_door_glass_c", shooter.pos_bub() + point::east, true );
    }
    SECTION( "birdshot vs glass door near" ) {
        arm_shooter( shooter, itype_mossberg_590, {}, itype_shot_bird );
        shoot_at_terrain( shooter, "t_door_glass_c", shooter.pos_bub() + point::east * 2, false );
    }
    SECTION( "birdshot vs screen door point blank" ) {
        arm_shooter( shooter, itype_mossberg_590, {}, itype_shot_bird );
        shoot_at_terrain( shooter, "t_screen_door_c", shooter.pos_bub() + point::east, true );
    }
    SECTION( "birdshot vs screen door near" ) {
        arm_shooter( shooter, itype_mossberg_590, {}, itype_shot_bird );
        shoot_at_terrain( shooter, "t_screen_door_c", shooter.pos_bub() + point::east * 2, false );
    }
    */
}
