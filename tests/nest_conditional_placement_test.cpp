#include "city.h"
#include "coordinates.h"
#include "game_constants.h"
#include "map.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "test_data.h"

static const oter_str_id field( "field" );
static const oter_str_id ncpt_predecessors( "ncpt_predecessors" );

static const overmap_special_id test_mutable( "nest_conditional_placement_test_mutable" );

static const ter_str_id placed( "t_linoleum_white" );
static const ter_str_id unplaced( "t_linoleum_gray" );

//Check unconditional nests place and that each nest condition works as intended
TEST_CASE( "nest_conditional_placement", "[map][nest]" )
{
    //Create a fresh overmap with no specials to guarentee no preexisting joins. Placed at an arbitary point so it doesn't overwrite other tests overmaps
    const point_abs_om overmap_point( 122, 122 );
    const std::vector<const overmap_special *> empty_specials;
    overmap_special_batch empty_special_batch( overmap_point, empty_specials );
    overmap_buffer.create_custom_overmap( overmap_point, empty_special_batch );
    overmap *om = overmap_buffer.get_existing( overmap_point );

    const overmap_special &special = *test_mutable;
    const tripoint_om_omt test_omt_pos( OMAPX / 2, OMAPY / 2, 0 );
    const tripoint_rel_omt tinymap_point_offset( OMAPX / 2, OMAPY / 2, 0 );
    const tripoint_abs_omt tinymap_point = om->global_base_point() + tinymap_point_offset;
    const om_direction::type dir = om_direction::type::north;
    const city cit;

    //Add land to guarentee mutable placement
    for( int i = -1; i < 2; i++ ) {
        for( int j = -1; j < 2; j++ ) {
            tripoint_rel_omt offset( i, j, 0 );
            om->ter_set( test_omt_pos + offset, field.id() );
        }
    }
    //Add the predecessor omt
    om->ter_set( test_omt_pos, ncpt_predecessors.id() );
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

    //The y0 row is always supposed to place while the y1 row should always fail
    const bool unconditional_success = tm.ter( { 0, 0, 0 } ) == placed &&
                                       tm.ter( { 0, 1, 0 } ) == unplaced;
    const bool neighbors_success = tm.ter( { 1, 0, 0 } ) == placed &&
                                   tm.ter( { 1, 1, 0 } ) == unplaced;
    const bool joins_success = tm.ter( { 2, 0, 0 } ) == placed &&
                               tm.ter( { 2, 1, 0 } ) == unplaced;
    const bool flags_success = tm.ter( { 3, 0, 0 } ) == placed &&
                               tm.ter( { 3, 1, 0 } ) == unplaced;
    const bool flags_any_success = tm.ter( { 4, 0, 0 } ) == placed &&
                                   tm.ter( { 4, 1, 0 } ) == unplaced;
    const bool predecessors_success = tm.ter( { 5, 0, 0 } ) == placed &&
                                      tm.ter( { 5, 1, 0 } ) == unplaced;
    const bool z_check_success = tm.ter( { 6, 0, 0 } ) == placed &&
                                 tm.ter( { 6, 1, 0 } ) == unplaced;
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
        const bool om_terrain_match_type_success = tm.ter( { 22, 0, 0 } ) == placed &&
                tm.ter( { 22, 1, 0 } ) == unplaced;
        //Check that all conditions are required when multiple are specified
        const bool multiconditional_success = tm.ter( { 23, 0, 0 } ) == placed &&
                                              tm.ter( { 23, 1, 0 } ) == unplaced;
        CHECK( om_terrain_match_type_success );
        CHECK( multiconditional_success );
    }
}
