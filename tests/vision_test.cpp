#include <functional>
#include <list>
#include <memory>
#include <new>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "avatar.h"
#include "cached_options.h"
#include "calendar.h"
#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "character.h"
#include "game.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "map_test_case.h"
#include "mapdata.h"
#include "mtype.h"
#include "options_helpers.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"
#include "units.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"

static const efftype_id effect_narcosis( "narcosis" );

static const field_type_str_id field_fd_smoke( "fd_smoke" );

static const move_mode_id move_mode_crouch( "crouch" );
static const move_mode_id move_mode_walk( "walk" );

static const mtype_id mon_test_camera( "mon_test_camera" );
static const mtype_id mon_zombie( "mon_zombie" );
static const mtype_id mon_zombie_electric( "mon_zombie_electric" );

static const ter_str_id ter_t_brick_wall( "t_brick_wall" );
static const ter_str_id ter_t_flat_roof( "t_flat_roof" );
static const ter_str_id ter_t_floor( "t_floor" );
static const ter_str_id ter_t_utility_light( "t_utility_light" );
static const ter_str_id ter_t_window_frame( "t_window_frame" );

static const trait_id trait_MYOPIC( "MYOPIC" );

static const vpart_id vpart_inboard_mirror( "inboard_mirror" );
static const vproto_id vehicle_prototype_meth_lab( "meth_lab" );
static const vproto_id vehicle_prototype_vehicle_camera_test( "vehicle_camera_test" );

static int get_actual_light_level( const map_test_case::tile &t )
{
    const map &here = get_map();
    const visibility_variables &vvcache = here.get_visibility_variables_cache();
    return static_cast<int>( here.apparent_light_at( t.p, vvcache ) );
}

static std::string vision_test_info( map_test_case &t )
{
    std::ostringstream out;
    map &here = get_map();

    using namespace map_test_case_common;

    out << "origin: " << t.get_origin() << '\n';
    out << "player: " << get_player_character().pos() << '\n';
    out << "unimpaired_range: " << get_player_character().unimpaired_range()  << '\n';
    out << "vision_threshold: " << here.get_visibility_variables_cache().vision_threshold << '\n';

    out << "fields:\n" <<  printers::fields( t ) << '\n';
    out << "transparency:\n" <<  printers::transparency( t ) << '\n';

    out << "seen:\n" <<  printers::seen( t ) << '\n';
    out << "lm:\n" <<  printers::lm( t ) << '\n';
    out << "apparent_light:\n" <<  printers::apparent_light( t ) << '\n';
    out << "obstructed:\n" <<  printers::obstructed( t ) << '\n';
    out << "floor_above:\n" <<  printers::floor( t, 1 ) << '\n';

    out << "expected:\n" <<  printers::expected( t ) << '\n';
    out << "actual:\n" << printers::format_2d_array(
    t.map_tiles_str( [&]( map_test_case::tile t, std::ostringstream & os ) {
        os << get_actual_light_level( t );
    } ) ) << '\n';

    return out.str();
}

static void assert_tile_light_level( map_test_case::tile t )
{
    if( t.expect_c < '0' || t.expect_c > '9' ) {
        FAIL( "unexpected result char '" << t.expect_c << "'" );
    }
    const int expected_level = t.expect_c - '0';
    REQUIRE( expected_level == get_actual_light_level( t ) );
}

static const time_point midnight = calendar::turn_zero + 0_hours;
static const time_point day_time = calendar::turn_zero + 9_hours + 30_minutes;

using namespace map_test_case_common;
using namespace map_test_case_common::tiles;

static const tile_predicate ter_set_flat_roof_above = ter_set( ter_t_flat_roof, tripoint_above );

static bool spawn_moncam( map_test_case::tile tile )
{
    monster *const slime = g->place_critter_at( mon_test_camera, tile.p );
    REQUIRE( slime->type->vision_day == 6 );
    slime->friendly = -1;
    return true;
}

static const tile_predicate set_up_tiles_common =
    ifchar( ' ', noop ) ||
    ifchar( 'U', noop ) ||
    ifchar( 'C', noop ) ||
    ifchar( 'Z', noop ) ||
    ifchar( 'z', ter_set( ter_t_floor ) + ter_set_flat_roof_above ) ||
    ifchar( 'u', ter_set( ter_t_floor ) + ter_set_flat_roof_above ) ||
    ifchar( 'L', ter_set( ter_t_utility_light ) + ter_set_flat_roof_above ) ||
    ifchar( '#', ter_set( ter_t_brick_wall ) + ter_set_flat_roof_above ) ||
    ifchar( '=', ter_set( ter_t_window_frame ) + ter_set_flat_roof_above ) ||
    ifchar( '-', ter_set( ter_t_floor ) + ter_set_flat_roof_above ) ||
    fail;

struct vision_test_flags {
    bool crouching = false;
    bool headlamp = false;
    bool blindfold = false;
    bool moncam = false;
    bool myopic = false;
};

struct vision_test_case {

    std::vector<std::string> setup;
    std::vector<std::string> expected_results;
    time_point time = day_time;
    vision_test_flags flags;
    tile_predicate set_up_tiles = set_up_tiles_common;
    std::string section_prefix;
    char anchor_char = 0;
    std::function<void()> intermission;

    vision_test_case( const std::vector<std::string> &setup,
                      const std::vector<std::string> &expectedResults,
                      const time_point &time ) : setup( setup ), expected_results( expectedResults ), time( time ) {}

    void test_all() const {
        Character &player_character = get_player_character();
        g->place_player( tripoint( 60, 60, 0 ) );
        player_character.clear_worn(); // Remove any light-emitting clothing
        player_character.clear_effects();
        player_character.clear_bionics();
        player_character.clear_mutations(); // remove mutations that potentially affect vision
        player_character.clear_moncams();
        clear_map( -2, OVERMAP_HEIGHT );
        g->reset_light_level();
        scoped_weather_override weather_clear( WEATHER_CLEAR );

        REQUIRE( !player_character.is_blind() );
        REQUIRE( !player_character.in_sleep_state() );
        REQUIRE( !player_character.has_effect( effect_narcosis ) );

        player_character.recalc_sight_limits();

        calendar::turn = time;

        map_test_case t;
        t.setup = setup;
        t.expected_results = expected_results;
        if( anchor_char == 0 ) {
            t.set_anchor_char_from( {'u', 'U', 'V'} );
        } else {
            t.set_anchor_char_from( {anchor_char} );
        }
        REQUIRE( t.anchor_char.has_value() );
        t.anchor_map_pos = player_character.pos();

        if( flags.crouching ) {
            player_character.set_movement_mode( move_mode_crouch );
        } else {
            player_character.set_movement_mode( move_mode_walk );
        }
        if( flags.headlamp ) {
            player_add_headlamp();
        }
        if( flags.blindfold ) {
            player_wear_blindfold();
        }
        if( flags.moncam ) {
            player_character.add_moncam( { mon_test_camera, 60 } );
        }
        if( flags.myopic ) {
            player_character.set_mutation( trait_MYOPIC );
        }

        std::stringstream section_name;
        section_name << section_prefix;
        section_name << t.generate_transform_combinations();

        // Sanity check on player placement in relation to `t`
        // must be invoked after transformations are applied to `t`
        t.validate_anchor_point( player_character.pos() );

        SECTION( section_name.str() ) {
            t.for_each_tile( set_up_tiles );
            int zlev = t.get_origin().z;
            map &here = get_map();
            // We have to run the whole thing twice, because the first time through the
            // player's vision_threshold is based on the previous lighting level (so
            // they might, for example, have poor nightvision due to having just been
            // in daylight)
            here.invalidate_visibility_cache();
            here.update_visibility_cache( zlev );
            // make sure floor caches are valid on all zlevels above
            for( int z = -2; z <= OVERMAP_HEIGHT; z++ ) {
                here.invalidate_map_cache( z );
            }
            here.build_map_cache( zlev );
            here.invalidate_visibility_cache();
            here.update_visibility_cache( zlev );
            here.invalidate_map_cache( zlev );
            here.build_map_cache( zlev );
            if( intermission ) {
                intermission();
            }

            INFO( vision_test_info( t ) );
            t.for_each_tile( assert_tile_light_level );
        }
    }
};

static std::optional<units::angle> testcase_veh_dir( point const &def, vision_test_case const &t,
        map_test_case::tile &tile )
{
    std::optional<units::angle> dir = std::nullopt;
    point const dim( t.setup[0].size(), t.setup.size() );
    if( tile.p_local == def ) {
        dir = 0_degrees;
    } else if( tile.p_local == def.rotate( 1, dim ) ) {
        dir = 90_degrees;
    } else if( tile.p_local == def.rotate( 2, dim ) ) {
        dir = 180_degrees;
    } else if( tile.p_local == def.rotate( 3, dim ) ) {
        dir = 270_degrees;
    }
    return dir;
}

// The following characters are used in these setups:
// ' ' - empty, outdoors
// '-' - empty, indoors
// 'U' - player, outdoors
// 'u' - player, indoors
// 'L' - light, indoors
// '#' - wall
// '=' - window frame

TEST_CASE( "vision_daylight", "[shadowcasting][vision]" )
{
    vision_test_case t {
        {
            "   ",
            "   ",
            " U ",
        },
        {
            "444",
            "444",
            "444",
        },
        day_time
    };

    t.test_all();
}

TEST_CASE( "vision_day_indoors", "[shadowcasting][vision]" )
{
    vision_test_case t {
        {
            "###",
            "#u#",
            "###",
        },
        {
            "111",
            "111",
            "111",
        },
        day_time
    };

    t.test_all();
}

TEST_CASE( "vision_light_shining_in", "[shadowcasting][vision]" )
{
    vision_test_case t {
        {
            "##########",
            "#--------#",
            "#u-------#",
            "#--------=",
            "##########",
        },
        {
            "1144444666",
            "1144444466",
            "1144444444",
            "1144444444",
            "1144444444",
        },
        day_time
    };

    t.test_all();
}

TEST_CASE( "vision_no_lights", "[shadowcasting][vision]" )
{
    vision_test_case t {
        {
            "   ",
            " U ",
        },
        {
            "111",
            "111",
        },
        midnight
    };

    t.test_all();
}

TEST_CASE( "vision_utility_light", "[shadowcasting][vision]" )
{
    vision_test_case t {
        {
            " L ",
            "   ",
            " U ",
        },
        {
            "444",
            "444",
            "444",
        },
        midnight
    };

    t.test_all();
}

TEST_CASE( "vision_wall_obstructs_light", "[shadowcasting][vision]" )
{
    vision_test_case t {
        {
            " L ",
            "###",
            " U ",
        },
        {
            "666",
            "111",
            "111",
        },
        midnight
    };

    t.test_all();
}

TEST_CASE( "vision_wall_can_be_lit_by_player", "[shadowcasting][vision]" )
{
    vision_test_case t {
        {
            " U",
            "  ",
            "  ",
            "##",
            "--",
        },
        {
            "44",
            "44",
            "44",
            "44",
            "66",
        },
        midnight
    };
    t.flags.headlamp = true;

    t.test_all();
}

TEST_CASE( "vision_crouching_blocks_vision_but_not_light", "[shadowcasting][vision]" )
{
    vision_test_case t {
        {
            "###",
            "#u#",
            "#=#",
            "   ",
        },
        {
            "444",
            "444",
            "444",
            "666",
        },
        day_time
    };
    t.flags.crouching = true;

    t.test_all();
}

TEST_CASE( "vision_see_wall_in_moonlight", "[shadowcasting][vision]" )
{
    const time_point full_moon = calendar::turn_zero + calendar::season_length() / 6;
    // Verify that I've picked the full_moon time correctly.
    CHECK( get_moon_phase( full_moon ) == MOON_FULL );

    vision_test_case t {
        {
            "---",
            "###",
            "   ",
            "   ",
            " U ",
        },
        {
            "666",
            "111",
            "111",
            "111",
            "111",
        },
        // Want a night time
        full_moon - time_past_midnight( full_moon )
    };

    t.test_all();
}

TEST_CASE( "vision_player_opaque_neighbors_still_visible_night", "[shadowcasting][vision]" )
{
    /**
     *  Even when stating inside the opaque wall and surrounded by opaque walls,
     *  you should see yourself and immediate surrounding.
     *  (walls here simulate the behavior of the fully opaque fields, e.g. thick smoke)
     */
    vision_test_case t {
        {
            "#####",
            "#####",
            "##u##",
            "#####",
            "#####",
        },
        {
            "66666",
            "61116",
            "61116",
            "61116",
            "66666",
        },
        midnight
    };

    if( GENERATE( false, true ) ) {
        // first scenario: player is surrounded by walls
        // overriding 'u' to set up brick wall and roof at player's position
        t.set_up_tiles =
            ifchar( 'u', ter_set( ter_t_brick_wall ) + ter_set_flat_roof_above ) ||
            t.set_up_tiles;

        t.section_prefix = "walls_";
    } else {
        // second scenario: player is surrounded by thick smoke
        // overriding 'u' to set thick smoke everywhere
        t.set_up_tiles = [&]( map_test_case::tile t ) {
            get_map().add_field( t.p, field_fd_smoke );
            return true;
        };
        t.section_prefix = "smoke_";
    }

    t.test_all();
}

TEST_CASE( "vision_single_tile_skylight", "[shadowcasting][vision]" )
{
    /**
     * Light shines through the single-tile hole in the roof. Apparent light should be symmetrical.
     */
    vision_test_case t {
        {
            "---------",
            "-#######-",
            "-#-----#-",
            "-#-----#-",
            "-#--U--#-",
            "-#-----#-",
            "-#-----#-",
            "-#######-",
            "---------",
        },
        {
            "666666666",
            "661111166",
            "611111116",
            "611141116",
            "611444116",
            "611141116",
            "611111116",
            "661111166",
            "666666666",
        },
        day_time
    };

    t.test_all();
}

TEST_CASE( "vision_junction_reciprocity", "[vision][reciprocity]" )
{
    bool player_in_junction = GENERATE( true, false );
    CAPTURE( player_in_junction );

    vision_test_case t {
        player_in_junction ?
        std::vector<std::string>{
            "###   ",
            "#u####",
            "#---z#",
            "######",
}:
        std::vector<std::string>{
            "###   ",
            "#z####",
            "#---u#",
            "######",
        },
        player_in_junction ?
        std::vector<std::string>{
            "444666",
            "444666",
            "444466",
            "444466",
}:
        std::vector<std::string>{
            "666666",
            "444444",
            "444444",
            "444444",
        },
        day_time
    };

    monster *zombie = nullptr;
    tile_predicate spawn_zombie = [&]( map_test_case::tile tile ) {
        zombie = g->place_critter_at( mon_zombie, tile.p );
        get_map().ter_set( tile.p + tripoint_above, ter_t_flat_roof );
        return true;
    };

    t.set_up_tiles =
        ifchar( 'z', spawn_zombie ) ||
        t.set_up_tiles;
    t.flags.headlamp = true;
    t.test_all();

    if( player_in_junction ) {
        REQUIRE( !get_avatar().sees( *zombie ) );
        REQUIRE( !zombie->sees( get_avatar() ) );
    } else {
        REQUIRE( get_avatar().sees( *zombie ) );
        REQUIRE( zombie->sees( get_avatar() ) );
    }
}

TEST_CASE( "vision_blindfold_reciprocity", "[vision][reciprocity]" )
{
    vision_test_case t {
        {
            "U  Z",
        },
        {
            "4666",
        },
        day_time
    };

    monster *zombie = nullptr;
    tile_predicate spawn_zombie = [&]( map_test_case::tile tile ) {
        zombie = g->place_critter_at( mon_zombie, tile.p );
        return true;
    };

    t.flags.blindfold = true;
    t.set_up_tiles =
        ifchar( 'C', spawn_moncam ) ||
        ifchar( 'Z', spawn_zombie ) ||
        t.set_up_tiles;
    t.test_all();

    REQUIRE( !get_avatar().sees( *zombie ) );
    // don't "optimize" lightcasting with player sight range
    REQUIRE( zombie->sees( get_avatar() ) );
}

TEST_CASE( "vision_moncam_basic", "[shadowcasting][vision][moncam]" )
{
    bool add_moncam = GENERATE( true, false );
    bool obstructed = GENERATE( true, false );

    vision_test_case t {
        obstructed ?
        std::vector<std::string>{
            "             ",
            "             ",
            "             ",
            "      Z      ",
            "             ",
            "             ",
            "      C      ",
            "             ",
            "             ",
            "             ",
            "             ",
            "           ##",
            "           #u",
} :
        std::vector<std::string>{
            "             ",
            "             ",
            "             ",
            "      Z      ",
            "             ",
            "             ",
            "      C      ",
            "             ",
            "             ",
            "             ",
            "             ",
            "             ",
            "            u",
        },
        add_moncam ?
        std::vector<std::string>{
            "6661111111666",
            "6611111111166",
            "6111111111116",
            "1111111111111",
            "1111111111111",
            "1111111111111",
            "1111114111111",
            "1111111111111",
            "1111111111111",
            "1111111111111",
            "6111111111116",
            "6611111111166",
            "6661111111664",
} :
        std::vector<std::string>{
            "6666666666666",
            "6666666666666",
            "6666666666666",
            "6666666666666",
            "6666666666666",
            "6666666666666",
            "6666666666666",
            "6666666666666",
            "6666666666666",
            "6666666666666",
            "6666666666666",
            "6666666666666",
            "6666666666664",
        }
        ,
        sunset( calendar::turn )
    };

    monster *zombie = nullptr;
    tile_predicate spawn_zombie = [&]( map_test_case::tile tile ) {
        zombie = g->place_critter_at( mon_zombie, tile.p );
        return true;
    };
    t.flags.blindfold = true;
    t.flags.moncam = add_moncam;
    t.set_up_tiles =
        ifchar( 'C', spawn_moncam ) ||
        ifchar( 'Z', spawn_zombie ) ||
        t.set_up_tiles;

    t.test_all();

    avatar &u = get_avatar();
    REQUIRE( zombie->sees( u ) == !obstructed );
    if( add_moncam ) {
        REQUIRE( u.sees( zombie->pos(), true ) );
    } else {
        REQUIRE( !u.sees( zombie->pos(), true ) );
    }
}

TEST_CASE( "vision_moncam_otherz", "[shadowcasting][vision][moncam]" )
{
    tripoint const disp = GENERATE( tripoint_below, tripoint_zero, tripoint_above );
    vision_test_case t {
        {
            "-c-",
            "###",
            "#u#",
            "###",
        },
        disp.z != 0 ?
        std::vector<std::string> {
            "666",
            "666",
            "616",
            "666",
}:
        std::vector<std::string> {
            "444",
            "414",
            "616",
            "666",
        },
        day_time
    };

    tile_predicate spawn_moncam_disp = [&]( map_test_case::tile tile ) {
        tile_predicate const p = ter_set( ter_t_floor ) + ter_set( ter_t_floor, tripoint_below ) +
                                 ter_set_flat_roof_above;
        p( tile );
        monster *const slime = g->place_critter_at( mon_test_camera, tile.p + disp );
        REQUIRE( slime->posz() == get_avatar().posz() + disp.z );
        REQUIRE( slime->type->vision_day == 6 );
        slime->friendly = -1;
        return true;
    };
    t.section_prefix = string_format( "%i_", disp.z );
    t.flags.moncam = true;
    t.flags.blindfold = true; // FIXME: remove once 3dfov takes LOS into account
    t.set_up_tiles =
        ifchar( 'c', spawn_moncam_disp ) ||
        t.set_up_tiles;

    t.test_all();
}

TEST_CASE( "vision_vehicle_mirrors", "[shadowcasting][vision][vehicle]" )
{
    clear_vehicles();
    bool const blindfold = GENERATE( true, false );
    vision_test_case t {
        {
            "        ",
            "        ",
            "       M",
            "       U",
            "       M",
            "        ",
            "        ",
        },
        blindfold ?
        std::vector<std::string> {
            "66666666",
            "66666666",
            "66666666",
            "66666664",
            "66666666",
            "66666666",
            "66666666",
} :
        std::vector<std::string> {
            "44444444",
            "44444444",
            "66666644",
            "66666644",
            "66666644",
            "44444444",
            "44444444",
        },
        day_time
    };
    tile_predicate spawn_veh = [&]( map_test_case::tile tile ) {
        std::optional<units::angle> dir = testcase_veh_dir( {7, 2}, t, tile );
        if( dir ) {
            vehicle *v = get_map().add_vehicle( vehicle_prototype_meth_lab, tile.p, *dir, 0, 0 );
            for( const vpart_reference &vp : v->get_avail_parts( "OPENABLE" ) ) {
                v->close( vp.part_index() );
            }
        }
        return true;
    };
    t.flags.blindfold = blindfold;
    t.set_up_tiles =
        ifchar( 'M', spawn_veh ) ||
        t.set_up_tiles;
    t.test_all();
    clear_vehicles();
}

TEST_CASE( "vision_vehicle_camera", "[shadowcasting][vision][vehicle]" )
{
    clear_vehicles();
    bool const blindfold = GENERATE( true, false );
    vision_test_case t {
        {
            " M ",
            "   ",
            "   ",
            "   ",
        },
        blindfold ?
        std::vector<std::string>{
            "616",
            "666",
            "666",
            "666",
} :
        std::vector<std::string>{
            "111",
            "444",
            "444",
            "444",
        },
        day_time
    };

    tile_predicate spawn_veh_cam = [&]( map_test_case::tile tile ) {
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        std::optional<units::angle> const dir = testcase_veh_dir( { 1, 0 }, t, tile );
        if( dir ) {
            vehicle *v =
                get_map().add_vehicle( vehicle_prototype_vehicle_camera_test, tile.p, *dir, 0, 0 );
            v->camera_on = true;
        }
        return true;
    };

    t.anchor_char = 'M';
    t.flags.blindfold = blindfold;
    t.set_up_tiles =
        ifchar( 'M', spawn_veh_cam ) ||
        t.set_up_tiles;

    t.test_all();
    clear_vehicles();
}

TEST_CASE( "vision_vehicle_camera_skew", "[shadowcasting][vision][vehicle][vehicle_fake]" )
{
    clear_vehicles();
    bool const camera_on = GENERATE( true, false );
    int const fiddle = GENERATE( 0, 1, 2 );
    vision_test_case t {
        {
            "    M",
            "     ",
            "     ",
            "     ",
            "     ",
        },
        camera_on ?
        std::vector<std::string>{
            "44611",
            "44444",
            "44446",
            "44446",
            "44444",
        }
:
        std::vector<std::string>{
            "66611",
            "66611",
            "66666",
            "66666",
            "66666",
        },     day_time
    };

    vehicle *v = nullptr;
    tile_predicate spawn_veh_cam = [&]( map_test_case::tile tile ) {
        std::optional<units::angle> const dir = testcase_veh_dir( { 4, 0 }, t, tile );
        if( dir ) {
            units::angle const skew = *dir + 45_degrees;
            v = get_map().add_vehicle( vehicle_prototype_vehicle_camera_test, tile.p, skew, 0,
                                       0 );
            v->camera_on = camera_on;
        }
        return true;
    };

    auto const fiddle_parts = [&]() {
        if( fiddle > 0 ) {
            std::vector<vehicle_part *> const horns = v->get_parts_at( v->global_pos3(), "HORN", {} );
            v->remove_part( *horns.front() );
        }
        if( fiddle > 1 ) {
            REQUIRE( v->install_part( point_zero, vpart_inboard_mirror ) != -1 );
        }
        if( fiddle > 0 ) {
            get_map().add_vehicle_to_cache( v );
            get_map().invalidate_map_cache( get_avatar().posz() );
            get_map().build_map_cache( get_avatar().posz() );
        }
    };

    t.anchor_char = 'M';
    t.intermission = fiddle_parts;
    t.set_up_tiles =
        ifchar( 'M', spawn_veh_cam ) ||
        t.set_up_tiles;

    CAPTURE( camera_on, fiddle );
    t.test_all();
    clear_vehicles();
}

TEST_CASE( "vision_moncam_invalidation", "[shadowcasting][vision][moncam]" )
{
    clear_vehicles();
    vision_test_case t {
        {
            "   ",
            " M ",
            "   ",
            "   ",
            "###",
            " C ",
        },
        {
            "111",
            "111",
            "444",
            "444",
            "444",
            "444",
        },
        day_time
    };

    tile_predicate spawn_veh_cam = [&]( map_test_case::tile tile ) {
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        std::optional<units::angle> const dir = testcase_veh_dir( { 1, 1 }, t, tile );
        if( dir ) {
            vehicle *v =
                get_map().add_vehicle( vehicle_prototype_vehicle_camera_test, tile.p, *dir, 0, 0 );
            v->camera_on = true;
        }
        return true;
    };

    monster *slime = nullptr;
    tile_predicate spawn_moncam_wiggle = [&]( map_test_case::tile tile ) {
        slime = g->place_critter_at( mon_test_camera, tile.p );
        slime->friendly = -1;
        return true;
    };

    auto wiggle_slime = [&]() {
        // vehicle camera should still work even if only the moncam moved
        slime->Creature::move_to( slime->get_location() + tripoint_east );
        get_map().build_map_cache( slime->posz() );
        slime->Creature::move_to( slime->get_location() - tripoint_east );
        get_map().build_map_cache( slime->posz() );
    };

    t.anchor_char = 'M';
    t.flags.moncam = true;
    t.intermission = wiggle_slime;
    t.set_up_tiles =
        ifchar( 'C', spawn_moncam_wiggle ) ||
        ifchar( 'M', spawn_veh_cam ) ||
        t.set_up_tiles;

    t.test_all();
    clear_vehicles();
}

TEST_CASE( "vision_bright_source", "[vision]" )
{
    vision_test_case t {
        {
            "U             Z",
        },
        {
            "444444444444462",
        },
        day_time
    };

    monster *zombie = nullptr;
    tile_predicate spawn_shocker = [&]( map_test_case::tile tile ) {
        zombie = g->place_critter_at( mon_zombie_electric, tile.p );
        return true;
    };

    t.flags.myopic = true;
    t.set_up_tiles =
        ifchar( 'Z', spawn_shocker ) ||
        t.set_up_tiles;
    t.test_all();
}

TEST_CASE( "vision_inside_meth_lab", "[shadowcasting][vision][moncam]" )
{
    clear_vehicles();

    bool door_open = GENERATE( false, true );
    bool moncam = GENERATE( false, true );

    vision_test_case t {
        {
            "  MCM  ", // left M is origin location of meth lab (driver's seat); camera can see side mirrors
            "       ",
            "       ",
            "   U   ",
            "       ",
            "       ",
            "   D   ", // D mark door to be opened
            "       "
        },
        door_open ?
        !moncam ?
        std::vector<std::string> {
            // when door is open, light shines inside, forming a cone
            "6666666",
            "6444446",
            "6444446",
            "6444446",
            "6444446",
            "6144416",
            "6444446",
            "6644466"
} :
        std::vector<std::string> {
            "4444444",
            "4444444",
            "4444444",
            "6444446",
            "6444446",
            "6144416",
            "6444446",
            "6644466"
} :

        moncam ?
        std::vector<std::string> {
            // active moncam can see through mirrors
            "4444444",
            "4444444",
            "4411144",
            "6411146",
            "6111116",
            "6111116",
            "6111116",
            "6666666"
} :
        std::vector<std::string> {
            // when door is closed, everything is dark
            "6666666",
            "6111116",
            "6111116",
            "6111116",
            "6111116",
            "6111116",
            "6111116",
            "6666666"
        },
        day_time
    };

    vehicle *v = nullptr;
    std::optional<tripoint> door = std::nullopt;

    // opens or closes a specific door (marked as 'D')
    // this is called twice: after either vehicle or door is set
    // and it executed a single time when both vehicle and door position are available
    auto open_door = [&]() {
        if( !door_open || !v || !door ) {
            return;
        }
        // open door at `door` location
        for( const vehicle_part *vp : v->get_parts_at( *door, "OPENABLE", part_status_flag::any ) ) {
            v -> open( v->index_of_part( vp ) );
        }
    };

    tile_predicate set_door_location = [&]( map_test_case::tile tile ) {
        door = tile.p;
        open_door();
        return true;
    };

    tile_predicate spawn_meth_lab = [&]( map_test_case::tile tile ) {
        std::optional<units::angle> dir;
        if( tile.p_local == point( 2, 0 ) ) {
            dir = 270_degrees;
        } else if( tile.p_local == point( 4, 7 ) ) {
            dir = 90_degrees;
        } else if( tile.p_local == point( 0, 4 ) ) {
            dir = 180_degrees;
        } else if( tile.p_local == point( 7, 2 ) ) {
            dir = 0_degrees;
        }
        if( dir ) {
            v = get_map().add_vehicle( vehicle_prototype_meth_lab, tile.p, *dir, 0, 0 );
            for( const vpart_reference &vp : v->get_avail_parts( "OPENABLE" ) ) {
                v -> close( vp.part_index() );
            }
            open_door();
        }
        return true;
    };

    t.flags.moncam = moncam;
    t.set_up_tiles =
        ifchar( 'C', spawn_moncam ) ||
        ifchar( 'M', spawn_meth_lab ) ||
        ifchar( 'D', set_door_location ) ||
        t.set_up_tiles;

    t.test_all();
    clear_vehicles();
}

TEST_CASE( "pl_sees-oob-nocrash", "[vision]" )
{
    // oob crash from game::place_player_overmap() or game::start_game(), simplified
    clear_avatar();
    get_map().load( project_to<coords::sm>( get_avatar().get_location() ) + point_south_east, false,
                    false );
    get_avatar().sees( tripoint_zero ); // CRASH?

    clear_avatar();
}
