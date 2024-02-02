#include <cctype>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iosfwd>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "activity_handlers.h"
#include "addiction.h"
#include "bionics.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_utility.h"
#include "character.h"
#include "creature.h"
#include "creature_tracker.h"
#include "debug.h"
#include "display.h"
#include "effect.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "field_type.h"
#include "flag.h"
#include "game.h"
#include "game_constants.h"
#include "inventory.h"
#include "item.h"
#include "item_location.h"
#include "magic_enchantment.h"
#include "map.h"
#include "memory_fast.h"
#include "messages.h"
#include "monster.h"
#include "morale_types.h"
#include "mtype.h"
#include "mutation.h"
#include "npc.h"
#include "options.h"
#include "output.h"
#include "overmapbuffer.h"
#include "pimpl.h"
#include "player_activity.h"
#include "point.h"
#include "rng.h"
#include "skill.h"
#include "sounds.h"
#include "stomach.h"
#include "string_formatter.h"
#include "teleport.h"
#include "text_snippets.h"
#include "translations.h"
#include "type_id.h"
#include "units.h"
#include "weather.h"
#include "weather_type.h"

static const addiction_id addiction_alcohol( "alcohol" );
static const addiction_id addiction_nicotine( "nicotine" );

static const bionic_id bio_dis_acid( "bio_dis_acid" );
static const bionic_id bio_dis_shock( "bio_dis_shock" );
static const bionic_id bio_geiger( "bio_geiger" );
static const bionic_id bio_gills( "bio_gills" );
static const bionic_id bio_power_weakness( "bio_power_weakness" );
static const bionic_id bio_radleak( "bio_radleak" );
static const bionic_id bio_sleep_shutdown( "bio_sleep_shutdown" );
static const bionic_id bio_synlungs( "bio_synlungs" );

static const efftype_id effect_adrenaline( "adrenaline" );
static const efftype_id effect_asthma( "asthma" );
static const efftype_id effect_blind( "blind" );
static const efftype_id effect_cig( "cig" );
static const efftype_id effect_datura( "datura" );
static const efftype_id effect_deaf( "deaf" );
static const efftype_id effect_disabled( "disabled" );
static const efftype_id effect_downed( "downed" );
static const efftype_id effect_drunk( "drunk" );
static const efftype_id effect_hallu( "hallu" );
static const efftype_id effect_incorporeal( "incorporeal" );
static const efftype_id effect_iodine( "iodine" );
static const efftype_id effect_masked_scent( "masked_scent" );
static const efftype_id effect_mending( "mending" );
static const efftype_id effect_narcosis( "narcosis" );
static const efftype_id effect_nausea( "nausea" );
static const efftype_id effect_onfire( "onfire" );
static const efftype_id effect_shakes( "shakes" );
static const efftype_id effect_sleep( "sleep" );
static const efftype_id effect_took_antiasthmatic( "took_antiasthmatic" );
static const efftype_id effect_took_xanax( "took_xanax" );
static const efftype_id effect_visuals( "visuals" );
static const efftype_id effect_weary_0( "weary_0" );
static const efftype_id effect_weary_1( "weary_1" );
static const efftype_id effect_weary_2( "weary_2" );
static const efftype_id effect_weary_3( "weary_3" );
static const efftype_id effect_weary_4( "weary_4" );
static const efftype_id effect_weary_5( "weary_5" );
static const efftype_id effect_weary_6( "weary_6" );
static const efftype_id effect_weary_7( "weary_7" );
static const efftype_id effect_weary_8( "weary_8" );
static const efftype_id effect_winded( "winded" );

static const itype_id itype_e_handcuffs( "e_handcuffs" );
static const itype_id itype_inhaler( "inhaler" );
static const itype_id itype_oxygen_tank( "oxygen_tank" );
static const itype_id itype_smoxygen_tank( "smoxygen_tank" );

static const json_character_flag json_flag_ALBINO( "ALBINO" );
static const json_character_flag json_flag_DAYFEAR( "DAYFEAR" );
static const json_character_flag json_flag_GILLS( "GILLS" );
static const json_character_flag json_flag_GLARE_RESIST( "GLARE_RESIST" );
static const json_character_flag json_flag_GRAB( "GRAB" );
static const json_character_flag json_flag_MEND_ALL( "MEND_ALL" );
static const json_character_flag json_flag_MEND_LIMB( "MEND_LIMB" );
static const json_character_flag json_flag_NYCTOPHOBIA( "NYCTOPHOBIA" );
static const json_character_flag json_flag_PAIN_IMMUNE( "PAIN_IMMUNE" );
static const json_character_flag json_flag_RAD_DETECT( "RAD_DETECT" );
static const json_character_flag json_flag_SUNBURN( "SUNBURN" );

static const trait_id trait_ADDICTIVE( "ADDICTIVE" );
static const trait_id trait_ASTHMA( "ASTHMA" );
static const trait_id trait_CHAOTIC( "CHAOTIC" );
static const trait_id trait_CHAOTIC_BAD( "CHAOTIC_BAD" );
static const trait_id trait_CHEMIMBALANCE( "CHEMIMBALANCE" );
static const trait_id trait_CHLOROMORPH( "CHLOROMORPH" );
static const trait_id trait_DEBUG_NOTEMP( "DEBUG_NOTEMP" );
static const trait_id trait_FRESHWATEROSMOSIS( "FRESHWATEROSMOSIS" );
static const trait_id trait_HAS_NEMESIS( "HAS_NEMESIS" );
static const trait_id trait_JAUNDICE( "JAUNDICE" );
static const trait_id trait_JITTERY( "JITTERY" );
static const trait_id trait_KILLER( "KILLER" );
static const trait_id trait_LEAVES( "LEAVES" );
static const trait_id trait_LEAVES2( "LEAVES2" );
static const trait_id trait_LEAVES2_FALL( "LEAVES2_FALL" );
static const trait_id trait_LEAVES3( "LEAVES3" );
static const trait_id trait_LEAVES3_FALL( "LEAVES3_FALL" );
static const trait_id trait_MOODSWINGS( "MOODSWINGS" );
static const trait_id trait_MUCUS_SECRETION( "MUCUS_SECRETION" );
static const trait_id trait_M_BLOSSOMS( "M_BLOSSOMS" );
static const trait_id trait_M_SPORES( "M_SPORES" );
static const trait_id trait_NARCOLEPTIC( "NARCOLEPTIC" );
static const trait_id trait_NONADDICTIVE( "NONADDICTIVE" );
static const trait_id trait_NO_LEFT_ARM( "NO_LEFT_ARM" );
static const trait_id trait_NO_LEFT_LEG( "NO_LEFT_LEG" );
static const trait_id trait_NO_RIGHT_ARM( "NO_RIGHT_ARM" );
static const trait_id trait_NO_RIGHT_LEG( "NO_RIGHT_LEG" );
static const trait_id trait_PER_SLIME( "PER_SLIME" );
static const trait_id trait_PLANTSKIN( "PLANTSKIN" );
static const trait_id trait_PYROMANIA( "PYROMANIA" );
static const trait_id trait_RADIOACTIVE1( "RADIOACTIVE1" );
static const trait_id trait_RADIOACTIVE2( "RADIOACTIVE2" );
static const trait_id trait_RADIOACTIVE3( "RADIOACTIVE3" );
static const trait_id trait_RADIOGENIC( "RADIOGENIC" );
static const trait_id trait_RADIOPHILE( "RADIOPHILE" );
static const trait_id trait_SHARKTEETH( "SHARKTEETH" );
static const trait_id trait_SHELL2( "SHELL2" );
static const trait_id trait_SHELL3( "SHELL3" );
static const trait_id trait_SHOUT1( "SHOUT1" );
static const trait_id trait_SHOUT2( "SHOUT2" );
static const trait_id trait_SHOUT3( "SHOUT3" );
static const trait_id trait_SNAIL_TRAIL( "SNAIL_TRAIL" );
static const trait_id trait_SORES( "SORES" );
static const trait_id trait_TROGLO( "TROGLO" );
static const trait_id trait_TROGLO2( "TROGLO2" );
static const trait_id trait_TROGLO3( "TROGLO3" );
static const trait_id trait_UNDINE_ABSORB_WATER( "UNDINE_ABSORB_WATER" );
static const trait_id trait_UNSTABLE( "UNSTABLE" );
static const trait_id trait_VINES1( "VINES1" );
static const trait_id trait_VINES2( "VINES2" );
static const trait_id trait_VINES3( "VINES3" );
static const trait_id trait_VOMITOUS( "VOMITOUS" );
static const trait_id trait_WEB_SPINNER( "WEB_SPINNER" );
static const trait_id trait_WEB_WEAVER( "WEB_WEAVER" );
static const trait_id trait_WINGS_INSECT_active( "WINGS_INSECT_active" );

static const vitamin_id vitamin_vitC( "vitC" );

namespace suffer
{
static void from_sunburn( Character &you, bool severe );
static void in_sunlight( Character &you, outfit &worn );
static void water_damage( Character &you, const trait_id &mut_id );
static void mutation_power( Character &you, const trait_id &mut_id );
static void while_underwater( Character &you );
static void while_grabbed( Character &you );
static void from_addictions( Character &you );
static void while_awake( Character &you, int current_stim );
static void from_chemimbalance( Character &you );
static void from_asthma( Character &you, int current_stim );
static void from_item_dropping( Character &you );
static void from_other_mutations( Character &you );
static void from_radiation( Character &you );
static void from_bad_bionics( Character &you );
static void from_stimulants( Character &you, int current_stim );
static void from_exertion( Character &you );
static void without_sleep( Character &you, int sleep_deprivation );
static void from_tourniquet( Character &you );
static void from_nyctophobia( Character &you );
static void from_artifact_resonance( Character &you, int amt );
} // namespace suffer

static float addiction_scaling( float at_min, float at_max, float add_lvl )
{
    // Not addicted
    if( add_lvl < MIN_ADDICTION_LEVEL ) {
        return 1.0f;
    }

    return lerp( at_min, at_max, ( add_lvl - MIN_ADDICTION_LEVEL ) / MAX_ADDICTION_LEVEL );
}

void suffer::water_damage( Character &you, const trait_id &mut_id )
{
    for( const std::pair<const bodypart_str_id, bodypart> &elem : you.get_body() ) {
        const float wetness_percentage = elem.second.get_wetness_percentage();
        const int dmg = mut_id->weakness_to_water * wetness_percentage;
        if( dmg > 0 ) {
            you.apply_damage( nullptr, elem.first, dmg );
            you.add_msg_player_or_npc( m_bad, _( "Your %s is damaged by the water." ),
                                       _( "<npcname>'s %s is damaged by the water." ),
                                       body_part_name( elem.first ) );
        } else if( dmg < 0 && elem.second.is_at_max_hp() ) {
            you.heal( elem.first, std::abs( dmg ) );
            you.add_msg_player_or_npc( m_good, _( "Your %s is healed by the water." ),
                                       _( "<npcname>'s %s is healed by the water." ),
                                       body_part_name( elem.first ) );
        }
    }
}

void suffer::mutation_power( Character &you, const trait_id &mut_id )
{
    if( you.get_cost_timer( mut_id ) > 0_turns ) {
        // Not ready to consume cost yet, the timer ticks on
        you.mod_cost_timer( mut_id, -1_turns );
    } else {
        // Ready to consume cost: pay the power cost and reset timer
        if( mut_id->cooldown > 0_turns ) {
            you.set_cost_timer( mut_id, mut_id->cooldown - 1_turns );
        }
        if( mut_id->hunger ) {
            if( you.get_bmi_fat() < character_weight_category::underweight ) {
                you.add_msg_if_player( m_warning,
                                       _( "You're too malnourished to keep your %s going." ),
                                       you.mutation_name( mut_id ) );
                you.deactivate_mutation( mut_id );
            } else {
                // does not directly modify hunger, but burns kcal
                you.mod_stored_kcal( -mut_id->cost );
            }
        }
        if( mut_id->thirst ) {
            // Well into Dehydrated
            if( you.get_thirst() >= 260 ) {
                you.add_msg_if_player( m_warning,
                                       _( "You're too dehydrated to keep your %s going." ),
                                       you.mutation_name( mut_id ) );
                you.deactivate_mutation( mut_id );
            } else {
                you.mod_thirst( mut_id->cost );
            }
        }
        if( mut_id->fatigue ) {
            // Exhausted
            if( you.get_fatigue() >= fatigue_levels::EXHAUSTED ) {
                you.add_msg_if_player( m_warning,
                                       _( "You're too exhausted to keep your %s going." ),
                                       you.mutation_name( mut_id ) );
                you.deactivate_mutation( mut_id );
            } else {
                you.mod_fatigue( mut_id->cost );
            }
        }

        // if you haven't deactivated then run the EOC
        for( const effect_on_condition_id &eoc : mut_id->processed_eocs ) {
            dialogue d( get_talker_for( you ), nullptr );
            d.set_value( "npctalk_var_this", mut_id.str() );
            if( eoc->type == eoc_type::ACTIVATION ) {
                eoc->activate( d );
            } else {
                debugmsg( "Must use an activation eoc for a mutation process.  If you don't want the effect_on_condition to happen on its own (without the mutation being activated), remove the recurrence min and max.  Otherwise, create a non-recurring effect_on_condition for this mutation with its condition and effects, then have a recurring one queue it." );
            }
        }
    }
}

void suffer::while_underwater( Character &you )
{
    if( !you.has_flag( json_flag_GILLS ) ) {
        you.oxygen--;
    }
    if( you.oxygen < 12 && you.worn_with_flag( flag_REBREATHER ) ) {
        you.oxygen += 12;
    }
    if( you.oxygen <= 5 ) {
        if( you.has_bionic( bio_gills ) && you.get_power_level() >= bio_gills->power_trigger ) {
            you.oxygen += 5;
            you.mod_power_level( -bio_gills->power_trigger );
        } else {
            you.add_msg_if_player( m_bad, _( "You're drowning!" ) );
            you.apply_damage( nullptr, bodypart_id( "torso" ), rng( 1, 4 ) );
        }
    }
    if( you.has_trait( trait_FRESHWATEROSMOSIS ) &&
        !get_map().has_flag_ter( ter_furn_flag::TFLAG_SALT_WATER, you.pos() ) &&
        you.get_thirst() > -60 ) {
        you.mod_thirst( -1 );
    }
    if( you.has_trait( trait_UNDINE_ABSORB_WATER ) && you.get_thirst() > -60 ) {
        you.mod_thirst( -1 );
    }
}

void suffer::while_grabbed( Character &you )
{
    // If you're grabbed you can't move around effectively, triggering the check
    // TODO: crowd crush without grabs
    // Count the number of monsters who can shove you
    // TODO: expand the search area - use the same logic as GROUP_BASH?
    map &here = get_map();
    creature_tracker &creatures = get_creature_tracker();
    int crowd = 0;
    int impassable_ter = 0;
    for( auto&& dest : here.points_in_radius( you.pos(), 1, 0 ) ) { // *NOPAD*
        const monster *const mon = creatures.creature_at<monster>( dest );
        if( mon && mon->has_flag( mon_flag_GROUP_BASH ) ) {
            crowd++;
            add_msg_debug( debugmode::DF_CHARACTER, "Crowd pressure check: monster %s found, crowd size %d",
                           mon->name(), crowd );
        }
        if( here.impassable( dest ) ) {
            impassable_ter++;
        }
    }

    // if we aren't near two monsters with GROUP_BASH we won't suffocate
    if( crowd < 2 ) {
        return;
    }
    // Getting crushed against the wall counts as a monster
    if( impassable_ter ) {
        you.add_msg_if_player( m_bad, _( "You're crushed against the walls!" ) );
        crowd += impassable_ter;
    }

    if( crowd == 2 ) {
        // only a chance to lose breath at low grab chance, none with only a single zombie
        you.oxygen -= rng( 0, 1 );
    } else if( crowd <= 4 ) {
        you.oxygen -= 1;
    } else if( crowd <= 6 ) {
        you.oxygen -= rng( 1, 2 );
    } else if( crowd <= 8 ) {
        you.oxygen -= 2;
    }

    // a few warnings before starting to take damage
    if( you.oxygen <= 5 ) {
        you.add_msg_if_player( m_bad, _( "You're suffocating!" ) );
        if( uistate.distraction_oxygen && you.is_avatar() ) {
            g->cancel_activity_or_ignore_query( distraction_type::oxygen, _( "You're suffocating!" ) );
        }
        // your characters chest is being crushed and you are dying
        you.apply_damage( nullptr, you.get_random_body_part_of_type( body_part_type::type::torso ), rng( 1,
                          4 ) );
    } else if( you.oxygen <= 15 ) {
        you.add_msg_if_player( m_bad, _( "You can't breathe with all this weight!" ) );
    } else if( you.oxygen <= 25 ) {
        you.add_msg_if_player( m_bad, _( "You're having difficulty breathing!" ) );
    }
}

void suffer::from_addictions( Character &you )
{
    time_duration timer = -6_hours;
    if( you.has_trait( trait_ADDICTIVE ) ) {
        timer = -10_hours;
    } else if( you.has_trait( trait_NONADDICTIVE ) ) {
        timer = -3_hours;
    }
    for( addiction &cur_addiction : you.addictions ) {
        if( cur_addiction.sated <= 0_turns &&
            cur_addiction.intensity >= MIN_ADDICTION_LEVEL ) {
            cur_addiction.run_effect( you );
        }
        cur_addiction.sated -= 1_turns;
        // Higher intensity addictions heal faster
        if( cur_addiction.sated - 10_minutes * cur_addiction.intensity < timer ) {
            if( cur_addiction.intensity <= 2 ) {
                you.rem_addiction( cur_addiction.type );
                break;
            } else {
                cur_addiction.intensity--;
                cur_addiction.sated = 0_turns;
            }
        }
    }
}

void suffer::while_awake( Character &you, const int current_stim )
{
    if( you.weight_carried() > 4 * you.weight_capacity() ) {
        if( you.has_effect( effect_downed ) ) {
            you.add_effect( effect_downed, 1_turns, false, 0, true );
        } else {
            you.add_effect( effect_downed, 2_turns, false, 0, true );
        }
    }

    if( you.has_flag( json_flag_NYCTOPHOBIA ) && !you.has_effect( effect_took_xanax ) ) {
        suffer::from_nyctophobia( you );
    }

    if( you.has_trait( trait_CHEMIMBALANCE ) ) {
        suffer::from_chemimbalance( you );
    }

    if( you.has_trait( trait_NARCOLEPTIC ) ) {
        if( one_turn_in( 8_hours ) ) {
            you.add_msg_if_player( m_bad,
                                   _( "You're suddenly overcome with the urge to sleep and you pass out." ) );
            you.fall_asleep( 20_minutes );
        }
    }

    int resonance = you.enchantment_cache->get_value_add( enchant_vals::mod::ARTIFACT_RESONANCE );
    if( resonance > 0 ) {
        suffer::from_artifact_resonance( you, resonance );
    }

    if( you.has_trait( trait_JITTERY ) && !you.has_effect( effect_shakes ) ) {
        if( current_stim > 50 && one_in( to_turns<int>( 30_minutes ) - ( current_stim * 6 ) ) ) {
            you.add_effect( effect_shakes, 30_minutes + 1_turns * current_stim );
        } else if( ( you.get_hunger() > 80 || you.get_kcal_percent() < 1.0f ) && you.get_hunger() > 0 &&
                   one_in( to_turns<int>( 50_minutes ) - ( you.get_hunger() * 6 ) ) ) {
            you.add_effect( effect_shakes, 40_minutes );
        }
    }

    if( you.has_trait( trait_MOODSWINGS ) && one_turn_in( 6_hours ) ) {
        if( rng( 1, 20 ) > 9 ) {
            // 55% chance
            you.add_morale( MORALE_MOODSWING, -20, -100 );
        } else {
            // 45% chance
            you.add_morale( MORALE_MOODSWING, 20, 100 );
        }
    }

    if( you.has_trait( trait_VOMITOUS ) && one_turn_in( 7_hours ) ) {
        you.vomit();
    }

    if( you.has_trait( trait_HAS_NEMESIS ) && one_turn_in( 2_minutes ) ) {
        you.signal_nemesis();
    }

    if( you.has_trait( trait_SHOUT1 ) && one_turn_in( 6_hours ) ) {
        you.shout();
    }
    if( you.has_trait( trait_SHOUT2 ) && one_turn_in( 4_hours ) ) {
        you.shout();
    }
    if( you.has_trait( trait_SHOUT3 ) && one_turn_in( 3_hours ) ) {
        you.shout();
    }
    if( you.has_trait( trait_M_SPORES ) && one_turn_in( 4_hours ) ) {
        you.spores();
    }
    if( you.has_trait( trait_M_BLOSSOMS ) && one_turn_in( 3_hours ) ) {
        you.blossoms();
    }
}

void suffer::from_chemimbalance( Character &you )
{
    if( one_turn_in( 6_hours ) && !you.has_flag( json_flag_PAIN_IMMUNE ) ) {
        you.add_msg_if_player( m_bad, _( "You suddenly feel sharp pain for no reason." ) );
        you.mod_pain( 3 * rng( 1, 3 ) );
    }
    if( one_turn_in( 6_hours ) ) {
        int pkilladd = 5 * rng( -1, 2 );
        if( pkilladd > 0 ) {
            you.add_msg_if_player( m_bad, _( "You suddenly feel numb." ) );
        } else if( ( pkilladd < 0 ) && ( !you.has_flag( json_flag_PAIN_IMMUNE ) ) ) {
            you.add_msg_if_player( m_bad, _( "You suddenly ache." ) );
        }
        you.mod_painkiller( pkilladd );
    }
    if( one_turn_in( 6_hours ) && !you.has_effect( effect_sleep ) ) {
        you.add_msg_if_player( m_bad, _( "You feel dizzy for a moment." ) );
        you.moves -= rng( 10, 30 );
    }
    if( one_turn_in( 6_hours ) ) {
        int hungadd = 5 * rng( -1, 3 );
        if( hungadd > 0 ) {
            you.add_msg_if_player( m_bad, _( "You suddenly feel hungry." ) );
        } else {
            you.add_msg_if_player( m_good, _( "You suddenly feel a little full." ) );
        }
        you.mod_hunger( hungadd );
    }
    if( one_turn_in( 6_hours ) ) {
        you.add_msg_if_player( m_bad, _( "You suddenly feel thirsty." ) );
        you.mod_thirst( 5 * rng( 1, 3 ) );
    }
    if( one_turn_in( 6_hours ) ) {
        you.add_msg_if_player( m_good, _( "You feel fatigued all of a sudden." ) );
        you.mod_fatigue( 10 * rng( 2, 4 ) );
    }
    if( one_turn_in( 8_hours ) ) {
        if( one_in( 3 ) ) {
            you.add_morale( MORALE_FEELING_GOOD, 20, 100 );
        } else {
            you.add_morale( MORALE_FEELING_BAD, -20, -100 );
        }
    }
    if( one_turn_in( 6_hours ) ) {
        if( one_in( 3 ) ) {
            you.add_msg_if_player( m_bad, _( "You suddenly feel very cold." ) );
            you.set_all_parts_temp_cur( BODYTEMP_VERY_COLD );
        } else {
            you.add_msg_if_player( m_bad, _( "You suddenly feel cold." ) );
            you.set_all_parts_temp_cur( BODYTEMP_COLD );
        }
    }
    if( one_turn_in( 6_hours ) ) {
        if( one_in( 3 ) ) {
            you.add_msg_if_player( m_bad, _( "You suddenly feel very hot." ) );
            you.set_all_parts_temp_cur( BODYTEMP_VERY_HOT );
        } else {
            you.add_msg_if_player( m_bad, _( "You suddenly feel hot." ) );
            you.set_all_parts_temp_cur( BODYTEMP_HOT );
        }
    }
}

void suffer::from_asthma( Character &you, const int current_stim )
{
    if( you.has_effect( effect_adrenaline ) ||
        you.has_effect( effect_datura ) ||
        you.has_effect( effect_took_antiasthmatic ) ) {
        return;
    }
    //cap asthma attacks to 1 per minute (or risk instantly killing players that rely on oxygen tanks)
    if( !one_in( std::max( to_turns<int>( 1_minutes ),
                           ( to_turns<int>( 6_hours ) - current_stim * 300 ) ) *
                 ( you.has_effect( effect_sleep ) ? 10 : 1 ) ) ) {
        return;
    }
    bool auto_use = you.has_charges( itype_inhaler, 1 ) || you.has_charges( itype_oxygen_tank, 1 ) ||
                    you.has_charges( itype_smoxygen_tank, 1 );
    bool oxygenator = you.has_bionic( bio_gills ) &&
                      you.get_power_level() >= ( bio_gills->power_trigger / 8 );
    if( you.underwater ) {
        you.oxygen = you.oxygen / 2;
        auto_use = false;
    }

    you.add_msg_player_or_npc( m_bad, _( "You have an asthma attack!" ),
                               _( "<npcname> starts wheezing and coughing." ) );

    map &here = get_map();
    if( you.in_sleep_state() && !you.has_effect( effect_narcosis ) ) {
        inventory map_inv;
        map_inv.form_from_map( you.pos(), 2, &you );
        // check if an inhaler is somewhere near
        bool nearby_use = auto_use || oxygenator || map_inv.has_charges( itype_inhaler, 1 ) ||
                          map_inv.has_charges( itype_oxygen_tank, 1 ) ||
                          map_inv.has_charges( itype_smoxygen_tank, 1 );
        // check if character has an oxygenator first
        if( oxygenator ) {
            you.mod_power_level( -bio_gills->power_trigger / 8 );
            you.add_msg_if_player( m_info, _( "You use your Oxygenator to clear it up, "
                                              "then go back to sleep." ) );
        } else if( auto_use  && !you.has_bionic( bio_sleep_shutdown ) ) {
            if( you.use_charges_if_avail( itype_inhaler, 1 ) ) {
                you.add_msg_if_player( m_info, _( "You use your inhaler and go back to sleep." ) );
                you.add_effect( effect_took_antiasthmatic, rng( 6_hours, 12_hours ) );
            } else if( you.use_charges_if_avail( itype_oxygen_tank, 1 ) ||
                       you.use_charges_if_avail( itype_smoxygen_tank, 1 ) ) {
                you.add_msg_if_player( m_info, _( "You take a deep breath from your oxygen tank "
                                                  "and go back to sleep." ) );
            }
        } else if( nearby_use  && !you.has_bionic( bio_sleep_shutdown ) ) {
            // create new variable to resolve a reference issue
            int amount = 1;
            if( !here.use_charges( you.pos(), 2, itype_inhaler, amount ).empty() ) {
                you.add_msg_if_player( m_info, _( "You use your inhaler and go back to sleep." ) );
                you.add_effect( effect_took_antiasthmatic, rng( 6_hours, 12_hours ) );
            } else if( !here.use_charges( you.pos(), 2, itype_oxygen_tank, amount ).empty() ||
                       !here.use_charges( you.pos(), 2, itype_smoxygen_tank, amount ).empty() ) {
                you.add_msg_if_player( m_info, _( "You take a deep breath from your oxygen tank "
                                                  "and go back to sleep." ) );
            }
        } else {
            you.add_effect( effect_asthma, rng( 5_minutes, 20_minutes ) );
            if( you.has_effect( effect_sleep ) ) {
                if( !you.has_bionic( bio_sleep_shutdown ) ) {
                    you.wake_up();
                }
            } else {
                if( uistate.distraction_asthma && !you.is_npc() ) {
                    g->cancel_activity_or_ignore_query( distraction_type::asthma,
                                                        _( "You can't focus while choking!" ) );
                }
            }
        }
    } else if( auto_use ) {
        int charges = 0;
        if( you.use_charges_if_avail( itype_inhaler, 1 ) ) {
            you.moves -= 40;
            charges = you.charges_of( itype_inhaler );
            if( charges == 0 ) {
                you.add_msg_if_player( m_bad, _( "You use your last inhaler charge." ) );
            } else {
                you.add_msg_if_player( m_info, n_gettext( "You use your inhaler; "
                                       "only %d charge left.",
                                       "You use your inhaler; "
                                       "only %d charges left.", charges ),
                                       charges );
            }
            you.add_effect( effect_took_antiasthmatic, rng( 6_hours, 12_hours ) );
        } else if( you.use_charges_if_avail( itype_oxygen_tank, 1 ) ||
                   you.use_charges_if_avail( itype_smoxygen_tank, 1 ) ) {
            you.moves -= 500; // synched with use action
            charges = you.charges_of( itype_oxygen_tank ) + you.charges_of( itype_smoxygen_tank );
            if( charges == 0 ) {
                you.add_msg_if_player( m_bad, _( "You breathe in the last bit of oxygen "
                                                 "from the tank." ) );
            } else {
                you.add_msg_if_player( m_info, n_gettext( "You take a deep breath from your oxygen "
                                       "tank; only %d charge left.",
                                       "You take a deep breath from your oxygen "
                                       "tank; only %d charges left.", charges ),
                                       charges );
            }
        }
    } else {
        you.add_effect( effect_asthma, rng( 5_minutes, 20_minutes ) );
        if( uistate.distraction_asthma && !you.is_npc() ) {
            g->cancel_activity_or_ignore_query( distraction_type::asthma,
                                                _( "You can't focus while choking!" ) );
        }
    }
}

void suffer::in_sunlight( Character &you, outfit &worn )
{
    const tripoint position = you.pos();

    if( !g->is_in_sunlight( position ) ) {
        return;
    }

    const bool leafy = you.has_trait( trait_LEAVES ) ||
                       you.has_trait( trait_LEAVES2 ) ||
                       you.has_trait( trait_LEAVES2_FALL ) ||
                       you.has_trait( trait_LEAVES3 ) ||
                       you.has_trait( trait_LEAVES3_FALL ) ||
                       you.has_trait( trait_PLANTSKIN ) ||
                       you.has_trait( trait_JAUNDICE );
    float phelloderm_surface = 0.0;
    float exposure = 0.0;
    int sunlight_nutrition = 0;
    if( leafy ) {
        // Phelloderm and bark photosynthesize.
        if( you.has_trait( trait_PLANTSKIN ) || you.has_trait( trait_JAUNDICE ) ) {
            std::map<bodypart_id, float> bp_exposure = you.bodypart_exposure();
            for( auto &bp_exp : bp_exposure ) {
                bodypart_id bp = bp_exp.first;
                exposure = bp_exp.second;
                if( ( ( ( bp == body_part_arm_l ) || ( bp == body_part_hand_l ) ) &&
                      you.has_trait( trait_NO_LEFT_ARM ) )
                    || ( ( ( bp == body_part_arm_r ) || ( bp == body_part_hand_r ) ) &&
                         you.has_trait( trait_NO_RIGHT_ARM ) )
                    || ( ( ( bp == body_part_leg_l ) || ( bp == body_part_foot_l ) ) &&
                         you.has_trait( trait_NO_LEFT_LEG ) )
                    || ( ( ( bp == body_part_leg_r ) || ( bp == body_part_foot_r ) ) &&
                         you.has_trait( trait_NO_RIGHT_LEG ) ) ) {
                    exposure = 0.0;
                }
                phelloderm_surface += exposure;
            }
            // The Jaundice mutation means you have some chloroplasts in your skin, but not as many.
            if( you.has_trait( trait_JAUNDICE ) ) {
                phelloderm_surface *= .5;
            };
            // Multiply phelloderm_surface here so we can average it with head_exposure later.
            phelloderm_surface *= 8.33;
        }
        // The Jaundice mutation means you have some chloroplasts in your skin, but not as many.
        if( you.has_trait( trait_JAUNDICE ) ) {
            phelloderm_surface *= .5;
        }
        // Multiply phelloderm_surface here so we can average it with head_exposure later.
        phelloderm_surface *= 8.33;
        const bool leafier = you.has_trait( trait_LEAVES2 ) || you.has_trait( trait_LEAVES2_FALL );
        const bool leafiest = you.has_trait( trait_LEAVES3 ) || you.has_trait( trait_LEAVES3_FALL );
        const bodypart_id left_arm( "arm_l" );
        const bodypart_id right_arm( "arm_r" );
        const bodypart_id head( "head" );
        // TODO: Limbify vines and give them some way to be covered. For now they will use the average of your arm coverage.
        // We'll assume they poke out of your sleeves to do their thing.
        float head_leaf_surface = 100 - worn.coverage_with_flags_exclude( head, { flag_INTEGRATED, flag_TRANSPARENT } );
        float rarm_leaf_surface = .5 - ( worn.coverage_with_flags_exclude( right_arm, { flag_INTEGRATED, flag_TRANSPARENT } )
                                         * .005 );
        float larm_leaf_surface = .5 - ( worn.coverage_with_flags_exclude( left_arm, { flag_INTEGRATED, flag_TRANSPARENT } )
                                         * .005 );
        float vine_leaf_surface = rarm_leaf_surface + larm_leaf_surface;
        if( you.has_trait( trait_NO_LEFT_ARM ) ) {
            larm_leaf_surface = .0;
        }
        if( you.has_trait( trait_NO_RIGHT_ARM ) ) {
            rarm_leaf_surface = .0;
        }
        if( you.has_trait( trait_VINES1 ) ) {
            vine_leaf_surface = vine_leaf_surface * 0.5;
        }
        if( !you.has_trait( trait_VINES1 ) && !you.has_trait( trait_VINES2 ) &&
            !you.has_trait( trait_VINES3 ) ) {
            vine_leaf_surface = 0;
        }
        const float weather_factor = std::min( incident_sun_irradiance( get_weather().weather_id,
                                               calendar::turn ) / irradiance::moderate, 1.f );
        const int player_local_temp = units::to_fahrenheit( get_weather().get_temperature( position ) );
        int flux = ( player_local_temp - 32 ) / 5;
        // Efficiency rapidly falls off when it's too hot due to photosynthesis being an enzymatic process.
        // Some tropical plants can overcome this with specific adaptations, but that would probably be its own mutation.
        if( player_local_temp > 104 ) {
            flux -= ( player_local_temp - 104 ) * 3;
        }
        if( you.has_trait( trait_LEAVES ) ) {
            head_leaf_surface *= .5;
        }
        if( you.has_trait( trait_LEAVES3 ) ) {
            head_leaf_surface *= 1.5;
        }
        if( you.has_trait( trait_LEAVES3 ) ) {
            head_leaf_surface *= 1.75;
        }
        if( you.has_trait( trait_LEAVES2_FALL ) || you.has_trait( trait_LEAVES3_FALL ) ) {
            vine_leaf_surface *= .9;
            larm_leaf_surface *= .9;
            rarm_leaf_surface *= .9;
            head_leaf_surface *= .9;
        }
        if( !you.has_trait( trait_LEAVES ) && !you.has_trait( trait_LEAVES2 ) &&
            !you.has_trait( trait_LEAVES2_FALL ) && !you.has_trait( trait_LEAVES3 ) &&
            !you.has_trait( trait_LEAVES3_FALL ) ) {
            head_leaf_surface = 0;
        }
        sunlight_nutrition += ( ( 20 + flux ) * weather_factor ) * ( ( head_leaf_surface +
                              phelloderm_surface ) / 200 );
        if( leafier || leafiest ) {
            const int rate = std::max( round( 7 * ( ( larm_leaf_surface + rarm_leaf_surface +
                                                    vine_leaf_surface ) / 2 ) + flux ), 0.0f ) * 2;
            sunlight_nutrition += rate * ( leafiest ? 1.75 : 1.5 ) * weather_factor;
        }
        you.get_size();
        // Multiply by the proportional difference in average height, as height roughly determines armspan,
        // and that's how far out our branches extend.
        if( you.get_size() == creature_size::tiny ) {
            sunlight_nutrition *= .015;
        }
        if( you.get_size() == creature_size::small ) {
            sunlight_nutrition *= .7;
        }
        if( you.get_size() == creature_size::large ) {
            sunlight_nutrition *= 1.54;
        }
        if( you.get_size() == creature_size::huge ) {
            sunlight_nutrition *= 2.35;
        }
        // Chloromorph makes photosynthesis more efficient while the animal parts are in sleep mode.
        if( you.has_effect( effect_sleep ) && you.has_trait( trait_CHLOROMORPH ) ) {
            sunlight_nutrition *= 1.1;
        }

        if( x_in_y( sunlight_nutrition, 12000 ) ) {
            you.vitamin_mod( vitamin_vitC, 1 );
            you.mod_hunger( -1 );
            // Photosynthesis absorbs kcal directly.
            you.mod_stored_kcal( 1 );
            you.stomach.ate();
        }
    }
    if( you.has_flag( json_flag_SUNBURN ) ) {
        suffer::from_sunburn( you, true );
    }

    // Albinism and datura have the same effects and do not stack with each other or sunburn.
    if( !you.has_flag( json_flag_SUNBURN ) &&
        ( you.has_flag( json_flag_ALBINO ) || you.has_effect( effect_datura ) ) ) {
        suffer::from_sunburn( you, false );
    }

    if( ( you.has_trait( trait_TROGLO ) || you.has_trait( trait_TROGLO2 ) ) &&
        incident_sun_irradiance( get_weather().weather_id, calendar::turn ) > irradiance::moderate ) {
        you.mod_str_bonus( -1 );
        you.mod_dex_bonus( -1 );
        you.add_miss_reason( _( "The sunlight distracts you." ), 1 );
        you.mod_int_bonus( -1 );
        you.mod_per_bonus( -1 );
    }
    if( you.has_trait( trait_TROGLO2 ) &&
        incident_sun_irradiance( get_weather().weather_id, calendar::turn ) > irradiance::low ) {
        you.mod_str_bonus( -1 );
        you.mod_dex_bonus( -1 );
        you.add_miss_reason( _( "The sunlight distracts you." ), 1 );
        you.mod_int_bonus( -1 );
        you.mod_per_bonus( -1 );
    }
    if( you.has_flag( json_flag_DAYFEAR ) ) {
        you.mod_str_bonus( -2 );
        you.mod_dex_bonus( -2 );
        you.add_miss_reason( _( "You fear the sunlight!" ), 2 );
        you.mod_int_bonus( -2 );
        you.mod_per_bonus( -2 );
    }
    if( you.has_trait( trait_TROGLO3 ) ) {
        you.mod_str_bonus( -4 );
        you.mod_dex_bonus( -4 );
        you.add_miss_reason( _( "You can't stand the sunlight!" ), 4 );
        you.mod_int_bonus( -4 );
        you.mod_per_bonus( -4 );
    }
}

std::map<bodypart_id, float> Character::bodypart_exposure()
{
    std::map<bodypart_id, float> bp_exposure;
    // May need to iterate over all body parts several times, so make a copy
    const std::vector<bodypart_id> all_body_parts = get_all_body_parts();

    // Initially, all parts are assumed to be fully exposed
    for( const bodypart_id &bp : all_body_parts ) {
        bp_exposure[bp] = 1.0f;
    }
    // For every item worn, for every body part, adjust coverage
    worn.bodypart_exposure( bp_exposure, all_body_parts );
    return bp_exposure;
}

// Linear interpolation between start and end values.
// Constant outside of interval (x_start; x_end)
static float linear_interpolation( float x_start, float y_start, float x_end, float y_end, float x )
{
    if( x < x_start ) {
        return y_start;
    }
    if( x > x_end ) {
        return y_end;
    }
    float interval = x_end - x_start;
    float perc_to_end = ( x - x_start ) / interval;
    return ( 1 - perc_to_end ) * y_start + perc_to_end * y_end;
}

static float light_eff_chance( float exp )
{
    // Sharp increase until 5%
    return linear_interpolation( 0.01, 0.0, 0.05, 1.0, exp );
}

static float medium_eff_chance( float exp )
{
    // Starts at 5%, peaks at 30%
    return linear_interpolation( 0.05, 0.0, 0.55, 0.25, exp );
}

static float heavy_eff_chance( float exp )
{
    // Starts at 15%, increases to 0.1 at 100%
    return linear_interpolation( 0.15, 0.0, 1.0, 0.1, exp );
}

void suffer::from_sunburn( Character &you, bool severe )
{
    // Sunburn effects and albinism/datura occur about once per minute
    if( !one_turn_in( 1_minutes ) ) {
        return;
    }

    // TODO: Could factor bodypart_exposure out of Character too
    std::map<bodypart_id, float> bp_exposure = you.bodypart_exposure();

    enum Sunburn { None, Focus_Loss, Pain, Damage };

    auto heavy_sunburn = [severe, &you]( bodypart_id bp ) {
        if( severe ) {
            // Because hands and feet share an HP pool with arms and legs, and the mouth shares
            // an HP pool with the head, those parts take an unfair share of damage in relation
            // to the torso, which only has one part.  Increase torso damage to balance this.
            if( bp == bodypart_id( "torso" ) ) {
                you.apply_damage( nullptr, bp, 2 );
            } else {
                you.apply_damage( nullptr, bp, 1 );
            }
            return Damage;
        } else {
            you.mod_pain( 1 );
            return Pain;
        }
    };
    auto medium_sunburn = [severe, &you] {
        if( severe )
        {
            you.mod_pain( 1 );
            return Pain;
        } else
        {
            you.mod_focus( -1 );
            return Focus_Loss;
        }
    };
    auto light_sunburn = [severe, &you] {
        if( severe )
        {
            you.mod_focus( -1 );
            return Focus_Loss;
        } else
        {
            return None;
        }
    };

    // Track body parts above the threshold
    std::map<bodypart_id, Sunburn> affected_bodyparts;

    for( auto &bp_exp : bp_exposure ) {
        bodypart_id bp = bp_exp.first;
        float exposure = bp_exp.second;

        if( bp == bodypart_id( "eyes" ) ) {
            // Sunglasses can keep the sun off the eyes.
            if( you.has_flag( json_flag_GLARE_RESIST )
                || you.worn_with_flag( flag_SUN_GLASSES )
                || you.worn_with_flag( flag_BLIND ) ) {
                continue;
            }
            // If no UV-/glare-protection gear is worn the eyes should be treated as unprotected
            exposure = 1.0;
        } else if( ( you.get_wielded_item() && you.get_wielded_item()->has_flag( flag_RAIN_PROTECT ) &&
                     !you.get_wielded_item()->has_flag( flag_TRANSPARENT ) )
                   || ( ( bp == body_part_hand_l || bp == body_part_hand_r )
                        && you.worn_with_flag( flag_POCKETS )
                        && you.can_use_pockets() )
                   || ( bp == body_part_head
                        && you.worn_with_flag( flag_HOOD )
                        && you.can_use_hood() )
                   || ( ( ( bp == body_part_arm_l ) || ( bp == body_part_hand_l ) ) &&
                        you.has_trait( trait_NO_LEFT_ARM ) )
                   || ( ( ( bp == body_part_arm_r ) || ( bp == body_part_hand_r ) ) &&
                        you.has_trait( trait_NO_RIGHT_ARM ) )
                   || ( ( ( bp == body_part_leg_l ) || ( bp == body_part_foot_l ) ) &&
                        you.has_trait( trait_NO_LEFT_LEG ) )
                   || ( ( ( bp == body_part_leg_r ) || ( bp == body_part_foot_r ) ) &&
                        you.has_trait( trait_NO_RIGHT_LEG ) )
                   || ( bp == body_part_mouth
                        && you.worn_with_flag( flag_COLLAR )
                        && you.can_use_collar() ) ) {
            // Eyes suffer even in the presence of the checks in this branch!
            // Umbrellas can keep the sun off all bodyparts, unless you grabbed a clear one
            // Pockets can keep the sun off your hands if you don't wield a too large item
            // Hoods can keep the sun off your unencumbered head
            // Missing limbs can't get sunburned
            // Collars can keep the sun off your unencumbered mouth
            continue;
        }

        float heavy_cumul_chance = heavy_eff_chance( exposure );
        float medium_cumul_chance = heavy_cumul_chance + medium_eff_chance( exposure );
        float light_cumul_chance = medium_cumul_chance + light_eff_chance( exposure );
        float roll = rng_float( 0.0, 1.0 );

        Sunburn eff;
        if( roll < heavy_cumul_chance ) {
            eff = heavy_sunburn( bp );
        } else if( roll < medium_cumul_chance ) {
            eff = medium_sunburn( );
        } else if( roll < light_cumul_chance ) {
            eff = light_sunburn( );
        } else {
            // Do nothing. Assert that exposure is lower than 0.05 as above that point at least light_eff should always happen
            if( exposure > 0.05 ) {
                debugmsg( "No sunburn effect was applied although the bodypart %s is sufficiently exposed at %f exposure",
                          body_part_name( bp ), exposure );
            };
            eff = None;
        }
        affected_bodyparts.emplace( bp, eff );
    }

    std::vector<std::string> affected_part_names;
    std::unordered_set<bodypart_id> excluded_other_parts;

    if( affected_bodyparts.empty() ) {
        return;
    }

    Sunburn worst_effect = None;
    bool contains_eyes = false;

    for( const std::pair<const bodypart_id, Sunburn> &exp_bp : affected_bodyparts ) {
        const bodypart_id &bp = exp_bp.first;
        const Sunburn effect = exp_bp.second;
        if( excluded_other_parts.count( bp ) || effect < worst_effect ) {
            continue;
        }
        if( effect > worst_effect ) {
            worst_effect = effect;
            contains_eyes = false;
            excluded_other_parts.clear();
            affected_part_names.clear();
        }

        const bodypart_id &opposite_bp = bp->opposite_part;
        // If these are different, we have a left/right part like a leg or arm.
        // If same, it's a central body part with no opposite, like head or torso.
        // Used to generate a simpler message when both arms or both legs are affected.
        int count_limbs = 1;
        if( bp == bodypart_id( "eyes" ) ) {
            contains_eyes = true;
        }
        if( bp != opposite_bp ) {
            const auto found = affected_bodyparts.find( opposite_bp );
            // Is opposite part exposed to the same level?
            // If it has a lower exposure, we don't include it in the message.
            // If it has a higher exposure we can immediately skip as this bodypart won't be included in the message.
            if( found != affected_bodyparts.end() ) {
                if( found->second > effect ) {
                    continue;
                } else if( found->second == effect ) {
                    ++count_limbs;
                }
                excluded_other_parts.insert( opposite_bp );
            }
        }
        // Get singular or plural body part name; append "and other body parts" if appropriate
        std::string bp_name = body_part_name( bp, count_limbs );
        affected_part_names.push_back( bp_name );
    }

    const std::string all_parts_list = enumerate_as_string( affected_part_names );

    const int plurality = affected_part_names.size() + contains_eyes;

    auto warn_and_wake_up = [ &you, &all_parts_list]
    ( const char *message, game_message_type type ) {
        you.add_msg_if_player( type, message, all_parts_list );
        // Wake up from skin irritation/burning
        if( you.has_effect( effect_sleep ) ) {
            you.wake_up();
        }
    };

    switch( worst_effect ) {
        case Damage:
            //~ %s is a list of body parts.  The plurality integer is the total
            //~ number of body parts (eyes count as 2 body parts)
            //~ This message indicates damage to bodyparts through sunshine
            warn_and_wake_up( n_gettext( "Your %s is bathed in sunlight.  It feels like it is burning up.",
                                         "Your %s are bathed in sunlight.  They feel like they are burning up.",
                                         plurality ),
                              m_bad );
            break;
        case Pain:
            //~ %s is a list of body parts.  The plurality integer is the total
            //~ number of body parts (eyes count as 2 body parts)
            //~ This message indicates pain through sunshine
            warn_and_wake_up( n_gettext( "The sunlight burns on your %s.",
                                         "The sunlight burns on your %s.",
                                         plurality ),
                              m_bad );
            break;
        case Focus_Loss:
            //~ %s is a list of body parts.  The plurality integer is the total
            //~ number of body parts (eyes count as 2 body parts)
            //~ This message indicates focus loss through sunshine
            warn_and_wake_up( n_gettext( "The sunlight on your %s irritates you.",
                                         "The sunlight on your %s irritates you.",
                                         plurality ),
                              m_bad );
            break;
        case None:
            break;
    }
}

void suffer::from_item_dropping( Character &you )
{
    if( you.has_effect( effect_incorporeal ) ) {
        std::vector<item *> dump = you.inv_dump();
        std::list<item> tumble_items;
        for( item *dump_item : dump ) {
            if( !dump_item->has_flag( flag_NO_UNWIELD ) && !dump_item->has_flag( flag_NO_TAKEOFF ) &&
                !dump_item->has_flag( flag_INTEGRATED ) ) {
                tumble_items.push_back( *dump_item );
            }
        }
        put_into_vehicle_or_drop( you, item_drop_reason::tumbling, tumble_items );
        for( item *i : dump ) {
            you.i_rem( i );
        }
    }
}

void suffer::from_other_mutations( Character &you )
{
    map &here = get_map();
    const tripoint position = you.pos();
    if( you.has_trait( trait_SHARKTEETH ) && one_turn_in( 24_hours ) ) {
        you.add_msg_if_player( m_neutral, _( "You shed a tooth!" ) );
        here.spawn_item( position, "bone", 1 );
    }

    if( you.has_trait( trait_WINGS_INSECT_active ) ) {
        //~Sound of buzzing Insect Wings
        sounds::sound( position, 10, sounds::sound_t::movement, _( "BZZZZZ" ), false, "misc",
                       "insect_wings" );
    }

    if( you.has_trait( trait_SORES ) ) {
        for( const bodypart_id &bp : you.get_all_body_parts() ) {
            if( bp == bodypart_id( "head" ) ) {
                continue;
            }
            int sores_pain = 5 + 0.4 * std::abs( you.encumb( bp ) );
            if( you.get_pain() < sores_pain ) {
                you.set_pain( sores_pain );
            }
        }
    }
    //Web Weavers...weave web
    if( you.has_active_mutation( trait_WEB_WEAVER ) && !you.in_vehicle ) {
        // this adds intensity to if its not already there.
        here.add_field( position, fd_web, 1 );

    }

    if( you.has_active_mutation( trait_SNAIL_TRAIL ) && !you.in_vehicle ) {
        here.add_field( position, fd_sludge, 1 );

    }

    // Blind/Deaf for brief periods about once an hour,
    // and visuals about once every 30 min.
    if( you.has_trait( trait_PER_SLIME ) ) {
        if( one_turn_in( 1_hours ) && !you.has_effect( effect_deaf ) ) {
            you.add_msg_if_player( m_bad, _( "Suddenly, you can't hear anything!" ) );
            you.add_effect( effect_deaf, rng( 20_minutes, 60_minutes ) );
        }
        if( one_turn_in( 1_hours ) && !you.has_effect( effect_blind ) ) {
            you.add_msg_if_player( m_bad, _( "Suddenly, your eyes stop working!" ) );
            you.add_effect( effect_blind, rng( 2_minutes, 6_minutes ) );
        }
        // Yes, you can be blind and hallucinate at the same time.
        // Your post-human biology is truly remarkable.
        if( one_turn_in( 30_minutes ) && !you.has_effect( effect_visuals ) ) {
            you.add_msg_if_player( m_bad, _( "Your visual centers must be acting up…" ) );
            you.add_effect( effect_visuals, rng( 36_minutes, 72_minutes ) );
        }
    }

    if( you.has_trait( trait_WEB_SPINNER ) && !you.in_vehicle && one_in( 3 ) ) {
        // this adds intensity to if its not already there.
        here.add_field( position, fd_web, 1 );
    }

    if( you.has_trait( trait_MUCUS_SECRETION ) && !you.in_vehicle && one_in( 2033 ) ) {
        here.add_field( position, fd_sludge, 1 );
    }

    bool should_mutate = you.has_trait( trait_UNSTABLE ) && !you.has_trait( trait_CHAOTIC_BAD ) &&
                         one_turn_in( 48_hours );
    should_mutate |= ( you.has_trait( trait_CHAOTIC ) || you.has_trait( trait_CHAOTIC_BAD ) ) &&
                     one_turn_in( 12_hours );
    if( should_mutate ) {
        you.mutate();
    }

    const bool needs_fire = !you.has_morale( MORALE_PYROMANIA_NEARFIRE ) &&
                            !you.has_morale( MORALE_PYROMANIA_STARTFIRE );
    if( you.has_trait( trait_PYROMANIA ) && needs_fire && !you.in_sleep_state() &&
        calendar::once_every( 2_hours ) ) {
        you.add_morale( MORALE_PYROMANIA_NOFIRE, -1, -30, 24_hours, 24_hours, true );
        if( calendar::once_every( 4_hours ) ) {
            const translation smokin_hot_fiyah =
                SNIPPET.random_from_category( "pyromania_withdrawal" ).value_or( translation() );
            you.add_msg_if_player( m_bad, "%s", smokin_hot_fiyah );
        }
    }
    if( you.has_trait( trait_KILLER ) && !you.has_morale( MORALE_KILLER_HAS_KILLED ) &&
        calendar::once_every( 2_hours ) ) {
        you.add_morale( MORALE_KILLER_NEED_TO_KILL, -1, -30, 24_hours, 24_hours );
        if( calendar::once_every( 4_hours ) ) {
            const translation snip = SNIPPET.random_from_category( "killer_withdrawal" ).value_or(
                                         translation() );
            you.add_msg_if_player( m_bad, "%s", snip );
        }
    }
}

void suffer::from_radiation( Character &you )
{
    map &here = get_map();
    // get radioactive leak level of your inventory
    float item_radiation = you.get_leak_level();
    const int map_radiation = here.get_radiation( you.pos() );
    float rads = map_radiation / 100.0f + item_radiation / 10.0f;

    int rad_mut = 0;
    if( you.has_trait( trait_RADIOACTIVE3 ) ) {
        rad_mut = 3;
    } else if( you.has_trait( trait_RADIOACTIVE2 ) ) {
        rad_mut = 2;
    } else if( you.has_trait( trait_RADIOACTIVE1 ) ) {
        rad_mut = 1;
    }

    // Spread less radiation when sleeping (slower metabolism etc.)
    // Otherwise it can quickly get to the point where you simply can't sleep at all
    const bool rad_mut_proc = rad_mut > 0 && x_in_y( rad_mut, to_turns<int>( you.in_sleep_state() ?
                              3_hours : 30_minutes ) );

    bool has_helmet = false;
    const bool power_armored = you.is_wearing_power_armor( &has_helmet );
    const bool rad_resist = power_armored || you.worn_with_flag( flag_RAD_RESIST );

    if( rad_mut > 0 ) {
        const bool kept_in = you.is_rad_immune() || ( rad_resist && !one_in( 4 ) );
        if( kept_in ) {
            // As if standing on a map tile with radiation level equal to rad_mut
            rads += rad_mut / 100.0f;
        }

        if( rad_mut_proc && !kept_in ) {
            // Irradiate a random nearby point
            // If you can't, irradiate the player instead
            tripoint rad_point = you.pos() + point( rng( -3, 3 ), rng( -3, 3 ) );
            // TODO: Radioactive vehicles?
            if( here.get_radiation( rad_point ) < rad_mut ) {
                here.adjust_radiation( rad_point, 1 );
            } else {
                rads += rad_mut;
            }
        }
    }

    // Used to control vomiting from radiation to make it not-annoying
    bool radiation_increasing = you.irradiate( rads );

    if( radiation_increasing && calendar::once_every( 3_minutes ) ) {
        if( you.has_bionic( bio_geiger ) ) {
            you.add_msg_if_player( m_warning,
                                   _( "You feel an anomalous sensation coming from "
                                      "your radiation sensors." ) );
        } else if( you.has_active_mutation( trait_RADIOPHILE ) ) {
            you.add_msg_if_player( m_warning,
                                   _( "Your flesh tingles with an air of danger, "
                                      "yet it is strangely pleasurable." ) );
        }
    }

    if( calendar::once_every( 15_minutes ) ) {
        if( you.get_rad() < 0 ) {
            you.set_rad( 0 );
        } else if( you.get_rad() > 2000 ) {
            you.set_rad( 2000 );
        }
        // Linear increase in chance to mutate with irriadation level.
        // 100 rads = 1 / 10000
        // 110 rads = 10 / 10000 = 1 / 1000
        // 200 rads = 100 / 10000 = 1 / 100
        // 1000 rads = 900 / 10000 = 9 / 100 = 10% !!!
        // 2000 rads = 2000 / 10000 = 1 / 5 = 20% !!!
        if( get_option<bool>( "RAD_MUTATION" ) && rng( 100, 10000 ) < you.get_rad() ) {
            you.mutate();
        }
        if( you.get_rad() > 50 && rng( 1, 3000 ) < you.get_rad() &&
            ( you.stomach.contains() > 0_ml || radiation_increasing || !you.in_sleep_state() ) ) {
            you.vomit();
            you.mod_rad( -1 );
        }
    }

    const bool radiogenic = you.has_trait( trait_RADIOGENIC );
    if( radiogenic && calendar::once_every( 30_minutes ) && you.get_rad() > 0 ) {
        // At 200 irradiation, twice as fast as REGEN
        if( x_in_y( you.get_rad(), 200 ) ) {
            you.healall( 1 );
            if( rad_mut == 0 ) {
                // Don't heal radiation if we're generating it naturally
                // That would counter the main downside of radioactivity
                you.mod_rad( -5 );
            }
        }
    }

    if( calendar::once_every( 1_days ) ) {
        if( !radiogenic && you.get_rad() > 0 ) {
            you.mod_daily_health( -you.get_rad(), -200 );
        }
    }

    if( you.get_rad() > 200 && calendar::once_every( 10_minutes ) && x_in_y( you.get_rad(), 1000 ) ) {
        you.hurtall( 1, nullptr );
        you.mod_rad( -5 );
    }

    while( you.slow_rad >= 1000 ) {
        you.mod_rad( 1 );
        you.slow_rad -= 1000;
    }
}

void suffer::from_bad_bionics( Character &you )
{
    // Negative bionics effects
    if( you.has_bionic( bio_synlungs ) && !you.has_active_bionic( bio_synlungs ) &&
        !you.has_effect( effect_narcosis ) ) {
        if( you.get_power_level() >= bio_synlungs->power_trigger ) {
            std::optional<bionic *> bio_opt = you.find_bionic_by_type( bio_synlungs );
            you.activate_bionic( **bio_opt );
        } else {
            you.mod_stamina( -2000 );
            you.add_msg_if_player( m_bad, _( "Emergency!  User's lungs are not powered!" ) );
        }
    }
    if( you.has_bionic( bio_dis_shock ) && you.get_power_level() > bio_dis_shock->power_trigger &&
        one_turn_in( 2_hours ) &&
        !you.has_effect( effect_narcosis ) ) {
        if( !you.has_flag( json_flag_PAIN_IMMUNE ) ) {
            you.add_msg_if_player( m_bad, _( "You suffer a painful electrical discharge!" ) );
            you.mod_pain( 1 );
        } else {
            you.add_msg_if_player( m_bad, _( "You experience an electrical discharge!" ) );
        }
        you.moves -= 150;
        you.mod_power_level( -bio_dis_shock->power_trigger );

        item_location weapon = you.get_wielded_item();
        if( weapon && weapon->typeId() == itype_e_handcuffs && weapon->charges > 0 ) {
            weapon->charges -= rng( 1, 3 ) * 50;
            if( weapon->charges < 1 ) {
                weapon->charges = 1;
            }

            you.add_msg_if_player( m_good, _( "The %s seems to be affected by the discharge." ),
                                   weapon->tname() );
        }
        sfx::play_variant_sound( "bionics", "elec_discharge", 100 );
    }
    if( you.has_bionic( bio_dis_acid ) && one_turn_in( 150_minutes ) ) {
        if( !you.has_flag( json_flag_PAIN_IMMUNE ) ) {
            you.add_msg_if_player( m_bad, _( "You suffer a burning acidic discharge!" ) );
        } else {
            you.add_msg_if_player( m_bad, _( "You experience an acidic discharge!" ) );
        }
        you.hurtall( 1, nullptr );
        sfx::play_variant_sound( "bionics", "acid_discharge", 100 );
        sfx::do_player_death_hurt( get_player_character(), false );
    }
    if( you.has_bionic( bio_power_weakness ) && you.has_max_power() &&
        you.get_power_level() >= you.get_max_power_level() * .75 ) {
        you.mod_str_bonus( -3 );
    }
    if( you.has_bionic( bio_radleak ) && one_turn_in( 300_minutes ) ) {
        you.add_msg_if_player( m_bad, _( "You CBM leaks radiation." ) );
        if( you.has_effect( effect_iodine ) ) {
            you.mod_rad( 2 );
        } else {
            you.mod_rad( 5 );
        }
    }
}

void suffer::from_stimulants( Character &you, const int current_stim )
{
    // Stim +250 kills
    if( current_stim > 210 ) {
        if( one_turn_in( 2_minutes ) && !you.is_on_ground() ) {
            you.add_msg_if_player( m_bad, _( "Your muscles spasm!" ) );
            if( !you.is_on_ground() ) {
                you.add_msg_if_player( m_bad, _( "You fall to the ground!" ) );
                you.add_effect( effect_downed, rng( 6_turns, 20_turns ) );
            }
        }
    }
    if( current_stim > 170 ) {
        if( !you.has_effect( effect_winded ) && calendar::once_every( 10_minutes ) ) {
            you.add_msg_if_player( m_bad, _( "You feel short of breath." ) );
            you.add_effect( effect_winded, 10_minutes + 1_turns );
        }
    }
    if( current_stim > 110 ) {
        if( !you.has_effect( effect_shakes ) && calendar::once_every( 10_minutes ) ) {
            you.add_msg_if_player( _( "You shake uncontrollably." ) );
            you.add_effect( effect_shakes, 15_minutes + 1_turns );
        }
    }
    if( current_stim > 75 ) {
        if( !one_turn_in( 2_minutes ) && !you.has_effect( effect_nausea ) ) {
            you.add_msg_if_player( _( "You feel nauseous…" ) );
            you.add_effect( effect_nausea, 5_minutes );
        }
    }

    //stim -200 or painkillers 240 kills
    if( current_stim < -160 || you.get_painkiller() > 200 ) {
        if( one_turn_in( 3_minutes ) && !you.in_sleep_state() ) {
            you.add_msg_if_player( m_bad, _( "You black out!" ) );
            const time_duration dur = rng( 30_minutes, 60_minutes );
            if( !you.is_on_ground() ) {
                you.add_effect( effect_downed, dur );
            }
            you.add_effect( effect_blind, dur );
            you.fall_asleep( dur );
        }
    }
    if( current_stim < -120 || you.get_painkiller() > 160 ) {
        if( !you.has_effect( effect_winded ) && calendar::once_every( 10_minutes ) ) {
            you.add_msg_if_player( m_bad, _( "Your breathing slows down." ) );
            you.add_effect( effect_winded, 10_minutes + 1_turns );
        }
    }
    if( current_stim < -85 || you.get_painkiller() > 145 ) {
        if( one_turn_in( 15_seconds ) && !you.has_effect( effect_sleep ) ) {
            you.add_msg_if_player( m_bad, _( "You feel dizzy for a moment." ) );
            you.mod_moves( -rng( 10, 30 ) );
            if( one_in( 3 ) && !you.is_on_ground() ) {
                you.add_msg_if_player( m_bad, _( "You stumble and fall over!" ) );
                you.add_effect( effect_downed, rng( 3_turns, 10_turns ) );
            }
        }
    }
    if( current_stim < -60 || you.get_painkiller() > 130 ) {
        if( calendar::once_every( 10_minutes ) ) {
            you.add_msg_if_player( m_warning, _( "You feel tired…" ) );
            you.mod_fatigue( rng( 1, 2 ) );
        }
    }
}

static void apply_weary_message( const Character &you, int level, int old )
{
    if( level - old > 0 ) {
        std::string msg;
        switch( level ) {
            case 1:
                msg = _( "You're beginning to feel like you've exerted yourself a bit." );
                break;
            case 2:
                msg = _( "You're tiring out mildly, and slowing down as a result." );
                break;
            case 3:
                msg = _( "The day's labors are taking their toll, and slowing you down." );
                break;
            case 4:
                msg = _( "You're getting very tired from all this hard work." );
                break;
            case 5:
            default:
                msg = _( "You're exhausted." );
                break;
        }
        you.add_msg_if_player( m_info, msg );
    } else {
        you.add_msg_if_player( m_good, _( "You're feeling a bit better rested from your exertions." ) );
    }
}

static void apply_weariness( Character &you, int level, int old )
{
    // Exertion cannot be negative!
    if( level < 0 || old < 0 ) {
        debugmsg( "Attempted to apply weariness to character with negative ( new: %d old: %d ) weariness",
                  level, old );
    }
    // A mapping of weariness level to the effect to be applied
    static const std::array<efftype_id, 9> weary_effects { {
            effect_weary_0,
            effect_weary_1,
            effect_weary_2,
            effect_weary_3,
            effect_weary_4,
            effect_weary_5,
            effect_weary_6,
            effect_weary_7,
            effect_weary_8,
        }};

    // If we're going above level 8, we're seriously messed up
    // So just stick to level 8
    if( level > 8 ) {
        level = 8;
    }
    if( old > 8 ) {
        old = 8;
    }

    /*
     * It's possible that old == level, but we need to make sure we're removing
     * the old effect when that's not true It's simple enough to just remove
     * the old effect then apply the new, effect, because no change will happen
     * if they're equal
     */
    you.remove_effect( weary_effects[old] );
    you.add_effect( weary_effects[level], 1_turns, true );
    apply_weary_message( you, level, old );
}

void suffer::from_exertion( Character &you )
{
    int new_weary_level = you.weariness_level();
    float max_activity = you.maximum_exertion_level();

    // Only if there are changes (duh)
    if( new_weary_level != you.old_weary_level ) {
        apply_weariness( you, new_weary_level, you.old_weary_level );
    }

    // Significantly slow the rate of messaging when in an activity
    const int chance = you.activity ? to_turns<int>( 48_minutes ) : to_turns<int>( 5_minutes );
    if( !you.in_sleep_state() && you.activity_history.activity( false ) > max_activity &&
        one_in( chance ) ) {
        you.add_msg_if_player( m_bad,
                               _( "You're tiring out; continuing to work at this rate will be slower." ) );
    }

    // This must happen at the end, for hopefully obvious reasons
    you.old_weary_level = new_weary_level;
}

void suffer::without_sleep( Character &you, const int sleep_deprivation )
{
    // redo as a snippet?
    if( sleep_deprivation >= SLEEP_DEPRIVATION_HARMLESS ) {
        if( one_turn_in( 50_minutes ) ) {
            switch( dice( 1, 4 ) ) {
                default:
                case 1:
                    you.add_msg_player_or_npc( m_warning, _( "You tiredly rub your eyes." ),
                                               _( "<npcname> tiredly rubs their eyes." ) );
                    break;
                case 2:
                    you.add_msg_player_or_npc( m_warning, _( "You let out a small yawn." ),
                                               _( "<npcname> lets out a small yawn." ) );
                    break;
                case 3:
                    you.add_msg_player_or_npc( m_warning, _( "You stretch your back." ),
                                               _( "<npcname> stretches their back." ) );
                    break;
                case 4:
                    you.add_msg_player_or_npc( m_warning, _( "You feel mentally tired." ),
                                               _( "<npcname> lets out a huge yawn." ) );
                    break;
            }
        }
    }
    // Minor discomfort
    if( sleep_deprivation >= SLEEP_DEPRIVATION_MINOR ) {
        if( one_turn_in( 75_minutes ) ) {
            you.add_msg_if_player( m_warning, _( "You feel lightheaded for a moment." ) );
            you.moves -= 10;
        }
        if( one_turn_in( 100_minutes ) ) {
            you.add_msg_if_player( m_warning, _( "Your muscles spasm uncomfortably." ) );
            you.mod_pain( 2 );
        }
        if( !you.has_effect( effect_visuals ) && one_turn_in( 150_minutes ) ) {
            you.add_msg_if_player( m_warning, _( "Your vision blurs a little." ) );
            you.add_effect( effect_visuals, rng( 1_minutes, 5_minutes ) );
        }
    }
    // Slight disability
    if( sleep_deprivation >= SLEEP_DEPRIVATION_SERIOUS ) {
        if( one_turn_in( 75_minutes ) ) {
            you.add_msg_if_player( m_bad, _( "Your mind lapses into unawareness briefly." ) );
            you.moves -= rng( 20, 80 );
        }
        if( one_turn_in( 125_minutes ) ) {
            you.add_msg_if_player( m_bad, _( "Your muscles ache in stressfully unpredictable ways." ) );
            you.mod_pain( rng( 2, 10 ) );
        }
        if( one_turn_in( 5_hours ) ) {
            you.add_msg_if_player( m_bad, _( "You have a distractingly painful headache." ) );
            you.mod_pain( rng( 10, 25 ) );
        }
    }
    // Major disability, high chance of passing out also relevant
    if( sleep_deprivation >= SLEEP_DEPRIVATION_MAJOR ) {
        if( !you.has_effect( effect_nausea ) && one_turn_in( 500_minutes ) ) {
            you.add_msg_if_player( m_bad, _( "You feel heartburn and an acid taste in your mouth." ) );
            you.mod_pain( 5 );
            you.add_effect( effect_nausea, rng( 5_minutes, 30_minutes ) );
        }
        if( one_turn_in( 5_hours ) ) {
            you.add_msg_if_player( m_bad, _( "Your mind is so tired that you feel you can't trust "
                                             "your eyes anymore." ) );
            you.add_effect( effect_hallu, rng( 5_minutes, 60_minutes ) );
        }
        if( !you.has_effect( effect_shakes ) && one_turn_in( 425_minutes ) ) {
            you.add_msg_if_player( m_bad, _( "Your muscles spasm uncontrollably, and you have "
                                             "trouble keeping your balance." ) );
            you.add_effect( effect_shakes, 15_minutes );
        } else if( you.has_effect( effect_shakes ) && one_turn_in( 75_seconds ) ) {
            you.moves -= 10;
            you.add_msg_player_or_npc( m_warning, _( "Your shaking legs make you stumble." ),
                                       _( "<npcname> stumbles." ) );
            if( !you.is_on_ground() && one_in( 10 ) ) {
                you.add_msg_player_or_npc( m_bad, _( "You fall over!" ), _( "<npcname> falls over!" ) );
                you.add_effect( effect_downed, rng( 3_turns, 10_turns ) );
            }
        }
    }
}

void suffer::from_tourniquet( Character &you )
{
    // shortcut for the most common scenario.
    if( !you.worn_with_flag( flag_TOURNIQUET ) ) {
        return;
    }
    for( const bodypart_id &bp : you.get_all_body_parts( get_body_part_flags::only_main ) ) {
        if( you.worn_with_flag( flag_TOURNIQUET, bp ) && one_turn_in( 30_seconds ) ) {
            you.mod_pain( 1 );
            you.apply_damage( nullptr, bp, 1, true );
            you.add_msg_player_or_npc( m_bad, _( "Your tourniquet hurts you." ),
                                       _( "<npcname> is hurting from the tourniquet." ) );
        }
    }
}

void suffer::from_nyctophobia( Character &you )
{
    const float nyctophobia_threshold = LIGHT_AMBIENT_LIT - 3.0f;

    const bool in_darkness = get_map().ambient_light_at( you.pos() ) < nyctophobia_threshold;
    if( in_darkness ) {
        if( one_in( 80 ) && !you.has_effect( effect_shakes ) ) {
            you.add_msg_if_player( m_bad,
                                   _( "Your fear of the dark is so intense that your hands start shaking uncontrollably." ) );
            you.add_effect( effect_shakes, rng( 1_minutes, 3_minutes ) );

            return;
        }

        if( one_in( 80 ) ) {
            you.add_msg_if_player( m_bad,
                                   _( "Your fear of the dark is so intense that you start breathing rapidly, and you feel like your heart is ready to jump out of your chest." ) );
            you.mod_stamina( -500 * rng( 1, 3 ) );

            return;
        }

        if( one_turn_in( 5_minutes ) ) {
            you.add_msg_if_player( m_bad, _( "You feel a twinge of panic as darkness engulfs you." ) );
        }

    }
}

void suffer::from_artifact_resonance( Character &you, int amt )
{
    int rng_outcome;
    if( rng( 0, 3600 + std::floor( amt / 2000 ) ) > 3600 ) {
        if( amt > 12500 && one_in( 2 ) ) {
            //deadly effects from way too high resonance
            rng_outcome = rng( 1, 3 );
            if( rng_outcome == 1 ) {
                you.add_msg_player_or_npc( m_bad, _( "You attract the attention of something horrible." ),
                                           _( "<npcname> attracts the attention of something horrible." ) );
                map &here = get_map();
                for( const tripoint &dest : here.points_in_radius( you.pos(), 12 ) ) {
                    if( here.is_cornerfloor( dest ) ) {
                        here.add_field( dest, fd_tindalos_rift, 3 );
                        add_msg( m_info, _( "You hear a low-pitched echoing howl." ) );
                    }
                }
            } else if( rng_outcome == 2 ) {
                you.add_msg_player_or_npc( m_bad, _( "Reality gives way under your feet like rotten scaffolding." ),
                                           _( "Reality gives way under <npcname>'s feet like rotten scaffolding." ) );
                map &here = get_map();
                here.add_field( you.pos(), fd_fatigue, 1 );
            } else if( rng_outcome == 3 ) {
                you.add_msg_player_or_npc( m_bad, _( "You suddenly lose all substance and corporeality." ),
                                           _( "<npcname> suddenly loses all substance and corporeality." ) );
                you.add_effect( effect_incorporeal, 1_minutes );
            }
        } else if( amt > 7500 && one_in( 2 ) ) {
            //severe effects from very high resonance
            rng_outcome = rng( 1, 3 );
            if( rng_outcome == 1 ) {
                you.add_msg_player_or_npc( m_bad, _( "You are suddenly beset with agonizing, unbearable pain." ),
                                           _( "<npcname> suddenly cries out in agony." ) );
                you.mod_pain( 100 );
                you.shout();
            } else if( rng_outcome == 2 ) {
                you.add_msg_player_or_npc( m_bad, _( "The air folds and distorts around you." ),
                                           _( "The air folds and distorts around <npcname>." ) );
                teleport::teleport( you );
            } else if( rng_outcome == 3 ) {
                you.add_msg_player_or_npc( m_bad, _( "You're bombarded with radioactive energy!" ),
                                           _( "<npcname> is bombarded with radioactive energy!" ) );
                you.irradiate( 5, true );
                you.irradiate( 45, false );
            }
        } else if( amt > 4500 && one_in( 2 ) ) {
            //bad effects from moderately high resonance
            rng_outcome = rng( 1, 3 );
            if( rng_outcome == 1  && !you.in_vehicle ) {
                you.add_msg_player_or_npc( m_bad, _( "You suddenly shift slightly." ),
                                           _( "<npcname> suddenly shifts slightly." ) );
                teleport::teleport( you, 1, 1, true, false );
            } else if( rng_outcome == 2 ) {
                you.add_msg_player_or_npc( m_bad,
                                           _( "You hear a painfully loud grinding noise from your location." ),
                                           _( "A painfully loud grinding noise suddenly blares from the location of <npcname>." ) );
                sounds::sound( you.pos(), 5000, sounds::sound_t::movement, _( "A horribly loud grinding sound!" ),
                               true, "misc", "scraping" );
            } else if( rng_outcome == 3 ) {
                you.add_msg_player_or_npc( m_bad,
                                           _( "The air suddenly crackles around you." ),
                                           _( "The air suddenly crackles around <npcname>." ) );
                you.irradiate( 10, false );
            }
        } else {
            //mild effects from somewhat high resonance
            rng_outcome = rng( 1, 3 );
            if( rng_outcome == 1 ) {
                you.add_msg_if_player( m_bad,
                                       _( "You suddenly feel a sharp, stabbing pain with no apparent source." ) );
                you.mod_pain( 5 );
            } else if( rng_outcome == 2 ) {
                you.add_msg_if_player( m_bad,
                                       _( "Your vision becomes blurry and you suddenly feel like you're falling." ) );
                you.add_effect( effect_visuals, 5_minutes );
            } else if( rng_outcome == 3 ) {
                you.add_msg_if_player( m_bad, _( "You suddenly feel very queasy." ) );
                you.add_effect( effect_nausea, 5_minutes );
            }
        }
    }
}

void Character::suffer()
{
    const int current_stim = get_stim();

    for( const bodypart_id &bp : get_all_body_parts( get_body_part_flags::only_main ) ) {
        if( is_limb_broken( bp ) ) {
            add_effect( effect_disabled, 1_turns, bp, true );
        }
    }

    for( bionic &bio : *my_bionics ) {
        process_bionic( bio );
    }

    for( const trait_id &mut_id : get_mutations() ) {
        if( calendar::once_every( 1_minutes ) && mut_id->weakness_to_water != 0 ) {
            suffer::water_damage( *this, mut_id );
        }
        if( has_active_mutation( mut_id ) || ( !mut_id->activated && !mut_id->processed_eocs.empty() ) ) {
            suffer::mutation_power( *this, mut_id );
        }
    }

    if( has_flag( json_flag_GRAB ) ) {
        suffer::while_grabbed( *this );
    }

    if( underwater ) {
        suffer::while_underwater( *this );
    }

    suffer::from_addictions( *this );

    if( !in_sleep_state() ) {
        suffer::while_awake( *this, current_stim );
    } // Done with while-awake-only effects

    if( has_trait( trait_ASTHMA ) ) {
        suffer::from_asthma( *this, current_stim );
    }

    suffer::in_sunlight( *this, worn );
    suffer::from_exertion( *this );
    suffer::from_item_dropping( *this );
    suffer::from_other_mutations( *this );
    suffer::from_radiation( *this );
    suffer::from_bad_bionics( *this );
    suffer::from_stimulants( *this, current_stim );
    int sleep_deprivation = in_sleep_state() ? 0 : get_sleep_deprivation();
    // Stimulants can lessen the PERCEIVED effects of sleep deprivation, but
    // they do nothing to cure it. As such, abuse is even more dangerous now.
    if( current_stim > 0 ) {
        // 100% of blacking out = 20160sd ; Max. stim modifier = 12500sd @ 250stim
        // Note: Very high stim already has its own slew of bad effects,
        // so the "useful" part of this bonus is actually lower.
        sleep_deprivation -= current_stim * 50;
    }

    suffer::without_sleep( *this, sleep_deprivation );
    suffer::from_tourniquet( *this );
    //Suffer from enchantments
    enchantment_cache->activate_passive( *this );
    if( calendar::once_every( 30_minutes ) ) {
        int healthy_mod = enchantment_cache->modify_value( enchant_vals::mod::MOD_HEALTH, 0 );
        int healthy_mod_cap = enchantment_cache->modify_value( enchant_vals::mod::MOD_HEALTH_CAP, 0 );
        mod_daily_health( healthy_mod, healthy_mod_cap );
    }
}

bool Character::irradiate( float rads, bool bypass )
{
    int rad_mut = 0;
    if( has_trait( trait_RADIOACTIVE3 ) ) {
        rad_mut = 3;
    } else if( has_trait( trait_RADIOACTIVE2 ) ) {
        rad_mut = 2;
    } else if( has_trait( trait_RADIOACTIVE1 ) ) {
        rad_mut = 1;
    }

    if( rads > 0 ) {
        bool has_helmet = false;
        const bool power_armored = is_wearing_power_armor( &has_helmet );
        const bool rad_resist = power_armored || worn_with_flag( flag_RAD_RESIST );

        if( is_rad_immune() && !bypass ) {
            // Power armor and some high-tech gear protects completely from radiation
            rads = 0.0f;
        } else if( rad_resist && !bypass ) {
            rads /= 4.0f;
        }

        if( has_effect( effect_iodine ) ) {
            // Radioactive mutation makes iodine less efficient (but more useful)
            rads *= 0.3f + 0.1f * rad_mut;
        }

        int rads_max = roll_remainder( rads );
        mod_rad( rng( 0, rads_max ) );

        // Apply rads to any radiation badges that are exposed (worn or wielded)
        for( item *const it : inv_dump() ) {
            if( !it->has_flag( json_flag_RAD_DETECT ) ) {
                continue;
            }

            // Actual irradiation levels of badges and the player aren't precisely matched.
            // This is intentional.
            const int before = it->irradiation;

            const int delta = rng( 0, rads_max );
            if( delta == 0 ) {
                continue;
            }

            it->irradiation += delta;

            // If in inventory (not worn), don't print anything.
            if( inv->has_item( *it ) ) {
                continue;
            }

            // If the color hasn't changed, don't print anything.
            const std::string &col_before = rad_badge_color( before ).first;
            const std::string &col_after = rad_badge_color( it->irradiation ).first;
            if( col_before == col_after ) {
                continue;
            }

            //~ %1$s = previous badge color, %2%s = current badge color
            add_msg_if_player( m_bad, _( "Your radiation badge changes from %1$s to %2$s!" ),
                               col_before, col_after );
        }

        if( rads > 0.0f ) {
            return true;
        }
    }
    return false;
}

void Character::mend( int rate_multiplier )
{
    // Wearing splints can slowly mend a broken limb back to 1 hp.
    bool any_broken = false;
    for( const bodypart_id &bp : get_all_body_parts() ) {
        if( is_limb_broken( bp ) ) {
            any_broken = true;
            break;
        }
    }

    if( !any_broken ) {
        return;
    }

    double healing_factor = 1.0;
    // Studies have shown that alcohol and tobacco use delay fracture healing time
    // Being under effect is 50% slowdown
    // Being addicted but not under effect scales from 25% slowdown to 75% slowdown
    // The improvement from being intoxicated over withdrawal is intended
    if( has_effect( effect_cig ) ) {
        healing_factor *= 0.5;
    } else {
        healing_factor *= addiction_scaling( 0.25f, 0.75f, addiction_level( addiction_nicotine ) );
    }

    if( has_effect( effect_drunk ) ) {
        healing_factor *= 0.5;
    } else {
        healing_factor *= addiction_scaling( 0.25f, 0.75f, addiction_level( addiction_alcohol ) );
    }

    if( get_rad() > 0 && !has_trait( trait_RADIOGENIC ) ) {
        healing_factor *= clamp( ( 1000.0f - get_rad() ) / 1000.0f, 0.0f, 1.0f );
    }

    // Bed rest speeds up mending
    if( has_effect( effect_sleep ) ) {
        healing_factor *= 4.0;
    } else if( get_fatigue() > fatigue_levels::DEAD_TIRED ) {
        // but being dead tired does not...
        healing_factor *= 0.75;
    } else {
        // If not dead tired, resting without sleep also helps
        healing_factor *= 1.0f + rest_quality();
    }

    // Being healthy helps.
    healing_factor *= 1.0f + get_lifestyle() / 200.0f;

    // Very hungry starts lowering the chance
    // square rooting the value makes the numbers drop off faster when below 1
    healing_factor *= std::sqrt( static_cast<float>( get_stored_kcal() ) / static_cast<float>
                                 ( get_healthy_kcal() ) );
    // Similar for thirst - starts at very thirsty, drops to 0 ~halfway between two last statuses
    healing_factor *= 1.0f - clamp( ( get_thirst() - 80.0f ) / 300.0f, 0.0f, 1.0f );

    // Mutagenic healing factor!
    bool needs_splint = true;

    healing_factor *= mutation_value( "mending_modifier" );

    add_msg_debug( debugmode::DF_CHAR_HEALTH, "Limb mend healing factor: %.2f", healing_factor );
    if( healing_factor <= 0.0f ) {
        // The section below assumes positive healing rate
        return;
    }

    for( const bodypart_id &bp : get_all_body_parts() ) {
        const bool broken = is_limb_broken( bp );
        needs_splint = !has_flag( json_flag_MEND_ALL );
        if( !broken ) {
            continue;
        }

        if( bp->has_flag( json_flag_MEND_LIMB ) ) {
            needs_splint = false;
        }

        if( needs_splint && !worn_with_flag( flag_SPLINT,  bp ) ) {
            continue;
        }

        healing_factor *= bp->mend_rate;

        const time_duration dur_inc = 1_turns * roll_remainder( rate_multiplier * healing_factor );
        effect &eff = get_effect( effect_mending, bp );
        if( eff.is_null() ) {
            add_effect( effect_mending, dur_inc, bp, true );
            continue;
        }

        eff.set_duration( eff.get_duration() + dur_inc );

        if( eff.get_duration() >= eff.get_max_duration() ) {
            set_part_hp_cur( bp, 1 );
            remove_effect( effect_mending, bp );
            get_event_bus().send<event_type::broken_bone_mends>( getID(), bp );
            //~ %s is bodypart
            add_msg_if_player( m_good, _( "Your %s has started to mend!" ),
                               body_part_name( bp ) );
        }
    }
}

void Character::drench( int saturation, const body_part_set &flags, bool ignore_waterproof )
{
    bool in_shell = has_active_mutation( trait_SHELL2 ) ||
                    has_active_mutation( trait_SHELL3 );
    if( saturation < 1 ) {
        return;
    }

    // OK, water gets in your AEP suit or whatever.  It wasn't built to keep you dry.
    if( has_trait( trait_DEBUG_NOTEMP ) || in_shell ||
        ( !ignore_waterproof && is_waterproof( flags ) ) ) {
        return;
    }

    // Make the body wet
    for( const bodypart_id &bp : get_all_body_parts() ) {
        // Different body parts have different size, they can only store so much water
        int bp_wetness_max = 0;
        if( flags.test( bp.id() ) ) {
            bp_wetness_max = get_part_drench_capacity( bp );
        }

        if( bp_wetness_max == 0 ) {
            continue;
        }
        // Different sources will only make the bodypart wet to a limit
        int source_wet_max = saturation * bp_wetness_max / 100;
        int wetness_increment = ignore_waterproof ? 100 : bp->drench_increment;
        // Respect maximums
        const int wetness_max = std::min( source_wet_max, bp_wetness_max );
        const int curr_wetness = get_part_wetness( bp );
        if( curr_wetness < wetness_max ) {
            set_part_wetness( bp, std::min( wetness_max, curr_wetness + wetness_increment * 100 ) );
        }
    }
    const int torso_wetness = get_part_wetness( bodypart_id( "torso" ) );
    if( torso_wetness >= get_part_drench_capacity( bodypart_id( "torso" ) ) / 2.0 &&
        has_effect( effect_masked_scent ) &&
        get_value( "waterproof_scent" ).empty() ) {
        add_msg_if_player( m_info, _( "The water washes away the scent." ) );
        restore_scent();
    }

    if( is_weak_to_water() ) {
        add_msg_if_player( m_bad, _( "You feel the water burning your skin." ) );
    }

    // Remove onfire effect
    if( saturation > 10 || x_in_y( saturation, 10 ) ) {
        remove_effect( effect_onfire );
    }
}

void Character::apply_wetness_morale( units::temperature temperature )
{
    // First, a quick check if we have any wetness to calculate morale from
    // Faster than checking all worn items for friendliness
    if( !has_atleast_one_wet_part() ) {
        return;
    }

    // Normalize temperature to [-1.0,1.0]
    float temperature_f = units::to_fahrenheit( temperature );
    temperature_f = std::max( 0.f, std::min( 100.f, temperature_f ) );
    const double global_temperature_mod = -1.0 + ( 2.0 * temperature_f / 100.0 );

    int total_morale = 0;
    const body_part_set wet_friendliness = exclusive_flag_coverage( flag_WATER_FRIENDLY );
    for( const bodypart_id &bp : get_all_body_parts() ) {
        // Sum of body wetness can go up to 103
        const int part_drench = get_part_wetness( bp ) / 100;
        if( part_drench == 0 ) {
            continue;
        }

        const std::array<int, NUM_WATER_TOLERANCE> &part_arr = get_part_mut_drench( bp );
        const int part_ignored = part_arr[WT_IGNORED];
        const int part_neutral = part_arr[WT_NEUTRAL];
        const int part_good    = part_arr[WT_GOOD];

        if( part_ignored >= part_drench ) {
            continue;
        }

        int bp_morale = 0;
        const bool is_friendly = wet_friendliness.test( bp.id() );
        const int effective_drench = part_drench - part_ignored;
        if( is_friendly ) {
            // Using entire bonus from mutations and then some "human" bonus
            bp_morale = std::min( part_good, effective_drench ) + effective_drench / 2;
        } else if( effective_drench < part_good ) {
            // Positive or 0
            // Won't go higher than part_good / 2
            // Wet slime/scale doesn't feel as good when covered by wet rags/fur/kevlar
            bp_morale = std::min( effective_drench, part_good - effective_drench );
        } else if( effective_drench > part_good + part_neutral ) {
            // This one will be negative
            bp_morale = part_good + part_neutral - effective_drench;
        }

        // Clamp to [COLD,HOT]
        const units::temperature part_temperature =
            std::min( BODYTEMP_HOT, std::max( BODYTEMP_COLD, get_part_temp_cur( bp ) ) );
        // 0.0 at COLD, 1.0 at HOT
        const double part_mod = ( part_temperature - BODYTEMP_COLD ) /
                                ( BODYTEMP_HOT - BODYTEMP_COLD );
        // Average of global and part temperature modifiers, each in range [-1.0, 1.0]
        double scaled_temperature = ( global_temperature_mod + part_mod ) / 2;

        bp_morale += bp->wet_morale;
        if( bp_morale < 0 ) {
            // Damp, hot clothing on hot skin feels bad
            scaled_temperature = std::fabs( scaled_temperature );
        }

        // For an unmutated human swimming in deep water, this will add up to:
        // +12 when hot in 100% water friendly clothing
        // -26 when cold/hot in 100% unfriendly clothing
        total_morale += static_cast<int>( bp_morale * ( 1.0 + scaled_temperature ) / 8.0 );
    }

    if( total_morale == 0 ) {
        return;
    }

    int morale_effect = total_morale / 8;
    if( morale_effect == 0 ) {
        if( total_morale > 0 ) {
            morale_effect = 1;
        } else {
            morale_effect = -1;
        }
    }
    // 61_seconds because decay is applied in 1_minutes increments
    add_morale( MORALE_WET, morale_effect, total_morale, 61_seconds, 61_seconds, true );
}

void Character::add_addiction( const addiction_id &type, int strength )
{
    if( type.is_null() ) {
        return;
    }
    time_duration timer = 2_hours;
    if( has_trait( trait_ADDICTIVE ) ) {
        strength *= 2;
        timer = 1_hours;
    } else if( has_trait( trait_NONADDICTIVE ) ) {
        strength /= 2;
        timer = 6_hours;
    }
    //Update existing addiction
    for( addiction &i : addictions ) {
        if( i.type != type ) {
            continue;
        }

        if( i.sated < 0_turns ) {
            i.sated = timer;
        } else if( i.sated < 10_minutes ) {
            // TODO: Make this variable?
            i.sated += timer;
        } else {
            i.sated += timer / 2;
        }
        if( i.intensity < MAX_ADDICTION_LEVEL && strength > i.intensity * rng( 2, 5 ) ) {
            i.intensity++;
        }

        add_msg_debug( debugmode::DF_CHAR_HEALTH, "Updating addiction: %d intensity, %d sated",
                       i.intensity, to_turns<int>( i.sated ) );

        return;
    }

    // Add a new addiction
    const int roll = rng( 0, 100 );
    add_msg_debug( debugmode::DF_CHAR_HEALTH, "Addiction: roll %d vs strength %d", roll, strength );
    if( roll < strength ) {
        const std::string type_name = type->get_type_name().translated();
        add_msg_debug( debugmode::DF_CHAR_HEALTH, "%s got addicted to %s", disp_name(), type_name );
        addictions.emplace_back( type, 1 );
        get_event_bus().send<event_type::gains_addiction>( getID(), type );
    }
}

bool Character::has_addiction( const addiction_id &type ) const
{
    return std::any_of( addictions.begin(), addictions.end(),
    [type]( const addiction & ad ) {
        return ad.type == type && ad.intensity >= MIN_ADDICTION_LEVEL;
    } );
}

void Character::rem_addiction( const addiction_id &type )
{
    auto iter = std::find_if( addictions.begin(), addictions.end(),
    [type]( const addiction & ad ) {
        return ad.type == type;
    } );

    if( iter != addictions.end() ) {
        addictions.erase( iter );
        get_event_bus().send<event_type::loses_addiction>( getID(), type );
    }
}

int Character::addiction_level( const addiction_id &type ) const
{
    auto iter = std::find_if( addictions.begin(), addictions.end(),
    [type]( const addiction & ad ) {
        return ad.type == type;
    } );
    return iter != addictions.end() ? iter->intensity : 0;
}
