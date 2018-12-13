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

static constexpr int postman_shirt_e = 0;
static constexpr int longshirt_e = 3;
static constexpr int jacket_jean_e = 11;

TEST_CASE( "regular_clothing_encumbrance", "[encumbrance]" )
{
    test_encumbrance( { "postman_shirt" }, "TORSO", postman_shirt_e );
    test_encumbrance( { "longshirt" }, "TORSO", longshirt_e );
    test_encumbrance( { "jacket_jean" }, "TORSO", jacket_jean_e );
}

TEST_CASE( "separate_layer_encumbrance", "[encumbrance]" )
{
    test_encumbrance( { "longshirt", "jacket_jean" }, "TORSO", longshirt_e + jacket_jean_e );
}

TEST_CASE( "out_of_order_encumbrance", "[encumbrance]" )
{
    test_encumbrance( { "jacket_jean", "longshirt" }, "TORSO", longshirt_e * 2 + jacket_jean_e );
}

TEST_CASE( "same_layer_encumbrance", "[encumbrance]" )
{
    // When stacking within a layer, encumbrance for additional items is
    // counted twice
    test_encumbrance( { "longshirt", "longshirt" }, "TORSO", longshirt_e * 2 + longshirt_e );
    // ... with a minimum of 2
    test_encumbrance( { "postman_shirt", "postman_shirt" }, "TORSO", postman_shirt_e * 2 + 2 );
    // ... and a maximum of 10
    test_encumbrance( { "jacket_jean", "jacket_jean" }, "TORSO", jacket_jean_e * 2 + 10 );
}
