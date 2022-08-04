#include <iosfwd>
#include <functional>
#include <regex>
#include <string>
#include <memory>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "creature.h"
#include "display.h"
#include "game_constants.h"
#include "monster.h"
#include "options.h"
#include "output.h"
#include "player_helpers.h"
#include "string_formatter.h"
#include "type_id.h"
#include "units.h"

static const mtype_id mon_cow( "mon_cow" );
static const mtype_id mon_dog_gpyrenees( "mon_dog_gpyrenees" );
static const mtype_id mon_dog_gshepherd( "mon_dog_gshepherd" );
static const mtype_id mon_horse( "mon_horse" );
static const mtype_id mon_pig( "mon_pig" );

static const trait_id trait_HUGE( "HUGE" );
static const trait_id trait_LARGE( "LARGE" );
static const trait_id trait_SMALL( "SMALL" );
static const trait_id trait_SMALL2( "SMALL2" );

// Return the `kcal_ratio` needed to reach the given `bmi`
//   BMI = 13 + (12 * kcal_ratio)
//   kcal_ratio = (BMI - 13) / 12
static float bmi_to_kcal_ratio( float bmi )
{
    return ( bmi - 13 ) / 12;
}

// Set enough stored calories to reach target BMI
static void set_player_bmi( Character &dummy, float bmi )
{
    dummy.set_stored_kcal( dummy.get_healthy_kcal() * bmi_to_kcal_ratio( bmi ) );
    REQUIRE( dummy.get_bmi() == Approx( bmi ).margin( 0.001f ) );
}

// Return the player's `get_bmi` at `kcal_percent` (actually a ratio of stored_kcal to healthy_kcal)
static float bmi_at_kcal_ratio( Character &dummy, float kcal_percent )
{
    dummy.set_stored_kcal( dummy.get_healthy_kcal() * kcal_percent );
    REQUIRE( dummy.get_kcal_percent() == Approx( kcal_percent ).margin( 0.001f ) );

    return dummy.get_bmi();
}

// Return the player's `get_max_healthy` value at the given body mass index
static int max_healthy_at_bmi( Character &dummy, float bmi )
{
    set_player_bmi( dummy, bmi );
    return dummy.get_max_healthy();
}

// Return the player's `kcal_speed_penalty` value at the given body mass index
static int kcal_speed_penalty_at_bmi( Character &dummy, float bmi )
{
    set_player_bmi( dummy, bmi );
    return dummy.kcal_speed_penalty();
}

// Return the player's `weight_string` at the given body mass index
static std::string weight_string_at_bmi( Character &dummy, float bmi )
{
    set_player_bmi( dummy, bmi );
    return remove_color_tags( display::weight_string( dummy ) );
}

// Return `bodyweight` in kilograms for a player at the given body mass index.
static float bodyweight_kg_at_bmi( Character &dummy, float bmi )
{
    set_player_bmi( dummy, bmi );
    return to_kilogram( dummy.bodyweight() );
}

// Return player `metabolic_rate_base` with a given mutation
static float metabolic_rate_with_mutation( Character &dummy, const std::string &trait_name )
{
    set_single_trait( dummy, trait_name );
    return dummy.metabolic_rate_base();
}

// Return player `get_bmr` (basal metabolic rate) at the given activity level.
static int bmr_at_act_level( Character &dummy, float activity_level )
{
    dummy.reset_activity_level();
    dummy.update_body( calendar::turn, calendar::turn );
    dummy.set_activity_level( activity_level );

    return dummy.get_bmr();
}

using avatar_ptr = std::shared_ptr<avatar>;

// Create a map of dummies of all available size categories
static std::map<creature_size, avatar_ptr> create_dummies_of_all_sizes( int init_height )
{
    const std::map<creature_size, std::string> size_mutations{
        { creature_size::tiny, "SMALL2" },
        { creature_size::small, "SMALL" },
        { creature_size::medium, "" },
        { creature_size::large, "LARGE" },
        { creature_size::huge, "HUGE" },
    };

    std::map<creature_size, avatar_ptr> dummies;
    for( const auto &pair : size_mutations ) {
        const creature_size &size = pair.first;
        const std::string &trait_name = pair.second;

        avatar_ptr dummy = std::make_shared<avatar>();
        clear_character( *dummy );

        if( !trait_name.empty() ) {
            set_single_trait( *dummy, trait_name );
        }
        REQUIRE( dummy->get_size() == size );

        dummy->mod_base_height( init_height - dummy->base_height() );
        REQUIRE( dummy->base_height() == init_height );

        dummies[size] = dummy;
    }
    return dummies;
}

static void for_each_size_category( const std::function< void( creature_size ) > &functor )
{
    for( int i = static_cast< int >( creature_size::tiny );
         i < static_cast< int >( creature_size::num_sizes ); ++i ) {
        creature_size size = static_cast< creature_size >( i );
        functor( size );
    }
}

TEST_CASE( "body mass index determines weight description", "[biometrics][bmi][weight]" )
{
    avatar dummy;

    CHECK( weight_string_at_bmi( dummy, 13.0f ) == "Skeletal" );
    CHECK( weight_string_at_bmi( dummy, 13.9f ) == "Skeletal" );
    // 14
    CHECK( weight_string_at_bmi( dummy, 14.1f ) == "Emaciated" );
    CHECK( weight_string_at_bmi( dummy, 15.9f ) == "Emaciated" );
    // 16
    CHECK( weight_string_at_bmi( dummy, 16.1f ) == "Underweight" );
    CHECK( weight_string_at_bmi( dummy, 18.4f ) == "Underweight" );
    // 18.5
    CHECK( weight_string_at_bmi( dummy, 18.6f ) == "Normal" );
    CHECK( weight_string_at_bmi( dummy, 24.9f ) == "Normal" );
    // 25
    CHECK( weight_string_at_bmi( dummy, 25.1f ) == "Overweight" );
    CHECK( weight_string_at_bmi( dummy, 29.9f ) == "Overweight" );
    // 30
    CHECK( weight_string_at_bmi( dummy, 30.1f ) == "Obese" );
    CHECK( weight_string_at_bmi( dummy, 34.9f ) == "Obese" );
    // 35
    CHECK( weight_string_at_bmi( dummy, 35.1f ) == "Very Obese" );
    CHECK( weight_string_at_bmi( dummy, 39.9f ) == "Very Obese" );
    // 40
    CHECK( weight_string_at_bmi( dummy, 40.1f ) == "Morbidly Obese" );
    CHECK( weight_string_at_bmi( dummy, 41.0f ) == "Morbidly Obese" );
}

TEST_CASE( "stored kcal ratio determines body mass index", "[biometrics][kcal][bmi]" )
{
    avatar dummy;

    // Skeletal (<14)
    CHECK( bmi_at_kcal_ratio( dummy, 0.01f ) == Approx( 13.12f ).margin( 0.01f ) );
    CHECK( bmi_at_kcal_ratio( dummy, 0.05f ) == Approx( 13.6f ).margin( 0.01f ) );
    // Emaciated (14)
    CHECK( bmi_at_kcal_ratio( dummy, 0.1f ) == Approx( 14.2f ).margin( 0.01f ) );
    CHECK( bmi_at_kcal_ratio( dummy, 0.2f ) == Approx( 15.4f ).margin( 0.01f ) );
    // Underweight (16)
    CHECK( bmi_at_kcal_ratio( dummy, 0.3f ) == Approx( 16.6f ).margin( 0.01f ) );
    CHECK( bmi_at_kcal_ratio( dummy, 0.4f ) == Approx( 17.8f ).margin( 0.01f ) );
    // Normal (18.5)
    CHECK( bmi_at_kcal_ratio( dummy, 0.5f ) == Approx( 19.0f ).margin( 0.01f ) );
    CHECK( bmi_at_kcal_ratio( dummy, 0.6f ) == Approx( 20.2f ).margin( 0.01f ) );
    CHECK( bmi_at_kcal_ratio( dummy, 0.7f ) == Approx( 21.4f ).margin( 0.01f ) );
    CHECK( bmi_at_kcal_ratio( dummy, 0.8f ) == Approx( 22.6f ).margin( 0.01f ) );
    CHECK( bmi_at_kcal_ratio( dummy, 0.9f ) == Approx( 23.8f ).margin( 0.01f ) );
    // Overweight (25)
    CHECK( bmi_at_kcal_ratio( dummy, 1.0f ) == Approx( 25.0f ).margin( 0.01f ) );
    CHECK( bmi_at_kcal_ratio( dummy, 1.1f ) == Approx( 26.2f ).margin( 0.01f ) );
    CHECK( bmi_at_kcal_ratio( dummy, 1.2f ) == Approx( 27.4f ).margin( 0.01f ) );
    CHECK( bmi_at_kcal_ratio( dummy, 1.3f ) == Approx( 28.6f ).margin( 0.01f ) );
    CHECK( bmi_at_kcal_ratio( dummy, 1.4f ) == Approx( 29.8f ).margin( 0.01f ) );
    // Obese (30)
    CHECK( bmi_at_kcal_ratio( dummy, 1.5f ) == Approx( 31.0f ).margin( 0.01f ) );
    CHECK( bmi_at_kcal_ratio( dummy, 1.6f ) == Approx( 32.2f ).margin( 0.01f ) );
    CHECK( bmi_at_kcal_ratio( dummy, 1.7f ) == Approx( 33.4f ).margin( 0.01f ) );
    CHECK( bmi_at_kcal_ratio( dummy, 1.8f ) == Approx( 34.6f ).margin( 0.01f ) );
    // Very obese (35)
    CHECK( bmi_at_kcal_ratio( dummy, 1.9f ) == Approx( 35.8f ).margin( 0.01f ) );
    CHECK( bmi_at_kcal_ratio( dummy, 2.0f ) == Approx( 37.0f ).margin( 0.01f ) );
    // Morbidly obese (40)
    CHECK( bmi_at_kcal_ratio( dummy, 2.25f ) == Approx( 40.0f ).margin( 0.01f ) );
    CHECK( bmi_at_kcal_ratio( dummy, 2.5f ) == Approx( 43.0f ).margin( 0.01f ) );
}

TEST_CASE( "body mass index determines maximum healthiness", "[biometrics][bmi][max]" )
{
    // "BMIs under 20 and over 25 have been associated with higher all-causes mortality,
    // with the risk increasing with distance from the 20–25 range."
    //
    // https://en.wikipedia.org/wiki/Body_mass_index

    avatar dummy;

    // Skeletal (<14)
    CHECK( max_healthy_at_bmi( dummy, 8.0f ) == -200 );
    CHECK( max_healthy_at_bmi( dummy, 9.0f ) == -200 );
    CHECK( max_healthy_at_bmi( dummy, 10.0f ) == -183 );
    CHECK( max_healthy_at_bmi( dummy, 11.0f ) == -115 );
    CHECK( max_healthy_at_bmi( dummy, 12.0f ) == -53 );
    CHECK( max_healthy_at_bmi( dummy, 13.0f ) == 2 );
    // Emaciated (14)
    CHECK( max_healthy_at_bmi( dummy, 14.0f ) == 51 );
    CHECK( max_healthy_at_bmi( dummy, 15.0f ) == 95 );
    // Underweight (16)
    CHECK( max_healthy_at_bmi( dummy, 16.0f ) == 133 );
    CHECK( max_healthy_at_bmi( dummy, 17.0f ) == 164 );
    CHECK( max_healthy_at_bmi( dummy, 18.0f ) == 189 );
    // Normal (18.5)
    CHECK( max_healthy_at_bmi( dummy, 19.0f ) == 200 );
    CHECK( max_healthy_at_bmi( dummy, 20.0f ) == 200 );
    CHECK( max_healthy_at_bmi( dummy, 21.0f ) == 200 );
    CHECK( max_healthy_at_bmi( dummy, 22.0f ) == 200 );
    CHECK( max_healthy_at_bmi( dummy, 23.0f ) == 200 );
    CHECK( max_healthy_at_bmi( dummy, 24.0f ) == 200 );
    CHECK( max_healthy_at_bmi( dummy, 25.0f ) == 200 );
    // Overweight (25)
    CHECK( max_healthy_at_bmi( dummy, 26.0f ) == 178 );
    CHECK( max_healthy_at_bmi( dummy, 27.0f ) == 149 );
    CHECK( max_healthy_at_bmi( dummy, 28.0f ) == 115 );
    CHECK( max_healthy_at_bmi( dummy, 29.0f ) == 74 );
    // Obese (30)
    CHECK( max_healthy_at_bmi( dummy, 30.0f ) == 28 );
    CHECK( max_healthy_at_bmi( dummy, 31.0f ) == -25 );
    CHECK( max_healthy_at_bmi( dummy, 32.0f ) == -83 );
    CHECK( max_healthy_at_bmi( dummy, 33.0f ) == -148 );
    CHECK( max_healthy_at_bmi( dummy, 34.0f ) == -200 );
    // Very obese (35)
    CHECK( max_healthy_at_bmi( dummy, 35.0f ) == -200 );
    CHECK( max_healthy_at_bmi( dummy, 36.0f ) == -200 );
    CHECK( max_healthy_at_bmi( dummy, 37.0f ) == -200 );
    CHECK( max_healthy_at_bmi( dummy, 38.0f ) == -200 );
    CHECK( max_healthy_at_bmi( dummy, 39.0f ) == -200 );
    // Morbidly obese (40)
    CHECK( max_healthy_at_bmi( dummy, 40.0f ) == -200 );
    CHECK( max_healthy_at_bmi( dummy, 41.0f ) == -200 );
    CHECK( max_healthy_at_bmi( dummy, 42.0f ) == -200 );
}

TEST_CASE( "character height should increase with their body size",
           "[biometrics][height][mutation]" )
{
    using DummyMap = std::map<creature_size, avatar_ptr>;
    DummyMap dummies_default_height = create_dummies_of_all_sizes( Character::default_height() );
    DummyMap dummies_min_height = create_dummies_of_all_sizes( Character::min_height() );
    DummyMap dummies_max_height = create_dummies_of_all_sizes( Character::max_height() );

    auto test_absolute_heights = []( DummyMap & dummies ) {
        for_each_size_category( [ &dummies ]( creature_size size ) {
            CHECK( dummies[size]->height() >= Character::min_height( size ) );
            CHECK( dummies[size]->height() <= Character::max_height( size ) );
            if( size != creature_size::huge ) {
                creature_size next_size = static_cast< creature_size >( static_cast< int >( size ) + 1 );
                CHECK( dummies[size]->height() < dummies[next_size]->height() );
            }
        } );
    };

    GIVEN( "default height character" ) {
        test_absolute_heights( dummies_default_height );
    }
    GIVEN( "min height character" ) {
        test_absolute_heights( dummies_min_height );
    }
    GIVEN( "max height character" ) {
        test_absolute_heights( dummies_max_height );
    }
}

TEST_CASE( "default character (175 cm) bodyweights at various BMIs", "[biometrics][bodyweight]" )
{
    avatar dummy;
    clear_character( dummy );

    // Body weight is calculated as ( BMI * (height/100)^2 ). At any given height, body weight
    // varies based on body mass index (which in turn depends on the amount of stored calories).

    GIVEN( "character height started at 175cm" ) {
        dummy.mod_base_height( 175 - dummy.base_height() );
        REQUIRE( dummy.base_height() == 175 );

        WHEN( "character is normal-sized (medium)" ) {
            REQUIRE_FALSE( dummy.has_trait( trait_SMALL2 ) );
            REQUIRE_FALSE( dummy.has_trait( trait_SMALL ) );
            REQUIRE_FALSE( dummy.has_trait( trait_LARGE ) );
            REQUIRE_FALSE( dummy.has_trait( trait_HUGE ) );
            REQUIRE( dummy.get_size() == creature_size::medium );

            THEN( "bodyweight varies from ~49-107kg" ) {
                // BMI [16-35] is "Emaciated/Underweight" to "Obese/Very Obese"
                CHECK( bodyweight_kg_at_bmi( dummy, 16.0 ) == Approx( 49.0 ).margin( 0.1f ) );
                CHECK( bodyweight_kg_at_bmi( dummy, 25.0 ) == Approx( 76.6 ).margin( 0.1f ) );
                CHECK( bodyweight_kg_at_bmi( dummy, 35.0 ) == Approx( 107.2 ).margin( 0.1f ) );
            }
        }
    }
}

TEST_CASE( "character's weight should increase with their body size and BMI",
           "[biometrics][bodyweight][mutation]" )
{
    using DummyMap = std::map<creature_size, avatar_ptr>;
    DummyMap dummies_default_height = create_dummies_of_all_sizes( Character::default_height() );
    DummyMap dummies_min_height = create_dummies_of_all_sizes( Character::min_height() );
    DummyMap dummies_max_height = create_dummies_of_all_sizes( Character::max_height() );

    auto test_weights = []( DummyMap & dummies ) {
        for_each_size_category( [&dummies]( creature_size size ) {
            if( size != creature_size::huge ) {
                creature_size next_size = static_cast< creature_size >( static_cast< int >( size ) + 1 );
                CHECK( dummies[size]->bodyweight() < dummies[next_size]->bodyweight() );
            }
            avatar_ptr &dummy = dummies[size];
            CHECK( bodyweight_kg_at_bmi( *dummy, 16.0f ) < bodyweight_kg_at_bmi( *dummy, 25.0f ) );
            CHECK( bodyweight_kg_at_bmi( *dummy, 25.0f ) < bodyweight_kg_at_bmi( *dummy, 35.0f ) );
        } );
    };

    GIVEN( "default height character" ) {
        test_weights( dummies_default_height );
    }
    GIVEN( "min height character" ) {
        test_weights( dummies_min_height );
    }
    GIVEN( "max height character" ) {
        test_weights( dummies_max_height );
    }
}

TEST_CASE( "riding various creatures at various sizes", "[avatar][bodyweight]" )
{
    monster cow( mon_cow );
    monster horse( mon_horse );
    monster pig( mon_pig );
    monster large_dog( mon_dog_gpyrenees );
    monster average_dog( mon_dog_gshepherd );

    using DummyMap = std::map<creature_size, avatar_ptr>;
    DummyMap dummies_default_height = create_dummies_of_all_sizes( Character::default_height() );
    DummyMap dummies_min_height = create_dummies_of_all_sizes( Character::min_height() );
    DummyMap dummies_max_height = create_dummies_of_all_sizes( Character::max_height() );

    auto can_mount = []( const avatar_ptr & dummy, const monster & steed ) {
        return dummy->bodyweight() <= steed.get_weight() * steed.get_mountable_weight_ratio();
    };

    SECTION( "default height character can ride all large steeds but not the small ones" ) {
        avatar_ptr dummy = dummies_default_height[creature_size::medium];
        CHECK( can_mount( dummy, cow ) );
        CHECK( can_mount( dummy, horse ) );
        CHECK( !can_mount( dummy, pig ) );
        CHECK( !can_mount( dummy, large_dog ) );
        CHECK( !can_mount( dummy, average_dog ) );
    }

    SECTION( "characters of any starting height can ride a horse" ) {
        CHECK( can_mount( dummies_min_height[creature_size::medium], horse ) );
        CHECK( can_mount( dummies_default_height[creature_size::medium], horse ) );
        CHECK( can_mount( dummies_max_height[creature_size::medium], horse ) );
    }

    SECTION( "huge characters can't ride a horse" ) {
        CHECK( !can_mount( dummies_min_height[creature_size::huge], horse ) );
        CHECK( !can_mount( dummies_default_height[creature_size::huge], horse ) );
        CHECK( !can_mount( dummies_max_height[creature_size::huge], horse ) );
    }

    SECTION( "only short tiny characters can ride large dogs" ) {
        CHECK( can_mount( dummies_min_height[creature_size::tiny], large_dog ) );
        CHECK( !can_mount( dummies_default_height[creature_size::tiny], large_dog ) );
        CHECK( !can_mount( dummies_max_height[creature_size::tiny], large_dog ) );
    }

    SECTION( "nobody can ride smaller dogs" ) {
        CHECK( !can_mount( dummies_min_height[creature_size::tiny], average_dog ) );
        CHECK( !can_mount( dummies_default_height[creature_size::tiny], average_dog ) );
        CHECK( !can_mount( dummies_max_height[creature_size::tiny], average_dog ) );
    }
}

// Return a string with multiple consecutive spaces replaced with a single space
static std::string condensed_spaces( const std::string &text )
{
    std::regex spaces( " +" );
    return std::regex_replace( text, spaces, " " );
}

// Make avatar have a given activity level for a certain time, returning to NO_EXERCISE when done
static void test_activity_duration( avatar &dummy, const float at_level,
                                    const time_duration &how_long )
{
    // Pass time at this level, updating body each turn
    for( int turn = 0; turn < to_turns<int>( how_long ); ++turn ) {
        // Make sure activity level persists (otherwise may be reset after 5 minutes)
        dummy.set_activity_level( at_level );
        calendar::turn += 1_turns;
        dummy.update_body();
    }
}

TEST_CASE( "activity levels and calories in daily diary", "[avatar][biometrics][activity][diary]" )
{
    // Typical spring start, day 61
    calendar::turn = calendar::turn_zero + 60_days;

    avatar dummy;
    clear_character( dummy );

    SECTION( "shows all zero at start of day 61" ) {
        CHECK( condensed_spaces( dummy.total_daily_calories_string() ) ==
               "<color_c_white> Minutes at each exercise level Calories per day</color>\n"
               "<color_c_yellow> Day Sleep None Light Moderate Brisk Active Extra Gained Spent Total</color>\n"
               "<color_c_light_gray> 61 0 0 0 0 0 0 0 0 0</color><color_c_light_gray> 0</color>\n" );
    }

    SECTION( "shows time at each activity level for the current day" ) {
        dummy.reset_activity_level();
        test_activity_duration( dummy, NO_EXERCISE, 59_minutes );
        test_activity_duration( dummy, LIGHT_EXERCISE, 45_minutes );
        test_activity_duration( dummy, MODERATE_EXERCISE, 30_minutes );
        test_activity_duration( dummy, BRISK_EXERCISE, 20_minutes );
        test_activity_duration( dummy, ACTIVE_EXERCISE, 15_minutes );
        test_activity_duration( dummy, EXTRA_EXERCISE, 10_minutes );
        test_activity_duration( dummy, NO_EXERCISE, 1_minutes );

        int expect_gained_kcal = 1282;
        int expect_net_kcal = 552;
        int expect_spent_kcal = 730;

        CHECK( condensed_spaces( dummy.total_daily_calories_string() ) == string_format(
                   "<color_c_white> Minutes at each exercise level Calories per day</color>\n"
                   "<color_c_yellow> Day Sleep None Light Moderate Brisk Active Extra Gained Spent Total</color>\n"
                   "<color_c_light_gray> 61 0 60 45 30 20 20 5 %d %d</color><color_c_light_blue> %d</color>\n",
                   expect_gained_kcal, expect_spent_kcal, expect_net_kcal ) );
    }
}

TEST_CASE( "mutations may affect character metabolic rate", "[biometrics][metabolism]" )
{
    avatar dummy;
    dummy.set_body();

    // Metabolic base rate uses PLAYER_HUNGER_RATE from game_balance.json, described as "base hunger
    // rate per 5 minutes". With no metabolism-affecting mutations, metabolism should be this value.
    const float normal_metabolic_rate = get_option<float>( "PLAYER_HUNGER_RATE" );
    dummy.clear_mutations();
    CHECK( dummy.metabolic_rate_base() == normal_metabolic_rate );

    // The remaining checks assume the configured base rate is 1.0; if this is ever changed in the
    // game balance JSON, the below tests are likely to fail and need adjustment.
    REQUIRE( normal_metabolic_rate == 1.0f );

    // Mutations with a "metabolism_modifier" in mutations.json add/subtract to the base rate.
    // For example the rapid / fast / very fast / extreme metabolisms:
    //
    //     MET_RAT (+0.333), HUNGER (+0.5), HUNGER2 (+1.0), HUNGER3 (+2.0)
    //
    // And the light eater / heat dependent / cold blooded spectrum:
    //
    //     LIGHTEATER (-0.333), COLDBLOOD (-0.333), COLDBLOOD2/3/4 (-0.5)
    //
    // If metabolism modifiers are changed, the below check(s) need to be adjusted as well.

    CHECK( metabolic_rate_with_mutation( dummy, "MET_RAT" ) == Approx( 1.333f ) );
    CHECK( metabolic_rate_with_mutation( dummy, "HUNGER" ) == Approx( 1.5f ) );
    CHECK( metabolic_rate_with_mutation( dummy, "HUNGER2" ) == Approx( 2.0f ) );
    CHECK( metabolic_rate_with_mutation( dummy, "HUNGER3" ) == Approx( 3.0f ) );
    CHECK( metabolic_rate_with_mutation( dummy, "LIGHTEATER" ) == Approx( 0.667f ) );
    CHECK( metabolic_rate_with_mutation( dummy, "COLDBLOOD" ) == Approx( 0.667f ) );
    CHECK( metabolic_rate_with_mutation( dummy, "COLDBLOOD2" ) == Approx( 0.5f ) );
    CHECK( metabolic_rate_with_mutation( dummy, "COLDBLOOD3" ) == Approx( 0.5f ) );
    CHECK( metabolic_rate_with_mutation( dummy, "COLDBLOOD4" ) == Approx( 0.5f ) );
}

TEST_CASE( "basal metabolic rate with various size and metabolism", "[biometrics][bmr]" )
{
    avatar dummy;
    dummy.set_body();

    // Basal metabolic rate depends on size (height), bodyweight (BMI), and activity level
    // scaled by metabolic base rate. Assume default metabolic rate.
    REQUIRE( dummy.metabolic_rate_base() == 1.0f );

    // To keep things simple, use normal BMI for all tests
    set_player_bmi( dummy, 25.0f );
    REQUIRE( dummy.get_bmi() == Approx( 25.0f ).margin( 0.001f ) );

    // Tests cover:
    // - normal, very fast, and cold-blooded metabolisms for normal body size
    // - three different levels of exercise (none, moderate, extra)

    // CHECK expressions have expected value on the left hand side for better readability.

    SECTION( "normal body size" ) {
        REQUIRE( dummy.get_size() == creature_size::medium );

        SECTION( "normal metabolism" ) {
            CHECK( 1738 == bmr_at_act_level( dummy, NO_EXERCISE ) );
            CHECK( 6952 == bmr_at_act_level( dummy, MODERATE_EXERCISE ) );
            CHECK( 17380 == bmr_at_act_level( dummy, EXTRA_EXERCISE ) );
        }

        SECTION( "very fast metabolism" ) {
            set_single_trait( dummy, "HUNGER2" );
            REQUIRE( dummy.metabolic_rate_base() == 2.0f );

            CHECK( 3476 == bmr_at_act_level( dummy, NO_EXERCISE ) );
            CHECK( 13904 == bmr_at_act_level( dummy, MODERATE_EXERCISE ) );
            CHECK( 34760 == bmr_at_act_level( dummy, EXTRA_EXERCISE ) );
        }

        SECTION( "very slow (cold-blooded) metabolism" ) {
            set_single_trait( dummy, "COLDBLOOD3" );
            REQUIRE( dummy.metabolic_rate_base() == 0.5f );

            CHECK( 869 == bmr_at_act_level( dummy, NO_EXERCISE ) );
            CHECK( 3476 == bmr_at_act_level( dummy, MODERATE_EXERCISE ) );
            CHECK( 8690 == bmr_at_act_level( dummy, EXTRA_EXERCISE ) );
        }
    }

    // Character's BMR should increase with body size for the same activity level
    std::map<creature_size, avatar_ptr> dummies = create_dummies_of_all_sizes( 175 );
    const std::vector<float> excercise_levels{ NO_EXERCISE, MODERATE_EXERCISE, EXTRA_EXERCISE };
    for( float level : excercise_levels ) {
        CHECK( bmr_at_act_level( *dummies[creature_size::tiny], level ) <
               bmr_at_act_level( *dummies[creature_size::small], level ) );
        CHECK( bmr_at_act_level( *dummies[creature_size::small], level ) <
               bmr_at_act_level( *dummies[creature_size::medium], level ) );
        CHECK( bmr_at_act_level( *dummies[creature_size::medium], level ) <
               bmr_at_act_level( *dummies[creature_size::large], level ) );
        CHECK( bmr_at_act_level( *dummies[creature_size::large ], level ) <
               bmr_at_act_level( *dummies[creature_size::huge], level ) );
    }

    // Character's BMR should increase with activity level for the same body size
    const std::vector<creature_size> sizes{ creature_size::tiny, creature_size::small, creature_size::medium, creature_size::large, creature_size::huge };
    for( creature_size size : sizes ) {
        CHECK( bmr_at_act_level( *dummies[size], NO_EXERCISE ) <
               bmr_at_act_level( *dummies[size], LIGHT_EXERCISE ) );
        CHECK( bmr_at_act_level( *dummies[size], LIGHT_EXERCISE ) <
               bmr_at_act_level( *dummies[size], MODERATE_EXERCISE ) );
        CHECK( bmr_at_act_level( *dummies[size], MODERATE_EXERCISE ) <
               bmr_at_act_level( *dummies[size], BRISK_EXERCISE ) );
        CHECK( bmr_at_act_level( *dummies[size], BRISK_EXERCISE ) <
               bmr_at_act_level( *dummies[size], ACTIVE_EXERCISE ) );
        CHECK( bmr_at_act_level( *dummies[size], ACTIVE_EXERCISE ) <
               bmr_at_act_level( *dummies[size], EXTRA_EXERCISE ) );
    }
}

TEST_CASE( "body mass effect on speed", "[bmi][speed]" )
{
    avatar dummy;

    // Practically dead
    CHECK( kcal_speed_penalty_at_bmi( dummy, 0.1f ) == -90 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 1.0f ) == -87 );
    // Skeletal (<14)
    CHECK( kcal_speed_penalty_at_bmi( dummy, 8.0f ) == -67 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 9.0f ) == -64 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 10.0f ) == -61 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 11.0f ) == -59 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 12.0f ) == -56 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 13.0f ) == -53 );
    // Emaciated (14)
    CHECK( kcal_speed_penalty_at_bmi( dummy, 14.0f ) == -50 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 14.5f ) == -44 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 15.0f ) == -38 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 15.5f ) == -31 );
    // Underweight (16)
    CHECK( kcal_speed_penalty_at_bmi( dummy, 16.0f ) == -25 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 16.5f ) == -20 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 17.0f ) == -15 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 17.5f ) == -10 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 18.0f ) == -5 );
    // Normal (18.5)
    CHECK( kcal_speed_penalty_at_bmi( dummy, 18.5f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 19.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 20.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 21.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 22.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 23.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 24.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 25.0f ) == 0 );
    // Overweight (25)
    CHECK( kcal_speed_penalty_at_bmi( dummy, 26.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 27.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 28.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 29.0f ) == 0 );
    // Obese (30)
    CHECK( kcal_speed_penalty_at_bmi( dummy, 30.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 31.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 32.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 33.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 34.0f ) == 0 );
    // Very obese (35)
    CHECK( kcal_speed_penalty_at_bmi( dummy, 35.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 36.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 37.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 38.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 39.0f ) == 0 );
    // Morbidly obese (40)
    CHECK( kcal_speed_penalty_at_bmi( dummy, 40.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 41.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 42.0f ) == 0 );
}
