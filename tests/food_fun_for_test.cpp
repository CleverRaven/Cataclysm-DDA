#include "avatar.h"
#include "cata_string_consts.h"
#include "catch/catch.hpp"
#include "type_id.h"


// Test cases for `Character::fun_for` defined in `src/consumption.cpp`

TEST_CASE( "non-food has no food fun value", "[fun_for][nonfood]" )
{
    avatar dummy;
    std::pair<int, int> fun;

    item rag( "rag" );
    REQUIRE_FALSE( rag.is_comestible() );
    fun = dummy.fun_for( rag );
    CHECK( fun.first == 0 );
    CHECK( fun.second == 0 );
}

TEST_CASE( "effect of sickness on food fun", "[food][fun_for][sick]" )
{
    avatar dummy;
    std::pair<int, int> fun;
    item &toastem = dummy.i_add( item( "toastem" ) );
    // Base fun value for toast-em
    int toastem_fun = toastem.get_comestible_fun();

    WHEN( "avatar has a cold" ) {
        dummy.add_effect( efftype_id( "common_cold" ), 1_days );
        REQUIRE( dummy.has_effect( efftype_id( "common_cold" ) ) );

        THEN( "it is much less fun to eat" ) {
            fun = dummy.fun_for( toastem );
            CHECK( fun.first == toastem_fun / 3 );
        }
    }

    WHEN( "avatar has the flu" ) {
        dummy.add_effect( efftype_id( "flu" ), 1_days );
        REQUIRE( dummy.has_effect( efftype_id( "flu" ) ) );

        THEN( "it is much less fun to eat" ) {
            fun = dummy.fun_for( toastem );
            CHECK( fun.first == toastem_fun / 3 );
        }
    }
}

TEST_CASE( "rotten food fun", "[food][fun_for][rotten]" )
{
    avatar dummy;
    std::pair<int, int> fun;

    GIVEN( "some rotten food" ) {
        item &nuts = dummy.i_add( item( "pine_nuts" ) );
        // food rot > 1.0 is rotten
        nuts.set_relative_rot( 1.5 );
        REQUIRE( nuts.rotten() );

        WHEN( "avatar has normal tastes" ) {
            REQUIRE_FALSE( dummy.has_trait( trait_id( "SAPROPHAGE" ) ) );
            REQUIRE_FALSE( dummy.has_trait( trait_id( "SAPROVORE" ) ) );

            THEN( "they don't like rotten food" ) {
                fun = dummy.fun_for( nuts );
                CHECK( fun.first < 0 );
            }
        }

        WHEN( "avatar is a saprophage" ) {
            dummy.toggle_trait( trait_id( "SAPROPHAGE" ) );
            REQUIRE( dummy.has_trait( trait_id( "SAPROPHAGE" ) ) );

            THEN( "they don't mind rotten food" ) {
                fun = dummy.fun_for( nuts );
                CHECK( fun.first > 0 );
            }
        }
        WHEN( "avatar is a saprovore" ) {
            dummy.toggle_trait( trait_id( "SAPROVORE" ) );
            REQUIRE( dummy.has_trait( trait_id( "SAPROVORE" ) ) );

            THEN( "they don't mind rotten food" ) {
                fun = dummy.fun_for( nuts );
                CHECK( fun.first > 0 );
            }
        }
    }
}

// N.B. food that tastes better hot is `modify_morale` with different math
TEST_CASE( "cold food fun", "[food][fun_for][cold]" )
{
    avatar dummy;
    std::pair<int, int> fun;

    GIVEN( "food that tastes good, but better when cold" ) {
        item cola( "cola" );
        REQUIRE( cola.has_flag( flag_EATEN_COLD ) );
        int cola_fun = cola.get_comestible_fun();

        WHEN( "it is cold" ) {
            cola.set_flag( flag_COLD );
            REQUIRE( cola.has_flag( flag_COLD ) );

            THEN( "it is more fun than normal" ) {
                fun = dummy.fun_for( cola );
                CHECK( fun.first == cola_fun * 2 );
            }
        }

        WHEN( "it is not cold" ) {
            REQUIRE_FALSE( cola.has_flag( flag_COLD ) );
            THEN( "it has normal fun" ) {
                fun = dummy.fun_for( cola );
                CHECK( fun.first == cola_fun );
            }
        }
    }

    GIVEN( "food that tastes bad, but better when cold" ) {
        item sports( "sports_drink" );
        int sports_fun = sports.get_comestible_fun();

        REQUIRE( sports_fun < 0 );
        REQUIRE( sports.has_flag( flag_EATEN_COLD ) );

        WHEN( "it is cold" ) {
            sports.set_flag( flag_COLD );
            REQUIRE( sports.has_flag( flag_COLD ) );

            THEN( "it doesn't taste bad" ) {
                fun = dummy.fun_for( sports );
                CHECK( fun.first > 0 );
            }
        }

        WHEN( "it is not cold" ) {
            REQUIRE_FALSE( sports.has_flag( flag_COLD ) );

            THEN( "it tastes as bad as usual" ) {
                fun = dummy.fun_for( sports );
                CHECK( fun.first == sports_fun );
            }
        }
    }
}

TEST_CASE( "melted food fun", "[food][fun_for][melted]" )
{
    avatar dummy;
    std::pair<int, int> fun;

    GIVEN( "food that is fun but melts" ) {
        item icecream( "icecream" );
        REQUIRE( icecream.has_flag( flag_MELTS ) );
        int icecream_fun = icecream.get_comestible_fun();

        WHEN( "it is not frozen" ) {
            REQUIRE_FALSE( icecream.has_flag( flag_FROZEN ) );

            THEN( "it is half as fun" ) {
                fun = dummy.fun_for( icecream );
                CHECK( fun.first == icecream_fun / 2 );
            }
        }
    }

    /* TODO: Mock-up such a food, as none exist in game
    GIVEN( "food that is not fun and also melts" ) {
        WHEN( "it is not frozen" ) {
            THEN( "it tastes worse" ) {
            }
        }
    }
    */
}

// Base fun level with no other effects is get_comestible_fun()
// - get_comestible_fun() is where flag_MUSHY is applied

// Food is less enjoyable when eaten too often
//
// Dog food tastes good to dogs
// Cat food tastes good to cats
// Gourmands really enjoy food
// Bionic taste blocker (if it has enough power) nullifies bad-tasting food
//
