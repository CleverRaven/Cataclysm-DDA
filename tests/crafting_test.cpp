#include <algorithm>
#include <climits>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "activity_type.h"
#include "avatar.h"
#include "calendar.h"
#include "cata_utility.h"
#include "cata_catch.h"
#include "character.h"
#include "game.h"
#include "inventory.h"
#include "item.h"
#include "item_pocket.h"
#include "itype.h"
#include "map.h"
#include "map_helpers.h"
#include "npc.h"
#include "optional.h"
#include "pimpl.h"
#include "player_activity.h"
#include "player_helpers.h"
#include "point.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "requirements.h"
#include "ret_val.h"
#include "skill.h"
#include "temp_crafting_inventory.h"
#include "type_id.h"
#include "value_ptr.h"

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
                const auto comp_recipes( subset.of_component( itype_id( "water" ) ) );

                CHECK( comp_recipes.size() == 1 );
                CHECK( comp_recipes.find( r ) != comp_recipes.end() );
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
        dummy.worn.emplace_back( "backpack" );
        item &craftbook = dummy.i_add( item( "manual_electronics" ) );
        REQUIRE( craftbook.is_book() );
        REQUIRE_FALSE( craftbook.type->book->recipes.empty() );
        REQUIRE_FALSE( dummy.knows_recipe( r ) );

        WHEN( "the player read it and has an appropriate skill" ) {
            dummy.do_read( craftbook );
            dummy.set_skill_level( r->skill_used, 2 );
            // Secondary skills are just set to be what the autolearn requires
            // but the primary is not
            for( const std::pair<const skill_id, int> &skl : r->required_skills ) {
                dummy.set_skill_level( skl.first, skl.second );
            }

            AND_WHEN( "he searches for the recipe in the book" ) {
                THEN( "he finds it!" ) {
                    // update the crafting inventory cache
                    dummy.moves++;
                    CHECK( dummy.get_recipes_from_books( dummy.crafting_inventory() ).contains( r ) );
                }
                THEN( "it's easier in the book" ) {
                    // update the crafting inventory cache
                    dummy.moves++;
                    CHECK( dummy.get_recipes_from_books( dummy.crafting_inventory() ).get_custom_difficulty( r ) == 2 );
                }
                THEN( "he still hasn't the recipe memorized" ) {
                    CHECK_FALSE( dummy.knows_recipe( r ) );
                }
            }
            AND_WHEN( "he gets rid of the book" ) {
                dummy.i_rem( &craftbook );

                THEN( "he can't brew the recipe anymore" ) {
                    // update the crafting inventory cache
                    dummy.moves++;
                    CHECK_FALSE( dummy.get_recipes_from_books( dummy.crafting_inventory() ).contains( r ) );
                }
            }
        }
    }

    GIVEN( "an eink pc with a sushi recipe" ) {
        const recipe *r2 = &recipe_id( "sushi_rice" ).obj();
        dummy.worn.emplace_back( "backpack" );
        item &eink = dummy.i_add( item( "eink_tablet_pc" ) );
        eink.set_var( "EIPC_RECIPES", ",sushi_rice," );
        REQUIRE_FALSE( dummy.knows_recipe( r2 ) );

        WHEN( "the player holds it and has an appropriate skill" ) {
            dummy.set_skill_level( r2->skill_used, 2 );

            AND_WHEN( "he searches for the recipe in the tablet" ) {
                THEN( "he finds it!" ) {
                    // update the crafting inventory cache
                    dummy.moves++;
                    CHECK( dummy.get_recipes_from_books( dummy.crafting_inventory() ).contains( r2 ) );
                }
                THEN( "he still hasn't the recipe memorized" ) {
                    CHECK_FALSE( dummy.knows_recipe( r2 ) );
                }
            }
            AND_WHEN( "he gets rid of the tablet" ) {
                dummy.i_rem( &eink );

                THEN( "he can't make the recipe anymore" ) {
                    // update the crafting inventory cache
                    dummy.moves++;
                    CHECK_FALSE( dummy.get_recipes_from_books( dummy.crafting_inventory() ).contains( r2 ) );
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
        // update the crafting inventory cache
        dummy.moves++;
        REQUIRE_FALSE( dummy.get_available_recipes( dummy.crafting_inventory(), &helpers ).contains( r ) );
        REQUIRE_FALSE( who.knows_recipe( r ) );

        WHEN( "you have the required skill" ) {
            dummy.set_skill_level( r->skill_used, r->difficulty );

            AND_WHEN( "he knows the recipe" ) {
                who.learn_recipe( r );

                THEN( "he helps you" ) {
                    // update the crafting inventory cache
                    dummy.moves++;
                    CHECK( dummy.get_available_recipes( dummy.crafting_inventory(), &helpers ).contains( r ) );
                }
            }
            AND_WHEN( "he has the cookbook in his inventory" ) {
                item &cookbook = who.i_add( item( "brewing_cookbook" ) );

                REQUIRE( cookbook.is_book() );
                REQUIRE_FALSE( cookbook.type->book->recipes.empty() );

                THEN( "he shows it to you" ) {
                    // update the crafting inventory cache
                    dummy.moves++;
                    CHECK( dummy.get_available_recipes( dummy.crafting_inventory(), &helpers ).contains( r ) );
                }
            }
        }
    }
}

static void give_tools( const std::vector<item> &tools )
{
    Character &player_character = get_player_character();
    player_character.worn.clear();
    player_character.calc_encumbrance();
    player_character.inv->clear();
    player_character.remove_weapon();
    const item backpack( "debug_backpack" );
    player_character.worn.push_back( backpack );

    for( const item &gear : tools ) {
        player_character.i_add( gear );
    }
}

static void grant_skills_to_character( Character &you, const recipe &r )
{
    // Ensure adequate skill for all "required" skills
    for( const std::pair<const skill_id, int> &skl : r.required_skills ) {
        you.set_skill_level( skl.first, skl.second );
    }
    // and just in case "used" skill difficulty is higher, set that too
    you.set_skill_level( r.skill_used, std::max( r.difficulty,
                         you.get_skill_level( r.skill_used ) ) );
}

static void prep_craft( const recipe_id &rid, const std::vector<item> &tools,
                        bool expect_craftable )
{
    clear_avatar();
    clear_map();

    const tripoint test_origin( 60, 60, 0 );
    Character &player_character = get_player_character();
    player_character.toggle_trait( trait_id( "DEBUG_CNF" ) );
    player_character.setpos( test_origin );
    const recipe &r = rid.obj();
    grant_skills_to_character( player_character, r );

    give_tools( tools );
    player_character.moves--;
    const inventory &crafting_inv = player_character.crafting_inventory();

    bool can_craft_with_crafting_inv = r.deduped_requirements().can_make_with_inventory(
                                           crafting_inv, r.get_component_filter() );
    REQUIRE( can_craft_with_crafting_inv == expect_craftable );
    bool can_craft_with_temp_inv = r.deduped_requirements().can_make_with_inventory(
                                       temp_crafting_inventory( crafting_inv ), r.get_component_filter() );
    REQUIRE( can_craft_with_temp_inv == expect_craftable );
}

static time_point midnight = calendar::turn_zero + 0_hours;
static time_point midday = calendar::turn_zero + 12_hours;

static void set_time( const time_point &time )
{
    calendar::turn = time;
    g->reset_light_level();
    int z = get_player_character().posz();
    map &here = get_map();
    here.update_visibility_cache( z );
    here.invalidate_map_cache( z );
    here.build_map_cache( z );
}

// This tries to actually run the whole craft activity, which is more thorough,
// but slow
static int actually_test_craft( const recipe_id &rid, int interrupt_after_turns,
                                int skill_level = -1 )
{
    set_time( midday ); // Ensure light for crafting
    avatar &player_character = get_avatar();
    const recipe &rec = rid.obj();
    REQUIRE( player_character.morale_crafting_speed_multiplier( rec ) == 1.0 );
    REQUIRE( player_character.lighting_craft_speed_multiplier( rec ) == 1.0 );
    REQUIRE( !player_character.activity );

    // This really shouldn't be needed, but for some reason the tests fail for mingw builds without it
    player_character.learn_recipe( &rec );
    const inventory &inv = player_character.crafting_inventory();
    REQUIRE( player_character.has_recipe( &rec, inv,
                                          player_character.get_crafting_helpers() ) != -1 );
    player_character.remove_weapon();
    REQUIRE( !player_character.is_armed() );
    player_character.make_craft( rid, 1 );
    REQUIRE( player_character.activity );
    REQUIRE( player_character.activity.id() == activity_id( "ACT_CRAFT" ) );
    int turns = 0;
    while( player_character.activity.id() == activity_id( "ACT_CRAFT" ) ) {
        if( turns >= interrupt_after_turns ||
            ( skill_level >= 0 && player_character.get_skill_level( rec.skill_used ) > skill_level ) ) {
            set_time( midnight ); // Kill light to interrupt crafting
        }
        ++turns;
        player_character.moves = 100;
        player_character.activity.do_turn( player_character );
        if( turns % 60 == 0 ) {
            player_character.update_mental_focus();
        }
    }
    return turns;
}

TEST_CASE( "UPS shows as a crafting component", "[crafting][ups]" )
{
    avatar dummy;
    clear_character( dummy );
    dummy.worn.emplace_back( "backpack" );
    item &ups = dummy.i_add( item( "UPS_off", calendar::turn_zero, 500 ) );
    REQUIRE( dummy.has_item( ups ) );
    REQUIRE( ups.charges == 500 );
    REQUIRE( dummy.charges_of( itype_id( "UPS_off" ) ) == 500 );
    REQUIRE( dummy.charges_of( itype_id( "UPS" ) ) == 500 );
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
        tools.emplace_back( "vac_mold" );


        // Materials needed
        tools.insert( tools.end(), 10, item( "solder_wire" ) );
        tools.insert( tools.end(), 6, item( "plastic_chunk" ) );
        tools.insert( tools.end(), 2, item( "blade" ) );
        tools.insert( tools.end(), 5, item( "cable" ) );
        tools.insert( tools.end(), 2, item( "polycarbonate_sheet" ) );
        tools.insert( tools.end(), 1, item( "knife_paring" ) );
        tools.emplace_back( "motor_tiny" );
        tools.emplace_back( "power_supply" );
        tools.emplace_back( "scrap" );

        // Charges needed to craft:
        // - 10 charges of soldering iron
        // - 10 charges of surface heat

        WHEN( "each tool has enough charges" ) {
            item hotplate = tool_with_ammo( "hotplate", 30 );
            REQUIRE( hotplate.ammo_remaining() == 30 );
            tools.push_back( hotplate );
            item soldering = tool_with_ammo( "soldering_iron", 20 );
            REQUIRE( soldering.ammo_remaining() == 20 );
            tools.push_back( soldering );
            item plastic_molding = tool_with_ammo( "vac_mold", 4 );
            REQUIRE( plastic_molding.ammo_remaining() == 4 );
            tools.push_back( plastic_molding );

            THEN( "crafting succeeds, and uses charges from each tool" ) {
                prep_craft( recipe_id( "carver_off" ), tools, true );
                int turns = actually_test_craft( recipe_id( "carver_off" ), INT_MAX );
                CAPTURE( turns );
                CHECK( get_remaining_charges( "hotplate" ) == 10 );
                CHECK( get_remaining_charges( "soldering_iron" ) == 10 );
            }
        }

        WHEN( "multiple tools have enough combined charges" ) {
            tools.insert( tools.end(), 2, tool_with_ammo( "hotplate", 5 ) );
            tools.insert( tools.end(), 2, tool_with_ammo( "soldering_iron", 5 ) );
            tools.insert( tools.end(), 1, tool_with_ammo( "vac_mold", 4 ) );

            THEN( "crafting succeeds, and uses charges from multiple tools" ) {
                prep_craft( recipe_id( "carver_off" ), tools, true );
                actually_test_craft( recipe_id( "carver_off" ), INT_MAX );
                CHECK( get_remaining_charges( "hotplate" ) == 0 );
                CHECK( get_remaining_charges( "soldering_iron" ) == 0 );
            }
        }

        WHEN( "UPS-modded tools have enough charges" ) {
            item hotplate( "hotplate" );
            hotplate.put_in( item( "battery_ups" ), item_pocket::pocket_type::MOD );
            tools.push_back( hotplate );
            item soldering_iron( "soldering_iron" );
            soldering_iron.put_in( item( "battery_ups" ), item_pocket::pocket_type::MOD );
            tools.push_back( soldering_iron );
            item UPS( "UPS_off" );
            item UPS_mag( UPS.magazine_default() );
            UPS_mag.ammo_set( UPS_mag.ammo_default(), 510 );
            UPS.put_in( UPS_mag, item_pocket::pocket_type::MAGAZINE_WELL );
            tools.emplace_back( UPS );
            tools.push_back( tool_with_ammo( "vac_mold", 4 ) );

            THEN( "crafting succeeds, and uses charges from the UPS" ) {
                prep_craft( recipe_id( "carver_off" ), tools, true );
                actually_test_craft( recipe_id( "carver_off" ), INT_MAX );
                CHECK( get_remaining_charges( "hotplate" ) == 0 );
                CHECK( get_remaining_charges( "soldering_iron" ) == 0 );
                CHECK( get_remaining_charges( "UPS_off" ) == 480 );
            }
        }

        WHEN( "UPS-modded tools do not have enough charges" ) {
            item hotplate( "hotplate" );
            hotplate.put_in( item( "battery_ups" ), item_pocket::pocket_type::MOD );
            tools.push_back( hotplate );
            item soldering_iron( "soldering_iron" );
            soldering_iron.put_in( item( "battery_ups" ), item_pocket::pocket_type::MOD );
            tools.push_back( soldering_iron );
            tools.emplace_back( "UPS_off", calendar::turn_zero, 10 );

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
        tools.push_back( tool_with_ammo( "hotplate", 20 ) );
        item plastic_bottle( "bottle_plastic" );
        plastic_bottle.put_in(
            item( "water", calendar::turn_zero, 2 ), item_pocket::pocket_type::CONTAINER );
        tools.push_back( plastic_bottle );
        tools.emplace_back( "pot" );

        // Can't actually test crafting here since crafting a liquid currently causes a ui prompt
        prep_craft( recipe_id( "water_clean" ), tools, true );
    }
    SECTION( "clean_water_in_loaded_mess_kit" ) {
        std::vector<item> tools;
        tools.push_back( tool_with_ammo( "hotplate", 20 ) );
        item plastic_bottle( "bottle_plastic" );
        plastic_bottle.put_in(
            item( "water", calendar::turn_zero, 2 ), item_pocket::pocket_type::CONTAINER );
        tools.push_back( plastic_bottle );
        tools.push_back( tool_with_ammo( "mess_kit", 20 ) );

        // Can't actually test crafting here since crafting a liquid currently causes a ui prompt
        prep_craft( recipe_id( "water_clean" ), tools, true );
    }
    SECTION( "clean_water_in_loaded_survivor_mess_kit" ) {
        std::vector<item> tools;
        tools.push_back( tool_with_ammo( "hotplate", 20 ) );
        item plastic_bottle( "bottle_plastic" );
        plastic_bottle.put_in(
            item( "water", calendar::turn_zero, 2 ), item_pocket::pocket_type::CONTAINER );
        tools.push_back( plastic_bottle );
        tools.push_back( tool_with_ammo( "survivor_mess_kit", 20 ) );

        // Can't actually test crafting here since crafting a liquid currently causes a ui prompt
        prep_craft( recipe_id( "water_clean" ), tools, true );
    }
    SECTION( "clean_water_in_occupied_cooking_vessel" ) {
        std::vector<item> tools;
        tools.push_back( tool_with_ammo( "hotplate", 20 ) );
        item plastic_bottle( "bottle_plastic" );
        plastic_bottle.put_in(
            item( "water", calendar::turn_zero, 2 ), item_pocket::pocket_type::CONTAINER );
        tools.push_back( plastic_bottle );
        item jar( "jar_glass_sealed" );
        // If it's not watertight the water will spill.
        REQUIRE( jar.is_watertight_container() );
        jar.put_in( item( "water", calendar::turn_zero, 2 ), item_pocket::pocket_type::CONTAINER );
        tools.push_back( jar );

        prep_craft( recipe_id( "water_clean" ), tools, false );
    }
}

// Resume the first in progress craft found in the player's inventory
static int resume_craft()
{
    avatar &player_character = get_avatar();
    std::vector<item *> crafts = player_character.items_with( []( const item & itm ) {
        return itm.is_craft();
    } );
    REQUIRE( crafts.size() == 1 );
    item *craft = crafts.front();
    set_time( midday ); // Ensure light for crafting
    REQUIRE( player_character.crafting_speed_multiplier( *craft, cata::nullopt ) == 1.0 );
    REQUIRE( !player_character.activity );
    player_character.use( player_character.get_item_position( craft ) );
    REQUIRE( player_character.activity );
    REQUIRE( player_character.activity.id() == activity_id( "ACT_CRAFT" ) );
    int turns = 0;
    while( player_character.activity.id() == activity_id( "ACT_CRAFT" ) ) {
        ++turns;
        player_character.moves = 100;
        player_character.activity.do_turn( player_character );
        if( turns % 60 == 0 ) {
            player_character.update_mental_focus();
        }
    }
    return turns;
}

static void verify_inventory( const std::vector<std::string> &has,
                              const std::vector<std::string> &hasnt )
{
    std::ostringstream os;
    os << "Inventory:\n";
    Character &player_character = get_player_character();
    for( const item *i : player_character.inv_dump() ) {
        os << "  " << i->typeId().str() << " (" << i->charges << ")\n";
    }
    os << "Wielded:\n" << player_character.weapon.tname() << "\n";
    INFO( os.str() );
    for( const std::string &i : has ) {
        INFO( "expecting " << i );
        const bool has_item =
            player_has_item_of_type( i ) || player_character.weapon.type->get_id() == itype_id( i );
        REQUIRE( has_item );
    }
    for( const std::string &i : hasnt ) {
        INFO( "not expecting " << i );
        const bool hasnt_item =
            !player_has_item_of_type( i ) && !( player_character.weapon.type->get_id() == itype_id( i ) );
        REQUIRE( hasnt_item );
    }
}

TEST_CASE( "total crafting time with or without interruption", "[crafting][time][resume]" )
{
    GIVEN( "a recipe and all the required tools and materials to craft it" ) {
        recipe_id test_recipe( "razor_shaving" );
        int expected_time_taken = test_recipe->batch_time( get_player_character(), 1, 1, 0 );
        int expected_turns_taken = divide_round_up( expected_time_taken, 100 );

        std::vector<item> tools;
        tools.emplace_back( "pockknife" );

        // Will interrupt after 2 turns, so craft needs to take at least that long
        REQUIRE( expected_turns_taken > 2 );
        int actual_turns_taken;

        WHEN( "crafting begins, and continues until the craft is completed" ) {
            tools.emplace_back( "razor_blade", calendar::turn_zero, 1 );
            tools.emplace_back( "plastic_chunk", calendar::turn_zero, 1 );
            prep_craft( test_recipe, tools, true );
            actual_turns_taken = actually_test_craft( test_recipe, INT_MAX );

            THEN( "it should take the expected number of turns" ) {
                CHECK( actual_turns_taken == expected_turns_taken );

                AND_THEN( "the finished item should be in the inventory" ) {
                    verify_inventory( { "razor_shaving" }, { "razor_blade" } );
                }
            }
        }

        WHEN( "crafting begins, but is interrupted after 2 turns" ) {
            tools.emplace_back( "razor_blade", calendar::turn_zero, 1 );
            tools.emplace_back( "plastic_chunk", calendar::turn_zero, 1 );
            prep_craft( test_recipe, tools, true );
            actual_turns_taken = actually_test_craft( test_recipe, 2 );
            REQUIRE( actual_turns_taken == 3 );

            THEN( "the in-progress craft should be in the inventory" ) {
                verify_inventory( { "craft" }, { "razor_shaving" } );

                AND_WHEN( "crafting resumes until the craft is finished" ) {
                    actual_turns_taken = resume_craft();

                    THEN( "it should take the remaining number of turns" ) {
                        CHECK( actual_turns_taken == expected_turns_taken - 2 );

                        AND_THEN( "the finished item should be in the inventory" ) {
                            verify_inventory( { "razor_shaving" }, { "craft" } );
                        }
                    }
                }
            }
        }
    }
}

static std::map<quality_id, itype_id> quality_to_tool = {{
        { quality_id( "CUT" ), itype_id( "pockknife" ) }, { quality_id( "SEW" ), itype_id( "needle_bone" ) }, { quality_id( "LEATHER_AWL" ), itype_id( "awl_bone" ) }, { quality_id( "ANVIL" ), itype_id( "anvil" ) }, { quality_id( "HAMMER" ), itype_id( "hammer" ) }, { quality_id( "SAW_M" ), itype_id( "hacksaw" ) }, { quality_id( "CHISEL" ), itype_id( "chisel" ) }
    }
};

static void grant_proficiencies_to_character( Character &you, const recipe &r,
        bool grant_optional_proficiencies )
{
    if( grant_optional_proficiencies ) {
        for( const proficiency_id &prof : r.assist_proficiencies() ) {
            you.add_proficiency( prof, true );
        }
    } else {
        REQUIRE( you.known_proficiencies().empty() );
    }
    for( const proficiency_id &prof : r.required_proficiencies() ) {
        you.add_proficiency( prof, true );
    }
}

static void test_skill_progression( const recipe_id &test_recipe, int expected_turns_taken,
                                    int morale_level, bool grant_optional_proficiencies )
{
    Character &you = get_player_character();
    int actual_turns_taken = 0;
    const recipe &r = *test_recipe;
    const skill_id skill_used = r.skill_used;
    // Do we need to check required skills too?
    const int starting_skill_level = r.difficulty;
    std::vector<item> tools;
    const requirement_data &req = r.simple_requirements();
    for( const std::vector<tool_comp> &tool_list : req.get_tools() ) {
        for( const tool_comp &tool : tool_list ) {
            tools.push_back( tool_with_ammo( tool.type.str(), tool.count ) );
            break;
        }
    }
    for( const std::vector<quality_requirement> &qualities : req.get_qualities() ) {
        for( const quality_requirement &quality : qualities ) {
            const auto &tool_id = quality_to_tool.find( quality.type );
            CAPTURE( quality.type.str() );
            REQUIRE( tool_id != quality_to_tool.end() );
            tools.emplace_back( tool_id->second );
            break;
        }
    }
    for( const std::vector<item_comp> &components : req.get_components() ) {
        for( const item_comp &component : components ) {
            for( int i = 0; i < component.count * 2; ++i ) {
                tools.emplace_back( component.type );
            }
            break;
        }
    }

    prep_craft( test_recipe, tools, true );
    grant_proficiencies_to_character( you, r, grant_optional_proficiencies );
    you.set_focus( 100 );
    if( morale_level != 0 ) {
        you.add_morale( morale_type( "morale_food_good" ), morale_level );
        REQUIRE( you.get_morale_level() == morale_level );
    }
    SkillLevel &level = you.get_skill_level_object( skill_used );
    int previous_exercise = level.exercise( true );
    do {
        actual_turns_taken += actually_test_craft( test_recipe, INT_MAX, starting_skill_level );
        if( you.get_skill_level( skill_used ) == starting_skill_level ) {
            int new_exercise = level.exercise( true );
            REQUIRE( previous_exercise < new_exercise );
            previous_exercise = new_exercise;
        }
        give_tools( tools );
    } while( you.get_skill_level( skill_used ) == starting_skill_level );
    CAPTURE( test_recipe.str() );
    CAPTURE( expected_turns_taken );
    CAPTURE( grant_optional_proficiencies );
    CHECK( you.get_skill_level( skill_used ) == starting_skill_level + 1 );
    CHECK( actual_turns_taken == expected_turns_taken );
}

TEST_CASE( "crafting_skill_gain", "[skill],[crafting],[slow]" )
{
    SECTION( "lvl 0 -> 1" ) {
        GIVEN( "nominal morale" ) {
            test_skill_progression( recipe_id( "blanket" ), 175, 0, true );
        }
        GIVEN( "high morale" ) {
            test_skill_progression( recipe_id( "blanket" ), 173, 50, true );
        }
        GIVEN( "very high morale" ) {
            test_skill_progression( recipe_id( "blanket" ), 173, 100, true );
        }
    }
    SECTION( "lvl 1 -> 2" ) {
        GIVEN( "nominal morale" ) {
            test_skill_progression( recipe_id( "2byarm_guard" ), 2140, 0, true );
        }
        GIVEN( "high morale" ) {
            test_skill_progression( recipe_id( "2byarm_guard" ), 1842, 50, true );
        }
        GIVEN( "very high morale" ) {
            test_skill_progression( recipe_id( "2byarm_guard" ), 1737, 100, true );
        }
    }
    SECTION( "lvl 2 -> lvl 3" ) {
        GIVEN( "nominal morale" ) {
            test_skill_progression( recipe_id( "vambrace_larmor" ), 6291, 0, true );
        }
        GIVEN( "high morale" ) {
            test_skill_progression( recipe_id( "vambrace_larmor" ), 5230, 50, true );
        }
        GIVEN( "very high morale" ) {
            test_skill_progression( recipe_id( "vambrace_larmor" ), 4836, 100, true );
        }
    }
    SECTION( "lvl 3 -> lvl 4" ) {
        GIVEN( "nominal morale" ) {
            test_skill_progression( recipe_id( "armguard_larmor" ), 12138, 0, true );
        }
        GIVEN( "high morale" ) {
            test_skill_progression( recipe_id( "armguard_larmor" ), 10003, 50, true );
        }
        GIVEN( "very high morale" ) {
            test_skill_progression( recipe_id( "armguard_larmor" ), 9203, 100, true );
        }
    }
    SECTION( "lvl 4 -> 5" ) {
        GIVEN( "nominal morale" ) {
            test_skill_progression( recipe_id( "armguard_metal" ), 19638, 0, true );
        }
        GIVEN( "high morale" ) {
            test_skill_progression( recipe_id( "armguard_metal" ), 16125, 50, true );
        }
        GIVEN( "very high morale" ) {
            test_skill_progression( recipe_id( "armguard_metal" ), 14805, 100, true );
        }
    }
    SECTION( "lvl 5 -> 6" ) {
        GIVEN( "nominal morale" ) {
            test_skill_progression( recipe_id( "armguard_chitin" ), 28817, 0, true );
        }
        GIVEN( "high morale" ) {
            test_skill_progression( recipe_id( "armguard_chitin" ), 23613, 50, true );
        }
        GIVEN( "very high morale" ) {
            test_skill_progression( recipe_id( "armguard_chitin" ), 21651, 100, true );
        }
    }
    SECTION( "lvl 6 -> 7" ) {
        GIVEN( "nominal morale" ) {
            test_skill_progression( recipe_id( "armguard_acidchitin" ), 39651, 0, true );
        }
        GIVEN( "high morale" ) {
            test_skill_progression( recipe_id( "armguard_acidchitin" ), 32470, 50, true );
        }
        GIVEN( "very high morale" ) {
            test_skill_progression( recipe_id( "armguard_acidchitin" ), 29755, 100, true );
        }
    }
    SECTION( "lvl 7 -> 8" ) {
        GIVEN( "nominal morale" ) {
            test_skill_progression( recipe_id( "armguard_lightplate" ), 52138, 0, true );
        }
        GIVEN( "high morale" ) {
            test_skill_progression( recipe_id( "armguard_lightplate" ), 42656, 50, true );
        }
        GIVEN( "very high morale" ) {
            test_skill_progression( recipe_id( "armguard_lightplate" ), 39078, 100, true );
        }
    }
    SECTION( "lvl 8 -> 9" ) {
        GIVEN( "nominal morale" ) {
            test_skill_progression( recipe_id( "helmet_scavenger" ), 66243, 0, true );
        }
        GIVEN( "high morale" ) {
            test_skill_progression( recipe_id( "helmet_scavenger" ), 54170, 50, true );
        }
        GIVEN( "very high morale" ) {
            test_skill_progression( recipe_id( "helmet_scavenger" ), 49609, 100, true );
        }
    }
    SECTION( "lvl 9 -> 10" ) {
        GIVEN( "nominal morale" ) {
            test_skill_progression( recipe_id( "helmet_kabuto" ), 82489, 0, true );
        }
        GIVEN( "high morale" ) {
            test_skill_progression( recipe_id( "helmet_kabuto" ), 67364, 50, true );
        }
        GIVEN( "very high morale" ) {
            test_skill_progression( recipe_id( "helmet_kabuto" ), 61584, 100, true );
        }
    }
    SECTION( "long craft with proficiency delays" ) {
        GIVEN( "nominal morale" ) {
            test_skill_progression( recipe_id( "longbow" ), 71187, 0, false );
            test_skill_progression( recipe_id( "longbow" ), 28804, 0, true );
        }
        GIVEN( "high morale" ) {
            test_skill_progression( recipe_id( "longbow" ), 56945, 50, false );
            test_skill_progression( recipe_id( "longbow" ), 23608, 50, true );
        }
        GIVEN( "very high morale" ) {
            test_skill_progression( recipe_id( "longbow" ), 52222, 100, false );
            test_skill_progression( recipe_id( "longbow" ), 21651, 100, true );
        }
    }
    SECTION( "extremely short craft" ) {
        GIVEN( "nominal morale" ) {
            test_skill_progression( recipe_id( "fishing_hook_basic" ), 174, 0, true );
        }
        GIVEN( "high morale" ) {
            test_skill_progression( recipe_id( "fishing_hook_basic" ), 172, 50, true );
        }
        GIVEN( "very high morale" ) {
            test_skill_progression( recipe_id( "fishing_hook_basic" ), 172, 100, true );
        }
    }
}

TEST_CASE( "check-tool_qualities" )
{
    CHECK( tool_with_ammo( "mess_kit", 20 ).has_quality( quality_id( "BOIL" ), 2, 1 ) );
    CHECK( tool_with_ammo( "survivor_mess_kit", 20 ).has_quality( quality_id( "BOIL" ), 2, 1 ) );
    CHECK( tool_with_ammo( "survivor_mess_kit", 20 ).get_quality( quality_id( "BOIL" ) ) > 0 );
}

TEST_CASE( "book_proficiency_mitigation", "[crafting][proficiency]" )
{
    GIVEN( "a recipe with required proficiencies" ) {
        clear_avatar();
        clear_map();
        const recipe &test_recipe = *recipe_id( "leather_belt" );

        grant_skills_to_character( get_player_character(), test_recipe );
        int unmitigated_time_taken = test_recipe.batch_time( get_player_character(), 1, 1, 0 );

        WHEN( "player has a book mitigating lack of proficiency" ) {
            std::vector<item> books;
            books.emplace_back( "manual_tailor" );
            give_tools( books );
            get_player_character().invalidate_crafting_inventory();
            int mitigated_time_taken = test_recipe.batch_time( get_player_character(), 1, 1, 0 );
            THEN( "it takes less time to craft the recipe" ) {
                CHECK( mitigated_time_taken < unmitigated_time_taken );
            }
            AND_WHEN( "player acquires missing proficiencies" ) {
                grant_proficiencies_to_character( get_player_character(), test_recipe, true );
                int proficient_time_taken = test_recipe.batch_time( get_player_character(), 1, 1, 0 );
                THEN( "it takes even less time to craft the recipe" ) {
                    CHECK( proficient_time_taken < mitigated_time_taken );
                }
            }
        }
    }
}

TEST_CASE( "partial_proficiency_mitigation", "[crafting][proficiency]" )
{
    GIVEN( "a recipe with required proficiencies" ) {
        clear_avatar();
        clear_map();
        Character &tester = get_player_character();
        const recipe &test_recipe = *recipe_id( "leather_belt" );

        grant_skills_to_character( tester, test_recipe );
        int unmitigated_time_taken = test_recipe.batch_time( tester, 1, 1, 0 );

        WHEN( "player acquires partial proficiency" ) {
            int np = 0;
            for( const proficiency_id &prof : test_recipe.assist_proficiencies() ) {
                np++;
                tester.set_proficiency_practice( prof, tester.proficiency_training_needed( prof ) / 2 );
            }
            int mitigated_time_taken = test_recipe.batch_time( tester, 1, 1, 0 );
            THEN( "it takes less time to craft the recipe" ) {
                CHECK( mitigated_time_taken < unmitigated_time_taken );
            }
            AND_WHEN( "player acquires missing proficiencies" ) {
                grant_proficiencies_to_character( tester, test_recipe, true );
                int proficient_time_taken = test_recipe.batch_time( tester, 1, 1, 0 );
                THEN( "it takes even less time to craft the recipe" ) {
                    CHECK( proficient_time_taken < mitigated_time_taken );
                }
            }
        }
    }
}
