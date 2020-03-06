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
    // and stacks with other morale modifiers in `modify_morale`

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

TEST_CASE( "food allergies and intolerances", "[modify_morale][allergy]" )
{
    avatar dummy;

    item &meat = dummy.i_add( item( "meat" ) );
    REQUIRE( meat.has_flag( "ALLERGEN_MEAT" ) );

    item &milk = dummy.i_add( item( "milk" ) );
    REQUIRE( milk.has_flag( "ALLERGEN_MILK" ) );

    item &wheat = dummy.i_add( item( "wheat" ) );
    REQUIRE( wheat.has_flag( "ALLERGEN_WHEAT" ) );

    item &veggy = dummy.i_add( item( "broccoli" ) );
    REQUIRE( veggy.has_flag( "ALLERGEN_VEGGY" ) );

    item &fruit = dummy.i_add( item( "apple" ) );
    REQUIRE( fruit.has_flag( "ALLERGEN_FRUIT" ) );

    item &junk = dummy.i_add( item( "neccowafers" ) );
    REQUIRE( junk.has_flag( "ALLERGEN_JUNK" ) );


    GIVEN( "character is vegetarian" ) {
        dummy.toggle_trait( trait_VEGETARIAN );
        REQUIRE( dummy.has_trait( trait_VEGETARIAN ) );

        THEN( "they get a morale penalty for eating meat" ) {
            dummy.clear_morale();
            dummy.modify_morale( meat );
            CHECK( dummy.has_morale( MORALE_VEGETARIAN ) );
            CHECK( dummy.get_morale_level() < -50 );
        }
    }

    GIVEN( "character is lactose intolerant" ) {
        dummy.toggle_trait( trait_LACTOSE );
        REQUIRE( dummy.has_trait( trait_LACTOSE ) );

        THEN( "they get a morale penalty for drinking milk" ) {
            dummy.clear_morale();
            dummy.modify_morale( milk );
            CHECK( dummy.has_morale( MORALE_LACTOSE ) );
            CHECK( dummy.get_morale_level() < -50 );
        }
    }

    GIVEN( "character is grain intolerant" ) {
        dummy.toggle_trait( trait_ANTIWHEAT );
        REQUIRE( dummy.has_trait( trait_ANTIWHEAT ) );

        THEN( "they get a morale penalty for eating wheat" ) {
            dummy.clear_morale();
            dummy.modify_morale( wheat );
            CHECK( dummy.has_morale( MORALE_ANTIWHEAT ) );
            CHECK( dummy.get_morale_level() < -50 );
        }
    }

    GIVEN( "character hates vegetables" ) {
        dummy.toggle_trait( trait_MEATARIAN );
        REQUIRE( dummy.has_trait( trait_MEATARIAN ) );

        THEN( "they get a morale penalty for eating vegetables" ) {
            dummy.clear_morale();
            dummy.modify_morale( veggy );
            CHECK( dummy.has_morale( MORALE_MEATARIAN ) );
            CHECK( dummy.get_morale_level() < -50 );
        }
    }

    GIVEN( "character hates fruit" ) {
        dummy.toggle_trait( trait_ANTIFRUIT );
        REQUIRE( dummy.has_trait( trait_ANTIFRUIT ) );

        THEN( "they get a morale penalty for eating fruit" ) {
            dummy.clear_morale();
            dummy.modify_morale( fruit );
            CHECK( dummy.has_morale( MORALE_ANTIFRUIT ) );
            CHECK( dummy.get_morale_level() < -50 );
        }
    }

    GIVEN( "character has a junk food intolerance" ) {
        dummy.toggle_trait( trait_ANTIJUNK );
        REQUIRE( dummy.has_trait( trait_ANTIJUNK ) );

        THEN( "they get a morale penalty for eating junk food" ) {
            dummy.clear_morale();
            dummy.modify_morale( junk );
            CHECK( dummy.has_morale( MORALE_ANTIJUNK ) );
            CHECK( dummy.get_morale_level() < -50 );
        }
    }
}

TEST_CASE( "saprophage character", "[modify_morale][saprophage]" )
{
    avatar dummy;

    GIVEN( "character is a saprophage, preferring rotted food" ) {
        dummy.clear_morale();
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

            THEN( "they get a morale penalty due to indigestion" ) {
                dummy.modify_morale( toastem );
                CHECK( dummy.has_morale( MORALE_NO_DIGEST ) );
                CHECK( dummy.get_morale_level() <= -25 );
                CHECK( dummy.get_morale_level() >= -125 );
            }
        }

    }
}

// FIXME: Also need at least 5 bear mutations
TEST_CASE( "ursine honey", "[modify_morale][ursine][honey][!mayfail]" )
{
    avatar dummy;
    std::pair<int, int> fun;

    item &honeycomb = dummy.i_add( item( "honeycomb" ) );
    REQUIRE( honeycomb.has_flag( flag_URSINE_HONEY ) );

    GIVEN( "character is post-threshold ursine" ) {
        dummy.toggle_trait( trait_THRESH_URSINE );
        REQUIRE( dummy.has_trait( trait_THRESH_URSINE ) );


        THEN( "they get a morale bonus for eating it" ) {
            dummy.modify_morale( honeycomb );

            // Should be greater than normal fun
            fun = dummy.fun_for( honeycomb );
            CHECK( dummy.get_morale_level() > fun.first );

            AND_THEN( "they enjoy it" ) {
                CHECK( dummy.has_morale( MORALE_FOOD_GOOD ) );
            }
        }
    }
}

