#include <functional>
#include <memory>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "character_attire.h"
#include "color.h"
#include "crafting_gui_helpers.h"
#include "game.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "options_helpers.h"
#include "player_helpers.h"
#include "type_id.h"
#include "weather_type.h"

class recipe;

static const itype_id itype_2x4( "2x4" );
static const itype_id itype_billet_wood( "billet_wood" );
static const itype_id itype_debug_backpack( "debug_backpack" );
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
static const recipe_id recipe_test_nested_weapons( "test_nested_weapons" );
static const recipe_id recipe_test_tallow( "test_tallow" );

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

    CHECK( recipe_sort_compare( &rec, &rec, avail_yes, avail_no, guy,
                                true, true, false ) );
    CHECK_FALSE( recipe_sort_compare( &rec, &rec, avail_no, avail_yes, guy,
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
    CHECK( recipe_sort_compare( &rec_hard, &rec_easy, avail_hard, avail_easy, guy,
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
    CHECK( recipe_sort_compare( &rec_meat, &rec_cudgel, avail_meat, avail_cudgel, guy,
                                true, true, false ) );
    CHECK_FALSE( recipe_sort_compare( &rec_cudgel, &rec_meat, avail_cudgel, avail_meat, guy,
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
    CHECK( recipe_sort_compare( &rec_slow, &rec_fast, avail_slow, avail_fast, guy,
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
    CHECK( recipe_sort_compare( &rec, &rec, avail, avail, guy,
                                false, true, true ) );
    // a_read=true (read), b_read=false (unread) -> b should sort first, so a < b is false
    CHECK_FALSE( recipe_sort_compare( &rec, &rec, avail, avail, guy,
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
    CHECK( recipe_sort_compare( &rec, &rec, avail_yes, avail_no, guy,
                                true, false, false ) );
}
