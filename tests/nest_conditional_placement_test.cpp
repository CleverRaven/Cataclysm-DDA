#include <vector>

#include "cata_catch.h"
#include "city.h"
#include "coordinates.h"
#include "map.h"
#include "map_scale_constants.h"
#include "omdata.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "point.h"
#include "type_id.h"

static const oter_str_id oter_field( "field" );
static const oter_str_id oter_ncpt_predecessors( "ncpt_predecessors" );

static const overmap_special_id
overmap_special_nest_conditional_placement_test_mutable( "nest_conditional_placement_test_mutable" );

static const ter_str_id ter_t_linoleum_gray( "t_linoleum_gray" ); //Signifies the nest wasn't placed
static const ter_str_id ter_t_linoleum_white( "t_linoleum_white" ); //Signifies the nest was placed

//Check unconditional nests place and that each nest condition works as intended
TEST_CASE( "nest_conditional_placement", "[map][nest]" )
{
    //Create a fresh overmap with no specials to guarentee no preexisting joins. Placed at an arbitary point so it doesn't overwrite other tests overmaps
    const point_abs_om overmap_point( 122, 122 );
    const std::vector<const overmap_special *> empty_specials;
    overmap_special_batch empty_special_batch( overmap_point, empty_specials );
    overmap_buffer.create_custom_overmap( overmap_point, empty_special_batch );
    overmap *om = overmap_buffer.get_existing( overmap_point );

    const overmap_special &special = *overmap_special_nest_conditional_placement_test_mutable;
    const tripoint_om_omt test_omt_pos( OMAPX / 2, OMAPY / 2, 0 );
    const tripoint_rel_omt tinymap_point_offset( OMAPX / 2, OMAPY / 2, 0 );
    const tripoint_abs_omt tinymap_point = om->global_base_point() + tinymap_point_offset;
    const om_direction::type dir = om_direction::type::north;
    const city cit;

    //Add land to guarentee mutable placement
    for( int i = -1; i < 2; i++ ) {
        for( int j = -1; j < 2; j++ ) {
            tripoint_rel_omt offset( i, j, 0 );
            om->ter_set( test_omt_pos + offset, oter_field.id() );
        }
    }
    //Add the predecessor omt
    om->ter_set( test_omt_pos, oter_ncpt_predecessors.id() );
    //Place the mutable over the predecessor omt
    if( om->can_place_special( special, test_omt_pos, dir, false ) ) {
        std::vector<tripoint_om_omt> placed_points =
            om->place_special( special, test_omt_pos, dir, cit, false, false );
        //Require that the mutable placed and placed more than just the root
        const bool mutable_placed_successfully = placed_points.size() > 1;
        REQUIRE( mutable_placed_successfully );
    }

    tinymap tm;
    tm.load( tinymap_point, false );

    //The y0 row has conditions that should result in t_linoleum_white being placed while the y1 row has conditions that should always fail to place t_linoleum_white
    //Can remove tripoint_omt_ms disambiguation once tinymap::ter raw tripoint overload is removed
    const bool unconditional_success = tm.ter( tripoint_omt_ms{ 0, 0, 0 } ) == ter_t_linoleum_white &&
                                       tm.ter( tripoint_omt_ms{ 0, 1, 0 } ) == ter_t_linoleum_gray;
    const bool neighbors_success = tm.ter( tripoint_omt_ms{ 1, 0, 0 } ) == ter_t_linoleum_white &&
                                   tm.ter( tripoint_omt_ms{ 1, 1, 0 } ) == ter_t_linoleum_gray;
    const bool joins_success = tm.ter( tripoint_omt_ms{ 2, 0, 0 } ) == ter_t_linoleum_white &&
                               tm.ter( tripoint_omt_ms{ 2, 1, 0 } ) == ter_t_linoleum_gray;
    const bool flags_success = tm.ter( tripoint_omt_ms{ 3, 0, 0 } ) == ter_t_linoleum_white &&
                               tm.ter( tripoint_omt_ms{ 3, 1, 0 } ) == ter_t_linoleum_gray;
    const bool flags_any_success = tm.ter( tripoint_omt_ms{ 4, 0, 0 } ) == ter_t_linoleum_white &&
                                   tm.ter( tripoint_omt_ms{ 4, 1, 0 } ) == ter_t_linoleum_gray;
    const bool predecessors_success = tm.ter( tripoint_omt_ms{ 5, 0, 0 } ) == ter_t_linoleum_white &&
                                      tm.ter( tripoint_omt_ms{ 5, 1, 0 } ) == ter_t_linoleum_gray;
    const bool z_check_success = tm.ter( tripoint_omt_ms{ 6, 0, 0 } ) == ter_t_linoleum_white &&
                                 tm.ter( tripoint_omt_ms{ 6, 1, 0 } ) == ter_t_linoleum_gray;
    CHECK( unconditional_success );
    CHECK( neighbors_success );
    CHECK( joins_success );
    CHECK( flags_success );
    CHECK( flags_any_success );
    CHECK( z_check_success );
    //Only run more advanced checks if we passed all the basic ones so as to not muddy more basic issues
    if( unconditional_success && neighbors_success && joins_success && flags_success &&
        flags_any_success && predecessors_success && z_check_success ) {
        //Check the neighbors condition works with om_terrain_match_type
        const bool om_terrain_match_type_success = tm.ter( tripoint_omt_ms{ 22, 0, 0 } ) ==
                ter_t_linoleum_white &&
                tm.ter( tripoint_omt_ms{ 22, 1, 0 } ) == ter_t_linoleum_gray;
        //Check that all conditions are required when multiple are specified
        const bool multiconditional_success = tm.ter( tripoint_omt_ms{ 23, 0, 0 } ) == ter_t_linoleum_white
                                              &&
                                              tm.ter( tripoint_omt_ms{ 23, 1, 0 } ) == ter_t_linoleum_gray;
        CHECK( om_terrain_match_type_success );
        CHECK( multiconditional_success );
    }
}
