#include "map_helpers_tests.h"

#include <algorithm>
#include <array>
#include <bitset>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "character_attire.h"
#include "coordinates.h"
#include "creature_tracker.h"
#include "game.h"
#include "game_constants.h"
#include "item.h"
#include "level_cache.h"
#include "lightmap.h"
#include "map.h"
#include "map_helpers.h"
#include "map_iterator.h"
#include "map_scale_constants.h"
#include "mdarray.h"
#include "monster.h"
#include "pocket_type.h"
#include "point.h"
#include "ret_val.h"
#include "shadowcasting.h"
#include "string_formatter.h"
#include "submap.h"
#include "type_id.h"

static const itype_id itype_blindfold( "blindfold" );
static const itype_id itype_medium_battery_cell( "medium_battery_cell" );
static const itype_id itype_wearable_light_on( "wearable_light_on" );

static const ter_str_id ter_t_open_air( "t_open_air" );
static const ter_str_id ter_t_rock( "t_rock" );

monster &spawn_test_monster( const std::string &monster_type, const tripoint_bub_ms &start,
                             const bool death_drops )
{
    mtype_id type( monster_type );
    REQUIRE( !type.is_null() );
    REQUIRE( get_creature_tracker().creature_at( start ) == nullptr );
    monster mon( type );
    map &here = get_map();
    CAPTURE( here.ter( start ) );
    CAPTURE( here.furn( start ) );
    CAPTURE( here.tr_at( start ) );
    CAPTURE( here.move_cost( start ) );
    REQUIRE( mon.will_move_to( start ) );
    REQUIRE( mon.know_danger_at( start ) );

    monster *const test_monster_ptr = g->place_critter_at( type, start );
    REQUIRE( test_monster_ptr );
    test_monster_ptr->death_drops = death_drops;
    return *test_monster_ptr;
}

// Build a map of size MAPSIZE_X x MAPSIZE_Y around tripoint::zero with a given
// terrain, and no furniture, traps, or items.
void build_test_map( const ter_id &terrain )
{
    map &here = get_map();
    for( const tripoint_bub_ms &p : here.points_in_rectangle( tripoint_bub_ms::zero,
            tripoint_bub_ms( MAPSIZE * SEEX, MAPSIZE * SEEY, 0 ) ) ) {
        here.furn_set( p, furn_id( "f_null" ) );
        here.ter_set( p, terrain );
        here.trap_set( p, trap_id( "tr_null" ) );
        here.i_clear( p );
    }

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );
}

void build_water_test_map( const ter_id &surface, const ter_id &mid, const ter_id &bottom )
{
    constexpr int z_surface = 0;
    constexpr int z_bottom = -2;

    clear_map_with_vision( z_bottom - 1, z_surface + 1, /* with_vision = */ false );

    map &here = get_map();
    const tripoint_bub_ms p1( 0, 0, z_bottom - 1 );
    const tripoint_bub_ms p2( MAPSIZE * SEEX, MAPSIZE * SEEY, z_surface + 1 );
    for( const tripoint_bub_ms &p : here.points_in_rectangle( p1, p2 ) ) {

        if( p.z() == z_surface ) {
            here.ter_set( p, surface );
        } else if( p.z() < z_surface && p.z() > z_bottom ) {
            here.ter_set( p, mid );
        } else if( p.z() == z_bottom ) {
            here.ter_set( p, bottom );
        } else if( p.z() < z_bottom ) {
            here.ter_set( p, ter_t_rock );
        } else if( p.z() > z_surface ) {
            here.ter_set( p, ter_t_open_air );
        }
    }

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );
}

void player_add_headlamp()
{
    item headlamp( itype_wearable_light_on );
    item battery( itype_medium_battery_cell );
    battery.ammo_set( battery.ammo_default(), -1 );
    headlamp.put_in( battery, pocket_type::MAGAZINE_WELL );
    Character &you = get_player_character();
    you.worn.wear_item( you, headlamp, false, true );
}

void player_wear_blindfold()
{
    item blindfold( itype_blindfold );
    Character &you = get_player_character();
    you.worn.wear_item( you, blindfold, false, true );
}

void set_time_to_day()
{
    time_point noon = calendar::turn - time_past_midnight( calendar::turn ) + 12_hours;
    if( noon < calendar::turn ) {
        noon = noon + 1_days;
    }
    set_time( noon );
}

// Set current time of day, and refresh map and caches for the new light level
void set_time( const time_point &time )
{
    calendar::turn = time;
    g->reset_light_level();
    Character &you = get_player_character();
    int z = you.posz();
    you.recalc_sight_limits();
    map &here = get_map();
    here.invalidate_visibility_cache();
    here.update_visibility_cache( z );
    here.invalidate_map_cache( z );
    here.build_map_cache( z );
}

los_pairs los_pairs_around( const tripoint_bub_ms &center, const int radius )
{
    los_pairs pairs;
    for( int dx = -radius; dx <= radius; ++dx ) {
        for( int dy = -radius; dy <= radius; ++dy ) {
            if( std::max( std::abs( dx ), std::abs( dy ) ) == radius ) {
                pairs.emplace_back( center, center + tripoint_rel_ms{ dx, dy, 0 } );
            }
        }
    }
    return pairs;
}

namespace
{
struct vision_cache_snapshot {
    std::map<int, std::unique_ptr<level_cache_default_zero_members>> levels;
    // per pair: optical and physical trace, each with and without fields
    std::vector<std::array<bool, 4>> sees;
    std::vector<int> vision_levels;
    uint64_t seen_generation = 0;
    std::map<int, std::pair<uint64_t, uint64_t>> level_generations;
    std::map<int, bool> has_colored_lights;
};
} // namespace

static std::array<bool, 4> sees_all_traces( const map &here, const tripoint_bub_ms &from,
        const tripoint_bub_ms &to )
{
    return { here.sees( from, to, -1, true, los_trace::optical ),
             here.sees( from, to, -1, false, los_trace::optical ),
             here.sees( from, to, -1, true, los_trace::physical ),
             here.sees( from, to, -1, false, los_trace::physical ) };
}

static vision_cache_snapshot snapshot_vision_caches( const los_pairs &pairs )
{
    const map &here = get_map();
    vision_cache_snapshot snap;
    for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; ++z ) {
        // any reader may ask any level for light, which builds it on demand
        here.light_at( tripoint_bub_ms( 0, 0, z ) );
    }
    for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; ++z ) {
        const level_cache &ch = here.access_cache( z );
        snap.levels.emplace( z, std::make_unique<level_cache_default_zero_members>( ch ) );
        snap.level_generations.emplace( z, std::make_pair( ch.lightmap_generation,
                                        ch.visibility_generation ) );
        snap.has_colored_lights.emplace( z, ch.has_colored_lights );
    }
    for( const std::pair<tripoint_bub_ms, tripoint_bub_ms> &pr : pairs ) {
        snap.sees.push_back( sees_all_traces( here, pr.first, pr.second ) );
    }
    snap.seen_generation = here.seen_generation();
    snap.vision_levels = here.vision_levels();
    return snap;
}

template<typename Get>
static void check_layer_matches( const std::string &layer, const int z, const Get &get )
{
    int mismatches = 0;
    std::string first;
    for( int x = 0; x < MAPSIZE_X; ++x ) {
        for( int y = 0; y < MAPSIZE_Y; ++y ) {
            if( !get( x, y ) ) {
                if( mismatches == 0 ) {
                    first = string_format( "(%d,%d)", x, y );
                }
                ++mismatches;
            }
        }
    }
    CAPTURE( layer, z, first );
    CHECK( mismatches == 0 );
}

// light is compared on every level; final classification on the levels a
// request at zlev reads, from the avatar's view range down
static void check_snapshots_match( const vision_cache_snapshot &a, const vision_cache_snapshot &b,
                                   const vision_layers layers, const int zlev )
{
    const int lowest_read = std::max( std::min( zlev, get_avatar().posz() - fov_3d_z_range ),
                                      -OVERMAP_DEPTH );
    CHECK( a.vision_levels == b.vision_levels );
    for( const auto &[z, la] : a.levels ) {
        const auto found = b.levels.find( z );
        if( found == b.levels.end() ) {
            continue;
        }
        const level_cache_default_zero_members &x = *la;
        const level_cache_default_zero_members &y = *found->second;
        check_layer_matches( "outside", z, [&]( int i, int j ) {
            return x.outside_cache[i][j] == y.outside_cache[i][j];
        } );
        check_layer_matches( "floor", z, [&]( int i, int j ) {
            return x.floor_cache[i][j] == y.floor_cache[i][j];
        } );
        check_layer_matches( "transparency", z, [&]( int i, int j ) {
            return x.transparency_cache[i][j] == y.transparency_cache[i][j];
        } );
        check_layer_matches( "transparent_wo_fields", z, [&]( int i, int j ) {
            return x.transparent_cache_wo_fields[i][j] == y.transparent_cache_wo_fields[i][j];
        } );
        check_layer_matches( "sight", z, [&]( int i, int j ) {
            return x.sight_cache[i][j] == y.sight_cache[i][j];
        } );
        check_layer_matches( "sight_wo_fields", z, [&]( int i, int j ) {
            return x.sight_cache_wo_fields[i][j] == y.sight_cache_wo_fields[i][j];
        } );
        check_layer_matches( "vision_transparency", z, [&]( int i, int j ) {
            return x.vision_transparency_cache[i][j] == y.vision_transparency_cache[i][j];
        } );
        check_layer_matches( "seen", z, [&]( int i, int j ) {
            return x.seen_cache[i][j] == y.seen_cache[i][j];
        } );
        check_layer_matches( "camera", z, [&]( int i, int j ) {
            return x.camera_cache[i][j] == y.camera_cache[i][j];
        } );
        if( layers == vision_layers::scene_and_fov ) {
            continue;
        }
        check_layer_matches( "lm", z, [&]( int i, int j ) {
            return x.lm[i][j].values == y.lm[i][j].values;
        } );
        check_layer_matches( "sm", z, [&]( int i, int j ) {
            return x.sm[i][j] == y.sm[i][j];
        } );
        check_layer_matches( "light_color", z, [&]( int i, int j ) {
            const light_color_rgb &cx = x.light_color_cache[i][j];
            const light_color_rgb &cy = y.light_color_cache[i][j];
            return cx.r == cy.r && cx.g == cy.g && cx.b == cy.b;
        } );
        CAPTURE( z );
        CHECK( a.has_colored_lights.at( z ) == b.has_colored_lights.at( z ) );
        if( layers != vision_layers::all || z < lowest_read || z > zlev ) {
            continue;
        }
        check_layer_matches( "visibility", z, [&]( int i, int j ) {
            return x.visibility_cache[i][j] == y.visibility_cache[i][j];
        } );
    }
    CHECK( a.sees == b.sees );
}

vision_cache_oracle::vision_cache_oracle( los_pairs pairs ) : pairs_( std::move( pairs ) ) {}

void vision_cache_oracle::prime() const
{
    const map &here = get_map();
    for( const std::pair<tripoint_bub_ms, tripoint_bub_ms> &pr : pairs_ ) {
        sees_all_traces( here, pr.first, pr.second );
    }
}

void vision_cache_oracle::check_matches_rebuild( const vision_layers layers ) const
{
    check_matches_rebuild( layers, get_avatar().posz() );
}

void vision_cache_oracle::check_matches_rebuild( const vision_layers layers, const int zlev ) const
{
    const vision_cache_snapshot incremental = snapshot_vision_caches( pairs_ );
    get_map().rebuild_vision_caches_from_scratch( zlev );
    const vision_cache_snapshot rebuilt = snapshot_vision_caches( pairs_ );
    CAPTURE( layers == vision_layers::all, zlev );
    check_snapshots_match( incremental, rebuilt, layers, zlev );
}

void check_stationary_build_is_noop()
{
    map &here = get_map();
    const int z = get_avatar().posz();
    here.build_map_cache( z );
    here.update_visibility_cache( z );
    const vision_cache_snapshot first = snapshot_vision_caches( {} );
    here.build_map_cache( z );
    here.update_visibility_cache( z );
    const vision_cache_snapshot second = snapshot_vision_caches( {} );
    check_snapshots_match( first, second, vision_layers::all, z );
    CHECK( first.seen_generation == second.seen_generation );
    CHECK( first.level_generations == second.level_generations );
}

bool map_meddler::has_altered_submaps( map &m )
{
    for( submap *sm : m.grid ) {
        if( sm->player_adjusted_map ) {
            return true;
        }
    }
    return false;
}

submap *map_meddler::unsafe_get_submap_at( tripoint_bub_ms &p, point_sm_ms &l )
{
    return get_map().unsafe_get_submap_at( p, l );
}
