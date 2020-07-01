#include <sstream>

#include "catch/catch.hpp"
#include "json.h"
#include "magic.h"
#include "magic_spell_effect_helpers.h"
#include "npc.h"
#include "player_helpers.h"

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
        "    \"effect\": \"line_attack\",\n"
        "    \"min_aoe\": 0,\n"
        "    \"max_aoe\": 0,\n"
        "    \"flags\": [ \"VERBAL\", \"NO_HANDS\", \"NO_LEGS\" ]\n"
        "  }\n" );

    JsonIn in( str );
    JsonObject obj( in );
    spell_type::load_spell( obj, "" );

    spell sp( spell_id( "test_line_spell" ) );

    // set up Character to test with, only need position
    npc &c = spawn_npc( point_zero, "test_talker" );
    clear_character( c );
    c.setpos( tripoint_zero );

    // target point 5 tiles east of zero
    tripoint target = tripoint_east * 5;

    // Ensure that AOE=0 spell covers the 5 tiles along vector towards target
    SECTION( "aoe=0" ) {
        const std::set<tripoint> reference( { tripoint_east * 1, tripoint_east * 2, tripoint_east * 3, tripoint_east * 4, tripoint_east * 5 } );

        std::set<tripoint> targets = calculate_spell_effect_area( sp, target,
                                     spell_effect::spell_effect_line, c, true );

        CHECK( reference == targets );
    }
}
