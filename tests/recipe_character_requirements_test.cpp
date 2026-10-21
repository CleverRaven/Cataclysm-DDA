#include <map>
#include <string>

#include "avatar.h"
#include "bonuses.h"
#include "cata_catch.h"
#include "character.h"
#include "player_helpers.h"
#include "recipe.h"
#include "type_id.h"

static const recipe_id
recipe_cudgel_character_requirements_all_stats( "cudgel_character_requirements_all_stats" );
static const recipe_id
recipe_cudgel_character_requirements_child_clear( "cudgel_character_requirements_child_clear" );
static const recipe_id
recipe_cudgel_character_requirements_child_inherit( "cudgel_character_requirements_child_inherit" );
static const recipe_id
recipe_cudgel_character_requirements_child_replace( "cudgel_character_requirements_child_replace" );
static const recipe_id
recipe_cudgel_character_requirements_control( "cudgel_character_requirements_control" );
static const recipe_id
recipe_cudgel_character_requirements_restricted( "cudgel_character_requirements_restricted" );
static const recipe_id
recipe_cudgel_character_requirements_zero_entries( "cudgel_character_requirements_zero_entries" );

static void set_primary_stats( Character &character, const int str, const int dex, const int intel,
                               const int per )
{
    character.set_str_base( str );
    character.set_dex_base( dex );
    character.set_int_base( intel );
    character.set_per_base( per );
    character.set_str_bonus( 0 );
    character.set_dex_bonus( 0 );
    character.set_int_bonus( 0 );
    character.set_per_bonus( 0 );
}

TEST_CASE( "recipe_character_requirements_load_as_minimum_stat_values",
           "[recipe][character_requirements][json]" )
{
    const recipe &result = recipe_cudgel_character_requirements_all_stats.obj();

    REQUIRE( result.has_character_requirements() );
    const std::map<scaling_stat, int> &requirements = result.get_character_requirements();
    REQUIRE( requirements.size() == 4 );

    CHECK( requirements.at( STAT_STR ) == 9 );
    CHECK( requirements.at( STAT_DEX ) == 6 );
    CHECK( requirements.at( STAT_INT ) == 4 );
    CHECK( requirements.at( STAT_PER ) == 12 );
}

TEST_CASE( "zero_character_requirements_have_no_effect",
           "[recipe][character_requirements][json]" )
{
    const recipe &result = recipe_cudgel_character_requirements_zero_entries.obj();

    CHECK_FALSE( result.has_character_requirements() );
    CHECK( result.get_character_requirements().empty() );
}

TEST_CASE( "recipe_character_requirements_inheritance",
           "[recipe][character_requirements][json][copy-from]" )
{
    SECTION( "omitting the field preserves inherited requirements" ) {
        const recipe &result = recipe_cudgel_character_requirements_child_inherit.obj();

        const auto &requirements = result.get_character_requirements();
        REQUIRE( requirements.size() == 2 );
        CHECK( requirements.at( STAT_STR ) == 9 );
        CHECK( requirements.at( STAT_DEX ) == 6 );
    }

    SECTION( "defining the field replaces all inherited requirements" ) {
        const recipe &result = recipe_cudgel_character_requirements_child_replace.obj();

        const auto &requirements = result.get_character_requirements();
        REQUIRE( requirements.size() == 1 );
        CHECK( requirements.count( STAT_STR ) == 0 );
        CHECK( requirements.count( STAT_DEX ) == 0 );
        CHECK( requirements.at( STAT_PER ) == 12 );
    }

    SECTION( "an empty object clears all inherited requirements" ) {
        const recipe &result = recipe_cudgel_character_requirements_child_clear.obj();

        CHECK_FALSE( result.has_character_requirements() );
        CHECK( result.get_character_requirements().empty() );
    }
}

TEST_CASE( "recipe_character_requirements_use_final_primary_stat_values",
           "[recipe][character_requirements][character]" )
{
    avatar &character = get_avatar();
    clear_character( character );
    set_primary_stats( character, 8, 6, 4, 12 );

    const recipe &result = recipe_cudgel_character_requirements_all_stats.obj();

    SECTION( "an unmet minimum rejects the character" ) {
        CHECK_FALSE( result.character_meets_requirements( character ) );
    }

    SECTION( "bonuses contribute to the final value" ) {
        character.set_str_bonus( 1 );

        REQUIRE( character.get_str() == 9 );
        CHECK( result.character_meets_requirements( character ) );
    }

    SECTION( "a value equal to the minimum is accepted" ) {
        character.set_str_base( 9 );

        CHECK( result.character_meets_requirements( character ) );
    }
}

TEST_CASE( "character_requirements_affect_craft_start_availability",
           "[recipe][character_requirements][crafting]" )
{
    avatar &character = get_avatar();
    clear_character( character );
    set_primary_stats( character, 8, 8, 8, 8 );

    const recipe &control = recipe_cudgel_character_requirements_control.obj();
    const recipe &restricted = recipe_cudgel_character_requirements_restricted.obj();

    REQUIRE( character.can_start_craft( &control, recipe_filter_flags::none ) );
    CHECK_FALSE( character.can_start_craft( &restricted, recipe_filter_flags::none ) );

    character.set_str_base( 9 );
    CHECK( character.can_start_craft( &restricted, recipe_filter_flags::none ) );
}
