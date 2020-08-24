#include "catch/catch.hpp"

#include "avatar.h"
#include "player_helpers.h"

// Tests for Character bodypart exposure
//
// Covers functions:
// - Character::bodypart_exposure

TEST_CASE( "character body part exposure", "[char][bodypart][exposure]" )
{
    Character &dummy = get_player_character();
    clear_avatar();
    bodypart_id foot_l = bodypart_str_id( "foot_l" );

    // This test covers four use cases on the left foot wearing:
    // - nothing: 100% exposure
    // - socks: 0% exposure
    // - croc socks: 50% exposure
    // - two layers of croc socks: 25% exposure

    GIVEN( "nothing is worn" ) {
        dummy.worn.clear();

        THEN( "exposure is 100%%" ) {
            CHECK( dummy.bodypart_exposure()[foot_l] == 1.0f );
        }

        WHEN( "wearing an item with 100%% coverage" ) {
            item socks( "test_socks" );
            REQUIRE( socks.get_coverage( foot_l ) == 100 );
            dummy.wear_item( socks, false );

            THEN( "exposure is 0" ) {
                CHECK( dummy.bodypart_exposure()[foot_l] == 0.0f );
            }
        }

        WHEN( "wearing an item with 50%% coverage" ) {
            item croc_socks( "test_croc_socks" );
            REQUIRE( croc_socks.get_coverage( foot_l ) == 50 );
            dummy.wear_item( croc_socks, false );

            THEN( "exposure is 50%%" ) {
                CHECK( dummy.bodypart_exposure()[foot_l] == 0.5f );
            }

            AND_WHEN( "wearing another item with 50%% coverage" ) {
                item croc_socks2( "test_croc_socks" );
                dummy.wear_item( croc_socks2, false );

                THEN( "exposure is reduced to 25%%" ) {
                    CHECK( dummy.bodypart_exposure()[foot_l] == 0.25f );
                }
            }
        }
    }
}

