#include <set>
#include <sstream>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "field.h"
#include "json.h"
#include "magic.h"
#include "magic_spell_effect_helpers.h"
#include "map.h"
#include "map_helpers.h"
#include "messages.h"
#include "npc.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"

static const spell_id spell_AO_CLOSE_TEAR( "AO_CLOSE_TEAR" );
static const spell_id spell_test_line_spell( "test_line_spell" );

static int count_fields( const field_type_str_id &field_type )
{
    map &m = get_map();
    int live_fields = 0;
    for( const tripoint &cursor : m.points_on_zlevel() ) {
        field_entry *entry = m.get_field( cursor, field_type );
        if( entry && entry->is_field_alive() ) {
            live_fields++;
        }
    }
    return live_fields;
}

TEST_CASE( "line_attack", "[magic]" )
{
    // manually construct a testable spell
    std::istringstream str(
        "  {\n"
        "    \"id\": \"test_line_spell\",\n"
        "    \"name\": { \"str\": \"Test Line Spell\" },\n"
        "    \"description\": \"Spews a line of magic\",\n"
        "    \"valid_targets\": [ \"ground\" ],\n"
        "    \"damage_type\": \"none\",\n"
        "    \"min_range\": 5,\n"
        "    \"max_range\": 5,\n"
        "    \"effect\": \"attack\",\n"
        "    \"shape\": \"line\","
        "    \"min_aoe\": 0,\n"
        "    \"max_aoe\": 0,\n"
        "    \"flags\": [ \"VERBAL\", \"NO_HANDS\", \"NO_LEGS\" ]\n"
        "  }\n" );

    JsonIn in( str );
    JsonObject obj( in );
    spell_type::load_spell( obj, "" );

    spell sp( spell_test_line_spell );

    // set up Character to test with, only need position
    npc &c = spawn_npc( point_zero, "test_talker" );
    clear_character( c );
    c.setpos( tripoint_zero );

    // target point 5 tiles east of zero
    tripoint target = tripoint_east * 5;

    // Ensure that AOE=0 spell covers the 5 tiles along vector towards target
    SECTION( "aoe=0" ) {
        const std::set<tripoint> reference( { tripoint_east * 1, tripoint_east * 2, tripoint_east * 3, tripoint_east * 4, tripoint_east * 5 } );

        std::set<tripoint> targets = calculate_spell_effect_area( sp, target, c );

        CHECK( reference == targets );
    }
}

TEST_CASE( "remove_field_fd_fatigue", "[magic]" )
{
    clear_map();

    map &m = get_map();
    spell sp( spell_AO_CLOSE_TEAR );

    avatar &dummy = get_avatar();
    clear_avatar();
    tripoint player_initial_pos;

    dummy.setpos( player_initial_pos );

    const auto setup_and_remove_fields = [&]( const bool & with_light ) {
        // create fd_fatigue of each intensity near player
        m.add_field( tripoint_east, fd_fatigue, 1, 1_hours );
        m.add_field( tripoint_east * 2, fd_fatigue, 2, 1_hours );
        m.add_field( tripoint_east * 3, fd_fatigue, 3, 1_hours );

        if( with_light ) {
            player_add_headlamp();
        }

        m.update_visibility_cache( 0 );
        m.invalidate_map_cache( 0 );
        m.build_map_cache( 0 );
        m.build_lightmap( 0, dummy.pos() );
        dummy.recalc_sight_limits();

        spell_effect::remove_field( sp, dummy, player_initial_pos );
        calendar::turn += 1_turns;
        m.process_fields();
        calendar::turn += 1_turns;
        m.process_fields();

        CHECK( count_fields( fd_fatigue ) == 2 );

        spell_effect::remove_field( sp, dummy, player_initial_pos );
        calendar::turn += 1_turns;
        m.process_fields();
        calendar::turn += 1_turns;
        m.process_fields();

        CHECK( count_fields( fd_fatigue ) == 1 );

        spell_effect::remove_field( sp, dummy, player_initial_pos );
        calendar::turn += 1_turns;
        m.process_fields();
        calendar::turn += 1_turns;
        m.process_fields();

        CHECK( count_fields( fd_fatigue ) == 0 );
    };

    setup_and_remove_fields( true );

    std::string odd_ripple_msg;
    std::string swirling_air_msg;
    std::string tear_in_reality_msg;

    const auto capture_removal_messages = [ &odd_ripple_msg, &swirling_air_msg,
                     &tear_in_reality_msg ]() {
        odd_ripple_msg = "";
        swirling_air_msg = "";
        tear_in_reality_msg = "";

        std::vector<std::pair<std::string, std::string>> msgs = Messages::recent_messages( 0 );

        for( std::pair<std::string, std::string> msg_pair : msgs ) {
            std::string msg = std::get<1>( msg_pair );

            if( msg.find( "odd ripple" ) != std::string::npos ) {
                odd_ripple_msg = msg;
            } else if( msg.find( "swirling air" ) != std::string::npos ) {
                swirling_air_msg = msg;
            } else if( msg.find( "tear in reality" ) != std::string::npos ) {
                tear_in_reality_msg = msg;
            }
        }
    };

    capture_removal_messages();

    CHECK( odd_ripple_msg == "The odd ripple fades.  You feel strange." );
    CHECK( swirling_air_msg == "The swirling air dissipates.  You feel strange." );
    CHECK( tear_in_reality_msg ==
           "The tear in reality pulls you in as it closes and ejects you violently!" );

    // check that the player got teleported
    CHECK( dummy.pos() != player_initial_pos );

    // remove 3 fields again but without lighting this time
    clear_map();
    clear_avatar();
    Messages::clear_messages();

    setup_and_remove_fields( false );
    capture_removal_messages();

    CHECK( odd_ripple_msg.empty() );
    CHECK( swirling_air_msg.empty() );
    CHECK( tear_in_reality_msg ==
           "A nearby tear in reality pulls you in as it closes and ejects you violently!" );
}
