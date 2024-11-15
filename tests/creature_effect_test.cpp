#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "monster.h"
#include "mtype.h"
#include "type_id.h"

static const efftype_id effect_badpoison( "badpoison" );
static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_corroding( "corroding" );
static const efftype_id effect_downed( "downed" );
static const efftype_id effect_grabbed( "grabbed" );
static const efftype_id effect_invisibility( "invisibility" );
static const efftype_id effect_paralyzepoison( "paralyzepoison" );
static const efftype_id effect_poison( "poison" );
static const efftype_id effect_venom_dmg( "venom_dmg" );
static const efftype_id effect_venom_player1( "venom_player1" );
static const efftype_id effect_venom_player2( "venom_player2" );
static const efftype_id effect_venom_weaken( "venom_weaken" );

static const flag_id json_flag_INVISIBLE( "INVISIBLE" );

static const mtype_id mon_flaming_eye( "mon_flaming_eye" );
static const mtype_id mon_fungaloid( "mon_fungaloid" );
static const mtype_id mon_graboid( "mon_graboid" );
static const mtype_id mon_hallu_mom( "mon_hallu_mom" );
static const mtype_id mon_razorclaw( "mon_razorclaw" );
static const mtype_id mon_zombie( "mon_zombie" );

static const species_id species_FUNGUS( "FUNGUS" );
static const species_id species_NETHER( "NETHER" );
static const species_id species_WORM( "WORM" );
static const species_id species_ZOMBIE( "ZOMBIE" );

static const trait_id trait_LEG_TENT_BRACE( "LEG_TENT_BRACE" );
static const trait_id trait_SLIMY( "SLIMY" );

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

// Characters have effects on separate body parts, or no particular part (indicated by `bp_null`)
TEST_CASE( "character_add_effect", "[creature][character][effect][add]" )
{
    avatar dummy;
    dummy.set_body();
    const bodypart_id left_arm( "arm_l" );
    const bodypart_id right_arm( "arm_r" );

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
                CHECK( dummy.has_effect( effect_bleed, left_arm.id() ) );
                CHECK( dummy.has_effect( effect_grabbed, right_arm.id() ) );
            }

            // Left arm not grabbed, right arm not bleeding
            THEN( "they do not have the effect on another body part" ) {
                CHECK_FALSE( dummy.has_effect( effect_grabbed, left_arm.id() ) );
                CHECK_FALSE( dummy.has_effect( effect_bleed, right_arm.id() ) );
            }
        }
    }
}

// Monsters may have effects added to them, but they don't have separate body parts.
TEST_CASE( "monster_add_effect", "[creature][monster][effect][add]" )
{
    monster mummy( mon_hallu_mom );

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

    // "Your mother" monster is of 'HALLUCINATION' species, which doesn't have any bleeding type set for it, so it is immune to bleeding.
    GIVEN( "monster is immune to effect" ) {
        REQUIRE( mummy.is_immune_effect( effect_bleed ) );

        THEN( "monster add_effect is called with force = false" ) {
            mummy.add_effect( effect_bleed, 1_minutes, false, 1, false );
            THEN( "they do not have the effect" ) {
                CHECK_FALSE( mummy.has_effect( effect_bleed ) );
            }
        }

        WHEN( "monster add_effect is called with force = true" ) {
            mummy.add_effect( effect_bleed, 1_minutes, false, 1, true );
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
    const bodypart_id left_arm( "arm_l" );
    const bodypart_id right_arm( "arm_r" );
    const bodypart_id left_leg( "leg_l" );
    const bodypart_id right_leg( "leg_r" );

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
        REQUIRE( dummy.has_effect( effect_grabbed, left_arm.id() ) );

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
                CHECK_FALSE( dummy.has_effect( effect_grabbed, left_arm.id() ) );
            }
        }

        THEN( "remove_effect with an unaffected body part returns false" ) {
            // Release (nonexistent) grab on right arm
            CHECK_FALSE( dummy.remove_effect( effect_grabbed, right_arm ) );
            // Left arm is still grabbed
            AND_THEN( "effect still applies to original body part" ) {
                CHECK( dummy.has_effect( effect_grabbed, left_arm.id() ) );
            }
        }
    }

    // Both arms grabbed
    WHEN( "character has effect on two body parts" ) {
        dummy.add_effect( effect_grabbed, 1_minutes, left_arm );
        dummy.add_effect( effect_grabbed, 1_minutes, right_arm );
        REQUIRE( dummy.has_effect( effect_grabbed, left_arm.id() ) );
        REQUIRE( dummy.has_effect( effect_grabbed, right_arm.id() ) );

        // Release all grabs
        THEN( "remove_effect with no body part returns true" ) {
            CHECK( dummy.remove_effect( effect_grabbed ) );
            // Both arms are released
            AND_THEN( "effect is removed from all body parts" ) {
                CHECK_FALSE( dummy.has_effect( effect_grabbed, left_arm.id() ) );
                CHECK_FALSE( dummy.has_effect( effect_grabbed, right_arm.id() ) );
            }
        }

        // Only release left arm
        THEN( "remove_effect with an affected body part returns true" ) {
            CHECK( dummy.remove_effect( effect_grabbed, left_arm ) );
            // Left arm is released
            AND_THEN( "effect is removed from that body part" ) {
                CHECK_FALSE( dummy.has_effect( effect_grabbed, left_arm.id() ) );
            }
            // Right arm still grabbed
            AND_THEN( "effect still applies to other body part" ) {
                CHECK( dummy.has_effect( effect_grabbed, right_arm.id() ) );
            }
        }

        // Release (nonexistent) grabs on legs
        THEN( "remove_effect with an unaffected body part returns false" ) {
            CHECK_FALSE( dummy.remove_effect( effect_grabbed, left_leg ) );
            CHECK_FALSE( dummy.remove_effect( effect_grabbed, right_leg ) );
            // Both arms still grabbed
            AND_THEN( "effect still applies to original body parts" ) {
                CHECK( dummy.has_effect( effect_grabbed, left_arm.id() ) );
                CHECK( dummy.has_effect( effect_grabbed, right_arm.id() ) );
            }
        }
    }
}

// Creature::has_effect
//
TEST_CASE( "has_effect", "[creature][effect][has]" )
{

    // For monster, has_effect is not body-part-specific (uses bp_null)
    SECTION( "monster has_effect" ) {
        monster mummy( mon_hallu_mom );

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
        const bodypart_id left_arm( "arm_l" );
        const bodypart_id right_arm( "arm_r" );

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
                CHECK( dummy.has_effect( effect_grabbed, left_arm.id() ) );
            }
            THEN( "has_effect is false for an unaffected body part" ) {
                CHECK_FALSE( dummy.has_effect( effect_grabbed, right_arm.id() ) );
            }
            THEN( "has_effect is true when body part is not specified" ) {
                CHECK( dummy.has_effect( effect_grabbed ) );
                CHECK( dummy.has_effect( effect_grabbed ) );
            }

        }

        // Downed
        WHEN( "character has effect on the whole body" ) {
            dummy.add_effect( effect_downed, 1_minutes );

            THEN( "has_effect is false for any body part" ) {
                CHECK_FALSE( dummy.has_effect( effect_downed, left_arm.id() ) );
                CHECK_FALSE( dummy.has_effect( effect_downed, right_arm.id() ) );
            }
            THEN( "has_effect is true when body part is not specified" ) {
                CHECK( dummy.has_effect( effect_downed ) );
                CHECK( dummy.has_effect( effect_downed ) );
            }
        }
    }
}

// Creature::has_effect_with_flag
//
TEST_CASE( "has_effect_with_flag", "[creature][effect][has][flag]" )
{
    monster mummy( mon_hallu_mom );

    mummy.clear_effects();

    WHEN( "monster does not have any effects" ) {

        THEN( "has_effect_with_flag is false" ) {
            CHECK_FALSE( mummy.has_effect_with_flag( json_flag_INVISIBLE ) );
        }
    }

    WHEN( "monster has only effects without flag" ) {
        mummy.add_effect( effect_downed, 1_minutes );

        THEN( "has_effect_with_flag is false" ) {
            CHECK_FALSE( mummy.has_effect_with_flag( json_flag_INVISIBLE ) );
        }
    }

    WHEN( "monster has an effect with the flag" ) {
        mummy.add_effect( effect_invisibility, 1_minutes );

        THEN( "has_effect_with_flag is true" ) {
            CHECK( mummy.has_effect_with_flag( json_flag_INVISIBLE ) );
        }
    }
}

// monster::is_immune_effect
//
TEST_CASE( "monster_is_immune_effect", "[creature][monster][effect][immune]" )
{
    // TODO: Monster may be immune to:
    // - onfire (if is_immune_damage DT_HEAT, made_of LIQUID, has_flag MF_FIREY)
    // - stunned (if has_flag MF_STUN_IMMUNE)

    WHEN( "monster is made of flesh, is not zombified, has blood and has no legs" ) {
        // graboid - fleshy, living snake of a species with bleed data
        monster graboid( mon_graboid );
        graboid.clear_effects();
        REQUIRE( graboid.made_of_any( Creature::cmat_flesh ) );
        REQUIRE( graboid.type->bodytype == "snake" );
        REQUIRE( graboid.type->in_species( species_WORM ) );

        THEN( "they can bleed" ) {
            CHECK_FALSE( graboid.is_immune_effect( effect_bleed ) );
        }

        THEN( "they can be poisoned by all poisons" ) {
            CHECK_FALSE( graboid.is_immune_effect( effect_poison ) );
            CHECK_FALSE( graboid.is_immune_effect( effect_badpoison ) );
            CHECK_FALSE( graboid.is_immune_effect( effect_paralyzepoison ) );
            CHECK_FALSE( graboid.is_immune_effect( effect_venom_dmg ) );
            CHECK_FALSE( graboid.is_immune_effect( effect_venom_player1 ) );
            CHECK_FALSE( graboid.is_immune_effect( effect_venom_player2 ) );
            CHECK_FALSE( graboid.is_immune_effect( effect_venom_weaken ) );
        }

        THEN( "they can't be downed" ) {
            CHECK( graboid.is_immune_effect( effect_downed ) );
        }
    }

    WHEN( "monster is a zombie, made of flesh, has blood and has legs" ) {
        // Zombie - fleshy humanoid zombie
        monster zed( mon_zombie );
        zed.clear_effects();
        REQUIRE( zed.made_of_any( Creature::cmat_flesh ) );
        REQUIRE( zed.type->in_species( species_ZOMBIE ) );
        REQUIRE( zed.type->bodytype == "human" );

        THEN( "they can bleed" ) {
            CHECK_FALSE( zed.is_immune_effect( effect_bleed ) );
        }

        THEN( "they can be poisoned by stronger poisons" ) {
            CHECK_FALSE( zed.is_immune_effect( effect_venom_dmg ) );
            CHECK_FALSE( zed.is_immune_effect( effect_venom_player1 ) );
            CHECK_FALSE( zed.is_immune_effect( effect_venom_player2 ) );
        }

        THEN( "they can't be poisoned by weaker poisons" ) {
            CHECK( zed.is_immune_effect( effect_poison ) );
            CHECK( zed.is_immune_effect( effect_badpoison ) );
            CHECK( zed.is_immune_effect( effect_paralyzepoison ) );
            CHECK( zed.is_immune_effect( effect_venom_weaken ) );
        }

        THEN( "they can be downed" ) {
            CHECK_FALSE( zed.is_immune_effect( effect_downed ) );
        }
    }

    WHEN( "monster is made of nether flesh and is flying" ) {
        // Flaming eye, flesh, Nether species and flying
        monster feye( mon_flaming_eye );
        feye.clear_effects();
        REQUIRE( feye.made_of_any( Creature::cmat_flesh ) );
        REQUIRE( feye.has_flag( mon_flag_FLIES ) );
        REQUIRE( feye.type->in_species( species_NETHER ) );

        THEN( "they can bleed" ) {
            CHECK_FALSE( feye.is_immune_effect( effect_bleed ) );
        }

        THEN( "they can't be poisoned" ) {
            CHECK( feye.is_immune_effect( effect_poison ) );
            CHECK( feye.is_immune_effect( effect_badpoison ) );
            CHECK( feye.is_immune_effect( effect_paralyzepoison ) );
            CHECK( feye.is_immune_effect( effect_venom_dmg ) );
            CHECK( feye.is_immune_effect( effect_venom_player1 ) );
            CHECK( feye.is_immune_effect( effect_venom_player2 ) );
            CHECK( feye.is_immune_effect( effect_venom_weaken ) );
        }

        THEN( "they can't be downed" ) {
            CHECK( feye.is_immune_effect( effect_downed ) );
        }
    }

    WHEN( "monster is not made of flesh or iflesh" ) {
        // Fungaloid - veggy, has no blood or bodytype
        monster fungaloid( mon_fungaloid );
        fungaloid.clear_effects();
        REQUIRE_FALSE( fungaloid.made_of_any( Creature::cmat_flesh ) );
        REQUIRE( fungaloid.type->in_species( species_FUNGUS ) );
        REQUIRE( fungaloid.type->bleed_rate == 0 );

        THEN( "their zero bleed rate makes them immune to bleed" ) {
            CHECK( fungaloid.is_immune_effect( effect_bleed ) );
        }

        THEN( "they can't be poisoned" ) {
            CHECK( fungaloid.is_immune_effect( effect_poison ) );
            CHECK( fungaloid.is_immune_effect( effect_badpoison ) );
            CHECK( fungaloid.is_immune_effect( effect_paralyzepoison ) );
            CHECK( fungaloid.is_immune_effect( effect_venom_dmg ) );
            CHECK( fungaloid.is_immune_effect( effect_venom_player1 ) );
            CHECK( fungaloid.is_immune_effect( effect_venom_player2 ) );
            CHECK( fungaloid.is_immune_effect( effect_venom_weaken ) );
        }

        THEN( "they can be downed" ) {
            CHECK_FALSE( fungaloid.is_immune_effect( effect_downed ) );
        }
    }

    WHEN( "monster species doesn't have bleeding type set, but it has flag describing its bleeding type" ) {
        // Razorclaw - has `MUTANT` species which doesn't have any bleeding type set, but has arthropod blood
        monster razorclaw( mon_razorclaw );
        razorclaw.clear_effects();
        REQUIRE( razorclaw.has_flag( mon_flag_ARTHROPOD_BLOOD ) );

        THEN( "they can bleed" ) {
            CHECK_FALSE( razorclaw.is_immune_effect( effect_bleed ) );
        }
    }
}

// Character::is_immune_effect
//
TEST_CASE( "character_is_immune_effect", "[creature][character][effect][immune]" )
{
    avatar dummy;
    dummy.set_body();
    dummy.clear_mutations();

    // TODO: Character may be immune to:
    // - onfire (if immune_damage DT_HEAT)
    // - deaf (if wearing something with DEAF/PARTIALDEAF, bionic ears, rm13 armor)

    WHEN( "character has Slimy mutation" ) {
        dummy.toggle_trait( trait_SLIMY );
        REQUIRE( dummy.has_trait( trait_SLIMY ) );

        THEN( "they are immune to the corroding effect" ) {
            CHECK( dummy.is_immune_effect( effect_corroding ) );
        }

        AND_WHEN( "they lose their Slimy mutation" ) {
            dummy.toggle_trait( trait_SLIMY );
            REQUIRE_FALSE( dummy.has_trait( trait_SLIMY ) );

            THEN( "they are no longer immune to the corroding effect" ) {
                CHECK_FALSE( dummy.is_immune_effect( effect_corroding ) );
            }
        }
    }

    WHEN( "character has Tentacle bracing mutation" ) {
        dummy.toggle_trait( trait_LEG_TENT_BRACE );
        REQUIRE( dummy.has_trait( trait_LEG_TENT_BRACE ) );

        THEN( "they are immune to the downed effect" ) {
            CHECK( dummy.is_immune_effect( effect_downed ) );
        }

        AND_WHEN( "they lose their Tentacle bracing mutation" ) {
            dummy.toggle_trait( trait_LEG_TENT_BRACE );
            REQUIRE_FALSE( dummy.has_trait( trait_LEG_TENT_BRACE ) );

            THEN( "they are no longer immune to the downed effect" ) {
                CHECK_FALSE( dummy.is_immune_effect( effect_downed ) );
            }
        }
    }
}

// Creature::resists_effect
//
TEST_CASE( "creature_effect_reistance", "[creature][effect][resist]" )
{
    // TODO: Creature resists effect if:
    // - has effect from eff.get_resist_effects
    // - has trait from eff.get_resist_traits
}

