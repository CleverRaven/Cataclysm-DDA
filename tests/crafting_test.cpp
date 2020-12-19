#include <algorithm>
#include <climits>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "cata_utility.h"
#include "catch/catch.hpp"
#include "coordinate_conversions.h"
#include "crafting.h"
#include "distribution_grid.h"
#include "game.h"
#include "item.h"
#include "itype.h"
#include "map.h"
#include "map_helpers.h"
#include "npc.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "player_activity.h"
#include "player_helpers.h"
#include "point.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "requirements.h"
#include "string_id.h"
#include "type_id.h"
#include "value_ptr.h"

class inventory;

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
    const recipe *r = &recipe_id( "magazine_battery_light_mod" ).obj();
    avatar dummy;

    REQUIRE( dummy.get_skill_level( r->skill_used ) == 0 );
    REQUIRE_FALSE( dummy.knows_recipe( r ) );
    REQUIRE( r->skill_used );

    GIVEN( "a recipe that can be automatically learned" ) {
        WHEN( "the player has lower skill" ) {
            for( const std::pair<const skill_id, int> &skl : r->required_skills ) {
                dummy.set_skill_level( skl.first, skl.second - 1 );
            }

            THEN( "he can't craft it" ) {
                CHECK_FALSE( dummy.knows_recipe( r ) );
            }
        }
        WHEN( "the player has just the skill that's required" ) {
            dummy.set_skill_level( r->skill_used, r->difficulty );
            for( const std::pair<const skill_id, int> &skl : r->required_skills ) {
                dummy.set_skill_level( skl.first, skl.second );
            }

            THEN( "he can craft it now!" ) {
                CHECK( dummy.knows_recipe( r ) );

                AND_WHEN( "his skill rusts" ) {
                    dummy.set_skill_level( r->skill_used, 0 );
                    for( const std::pair<const skill_id, int> &skl : r->required_skills ) {
                        dummy.set_skill_level( skl.first, 0 );
                    }

                    THEN( "he still remembers how to craft it" ) {
                        CHECK( dummy.knows_recipe( r ) );
                    }
                }
            }
        }
    }

    GIVEN( "an appropriate book" ) {
        item &craftbook = dummy.i_add( item( "manual_electronics" ) );

        REQUIRE( craftbook.is_book() );
        REQUIRE_FALSE( craftbook.type->book->recipes.empty() );
        REQUIRE_FALSE( dummy.knows_recipe( r ) );

        WHEN( "the player read it and has an appropriate skill" ) {
            dummy.do_read( item_location( dummy, &craftbook ) );
            dummy.set_skill_level( r->skill_used, 2 );
            // Secondary skills are just set to be what the autolearn requires
            // but the primary is not
            for( const std::pair<const skill_id, int> &skl : r->required_skills ) {
                dummy.set_skill_level( skl.first, skl.second );
            }

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
                dummy.i_rem( &craftbook );

                THEN( "he can't brew the recipe anymore" ) {
                    CHECK_FALSE( dummy.get_recipes_from_books( dummy.inv ).contains( r ) );
                }
            }
        }
    }

    GIVEN( "an eink pc with a sushi recipe" ) {
        const recipe *r2 = &recipe_id( "sushi_rice" ).obj();
        item &eink = dummy.i_add( item( "eink_tablet_pc" ) );
        eink.set_var( "EIPC_RECIPES", ",sushi_rice," );
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

                THEN( "he can't make the recipe anymore" ) {
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
    avatar dummy;

    REQUIRE( dummy.get_skill_level( r->skill_used ) == 0 );
    REQUIRE_FALSE( dummy.knows_recipe( r ) );
    REQUIRE( r->skill_used );

    GIVEN( "a companion who can help with crafting" ) {
        standard_npc who( "helper" );

        who.set_attitude( NPCATT_FOLLOW );
        who.spawn_at_sm( tripoint_zero );

        g->load_npcs();

        CHECK( !dummy.in_vehicle );
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

static void prep_craft( const recipe_id &rid, const std::vector<item> &tools,
                        bool expect_craftable )
{
    clear_avatar();
    clear_map();

    const tripoint test_origin( 60, 60, 0 );
    g->u.setpos( test_origin );
    const item backpack( "backpack" );
    g->u.wear( g->u.i_add( backpack ), false );
    for( const item &gear : tools ) {
        g->u.i_add( gear );
    }

    const recipe &r = rid.obj();

    // Ensure adequate skill for all "required" skills
    for( const std::pair<const skill_id, int> &skl : r.required_skills ) {
        g->u.set_skill_level( skl.first, skl.second );
    }
    // and just in case "used" skill difficulty is higher, set that too
    g->u.set_skill_level( r.skill_used, std::max( r.difficulty,
                          g->u.get_skill_level( r.skill_used ) ) );

    const inventory &crafting_inv = g->u.crafting_inventory();
    bool can_craft = r.deduped_requirements().can_make_with_inventory(
                         crafting_inv, r.get_component_filter() );
    REQUIRE( can_craft == expect_craftable );
}

static time_point midnight = calendar::turn_zero + 0_hours;
static time_point midday = calendar::turn_zero + 12_hours;

// This tries to actually run the whole craft activity, which is more thorough,
// but slow
static int actually_test_craft( const recipe_id &rid, const std::vector<item> &tools,
                                int interrupt_after_turns )
{
    prep_craft( rid, tools, true );
    set_time( midday ); // Ensure light for crafting
    const recipe &rec = rid.obj();
    REQUIRE( g->u.morale_crafting_speed_multiplier( rec ) == 1.0 );
    REQUIRE( g->u.lighting_craft_speed_multiplier( rec ) == 1.0 );
    REQUIRE( !g->u.activity );

    // This really shouldn't be needed, but for some reason the tests fail for mingw builds without it
    g->u.learn_recipe( &rec );
    REQUIRE( g->u.has_recipe( &rec, g->u.crafting_inventory(), g->u.get_crafting_helpers() ) != -1 );

    g->u.make_craft( rid, 1 );
    REQUIRE( g->u.activity );
    REQUIRE( g->u.activity.id() == activity_id( "ACT_CRAFT" ) );
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

TEST_CASE( "tools use charge to craft", "[crafting][charge]" )
{
    std::vector<item> tools;

    GIVEN( "recipe and required tools/materials" ) {
        recipe_id carver( "carver_off" );
        // Uses fabrication skill
        // Requires electronics 3
        // Difficulty 4
        // Learned from advanced_electronics or textbook_electronics

        // Tools needed:
        tools.emplace_back( "screwdriver" );
        tools.emplace_back( "mold_plastic" );

        // Materials needed
        tools.insert( tools.end(), 10, item( "solder_wire" ) );
        tools.insert( tools.end(), 6, item( "plastic_chunk" ) );
        tools.insert( tools.end(), 2, item( "blade" ) );
        tools.insert( tools.end(), 5, item( "cable" ) );
        tools.emplace_back( "motor_tiny" );
        tools.emplace_back( "power_supply" );
        tools.emplace_back( "scrap" );

        // Charges needed to craft:
        // - 10 charges of soldering iron
        // - 10 charges of surface heat

        WHEN( "each tool has enough charges" ) {
            tools.emplace_back( "hotplate", -1, 20 );
            tools.emplace_back( "soldering_iron", -1, 20 );

            THEN( "crafting succeeds, and uses charges from each tool" ) {
                int turns = actually_test_craft( recipe_id( "carver_off" ), tools, INT_MAX );
                CAPTURE( turns );
                CHECK( get_remaining_charges( "hotplate" ) == 10 );
                CHECK( get_remaining_charges( "soldering_iron" ) == 10 );
            }
        }

        WHEN( "multiple tools have enough combined charges" ) {
            tools.emplace_back( "hotplate", -1, 5 );
            tools.emplace_back( "hotplate", -1, 5 );
            tools.emplace_back( "soldering_iron", -1, 5 );
            tools.emplace_back( "soldering_iron", -1, 5 );

            THEN( "crafting succeeds, and uses charges from multiple tools" ) {
                actually_test_craft( recipe_id( "carver_off" ), tools, INT_MAX );
                CHECK( get_remaining_charges( "hotplate" ) == 0 );
                CHECK( get_remaining_charges( "soldering_iron" ) == 0 );
            }
        }

        WHEN( "UPS-modded tools have enough charges" ) {
            item hotplate( "hotplate", -1, 0 );
            hotplate.put_in( item( "battery_ups" ) );
            tools.push_back( hotplate );
            item soldering_iron( "soldering_iron", -1, 0 );
            soldering_iron.put_in( item( "battery_ups" ) );
            tools.push_back( soldering_iron );
            tools.emplace_back( "UPS_off", -1, 500 );

            THEN( "crafting succeeds, and uses charges from the UPS" ) {
                actually_test_craft( recipe_id( "carver_off" ), tools, INT_MAX );
                CHECK( get_remaining_charges( "hotplate" ) == 0 );
                CHECK( get_remaining_charges( "soldering_iron" ) == 0 );
                CHECK( get_remaining_charges( "UPS_off" ) == 480 );
            }
        }

        WHEN( "UPS-modded tools do not have enough charges" ) {
            item hotplate( "hotplate", -1, 0 );
            hotplate.put_in( item( "battery_ups" ) );
            tools.push_back( hotplate );
            item soldering_iron( "soldering_iron", -1, 0 );
            soldering_iron.put_in( item( "battery_ups" ) );
            tools.push_back( soldering_iron );
            tools.emplace_back( "UPS_off", -1, 10 );

            THEN( "crafting fails, and no charges are used" ) {
                prep_craft( recipe_id( "carver_off" ), tools, false );
                CHECK( get_remaining_charges( "UPS_off" ) == 10 );
            }
        }
    }
}

TEST_CASE( "tool_use", "[crafting][tool]" )
{
    SECTION( "clean_water" ) {
        std::vector<item> tools;
        tools.emplace_back( "hotplate", -1, 20 );
        item plastic_bottle( "bottle_plastic" );
        plastic_bottle.put_in( item( "water", -1, 2 ) );
        tools.push_back( plastic_bottle );
        tools.emplace_back( "pot" );

        // Can't actually test crafting here since crafting a liquid currently causes a ui prompt
        prep_craft( recipe_id( "water_clean" ), tools, true );
    }
    SECTION( "clean_water_in_occupied_cooking_vessel" ) {
        std::vector<item> tools;
        tools.emplace_back( "hotplate", -1, 20 );
        item plastic_bottle( "bottle_plastic" );
        plastic_bottle.put_in( item( "water", -1, 2 ) );
        tools.push_back( plastic_bottle );
        item jar( "jar_glass" );
        // If it's not watertight the water will spill.
        REQUIRE( jar.is_watertight_container() );
        jar.put_in( item( "water", -1, 2 ) );
        tools.push_back( jar );

        prep_craft( recipe_id( "water_clean" ), tools, false );
    }
}

// Resume the first in progress craft found in the player's inventory
static int resume_craft()
{
    std::vector<item *> crafts = g->u.items_with( []( const item & itm ) {
        return itm.is_craft();
    } );
    REQUIRE( crafts.size() == 1 );
    item *craft = crafts.front();
    set_time( midday ); // Ensure light for crafting
    REQUIRE( crafting_speed_multiplier( g->u, *craft, bench_location{bench_type::hands, g->u.pos()} ) ==
             1.0 );
    REQUIRE( !g->u.activity );
    g->u.use( g->u.get_item_position( craft ) );
    REQUIRE( g->u.activity );
    REQUIRE( g->u.activity.id() == activity_id( "ACT_CRAFT" ) );
    int turns = 0;
    while( g->u.activity.id() == activity_id( "ACT_CRAFT" ) ) {
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
    os << "Wielded:\n" << g->u.weapon.tname() << "\n";
    INFO( os.str() );
    for( const std::string &i : has ) {
        INFO( "expecting " << i );
        const bool has_item = player_has_item_of_type( i ) || g->u.weapon.type->get_id() == i;
        REQUIRE( has_item );
    }
    for( const std::string &i : hasnt ) {
        INFO( "not expecting " << i );
        const bool hasnt_item = !player_has_item_of_type( i ) && !( g->u.weapon.type->get_id() == i );
        REQUIRE( hasnt_item );
    }
}

TEST_CASE( "total crafting time with or without interruption", "[crafting][time][resume]" )
{
    GIVEN( "a recipe and all the required tools and materials to craft it" ) {
        recipe_id test_recipe( "crude_picklock" );
        int expected_time_taken = test_recipe->batch_time( 1, 1, 0 );
        int expected_turns_taken = divide_round_up( expected_time_taken, 100 );

        std::vector<item> tools;
        tools.emplace_back( "hammer" );

        // Will interrupt after 2 turns, so craft needs to take at least that long
        REQUIRE( expected_turns_taken > 2 );
        int actual_turns_taken;

        WHEN( "crafting begins, and continues until the craft is completed" ) {
            tools.emplace_back( "scrap", -1, 1 );
            actual_turns_taken = actually_test_craft( test_recipe, tools, INT_MAX );

            THEN( "it should take the expected number of turns" ) {
                CHECK( actual_turns_taken == expected_turns_taken );

                AND_THEN( "the finished item should be in the inventory" ) {
                    verify_inventory( { "crude_picklock" }, { "scrap" } );
                }
            }
        }

        WHEN( "crafting begins, but is interrupted after 2 turns" ) {
            tools.emplace_back( "scrap", -1, 1 );
            actual_turns_taken = actually_test_craft( test_recipe, tools, 2 );
            REQUIRE( actual_turns_taken == 3 );

            THEN( "the in-progress craft should be in the inventory" ) {
                verify_inventory( { "craft" }, { "crude_picklock" } );

                AND_WHEN( "crafting resumes until the craft is finished" ) {
                    actual_turns_taken = resume_craft();

                    THEN( "it should take the remaining number of turns" ) {
                        CHECK( actual_turns_taken == expected_turns_taken - 2 );

                        AND_THEN( "the finished item should be in the inventory" ) {
                            verify_inventory( { "crude_picklock" }, { "craft" } );
                        }
                    }
                }
            }
        }
    }
}

#include "output.h"
TEST_CASE( "oven electric grid test", "[crafting][overmap][grids][slow]" )
{
    constexpr tripoint start_pos = tripoint( 60, 60, 0 );
    g->u.setpos( start_pos );
    clear_avatar();
    clear_map();
    GIVEN( "player is near an oven on an electric grid with a battery on it" ) {
        auto om = overmap_buffer.get_om_global( sm_to_omt_copy( g->m.getabs( g->u.pos() ) ) );
        // TODO: That's a lot of setup, implying barely testable design
        om.om->set_electric_grid_connections( om.local, {} );
        // Mega ugly
        g->load_map( g->m.get_abs_sub() );
        g->m.furn_set( start_pos + point( 10, 0 ), furn_str_id( "f_battery" ) );
        g->m.furn_set( start_pos + point( 1, 0 ), furn_str_id( "f_oven" ) );

        distribution_grid &grid = g->m.distribution_grid_at( start_pos + point( 10, 0 ) );
        REQUIRE( &grid == &g->m.distribution_grid_at( start_pos + point( 1, 0 ) ) );
        WHEN( "the grid is charged with 10 units of power" ) {
            REQUIRE( grid.get_resource() == 0 );
            grid.mod_resource( 10 );
            REQUIRE( grid.get_resource() == 10 );
            AND_WHEN( "crafting inventory is built" ) {
                g->u.invalidate_crafting_inventory();
                const inventory &crafting_inv = g->u.crafting_inventory();
                THEN( "it contains an oven item with 10 charges" ) {
                    REQUIRE( crafting_inv.has_charges( itype_id( "fake_oven" ), 10 ) );
                }
            }

            AND_WHEN( "the player is near a pot and a bottle of water" ) {
                g->u.invalidate_crafting_inventory();
                g->m.add_item( g->u.pos(), item( "pot" ) );
                g->m.add_item( g->u.pos(), item( "water" ).in_its_container() );
                THEN( "clean water can be crafted" ) {
                    const recipe &r = *recipe_id( "water_clean" );
                    const inventory &crafting_inv = g->u.crafting_inventory();
                    bool can_craft = r.deduped_requirements().can_make_with_inventory(
                                         crafting_inv, r.get_component_filter() );
                    REQUIRE( can_craft );
                }
            }
        }
    }
}
