#include <iosfwd>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "catch/catch.hpp"
#include "field.h"
#include "field_type.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"

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

    CHECK( count_fields( field_type ) == Approx( 8712 ).margin( 300 ) );
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
    CAPTURE( terrain_type );
    CAPTURE( to_string( minimum ) );
    CAPTURE( to_string( maximum ) );

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
    {
        INFO( "Fire duration: " << to_string( calendar::turn - before_time ) );
        CHECK( field_alive );
    }
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

// helper method that stores a global variable used to remember calendar::turn value before the test
static time_point &fields_test_time_before()
{
    static time_point time_before;
    return time_before;
}

static int fields_test_turns()
{
    return to_turns<int>( calendar::turn - fields_test_time_before() );
}

static void fields_test_setup()
{
    fields_test_time_before() = calendar::turn;
    clear_map();
}
static void fields_test_cleanup()
{
    calendar::turn = fields_test_time_before();
    clear_map();
}

TEST_CASE( "fd_acid falls down", "[field]" )
{
    fields_test_setup();

    const tripoint p{ 33, 33, 0 };
    map &m = get_map();

    m.add_field( p, fd_acid, 3 );

    REQUIRE_FALSE( m.valid_move( p, p + tripoint_below ) );
    REQUIRE( m.get_field( p, fd_acid ) );
    REQUIRE_FALSE( m.get_field( p + tripoint_below, fd_acid ) );

    calendar::turn += 1_turns;
    m.process_fields();
    calendar::turn += 1_turns;
    m.process_fields();

    // remove floor under the acid field
    m.ter_set( p, t_open_air );
    m.build_floor_cache( 0 );
    REQUIRE( m.valid_move( p, p + tripoint_below ) );

    calendar::turn += 1_turns;
    m.process_fields();
    calendar::turn += 1_turns;
    m.process_fields();

    {
        INFO( "acid field is dropped by exactly one point" );
        field_entry *acid_here = m.get_field( p, fd_acid );
        CHECK( ( !acid_here || !acid_here->is_field_alive() ) );
        CHECK( m.get_field( p + tripoint_below, fd_acid ) );
    }

    fields_test_cleanup();
}

TEST_CASE( "fire spreading", "[field]" )
{
    fields_test_setup();

    const tripoint p{ 33, 33, 0 };
    const tripoint far_p = p + tripoint_east * 3;

    map &m = get_map();

    m.add_field( p, fd_fire, 3 );

    const auto check_spreading = [&]( const time_duration time_limit ) {
        const int time_limit_turns = to_turns<int>( time_limit );
        while( !m.get_field( far_p, fd_fire ) && fields_test_turns() < time_limit_turns ) {
            calendar::turn += 1_turns;
            m.process_fields();
        }
        {
            INFO( string_format( "Fire should've spread to the far point in under %d turns",
                                 time_limit_turns ) );
            CHECK( fields_test_turns() < time_limit_turns );
        }
    };

    SECTION( "fire spreads on fd_web" ) {
        for( tripoint p0 = p; p0 != far_p + tripoint_east; p0 += tripoint_east ) {
            m.add_field( p0, fd_web, 1 );
        }
        // note: time limit here was chosen arbitrarily. It could be too low or too high.
        check_spreading( 5_minutes );
    }
    SECTION( "fire spreads on flammable items" ) {
        for( tripoint p0 = p; p0 != far_p + tripoint_east; p0 += tripoint_east ) {
            m.add_item( p0, item( "test_2x4" ) );
        }
        // note: time limit here was chosen arbitrarily. It could be too low or too high.
        check_spreading( 30_minutes );
    }
    SECTION( "fire spreads on flammable terrain" ) {
        for( tripoint p0 = p; p0 != far_p + tripoint_east; p0 += tripoint_east ) {
            REQUIRE( ter_str_id( "t_tree_walnut" )->has_flag( TFLAG_FLAMMABLE_ASH ) );
            m.ter_set( p0, ter_str_id( "t_tree_walnut" ) );
        }
        // note: time limit here was chosen arbitrarily. It could be too low or too high.
        check_spreading( 30_minutes );
    }

    fields_test_cleanup();
}

// tests fd_fire_vent <-> fd_flame_burst cycle
TEST_CASE( "fd_fire and fd_fire_vent test", "[field]" )
{
    fields_test_setup();

    const tripoint p{ 33, 33, 0 };
    map &m = get_map();

    m.add_field( p, fd_fire_vent, 3 );
    CHECK( m.get_field( p, fd_fire_vent ) );
    CHECK_FALSE( m.get_field( p, fd_flame_burst ) );

    const int time_limit_turns = to_turns<int>( 2_minutes );

    while( !m.get_field( p, fd_flame_burst ) && fields_test_turns() < time_limit_turns ) {
        calendar::turn += 1_turns;
        m.process_fields();
    }

    {
        INFO( string_format( "Should've converted to flame burst in under %d turns", time_limit_turns ) );
        CHECK( fields_test_turns() < time_limit_turns );
    }

    {
        field_entry *fire_vent = m.get_field( p, fd_fire_vent );
        field_entry *flame_burst = m.get_field( p, fd_flame_burst );
        INFO( "Flame burst should replace fire vent" );
        CAPTURE( fire_vent ? fire_vent->get_field_intensity() : 0 );
        CHECK( ( !fire_vent || !fire_vent->is_field_alive() ) );
        REQUIRE( flame_burst );
        CHECK( flame_burst->get_field_intensity() >= 2 );
    }

    // With the current implementation `fd_flame_burst` can be processed once in the same turn it is created.
    // This could be a bug.

    const int flame_burst_intensity = m.get_field( p, fd_flame_burst )->get_field_intensity();

    for( int i = flame_burst_intensity - 1; i >= 0; i-- ) {
        calendar::turn += 1_turns;
        m.process_fields();

        field_entry *flame_burst = m.get_field( p, fd_flame_burst );
        INFO( "Flame burst intensity should drop" );
        CAPTURE( i );
        CHECK( ( flame_burst &&
                 flame_burst->is_field_alive() ? flame_burst->get_field_intensity() : 0 ) == i );
    }

    {
        INFO( "Flame burst should convert back to fire vent" );
        field_entry *fire_vent = m.get_field( p, fd_fire_vent );
        field_entry *flame_burst = m.get_field( p, fd_flame_burst );
        REQUIRE( fire_vent );
        CHECK( fire_vent->get_field_intensity() >= 2 );
        CHECK( ( !flame_burst || !flame_burst->is_field_alive() ) );
    }

    fields_test_cleanup();
}

// tests wandering_field property fd_smoke_vent
TEST_CASE( "wandering_field test", "[field]" )
{
    fields_test_setup();

    const tripoint p{ 33, 33, 0 };
    map &m = get_map();

    REQUIRE( fd_smoke_vent->wandering_field == fd_smoke );

    // 1 second old, so gets processed right away
    m.add_field( p, fd_smoke_vent, 3, 1_seconds );

    REQUIRE( m.get_field( p, fd_smoke_vent ) );
    REQUIRE( count_fields( fd_smoke ) == 0 );

    calendar::turn += 1_turns;
    m.process_fields();

    {
        INFO( "fd_smoke should've spawned around the vent in a 5x5 area" )
        CHECK( count_fields( fd_smoke ) == 25 );
    }

    for( const tripoint &pnt : points_in_radius( p, 2 ) ) {
        CHECK( m.get_field( pnt, fd_smoke ) );
    }

    fields_test_cleanup();
}

TEST_CASE( "radioactive field", "[field]" )
{
    fields_test_setup();
    clear_radiation();

    const tripoint p{ 33, 33, 0 };
    map &m = get_map();

    REQUIRE( fd_nuke_gas->get_intensity_level().extra_radiation_max > 0 );
    REQUIRE( m.get_radiation( p ) == 0 );

    m.add_field( p, fd_nuke_gas, 1 );

    const int time_limit_turns = to_turns<int>( 5_minutes );

    while( m.get_radiation( p ) == 0 && fields_test_turns() < time_limit_turns ) {
        calendar::turn += 1_turns;
        m.process_fields();
    }
    {
        INFO( string_format( "Terrain should be irradiated in no more than %d turns", time_limit_turns ) );
        CHECK( fields_test_turns() <= time_limit_turns );
    }

    // cleanup
    clear_radiation();
    fields_test_cleanup();
}

TEST_CASE( "fungal haze test", "[field]" )
{
    fields_test_setup();
    const tripoint p{ 33, 33, 0 };
    map &m = get_map();

    REQUIRE_FALSE( m.has_flag( "FUNGUS", p ) );
    m.add_field( p, fd_fungal_haze, 3 );

    // note: time limit was chosen arbitrary. It could be too low.
    const int time_limit_turns = to_turns<int>( 1_hours );

    while( !m.has_flag( "FUNGUS", p ) && fields_test_turns() < time_limit_turns ) {
        calendar::turn += 1_turns;
        m.process_fields();

        // maintain fungal field up to the time limit
        if( !m.get_field( p, fd_fungal_haze ) ) {
            m.add_field( p, fd_fungal_haze, 3 );
        }
    }
    {
        INFO( string_format( "Terrain should be fungalized in below %d turns", time_limit_turns ) );
        CHECK( fields_test_turns() < time_limit_turns );
    }

    fields_test_cleanup();
}

TEST_CASE( "player_in_field test", "[field][player]" )
{
    fields_test_setup();
    clear_avatar();
    const tripoint p{ 33, 33, 0 };

    Character &dummy = get_avatar();
    const tripoint prev_char_pos = dummy.pos();
    dummy.setpos( p );

    map &m = get_map();

    m.add_field( p, fd_sap, 3 );

    // Also add bunch of unrelated fields
    m.add_field( p, fd_blood, 3 );
    m.add_field( p, fd_blood_insect, 3 );
    m.add_field( p, fd_blood_veggy, 3 );
    m.add_field( p, fd_blood_invertebrate, 3 );

    const int time_limit_turns = to_turns<int>( 5_minutes );

    bool is_field_alive = true;
    while( is_field_alive && fields_test_turns() < time_limit_turns ) {
        calendar::turn += 1_turns;
        m.creature_in_field( dummy );
        const field_entry *sap_field = m.get_field( p, fd_sap );
        is_field_alive = sap_field && sap_field->is_field_alive();
    }
    {
        INFO( string_format( "Sap should disappear in under %d turns", time_limit_turns ) );
        CHECK( fields_test_turns() < time_limit_turns );
    }

    clear_avatar();
    dummy.setpos( prev_char_pos );
    fields_test_cleanup();
}

TEST_CASE( "field API test", "[field]" )
{
    fields_test_setup();
    const tripoint p{ 33, 33, 0 };
    map &m = get_map();
    field &f = m.field_at( p );

    REQUIRE_FALSE( f.find_field( fd_fire ) );

    CHECK( m.add_field( p, fd_fire, 1 ) );

    CHECK( f.find_field( fd_fire ) );
    CHECK( f.find_field( fd_fire, /*alive_only*/ false ) );
    CHECK( m.get_field( p, fd_fire ) );
    CHECK( m.get_field_intensity( p, fd_fire ) == 1 );

    m.remove_field( p, fd_fire );

    CHECK_FALSE( f.find_field( fd_fire ) );
    {
        INFO( "Field is still there, actually removed only by process_fields" );
        CHECK( f.find_field( fd_fire, /*alive_only*/ false ) );
    }
    CHECK_FALSE( m.get_field( p, fd_fire ) );
    CHECK( m.get_field_intensity( p, fd_fire ) == 0 );

    CHECK( m.set_field_intensity( p, fd_fire, 1 ) == 1 );
    CHECK( f.find_field( fd_fire ) );

    fields_test_cleanup();
}
