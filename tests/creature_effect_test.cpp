#include "avatar.h"
#include "creature.h"
#include "monster.h"

#include "catch/catch.hpp"

// TODO:
// - Creature::process_effects
// - Creature::clear_effects
// - Creature::add_env_effect
// - Creature::has_effect_with_flag
// - Creature::get_effect
// - Creature::get_effect_dur
// - Creature::get_effect_int
// - Creature::resists_effect
// - Creature::is_immune_effect

// Creature::has_effect
// - Works for both monster and player
// - For monster, has_effect is not body-part-specific
// - For player, has_effect may apply to individual body parts
TEST_CASE( "monster has_effect", "[creature][monster][effect][has]" )
{
    monster mummy( mtype_id( "debug_mon" ) );
    const efftype_id effect_downed( "downed" );

    mummy.clear_effects();

    WHEN( "monster does not have the effect" ) {

        THEN( "has_effect is false" ) {
            CHECK_FALSE( mummy.has_effect( effect_downed ) );
        }
    }

    WHEN( "monster has the effect" ) {
        mummy.add_effect( effect_downed, 1_minutes );

        THEN( "has_effect is true" ) {
            CHECK( mummy.has_effect( effect_downed ) );
        }
    }
}

TEST_CASE( "player has_effect", "[creature][monster][effect][has]" )
{
    avatar dummy;
    const efftype_id effect_grabbed( "grabbed" );
    const body_part left_arm = bodypart_id( "arm_l" )->token;
    const body_part right_arm = bodypart_id( "arm_r" )->token;

    dummy.clear_effects();

    WHEN( "player has the effect on a single body part" ) {
        dummy.add_effect( effect_grabbed, 1_minutes, left_arm );

        THEN( "has_effect with no body part returns true" ) {
            CHECK( dummy.has_effect( effect_grabbed ) );
        }
        THEN( "has_effect with the affected body part returns true" ) {
            CHECK( dummy.has_effect( effect_grabbed, left_arm ) );
        }
        THEN( "has_effect with an unaffected body part returns false" ) {
            CHECK_FALSE( dummy.has_effect( effect_grabbed, right_arm ) );
        }
    }
}

// Creature::add_effect
// TODO:
// Parameters:
// - Has id, duration
// - Optional bodypart, permanence, intensity
// - May be "permanent" (also known as "paused")
// - May be "force"d (to override immunity)
// - May be "deferred" to not process it immediately
// Behavior:
// - If already have same effect, modify duration and intensity
// - Duration is limited to maximum for effect
//
TEST_CASE( "monster add_effect", "[creature][effect][add]" )
{
    monster mummy( mtype_id( "debug_mon" ) );
    const efftype_id effect_grabbed( "grabbed" );

    mummy.clear_effects();

    WHEN( "monster add_effect is called" ) {
        mummy.add_effect( effect_grabbed, 1_minutes );
        THEN( "has_effect is true" ) {
            CHECK( mummy.has_effect( effect_grabbed ) );
        }
    }
}

// Creature::remove_effect
//
TEST_CASE( "remove_effect", "[creature][effect][remove]" )
{
    avatar dummy;
    const efftype_id effect_grabbed( "grabbed" );
    const body_part left_arm = bodypart_id( "arm_l" )->token;
    const body_part right_arm = bodypart_id( "arm_r" )->token;
    const body_part left_leg = bodypart_id( "leg_l" )->token;
    const body_part right_leg = bodypart_id( "leg_r" )->token;

    dummy.clear_effects();

    WHEN( "player does not have the effect" ) {
        REQUIRE_FALSE( dummy.has_effect( effect_grabbed ) );
        THEN( "remove_effect returns false" ) {
            CHECK_FALSE( dummy.remove_effect( effect_grabbed ) );
        }
    }

    WHEN( "player has the effect on one body part" ) {
        // Left arm grabbed
        dummy.add_effect( effect_grabbed, 1_minutes, left_arm );
        REQUIRE( dummy.has_effect( effect_grabbed, left_arm ) );

        THEN( "remove_effect with no body part returns true" ) {
            // Release all grabs
            CHECK( dummy.remove_effect( effect_grabbed ) );
            // All grabs are released
            AND_THEN( "the effect is gone" ) {
                CHECK_FALSE( dummy.has_effect( effect_grabbed ) );
            }
        }

        THEN( "remove_effect with the affected body part returns true" ) {
            // Release grab on left arm
            CHECK( dummy.remove_effect( effect_grabbed, left_arm ) );
            // Left arm is released
            AND_THEN( "the effect is gone from that body part" ) {
                CHECK_FALSE( dummy.has_effect( effect_grabbed, left_arm ) );
            }
        }

        THEN( "remove_effect with an unaffected body part returns false" ) {
            // Release (nonexistent) grab on right arm
            CHECK_FALSE( dummy.remove_effect( effect_grabbed, right_arm ) );
            // Left arm is still grabbed
            AND_THEN( "the effect still applies to the original body part" ) {
                CHECK( dummy.has_effect( effect_grabbed, left_arm ) );
            }
        }
    }

    WHEN( "player has the effect on two body parts" ) {
        // Both arms grabbed
        dummy.add_effect( effect_grabbed, 1_minutes, left_arm );
        dummy.add_effect( effect_grabbed, 1_minutes, right_arm );
        REQUIRE( dummy.has_effect( effect_grabbed, left_arm ) );
        REQUIRE( dummy.has_effect( effect_grabbed, right_arm ) );

        THEN( "remove_effect with no body part returns true" ) {
            // Release all grabs
            CHECK( dummy.remove_effect( effect_grabbed ) );
            // Both arms are released
            AND_THEN( "the effect is gone from all body parts" ) {
                CHECK_FALSE( dummy.has_effect( effect_grabbed, left_arm ) );
                CHECK_FALSE( dummy.has_effect( effect_grabbed, right_arm ) );
            }
        }

        THEN( "remove_effect with an affected body part returns true" ) {
            // Only release left arm
            CHECK( dummy.remove_effect( effect_grabbed, left_arm ) );
            // Left arm is released
            AND_THEN( "the effect is gone from that body part" ) {
                CHECK_FALSE( dummy.has_effect( effect_grabbed, left_arm ) );
            }
            // Right arm still grabbed
            AND_THEN( "the effect still applies to the other body part" ) {
                CHECK( dummy.has_effect( effect_grabbed, right_arm ) );
            }
        }

        THEN( "remove_effect with an unaffected body part returns false" ) {
            // Release (nonexistent) grabs on legs
            CHECK_FALSE( dummy.remove_effect( effect_grabbed, left_leg ) );
            CHECK_FALSE( dummy.remove_effect( effect_grabbed, right_leg ) );
            // Arms are still grabbed
            AND_THEN( "the effect still applies to the original body parts" ) {
                CHECK( dummy.has_effect( effect_grabbed, left_arm ) );
                CHECK( dummy.has_effect( effect_grabbed, right_arm ) );
            }
        }
    }
}

