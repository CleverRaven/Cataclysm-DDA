#include <cstddef>
#include <string>
#include <vector>

#include "calendar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "field.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "map_scale_constants.h"
#include "mapbuffer.h"
#include "mapgen_post_process.h"
#include "overmapbuffer.h"
#include "player_helpers.h"
#include "point.h"
#include "rng.h"
#include "type_id.h"

static const field_type_str_id field_fd_blood( "fd_blood" );
static const field_type_str_id field_fd_fire( "fd_fire" );

static const furn_str_id furn_f_planter_seedling( "f_planter_seedling" );
static const furn_str_id furn_f_table( "f_table" );

static const itype_id itype_rock( "rock" );
static const itype_id itype_seed_rose( "seed_rose" );

static const oter_str_id oter_field( "field" );
static const oter_str_id oter_test_pp_riot_building( "test_pp_riot_building" );

static const pp_generator_id pp_generator_riot_damage( "riot_damage" );
static const pp_generator_id pp_generator_riot_damage_road( "riot_damage_road" );
static const pp_generator_id pp_generator_test_pp_custom( "test_pp_custom" );
static const pp_generator_id pp_generator_test_pp_riot( "test_pp_riot" );
static const pp_generator_id pp_generator_test_pp_road( "test_pp_road" );

static const ter_str_id ter_t_dirt( "t_dirt" );
static const ter_str_id ter_t_floor( "t_floor" );
static const ter_str_id ter_t_floor_burnt( "t_floor_burnt" );
static const ter_str_id ter_t_rock_wall( "t_rock_wall" );
static const ter_str_id ter_t_wall_burnt( "t_wall_burnt" );
static const ter_str_id ter_t_wall_wood( "t_wall_wood" );
static const ter_str_id ter_t_wood_stairs_up( "t_wood_stairs_up" );

// Scan a 24x24 OMT area on a map and collect terrain/field statistics.
struct pp_scan_result {
    int wall_wood_count = 0;
    int wall_burnt_count = 0;
    int floor_count = 0;
    int floor_burnt_count = 0;
    int stairs_up_count = 0;
    int dirt_count = 0;
    int blood_field_count = 0;
    int fire_field_count = 0;
    int furniture_count = 0;
};

static pp_scan_result scan_omt( map &m, int z_level )
{
    pp_scan_result result;
    for( int x = 0; x < SEEX * 2; x++ ) {
        for( int y = 0; y < SEEY * 2; y++ ) {
            tripoint_bub_ms p( x, y, z_level );
            ter_id ter = m.ter( p );
            if( ter == ter_t_wall_wood.id() ) {
                result.wall_wood_count++;
            } else if( ter == ter_t_wall_burnt.id() ) {
                result.wall_burnt_count++;
            } else if( ter == ter_t_floor.id() ) {
                result.floor_count++;
            } else if( ter == ter_t_floor_burnt.id() ) {
                result.floor_burnt_count++;
            } else if( ter == ter_t_wood_stairs_up.id() ) {
                result.stairs_up_count++;
            } else if( ter == ter_t_dirt.id() ) {
                result.dirt_count++;
            }
            if( m.field_at( p ).find_field( field_fd_blood ) ) {
                result.blood_field_count++;
            }
            if( m.field_at( p ).find_field( field_fd_fire ) ) {
                result.fire_field_count++;
            }
            if( m.furn( p ) != furn_str_id::NULL_ID() ) {
                result.furniture_count++;
            }
        }
    }
    return result;
}

// Smoke tests: exercise post-process generators through the full map::generate() pipeline
// using a controlled test OMT with trivial mapgen (seeded RNG gives deterministic output).
//
// Aftershock ruin not tested here -- its terrain IDs are mod-only, not loaded in tests.

TEST_CASE( "post_process_riot_damage_seeded_smoke", "[mapgen][post_process]" )
{
    clear_overmaps();
    clear_map();
    clear_avatar();
    const tripoint_abs_omt pos( 50, 50, 0 );
    overmap_buffer.ter_set( pos, oter_test_pp_riot_building.id() );
    // Set surrounding tiles to field to avoid neighbor-dependent behavior
    for( int dx = -1; dx <= 1; dx++ ) {
        for( int dy = -1; dy <= 1; dy++ ) {
            if( dx == 0 && dy == 0 ) {
                continue;
            }
            overmap_buffer.ter_set( pos + tripoint( dx, dy, 0 ), oter_field.id() );
        }
    }

    SECTION( "riot damage dispatch sanity at day 7" ) {
        // Verifies that PP_GENERATE_RIOT_DAMAGE flag triggers post-processing
        // through the full map::generate() pipeline. Not a detailed behavioral
        // test -- the direct orchestrator tests below cover sub-generator behavior.
        calendar::turn = calendar::start_of_cataclysm + 7_days;
        rng_set_engine_seed( 42424242 );
        MAPBUFFER.clear_outside_reality_bubble();

        smallmap tm;
        tm.generate( pos, calendar::turn, false, true );

        pp_scan_result result = scan_omt( *tm.cast_to_map(), 0 );

        // Blood proves PP dispatch ran
        CHECK( result.blood_field_count > 0 );

        // Stairs survive regardless of whether pre_burn fired
        CHECK( result.stairs_up_count == 1 );

        tm.delete_unmerged_submaps();
    }

    SECTION( "pre_burn inactive before day 3" ) {
        calendar::turn = calendar::start_of_cataclysm + 0_turns;
        rng_set_engine_seed( 42424242 );
        MAPBUFFER.clear_outside_reality_bubble();

        smallmap tm;
        tm.generate( pos, calendar::turn, false, true );

        pp_scan_result result = scan_omt( *tm.cast_to_map(), 0 );

        // Pre_burn doesn't fire before day 3.
        CHECK( result.wall_burnt_count == 0 );
        CHECK( result.floor_burnt_count == 0 );
        CHECK( result.dirt_count == 0 );

        // But bash/blood still runs. Blood proves PP executed.
        CHECK( result.blood_field_count > 0 );

        tm.delete_unmerged_submaps();
    }
}

// Capture full tile-by-tile snapshot of a 24x24 OMT for exact comparison.
struct tile_state {
    ter_id ter;
    furn_id furn;
    int blood_intensity;
    int fire_intensity;
};

static std::vector<tile_state> snapshot_omt( map &m, int z_level )
{
    std::vector<tile_state> snap;
    snap.reserve( SEEX * 2 * SEEY * 2 );
    for( int x = 0; x < SEEX * 2; x++ ) {
        for( int y = 0; y < SEEY * 2; y++ ) {
            tripoint_bub_ms p( x, y, z_level );
            tile_state ts;
            ts.ter = m.ter( p );
            ts.furn = m.furn( p );
            const field_entry *blood = m.field_at( p ).find_field( field_fd_blood );
            ts.blood_intensity = blood ? blood->get_field_intensity() : 0;
            const field_entry *fire = m.field_at( p ).find_field( field_fd_fire );
            ts.fire_intensity = fire ? fire->get_field_intensity() : 0;
            snap.push_back( ts );
        }
    }
    return snap;
}

TEST_CASE( "post_process_non_rerun_guard", "[mapgen][post_process]" )
{
    clear_overmaps();
    clear_map();
    clear_avatar();
    const tripoint_abs_omt pos( 50, 50, 0 );
    overmap_buffer.ter_set( pos, oter_test_pp_riot_building.id() );
    for( int dx = -1; dx <= 1; dx++ ) {
        for( int dy = -1; dy <= 1; dy++ ) {
            if( dx == 0 && dy == 0 ) {
                continue;
            }
            overmap_buffer.ter_set( pos + tripoint( dx, dy, 0 ), oter_field.id() );
        }
    }

    calendar::turn = calendar::start_of_cataclysm + 7_days;
    rng_set_engine_seed( 42424242 );
    MAPBUFFER.clear_outside_reality_bubble();

    // First generation: save_results=true so submaps persist in MAPBUFFER.
    // any_missing=true (submaps don't exist yet), PP runs.
    smallmap tm;
    tm.generate( pos, calendar::turn, true, true );

    std::vector<tile_state> first_snap = snapshot_omt( *tm.cast_to_map(), 0 );

    // Second generation: save_results=true, submaps already in MAPBUFFER.
    // any_missing=false, !save_results=false, guard blocks PP.
    smallmap tm2;
    tm2.generate( pos, calendar::turn, true, true );

    std::vector<tile_state> second_snap = snapshot_omt( *tm2.cast_to_map(), 0 );

    // Tile-by-tile comparison. Map must be identical - PP did not re-run.
    REQUIRE( first_snap.size() == second_snap.size() );
    int mismatches = 0;
    for( size_t i = 0; i < first_snap.size(); i++ ) {
        if( first_snap[i].ter != second_snap[i].ter ||
            first_snap[i].furn != second_snap[i].furn ||
            first_snap[i].blood_intensity != second_snap[i].blood_intensity ||
            first_snap[i].fire_intensity != second_snap[i].fire_intensity ) {
            mismatches++;
        }
    }
    CAPTURE( mismatches );
    CHECK( mismatches == 0 );

    // Clean up persisted overmap/submap state from save_results=true calls.
    clear_overmaps();
}

// Characterization tests via direct orchestrator calls.
// GENERATOR_riot_damage and GENERATOR_aftershock_ruin are callable from tests.
// Sub-generators remain file-internal -- these tests characterize end-to-end
// orchestrator behavior on controlled inputs, not sub-generator units.

// Build a controlled 24x24 test area on the given map:
// perimeter = t_wall_wood, interior = t_floor, stairs at (12,12), tables at (5,5) and (5,18).
// Optionally place t_rock_wall at corners for NATURAL_UNDERGROUND testing.
static void build_pp_test_layout( map &m, int z_level, bool with_underground_tiles )
{
    for( int x = 0; x < SEEX * 2; x++ ) {
        for( int y = 0; y < SEEY * 2; y++ ) {
            tripoint_bub_ms p( x, y, z_level );
            m.furn_set( p, furn_str_id::NULL_ID() );
            m.i_clear( p );
            // Clear fields
            m.clear_fields( p );
            if( x == 0 || x == SEEX * 2 - 1 || y == 0 || y == SEEY * 2 - 1 ) {
                m.ter_set( p, ter_t_wall_wood );
            } else if( x == 12 && y == 12 ) {
                m.ter_set( p, ter_t_wood_stairs_up );
            } else {
                m.ter_set( p, ter_t_floor );
            }
        }
    }
    // Place furniture
    m.furn_set( tripoint_bub_ms( 5, 5, z_level ), furn_f_table );
    m.furn_set( tripoint_bub_ms( 5, 18, z_level ), furn_f_table );

    // Place loose items on a few floor tiles (for item-move testing)
    m.add_item( tripoint_bub_ms( 3, 3, z_level ), item( itype_rock ) );
    m.add_item( tripoint_bub_ms( 10, 10, z_level ), item( itype_rock ) );
    m.add_item( tripoint_bub_ms( 15, 15, z_level ), item( itype_rock ) );

    // Planter with a seed -- move_items must not displace it
    m.furn_set( tripoint_bub_ms( 8, 8, z_level ), furn_f_planter_seedling );
    m.add_item( tripoint_bub_ms( 8, 8, z_level ), item( itype_seed_rose ) );

    if( with_underground_tiles ) {
        // Place rock walls at interior corners for NATURAL_UNDERGROUND skip test
        m.ter_set( tripoint_bub_ms( 1, 1, z_level ), ter_t_rock_wall );
        m.ter_set( tripoint_bub_ms( 1, 22, z_level ), ter_t_rock_wall );
        m.ter_set( tripoint_bub_ms( 22, 1, z_level ), ter_t_rock_wall );
        m.ter_set( tripoint_bub_ms( 22, 22, z_level ), ter_t_rock_wall );
    }
}

TEST_CASE( "post_process_riot_road_vs_building", "[mapgen][post_process][characterization]" )
{
    // Road mode (is_a_road=true) skips pre_burn.
    // Building mode (is_a_road=false) runs pre_burn.
    // Use a seed + day count where pre_burn fires for building mode.
    clear_overmaps();
    clear_map();
    clear_avatar();
    map &here = get_map();
    const tripoint_abs_omt pos = project_to<coords::omt>( here.get_abs_sub() );

    // Try seeds until we find one where pre_burn fires.
    bool found_burn_seed = false;
    unsigned int burn_seed = 0;
    calendar::turn = calendar::start_of_cataclysm + 14_days;
    for( unsigned int seed = 100; seed < 200; seed++ ) {
        build_pp_test_layout( here, 0, false );
        rng_set_engine_seed( seed );
        pp_generator_riot_damage.obj().execute( here, pos );
        pp_scan_result r = scan_omt( here, 0 );
        if( r.wall_burnt_count > 0 ) {
            found_burn_seed = true;
            burn_seed = seed;
            break;
        }
    }
    REQUIRE( found_burn_seed );

    // Now run with that seed in building mode: pre_burn fires
    build_pp_test_layout( here, 0, false );
    rng_set_engine_seed( burn_seed );
    pp_generator_riot_damage.obj().execute( here, pos );
    pp_scan_result building_result = scan_omt( here, 0 );
    CHECK( building_result.wall_burnt_count > 0 );
    CHECK( building_result.floor_burnt_count > 0 );

    // Same seed in road mode: pre_burn skipped
    build_pp_test_layout( here, 0, false );
    rng_set_engine_seed( burn_seed );
    pp_generator_riot_damage_road.obj().execute( here, pos );
    pp_scan_result road_result = scan_omt( here, 0 );
    CHECK( road_result.wall_burnt_count == 0 );
    CHECK( road_result.floor_burnt_count == 0 );
    CHECK( road_result.dirt_count == 0 );

    // Road mode still has blood (PP ran, just no pre_burn)
    CHECK( road_result.blood_field_count > 0 );

    clear_overmaps();
}

TEST_CASE( "post_process_natural_underground_skip", "[mapgen][post_process][characterization]" )
{
    clear_overmaps();
    clear_map();
    clear_avatar();
    map &here = get_map();
    const tripoint_abs_omt pos = project_to<coords::omt>( here.get_abs_sub() );

    build_pp_test_layout( here, 0, true );
    calendar::turn = calendar::start_of_cataclysm + 7_days;
    rng_set_engine_seed( 42424242 );
    pp_generator_riot_damage.obj().execute( here, pos );

    // Rock wall tiles (NATURAL_UNDERGROUND) must be unchanged
    CHECK( here.ter( tripoint_bub_ms( 1, 1, 0 ) ) == ter_t_rock_wall.id() );
    CHECK( here.ter( tripoint_bub_ms( 1, 22, 0 ) ) == ter_t_rock_wall.id() );
    CHECK( here.ter( tripoint_bub_ms( 22, 1, 0 ) ) == ter_t_rock_wall.id() );
    CHECK( here.ter( tripoint_bub_ms( 22, 22, 0 ) ) == ter_t_rock_wall.id() );

    clear_overmaps();
}

TEST_CASE( "post_process_stair_preservation", "[mapgen][post_process][characterization]" )
{
    // When pre_burn fires, stairs (GOES_UP/GOES_DOWN) must survive.
    clear_overmaps();
    clear_map();
    clear_avatar();
    map &here = get_map();
    const tripoint_abs_omt pos = project_to<coords::omt>( here.get_abs_sub() );

    calendar::turn = calendar::start_of_cataclysm + 14_days;

    // Find seed where pre_burn fires
    bool found = false;
    unsigned int burn_seed = 0;
    for( unsigned int seed = 100; seed < 200; seed++ ) {
        build_pp_test_layout( here, 0, false );
        rng_set_engine_seed( seed );
        pp_generator_riot_damage.obj().execute( here, pos );
        pp_scan_result r = scan_omt( here, 0 );
        if( r.wall_burnt_count > 0 ) {
            found = true;
            burn_seed = seed;
            break;
        }
    }
    REQUIRE( found );

    // Run with burn seed, verify stairs survive
    build_pp_test_layout( here, 0, false );
    rng_set_engine_seed( burn_seed );
    pp_generator_riot_damage.obj().execute( here, pos );

    CHECK( here.ter( tripoint_bub_ms( 12, 12, 0 ) ) == ter_t_wood_stairs_up.id() );

    // Furniture should be destroyed by pre_burn
    pp_scan_result result = scan_omt( here, 0 );
    CHECK( result.furniture_count == 0 );

    clear_overmaps();
}

TEST_CASE( "post_process_fire_decay", "[mapgen][post_process][characterization]" )
{
    clear_overmaps();
    clear_map();
    clear_avatar();
    map &here = get_map();
    const tripoint_abs_omt pos = project_to<coords::omt>( here.get_abs_sub() );

    // Fire stops spawning well before day 30.
    build_pp_test_layout( here, 0, false );
    calendar::turn = calendar::start_of_cataclysm + 30_days;
    rng_set_engine_seed( 42424242 );
    pp_generator_riot_damage.obj().execute( here, pos );

    pp_scan_result result = scan_omt( here, 0 );
    CHECK( result.fire_field_count == 0 );

    clear_overmaps();
}

TEST_CASE( "post_process_item_move_planter_preservation",
           "[mapgen][post_process][characterization]" )
{
    // Seeds in SEALED+CONTAINER+PLANT furniture must not be moved by GENERATOR_move_items.
    clear_overmaps();
    clear_map();
    clear_avatar();
    map &here = get_map();
    const tripoint_abs_omt pos = project_to<coords::omt>( here.get_abs_sub() );

    build_pp_test_layout( here, 0, false );
    calendar::turn = calendar::start_of_cataclysm + 7_days;

    // Verify setup: seed is in the planter tile
    const tripoint_bub_ms planter_pos( 8, 8, 0 );
    REQUIRE( here.furn( planter_pos ) == furn_f_planter_seedling.id() );
    bool has_seed_before = false;
    for( const item &it : here.i_at( planter_pos ) ) {
        if( it.is_seed() ) {
            has_seed_before = true;
            break;
        }
    }
    REQUIRE( has_seed_before );

    // Road mode skips pre_burn (which would destroy everything).
    // Bash can still destroy the planter, so only check seed when planter survived.
    int planter_survived_count = 0;
    for( unsigned int seed = 1000; seed < 1020; seed++ ) {
        build_pp_test_layout( here, 0, false );
        rng_set_engine_seed( seed );
        pp_generator_riot_damage_road.obj().execute( here, pos );

        // Only check seed if planter furniture survived bash
        if( here.furn( planter_pos ) == furn_f_planter_seedling.id() ) {
            planter_survived_count++;
            bool has_seed_after = false;
            for( const item &it : here.i_at( planter_pos ) ) {
                if( it.is_seed() ) {
                    has_seed_after = true;
                    break;
                }
            }
            CAPTURE( seed );
            CHECK( has_seed_after );
        }
    }
    // Ensure we actually tested the invariant on at least some seeds
    CHECK( planter_survived_count > 0 );

    clear_overmaps();
}

TEST_CASE( "post_process_json_load_roundtrip", "[mapgen][post_process]" )
{
    // Uses test data ids (test_pp_*) to avoid coupling to real game JSON.
    SECTION( "test_pp_riot has 5 sub-generators with expected params" ) {
        const pp_generator &riot = pp_generator_test_pp_riot.obj();
        REQUIRE( riot.sub_generators().size() == 5 );
        CHECK( riot.sub_generators()[0].type == sub_generator_type::bash_damage );
        CHECK( riot.sub_generators()[0].attempts == 100 );
        CHECK( riot.sub_generators()[0].chance == 5 );
        CHECK( riot.sub_generators()[0].min_intensity == 3 );
        CHECK( riot.sub_generators()[0].max_intensity == 30 );
        CHECK( riot.sub_generators()[1].type == sub_generator_type::move_items );
        CHECK( riot.sub_generators()[1].attempts == 50 );
        CHECK( riot.sub_generators()[1].chance == 8 );
        CHECK( riot.sub_generators()[2].type == sub_generator_type::add_fire );
        CHECK( riot.sub_generators()[2].scaling_days_end == 7 );
        CHECK( riot.sub_generators()[3].type == sub_generator_type::pre_burn );
        CHECK( riot.sub_generators()[3].min_intensity == 10 );
        CHECK( riot.sub_generators()[3].max_intensity == 40 );
        CHECK( riot.sub_generators()[3].scaling_days_start == 2 );
        CHECK( riot.sub_generators()[3].scaling_days_end == 10 );
        CHECK( riot.sub_generators()[4].type == sub_generator_type::place_blood );
        CHECK( riot.sub_generators()[4].attempts == 200 );
        CHECK( riot.sub_generators()[4].chance == 20 );
    }

    SECTION( "test_pp_road has 4 sub-generators (no pre_burn)" ) {
        const pp_generator &road = pp_generator_test_pp_road.obj();
        REQUIRE( road.sub_generators().size() == 4 );
        CHECK( road.sub_generators()[0].type == sub_generator_type::bash_damage );
        CHECK( road.sub_generators()[1].type == sub_generator_type::move_items );
        CHECK( road.sub_generators()[2].type == sub_generator_type::add_fire );
        CHECK( road.sub_generators()[3].type == sub_generator_type::place_blood );
    }

    SECTION( "test_pp_custom has 1 aftershock_ruin sub-generator" ) {
        const pp_generator &custom = pp_generator_test_pp_custom.obj();
        REQUIRE( custom.sub_generators().size() == 1 );
        CHECK( custom.sub_generators()[0].type == sub_generator_type::aftershock_ruin );
    }
}

