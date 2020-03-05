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
TEST_CASE( "cannibal food", "[modify_morale][cannibal]" )
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

//
// food with any allergy type gives morale penalty for allergy
//
// food with ALLERGEN_JUNK flag
// - PROJUNK character gets MORALE_SWEETTOOTH bonus
// - PROJUNK2 character gets larger/longer MORALE_SWEETTOOTH bonus
//
// carnivore eating non-CARNIVORE_OK food gets MORALE_NO_DIGEST penalty
//
// non-rotten food for SAPROPHAGE character gives MORALE_NO_DIGEST penalty
//
// URSINE_HONEY food gives MORALE_HONEY bonus to THRESH_URSINE characters who haven't crossed
//

