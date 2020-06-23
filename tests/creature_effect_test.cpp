#include "avatar.h"
#include "creature.h"
#include "monster.h"

#include "catch/catch.hpp"

// Test effect methods from `Creature` class on both `monster` and `player`

// Functions covered:
// Creature::add_effect
// Creature::remove_effect
// Creature::has_effect
// Creature::has_effect_with_flag
//
// TODO:
// - Creature::resists_effect
// - Creature::process_effects
// - Creature::add_env_effect
// - Creature::get_effect
// - Creature::get_effect_dur
// - Creature::get_effect_int

// Creature::add_effect
//
// TODO:
// - May be "deferred" to not process it immediately
// - For knockdown, if riding/ridden, force dismount
// - Duration is limited to maximum for effect
// - If already have same effect, modify duration and intensity
// - Otherwise, add effect, and check if it is blocked by another effect

// Characters have effects on separate body parts, or no particular part (indicated by `num_bp`)
TEST_CASE( "character add_effect", "[creature][character][effect][add]" )
{
    avatar dummy;
    const efftype_id effect_bleed( "bleed" );
    const efftype_id effect_grabbed( "grabbed" );
    const body_part left_arm = bodypart_id( "arm_l" )->token;
    const body_part right_arm = bodypart_id( "arm_r" )->token;

    GIVEN( "character is susceptible to effect" ) {
        REQUIRE_FALSE( dummy.is_immune_effect( effect_bleed ) );
        REQUIRE_FALSE( dummy.is_immune_effect( effect_grabbed ) );

        WHEN( "character add_effect is called without a body part" ) {
            dummy.add_effect( effect_bleed, 1_minutes );
            dummy.add_effect( effect_grabbed, 1_minutes );

            THEN( "they have the effect" ) {
                CHECK( dummy.has_effect( effect_bleed ) );
                CHECK( dummy.has_effect( effect_grabbed ) );
            }
        }

        // Make left arm bleed, and grab right arm
        WHEN( "character add_effect is called with a body part" ) {
            dummy.add_effect( effect_bleed, 1_minutes, left_arm );
            dummy.add_effect( effect_grabbed, 1_minutes, right_arm );

            // Left arm bleeding, right arm grabbed
            THEN( "they have the effect on that body part" ) {
                CHECK( dummy.has_effect( effect_bleed, left_arm ) );
                CHECK( dummy.has_effect( effect_grabbed, right_arm ) );
            }

            // Left arm not grabbed, right arm not bleeding
            THEN( "they do not have the effect on another body part" ) {
                CHECK_FALSE( dummy.has_effect( effect_grabbed, left_arm ) );
                CHECK_FALSE( dummy.has_effect( effect_bleed, right_arm ) );
            }
        }
    }
}

// Monsters may have effects added to them, but they don't have separate body parts.
TEST_CASE( "monster add_effect", "[creature][monster][effect][add]" )
{
    monster mummy( mtype_id( "debug_mon" ) );
    const efftype_id effect_bleed( "bleed" );
    const efftype_id effect_grabbed( "grabbed" );

    mummy.clear_effects();

    GIVEN( "monster is susceptible to effect" ) {
        REQUIRE_FALSE( mummy.is_immune_effect( effect_grabbed ) );
        WHEN( "monster add_effect is called" ) {
            mummy.add_effect( effect_grabbed, 1_minutes );
            THEN( "they have the effect" ) {
                CHECK( mummy.has_effect( effect_grabbed ) );
            }
        }
    }

    // Debug monster is "flesh", but doesn't have the "WARM" flag, so is immune to bleeding.
    GIVEN( "monster is immune to effect" ) {
        REQUIRE( mummy.is_immune_effect( effect_bleed ) );

        THEN( "monster add_effect is called with force = false" ) {
            mummy.add_effect( effect_bleed, 1_minutes, num_bp, false, 1, false );
            THEN( "they do not have the effect" ) {
                CHECK_FALSE( mummy.has_effect( effect_bleed ) );
            }
        }

        WHEN( "monster add_effect is called with force = true" ) {
            mummy.add_effect( effect_bleed, 1_minutes, num_bp, false, 1, true );
            THEN( "they have the effect" ) {
                CHECK( mummy.has_effect( effect_bleed ) );
            }
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

    WHEN( "character does not have effect" ) {
        REQUIRE_FALSE( dummy.has_effect( effect_grabbed ) );
        THEN( "remove_effect returns false" ) {
            CHECK_FALSE( dummy.remove_effect( effect_grabbed ) );
        }
    }

    // Left arm grabbed
    WHEN( "character has effect on one body part" ) {
        dummy.add_effect( effect_grabbed, 1_minutes, left_arm );
        REQUIRE( dummy.has_effect( effect_grabbed, left_arm ) );

        THEN( "remove_effect with no body part returns true" ) {
            // Release all grabs
            CHECK( dummy.remove_effect( effect_grabbed ) );
            // All grabs are released
            AND_THEN( "effect is removed" ) {
                CHECK_FALSE( dummy.has_effect( effect_grabbed ) );
            }
        }

        THEN( "remove_effect with affected body part returns true" ) {
            // Release grab on left arm
            CHECK( dummy.remove_effect( effect_grabbed, left_arm ) );
            // Left arm is released
            AND_THEN( "effect is removed from that body part" ) {
                CHECK_FALSE( dummy.has_effect( effect_grabbed, left_arm ) );
            }
        }

        THEN( "remove_effect with an unaffected body part returns false" ) {
            // Release (nonexistent) grab on right arm
            CHECK_FALSE( dummy.remove_effect( effect_grabbed, right_arm ) );
            // Left arm is still grabbed
            AND_THEN( "effect still applies to original body part" ) {
                CHECK( dummy.has_effect( effect_grabbed, left_arm ) );
            }
        }
    }

    // Both arms grabbed
    WHEN( "character has effect on two body parts" ) {
        dummy.add_effect( effect_grabbed, 1_minutes, left_arm );
        dummy.add_effect( effect_grabbed, 1_minutes, right_arm );
        REQUIRE( dummy.has_effect( effect_grabbed, left_arm ) );
        REQUIRE( dummy.has_effect( effect_grabbed, right_arm ) );

        // Release all grabs
        THEN( "remove_effect with no body part returns true" ) {
            CHECK( dummy.remove_effect( effect_grabbed ) );
            // Both arms are released
            AND_THEN( "effect is removed from all body parts" ) {
                CHECK_FALSE( dummy.has_effect( effect_grabbed, left_arm ) );
                CHECK_FALSE( dummy.has_effect( effect_grabbed, right_arm ) );
            }
        }

        // Only release left arm
        THEN( "remove_effect with an affected body part returns true" ) {
            CHECK( dummy.remove_effect( effect_grabbed, left_arm ) );
            // Left arm is released
            AND_THEN( "effect is removed from that body part" ) {
                CHECK_FALSE( dummy.has_effect( effect_grabbed, left_arm ) );
            }
            // Right arm still grabbed
            AND_THEN( "effect still applies to other body part" ) {
                CHECK( dummy.has_effect( effect_grabbed, right_arm ) );
            }
        }

        // Release (nonexistent) grabs on legs
        THEN( "remove_effect with an unaffected body part returns false" ) {
            CHECK_FALSE( dummy.remove_effect( effect_grabbed, left_leg ) );
            CHECK_FALSE( dummy.remove_effect( effect_grabbed, right_leg ) );
            // Both arms still grabbed
            AND_THEN( "effect still applies to original body parts" ) {
                CHECK( dummy.has_effect( effect_grabbed, left_arm ) );
                CHECK( dummy.has_effect( effect_grabbed, right_arm ) );
            }
        }
    }
}

// Creature::has_effect
//
TEST_CASE( "has_effect", "[creature][effect][has]" )
{
    const efftype_id effect_downed( "downed" );
    const efftype_id effect_grabbed( "grabbed" );
    const efftype_id effect_invisibility( "invisibility" );

    // For monster, has_effect is not body-part-specific (uses num_bp)
    SECTION( "monster has_effect" ) {
        monster mummy( mtype_id( "debug_mon" ) );

        mummy.clear_effects();

        // Not downed or grabbed
        WHEN( "monster does not have effect" ) {

            THEN( "has_effect is false" ) {
                CHECK_FALSE( mummy.has_effect( effect_downed ) );
                CHECK_FALSE( mummy.has_effect( effect_grabbed ) );
            }
        }

        // Downed but not grabbed
        WHEN( "monster has one effect" ) {
            mummy.add_effect( effect_downed, 1_minutes );

            THEN( "has_effect is true for that effect" ) {
                CHECK( mummy.has_effect( effect_downed ) );
            }
            THEN( "has_effect is false for other effect" ) {
                CHECK_FALSE( mummy.has_effect( effect_grabbed ) );
            }
        }

        // Both downed and grabbed
        WHEN( "monster has two effects" ) {
            mummy.add_effect( effect_downed, 1_minutes );
            mummy.add_effect( effect_grabbed, 1_minutes );

            THEN( "has_effect is true for both effects" ) {
                CHECK( mummy.has_effect( effect_downed ) );
                CHECK( mummy.has_effect( effect_grabbed ) );
            }
        }
    }

    // For character, has_effect may apply to individual body parts
    SECTION( "character has_effect" ) {
        avatar dummy;
        const body_part left_arm = bodypart_id( "arm_l" )->token;
        const body_part right_arm = bodypart_id( "arm_r" )->token;

        dummy.clear_effects();

        // Not downed or grabbed
        WHEN( "character does not have effect" ) {
            THEN( "has_effect is false" ) {
                CHECK_FALSE( dummy.has_effect( effect_downed ) );
                CHECK_FALSE( dummy.has_effect( effect_grabbed ) );
            }
        }

        // Left arm grabbed
        WHEN( "character has effect on one body part" ) {
            dummy.add_effect( effect_grabbed, 1_minutes, left_arm );

            THEN( "has_effect is true for affected body part" ) {
                CHECK( dummy.has_effect( effect_grabbed, left_arm ) );
            }
            THEN( "has_effect is false for an unaffected body part" ) {
                CHECK_FALSE( dummy.has_effect( effect_grabbed, right_arm ) );
            }
            THEN( "has_effect is true when body part is not specified" ) {
                // num_bp (default) is any/all body parts
                CHECK( dummy.has_effect( effect_grabbed ) );
                CHECK( dummy.has_effect( effect_grabbed, num_bp ) );
            }

        }

        // Downed
        WHEN( "character has effect on the whole body" ) {
            dummy.add_effect( effect_downed, 1_minutes, num_bp );

            THEN( "has_effect is false for any body part" ) {
                CHECK_FALSE( dummy.has_effect( effect_downed, left_arm ) );
                CHECK_FALSE( dummy.has_effect( effect_downed, right_arm ) );
            }
            THEN( "has_effect is true when body part is not specified" ) {
                CHECK( dummy.has_effect( effect_downed ) );
                CHECK( dummy.has_effect( effect_downed, num_bp ) );
            }
        }
    }
}

// Creature::has_effect_with_flag
//
TEST_CASE( "has_effect_with_flag", "[creature][effect][has][flag]" )
{
    const efftype_id effect_downed( "downed" );
    const efftype_id effect_invisibility( "invisibility" );
    const std::string invisibility_flag = "EFFECT_INVISIBLE";

    monster mummy( mtype_id( "debug_mon" ) );

    mummy.clear_effects();

    WHEN( "monster does not have any effects" ) {

        THEN( "has_effect_with_flag is false" ) {
            CHECK_FALSE( mummy.has_effect_with_flag( invisibility_flag ) );
        }
    }

    WHEN( "monster has only effects without flag" ) {
        mummy.add_effect( effect_downed, 1_minutes );

        THEN( "has_effect_with_flag is false" ) {
            CHECK_FALSE( mummy.has_effect_with_flag( invisibility_flag ) );
        }
    }

    WHEN( "monster has an effect with the flag" ) {
        mummy.add_effect( effect_invisibility, 1_minutes );

        THEN( "has_effect_with_flag is true" ) {
            CHECK( mummy.has_effect_with_flag( invisibility_flag ) );
        }
    }
}

// monster::is_immune_effect
//
TEST_CASE( "monster is_immune_effect", "[creature][monster][effect][immune]" )
{
    const efftype_id effect_bleed( "bleed" );
    const efftype_id effect_poison( "poison" );
    const efftype_id effect_badpoison( "badpoison" );
    const efftype_id effect_paralyzepoison( "paralyzepoison" );

    // TODO: Monster may be immune to:
    // - onfire (if is_immune_damage DT_HEAT, made_of LIQUID, has_flag MF_FIREY)
    // - stunned (if has_flag MF_STUN_IMMUNE)

    WHEN( "monster is made of flesh and is warm-blooded" ) {
        // Regular zombie - fleshy and warm
        monster zed( mtype_id( "mon_zombie" ) );
        zed.clear_effects();
        REQUIRE( zed.made_of( material_id( "flesh" ) ) );
        REQUIRE( zed.has_flag( MF_WARM ) );

        THEN( "they can bleed" ) {
            CHECK_FALSE( zed.is_immune_effect( effect_bleed ) );
        }
        THEN( "they can be poisoned" ) {
            CHECK_FALSE( zed.is_immune_effect( effect_bleed ) );
            CHECK_FALSE( zed.is_immune_effect( effect_poison ) );
            CHECK_FALSE( zed.is_immune_effect( effect_badpoison ) );
            CHECK_FALSE( zed.is_immune_effect( effect_paralyzepoison ) );
        }
    }

    WHEN( "monster is not not made of flesh and not warm-blooded" ) {
        // Skeleton - no flesh, not warm-blooded
        monster skelly( mtype_id( "mon_skeleton" ) );
        skelly.clear_effects();
        REQUIRE_FALSE( skelly.made_of( material_id( "flesh" ) ) );
        REQUIRE_FALSE( skelly.made_of( material_id( "iflesh" ) ) );
        REQUIRE_FALSE( skelly.has_flag( MF_WARM ) );

        THEN( "they are immune to the bleed effect" ) {
            CHECK( skelly.is_immune_effect( effect_bleed ) );
        }

        THEN( "they are immune to all poison effects" ) {
            CHECK( skelly.is_immune_effect( effect_bleed ) );
            CHECK( skelly.is_immune_effect( effect_poison ) );
            CHECK( skelly.is_immune_effect( effect_badpoison ) );
            CHECK( skelly.is_immune_effect( effect_paralyzepoison ) );
        }
    }

    WHEN( "monster is made of flesh, but not warm-blooded" ) {
        // Razorclaw - fleshy, with arthropod blood
        monster razorclaw( mtype_id( "mon_razorclaw" ) );
        razorclaw.clear_effects();
        REQUIRE( razorclaw.made_of( material_id( "flesh" ) ) );
        REQUIRE_FALSE( razorclaw.has_flag( MF_WARM ) );

        THEN( "they are immune to the bleed effect" ) {
            CHECK( razorclaw.is_immune_effect( effect_bleed ) );
        }

        THEN( "they are immune to all poison effects" ) {
            CHECK( razorclaw.is_immune_effect( effect_bleed ) );
            CHECK( razorclaw.is_immune_effect( effect_poison ) );
            CHECK( razorclaw.is_immune_effect( effect_badpoison ) );
            CHECK( razorclaw.is_immune_effect( effect_paralyzepoison ) );
        }
    }
}

// Character::is_immune_effect
//
TEST_CASE( "character is_immune_effect", "[creature][character][effect][immune]" )
{
    avatar dummy;
    dummy.clear_mutations();

    // TODO: Character may be immune to:
    // - onfire (if immune_damage DT_HEAT)
    // - deaf (if wearing something with DEAF/PARTIALDEAF, bionic ears, rm13 armor)

    WHEN( "character has Slimy mutation" ) {
        const trait_id trait_slimy( "SLIMY" );
        const efftype_id effect_corroding( "corroding" );

        dummy.toggle_trait( trait_slimy );
        REQUIRE( dummy.has_trait( trait_slimy ) );

        THEN( "they are immune to the corroding effect" ) {
            CHECK( dummy.is_immune_effect( effect_corroding ) );
        }

        AND_WHEN( "they lose their Slimy mutation" ) {
            dummy.toggle_trait( trait_slimy );
            REQUIRE_FALSE( dummy.has_trait( trait_slimy ) );

            THEN( "they are no longer immune to the corroding effect" ) {
                CHECK_FALSE( dummy.is_immune_effect( effect_corroding ) );
            }
        }
    }

    WHEN( "character has Tentacle bracing mutation" ) {
        const trait_id trait_tentacle_brace( "LEG_TENT_BRACE" );
        const efftype_id effect_downed( "downed" );

        dummy.toggle_trait( trait_tentacle_brace );
        REQUIRE( dummy.has_trait( trait_tentacle_brace ) );

        THEN( "they are immune to the downed effect" ) {
            CHECK( dummy.is_immune_effect( effect_downed ) );
        }

        AND_WHEN( "they lose their Tentacle bracing mutation" ) {
            dummy.toggle_trait( trait_tentacle_brace );
            REQUIRE_FALSE( dummy.has_trait( trait_tentacle_brace ) );

            THEN( "they are no longer immune to the downed effect" ) {
                CHECK_FALSE( dummy.is_immune_effect( effect_downed ) );
            }
        }
    }

    WHEN( "character has Strong Stomach mutation" ) {
        const trait_id trait_strong_stomach( "STRONGSTOMACH" );
        const efftype_id effect_nausea( "nausea" );

        dummy.toggle_trait( trait_strong_stomach );
        REQUIRE( dummy.has_trait( trait_strong_stomach ) );

        THEN( "they are immune to the nausea effect" ) {
            CHECK( dummy.is_immune_effect( effect_nausea ) );
        }

        AND_WHEN( "they lose their Strong Stomach mutation" ) {
            dummy.toggle_trait( trait_strong_stomach );
            REQUIRE_FALSE( dummy.has_trait( trait_strong_stomach ) );

            THEN( "they are no longer immune to the nausea effect" ) {
                CHECK_FALSE( dummy.is_immune_effect( effect_nausea ) );
            }
        }
    }
}

// Creature::resists_effect
//
TEST_CASE( "creature effect reistance", "[creature][effect][resist]" )
{
    // TODO: Creature resists effect if:
    // - has effect from eff.get_resist_effects
    // - has trait from eff.get_resist_traits
}

