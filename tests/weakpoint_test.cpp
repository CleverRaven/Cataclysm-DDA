#include <string>

#include "cata_catch.h"
#include "damage.h"
#include "game_constants.h"
#include "monster.h"
#include "point.h"
#include "type_id.h"

static void damage_monster( const std::string &target_type, const damage_instance &dam, float ave)
{
    CAPTURE( target_type );
    int total = 0;
    for (int i = 0; i < 100; i++) {
        monster target{ mtype_id( target_type ), tripoint_zero };
        target.deal_damage( NULL, bodypart_id( "torso" ), dam );
        int damage_dealt = target.get_hp_max() - target.get_hp();
        total += damage_dealt;
        CAPTURE( damage_dealt );
    }
    CHECK( (float)total/100.0f == Approx( ave ).epsilon( 0.15f ) );
}

TEST_CASE( "monster_weakpoint", "[monster]" )
{
    // Debug Monster should hit weakpoint 25% of time, and do 75 damage up from 0 when not hitting weakpoint, so average of 18.75
    damage_monster( "debug_mon", damage_instance( damage_type::BULLET, 100, 0.0f ), 18.75f );
    // 25 arm pen adds to average
    damage_monster( "debug_mon", damage_instance( damage_type::BULLET, 100, 25.0f ), 18.75f + 25.0f );
    // 50 arm pen only helps non-weakpoint hits, weakpoint saturates at 25 AP
    damage_monster( "debug_mon", damage_instance( damage_type::BULLET, 100, 50.0f ), 62.5f );
    // No cut armor
    damage_monster( "debug_mon", damage_instance( damage_type::CUT, 100, 0.0f ), 100.0f );
}
