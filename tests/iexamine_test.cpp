#include <cstddef>
#include <functional>
#include <sstream>
#include <string>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "coordinates.h"
#include "flexbuffer_json.h"
#include "iexamine.h"
#include "json.h"
#include "json_loader.h"
#include "map.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "mapdata.h"
#include "options_helpers.h"
#include "player_activity.h"
#include "player_helpers.h"
#include "point.h"
#include "string_formatter.h"
#include "translation.h"
#include "type_id.h"

static const activity_id ACT_HARVEST( "ACT_HARVEST" );

static const ter_str_id ter_t_tree_apple( "t_tree_apple" );
static const ter_str_id ter_t_tree_apple_harvested( "t_tree_apple_harvested" );

TEST_CASE( "mapdata_examine" )
{
    ter_t data;
    data.set_examine( iexamine_functions{iexamine::always_true, iexamine::water_source, no_translation( "test water source" ) } );

    CHECK( data.has_examine( iexamine::water_source ) );
    CHECK_FALSE( data.has_examine( iexamine::fungus ) );
    CHECK_FALSE( data.has_examine( iexamine::dirtmound ) );
    CHECK_FALSE( data.has_examine( iexamine::none ) );
}

TEST_CASE( "examine_bush" )
{
    clear_map_without_vision();
    map &m = get_map();
    const tripoint_bub_ms &pine_loc = tripoint_bub_ms::zero;
    const tripoint_bub_ms &elderberry_loc = pine_loc + tripoint::east;

    m.ter_set( pine_loc, ter_id( "t_tree_pine" ) );
    m.ter_set( elderberry_loc, ter_id( "t_tree_elderberry" ) );

    CHECK( m.ter( pine_loc )->has_examine( iexamine::harvest_ter ) );
    CHECK( m.ter( elderberry_loc )->has_examine( iexamine::harvest_ter_nectar ) );

    // In spring, pine is harvestable but elderberry is not
    calendar::turn = calendar::turn_zero;
    CHECK( m.ter( pine_loc )->can_examine( pine_loc ) );
    CHECK_FALSE( m.ter( elderberry_loc )->can_examine( elderberry_loc ) );

    // In summer, both are harvestable
    calendar::turn = calendar::turn_zero + calendar::season_length() + 1_days;
    CHECK( m.ter( pine_loc )->can_examine( pine_loc ) );
    CHECK( m.ter( elderberry_loc )->can_examine( elderberry_loc ) );

    // In fall, just pine again
    calendar::turn = calendar::turn_zero + calendar::season_length() * 2 + 1_days;
    CHECK( m.ter( pine_loc )->can_examine( pine_loc ) );
    CHECK_FALSE( m.ter( elderberry_loc )->can_examine( elderberry_loc ) );
}

static void finish_activity( avatar &u )
{
    for( int turn = 0; turn < 100 && u.activity; ++turn ) {
        u.set_moves( u.get_speed() );
        u.activity.do_turn( u );
    }
}

TEST_CASE( "harvest_finishes_on_its_tree_after_the_map_shifts", "[iexamine][harvest]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    avatar &u = get_avatar();
    // autumn, apple tree bearing fruit
    set_time( calendar::turn_zero + 2 * calendar::season_length() + 12_hours );
    override_option auto_features( "AUTO_FEATURES", "true" );
    override_option auto_foraging( "AUTO_FORAGING", "trees" );
    const tripoint_bub_ms tree = u.pos_bub() + tripoint::east;
    here.ter_set( tree, ter_t_tree_apple );
    const tripoint_abs_ms tree_abs = here.get_abs( tree );

    GIVEN( "avatar starts picking apples" ) {
        iexamine::harvest_ter( u, tree );
        REQUIRE( u.activity.id() == ACT_HARVEST );

        WHEN( "map shifts before harvest ends" ) {
            here.shift( point_rel_sm::east );
            on_out_of_scope shift_back( [&here]() {
                here.shift( point_rel_sm::west );
            } );
            REQUIRE( here.get_bub( tree_abs ) != tree );
            finish_activity( u );
            THEN( "apple tree is the one harvested" ) {
                CHECK_FALSE( u.activity );
                CHECK( here.ter( here.get_bub( tree_abs ) ) == ter_t_tree_apple_harvested );
                CHECK( here.ter( tree ) != ter_t_tree_apple_harvested );
            }
        }
        WHEN( "harvest saved, map shifted, harvest loaded" ) {
            std::ostringstream os;
            JsonOut jsout( os );
            u.activity.serialize( jsout );
            here.shift( point_rel_sm::east );
            on_out_of_scope shift_back( [&here]() {
                here.shift( point_rel_sm::west );
            } );
            u.activity = player_activity();
            u.activity.deserialize( json_loader::from_string( os.str() ).get_object() );
            REQUIRE( u.activity.id() == ACT_HARVEST );
            finish_activity( u );
            THEN( "apple tree is the one harvested" ) {
                CHECK_FALSE( u.activity );
                CHECK( here.ter( here.get_bub( tree_abs ) ) == ter_t_tree_apple_harvested );
                CHECK( here.ter( tree ) != ter_t_tree_apple_harvested );
            }
        }
        WHEN( "save holds harvest target as bubble position" ) {
            std::ostringstream os;
            JsonOut jsout( os );
            u.activity.serialize( jsout );
            std::string saved = os.str();
            const size_t key = saved.find( "\"target_abs\":" );
            REQUIRE( key != std::string::npos );
            const size_t end = saved.find( ']', key );
            saved.replace( key, end + 1 - key, string_format( "\"target\":[%d,%d,%d]", tree.x(),
                           tree.y(), tree.z() ) );
            u.activity = player_activity();
            u.activity.deserialize( json_loader::from_string( saved ).get_object() );
            REQUIRE( u.activity.id() == ACT_HARVEST );
            finish_activity( u );
            THEN( "load reads it on the map it was saved with" ) {
                CHECK_FALSE( u.activity );
                CHECK( here.ter( tree ) == ter_t_tree_apple_harvested );
            }
        }
    }
}
