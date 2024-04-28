#include <memory>
#include <vector>

#include "all_enum_values.h"
#include "ammo.h"
#include "calendar.h"
#include "cata_catch.h"
#include "city.h"
#include "common_types.h"
#include "coordinates.h"
#include "enums.h"
#include "game_constants.h"
#include "global_vars.h"
#include "item_factory.h"
#include "itype.h"
#include "map.h"
#include "map_iterator.h"
#include "mapbuffer.h"
#include "omdata.h"
#include "output.h"
#include "overmap.h"
#include "overmap_types.h"
#include "overmapbuffer.h"
#include "test_data.h"
#include "type_id.h"
#include "vehicle.h"
#include "vpart_position.h"

static const oter_str_id oter_cabin( "cabin" );
static const oter_str_id oter_cabin_east( "cabin_east" );
static const oter_str_id oter_cabin_north( "cabin_north" );
static const oter_str_id oter_cabin_south( "cabin_south" );
static const oter_str_id oter_cabin_west( "cabin_west" );

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

static bool tally_items( std::unordered_map<itype_id, float> &global_item_count,
                         std::unordered_map<itype_id, int> &item_count, tinymap &tm )
{
    bool found = false;
    for( const tripoint &p : tm.points_on_zlevel() ) {
        for( item &i : tm.i_at( p ) ) {
            std::unordered_map<itype_id, float>::iterator iter = global_item_count.find( i.typeId() );
            if( iter != global_item_count.end() ) {
                found = true;
                item_count.emplace( i.typeId(), 0 ).first->second += i.count();
            }
            for( const item *it : i.all_items_ptr() ) {
                iter = global_item_count.find( it->typeId() );
                if( iter != global_item_count.end() ) {
                    found = true;
                    item_count.emplace( i.typeId(), 0 ).first->second += i.count();
                }
            }
        }
        if( const optional_vpart_position ovp = tm.veh_at( p ) ) {
            vehicle *const veh = &ovp->vehicle();
            for( const int elem : veh->parts_at_relative( ovp->mount(), true ) ) {
                const vehicle_part &vp = veh->part( elem );
                for( item &i : veh->get_items( vp ) ) {
                    std::unordered_map<itype_id, float>::iterator iter = global_item_count.find( i.typeId() );
                    if( iter != global_item_count.end() ) {
                        found = true;
                        item_count.emplace( i.typeId(), 0 ).first->second += i.count();
                    }
                    for( const item *it : i.all_items_ptr() ) {
                        iter = global_item_count.find( it->typeId() );
                        if( iter != global_item_count.end() ) {
                            found = true;
                            item_count.emplace( i.typeId(), 0 ).first->second += i.count();
                        }
                    }
                }
            }
        }
    }
    return found;
}

TEST_CASE( "enumerate_items", "[.]" )
{
    for( const itype *id : item_controller->find(
    []( const itype & type ) -> bool {
    return !!type.gun;
} ) ) {
        printf( "%s ", id->gun->skill_used.str().c_str() );
        printf( "%s\n", id->get_id().str().c_str() );
    }
}

static void finalize_item_counts( std::unordered_map<itype_id, float> &item_counts )
{
    for( std::pair<const std::string, item_demographic_test_data> &category :
         test_data::item_demographics ) {
        // Scan for match ing ammo types and shim them into the provided data so
        // the test doesn't break every time we add a new item variant.
        // Ammo has a lot of things in it we don't consider "real ammo" so there's a list
        // of item types to ignore that are applied here.
        if( category.first == "ammo" ) {
            for( const itype *id : item_controller->find( []( const itype & type ) -> bool {
            return !!type.ammo;
        } ) ) {
                if( category.second.ignored_items.find( id->get_id() ) != category.second.ignored_items.end() ) {
                    continue;
                }
                category.second.item_weights[id->get_id()] = 1;
                auto ammotype_map_iter = category.second.groups.find( id->ammo->type.str() );
                if( ammotype_map_iter == category.second.groups.end() ) {
                    // If there's no matching ammotype in the test data,
                    // stick the item in the "other" group.
                    category.second.groups["other"].second[id->get_id()];
                } else {
                    // If there is a matching ammotype and the item id isn't already populated,
                    // add it.
                    if( ammotype_map_iter->second.second.find( id->get_id() ) ==
                        ammotype_map_iter->second.second.end() ) {
                        ammotype_map_iter->second.second[id->get_id()] = 1;
                    }
                }
            }
        } else if( category.first == "gun" ) {
            for( const itype *id : item_controller->find( []( const itype & type ) -> bool {
            return !!type.gun;
        } ) ) {
                if( category.second.ignored_items.find( id->get_id() ) != category.second.ignored_items.end() ) {
                    continue;
                }
                category.second.item_weights[id->get_id()] = 1; // ???
                for( const ammotype &ammotype_id : id->gun->ammo ) {
                    auto ammotype_map_iter = category.second.groups.find( ammotype_id.str() );
                    if( ammotype_map_iter == category.second.groups.end() ) {
                        // If there's no matching ammotype in the test data,
                        // stick the item in the "other" group.
                        category.second.groups["other"].second[id->get_id()];
                    } else {
                        // If there is a matching ammotype and the item id isn't already populated,
                        // add it.
                        if( ammotype_map_iter->second.second.find( id->get_id() ) ==
                            ammotype_map_iter->second.second.end() ) {
                            ammotype_map_iter->second.second[id->get_id()] = 1; // ???
                        }
                    }
                }
            }
        }
        for( std::pair<const itype_id, int> demographics : category.second.item_weights ) {
            item_counts[demographics.first] = 0.0;
        }
    }
}

// Toggle this to enable the (very very very expensive) item demographics test.
static bool enable_item_demographics = false;

TEST_CASE( "overmap_terrain_coverage", "[overmap][slow]" )
{
    // The goal of this test is to generate a lot of overmaps, and count up how
    // many times we see each terrain, so that we can check that everything
    // generates at least sometimes.
    struct omt_stats {
        explicit omt_stats( const tripoint_abs_omt &p ) : last_observed( p ) {}

        tripoint_abs_omt last_observed;
        int count = 0;
        int samples = 0;
        bool found = false;
        std::unordered_map<itype_id, int> item_counts;
    };
    std::unordered_map<oter_type_id, omt_stats> stats;
    std::unordered_map<itype_id, float> item_counts;
    finalize_item_counts( item_counts );
    map &main_map = get_map();
    point_abs_omt map_origin = project_to<coords::omt>( main_map.get_abs_sub().xy() );
    for( int i = 0; i < 10; ++i ) {
        for( const point_abs_omt &p : closest_points_first( map_origin, 5, 3 * ( OMAPX - 1 ) ) ) {
            // We need to avoid OMTs that overlap with the 'main' map, so we start at a
            // non-zero minimum radius and ensure that the 'main' map is inside that
            // minimum radius.
            if( main_map.inbounds( tripoint_abs_ms( project_to<coords::ms>( p ), 0 ) ) ) {
                continue;
            }
            for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; ++z ) {
                tripoint_abs_omt tp( p, z );
                oter_type_id id = overmap_buffer.ter( tp )->get_type_id();
                auto iter_bool = stats.emplace( id, tp );
                iter_bool.first->second.last_observed = tp;
                ++iter_bool.first->second.count;
            }
        }
        // The second phase of this test is to perform the tile-level mapgen
        // for each oter_type, in hopes of triggering any errors that might arise
        // with that.
        // In addition, if enable_item_demographics == true,
        // perform extensive mapgen analysis and make assertions about
        // the ratios of selected item occurrence rates.
        for( std::pair<const oter_type_id, omt_stats> &p : stats ) {
            const std::string oter_type_id = p.first->id.str();
            const tripoint_abs_omt pos = p.second.last_observed;
            const int count = p.second.count;
            // Once we find items of interest in an oter_type, increase the sampling for it.
            int sampling_exponent = p.second.found ? 3 : 2;
            int goal_samples = std::pow( std::log( std::max( 10, count ) ), sampling_exponent );
            if( !enable_item_demographics ) {
                goal_samples = 1;
            }
            // We are sampling with logarithmic falloff. Once we pass a threshold where
            // log( total_count ) yields the next-higher integer value, we take another sample.
            int sample_size = goal_samples - p.second.samples;
            if( sample_size <= 0 ) {
                continue;
            }
            CAPTURE( oter_type_id );
            const std::string msg = capture_debugmsg_during( [pos,
            &item_counts, &p, &sample_size, &goal_samples, count, oter_type_id]() {
                for( int i = 0; i < sample_size; ++i ) {
                    // clear the generated maps so we keep getting new results.
                    MAPBUFFER.clear_outside_reality_bubble();
                    tinymap tm;
                    tm.generate( pos, calendar::turn );
                    bool found = tally_items( item_counts, p.second.item_counts, tm );
                    if( enable_item_demographics && found && !p.second.found ) {
                        goal_samples = std::pow( std::log( std::max( 10, count ) ), 3 );
                        sample_size = goal_samples - p.second.samples;
                        p.second.found = true;
                    }
                }
            } );
            p.second.samples = goal_samples;
            CAPTURE( msg );
            CAPTURE( msg.empty() );
        }

        overmap_buffer.reset();
    }

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
            } else if( !test_data::overmap_terrain_coverage_whitelist.count( id ) ) {
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
        const std::string missing_oter_type_ids = enumerate_as_string( missing,
        []( const oter_type_id & id ) {
            return id->id.str();
        } );
        CAPTURE( missing_oter_type_ids );
        INFO( "To resolve errors about missing terrains you can either give the terrain the "
              "SHOULD_NOT_SPAWN flag (intended for terrains that should never spawn, for example "
              "test terrains or work in progress), or tweak the constraints so that the terrain "
              "can spawn more reliably, or add them to the whitelist above in this function "
              "(inteded for terrains that sometimes spawn, but cannot be expected to spawn "
              "reliably enough for this test)" );
        CHECK( num_missing == 0 );
    }

    if( !enable_item_demographics ) {
        // This should be SKIP() but we haven't updated to Catch 3.3.0 yet.
        return;;
    }
    // Copy and scale the final results from all oter_types to the global item_count map for analysis.
    for( const std::pair<const oter_type_id, omt_stats> &omt_entry : stats ) {
        REQUIRE( omt_entry.second.samples != 0 );
        float sampling_factor = static_cast<float>( omt_entry.second.count ) /
                                static_cast<float>( omt_entry.second.samples );
        for( std::pair<const itype_id, int> item_entry : omt_entry.second.item_counts ) {
            const itype *item_type = item::find_type( item_entry.first );
            // Adjust this filter to limit output.
            if( !!item_type->gun ) {
                printf( "Found %f %s in %d instances of %s (scaled)\n",
                        static_cast<float>( item_entry.second ) * sampling_factor,
                        item_entry.first.c_str(), omt_entry.second.count,
                        omt_entry.first.id().c_str() );
            }
            item_counts[item_entry.first] += static_cast<float>( item_entry.second ) * sampling_factor;
        }
    }

    // We're asserting ratios for three things here.
    // 1. global weight within a category, so e.g. number of glock 19s / all guns spawned.
    // 2. weight of a sub-category within a category, e.g. number of 9mm firearms / all guns.
    // 3. weight of individual items within a sub-category, e.g. number of glock 19s / all 9mm firearms.
    for( std::pair<const std::string, item_demographic_test_data> &category :
         test_data::item_demographics ) {
        if( category.second.tests.count( "items" ) ) {
            float category_weight_total = 0.0;
            float category_actual_total = 0.0;
            // I don't see an alternative to just walking this twice?
            for( std::pair<const itype_id, int> &item_ratio : category.second.item_weights ) {
                category_weight_total += item_ratio.second;
                category_actual_total += item_counts[item_ratio.first];
            }
            for( std::pair<const itype_id, int> &item_ratio : category.second.item_weights ) {
                float actual_item_count = item_counts[item_ratio.first];
                float actual_item_ratio = actual_item_count / category_actual_total;
                float expected_item_total = item_ratio.second;
                float expected_item_ratio = expected_item_total / category_weight_total;
                CAPTURE( item_ratio.first );
                CAPTURE( actual_item_count );
                CAPTURE( expected_item_total );
                CHECK_THAT( actual_item_ratio, Catch::Matchers::WithinRel( expected_item_ratio, 0.1f ) );
            }
        }
        if( category.second.tests.count( "groups" ) ) {
            float all_type_weight_total = 0.0;
            float all_type_actual_total = 0.0;
            // I don't see an alternative to just walking this twice?
            for( std::pair < const std::string,
                 std::pair<int, std::map<itype_id, int>>> &group : category.second.groups ) {
                all_type_weight_total += group.second.first;
                for( std::pair<const itype_id, int> &item_ratio : group.second.second ) {
                    all_type_actual_total += item_counts[item_ratio.first];
                }
            }
            for( std::pair < const std::string,
                 std::pair<int, std::map<itype_id, int>>> &group : category.second.groups ) {
                float current_type_actual_total = 0.0;
                float current_type_weight_total = group.second.first;
                for( std::pair<const itype_id, int> &item_ratio : group.second.second ) {
                    current_type_actual_total += item_counts[item_ratio.first];
                }
                if( category.second.tests.count( "inner-group" ) ) {
                    for( std::pair<const itype_id, int> &item_ratio : group.second.second ) {
                        float actual_item_count = item_counts[item_ratio.first];
                        float actual_item_ratio = actual_item_count / current_type_actual_total;
                        float expected_item_ratio = item_ratio.second / current_type_weight_total;
                        CAPTURE( category.first );
                        CAPTURE( item_ratio.first );
                        CAPTURE( actual_item_count );
                        CAPTURE( expected_item_ratio );
                        CHECK_THAT( actual_item_ratio,
                                    Catch::Matchers::WithinRel( expected_item_ratio, 0.1f ) );
                    }
                }
                float current_type_expected_ratio = current_type_weight_total / all_type_weight_total;
                float current_type_actual_ratio = current_type_actual_total / all_type_actual_total;
                CAPTURE( category.first );
                CAPTURE( group.first );
                CAPTURE( current_type_expected_ratio );
                CAPTURE( current_type_actual_ratio );
                INFO( "Difference: " << current_type_actual_ratio - current_type_expected_ratio );
                CHECK_THAT( current_type_actual_ratio,
                            Catch::Matchers::WithinRel( current_type_expected_ratio, 0.1f ) );
            }
        }
    }
}
