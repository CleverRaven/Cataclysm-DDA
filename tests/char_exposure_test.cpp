#include <list>
#include <map>
#include <utility>

#include "cata_catch.h"
#include "character.h"
#include "item.h"
#include "player_helpers.h"
#include "type_id.h"

// Tests for Character bodypart exposure
//
// Covers functions:
// - Character::bodypart_exposure

TEST_CASE( "character body part exposure", "[char][bodypart][exposure]" )
{
    Character &dummy = get_player_character();
    clear_avatar();

    bodypart_id head = bodypart_str_id( "head" );
    bodypart_id torso = bodypart_str_id( "torso" );
    bodypart_id arm_l = bodypart_str_id( "arm_l" );
    bodypart_id arm_r = bodypart_str_id( "arm_r" );
    bodypart_id hand_l = bodypart_str_id( "hand_l" );
    bodypart_id hand_r = bodypart_str_id( "hand_r" );
    bodypart_id leg_l = bodypart_str_id( "leg_l" );
    bodypart_id leg_r = bodypart_str_id( "leg_r" );
    bodypart_id foot_l = bodypart_str_id( "foot_l" );
    bodypart_id foot_r = bodypart_str_id( "foot_r" );

    std::map<bodypart_id, float> bp_exposure;

    GIVEN( "nothing is worn" ) {
        dummy.worn.clear();

        THEN( "exposure is 100% on all body parts" ) {
            bp_exposure = dummy.bodypart_exposure();
            for( std::pair<const bodypart_id, float> &bp_exp : bp_exposure ) {
                CHECK( bp_exp.second == 1.0 );
            }
        }

        WHEN( "wearing an item with 100% coverage on some parts" ) {
            item socks( "test_socks" );
            REQUIRE( socks.get_coverage( foot_l ) == 100 );
            REQUIRE( socks.get_coverage( foot_r ) == 100 );
            dummy.wear_item( socks, false );

            bp_exposure = dummy.bodypart_exposure();

            THEN( "exposure is 0 on covered parts" ) {
                CHECK( bp_exposure[foot_l] == 0.0f );
                CHECK( bp_exposure[foot_l] == 0.0f );
            }

            THEN( "exposure is 100% on exposed parts" ) {
                CHECK( bp_exposure[hand_l] == 1.0f );
                CHECK( bp_exposure[hand_r] == 1.0f );
            }
        }

        WHEN( "wearing an item with 50% coverage on some parts" ) {
            item croc_socks( "test_croc_socks" );
            REQUIRE( croc_socks.get_coverage( foot_l ) == 50 );
            REQUIRE( croc_socks.get_coverage( foot_r ) == 50 );
            dummy.wear_item( croc_socks, false );

            bp_exposure = dummy.bodypart_exposure();

            THEN( "exposure is 50% on covered parts" ) {
                CHECK( bp_exposure[foot_l] == 0.5f );
                CHECK( bp_exposure[foot_r] == 0.5f );
            }

            AND_WHEN( "wearing another item with 50% coverage on the same parts" ) {
                item croc_socks2( "test_croc_socks" );
                dummy.wear_item( croc_socks2, false );

                bp_exposure = dummy.bodypart_exposure();

                THEN( "exposure is reduced to 25% on doubly-covered parts" ) {
                    CHECK( bp_exposure[foot_l] == 0.25f );
                    CHECK( bp_exposure[foot_r] == 0.25f );
                }
            }
        }

        WHEN( "wearing an item with 90% coverage on some parts" ) {
            item shirt( "test_longshirt" );
            REQUIRE( shirt.get_coverage( torso ) == 90 );
            REQUIRE( shirt.get_coverage( arm_l ) == 90 );
            REQUIRE( shirt.get_coverage( arm_r ) == 90 );
            dummy.wear_item( shirt, false );

            bp_exposure = dummy.bodypart_exposure();

            THEN( "exposure is 10% on covered parts" ) {
                CHECK( bp_exposure[torso] == Approx( 0.1f ) );
                CHECK( bp_exposure[arm_l] == Approx( 0.1f ) );
                CHECK( bp_exposure[arm_r] == Approx( 0.1f ) );
            }
            THEN( "exposure is 100% on exposed parts" ) {
                CHECK( bp_exposure[head] == 1.0f );
                CHECK( bp_exposure[leg_l] == 1.0f );
                CHECK( bp_exposure[leg_r] == 1.0f );
                CHECK( bp_exposure[hand_l] == 1.0f );
                CHECK( bp_exposure[hand_r] == 1.0f );
                CHECK( bp_exposure[foot_l] == 1.0f );
                CHECK( bp_exposure[foot_r] == 1.0f );
            }

            AND_WHEN( "wearing another item with 90% coverage on the same parts" ) {
                item shirt2( "test_longshirt" );
                dummy.wear_item( shirt2, false );

                bp_exposure = dummy.bodypart_exposure();

                THEN( "exposure is reduced to 1% on doubly-covered parts" ) {
                    CHECK( bp_exposure[torso] == Approx( 0.01f ) );
                    CHECK( bp_exposure[arm_l] == Approx( 0.01f ) );
                    CHECK( bp_exposure[arm_r] == Approx( 0.01f ) );
                }
            }
        }
    }
}

