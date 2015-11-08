#include "catch/catch.hpp"

#include "player.h"
#include "npc.h"
#include "game.h"
#include "map.h"

#include <string>

void on_load_test( npc &who, calendar from, calendar to )
{
    calendar::turn = from;
    who.on_unload();
    calendar::turn = to;
    who.on_load();
}

void sane( const npc &who )
{
    CHECK( who.get_hunger() >= 0 );
    CHECK( who.thirst >= 0 );
    CHECK( who.fatigue >= -25 );
}

TEST_CASE("NPC on_load correctly approximates hunger/thirst/fatigue")
{

    npc model_npc;
    model_npc.normalize();
    model_npc.randomize();
    model_npc.set_hunger( 0 );
    model_npc.thirst = 0;
    model_npc.fatigue = 0;
    model_npc.remove_effect( "sleep" );

    SECTION("Awake for 10 minutes, gaining hunger/thirst/fatigue") {
        npc test_npc = model_npc;
        const int five_min_ticks = 2;
        on_load_test( test_npc, 0, MINUTES(5 * five_min_ticks) );

        const int margin = 1;
        CHECK( test_npc.get_hunger() <= five_min_ticks + margin );
        CHECK( test_npc.thirst <= five_min_ticks + margin );
        CHECK( test_npc.fatigue <= five_min_ticks + margin );
        CHECK( test_npc.get_hunger() >= five_min_ticks - margin );
        CHECK( test_npc.thirst >= five_min_ticks - margin );
        CHECK( test_npc.fatigue >= five_min_ticks - margin );
    }

    SECTION("Awake for 2 days, gaining hunger/thirst/fatigue") {
        npc test_npc = model_npc;
        const int five_min_ticks = 12 * 24 * 2;
        on_load_test( test_npc, 0, MINUTES(5 * five_min_ticks) );

        const int margin = 10;
        CHECK( test_npc.get_hunger() <= five_min_ticks + margin );
        CHECK( test_npc.thirst <= five_min_ticks + margin );
        CHECK( test_npc.fatigue <= five_min_ticks + margin );
        CHECK( test_npc.get_hunger() >= five_min_ticks - margin );
        CHECK( test_npc.thirst >= five_min_ticks - margin );
        CHECK( test_npc.fatigue >= five_min_ticks - margin );
    }

    SECTION("Sleeping for 6 hours, gaining hunger/thirst, losing fatigue") {
        npc test_npc = model_npc;
        test_npc.add_effect( "sleep", HOURS(6) );
        test_npc.fatigue = 1000;
        const int five_min_ticks = 12 * 6;
        on_load_test( test_npc, 0, MINUTES(5 * five_min_ticks) );

        const int margin = 10;
        CHECK( test_npc.get_hunger() <= five_min_ticks + margin );
        CHECK( test_npc.thirst <= five_min_ticks + margin );
        CHECK( test_npc.fatigue <= 1000 - five_min_ticks + margin );
        CHECK( test_npc.get_hunger() >= five_min_ticks - margin );
        CHECK( test_npc.thirst >= five_min_ticks - margin );
        CHECK( test_npc.fatigue >= 1000 - five_min_ticks - margin );
    }
}
