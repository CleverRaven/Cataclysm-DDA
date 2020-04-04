#include "avatar.h"

#include "catch/catch.hpp"
#include "player_helpers.h"

TEST_CASE( "stored kcals and body mass index", "[healthy][kcal][bmi]" )
{
    avatar dummy;

    GIVEN( "character with normal calorie requirements" ) {

        // Assuming healthy_calories is set to 55000 in Character constructor
        REQUIRE( dummy.get_healthy_kcal() == 55000 );

        WHEN( "stored calories are exactly equal to healthy value" ) {
            dummy.set_stored_kcal( dummy.get_healthy_kcal() );
            REQUIRE( dummy.get_kcal_percent() == 1.0f );

            // 1.0*12 +13 = 25
            THEN( "BMI = 25, on the cusp of 'Normal' to 'Overweight'" ) {
                CHECK( dummy.get_bmi() == Approx( 25.0f ) );
                CHECK( dummy.get_bmi() == Approx( character_weight_category::overweight ) );
                CHECK( dummy.get_weight_string() == "Normal" );
            }
        }

        WHEN( "stored calories are 3/4 of healthy value" ) {
            dummy.set_stored_kcal( 0.75f * dummy.get_healthy_kcal() );
            REQUIRE( dummy.get_kcal_percent() == 0.75f );

            // 0.75*12 +13 = 22
            THEN( "BMI = 22, 'Normal'" ) {
                CHECK( dummy.get_bmi() == Approx( 22.0f ) );
                CHECK( dummy.get_bmi() < character_weight_category::overweight );
                CHECK( dummy.get_bmi() > character_weight_category::normal );
                CHECK( dummy.get_weight_string() == "Normal" );
            }
        }

        WHEN( "stored calories are 1/2 of healthy value" ) {
            dummy.set_stored_kcal( 0.5f * dummy.get_healthy_kcal() );
            REQUIRE( dummy.get_kcal_percent() == 0.5f );

            // 0.5*12 +13 = 19
            THEN( "BMI = 19, 'Normal'" ) {
                CHECK( dummy.get_bmi() == Approx( 19.0f ) );
                CHECK( dummy.get_bmi() < character_weight_category::overweight );
                CHECK( dummy.get_bmi() > character_weight_category::normal );
                CHECK( dummy.get_weight_string() == "Normal" );
            }
        }

        WHEN( "stored calories are 1/3 of healthy value" ) {
            dummy.set_stored_kcal( 1.0f / 3 * dummy.get_healthy_kcal() );
            REQUIRE( dummy.get_kcal_percent() == Approx( 1.0f / 3 ).margin( 0.001f ) );

            // 0.33*12 +13 = 17
            THEN( "BMI = 17, 'Underweight'" ) {
                CHECK( dummy.get_bmi() == Approx( 17.0f ).margin( 0.001f ) );
                CHECK( dummy.get_bmi() < character_weight_category::normal );
                CHECK( dummy.get_bmi() > character_weight_category::underweight );
                CHECK( dummy.get_weight_string() == "Underweight" );
            }
        }

        WHEN( "stored calories are 1/4 of healthy value" ) {
            dummy.set_stored_kcal( 0.25f * dummy.get_healthy_kcal() );
            REQUIRE( dummy.get_kcal_percent() == 0.25f );

            // 0.25*12 +13 = 16
            THEN( "BMI = 16, on the cusp of 'Underweight' to 'Emaciated'" ) {
                CHECK( dummy.get_bmi() == Approx( 16.0f ) );
                CHECK( dummy.get_bmi() == Approx( character_weight_category::underweight ) );
                CHECK( dummy.get_weight_string() == "Emaciated" );
            }
        }

        WHEN( "stored calories are 1.25x healthy value" ) {
            dummy.set_stored_kcal( 1.25f * dummy.get_healthy_kcal() );
            REQUIRE( dummy.get_kcal_percent() == 1.25f );

            // 1.25*12 +13 = 28
            THEN( "BMI = 28, 'Overweight'" ) {
                CHECK( dummy.get_bmi() == Approx( 28.0f ) );
                CHECK( dummy.get_bmi() > character_weight_category::overweight );
                CHECK( dummy.get_bmi() < character_weight_category::obese );
                CHECK( dummy.get_weight_string() == "Overweight" );
            }
        }
        WHEN( "stored calories are 1.5x healthy value" ) {
            dummy.set_stored_kcal( 1.5f * dummy.get_healthy_kcal() );
            REQUIRE( dummy.get_kcal_percent() == 1.5f );

            // 1.5*12 +13 = 31
            THEN( "BMI = 31, 'Obese'" ) {
                CHECK( dummy.get_bmi() == Approx( 31.0f ) );
                CHECK( dummy.get_bmi() > character_weight_category::obese );
                CHECK( dummy.get_bmi() < character_weight_category::very_obese );
                CHECK( dummy.get_weight_string() == "Obese" );
            }
        }

        WHEN( "stored calories are 2x healthy value" ) {
            dummy.set_stored_kcal( 2.0f * dummy.get_healthy_kcal() );
            REQUIRE( dummy.get_kcal_percent() == 2.0f );

            // 2*12 +13 = 37
            THEN( "BMI = 37, 'Very Obese'" ) {
                CHECK( dummy.get_bmi() == Approx( 37.0f ) );
                CHECK( dummy.get_bmi() > character_weight_category::very_obese );
                CHECK( dummy.get_bmi() < character_weight_category::morbidly_obese );
                CHECK( dummy.get_weight_string() == "Very Obese" );
            }
        }

        WHEN( "stored calories are 2.5x healthy value" ) {
            dummy.set_stored_kcal( 2.5f * dummy.get_healthy_kcal() );
            REQUIRE( dummy.get_kcal_percent() == 2.5f );

            // 2.5*12 +13 = 43
            THEN( "BMI = 43, 'Morbidly Obese'" ) {
                CHECK( dummy.get_bmi() == Approx( 43.0f ) );
                CHECK( dummy.get_bmi() > character_weight_category::morbidly_obese );
                CHECK( dummy.get_weight_string() == "Morbidly Obese" );
            }
        }
    }

}

TEST_CASE( "maximum healthiness", "[healthy][max]" )
{
}
