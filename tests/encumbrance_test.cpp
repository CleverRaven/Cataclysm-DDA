#include <functional>
#include <iosfwd>
#include <list>
#include <string>
#include <vector>

#include "bodypart.h"
#include "cata_catch.h"
#include "character.h"
#include "item.h"
#include "npc.h"
#include "type_id.h"

static const flag_id json_flag_UNDERSIZE( "UNDERSIZE" );

static void test_encumbrance_on(
    Character &p,
    const std::vector<item> &clothing,
    const std::string &body_part,
    int expected_encumbrance,
    const std::function<void( Character & )> &tweak_player = {}
)
{
    CAPTURE( body_part );
    p.set_body();
    p.clear_mutations();
    p.worn.clear();
    if( tweak_player ) {
        tweak_player( p );
    }
    for( const item &i : clothing ) {
        p.worn.wear_item( p, i, false, false, false );
    }
    p.calc_encumbrance();
    encumbrance_data enc = p.get_part_encumbrance_data( bodypart_id( body_part ) );
    CHECK( enc.encumbrance == expected_encumbrance );
}

static void test_encumbrance_items(
    const std::vector<item> &clothing,
    const std::string &body_part,
    const int expected_encumbrance,
    const std::function<void( Character & )> &tweak_player = {}
)
{
    // Test NPC first because NPC code can accidentally end up using properties
    // of g->u, and such bugs are hidden if we test the other way around.
    SECTION( "testing on npc" ) {
        npc example_npc;
        test_encumbrance_on( example_npc, clothing, body_part, expected_encumbrance, tweak_player );
    }
    SECTION( "testing on player" ) {
        test_encumbrance_on( get_player_character(), clothing, body_part, expected_encumbrance,
                             tweak_player );
    }
}

static void test_encumbrance(
    const std::vector<std::string> &clothing_types,
    const std::string &body_part,
    const int expected_encumbrance
)
{
    CAPTURE( clothing_types );
    std::vector<item> clothing;
    clothing.reserve( clothing_types.size() );
    for( const std::string &type : clothing_types ) {
        clothing.emplace_back( type );
    }
    test_encumbrance_items( clothing, body_part, expected_encumbrance );
}

struct add_trait {
    explicit add_trait( const std::string &t ) : trait( t ) {}
    explicit add_trait( const trait_id &t ) : trait( t ) {}

    void operator()( Character &p ) const {
        p.toggle_trait( trait );
    }

    trait_id trait;
};

static constexpr int postman_shirt_e = 0;
static constexpr int longshirt_e = 3;
static constexpr int jacket_jean_e = 9;
static constexpr int ballistic = 6;
static constexpr int load_bearing = 2;
static constexpr int plate = 2;

TEST_CASE( "regular_clothing_encumbrance", "[encumbrance]" )
{
    test_encumbrance( { "test_postman_shirt" }, "torso", postman_shirt_e );
    test_encumbrance( { "test_longshirt" }, "torso", longshirt_e );
    test_encumbrance( { "test_jacket_jean" }, "torso", jacket_jean_e );
    test_encumbrance( { "test_ballistic_vest" }, "torso", ballistic );
    test_encumbrance( { "test_load_bearing_vest" }, "torso", load_bearing );
}

TEST_CASE( "plate_encumbrance", "[encumbrance]" )
{
    item with_plates( "test_ballistic_vest" );
    with_plates.force_insert_item( item( "test_plate" ), item_pocket::pocket_type::CONTAINER );
    test_encumbrance_items( { with_plates }, "torso", ballistic + plate );
}

TEST_CASE( "separate_layer_encumbrance", "[encumbrance]" )
{
    test_encumbrance( { "test_longshirt", "test_jacket_jean" }, "torso", longshirt_e + jacket_jean_e );
    test_encumbrance( { "test_longshirt", "test_ballistic_vest" }, "torso", longshirt_e + ballistic );
    test_encumbrance( { "test_longshirt", "test_load_bearing_vest" }, "torso",
                      longshirt_e + load_bearing );
}

// make sure ordering still works with pockets
TEST_CASE( "additional_pockets_encumbrance", "[encumbrance]" )
{
    item addition_vest( "test_load_bearing_vest" );
    item undershirt( "test_longshirt" );
    addition_vest.get_contents().add_pocket( item( "holster" ) );

    test_encumbrance_items( { undershirt, addition_vest }, "torso", longshirt_e + load_bearing );
}

TEST_CASE( "out_of_order_encumbrance", "[encumbrance]" )
{
    test_encumbrance( { "test_jacket_jean", "test_longshirt" }, "torso",
                      longshirt_e * 2 + jacket_jean_e );
}

TEST_CASE( "same_layer_encumbrance", "[encumbrance]" )
{
    // When stacking within a layer, encumbrance for additional items is
    // counted twice
    test_encumbrance( { "test_longshirt", "test_longshirt" }, "torso", longshirt_e * 2 + longshirt_e );
    // ... with a minimum of 2
    test_encumbrance( { "test_postman_shirt", "test_postman_shirt" }, "torso",
                      postman_shirt_e * 2 + 2 );
    // ... and a maximum of 10
    test_encumbrance( { "test_jacket_jean", "test_jacket_jean" }, "torso", jacket_jean_e * 2 + 10 );
}

TEST_CASE( "tiny_clothing", "[encumbrance]" )
{
    item i( "test_longshirt" );
    i.set_flag( json_flag_UNDERSIZE );
    test_encumbrance_items( { i }, "torso", longshirt_e * 3 );
}

TEST_CASE( "tiny_character", "[encumbrance]" )
{
    item i( "test_longshirt" );
    SECTION( "regular shirt" ) {
        test_encumbrance_items( { i }, "torso", longshirt_e * 2, add_trait( "SMALL2" ) );
    }
    SECTION( "undersize shrt" ) {
        i.set_flag( json_flag_UNDERSIZE );
        test_encumbrance_items( { i }, "torso", longshirt_e, add_trait( "SMALL2" ) );
    }
}
