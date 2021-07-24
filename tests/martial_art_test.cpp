#include "catch/catch.hpp"
#include "player_helpers.h"

#include "avatar.h"
#include "martialarts.h"

static const activity_id ACT_NULL( "ACT_NULL" );
static const activity_id ACT_SHEARING( "ACT_SHEARING" );

static const efftype_id effect_pet( "pet" );
static const efftype_id effect_tied( "tied" );

static const itype_id itype_test_battery_disposable( "test_battery_disposable" );
static const itype_id itype_test_shears( "test_shears" );
static const itype_id itype_test_shears_off( "test_shears_off" );

static const itype_id itype_test_weapon1( "test_weapon1" );
static const itype_id itype_test_weapon2( "test_weapon2" );

static const matype_id matype_test_style_ma1( "test_style_ma1" );

static const mtype_id mon_test_shearable( "mon_test_shearable" );
static const mtype_id mon_test_non_shearable( "mon_test_non_shearable" );

static const quality_id qual_SHEAR( "SHEAR" );

TEST_CASE( "martial arts", "[martial_arts]" )
{
    avatar &dummy = get_avatar();
    clear_avatar();

    SECTION( "martial art valid weapon" ) {
        GIVEN( "a weapon that fits the martial art" ) {
            CHECK( matype_test_style_ma1->has_weapon( itype_test_weapon1 ) );
        }

        GIVEN( "a weapon that fits the martial art weapon category" ) {
            CHECK( matype_test_style_ma1->has_weapon( itype_test_weapon2 ) );
        }
    }
}
