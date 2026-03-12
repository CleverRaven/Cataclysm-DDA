#include <string>

#include "calendar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "map.h"
#include "map_helpers.h"
#include "monster.h"
#include "point.h"
#include "type_id.h"

static const ter_str_id ter_t_floor( "t_floor" );

static monster &spawn_and_clear( const tripoint_bub_ms &pos, bool set_floor )
{
    if( set_floor ) {
        get_map().set( pos, ter_t_floor, furn_str_id::NULL_ID() );
    }
    return spawn_test_monster( "mon_zombie", pos );
}

static const time_point midday = calendar::turn_zero + 12_hours;

TEST_CASE( "monsters_should_not_see_through_floors", "[vision]" )
{
    const map &here = get_map();

    calendar::turn = midday;
    clear_map( -2, 1 );
    monster &upper = spawn_and_clear( { 5, 5, 0 }, true );
    monster &adjacent = spawn_and_clear( { 5, 6, 0 }, true );
    monster &distant = spawn_and_clear( { 5, 3, 0 }, true );
    monster &lower = spawn_and_clear( { 5, 5, -1 }, true );
    monster &deep = spawn_and_clear( { 5, 5, -2 }, true );
    monster &sky = spawn_and_clear( { 5, 5, 1 }, false );

    // First check monsters whose vision should be blocked by floors.
    // One intervening floor between monsters.
    CHECK( !upper.sees( here, lower ) );
    CHECK( !lower.sees( here, upper ) );
    // Two intervening floors between monsters.
    CHECK( !upper.sees( here, deep ) );
    CHECK( !deep.sees( here,  upper ) );
    // One intervening floor and an open space between monsters.
    CHECK( !sky.sees( here, lower ) );
    CHECK( !lower.sees( here, sky ) );
    // One intervening floor between monsters, and offset one tile horizontally.
    CHECK( !adjacent.sees( here, lower ) );
    CHECK( !lower.sees( here,  adjacent ) );
    // One intervening floor between monsters, and offset two tiles horizontally.
    CHECK( !distant.sees( here, lower ) );
    CHECK( !lower.sees( here, distant ) );
    // Two intervening floors between monsters, and offset one tile horizontally.
    CHECK( !adjacent.sees( here,  deep ) );
    CHECK( !deep.sees( here, adjacent ) );
    // Two intervening floor between monsters, and offset two tiles horizontally.
    CHECK( !distant.sees( here, deep ) );
    CHECK( !deep.sees( here,  distant ) );

    // Then cases where they should be able to see each other.
    // No floor between monsters
    CHECK( upper.sees( here, sky ) );
    CHECK( sky.sees( here, upper ) );
    // Adjacent monsters.
    CHECK( upper.sees( here, adjacent ) );
    CHECK( adjacent.sees( here, upper ) );
    // distant monsters.
    CHECK( upper.sees( here, distant ) );
    CHECK( distant.sees( here, upper ) );
    // One intervening vertical tile and one intervening horizontal tile.
    CHECK( sky.sees( here, adjacent ) );
    CHECK( adjacent.sees( here, sky ) );
    // One intervening vertical tile and two intervening horizontal tiles.
    CHECK( sky.sees( here, distant ) );
    CHECK( distant.sees( here, sky ) );
}
