#include <map>

#include "avatar.h"
#include "bonuses.h"
#include "cata_catch.h"
#include "character.h"
#include "player_helpers.h"
#include "recipe.h"

static const recipe_id
recipe_cudgel_character_requirements_child_clear( "cudgel_character_requirements_child_clear" );
static const recipe_id
recipe_cudgel_character_requirements_child_inherit( "cudgel_character_requirements_child_inherit" );
static const recipe_id
recipe_cudgel_character_requirements_child_replace( "cudgel_character_requirements_child_replace" );
static const recipe_id
recipe_cudgel_character_requirements_control( "cudgel_character_requirements_control" );
static const recipe_id
recipe_cudgel_character_requirements_mixed( "cudgel_character_requirements_mixed" );
static const recipe_id
recipe_cudgel_character_requirements_restricted( "cudgel_character_requirements_restricted" );
static const recipe_id
recipe_cudgel_character_requirements_zero_bounds( "cudgel_character_requirements_zero_bounds" );
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

TEST_CASE( "character stat requirement bounds are inclusive", "[recipe][character_requirements]" )
{
    character_stat_requirement requirement;

    SECTION( "no bounds" ) {
        CHECK( requirement.is_met( 0 ) );
        CHECK( requirement.is_met( 10 ) );
    }

    SECTION( "minimum only" ) {
        requirement.min = 5;

        CHECK_FALSE( requirement.is_met( 4 ) );
        CHECK( requirement.is_met( 5 ) );
        CHECK( requirement.is_met( 6 ) );
    }

    SECTION( "maximum only" ) {
        requirement.max = 10;

        CHECK( requirement.is_met( 9 ) );
        CHECK( requirement.is_met( 10 ) );
        CHECK_FALSE( requirement.is_met( 11 ) );
    }

    SECTION( "minimum and maximum" ) {
        requirement.min = 5;
        requirement.max = 10;

        CHECK_FALSE( requirement.is_met( 4 ) );
        CHECK( requirement.is_met( 5 ) );
        CHECK( requirement.is_met( 10 ) );
        CHECK_FALSE( requirement.is_met( 11 ) );
    }
}

TEST_CASE( "recipe character requirements load from supported JSON forms",
           "[recipe][character_requirements][json]" )
{
    const recipe &result = recipe_cudgel_character_requirements_mixed.obj();

    REQUIRE( result.has_character_requirements() );
    const std::map<scaling_stat, character_stat_requirement> &requirements =
        result.get_character_requirements();
    REQUIRE( requirements.size() == 4 );

    CHECK( requirements.at( STAT_STR ).min == 9 );
    CHECK_FALSE( requirements.at( STAT_STR ).max );

    CHECK( requirements.at( STAT_DEX ).min == 6 );
    CHECK_FALSE( requirements.at( STAT_DEX ).max );

    CHECK( requirements.at( STAT_INT ).min == 4 );
    CHECK( requirements.at( STAT_INT ).max == 10 );

    CHECK_FALSE( requirements.at( STAT_PER ).min );
    CHECK( requirements.at( STAT_PER ).max == 12 );
}

TEST_CASE( "zero character requirement bounds have no effect",
           "[recipe][character_requirements][json]" )
{
    SECTION( "all zero entries are discarded" ) {
        const recipe &result = recipe_cudgel_character_requirements_zero_entries.obj();

        CHECK_FALSE( result.has_character_requirements() );
        CHECK( result.get_character_requirements().empty() );
    }

    SECTION( "zero removes only its own bound" ) {
        const recipe &result = recipe_cudgel_character_requirements_zero_bounds.obj();

        const auto &requirements = result.get_character_requirements();
        REQUIRE( requirements.size() == 2 );

        CHECK_FALSE( requirements.at( STAT_STR ).min );
        CHECK( requirements.at( STAT_STR ).max == 10 );
        CHECK( requirements.at( STAT_DEX ).min == 5 );
        CHECK_FALSE( requirements.at( STAT_DEX ).max );
    }
}

TEST_CASE( "recipe character requirements inheritance",
           "[recipe][character_requirements][json][copy-from]" )
{
    SECTION( "omitting the field preserves inherited requirements" ) {
        const recipe &result = recipe_cudgel_character_requirements_child_inherit.obj();

        const auto &requirements = result.get_character_requirements();
        REQUIRE( requirements.size() == 2 );
        CHECK( requirements.at( STAT_STR ).min == 9 );
        CHECK( requirements.at( STAT_DEX ).min == 6 );
    }

    SECTION( "defining the field replaces all inherited requirements" ) {
        const recipe &result = recipe_cudgel_character_requirements_child_replace.obj();

        const auto &requirements = result.get_character_requirements();
        REQUIRE( requirements.size() == 1 );
        CHECK( requirements.count( STAT_STR ) == 0 );
        CHECK( requirements.count( STAT_DEX ) == 0 );
        CHECK( requirements.at( STAT_PER ).min == 12 );
    }

    SECTION( "an empty object clears all inherited requirements" ) {
        const recipe &result = recipe_cudgel_character_requirements_child_clear.obj();

        CHECK_FALSE( result.has_character_requirements() );
        CHECK( result.get_character_requirements().empty() );
    }
}

TEST_CASE( "recipe character requirements use final primary stat values",
           "[recipe][character_requirements][character]" )
{
    avatar &character = get_avatar();
    clear_character( character );
    set_primary_stats( character, 8, 6, 10, 12 );

    const recipe &result = recipe_cudgel_character_requirements_mixed.obj();

    SECTION( "an unmet minimum rejects the character" ) {
        CHECK_FALSE( result.character_meets_requirements( character ) );
    }

    SECTION( "bonuses contribute to the final value" ) {
        character.set_str_bonus( 1 );

        REQUIRE( character.get_str() == 9 );
        CHECK( result.character_meets_requirements( character ) );
    }

    SECTION( "an exceeded maximum rejects the character" ) {
        character.set_str_base( 9 );
        character.set_int_base( 11 );

        CHECK_FALSE( result.character_meets_requirements( character ) );
    }

    SECTION( "inclusive maximum accepts an equal value" ) {
        character.set_str_base( 9 );

        REQUIRE( character.get_int() == 10 );
        REQUIRE( character.get_per() == 12 );
        CHECK( result.character_meets_requirements( character ) );
    }
}

TEST_CASE( "character requirements affect craft start availability",
           "[recipe][character_requirements][crafting]" )
{
    avatar &character = get_avatar();
    clear_character( character );
    set_primary_stats( character, 8, 8, 8, 8 );

    const recipe &control = recipe_cudgel_character_requirements_control.obj();
    const recipe &restricted = recipe_cudgel_character_requirements_restricted.obj();

    // Verify that the synthetic recipe itself is otherwise startable.  This keeps a failure
    // in unrelated crafting prerequisites separate from the character requirement assertion.
    REQUIRE( character.can_start_craft( &control, recipe_filter_flags::none ) );

    CHECK_FALSE( character.can_start_craft( &restricted, recipe_filter_flags::none ) );

    character.set_str_base( 9 );
    CHECK( character.can_start_craft( &restricted, recipe_filter_flags::none ) );
}
