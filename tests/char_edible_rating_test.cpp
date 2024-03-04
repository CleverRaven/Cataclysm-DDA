#include <iosfwd>
#include <memory>
#include <string>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "flag.h"
#include "item.h"
#include "itype.h"
#include "ret_val.h"
#include "type_id.h"
#include "value_ptr.h"

// Character "edible rating" tests, covering the `can_eat` and `will_eat` functions
static const efftype_id effect_nausea( "nausea" );

static const trait_id trait_ANTIFRUIT( "ANTIFRUIT" );
static const trait_id trait_CANNIBAL( "CANNIBAL" );
static const trait_id trait_CARNIVORE( "CARNIVORE" );
static const trait_id trait_HERBIVORE( "HERBIVORE" );
static const trait_id trait_M_DEPENDENT( "M_DEPENDENT" );
static const trait_id trait_PROBOSCIS( "PROBOSCIS" );
static const trait_id trait_SAPROPHAGE( "SAPROPHAGE" );
static const trait_id trait_SAPROVORE( "SAPROVORE" );
static const trait_id trait_THRESH_BIRD( "THRESH_BIRD" );
static const trait_id trait_THRESH_CATTLE( "THRESH_CATTLE" );
static const trait_id trait_WATERSLEEP( "WATERSLEEP" );

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

TEST_CASE( "cannot_eat_non-comestible", "[can_eat][will_eat][edible_rating][nonfood]" )
{
    avatar dummy;
    GIVEN( "something not edible" ) {
        item sheet_cotton( "sheet_cotton" );

        THEN( "they cannot eat it" ) {
            expect_cannot_eat( dummy, sheet_cotton, "That doesn't look edible.", INEDIBLE );
        }
    }
}

TEST_CASE( "cannot_eat_dirty_food", "[can_eat][edible_rating][dirty]" )
{
    avatar dummy;

    GIVEN( "food that is dirty" ) {
        item chocolate( "chocolate" );
        chocolate.set_flag( flag_DIRTY );
        REQUIRE( chocolate.has_own_flag( flag_DIRTY ) );

        THEN( "they cannot eat it" ) {
            expect_cannot_eat( dummy, chocolate, "This is full of dirt after being on the ground." );
        }
    }
}

TEST_CASE( "who_can_eat_while_underwater", "[can_eat][edible_rating][underwater]" )
{
    avatar dummy;
    dummy.set_body();
    item sushi( "sushi_fishroll" );
    item water( "water_clean" );

    GIVEN( "character is underwater" ) {
        dummy.set_underwater( true );
        REQUIRE( dummy.is_underwater() );

        WHEN( "they have no special mutation" ) {
            REQUIRE_FALSE( dummy.has_trait( trait_WATERSLEEP ) );

            THEN( "they cannot eat" ) {
                expect_cannot_eat( dummy, sushi, "You can't do that while underwater." );
            }

            THEN( "they cannot drink" ) {
                expect_cannot_eat( dummy, water, "You can't do that while underwater." );
            }
        }

        WHEN( "they have the Aqueous Repose mutation" ) {
            dummy.toggle_trait( trait_WATERSLEEP );
            REQUIRE( dummy.has_trait( trait_WATERSLEEP ) );

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

TEST_CASE( "when_frozen_food_can_be_eaten", "[can_eat][edible_rating][frozen]" )
{
    avatar dummy;

    GIVEN( "food that is not edible when frozen" ) {
        item apple( "apple" );
        REQUIRE_FALSE( apple.has_flag( flag_EDIBLE_FROZEN ) );
        REQUIRE_FALSE( apple.has_flag( flag_MELTS ) );

        WHEN( "it is not frozen" ) {
            REQUIRE_FALSE( apple.has_own_flag( flag_FROZEN ) );

            THEN( "they can eat it" ) {
                expect_can_eat( dummy, apple );
            }
        }

        WHEN( "it is frozen" ) {
            apple.set_flag( flag_FROZEN );
            REQUIRE( apple.has_own_flag( flag_FROZEN ) );

            THEN( "they cannot eat it" ) {
                expect_cannot_eat( dummy, apple, "It's frozen solid.  You must defrost it before you can eat it." );
            }
        }
    }

    GIVEN( "drink that is not drinkable when frozen" ) {
        item water( "water_clean" );
        REQUIRE_FALSE( water.has_flag( flag_EDIBLE_FROZEN ) );
        REQUIRE_FALSE( water.has_flag( flag_MELTS ) );

        WHEN( "it is not frozen" ) {
            REQUIRE_FALSE( water.has_own_flag( flag_FROZEN ) );

            THEN( "they can drink it" ) {
                expect_can_eat( dummy, water );
            }
        }

        WHEN( "it is frozen" ) {
            water.set_flag( flag_FROZEN );
            REQUIRE( water.has_own_flag( flag_FROZEN ) );

            THEN( "they cannot drink it" ) {
                expect_cannot_eat( dummy, water, "You can't drink it while it's frozen." );
            }
        }
    }

    GIVEN( "food that is edible when frozen" ) {
        item necco( "neccowafers" );
        REQUIRE( necco.has_flag( flag_EDIBLE_FROZEN ) );

        WHEN( "it is frozen" ) {
            necco.set_flag( flag_FROZEN );
            REQUIRE( necco.has_own_flag( flag_FROZEN ) );

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
        REQUIRE( milkshake.has_flag( flag_MELTS ) );

        WHEN( "it is frozen" ) {
            milkshake.set_flag( flag_FROZEN );
            REQUIRE( milkshake.has_own_flag( flag_FROZEN ) );

            THEN( "they can eat it" ) {
                expect_can_eat( dummy, milkshake );
            }
        }
    }
}

TEST_CASE( "who_can_eat_inedible_animal_food", "[can_eat][edible_rating][inedible][animal]" )
{
    avatar dummy;
    dummy.set_body();
    // Note: There are similar conditions for INEDIBLE food with FELINE or LUPINE flags, but
    // "birdfood" and "cattlefodder" are the only INEDIBLE items that exist in the game.

    GIVEN( "food for animals" ) {
        item birdfood( "birdfood" );
        item cattlefodder( "cattlefodder" );

        REQUIRE( birdfood.has_flag( flag_INEDIBLE ) );
        REQUIRE( cattlefodder.has_flag( flag_INEDIBLE ) );

        REQUIRE( birdfood.has_flag( flag_BIRD ) );
        REQUIRE( cattlefodder.has_flag( flag_CATTLE ) );

        WHEN( "character is not bird or cattle" ) {
            REQUIRE_FALSE( dummy.has_trait( trait_THRESH_BIRD ) );
            REQUIRE_FALSE( dummy.has_trait( trait_THRESH_CATTLE ) );

            const std::string expect_reason = "That doesn't look edible to you.";

            THEN( "they cannot eat bird food" ) {
                expect_cannot_eat( dummy, birdfood, expect_reason );
            }

            THEN( "they cannot eat cattle fodder" ) {
                expect_cannot_eat( dummy, cattlefodder, expect_reason );
            }
        }

        WHEN( "character is a bird" ) {
            dummy.toggle_trait( trait_THRESH_BIRD );
            REQUIRE( dummy.has_trait( trait_THRESH_BIRD ) );

            THEN( "they can eat bird food" ) {
                expect_can_eat( dummy, birdfood );
            }
        }

        WHEN( "character is cattle" ) {
            dummy.toggle_trait( trait_THRESH_CATTLE );
            REQUIRE( dummy.has_trait( trait_THRESH_CATTLE ) );

            THEN( "they can eat cattle fodder" ) {
                expect_can_eat( dummy, cattlefodder );
            }
        }
    }
}

TEST_CASE( "what_herbivores_can_eat", "[can_eat][edible_rating][herbivore]" )
{
    avatar dummy;
    dummy.set_body();

    GIVEN( "character is an herbivore" ) {
        dummy.toggle_trait( trait_HERBIVORE );
        REQUIRE( dummy.has_trait( trait_HERBIVORE ) );

        std::string expect_reason = "The thought of eating that makes you feel sick.";

        THEN( "they cannot eat meat" ) {
            item meat( "meat_cooked" );
            REQUIRE( meat.has_flag( flag_ALLERGEN_MEAT ) );

            expect_cannot_eat( dummy, meat, expect_reason, INEDIBLE_MUTATION );
        }

        THEN( "they cannot eat eggs" ) {
            item eggs( "scrambled_eggs" );
            REQUIRE( eggs.has_flag( flag_ALLERGEN_EGG ) );

            expect_cannot_eat( dummy, eggs, expect_reason, INEDIBLE_MUTATION );
        }
    }
}

TEST_CASE( "what_carnivores_can_eat", "[can_eat][edible_rating][carnivore]" )
{
    avatar dummy;
    dummy.set_body();

    GIVEN( "character is a carnivore" ) {
        dummy.toggle_trait( trait_CARNIVORE );
        REQUIRE( dummy.has_trait( trait_CARNIVORE ) );

        std::string expect_reason = "Eww.  Inedible plant stuff!";

        THEN( "they cannot eat veggies" ) {
            item veggy( "veggy" );
            REQUIRE( veggy.has_flag( flag_ALLERGEN_VEGGY ) );

            expect_cannot_eat( dummy, veggy, expect_reason, INEDIBLE_MUTATION );
        }

        THEN( "they cannot eat fruit" ) {
            item apple( "apple" );
            REQUIRE( apple.has_flag( flag_ALLERGEN_FRUIT ) );

            expect_cannot_eat( dummy, apple, expect_reason, INEDIBLE_MUTATION );
        }

        THEN( "they cannot eat wheat" ) {
            item bread( "sourdough_bread" );
            REQUIRE( bread.has_flag( flag_ALLERGEN_WHEAT ) );

            expect_cannot_eat( dummy, bread, expect_reason, INEDIBLE_MUTATION );
        }

        THEN( "they cannot eat nuts" ) {
            item nuts( "pine_nuts" );
            REQUIRE( nuts.has_flag( flag_ALLERGEN_NUT ) );

            expect_cannot_eat( dummy, nuts, expect_reason, INEDIBLE_MUTATION );
        }

        THEN( "they can eat junk food, but are allergic to it" ) {
            item chocolate( "chocolate" );
            REQUIRE( chocolate.has_flag( flag_ALLERGEN_JUNK ) );

            expect_can_eat( dummy, chocolate );
            expect_will_eat( dummy, chocolate, "Your stomach won't be happy (allergy).", ALLERGY );
        }
    }
}

TEST_CASE( "what_you_can_eat_with_a_mycus_dependency", "[can_eat][edible_rating][mycus]" )
{
    avatar dummy;
    dummy.set_body();

    GIVEN( "character is mycus-dependent" ) {
        dummy.toggle_trait( trait_M_DEPENDENT );
        REQUIRE( dummy.has_trait( trait_M_DEPENDENT ) );

        THEN( "they cannot eat normal food" ) {
            item nuts( "pine_nuts" );
            REQUIRE_FALSE( nuts.has_flag( flag_MYCUS_OK ) );

            expect_cannot_eat( dummy, nuts, "We can't eat that.  It's not right for us.", INEDIBLE_MUTATION );
        }

        THEN( "they can eat mycus food" ) {
            item berry( "marloss_berry" );
            REQUIRE( berry.has_flag( flag_MYCUS_OK ) );

            expect_can_eat( dummy, berry );
        }
    }
}

TEST_CASE( "what_you_can_drink_with_a_proboscis", "[can_eat][edible_rating][proboscis]" )
{
    avatar dummy;
    dummy.set_body();

    GIVEN( "character has a proboscis" ) {
        dummy.toggle_trait( trait_PROBOSCIS );
        REQUIRE( dummy.has_trait( trait_PROBOSCIS ) );

        // Cannot drink
        std::string expect_reason = "Ugh, you can't drink that!";

        GIVEN( "a drink that is 'eaten' (USE_EAT_VERB)" ) {
            item soup( "soup_veggy" );
            REQUIRE( soup.has_flag( flag_USE_EAT_VERB ) );

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
            REQUIRE_FALSE( broth.has_flag( flag_USE_EAT_VERB ) );

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

TEST_CASE( "can_eat_with_nausea", "[will_eat][edible_rating][nausea]" )
{
    avatar dummy;
    item toastem( "toastem" );

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

TEST_CASE( "can_eat_with_allergies", "[will_eat][edible_rating][allergy]" )
{
    avatar dummy;
    dummy.set_body();
    item fruit( "apple" );
    REQUIRE( fruit.has_flag( flag_ALLERGEN_FRUIT ) );

    GIVEN( "character hates fruit" ) {
        dummy.toggle_trait( trait_ANTIFRUIT );
        REQUIRE( dummy.has_trait( trait_ANTIFRUIT ) );

        THEN( "they can eat fruit, but won't like it" ) {
            expect_can_eat( dummy, fruit );
            expect_will_eat( dummy, fruit, "Your stomach won't be happy (allergy).", ALLERGY );
        }
    }
}

TEST_CASE( "who_will_eat_rotten_food", "[will_eat][edible_rating][rotten]" )
{
    avatar dummy;
    dummy.set_body();

    GIVEN( "food just barely rotten" ) {
        item toastem_rotten = item( "toastem" );
        toastem_rotten.set_relative_rot( 1.01 );
        REQUIRE( toastem_rotten.rotten() );

        WHEN( "character is normal" ) {
            REQUIRE_FALSE( dummy.has_trait( trait_SAPROPHAGE ) );
            REQUIRE_FALSE( dummy.has_trait( trait_SAPROVORE ) );

            THEN( "they can eat it, though they are disgusted by it" ) {
                expect_can_eat( dummy, toastem_rotten );

                auto conseq = dummy.will_eat( toastem_rotten, false );
                CHECK( conseq.value() == ROTTEN );
                CHECK( conseq.str() == "This is rotten and smells awful!" );
            }
        }

        WHEN( "character is a saprovore" ) {
            dummy.toggle_trait( trait_SAPROVORE );
            REQUIRE( dummy.has_trait( trait_SAPROVORE ) );

            THEN( "they can eat it, and don't mind that it is rotten" ) {
                expect_can_eat( dummy, toastem_rotten );

                auto conseq = dummy.will_eat( toastem_rotten, false );
                CHECK( conseq.value() == EDIBLE );
                CHECK( conseq.str().empty() );
            }
        }

        WHEN( "character is a saprophage" ) {
            dummy.toggle_trait( trait_SAPROPHAGE );
            REQUIRE( dummy.has_trait( trait_SAPROPHAGE ) );

            THEN( "they can eat it, and like that it is rotten" ) {
                expect_can_eat( dummy, toastem_rotten );

                auto conseq = dummy.will_eat( toastem_rotten, false );
                CHECK( conseq.value() == EDIBLE );
                CHECK( conseq.str().empty() );
            }
        }
    }
}

TEST_CASE( "who_will_eat_cooked_human_flesh", "[will_eat][edible_rating][cannibal]" )
{
    avatar dummy;
    dummy.set_body();

    GIVEN( "some cooked human flesh" ) {
        item flesh( "human_cooked" );
        REQUIRE( flesh.has_flag( flag_CANNIBALISM ) );

        WHEN( "character is not a cannibal" ) {
            REQUIRE_FALSE( dummy.has_trait( trait_CANNIBAL ) );

            THEN( "they can eat it, but feel sick about it" ) {
                expect_can_eat( dummy, flesh );
                expect_will_eat( dummy, flesh, "The thought of eating human flesh makes you feel sick.",
                                 CANNIBALISM );
            }
        }

        WHEN( "character is a cannibal" ) {
            dummy.toggle_trait( trait_CANNIBAL );
            REQUIRE( dummy.has_trait( trait_CANNIBAL ) );

            THEN( "they can eat it without any qualms" ) {
                expect_can_eat( dummy, flesh );
                expect_will_eat( dummy, flesh, "", EDIBLE );
            }
        }
    }
}

