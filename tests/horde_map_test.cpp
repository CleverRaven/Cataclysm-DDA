#include "horde_map.h"

#include "cata_catch.h"
#include "coordinates.h"
#include "monster.h"
#include "rng.h"

static const mtype_id mon_pseudo_dormant_zombie( "mon_pseudo_dormant_zombie" );
static const mtype_id mon_zombie( "mon_zombie" );

static tripoint_om_ms random_location()
{
    return tripoint_om_ms( rng( 0, 180 ), rng( 0, 180 ), 0 );
}

static tripoint_abs_ms random_abs_location( horde_map &test_horde )
{
    return project_combine( test_horde.get_location(), random_location() );
}

// Simple dumb placement code because these tests mostly don't care where the entities end up.
static tripoint_om_ms pick_available_location( horde_map &test_horde )
{
    tripoint_om_ms candidate;
    do {
        candidate = random_location();
    } while( test_horde.entity_at( candidate ) != nullptr );
    return candidate;
}

static int count_entities( horde_map &test_horde, int filter )
{
    int entity_count = 0;
    for( [[maybe_unused]]std::pair<const tripoint_abs_ms, horde_entity> &entity : test_horde.get_view(
             filter ) ) {
        entity_count++;
    }
    return entity_count;
}

static void place_entity( horde_map &test_horde, mtype_id id )
{
    tripoint_om_ms monster_relative = pick_available_location( test_horde );
    tripoint_abs_ms monster_location( project_combine( test_horde.get_location(), monster_relative ) );
    test_horde.spawn_entity( monster_location, id );
    REQUIRE( test_horde.entity_at( monster_relative ) != nullptr );
}

static void place_monster_as_entity( horde_map &test_horde, monster &mon )
{
    tripoint_om_ms monster_relative = pick_available_location( test_horde );
    tripoint_abs_ms monster_location( project_combine( test_horde.get_location(), monster_relative ) );
    test_horde.spawn_entity( monster_location, mon );
    REQUIRE( test_horde.entity_at( monster_relative ) != nullptr );
}

/*
 * The main things to test here are the automatic filtering based on monster attributes and
 * simple round-tripping.
 * Specifically dormant monsters, idle monsters (with no current destination) and
 * active monsters (that DO have a current destination) are handled separately.
*/
TEST_CASE( "horde_map_insertion_and_retrieval", "[hordes]" )
{
    horde_map test_horde;
    point_abs_om om_origin( 42, 42 );
    test_horde.set_location( om_origin );

    monster wandering_monster( mon_zombie );
    wandering_monster.wander_to( random_abs_location( test_horde ), 100 );
    place_monster_as_entity( test_horde, wandering_monster );

    monster targeted_monster( mon_zombie );
    targeted_monster.set_dest( random_abs_location( test_horde ) );
    place_monster_as_entity( test_horde, targeted_monster );

    monster idle_monster( mon_zombie );
    place_monster_as_entity( test_horde, idle_monster );

    place_entity( test_horde, mon_zombie );
    place_entity( test_horde, mon_zombie );
    place_entity( test_horde, mon_pseudo_dormant_zombie );

    int entity_count = 0;
    for( [[maybe_unused]]std::pair<const tripoint_abs_ms, horde_entity> &entity : test_horde ) {
        entity_count++;
    }
    CHECK( entity_count == 6 );
    CHECK( count_entities( test_horde, horde_map_flavors::active ) == 2 );
    CHECK( count_entities( test_horde, horde_map_flavors::idle ) == 3 );
    CHECK( count_entities( test_horde, horde_map_flavors::dormant ) == 1 );
    CHECK( count_entities( test_horde, horde_map_flavors::active | horde_map_flavors::idle ) == 5 );
    CHECK( count_entities( test_horde, horde_map_flavors::active | horde_map_flavors::dormant ) == 3 );
    CHECK( count_entities( test_horde, horde_map_flavors::idle | horde_map_flavors::dormant ) == 4 );

    // Remove an idle and a dormant entity, give them a goal, and re-insert them.
    // TODO: This stays dormant! Need to add support for them changing.
    horde_map::node_type dormant_node = test_horde.extract( test_horde.get_view(
                                            horde_map_flavors::dormant ).begin() );
    dormant_node.mapped().tracking_intensity = 100;
    dormant_node.mapped().destination = random_abs_location( test_horde );;
    test_horde.insert( std::move( dormant_node ) );

    horde_map::node_type idle_node = test_horde.extract( test_horde.get_view(
                                         horde_map_flavors::idle ).begin() );
    idle_node.mapped().tracking_intensity = 100;
    idle_node.mapped().destination = random_abs_location( test_horde );;
    test_horde.insert( std::move( idle_node ) );

    entity_count = 0;
    for( [[maybe_unused]]std::pair<const tripoint_abs_ms, horde_entity> &entity : test_horde ) {
        entity_count++;
    }
    CHECK( entity_count == 6 );
    CHECK( count_entities( test_horde, horde_map_flavors::active ) == 3 );
    CHECK( count_entities( test_horde, horde_map_flavors::idle ) == 2 );
    CHECK( count_entities( test_horde, horde_map_flavors::dormant ) == 1 );
    CHECK( count_entities( test_horde, horde_map_flavors::active | horde_map_flavors::idle ) == 5 );
    CHECK( count_entities( test_horde, horde_map_flavors::active | horde_map_flavors::dormant ) == 4 );
    CHECK( count_entities( test_horde, horde_map_flavors::idle | horde_map_flavors::dormant ) == 3 );
}

TEST_CASE( "horde_map_corner_cases", "[hordes]" )
{
    // Make sure iterator handling is ok with empty container.
    horde_map test_horde;
    for( [[maybe_unused]]std::pair<const tripoint_abs_ms, horde_entity> &entity : test_horde ) {
        FAIL( "Unreachable loop entered, should not happen with empty horde_map." );
    }
    // Populated container but accessed in a way that filters out everything.
    place_entity( test_horde, mon_zombie );
    for( [[maybe_unused]]std::pair<const tripoint_abs_ms, horde_entity> &entity : test_horde.get_view(
             horde_map_flavors::active ) ) {
        FAIL( "Unreachable loop entered, should not happen with empty horde_map." );
    }

}
