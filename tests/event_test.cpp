#include "catch/catch.hpp"

#include "event.h"
#include "event_bus.h"
#include "type_id.h"

using itype_id = std::string;

TEST_CASE( "construct_event", "[event]" )
{
    time_point time = calendar::turn_zero + 3_hours;
    event e = event::make<event_type::kill_monster>(
                  time, mtype_id( "zombie" ), itype_id( "knife_spear" ) );
    CHECK( e.type() == event_type::kill_monster );
    CHECK( e.time() == time );
    CHECK( e.get<cata_variant_type::mtype_id>( "victim_type" ) == mtype_id( "zombie" ) );
    CHECK( e.get<cata_variant_type::itype_id>( "weapon_type" ) == itype_id( "knife_spear" ) );
    CHECK( e.get<mtype_id>( "victim_type" ) == mtype_id( "zombie" ) );
    CHECK( e.get<itype_id>( "weapon_type" ) == itype_id( "knife_spear" ) );
}

struct test_subscriber : event_subscriber {
    test_subscriber( event_bus &bus ) {
        bus.subscribe( this );
    }
    void notify( const event &e ) override {
        events.push_back( e );
    }

    std::vector<event> events;
};

TEST_CASE( "send_event_through_bus", "[event]" )
{
    event_bus bus;
    test_subscriber sub( bus );

    time_point time = calendar::turn_zero + 5_days;
    bus.send( event::make<event_type::kill_monster>(
                  time, mtype_id( "zombie" ), itype_id( "knife_spear" ) ) );
    REQUIRE( sub.events.size() == 1 );
    const event &e = sub.events[0];
    CHECK( e.type() == event_type::kill_monster );
    CHECK( e.time() == time );
    CHECK( e.get<mtype_id>( "victim_type" ) == mtype_id( "zombie" ) );
    CHECK( e.get<itype_id>( "weapon_type" ) == itype_id( "knife_spear" ) );
}
