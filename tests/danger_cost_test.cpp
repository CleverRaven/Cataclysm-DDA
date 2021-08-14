#include <map>
#include <game.h>
#include <field_type.h>

#include "cata_catch.h"
#include "map.h"
#include "map_helpers.h"


TEST_CASE( "Tile danger cost", "[field][monster]" )
{
    map &m = get_map();

    WHEN( "lava on tile" ) {
        clear_map();
        monster &test_monster = spawn_test_monster( "mon_mi_go", { 32, 33, 0 } );

        const tripoint lava_loc{ 33, 33, 0 };
        m.ter_set( lava_loc, ter_id( "t_lava" ) );

        THEN( "path costs 1000" ) {
            const int danger_cost = test_monster.danger_cost( lava_loc );
            CHECK( danger_cost == 1000 );
        }
    }

    WHEN( "dangerous trap on tile" ) {
        clear_map();
        monster &test_monster = spawn_test_monster( "mon_mi_go", { 32, 33, 0 } );

        const tripoint trap_loc{ 33, 33, 0 };
        m.trap_set( trap_loc, trap_id( "tr_beartrap" ) );

        THEN( "path costs 500" ) {
            const int danger_cost = test_monster.danger_cost( trap_loc );
            CHECK( danger_cost == 500 );
        }
    }

    WHEN( "benign trap on tile" ) {
        clear_map();
        monster &test_monster = spawn_test_monster( "mon_mi_go", { 32, 33, 0 } );

        const tripoint trap_loc{ 33, 33, 0 };
        m.trap_set( trap_loc, trap_id( "tr_fur_rollmat" ) );

        THEN( "path costs 0" ) {
            const int danger_cost = test_monster.danger_cost( trap_loc );
            CHECK( danger_cost == 0 );
        }
    }

    WHEN( "cliff on tile" ) {
        clear_map();
        monster &test_monster = spawn_test_monster( "mon_mi_go", { 32, 33, 0 } );

        const tripoint cliff_loc{ 33, 33, 0 };
        m.ter_set( cliff_loc, ter_id( "t_open_air" ) );

        THEN( "path costs 500" ) {
            const int danger_cost = test_monster.danger_cost( cliff_loc );
            CHECK( danger_cost == 1000 );
        }
    }

    WHEN( "cliff on tile but can fly" ) {
        clear_map();
        monster &test_monster = spawn_test_monster( "mon_crow_mutant_small", { 32, 33, 0 } );

        const tripoint cliff_loc{ 33, 33, 0 };
        m.ter_set( cliff_loc, ter_id( "t_open_air" ) );

        THEN( "path costs 0" ) {
            const int danger_cost = test_monster.danger_cost( cliff_loc );
            CHECK( danger_cost == 0 );
        }
    }
}