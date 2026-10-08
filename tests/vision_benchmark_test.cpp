#include <cstdlib>
#include <initializer_list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "avatar_action.h"
#include "calendar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "enums.h"
#include "game.h"
#include "level_cache.h"
#include "map.h"
#include "map_helpers.h"
#include "map_scale_constants.h"
#include "mdarray.h"
#include "monster.h"
#include "options_helpers.h"
#include "overmap_ui.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"
#include "units.h"
#include "vehicle.h"

static const move_mode_id move_mode_crouch( "crouch" );
static const move_mode_id move_mode_walk( "walk" );

static const mtype_id mon_test_camera( "mon_test_camera" );
static const mtype_id mon_zombie( "mon_zombie" );

static const ter_str_id ter_t_brick_wall( "t_brick_wall" );
static const ter_str_id ter_t_door_c( "t_door_c" );
static const ter_str_id ter_t_flat_roof( "t_flat_roof" );
static const ter_str_id ter_t_floor( "t_floor" );
static const ter_str_id ter_t_utility_light( "t_utility_light" );
static const ter_str_id ter_t_window_frame( "t_window_frame" );

static const vproto_id vehicle_prototype_vehicle_camera_test( "vehicle_camera_test" );

static const weather_type_id weather_clear( "clear" );
static const weather_type_id weather_fog( "fog" );

// Builds against the map API from before and after the vision cache rework, so
// the same file measures the baseline and the result.

// what do_turn invalidates each turn: the per-turn light hook where the map
// has one, else every level's light and all final visibility
namespace
{
struct turn_fallback {};
struct turn_preferred : turn_fallback {};
} // namespace

template<typename Map>
static auto start_turn( Map &here, turn_preferred ) -> decltype( here.mark_turn_light_dirty(),
        void() )
{
    here.mark_turn_light_dirty();
}

template<typename Map>
static void start_turn( Map &here, turn_fallback )
{
    for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; ++z ) {
        here.set_lightmap_cache_dirty( z );
    }
    here.invalidate_visibility_cache();
}

static void build_vision()
{
    map &here = get_map();
    here.build_map_cache( 0 );
    here.update_visibility_cache( 0 );
}

static void rebuild_vision_from_scratch()
{
    map &here = get_map();
    for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; ++z ) {
        here.invalidate_map_cache( z );
    }
    g->reset_light_level();
    here.build_map_cache( 0 );
    here.invalidate_visibility_cache();
    here.update_visibility_cache( 0 );
}

// a field with a lit house, a roof light one level up, a moncam and a door
// far out of sight
static void set_up_open_scene( const tripoint_bub_ms &origin )
{
    clear_avatar();
    clear_map_without_vision( -2, OVERMAP_HEIGHT );
    clear_vehicles();
    g->place_player( origin );
    // placing the avatar can load monsters, and safe mode would stop the moves
    clear_creatures();
    g->set_safe_mode( SAFE_MODE_OFF );
    calendar::turn = calendar::turn_zero + 21_hours;
    g->reset_light_level();
    map &here = get_map();
    const tripoint_bub_ms house = origin + tripoint_rel_ms{ 4, -2, 0 };
    for( int dx = 0; dx < 5; ++dx ) {
        for( int dy = 0; dy < 5; ++dy ) {
            const tripoint_bub_ms p = house + tripoint_rel_ms{ dx, dy, 0 };
            const bool edge = dx == 0 || dy == 0 || dx == 4 || dy == 4;
            here.ter_set( p, edge ? ter_t_brick_wall : ter_t_floor );
            here.ter_set( p + tripoint::above, ter_t_flat_roof );
        }
    }
    here.ter_set( house + tripoint_rel_ms{ 0, 2, 0 }, ter_t_door_c );
    here.ter_set( house + tripoint_rel_ms{ 2, 4, 0 }, ter_t_window_frame );
    here.ter_set( house + tripoint_rel_ms{ 2, 2, 0 }, ter_t_utility_light );
    here.ter_set( house + tripoint_rel_ms{ 2, 2, 1 }, ter_t_utility_light );
    const tripoint_bub_ms far_room = origin + tripoint_rel_ms{ 30, 30, 0 };
    for( int dx = -2; dx <= 2; ++dx ) {
        for( int dy = -2; dy <= 2; ++dy ) {
            const tripoint_bub_ms p = far_room + tripoint_rel_ms{ dx, dy, 0 };
            const bool edge = std::abs( dx ) == 2 || std::abs( dy ) == 2;
            here.ter_set( p, edge ? ter_t_brick_wall : ter_t_floor );
            here.ter_set( p + tripoint::above, ter_t_flat_roof );
        }
    }
    here.ter_set( far_room, ter_t_door_c );
    monster *camera = g->place_critter_at( mon_test_camera, origin + tripoint_rel_ms{ -5, 5, 0 } );
    REQUIRE( camera != nullptr );
    camera->friendly = -1;
    get_avatar().add_moncam( { mon_test_camera, 60 } );
    rebuild_vision_from_scratch();
    rebuild_vision_from_scratch();
}

TEST_CASE( "vision_cache_benchmark", "[.][vision][benchmark]" )
{
    const tripoint_bub_ms origin{ 64, 64, 0 };
    map &here = get_map();
    avatar &you = get_avatar();
    set_up_open_scene( origin );
    const tripoint_bub_ms near_door = origin + tripoint_rel_ms{ 4, 0, 0 };
    const tripoint_bub_ms far_door = origin + tripoint_rel_ms{ 30, 30, 0 };
    std::vector<std::pair<tripoint_bub_ms, tripoint_bub_ms>> pairs;
    for( int dx = -20; dx <= 20; dx += 2 ) {
        for( int dy = -20; dy <= 20; dy += 2 ) {
            pairs.emplace_back( origin + tripoint_rel_ms{ dx, dy, 0 },
                                origin + tripoint_rel_ms{ -dy, dx, 0 } );
        }
    }

    // step case measures movement only if moves happen
    REQUIRE( avatar_action::move( you, here, tripoint_rel_ms::east ) );
    REQUIRE( you.pos_bub() == origin + tripoint::east );
    REQUIRE( avatar_action::move( you, here, tripoint_rel_ms::west ) );
    REQUIRE( you.pos_bub() == origin );
    build_vision();

    // offscreen case measures a door no cast reaches only if none does
    REQUIRE( here.access_cache( 0 ).seen_cache[far_door.x()][far_door.y()] <= 0.0f );
    REQUIRE( here.access_cache( 0 ).camera_cache[far_door.x()][far_door.y()] <= 0.0f );
    // the weather case measures a change only if fog and clear differ here
    const tripoint_bub_ms open_ground = origin + tripoint_rel_ms{ -3, 3, 0 };
    float clear_transparency = 0.0f;
    {
        scoped_weather_override clear( weather_clear );
        build_vision();
        clear_transparency = here.access_cache( 0 ).transparency_cache[open_ground.x()][open_ground.y()];
    }
    {
        scoped_weather_override fog( weather_fog );
        build_vision();
        REQUIRE( here.access_cache( 0 ).transparency_cache[open_ground.x()][open_ground.y()] !=
                 clear_transparency );
    }
    scoped_weather_override clear( weather_clear );
    build_vision();

    BENCHMARK( "stationary_build" ) {
        build_vision();
    };
    BENCHMARK( "step_east_then_west" ) {
        avatar_action::move( you, here, tripoint_rel_ms::east );
        build_vision();
        avatar_action::move( you, here, tripoint_rel_ms::west );
        build_vision();
    };
    BENCHMARK( "crouch_then_stand" ) {
        you.set_movement_mode( move_mode_crouch );
        build_vision();
        you.set_movement_mode( move_mode_walk );
        build_vision();
    };
    BENCHMARK( "door_in_view_open_then_close" ) {
        here.open_door( you, near_door, true );
        build_vision();
        here.close_door( near_door, true, false );
        build_vision();
    };
    // the same transitions with the refresh master needs to show them; on
    // master the plain cases skip that work and leave stale caches
    BENCHMARK( "crouch_then_stand_with_refresh" ) {
        you.set_movement_mode( move_mode_crouch );
        here.invalidate_visibility_cache();
        build_vision();
        you.set_movement_mode( move_mode_walk );
        here.invalidate_visibility_cache();
        build_vision();
    };
    BENCHMARK( "door_in_view_open_then_close_with_refresh" ) {
        here.open_door( you, near_door, true );
        here.invalidate_visibility_cache();
        build_vision();
        here.close_door( near_door, true, false );
        here.invalidate_visibility_cache();
        build_vision();
    };
    BENCHMARK( "door_offscreen_open_then_close" ) {
        here.open_door( you, far_door, true );
        build_vision();
        here.close_door( far_door, true, false );
        build_vision();
    };
    BENCHMARK( "weather_fog_then_clear" ) {
        {
            scoped_weather_override fog( weather_fog );
            build_vision();
        }
        scoped_weather_override back_to_clear( weather_clear );
        build_vision();
    };
    BENCHMARK( "sees_batch" ) {
        int seen = 0;
        for( const std::pair<tripoint_bub_ms, tripoint_bub_ms> &pr : pairs ) {
            seen += here.sees( pr.first, pr.second, -1 ) ? 1 : 0;
        }
        return seen;
    };
    BENCHMARK( "rebuild_from_scratch" ) {
        rebuild_vision_from_scratch();
    };
}

// creatures on three floors: the cache upkeep of a turn in which nothing changes,
// and their sight checks after light and visibility were all marked dirty
TEST_CASE( "vision_cache_creature_sight_benchmark", "[.][vision][benchmark]" )
{
    const tripoint_bub_ms origin{ 64, 64, 0 };
    map &here = get_map();
    avatar &you = get_avatar();
    set_up_open_scene( origin );
    const tripoint_bub_ms cellar = origin + tripoint_rel_ms{ -12, -12, -1 };
    for( int dx = 0; dx < 9; ++dx ) {
        for( int dy = 0; dy < 9; ++dy ) {
            here.ter_set( cellar + tripoint_rel_ms{ dx, dy, 0 }, ter_t_floor );
        }
    }
    here.ter_set( cellar + tripoint_rel_ms{ 4, 4, 0 }, ter_t_utility_light );
    const tripoint_bub_ms roof = origin + tripoint_rel_ms{ 5, -1, 1 };
    std::vector<monster *> watchers;
    for( const tripoint_bub_ms &p : {
             cellar + tripoint_rel_ms{ 1, 1, 0 }, cellar + tripoint_rel_ms{ 7, 7, 0 },
             roof, roof + tripoint_rel_ms{ 2, 2, 0 },
             origin + tripoint_rel_ms{ -10, 8, 0 }, origin + tripoint_rel_ms{ 10, 10, 0 }
         } ) {
        monster *m = g->place_critter_at( mon_zombie, p );
        REQUIRE( m != nullptr );
        watchers.push_back( m );
    }
    rebuild_vision_from_scratch();

    BENCHMARK( "turn_with_creatures_on_three_floors" ) {
        start_turn( here, turn_preferred{} );
        build_vision();
    };
    BENCHMARK( "creatures_on_three_floors_look_around" ) {
        for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; ++z ) {
            here.set_lightmap_cache_dirty( z );
        }
        here.invalidate_visibility_cache();
        build_vision();
        int seen = 0;
        for( const monster *a : watchers ) {
            seen += you.sees( here, *a ) ? 1 : 0;
            for( const monster *b : watchers ) {
                seen += a != b && a->sees( here, *b ) ? 1 : 0;
            }
        }
        return seen;
    };
}

TEST_CASE( "vision_cache_vehicle_camera_benchmark", "[.][vision][benchmark]" )
{
    const tripoint_bub_ms origin{ 64, 64, 0 };
    clear_avatar();
    clear_map_without_vision( -2, OVERMAP_HEIGHT );
    clear_vehicles();
    map &here = get_map();
    vehicle *v = here.add_vehicle( vehicle_prototype_vehicle_camera_test, origin, 0_degrees, 0,
                                   veh_spawn_status::UNDAMAGED );
    REQUIRE( v != nullptr );
    v->camera_on = true;
    g->place_player( origin );
    calendar::turn = calendar::turn_zero + 21_hours;
    rebuild_vision_from_scratch();
    rebuild_vision_from_scratch();

    BENCHMARK( "stationary_build_in_vehicle" ) {
        build_vision();
    };
    BENCHMARK( "camera_off_then_on" ) {
        v->camera_on = false;
        build_vision();
        v->camera_on = true;
        build_vision();
    };
    // what vehicle menu does after a toggle
    BENCHMARK( "camera_off_then_on_with_refresh" ) {
        v->camera_on = false;
        here.invalidate_map_cache( 0 );
        here.invalidate_visibility_cache();
        build_vision();
        v->camera_on = true;
        here.invalidate_map_cache( 0 );
        here.invalidate_visibility_cache();
        build_vision();
    };
    clear_vehicles();
}
