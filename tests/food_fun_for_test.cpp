#include "catch/catch.hpp"

#include <cstdlib>
#include <memory>
#include <string>
#include <utility>

#include "avatar.h"
#include "calendar.h"
#include "item.h"
#include "itype.h"
#include "player_helpers.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"

static const bionic_id bio_taste_blocker( "bio_taste_blocker" );

static const std::string flag_COLD( "COLD" );
static const std::string flag_EATEN_COLD( "EATEN_COLD" );
static const std::string flag_FELINE( "FELINE" );
static const std::string flag_FROZEN( "FROZEN" );
static const std::string flag_LUPINE( "LUPINE" );
static const std::string flag_MELTS( "MELTS" );
static const trait_id trait_THRESH_FELINE( "THRESH_FELINE" );
static const trait_id trait_THRESH_LUPINE( "THRESH_LUPINE" );

// Test cases for `Character::fun_for` defined in `src/consumption.cpp`

TEST_CASE( "fun for non-food", "[fun_for][nonfood]" )
{
    avatar dummy;
    std::pair<int, int> actual_fun;

    SECTION( "non-food has no fun value" ) {
        item rag( "rag" );
        REQUIRE_FALSE( rag.is_comestible() );

        actual_fun = dummy.fun_for( rag );
        CHECK( actual_fun.first == 0 );
        CHECK( actual_fun.second == 0 );
    }
}

TEST_CASE( "fun for food eaten while sick", "[fun_for][food][sick]" )
{
    avatar dummy;
    std::pair<int, int> actual_fun;
    item toastem( "toastem" );
    REQUIRE( toastem.is_comestible() );
    // Base fun value for toast-em
    int toastem_fun = toastem.get_comestible_fun();

    WHEN( "character has a cold" ) {
        dummy.add_effect( efftype_id( "common_cold" ), 1_days );
        REQUIRE( dummy.has_effect( efftype_id( "common_cold" ) ) );

        THEN( "it is much less fun to eat" ) {
            actual_fun = dummy.fun_for( toastem );
            CHECK( actual_fun.first < toastem_fun / 2 );
        }
    }

    WHEN( "character has the flu" ) {
        dummy.add_effect( efftype_id( "flu" ), 1_days );
        REQUIRE( dummy.has_effect( efftype_id( "flu" ) ) );

        THEN( "it is much less fun to eat" ) {
            actual_fun = dummy.fun_for( toastem );
            CHECK( actual_fun.first < toastem_fun / 2 );
        }
    }
}

TEST_CASE( "fun for rotten food", "[fun_for][food][rotten]" )
{
    avatar dummy;
    dummy.set_body();
    std::pair<int, int> actual_fun;

    GIVEN( "some rotten food" ) {
        item nuts( "pine_nuts" );
        REQUIRE( nuts.is_comestible() );
        // food rot > 1.0 is rotten
        nuts.set_relative_rot( 1.5 );
        REQUIRE( nuts.rotten() );

        WHEN( "character has normal tastes" ) {
            REQUIRE_FALSE( dummy.has_trait( trait_id( "SAPROPHAGE" ) ) );
            REQUIRE_FALSE( dummy.has_trait( trait_id( "SAPROVORE" ) ) );

            THEN( "they don't like rotten food" ) {
                actual_fun = dummy.fun_for( nuts );
                CHECK( actual_fun.first < 0 );
            }
        }

        WHEN( "character is a saprophage" ) {
            dummy.toggle_trait( trait_id( "SAPROPHAGE" ) );
            REQUIRE( dummy.has_trait( trait_id( "SAPROPHAGE" ) ) );

            THEN( "they like rotten food" ) {
                actual_fun = dummy.fun_for( nuts );
                CHECK( actual_fun.first > 0 );
            }
        }
        WHEN( "character is a saprovore" ) {
            dummy.toggle_trait( trait_id( "SAPROVORE" ) );
            REQUIRE( dummy.has_trait( trait_id( "SAPROVORE" ) ) );

            THEN( "they like rotten food" ) {
                actual_fun = dummy.fun_for( nuts );
                CHECK( actual_fun.first > 0 );
            }
        }
    }
}

// N.B. food that tastes better hot is `modify_morale` with different math
TEST_CASE( "fun for cold food", "[fun_for][food][cold]" )
{
    avatar dummy;
    std::pair<int, int> actual_fun;

    GIVEN( "food that tastes good, but better when cold" ) {
        item cola( "cola" );
        REQUIRE( cola.is_comestible() );
        REQUIRE( cola.has_flag( flag_EATEN_COLD ) );
        int cola_fun = cola.get_comestible_fun();

        WHEN( "it is cold" ) {
            cola.set_flag( flag_COLD );
            REQUIRE( cola.has_flag( flag_COLD ) );

            THEN( "it is more fun than normal" ) {
                actual_fun = dummy.fun_for( cola );
                CHECK( actual_fun.first == cola_fun * 2 );
            }
        }

        WHEN( "it is not cold" ) {
            REQUIRE_FALSE( cola.has_flag( flag_COLD ) );
            THEN( "it has normal fun" ) {
                actual_fun = dummy.fun_for( cola );
                CHECK( actual_fun.first == cola_fun );
            }
        }
    }

    GIVEN( "food that tastes bad, but better when cold" ) {
        item sports( "sports_drink" );
        REQUIRE( sports.is_comestible() );
        int sports_fun = sports.get_comestible_fun();

        REQUIRE( sports_fun < 0 );
        REQUIRE( sports.has_flag( flag_EATEN_COLD ) );

        WHEN( "it is cold" ) {
            sports.set_flag( flag_COLD );
            REQUIRE( sports.has_flag( flag_COLD ) );

            THEN( "it doesn't taste bad" ) {
                actual_fun = dummy.fun_for( sports );
                CHECK( actual_fun.first > 0 );
            }
        }

        WHEN( "it is not cold" ) {
            REQUIRE_FALSE( sports.has_flag( flag_COLD ) );

            THEN( "it tastes as bad as usual" ) {
                actual_fun = dummy.fun_for( sports );
                CHECK( actual_fun.first == sports_fun );
            }
        }
    }

    GIVEN( "food that tastes good, but no better when cold" ) {
        item coffee( "coffee" );
        REQUIRE( coffee.is_comestible() );
        int coffee_fun = coffee.get_comestible_fun();

        REQUIRE( coffee_fun > 0 );
        REQUIRE_FALSE( coffee.has_flag( flag_EATEN_COLD ) );

        WHEN( "it is cold" ) {
            coffee.set_flag( flag_COLD );
            REQUIRE( coffee.has_flag( flag_COLD ) );

            THEN( "it tastes just as good as usual" ) {
                actual_fun = dummy.fun_for( coffee );
                CHECK( actual_fun.first == coffee_fun );
            }
        }

        WHEN( "it is not cold" ) {
            REQUIRE_FALSE( coffee.has_flag( flag_COLD ) );

            THEN( "it tastes just as good as usual" ) {
                actual_fun = dummy.fun_for( coffee );
                CHECK( actual_fun.first == coffee_fun );
            }
        }

        // Note: One might expect a "HOT" test to go here, since
        // coffee is actually EATEN_HOT, but that is calculated in
        // Character::modify_morale, not Character::food_fun
    }
}

TEST_CASE( "fun for melted food", "[fun_for][food][melted]" )
{
    avatar dummy;
    std::pair<int, int> actual_fun;

    GIVEN( "food that is fun but melts" ) {
        item icecream( "icecream" );
        REQUIRE( icecream.is_comestible() );
        REQUIRE( icecream.has_flag( flag_MELTS ) );
        int icecream_fun = icecream.get_comestible_fun();

        WHEN( "it is not frozen" ) {
            REQUIRE_FALSE( icecream.has_flag( flag_FROZEN ) );

            THEN( "it is half as fun" ) {
                actual_fun = dummy.fun_for( icecream );
                CHECK( actual_fun.first == icecream_fun / 2 );
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

TEST_CASE( "fun for cat food", "[fun_for][food][cat][feline]" )
{
    avatar dummy;
    dummy.set_body();
    std::pair<int, int> actual_fun;

    GIVEN( "cat food" ) {
        item catfood( "catfood" );
        REQUIRE( catfood.is_comestible() );
        REQUIRE( catfood.has_flag( flag_FELINE ) );

        WHEN( "character is not feline" ) {
            REQUIRE_FALSE( dummy.has_trait( trait_THRESH_FELINE ) );
            THEN( "they dislike cat food" ) {
                actual_fun = dummy.fun_for( catfood );
                CHECK( actual_fun.first < 0 );
            }
        }

        WHEN( "character is feline" ) {
            dummy.toggle_trait( trait_THRESH_FELINE );
            REQUIRE( dummy.has_trait( trait_THRESH_FELINE ) );

            THEN( "they like cat food" ) {
                actual_fun = dummy.fun_for( catfood );
                CHECK( actual_fun.first > 0 );
            }
        }
    }
}

TEST_CASE( "fun for dog food", "[fun_for][food][dog][lupine]" )
{
    avatar dummy;
    dummy.set_body();
    std::pair<int, int> actual_fun;

    GIVEN( "dog food" ) {
        item dogfood( "dogfood" );
        REQUIRE( dogfood.is_comestible() );
        REQUIRE( dogfood.has_flag( flag_LUPINE ) );

        WHEN( "character is not lupine" ) {
            REQUIRE_FALSE( dummy.has_trait( trait_THRESH_LUPINE ) );

            THEN( "they dislike dog food" ) {
                actual_fun = dummy.fun_for( dogfood );
                CHECK( actual_fun.first < 0 );
            }
        }

        WHEN( "character is lupine" ) {
            dummy.toggle_trait( trait_THRESH_LUPINE );
            REQUIRE( dummy.has_trait( trait_THRESH_LUPINE ) );

            THEN( "they like dog food" ) {
                actual_fun = dummy.fun_for( dogfood );
                CHECK( actual_fun.first > 0 );
            }
        }
    }
}

TEST_CASE( "fun for gourmand", "[fun_for][food][gourmand]" )
{
    avatar dummy;
    dummy.set_body();
    std::pair<int, int> actual_fun;

    GIVEN( "food that tastes good" ) {
        item toastem( "toastem" );
        REQUIRE( toastem.is_comestible() );
        int toastem_fun = toastem.get_comestible_fun();
        REQUIRE( toastem_fun > 0 );

        WHEN( "character is not a gourmand" ) {
            REQUIRE_FALSE( dummy.has_trait( trait_id( "GOURMAND" ) ) );

            THEN( "it tastes just as good as normal" ) {
                actual_fun = dummy.fun_for( toastem );
                CHECK( actual_fun.first == toastem_fun );
            }
        }
        WHEN( "character is a gourmand" ) {
            dummy.toggle_trait( trait_id( "GOURMAND" ) );
            REQUIRE( dummy.has_trait( trait_id( "GOURMAND" ) ) );

            THEN( "it tastes better than normal" ) {
                actual_fun = dummy.fun_for( toastem );
                CHECK( actual_fun.first > toastem_fun );
            }
        }
    }

    GIVEN( "food that tastes bad" ) {
        item garlic( "garlic" );
        REQUIRE( garlic.is_comestible() );
        int garlic_fun = garlic.get_comestible_fun();
        // At fun == -1, Gourmand trait has no effect
        REQUIRE( garlic_fun < -1 );

        WHEN( "character is not a gourmand" ) {
            REQUIRE_FALSE( dummy.has_trait( trait_id( "GOURMAND" ) ) );

            THEN( "it tastes just as bad as normal" ) {
                actual_fun = dummy.fun_for( garlic );
                CHECK( actual_fun.first == garlic_fun );
            }
        }

        WHEN( "character is a gourmand" ) {
            dummy.toggle_trait( trait_id( "GOURMAND" ) );
            REQUIRE( dummy.has_trait( trait_id( "GOURMAND" ) ) );

            THEN( "it still tastes bad, but not as bad as normal" ) {
                actual_fun = dummy.fun_for( garlic );
                CHECK( actual_fun.first > garlic_fun );
            }
        }
    }
}

TEST_CASE( "fun for food eaten too often", "[fun_for][food][monotony]" )
{
    avatar dummy;
    std::pair<int, int> actual_fun;

    // A big box of tasty toast-ems
    item toastem( "toastem", calendar::turn, 10 );
    REQUIRE( toastem.is_comestible() );

    // Base fun value and monotony penalty for toast-em
    int toastem_fun = toastem.get_comestible_fun();
    int toastem_penalty = toastem.get_comestible()->monotony_penalty;

    // Will do 2 rounds of penalty testing, so base fun needs to be at least 2x that
    REQUIRE( toastem_fun > 2 * toastem_penalty );

    GIVEN( "food that is fun to eat" ) {
        WHEN( "character has not had any recently" ) {

            THEN( "it tastes as good as it should" ) {
                actual_fun = dummy.fun_for( toastem );
                CHECK( actual_fun.first == toastem_fun );
            }
        }

        WHEN( "character has just eaten one" ) {
            dummy.consume( toastem );

            THEN( "the next one is less enjoyable" ) {
                actual_fun = dummy.fun_for( toastem );
                CHECK( actual_fun.first == toastem_fun - toastem_penalty );
            }

            AND_WHEN( "character has eaten another one" ) {
                dummy.consume( toastem );

                THEN( "the one after that is even less enjoyable" ) {
                    actual_fun = dummy.fun_for( toastem );
                    CHECK( actual_fun.first == toastem_fun - 2 * toastem_penalty );
                }
            }
        }
    }
}

TEST_CASE( "fun for bionic bio taste blocker", "[fun_for][food][bionic]" )
{
    avatar dummy;
    dummy.set_body();
    std::pair<int, int> actual_fun;

    GIVEN( "food that tastes bad" ) {
        item garlic( "garlic" );
        REQUIRE( garlic.is_comestible() );
        int garlic_fun = garlic.get_comestible_fun();
        REQUIRE( garlic_fun < 0 );

        AND_GIVEN( "character has a taste modifier CBM" ) {
            dummy.set_max_power_level( 1000_kJ );
            give_and_activate_bionic( dummy, bionic_id( "bio_taste_blocker" ) );
            REQUIRE( dummy.has_active_bionic( bio_taste_blocker ) );

            WHEN( "it does not have enough power" ) {
                // Needs 1 kJ per negative fun unit to nullify bad taste
                dummy.set_power_level( 10_kJ );
                REQUIRE( garlic_fun < -10 );
                REQUIRE_FALSE( dummy.get_power_level() > units::from_kilojoule( std::abs( garlic_fun ) ) );

                THEN( "the bad taste remains" ) {
                    actual_fun = dummy.fun_for( garlic );
                    CHECK( actual_fun.first == garlic_fun );
                }
            }

            WHEN( "it has enough power" ) {
                REQUIRE( garlic_fun >= -20 );
                dummy.set_power_level( 20_kJ );
                REQUIRE( dummy.get_power_level() > units::from_kilojoule( std::abs( garlic_fun ) ) );

                THEN( "the bad taste is nullified" ) {
                    actual_fun = dummy.fun_for( garlic );
                    CHECK( actual_fun.first == 0 );
                }
            }
        }
    }
}

