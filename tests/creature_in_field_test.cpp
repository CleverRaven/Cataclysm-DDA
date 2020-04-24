#include <memory>

#include "catch/catch.hpp"
#include "game.h"
#include "map.h"
#include "map_helpers.h"
#include "monster.h"
#include "point.h"
#include "type_id.h"

TEST_CASE( "creature_in_field", "[monster],[field]" )
{
    static const tripoint target_location{ 5, 5, 0 };
    clear_map();
    GIVEN( "An acid field" ) {
        g->m.add_field( target_location, field_type_id( "fd_acid" ) );
        WHEN( "a monster stands on it" ) {
            monster &test_monster = spawn_test_monster( "mon_zombie", target_location );
            REQUIRE( test_monster.get_hp() == test_monster.get_hp_max() );
            THEN( "the monster takes damage" ) {
                g->m.creature_in_field( test_monster );
                CHECK( test_monster.get_hp() < test_monster.get_hp_max() );
            }
        }
        WHEN( "A monster in a vehicle stands in it" ) {
            g->m.add_vehicle( vproto_id( "handjack" ), target_location, 0 );
            monster &test_monster = spawn_test_monster( "mon_zombie", target_location );
            REQUIRE( test_monster.get_hp() == test_monster.get_hp_max() );
            THEN( "the monster doesn't take damage" ) {
                g->m.creature_in_field( test_monster );
                CHECK( test_monster.get_hp() == test_monster.get_hp_max() );
            }

        }
    }
}
