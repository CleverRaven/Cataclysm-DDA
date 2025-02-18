#include <algorithm>
#include <string>
#include <vector>

#include "cata_catch.h"
#include "monster.h"
#include "mtype.h"
#include "player_helpers.h"
#include "type_id.h"

static const mtype_id mon_test_speed_desc_base( "mon_test_speed_desc_base" );
static const mtype_id mon_test_speed_desc_base_150( "mon_test_speed_desc_base_150" );
static const mtype_id mon_test_speed_desc_base_25( "mon_test_speed_desc_base_25" );
static const mtype_id mon_test_speed_desc_base_50( "mon_test_speed_desc_base_50" );
static const mtype_id mon_test_speed_desc_base_immobile( "mon_test_speed_desc_base_immobile" );
static const mtype_id mon_test_speed_desc_multiple( "mon_test_speed_desc_multiple" );

static const speed_description_id
speed_description_ID_THAT_DOES_NOT_EXIST( "ID_THAT_DOES_NOT_EXIST" );

TEST_CASE( "monster_speed_description", "[monster][speed_description]" )
{
    /*
     * A default character has a speed of 100 and a base move cost of 116
     * That means their tiles per turn is 100 / 116 == 0.86206896551
     * The tiles per turn ratio is monster_ratio / player_ratio
     */

    auto get_speed_string = []( const mtype_id & mon_id ) {
        clear_avatar();
        monster mon( mon_id );
        return monster::speed_description(
                   mon.speed_rating(),
                   mon.has_flag( mon_flag_IMMOBILE ),
                   mon.type->speed_desc
               );
    };

    // while lambdas speed things up, in-case the check fails
    // they don't give the line where the check failed
    // so give the contributor as much info as possible when describing the cases
    auto make_test = [&get_speed_string]( const mtype_id & mon_id,
    const std::vector<std::string> &descriptions ) {
        std::string speed_string = get_speed_string( mon_id );
        THEN( "returned string is the one expected" ) {
            const bool is_returned_string_is_inside_vector = std::find(
                        descriptions.begin(), descriptions.end(),
                        speed_string ) != descriptions.end();
            // variable name will show up when error fails
            CHECK( is_returned_string_is_inside_vector );
        }
    };

    SECTION( "passing invalid parameters" ) {
        GIVEN( "a null id" ) {
            std::string speed_string =
                monster::speed_description( 1.0, false, speed_description_id::NULL_ID() );

            THEN( "returned string is empty" ) {
                CHECK( speed_string.empty() );
            }
        }

        GIVEN( "an invalid id" ) {
            std::string speed_string =
                monster::speed_description( 1.0, false,
                                            speed_description_ID_THAT_DOES_NOT_EXIST );

            THEN( "returned string is empty" ) {
                CHECK( speed_string.empty() );
            }
        }
    }

    SECTION( "monster with valid speed description" ) {
        GIVEN( "immobile monster" ) {
            make_test( mon_test_speed_desc_base_immobile, {"Monster immobile"} );
        }

        GIVEN( "monster with 25 speed" ) {
            make_test( mon_test_speed_desc_base_25, {"Monster speed 25"} );
        }

        GIVEN( "monster with 50 speed" ) {
            make_test( mon_test_speed_desc_base_50, {"Monster speed 50"} );
        }

        GIVEN( "monster with 100 speed" ) {
            make_test( mon_test_speed_desc_base, {"Monster speed 100"} );
        }

        GIVEN( "monster with 150 speed" ) {
            make_test( mon_test_speed_desc_base_150, {"Monster speed 150"} );
        }
    }

    SECTION( "monster with multiple speed description values" ) {
        GIVEN( "monster with 100 speed and multiple speed descriptions" ) {
            make_test( mon_test_speed_desc_multiple, {
                "A Monster speed description A",
                "B Monster speed description B",
                "C Monster speed description C"
            } );
        }
    }
}
