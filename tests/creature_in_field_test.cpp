#include "cata_catch.h"
#include "map.h"
#include "map_helpers.h"
#include "monster.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"
#include "units.h"

static const vproto_id vehicle_prototype_handjack( "handjack" );

TEST_CASE( "creature_in_field", "[monster],[field]" )
{
    static const tripoint target_location{ 5, 5, 0 };
    clear_map();
    map &here = get_map();
    GIVEN( "An acid field" ) {
        here.add_field( target_location, field_type_id( "fd_acid" ) );
        WHEN( "a monster stands on it" ) {
            monster &test_monster = spawn_test_monster( "mon_zombie", target_location );
            REQUIRE( test_monster.get_hp() == test_monster.get_hp_max() );
            THEN( "the monster takes damage" ) {
                here.creature_in_field( test_monster );
                CHECK( test_monster.get_hp() < test_monster.get_hp_max() );
            }
        }
        WHEN( "A monster in a vehicle stands in it" ) {
            here.add_vehicle( vehicle_prototype_handjack, target_location, 0_degrees );
            monster &test_monster = spawn_test_monster( "mon_zombie", target_location );
            REQUIRE( test_monster.get_hp() == test_monster.get_hp_max() );
            THEN( "the monster doesn't take damage" ) {
                here.creature_in_field( test_monster );
                CHECK( test_monster.get_hp() == test_monster.get_hp_max() );
            }

        }
    }
}

TEST_CASE( "character in fd_electric field", "[field]" )
{
    static const tripoint target_location{ 5, 5, 0 };
    clear_map();
    map &here = get_map();
    standard_npc dummy( "TestCharacter", target_location, {}, 0, 8, 8, 8, 8 );
    dummy.setpos( target_location );
    dummy.healall( 100 );

    GIVEN( "an electric field of intensity 2" ) {
        here.add_field( target_location, field_type_id( "fd_electricity" ), 2 );

        WHEN( "character stands on the field for a few turns" ) {
            for( int i = 0; i < 20 && !dummy.is_dead(); i++ ) {
                here.creature_in_field( dummy );
            }

            THEN( "character takes damage to the legs" ) {
                CHECK( dummy.get_hp( body_part_leg_l ) < dummy.get_hp_max( body_part_leg_l ) );
                CHECK( dummy.get_hp( body_part_leg_r ) < dummy.get_hp_max( body_part_leg_r ) );
            }
        }

        AND_GIVEN( "character is wearing rubber boots" ) {
            item rubber_boots = item( "boots_rubber" );
            dummy.wear_item( rubber_boots, false );

            WHEN( "character stands on the field for a few turns" ) {
                for( int i = 0; i < 20 && !dummy.is_dead(); i++ ) {
                    here.creature_in_field( dummy );
                }

                THEN( "character doesn't take any damage" ) {
                    CHECK( dummy.get_hp( body_part_leg_l ) == dummy.get_hp_max( body_part_leg_l ) );
                    CHECK( dummy.get_hp( body_part_leg_r ) == dummy.get_hp_max( body_part_leg_r ) );
                }

            }
        }
    }

    GIVEN( "an electric field of intensity 6" ) {
        here.add_field( target_location, field_type_id( "fd_electricity" ), 6 );

        AND_GIVEN( "character is wearing rubber boots" ) {
            item rubber_boots = item( "boots_rubber" );
            dummy.wear_item( rubber_boots, false );

            WHEN( "character stands on the field for a few turns" ) {
                for( int i = 0; i < 10 && !dummy.is_dead(); i++ ) {
                    here.creature_in_field( dummy );
                }

                THEN( "character takes damage" ) {
                    CHECK( dummy.get_hp() < dummy.get_hp_max() );
                }
            }
        }
    }
}
