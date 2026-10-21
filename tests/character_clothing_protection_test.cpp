#include <functional>
#include <map>
#include <optional>
#include <string>

#include "avatar.h"
#include "bodypart.h"
#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "character_attire.h"
#include "coordinates.h"
#include "debug.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "map_iterator.h"
#include "options_helpers.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"
#include "units.h"
#include "weather.h"
#include "weather_type.h"

static const damage_type_id damage_heat( "heat" );
static const itype_id itype_test_extra_bodypart_clothing( "test_extra_bodypart_clothing" );
static const itype_id itype_test_protective_clothing( "test_protective_clothing" );

TEST_CASE( "clothing_warmth_ignores_absent_body_parts", "[character][clothing][warmth]" )
{
    avatar &dummy = get_avatar();
    clear_character( dummy );

    const bodypart_id tail_long( "tail_long" );
    REQUIRE_FALSE( dummy.has_part( tail_long ) );
    dummy.wear_item( item( itype_test_extra_bodypart_clothing ), false );

    std::map<bodypart_id, int> warmth;
    const std::string debug_message = capture_debugmsg_during( [&] {
        warmth = dummy.worn.warmth( dummy );
    } );

    CHECK( debug_message.empty() );
    CHECK( warmth.at( body_part_torso ) == 20 );
    CHECK( warmth.count( tail_long ) == 0 );
}

TEST_CASE( "worn_clothing_provides_wind_resistance", "[character][clothing][wind]" )
{
    avatar &dummy = get_avatar();
    clear_character( dummy );

    const std::map<bodypart_id, int> unprotected = dummy.get_wind_resistance();
    CHECK( unprotected.at( body_part_torso ) == 0 );

    dummy.wear_item( item( itype_test_protective_clothing ), false );

    const std::map<bodypart_id, int> protected_parts = dummy.get_wind_resistance();
    CHECK( protected_parts.at( body_part_torso ) == 100 );
    CHECK( protected_parts.at( body_part_head ) == 0 );
}

TEST_CASE( "wind_resistance_is_applied_independently_per_body_part", "[character][clothing][wind]" )
{
    avatar &dummy = get_avatar();
    clear_map();

    map &here = get_map();
    for( const tripoint_bub_ms &p : here.points_in_radius( dummy.pos_bub(), 1 ) ) {
        here.ter_set( p, ter_id( "t_grass" ) );
    }
    here.ter_set( dummy.pos_bub() + tripoint::above, ter_id( "t_open_air" ) );
    here.build_map_cache( dummy.posz() );
    here.build_map_cache( dummy.posz() + 1 );

    scoped_weather_override weather_clear( WEATHER_CLEAR );
    weather_clear.with_windspeed( 30 );
    restore_on_out_of_scope restore_temperature( get_weather().forced_temperature );
    get_weather().forced_temperature = units::from_fahrenheit( -10 );

    const auto hand_frostbite_timer = [&]( const bool wear_protective_clothing ) {
        clear_character( dummy );
        dummy.set_all_parts_temp_cur( BODYTEMP_VERY_COLD );
        dummy.set_all_parts_temp_conv( BODYTEMP_VERY_COLD );
        dummy.set_part_frostbite_timer( body_part_hand_l, 0 );
        if( wear_protective_clothing ) {
            dummy.wear_item( item( itype_test_protective_clothing ), false );
        }

        dummy.update_bodytemp();
        return dummy.get_part_frostbite_timer( body_part_hand_l );
    };

    // The test clothing protects only the torso.  It must not reduce the wind experienced by
    // the bare hand, which is processed later by update_bodytemp().
    const int unprotected = hand_frostbite_timer( false );
    const int torso_protected = hand_frostbite_timer( true );

    REQUIRE( unprotected > 8 );
    CHECK( torso_protected == unprotected );
}

TEST_CASE( "worn_clothing_protects_from_wetting", "[character][clothing][wetting]" )
{
    avatar &dummy = get_avatar();
    clear_character( dummy );
    dummy.wear_item( item( itype_test_protective_clothing ), false );
    dummy.set_part_wetness( body_part_torso, 0 );
    dummy.set_part_wetness( body_part_head, 0 );

    dummy.drench( 100, { body_part_torso }, false );
    CHECK( dummy.get_part_wetness( body_part_torso ) == 0 );

    dummy.drench( 100, { body_part_head }, false );
    CHECK( dummy.get_part_wetness( body_part_head ) > 0 );
}

TEST_CASE( "worn_clothing_protects_from_heat", "[character][clothing][heat]" )
{
    avatar &dummy = get_avatar();
    clear_character( dummy );

    const std::map<bodypart_id, int> unprotected = dummy.get_all_armor_type( damage_heat );
    dummy.wear_item( item( itype_test_protective_clothing ), false );
    const std::map<bodypart_id, int> protected_parts = dummy.get_all_armor_type( damage_heat );

    CHECK( protected_parts.at( body_part_torso ) > unprotected.at( body_part_torso ) );
    CHECK( protected_parts.at( body_part_head ) == unprotected.at( body_part_head ) );
}
