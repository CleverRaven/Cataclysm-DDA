#include "avatar.h"
#include "cata_catch.h"
#include "flag.h"
#include "map.h"
#include "map_helpers.h"
#include "player_helpers.h"

// Test cases related to movement cost
//
// Functions:
// - Character::run_cost - Modified base movement cost, with effects of mutations and footwear
//
// Mutation fields:
// - movecost_obstacle_modifier
// - movecost_modifier
// - movecost_flatground_modifier
// - stamina_move_cost_mod
//
// TODO:
// - Shoe only on one foot
// - Mycus on fungus

static const character_modifier_id
character_modifier_limb_footing_movecost_mod( "limb_footing_movecost_mod" );
static const character_modifier_id character_modifier_limb_run_cost_mod( "limb_run_cost_mod" );
static const character_modifier_id
character_modifier_limb_speed_movecost_mod( "limb_speed_movecost_mod" );

static const efftype_id effect_downed( "downed" );

static const trait_id trait_HOOVES( "HOOVES" );
static const trait_id trait_LEG_TENTACLES( "LEG_TENTACLES" );
static const trait_id trait_PADDED_FEET( "PADDED_FEET" );
static const trait_id trait_TOUGH_FEET( "TOUGH_FEET" );

TEST_CASE( "being knocked down triples movement cost", "[move_cost][downed]" )
{
    avatar &ava = get_avatar();
    clear_avatar();

    // Put on sneakers to normalize run cost to 100
    ava.wear_item( item( "sneakers" ) );
    REQUIRE( ava.run_cost( 100 ) == 100 );

    ava.add_effect( effect_downed, 1_turns );
    CHECK( ava.run_cost( 100 ) == 300 );
    CHECK( ava.run_cost( 200 ) == 600 );
    CHECK( ava.run_cost( 300 ) == 900 );
    CHECK( ava.run_cost( 400 ) == 1200 );
}

TEST_CASE( "footwear may affect movement cost", "[move_cost][shoes]" )
{
    avatar &ava = get_avatar();
    map &here = get_map();
    clear_avatar();
    clear_map();

    // Ensure no interference from mutations
    REQUIRE( ava.mutation_value( "movecost_modifier" ) == 1 );
    REQUIRE( ava.mutation_value( "movecost_flatground_modifier" ) == 1 );
    REQUIRE( ava.mutation_value( "movecost_obstacle_modifier" ) == 1 );
    // Ensure expected base modifiers
    REQUIRE( ava.get_modifier( character_modifier_limb_run_cost_mod ) == 1 );
    REQUIRE( ava.get_modifier( character_modifier_limb_speed_movecost_mod ) == 1 );
    REQUIRE( ava.get_modifier( character_modifier_limb_footing_movecost_mod ) == 1 );

    SECTION( "without shoes" ) {
        ava.worn.clear();
        REQUIRE_FALSE( ava.is_wearing_shoes() );
        // Having no shoes adds +8 per foot to base run cost
        CHECK( ava.run_cost( 100 ) == 116 );
    }

    SECTION( "wearing sneakers" ) {
        ava.worn.clear();
        ava.wear_item( item( "sneakers" ) );
        REQUIRE( ava.is_wearing_shoes() );
        REQUIRE( ava.get_modifier( character_modifier_limb_run_cost_mod ) == Approx( 1.0 ) );
        // Sneakers eliminate the no-shoes penalty
        CHECK( ava.run_cost( 100 ) == 100 );
    }

    SECTION( "wearing swim fins" ) {
        ava.worn.clear();
        ava.wear_item( item( "swim_fins" ) );
        REQUIRE( ava.is_wearing_shoes() );
        REQUIRE( ava.get_modifier( character_modifier_limb_run_cost_mod ) == Approx( 1.11696 ) );
        // Swim fins multiply cost by 1.5
        CHECK( ava.run_cost( 100 ) == 167 );
    }

    GIVEN( "wearing rollerblades (ROLLER_INLINE)" ) {
        ava.worn.clear();
        ava.wear_item( item( "roller_blades" ) );
        REQUIRE( ava.worn_with_flag( flag_ROLLER_INLINE ) );
        REQUIRE( ava.get_modifier( character_modifier_limb_run_cost_mod ) == Approx( 1.11696 ) );
        WHEN( "on pavement" ) {
            here.ter_set( ava.pos(), t_pavement );
            THEN( "much faster than sneakers" ) {
                CHECK( ava.run_cost( 100 ) == 55 );
            }
        }
        WHEN( "on grass" ) {
            here.ter_set( ava.pos(), t_grass );
            THEN( "much slower than sneakers" ) {
                CHECK( ava.run_cost( 100 ) == 167 );
            }
        }
    }

    GIVEN( "wearing roller skates (ROLLER_QUAD)" ) {
        ava.worn.clear();
        ava.wear_item( item( "rollerskates" ) );
        REQUIRE( ava.worn_with_flag( flag_ROLLER_QUAD ) );
        REQUIRE( ava.get_modifier( character_modifier_limb_run_cost_mod ) == Approx( 1.11696 ) );
        WHEN( "on pavement" ) {
            here.ter_set( ava.pos(), t_pavement );
            THEN( "faster than sneakers" ) {
                CHECK( ava.run_cost( 100 ) == 78 );
            }
        }
        WHEN( "on grass" ) {
            here.ter_set( ava.pos(), t_grass );
            THEN( "slower than sneakers" ) {
                CHECK( ava.run_cost( 100 ) == 145 );
            }
        }
    }

    GIVEN( "wearing heelys (ROLLER_ONE)" ) {
        ava.worn.clear();
        ava.wear_item( item( "roller_shoes_on" ) );
        REQUIRE( ava.worn_with_flag( flag_ROLLER_ONE ) );
        REQUIRE( ava.get_modifier( character_modifier_limb_run_cost_mod ) == Approx( 1.0 ) );
        WHEN( "on pavement" ) {
            here.ter_set( ava.pos(), t_pavement );

            THEN( "slightly faster than sneakers" ) {
                CHECK( ava.run_cost( 100 ) == 85 );
            }
        }
        WHEN( "on grass" ) {
            here.ter_set( ava.pos(), t_grass );
            THEN( "slightly slower than sneakers" ) {
                CHECK( ava.run_cost( 100 ) == 110 );
            }
        }
    }
}

TEST_CASE( "mutations may affect movement cost", "[move_cost][mutation]" )
{
    avatar &ava = get_avatar();
    clear_avatar();

    // Base movement cost for relative comparisons
    // This is the speed of an unmutated character wearing sneakers on both feet
    const int base_cost = 100;

    GIVEN( "no mutations" ) {
        THEN( "wearing sneakers gives baseline 100 movement speed" ) {
            ava.worn.clear();
            ava.wear_item( item( "sneakers" ) );
            CHECK( ava.run_cost( 100 ) == Approx( base_cost ) );
        }
        THEN( "being barefoot gives a +16 movement cost penalty" ) {
            ava.worn.clear();
            CHECK( ava.run_cost( 100 ) == Approx( base_cost + 16 ) );
        }
    }

    GIVEN( "LEG_TENTACLES" ) {
        ava.toggle_trait( trait_LEG_TENTACLES );
        THEN( "barefoot penalty does not apply, but movement is slower" ) {
            ava.worn.clear();
            CHECK( ava.run_cost( 100 ) == Approx( 1.2 * base_cost ) );
        }
    }

    GIVEN( "HOOVES" ) {
        ava.toggle_trait( trait_HOOVES );
        THEN( "barefoot penalty does not apply" ) {
            ava.worn.clear();
            CHECK( ava.run_cost( 100 ) == Approx( base_cost ) );
        }
    }

    GIVEN( "TOUGH_FEET" ) {
        ava.toggle_trait( trait_TOUGH_FEET );
        THEN( "barefoot penalty does not apply" ) {
            ava.worn.clear();
            CHECK( ava.run_cost( 100 ) == Approx( base_cost ) );
        }
    }

    GIVEN( "PADDED_FEET" ) {
        ava.toggle_trait( trait_PADDED_FEET );
        THEN( "wearing sneakers gives baseline 100 movement speed" ) {
            ava.worn.clear();
            ava.wear_item( item( "sneakers" ) );
            CHECK( ava.run_cost( 100 ) == Approx( base_cost ) );
        }
        THEN( "being barefoot is faster than wearing sneakers" ) {
            ava.worn.clear();
            CHECK( ava.run_cost( 100 ) == Approx( 0.9 * base_cost ) );
        }
    }
}

