#include "cached_options.h"
#include "calendar.h"
#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "map.h"
#include "map_helpers.h"
#include "mapdata.h"
#include "monster.h"
#include "options_helpers.h"

struct tripoint;

static monster &spawn_and_clear( const tripoint &pos, bool set_floor )
{
    if( set_floor ) {
        get_map().set( pos, t_floor, f_null );
    }
    return spawn_test_monster( "mon_zombie", pos );
}

static const time_point midday = calendar::turn_zero + 12_hours;

TEST_CASE( "monsters_should_not_see_through_floors", "[vision]" )
{
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
    CHECK( !upper.sees( lower ) );
    CHECK( !lower.sees( upper ) );
    // Two intervening floors between monsters.
    CHECK( !upper.sees( deep ) );
    CHECK( !deep.sees( upper ) );
    // One intervening floor and an open space between monsters.
    CHECK( !sky.sees( lower ) );
    CHECK( !lower.sees( sky ) );
    // One intervening floor between monsters, and offset one tile horizontally.
    CHECK( !adjacent.sees( lower ) );
    CHECK( !lower.sees( adjacent ) );
    // One intervening floor between monsters, and offset two tiles horizontally.
    CHECK( !distant.sees( lower ) );
    CHECK( !lower.sees( distant ) );
    // Two intervening floors between monsters, and offset one tile horizontally.
    CHECK( !adjacent.sees( deep ) );
    CHECK( !deep.sees( adjacent ) );
    // Two intervening floor between monsters, and offset two tiles horizontally.
    CHECK( !distant.sees( deep ) );
    CHECK( !deep.sees( distant ) );

    // Then cases where they should be able to see each other.
    // No floor between monsters
    CHECK( upper.sees( sky ) );
    CHECK( sky.sees( upper ) );
    // Adjacent monsters.
    CHECK( upper.sees( adjacent ) );
    CHECK( adjacent.sees( upper ) );
    // distant monsters.
    CHECK( upper.sees( distant ) );
    CHECK( distant.sees( upper ) );
    // One intervening vertical tile and one intervening horizontal tile.
    CHECK( sky.sees( adjacent ) );
    CHECK( adjacent.sees( sky ) );
    // One intervening vertical tile and two intervening horizontal tiles.
    CHECK( sky.sees( distant ) );
    CHECK( distant.sees( sky ) );
}
