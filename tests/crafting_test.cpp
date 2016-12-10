#include "catch/catch.hpp"

#include "crafting.h"
#include "game.h"
#include "itype.h"
#include "npc.h"
#include "player.h"
#include "recipe_dictionary.h"

TEST_CASE( "recipe_subset" ) {
    recipe_subset subset;

    REQUIRE( subset.size() == 0 );

    GIVEN( "a recipe of rum" ) {
        const recipe *r = &recipe_dict[ "brew_rum" ];

        WHEN( "the recipe is included" ) {
            subset.include( r );

            THEN( "it's in the subset" ) {
                CHECK( subset.size() == 1 );
                CHECK( subset.contains( r ) );
            }

            THEN( "it has its default difficulty" ) {
                CHECK( subset.get_custom_difficulty( r ) == r->difficulty );
            }

            THEN( "it's in the right category" ) {
                const auto cat_recipes( subset.in_category( "CC_FOOD" ) );

                CHECK( cat_recipes.size() == 1 );
                CHECK( std::find( cat_recipes.begin(), cat_recipes.end(), r ) != cat_recipes.end() );
            }

            THEN( "it uses water" ) {
                const auto comp_recipes( subset.of_component( "water" ) );

                CHECK( comp_recipes.size() == 1 );
                CHECK( std::find( comp_recipes.begin(), comp_recipes.end(), r ) != comp_recipes.end() );
            }

            AND_WHEN( "the subset is cleared" ) {
                subset.clear();

                THEN( "it's no longer in the subset" ) {
                    CHECK( subset.size() == 0 );
                    CHECK_FALSE( subset.contains( r ) );
                }
            }
        }

        WHEN( "the recipe is included with higher difficulty" ) {
            subset.include( r, r->difficulty + 1 );

            THEN( "it's harder to perform" ) {
                CHECK( subset.get_custom_difficulty( r ) == r->difficulty + 1 );
            }

            AND_WHEN( "it's included again with default difficulty" ) {
                subset.include( r );

                THEN( "it recovers its normal difficulty" ) {
                    CHECK( subset.get_custom_difficulty( r ) == r->difficulty );
                }
            }

            AND_WHEN( "it's included again with lower difficulty" ) {
                subset.include( r, r->difficulty - 1 );

                THEN( "it becomes easier to perform" ) {
                    CHECK( subset.get_custom_difficulty( r ) == r->difficulty - 1 );
                }
            }
        }

        WHEN( "the recipe is included with lower difficulty" ) {
            subset.include( r, r->difficulty - 1 );

            THEN( "it's easier to perform" ) {
                CHECK( subset.get_custom_difficulty( r ) == r->difficulty - 1 );
            }

            AND_WHEN( "it's included again with default difficulty" ) {
                subset.include( r );

                THEN( "it's still easier to perform" ) {
                    CHECK( subset.get_custom_difficulty( r ) == r->difficulty - 1 );
                }
            }

            AND_WHEN( "it's included again with higher difficulty" ) {
                subset.include( r, r->difficulty + 1 );

                THEN( "it's still easier to perform" ) {
                    CHECK( subset.get_custom_difficulty( r ) == r->difficulty - 1 );
                }
            }
        }
    }
}

// This crashes subsequent testcases for some reason.
TEST_CASE( "available_recipes", "[.]" ) {
    const recipe *r = &recipe_dict[ "brew_mead" ];
    player dummy;

    REQUIRE( dummy.get_skill_level( r->skill_used ) == 0 );
    REQUIRE_FALSE( dummy.knows_recipe( r ) );
    REQUIRE( r->autolearn );
    REQUIRE( r->skill_used );

    GIVEN( "a recipe that can be automatically learned" ) {
        WHEN( "the player has lower skill" ) {
            dummy.set_skill_level( r->skill_used, r->difficulty - 1 );

            THEN( "he can't brew it") {
                CHECK_FALSE( dummy.knows_recipe( r ) );
            }
        }

        WHEN( "the player has just the skill that's required" ) {
            dummy.set_skill_level( r->skill_used, r->difficulty );

            THEN( "he can brew it now!" ) {
                CHECK( dummy.knows_recipe( r ) );

                AND_WHEN( "his skill rusts" ) {
                    dummy.set_skill_level( r->skill_used, 0 );

                    THEN( "he still remembers how to brew it" ) {
                        CHECK( dummy.knows_recipe( r ) );
                    }
                }
            }
        }
    }

    GIVEN( "a brewing cookbook" ) {
        item &cookbook = dummy.i_add( item( "brewing_cookbook" ) );

        REQUIRE( cookbook.is_book() );
        REQUIRE_FALSE( cookbook.type->book->recipes.empty() );
        REQUIRE_FALSE( dummy.knows_recipe( r ) );

        WHEN( "the player read it and has an appropriate skill" ) {
            dummy.do_read( &cookbook );
            dummy.set_skill_level( r->skill_used, 2 );

            AND_WHEN( "he searches for the recipe in the book" ) {
                THEN( "he finds it!" ) {
                    CHECK( dummy.get_recipes_from_books( dummy.inv ).contains( r ) );
                }

                THEN( "it's easier in the book" ) {
                    CHECK( dummy.get_recipes_from_books( dummy.inv ).get_custom_difficulty( r ) == 2 );
                }

                THEN( "he still hasn't the recipe memorized") {
                    CHECK_FALSE( dummy.knows_recipe( r ) );
                }
            }

            AND_WHEN( "he gets rid of the book" ) {
                dummy.i_rem( &cookbook );

                THEN( "he cant brew the recipe anymore" ) {
                    CHECK_FALSE( dummy.get_recipes_from_books( dummy.inv ).contains( r ) );
                }
            }
        }
    }

    GIVEN( "a companion who can help with crafting" ) {
        standard_npc who( "helper", {}, 0 );

        who.attitude = NPCATT_FOLLOW;
        who.spawn_at( 0, 0, 0 );

        g->load_npcs();

        dummy.setpos( who.pos() );
        const auto helpers( dummy.get_crafting_helpers() );

        REQUIRE( std::find( helpers.begin(), helpers.end(), &who ) != helpers.end() );
        REQUIRE_FALSE( dummy.get_available_recipes( dummy.inv, &helpers ).contains( r ) );
        REQUIRE_FALSE( who.knows_recipe( r ) );

        WHEN( "you have the required skill" ) {
            dummy.set_skill_level( r->skill_used, r->difficulty );

            AND_WHEN( "he knows the recipe" ) {
                who.learn_recipe( r );

                THEN( "he helps you" ) {
                    CHECK( dummy.get_available_recipes( dummy.inv, &helpers ).contains( r ) );
                }
            }

            AND_WHEN( "he has the cookbook in his inventory" ) {
                item &cookbook = who.i_add( item( "brewing_cookbook" ) );

                REQUIRE( cookbook.is_book() );
                REQUIRE_FALSE( cookbook.type->book->recipes.empty() );

                THEN( "he shows it to you" ) {
                    CHECK( dummy.get_available_recipes( dummy.inv, &helpers ).contains( r ) );
                }
            }
        }
    }
}
