#include <vector>

#include "avatar.h"
#include "character.h"
#include "itype.h"
#include "type_id.h"

#include "catch/catch.hpp"

// Character "edible rating" tests, covering the `can_eat` and `will_eat` functions

// can_eat: Can the food be [theoretically] eaten no matter the consequences?
// will_eat: Same, but includes consequences. Asks about them if interactive is true.

// Tests for can_eat
//
// Cannot eat food made of inedible materials(?)
//
// Check every mutation for can_only_eat incompatibilities

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

// NOTE: Lines 558-564 of src/game_inventory.cpp also dealing with can_eat - refactor?
//   "Can't drink spilt liquids"
//   "Your biology is not compatible with that item")

static void expect_can_eat( avatar &dummy, item &food )
{
    auto rate_can = dummy.can_eat( food );
    CHECK( rate_can.success() );
    CHECK( rate_can.str() == "" );
    CHECK( rate_can.value() == EDIBLE );
}

static void expect_cannot_eat( avatar &dummy, item &food, std::string expect_reason,
                               int expect_rating = INEDIBLE )
{
    auto rate_can = dummy.can_eat( food );
    CHECK_FALSE( rate_can.success() );
    CHECK( rate_can.str() == expect_reason );
    CHECK( rate_can.value() == expect_rating );
}

static void expect_will_eat( avatar &dummy, item &food, std::string expect_consequence,
                             int expect_rating )
{
    // will_eat returns the first element in a vector of ret_val<edible_rating>
    // this function only looks at the first
    auto rate_will = dummy.will_eat( food );
    CHECK( rate_will.str() == expect_consequence );
    CHECK( rate_will.value() == expect_rating );
}


TEST_CASE( "non-comestible", "[can_eat][will_eat][edible_rating][nonfood]" )
{
    avatar dummy;
    GIVEN( "something not edible" ) {
        item rag( "rag" );

        THEN( "they cannot eat it" ) {
            expect_cannot_eat( dummy, rag, "That doesn't look edible.", INEDIBLE );
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
            expect_cannot_eat( dummy, chocolate, "This is full of dirt after being on the ground." );
        }
    }
}

TEST_CASE( "eating while underwater", "[can_eat][edible_rating][underwater]" )
{
    avatar dummy;
    item sushi( "sushi_fishroll" );
    item water( "water_clean" );

    GIVEN( "character is underwater" ) {
        dummy.set_underwater( true );
        REQUIRE( dummy.is_underwater() );

        WHEN( "they have no special mutation" ) {
            REQUIRE_FALSE( dummy.has_trait( trait_id( "WATERSLEEP" ) ) );

            THEN( "they cannot eat" ) {
                expect_cannot_eat( dummy, sushi, "You can't do that while underwater." );
            }

            THEN( "they cannot drink" ) {
                expect_cannot_eat( dummy, water, "You can't do that while underwater." );
            }
        }

        WHEN( "they have the Aqueous Repose mutation" ) {
            dummy.toggle_trait( trait_id( "WATERSLEEP" ) );
            REQUIRE( dummy.has_trait( trait_id( "WATERSLEEP" ) ) );

            THEN( "they can eat" ) {
                expect_can_eat( dummy, sushi );
            }

            // FIXME: This fails, despite what it says in the mutation description
            THEN( "they cannot drink" ) {
                expect_cannot_eat( dummy, water, "You can't do that while underwater." );
            }
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
                expect_can_eat( dummy, apple );
            }
        }

        WHEN( "it is frozen" ) {
            apple.item_tags.insert( "FROZEN" );
            REQUIRE( apple.item_tags.count( "FROZEN" ) );

            THEN( "they cannot eat it" ) {
                expect_cannot_eat( dummy, apple, "It's frozen solid.  You must defrost it before you can eat it." );
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
                expect_can_eat( dummy, water );
            }
        }

        WHEN( "it is frozen" ) {
            water.item_tags.insert( "FROZEN" );
            REQUIRE( water.item_tags.count( "FROZEN" ) );

            THEN( "they cannot drink it" ) {
                expect_cannot_eat( dummy, water, "You can't drink it while it's frozen." );
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
                expect_can_eat( dummy, necco );
            }
        }
    }

    GIVEN( "food that melts" ) {
        item milkshake( "milkshake" );

        // When food does not have EDIBLE_FROZEN, it will still be edible
        // frozen if it MELTS. Ice cream, milkshakes and such do not have
        // EDIBLE_FROZEN, but they have MELTS, which has the same effect.
        REQUIRE( milkshake.has_flag( "MELTS" ) );

        WHEN( "it is frozen" ) {
            milkshake.item_tags.insert( "FROZEN" );
            REQUIRE( milkshake.item_tags.count( "FROZEN" ) );

            THEN( "they can eat it" ) {
                expect_can_eat( dummy, milkshake );
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

            std::string expect_reason = "That doesn't look edible to you.";

            THEN( "they cannot eat bird food" ) {
                expect_cannot_eat( dummy, birdfood, "That doesn't look edible to you." );
            }

            THEN( "they cannot eat cattle fodder" ) {
                expect_cannot_eat( dummy, cattlefodder, "That doesn't look edible to you." );
            }
        }

        WHEN( "character is a bird" ) {
            dummy.toggle_trait( trait_id( "THRESH_BIRD" ) );
            REQUIRE( dummy.has_trait( trait_id( "THRESH_BIRD" ) ) );

            THEN( "they can eat bird food" ) {
                expect_can_eat( dummy, birdfood );
            }
        }

        WHEN( "character is cattle" ) {
            dummy.toggle_trait( trait_id( "THRESH_CATTLE" ) );
            REQUIRE( dummy.has_trait( trait_id( "THRESH_CATTLE" ) ) );

            THEN( "they can eat cattle fodder" ) {
                expect_can_eat( dummy, cattlefodder );
            }
        }
    }
}

TEST_CASE( "herbivore mutation", "[can_eat][edible_rating][herbivore]" )
{
    avatar dummy;

    GIVEN( "character is an herbivore" ) {
        dummy.toggle_trait( trait_id( "HERBIVORE" ) );
        REQUIRE( dummy.has_trait( trait_id( "HERBIVORE" ) ) );

        std::string expect_reason = "The thought of eating that makes you feel sick.";

        THEN( "they cannot eat meat" ) {
            item meat( "meat_cooked" );
            REQUIRE( meat.has_flag( "ALLERGEN_MEAT" ) );

            expect_cannot_eat( dummy, meat, expect_reason, INEDIBLE_MUTATION );
        }

        THEN( "they cannot eat eggs" ) {
            item eggs( "scrambled_eggs" );
            REQUIRE( eggs.has_flag( "ALLERGEN_EGG" ) );

            expect_cannot_eat( dummy, eggs, expect_reason, INEDIBLE_MUTATION );
        }
    }
}

TEST_CASE( "carnivore mutation", "[can_eat][edible_rating][carnivore]" )
{
    avatar dummy;

    GIVEN( "character is a carnivore" ) {
        dummy.toggle_trait( trait_id( "CARNIVORE" ) );
        REQUIRE( dummy.has_trait( trait_id( "CARNIVORE" ) ) );

        std::string expect_reason = "Eww.  Inedible plant stuff!";

        THEN( "they cannot eat veggies" ) {
            item veggy( "veggy" );
            REQUIRE( veggy.has_flag( "ALLERGEN_VEGGY" ) );

            expect_cannot_eat( dummy, veggy, expect_reason, INEDIBLE_MUTATION );
        }

        THEN( "they cannot eat fruit" ) {
            item apple( "apple" );
            REQUIRE( apple.has_flag( "ALLERGEN_FRUIT" ) );

            expect_cannot_eat( dummy, apple, expect_reason, INEDIBLE_MUTATION );
        }

        THEN( "they cannot eat wheat" ) {
            item bread( "sourdough_bread" );
            REQUIRE( bread.has_flag( "ALLERGEN_WHEAT" ) );

            expect_cannot_eat( dummy, bread, expect_reason, INEDIBLE_MUTATION );
        }

        THEN( "they cannot eat nuts" ) {
            item nuts( "pine_nuts" );
            REQUIRE( nuts.has_flag( "ALLERGEN_NUT" ) );

            expect_cannot_eat( dummy, nuts, expect_reason, INEDIBLE_MUTATION );
        }

        THEN( "they can eat junk food, but are allergic to it" ) {
            item chocolate( "chocolate" );
            REQUIRE( chocolate.has_flag( "ALLERGEN_JUNK" ) );

            expect_can_eat( dummy, chocolate );
            expect_will_eat( dummy, chocolate, "Your stomach won't be happy (allergy).", ALLERGY );
        }
    }
}

TEST_CASE( "mycus dependency mutation", "[can_eat][edible_rating][mycus]" )
{
    avatar dummy;

    GIVEN( "character is mycus-dependent" ) {
        dummy.toggle_trait( trait_id( "M_DEPENDENT" ) );
        REQUIRE( dummy.has_trait( trait_id( "M_DEPENDENT" ) ) );

        THEN( "they cannot eat normal food" ) {
            item nuts( "pine_nuts" );
            REQUIRE_FALSE( nuts.has_flag( "MYCUS_OK" ) );

            expect_cannot_eat( dummy, nuts, "We can't eat that.  It's not right for us.", INEDIBLE_MUTATION );
        }

        THEN( "they can eat mycus food" ) {
            item berry( "marloss_berry" );
            REQUIRE( berry.has_flag( "MYCUS_OK" ) );

            expect_can_eat( dummy, berry );
        }
    }
}

TEST_CASE( "proboscis mutation", "[can_eat][edible_rating][proboscis]" )
{
    avatar dummy;

    GIVEN( "character has a proboscis" ) {
        dummy.toggle_trait( trait_id( "PROBOSCIS" ) );
        REQUIRE( dummy.has_trait( trait_id( "PROBOSCIS" ) ) );

        // Cannot drink
        std::string expect_reason = "Ugh, you can't drink that!";

        GIVEN( "a drink that is 'eaten' (USE_EAT_VERB)" ) {
            item soup( "soup_veggy" );
            REQUIRE( soup.has_flag( "USE_EAT_VERB" ) );

            THEN( "they cannot drink it" ) {
                expect_cannot_eat( dummy, soup, expect_reason, INEDIBLE_MUTATION );
            }
        }

        GIVEN( "food that must be chewed" ) {
            item toastem( "toastem" );
            REQUIRE( toastem.get_comestible()->comesttype == "FOOD" );

            THEN( "they cannot drink it" ) {
                expect_cannot_eat( dummy, toastem, expect_reason, INEDIBLE_MUTATION );
            }
        }

        // Can drink

        GIVEN( "a drink that is not 'eaten'" ) {
            item broth( "broth" );
            REQUIRE_FALSE( broth.has_flag( "USE_EAT_VERB" ) );

            THEN( "they can drink it" ) {
                expect_can_eat( dummy, broth );
            }
        }

        GIVEN( "some medicine" ) {
            item aspirin( "aspirin" );
            REQUIRE( aspirin.is_medication() );

            THEN( "they can consume it" ) {
                expect_can_eat( dummy, aspirin );
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

// will_eat test cases
//
// Consequences:
// "This is rotten and smells awful!"
// "The thought of eating human flesh makes you feel sick."
// "You still feel nauseous and will probably puke it all up again."
// "Your stomach won't be happy (allergy)."
// "Your stomach won't be happy (not rotten enough)."
// "You're full already and will be forcing yourself to eat."
// "You're full already and will be forcing yourself to drink."

TEST_CASE( "rotten food", "[will_eat][edible_rating][rotten]" )
{
    avatar dummy;

    GIVEN( "food just barely rotten" ) {
        item toastem_rotten = item( "toastem" );
        toastem_rotten.set_relative_rot( 1.01 );
        REQUIRE( toastem_rotten.rotten() );

        WHEN( "character is normal" ) {
            REQUIRE_FALSE( dummy.has_trait( trait_id( "SAPROPHAGE" ) ) );
            REQUIRE_FALSE( dummy.has_trait( trait_id( "SAPROVORE" ) ) );

            THEN( "they can eat it, though they are disgusted by it" ) {
                expect_can_eat( dummy, toastem_rotten );

                auto conseq = dummy.will_eat( toastem_rotten, false );
                CHECK( conseq.value() == ROTTEN );
                CHECK( conseq.str() == "This is rotten and smells awful!" );
            }
        }

        WHEN( "character is a saprovore" ) {
            dummy.toggle_trait( trait_id( "SAPROVORE" ) );
            REQUIRE( dummy.has_trait( trait_id( "SAPROVORE" ) ) );

            THEN( "they can eat it, and don't mind that it is rotten" ) {
                expect_can_eat( dummy, toastem_rotten );

                auto conseq = dummy.will_eat( toastem_rotten, false );
                CHECK( conseq.value() == EDIBLE );
                CHECK( conseq.str() == "" );
            }
        }

        WHEN( "character is a saprophage" ) {
            dummy.toggle_trait( trait_id( "SAPROPHAGE" ) );
            REQUIRE( dummy.has_trait( trait_id( "SAPROPHAGE" ) ) );

            THEN( "they can eat it, but would prefer it to be more rotten" ) {
                expect_can_eat( dummy, toastem_rotten );

                auto conseq = dummy.will_eat( toastem_rotten, false );
                CHECK( conseq.value() == ALLERGY_WEAK );
                CHECK( conseq.str() == "Your stomach won't be happy (not rotten enough)." );
            }

            /* NOT TRUE
             *
            AND_WHEN( "the food is thoroughly rotten" ) {
                toastem_rotten.set_relative_rot( 2.01 );
                REQUIRE( toastem_rotten.has_rotten_away() );

                THEN( "they can eat it without any qualms" ) {
                    expect_can_eat( dummy, toastem_rotten );

                    auto conseq = dummy.will_eat( toastem_rotten, false );
                    CHECK( conseq.value() == EDIBLE );
                    CHECK( conseq.str() == "" );
                }
            }
            */
        }
    }
}

TEST_CASE( "", "[will_eat][edible_rating]" )
{
}

TEST_CASE( "", "[will_eat][edible_rating]" )
{
}

