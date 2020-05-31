#include <algorithm>
#include <string>
#include <vector>

#include "calendar.h"
#include "cata_variant.h"
#include "catch/catch.hpp"
#include "character_id.h"
#include "event.h"
#include "event_bus.h"
#include "string_id.h"
#include "type_id.h"

TEST_CASE( "construct_event", "[event]" )
{
    cata::event e = cata::event::make<event_type::character_kills_monster>(
                        character_id( 7 ), mtype_id( "zombie" ) );
    CHECK( e.type() == event_type::character_kills_monster );
    CHECK( e.time() == calendar::turn );
    CHECK( e.get<cata_variant_type::character_id>( "killer" ) == character_id( 7 ) );
    CHECK( e.get<cata_variant_type::mtype_id>( "victim_type" ) == mtype_id( "zombie" ) );
    CHECK( e.get<character_id>( "killer" ) == character_id( 7 ) );
    CHECK( e.get<mtype_id>( "victim_type" ) == mtype_id( "zombie" ) );
}

struct test_subscriber : public event_subscriber {
    void notify( const cata::event &e ) override {
        events.push_back( e );
    }

    std::vector<cata::event> events;
};

TEST_CASE( "send_event_through_bus", "[event]" )
{
    event_bus bus;
    test_subscriber sub;
    bus.subscribe( &sub );

    bus.send( cata::event::make<event_type::character_kills_monster>(
                  character_id( 5 ), mtype_id( "zombie" ) ) );
    REQUIRE( sub.events.size() == 1 );
    const cata::event &e = sub.events[0];
    CHECK( e.type() == event_type::character_kills_monster );
    CHECK( e.time() == calendar::turn );
    CHECK( e.get<character_id>( "killer" ) == character_id( 5 ) );
    CHECK( e.get<mtype_id>( "victim_type" ) == mtype_id( "zombie" ) );
}

TEST_CASE( "destroy_bus_before_subscriber", "[event]" )
{
    test_subscriber sub;
    event_bus bus;
    bus.subscribe( &sub );

    bus.send( cata::event::make<event_type::character_kills_monster>(
                  character_id( 5 ), mtype_id( "zombie" ) ) );
    CHECK( sub.events.size() == 1 );
}
