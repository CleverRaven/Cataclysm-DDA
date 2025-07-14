#include <functional>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "cata_catch.h"
#include "character.h"
#include "character_attire.h"
#include "coordinates.h"
#include "creature_tracker.h"
#include "field.h"
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

class monster;

// TODO: map::bash doesn't handle fields
/*
static const field_type_str_id field_fd_bash_bothfield( "fd_bash_bothfield" );
static const field_type_str_id field_fd_bash_destroyfield( "fd_bash_destroyfield" );
static const field_type_str_id field_fd_bash_hitfield( "fd_bash_hitfield" );
static const field_type_str_id field_fd_bash_nofield( "fd_bash_nofield" );
*/
static const field_type_str_id field_fd_marker1( "fd_marker1" );
static const field_type_str_id field_fd_marker2( "fd_marker2" );

static const furn_str_id furn_test_f_bash_bothfield( "test_f_bash_bothfield" );
static const furn_str_id furn_test_f_bash_destroyfield( "test_f_bash_destroyfield" );
static const furn_str_id furn_test_f_bash_hitfield( "test_f_bash_hitfield" );
static const furn_str_id furn_test_f_bash_nofield( "test_f_bash_nofield" );
static const furn_str_id furn_test_f_bash_persist( "test_f_bash_persist" );
static const furn_str_id furn_test_f_eoc( "test_f_eoc" );

static const itype_id itype_backpack( "backpack" );
static const itype_id itype_mossberg_590( "mossberg_590" );
static const itype_id itype_shot_bird( "shot_bird" );

static const ter_str_id ter_t_floor( "t_floor" );
static const ter_str_id ter_test_t_bash_bothfield( "test_t_bash_bothfield" );
static const ter_str_id ter_test_t_bash_destroyfield( "test_t_bash_destroyfield" );
static const ter_str_id ter_test_t_bash_hitfield( "test_t_bash_hitfield" );
static const ter_str_id ter_test_t_bash_nofield( "test_t_bash_nofield" );
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

static void test_bash_fields( const std::function<void( map &, const tripoint_bub_ms & )> &place,
                              const std::function<bool( map &, const tripoint_bub_ms & )> &present,
                              const std::function<std::pair<int, int>()> &bash_limits,
                              bool hit_field, field_type_str_id hit, int hit_intensity,
                              bool destroy_field, field_type_str_id destroy, int destroy_intensity )
{
    clear_map();
    map &here = get_map();

    const tripoint_bub_ms pt_1( 30, 30, 0 );
    const tripoint_bub_ms pt_2( pt_1 + point::east );
    const tripoint_bub_ms pt_3( pt_2 + point::east );

    // so we can do a hit that does no damage
    REQUIRE( bash_limits().first > 2 );
    // can hit and do damage without breaking
    REQUIRE( bash_limits().first + 2 < bash_limits().second );

    place( here, pt_1 );
    place( here, pt_2 );
    place( here, pt_3 );

    REQUIRE( present( here, pt_1 ) );
    REQUIRE( present( here, pt_2 ) );
    REQUIRE( present( here, pt_3 ) );
    // no fields
    REQUIRE( here.field_at( pt_1 ).find_field( hit ) == nullptr );
    REQUIRE( here.field_at( pt_1 ).find_field( destroy ) == nullptr );
    REQUIRE( here.field_at( pt_2 ).find_field( hit ) == nullptr );
    REQUIRE( here.field_at( pt_2 ).find_field( destroy ) == nullptr );
    REQUIRE( here.field_at( pt_3 ).find_field( hit ) == nullptr );
    REQUIRE( here.field_at( pt_3 ).find_field( destroy ) == nullptr );

    // weak hit, no damage
    {
        here.bash( pt_1, bash_limits().first - 1 );

        CHECK( present( here, pt_1 ) );
        const field &fields = here.field_at( pt_1 );
        const field_entry *fd_here = fields.find_field( hit );
        if( hit_field ) {
            REQUIRE( fd_here != nullptr );
            CHECK( fd_here->get_field_intensity() == hit_intensity );
        } else {
            CHECK( fd_here == nullptr );
        }
    }

    // hit with damage
    {
        here.bash( pt_2, bash_limits().first + 1 );

        CHECK( present( here, pt_2 ) );
        const field &fields = here.field_at( pt_2 );
        const field_entry *fd_here = fields.find_field( hit );
        if( hit_field ) {
            REQUIRE( fd_here != nullptr );
            CHECK( fd_here->get_field_intensity() == hit_intensity );
        } else {
            CHECK( fd_here == nullptr );
        }
    }

    // destroying hit
    {
        here.bash( pt_3, bash_limits().second + 1 );

        CHECK_FALSE( present( here, pt_3 ) );
        const field &fields = here.field_at( pt_3 );
        const field_entry *fd_hit_here = fields.find_field( hit );
        const field_entry *fd_destroy_here = fields.find_field( destroy );
        if( hit_field ) {
            REQUIRE( fd_hit_here != nullptr );
            CHECK( fd_hit_here->get_field_intensity() == hit_intensity );
        } else {
            CHECK( fd_hit_here == nullptr );
        }
        if( destroy_field ) {
            REQUIRE( fd_destroy_here != nullptr );
            CHECK( fd_destroy_here->get_field_intensity() == destroy_intensity );
        } else {
            CHECK( fd_destroy_here == nullptr );
        }
    }
}

static void test_furniture_fields( furn_str_id furn,
                                   bool hit_field, field_type_str_id hit, int hit_intensity,
                                   bool destroy_field, field_type_str_id destroy, int destroy_intensity )
{
    CAPTURE( furn );
    const auto place = [&furn]( map & here, const tripoint_bub_ms & pt ) {
        here.furn_set( pt, furn );
    };
    const auto present = [&furn]( map & here, const tripoint_bub_ms & pt ) {
        return here.furn( pt ) == furn;
    };
    const auto bash_limits = [&furn]() {
        return std::make_pair( furn->bash->str_min, furn->bash->str_max );
    };
    REQUIRE( furn.is_valid() );
    REQUIRE( furn->bash );
    test_bash_fields( place, present, bash_limits,
                      hit_field, hit, hit_intensity,
                      destroy_field, destroy, destroy_intensity );
}

static void test_terrain_fields( ter_str_id ter,
                                 bool hit_field, field_type_str_id hit, int hit_intensity,
                                 bool destroy_field, field_type_str_id destroy, int destroy_intensity )
{
    CAPTURE( ter );
    const auto place = [&ter]( map & here, const tripoint_bub_ms & pt ) {
        here.ter_set( pt, ter );
    };
    const auto present = [&ter]( map & here, const tripoint_bub_ms & pt ) {
        return here.ter( pt ) == ter;
    };
    const auto bash_limits = [&ter]() {
        return std::make_pair( ter->bash->str_min, ter->bash->str_max );
    };
    REQUIRE( ter.is_valid() );
    REQUIRE( ter->bash );
    test_bash_fields( place, present, bash_limits,
                      hit_field, hit, hit_intensity,
                      destroy_field, destroy, destroy_intensity );
}

// TODO: map::bash doesn't handle fields
/*
static void test_field_fields( field_type_str_id fd,
                               bool hit_field, field_type_str_id hit, int hit_intensity,
                               bool destroy_field, field_type_str_id destroy, int destroy_intensity )
{
    CAPTURE( fd );
    const auto place = [&fd]( map & here, const tripoint_bub_ms & pt ) {
        REQUIRE( here.add_field( pt, fd, 1 ) );
    };
    const auto present = [&fd]( map & here, const tripoint_bub_ms & pt ) {
        return here.field_at( pt ).find_field( fd ) != nullptr;
    };
    const auto bash_limits = [&fd]() {
        return std::make_pair( fd->bash_info->str_min, fd->bash_info->str_max );
    };
    REQUIRE( fd.is_valid() );
    REQUIRE( fd->bash_info );
    test_bash_fields( place, present, bash_limits,
                      hit_field, hit, hit_intensity,
                      destroy_field, destroy, destroy_intensity );
}
*/

TEST_CASE( "map_bash_create_fields", "[map][bash]" )
{
    SECTION( "furniture" ) {
        test_furniture_fields( furn_test_f_bash_nofield,
                               false, field_type_str_id::NULL_ID(), 0,
                               false, field_type_str_id::NULL_ID(), 0 );
        test_furniture_fields( furn_test_f_bash_hitfield,
                               true, field_fd_marker1, 2,
                               false, field_type_str_id::NULL_ID(), 0 );
        test_furniture_fields( furn_test_f_bash_destroyfield,
                               false, field_type_str_id::NULL_ID(), 0,
                               true, field_fd_marker1, 1 );
        test_furniture_fields( furn_test_f_bash_bothfield,
                               true, field_fd_marker2, 1,
                               true, field_fd_marker1, 2 );
    }

    SECTION( "terrain" ) {
        test_terrain_fields( ter_test_t_bash_nofield,
                             false, field_type_str_id::NULL_ID(), 0,
                             false, field_type_str_id::NULL_ID(), 0 );
        test_terrain_fields( ter_test_t_bash_hitfield,
                             true, field_fd_marker2, 1,
                             false, field_type_str_id::NULL_ID(), 0 );
        test_terrain_fields( ter_test_t_bash_destroyfield,
                             false, field_type_str_id::NULL_ID(), 0,
                             true, field_fd_marker2, 2 );
        test_terrain_fields( ter_test_t_bash_bothfield,
                             true, field_fd_marker1, 2,
                             true, field_fd_marker2, 1 );
    }

    // TODO: map::bash doesn't handle fields
    /*
        SECTION( "fields" ) {
            test_field_fields( field_fd_bash_nofield,
                               false, field_type_str_id::NULL_ID(), 0,
                               false, field_type_str_id::NULL_ID(), 0 );
            test_field_fields( field_fd_bash_hitfield,
                               true, field_fd_marker2, 1,
                               false, field_type_str_id::NULL_ID(), 0 );
            test_field_fields( field_fd_bash_destroyfield,
                               false, field_type_str_id::NULL_ID(), 0,
                               true, field_fd_marker2, 2 );
            test_field_fields( field_fd_bash_bothfield,
                               true, field_fd_marker1, 2,
                               true, field_fd_marker2, 1 );
        }
        */
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

static void shoot_at_terrain( std::function<void()> &setup, npc &shooter,
                              const std::string &ter_str, tripoint_bub_ms wall_pos,
                              bool expected_to_break, tripoint_bub_ms aim_pos = tripoint_bub_ms::zero )
{
    map &here = get_map();
    ter_str_id id{ ter_str };
    if( aim_pos == tripoint_bub_ms::zero ) {
        aim_pos = wall_pos;
    }

    int num_trials = 20;
    int broken_count = 0;
    for( int i = 0; i < num_trials; i++ ) {
        // Place a terrain
        here.ter_set( wall_pos, id );

        setup();

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
        broken_count += here.ter( wall_pos ) != id;
    }
    if( expected_to_break ) {
        CHECK( broken_count > 0.8 * num_trials );
    } else {
        CHECK( broken_count < 0.2 * num_trials );
    }

}

TEST_CASE( "shooting_at_terrain", "[rng][map][bash][ranged]" )
{
    clear_map();

    // Make a shooter
    standard_npc shooter( "Shooter", { 30, 30, 0 } );
    shooter.set_body();
    shooter.worn.wear_item( shooter, item( itype_backpack ), false, false );
    SECTION( "birdshot vs brick wall near" ) {
        std::function<void()> setup = [&shooter]() {
            arm_shooter( shooter, itype_mossberg_590, {}, itype_shot_bird );
        };
        shoot_at_terrain( setup, shooter, "t_brick_wall",
                          shooter.pos_bub() + point::east, false );
    }
    // Broken. See https://github.com/CleverRaven/Cataclysm-DDA/issues/79770
    //SECTION( "birdshot vs adobe wall point blank" ) {
    //    shoot_at_terrain( shooter, itype_mossberg_590, itype_shot_bird,
    //                      "t_adobe_brick_wall", shooter.pos_bub() + point::east, false );
    //}
    SECTION( "birdshot vs adobe wall near" ) {
        std::function<void()> setup = [&shooter]() {
            arm_shooter( shooter, itype_mossberg_590, {}, itype_shot_bird );
        };
        shoot_at_terrain( setup, shooter, "t_adobe_brick_wall",
                          shooter.pos_bub() + point::east * 2, false );
    }
    SECTION( "birdshot vs opaque glass door point blank" ) {
        std::function<void()> setup = [&shooter]() {
            arm_shooter( shooter, itype_mossberg_590, {}, itype_shot_bird );
        };
        shoot_at_terrain( setup, shooter, "test_t_door_glass_opaque_c",
                          shooter.pos_bub() + point::east, true );
    }
    SECTION( "birdshot vs opaque glass door near" ) {
        std::function<void()> setup = [&shooter]() {
            arm_shooter( shooter, itype_mossberg_590, {}, itype_shot_bird );
        };
        shoot_at_terrain( setup, shooter, "test_t_door_glass_opaque_c",
                          shooter.pos_bub() + point::east * 2, false );
    }
    SECTION( "birdshot vs door near" ) {
        std::function<void()> setup = [&shooter]() {
            arm_shooter( shooter, itype_mossberg_590, {}, itype_shot_bird );
        };
        shoot_at_terrain( setup, shooter, "t_door_c",
                          shooter.pos_bub() + point::east * 2, false );
    }
    // I thought I saw some failures based on whether an unseen monster was present,
    // But I think it was just shooting at door wthout a 100% chance to break it and getting unlucky.
    SECTION( "birdshot through door at nothing" ) {
        std::function<void()> setup = [&shooter]() {
            arm_shooter( shooter, itype_mossberg_590, {}, itype_shot_bird );
        };
        shoot_at_terrain( setup, shooter, "t_door_c",
                          shooter.pos_bub() + point::east, true,
                          shooter.pos_bub() + point::east * 2 );
    }
    SECTION( "birdshot through door at monster" ) {
        std::function<void()> setup = [&shooter]() {
            arm_shooter( shooter, itype_mossberg_590, {}, itype_shot_bird );
            if( get_creature_tracker().creature_at<monster>( shooter.pos_bub() + point::east * 2 ) ==
                nullptr ) {
                spawn_test_monster( "mon_zombie", shooter.pos_bub() + point::east * 2 );
            }
        };
        shoot_at_terrain( setup, shooter, "t_door_c",
                          shooter.pos_bub() + point::east, true,
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
