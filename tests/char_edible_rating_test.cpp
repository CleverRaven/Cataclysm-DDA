#include <vector>

#include "avatar.h"
#include "character.h"
#include "type_id.h"

#include "catch/catch.hpp"

// Character "edible rating" tests, covering the `can_eat` and `will_eat` functions

// can_eat: Can the food be [theoretically] eaten no matter the consequences?
// will_eat: Same, but includes consequences. Asks about them if interactive is true.


// Tests for can_eat
//
// Non-fish(?) cannot eat anything while underwater
//
// Cannot eat food made of inedible materials(?)
//
// Mycus-dependent(?) avatar cannot eat non-mycus food
//
// Avatar with proboscis cannot consume non-drinkables
//
// Carnivore cannot eat from the carnivore blacklist (?unless it gives no nutrition?)
//
// FINALLY, CHECK EVERY SINGLE MUTATION for can_only_eat incompatibilities

/*
// This tries to represent both rating and
// character's decision to respect said rating
enum edible_rating {
    // Edible or we pretend it is
    EDIBLE,
    // Not food at all
    INEDIBLE,
    // Not food because mutated mouth/system
    INEDIBLE_MUTATION,
    // You can eat it, but it will hurt morale
    ALLERGY,
    // Smaller allergy penalty
    ALLERGY_WEAK,
    // Cannibalism (unless psycho/cannibal)
    CANNIBALISM,
    // Rotten or not rotten enough (for saprophages)
    ROTTEN,
    // Can provoke vomiting if you already feel nauseous.
    NAUSEA,
    // We can eat this, but we'll overeat
    TOO_FULL,
    // Some weird stuff that requires a tool we don't have
    NO_TOOL
};
*/

TEST_CASE( "non-comestible", "[can_eat][edible_rating][nonfood]" )
{
    avatar dummy;
    GIVEN( "something not edible" ) {
        item rag( "rag" );

        THEN( "they cannot eat it" ) {
            auto rating = dummy.can_eat( rag );
            CHECK_FALSE( rating.success() );
            CHECK( rating.str() == "That doesn't look edible." );
        }
    }
}

TEST_CASE( "dirty food", "[can_eat][edible_rating][dirty]" )
{
    avatar dummy;

    GIVEN( "food that is dirty" ) {
        item chocolate( "chocolate" );
        chocolate.item_tags.insert( "DIRTY" );
        REQUIRE( chocolate.item_tags.count( "DIRTY" ) );

        THEN( "they cannot eat it" ) {
            auto rating = dummy.can_eat( chocolate );
            CHECK_FALSE( rating.success() );
            CHECK( rating.str() == "This is full of dirt after being on the ground." );
        }
    }
}

TEST_CASE( "frozen food", "[can_eat][edible_rating][frozen]" )
{
    avatar dummy;

    GIVEN( "food that is not edible when frozen" ) {
        item apple( "apple" );
        REQUIRE_FALSE( apple.has_flag( "EDIBLE_FROZEN" ) );
        REQUIRE_FALSE( apple.has_flag( "MELTS" ) );

        WHEN( "it is not frozen" ) {
            REQUIRE_FALSE( apple.item_tags.count( "FROZEN" ) );

            THEN( "they can eat it" ) {
                auto rating = dummy.can_eat( apple );
                CHECK( rating.success() );
                CHECK( rating.str() == "" );
            }
        }

        WHEN( "it is frozen" ) {
            apple.item_tags.insert( "FROZEN" );
            REQUIRE( apple.item_tags.count( "FROZEN" ) );

            THEN( "they cannot eat it" ) {
                auto rating = dummy.can_eat( apple );
                CHECK_FALSE( rating.success() );
                CHECK( rating.str() == "It's frozen solid.  You must defrost it before you can eat it." );
            }
        }
    }

    GIVEN( "drink that is not drinkable when frozen" ) {
        item water( "water_clean" );
        REQUIRE_FALSE( water.has_flag( "EDIBLE_FROZEN" ) );
        REQUIRE_FALSE( water.has_flag( "MELTS" ) );

        WHEN( "it is not frozen" ) {
            REQUIRE_FALSE( water.item_tags.count( "FROZEN" ) );

            THEN( "they can drink it" ) {
                auto rating = dummy.can_eat( water );
                CHECK( rating.success() );
                CHECK( rating.str() == "" );
            }
        }

        WHEN( "it is frozen" ) {
            water.item_tags.insert( "FROZEN" );
            REQUIRE( water.item_tags.count( "FROZEN" ) );

            THEN( "they cannot drink it" ) {
                auto rating = dummy.can_eat( water );
                CHECK_FALSE( rating.success() );
                CHECK( rating.str() == "You can't drink it while it's frozen." );
            }
        }
    }

    GIVEN( "food that is edible when frozen" ) {
        item necco( "neccowafers" );
        REQUIRE( necco.has_flag( "EDIBLE_FROZEN" ) );

        WHEN( "it is frozen" ) {
            necco.item_tags.insert( "FROZEN" );
            REQUIRE( necco.item_tags.count( "FROZEN" ) );

            THEN( "they can eat it" ) {
                auto rating = dummy.can_eat( necco );
                CHECK( rating.success() );
                CHECK( rating.str() == "" );
            }
        }
    }
}

TEST_CASE( "inedible animal food", "[can_eat][edible_rating][inedible][animal]" )
{
    avatar dummy;

    // Note: There are similar conditions for INEDIBLE food with FELINE or LUPINE flags, but
    // "birdfood" and "cattlefodder" are the only INEDIBLE items that exist in the game.

    GIVEN( "food for animals" ) {
        item birdfood( "birdfood" );
        item cattlefodder( "cattlefodder" );

        REQUIRE( birdfood.has_flag( "INEDIBLE" ) );
        REQUIRE( cattlefodder.has_flag( "INEDIBLE" ) );

        REQUIRE( birdfood.has_flag( "BIRD" ) );
        REQUIRE( cattlefodder.has_flag( "CATTLE" ) );

        WHEN( "character is not bird or cattle" ) {
            REQUIRE_FALSE( dummy.has_trait( trait_id( "THRESH_BIRD" ) ) );
            REQUIRE_FALSE( dummy.has_trait( trait_id( "THRESH_CATTLE" ) ) );

            THEN( "they cannot eat bird food" ) {
                auto rating = dummy.can_eat( birdfood );
                CHECK_FALSE( rating.success() );
                CHECK( rating.str() == "That doesn't look edible to you." );
            }

            THEN( "they cannot eat cattle fodder" ) {
                auto rating = dummy.can_eat( cattlefodder );
                CHECK_FALSE( rating.success() );
                CHECK( rating.str() == "That doesn't look edible to you." );
            }
        }

        WHEN( "character is a bird" ) {
            dummy.toggle_trait( trait_id( "THRESH_BIRD" ) );
            REQUIRE( dummy.has_trait( trait_id( "THRESH_BIRD" ) ) );

            THEN( "they can eat bird food" ) {
                auto rating = dummy.can_eat( birdfood );
                CHECK( rating.success() );
                CHECK( rating.str() == "" );
            }
        }

        WHEN( "character is cattle" ) {
            dummy.toggle_trait( trait_id( "THRESH_CATTLE" ) );
            REQUIRE( dummy.has_trait( trait_id( "THRESH_CATTLE" ) ) );

            THEN( "they can eat cattle fodder" ) {
                auto rating = dummy.can_eat( cattlefodder );
                CHECK( rating.success() );
                CHECK( rating.str() == "" );
            }
        }
    }
}

TEST_CASE( "food not ok for herbivores", "[can_eat][edible_rating][herbivore]" )
{
    avatar dummy;

    item meat( "meat_cooked" );
    item eggs( "scrambled_eggs" );
    REQUIRE( meat.has_flag( "ALLERGEN_MEAT" ) );
    REQUIRE( eggs.has_flag( "ALLERGEN_EGG" ) );

    GIVEN( "character is an herbivore" ) {
        dummy.toggle_trait( trait_id( "HERBIVORE" ) );
        REQUIRE( dummy.has_trait( trait_id( "HERBIVORE" ) ) );
        THEN( "they cannot eat meat" ) {
            auto rating = dummy.can_eat( meat );
            CHECK_FALSE( rating.success() );
            CHECK( rating.str() == "The thought of eating that makes you feel sick." );
        }

        THEN( "they cannot eat eggs" ) {
            auto rating = dummy.can_eat( eggs );
            CHECK_FALSE( rating.success() );
            CHECK( rating.str() == "The thought of eating that makes you feel sick." );
        }
    }
}

TEST_CASE( "comestible requiring tool to use", "[can_eat][edible_rating][tool][!mayfail]" )
{
    avatar dummy;

    GIVEN( "substance requiring a tool to consume" ) {
        item heroin( "heroin" );
        item syringe( "syringe" );

        WHEN( "they don't have the necessary tool" ) {
            //REQUIRE_FALSE( dummy.has_item( syringe, 1 ) );

            // FIXME: This is failing - heroin is can_eat without a tool!?
            THEN( "they cannot consume the substance" ) {
                auto rating = dummy.can_eat( heroin );
                CHECK_FALSE( rating.success() );
                CHECK( rating.str() == "You need a syringe to consume that!" );
            }
        }
    }
}

TEST_CASE( "crafted food", "[can_eat][edible_rating][craft]" )
{
    avatar dummy;
    std::vector<item> parts;

    parts.emplace_back( "water" );
    parts.emplace_back( "water_purifier" );

    recipe_id clean_water( "water_clean_using_water_purifier" );

    GIVEN( "food that is crafted" ) {
        WHEN( "crafting is not finished" ) {
            THEN( "they cannot eat it" ) {
            }

            AND_WHEN( "crafting is finished" ) {
                THEN( "they can eat it" ) {
                }
            }
        }
    }
}

