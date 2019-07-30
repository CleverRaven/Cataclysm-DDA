#include "catch/catch.hpp"
#include "npc.h"
#include "mtype.h"

npc create_model()
{
    standard_npc dude( "TestCharacter", {}, 3, 8, 8, 8, 8 );
    dude.weapon = item( "1911" );
    return dude;
}

TEST_CASE( "NPC Shoots enemy without player in the way.", "[.ranged]" )
{
    monster zed( mtype_id( "mon_zombie" ) );
}
