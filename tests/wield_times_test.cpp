#include "catch/catch.hpp"

#include "player.h"
#include "game.h"
#include "map.h"

#include <string>

// As macro, so that we can generate the test cases for easy copypasting
#define wield_check(section_text, dummy, the_item, expected_cost) wield_check_internal(dummy, the_item, #section_text, #the_item, generating_cases ? -1 : expected_cost)

void wield_check_internal( player &dummy, item &the_item, const char *section_text, const std::string &var_name, int expected_cost )
{
    SECTION( section_text ) {
        dummy.weapon = dummy.ret_null;
        dummy.set_moves( 1000 );
        int old_moves = dummy.moves;
        dummy.wield( the_item );
        int move_cost = old_moves - dummy.moves;
        if( expected_cost < 0 ) {
            printf( "    wield_check( %s, dummy, %s, %d );\n", section_text, var_name.c_str(), move_cost );
        } else {
            int max_cost = move_cost * 1.1f;
            int min_cost = move_cost * 0.9f;
            CHECK( expected_cost <= max_cost );
            CHECK( expected_cost >= min_cost );
        }
    }
}

void prepare_test()
{
    player &dummy = g->u;

    // Remove first worn item until there are none left.
    std::list<item> temp;
    while( dummy.takeoff( dummy.i_at( -2 ), &temp ) );
    // Prevent spilling, but don't cause encumbrance
    if( !dummy.has_trait( "DEBUG_STORAGE" ) ) {
        dummy.set_mutation( "DEBUG_STORAGE" );
    }

    const tripoint spot( 60, 60, 0 );
    dummy.setpos( spot );
    g->m.ter_set( spot, ter_id( "t_dirt" ) );
    g->m.furn_set( spot, furn_id( "f_null" ) );
    g->m.i_clear( spot );
}

void do_test( bool generating_cases )
{
    player &dummy = g->u;
    const tripoint spot = dummy.pos();

    dummy.worn.clear();
    dummy.reset_encumbrance();
    wield_check( "Wielding halberd from inventory while unencumbered", dummy, dummy.i_add( item( "halberd" ) ), 287 );
    wield_check( "Wielding 1 aspirin from inventory while unencumbered", dummy, dummy.i_add( item( "aspirin" ).split( 1 ) ), 100 );
    wield_check( "Wielding combat knife from inventory while unencumbered", dummy, dummy.i_add( item( "knife_combat" ) ), 125 );
    wield_check( "Wielding metal tank from outside inventory while unencumbered", dummy, g->m.add_item( spot, item( "metal_tank" ) ), 300 );

    dummy.worn = {{ item( "gloves_work" ) }};
    dummy.reset_encumbrance();
    wield_check( "Wielding halberd from inventory while wearing work gloves", dummy, dummy.i_add( item( "halberd" ) ), 307 );
    wield_check( "Wielding 1 aspirin from inventory while wearing work gloves", dummy, dummy.i_add( item( "aspirin" ).split( 1 ) ), 120 );
    wield_check( "Wielding combat knife from inventory while wearing work gloves", dummy, dummy.i_add( item( "knife_combat" ) ), 145 );
    wield_check( "Wielding metal tank from outside inventory while wearing work gloves", dummy, g->m.add_item( spot, item( "metal_tank" ) ), 340 );

    dummy.worn = {{ item( "boxing_gloves" ) }};
    dummy.reset_encumbrance();
    wield_check( "Wielding halberd from inventory while wearing boxing gloves", dummy, dummy.i_add( item( "halberd" ) ), 357 );
    wield_check( "Wielding 1 aspirin from inventory while wearing boxing gloves", dummy, dummy.i_add( item( "aspirin" ).split( 1 ) ), 170 );
    wield_check( "Wielding combat knife from inventory while wearing boxing gloves", dummy, dummy.i_add( item( "knife_combat" ) ), 195 );
    wield_check( "Wielding metal tank from outside inventory while wearing boxing gloves", dummy, g->m.add_item( spot, item( "metal_tank" ) ), 400 );
}

TEST_CASE("Wield time test", "[wield]") {
    prepare_test();
    do_test( false );
}

TEST_CASE("Wield time make cases", "[.]") {
    prepare_test();
    do_test( true );
}
