#include "catch/catch.hpp"

#include "crafting.h"
#include "game.h"
#include "itype.h"
#include "npc.h"
#include "player.h"
#include "recipe_dictionary.h"

#include "map_helpers.h"
#include "player_helpers.h"

TEST_CASE( "recipe_subset" )
{
    recipe_subset subset;

    REQUIRE( subset.size() == 0 );
    GIVEN( "a recipe of rum" ) {
        const recipe *r = &recipe_id( "brew_rum" ).obj();

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
TEST_CASE( "available_recipes", "[.]" )
{
    const recipe *r = &recipe_id( "brew_mead" ).obj();
    player dummy;

    REQUIRE( dummy.get_skill_level( r->skill_used ) == 0 );
    REQUIRE_FALSE( dummy.knows_recipe( r ) );
    REQUIRE( r->skill_used );

    GIVEN( "a recipe that can be automatically learned" ) {
        WHEN( "the player has lower skill" ) {
            dummy.set_skill_level( r->skill_used, r->difficulty - 1 );

            THEN( "he can't brew it" ) {
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
            dummy.do_read( cookbook );
            dummy.set_skill_level( r->skill_used, 2 );

            AND_WHEN( "he searches for the recipe in the book" ) {
                THEN( "he finds it!" ) {
                    CHECK( dummy.get_recipes_from_books( dummy.inv ).contains( r ) );
                }
                THEN( "it's easier in the book" ) {
                    CHECK( dummy.get_recipes_from_books( dummy.inv ).get_custom_difficulty( r ) == 2 );
                }
                THEN( "he still hasn't the recipe memorized" ) {
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

        who.set_attitude( NPCATT_FOLLOW );
        who.spawn_at_sm( 0, 0, 0 );

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

static void test_craft( const recipe_id &rid, const std::vector<item> tools,
                        bool expect_craftable )
{
    clear_player();
    clear_map();

    tripoint test_origin( 60, 60, 0 );
    g->u.setpos( test_origin );
    item backpack( "backpack" );
    g->u.wear( g->u.i_add( backpack ), false );
    for( item gear : tools ) {
        g->u.i_add( gear );
    }

    const recipe *r = &rid.obj();

    requirement_data reqs = r->requirements();
    inventory crafting_inv = g->u.crafting_inventory();
    bool can_craft = reqs.can_make_with_inventory( g->u.crafting_inventory() );
    CHECK( can_craft == expect_craftable );
    if( expect_craftable ) {
        g->u.consume_components_for_craft( r, 1 );
        g->u.invalidate_crafting_inventory();
    }
}

TEST_CASE( "charge_handling" )
{
    SECTION( "carver" ) {
        std::vector<item> tools;
        tools.emplace_back( "hotplate", -1, 20 );
        tools.emplace_back( "soldering_iron", -1, 20 );
        tools.insert( tools.end(), 10, item( "solder_wire" ) );
        tools.emplace_back( "screwdriver" );
        tools.emplace_back( "mold_plastic" );
        tools.insert( tools.end(), 6, item( "plastic_chunk" ) );
        tools.insert( tools.end(), 2, item( "blade" ) );
        tools.insert( tools.end(), 5, item( "cable" ) );
        tools.emplace_back( "motor_small" );
        tools.emplace_back( "power_supply" );
        tools.emplace_back( "scrap" );

        test_craft( recipe_id( "carver_off" ), tools, true );
        CHECK( get_remaining_charges( "hotplate" ) == 10 );
        CHECK( get_remaining_charges( "soldering_iron" ) == 10 );
    }
    SECTION( "carver_split_charges" ) {
        std::vector<item> tools;
        tools.emplace_back( "hotplate", -1, 5 );
        tools.emplace_back( "hotplate", -1, 5 );
        tools.emplace_back( "soldering_iron", -1, 5 );
        tools.emplace_back( "soldering_iron", -1, 5 );
        tools.insert( tools.end(), 10, item( "solder_wire" ) );
        tools.emplace_back( "screwdriver" );
        tools.emplace_back( "mold_plastic" );
        tools.insert( tools.end(), 6, item( "plastic_chunk" ) );
        tools.insert( tools.end(), 2, item( "blade" ) );
        tools.insert( tools.end(), 5, item( "cable" ) );
        tools.emplace_back( "motor_small" );
        tools.emplace_back( "power_supply" );
        tools.emplace_back( "scrap" );

        test_craft( recipe_id( "carver_off" ), tools, true );
        CHECK( get_remaining_charges( "hotplate" ) == 0 );
        CHECK( get_remaining_charges( "soldering_iron" ) == 0 );
    }
    SECTION( "UPS_modded_carver" ) {
        std::vector<item> tools;
        item hotplate( "hotplate", -1, 0 );
        hotplate.contents.emplace_back( "battery_ups" );
        tools.push_back( hotplate );
        item soldering_iron( "soldering_iron", -1, 0 );
        tools.insert( tools.end(), 10, item( "solder_wire" ) );
        soldering_iron.contents.emplace_back( "battery_ups" );
        tools.push_back( soldering_iron );
        tools.emplace_back( "screwdriver" );
        tools.emplace_back( "mold_plastic" );
        tools.insert( tools.end(), 6, item( "plastic_chunk" ) );
        tools.insert( tools.end(), 2, item( "blade" ) );
        tools.insert( tools.end(), 5, item( "cable" ) );
        tools.emplace_back( "motor_small" );
        tools.emplace_back( "power_supply" );
        tools.emplace_back( "scrap" );
        tools.emplace_back( "UPS_off", -1, 500 );

        test_craft( recipe_id( "carver_off" ), tools, true );
        CHECK( get_remaining_charges( "hotplate" ) == 0 );
        CHECK( get_remaining_charges( "soldering_iron" ) == 0 );
        CHECK( get_remaining_charges( "UPS_off" ) == 480 );
    }
    SECTION( "UPS_modded_carver_missing_charges" ) {
        std::vector<item> tools;
        item hotplate( "hotplate", -1, 0 );
        hotplate.contents.emplace_back( "battery_ups" );
        tools.push_back( hotplate );
        item soldering_iron( "soldering_iron", -1, 0 );
        tools.insert( tools.end(), 10, item( "solder_wire" ) );
        soldering_iron.contents.emplace_back( "battery_ups" );
        tools.push_back( soldering_iron );
        tools.emplace_back( "screwdriver" );
        tools.emplace_back( "mold_plastic" );
        tools.insert( tools.end(), 6, item( "plastic_chunk" ) );
        tools.insert( tools.end(), 2, item( "blade" ) );
        tools.insert( tools.end(), 5, item( "cable" ) );
        tools.emplace_back( "motor_small" );
        tools.emplace_back( "power_supply" );
        tools.emplace_back( "scrap" );
        tools.emplace_back( "UPS_off", -1, 10 );

        test_craft( recipe_id( "carver_off" ), tools, false );
    }
}

TEST_CASE( "tool_use" )
{
    SECTION( "clean_water" ) {
        std::vector<item> tools;
        tools.emplace_back( "hotplate", -1, 20 );
        item plastic_bottle( "bottle_plastic" );
        plastic_bottle.contents.emplace_back( "water", -1, 2 );
        tools.push_back( plastic_bottle );
        tools.emplace_back( "pot" );

        test_craft( recipe_id( "water_clean" ), tools, true );
    }
    SECTION( "clean_water_in_occupied_cooking_vessel" ) {
        std::vector<item> tools;
        tools.emplace_back( "hotplate", -1, 20 );
        item plastic_bottle( "bottle_plastic" );
        plastic_bottle.contents.emplace_back( "water", -1, 2 );
        tools.push_back( plastic_bottle );
        item jar( "jar_glass" );
        // If it's not watertight the water will spill.
        REQUIRE( jar.is_watertight_container() );
        jar.contents.emplace_back( "water", -1, 2 );
        tools.push_back( jar );

        test_craft( recipe_id( "water_clean" ), tools, false );
    }
}
