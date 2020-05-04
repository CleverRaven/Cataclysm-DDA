#include "avatar.h"
#include "effect.h"
#include "game.h"
#include "player.h"

#include "catch/catch.hpp"
#include "player_helpers.h"
#include "map_helpers.h"

/* Test effects used in player_hardcoded_effects
- effect_alarm_clock
- effect_antibiotic
- effect_asthma
- effect_attention
- effect_bite
- effect_bleed
- effect_blind
- effect_bloodworms
- effect_boomered
- effect_brainworms
- effect_cold
- effect_datura
- effect_dermatik
- effect_disabled
- effect_downed
- effect_evil
- effect_formication
- effect_frostbite
- effect_fungus
- effect_grabbed
- effect_grabbing
- effect_hallu
- effect_hot
- effect_infected
- effect_lying_down
- effect_mending
- effect_meth
- effect_motor_seizure
- effect_narcosis
- effect_onfire
- effect_paincysts
- effect_panacea
- effect_rat
- effect_recover
- effect_shakes
- effect_sleep
- effect_slept_through_alarm
- effect_spores
- effect_strong_antibiotic
- effect_stunned
- effect_tapeworm
- effect_teleglow
- effect_tetanus
- effect_toxin_buildup
- effect_valium
- effect_visuals
- effect_weak_antibiotic
*/

// Functions to cover:
// - player::hardcoded_effects
// - player::process_effects
// - player::process_one_effect


// effect_mending
//
// Applied to a body part, this effect indicates when a limb is mending from being broken.
// Mending itself occurs in Character::mend (outside the scope of these tests).
//
// FIXME: This test (and the effect_disabled test) are kind of pointless and over-complicated,
// considering their simplistic implementation. Consider removing these (maybe more maintenance than
// they are worth).
//
TEST_CASE( "mending broken limbs", "[player][effect][mending]" )
{
    avatar dummy;
    const efftype_id effect_mending( "mending" );
    const body_part right_leg = bodypart_id( "leg_r" )->token;

    dummy.clear_effects();

    GIVEN( "player has a broken and mending body part" ) {
        // Break a leg, and make it "permanently" mending
        dummy.apply_damage( nullptr, bodypart_id( "leg_r" ), 999 );
        dummy.add_effect( effect_mending, 1_turns, right_leg, true );

        // Keep the effect object for hardcoded_effects
        effect &effect_obj = dummy.get_effect( effect_mending, right_leg );

        // Make sure setup worked as expected
        REQUIRE( dummy.has_effect( effect_mending, right_leg ) );
        REQUIRE( effect_obj.is_permanent() );

        WHEN( "it is still broken" ) {
            REQUIRE( dummy.is_limb_broken( hp_leg_r ) );

            WHEN( "hardcoded effects apply" ) {
                dummy.hardcoded_effects( effect_obj );

                THEN( "mending effect duration > 0" ) {
                    CHECK( dummy.has_effect( effect_mending, right_leg ) );
                    CHECK( to_turns<int>( effect_obj.get_duration() ) > 0 );
                }
            }
        }

        WHEN( "it is no longer broken" ) {
            // "Un-break" the leg, to avoid the complexity of Character::mend
            dummy.hp_cur[hp_leg_r] = 1;
            REQUIRE_FALSE( dummy.is_limb_broken( hp_leg_r ) );

            WHEN( "hardcoded effects apply" ) {
                dummy.hardcoded_effects( effect_obj );

                THEN( "mending effect duration is 0" ) {
                    CHECK( dummy.has_effect( effect_mending, right_leg ) );
                    CHECK( to_turns<int>( effect_obj.get_duration() ) == 0 );
                }
            }
        }
    }
}

// effect_disabled
//
TEST_CASE( "disabled body parts", "[player][effect][disabled]" )
{
    avatar dummy;
    const efftype_id effect_disabled( "disabled" );
    const body_part right_leg = bodypart_id( "leg_r" )->token;

    dummy.clear_effects();

    GIVEN( "player has a broken and disabled body part" ) {
        // Break a leg, and make it "permanently" disabled
        dummy.apply_damage( nullptr, bodypart_id( "leg_r" ), 999 );
        dummy.add_effect( effect_disabled, 1_turns, right_leg, true );

        // Keep the effect object for hardcoded_effects
        effect &effect_obj = dummy.get_effect( effect_disabled, right_leg );

        // Make sure setup worked as expected
        REQUIRE( dummy.has_effect( effect_disabled, right_leg ) );
        REQUIRE( effect_obj.is_permanent() );

        WHEN( "it is still broken" ) {
            REQUIRE( dummy.is_limb_broken( hp_leg_r ) );

            WHEN( "hardcoded effects apply" ) {
                dummy.hardcoded_effects( effect_obj );

                THEN( "it remains permanently disabled" ) {
                    CHECK( dummy.has_effect( effect_disabled, right_leg ) );
                    CHECK( effect_obj.is_permanent() );
                }
            }
        }

        WHEN( "it is no longer broken" ) {
            // "Un-break" the leg, to avoid the complexity of Character::mend
            dummy.hp_cur[hp_leg_r] = 1;
            REQUIRE_FALSE( dummy.is_limb_broken( hp_leg_r ) );

            WHEN( "hardcoded effects apply" ) {
                dummy.hardcoded_effects( effect_obj );

                THEN( "it remains disabled but only temporarily" ) {
                    CHECK( dummy.has_effect( effect_disabled, right_leg ) );
                    CHECK_FALSE( effect_obj.is_permanent() );
                }
            }
        }
    }
}

// effect_panacea
//
TEST_CASE( "panacea heals the body and reduces pain", "[player][effect][panacea]" )
{
    // GIVEN player has the panacea effect
    // THEN all body parts are healed
    // AND pain is reduced
}

// effect_alarm_clock
//
// IF in_sleep_state
// Bionic watch
// - Not asleep: "Your internal chronometer went off and you haven't slept a wink."
// - Sleeping: random based on alarm volume and sleepiness traits
// -- "Your internal chronometer finally wakes you up." (slept_through)
// -- "Your internal chronometer wakes you up."
// - Otherwise, add effect_slept_through_alarm
// No bionic watch:
// - If still asleep, add effect_slept_through_alarm
// - "beep-beep-beep!" (and let the sound code handle the wake-up part??)
// ELSE (not in_sleep_state)
// - "beep-beep-beep!"
// - "Your alarm is going off."
// - (ignored) "Your alarm went off."


/* Recovery chances, use binomial distributions if balancing here. Healing in the bite
 * stage provides additional benefits, so both the bite stage chance of healing and
 * the cumulative chances for spontaneous healing are both given.
 * Cumulative heal chances for the bite + infection stages:
 * -200 health - 38.6%
 *    0 health - 46.8%
 *  200 health - 53.7%
 *
 * Heal chances in the bite stage:
 * -200 health - 23.4%
 *    0 health - 28.3%
 *  200 health - 32.9%
 *
 * Cumulative heal chances the bite + infection stages with the resistant mutation:
 * -200 health - 82.6%
 *    0 health - 84.5%
 *  200 health - 86.1%
 *
 * Heal chances in the bite stage with the resistant mutation:
 * -200 health - 60.7%
 *    0 health - 63.2%
 *  200 health - 65.6%
 */

// effect_bite
//
// Every 10 turns, check for recovery:
// - Base factor: 100
// - If effect_recover, REDUCE recovery factor by effect_recover duration / 1 hour (??)
// - Trait INFRESIST +200
// stacking with one of:
// - panacea to MAX (cannot fail)
// - strong_antibiotic +400
// - antibiotic +200
// - weak_antibiotic +100
// Stacking with
// - ++get_healthy/10
//
// If factor in 648,000, recover!
//
// If not recovered:
// - After 6 hours, infected
// Else, get one of:
// - strong_antibiotic effect -1 turn
// - antibiotic effect: +1 turn every 8 hours
// - weak_antibiotic effect: +1 turn every 2 turns
// - Otherwise: mod_duration +1 turn
//
// effect_bite usage notes
// REFACTOR duplicate bite logic (40m, 25m, 1t)
// - monattack.cpp stretch_bite (L4095-4103)
// - mattack_actors.cpp on_damage (L360-368)
// - character.cpp deal_damage (L8542-8550) "each point has a 1% chance of causing infection"

static float bite_recovery_chance( const time_duration time_to_recover,
                                   const std::string with_effect_name )
{
    // Total number of bite-to-infection trials to run
    const int num_trials = 1000;
    // How many times we recover
    int recovery_count = 0;

    const efftype_id effect_bite( "bite" );
    const efftype_id effect_also( with_effect_name );
    const body_part torso = bodypart_id( "torso" )->token;

    // Start bites with enough duration to give the desired recovery time (up to 6 full hours)
    const time_duration start_bite_duration = 6_hours - time_to_recover + 1_turns;
    REQUIRE( to_turns<int>( start_bite_duration ) > 0 );

    avatar dummy;

    for( int i = 0; i < num_trials; i++ ) {
        dummy.clear_effects();
        calendar::turn = calendar::turn_zero;
        // Get a fresh bite with the given starting duration
        dummy.add_effect( effect_bite, start_bite_duration, torso, true );

        // Apply the additional effect
        dummy.add_effect( effect_also, 12_hours, torso );

        // Keep track of the bite effect to know whether it recovers
        effect &bite_obj = dummy.get_effect( efftype_id( "bite" ), torso );
        bool recovered = false;

        // Without recovery, infection is automatic at 6 hours; quit before then
        while( !recovered && bite_obj.get_duration() < 6_hours ) {
            dummy.hardcoded_effects( bite_obj );
            calendar::turn = calendar::turn + 1_turns;

            // If duration reaches 0 turns, the bite has recovered
            if( bite_obj.get_duration() == 0_turns ) {
                recovered = true;
                recovery_count++;
            }
        }
    }

    // Return percentage of times healed within the six-hour infection limit
    return static_cast<float>( 100.0f * recovery_count / num_trials );
}

TEST_CASE( "chance of recovering from bite", "[player][effect][bite][recovery][slow][!mayfail]" )
{
    // FIXME: This test with panacea is boring and a waste of cycles
    //CHECK( 100.0f == bite_recovery_chance( "panacea" ) );

    // Antibiotics work better with more time to recover (6 hours max)

    CHECK( 100.0f == bite_recovery_chance( 1_hours, "strong_antibiotic" ) );

    // FIXME: These are flakier than golden-crusted butter croissants
    const float marg = 2.0f;
    CHECK( Approx( 100.0f ).margin( marg ) == bite_recovery_chance( 6_hours, "antibiotic" ) );
    CHECK( Approx( 98.3f ).margin( marg ) == bite_recovery_chance( 3_hours, "antibiotic" ) );
    CHECK( Approx( 73.3f ).margin( marg ) == bite_recovery_chance( 1_hours, "antibiotic" ) );

    CHECK( Approx( 73.4f ).margin( marg ) == bite_recovery_chance( 6_hours, "weak_antibiotic" ) );
    CHECK( Approx( 49.1f ).margin( marg ) == bite_recovery_chance( 3_hours, "weak_antibiotic" ) );
    CHECK( Approx( 19.8f ).margin( marg ) == bite_recovery_chance( 1_hours, "weak_antibiotic" ) );

    CHECK( Approx( 27.9f ).margin( marg ) == bite_recovery_chance( 6_hours, "recover" ) );
    CHECK( Approx( 15.6f ).margin( marg ) == bite_recovery_chance( 3_hours, "recover" ) );
    CHECK( Approx( 5.3f ).margin( marg ) == bite_recovery_chance( 1_hours, "recover" ) );
}


// Bite effects are "permanent" until they either recover or become infected. When a bite is first
// applied to a body part, it has a nonzero starting duration. In player::hardcoded_effects, that
// bite duration may be modified up or down, depending on several factors.
//
// With no treatment, duration increases with every tick of the clock; if it exceeds six hours
// without recovering, the bite becomes infected.
//
// The chance of recovery is randomized, influenced by other factors such as antibiotics, mutations,
// and hidden health. These are tallied and rolled every 10 turns in hardcoded_effects to determine
// whether the bite recovers. If so, the bite effect duration is set to 0, to indicate that the bite
// effect shall no longer apply. The effect itself will later be removed by effect::decay,
// invoked by Creature::process_effects (outside the scope of these tests, which are focused
// on player::hardcoded_effects).
//
// When an effect is "permanent" like this, its duration is not the total *remaining* effect time,
// but rather as a kind of flexible deadline, modified by the treatments used in the 6-ish-hour
// interim. Antibiotics may slow (or even reverse) the tick rate towards infection, so the "6 hours"
// of effect duration may amount to more or less actual in-game time.
//
TEST_CASE( "antibiotic effects on bites", "[player][effect][bite][antibiotic]" )
{
    avatar dummy;

    const efftype_id effect_strong_antibiotic( "strong_antibiotic" );
    const efftype_id effect_antibiotic( "antibiotic" );
    const efftype_id effect_weak_antibiotic( "weak_antibiotic" );

    const efftype_id effect_bite( "bite" );

    // Use starting bite duration at 3 hours (halfway to infected)
    const time_duration start_bite_duration = 3_hours;
    const int start_bite_turns = to_turns<int>( start_bite_duration );
    // 3 h = 3x60x60 = 10,800 turns
    REQUIRE( start_bite_turns == 10800 );

    // How many turns to pass with each antibiotic to measure their effect
    const int pass_turns = 800;

    GIVEN( "player is bitten" ) {
        dummy.clear_effects();

        dummy.add_effect( effect_bite, start_bite_duration );
        effect &effect_obj = dummy.get_effect( effect_bite );

        // FIXME: Condense these into a helper function
        //
        // Apply hardcoded effects N times, and measure the change in bite duration

        WHEN( "they have no antibiotic effects" ) {
            REQUIRE_FALSE( dummy.has_effect( effect_strong_antibiotic ) );
            REQUIRE_FALSE( dummy.has_effect( effect_antibiotic ) );
            REQUIRE_FALSE( dummy.has_effect( effect_weak_antibiotic ) );

            THEN( "bite duration increases by 1 every turn" ) {
                for( int i = 0; i < pass_turns; i++ ) {
                    dummy.hardcoded_effects( effect_obj );
                    calendar::turn = calendar::turn + 1_turns;
                }
                CHECK( to_turns<int>( effect_obj.get_duration() ) == start_bite_turns + pass_turns );
            }
        }

        WHEN( "they have strong antibiotic effects" ) {
            dummy.add_effect( effect_strong_antibiotic, 1_turns );
            REQUIRE( dummy.has_effect( effect_strong_antibiotic ) );

            THEN( "bite duration decreases by 1 every turn" ) {
                for( int i = 0; i < pass_turns; i++ ) {
                    dummy.hardcoded_effects( effect_obj );
                    calendar::turn = calendar::turn + 1_turns;
                }
                CHECK( to_turns<int>( effect_obj.get_duration() ) == start_bite_turns - pass_turns );
            }
        }

        WHEN( "they have normal antibiotic effects" ) {
            dummy.add_effect( effect_antibiotic, 1_turns );
            REQUIRE( dummy.has_effect( effect_antibiotic ) );

            THEN( "bite duration increases by 1 every 8 turns" ) {
                for( int i = 0; i < pass_turns; i++ ) {
                    dummy.hardcoded_effects( effect_obj );
                    calendar::turn = calendar::turn + 1_turns;
                }
                CHECK( to_turns<int>( effect_obj.get_duration() ) == start_bite_turns + pass_turns / 8 );
            }
        }

        WHEN( "they have weak antibiotic effects" ) {
            dummy.add_effect( effect_weak_antibiotic, 1_turns );
            REQUIRE( dummy.has_effect( effect_weak_antibiotic ) );

            THEN( "bite duration increases by 1 every 2 turns" ) {
                for( int i = 0; i < pass_turns; i++ ) {
                    dummy.hardcoded_effects( effect_obj );
                    calendar::turn = calendar::turn + 1_turns;
                }
                CHECK( to_turns<int>( effect_obj.get_duration() ) == start_bite_turns + pass_turns / 2 );
            }
        }
    }
}

// effect_infected (~50 lines)
//
// Almost a straight copy/paste of effect_bite
//
// If factor in 5,184,000, recover!
//
// If not recovered:
// - After 1 day, death ("You succumb to the infection."
// Else, get one of:
// - strong_antibiotic effect -1 turn
// - antibiotic effect: +1 turn every 8 hours
// - weak_antibiotic effect: +1 turn every 2 turns
// - Otherwise: mod_duration +1 turn


// effect_lying_down
//
// If lying down:
// - Sets moves to 0
// - If can sleep, fall asleep; lying_down duration = 0
// - If lying_down duration == 1 turn AND was not already sleeping, "You try to sleep, but can't..."
TEST_CASE( "lying down", "[effect][lying]" )
{
    player &dummy = g->u;
    clear_map();
    clear_character( dummy );

    const efftype_id effect_lying_down( "lying_down" );
    const efftype_id effect_sleep( "sleep" );
    const efftype_id effect_meth( "meth" );

    GIVEN( "player cannot sleep" ) {
        dummy.add_effect( effect_meth, 1_days );
        REQUIRE_FALSE( dummy.can_sleep() );

        WHEN( "lying down" ) {
            dummy.add_effect( effect_lying_down, 1_turns );
            effect &effect_obj = dummy.get_effect( effect_lying_down );
            REQUIRE( dummy.has_effect( effect_lying_down ) );

            dummy.hardcoded_effects( effect_obj );

            THEN( "they do not fall asleep" ) {
                REQUIRE_FALSE( dummy.has_effect( effect_sleep ) );
                AND_THEN( "they are still lying down" ) {
                    REQUIRE( to_turns<int>( effect_obj.get_duration() ) == 1 );
                }
            }
        }
    }

    GIVEN( "player can sleep" ) {
        // TODO: Find a way to ensure can_sleep is true
        //REQUIRE( dummy.can_sleep() );
    }
}

// effect_sleep
//
// WHILE SLEEPING (1032-1259)
// - Sets moves to 0
// - Intensity < 1, set to 1 (minimum)
// - Intensity 1-24, add 1
// Waking up
// - Prevent waking up naturally while under anesthesia
// - If fatigue reaches 0 or less, and not narcotized, wake up
// Mutation stuff
// - Every 10 minutes, while outside:
// -- Chloromorphosis: light and compatible weather gives reduced hunger and thirst
// -- Mycogenesis: Fatigue lessens, SpOrEs happen!
// -- Aqueous Repose: Need less sleep in water (but we are not in water?)
// - Possibly have mutation dreams
// -- (functions: get_highest_category, mutation_category_level, crossed_threshold)
// -- [crossed]: strength 4 (post-human)
// -- [50..??): strength 3
// -- [35..50): strength 2
// -- [20..35): strength 1
// -- [0..20): strength 0
// -- Any strength > 0 may give dreams
// -- In dreams is where mycus may upgrade
// Wait, did we wake up?? (woke_up=false)
// - Calculate tirednessVal (random based on fatigue)
// - If not blind or narcotized, light can wake you up:
// -- Lidless Eyes SEESLEEP: Can see hostiles approaching and wake on sight
// -- Very Heavy Sleeper HEAVYSLEEPER2
// -- Hibernation HIBERNATE
// - If have not woken, and not narcotized
// -- Cold or heat may wake you up
// -- Can sleep through cold or heat if fatigued enough
// - Schizo may have hallucination that makes them wake up
// If woke up:
// - Print health if got more than 2 hours of sleep
// - Check for alarm that hasn't gone off yet
// - Also check for sleeping through alarm clock

TEST_CASE( "too much light to sleep", "[player][effect][sleep][light]" )
{
}

TEST_CASE( "too hot or cold to sleep", "[player][effect][sleep][hot][cold]" )
{
}

TEST_CASE( "mutation dreams while sleeping", "[player][effect][sleep][dream][mutation]" )
{
}

// effect_toxin_buildup
//
// Toxin buildup (effect_toxin_buildup): L1333-1406
// - Does nothing if in_sleep_state
//
// "Each symptom is twice as frequent for each level of intensity above the one it first appears for."
//
// Tonic-clonic seizure (full body convulsive seizure)
// - "You lose control of your body as it begins to convulse!" (downed, stunned)
// - "You lose consciousness!" (1/3)
//
// Myoclonic seizure (muscle spasm)
// - "Your %s suddenly jerks in an unexpected direction!" (arm, hand, leg)
// - "However, you manage to keep hold of your weapon." (arm)
// - "However, you manage to keep your footing." (leg)
//
// Atonic seizure (a.k.a. drop seizure)
// - "You suddenly lose all muscle tone, and can't support your own weight!"
//
// Migraine
// - "You have a splitting headache."

