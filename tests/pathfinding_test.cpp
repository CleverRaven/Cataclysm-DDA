#include <map>
#include <game.h>
#include <field_type.h>

#include "cata_catch.h"
#include "map.h"
#include "map_helpers.h"


TEST_CASE( "Monster pathing with field effects", "[field][monster][behavior]" )
{
    clear_map();
    map &m = get_map();

    WHEN( "fire between smart monster and target" ) {
        const tripoint monster_loc{ 32, 33, 0 };
        const tripoint fire_loc{ 33, 33, 0 };
        const tripoint target_loc{ 34, 33, 0 };
        monster &test_monster = spawn_test_monster( "mon_mi_go", monster_loc );
        m.add_field( fire_loc, fd_fire, 3 );

        THEN( "path leads around fire" ) {
            const std::vector<tripoint> &path = m.route( monster_loc, target_loc,
                                                test_monster.get_pathfinding_settings(),
                                                test_monster.get_path_avoid(),
            [test_monster]( const tripoint & p ) {
                return test_monster.danger_cost( p );
            } );
            CHECK( path.front() != fire_loc );
        }
    }

    WHEN( "fire between dumb monster and target" ) {
        const tripoint monster_loc{ 32, 33, 0 };
        const tripoint fire_loc{ 33, 33, 0 };
        const tripoint target_loc{ 34, 33, 0 };
        monster &test_monster = spawn_test_monster( "mon_zombie_runner", monster_loc );
        m.add_field( fire_loc, fd_fire, 3 );

        THEN( "path leads through fire" ) {
            const std::vector<tripoint> &path = m.route( monster_loc, target_loc,
                                                test_monster.get_pathfinding_settings(),
                                                test_monster.get_path_avoid(),
            [test_monster]( const tripoint & p ) {
                return test_monster.danger_cost( p );
            } );
            CHECK( path.front() == fire_loc );
        }
    }
}