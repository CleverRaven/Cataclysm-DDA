#include <sstream>

#include "catch/catch.hpp"
#include "crafting.h"
#include "game.h"
#include "itype.h"
#include "map.h"
#include "map_helpers.h"
#include "npc.h"
#include "output.h"
#include "player.h"
#include "player_helpers.h"
#include "recipe_dictionary.h"

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

TEST_CASE( "available_recipes", "[recipes]" )
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

    GIVEN( "an eink pc with a cannibal recipe" ) {
        const recipe *r2 = &recipe_id( "soup_human" ).obj();
        item &eink = dummy.i_add( item( "eink_tablet_pc" ) );
        eink.set_var( "EIPC_RECIPES", ",soup_human," );
        REQUIRE_FALSE( dummy.knows_recipe( r2 ) );

        WHEN( "the player holds it and has an appropriate skill" ) {
            dummy.set_skill_level( r2->skill_used, 2 );

            AND_WHEN( "he searches for the recipe in the tablet" ) {
                THEN( "he finds it!" ) {
                    CHECK( dummy.get_recipes_from_books( dummy.inv ).contains( r2 ) );
                }
                THEN( "he still hasn't the recipe memorized" ) {
                    CHECK_FALSE( dummy.knows_recipe( r2 ) );
                }
            }
            AND_WHEN( "he gets rid of the tablet" ) {
                dummy.i_rem( &eink );

                THEN( "he cant make the recipe anymore" ) {
                    CHECK_FALSE( dummy.get_recipes_from_books( dummy.inv ).contains( r2 ) );
                }
            }
        }
    }
}

// This crashes subsequent testcases for some reason.
TEST_CASE( "crafting_with_a_companion", "[.]" )
{
    const recipe *r = &recipe_id( "brew_mead" ).obj();
    player dummy;

    REQUIRE( dummy.get_skill_level( r->skill_used ) == 0 );
    REQUIRE_FALSE( dummy.knows_recipe( r ) );
    REQUIRE( r->skill_used );

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

static void prep_craft( const recipe_id &rid, const std::vector<item> tools,
                        bool expect_craftable )
{
    clear_player();
    clear_map();

    const tripoint test_origin( 60, 60, 0 );
    g->u.setpos( test_origin );
    const item backpack( "backpack" );
    g->u.wear( g->u.i_add( backpack ), false );
    for( const item gear : tools ) {
        g->u.i_add( gear );
    }

    const recipe &r = rid.obj();

    const requirement_data &reqs = r.requirements();
    inventory crafting_inv = g->u.crafting_inventory();
    bool can_craft = reqs.can_make_with_inventory( g->u.crafting_inventory() );
    CHECK( can_craft == expect_craftable );
}

// This fakes a craft in a reasonable way which is fast
static void fake_test_craft( const recipe_id &rid, const std::vector<item> tools,
                             bool expect_craftable )
{
    prep_craft( rid, tools, expect_craftable );
    if( expect_craftable ) {
        g->u.consume_components_for_craft( rid.obj(), 1 );
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
        tools.emplace_back( "motor_tiny" );
        tools.emplace_back( "power_supply" );
        tools.emplace_back( "scrap" );

        fake_test_craft( recipe_id( "carver_off" ), tools, true );
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
        tools.emplace_back( "motor_tiny" );
        tools.emplace_back( "power_supply" );
        tools.emplace_back( "scrap" );

        fake_test_craft( recipe_id( "carver_off" ), tools, true );
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
        tools.emplace_back( "motor_tiny" );
        tools.emplace_back( "power_supply" );
        tools.emplace_back( "scrap" );
        tools.emplace_back( "UPS_off", -1, 500 );

        fake_test_craft( recipe_id( "carver_off" ), tools, true );
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
        tools.emplace_back( "motor_tiny" );
        tools.emplace_back( "power_supply" );
        tools.emplace_back( "scrap" );
        tools.emplace_back( "UPS_off", -1, 10 );

        fake_test_craft( recipe_id( "carver_off" ), tools, false );
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

        fake_test_craft( recipe_id( "water_clean" ), tools, true );
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

        fake_test_craft( recipe_id( "water_clean" ), tools, false );
    }
}

static constexpr int midnight = HOURS( 0 );
static constexpr int midday = HOURS( 12 );

static void set_time( int time )
{
    calendar::turn = time;
    g->reset_light_level();
    int z = g->u.posz();
    g->m.update_visibility_cache( z );
    g->m.build_map_cache( z );
}

// This tries to actually run the whole craft activity, which is more thorough,
// but slow
static int actually_test_craft( const recipe_id &rid, const std::vector<item> tools,
                                int interrupt_after_turns )
{
    prep_craft( rid, tools, true );
    set_time( midday ); // Ensure light for crafting
    const recipe &rec = rid.obj();
    REQUIRE( g->u.morale_crafting_speed_multiplier( rec ) == 1.0 );
    REQUIRE( g->u.lighting_craft_speed_multiplier( rec ) == 1.0 );
    REQUIRE( !g->u.activity );
    g->u.make_craft( rid, 1 );
    CHECK( g->u.activity );
    CHECK( g->u.activity.id() == activity_id( "ACT_CRAFT" ) );
    int turns = 0;
    while( g->u.activity.id() == activity_id( "ACT_CRAFT" ) ) {
        if( turns >= interrupt_after_turns ) {
            set_time( midnight ); // Kill light to interrupt crafting
        }
        ++turns;
        g->u.moves = 100;
        g->u.activity.do_turn( g->u );
    }
    return turns;
}

static void verify_inventory( const std::vector<std::string> &has,
                              const std::vector<std::string> &hasnt )
{
    std::ostringstream os;
    os << "Inventory:\n";
    for( const item *i : g->u.inv_dump() ) {
        os << "  " << i->typeId() << " (" << i->charges << ")\n";
    }
    INFO( os.str() );
    for( const std::string &i : has ) {
        INFO( "expecting " << i );
        CHECK( player_has_item_of_type( i ) );
    }
    for( const std::string &i : hasnt ) {
        INFO( "not expecting " << i );
        CHECK( !player_has_item_of_type( i ) );
    }
}

TEST_CASE( "crafting_interruption" )
{
    std::vector<item> tools;
    tools.emplace_back( "hammer" );
    tools.emplace_back( "scrap", -1, 1 );
    recipe_id test_recipe( "crude_picklock" );
    int time_taken = test_recipe->batch_time( 1, 1, 0 );
    int expected_turns_taken = divide_round_up( time_taken, 100 );
    REQUIRE( expected_turns_taken > 2 );

    SECTION( "regular_craft" ) {
        int turns_taken = actually_test_craft( test_recipe, tools, INT_MAX );
        CHECK( turns_taken == expected_turns_taken );
        verify_inventory( { "crude_picklock" }, { "scrap" } );
    }
    SECTION( "interrupted_craft" ) {
        int turns_taken = actually_test_craft( test_recipe, tools, 2 );
        CHECK( turns_taken == 3 );
        verify_inventory( { "scrap" }, { "crude_picklock" } );
        SECTION( "resumed_craft" ) {
            turns_taken = actually_test_craft( test_recipe, tools, INT_MAX );
            CHECK( turns_taken == expected_turns_taken - 2 );
            verify_inventory( { "crude_picklock" }, { "scrap" } );
        }
    }
}
