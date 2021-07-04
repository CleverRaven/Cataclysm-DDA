#include <functional>
#include <list>
#include <memory>
#include <new>
#include <sstream>
#include <string>
#include <vector>

#include "cached_options.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "game.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "map_test_case.h"
#include "mapdata.h"
#include "optional.h"
#include "point.h"
#include "type_id.h"
#include "units.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"

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

    out << "zlevels: " << here.has_zlevels() << '\n';
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
static const time_point midday = calendar::turn_zero + 12_hours;

static const move_mode_id move_mode_walk( "walk" );
static const move_mode_id move_mode_crouch( "crouch" );

using namespace map_test_case_common;
using namespace map_test_case_common::tiles;

auto static const ter_set_flat_roof_above = ter_set( ter_str_id( "t_flat_roof" ), tripoint_above );

static const tile_predicate set_up_tiles_common =
    ifchar( ' ', noop ) ||
    ifchar( 'U', noop ) ||
    ifchar( 'u', ter_set( ter_str_id( "t_floor" ) ) + ter_set_flat_roof_above ) ||
    ifchar( 'L', ter_set( ter_str_id( "t_utility_light" ) ) + ter_set_flat_roof_above ) ||
    ifchar( '#', ter_set( ter_str_id( "t_brick_wall" ) ) + ter_set_flat_roof_above ) ||
    ifchar( '=', ter_set( ter_str_id( "t_window_frame" ) ) + ter_set_flat_roof_above ) ||
    ifchar( '-', ter_set( ter_str_id( "t_floor" ) ) + ter_set_flat_roof_above ) ||
    fail;

struct vision_test_flags {
    bool crouching = false;
    bool headlamp = false;
};

struct vision_test_case {

    std::vector<std::string> setup;
    std::vector<std::string> expected_results;
    time_point time = midday;
    vision_test_flags flags;
    tile_predicate set_up_tiles = set_up_tiles_common;
    std::string section_prefix;

    vision_test_case( const std::vector<std::string> &setup,
                      const std::vector<std::string> &expectedResults,
                      const time_point &time ) : setup( setup ), expected_results( expectedResults ), time( time ) {}

    void test_all() const {
        const efftype_id effectNarcosis( "narcosis" );
        Character &player_character = get_player_character();
        g->place_player( tripoint( 60, 60, 0 ) );
        player_character.worn.clear(); // Remove any light-emitting clothing
        player_character.clear_effects();
        player_character.clear_bionics();
        player_character.clear_mutations(); // remove mutations that potentially affect vision
        clear_map();
        g->reset_light_level();

        REQUIRE( !player_character.is_blind() );
        REQUIRE( !player_character.in_sleep_state() );
        REQUIRE( !player_character.has_effect( effectNarcosis ) );

        player_character.recalc_sight_limits();

        calendar::turn = time;

        map_test_case t;
        t.setup = setup;
        t.expected_results = expected_results;
        t.set_anchor_char_from( {'u', 'U', 'V'} );
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

        // test both 2d and 3d cases
        restore_on_out_of_scope<bool> restore_fov_3d( fov_3d );
        fov_3d = GENERATE( false, true );

        std::stringstream section_name;
        section_name << section_prefix;
        section_name << ( fov_3d ? "3d" : "2d" ) << "_casting__";
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
            here.update_visibility_cache( zlev );
            here.invalidate_map_cache( zlev );
            here.build_map_cache( zlev );
            here.update_visibility_cache( zlev );
            here.invalidate_map_cache( zlev );
            here.build_map_cache( zlev );

            INFO( vision_test_info( t ) );
            t.for_each_tile( assert_tile_light_level );
        }
    }
};

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
        midday
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
        midday
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
        midday
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
        midday
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
            ifchar( 'u', ter_set( ter_str_id( "t_brick_wall" ) ) + ter_set_flat_roof_above ) ||
            t.set_up_tiles;

        t.section_prefix = "walls_";
    } else {
        // second scenario: player is surrounded by thick smoke
        // overriding 'u' to set thick smoke everywhere
        t.set_up_tiles = [&]( map_test_case::tile t ) {
            get_map().add_field( t.p, field_type_str_id( "fd_smoke" ) );
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
        midday
    };

    t.test_all();
}

TEST_CASE( "vision_inside_meth_lab", "[shadowcasting][vision]" )
{
    clear_vehicles();

    bool door_open = GENERATE( false, true );

    vision_test_case t {
        {
            "  M M  ", // left M is origin location of meth lab (driver's seat)
            "       ",
            "       ",
            "   U   ",
            "       ",
            "       ",
            "   D   ", // D mark door to be opened
            "       "
        },
        door_open ?
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
        midday
    };

    vehicle *v = nullptr;
    cata::optional<tripoint> door = cata::nullopt;

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
        cata::optional<units::angle> dir;
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
            v = get_map().add_vehicle( vproto_id( "meth_lab" ), tile.p, *dir, 0, 0 );
            for( const vpart_reference &vp : v->get_avail_parts( "OPENABLE" ) ) {
                v -> close( vp.part_index() );
            }
            open_door();
        }
        return true;
    };

    t.set_up_tiles =
        ifchar( 'M', spawn_meth_lab ) ||
        ifchar( 'D', set_door_location ) ||
        t.set_up_tiles;

    t.test_all();
    clear_vehicles();
}
