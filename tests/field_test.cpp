#include "catch/catch.hpp"

#include "field.h"
#include "map.h"
#include "map_iterator.h"
#include "type_id.h"

#include "map_helpers.h"

#include <algorithm>

static int count_fields( const field_type_str_id &field_type )
{
    map &m = get_map();
    int live_fields = 0;
    for( const tripoint &cursor : m.points_on_zlevel() ) {
        if( m.get_field( cursor, field_type ) != nullptr ) {
            live_fields++;
        }
    }
    return live_fields;
}

// This is a large scale integration test, it gets prohibitively expensive if it uses
// a long-lived field, and it isn't capable of handling fields that trigger spread/join mechanics
// or dynamically update their own lifetimes.
TEST_CASE( "acid_field_expiry_on_map", "[field]" )
{
    clear_map();
    map &m = get_map();
    const field_type_str_id field_type( "fd_acid" );
    // place a smoke field
    for( const tripoint &cursor : m.points_on_zlevel() ) {
        m.add_field( cursor, field_type, 1 );
    }
    REQUIRE( count_fields( field_type ) == 17424 );
    const time_point before_time = calendar::turn;
    // run time forward until it goes away
    while( calendar::turn - before_time < field_type.obj().half_life ) {
        m.process_fields();
        calendar::turn += 1_seconds;
    }

    CHECK( count_fields( field_type ) == Approx( 8712 ).margin( 200 ) );
}

static void test_field_expiry( const std::string &field_type_str )
{
    const field_type_str_id field_type( field_type_str );
    std::vector<field_entry> test_fields;
    for( int i = 0; i < 10000; ++i ) {
        test_fields.emplace_back( field_type, 1, 0_seconds );
        test_fields[i].do_decay();
    }
    // Reduce time advancement by 2 seconds because age gets incremented by do_decay()
    calendar::turn += field_type.obj().half_life - 2_seconds;
    float decayed = 0.0f;
    float alive = 0.0f;
    for( field_entry &test_field : test_fields ) {
        test_field.do_decay();
        if( test_field.is_field_alive() ) {
            alive += 1.0f;
        } else {
            decayed += 1.0f;
        }
    }
    CAPTURE( field_type_str );
    CHECK( alive == Approx( decayed ).epsilon( 0.1f ) );
}

TEST_CASE( "field_expiry", "[field]" )
{
    // Test fields with a wide range of half lives.
    test_field_expiry( "fd_acid" );
    test_field_expiry( "fd_smoke" );
    test_field_expiry( "fd_blood" );
    test_field_expiry( "fd_sludge" );
    test_field_expiry( "fd_extinguisher" );
    test_field_expiry( "fd_electricity" );
}

static void fire_duration( const std::string &terrain_type, const time_duration minimum,
                           const time_duration maximum )
{
    clear_map();
    const tripoint fire_loc{ 33, 33, 0 };
    map &m = get_map();
    m.ter_set( fire_loc, ter_id( terrain_type ) );
    m.add_field( fire_loc, fd_fire, 1, 10_minutes );
    REQUIRE( m.get_field( fire_loc, fd_fire ) );
    CHECK( m.get_field( fire_loc, fd_fire )->is_field_alive() );
    const time_point before_time = calendar::turn;
    bool field_alive = true;
    while( field_alive && calendar::turn - before_time < minimum ) {
        m.process_fields();
        calendar::turn += 1_seconds;
        const int effective_age = to_turns<int>( calendar::turn - before_time );
        INFO( effective_age << " seconds" );
        field_entry *this_field = m.get_field( fire_loc, fd_fire );
        field_alive = this_field && this_field->is_field_alive();
    }
    CHECK( field_alive );
    while( field_alive && calendar::turn - before_time < maximum ) {
        m.process_fields();
        calendar::turn += 1_seconds;
        const int effective_age = to_turns<int>( calendar::turn - before_time );
        INFO( effective_age << " seconds" );
        field_entry *this_field = m.get_field( fire_loc, fd_fire );
        field_alive = this_field && this_field->is_field_alive();
    }
    CHECK( !field_alive );
}

TEST_CASE( "firebugs", "[field]" )
{
    fire_duration( "t_grass", 5_minutes, 30_minutes );
    fire_duration( "t_shrub_raspberry", 60_minutes, 120_minutes );
}
