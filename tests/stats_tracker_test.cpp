#include "catch/catch.hpp"

#include "avatar.h"
#include "game.h"
#include "stats_tracker.h"

TEST_CASE( "stats_tracker_count_events", "[stats]" )
{
    stats_tracker s;
    event_bus b;
    b.subscribe( &s );

    const character_id u_id = g->u.getID();
    const mtype_id mon1( "mon_zombie" );
    const mtype_id mon2( "mon_zombie_brute" );
    const cata::event kill1 = cata::event::make<event_type::character_kills_monster>( u_id, mon1 );
    const cata::event kill2 = cata::event::make<event_type::character_kills_monster>( u_id, mon2 );
    const cata::event::data_type char_is_player{ { "killer", cata_variant( u_id ) } };

    CHECK( s.count( kill1 ) == 0 );
    CHECK( s.count( kill2 ) == 0 );
    CHECK( s.count( event_type::character_kills_monster, char_is_player ) == 0 );
    b.send( kill1 );
    CHECK( s.count( kill1 ) == 1 );
    CHECK( s.count( kill2 ) == 0 );
    CHECK( s.count( event_type::character_kills_monster, char_is_player ) == 1 );
    b.send( kill2 );
    CHECK( s.count( kill1 ) == 1 );
    CHECK( s.count( kill2 ) == 1 );
    CHECK( s.count( event_type::character_kills_monster, char_is_player ) == 2 );
}

TEST_CASE( "stats_tracker_in_game", "[stats]" )
{
    g->stats().clear();
    cata::event e = cata::event::make<event_type::awakes_dark_wyrms>();
    g->events().send( e );
    CHECK( g->stats().count( e ) == 1 );
}
