#include <algorithm>
#include <array>
#include <list>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "catch/catch.hpp"
#include "avatar.h"
#include "calendar.h"
#include "game.h"
#include "map.h"
#include "map_helpers.h"
#include "npc.h"
#include "item.h"
#include "item_location.h"
#include "player.h"
#include "player_helpers.h"
#include "point.h"
#include "ranged.h"
#include "vehicle.h"

extern bool can_fire_turret( avatar &you, const map &m, const turret_data &turret );

static constexpr tripoint shooter_pos( 60, 60, 0 );

static void set_up_player_vision()
{
    g->place_player( shooter_pos );
    g->u.worn.clear();
    g->reset_light_level();

    REQUIRE( !g->u.is_blind() );
    REQUIRE( !g->u.in_sleep_state() );

    g->u.recalc_sight_limits();

    calendar::turn = calendar::turn_zero + 12_hours;

    g->m.update_visibility_cache( shooter_pos.z );
    g->m.invalidate_map_cache( shooter_pos.z );
    g->m.build_map_cache( shooter_pos.z );
    g->m.update_visibility_cache( shooter_pos.z );
    g->m.invalidate_map_cache( shooter_pos.z );
    g->m.build_map_cache( shooter_pos.z );
}

TEST_CASE( "Aiming at a clearly visible target", "[ranged][aiming]" )
{
    clear_map();
    set_up_player_vision();
    player &shooter = g->u;
    arm_character( shooter, "glock_19" );
    int max_range = shooter.weapon.gun_range( &shooter );
    REQUIRE( max_range >= 10 );
    REQUIRE( max_range < 30 );

    WHEN( "The target is within range" ) {
        monster &z = spawn_test_monster( "debug_mon", shooter_pos + point( 5, 0 ) );
        THEN( "The target is in targetable creatures" ) {
            std::vector<Creature *> t = ranged::targetable_creatures( shooter, max_range );
            CHECK( std::count( t.begin(), t.end(), &z ) > 0 );
        }
    }

    WHEN( "The target is outside range" ) {
        monster &z = spawn_test_monster( "debug_mon", shooter_pos + point( 30, 0 ) );
        THEN( "The target is not in targetable creatures" ) {
            std::vector<Creature *> t = ranged::targetable_creatures( shooter, max_range );
            CHECK( std::count( t.begin(), t.end(), &z ) == 0 );
        }
    }

    WHEN( "Multiple targets are within range" ) {
        constexpr int num_creatures = 5;
        for( int x = 0; x < num_creatures; x++ ) {
            spawn_test_monster( "debug_mon", shooter_pos + point( 5 + x, 0 ) );
        }
        THEN( "All are targetable" ) {
            std::vector<Creature *> t = ranged::targetable_creatures( shooter, max_range );
            CHECK( t.size() == num_creatures );
        }
    }

    WHEN( "One target is within range, one is outside it" ) {
        monster &z1 = spawn_test_monster( "debug_mon", shooter_pos + point( 5, 0 ) );
        monster &z2 = spawn_test_monster( "debug_mon", shooter_pos + point( 30, 0 ) );
        THEN( "Only the target within range is targetable" ) {
            std::vector<Creature *> t = ranged::targetable_creatures( shooter, max_range );
            CHECK( t.size() == 1 );
            CHECK( std::count( t.begin(), t.end(), &z1 ) == 1 );
            CHECK( std::count( t.begin(), t.end(), &z2 ) == 0 );
        }
    }
}

TEST_CASE( "Aiming at a target behind wall", "[ranged][aiming]" )
{
    clear_map();
    player &shooter = g->u;
    clear_character( shooter, true );
    shooter.add_effect( efftype_id( "debug_clairvoyance" ), time_duration::from_seconds( 1 ) );
    arm_character( shooter, "glock_19" );
    int max_range = shooter.weapon.gun_range( &shooter );
    REQUIRE( max_range >= 5 );
    for( int y_off = -1; y_off <= 1; y_off++ ) {
        g->m.ter_set( shooter_pos + point( 1, y_off ), t_wall );
    }

    set_up_player_vision();
    monster &z = spawn_test_monster( "debug_mon", shooter_pos + point( 2, 0 ) );
    WHEN( "There is no direct, passable line to target" ) {
        const auto path = g->m.find_clear_path( shooter.pos(), z.pos() );
        int impassable_tiles = std::count_if( path.begin(), path.end(),
        []( const tripoint & p ) {
            return g->m.impassable( p );
        } );
        REQUIRE( impassable_tiles >= 1 );
        AND_WHEN( "All the tiles on the most direct line are opaque" ) {
            int non_transparent_tiles = std::count_if( path.begin(), path.end(),
            []( const tripoint & p ) {
                return !g->m.is_transparent( p );
            } );
            REQUIRE( non_transparent_tiles > 0 );
            AND_WHEN( "The shooter sees the target due to a non-vision effect" ) {

                REQUIRE( shooter.sees( z ) );
                THEN( "The target is not in targetable creatures" ) {
                    std::vector<Creature *> t = ranged::targetable_creatures( shooter, max_range );
                    CHECK( std::count( t.begin(), t.end(), &z ) == 0 );
                }
            }
        }
    }
}

TEST_CASE( "Aiming at a target behind bars", "[ranged][aiming]" )
{
    clear_map();
    set_up_player_vision();
    player &shooter = g->u;
    arm_character( shooter, "glock_19" );
    int max_range = shooter.weapon.gun_range( &shooter );
    REQUIRE( max_range >= 5 );
    for( int y_off = -1; y_off <= 1; y_off++ ) {
        g->m.ter_set( shooter_pos + point( 1, y_off ), t_window_bars );
    }
    monster &z = spawn_test_monster( "debug_mon", shooter_pos + point( 2, 0 ) );
    WHEN( "There is no direct, passable line to target" ) {
        const auto path = g->m.find_clear_path( shooter.pos(), z.pos() );
        int impassable_tiles = std::count_if( path.begin(), path.end(),
        []( const tripoint & p ) {
            return g->m.impassable( p );
        } );
        REQUIRE( impassable_tiles >= 1 );
        // Transparent and NOT hardcoded to absorb bullets (t_window etc.)
        AND_WHEN( "All the tiles on the most direct line are transparent" ) {
            int non_transparent_tiles = std::count_if( path.begin(), path.end(),
            []( const tripoint & p ) {
                return !g->m.is_transparent( p );
            } );
            REQUIRE( non_transparent_tiles == 0 );
            THEN( "The target is in targetable creatures" ) {
                std::vector<Creature *> t = ranged::targetable_creatures( shooter, max_range );
                CHECK( std::count( t.begin(), t.end(), &z ) > 0 );
            }
        }
    }
}

TEST_CASE( "Aiming a turret from a solid vehicle", "[ranged][aiming]" )
{
    clear_map();
    set_up_player_vision();
    avatar &shooter = g->u;
    shooter.setpos( shooter_pos );
    arm_character( shooter, "glock_19" );
    int max_range = shooter.weapon.gun_range( &shooter );
    REQUIRE( max_range >= 5 );

    monster &z = spawn_test_monster( "debug_mon", shooter_pos + point( 5, 0 ) );

    const auto path = g->m.find_clear_path( shooter.pos(), z.pos() );
    int impassable_tiles_before = std::count_if( path.begin(), path.end(),
    []( const tripoint & p ) {
        return g->m.impassable( p );
    } );
    REQUIRE( impassable_tiles_before == 0 );

    vehicle *veh = g->m.add_vehicle( vproto_id( "turret_test" ), shooter_pos, 0, 100, 0, false );
    REQUIRE( veh != nullptr );

    WHEN( "Shooter's line of fire becomes blocked by vehicle's windshield" ) {
        int impassable_tiles_after = std::count_if( path.begin(), path.end(),
        []( const tripoint & p ) {
            return g->m.impassable( p );
        } );
        REQUIRE( impassable_tiles_after >= 1 );
        REQUIRE( shooter.sees( z ) );
        AND_WHEN( "All the blocking tiles are impassable only because of the vehicle" ) {
            int non_vehicle_blocking_tiles = std::count_if( path.begin(), path.end(),
            [&veh]( const tripoint & p ) {
                return g->m.move_cost( p, veh ) == 0;
            } );
            REQUIRE( non_vehicle_blocking_tiles == 0 );
            AND_WHEN( "The shooter aims the turret" ) {
                turret_data turret = veh->turret_query( shooter_pos );
                REQUIRE( static_cast<bool>( turret ) );
                REQUIRE( turret.query() == turret_data::status::ready );
                REQUIRE( can_fire_turret( shooter, g->m, turret ) );
                THEN( "The list of targets inclues the target" ) {
                    std::vector<Creature *> t = ranged::targetable_creatures( shooter, max_range, turret );
                    CHECK( std::count( t.begin(), t.end(), &z ) > 0 );
                }
            }
        }
    }
}

TEST_CASE( "Aiming at a target partially covered by a wall", "[.][ranged][aiming][slow]" )
{
    clear_map();
    standard_npc shooter( "Shooter", shooter_pos, {}, 0, 8, 8, 8, 8 );
    arm_character( shooter, "win70" );
    int max_range = shooter.weapon.gun_range( &shooter );
    REQUIRE( max_range >= 55 );

    int unseen = 0;
    std::vector<std::pair<tripoint, tripoint>> failed;

    for( int rot = 0; rot < 4; rot++ ) {
        for( int x = 5; x < 30; x++ ) {
            for( int y = 5; y < 30; y++ ) {
                point wall_offset = point( x, y ).rotate( rot, {0, 0} );
                const tripoint wall_pos = shooter_pos + wall_offset;
                g->m.ter_set( wall_pos, t_wall );
                point mon_offset = point( x, y + 1 ).rotate( rot, {0, 0} );
                const tripoint monster_pos = shooter_pos + mon_offset;
                monster &z = spawn_test_monster( "debug_mon", monster_pos );
                if( !shooter.sees( z ) ) {
                    // TODO: Use player for this, so that this isn't needed
                    unseen++;
                    continue;
                }
                const auto path = g->m.find_clear_path( shooter.pos(), z.pos() );
                std::vector<Creature *> t = ranged::targetable_creatures( shooter, max_range );
                if( std::count( t.begin(), t.end(), &z ) == 0 ) {
                    failed.emplace_back( wall_pos, monster_pos );
                }

                g->m.ter_set( wall_pos, t_dirt );
                clear_creatures();
            }
        }
    }

    CAPTURE( unseen );
    CAPTURE( failed );
    CHECK( failed.empty() );
    CHECK( unseen == 0 );
}
