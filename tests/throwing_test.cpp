#include "catch/catch.hpp"

#include "ballistics.h"
#include "dispersion.h"
#include "game.h"
#include "monattack.h"
#include "monster.h"
#include "npc.h"
#include "units.h"

#include "test_statistics.h"
#include "map_helpers.h"

#include <vector>

TEST_CASE( "throwing distance test", "[throwing], [balance]" )
{
    standard_npc thrower( "Thrower", {}, 4, 10, 10, 10, 10 );
    item grenade( "grenade" );
    CHECK( thrower.throw_range( grenade ) >= 30 );
    CHECK( thrower.throw_range( grenade ) <= 35 );
}
