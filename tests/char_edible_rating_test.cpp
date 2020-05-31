#include <memory>
#include <string>

#include "avatar.h"
#include "calendar.h"
#include "catch/catch.hpp"
#include "character.h"
#include "flat_set.h"
#include "item.h"
#include "itype.h"
#include "ret_val.h"
#include "type_id.h"
#include "value_ptr.h"

// Character "edible rating" tests, covering the `can_eat` and `will_eat` functions

static void expect_can_eat( avatar &dummy, item &food )
{
    auto rate_can = dummy.can_eat( food );
    CHECK( rate_can.success() );
    CHECK( rate_can.str().empty() );
    CHECK( rate_can.value() == EDIBLE );
}

static void expect_cannot_eat( avatar &dummy, item &food, const std::string &expect_reason,
                               int expect_rating = INEDIBLE )
{
    auto rate_can = dummy.can_eat( food );
    CHECK_FALSE( rate_can.success() );
    CHECK( rate_can.str() == expect_reason );
    CHECK( rate_can.value() == expect_rating );
}

static void expect_will_eat( avatar &dummy, item &food, const std::string &expect_consequence,
                             int expect_rating )
{
    // will_eat returns the first element in a vector of ret_val<edible_rating>
    // this function only looks at the first (since each is tested separately)
    auto rate_will = dummy.will_eat( food );
    CHECK( rate_will.str() == expect_consequence );
    CHECK( rate_will.value() == expect_rating );
}

TEST_CASE( "cannot eat non-comestible", "[can_eat][will_eat][edible_rating][nonfood]" )
{
    avatar dummy;
    GIVEN( "something not edible" ) {
        item rag( "rag" );

        THEN( "they cannot eat it" ) {
            expect_cannot_eat( dummy, rag, "That doesn't look edible.", INEDIBLE );
        }
    }
}

TEST_CASE( "cannot eat dirty food", "[can_eat][edible_rating][dirty]" )
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

TEST_CASE( "who can eat while underwater", "[can_eat][edible_rating][underwater]" )
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

            /*
            // FIXME: This fails, despite what it says in the mutation description
            THEN( "they cannot drink" ) {
                expect_cannot_eat( dummy, water, "You can't do that while underwater." );
            }
            */
        }
    }
}

TEST_CASE( "when frozen food can be eaten", "[can_eat][edible_rating][frozen]" )
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

TEST_CASE( "who can eat inedible animal food", "[can_eat][edible_rating][inedible][animal]" )
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

TEST_CASE( "what herbivores can eat", "[can_eat][edible_rating][herbivore]" )
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

TEST_CASE( "what carnivores can eat", "[can_eat][edible_rating][carnivore]" )
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

TEST_CASE( "what you can eat with a mycus dependency", "[can_eat][edible_rating][mycus]" )
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

TEST_CASE( "what you can drink with a proboscis", "[can_eat][edible_rating][proboscis]" )
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

TEST_CASE( "can eat with nausea", "[will_eat][edible_rating][nausea]" )
{
    avatar dummy;
    item toastem( "toastem" );
    const efftype_id effect_nausea( "nausea" );

    GIVEN( "character has nausea" ) {
        dummy.add_effect( effect_nausea, 10_minutes );
        REQUIRE( dummy.has_effect( effect_nausea ) );

        THEN( "they can eat food, but it nauseates them" ) {
            expect_can_eat( dummy, toastem );
            expect_will_eat( dummy, toastem, "You still feel nauseous and will probably puke it all up again.",
                             NAUSEA );
        }
    }
}

TEST_CASE( "can eat with allergies", "[will_eat][edible_rating][allergy]" )
{
    avatar dummy;
    item fruit( "apple" );
    REQUIRE( fruit.has_flag( "ALLERGEN_FRUIT" ) );

    GIVEN( "character hates fruit" ) {
        dummy.toggle_trait( trait_id( "ANTIFRUIT" ) );
        REQUIRE( dummy.has_trait( trait_id( "ANTIFRUIT" ) ) );

        THEN( "they can eat fruit, but won't like it" ) {
            expect_can_eat( dummy, fruit );
            expect_will_eat( dummy, fruit, "Your stomach won't be happy (allergy).", ALLERGY );
        }
    }
}

TEST_CASE( "who will eat rotten food", "[will_eat][edible_rating][rotten]" )
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
                CHECK( conseq.str().empty() );
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
        }
    }
}

TEST_CASE( "who will eat cooked human flesh", "[will_eat][edible_rating][cannibal]" )
{
    avatar dummy;

    GIVEN( "some cooked human flesh" ) {
        item flesh( "human_cooked" );
        REQUIRE( flesh.has_flag( "CANNIBALISM" ) );

        WHEN( "character is not a cannibal" ) {
            REQUIRE_FALSE( dummy.has_trait( trait_id( "CANNIBAL" ) ) );

            THEN( "they can eat it, but feel sick about it" ) {
                expect_can_eat( dummy, flesh );
                expect_will_eat( dummy, flesh, "The thought of eating human flesh makes you feel sick.",
                                 CANNIBALISM );
            }
        }

        WHEN( "character is a cannibal" ) {
            dummy.toggle_trait( trait_id( "CANNIBAL" ) );
            REQUIRE( dummy.has_trait( trait_id( "CANNIBAL" ) ) );

            THEN( "they can eat it without any qualms" ) {
                expect_can_eat( dummy, flesh );
                expect_will_eat( dummy, flesh, "", EDIBLE );
            }
        }
    }
}

