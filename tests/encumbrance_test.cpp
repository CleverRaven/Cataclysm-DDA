#include "catch/catch.hpp"

#include "game.h"
#include "npc.h"
#include "player.h"

void test_encumbrance_on(
    player &p,
    const std::vector<itype_id> &clothing,
    const std::string &body_part,
    int expected_encumbrance
)
{
    CAPTURE( clothing, body_part );
    p.worn.clear();
    for( const itype_id &type : clothing ) {
        p.worn.push_back( item( type ) );
    }
    p.reset_encumbrance();
    encumbrance_data enc = p.get_encumbrance()[ get_body_part_token( body_part ) ];
    CHECK( enc.encumbrance == expected_encumbrance );
}

void test_encumbrance(
    const std::vector<itype_id> &clothing,
    const std::string &body_part,
    int expected_encumbrance
)
{
    {
        INFO( "testing on player" );
        test_encumbrance_on( g->u, clothing, body_part, expected_encumbrance );
    }
    {
        INFO( "testing on npc" );
        npc example_npc;
        test_encumbrance_on( example_npc, clothing, body_part, expected_encumbrance );
    }
}

TEST_CASE( "regular_clothing_encumbrance", "[encumbrance]" )
{
    test_encumbrance( { "longshirt" }, "TORSO", 3 );
}
