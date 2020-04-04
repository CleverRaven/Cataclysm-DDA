#include "avatar.h"

#include "catch/catch.hpp"

// Return the player's `get_bmi` at `kcal_percent` (actually a ratio of stored_kcal to healthy_kcal)
static float bmi_at_kcal_ratio( player &dummy, float kcal_percent )
{
    dummy.set_stored_kcal( dummy.get_healthy_kcal() * kcal_percent );
    REQUIRE( dummy.get_kcal_percent() == Approx( kcal_percent ).margin( 0.001f ) );

    return dummy.get_bmi();
}

// Return the `kcal_ratio` needed to reach the given `bmi`
//   BMI = 13 + (12 * kcal_ratio)
//   kcal_ratio = (BMI - 13) / 12
static float bmi_to_kcal_ratio( float bmi )
{
    return ( bmi - 13 ) / 12;
}

// Return the player's `get_max_healthy` value at the given body mass index
static int max_healthy_at_bmi( player &dummy, float bmi )
{
    dummy.set_stored_kcal( dummy.get_healthy_kcal() * bmi_to_kcal_ratio( bmi ) );
    REQUIRE( dummy.get_bmi() == Approx( bmi ).margin( 0.001f ) );

    return dummy.get_max_healthy();
}

// Return the player's `get_weight_string` at the given body mass index
static std::string weight_string_at_bmi( player &dummy, float bmi )
{
    dummy.set_stored_kcal( dummy.get_healthy_kcal() * bmi_to_kcal_ratio( bmi ) );
    REQUIRE( dummy.get_bmi() == Approx( bmi ).margin( 0.001f ) );

    return dummy.get_weight_string();
}


TEST_CASE( "body mass index determines weight description", "[healthy][bmi][weight]" )
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

TEST_CASE( "stored kcal ratio determines body mass index", "[healthy][kcal][bmi]" )
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

TEST_CASE( "body mass index determines maximum healthiness", "[healthy][bmi][max]" )
{
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
    // Overweight (25)
    CHECK( max_healthy_at_bmi( dummy, 25.0f ) == 200 );
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

