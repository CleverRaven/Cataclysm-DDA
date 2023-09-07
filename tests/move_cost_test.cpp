#include "avatar.h"
#include "avatar_action.h"
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

static const move_mode_id move_mode_crouch( "crouch" );
static const move_mode_id move_mode_prone( "prone" );
static const move_mode_id move_mode_run( "run" );
static const move_mode_id move_mode_walk( "walk" );

static const trait_id trait_HOOVES( "HOOVES" );
static const trait_id trait_LEG_TENTACLES( "LEG_TENTACLES" );
static const trait_id trait_PADDED_FEET( "PADDED_FEET" );
static const trait_id trait_TOUGH_FEET( "TOUGH_FEET" );

TEST_CASE( "being_knocked_down_triples_movement_cost", "[move_cost][downed]" )
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

TEST_CASE( "footwear_may_affect_movement_cost", "[move_cost][shoes]" )
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
        ava.clear_worn();
        REQUIRE_FALSE( ava.is_wearing_shoes() );
        // Having no shoes adds +8 per foot to base run cost
        CHECK( ava.run_cost( 100 ) == 116 );
    }

    SECTION( "wearing sneakers" ) {
        ava.clear_worn();
        ava.wear_item( item( "sneakers" ) );
        REQUIRE( ava.is_wearing_shoes() );
        REQUIRE( ava.get_modifier( character_modifier_limb_run_cost_mod ) == Approx( 1.0 ) );
        // Sneakers eliminate the no-shoes penalty
        CHECK( ava.run_cost( 100 ) == 100 );
    }

    SECTION( "wearing swim fins" ) {
        ava.clear_worn();
        ava.wear_item( item( "swim_fins" ) );
        REQUIRE( ava.is_wearing_shoes() );
        REQUIRE( ava.get_modifier( character_modifier_limb_run_cost_mod ) == Approx( 1.11696 ) );
        // Swim fins multiply cost by 1.5
        CHECK( ava.run_cost( 100 ) == 167 );
    }

    GIVEN( "wearing rollerblades (ROLLER_INLINE)" ) {
        ava.clear_worn();
        ava.wear_item( item( "roller_blades" ) );
        REQUIRE( ava.worn_with_flag( flag_ROLLER_INLINE ) );
        REQUIRE( ava.get_modifier( character_modifier_limb_run_cost_mod ) == Approx( 1.11696 ) );
        WHEN( "on pavement and running" ) {
            ava.set_movement_mode( move_mode_run );
            here.ter_set( ava.pos(), t_pavement );
            THEN( "much faster than sneakers" ) {
                CHECK( ava.run_cost( 100 ) == 27 );
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
        ava.clear_worn();
        ava.wear_item( item( "rollerskates" ) );
        REQUIRE( ava.worn_with_flag( flag_ROLLER_QUAD ) );
        REQUIRE( ava.get_modifier( character_modifier_limb_run_cost_mod ) == Approx( 1.11696 ) );
        WHEN( "on pavement and running" ) {
            ava.set_movement_mode( move_mode_run );
            here.ter_set( ava.pos(), t_pavement );
            THEN( "faster than sneakers" ) {
                CHECK( ava.run_cost( 100 ) == 39 );
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
        ava.clear_worn();
        ava.wear_item( item( "roller_shoes_on" ) );
        REQUIRE( ava.worn_with_flag( flag_ROLLER_ONE ) );
        REQUIRE( ava.get_modifier( character_modifier_limb_run_cost_mod ) == Approx( 1.0 ) );
        WHEN( "on pavement and running" ) {
            ava.set_movement_mode( move_mode_run );
            here.ter_set( ava.pos(), t_pavement );
            THEN( "slightly faster than sneakers" ) {
                CHECK( ava.run_cost( 100 ) == 42 );
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

TEST_CASE( "mutations_may_affect_movement_cost", "[move_cost][mutation]" )
{
    avatar &ava = get_avatar();
    clear_avatar();

    // Base movement cost for relative comparisons
    // This is the speed of an unmutated character wearing sneakers on both feet
    const int base_cost = 100;

    GIVEN( "no mutations" ) {
        THEN( "wearing sneakers gives baseline 100 movement speed" ) {
            ava.clear_worn();
            ava.wear_item( item( "sneakers" ) );
            CHECK( ava.run_cost( 100 ) == Approx( base_cost ) );
        }
        THEN( "being barefoot gives a +16 movement cost penalty" ) {
            ava.clear_worn();
            CHECK( ava.run_cost( 100 ) == Approx( base_cost + 16 ) );
        }
    }

    GIVEN( "LEG_TENTACLES" ) {
        ava.toggle_trait( trait_LEG_TENTACLES );
        THEN( "barefoot penalty does not apply, but movement is slower" ) {
            ava.clear_worn();
            CHECK( ava.run_cost( 100 ) == Approx( 1.2 * base_cost ) );
        }
    }

    GIVEN( "HOOVES" ) {
        ava.toggle_trait( trait_HOOVES );
        THEN( "barefoot penalty does not apply" ) {
            ava.clear_worn();
            CHECK( ava.run_cost( 100 ) == Approx( base_cost ) );
        }
    }

    GIVEN( "TOUGH_FEET" ) {
        ava.toggle_trait( trait_TOUGH_FEET );
        THEN( "barefoot penalty does not apply" ) {
            ava.clear_worn();
            CHECK( ava.run_cost( 100 ) == Approx( base_cost ) );
        }
    }

    GIVEN( "PADDED_FEET" ) {
        ava.toggle_trait( trait_PADDED_FEET );
        THEN( "wearing sneakers gives baseline 100 movement speed" ) {
            ava.clear_worn();
            ava.wear_item( item( "sneakers" ) );
            CHECK( ava.run_cost( 100 ) == Approx( base_cost ) );
        }
        THEN( "being barefoot is faster than wearing sneakers" ) {
            ava.clear_worn();
            CHECK( ava.run_cost( 100 ) == Approx( 0.9 * base_cost ) );
        }
    }
}

TEST_CASE( "Crawl_score_effects_on_movement_cost", "[move_cost]" )
{
    // No limb damage
    GIVEN( "Character is uninjured and unencumbered" ) {
        avatar &u = get_avatar();
        clear_avatar();
        clear_map();
        u.wear_item( item( "sneakers" ) );
        u.moves = 0;

        WHEN( "is walking" ) {
            u.set_movement_mode( move_mode_walk );
            REQUIRE( u.moves == 0 );
            THEN( "no crawling modifier" ) {
                CHECK( u.run_cost( 100 ) == 100 );
                avatar_action::move( u, get_map(), point_south );
                CHECK( u.moves == -100 );
            }
        }
        WHEN( "is crouching" ) {
            u.set_movement_mode( move_mode_crouch );
            REQUIRE( u.moves == 0 );
            THEN( "no crawling modifier" ) {
                CHECK( u.run_cost( 100 ) == 200 );
                avatar_action::move( u, get_map(), point_south );
                CHECK( u.moves == -200 );
            }
        }
        WHEN( "is prone" ) {
            u.set_movement_mode( move_mode_prone );
            REQUIRE( u.moves == 0 );
            THEN( "apply crawling modifier" ) {
                CHECK( u.run_cost( 100 ) == 600 );
                avatar_action::move( u, get_map(), point_south );
                CHECK( u.moves == -600 );
            }
        }
    }
    GIVEN( "Character is uninjured and has 30 arm encumbrance" ) {
        avatar &u = get_avatar();
        clear_avatar();
        u.wear_item( item( "sneakers" ) );
        u.wear_item( item( "test_briefcase" ) );
        u.moves = 0;

        WHEN( "is walking" ) {
            u.set_movement_mode( move_mode_walk );
            REQUIRE( u.moves == 0 );
            THEN( "no crawling modifier" ) {
                CHECK( u.run_cost( 100 ) == 100 );
                avatar_action::move( u, get_map(), point_south );
                CHECK( u.moves == -100 );
            }
        }
        WHEN( "is crouching" ) {
            u.set_movement_mode( move_mode_crouch );
            REQUIRE( u.moves == 0 );
            THEN( "no crawling modifier" ) {
                CHECK( u.run_cost( 100 ) == 200 );
                avatar_action::move( u, get_map(), point_south );
                CHECK( u.moves == -200 );
            }
        }
        WHEN( "is prone" ) {
            u.set_movement_mode( move_mode_prone );
            REQUIRE( u.moves == 0 );
            THEN( "apply crawling modifier" ) {
                CHECK( u.run_cost( 100 ) == 660 );
                avatar_action::move( u, get_map(), point_south );
                CHECK( u.moves == -660 );
            }
        }
    }
    GIVEN( "Character is uninjured and has 37 encumbrance, all limbs" ) {
        avatar &u = get_avatar();
        clear_avatar();
        u.wear_item( item( "test_hazmat_suit" ) );
        u.moves = 0;

        WHEN( "is walking" ) {
            u.set_movement_mode( move_mode_walk );
            REQUIRE( u.moves == 0 );
            THEN( "no crawling modifier" ) {
                CHECK( u.run_cost( 100 ) == 154 );
                avatar_action::move( u, get_map(), point_south );
                CHECK( u.moves == -154 );
            }
        }
        WHEN( "is crouching" ) {
            u.set_movement_mode( move_mode_crouch );
            REQUIRE( u.moves == 0 );
            THEN( "no crawling modifier" ) {
                CHECK( u.run_cost( 100 ) == 309 );
                avatar_action::move( u, get_map(), point_south );
                CHECK( u.moves == -309 );
            }
        }
        WHEN( "is prone" ) {
            u.set_movement_mode( move_mode_prone );
            REQUIRE( u.moves == 0 );
            THEN( "apply crawling modifier" ) {
                CHECK( u.run_cost( 100 ) == 932 );
                avatar_action::move( u, get_map(), point_south );
                CHECK( u.moves == -932 );
            }
        }
    }
    // Damaged arm
    GIVEN( "Character has damaged arm and is unencumbered" ) {
        avatar &u = get_avatar();
        clear_avatar();
        u.wear_item( item( "sneakers" ) );
        u.set_part_hp_cur( body_part_arm_l, 10 );
        u.moves = 0;

        WHEN( "is walking" ) {
            u.set_movement_mode( move_mode_walk );
            REQUIRE( u.moves == 0 );
            THEN( "no crawling modifier" ) {
                CHECK( u.run_cost( 100 ) == 100 );
                avatar_action::move( u, get_map(), point_south );
                CHECK( u.moves == -100 );
            }
        }
        WHEN( "is crouching" ) {
            u.set_movement_mode( move_mode_crouch );
            REQUIRE( u.moves == 0 );
            THEN( "no crawling modifier" ) {
                CHECK( u.run_cost( 100 ) == 200 );
                avatar_action::move( u, get_map(), point_south );
                CHECK( u.moves == -200 );
            }
        }
        WHEN( "is prone" ) {
            u.set_movement_mode( move_mode_prone );
            REQUIRE( u.moves == 0 );
            THEN( "apply crawling modifier" ) {
                CHECK( u.run_cost( 100 ) == 802 );
                avatar_action::move( u, get_map(), point_south );
                CHECK( u.moves == -802 );
            }
        }
    }
    GIVEN( "Character has damaged arm and 30 arm encumbrance" ) {
        avatar &u = get_avatar();
        clear_avatar();
        u.wear_item( item( "sneakers" ) );
        u.wear_item( item( "test_briefcase" ) );
        u.set_part_hp_cur( body_part_arm_l, 10 );
        u.moves = 0;

        WHEN( "is walking" ) {
            u.set_movement_mode( move_mode_walk );
            REQUIRE( u.moves == 0 );
            THEN( "no crawling modifier" ) {
                CHECK( u.run_cost( 100 ) == 100 );
                avatar_action::move( u, get_map(), point_south );
                CHECK( u.moves == -100 );
            }
        }
        WHEN( "is crouching" ) {
            u.set_movement_mode( move_mode_crouch );
            REQUIRE( u.moves == 0 );
            THEN( "no crawling modifier" ) {
                CHECK( u.run_cost( 100 ) == 200 );
                avatar_action::move( u, get_map(), point_south );
                CHECK( u.moves == -200 );
            }
        }
        WHEN( "is prone" ) {
            u.set_movement_mode( move_mode_prone );
            REQUIRE( u.moves == 0 );
            THEN( "apply crawling modifier" ) {
                CHECK( u.run_cost( 100 ) == 818 );
                avatar_action::move( u, get_map(), point_south );
                CHECK( u.moves == -818 );
            }
        }
    }
    GIVEN( "Character has damaged arm and 37 encumbrance, all limbs" ) {
        avatar &u = get_avatar();
        clear_avatar();
        u.wear_item( item( "test_hazmat_suit" ) );
        u.set_part_hp_cur( body_part_arm_l, 10 );
        u.moves = 0;

        WHEN( "is walking" ) {
            u.set_movement_mode( move_mode_walk );
            REQUIRE( u.moves == 0 );
            THEN( "no crawling modifier" ) {
                CHECK( u.run_cost( 100 ) == 154 );
                avatar_action::move( u, get_map(), point_south );
                CHECK( u.moves == -154 );
            }
        }
        WHEN( "is crouching" ) {
            u.set_movement_mode( move_mode_crouch );
            REQUIRE( u.moves == 0 );
            THEN( "no crawling modifier" ) {
                CHECK( u.run_cost( 100 ) == 309 );
                avatar_action::move( u, get_map(), point_south );
                CHECK( u.moves == -309 );
            }
        }
        WHEN( "is prone" ) {
            u.set_movement_mode( move_mode_prone );
            REQUIRE( u.moves == 0 );
            THEN( "apply crawling modifier" ) {
                CHECK( u.run_cost( 100 ) == 1245 );
                avatar_action::move( u, get_map(), point_south );
                CHECK( u.moves == -1245 );
            }
        }
    }
    // Broken legs
    GIVEN( "Character has 2 broken legs and is unencumbered" ) {
        avatar &u = get_avatar();
        clear_avatar();
        u.wear_item( item( "sneakers" ) );
        u.set_part_hp_cur( body_part_leg_l, 0 );
        u.set_part_hp_cur( body_part_leg_r, 0 );
        u.moves = 0;

        // Can't walk or couch w/ broken legs
        WHEN( "is prone" ) {
            u.set_movement_mode( move_mode_prone );
            REQUIRE( u.moves == 0 );
            THEN( "apply crawling modifier" ) {
                CHECK( u.run_cost( 100 ) == 1000 );
                avatar_action::move( u, get_map(), point_south );
                CHECK( u.moves == -1000 );
            }
        }
    }
    GIVEN( "Character has 2 broken legs arm and 30 arm encumbrance" ) {
        avatar &u = get_avatar();
        clear_avatar();
        u.wear_item( item( "sneakers" ) );
        u.wear_item( item( "test_briefcase" ) );
        u.set_part_hp_cur( body_part_leg_l, 0 );
        u.set_part_hp_cur( body_part_leg_r, 0 );
        u.moves = 0;

        // Can't walk or couch w/ broken legs
        WHEN( "is prone" ) {
            u.set_movement_mode( move_mode_prone );
            REQUIRE( u.moves == 0 );
            THEN( "apply crawling modifier" ) {
                CHECK( u.run_cost( 100 ) == 1179 );
                avatar_action::move( u, get_map(), point_south );
                CHECK( u.moves == -1179 );
            }
        }
    }
    GIVEN( "Character has 2 broken legs and 37 encumbrance, all limbs" ) {
        avatar &u = get_avatar();
        clear_avatar();
        u.wear_item( item( "test_hazmat_suit" ) );
        u.set_part_hp_cur( body_part_leg_l, 0 );
        u.set_part_hp_cur( body_part_leg_r, 0 );
        u.moves = 0;

        // Can't walk or couch w/ broken legs
        WHEN( "is prone" ) {
            u.set_movement_mode( move_mode_prone );
            REQUIRE( u.moves == 0 );
            THEN( "apply crawling modifier" ) {
                CHECK( u.run_cost( 100 ) == 1561 );
                avatar_action::move( u, get_map(), point_south );
                CHECK( u.moves == -1561 );
            }
        }
    }
}
