#include "avatar.h"
#include "character.h"
#include "catch/catch.hpp"
#include "cata_string_consts.h"
#include "morale.h"

// Test cases for `Character::modify_morale` defined in `src/consumption.cpp`

TEST_CASE( "food enjoyability", "[modify_morale][fun]" )
{
    avatar dummy;
    std::pair<int, int> fun;

    // FIXME: the `fun_for` method itself has quite a few caveats and addendums

    GIVEN( "food with positive fun" ) {
        item &toastem = dummy.i_add( item( "toastem" ) );
        fun = dummy.fun_for( toastem );
        REQUIRE( fun.first > 0 );

        THEN( "character gets a morale bonus for it" ) {
            dummy.modify_morale( toastem );
            CHECK( dummy.has_morale( MORALE_FOOD_GOOD ) );
            // Positive morale (first < second)
            CHECK( fun.first <= dummy.get_morale_level() );
            CHECK( dummy.get_morale_level() <= fun.second );
        }
    }

    GIVEN( "food with negative fun" ) {
        item &garlic = dummy.i_add( item( "garlic" ) );
        fun = dummy.fun_for( garlic );
        REQUIRE( fun.first < 0 );

        THEN( "character gets a morale penalty for it" ) {
            dummy.modify_morale( garlic );
            CHECK( dummy.has_morale( MORALE_FOOD_BAD ) );
            // Negative morale (second < first)
            CHECK( fun.second <= dummy.get_morale_level() );
            CHECK( dummy.get_morale_level() <= fun.first );
        }
    }
}

// non-rotten food that you're not allergic to:
// - gives ATE_WITH_TABLE if table and chair are near
// - gives ATE_WITHOUT_TABLE (bad) if TABLEMANNERS trait
//
TEST_CASE( "eating with a table", "[modify_morale][table]" )
{
}

// FIXME: NO FOOD IN THE GAME HAS THIS FLAG
//
// food with HIDDEN_HALLU flag
// - gives MORALE_FOOD_GOOD
// - even more so for SPIRITUAL trait


// hot food morale bonus lasts 3 hours
// hot food adds MORALE_FOOD_HOT



// food with CANNIBALISM flag:
// - adds MORALE_CANNIBAL for ALL characters
// - amount varies by PSYCHOPATH, SAPIOVORE, SPIRITUAL
// - gives huge morale penalty for non-cannibals
TEST_CASE( "cannibalism", "[modify_morale][cannibal]" )
{
    avatar dummy;

    // TODO: SAPIOVORE

    GIVEN( "food for a cannibal" ) {
        item &human = dummy.i_add( item( "bone_human" ) );

        WHEN( "character is not a cannibal" ) {
            REQUIRE_FALSE( dummy.has_trait( trait_CANNIBAL ) );

            THEN( "they get a large morale penalty for eating it" ) {
                dummy.modify_morale( human );
                CHECK( dummy.has_morale( MORALE_CANNIBAL ) );
                CHECK( dummy.get_morale_level() <= -60 );
            }

            AND_WHEN( "character is a psychopath" ) {
                dummy.toggle_trait( trait_PSYCHOPATH );
                REQUIRE( dummy.has_trait( trait_PSYCHOPATH ) );

                THEN( "their morale is unffected by eating it" ) {
                    dummy.modify_morale( human );
                    CHECK_FALSE( dummy.has_morale( MORALE_CANNIBAL ) );
                    CHECK( dummy.get_morale_level() == 0 );
                }

                AND_WHEN( "they are also spiritual" ) {
                    dummy.toggle_trait( trait_SPIRITUAL );
                    REQUIRE( dummy.has_trait( trait_SPIRITUAL ) );

                    THEN( "they get a small morale bonus for eating it" ) {
                        dummy.modify_morale( human );
                        CHECK( dummy.has_morale( MORALE_CANNIBAL ) );
                        CHECK( dummy.get_morale_level() >= 5 );
                    }
                }
            }
        }

        WHEN( "character is a cannibal" ) {
            dummy.toggle_trait( trait_CANNIBAL );
            REQUIRE( dummy.has_trait( trait_CANNIBAL ) );

            THEN( "they get a morale bonus for eating it" ) {
                dummy.modify_morale( human );
                CHECK( dummy.has_morale( MORALE_CANNIBAL ) );
                CHECK( dummy.get_morale_level() >= 10 );
            }

            AND_WHEN( "they are also a psychopath" ) {
                dummy.toggle_trait( trait_PSYCHOPATH );
                REQUIRE( dummy.has_trait( trait_PSYCHOPATH ) );

                THEN( "they get a substantial morale bonus for eating it" ) {
                    dummy.modify_morale( human );
                    CHECK( dummy.has_morale( MORALE_CANNIBAL ) );
                    CHECK( dummy.get_morale_level() >= 15 );
                }

                AND_WHEN( "they are also spiritual" ) {
                    dummy.toggle_trait( trait_SPIRITUAL );
                    REQUIRE( dummy.has_trait( trait_SPIRITUAL ) );

                    THEN( "they get a large morale bonus for eating it" ) {
                        dummy.modify_morale( human );
                        CHECK( dummy.has_morale( MORALE_CANNIBAL ) );
                        CHECK( dummy.get_morale_level() >= 25 );
                    }
                }
            }
        }
    }
}

TEST_CASE( "sweet junk food", "[modify_morale][junk][sweet]" )
{
    avatar dummy;
    std::pair<int, int> fun;

    GIVEN( "some sweet junk food" ) {
        item &necco = dummy.i_add( item( "neccowafers" ) );

        WHEN( "character has a sweet tooth" ) {
            dummy.toggle_trait( trait_PROJUNK );
            REQUIRE( dummy.has_trait( trait_PROJUNK ) );

            THEN( "they get a morale bonus from its sweetness" ) {
                dummy.modify_morale( necco );
                CHECK( dummy.has_morale( MORALE_SWEETTOOTH ) );
                CHECK( dummy.get_morale_level() >= 5 );
                CHECK( dummy.get_morale_level() <= 30 );

                AND_THEN( "they enjoy it" ) {
                    CHECK( dummy.has_morale( MORALE_FOOD_GOOD ) );
                }
            }
        }

        WHEN( "character is sugar-loving" ) {
            dummy.toggle_trait( trait_PROJUNK2 );
            REQUIRE( dummy.has_trait( trait_PROJUNK2 ) );

            THEN( "they get a significant morale bonus from its sweetness" ) {
                dummy.modify_morale( necco );
                CHECK( dummy.has_morale( MORALE_SWEETTOOTH ) );
                CHECK( dummy.get_morale_level() >= 10 );
                CHECK( dummy.get_morale_level() <= 50 );

                AND_THEN( "they enjoy it" ) {
                    CHECK( dummy.has_morale( MORALE_FOOD_GOOD ) );
                }
            }
        }

        WHEN( "character is a carnivore" ) {
            dummy.toggle_trait( trait_CARNIVORE );
            REQUIRE( dummy.has_trait( trait_CARNIVORE ) );

            THEN( "they get an overall morale penalty due to indigestion" ) {
                dummy.modify_morale( necco );
                CHECK( dummy.has_morale( MORALE_NO_DIGEST ) );
                CHECK( dummy.get_morale_level() < 0 );

                AND_THEN( "they still enjoy it a little" ) {
                    CHECK( dummy.has_morale( MORALE_FOOD_GOOD ) );

                    // Penalty stacks with the base fun value
                    fun = dummy.fun_for( necco );
                    CHECK( dummy.get_morale_level() <= -25 + fun.first );
                }
            }
        }
    }
}

// food with any allergy type gives morale penalty for allergy
//
TEST_CASE( "food with allergen", "[modify_morale][allergy]" )
{

}

TEST_CASE( "saprophage character", "[modify_morale][saprophage]" )
{
    avatar dummy;

    GIVEN( "character is a saprophage, preferring rotted food" ) {
        dummy.toggle_trait( trait_SAPROPHAGE );
        REQUIRE( dummy.has_trait( trait_SAPROPHAGE ) );

        AND_GIVEN( "some rotten chewable food" ) {
            item &toastem = dummy.i_add( item( "toastem" ) );
            // food rot > 1.0 is rotten
            toastem.set_relative_rot( 1.5 );
            REQUIRE( toastem.rotten() );

            THEN( "they enjoy it" ) {
                dummy.modify_morale( toastem );
                CHECK( dummy.has_morale( MORALE_FOOD_GOOD ) );
                CHECK( dummy.get_morale_level() > 10 );
            }
        }

        AND_GIVEN( "some fresh chewable food" ) {
            item &toastem = dummy.i_add( item( "toastem" ) );
            // food rot < 0.1 is fresh
            toastem.set_relative_rot( 0.0 );
            REQUIRE( toastem.is_fresh() );

            THEN( "it gives a morale penalty due to indigestion" ) {
                dummy.modify_morale( toastem );
                CHECK( dummy.has_morale( MORALE_NO_DIGEST ) );
                CHECK( dummy.get_morale_level() <= -25 );
                CHECK( dummy.get_morale_level() >= -125 );
            }
        }

    }
}

// URSINE_HONEY food gives MORALE_HONEY bonus to THRESH_URSINE characters who haven't crossed

