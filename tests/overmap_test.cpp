#include <memory>
#include <vector>

#include "all_enum_values.h"
#include "calendar.h"
#include "cata_catch.h"
#include "city.h"
#include "common_types.h"
#include "coordinates.h"
#include "enums.h"
#include "game_constants.h"
#include "global_vars.h"
#include "map.h"
#include "mapbuffer.h"
#include "omdata.h"
#include "overmap.h"
#include "overmap_types.h"
#include "overmapbuffer.h"
#include "type_id.h"

static const oter_str_id oter_cabin( "cabin" );
static const oter_str_id oter_cabin_east( "cabin_east" );
static const oter_str_id oter_cabin_north( "cabin_north" );
static const oter_str_id oter_cabin_south( "cabin_south" );
static const oter_str_id oter_cabin_west( "cabin_west" );

static const oter_type_str_id oter_type_ants_lab( "ants_lab" );
static const oter_type_str_id oter_type_ants_lab_stairs( "ants_lab_stairs" );
static const oter_type_str_id oter_type_bunker_shop_b( "bunker_shop_b" );
static const oter_type_str_id oter_type_bunker_shop_g( "bunker_shop_g" );
static const oter_type_str_id oter_type_ravine( "ravine" );
static const oter_type_str_id oter_type_ravine_edge( "ravine_edge" );
static const oter_type_str_id oter_type_ravine_floor( "ravine_floor" );
static const oter_type_str_id oter_type_ravine_floor_edge( "ravine_floor_edge" );
static const oter_type_str_id oter_type_rock_border( "rock_border" );
static const oter_type_str_id oter_type_s_gas_b11( "s_gas_b11" );
static const oter_type_str_id oter_type_s_gas_b20( "s_gas_b20" );
static const oter_type_str_id oter_type_s_gas_b21( "s_gas_b21" );
static const oter_type_str_id oter_type_s_gas_g0( "s_gas_g0" );
static const oter_type_str_id oter_type_s_gas_g0_roof( "s_gas_g0_roof" );
static const oter_type_str_id oter_type_s_gas_g1( "s_gas_g1" );
static const oter_type_str_id oter_type_s_gas_g1_roof( "s_gas_g1_roof" );
static const oter_type_str_id oter_type_s_restaurant_deserted_test( "s_restaurant_deserted_test" );

static const overmap_special_id overmap_special_Cabin( "Cabin" );
static const overmap_special_id overmap_special_Lab( "Lab" );

TEST_CASE( "set_and_get_overmap_scents", "[overmap]" )
{
    std::unique_ptr<overmap> test_overmap = std::make_unique<overmap>( point_abs_om() );

    // By default there are no scents set.
    for( int x = 0; x < 180; ++x ) {
        for( int y = 0; y < 180; ++y ) {
            for( int z = -10; z < 10; ++z ) {
                REQUIRE( test_overmap->scent_at( { x, y, z } ).creation_time ==
                         calendar::before_time_starts );
            }
        }
    }

    time_point creation_time = calendar::turn_zero + 50_turns;
    scent_trace test_scent( creation_time, 90 );
    test_overmap->set_scent( { 75, 85, 0 }, test_scent );
    REQUIRE( test_overmap->scent_at( { 75, 85, 0} ).creation_time == creation_time );
    REQUIRE( test_overmap->scent_at( { 75, 85, 0} ).initial_strength == 90 );
}

TEST_CASE( "default_overmap_generation_always_succeeds", "[overmap][slow]" )
{
    int overmaps_to_construct = 10;
    for( const point_abs_om &candidate_addr : closest_points_first( point_abs_om(), 10 ) ) {
        // Skip populated overmaps.
        if( overmap_buffer.has( candidate_addr ) ) {
            continue;
        }
        overmap_special_batch test_specials = overmap_specials::get_default_batch( candidate_addr );
        overmap_buffer.create_custom_overmap( candidate_addr, test_specials );
        for( const overmap_special_placement &special_placement : test_specials ) {
            const overmap_special *special = special_placement.special_details;
            INFO( "In attempt #" << overmaps_to_construct
                  << " failed to place " << special->id.str() );
            int min_occur = special->get_constraints().occurrences.min;
            CHECK( min_occur <= special_placement.instances_placed );
        }
        if( --overmaps_to_construct <= 0 ) {
            break;
        }
    }
    overmap_buffer.clear();
}

TEST_CASE( "default_overmap_generation_has_non_mandatory_specials_at_origin", "[overmap][slow]" )
{
    const point_abs_om origin{};

    overmap_special mandatory;
    overmap_special optional;

    // Get some specific overmap specials so we can assert their presence later.
    // This should probably be replaced with some custom specials created in
    // memory rather than tying this test to these, but it works for now...
    for( const overmap_special &elem : overmap_specials::get_all() ) {
        if( elem.id == overmap_special_Cabin ) {
            optional = elem;
        } else if( elem.id == overmap_special_Lab ) {
            mandatory = elem;
        }
    }

    // Make this mandatory special impossible to place.
    const_cast<int &>( mandatory.get_constraints().city_size.min ) = 999;

    // Construct our own overmap_special_batch containing only our single mandatory
    // and single optional special, so we can make some assertions.
    std::vector<const overmap_special *> specials;
    specials.push_back( &mandatory );
    specials.push_back( &optional );
    overmap_special_batch test_specials = overmap_special_batch( origin, specials );

    // Run the overmap creation, which will try to place our specials.
    overmap_buffer.create_custom_overmap( origin, test_specials );

    // Get the origin overmap...
    overmap *test_overmap = overmap_buffer.get_existing( origin );

    // ...and assert that the optional special exists on this map.
    bool found_optional = false;
    for( int x = 0; x < OMAPX; ++x ) {
        for( int y = 0; y < OMAPY; ++y ) {
            const oter_id t = test_overmap->ter( { x, y, 0 } );
            if( t->id == oter_cabin ||
                t->id == oter_cabin_north || t->id == oter_cabin_east ||
                t->id == oter_cabin_south || t->id == oter_cabin_west ) {
                found_optional = true;
            }
        }
    }

    INFO( "Failed to place optional special on origin " );
    CHECK( found_optional == true );
    overmap_buffer.clear();
}

TEST_CASE( "is_ot_match", "[overmap][terrain]" )
{
    SECTION( "exact match" ) {
        // Matches the complete string
        // NOLINTNEXTLINE(cata-ot-match)
        CHECK( is_ot_match( "forest", oter_id( "forest" ), ot_match_type::exact ) );
        // NOLINTNEXTLINE(cata-ot-match)
        CHECK( is_ot_match( "central_lab", oter_id( "central_lab" ), ot_match_type::exact ) );

        // Does not exactly match if rotation differs
        // NOLINTNEXTLINE(cata-ot-match)
        CHECK_FALSE( is_ot_match( "sub_station", oter_id( "sub_station_north" ), ot_match_type::exact ) );
        // NOLINTNEXTLINE(cata-ot-match)
        CHECK_FALSE( is_ot_match( "sub_station", oter_id( "sub_station_south" ), ot_match_type::exact ) );
    }

    SECTION( "type match" ) {
        // Matches regardless of rotation
        // NOLINTNEXTLINE(cata-ot-match)
        CHECK( is_ot_match( "sub_station", oter_id( "sub_station_north" ), ot_match_type::type ) );
        // NOLINTNEXTLINE(cata-ot-match)
        CHECK( is_ot_match( "sub_station", oter_id( "sub_station_south" ), ot_match_type::type ) );
        // NOLINTNEXTLINE(cata-ot-match)
        CHECK( is_ot_match( "sub_station", oter_id( "sub_station_east" ), ot_match_type::type ) );
        // NOLINTNEXTLINE(cata-ot-match)
        CHECK( is_ot_match( "sub_station", oter_id( "sub_station_west" ), ot_match_type::type ) );

        // Does not match if base type does not match
        // NOLINTNEXTLINE(cata-ot-match)
        CHECK_FALSE( is_ot_match( "lab", oter_id( "central_lab" ), ot_match_type::type ) );
        // NOLINTNEXTLINE(cata-ot-match)
        CHECK_FALSE( is_ot_match( "sub_station", oter_id( "sewer_sub_station" ), ot_match_type::type ) );
    }

    SECTION( "prefix match" ) {
        // Matches the complete string
        CHECK( is_ot_match( "forest", oter_id( "forest" ), ot_match_type::prefix ) );
        CHECK( is_ot_match( "central_lab", oter_id( "central_lab" ), ot_match_type::prefix ) );

        // Prefix matches when an underscore separator exists
        CHECK( is_ot_match( "central", oter_id( "central_lab" ), ot_match_type::prefix ) );
        CHECK( is_ot_match( "central", oter_id( "central_lab_stairs" ), ot_match_type::prefix ) );

        // Prefix itself may contain underscores
        CHECK( is_ot_match( "central_lab", oter_id( "central_lab_stairs" ), ot_match_type::prefix ) );
        CHECK( is_ot_match( "central_lab_train", oter_id( "central_lab_train_depot" ),
                            ot_match_type::prefix ) );

        // Prefix does not match without an underscore separator
        CHECK_FALSE( is_ot_match( "fore", oter_id( "forest" ), ot_match_type::prefix ) );
        CHECK_FALSE( is_ot_match( "fore", oter_id( "forest_thick" ), ot_match_type::prefix ) );

        // Prefix does not match the middle or end
        CHECK_FALSE( is_ot_match( "lab", oter_id( "central_lab" ), ot_match_type::prefix ) );
        CHECK_FALSE( is_ot_match( "lab", oter_id( "central_lab_stairs" ), ot_match_type::prefix ) );
    }

    SECTION( "contains match" ) {
        // Matches the complete string
        CHECK( is_ot_match( "forest", oter_id( "forest" ), ot_match_type::contains ) );
        CHECK( is_ot_match( "central_lab", oter_id( "central_lab" ), ot_match_type::contains ) );

        // Matches the beginning/middle/end of an underscore-delimited id
        CHECK( is_ot_match( "central", oter_id( "central_lab_stairs" ), ot_match_type::contains ) );
        CHECK( is_ot_match( "lab", oter_id( "central_lab_stairs" ), ot_match_type::contains ) );
        CHECK( is_ot_match( "stairs", oter_id( "central_lab_stairs" ), ot_match_type::contains ) );

        // Matches the beginning/middle/end without undercores as well
        CHECK( is_ot_match( "cent", oter_id( "central_lab_stairs" ), ot_match_type::contains ) );
        CHECK( is_ot_match( "ral_lab", oter_id( "central_lab_stairs" ), ot_match_type::contains ) );
        CHECK( is_ot_match( "_lab_", oter_id( "central_lab_stairs" ), ot_match_type::contains ) );
        CHECK( is_ot_match( "airs", oter_id( "central_lab_stairs" ), ot_match_type::contains ) );

        // Does not match if substring is not contained
        CHECK_FALSE( is_ot_match( "forest", oter_id( "central_lab" ), ot_match_type::contains ) );
        CHECK_FALSE( is_ot_match( "forestry", oter_id( "forest" ), ot_match_type::contains ) );
    }
}

TEST_CASE( "mutable_overmap_placement", "[overmap][slow]" )
{
    const overmap_special &special =
        *overmap_special_id( GENERATE( "test_anthill", "test_crater", "test_microlab" ) );
    const city cit;

    constexpr int num_overmaps = 100;
    constexpr int num_trials_per_overmap = 100;

    global_variables &globvars = get_globals();
    globvars.clear_global_values();

    for( int j = 0; j < num_overmaps; ++j ) {
        // overmap objects are really large, so we don't want them on the
        // stack.  Use unique_ptr and put it on the heap
        std::unique_ptr<overmap> om = std::make_unique<overmap>( point_abs_om( point_zero ) );
        om_direction::type dir = om_direction::type::north;

        int successes = 0;

        for( int i = 0; i < num_trials_per_overmap; ++i ) {
            tripoint_om_omt try_pos( rng( 0, OMAPX - 1 ), rng( 0, OMAPY - 1 ), 0 );

            // This test can get very spammy, so abort once an error is
            // observed
            if( debug_has_error_been_observed() ) {
                return;
            }

            if( om->can_place_special( special, try_pos, dir, false ) ) {
                std::vector<tripoint_om_omt> placed_points =
                    om->place_special( special, try_pos, dir, cit, false, false );
                CHECK( !placed_points.empty() );
                ++successes;
            }
        }

        CHECK( successes > num_trials_per_overmap / 2 );
    }
}

TEST_CASE( "overmap_terrain_coverage", "[overmap][slow]" )
{
    // The goal of this test is to generate a lot of overmaps, and count up how
    // many times we see each terrain, so that we can check that everything
    // generates at least sometimes.

    struct omt_stats {
        explicit omt_stats( const tripoint_abs_omt &p ) : first_observed( p ) {}

        tripoint_abs_omt first_observed;
        int count = 0;
    };
    std::unordered_map<oter_type_id, omt_stats> stats;
    point_abs_omt origin;
    map &main_map = get_map();

    for( const point_abs_omt &p : closest_points_first( origin, 0, 10 * OMAPX - 1 ) ) {
        // We need to avoid OMTs that overlap with the 'main' map, so we start at a
        // non-zero minimum radius and ensure that the 'main' map is inside that
        // minimum radius.
        if( main_map.inbounds( tripoint_abs_ms( project_to<coords::ms>( p ), 0 ) ) ) {
            continue;
        }
        for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; ++z ) {
            tripoint_abs_omt tp( p, z );
            oter_type_id id = overmap_buffer.ter( tp )->get_type_id();
            auto it = stats.emplace( id, tp ).first;
            ++it->second.count;
        }
    }

    std::unordered_set<oter_type_id> whitelist = {
        oter_type_ants_lab.id(), // ant lab is a very improbable spawn
        oter_type_ants_lab_stairs.id(),
        oter_type_bunker_shop_b.id(),
        oter_type_bunker_shop_g.id(),
        oter_type_ravine.id(), // ravine only in desert & Aftershock
        oter_type_ravine_edge.id(),
        oter_type_ravine_floor_edge.id(),
        oter_type_ravine_floor.id(),
        oter_type_rock_border.id(), // only in the bordered scenario
        oter_type_s_gas_b11.id(),
        oter_type_s_gas_b20.id(),
        oter_type_s_gas_b21.id(),
        oter_type_s_gas_g0.id(),
        oter_type_s_gas_g0_roof.id(),
        oter_type_s_gas_g1.id(),
        oter_type_s_gas_g1_roof.id(),
        oter_type_s_restaurant_deserted_test.id(), // only in the desert test region
    };

    std::unordered_set<oter_type_id> done;
    std::vector<oter_type_id> missing;

    global_variables &globvars = get_globals();
    globvars.clear_global_values();

    for( const oter_t &ter : overmap_terrains::get_all() ) {
        oter_type_id id = ter.get_type_id();
        oter_type_str_id id_s = id.id();
        if( id_s.is_empty() || id_s.is_null() ) {
            continue;
        }
        if( done.insert( id ).second ) {
            CAPTURE( id );
            auto it = stats.find( id );
            const bool found = it != stats.end();
            const bool should_be_found = !id->has_flag( oter_flags::should_not_spawn );

            if( found == should_be_found ) {
                continue;
            }

            // We also want to skip any terrain that's the result of a faction
            // camp construction recipe
            const recipe_id recipe( id_s.c_str() );
            if( recipe.is_valid() && recipe->is_blueprint() ) {
                continue;
            }

            if( found ) {
                FAIL( "oter_type_id was found in map but had SHOULD_NOT_SPAWN flag" );
            } else if( !whitelist.count( id ) ) {
                missing.push_back( id );
            }
        }
    }

    {
        size_t num_missing = missing.size();
        CAPTURE( num_missing );
        constexpr size_t max_to_report = 100;
        if( num_missing > max_to_report ) {
            std::shuffle( missing.begin(), missing.end(), rng_get_engine() );
            missing.erase( missing.begin() + max_to_report, missing.end() );
        }
        std::sort( missing.begin(), missing.end() );
        CAPTURE( missing );
        INFO( "To resolve errors about missing terrains you can either give the terrain the "
              "SHOULD_NOT_SPAWN flag (intended for terrains that should never spawn, for example "
              "test terrains or work in progress), or tweak the constraints so that the terrain "
              "can spawn more reliably, or add them to the whitelist above in this function "
              "(inteded for terrains that sometimes spawn, but cannot be expected to spawn "
              "reliably enough for this test)" );
        CHECK( num_missing == 0 );
    }

    // The second phase of this test is to perform the tile-level mapgen once
    // for each oter_type, in hopes of triggering any errors that might arise
    // with that.
    int num_generated_since_last_clear = 0;
    for( const std::pair<const oter_type_id, omt_stats> &p : stats ) {
        const tripoint_abs_omt pos = p.second.first_observed;
        tinymap tm;
        tm.load( project_to<coords::sm>( pos ), false );

        // Periodically clear the generated maps to save memory
        if( ++num_generated_since_last_clear >= 64 ) {
            MAPBUFFER.clear_outside_reality_bubble();
            num_generated_since_last_clear = 0;
        }
    }
}
