#include <algorithm>
#include <climits>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <optional>
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

static const activity_id ACT_CRAFT( "ACT_CRAFT" );

static const flag_id json_flag_ITEM_BROKEN( "ITEM_BROKEN" );
static const flag_id json_flag_USE_UPS( "USE_UPS" );

static const itype_id itype_awl_bone( "awl_bone" );
static const itype_id itype_candle( "candle" );
static const itype_id itype_cash_card( "cash_card" );
static const itype_id itype_chisel( "chisel" );
static const itype_id itype_fake_anvil( "fake_anvil" );
static const itype_id itype_hacksaw( "hacksaw" );
static const itype_id itype_hammer( "hammer" );
static const itype_id itype_kevlar_shears( "kevlar_shears" );
static const itype_id itype_pockknife( "pockknife" );
static const itype_id itype_sewing_kit( "sewing_kit" );
static const itype_id itype_sheet_cotton( "sheet_cotton" );
static const itype_id itype_test_cracklins( "test_cracklins" );
static const itype_id itype_test_gum( "test_gum" );
static const itype_id itype_thread( "thread" );
static const itype_id itype_water_clean( "water_clean" );

static const morale_type morale_food_good( "morale_food_good" );

static const proficiency_id proficiency_prof_carving( "prof_carving" );

static const quality_id qual_ANVIL( "ANVIL" );
static const quality_id qual_BOIL( "BOIL" );
static const quality_id qual_CHISEL( "CHISEL" );
static const quality_id qual_CUT( "CUT" );
static const quality_id qual_FABRIC_CUT( "FABRIC_CUT" );
static const quality_id qual_HAMMER( "HAMMER" );
static const quality_id qual_LEATHER_AWL( "LEATHER_AWL" );
static const quality_id qual_SAW_M( "SAW_M" );
static const quality_id qual_SEW( "SEW" );

static const recipe_id recipe_2byarm_guard( "2byarm_guard" );
static const recipe_id recipe_armguard_acidchitin( "armguard_acidchitin" );
static const recipe_id recipe_armguard_chitin( "armguard_chitin" );
static const recipe_id recipe_armguard_larmor( "armguard_larmor" );
static const recipe_id recipe_armguard_lightplate( "armguard_lightplate" );
static const recipe_id recipe_armguard_metal( "armguard_metal" );
static const recipe_id recipe_balclava( "balclava" );
static const recipe_id recipe_blanket( "blanket" );
static const recipe_id recipe_brew_mead( "brew_mead" );
static const recipe_id recipe_brew_rum( "brew_rum" );
static const recipe_id recipe_carver_off( "carver_off" );
static const recipe_id recipe_cudgel_simple( "cudgel_simple" );
static const recipe_id recipe_cudgel_slow( "cudgel_slow" );
static const recipe_id recipe_dry_meat( "dry_meat" );
static const recipe_id recipe_fishing_hook_basic( "fishing_hook_basic" );
static const recipe_id recipe_helmet_kabuto( "helmet_kabuto" );
static const recipe_id recipe_helmet_scavenger( "helmet_scavenger" );
static const recipe_id recipe_leather_belt( "leather_belt" );
static const recipe_id recipe_longbow( "longbow" );
static const recipe_id recipe_macaroni_cooked( "macaroni_cooked" );
static const recipe_id recipe_magazine_battery_light_mod( "magazine_battery_light_mod" );
static const recipe_id recipe_makeshift_funnel( "makeshift_funnel" );
static const recipe_id recipe_sushi_rice( "sushi_rice" );
static const recipe_id recipe_test_tallow( "test_tallow" );
static const recipe_id recipe_test_tallow2( "test_tallow2" );
static const recipe_id recipe_test_waist_apron_long( "test_waist_apron_long" );
static const recipe_id
recipe_test_waist_apron_long_pink_apron_cotton( "test_waist_apron_long_pink_apron_cotton" );
static const recipe_id recipe_vambrace_larmor( "vambrace_larmor" );
static const recipe_id recipe_water_clean( "water_clean" );

static const skill_id skill_fabrication( "fabrication" );
static const skill_id skill_survival( "survival" );

static const trait_id trait_DEBUG_CNF( "DEBUG_CNF" );

TEST_CASE( "recipe_subset" )
{
    recipe_subset subset;

    REQUIRE( subset.size() == 0 );
    GIVEN( "a recipe of rum" ) {
        const recipe *r = &recipe_brew_rum.obj();

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
            THEN( "it uses clean water" ) {
                const auto &comp_recipes( subset.of_component( itype_water_clean ) );

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
    const recipe *r = &recipe_magazine_battery_light_mod.obj();
    avatar dummy;

    REQUIRE( static_cast<int>( dummy.get_skill_level( r->skill_used ) ) == 0 );
    REQUIRE_FALSE( dummy.knows_recipe( r ) );
    REQUIRE( r->skill_used );

    GIVEN( "a recipe that can be automatically learned" ) {
        WHEN( "the player has lower skill" ) {
            for( const std::pair<const skill_id, int> &skl : r->required_skills ) {
                dummy.set_skill_level( skl.first, skl.second - 1 );
                dummy.set_knowledge_level( skl.first, skl.second - 1 );
            }

            THEN( "he can't craft it" ) {
                CHECK_FALSE( dummy.knows_recipe( r ) );
            }
        }
        WHEN( "the player has just the skill that's required" ) {
            dummy.set_skill_level( r->skill_used, r->difficulty );
            for( const std::pair<const skill_id, int> &skl : r->required_skills ) {
                dummy.set_skill_level( skl.first, skl.second );
                dummy.set_knowledge_level( skl.first, skl.second );
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
        dummy.worn.wear_item( dummy, item( "backpack" ), false, false );
        item_location craftbook = dummy.i_add( item( "manual_electronics" ) );
        REQUIRE( craftbook->is_book() );
        REQUIRE_FALSE( craftbook->type->book->recipes.empty() );
        REQUIRE_FALSE( dummy.knows_recipe( r ) );

        WHEN( "the player read it and has an appropriate skill" ) {
            dummy.identify( *craftbook );
            dummy.set_knowledge_level( r->skill_used, 2 );
            // Secondary skills are just set to be what the autolearn requires
            // but the primary is not
            for( const std::pair<const skill_id, int> &skl : r->required_skills ) {
                dummy.set_knowledge_level( skl.first, skl.second );
            }

            AND_WHEN( "he searches for the recipe in the book" ) {
                THEN( "he finds it!" ) {
                    dummy.invalidate_crafting_inventory();
                    CHECK( dummy.get_recipes_from_books( dummy.crafting_inventory() ).contains( r ) );
                }
                THEN( "it's easier in the book" ) {
                    dummy.invalidate_crafting_inventory();
                    CHECK( dummy.get_recipes_from_books( dummy.crafting_inventory() ).get_custom_difficulty( r ) == 2 );
                }
                THEN( "he still hasn't the recipe memorized" ) {
                    CHECK_FALSE( dummy.knows_recipe( r ) );
                }
            }
            AND_WHEN( "he gets rid of the book" ) {
                craftbook.remove_item();

                THEN( "he can't brew the recipe anymore" ) {
                    dummy.invalidate_crafting_inventory();
                    CHECK_FALSE( dummy.get_recipes_from_books( dummy.crafting_inventory() ).contains( r ) );
                }
            }
        }
    }

    GIVEN( "an eink pc with a sushi recipe" ) {
        const recipe *r2 = &recipe_id( recipe_sushi_rice ).obj();
        dummy.worn.wear_item( dummy, item( "backpack" ), false, false );
        item_location eink = dummy.i_add( item( "eink_tablet_pc" ) );
        eink->set_var( "EIPC_RECIPES", ",sushi_rice," );
        REQUIRE_FALSE( dummy.knows_recipe( r2 ) );

        WHEN( "the player holds it and has an appropriate skill" ) {
            dummy.set_knowledge_level( r2->skill_used, 2 );

            AND_WHEN( "he searches for the recipe in the tablet" ) {
                THEN( "he finds it!" ) {
                    dummy.invalidate_crafting_inventory();
                    CHECK( dummy.get_recipes_from_books( dummy.crafting_inventory() ).contains( r2 ) );
                }
                THEN( "he still hasn't the recipe memorized" ) {
                    CHECK_FALSE( dummy.knows_recipe( r2 ) );
                }
            }
            AND_WHEN( "he gets rid of the tablet" ) {
                eink.remove_item();

                THEN( "he can't make the recipe anymore" ) {
                    dummy.invalidate_crafting_inventory();
                    CHECK_FALSE( dummy.get_recipes_from_books( dummy.crafting_inventory() ).contains( r2 ) );
                }
            }
        }
    }
}

// This crashes subsequent testcases for some reason.
TEST_CASE( "crafting_with_a_companion", "[.]" )
{
    const recipe *r = &recipe_brew_mead.obj();
    avatar dummy;

    REQUIRE( static_cast<int>( dummy.get_skill_level( r->skill_used ) ) == 0 );
    REQUIRE_FALSE( dummy.knows_recipe( r ) );
    REQUIRE( r->skill_used );

    GIVEN( "a companion who can help with crafting" ) {
        standard_npc who( "helper" );

        who.set_attitude( NPCATT_FOLLOW );
        who.spawn_at_omt( tripoint_abs_omt( tripoint_zero ) );

        g->load_npcs();

        CHECK( !dummy.in_vehicle );
        dummy.setpos( who.pos() );
        const auto helpers( dummy.get_crafting_helpers() );

        REQUIRE( std::find( helpers.begin(), helpers.end(), &who ) != helpers.end() );
        dummy.invalidate_crafting_inventory();
        REQUIRE_FALSE( dummy.get_available_recipes( dummy.crafting_inventory(), &helpers ).contains( r ) );
        REQUIRE_FALSE( who.knows_recipe( r ) );

        WHEN( "you have the required skill" ) {
            dummy.set_skill_level( r->skill_used, r->difficulty );

            AND_WHEN( "he knows the recipe" ) {
                who.learn_recipe( r );

                THEN( "he helps you" ) {
                    dummy.invalidate_crafting_inventory();
                    CHECK( dummy.get_available_recipes( dummy.crafting_inventory(), &helpers ).contains( r ) );
                }
            }
            AND_WHEN( "he has the cookbook in his inventory" ) {
                item_location cookbook = who.i_add( item( "brewing_cookbook" ) );

                REQUIRE( cookbook->is_book() );
                REQUIRE_FALSE( cookbook->type->book->recipes.empty() );

                THEN( "he shows it to you" ) {
                    dummy.invalidate_crafting_inventory();
                    CHECK( dummy.get_available_recipes( dummy.crafting_inventory(), &helpers ).contains( r ) );
                }
            }
        }
    }
}

static void give_tools( const std::vector<item> &tools )
{
    Character &player_character = get_player_character();
    player_character.clear_worn();
    player_character.calc_encumbrance();
    player_character.inv->clear();
    player_character.remove_weapon();
    const item backpack( "debug_backpack" );
    player_character.worn.wear_item( player_character, backpack, false, false );

    std::vector<item> boil;
    for( const item &gear : tools ) {
        if( gear.get_quality( qual_BOIL, false ) == 0 ) {
            REQUIRE( player_character.i_add( gear ) );
        } else {
            boil.emplace_back( gear );
        }
    }
    // add BOIL tools later so that they don't contain anything
    for( const item &gear : boil ) {
        REQUIRE( player_character.i_add( gear ) );
    }
    player_character.invalidate_crafting_inventory();
}

static int apply_offset( int skill, int offset )
{
    return clamp( skill + offset, 0, MAX_SKILL );
}

static void grant_skills_to_character( Character &you, const recipe &r, int offset )
{
    // Ensure adequate skill for all "required" skills
    for( const std::pair<const skill_id, int> &skl : r.required_skills ) {
        you.set_skill_level( skl.first, apply_offset( skl.second, offset ) );
        you.set_knowledge_level( skl.first, apply_offset( skl.second, offset ) );
    }
    // and just in case "used" skill difficulty is higher, set that too
    int value = apply_offset( std::max( r.difficulty,
                                        static_cast<int>( you.get_skill_level( r.skill_used ) ) ), offset );
    you.set_skill_level( r.skill_used, value );
    you.set_knowledge_level( r.skill_used, value );
}

static void grant_profs_to_character( Character &you, const recipe &r )
{
    for( const recipe_proficiency &rprof : r.proficiencies ) {
        you.add_proficiency( rprof.id, true );
    }
}

static void prep_craft( const recipe_id &rid, const std::vector<item> &tools,
                        bool expect_craftable, int offset = 0, bool grant_profs = false )
{
    clear_avatar();
    clear_map();

    const tripoint test_origin( 60, 60, 0 );
    Character &player_character = get_player_character();
    player_character.toggle_trait( trait_DEBUG_CNF );
    player_character.setpos( test_origin );
    const recipe &r = rid.obj();
    grant_skills_to_character( player_character, r, offset );
    if( grant_profs ) {
        grant_profs_to_character( player_character, r );
    }

    give_tools( tools );
    const inventory &crafting_inv = player_character.crafting_inventory();

    bool can_craft_with_crafting_inv = r.deduped_requirements()
                                       .can_make_with_inventory( crafting_inv, r.get_component_filter() );
    const std::string missing_alt_reqs = enumerate_as_string( r.deduped_requirements().alternatives(),
    []( const requirement_data & rd ) {
        return string_format( "req id: '%s' missing: %s", rd.id().str(), rd.list_missing() );
    } );
    CAPTURE( missing_alt_reqs );
    REQUIRE( can_craft_with_crafting_inv == expect_craftable );
    bool can_craft_with_temp_inv = r.deduped_requirements()
                                   .can_make_with_inventory( temp_crafting_inventory( crafting_inv ), r.get_component_filter() );
    REQUIRE( can_craft_with_temp_inv == expect_craftable );
}

static time_point midnight = calendar::turn_zero + 0_hours;
static time_point midday = calendar::turn_zero + 12_hours;

static void setup_test_craft( const recipe_id &rid )
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
    REQUIRE( player_character.has_recipe( &rec, inv, player_character.get_crafting_helpers() ) );
    player_character.remove_weapon();
    REQUIRE( !player_character.is_armed() );
    player_character.make_craft( rid, 1 );
    REQUIRE( player_character.activity );
    REQUIRE( player_character.activity.id() == ACT_CRAFT );
    // Extra safety
    item_location wielded = player_character.get_wielded_item();
    REQUIRE( wielded );
}

// This tries to actually run the whole craft activity, which is more thorough,
// but slow
static int actually_test_craft( const recipe_id &rid, int interrupt_after_turns,
                                int skill_level = -1 )
{
    setup_test_craft( rid );
    avatar &player_character = get_avatar();

    int turns = 0;
    while( player_character.activity.id() == ACT_CRAFT ) {
        if( turns >= interrupt_after_turns ||
            ( skill_level >= 0 &&
              static_cast<int>( player_character.get_skill_level( rid->skill_used ) ) > skill_level ) ) {
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

static int test_craft_for_prof( const recipe_id &rid, const proficiency_id &prof,
                                float target_progress )
{
    setup_test_craft( rid );
    avatar &player_character = get_avatar();

    int turns = 0;
    while( player_character.activity.id() == ACT_CRAFT ) {
        if( player_character.get_proficiency_practice( prof ) >= target_progress ) {
            set_time( midnight );
        }

        player_character.moves = 100;
        player_character.set_focus( 100 );
        player_character.activity.do_turn( player_character );
        ++turns;
    }

    return turns;
}

// Test gaining proficiency by repeatedly crafting short recipe
TEST_CASE( "proficiency_gain_short_crafts", "[crafting][proficiency]" )
{
    std::vector<item> tools = { item( "2x4" ) };

    const recipe_id &rec = recipe_cudgel_simple;
    prep_craft( rec, tools, true );
    avatar &ch = get_avatar();
    // Set skill above requirement so that skill training doesn't steal any focus
    ch.set_skill_level( skill_fabrication, 1 );
    REQUIRE( rec->get_skill_cap() < static_cast<int>( ch.get_skill_level( rec->skill_used ) ) );

    REQUIRE( ch.get_proficiency_practice( proficiency_prof_carving ) == 0.0f );

    int turns_taken = 0;
    const int max_turns = 100'000;

    float time_malus = rec->proficiency_time_maluses( ch );

    // Proficiency progress is checked every 5% of craft progress, so up to 5% of one craft worth can be wasted depending on timing
    // Rounding effects account for another tiny bit
    time_duration overrun = time_duration::from_turns( 466 );

    do {
        turns_taken += test_craft_for_prof( rec, proficiency_prof_carving, 1.0f );
        give_tools( tools );

        // Escape door to avoid infinite loop if there is no progress
        REQUIRE( turns_taken < max_turns );
    } while( !ch.has_proficiency( proficiency_prof_carving ) );

    time_duration expected_time = proficiency_prof_carving->time_to_learn() * time_malus + overrun;
    CHECK( time_duration::from_turns( turns_taken ) == expected_time );
}

// Test gaining proficiency all at once after finishing 5% of a very long recipe
TEST_CASE( "proficiency_gain_long_craft", "[crafting][proficiency]" )
{
    std::vector<item> tools = { item( "2x4" ) };
    const recipe_id &rec = recipe_cudgel_slow;
    prep_craft( rec, tools, true );
    avatar &ch = get_avatar();
    // Set skill above requirement so that skill training doesn't steal any focus
    ch.set_skill_level( skill_fabrication, 1 );
    REQUIRE( rec->get_skill_cap() < static_cast<int>( ch.get_skill_level( rec->skill_used ) ) );
    REQUIRE( ch.get_proficiency_practice( proficiency_prof_carving ) == 0.0f );

    test_craft_for_prof( rec, proficiency_prof_carving, 1.0f );

    const item_location &craft = ch.get_wielded_item();

    // Check exactly one 5% tick has passed
    // 500k counter = 5% progress
    // If counter is 0, this means the craft finished before we gained the proficiency
    CHECK( craft->item_counter >= 500'000 );
    CHECK( craft->item_counter < 501'000 );
}

static float craft_aggregate_fail_chance( const recipe_id &rid )
{
    Character &player_character = get_player_character();
    int fails = 0;
    const int iterations = 100000;
    for( int i = 0; i < iterations; ++i ) {
        fails += player_character.crafting_success_roll( *rid ) < 1.f;
    }
    return 100.f * ( static_cast<float>( fails ) / iterations );
}

static std::pair<float, float> scen_fail_chance( const recipe_id &rid, int offset, bool has_profs )
{
    clear_avatar();
    avatar &player_character = get_avatar();
    const recipe &rec = *rid;
    grant_skills_to_character( player_character, rec, offset );
    if( has_profs ) {
        grant_profs_to_character( player_character, rec );
    }
    REQUIRE_FALSE( player_character.has_trait( trait_DEBUG_CNF ) );
    return std::pair<float, float>( ( 1.f - player_character.recipe_success_chance( *rid ) ) * 100.f,
                                    player_character.item_destruction_chance( *rid ) * 100.f );
}

TEST_CASE( "synthetic_recipe_fail_chances", "[synthetic][.][crafting]" )
{
    std::vector<std::pair<int, bool>> scens = {
        { -MAX_SKILL, false },
        { -2, false },
        { -1, false },
        { -1, true },
        { 0, false },
        { 0, true },
        { 1, false },
        { 1, true },
        { 2, false },
        { 2, true }
    };
    for( const std::pair<int, bool> &scen : scens ) {
        std::ofstream output;
        std::string filename = string_format( "recipe_fails_skill%d_%s.csv", scen.first,
                                              scen.second ? "has_profs" : "no_profs" );
        output.open( filename );
        REQUIRE( output.is_open() );
        output << "Recipe,Minor Fail Chance,Catastrophic Fail Chance\n";
        for( const std::pair<const recipe_id, recipe> &rec : recipe_dict ) {
            std::pair<float, float> ret = scen_fail_chance( rec.first, scen.first, scen.second );
            output << string_format( "%s,%g,%g\n", rec.first.str(), ret.first, ret.second );
        }
        std::cout << "Output written to " << filename << std::endl;
    }
}

static void test_fail_scenario( const std::string &scen, const recipe_id &rid, int offset,
                                bool has_profs, float expected )
{
    // In each of these, we modify the avatar, so make sure they start in a known state.
    clear_avatar();

    const recipe &rec = *rid;
    Character &player_character = get_player_character();
    // Grant the character skills and proficiencies as appropriate to the scenario
    grant_skills_to_character( player_character, rec, offset );
    if( has_profs ) {
        grant_profs_to_character( player_character, rec );
    }

    INFO( rid.str() );
    INFO( scen );

    // If they have the trait that prevents crafting failures, this test would be a little pointless
    REQUIRE_FALSE( player_character.has_trait( trait_DEBUG_CNF ) );
    // Ensure that they either have the profs we gave them, or no profs.
    if( has_profs ) {
        for( const recipe_proficiency &prof : rec.proficiencies ) {
            REQUIRE( player_character.has_proficiency( prof.id ) );
        }
    } else {
        REQUIRE( player_character.known_proficiencies().empty() );
    }
    // The skills that the character is expected to have are the skills the recipes uses + the offset
    std::map<skill_id, int> expected_skills = rec.required_skills;
    INFO( string_format( "primary_skill: %s",
                         remove_color_tags( rec.primary_skill_string( player_character ) ) ) );
    INFO( string_format( "required skills: %s",
                         remove_color_tags( rec.required_skills_string( player_character ) ) ) );
    // In case the difficulty and required skills for the primary skill mismatch
    expected_skills[rec.skill_used] = std::max( rec.difficulty, expected_skills[rec.skill_used] );
    for( const std::pair<const skill_id, SkillLevel> &skill : player_character.get_all_skills() ) {
        INFO( string_format( "skill %s: prac %d know %d", skill.first.str(), skill.second.level(),
                             skill.second.knowledgeLevel() ) );
        // Only apply to skills that we expect to have a value
        if( expected_skills.count( skill.first ) != 0 ) {
            expected_skills[skill.first] = clamp( expected_skills[skill.first] + offset, 0, MAX_SKILL );
        } else {
            expected_skills[skill.first] = 0;
        }

        REQUIRE( skill.second.level() == expected_skills[skill.first] );
        REQUIRE( skill.second.knowledgeLevel() == expected_skills[skill.first] );
    }

    // The chance of failing the craft based on doing the craft roll many times
    float chance = craft_aggregate_fail_chance( rid );
    // The chance of failing the craft as calculated by math
    float calculated = ( 1.f - player_character.recipe_success_chance( rec ) ) * 100;
    CHECK( chance == Approx( calculated ).margin( 1 ) );
    CHECK( chance == Approx( expected ).margin( 1 ) );
}

static void test_chances_for( const recipe_id &rid, float none, float underskill, float matched,
                              float matched_profs, float overskill, float overskill_profs, float extra_overskill )
{
    test_fail_scenario( "The character has no skills or proficiencies", rid, -MAX_SKILL, false, none );
    test_fail_scenario( "The character is one level underskilled and lacks proficiencies", rid, -1,
                        false, underskill );
    test_fail_scenario( "The character has appropriate skills but lacks proficiencies", rid, 0, false,
                        matched );
    test_fail_scenario( "The character has appropriate skills and proficiencies", rid, 0, true,
                        matched_profs );
    test_fail_scenario( "The character is one level overskilled and lacks proficiencies", rid, 1, false,
                        overskill );
    test_fail_scenario( "The character is one level overskilled and has proficiencies", rid, 1, true,
                        overskill_profs );
    test_fail_scenario( "The character is two levels overskilled and has proficiencies", rid, 2, true,
                        extra_overskill );
}

TEST_CASE( "crafting_failure_rates_match_calculated", "[crafting][random]" )
{
    // NOLINTs to avoid very long variable names, and this test is meant for game (src) code more than tests
    // NOLINTNEXTLINE(cata-static-string_id-constants
    const recipe_id makeshift_crowbar( "makeshift_crowbar_test_no_tools" );
    // NOLINTNEXTLINE(cata-static-string_id-constants
    const recipe_id meat_cooked( "meat_cooked_test_no_tools" );
    // NOLINTNEXTLINE(cata-static-string_id-constants
    const recipe_id club_wooden_large( "club_wooden_large_test_no_tools" );
    // NOLINTNEXTLINE(cata-static-string_id-constants
    const recipe_id nailboard( "nailboard_test_no_tools" );
    // NOLINTNEXTLINE(cata-static-string_id-constants
    const recipe_id cudgel( "cudgel_test_no_tools" );
    // NOLINTNEXTLINE(cata-static-string_id-constants
    const recipe_id pumpkin_muffins( "pumpkin_muffins_test_no_tools" );
    // NOLINTNEXTLINE(cata-static-string_id-constants
    const recipe_id bp_40fmj( "bp_40fmj_test_no_tools" );
    // NOLINTNEXTLINE(cata-static-string_id-constants
    const recipe_id armor_qt_lightplate( "armor_qt_lightplate_test_no_tools" );

    // Skill zero recipes
    test_chances_for( makeshift_crowbar, 50.f, 50.f, 50.f, 50.f, 21.19f, 21.19f, 2.28f );
    test_chances_for( meat_cooked, 50.f, 50.f, 50.f, 50.f, 21.19f, 21.19f, 2.28f );
    test_chances_for( club_wooden_large, 50.f, 50.f, 50.f, 50.f, 21.19f, 21.19f, 2.28f );
    test_chances_for( nailboard, 50.f, 50.f, 50.f, 50.f, 21.19f, 21.19f, 2.28f );
    // Recipes requring various degrees of skill and proficiencies
    test_chances_for( cudgel, 82.5, 72.f, 50.f, 50.f, 21.f, 21.f, 2.25 );
    test_chances_for( pumpkin_muffins, 92.5, 82.f, 67.f, 50.f, 43.f, 21.f, 2.25 );
    test_chances_for( bp_40fmj, 94.5, 79.f, 62.f, 50.f, 36.f, 21.f, 2.25 );
    test_chances_for( armor_qt_lightplate, 98.f, 92.5, 89.f, 50.f, 81.f, 21.f, 2.25 );
}

TEST_CASE( "UPS_shows_as_a_crafting_component", "[crafting][ups]" )
{
    avatar dummy;
    clear_character( dummy );
    dummy.worn.wear_item( dummy, item( "backpack" ), false, false );
    item_location ups = dummy.i_add( item( "UPS_ON" ) );
    item ups_mag( ups->magazine_default() );
    ups_mag.ammo_set( ups_mag.ammo_default(), 500 );
    ret_val<void> result = ups->put_in( ups_mag, item_pocket::pocket_type::MAGAZINE_WELL );
    INFO( result.c_str() );
    REQUIRE( result.success() );
    REQUIRE( dummy.has_item( *ups ) );
    REQUIRE( ups->ammo_remaining() == 500 );
    REQUIRE( units::to_kilojoule( dummy.available_ups() ) == 500 );
}

TEST_CASE( "UPS_modded_tools", "[crafting][ups]" )
{
    constexpr int ammo_count = 500;
    bool const ups_on_ground = GENERATE( true, false );
    CAPTURE( ups_on_ground );
    avatar dummy;
    clear_map();
    clear_character( dummy );
    tripoint const test_loc = dummy.pos();
    dummy.worn.wear_item( dummy, item( "backpack" ), false, false );

    item ups = GENERATE( item( "UPS_ON" ), item( "test_ups" ) );
    CAPTURE( ups.typeId() );
    item_location ups_loc;
    if( ups_on_ground ) {
        item &ups_on_map = get_map().add_item( test_loc, ups );
        REQUIRE( !ups_on_map.is_null() );
        ups_loc = item_location( map_cursor( test_loc ), &ups_on_map );
    } else {
        ups_loc = dummy.i_add( ups );
        REQUIRE( dummy.has_item( *ups_loc ) );
    }

    item ups_mag( ups_loc->magazine_default() );
    ups_mag.ammo_set( ups_mag.ammo_default(), ammo_count );
    ret_val<void> result = ups_loc->put_in( ups_mag, item_pocket::pocket_type::MAGAZINE_WELL );
    REQUIRE( result.success() );

    item_location soldering_iron = dummy.i_add( item( "soldering_iron" ) );
    item battery_ups( "battery_ups" );
    ret_val<void> ret_solder = soldering_iron->put_in( battery_ups, item_pocket::pocket_type::MOD );
    REQUIRE( ret_solder.success() );
    REQUIRE( soldering_iron->has_flag( json_flag_USE_UPS ) );

    REQUIRE( ups_loc->ammo_remaining() == ammo_count );
    if( !ups_on_ground ) {
        REQUIRE( dummy.charges_of( soldering_iron->typeId() ) == ammo_count );
    }
    REQUIRE( dummy.crafting_inventory().charges_of( soldering_iron->typeId() ) == ammo_count );
    temp_crafting_inventory tinv;
    tinv.add_all_ref( dummy );
    if( ups_on_ground ) {
        tinv.add_item_ref( *ups_loc );
    }
    REQUIRE( tinv.charges_of( soldering_iron->typeId() ) == ammo_count );
}

TEST_CASE( "tools_use_charge_to_craft", "[crafting][charge]" )
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
        tools.emplace_back( "motor_micro" );
        tools.emplace_back( "power_supply" );
        tools.emplace_back( "scrap" );

        // Charges needed to craft:
        // - 10 charges of soldering iron
        // - 20 charges of surface heat

        WHEN( "each tool has enough charges" ) {
            item hotplate = tool_with_ammo( "hotplate_induction", 500 );
            REQUIRE( hotplate.ammo_remaining() == 500 );
            tools.push_back( hotplate );
            item soldering = tool_with_ammo( "soldering_iron", 20 );
            REQUIRE( soldering.ammo_remaining() == 20 );
            tools.push_back( soldering );
            item plastic_molding = tool_with_ammo( "vac_mold", 4 );
            REQUIRE( plastic_molding.ammo_remaining() == 4 );
            tools.push_back( plastic_molding );

            THEN( "crafting succeeds, and uses charges from each tool" ) {
                prep_craft( recipe_carver_off, tools, true );
                int turns = actually_test_craft( recipe_carver_off, INT_MAX );
                CAPTURE( turns );
                CHECK( get_remaining_charges( "hotplate_induction" ) == 0 );
                CHECK( get_remaining_charges( "soldering_iron" ) == 10 );
            }
        }

        WHEN( "multiple tools have enough combined charges" ) {
            tools.insert( tools.end(), 2, tool_with_ammo( "hotplate_induction", 250 ) );
            tools.insert( tools.end(), 2, tool_with_ammo( "soldering_iron", 5 ) );
            tools.insert( tools.end(), 1, tool_with_ammo( "vac_mold", 4 ) );

            THEN( "crafting succeeds, and uses charges from multiple tools" ) {
                prep_craft( recipe_carver_off, tools, true );
                actually_test_craft( recipe_carver_off, INT_MAX );
                CHECK( get_remaining_charges( "hotplate_induction" ) == 0 );
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
            UPS_mag.ammo_set( UPS_mag.ammo_default(), 1000 );
            UPS.put_in( UPS_mag, item_pocket::pocket_type::MAGAZINE_WELL );
            tools.emplace_back( UPS );
            tools.push_back( tool_with_ammo( "vac_mold", 4 ) );

            THEN( "crafting succeeds, and uses charges from the UPS" ) {
                prep_craft( recipe_carver_off, tools, true );
                actually_test_craft( recipe_carver_off, INT_MAX );
                CHECK( get_remaining_charges( "hotplate" ) == 0 );
                CHECK( get_remaining_charges( "soldering_iron" ) == 0 );
                CHECK( get_remaining_charges( "UPS_off" ) == 290 );
            }
        }

        WHEN( "UPS-modded tools do not have enough charges" ) {
            item hotplate( "hotplate" );
            hotplate.put_in( item( "battery_ups" ), item_pocket::pocket_type::MOD );
            tools.push_back( hotplate );
            item soldering_iron( "soldering_iron" );
            soldering_iron.put_in( item( "battery_ups" ), item_pocket::pocket_type::MOD );
            tools.push_back( soldering_iron );

            item ups( "UPS_off" );
            item ups_mag( ups.magazine_default() );
            ups_mag.ammo_set( ups_mag.ammo_default(), 10 );
            ups.put_in( ups_mag, item_pocket::pocket_type::MAGAZINE_WELL );
            tools.push_back( ups );

            THEN( "crafting fails, and no charges are used" ) {
                prep_craft( recipe_carver_off, tools, false );
                CHECK( get_remaining_charges( "UPS_off" ) == 10 );
            }
        }
    }
}

TEST_CASE( "tool_use", "[crafting][tool]" )
{
    SECTION( "clean_water" ) {
        std::vector<item> tools;
        tools.push_back( tool_with_ammo( "hotplate", 500 ) );
        item plastic_bottle( "bottle_plastic" );
        plastic_bottle.put_in(
            item( "water", calendar::turn_zero, 2 ), item_pocket::pocket_type::CONTAINER );
        tools.push_back( plastic_bottle );
        tools.emplace_back( "pot" );

        // Can't actually test crafting here since crafting a liquid currently causes a ui prompt
        prep_craft( recipe_water_clean, tools, true );
    }
    SECTION( "clean_water_in_loaded_survivor_mess_kit" ) {
        std::vector<item> tools;
        tools.push_back( tool_with_ammo( "hotplate", 500 ) );
        item plastic_bottle( "bottle_plastic" );
        plastic_bottle.put_in(
            item( "water", calendar::turn_zero, 2 ), item_pocket::pocket_type::CONTAINER );
        tools.push_back( plastic_bottle );
        tools.push_back( tool_with_ammo( "survivor_mess_kit", 500 ) );

        // Can't actually test crafting here since crafting a liquid currently causes a ui prompt
        prep_craft( recipe_water_clean, tools, true );
    }
    SECTION( "clean_water_in_occupied_cooking_vessel" ) {
        std::vector<item> tools;
        tools.push_back( tool_with_ammo( "hotplate", 500 ) );
        item plastic_bottle( "bottle_plastic" );
        plastic_bottle.put_in(
            item( "water", calendar::turn_zero, 2 ), item_pocket::pocket_type::CONTAINER );
        tools.push_back( plastic_bottle );
        item jar( "jar_glass_sealed" );
        // If it's not watertight the water will spill.
        REQUIRE( jar.is_watertight_container() );
        jar.put_in( item( "water", calendar::turn_zero, 2 ), item_pocket::pocket_type::CONTAINER );
        tools.push_back( jar );

        prep_craft( recipe_water_clean, tools, false );
    }
    SECTION( "clean_water with broken tool" ) {
        std::vector<item> tools;
        tools.push_back( tool_with_ammo( "hotplate", 500 ) );
        item plastic_bottle( "bottle_plastic" );
        plastic_bottle.put_in(
            item( "water", calendar::turn_zero, 2 ), item_pocket::pocket_type::CONTAINER );
        tools.push_back( plastic_bottle );
        tools.emplace_back( "pot" );

        tools.front().set_flag( json_flag_ITEM_BROKEN );
        REQUIRE( tools.front().is_broken() );

        prep_craft( recipe_water_clean, tools, false );
    }
}

TEST_CASE( "broken_component", "[crafting][component]" )
{
    GIVEN( "a recipe with its required components" ) {
        recipe_id test_recipe( "flashlight" );

        std::vector<item> tools;
        tools.emplace_back( "amplifier" );
        tools.emplace_back( "bottle_glass" );
        tools.emplace_back( "light_bulb" );
        tools.insert( tools.end(), 10, item( "cable" ) );

        WHEN( "one of its components is broken" ) {
            tools.front().set_flag( json_flag_ITEM_BROKEN );
            REQUIRE( tools.front().is_broken() );

            THEN( "it should not be able to craft it" ) {
                prep_craft( test_recipe, tools, false );
            }
        }
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
    REQUIRE( player_character.crafting_speed_multiplier( *craft, std::nullopt ) == 1.0 );
    REQUIRE( !player_character.activity );
    player_character.use( player_character.get_item_position( craft ) );
    REQUIRE( player_character.activity );
    REQUIRE( player_character.activity.id() == ACT_CRAFT );
    int turns = 0;
    while( player_character.activity.id() == ACT_CRAFT ) {
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
    os << "Wielded:\n" << player_character.get_wielded_item()->tname() << "\n";
    INFO( os.str() );
    for( const std::string &i : has ) {
        INFO( "expecting " << i );
        const bool has_item =
            player_has_item_of_type( i ) ||
            player_character.get_wielded_item()->type->get_id() == itype_id( i );
        REQUIRE( has_item );
    }
    for( const std::string &i : hasnt ) {
        INFO( "not expecting " << i );
        const bool hasnt_item =
            !player_has_item_of_type( i ) &&
            !( player_character.get_wielded_item()->type->get_id() == itype_id( i ) );
        REQUIRE( hasnt_item );
    }
}

TEST_CASE( "total_crafting_time_with_or_without_interruption", "[crafting][time][resume]" )
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
        { qual_CUT, itype_pockknife }, { qual_SEW, itype_sewing_kit }, { qual_LEATHER_AWL, itype_awl_bone }, { qual_ANVIL, itype_fake_anvil }, { qual_HAMMER, itype_hammer }, { qual_SAW_M, itype_hacksaw }, { qual_CHISEL, itype_chisel }, { qual_FABRIC_CUT, itype_kevlar_shears }
    }
};

static void grant_proficiencies_to_character( Character &you, const recipe &r,
        bool grant_optional_proficiencies )
{
    if( grant_optional_proficiencies ) {
        for( const proficiency_id &prof : r.used_proficiencies() ) {
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
        you.add_morale( morale_food_good, morale_level );
        REQUIRE( you.get_morale_level() == morale_level );
    }
    SkillLevel &level = you.get_skill_level_object( skill_used );
    int previous_exercise = level.exercise( true );
    int previous_knowledge = level.knowledgeExperience( true );
    do {
        actual_turns_taken += actually_test_craft( test_recipe, INT_MAX, starting_skill_level );
        if( static_cast<int>( you.get_skill_level( skill_used ) ) == starting_skill_level ) {
            int new_exercise = level.exercise( true );
            REQUIRE( previous_exercise < new_exercise );
            previous_exercise = new_exercise;
        }
        if( you.get_knowledge_level( skill_used ) == starting_skill_level ) {
            int new_knowledge = level.knowledgeExperience( true );
            REQUIRE( previous_knowledge < new_knowledge );
            previous_knowledge = new_knowledge;
        }
        give_tools( tools );
    } while( static_cast<int>( you.get_skill_level( skill_used ) ) == starting_skill_level );
    CAPTURE( test_recipe.str() );
    CAPTURE( expected_turns_taken );
    CAPTURE( grant_optional_proficiencies );
    CHECK( static_cast<int>( you.get_skill_level( skill_used ) ) == starting_skill_level + 1 );
    // since your knowledge and skill were the same to start, your theory should come out the same as skill in the end.
    CHECK( you.get_knowledge_level( skill_used ) == static_cast<int>( you.get_skill_level(
                skill_used ) ) );
    CHECK( actual_turns_taken == expected_turns_taken );
}

TEST_CASE( "crafting_skill_gain", "[skill],[crafting],[slow]" )
{
    SECTION( "lvl 0 -> 1" ) {
        GIVEN( "nominal morale" ) {
            test_skill_progression( recipe_blanket, 174, 0, true );
        }
        GIVEN( "high morale" ) {
            test_skill_progression( recipe_blanket, 173, 50, true );
        }
        GIVEN( "very high morale" ) {
            test_skill_progression( recipe_blanket, 172, 100, true );
        }
    }
    SECTION( "lvl 1 -> 2" ) {
        GIVEN( "nominal morale" ) {
            test_skill_progression( recipe_2byarm_guard, 2140, 0, true );
        }
        GIVEN( "high morale" ) {
            test_skill_progression( recipe_2byarm_guard, 1843, 50, true );
        }
        GIVEN( "very high morale" ) {
            test_skill_progression( recipe_2byarm_guard, 1737, 100, true );
        }
    }
    SECTION( "lvl 2 -> lvl 3" ) {
        GIVEN( "nominal morale" ) {
            test_skill_progression( recipe_vambrace_larmor, 6299, 0, true );
        }
        GIVEN( "high morale" ) {
            test_skill_progression( recipe_vambrace_larmor, 5237, 50, true );
        }
        GIVEN( "very high morale" ) {
            test_skill_progression( recipe_vambrace_larmor, 4841, 100, true );
        }
    }
    SECTION( "lvl 3 -> lvl 4" ) {
        GIVEN( "nominal morale" ) {
            test_skill_progression( recipe_armguard_larmor, 12112, 0, true );
        }
        GIVEN( "high morale" ) {
            test_skill_progression( recipe_armguard_larmor, 9982, 50, true );
        }
        GIVEN( "very high morale" ) {
            test_skill_progression( recipe_armguard_larmor, 9184, 100, true );
        }
    }
    SECTION( "lvl 4 -> 5" ) {
        GIVEN( "nominal morale" ) {
            test_skill_progression( recipe_armguard_metal, 19638, 0, true );
        }
        GIVEN( "high morale" ) {
            test_skill_progression( recipe_armguard_metal, 16126, 50, true );
        }
        GIVEN( "very high morale" ) {
            test_skill_progression( recipe_armguard_metal, 14805, 100, true );
        }
    }
    SECTION( "lvl 5 -> 6" ) {
        GIVEN( "nominal morale" ) {
            test_skill_progression( recipe_armguard_chitin, 28818, 0, true );
        }
        GIVEN( "high morale" ) {
            test_skill_progression( recipe_armguard_chitin, 23613, 50, true );
        }
        GIVEN( "very high morale" ) {
            test_skill_progression( recipe_armguard_chitin, 21651, 100, true );
        }
    }
    SECTION( "lvl 6 -> 7" ) {
        GIVEN( "nominal morale" ) {
            test_skill_progression( recipe_armguard_acidchitin, 39651, 0, true );
        }
        GIVEN( "high morale" ) {
            test_skill_progression( recipe_armguard_acidchitin, 32471, 50, true );
        }
        GIVEN( "very high morale" ) {
            test_skill_progression( recipe_armguard_acidchitin, 29755, 100, true );
        }
    }
    SECTION( "lvl 7 -> 8" ) {
        GIVEN( "nominal morale" ) {
            test_skill_progression( recipe_armguard_lightplate, 52138, 0, true );
        }
        GIVEN( "high morale" ) {
            test_skill_progression( recipe_armguard_lightplate, 42657, 50, true );
        }
        GIVEN( "very high morale" ) {
            test_skill_progression( recipe_armguard_lightplate, 39079, 100, true );
        }
    }
    SECTION( "lvl 8 -> 9" ) {
        GIVEN( "nominal morale" ) {
            test_skill_progression( recipe_helmet_scavenger, 66244, 0, true );
        }
        GIVEN( "high morale" ) {
            test_skill_progression( recipe_helmet_scavenger, 54171, 50, true );
        }
        GIVEN( "very high morale" ) {
            test_skill_progression( recipe_helmet_scavenger, 49610, 100, true );
        }
    }
    SECTION( "lvl 9 -> 10" ) {
        GIVEN( "nominal morale" ) {
            test_skill_progression( recipe_helmet_kabuto, 82490, 0, true );
        }
        GIVEN( "high morale" ) {
            test_skill_progression( recipe_helmet_kabuto, 67365, 50, true );
        }
        GIVEN( "very high morale" ) {
            test_skill_progression( recipe_helmet_kabuto, 61584, 100, true );
        }
    }
    SECTION( "long craft with proficiency delays" ) {
        GIVEN( "nominal morale" ) {
            test_skill_progression( recipe_longbow, 71192, 0, false );
            test_skill_progression( recipe_longbow, 28805, 0, true );
        }
        GIVEN( "high morale" ) {
            test_skill_progression( recipe_longbow, 56945, 50, false );
            test_skill_progression( recipe_longbow, 23609, 50, true );
        }
        GIVEN( "very high morale" ) {
            test_skill_progression( recipe_longbow, 52211, 100, false );
            test_skill_progression( recipe_longbow, 21651, 100, true );
        }
    }
    SECTION( "extremely short craft" ) {
        GIVEN( "nominal morale" ) {
            test_skill_progression( recipe_fishing_hook_basic, 174, 0, true );
        }
        GIVEN( "high morale" ) {
            test_skill_progression( recipe_fishing_hook_basic, 173, 50, true );
        }
        GIVEN( "very high morale" ) {
            test_skill_progression( recipe_fishing_hook_basic, 172, 100, true );
        }
    }
}

TEST_CASE( "book_proficiency_mitigation", "[crafting][proficiency]" )
{
    GIVEN( "a recipe with required proficiencies" ) {
        clear_avatar();
        clear_map();
        const recipe &test_recipe = *recipe_leather_belt;

        grant_skills_to_character( get_player_character(), test_recipe, 0 );
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
        const recipe &test_recipe = *recipe_leather_belt;

        grant_skills_to_character( tester, test_recipe, 0 );
        int unmitigated_time_taken = test_recipe.batch_time( tester, 1, 1, 0 );

        WHEN( "player acquires partial proficiency" ) {
            for( const proficiency_id &prof : test_recipe.used_proficiencies() ) {
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

static void clear_and_setup( Character &c, map &m, item &tool )
{
    clear_character( c );
    c.get_learned_recipes(); // cache auto-learned recipes
    c.set_skill_level( skill_fabrication, 10 );
    c.wield( tool );
    m.i_clear( c.pos() );
}

TEST_CASE( "prompt_for_liquid_containers_-_crafting_1_makeshift_funnel", "[crafting]" )
{
    map &m = get_map();
    item pocketknife( itype_pockknife );
    const item backpack( "debug_backpack" );

    GIVEN( "crafting 1 makeshift funnel" ) {
        WHEN( "3 empty plastic bottles on the ground" ) {
            item plastic_bottle( "bottle_plastic" );
            REQUIRE( plastic_bottle.is_watertight_container() );
            REQUIRE( plastic_bottle.empty_container() );
            Character &c = get_player_character();
            clear_and_setup( c, m, pocketknife );
            REQUIRE( m.i_at( c.pos() ).empty() );
            c.i_add_or_drop( plastic_bottle, 3 );
            THEN( "no prompt" ) {
                craft_command cmd( &*recipe_makeshift_funnel, 1, false, &c, c.pos() );
                cmd.execute( true );
                item_filter filter = recipe_makeshift_funnel->get_component_filter();
                CHECK( cmd.continue_prompt_liquids( filter, true ) == true );
                const map_stack &items = m.i_at( c.pos() );
                CHECK( items.size() == 3 );
                auto iter = items.begin();
                CHECK( iter->typeId() == plastic_bottle.typeId() );
                iter++;
                CHECK( iter->typeId() == plastic_bottle.typeId() );
                iter++;
                CHECK( iter->typeId() == plastic_bottle.typeId() );
            }
        }

        WHEN( "3 empty plastic bottles in inventory" ) {
            item plastic_bottle( "bottle_plastic" );
            REQUIRE( plastic_bottle.is_watertight_container() );
            REQUIRE( plastic_bottle.empty_container() );
            Character &c = get_player_character();
            clear_and_setup( c, m, pocketknife );
            REQUIRE( m.i_at( c.pos() ).empty() );
            c.worn.wear_item( c, backpack, false, false );
            c.i_add( plastic_bottle );
            c.i_add( plastic_bottle );
            c.i_add( plastic_bottle );
            THEN( "no prompt" ) {
                craft_command cmd( &*recipe_makeshift_funnel, 1, false, &c, c.pos() );
                cmd.execute( true );
                item_filter filter = recipe_makeshift_funnel->get_component_filter();
                CHECK( cmd.continue_prompt_liquids( filter, true ) == true );
                CHECK( m.i_at( c.pos() ).empty() );
                CHECK( c.crafting_inventory().count_item( plastic_bottle.typeId() ) == 3 );
            }
        }

        WHEN( "3 full plastic bottles on the ground" ) {
            item plastic_bottle( "bottle_plastic" );
            plastic_bottle.put_in( item( "water", calendar::turn_zero, 2 ),
                                   item_pocket::pocket_type::CONTAINER );
            REQUIRE( plastic_bottle.is_watertight_container() );
            REQUIRE( !plastic_bottle.empty_container() );
            Character &c = get_player_character();
            clear_and_setup( c, m, pocketknife );
            REQUIRE( m.i_at( c.pos() ).empty() );
            c.i_add_or_drop( plastic_bottle, 3 );
            REQUIRE( !m.i_at( c.pos() ).begin()->empty_container() );
            THEN( "player is prompted" ) {
                REQUIRE( c.crafting_inventory().count_item( plastic_bottle.typeId() ) == 3 );
                craft_command cmd( &*recipe_makeshift_funnel, 1, false, &c, c.pos() );
                cmd.execute( true );
                item_filter filter = recipe_makeshift_funnel->get_component_filter();
                CHECK( cmd.continue_prompt_liquids( filter, true ) == false );
                const map_stack &items = m.i_at( c.pos() );
                CHECK( items.size() == 3 );
                auto iter = items.begin();
                CHECK( iter->typeId() == plastic_bottle.typeId() );
                iter++;
                CHECK( iter->typeId() == plastic_bottle.typeId() );
                iter++;
                CHECK( iter->typeId() == plastic_bottle.typeId() );
            }
        }

        WHEN( "3 full plastic bottles in inventory" ) {
            item plastic_bottle( "bottle_plastic" );
            plastic_bottle.put_in( item( "water", calendar::turn_zero, 2 ),
                                   item_pocket::pocket_type::CONTAINER );
            REQUIRE( plastic_bottle.is_watertight_container() );
            REQUIRE( !plastic_bottle.empty_container() );
            Character &c = get_player_character();
            clear_and_setup( c, m, pocketknife );
            REQUIRE( m.i_at( c.pos() ).empty() );
            c.worn.wear_item( c, backpack, false, false );
            c.i_add( plastic_bottle );
            c.i_add( plastic_bottle );
            c.i_add( plastic_bottle );
            REQUIRE( !( *c.worn.front().all_items_top().begin() )->empty_container() );
            THEN( "player is prompted" ) {
                craft_command cmd( &*recipe_makeshift_funnel, 1, false, &c, c.pos() );
                cmd.execute( true );
                item_filter filter = recipe_makeshift_funnel->get_component_filter();
                CHECK( cmd.continue_prompt_liquids( filter, true ) == false );
                CHECK( m.i_at( c.pos() ).empty() );
                CHECK( c.crafting_inventory().count_item( plastic_bottle.typeId() ) == 3 );
            }
        }

        WHEN( "3 empty and 3 full plastic bottles on the ground" ) {
            item empty_plastic_bottle( "bottle_plastic" );
            item full_plastic_bottle( "bottle_plastic" );
            full_plastic_bottle.put_in( item( "water", calendar::turn_zero, 2 ),
                                        item_pocket::pocket_type::CONTAINER );
            REQUIRE( empty_plastic_bottle.is_watertight_container() );
            REQUIRE( empty_plastic_bottle.empty_container() );
            REQUIRE( !full_plastic_bottle.empty_container() );
            Character &c = get_player_character();
            clear_and_setup( c, m, pocketknife );
            REQUIRE( m.i_at( c.pos() ).empty() );
            c.i_add_or_drop( empty_plastic_bottle, 3 );
            c.i_add_or_drop( full_plastic_bottle, 3 );
            REQUIRE( m.i_at( c.pos() ).size() == 6 );
            THEN( "no prompt" ) {
                REQUIRE( c.crafting_inventory().count_item( empty_plastic_bottle.typeId() ) == 6 );
                craft_command cmd( &*recipe_makeshift_funnel, 1, false, &c, c.pos() );
                cmd.execute( true );
                item_filter filter = recipe_makeshift_funnel->get_component_filter();
                CHECK( cmd.continue_prompt_liquids( filter, true ) == true );
                CHECK( m.i_at( c.pos() ).size() == 6 );
            }
        }

        WHEN( "3 empty and 3 full plastic bottles in inventory" ) {
            item empty_plastic_bottle( "bottle_plastic" );
            item full_plastic_bottle( "bottle_plastic" );
            full_plastic_bottle.put_in( item( "water", calendar::turn_zero, 2 ),
                                        item_pocket::pocket_type::CONTAINER );
            REQUIRE( empty_plastic_bottle.is_watertight_container() );
            REQUIRE( empty_plastic_bottle.empty_container() );
            REQUIRE( !full_plastic_bottle.empty_container() );
            Character &c = get_player_character();
            clear_and_setup( c, m, pocketknife );
            REQUIRE( m.i_at( c.pos() ).empty() );
            c.worn.wear_item( c, backpack, false, false );
            c.i_add( empty_plastic_bottle );
            c.i_add( empty_plastic_bottle );
            c.i_add( empty_plastic_bottle );
            c.i_add( full_plastic_bottle );
            c.i_add( full_plastic_bottle );
            c.i_add( full_plastic_bottle );
            REQUIRE( m.i_at( c.pos() ).empty() );
            REQUIRE( c.worn.front().all_items_top().size() == 6 );
            THEN( "no prompt" ) {
                REQUIRE( c.crafting_inventory().count_item( empty_plastic_bottle.typeId() ) == 6 );
                craft_command cmd( &*recipe_makeshift_funnel, 1, false, &c, c.pos() );
                cmd.execute( true );
                item_filter filter = recipe_makeshift_funnel->get_component_filter();
                CHECK( cmd.continue_prompt_liquids( filter, true ) == true );
                CHECK( m.i_at( c.pos() ).empty() );
                CHECK( c.worn.front().all_items_top().size() == 6 );
            }
        }

        WHEN( "2 empty and 3 full plastic bottles on the ground" ) {
            item empty_plastic_bottle( "bottle_plastic" );
            item full_plastic_bottle( "bottle_plastic" );
            full_plastic_bottle.put_in( item( "water", calendar::turn_zero, 2 ),
                                        item_pocket::pocket_type::CONTAINER );
            REQUIRE( empty_plastic_bottle.is_watertight_container() );
            REQUIRE( empty_plastic_bottle.empty_container() );
            REQUIRE( !full_plastic_bottle.empty_container() );
            Character &c = get_player_character();
            clear_and_setup( c, m, pocketknife );
            REQUIRE( m.i_at( c.pos() ).empty() );
            c.i_add_or_drop( empty_plastic_bottle, 2 );
            c.i_add_or_drop( full_plastic_bottle, 3 );
            REQUIRE( m.i_at( c.pos() ).size() == 5 );
            THEN( "player is prompted" ) {
                REQUIRE( c.crafting_inventory().count_item( empty_plastic_bottle.typeId() ) == 5 );
                craft_command cmd( &*recipe_makeshift_funnel, 1, false, &c, c.pos() );
                cmd.execute( true );
                item_filter filter = recipe_makeshift_funnel->get_component_filter();
                CHECK( cmd.continue_prompt_liquids( filter, true ) == false );
                CHECK( m.i_at( c.pos() ).size() == 5 );
            }
        }

        WHEN( "2 empty and 3 full plastic bottles in inventory" ) {
            item empty_plastic_bottle( "bottle_plastic" );
            item full_plastic_bottle( "bottle_plastic" );
            full_plastic_bottle.put_in( item( "water", calendar::turn_zero, 2 ),
                                        item_pocket::pocket_type::CONTAINER );
            REQUIRE( empty_plastic_bottle.is_watertight_container() );
            REQUIRE( empty_plastic_bottle.empty_container() );
            REQUIRE( !full_plastic_bottle.empty_container() );
            Character &c = get_player_character();
            clear_and_setup( c, m, pocketknife );
            REQUIRE( m.i_at( c.pos() ).empty() );
            c.worn.wear_item( c, backpack, false, false );
            c.i_add( empty_plastic_bottle );
            c.i_add( empty_plastic_bottle );
            c.i_add( full_plastic_bottle );
            c.i_add( full_plastic_bottle );
            c.i_add( full_plastic_bottle );
            REQUIRE( m.i_at( c.pos() ).empty() );
            REQUIRE( c.worn.front().all_items_top().size() == 5 );
            THEN( "player is prompted" ) {
                REQUIRE( c.crafting_inventory().count_item( empty_plastic_bottle.typeId() ) == 5 );
                craft_command cmd( &*recipe_makeshift_funnel, 1, false, &c, c.pos() );
                cmd.execute( true );
                item_filter filter = recipe_makeshift_funnel->get_component_filter();
                CHECK( cmd.continue_prompt_liquids( filter, true ) == false );
                CHECK( m.i_at( c.pos() ).empty() );
                CHECK( c.worn.front().all_items_top().size() == 5 );
            }
        }
    }
}

TEST_CASE( "prompt_for_liquid_containers_-_batch_crafting_3_makeshift_funnels", "[crafting]" )
{
    map &m = get_map();
    item pocketknife( itype_pockknife );
    const item backpack( "debug_backpack" );

    GIVEN( "crafting batch of 3 makeshift funnels" ) {
        WHEN( "10 empty plastic bottles on the ground" ) {
            item plastic_bottle( "bottle_plastic" );
            REQUIRE( plastic_bottle.is_watertight_container() );
            REQUIRE( plastic_bottle.empty_container() );
            Character &c = get_player_character();
            clear_and_setup( c, m, pocketknife );
            REQUIRE( m.i_at( c.pos() ).empty() );
            c.i_add_or_drop( plastic_bottle, 10 );
            THEN( "no prompt" ) {
                craft_command cmd( &*recipe_makeshift_funnel, 3, false, &c, c.pos() );
                cmd.execute( true );
                item_filter filter = recipe_makeshift_funnel->get_component_filter();
                CHECK( cmd.continue_prompt_liquids( filter, true ) == true );
                CHECK( m.i_at( c.pos() ).size() == 10 );
            }
        }

        WHEN( "10 empty plastic bottles in inventory" ) {
            item plastic_bottle( "bottle_plastic" );
            REQUIRE( plastic_bottle.is_watertight_container() );
            REQUIRE( plastic_bottle.empty_container() );
            Character &c = get_player_character();
            clear_and_setup( c, m, pocketknife );
            REQUIRE( m.i_at( c.pos() ).empty() );
            c.worn.wear_item( c, backpack, false, false );
            c.i_add( plastic_bottle );
            c.i_add( plastic_bottle );
            c.i_add( plastic_bottle );
            c.i_add( plastic_bottle );
            c.i_add( plastic_bottle );
            c.i_add( plastic_bottle );
            c.i_add( plastic_bottle );
            c.i_add( plastic_bottle );
            c.i_add( plastic_bottle );
            c.i_add( plastic_bottle );
            THEN( "no prompt" ) {
                craft_command cmd( &*recipe_makeshift_funnel, 3, false, &c, c.pos() );
                cmd.execute( true );
                item_filter filter = recipe_makeshift_funnel->get_component_filter();
                CHECK( cmd.continue_prompt_liquids( filter, true ) == true );
                CHECK( m.i_at( c.pos() ).empty() );
                CHECK( c.crafting_inventory().count_item( plastic_bottle.typeId() ) == 10 );
            }
        }

        WHEN( "10 full plastic bottles on the ground" ) {
            item plastic_bottle( "bottle_plastic" );
            plastic_bottle.put_in( item( "water", calendar::turn_zero, 2 ),
                                   item_pocket::pocket_type::CONTAINER );
            REQUIRE( plastic_bottle.is_watertight_container() );
            REQUIRE( !plastic_bottle.empty_container() );
            Character &c = get_player_character();
            clear_and_setup( c, m, pocketknife );
            REQUIRE( m.i_at( c.pos() ).empty() );
            c.i_add_or_drop( plastic_bottle, 10 );
            REQUIRE( !m.i_at( c.pos() ).begin()->empty_container() );
            THEN( "player is prompted" ) {
                REQUIRE( c.crafting_inventory().count_item( plastic_bottle.typeId() ) == 10 );
                craft_command cmd( &*recipe_makeshift_funnel, 3, false, &c, c.pos() );
                cmd.execute( true );
                item_filter filter = recipe_makeshift_funnel->get_component_filter();
                CHECK( cmd.continue_prompt_liquids( filter, true ) == false );
                CHECK( m.i_at( c.pos() ).size() == 10 );
            }
        }

        WHEN( "10 full plastic bottles in inventory" ) {
            item plastic_bottle( "bottle_plastic" );
            plastic_bottle.put_in( item( "water", calendar::turn_zero, 2 ),
                                   item_pocket::pocket_type::CONTAINER );
            REQUIRE( plastic_bottle.is_watertight_container() );
            REQUIRE( !plastic_bottle.empty_container() );
            Character &c = get_player_character();
            clear_and_setup( c, m, pocketknife );
            REQUIRE( m.i_at( c.pos() ).empty() );
            c.worn.wear_item( c, backpack, false, false );
            c.i_add( plastic_bottle );
            c.i_add( plastic_bottle );
            c.i_add( plastic_bottle );
            c.i_add( plastic_bottle );
            c.i_add( plastic_bottle );
            c.i_add( plastic_bottle );
            c.i_add( plastic_bottle );
            c.i_add( plastic_bottle );
            c.i_add( plastic_bottle );
            c.i_add( plastic_bottle );
            REQUIRE( !( *c.worn.front().all_items_top().begin() )->empty_container() );
            THEN( "player is prompted" ) {
                craft_command cmd( &*recipe_makeshift_funnel, 3, false, &c, c.pos() );
                cmd.execute( true );
                item_filter filter = recipe_makeshift_funnel->get_component_filter();
                CHECK( cmd.continue_prompt_liquids( filter, true ) == false );
                CHECK( m.i_at( c.pos() ).empty() );
                CHECK( c.crafting_inventory().count_item( plastic_bottle.typeId() ) == 10 );
            }
        }

        WHEN( "10 empty and 3 full plastic bottles on the ground" ) {
            item empty_plastic_bottle( "bottle_plastic" );
            item full_plastic_bottle( "bottle_plastic" );
            full_plastic_bottle.put_in( item( "water", calendar::turn_zero, 2 ),
                                        item_pocket::pocket_type::CONTAINER );
            REQUIRE( empty_plastic_bottle.is_watertight_container() );
            REQUIRE( empty_plastic_bottle.empty_container() );
            REQUIRE( !full_plastic_bottle.empty_container() );
            Character &c = get_player_character();
            clear_and_setup( c, m, pocketknife );
            REQUIRE( m.i_at( c.pos() ).empty() );
            c.i_add_or_drop( empty_plastic_bottle, 10 );
            c.i_add_or_drop( full_plastic_bottle, 3 );
            REQUIRE( m.i_at( c.pos() ).size() == 13 );
            THEN( "no prompt" ) {
                REQUIRE( c.crafting_inventory().count_item( empty_plastic_bottle.typeId() ) == 13 );
                craft_command cmd( &*recipe_makeshift_funnel, 3, false, &c, c.pos() );
                cmd.execute( true );
                item_filter filter = recipe_makeshift_funnel->get_component_filter();
                CHECK( cmd.continue_prompt_liquids( filter, true ) == true );
                CHECK( m.i_at( c.pos() ).size() == 13 );
            }
        }

        WHEN( "10 empty and 3 full plastic bottles in inventory" ) {
            item empty_plastic_bottle( "bottle_plastic" );
            item full_plastic_bottle( "bottle_plastic" );
            full_plastic_bottle.put_in( item( "water", calendar::turn_zero, 2 ),
                                        item_pocket::pocket_type::CONTAINER );
            REQUIRE( empty_plastic_bottle.is_watertight_container() );
            REQUIRE( empty_plastic_bottle.empty_container() );
            REQUIRE( !full_plastic_bottle.empty_container() );
            Character &c = get_player_character();
            clear_and_setup( c, m, pocketknife );
            REQUIRE( m.i_at( c.pos() ).empty() );
            c.worn.wear_item( c, backpack, false, false );
            c.i_add( empty_plastic_bottle );
            c.i_add( empty_plastic_bottle );
            c.i_add( empty_plastic_bottle );
            c.i_add( empty_plastic_bottle );
            c.i_add( empty_plastic_bottle );
            c.i_add( empty_plastic_bottle );
            c.i_add( empty_plastic_bottle );
            c.i_add( empty_plastic_bottle );
            c.i_add( empty_plastic_bottle );
            c.i_add( empty_plastic_bottle );
            c.i_add( full_plastic_bottle );
            c.i_add( full_plastic_bottle );
            c.i_add( full_plastic_bottle );
            REQUIRE( m.i_at( c.pos() ).empty() );
            REQUIRE( c.worn.front().all_items_top().size() == 13 );
            THEN( "no prompt" ) {
                REQUIRE( c.crafting_inventory().count_item( empty_plastic_bottle.typeId() ) == 13 );
                craft_command cmd( &*recipe_makeshift_funnel, 3, false, &c, c.pos() );
                cmd.execute( true );
                item_filter filter = recipe_makeshift_funnel->get_component_filter();
                CHECK( cmd.continue_prompt_liquids( filter, true ) == true );
                CHECK( m.i_at( c.pos() ).empty() );
                CHECK( c.worn.front().all_items_top().size() == 13 );
            }
        }

        WHEN( "7 empty and 3 full plastic bottles on the ground" ) {
            item empty_plastic_bottle( "bottle_plastic" );
            item full_plastic_bottle( "bottle_plastic" );
            full_plastic_bottle.put_in( item( "water", calendar::turn_zero, 2 ),
                                        item_pocket::pocket_type::CONTAINER );
            REQUIRE( empty_plastic_bottle.is_watertight_container() );
            REQUIRE( empty_plastic_bottle.empty_container() );
            REQUIRE( !full_plastic_bottle.empty_container() );
            Character &c = get_player_character();
            clear_and_setup( c, m, pocketknife );
            REQUIRE( m.i_at( c.pos() ).empty() );
            c.i_add_or_drop( empty_plastic_bottle, 7 );
            c.i_add_or_drop( full_plastic_bottle, 3 );
            REQUIRE( m.i_at( c.pos() ).size() == 10 );
            THEN( "player is prompted" ) {
                REQUIRE( c.crafting_inventory().count_item( empty_plastic_bottle.typeId() ) == 10 );
                craft_command cmd( &*recipe_makeshift_funnel, 3, false, &c, c.pos() );
                cmd.execute( true );
                item_filter filter = recipe_makeshift_funnel->get_component_filter();
                CHECK( cmd.continue_prompt_liquids( filter, true ) == false );
                CHECK( m.i_at( c.pos() ).size() == 10 );
            }
        }

        WHEN( "7 empty and 3 full plastic bottles in inventory" ) {
            item empty_plastic_bottle( "bottle_plastic" );
            item full_plastic_bottle( "bottle_plastic" );
            full_plastic_bottle.put_in( item( "water", calendar::turn_zero, 2 ),
                                        item_pocket::pocket_type::CONTAINER );
            REQUIRE( empty_plastic_bottle.is_watertight_container() );
            REQUIRE( empty_plastic_bottle.empty_container() );
            REQUIRE( !full_plastic_bottle.empty_container() );
            Character &c = get_player_character();
            clear_and_setup( c, m, pocketknife );
            REQUIRE( m.i_at( c.pos() ).empty() );
            c.worn.wear_item( c, backpack, false, false );
            c.i_add( empty_plastic_bottle );
            c.i_add( empty_plastic_bottle );
            c.i_add( empty_plastic_bottle );
            c.i_add( empty_plastic_bottle );
            c.i_add( empty_plastic_bottle );
            c.i_add( empty_plastic_bottle );
            c.i_add( empty_plastic_bottle );
            c.i_add( full_plastic_bottle );
            c.i_add( full_plastic_bottle );
            c.i_add( full_plastic_bottle );
            REQUIRE( m.i_at( c.pos() ).empty() );
            REQUIRE( c.worn.front().all_items_top().size() == 10 );
            THEN( "player is prompted" ) {
                REQUIRE( c.crafting_inventory().count_item( empty_plastic_bottle.typeId() ) == 10 );
                craft_command cmd( &*recipe_makeshift_funnel, 3, false, &c, c.pos() );
                cmd.execute( true );
                item_filter filter = recipe_makeshift_funnel->get_component_filter();
                CHECK( cmd.continue_prompt_liquids( filter, true ) == false );
                CHECK( m.i_at( c.pos() ).empty() );
                CHECK( c.worn.front().all_items_top().size() == 10 );
            }
        }
    }
}

TEST_CASE( "Unloading_non-empty_components", "[crafting]" )
{
    item candle( itype_candle );
    item cash_card( itype_cash_card );
    item sewing_kit( itype_sewing_kit );
    candle.ammo_set( candle.ammo_default(), -1 );
    cash_card.ammo_set( cash_card.ammo_default(), -1 );
    sewing_kit.ammo_set( sewing_kit.ammo_default(), -1 );
    REQUIRE( !candle.is_container_empty() );
    REQUIRE( !cash_card.is_container_empty() );
    REQUIRE( !sewing_kit.is_container_empty() );

    // candle -> candle wax: not ok
    CHECK( craft_command::safe_to_unload_comp( candle ) == false );
    // cash card -> cents: not ok
    CHECK( craft_command::safe_to_unload_comp( cash_card ) == false );
    // sewing kit -> thread: ok
    CHECK( craft_command::safe_to_unload_comp( sewing_kit ) == true );
}

TEST_CASE( "Warn_when_using_favorited_component", "[crafting]" )
{
    map &m = get_map();
    clear_map();
    item pocketknife( itype_pockknife );

    GIVEN( "crafting 1 makeshift funnel" ) {
        WHEN( "no favorited components" ) {
            item plastic_bottle( "bottle_plastic" );
            Character &c = get_player_character();
            clear_and_setup( c, m, pocketknife );
            REQUIRE( m.i_at( c.pos() ).empty() );
            c.i_add_or_drop( plastic_bottle, 3 );
            map_stack items = m.i_at( c.pos() );
            REQUIRE( !items.empty() );
            REQUIRE( !items.begin()->is_favorite );
            THEN( "no warning" ) {
                CHECK( c.can_start_craft( &*recipe_makeshift_funnel, recipe_filter_flags::none ) );
                CHECK( c.can_start_craft( &*recipe_makeshift_funnel, recipe_filter_flags::no_favorite ) );
            }
        }
        WHEN( "all favorited components" ) {
            item plastic_bottle( "bottle_plastic" );
            plastic_bottle.is_favorite = true;
            Character &c = get_player_character();
            clear_and_setup( c, m, pocketknife );
            REQUIRE( m.i_at( c.pos() ).empty() );
            c.i_add_or_drop( plastic_bottle, 3 );
            map_stack items = m.i_at( c.pos() );
            REQUIRE( !items.empty() );
            REQUIRE( items.begin()->is_favorite );
            THEN( "warning" ) {
                CHECK( c.can_start_craft( &*recipe_makeshift_funnel, recipe_filter_flags::none ) );
                CHECK( !c.can_start_craft( &*recipe_makeshift_funnel, recipe_filter_flags::no_favorite ) );
            }
        }
        WHEN( "1 favorited component" ) {
            item plastic_bottle( "bottle_plastic" );
            Character &c = get_player_character();
            clear_and_setup( c, m, pocketknife );
            REQUIRE( m.i_at( c.pos() ).empty() );
            c.i_add_or_drop( plastic_bottle, 3 );
            map_stack items = m.i_at( c.pos() );
            REQUIRE( !items.empty() );
            items.begin()->is_favorite = true;
            REQUIRE( items.begin()->is_favorite );
            THEN( "warning" ) {
                CHECK( c.can_start_craft( &*recipe_makeshift_funnel, recipe_filter_flags::none ) );
                CHECK( !c.can_start_craft( &*recipe_makeshift_funnel, recipe_filter_flags::no_favorite ) );
            }
        }
        WHEN( "1 favorited component, extra non-favorited components" ) {
            item plastic_bottle( "bottle_plastic" );
            Character &c = get_player_character();
            clear_and_setup( c, m, pocketknife );
            REQUIRE( m.i_at( c.pos() ).empty() );
            c.i_add_or_drop( plastic_bottle, 4 );
            map_stack items = m.i_at( c.pos() );
            REQUIRE( !items.empty() );
            items.begin()->is_favorite = true;
            REQUIRE( items.begin()->is_favorite );
            THEN( "no warning" ) {
                CHECK( c.can_start_craft( &*recipe_makeshift_funnel, recipe_filter_flags::none ) );
                CHECK( c.can_start_craft( &*recipe_makeshift_funnel, recipe_filter_flags::no_favorite ) );
            }
        }
    }
}

static bool found_all_in_list( const std::vector<item> &items,
                               std::map<const itype_id, std::pair<std::pair<const int, const int>, int>> &expected )
{
    bool ret = true;
    for( const item &i : items ) {
        bool expected_item = false;
        for( auto &found : expected ) {
            if( i.typeId() == found.first ) {
                expected_item = true;
                found.second.second += i.count_by_charges() ? i.charges : 1;
            }
        }
        if( !expected_item ) {
            ret = false;
        }
    }
    for( const auto &found : expected ) {
        CAPTURE( found.first.c_str() );
        CHECK( found.second.second >= found.second.first.first );
        CHECK( found.second.second <= found.second.first.second );
    }
    return ret;
}

TEST_CASE( "recipe_byproducts_and_byproduct_groups", "[recipes][crafting]" )
{
    GIVEN( "recipe with byproducts, normal definition" ) {
        const recipe *r = &recipe_test_tallow.obj();
        REQUIRE( r->has_byproducts() );
        const int count_cracklins = 4;
        const int count_gum = 10;

        WHEN( "crafting in batch of 1" ) {
            const int batch = 1;
            std::vector<item> bps = r->create_byproducts( batch );
            std::map<const itype_id, std::pair<std::pair<const int, const int>, int>> found_itype_count = {
                { itype_test_cracklins, { { count_cracklins * batch, count_cracklins * batch }, 0 } },
                { itype_test_gum, { { count_gum * batch, count_gum * batch }, 0 } }
            };
            CHECK( found_all_in_list( bps, found_itype_count ) );
        }

        WHEN( "crafting in batch of 2" ) {
            const int batch = 2;
            std::vector<item> bps = r->create_byproducts( batch );
            std::map<const itype_id, std::pair<std::pair<const int, const int>, int>> found_itype_count = {
                { itype_test_cracklins, { { count_cracklins * batch, count_cracklins * batch }, 0 } },
                { itype_test_gum, { { count_gum * batch, count_gum * batch }, 0 } }
            };
            CHECK( found_all_in_list( bps, found_itype_count ) );
        }

        WHEN( "crafting in batch of 10" ) {
            const int batch = 10;
            std::vector<item> bps = r->create_byproducts( batch );
            std::map<const itype_id, std::pair<std::pair<const int, const int>, int>> found_itype_count = {
                { itype_test_cracklins, { { count_cracklins * batch, count_cracklins * batch }, 0 } },
                { itype_test_gum, { { count_gum * batch, count_gum * batch }, 0 } }
            };
            CHECK( found_all_in_list( bps, found_itype_count ) );
        }
    }

    GIVEN( "recipe with byproducts, item group definition" ) {
        const recipe *r = &recipe_test_tallow2.obj();
        REQUIRE( r->has_byproducts() );
        const int count_cracklins = 1; // defined "charges" doesn't produce full stack
        const int count_gum = 10;
        const int lo = 2;
        const int hi = 3;

        WHEN( "crafting in batch of 1" ) {
            const int batch = 1;
            std::vector<item> bps = r->create_byproducts( batch );
            std::map<const itype_id, std::pair<std::pair<const int, const int>, int>> found_itype_count = {
                { itype_test_cracklins, { { count_cracklins *batch * lo, count_cracklins *batch * hi }, 0 } },
                { itype_test_gum, { { count_gum *batch * lo, count_gum *batch * hi }, 0 } }
            };
            CHECK( found_all_in_list( bps, found_itype_count ) );
        }

        WHEN( "crafting in batch of 2" ) {
            const int batch = 2;
            std::vector<item> bps = r->create_byproducts( batch );
            std::map<const itype_id, std::pair<std::pair<const int, const int>, int>> found_itype_count = {
                { itype_test_cracklins, { { count_cracklins *batch * lo, count_cracklins *batch * hi }, 0 } },
                { itype_test_gum, { { count_gum *batch * lo, count_gum *batch * hi }, 0 } }
            };
            CHECK( found_all_in_list( bps, found_itype_count ) );
        }

        WHEN( "crafting in batch of 10" ) {
            const int batch = 10;
            std::vector<item> bps = r->create_byproducts( batch );
            std::map<const itype_id, std::pair<std::pair<const int, const int>, int>> found_itype_count = {
                { itype_test_cracklins, { { count_cracklins *batch * lo, count_cracklins *batch * hi }, 0 } },
                { itype_test_gum, { { count_gum *batch * lo, count_gum *batch * hi }, 0 } }
            };
            CHECK( found_all_in_list( bps, found_itype_count ) );
        }
    }
}

TEST_CASE( "tools_with_charges_as_components", "[crafting]" )
{
    const int cotton_sheets_in_recipe = 2;
    const int threads_in_recipe = 10;
    map &m = get_map();
    Character &c = get_player_character();
    item pocketknife( itype_pockknife );
    item sew_kit( itype_sewing_kit );
    item thread( "thread" );
    item sheet_cotton( "sheet_cotton" );
    thread.charges = 100;
    sew_kit.put_in( thread, item_pocket::pocket_type::MAGAZINE );
    REQUIRE( sew_kit.ammo_remaining() == 100 );
    clear_and_setup( c, m, pocketknife );
    c.learn_recipe( &*recipe_balclava );
    c.set_skill_level( skill_survival, 10 );

    GIVEN( "sewing kit with thread on the ground" ) {
        REQUIRE( m.i_at( c.pos() ).empty() );
        c.i_add_or_drop( sew_kit );
        c.i_add_or_drop( thread );
        c.i_add_or_drop( sheet_cotton, cotton_sheets_in_recipe );
        WHEN( "crafting a balaclava" ) {
            craft_command cmd( &*recipe_balclava, 1, false, &c, c.pos() );
            cmd.execute( true );
            item res = cmd.create_in_progress_craft();
            THEN( "craft uses the free thread instead of tool ammo as component" ) {
                CHECK( !res.is_null() );
                CHECK( res.is_craft() );
                CHECK( res.components[itype_sheet_cotton].size() == cotton_sheets_in_recipe );
                // when threads aren't count by charges anymore, see line above
                CHECK( res.components[itype_thread].front().count() == threads_in_recipe );
                int cotton_sheets = 0;
                int threads = 0;
                int threads_in_tool = 0;
                for( const item &i : m.i_at( c.pos() ) ) {
                    if( i.typeId() == itype_sheet_cotton ) {
                        cotton_sheets += i.count_by_charges() ? i.charges : 1;
                    } else if( i.typeId() == itype_thread ) {
                        threads += i.count_by_charges() ? i.charges : 1;
                    } else if( i.typeId() == itype_sewing_kit ) {
                        threads_in_tool += i.ammo_remaining();
                    }
                }
                CHECK( cotton_sheets == 0 );
                CHECK( threads == 100 - threads_in_recipe );
                CHECK( threads_in_tool == 100 );
            }
        }
    }

    GIVEN( "sewing kit with thread in inventory" ) {
        const item backpack( "debug_backpack" );
        item_location pack_loc( c, & **c.wear_item( backpack, false ) );
        REQUIRE( !!pack_loc.get_item() );
        REQUIRE( pack_loc->is_container_empty() );
        c.i_add_or_drop( sew_kit );
        c.i_add_or_drop( thread );
        c.i_add_or_drop( sheet_cotton, cotton_sheets_in_recipe );
        WHEN( "crafting a balaclava" ) {
            craft_command cmd( &*recipe_balclava, 1, false, &c, c.pos() );
            cmd.execute( true );
            item res = cmd.create_in_progress_craft();
            THEN( "craft uses the free thread instead of tool ammo as component" ) {
                CHECK( !res.is_null() );
                CHECK( res.is_craft() );
                CHECK( res.components[itype_sheet_cotton].size() == cotton_sheets_in_recipe );
                // when threads aren't count by charges anymore, see line above
                CHECK( res.components[itype_thread].front().count() == threads_in_recipe );
                int cotton_sheets = 0;
                int threads = 0;
                int threads_in_tool = 0;
                for( const item *i : pack_loc->all_items_top() ) {
                    if( i->typeId() == itype_sheet_cotton ) {
                        cotton_sheets += i->count_by_charges() ? i->charges : 1;
                    } else if( i->typeId() == itype_thread ) {
                        threads += i->count_by_charges() ? i->charges : 1;
                    } else if( i->typeId() == itype_sewing_kit ) {
                        threads_in_tool += i->ammo_remaining();
                    }
                }
                CHECK( cotton_sheets == 0 );
                CHECK( threads == 100 - threads_in_recipe );
                CHECK( threads_in_tool == 100 );
            }
        }
    }
}

// This test makes sure that rot is inherited properly when crafting. See the comments on
// inherit_rot_from_components for a description of what "inheritied properly" means
// using a default hotplate the macaroni uses 35x7 = 245 charges of hotplate, meat uses 35x20 = 700 charges of hotplate and 80x30 = 2400 charges of dehydrator
// looks like tool_with_ammo cannot spawn a hotplate/dehydrator with more than 500 charges, so until the default battery is changed I'm giving player 10 of each
TEST_CASE( "recipes_inherit_rot_of_components_properly", "[crafting][rot]" )
{
    Character &player_character = get_player_character();
    std::vector<item> tools;
    tools.insert( tools.end(), 10, tool_with_ammo( "hotplate", 500 ) );
    tools.insert( tools.end(), 10, tool_with_ammo( "dehydrator", 500 ) );
    tools.emplace_back( "pot_canning" );
    tools.emplace_back( "knife_butcher" );

    GIVEN( "1 hour until rotten macaroni and fresh cheese" ) {

        item macaroni( "macaroni_raw" );
        item cheese( "cheese" );
        item water_clean( "water_clean" );

        macaroni.set_rot( macaroni.get_shelf_life() - 1_hours );
        REQUIRE( cheese.get_shelf_life() - cheese.get_rot() > 1_hours );

        tools.insert( tools.end(), 1, macaroni );
        tools.insert( tools.end(), 1, cheese );
        item &bottle = tools.emplace_back( "bottle_plastic" ); // water container
        bottle.get_contents().insert_item( item( itype_water_clean ), item_pocket::pocket_type::CONTAINER );

        WHEN( "crafting the mac and cheese" ) {
            prep_craft( recipe_macaroni_cooked, tools, true );
            actually_test_craft( recipe_macaroni_cooked, INT_MAX, 10 );

            THEN( "it should have exactly 1 hour until it spoils" ) {
                item_location mac_and_cheese = player_character.get_wielded_item();

                REQUIRE( mac_and_cheese->type->get_id() == recipe_macaroni_cooked->result() );

                CHECK( mac_and_cheese->get_shelf_life() - mac_and_cheese->get_rot() == 1_hours );
            }
        }
    }

    GIVEN( "fresh macaroni and fresh cheese" ) {
        item macaroni( "macaroni_raw" );
        item cheese( "cheese" );
        item water_clean( "water_clean" );

        REQUIRE( macaroni.get_rot() == 0_turns );
        REQUIRE( cheese.get_rot() == 0_turns );

        tools.insert( tools.end(), 1, macaroni );
        tools.insert( tools.end(), 1, cheese );
        item &bottle = tools.emplace_back( "bottle_plastic" ); // water container
        bottle.get_contents().insert_item( item( itype_water_clean ), item_pocket::pocket_type::CONTAINER );

        WHEN( "crafting the mac and cheese" ) {
            prep_craft( recipe_macaroni_cooked, tools, true );
            actually_test_craft( recipe_macaroni_cooked, INT_MAX, 10 );

            THEN( "it should have no rot" ) {
                item_location mac_and_cheese = player_character.get_wielded_item();

                REQUIRE( mac_and_cheese->type->get_id() == recipe_macaroni_cooked->result() );

                CHECK( mac_and_cheese->get_rot() == 0_turns );
            }
        }
    }

    GIVEN( "meat with 1 percent of its shelf life left" ) {
        item meat( "meat" );

        meat.set_relative_rot( 0.01 );

        tools.insert( tools.end(), 1, meat );

        WHEN( "crafting dehydrated meat" ) {
            prep_craft( recipe_dry_meat, tools, true );
            actually_test_craft( recipe_dry_meat, INT_MAX, 10 );

            THEN( "it should have 1 percent of its shelf life left" ) {
                item_location dehydrated_meat = player_character.get_wielded_item();

                REQUIRE( dehydrated_meat->type->get_id() == recipe_dry_meat->result() );

                CHECK( dehydrated_meat->get_relative_rot() == 0.01 );
            }
        }
    }
}

TEST_CASE( "variant_crafting_recipes", "[crafting][slow]" )
{
    constexpr int max_iters = 50;
    Character &player_character = get_player_character();

    SECTION( "crafting non-variant recipe" ) {
        std::map<std::string, int> variant_counts;
        for( int i = 0; i < max_iters; i++ ) {
            std::vector<item> tools;
            tools.emplace_back( itype_sewing_kit );
            tools.emplace_back( "scissors" );
            tools.insert( tools.end(), 10, item( "sheet_cotton" ) );
            tools.insert( tools.end(), 10, item( "thread" ) );
            prep_craft( recipe_test_waist_apron_long, tools, true );
            actually_test_craft( recipe_test_waist_apron_long, INT_MAX, 10 );
            item_location apron = player_character.get_wielded_item();

            REQUIRE( apron->type->get_id() == recipe_test_waist_apron_long->result() );
            REQUIRE( apron->has_itype_variant() );

            if( variant_counts.count( apron->itype_variant().id ) == 0 ) {
                variant_counts[apron->itype_variant().id] = 0;
            }
            variant_counts[apron->itype_variant().id]++;
        }
        int variants_captured = 0;
        for( const std::pair<std::string, int> var_count : variant_counts ) {
            CHECK( var_count.second < max_iters );
            variants_captured++;
        }
        CHECK( variants_captured > 1 );
    }

    SECTION( "crafting variant recipe" ) {
        int specific_variant_count = 0;
        for( int i = 0; i < max_iters; i++ ) {
            std::vector<item> tools;
            tools.emplace_back( itype_sewing_kit );
            tools.emplace_back( "scissors" );
            tools.insert( tools.end(), 10, item( "sheet_cotton" ) );
            tools.insert( tools.end(), 10, item( "thread" ) );
            prep_craft( recipe_test_waist_apron_long_pink_apron_cotton, tools, true );
            actually_test_craft( recipe_test_waist_apron_long_pink_apron_cotton, INT_MAX, 10 );
            item_location apron = player_character.get_wielded_item();

            REQUIRE( apron->type->get_id() == recipe_test_waist_apron_long_pink_apron_cotton->result() );
            REQUIRE( apron->has_itype_variant() );

            if( apron->itype_variant().id == "pink_apron_cotton" ) {
                specific_variant_count++;
            }
        }
        CHECK( specific_variant_count == max_iters );
    }
}
