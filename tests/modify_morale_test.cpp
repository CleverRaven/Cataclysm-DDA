#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "catch/catch.hpp"
#include "game.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "morale_types.h"
#include "point.h"
#include "type_id.h"

static const std::string flag_ALLERGEN_JUNK( "ALLERGEN_JUNK" );
static const std::string flag_CANNIBALISM( "CANNIBALISM" );
static const std::string flag_EATEN_HOT( "EATEN_HOT" );
static const std::string flag_HOT( "HOT" );
static const std::string flag_URSINE_HONEY( "URSINE_HONEY" );

static const trait_id trait_ANTIFRUIT( "ANTIFRUIT" );
static const trait_id trait_ANTIJUNK( "ANTIJUNK" );
static const trait_id trait_ANTIWHEAT( "ANTIWHEAT" );
static const trait_id trait_BADTEMPER( "BADTEMPER" );
static const trait_id trait_CANNIBAL( "CANNIBAL" );
static const trait_id trait_CARNIVORE( "CARNIVORE" );
static const trait_id trait_CLAWS( "CLAWS" );
static const trait_id trait_FAT( "FAT" );
static const trait_id trait_HIBERNATE( "HIBERNATE" );
static const trait_id trait_LACTOSE( "LACTOSE" );
static const trait_id trait_LARGE( "LARGE" );
static const trait_id trait_MEATARIAN( "MEATARIAN" );
static const trait_id trait_PADDED_FEET( "PADDED_FEET" );
static const trait_id trait_PROJUNK( "PROJUNK" );
static const trait_id trait_PROJUNK2( "PROJUNK2" );
static const trait_id trait_PSYCHOPATH( "PSYCHOPATH" );
static const trait_id trait_SAPIOVORE( "SAPIOVORE" );
static const trait_id trait_SAPROPHAGE( "SAPROPHAGE" );
static const trait_id trait_SPIRITUAL( "SPIRITUAL" );
static const trait_id trait_TABLEMANNERS( "TABLEMANNERS" );
static const trait_id trait_THRESH_URSINE( "THRESH_URSINE" );
static const trait_id trait_URSINE_EYE( "URSINE_EYE" );
static const trait_id trait_URSINE_FUR( "URSINE_FUR" );
static const trait_id trait_VEGETARIAN( "VEGETARIAN" );

// Test cases for `Character::modify_morale` defined in `src/consumption.cpp`

TEST_CASE( "food enjoyability", "[food][modify_morale][fun]" )
{
    avatar dummy;
    dummy.worn.push_back( item( "backpack" ) );
    std::pair<int, int> fun;

    GIVEN( "food with positive fun" ) {
        item &toastem = dummy.i_add( item( "toastem" ) );
        fun = dummy.fun_for( toastem );
        REQUIRE( fun.first > 0 );

        THEN( "character gets a morale bonus becase it tastes good" ) {
            dummy.modify_morale( toastem );
            CHECK( dummy.has_morale( MORALE_FOOD_GOOD ) >= fun.first );
        }
    }

    GIVEN( "food with negative fun" ) {
        item &garlic = dummy.i_add( item( "garlic" ) );
        fun = dummy.fun_for( garlic );
        REQUIRE( fun.first < 0 );

        THEN( "character gets a morale penalty because it tastes bad" ) {
            dummy.modify_morale( garlic );
            CHECK( dummy.has_morale( MORALE_FOOD_BAD ) <= fun.first );
        }
    }
}

TEST_CASE( "dining with table and chair", "[food][modify_morale][table][chair]" )
{
    clear_map();
    avatar dummy;
    const tripoint avatar_pos( 60, 60, 0 );
    dummy.setpos( avatar_pos );
    dummy.worn.push_back( item( "backpack" ) );

    // Morale bonus only applies to unspoiled food that is not junk
    item &bread = dummy.i_add( item( "sourdough_bread" ) );
    REQUIRE( bread.is_fresh() );
    REQUIRE_FALSE( bread.has_flag( flag_ALLERGEN_JUNK ) );

    // Much of the below code is to support the "Rigid Table Manners" trait, notoriously prone to
    // causing unexpected morale effects from bandages, aspirin, cigs etc. (#38698, #39580)
    // Table-related morale effects should not apply to any of the following items:
    const std::vector<std::string> no_table_eating_bonus = {
        {
            "aspirin",
            "bandages",
            "caffeine",
            "cig",
            "codeine",
            "crack",
            "dayquil",
            "disinfectant",
            "joint",
            "oxycodone",
            "weed"
        }
    };

    GIVEN( "no table or chair are nearby" ) {
        REQUIRE_FALSE( g->m.has_nearby_table( dummy.pos(), 1 ) );
        REQUIRE_FALSE( g->m.has_nearby_chair( dummy.pos(), 1 ) );

        AND_GIVEN( "character has normal table manners" ) {
            REQUIRE_FALSE( dummy.has_trait( trait_TABLEMANNERS ) );

            THEN( "their morale is unaffected by eating without a table" ) {
                dummy.clear_morale();
                dummy.modify_morale( bread );
                CHECK_FALSE( dummy.has_morale( MORALE_ATE_WITHOUT_TABLE ) );
            }
        }

        AND_GIVEN( "character has strict table manners" ) {
            dummy.toggle_trait( trait_TABLEMANNERS );
            REQUIRE( dummy.has_trait( trait_TABLEMANNERS ) );

            THEN( "they get a morale penalty for eating food without a table" ) {
                dummy.clear_morale();
                dummy.modify_morale( bread );
                CHECK( dummy.has_morale( MORALE_ATE_WITHOUT_TABLE ) <= -2 );
            }

            for( std::string item_name : no_table_eating_bonus ) {
                item test_item( item_name );

                THEN( "they get no morale penalty for using " + item_name + " at a table" ) {
                    dummy.clear_morale();
                    dummy.modify_morale( test_item );
                    CHECK_FALSE( dummy.has_morale( MORALE_ATE_WITHOUT_TABLE ) );
                }
            }
        }
    }

    GIVEN( "a table and chair are nearby" ) {
        g->m.furn_set( avatar_pos + tripoint_north, furn_id( "f_table" ) );
        g->m.furn_set( avatar_pos + tripoint_east, furn_id( "f_chair" ) );
        REQUIRE( g->m.has_nearby_table( dummy.pos(), 1 ) );
        REQUIRE( g->m.has_nearby_chair( dummy.pos(), 1 ) );

        AND_GIVEN( "character has normal table manners" ) {
            REQUIRE_FALSE( dummy.has_trait( trait_TABLEMANNERS ) );

            THEN( "they get a minimal morale bonus for eating with a table" ) {
                dummy.clear_morale();
                dummy.modify_morale( bread );
                CHECK( dummy.has_morale( MORALE_ATE_WITH_TABLE ) >= 1 );
            }

            for( std::string item_name : no_table_eating_bonus ) {
                item test_item( item_name );

                THEN( "they get no morale bonus for using " + item_name + " at a table" ) {
                    dummy.clear_morale();
                    dummy.modify_morale( test_item );
                    CHECK_FALSE( dummy.has_morale( MORALE_ATE_WITH_TABLE ) );
                }
            }
        }

        AND_GIVEN( "character has strict table manners" ) {
            dummy.toggle_trait( trait_TABLEMANNERS );
            REQUIRE( dummy.has_trait( trait_TABLEMANNERS ) );

            THEN( "they get a small morale bonus for eating with a table" ) {
                dummy.clear_morale();
                dummy.modify_morale( bread );
                CHECK( dummy.has_morale( MORALE_ATE_WITH_TABLE ) >= 3 );
            }

            for( std::string item_name : no_table_eating_bonus ) {
                item test_item( item_name );

                THEN( "they get no morale bonus for using " + item_name + " at a table" ) {
                    dummy.clear_morale();
                    dummy.modify_morale( test_item );
                    CHECK_FALSE( dummy.has_morale( MORALE_ATE_WITH_TABLE ) );
                }
            }
        }
    }
}

TEST_CASE( "eating hot food", "[food][modify_morale][hot]" )
{
    avatar dummy;
    dummy.worn.push_back( item( "backpack" ) );

    GIVEN( "some food that tastes better when hot" ) {
        item &bread = dummy.i_add( item( "sourdough_bread" ) );
        REQUIRE( bread.has_flag( flag_EATEN_HOT ) );

        WHEN( "it is hot" ) {
            bread.set_flag( flag_HOT );
            REQUIRE( bread.has_flag( flag_HOT ) );

            THEN( "character gets a morale bonus for having a hot meal" ) {
                dummy.clear_morale();
                dummy.modify_morale( bread );
                CHECK( dummy.has_morale( MORALE_FOOD_HOT ) > 0 );
            }
        }

        WHEN( "it is not hot" ) {
            REQUIRE_FALSE( bread.has_flag( flag_HOT ) );

            THEN( "character does not get any morale bonus" ) {
                dummy.clear_morale();
                dummy.modify_morale( bread );
                CHECK_FALSE( dummy.has_morale( MORALE_FOOD_HOT ) );
            }
        }
    }
}


TEST_CASE( "drugs", "[food][modify_morale][drug]" )
{
    avatar dummy;
    std::pair<int, int> fun;


    const std::vector<std::string> drugs_to_test = {
        {
            "gum",
            "caff_gum",
            "nic_gum",
            "cig",
            "ecig",
            "cigar",
            "joint",
            "lsd",
            "weed",
            "crack",
            "meth",
            "heroin",
            "tobacco",
            "morphine"
        }
    };

    GIVEN( "avatar has baseline morale" ) {
        dummy.clear_morale();
        REQUIRE( dummy.has_morale( MORALE_FOOD_GOOD ) == 0 );

        for( std::string drug_name : drugs_to_test ) {
            item drug( drug_name );
            fun = dummy.fun_for( drug );
            REQUIRE( fun.first > 0 );

            THEN( "they enjoy " + drug_name ) {
                dummy.modify_morale( drug );
                CHECK( dummy.has_morale( MORALE_FOOD_GOOD ) >= fun.first );
            }
        }
    }
}

TEST_CASE( "cannibalism", "[food][modify_morale][cannibal]" )
{
    avatar dummy;
    dummy.worn.push_back( item( "backpack" ) );

    item &human = dummy.i_add( item( "bone_human" ) );
    REQUIRE( human.has_flag( flag_CANNIBALISM ) );

    GIVEN( "character is not a cannibal or sapiovore" ) {
        REQUIRE_FALSE( dummy.has_trait( trait_CANNIBAL ) );
        REQUIRE_FALSE( dummy.has_trait( trait_SAPIOVORE ) );

        THEN( "they get a large morale penalty for eating humans" ) {
            dummy.clear_morale();
            dummy.modify_morale( human );
            CHECK( dummy.has_morale( MORALE_CANNIBAL ) <= -60 );
        }

        WHEN( "character is a psychopath" ) {
            dummy.toggle_trait( trait_PSYCHOPATH );
            REQUIRE( dummy.has_trait( trait_PSYCHOPATH ) );

            THEN( "their morale is unffected by eating humans" ) {
                dummy.clear_morale();
                dummy.modify_morale( human );
                CHECK( dummy.has_morale( MORALE_CANNIBAL ) == 0 );
            }

            AND_WHEN( "character is a spiritual psychopath" ) {
                dummy.toggle_trait( trait_SPIRITUAL );
                REQUIRE( dummy.has_trait( trait_SPIRITUAL ) );

                THEN( "they get a small morale bonus for eating humans" ) {
                    dummy.clear_morale();
                    dummy.modify_morale( human );
                    CHECK( dummy.has_morale( MORALE_CANNIBAL ) >= 5 );
                }
            }
        }
    }

    WHEN( "character is a cannibal" ) {
        dummy.toggle_trait( trait_CANNIBAL );
        REQUIRE( dummy.has_trait( trait_CANNIBAL ) );

        THEN( "they get a morale bonus for eating humans" ) {
            dummy.clear_morale();
            dummy.modify_morale( human );
            CHECK( dummy.has_morale( MORALE_CANNIBAL ) >= 10 );
        }

        AND_WHEN( "they are also a psychopath" ) {
            dummy.toggle_trait( trait_PSYCHOPATH );
            REQUIRE( dummy.has_trait( trait_PSYCHOPATH ) );

            THEN( "they get a substantial morale bonus for eating humans" ) {
                dummy.clear_morale();
                dummy.modify_morale( human );
                CHECK( dummy.has_morale( MORALE_CANNIBAL ) >= 15 );
            }

            AND_WHEN( "they are also spiritual" ) {
                dummy.toggle_trait( trait_SPIRITUAL );
                REQUIRE( dummy.has_trait( trait_SPIRITUAL ) );

                THEN( "they get a large morale bonus for eating humans" ) {
                    dummy.clear_morale();
                    dummy.modify_morale( human );
                    CHECK( dummy.has_morale( MORALE_CANNIBAL ) >= 25 );
                }
            }
        }
    }
}

TEST_CASE( "sweet junk food", "[food][modify_morale][junk][sweet]" )
{
    avatar dummy;
    dummy.worn.push_back( item( "backpack" ) );

    GIVEN( "some sweet junk food" ) {
        item &necco = dummy.i_add( item( "neccowafers" ) );

        WHEN( "character has a sweet tooth" ) {
            dummy.toggle_trait( trait_PROJUNK );
            REQUIRE( dummy.has_trait( trait_PROJUNK ) );

            THEN( "they get a morale bonus from its sweetness" ) {
                dummy.clear_morale();
                dummy.modify_morale( necco );
                CHECK( dummy.has_morale( MORALE_SWEETTOOTH ) >= 5 );
                CHECK( dummy.has_morale( MORALE_SWEETTOOTH ) <= 5 );

                AND_THEN( "they enjoy it" ) {
                    CHECK( dummy.has_morale( MORALE_FOOD_GOOD ) > 0 );
                }
            }
        }

        WHEN( "character is sugar-loving" ) {
            dummy.toggle_trait( trait_PROJUNK2 );
            REQUIRE( dummy.has_trait( trait_PROJUNK2 ) );

            THEN( "they get a significant morale bonus from its sweetness" ) {
                dummy.clear_morale();
                dummy.modify_morale( necco );
                CHECK( dummy.has_morale( MORALE_SWEETTOOTH ) >= 10 );
                CHECK( dummy.has_morale( MORALE_SWEETTOOTH ) <= 50 );

                AND_THEN( "they enjoy it" ) {
                    CHECK( dummy.has_morale( MORALE_FOOD_GOOD ) );
                }
            }
        }

        WHEN( "character is a carnivore" ) {
            dummy.toggle_trait( trait_CARNIVORE );
            REQUIRE( dummy.has_trait( trait_CARNIVORE ) );

            THEN( "they get an morale penalty due to indigestion" ) {
                dummy.clear_morale();
                dummy.modify_morale( necco );
                CHECK( dummy.has_morale( MORALE_NO_DIGEST ) <= -25 );
            }
        }
    }
}

TEST_CASE( "junk food that is not ingested", "[modify_morale][junk][no_ingest]" )
{
    avatar dummy;
    dummy.worn.push_back( item( "backpack" ) );

    item &caff_gum = dummy.i_add( item( "caff_gum" ) );

    // This is a regression test for gum having "junk" material, and being
    // treated as junk food (despite not being ingested). At the time of
    // writing this test, gum and caffeinated gum are made of "junk", and thus
    // are treated as junk food, but might not always be so. Here we set the
    // relevant flags to cover the scenario we're interested in, namely any
    // comestible having both "junk" and "no ingest" flags.
    caff_gum.set_flag( "ALLERGEN_JUNK" );
    caff_gum.set_flag( "NO_INGEST" );

    REQUIRE( caff_gum.has_flag( "ALLERGEN_JUNK" ) );
    REQUIRE( caff_gum.has_flag( "NO_INGEST" ) );

    GIVEN( "character has a sweet tooth" ) {
        dummy.toggle_trait( trait_PROJUNK );
        REQUIRE( dummy.has_trait( trait_PROJUNK ) );

        THEN( "they do not get an extra morale bonus for chewing gum" ) {
            dummy.clear_morale();
            dummy.modify_morale( caff_gum );
            CHECK( dummy.has_morale( MORALE_SWEETTOOTH ) == 0 );

            AND_THEN( "they still enjoy it" ) {
                CHECK( dummy.has_morale( MORALE_FOOD_GOOD ) > 0 );
            }
        }
    }

    GIVEN( "character is sugar-loving" ) {
        dummy.toggle_trait( trait_PROJUNK2 );
        REQUIRE( dummy.has_trait( trait_PROJUNK2 ) );

        THEN( "they do not get an extra morale bonus for chewing gum" ) {
            dummy.clear_morale();
            dummy.modify_morale( caff_gum );
            CHECK( dummy.has_morale( MORALE_SWEETTOOTH ) == 0 );

            AND_THEN( "they still enjoy it" ) {
                CHECK( dummy.has_morale( MORALE_FOOD_GOOD ) > 0 );
            }
        }
    }

    GIVEN( "character has junk food intolerance" ) {
        dummy.toggle_trait( trait_ANTIJUNK );
        REQUIRE( dummy.has_trait( trait_ANTIJUNK ) );

        THEN( "they do not get a morale penalty for chewing gum" ) {
            dummy.clear_morale();
            dummy.modify_morale( caff_gum );
            CHECK( dummy.has_morale( MORALE_ANTIJUNK ) == 0 );

            AND_THEN( "they still enjoy it" ) {
                CHECK( dummy.has_morale( MORALE_FOOD_GOOD ) > 0 );
            }
        }
    }
}

TEST_CASE( "food allergies and intolerances", "[food][modify_morale][allergy]" )
{
    avatar dummy;
    dummy.worn.push_back( item( "backpack" ) );
    int penalty = -75;

    GIVEN( "character is vegetarian" ) {
        dummy.toggle_trait( trait_VEGETARIAN );
        REQUIRE( dummy.has_trait( trait_VEGETARIAN ) );

        THEN( "they get a morale penalty for eating meat" ) {
            item &meat = dummy.i_add( item( "meat" ) );
            REQUIRE( meat.has_flag( "ALLERGEN_MEAT" ) );
            dummy.clear_morale();
            dummy.modify_morale( meat );
            CHECK( dummy.has_morale( MORALE_VEGETARIAN ) <= penalty );
        }
    }

    GIVEN( "character is lactose intolerant" ) {
        dummy.toggle_trait( trait_LACTOSE );
        REQUIRE( dummy.has_trait( trait_LACTOSE ) );

        THEN( "they get a morale penalty for drinking milk" ) {
            item &milk_container = dummy.i_add( item( "milk" ).in_its_container() );
            item &milk = milk_container.contents.only_item();
            REQUIRE( milk.has_flag( "ALLERGEN_MILK" ) );
            dummy.clear_morale();
            dummy.modify_morale( milk );
            CHECK( dummy.has_morale( MORALE_LACTOSE ) <= penalty );
        }
    }

    GIVEN( "character is grain intolerant" ) {
        dummy.toggle_trait( trait_ANTIWHEAT );
        REQUIRE( dummy.has_trait( trait_ANTIWHEAT ) );

        THEN( "they get a morale penalty for eating wheat" ) {
            item &wheat = dummy.i_add( item( "wheat" ) );
            REQUIRE( wheat.has_flag( "ALLERGEN_WHEAT" ) );
            dummy.clear_morale();
            dummy.modify_morale( wheat );
            CHECK( dummy.has_morale( MORALE_ANTIWHEAT ) <= penalty );
        }
    }

    GIVEN( "character hates vegetables" ) {
        dummy.toggle_trait( trait_MEATARIAN );
        REQUIRE( dummy.has_trait( trait_MEATARIAN ) );

        THEN( "they get a morale penalty for eating vegetables" ) {
            item &veggy = dummy.i_add( item( "broccoli" ) );
            REQUIRE( veggy.has_flag( "ALLERGEN_VEGGY" ) );
            dummy.clear_morale();
            dummy.modify_morale( veggy );
            CHECK( dummy.has_morale( MORALE_MEATARIAN ) <= penalty );
        }
    }

    GIVEN( "character hates fruit" ) {
        dummy.toggle_trait( trait_ANTIFRUIT );
        REQUIRE( dummy.has_trait( trait_ANTIFRUIT ) );

        THEN( "they get a morale penalty for eating fruit" ) {
            item &fruit = dummy.i_add( item( "apple" ) );
            REQUIRE( fruit.has_flag( "ALLERGEN_FRUIT" ) );
            dummy.clear_morale();
            dummy.modify_morale( fruit );
            CHECK( dummy.has_morale( MORALE_ANTIFRUIT ) <= penalty );
        }
    }

    GIVEN( "character has a junk food intolerance" ) {
        dummy.toggle_trait( trait_ANTIJUNK );
        REQUIRE( dummy.has_trait( trait_ANTIJUNK ) );

        THEN( "they get a morale penalty for eating junk food" ) {
            item &junk = dummy.i_add( item( "neccowafers" ) );
            REQUIRE( junk.has_flag( "ALLERGEN_JUNK" ) );
            dummy.clear_morale();
            dummy.modify_morale( junk );
            CHECK( dummy.has_morale( MORALE_ANTIJUNK ) <= penalty );
        }
    }
}

TEST_CASE( "saprophage character", "[food][modify_morale][saprophage]" )
{
    avatar dummy;
    dummy.worn.push_back( item( "backpack" ) );

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
                CHECK( dummy.has_morale( MORALE_FOOD_GOOD ) > 10 );
            }
        }

        AND_GIVEN( "some fresh chewable food" ) {
            item &toastem = dummy.i_add( item( "toastem" ) );
            // food rot < 0.1 is fresh
            toastem.set_relative_rot( 0.0 );
            REQUIRE( toastem.is_fresh() );

            THEN( "they get a morale penalty due to indigestion" ) {
                dummy.modify_morale( toastem );
                CHECK( dummy.has_morale( MORALE_NO_DIGEST ) <= -25 );
            }
        }
    }
}

TEST_CASE( "ursine honey", "[food][modify_morale][ursine][honey]" )
{
    avatar dummy;
    dummy.worn.push_back( item( "backpack" ) );

    item &honeycomb = dummy.i_add( item( "honeycomb" ) );
    REQUIRE( honeycomb.has_flag( flag_URSINE_HONEY ) );

    GIVEN( "character is post-threshold ursine" ) {
        dummy.toggle_trait( trait_THRESH_URSINE );
        REQUIRE( dummy.has_trait( trait_THRESH_URSINE ) );

        AND_GIVEN( "they have a lot of ursine mutations" ) {
            dummy.mutate_towards( trait_FAT );
            dummy.mutate_towards( trait_LARGE );
            dummy.mutate_towards( trait_CLAWS );
            dummy.mutate_towards( trait_BADTEMPER );
            dummy.mutate_towards( trait_HIBERNATE );
            dummy.mutate_towards( trait_URSINE_FUR );
            dummy.mutate_towards( trait_URSINE_EYE );
            dummy.mutate_towards( trait_PADDED_FEET );
            REQUIRE( dummy.mutation_category_level["URSINE"] > 40 );

            THEN( "they get an extra honey morale bonus for eating it" ) {
                dummy.modify_morale( honeycomb );
                CHECK( dummy.has_morale( MORALE_HONEY ) > 0 );

                AND_THEN( "they enjoy it" ) {
                    CHECK( dummy.has_morale( MORALE_FOOD_GOOD ) > 0 );
                }
            }
        }
    }
}

