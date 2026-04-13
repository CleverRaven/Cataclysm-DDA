#include <algorithm>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "character_attire.h"
#include "color.h"
#include "crafting_gui_helpers.h"
#include "flat_set.h"
#include "npc.h"
#include "recipe_dictionary.h"
#include "game.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "options_helpers.h"
#include "player_helpers.h"
#include "proficiency.h"
#include "recipe.h"
#include "type_id.h"
#include "uistate.h"
#include "weather_type.h"

static const itype_id itype_2x4( "2x4" );
static const itype_id itype_billet_wood( "billet_wood" );
static const itype_id itype_cudgel( "cudgel" );
static const itype_id itype_debug_backpack( "debug_backpack" );
static const itype_id itype_family_cookbook( "family_cookbook" );
static const itype_id itype_fat( "fat" );
static const itype_id itype_hammer( "hammer" );
static const itype_id itype_knife_hunting( "knife_hunting" );
static const itype_id itype_rock( "rock" );

static const proficiency_id proficiency_prof_carving( "prof_carving" );
static const proficiency_id proficiency_prof_food_prep( "prof_food_prep" );
static const proficiency_id proficiency_prof_knapping( "prof_knapping" );

static const recipe_id recipe_cudgel_simple( "cudgel_simple" );
static const recipe_id recipe_cudgel_slow( "cudgel_slow" );
static const recipe_id recipe_cudgel_test_no_tools( "cudgel_test_no_tools" );
static const recipe_id recipe_meat_cooked_test_no_tools( "meat_cooked_test_no_tools" );
static const recipe_id recipe_prac_knapping( "prac_knapping" );
static const recipe_id recipe_test_longshirt_test_poor_fit( "test_longshirt_test_poor_fit" );
static const recipe_id recipe_test_nested_weapons( "test_nested_weapons" );
static const recipe_id recipe_test_tallow( "test_tallow" );
static const recipe_id recipe_water_clean_test_in_jar( "water_clean_test_in_jar" );

static const skill_id skill_cooking( "cooking" );
static const skill_id skill_fabrication( "fabrication" );
static const skill_id skill_melee( "melee" );
static const skill_id skill_survival( "survival" );

// Helper: clear everything and return the avatar with a debug backpack
// so that i_add() has pockets to place items into (clear_avatar strips
// all worn items, leaving no storage capacity).
static Character &setup_character()
{
    clear_avatar();
    clear_map_without_vision();
    Character &guy = get_avatar();
    guy.worn.wear_item( guy, item( itype_debug_backpack ), false, false );
    guy.invalidate_crafting_inventory();
    return guy;
}


TEST_CASE( "recipe_availability_has_components_and_skills", "[crafting][gui]" )
{
    Character &guy = setup_character();
    guy.set_skill_level( skill_cooking, 3 );
    guy.i_add( item( itype_fat ) );
    guy.i_add( item( itype_fat ) );
    guy.i_add( item( itype_knife_hunting ) ); // CUT 2 quality
    guy.invalidate_crafting_inventory();

    const recipe &rec = recipe_test_tallow.obj();
    availability avail( guy, &rec );

    CHECK( avail.can_craft );
    CHECK( avail.has_all_skills );
    CHECK( avail.color() == c_white );
}

TEST_CASE( "recipe_availability_missing_component", "[crafting][gui]" )
{
    Character &guy = setup_character();
    guy.set_skill_level( skill_cooking, 3 );
    guy.i_add( item( itype_knife_hunting ) ); // has tool but no fat
    guy.invalidate_crafting_inventory();

    const recipe &rec = recipe_test_tallow.obj();
    availability avail( guy, &rec );

    CHECK_FALSE( avail.can_craft );
    CHECK( avail.color() == c_dark_gray );
}

TEST_CASE( "recipe_availability_insufficient_skill", "[crafting][gui]" )
{
    Character &guy = setup_character();
    guy.set_skill_level( skill_cooking, 0 );
    guy.i_add( item( itype_fat ) );
    guy.i_add( item( itype_fat ) );
    guy.i_add( item( itype_knife_hunting ) );
    guy.invalidate_crafting_inventory();

    const recipe &rec = recipe_test_tallow.obj();
    availability avail( guy, &rec );

    // can_craft gates on components, not skill level (test_tallow is not practice)
    CHECK( avail.can_craft );
    CHECK_FALSE( avail.has_all_skills );
    // cooking 0 vs difficulty 3: knowledge_level < difficulty*0.8, so no primary skill
    CHECK_FALSE( avail.crafter_has_primary_skill );
    CHECK( avail.color() == c_light_red );
}

TEST_CASE( "recipe_availability_would_use_rotten", "[crafting][gui]" )
{
    Character &guy = setup_character();
    guy.set_skill_level( skill_cooking, 3 );
    item rotten_fat1( itype_fat );
    rotten_fat1.set_rot( 1000_hours ); // force rotten
    guy.i_add( rotten_fat1 );
    item rotten_fat2( itype_fat );
    rotten_fat2.set_rot( 1000_hours );
    guy.i_add( rotten_fat2 );
    guy.i_add( item( itype_knife_hunting ) );
    guy.invalidate_crafting_inventory();

    const recipe &rec = recipe_test_tallow.obj();
    availability avail( guy, &rec );

    CHECK( avail.would_use_rotten );
    // has_all_skills is true, so rotten color is c_brown
    CHECK( avail.color() == c_brown );
}

TEST_CASE( "recipe_availability_would_use_favorite", "[crafting][gui]" )
{
    Character &guy = setup_character();
    guy.set_skill_level( skill_fabrication, 2 );
    guy.set_skill_level( skill_melee, 1 );
    item fav_2x4( itype_2x4 );
    fav_2x4.set_favorite( true );
    guy.i_add( fav_2x4 );
    guy.invalidate_crafting_inventory();

    const recipe &rec = recipe_cudgel_test_no_tools.obj();
    availability avail( guy, &rec );

    CHECK( avail.can_craft );
    CHECK( avail.would_use_favorite );
    CHECK( avail.color() == c_pink );
}

TEST_CASE( "recipe_availability_useless_practice", "[crafting][gui]" )
{
    Character &guy = setup_character();
    // prac_knapping: survival practice, skill_limit 2, requires fabrication 1
    // also uses prof_knapping
    guy.set_skill_level( skill_survival, 3 ); // above skill_limit
    guy.set_skill_level( skill_fabrication, 1 );
    guy.add_proficiency( proficiency_prof_knapping );
    // Tools: HAMMER 1 + HAMMER_SOFT 1
    guy.i_add( item( itype_hammer ) );     // HAMMER quality
    guy.i_add( item( itype_billet_wood ) ); // HAMMER_SOFT quality
    // Components: rock
    guy.i_add( item( itype_rock ) );
    guy.i_add( item( itype_rock ) );
    guy.invalidate_crafting_inventory();

    const recipe &rec = recipe_prac_knapping.obj();
    availability avail( guy, &rec );

    CHECK( avail.useless_practice );
}

TEST_CASE( "recipe_availability_nested_craftable", "[crafting][gui]" )
{
    Character &guy = setup_character();
    guy.set_skill_level( skill_fabrication, 2 );
    guy.set_skill_level( skill_melee, 1 );
    guy.i_add( item( itype_2x4 ) ); // child recipe cudgel_test_no_tools needs 2x4
    guy.invalidate_crafting_inventory();

    const recipe &rec = recipe_test_nested_weapons.obj();
    availability avail( guy, &rec );

    CHECK( avail.is_nested_category );
    CHECK( avail.can_craft ); // at least one child is craftable
    CHECK( avail.color() == c_light_blue );
}

TEST_CASE( "recipe_availability_nested_none_craftable", "[crafting][gui]" )
{
    Character &guy = setup_character();
    guy.set_skill_level( skill_fabrication, 2 );
    guy.set_skill_level( skill_melee, 1 );
    // No 2x4 -> child recipe is not craftable
    guy.invalidate_crafting_inventory();

    const recipe &rec = recipe_test_nested_weapons.obj();
    availability avail( guy, &rec );

    CHECK( avail.is_nested_category );
    CHECK_FALSE( avail.can_craft );
    CHECK( avail.color() == c_blue );
}

TEST_CASE( "recipe_availability_proficiency_cache", "[crafting][gui]" )
{
    Character &guy = setup_character();
    guy.set_skill_level( skill_fabrication, 0 );
    guy.i_add( item( itype_2x4 ) );
    // Do NOT grant prof_carving -- malus should be > 1.0
    guy.invalidate_crafting_inventory();

    const recipe &rec = recipe_cudgel_simple.obj();
    availability avail( guy, &rec );

    float malus1 = avail.get_proficiency_time_maluses();
    float malus2 = avail.get_proficiency_time_maluses();
    CHECK( malus1 >= 1.0f );
    CHECK( malus1 == malus2 ); // lazy cache returns same value
}


TEST_CASE( "can_start_craft_ok", "[crafting][gui]" )
{
    Character &guy = setup_character();
    guy.set_skill_level( skill_fabrication, 2 );
    guy.set_skill_level( skill_melee, 1 );
    guy.i_add( item( itype_2x4 ) );
    guy.invalidate_crafting_inventory();
    // Midday for good lighting
    scoped_weather_override weather_clear( WEATHER_CLEAR );
    calendar::turn = calendar::turn_zero + 9_hours + 30_minutes;
    g->reset_light_level();
    get_map().build_map_cache( 0, false );

    const recipe &rec = recipe_cudgel_test_no_tools.obj();
    availability avail( guy, &rec );

    CHECK( can_start_craft( rec, avail, guy ) == craft_confirm_result::ok );
}

TEST_CASE( "can_start_craft_cannot_craft_no_components", "[crafting][gui]" )
{
    Character &guy = setup_character();
    guy.set_skill_level( skill_fabrication, 2 );
    guy.set_skill_level( skill_melee, 1 );
    // No 2x4 in inventory
    guy.invalidate_crafting_inventory();
    scoped_weather_override weather_clear( WEATHER_CLEAR );
    calendar::turn = calendar::turn_zero + 9_hours + 30_minutes;
    g->reset_light_level();
    get_map().build_map_cache( 0, false );

    const recipe &rec = recipe_cudgel_test_no_tools.obj();
    availability avail( guy, &rec );

    CHECK_FALSE( avail.can_craft );
    CHECK( can_start_craft( rec, avail, guy ) == craft_confirm_result::cannot_craft );
}

TEST_CASE( "can_start_craft_cannot_craft_no_primary_skill", "[crafting][gui]" )
{
    Character &guy = setup_character();
    guy.set_skill_level( skill_fabrication, 0 );
    guy.set_skill_level( skill_melee, 0 );
    guy.i_add( item( itype_2x4 ) );
    guy.invalidate_crafting_inventory();
    scoped_weather_override weather_clear( WEATHER_CLEAR );
    calendar::turn = calendar::turn_zero + 9_hours + 30_minutes;
    g->reset_light_level();
    get_map().build_map_cache( 0, false );

    const recipe &rec = recipe_cudgel_test_no_tools.obj();
    availability avail( guy, &rec );

    // Has components so can_craft is true, but primary skill is too low
    // knowledge_level(0) < difficulty(2) * 0.8 = 1.6
    CHECK( avail.can_craft );
    CHECK_FALSE( avail.crafter_has_primary_skill );
    CHECK( can_start_craft( rec, avail, guy ) == craft_confirm_result::cannot_craft );
}

TEST_CASE( "can_start_craft_too_dark", "[crafting][gui]" )
{
    Character &guy = setup_character();
    guy.set_skill_level( skill_fabrication, 2 );
    guy.set_skill_level( skill_melee, 1 );
    guy.i_add( item( itype_2x4 ) );
    guy.invalidate_crafting_inventory();
    // Midnight, no light
    scoped_weather_override weather_clear( WEATHER_CLEAR );
    calendar::turn = calendar::turn_zero;
    g->reset_light_level();
    get_map().build_map_cache( 0, false );

    const recipe &rec = recipe_cudgel_test_no_tools.obj();
    availability avail( guy, &rec );

    CHECK( avail.can_craft );
    CHECK( avail.crafter_has_primary_skill );
    CHECK( can_start_craft( rec, avail, guy ) == craft_confirm_result::too_dark );
}

TEST_CASE( "can_start_craft_ok_at_midday", "[crafting][gui]" )
{
    Character &guy = setup_character();
    guy.set_skill_level( skill_fabrication, 2 );
    guy.set_skill_level( skill_melee, 1 );
    guy.i_add( item( itype_2x4 ) );
    guy.invalidate_crafting_inventory();
    // Midday, good light
    scoped_weather_override weather_clear( WEATHER_CLEAR );
    calendar::turn = calendar::turn_zero + 9_hours + 30_minutes;
    g->reset_light_level();
    get_map().build_map_cache( 0, false );

    const recipe &rec = recipe_cudgel_test_no_tools.obj();
    availability avail( guy, &rec );

    CHECK( can_start_craft( rec, avail, guy ) == craft_confirm_result::ok );
}


TEST_CASE( "recipe_sort_craftable_before_uncraftable", "[crafting][gui]" )
{
    Character &guy = setup_character();
    guy.set_skill_level( skill_fabrication, 2 );
    guy.set_skill_level( skill_melee, 1 );
    guy.i_add( item( itype_2x4 ) );
    guy.invalidate_crafting_inventory();

    const recipe &rec = recipe_cudgel_test_no_tools.obj();
    availability avail_yes( guy, &rec );
    CHECK( avail_yes.can_craft );

    // Remove the component so a second availability is not craftable
    guy.remove_items_with( []( const item & it ) {
        return it.typeId() == itype_2x4;
    } );
    guy.invalidate_crafting_inventory();
    availability avail_no( guy, &rec );
    CHECK_FALSE( avail_no.can_craft );

    CHECK( recipe_sort_compare( &rec, &rec, avail_yes, avail_no, guy, {},
                                true, true, false ) );
    CHECK_FALSE( recipe_sort_compare( &rec, &rec, avail_no, avail_yes, guy, {},
                                      true, true, false ) );
}

TEST_CASE( "recipe_sort_by_difficulty", "[crafting][gui]" )
{
    Character &guy = setup_character();
    guy.set_skill_level( skill_fabrication, 2 );
    guy.set_skill_level( skill_melee, 1 );
    guy.add_proficiency( proficiency_prof_carving );
    guy.i_add( item( itype_2x4 ) );
    guy.invalidate_crafting_inventory();

    const recipe &rec_hard = recipe_cudgel_test_no_tools.obj();
    const recipe &rec_easy = recipe_cudgel_simple.obj();
    availability avail_hard( guy, &rec_hard );
    availability avail_easy( guy, &rec_easy );

    // Higher difficulty sorts first (existing behavior: b->difficulty < a->difficulty)
    CHECK( recipe_sort_compare( &rec_hard, &rec_easy, avail_hard, avail_easy, guy, {},
                                true, true, false ) );
}

TEST_CASE( "recipe_sort_by_name", "[crafting][gui]" )
{
    Character &guy = setup_character();
    guy.set_skill_level( skill_fabrication, 0 );
    guy.set_skill_level( skill_cooking, 0 );
    guy.add_proficiency( proficiency_prof_carving );
    guy.add_proficiency( proficiency_prof_food_prep );
    guy.i_add( item( itype_2x4 ) );
    guy.invalidate_crafting_inventory();

    const recipe &rec_cudgel = recipe_cudgel_simple.obj();
    const recipe &rec_meat = recipe_meat_cooked_test_no_tools.obj();
    availability avail_cudgel( guy, &rec_cudgel );
    availability avail_meat( guy, &rec_meat );

    // "cooked meat" < "cudgel" alphabetically
    CHECK( recipe_sort_compare( &rec_meat, &rec_cudgel, avail_meat, avail_cudgel, guy, {},
                                true, true, false ) );
    CHECK_FALSE( recipe_sort_compare( &rec_cudgel, &rec_meat, avail_cudgel, avail_meat, guy, {},
                                      true, true, false ) );
}

TEST_CASE( "recipe_sort_by_craft_time_tiebreaker", "[crafting][gui]" )
{
    Character &guy = setup_character();
    guy.set_skill_level( skill_fabrication, 0 );
    guy.add_proficiency( proficiency_prof_carving );
    guy.i_add( item( itype_2x4 ) );
    guy.invalidate_crafting_inventory();

    const recipe &rec_fast = recipe_cudgel_simple.obj();
    const recipe &rec_slow = recipe_cudgel_slow.obj();
    availability avail_fast( guy, &rec_fast );
    availability avail_slow( guy, &rec_slow );

    // Same name, same difficulty -> longer craft time sorts first
    CHECK( recipe_sort_compare( &rec_slow, &rec_fast, avail_slow, avail_fast, guy, {},
                                true, true, false ) );
}

TEST_CASE( "recipe_sort_unread_first", "[crafting][gui]" )
{
    Character &guy = setup_character();
    guy.set_skill_level( skill_fabrication, 2 );
    guy.set_skill_level( skill_melee, 1 );
    guy.i_add( item( itype_2x4 ) );
    guy.invalidate_crafting_inventory();

    const recipe &rec = recipe_cudgel_test_no_tools.obj();
    availability avail( guy, &rec );

    // a_read=false (unread), b_read=true (read), unread_first=true -> a sorts before b
    CHECK( recipe_sort_compare( &rec, &rec, avail, avail, guy, {},
                                false, true, true ) );
    // a_read=true (read), b_read=false (unread) -> b should sort first, so a < b is false
    CHECK_FALSE( recipe_sort_compare( &rec, &rec, avail, avail, guy, {},
                                      true, false, true ) );
}

TEST_CASE( "recipe_sort_unread_disabled", "[crafting][gui]" )
{
    Character &guy = setup_character();
    guy.set_skill_level( skill_fabrication, 2 );
    guy.set_skill_level( skill_melee, 1 );
    guy.i_add( item( itype_2x4 ) );
    guy.invalidate_crafting_inventory();

    const recipe &rec = recipe_cudgel_test_no_tools.obj();
    availability avail_yes( guy, &rec );

    guy.remove_items_with( []( const item & it ) {
        return it.typeId() == itype_2x4;
    } );
    guy.invalidate_crafting_inventory();
    availability avail_no( guy, &rec );

    // unread_first=false: read state ignored, craftability dominates
    CHECK( recipe_sort_compare( &rec, &rec, avail_yes, avail_no, guy, {},
                                true, false, false ) );
}

// Helper: join vector of strings for substring searching
static std::string join_lines( const std::vector<std::string> &lines )
{
    std::string result;
    for( const std::string &l : lines ) {
        result += l + "\n";
    }
    return result;
}

// Wide fold width to avoid wrapping artifacts in tests
static const int test_fold_width = 500;

TEST_CASE( "recipe_info_basic_content", "[crafting][gui]" )
{
    Character &guy = setup_character();
    guy.set_skill_level( skill_cooking, 3 );
    guy.i_add( item( itype_fat ) );
    guy.i_add( item( itype_fat ) );
    guy.i_add( item( itype_knife_hunting ) );
    guy.invalidate_crafting_inventory();

    const recipe &rec = recipe_test_tallow.obj();
    availability avail( guy, &rec );
    REQUIRE( avail.can_craft );
    REQUIRE( avail.has_all_skills );

    std::vector<Character *> group = { &guy };
    std::string output = join_lines(
                             recipe_info( rec, avail, guy, "", 1, test_fold_width, c_white, group ) );

    CHECK( output.find( "Primary skill:" ) != std::string::npos );
    CHECK( output.find( "Time to complete:" ) != std::string::npos );
    CHECK( output.find( "Activity level:" ) != std::string::npos );
    CHECK( output.find( "Craftable in the dark?" ) != std::string::npos );
    CHECK( output.find( "Minor Failure Chance:" ) != std::string::npos );
    CHECK( output.find( "Catastrophic Failure Chance:" ) != std::string::npos );
}

TEST_CASE( "recipe_info_byproducts", "[crafting][gui]" )
{
    Character &guy = setup_character();
    guy.set_skill_level( skill_cooking, 3 );
    guy.i_add( item( itype_fat ) );
    guy.i_add( item( itype_fat ) );
    guy.i_add( item( itype_knife_hunting ) );
    guy.invalidate_crafting_inventory();

    const recipe &rec = recipe_test_tallow.obj();
    REQUIRE( rec.has_byproducts() );
    availability avail( guy, &rec );

    std::vector<Character *> group = { &guy };
    std::string output = join_lines(
                             recipe_info( rec, avail, guy, "", 1, test_fold_width, c_white, group ) );

    CHECK( output.find( "Byproducts:" ) != std::string::npos );
    CHECK( output.find( "cracklins" ) != std::string::npos );
    CHECK( output.find( "test chewing gum" ) != std::string::npos );
}

TEST_CASE( "recipe_info_recipe_makes", "[crafting][gui]" )
{
    Character &guy = setup_character();
    guy.set_skill_level( skill_cooking, 3 );
    guy.i_add( item( itype_fat ) );
    guy.i_add( item( itype_fat ) );
    guy.i_add( item( itype_knife_hunting ) );
    guy.invalidate_crafting_inventory();

    const recipe &rec = recipe_test_tallow.obj();
    REQUIRE( rec.makes_amount() > 1 );
    availability avail( guy, &rec );

    std::vector<Character *> group = { &guy };
    std::string output = join_lines(
                             recipe_info( rec, avail, guy, "", 1, test_fold_width, c_white, group ) );

    CHECK( output.find( "Recipe makes:" ) != std::string::npos );
}

TEST_CASE( "recipe_info_missing_primary_skill", "[crafting][gui]" )
{
    Character &guy = setup_character();
    guy.set_skill_level( skill_fabrication, 0 );
    guy.set_skill_level( skill_melee, 0 );
    guy.i_add( item( itype_2x4 ) );
    guy.invalidate_crafting_inventory();

    const recipe &rec = recipe_cudgel_test_no_tools.obj();
    availability avail( guy, &rec );
    REQUIRE_FALSE( avail.crafter_has_primary_skill );

    std::vector<Character *> group = { &guy };
    std::string output = join_lines(
                             recipe_info( rec, avail, guy, "", 1, test_fold_width, c_white, group ) );

    CHECK( output.find( "lack the theoretical knowledge" ) != std::string::npos );
}

TEST_CASE( "recipe_info_would_use_rotten", "[crafting][gui]" )
{
    Character &guy = setup_character();
    guy.set_skill_level( skill_cooking, 3 );
    item rotten_fat1( itype_fat );
    rotten_fat1.set_rot( 1000_hours );
    guy.i_add( rotten_fat1 );
    item rotten_fat2( itype_fat );
    rotten_fat2.set_rot( 1000_hours );
    guy.i_add( rotten_fat2 );
    guy.i_add( item( itype_knife_hunting ) );
    guy.invalidate_crafting_inventory();

    const recipe &rec = recipe_test_tallow.obj();
    availability avail( guy, &rec );
    REQUIRE( avail.can_craft );
    REQUIRE( avail.would_use_rotten );

    std::vector<Character *> group = { &guy };
    std::string output = join_lines(
                             recipe_info( rec, avail, guy, "", 1, test_fold_width, c_white, group ) );

    CHECK( output.find( "Will use rotten ingredients" ) != std::string::npos );
}

TEST_CASE( "recipe_info_would_use_favorite", "[crafting][gui]" )
{
    Character &guy = setup_character();
    guy.set_skill_level( skill_fabrication, 2 );
    guy.set_skill_level( skill_melee, 1 );
    item fav_2x4( itype_2x4 );
    fav_2x4.set_favorite( true );
    guy.i_add( fav_2x4 );
    guy.invalidate_crafting_inventory();

    const recipe &rec = recipe_cudgel_test_no_tools.obj();
    availability avail( guy, &rec );
    REQUIRE( avail.can_craft );
    REQUIRE( avail.would_use_favorite );

    std::vector<Character *> group = { &guy };
    std::string output = join_lines(
                             recipe_info( rec, avail, guy, "", 1, test_fold_width, c_white, group ) );

    CHECK( output.find( "Will use favorited ingredients" ) != std::string::npos );
}

TEST_CASE( "recipe_info_proficiency_bonus", "[crafting][gui]" )
{
    Character &guy = setup_character();
    guy.set_skill_level( skill_fabrication, 2 );
    guy.add_proficiency( proficiency_prof_carving );
    guy.i_add( item( itype_2x4 ) );
    guy.invalidate_crafting_inventory();

    const recipe &rec = recipe_cudgel_simple.obj();
    availability avail( guy, &rec );
    REQUIRE( avail.get_proficiency_time_maluses() < avail.get_max_proficiency_time_maluses() );

    std::vector<Character *> group = { &guy };
    std::string output = join_lines(
                             recipe_info( rec, avail, guy, "", 1, test_fold_width, c_white, group ) );

    CHECK( output.find( "faster than normal" ) != std::string::npos );
}

TEST_CASE( "recipe_info_not_memorized", "[crafting][gui]" )
{
    Character &guy = setup_character();
    guy.set_skill_level( skill_cooking, 3 );
    guy.i_add( item( itype_fat ) );
    guy.i_add( item( itype_fat ) );
    guy.i_add( item( itype_knife_hunting ) );
    guy.invalidate_crafting_inventory();

    const recipe &rec = recipe_test_tallow.obj();
    REQUIRE_FALSE( guy.knows_recipe( &rec ) );
    availability avail( guy, &rec );

    std::vector<Character *> group = { &guy };
    std::string output = join_lines(
                             recipe_info( rec, avail, guy, "", 1, test_fold_width, c_white, group ) );

    CHECK( output.find( "Recipe not memorized yet" ) != std::string::npos );
}

TEST_CASE( "recipe_info_nested_skips_time", "[crafting][gui]" )
{
    Character &guy = setup_character();
    guy.set_skill_level( skill_fabrication, 2 );
    guy.set_skill_level( skill_melee, 1 );
    guy.i_add( item( itype_2x4 ) );
    guy.invalidate_crafting_inventory();

    const recipe &rec = recipe_test_nested_weapons.obj();
    REQUIRE( rec.is_nested() );
    availability avail( guy, &rec );

    std::vector<Character *> group = { &guy };
    std::string output = join_lines(
                             recipe_info( rec, avail, guy, "", 1, test_fold_width, c_white, group ) );

    CHECK( output.find( "Time to complete:" ) == std::string::npos );
}

TEST_CASE( "practice_recipe_description_above_limit", "[crafting][gui]" )
{
    Character &guy = setup_character();
    guy.set_skill_level( skill_survival, 3 ); // above skill_limit 2

    const recipe &rec = recipe_prac_knapping.obj();
    REQUIRE( rec.practice_data );

    std::string output = practice_recipe_description( rec, guy );

    CHECK( output.find( "will not increase" ) != std::string::npos );
    CHECK( output.find( "<color_brown>" ) != std::string::npos );
}

TEST_CASE( "practice_recipe_description_below_limit", "[crafting][gui]" )
{
    Character &guy = setup_character();
    guy.set_skill_level( skill_survival, 1 ); // below skill_limit 2

    const recipe &rec = recipe_prac_knapping.obj();
    REQUIRE( rec.practice_data );

    std::string output = practice_recipe_description( rec, guy );

    CHECK( output.find( "Practice basic knapping" ) != std::string::npos );
    CHECK( output.find( "Difficulty range:" ) != std::string::npos );
    CHECK( output.find( "will not increase" ) != std::string::npos );
    CHECK( output.find( "<color_brown>" ) == std::string::npos );
}

TEST_CASE( "recipe_info_known_by_helper", "[crafting][gui]" )
{
    Character &guy = setup_character();
    guy.set_skill_level( skill_cooking, 3 );
    guy.i_add( item( itype_fat ) );
    guy.i_add( item( itype_fat ) );
    guy.i_add( item( itype_knife_hunting ) );
    guy.invalidate_crafting_inventory();

    const recipe &rec = recipe_test_tallow.obj();
    REQUIRE_FALSE( guy.knows_recipe( &rec ) );

    // Create an NPC helper who knows the recipe
    standard_npc helper( "TestHelper" );
    helper.learn_recipe( &rec );
    REQUIRE( helper.knows_recipe( &rec ) );

    availability avail( guy, &rec );
    std::vector<Character *> group = { &guy, &helper };
    std::string output = join_lines(
                             recipe_info( rec, avail, guy, "", 1, test_fold_width, c_white, group ) );

    CHECK( output.find( "Recipe not memorized yet" ) != std::string::npos );
    CHECK( output.find( "Known by:" ) != std::string::npos );
    CHECK( output.find( "TestHelper" ) != std::string::npos );
}

TEST_CASE( "recipe_info_written_in_book", "[crafting][gui]" )
{
    Character &guy = setup_character();
    guy.set_skill_level( skill_cooking, 3 );
    guy.i_add( item( itype_fat ) );
    guy.i_add( item( itype_fat ) );
    guy.i_add( item( itype_knife_hunting ) );
    item cookbook( itype_family_cookbook );
    guy.identify( cookbook );
    guy.i_add( cookbook );
    guy.invalidate_crafting_inventory();

    const recipe &rec = recipe_test_tallow.obj();
    REQUIRE_FALSE( guy.knows_recipe( &rec ) );

    availability avail( guy, &rec );
    std::vector<Character *> group = { &guy };
    std::string output = join_lines(
                             recipe_info( rec, avail, guy, "", 1, test_fold_width, c_white, group ) );

    CHECK( output.find( "Recipe not memorized yet" ) != std::string::npos );
    CHECK( output.find( "Written in:" ) != std::string::npos );
}

TEST_CASE( "practice_recipe_description_non_practice_recipe", "[crafting][gui]" )
{
    Character &guy = setup_character();
    guy.set_skill_level( skill_cooking, 3 );

    const recipe &rec = recipe_test_tallow.obj();
    REQUIRE_FALSE( rec.practice_data );

    // Should not crash -- returns just the base description
    std::string output = practice_recipe_description( rec, guy );
    CHECK_FALSE( output.empty() );
}

// --- Filter engine tests ---

static const std::function<void( size_t, size_t )> no_progress;

static recipe_subset make_subset( const std::vector<const recipe *> &recipes )
{
    recipe_subset subset;
    for( const recipe *r : recipes ) {
        subset.include( r );
    }
    return subset;
}

TEST_CASE( "filter_by_name", "[crafting][gui][filter]" )
{
    Character &guy = setup_character();
    const recipe *cudgel = &recipe_cudgel_test_no_tools.obj();
    const recipe *tallow = &recipe_test_tallow.obj();
    recipe_subset subset = make_subset( { cudgel, tallow } );

    recipe_subset result = filter_recipes( subset, "cudgel", guy, no_progress );

    CHECK( result.contains( cudgel ) );
    CHECK_FALSE( result.contains( tallow ) );
}

TEST_CASE( "filter_by_name_exclude", "[crafting][gui][filter]" )
{
    Character &guy = setup_character();
    const recipe *cudgel = &recipe_cudgel_test_no_tools.obj();
    const recipe *tallow = &recipe_test_tallow.obj();
    recipe_subset subset = make_subset( { cudgel, tallow } );

    recipe_subset result = filter_recipes( subset, "-cudgel", guy, no_progress );

    CHECK( result.contains( tallow ) );
    CHECK_FALSE( result.contains( cudgel ) );
}

TEST_CASE( "filter_by_component", "[crafting][gui][filter]" )
{
    Character &guy = setup_character();
    const recipe *cudgel = &recipe_cudgel_test_no_tools.obj();
    const recipe *tallow = &recipe_test_tallow.obj();
    recipe_subset subset = make_subset( { cudgel, tallow } );

    // cudgel uses 2x4, tallow uses fat
    recipe_subset result = filter_recipes( subset, "c:fat", guy, no_progress );

    CHECK( result.contains( tallow ) );
    CHECK_FALSE( result.contains( cudgel ) );
}

TEST_CASE( "filter_by_primary_skill", "[crafting][gui][filter]" )
{
    Character &guy = setup_character();
    const recipe *cudgel = &recipe_cudgel_test_no_tools.obj();
    const recipe *tallow = &recipe_test_tallow.obj();
    recipe_subset subset = make_subset( { cudgel, tallow } );

    // cudgel uses fabrication, tallow uses cooking
    recipe_subset result = filter_recipes( subset, "p:fabrication", guy, no_progress );

    CHECK( result.contains( cudgel ) );
    CHECK_FALSE( result.contains( tallow ) );
}

TEST_CASE( "filter_by_skill", "[crafting][gui][filter]" )
{
    Character &guy = setup_character();
    const recipe *cudgel = &recipe_cudgel_test_no_tools.obj();
    const recipe *tallow = &recipe_test_tallow.obj();
    recipe_subset subset = make_subset( { cudgel, tallow } );

    // cooking skill's display name is "food handling"
    recipe_subset result = filter_recipes( subset, "s:food handling", guy, no_progress );

    CHECK( result.contains( tallow ) );
    CHECK_FALSE( result.contains( cudgel ) );
}

TEST_CASE( "filter_by_difficulty_range", "[crafting][gui][filter]" )
{
    Character &guy = setup_character();
    const recipe *cudgel = &recipe_cudgel_test_no_tools.obj();
    const recipe *tallow = &recipe_test_tallow.obj();
    recipe_subset subset = make_subset( { cudgel, tallow } );

    // cudgel diff 2, tallow diff 3
    recipe_subset result = filter_recipes( subset, "l:3~5", guy, no_progress );

    CHECK( result.contains( tallow ) );
    CHECK_FALSE( result.contains( cudgel ) );
}

TEST_CASE( "filter_comma_chaining", "[crafting][gui][filter]" )
{
    Character &guy = setup_character();
    const recipe *cudgel = &recipe_cudgel_test_no_tools.obj();
    const recipe *tallow = &recipe_test_tallow.obj();
    const recipe *meat = &recipe_meat_cooked_test_no_tools.obj();
    recipe_subset subset = make_subset( { cudgel, tallow, meat } );

    // p:food handling narrows to tallow + meat (both use cooking aka "food handling"),
    // then -tallow excludes tallow
    recipe_subset result = filter_recipes( subset, "p:food handling, -tallow", guy, no_progress );

    CHECK( result.contains( meat ) );
    CHECK_FALSE( result.contains( tallow ) );
    CHECK_FALSE( result.contains( cudgel ) );
}

TEST_CASE( "filter_memorized_yes", "[crafting][gui][filter]" )
{
    Character &guy = setup_character();
    const recipe *meat = &recipe_meat_cooked_test_no_tools.obj();
    // test_tallow is autolearn:false, book_learn only
    const recipe *tallow = &recipe_test_tallow.obj();
    // Explicitly learn one recipe instead of relying on autolearn cache
    guy.learn_recipe( meat );

    REQUIRE( guy.knows_recipe( meat ) );
    REQUIRE_FALSE( guy.knows_recipe( tallow ) );

    recipe_subset subset = make_subset( { meat, tallow } );
    recipe_subset result = filter_recipes( subset, "m:yes", guy, no_progress );

    CHECK( result.contains( meat ) );
    CHECK_FALSE( result.contains( tallow ) );
}

TEST_CASE( "filter_memorized_no", "[crafting][gui][filter]" )
{
    Character &guy = setup_character();
    const recipe *meat = &recipe_meat_cooked_test_no_tools.obj();
    const recipe *tallow = &recipe_test_tallow.obj();
    guy.learn_recipe( meat );

    REQUIRE( guy.knows_recipe( meat ) );
    REQUIRE_FALSE( guy.knows_recipe( tallow ) );

    recipe_subset subset = make_subset( { meat, tallow } );
    recipe_subset result = filter_recipes( subset, "m:no", guy, no_progress );

    CHECK( result.contains( tallow ) );
    CHECK_FALSE( result.contains( meat ) );
}

TEST_CASE( "filter_by_book", "[crafting][gui][filter]" )
{
    Character &guy = setup_character();
    guy.set_skill_level( skill_cooking, 3 );
    item cookbook( itype_family_cookbook );
    guy.identify( cookbook );
    guy.i_add( cookbook );
    guy.invalidate_crafting_inventory();

    // test_tallow has book_learn: family_cookbook:1
    const recipe *tallow = &recipe_test_tallow.obj();
    // cudgel has no book_learn
    const recipe *cudgel = &recipe_cudgel_test_no_tools.obj();
    recipe_subset subset = make_subset( { tallow, cudgel } );

    recipe_subset result = filter_recipes( subset, "b:family", guy, no_progress );

    CHECK( result.contains( tallow ) );
    CHECK_FALSE( result.contains( cudgel ) );
}

TEST_CASE( "filter_r_prefix_replaces_not_intersects", "[crafting][gui][filter]" )
{
    // Characterization test: the r: branch rebuilds from available_recipes
    // (the unfiltered superset), not from filtered_recipes.
    // This means prior filters in a comma chain are lost.
    Character &guy = setup_character();
    const recipe *cudgel = &recipe_cudgel_test_no_tools.obj();
    const recipe *tallow = &recipe_test_tallow.obj();
    const recipe *meat = &recipe_meat_cooked_test_no_tools.obj();
    recipe_subset subset = make_subset( { cudgel, tallow, meat } );

    // p:food handling narrows to tallow + meat, then r:cudgel replaces with
    // recipes producing items matching "cudgel" from available_recipes
    recipe_subset result = filter_recipes( subset, "p:food handling, r:cudgel", guy, no_progress );

    // r: searched available_recipes, ignoring the p:cooking narrowing
    CHECK( result.contains( cudgel ) );
    CHECK_FALSE( result.contains( tallow ) );
    CHECK_FALSE( result.contains( meat ) );
}

TEST_CASE( "filter_difficulty_malformed_matches_all", "[crafting][gui][filter]" )
{
    // Characterization test: malformed difficulty input matches everything.
    // Non-digit, non-tilde chars trigger early return true in the
    // recipe_subset::search difficulty parser.
    Character &guy = setup_character();
    const recipe *cudgel = &recipe_cudgel_test_no_tools.obj();
    const recipe *tallow = &recipe_test_tallow.obj();
    recipe_subset subset = make_subset( { cudgel, tallow } );

    recipe_subset result = filter_recipes( subset, "l:abc", guy, no_progress );

    CHECK( result.contains( cudgel ) );
    CHECK( result.contains( tallow ) );
}

// --- Recipe result info tests ---

static bool info_has_text( const std::vector<iteminfo> &info, const std::string &text )
{
    return std::any_of( info.begin(), info.end(), [&]( const iteminfo & i ) {
        return i.sName.find( text ) != std::string::npos;
    } );
}

TEST_CASE( "result_item_basic_properties", "[crafting][gui][result_info]" )
{
    Character &guy = setup_character();
    const recipe &rec = recipe_cudgel_test_no_tools.obj();

    item result = get_recipe_result_item( rec, guy );

    CHECK( result.typeId() == itype_cudgel );
    CHECK( result.has_var( "recipe_exemplar" ) );
    CHECK( result.get_var( "recipe_exemplar" ) == rec.ident().str() );
}

TEST_CASE( "result_item_charges_reset", "[crafting][gui][result_info]" )
{
    Character &guy = setup_character();
    const recipe &rec = recipe_test_tallow.obj();

    item result = get_recipe_result_item( rec, guy );

    CHECK( result.count_by_charges() );
    CHECK( result.charges == 1 );
}

TEST_CASE( "result_info_simple_recipe", "[crafting][gui][result_info]" )
{
    Character &guy = setup_character();
    const recipe &rec = recipe_cudgel_test_no_tools.obj();

    std::vector<iteminfo> info = recipe_result_info( rec, guy, 1, 80 );

    CHECK( info_has_text( info, "Result" ) );
    CHECK_FALSE( info_has_text( info, "Recipe Outputs" ) );
    CHECK_FALSE( info_has_text( info, "Byproduct" ) );
    CHECK_FALSE( info_has_text( info, "Container" ) );
}

TEST_CASE( "result_info_with_byproducts", "[crafting][gui][result_info]" )
{
    Character &guy = setup_character();
    const recipe &rec = recipe_test_tallow.obj();

    std::vector<iteminfo> info = recipe_result_info( rec, guy, 1, 80 );

    CHECK( info_has_text( info, "Recipe Outputs" ) );
    CHECK( info_has_text( info, "Byproduct" ) );
}

TEST_CASE( "result_info_with_container", "[crafting][gui][result_info]" )
{
    Character &guy = setup_character();
    const recipe &rec = recipe_water_clean_test_in_jar.obj();

    std::vector<iteminfo> info = recipe_result_info( rec, guy, 1, 80 );

    CHECK( info_has_text( info, "Recipe Outputs" ) );
    CHECK( info_has_text( info, "Container" ) );
}

TEST_CASE( "result_info_batch_scaling", "[crafting][gui][result_info]" )
{
    Character &guy = setup_character();
    const recipe &rec = recipe_cudgel_test_no_tools.obj();

    // batch_size=5, makes_amount=1 -> total_quantity=5 -> " (5)" appended
    std::vector<iteminfo> info_batch5 = recipe_result_info( rec, guy, 5, 80 );
    CHECK( info_has_text( info_batch5, "(5)" ) );

    // batch_size=1, makes_amount=1 -> total_quantity=1 -> no count appended
    std::vector<iteminfo> info_batch1 = recipe_result_info( rec, guy, 1, 80 );
    CHECK_FALSE( info_has_text( info_batch1, "(1)" ) );
}

TEST_CASE( "result_info_varsize_poor_fit_warning", "[crafting][gui][result_info]" )
{
    // test_longshirt has VARSIZE. get_recipe_result_item adds FIT for a
    // human-sized character. result_item_header then finds the VARSIZE
    // component and emits the "poorly-fitted" warning.
    Character &guy = setup_character();
    const recipe &rec = recipe_test_longshirt_test_poor_fit.obj();

    std::vector<iteminfo> info = recipe_result_info( rec, guy, 1, 80 );

    CHECK( info_has_text( info, "poorly" ) );
}

// --- Recipe list pipeline tests ---

static void clear_recipe_ui_state()
{
    uistate.hidden_recipes.clear();
    uistate.expanded_recipes.clear();
    uistate.read_recipes.clear();
}

TEST_CASE( "build_recipe_list_hides_hidden", "[crafting][gui][recipe_list]" )
{
    clear_recipe_ui_state();
    Character &guy = setup_character();

    const recipe *cudgel = &recipe_cudgel_test_no_tools.obj();
    const recipe *tallow = &recipe_test_tallow.obj();
    std::vector<const recipe *> picking = { cudgel, tallow };
    recipe_subset available = make_subset( picking );

    uistate.hidden_recipes.insert( recipe_cudgel_test_no_tools );

    std::map<const recipe *, availability> cache;
    recipe_list_data result = build_recipe_list( picking, false, false,
                              guy, false, nullptr, false, false, cache, available );

    for( const recipe *entry : result.entries ) {
        CHECK_FALSE( entry->ident() == recipe_cudgel_test_no_tools );
    }
    CHECK( result.num_hidden == 1 );
}

TEST_CASE( "build_recipe_list_skip_hidden_filter", "[crafting][gui][recipe_list]" )
{
    clear_recipe_ui_state();
    Character &guy = setup_character();

    const recipe *cudgel = &recipe_cudgel_test_no_tools.obj();
    std::vector<const recipe *> picking = { cudgel };
    recipe_subset available = make_subset( picking );

    uistate.hidden_recipes.insert( recipe_cudgel_test_no_tools );

    std::map<const recipe *, availability> cache;
    recipe_list_data result = build_recipe_list( picking, true, false,
                              guy, false, nullptr, false, false, cache, available );

    CHECK_FALSE( result.entries.empty() );
    CHECK( result.entries[0]->ident() == recipe_cudgel_test_no_tools );
    CHECK( result.num_hidden == 0 );
}

TEST_CASE( "build_recipe_list_sort_craftable_first", "[crafting][gui][recipe_list]" )
{
    clear_recipe_ui_state();
    Character &guy = setup_character();
    guy.set_skill_level( skill_fabrication, 2 );
    guy.set_skill_level( skill_melee, 1 );
    guy.i_add( item( itype_2x4 ) );
    guy.invalidate_crafting_inventory();

    // cudgel_test_no_tools: needs 2x4, no tools -- craftable with inventory above
    // test_tallow: needs fat + CUT 2 quality -- not craftable (no fat or knife)
    const recipe *cudgel = &recipe_cudgel_test_no_tools.obj();
    const recipe *tallow = &recipe_test_tallow.obj();
    std::vector<const recipe *> picking = { tallow, cudgel };
    recipe_subset available = make_subset( picking );

    std::map<const recipe *, availability> cache;
    recipe_list_data result = build_recipe_list( picking, false, false,
                              guy, false, nullptr, false, false, cache, available );

    REQUIRE( result.available.size() == 2 );
    // Craftable recipes come first in sorted order
    bool seen_uncraftable = false;
    for( const availability &entry : result.available ) {
        if( !entry.can_craft ) {
            seen_uncraftable = true;
        } else {
            CHECK_FALSE( seen_uncraftable );
        }
    }
}

TEST_CASE( "build_recipe_list_expands_nested", "[crafting][gui][recipe_list]" )
{
    clear_recipe_ui_state();
    Character &guy = setup_character();
    guy.set_skill_level( skill_fabrication, 2 );
    guy.set_skill_level( skill_melee, 1 );

    const recipe *nested = &recipe_test_nested_weapons.obj();
    const recipe *cudgel = &recipe_cudgel_test_no_tools.obj();
    std::vector<const recipe *> picking = { nested };
    // available_recipes must contain the child recipe for expansion to include it
    recipe_subset available = make_subset( { nested, cudgel } );

    uistate.expanded_recipes.insert( recipe_test_nested_weapons );

    std::map<const recipe *, availability> cache;
    recipe_list_data result = build_recipe_list( picking, false, false,
                              guy, false, nullptr, false, false, cache, available );

    // Expansion should have inserted the child after the nested category
    REQUIRE( result.entries.size() == 2 );
    CHECK( result.entries[0]->ident() == recipe_test_nested_weapons );
    CHECK( result.entries[1]->ident() == recipe_cudgel_test_no_tools );
    CHECK( result.indent[0] < result.indent[1] );
}

TEST_CASE( "list_nested_generates_tree", "[crafting][gui][recipe_list]" )
{
    clear_recipe_ui_state();
    Character &guy = setup_character();
    guy.set_skill_level( skill_fabrication, 2 );
    guy.set_skill_level( skill_melee, 1 );

    const recipe *nested = &recipe_test_nested_weapons.obj();
    const recipe *cudgel = &recipe_cudgel_test_no_tools.obj();
    recipe_subset available = make_subset( { nested, cudgel } );

    std::string desc = list_nested( guy, nested, available );

    CHECK( desc.find( "test nested weapons" ) != std::string::npos );
    CHECK( desc.find( "cudgel" ) != std::string::npos );
}
