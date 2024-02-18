#include <set>
#include <sstream>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "field.h"
#include "json.h"
#include "json_loader.h"
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

static std::set<tripoint_abs_ms> count_fields_near(
    const tripoint_abs_ms &p, const field_type_str_id &field_type )
{
    map &m = get_map();
    std::set<tripoint_abs_ms> live_fields;
    for( const tripoint_abs_ms &cursor : closest_points_first( p, 10 ) ) {
        field_entry *entry = m.get_field( m.getlocal( cursor ), field_type );
        if( entry && entry->is_field_alive() ) {
            live_fields.insert( cursor );
        }
    }
    return live_fields;
}

TEST_CASE( "line_attack", "[magic]" )
{
    // manually construct a testable spell
    JsonObject obj = json_loader::from_string(
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
    // Test relies on lighting conditions, so ensure we control the level of
    // daylight.
    set_time( calendar::turn_zero );
    clear_map();

    map &m = get_map();
    spell sp( spell_AO_CLOSE_TEAR );

    avatar &dummy = get_avatar();
    clear_avatar();
    tripoint player_initial_pos = dummy.pos();

    const auto setup_and_remove_fields = [&]( const bool & with_light ) {
        CAPTURE( with_light );
        CHECK( dummy.pos() == player_initial_pos );
        tripoint_abs_ms player_global_pos = m.getglobal( player_initial_pos );

        // create fd_fatigue of each intensity near player
        tripoint_abs_ms p1 = player_global_pos + tripoint_east * 10;
        tripoint_abs_ms p2 = player_global_pos + tripoint_east * 11;
        tripoint_abs_ms p3 = player_global_pos + tripoint_east * 12;
        tripoint_abs_ms p4 = player_global_pos + tripoint_east * 13;
        m.add_field( m.getlocal( p1 ), fd_fatigue, 1, 1_hours );
        m.add_field( m.getlocal( p2 ), fd_fatigue, 2, 1_hours );
        m.add_field( m.getlocal( p3 ), fd_fatigue, 3, 1_hours );
        m.add_field( m.getlocal( p4 ), fd_fatigue, 3, 1_hours );

        if( with_light ) {
            player_add_headlamp();
        }

        m.update_visibility_cache( 0 );
        m.invalidate_map_cache( 0 );
        m.build_map_cache( 0 );
        dummy.recalc_sight_limits();

        CHECK( m.getglobal( dummy.pos() ) == player_global_pos );
        CHECK( count_fields_near( p1, fd_fatigue ) == std::set<tripoint_abs_ms> { p1, p2, p3, p4 } );

        spell_effect::remove_field( sp, dummy, player_initial_pos );
        calendar::turn += 1_turns;
        m.process_fields();
        calendar::turn += 1_turns;
        m.process_fields();

        CHECK( m.getglobal( dummy.pos() ) == player_global_pos );
        CHECK( count_fields_near( p1, fd_fatigue ) == std::set<tripoint_abs_ms> { p2, p3, p4 } );

        spell_effect::remove_field( sp, dummy, player_initial_pos );
        calendar::turn += 1_turns;
        m.process_fields();
        calendar::turn += 1_turns;
        m.process_fields();

        CHECK( m.getglobal( dummy.pos() ) == player_global_pos );
        CHECK( count_fields_near( p1, fd_fatigue ) == std::set<tripoint_abs_ms> { p3, p4 } );

        spell_effect::remove_field( sp, dummy, player_initial_pos );
        calendar::turn += 1_turns;
        m.process_fields();
        calendar::turn += 1_turns;
        m.process_fields();

        CHECK( count_fields_near( p1, fd_fatigue ) == std::set<tripoint_abs_ms> { p4 } );
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

    CAPTURE( calendar::turn );

    capture_removal_messages();

    CHECK( odd_ripple_msg == "The odd ripple fades.  You feel strange." );
    CHECK( swirling_air_msg == "The swirling air dissipates.  You feel strange." );
    CHECK( tear_in_reality_msg ==
           "The tear in reality pulls you in as it closes and ejects you violently!" );

    // check that the player got teleported
    CHECK( dummy.pos() != player_initial_pos );

    // remove 3 fields again but without lighting this time
    clear_avatar();
    clear_map();
    Messages::clear_messages();

    setup_and_remove_fields( false );
    capture_removal_messages();

    CHECK( odd_ripple_msg.empty() );
    CHECK( swirling_air_msg.empty() );
    CHECK( tear_in_reality_msg ==
           "A nearby tear in reality pulls you in as it closes and ejects you violently!" );
}
