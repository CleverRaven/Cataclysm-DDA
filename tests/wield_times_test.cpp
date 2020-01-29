#include <cstdio>
#include <string>
#include <list>
#include <memory>

#include "avatar.h"
#include "catch/catch.hpp"
#include "game.h"
#include "map.h"
#include "map_helpers.h"
#include "player.h"
#include "player_helpers.h"
#include "item.h"
#include "point.h"

static void wield_check_internal( player &dummy, item &the_item, const char *section_text,
                                  const std::string &var_name, const int expected_cost )
{
    dummy.weapon = item();
    dummy.set_moves( 1000 );
    const int old_moves = dummy.moves;
    dummy.wield( the_item );
    int move_cost = old_moves - dummy.moves;
    if( expected_cost < 0 ) {
        printf( "    wield_check( %s, dummy, %s, %d );\n", section_text, var_name.c_str(), move_cost );
    } else {
        INFO( "Strength:" << dummy.get_str() );
        int max_cost = expected_cost * 1.1f;
        int min_cost = expected_cost * 0.9f;
        CHECK( move_cost <= max_cost );
        CHECK( move_cost >= min_cost );
    }
}

// As macro, so that we can generate the test cases for easy copypasting
#define wield_check(section_text, dummy, the_item, expected_cost) \
    SECTION( section_text) { \
        wield_check_internal(dummy, the_item, #section_text, #the_item, generating_cases ? -1 : (expected_cost)); \
    }

static void do_test( const bool generating_cases )
{
    player &dummy = g->u;
    const tripoint spot = dummy.pos();

    dummy.worn.clear();
    dummy.reset_encumbrance();
    wield_check( "Wielding halberd from inventory while unencumbered", dummy,
                 dummy.i_add( item( "halberd" ) ), 287 );
    wield_check( "Wielding 1 aspirin from inventory while unencumbered", dummy,
                 dummy.i_add( item( "aspirin" ).split( 1 ) ), 100 );
    wield_check( "Wielding combat knife from inventory while unencumbered", dummy,
                 dummy.i_add( item( "knife_combat" ) ), 125 );
    wield_check( "Wielding metal tank from outside inventory while unencumbered", dummy,
                 g->m.add_item( spot, item( "metal_tank" ) ), 300 );

    dummy.worn = {{ item( "gloves_work" ) }};
    dummy.reset_encumbrance();
    wield_check( "Wielding halberd from inventory while wearing work gloves", dummy,
                 dummy.i_add( item( "halberd" ) ), 307 );
    wield_check( "Wielding 1 aspirin from inventory while wearing work gloves", dummy,
                 dummy.i_add( item( "aspirin" ).split( 1 ) ), 120 );
    wield_check( "Wielding combat knife from inventory while wearing work gloves", dummy,
                 dummy.i_add( item( "knife_combat" ) ), 150 );
    wield_check( "Wielding metal tank from outside inventory while wearing work gloves", dummy,
                 g->m.add_item( spot, item( "metal_tank" ) ), 340 );

    dummy.worn = {{ item( "boxing_gloves" ) }};
    dummy.reset_encumbrance();
    wield_check( "Wielding halberd from inventory while wearing boxing gloves", dummy,
                 dummy.i_add( item( "halberd" ) ), 365 );
    wield_check( "Wielding 1 aspirin from inventory while wearing boxing gloves", dummy,
                 dummy.i_add( item( "aspirin" ).split( 1 ) ), 170 );
    wield_check( "Wielding combat knife from inventory while wearing boxing gloves", dummy,
                 dummy.i_add( item( "knife_combat" ) ), 200 );
    wield_check( "Wielding metal tank from outside inventory while wearing boxing gloves", dummy,
                 g->m.add_item( spot, item( "metal_tank" ) ), 400 );
}

TEST_CASE( "Wield time test", "[wield]" )
{
    clear_avatar();
    clear_map();
    do_test( false );
}

TEST_CASE( "Wield time make cases", "[.]" )
{
    clear_avatar();
    clear_map();
    do_test( true );
}
