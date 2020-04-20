#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "addiction.h"
#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_utility.h"
#include "character.h"
#include "effect.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "field_type.h"
#include "game.h"
#include "game_constants.h"
#include "inventory.h"
#include "item.h"
#include "item_location.h"
#include "map.h"
#include "memory_fast.h"
#include "messages.h"
#include "monster.h"
#include "morale_types.h"
#include "mtype.h"
#include "mutation.h"
#include "name.h"
#include "npc.h"
#include "optional.h"
#include "options.h"
#include "overmapbuffer.h"
#include "pldata.h"
#include "point.h"
#include "rng.h"
#include "skill.h"
#include "sounds.h"
#include "stomach.h"
#include "string_formatter.h"
#include "string_id.h"
#include "teleport.h"
#include "text_snippets.h"
#include "translations.h"
#include "type_id.h"
#include "units.h"
#include "weather.h"

static const bionic_id bio_advreactor( "bio_advreactor" );
static const bionic_id bio_dis_acid( "bio_dis_acid" );
static const bionic_id bio_dis_shock( "bio_dis_shock" );
static const bionic_id bio_drain( "bio_drain" );
static const bionic_id bio_geiger( "bio_geiger" );
static const bionic_id bio_gills( "bio_gills" );
static const bionic_id bio_glowy( "bio_glowy" );
static const bionic_id bio_itchy( "bio_itchy" );
static const bionic_id bio_leaky( "bio_leaky" );
static const bionic_id bio_noise( "bio_noise" );
static const bionic_id bio_plut_filter( "bio_plut_filter" );
static const bionic_id bio_power_weakness( "bio_power_weakness" );
static const bionic_id bio_reactor( "bio_reactor" );
static const bionic_id bio_shakes( "bio_shakes" );
static const bionic_id bio_sleepy( "bio_sleepy" );
static const bionic_id bio_spasm( "bio_spasm" );
static const bionic_id bio_sunglasses( "bio_sunglasses" );
static const bionic_id bio_trip( "bio_trip" );

static const efftype_id effect_adrenaline( "adrenaline" );
static const efftype_id effect_asthma( "asthma" );
static const efftype_id effect_attention( "attention" );
static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_blind( "blind" );
static const efftype_id effect_cig( "cig" );
static const efftype_id effect_datura( "datura" );
static const efftype_id effect_deaf( "deaf" );
static const efftype_id effect_disabled( "disabled" );
static const efftype_id effect_downed( "downed" );
static const efftype_id effect_drunk( "drunk" );
static const efftype_id effect_formication( "formication" );
static const efftype_id effect_glowy_led( "glowy_led" );
static const efftype_id effect_hallu( "hallu" );
static const efftype_id effect_iodine( "iodine" );
static const efftype_id effect_masked_scent( "masked_scent" );
static const efftype_id effect_mending( "mending" );
static const efftype_id effect_narcosis( "narcosis" );
static const efftype_id effect_nausea( "nausea" );
static const efftype_id effect_onfire( "onfire" );
static const efftype_id effect_shakes( "shakes" );
static const efftype_id effect_sleep( "sleep" );
static const efftype_id effect_stunned( "stunned" );
static const efftype_id effect_took_thorazine( "took_thorazine" );
static const efftype_id effect_valium( "valium" );
static const efftype_id effect_visuals( "visuals" );
static const efftype_id effect_winded( "winded" );

static const trait_id trait_ADDICTIVE( "ADDICTIVE" );
static const trait_id trait_ALBINO( "ALBINO" );
static const trait_id trait_ASTHMA( "ASTHMA" );
static const trait_id trait_CHAOTIC( "CHAOTIC" );
static const trait_id trait_CHAOTIC_BAD( "CHAOTIC_BAD" );
static const trait_id trait_CHEMIMBALANCE( "CHEMIMBALANCE" );
static const trait_id trait_DEBUG_NOTEMP( "DEBUG_NOTEMP" );
static const trait_id trait_DEBUG_STORAGE( "DEBUG_STORAGE" );
static const trait_id trait_FRESHWATEROSMOSIS( "FRESHWATEROSMOSIS" );
static const trait_id trait_GILLS( "GILLS" );
static const trait_id trait_GILLS_CEPH( "GILLS_CEPH" );
static const trait_id trait_JITTERY( "JITTERY" );
static const trait_id trait_KILLER( "KILLER" );
static const trait_id trait_LEAVES( "LEAVES" );
static const trait_id trait_LEAVES2( "LEAVES2" );
static const trait_id trait_LEAVES3( "LEAVES3" );
static const trait_id trait_M_BLOSSOMS( "M_BLOSSOMS" );
static const trait_id trait_M_SPORES( "M_SPORES" );
static const trait_id trait_MOODSWINGS( "MOODSWINGS" );
static const trait_id trait_NARCOLEPTIC( "NARCOLEPTIC" );
static const trait_id trait_NONADDICTIVE( "NONADDICTIVE" );
static const trait_id trait_NOPAIN( "NOPAIN" );
static const trait_id trait_PER_SLIME( "PER_SLIME" );
static const trait_id trait_PYROMANIA( "PYROMANIA" );
static const trait_id trait_RADIOACTIVE1( "RADIOACTIVE1" );
static const trait_id trait_RADIOACTIVE2( "RADIOACTIVE2" );
static const trait_id trait_RADIOACTIVE3( "RADIOACTIVE3" );
static const trait_id trait_RADIOGENIC( "RADIOGENIC" );
static const trait_id trait_REGEN_LIZ( "REGEN_LIZ" );
static const trait_id trait_ROOTS3( "ROOTS3" );
static const trait_id trait_SCHIZOPHRENIC( "SCHIZOPHRENIC" );
static const trait_id trait_SHARKTEETH( "SHARKTEETH" );
static const trait_id trait_SHELL2( "SHELL2" );
static const trait_id trait_SHOUT1( "SHOUT1" );
static const trait_id trait_SHOUT2( "SHOUT2" );
static const trait_id trait_SHOUT3( "SHOUT3" );
static const trait_id trait_SORES( "SORES" );
static const trait_id trait_SUNBURN( "SUNBURN" );
static const trait_id trait_TROGLO( "TROGLO" );
static const trait_id trait_TROGLO2( "TROGLO2" );
static const trait_id trait_TROGLO3( "TROGLO3" );
static const trait_id trait_UNSTABLE( "UNSTABLE" );
static const trait_id trait_VOMITOUS( "VOMITOUS" );
static const trait_id trait_WEB_SPINNER( "WEB_SPINNER" );
static const trait_id trait_WEB_WEAVER( "WEB_WEAVER" );
static const trait_id trait_WINGS_INSECT( "WINGS_INSECT" );

static const mtype_id mon_zombie( "mon_zombie" );
static const mtype_id mon_zombie_cop( "mon_zombie_cop" );
static const mtype_id mon_zombie_fat( "mon_zombie_fat" );
static const mtype_id mon_zombie_fireman( "mon_zombie_fireman" );
static const mtype_id mon_zombie_soldier( "mon_zombie_soldier" );

static const std::string flag_BLIND( "BLIND" );
static const std::string flag_PLOWABLE( "PLOWABLE" );
static const std::string flag_RAD_RESIST( "RAD_RESIST" );
static const std::string flag_SUN_GLASSES( "SUN_GLASSES" );

static float addiction_scaling( float at_min, float at_max, float add_lvl )
{
    // Not addicted
    if( add_lvl < MIN_ADDICTION_LEVEL ) {
        return 1.0f;
    }

    return lerp( at_min, at_max, ( add_lvl - MIN_ADDICTION_LEVEL ) / MAX_ADDICTION_LEVEL );
}

void Character::suffer_water_damage( const mutation_branch &mdata )
{
    for( const body_part bp : all_body_parts ) {
        const float wetness_percentage = static_cast<float>( body_wetness[bp] ) /
                                         drench_capacity[bp];
        const int dmg = mdata.weakness_to_water * wetness_percentage;
        if( dmg > 0 ) {
            apply_damage( nullptr, convert_bp( bp ).id(), dmg );
            add_msg_player_or_npc( m_bad, _( "Your %s is damaged by the water." ),
                                   _( "<npcname>'s %s is damaged by the water." ),
                                   body_part_name( bp ) );
        } else if( dmg < 0 && hp_cur[bp_to_hp( bp )] != hp_max[bp_to_hp( bp )] ) {
            heal( bp, std::abs( dmg ) );
            add_msg_player_or_npc( m_good, _( "Your %s is healed by the water." ),
                                   _( "<npcname>'s %s is healed by the water." ),
                                   body_part_name( bp ) );
        }
    }
}

void Character::suffer_mutation_power( const mutation_branch &mdata,
                                       Character::trait_data &tdata )
{
    if( tdata.powered && tdata.charge > 0 ) {
        // Already-on units just lose a bit of charge
        tdata.charge--;
    } else {
        // Not-on units, or those with zero charge, have to pay the power cost
        if( mdata.cooldown > 0 ) {
            tdata.powered = true;
            tdata.charge = mdata.cooldown - 1;
        }
        if( mdata.hunger ) {
            // does not directly modify hunger, but burns kcal
            mod_stored_nutr( mdata.cost );
            if( get_bmi() < character_weight_category::underweight ) {
                add_msg_if_player( m_warning,
                                   _( "You're too malnourished to keep your %s going." ),
                                   mdata.name() );
                tdata.powered = false;
            }
        }
        if( mdata.thirst ) {
            mod_thirst( mdata.cost );
            // Well into Dehydrated
            if( get_thirst() >= 260 ) {
                add_msg_if_player( m_warning,
                                   _( "You're too dehydrated to keep your %s going." ),
                                   mdata.name() );
                tdata.powered = false;
            }
        }
        if( mdata.fatigue ) {
            mod_fatigue( mdata.cost );
            // Exhausted
            if( get_fatigue() >= EXHAUSTED ) {
                add_msg_if_player( m_warning,
                                   _( "You're too exhausted to keep your %s going." ),
                                   mdata.name() );
                tdata.powered = false;
            }
        }
        if( !tdata.powered ) {
            apply_mods( mdata.id, false );
        }
    }
}

void Character::suffer_while_underwater()
{
    if( !has_trait( trait_GILLS ) && !has_trait( trait_GILLS_CEPH ) ) {
        oxygen--;
    }
    if( oxygen < 12 && worn_with_flag( "REBREATHER" ) ) {
        oxygen += 12;
    }
    if( oxygen <= 5 ) {
        if( has_bionic( bio_gills ) && get_power_level() >= 25_kJ ) {
            oxygen += 5;
            mod_power_level( -25_kJ );
        } else {
            add_msg_if_player( m_bad, _( "You're drowning!" ) );
            apply_damage( nullptr, bodypart_id( "torso" ), rng( 1, 4 ) );
        }
    }
    if( has_trait( trait_FRESHWATEROSMOSIS ) && !g->m.has_flag_ter( "SALT_WATER", pos() ) &&
        get_thirst() > -60 ) {
        mod_thirst( -1 );
    }
}

void Character::suffer_from_addictions()
{
    time_duration timer = -6_hours;
    if( has_trait( trait_ADDICTIVE ) ) {
        timer = -10_hours;
    } else if( has_trait( trait_NONADDICTIVE ) ) {
        timer = -3_hours;
    }
    for( addiction &cur_addiction : addictions ) {
        if( cur_addiction.sated <= 0_turns &&
            cur_addiction.intensity >= MIN_ADDICTION_LEVEL ) {
            addict_effect( *this, cur_addiction );
        }
        cur_addiction.sated -= 1_turns;
        // Higher intensity addictions heal faster
        if( cur_addiction.sated - 10_minutes * cur_addiction.intensity < timer ) {
            if( cur_addiction.intensity <= 2 ) {
                rem_addiction( cur_addiction.type );
                break;
            } else {
                cur_addiction.intensity--;
                cur_addiction.sated = 0_turns;
            }
        }
    }
}

void Character::suffer_while_awake( const int current_stim )
{
    if( !has_trait( trait_DEBUG_STORAGE ) &&
        ( weight_carried() > 4 * weight_capacity() ) ) {
        if( has_effect( effect_downed ) ) {
            add_effect( effect_downed, 1_turns, num_bp, false, 0, true );
        } else {
            add_effect( effect_downed, 2_turns, num_bp, false, 0, true );
        }
    }
    if( has_trait( trait_CHEMIMBALANCE ) ) {
        suffer_from_chemimbalance();
    }
    if( ( has_trait( trait_SCHIZOPHRENIC ) || has_artifact_with( AEP_SCHIZO ) ) &&
        !has_effect( effect_took_thorazine ) ) {
        suffer_from_schizophrenia();
    }

    if( ( has_trait( trait_NARCOLEPTIC ) || has_artifact_with( AEP_SCHIZO ) ) ) {
        if( one_turn_in( 8_hours ) ) {
            add_msg( m_bad,
                     _( "You're suddenly overcome with the urge to sleep and you pass out." ) );
            fall_asleep( 20_minutes );
        }
    }

    if( has_trait( trait_JITTERY ) && !has_effect( effect_shakes ) ) {
        if( current_stim > 50 && one_in( to_turns<int>( 30_minutes ) - ( current_stim * 6 ) ) ) {
            add_effect( effect_shakes, 30_minutes + 1_turns * current_stim );
        } else if( ( get_hunger() > 80 || get_kcal_percent() < 1.0f ) && get_hunger() > 0 &&
                   one_in( to_turns<int>( 50_minutes ) - ( get_hunger() * 6 ) ) ) {
            add_effect( effect_shakes, 40_minutes );
        }
    }

    if( has_trait( trait_MOODSWINGS ) && one_turn_in( 6_hours ) ) {
        if( rng( 1, 20 ) > 9 ) {
            // 55% chance
            add_morale( MORALE_MOODSWING, -100, -500 );
        } else {
            // 45% chance
            add_morale( MORALE_MOODSWING, 100, 500 );
        }
    }

    if( has_trait( trait_VOMITOUS ) && one_turn_in( 7_hours ) ) {
        vomit();
    }

    if( has_trait( trait_SHOUT1 ) && one_turn_in( 6_hours ) ) {
        shout();
    }
    if( has_trait( trait_SHOUT2 ) && one_turn_in( 4_hours ) ) {
        shout();
    }
    if( has_trait( trait_SHOUT3 ) && one_turn_in( 3_hours ) ) {
        shout();
    }
    if( has_trait( trait_M_SPORES ) && one_turn_in( 4_hours ) ) {
        spores();
    }
    if( has_trait( trait_M_BLOSSOMS ) && one_turn_in( 3_hours ) ) {
        blossoms();
    }
}

void Character::suffer_from_chemimbalance()
{
    if( one_turn_in( 6_hours ) && !has_trait( trait_NOPAIN ) ) {
        add_msg_if_player( m_bad, _( "You suddenly feel sharp pain for no reason." ) );
        mod_pain( 3 * rng( 1, 3 ) );
    }
    if( one_turn_in( 6_hours ) ) {
        int pkilladd = 5 * rng( -1, 2 );
        if( pkilladd > 0 ) {
            add_msg_if_player( m_bad, _( "You suddenly feel numb." ) );
        } else if( ( pkilladd < 0 ) && ( !( has_trait( trait_NOPAIN ) ) ) ) {
            add_msg_if_player( m_bad, _( "You suddenly ache." ) );
        }
        mod_painkiller( pkilladd );
    }
    if( one_turn_in( 6_hours ) && !has_effect( effect_sleep ) ) {
        add_msg_if_player( m_bad, _( "You feel dizzy for a moment." ) );
        moves -= rng( 10, 30 );
    }
    if( one_turn_in( 6_hours ) ) {
        int hungadd = 5 * rng( -1, 3 );
        if( hungadd > 0 ) {
            add_msg_if_player( m_bad, _( "You suddenly feel hungry." ) );
        } else {
            add_msg_if_player( m_good, _( "You suddenly feel a little full." ) );
        }
        mod_hunger( hungadd );
    }
    if( one_turn_in( 6_hours ) ) {
        add_msg_if_player( m_bad, _( "You suddenly feel thirsty." ) );
        mod_thirst( 5 * rng( 1, 3 ) );
    }
    if( one_turn_in( 6_hours ) ) {
        add_msg_if_player( m_good, _( "You feel fatigued all of a sudden." ) );
        mod_fatigue( 10 * rng( 2, 4 ) );
    }
    if( one_turn_in( 8_hours ) ) {
        if( one_in( 3 ) ) {
            add_morale( MORALE_FEELING_GOOD, 20, 100 );
        } else {
            add_morale( MORALE_FEELING_BAD, -20, -100 );
        }
    }
    if( one_turn_in( 6_hours ) ) {
        if( one_in( 3 ) ) {
            add_msg_if_player( m_bad, _( "You suddenly feel very cold." ) );
            temp_cur.fill( BODYTEMP_VERY_COLD );
        } else {
            add_msg_if_player( m_bad, _( "You suddenly feel cold." ) );
            temp_cur.fill( BODYTEMP_COLD );
        }
    }
    if( one_turn_in( 6_hours ) ) {
        if( one_in( 3 ) ) {
            add_msg_if_player( m_bad, _( "You suddenly feel very hot." ) );
            temp_cur.fill( BODYTEMP_VERY_HOT );
        } else {
            add_msg_if_player( m_bad, _( "You suddenly feel hot." ) );
            temp_cur.fill( BODYTEMP_HOT );
        }
    }
}

void Character::suffer_from_schizophrenia()
{
    std::string i_name_w;
    if( !weapon.is_null() ) {
        i_name_w = weapon.has_var( "item_label" ) ? weapon.get_var( "item_label" ) :
                   //~ %1$s: weapon name
                   string_format( _( "your %1$s" ), weapon.type_name() );
    }
    // Start with the effects that both NPCs and avatars can suffer from
    // Delusions
    if( one_turn_in( 8_hours ) ) {
        if( rng( 1, 20 ) > 5 ) {  // 75% chance
            const translation snip = SNIPPET.random_from_category( "schizo_delusion_paranoid" ).value_or(
                                         translation() );
            add_msg_if_player( m_warning, "%s", snip );
            add_morale( MORALE_FEELING_BAD, -20, -100 );
        } else { // 25% chance
            const translation snip = SNIPPET.random_from_category( "schizo_delusion_grandiose" ).value_or(
                                         translation() );
            add_msg_if_player( m_good, "%s", snip );
            add_morale( MORALE_FEELING_GOOD, 20, 100 );
        }
        return;
    }
    // Formication
    if( one_turn_in( 6_hours ) ) {
        const translation snip = SNIPPET.random_from_category( "schizo_formication" ).value_or(
                                     translation() );
        body_part bp = random_body_part( true );
        add_effect( effect_formication, 45_minutes, bp );
        add_msg_if_player( m_bad, "%s", snip );
        return;
    }
    // Numbness
    if( one_turn_in( 4_hours ) ) {
        add_msg_if_player( m_bad, _( "You suddenly feel so numb…" ) );
        mod_painkiller( 25 );
        return;
    }
    // Hallucination
    if( one_turn_in( 6_hours ) ) {
        add_effect( effect_hallu, 6_hours );
        return;
    }
    // Visuals
    if( one_turn_in( 2_hours ) ) {
        add_effect( effect_visuals, rng( 15_turns, 60_turns ) );
        return;
    }
    // Shaking
    if( !has_effect( effect_valium ) && one_turn_in( 4_hours ) ) {
        add_msg_player_or_npc( m_bad, _( "You start to shake uncontrollably." ),
                               _( "<npcname> starts to shake uncontrollably." ) );
        add_effect( effect_shakes, rng( 2_minutes, 5_minutes ) );
        return;
    }
    // Shout
    if( one_turn_in( 4_hours ) ) {
        shout( SNIPPET.random_from_category( "schizo_self_shout" ).value_or( translation() ).translated() );
        return;
    }
    // Drop weapon
    if( one_turn_in( 2_days ) && !weapon.is_null() ) {
        const translation snip = SNIPPET.random_from_category( "schizo_weapon_drop" ).value_or(
                                     translation() );
        std::string str = string_format( snip, i_name_w );
        str[0] = toupper( str[0] );

        add_msg_if_player( m_bad, "%s", str );
        item_location loc( *this, &weapon );
        drop( loc, pos() );
        return;
    }
    // Talk to self
    if( one_turn_in( 4_hours ) ) {
        const translation snip = SNIPPET.random_from_category( "schizo_self_talk" ).value_or(
                                     translation() );
        add_msg( _( "%1$s says: \"%2$s\"" ), name, snip );
        return;
    }

    // effects of this point are entirely internal, so NPCs can't suffer from them
    if( is_npc() ) {
        return;
    }
    // Sound
    if( one_turn_in( 4_hours ) ) {
        sound_hallu();
    }
    // Follower turns hostile
    if( one_turn_in( 4_hours ) ) {
        std::vector<shared_ptr_fast<npc>> followers = overmap_buffer.get_npcs_near_player( 12 );

        std::string who_gets_angry = name;
        if( !followers.empty() ) {
            who_gets_angry = random_entry_ref( followers )->name;
        }
        add_msg( m_bad, _( "%1$s gets angry!" ), who_gets_angry );
        return;
    }

    // Monster dies
    if( one_turn_in( 6_hours ) ) {
        // TODO: move to monster group json
        static const std::array<mtype_id, 5> monsters = { {
                mon_zombie, mon_zombie_fat, mon_zombie_fireman, mon_zombie_cop, mon_zombie_soldier
            }
        };
        add_msg( _( "%s dies!" ), random_entry_ref( monsters )->nname() );
        return;
    }

    // Limb Breaks
    if( one_turn_in( 4_hours ) ) {
        const translation snip = SNIPPET.random_from_category( "broken_limb" ).value_or( translation() );
        add_msg( m_bad, "%s", snip );
        return;
    }

    // NPC chat
    if( one_turn_in( 4_hours ) ) {
        std::string i_name = Name::generate( one_in( 2 ) );

        std::string i_talk = SNIPPET.expand( SNIPPET.random_from_category( "<lets_talk>" ).value_or(
                translation() ).translated() );
        parse_tags( i_talk, *this, *this );

        add_msg( _( "%1$s says: \"%2$s\"" ), i_name, i_talk );
        return;
    }

    // Skill raise
    if( one_turn_in( 12_hours ) ) {
        skill_id raised_skill = Skill::random_skill();
        add_msg( m_good, _( "You increase %1$s to level %2$d." ), raised_skill.obj().name(),
                 get_skill_level( raised_skill ) + 1 );
        return;
    }

    // Talking weapon
    if( !weapon.is_null() ) {
        // If player has a weapon, picks a message from said weapon
        // Weapon tells player to kill a monster if any are nearby
        // Weapon is concerned for player if bleeding
        // Weapon is concerned for itself if damaged
        // Otherwise random chit-chat
        std::vector<weak_ptr_fast<monster>> mons = g->all_monsters().items;

        std::string i_talk_w;
        bool does_talk = false;
        if( !mons.empty() && one_turn_in( 12_minutes ) ) {
            std::vector<std::string> seen_mons;
            for( weak_ptr_fast<monster> &n : mons ) {
                if( sees( *n.lock() ) ) {
                    seen_mons.emplace_back( n.lock()->get_name() );
                }
            }
            if( !seen_mons.empty() ) {
                const translation talk_w = SNIPPET.random_from_category( "schizo_weapon_talk_monster" ).value_or(
                                               translation() );
                i_talk_w = string_format( talk_w, random_entry_ref( seen_mons ) );
                does_talk = true;
            }
        }
        if( !does_talk && has_effect( effect_bleed ) && one_turn_in( 5_minutes ) ) {
            i_talk_w = SNIPPET.random_from_category( "schizo_weapon_talk_bleeding" ).value_or(
                           translation() ).translated();
            does_talk = true;
        } else if( weapon.damage() >= weapon.max_damage() / 3 && one_turn_in( 1_hours ) ) {
            i_talk_w = SNIPPET.random_from_category( "schizo_weapon_talk_damaged" ).value_or(
                           translation() ).translated();
            does_talk = true;
        } else if( one_turn_in( 4_hours ) ) {
            i_talk_w = SNIPPET.random_from_category( "schizo_weapon_talk_misc" ).value_or(
                           translation() ).translated();
            does_talk = true;
        }
        if( does_talk ) {
            add_msg( _( "%1$s says: \"%2$s\"" ), i_name_w, i_talk_w );
            return;
        }
    }
}

void Character::suffer_from_asthma( const int current_stim )
{
    if( has_effect( effect_adrenaline ) || has_effect( effect_datura ) ) {
        return;
    }
    if( !one_in( ( to_turns<int>( 6_hours ) - current_stim * 300 ) *
                 ( has_effect( effect_sleep ) ? 10 : 1 ) ) ) {
        return;
    }
    bool auto_use = has_charges( "inhaler", 1 ) || has_charges( "oxygen_tank", 1 ) ||
                    has_charges( "smoxygen_tank", 1 );
    bool oxygenator = has_bionic( bio_gills ) && get_power_level() >= 3_kJ;
    if( underwater ) {
        oxygen = oxygen / 2;
        auto_use = false;
    }

    add_msg_player_or_npc( m_bad, _( "You have an asthma attack!" ),
                           "<npcname> starts wheezing and coughing." );

    if( in_sleep_state() && !has_effect( effect_narcosis ) ) {
        inventory map_inv;
        map_inv.form_from_map( g->u.pos(), 2, &g->u );
        // check if an inhaler is somewhere near
        bool nearby_use = auto_use || oxygenator || map_inv.has_charges( "inhaler", 1 ) ||
                          map_inv.has_charges( "oxygen_tank", 1 ) ||
                          map_inv.has_charges( "smoxygen_tank", 1 );
        // check if character has an oxygenator first
        if( oxygenator ) {
            mod_power_level( -3_kJ );
            add_msg_if_player( m_info, _( "You use your Oxygenator to clear it up, "
                                          "then go back to sleep." ) );
        } else if( auto_use ) {
            if( use_charges_if_avail( "inhaler", 1 ) ) {
                add_msg_if_player( m_info, _( "You use your inhaler and go back to sleep." ) );
            } else if( use_charges_if_avail( "oxygen_tank", 1 ) ||
                       use_charges_if_avail( "smoxygen_tank", 1 ) ) {
                add_msg_if_player( m_info, _( "You take a deep breath from your oxygen tank "
                                              "and go back to sleep." ) );
            }
        } else if( nearby_use ) {
            // create new variable to resolve a reference issue
            int amount = 1;
            if( !g->m.use_charges( g->u.pos(), 2, "inhaler", amount ).empty() ) {
                add_msg_if_player( m_info, _( "You use your inhaler and go back to sleep." ) );
            } else if( !g->m.use_charges( g->u.pos(), 2, "oxygen_tank", amount ).empty() ||
                       !g->m.use_charges( g->u.pos(), 2, "smoxygen_tank", amount ).empty() ) {
                add_msg_if_player( m_info, _( "You take a deep breath from your oxygen tank "
                                              "and go back to sleep." ) );
            }
        } else {
            add_effect( effect_asthma, rng( 5_minutes, 20_minutes ) );
            if( has_effect( effect_sleep ) ) {
                wake_up();
            } else {
                if( !is_npc() ) {
                    g->cancel_activity_or_ignore_query( distraction_type::asthma,
                                                        _( "You can't focus while choking!" ) );
                }
            }
        }
    } else if( auto_use ) {
        int charges = 0;
        if( use_charges_if_avail( "inhaler", 1 ) ) {
            moves -= 40;
            charges = charges_of( "inhaler" );
            if( charges == 0 ) {
                add_msg_if_player( m_bad, _( "You use your last inhaler charge." ) );
            } else {
                add_msg_if_player( m_info, ngettext( "You use your inhaler, "
                                                     "only %d charge left.",
                                                     "You use your inhaler, "
                                                     "only %d charges left.", charges ),
                                   charges );
            }
        } else if( use_charges_if_avail( "oxygen_tank", 1 ) ||
                   use_charges_if_avail( "smoxygen_tank", 1 ) ) {
            moves -= 500; // synched with use action
            charges = charges_of( "oxygen_tank" ) + charges_of( "smoxygen_tank" );
            if( charges == 0 ) {
                add_msg_if_player( m_bad, _( "You breathe in last bit of oxygen "
                                             "from the tank." ) );
            } else {
                add_msg_if_player( m_info, ngettext( "You take a deep breath from your oxygen "
                                                     "tank, only %d charge left.",
                                                     "You take a deep breath from your oxygen "
                                                     "tank, only %d charges left.", charges ),
                                   charges );
            }
        }
    } else {
        add_effect( effect_asthma, rng( 5_minutes, 20_minutes ) );
        if( !is_npc() ) {
            g->cancel_activity_or_ignore_query( distraction_type::asthma,
                                                _( "You can't focus while choking!" ) );
        }
    }
}

void Character::suffer_in_sunlight()
{
    double sleeve_factor = armwear_factor();
    const bool has_hat = wearing_something_on( bp_head );
    const bool leafy = has_trait( trait_LEAVES ) || has_trait( trait_LEAVES2 ) ||
                       has_trait( trait_LEAVES3 );
    const bool leafier = has_trait( trait_LEAVES2 ) || has_trait( trait_LEAVES3 );
    const bool leafiest = has_trait( trait_LEAVES3 );
    int sunlight_nutrition = 0;
    if( leafy && g->m.is_outside( pos() ) && ( g->light_level( pos().z ) >= 40 ) ) {
        const float weather_factor = ( g->weather.weather == WEATHER_CLEAR ||
                                       g->weather.weather == WEATHER_SUNNY ) ? 1.0 : 0.5;
        const int player_local_temp = g->weather.get_temperature( pos() );
        int flux = ( player_local_temp - 65 ) / 2;
        if( !has_hat ) {
            sunlight_nutrition += ( 100 + flux ) * weather_factor;
        }
        if( leafier ) {
            int rate = ( ( 100 * sleeve_factor ) + flux ) * 2;
            sunlight_nutrition += ( rate * ( leafiest ? 2 : 1 ) ) * weather_factor;
        }
    }

    if( x_in_y( sunlight_nutrition, 18000 ) ) {
        vitamin_mod( vitamin_id( "vitA" ), 1, true );
        vitamin_mod( vitamin_id( "vitC" ), 1, true );
    }

    if( x_in_y( sunlight_nutrition, 12000 ) ) {
        mod_hunger( -1 );
        // photosynthesis absorbs kcal directly
        mod_stored_nutr( -1 );
        stomach.ate();
    }

    if( !g->is_in_sunlight( pos() ) ) {
        return;
    }

    if( has_trait( trait_ALBINO ) || has_effect( effect_datura ) ) {
        suffer_from_albinism();
    }

    if( has_trait( trait_SUNBURN ) && one_in( 10 ) ) {
        if( !( weapon.has_flag( "RAIN_PROTECT" ) ) ) {
            add_msg_if_player( m_bad, _( "The sunlight burns your skin!" ) );
            if( has_effect( effect_sleep ) && !has_effect( effect_narcosis ) ) {
                wake_up();
            }
            mod_pain( 1 );
            hurtall( 1, nullptr );
        }
    }

    if( ( has_trait( trait_TROGLO ) || has_trait( trait_TROGLO2 ) ) &&
        g->weather.weather == WEATHER_SUNNY ) {
        mod_str_bonus( -1 );
        mod_dex_bonus( -1 );
        add_miss_reason( _( "The sunlight distracts you." ), 1 );
        mod_int_bonus( -1 );
        mod_per_bonus( -1 );
    }
    if( has_trait( trait_TROGLO2 ) ) {
        mod_str_bonus( -1 );
        mod_dex_bonus( -1 );
        add_miss_reason( _( "The sunlight distracts you." ), 1 );
        mod_int_bonus( -1 );
        mod_per_bonus( -1 );
    }
    if( has_trait( trait_TROGLO3 ) ) {
        mod_str_bonus( -4 );
        mod_dex_bonus( -4 );
        add_miss_reason( _( "You can't stand the sunlight!" ), 4 );
        mod_int_bonus( -4 );
        mod_per_bonus( -4 );
    }
}

void Character::suffer_from_albinism()
{
    if( !one_turn_in( 1_minutes ) ) {
        return;
    }
    // Sunglasses can keep the sun off the eyes.
    if( !has_bionic( bio_sunglasses ) &&
        !( wearing_something_on( bp_eyes ) &&
           ( worn_with_flag( flag_SUN_GLASSES ) || worn_with_flag( flag_BLIND ) ) ) ) {
        add_msg_if_player( m_bad, _( "The sunlight is really irritating your eyes." ) );
        if( one_turn_in( 1_minutes ) ) {
            mod_pain( 1 );
        } else {
            focus_pool --;
        }
    }
    // Umbrellas can keep the sun off the skin
    if( weapon.has_flag( "RAIN_PROTECT" ) ) {
        return;
    }
    //calculate total coverage of skin
    body_part_set affected_bp { {
            bp_leg_l, bp_leg_r, bp_torso, bp_head, bp_mouth, bp_arm_l,
            bp_arm_r, bp_foot_l, bp_foot_r, bp_hand_l, bp_hand_r
        }
    };
    //pecentage of "open skin" by body part
    std::map<body_part, float> open_percent;
    //initialize coverage
    for( const body_part &bp : all_body_parts ) {
        if( affected_bp.test( bp ) ) {
            open_percent[bp] = 1.0;
        }
    }
    //calculate coverage for every body part
    for( const item &i : worn ) {
        body_part_set covered = i.get_covered_body_parts();
        for( const body_part &bp : all_body_parts )  {
            if( !affected_bp.test( bp ) || !covered.test( bp ) ) {
                continue;
            }
            //percent of "not covered skin"
            float p = 1.0 - i.get_coverage() / 100.0;
            open_percent[bp] = open_percent[bp] * p;
        }
    }

    const float COVERAGE_LIMIT = 0.01;
    body_part max_affected_bp = num_bp;
    float max_affected_bp_percent = 0;
    int count_affected_bp = 0;
    for( const std::pair<const body_part, float> &it : open_percent ) {
        const body_part &bp = it.first;
        const float &p = it.second;

        if( p <= COVERAGE_LIMIT ) {
            continue;
        }
        ++count_affected_bp;
        if( max_affected_bp_percent < p ) {
            max_affected_bp_percent = p;
            max_affected_bp = bp;
        }
    }
    if( count_affected_bp > 0 && max_affected_bp != num_bp ) {
        //Check if both arms/legs are affected
        int parts_count = 1;
        body_part other_bp = static_cast<body_part>( bp_aiOther[max_affected_bp] );
        body_part other_bp_rev = static_cast<body_part>( bp_aiOther[other_bp] );
        if( other_bp != other_bp_rev ) {
            const auto found = open_percent.find( other_bp );
            if( found != open_percent.end() && found->second > COVERAGE_LIMIT ) {
                ++parts_count;
            }
        }
        std::string bp_name = body_part_name( max_affected_bp, parts_count );
        if( count_affected_bp > parts_count ) {
            bp_name = string_format( _( "%s and other body parts" ), bp_name );
        }
        add_msg_if_player( m_bad, _( "The sunlight is really irritating your %s." ), bp_name );

        //apply effects
        if( has_effect( effect_sleep ) && !has_effect( effect_narcosis ) ) {
            wake_up();
        }
        if( one_turn_in( 1_minutes ) ) {
            mod_pain( 1 );
        } else {
            focus_pool --;
        }
    }
}

void Character::suffer_from_other_mutations()
{
    if( has_trait( trait_SHARKTEETH ) && one_turn_in( 24_hours ) ) {
        add_msg_if_player( m_neutral, _( "You shed a tooth!" ) );
        g->m.spawn_item( pos(), "bone", 1 );
    }

    if( has_active_mutation( trait_WINGS_INSECT ) ) {
        //~Sound of buzzing Insect Wings
        sounds::sound( pos(), 10, sounds::sound_t::movement, _( "BZZZZZ" ), false, "misc",
                       "insect_wings" );
    }

    bool wearing_shoes = is_wearing_shoes( side::LEFT ) || is_wearing_shoes( side::RIGHT );
    int root_vitamins = 0;
    int root_water = 0;
    if( has_trait( trait_ROOTS3 ) && g->m.has_flag( flag_PLOWABLE, pos() ) && !wearing_shoes ) {
        root_vitamins += 1;
        if( get_thirst() <= -2000 ) {
            root_water += 51;
        }
    }

    if( x_in_y( root_vitamins, 576 ) ) {
        vitamin_mod( vitamin_id( "iron" ), 1, true );
        vitamin_mod( vitamin_id( "calcium" ), 1, true );
        mod_healthy_mod( 5, 50 );
    }

    if( x_in_y( root_water, 2550 ) ) {
        // Plants draw some crazy amounts of water from the ground in real life,
        // so these numbers try to reflect that uncertain but large amount
        // this should take 12 hours to meet your daily needs with ROOTS2, and 8 with ROOTS3
        mod_thirst( -1 );
    }

    if( has_trait( trait_SORES ) ) {
        for( const body_part bp : all_body_parts ) {
            if( bp == bp_head ) {
                continue;
            }
            int sores_pain = 5 + 0.4 * std::abs( encumb( bp ) );
            if( get_pain() < sores_pain ) {
                set_pain( sores_pain );
            }
        }
    }
    //Web Weavers...weave web
    if( has_active_mutation( trait_WEB_WEAVER ) && !in_vehicle ) {
        // this adds intensity to if its not already there.
        g->m.add_field( pos(), fd_web, 1 );

    }

    // Blind/Deaf for brief periods about once an hour,
    // and visuals about once every 30 min.
    if( has_trait( trait_PER_SLIME ) ) {
        if( one_turn_in( 1_hours ) && !has_effect( effect_deaf ) ) {
            add_msg_if_player( m_bad, _( "Suddenly, you can't hear anything!" ) );
            add_effect( effect_deaf, rng( 20_minutes, 60_minutes ) );
        }
        if( one_turn_in( 1_hours ) && !( has_effect( effect_blind ) ) ) {
            add_msg_if_player( m_bad, _( "Suddenly, your eyes stop working!" ) );
            add_effect( effect_blind, rng( 2_minutes, 6_minutes ) );
        }
        // Yes, you can be blind and hallucinate at the same time.
        // Your post-human biology is truly remarkable.
        if( one_turn_in( 30_minutes ) && !( has_effect( effect_visuals ) ) ) {
            add_msg_if_player( m_bad, _( "Your visual centers must be acting up…" ) );
            add_effect( effect_visuals, rng( 36_minutes, 72_minutes ) );
        }
    }

    if( has_trait( trait_WEB_SPINNER ) && !in_vehicle && one_in( 3 ) ) {
        // this adds intensity to if its not already there.
        g->m.add_field( pos(), fd_web, 1 );
    }

    bool should_mutate = has_trait( trait_UNSTABLE ) && !has_trait( trait_CHAOTIC_BAD ) &&
                         one_turn_in( 48_hours );
    should_mutate |= ( has_trait( trait_CHAOTIC ) || has_trait( trait_CHAOTIC_BAD ) ) &&
                     one_turn_in( 12_hours );
    if( should_mutate ) {
        mutate();
    }

    const bool needs_fire = !has_morale( MORALE_PYROMANIA_NEARFIRE ) &&
                            !has_morale( MORALE_PYROMANIA_STARTFIRE );
    if( has_trait( trait_PYROMANIA ) && needs_fire && !in_sleep_state() &&
        calendar::once_every( 2_hours ) ) {
        add_morale( MORALE_PYROMANIA_NOFIRE, -1, -30, 24_hours, 24_hours, true );
        if( calendar::once_every( 4_hours ) ) {
            const translation smokin_hot_fiyah =
                SNIPPET.random_from_category( "pyromania_withdrawal" ).value_or( translation() );
            add_msg_if_player( m_bad, "%s", smokin_hot_fiyah );
        }
    }
    if( has_trait( trait_KILLER ) && !has_morale( MORALE_KILLER_HAS_KILLED ) &&
        calendar::once_every( 2_hours ) ) {
        add_morale( MORALE_KILLER_NEED_TO_KILL, -1, -30, 24_hours, 24_hours );
        if( calendar::once_every( 4_hours ) ) {
            const translation snip = SNIPPET.random_from_category( "killer_withdrawal" ).value_or(
                                         translation() );
            add_msg_if_player( m_bad, "%s", snip );
        }
    }
}

void Character::suffer_from_radiation()
{
    // checking for radioactive items in inventory
    const int item_radiation = leak_level( "RADIOACTIVE" );
    const int map_radiation = g->m.get_radiation( pos() );
    float rads = map_radiation / 100.0f + item_radiation / 10.0f;

    int rad_mut = 0;
    if( has_trait( trait_RADIOACTIVE3 ) ) {
        rad_mut = 3;
    } else if( has_trait( trait_RADIOACTIVE2 ) ) {
        rad_mut = 2;
    } else if( has_trait( trait_RADIOACTIVE1 ) ) {
        rad_mut = 1;
    }

    // Spread less radiation when sleeping (slower metabolism etc.)
    // Otherwise it can quickly get to the point where you simply can't sleep at all
    const bool rad_mut_proc = rad_mut > 0 && x_in_y( rad_mut, to_turns<int>( in_sleep_state() ?
                              3_hours : 30_minutes ) );

    bool has_helmet = false;
    const bool power_armored = is_wearing_power_armor( &has_helmet );
    const bool rad_resist = power_armored || worn_with_flag( flag_RAD_RESIST );

    if( rad_mut > 0 ) {
        const bool kept_in = is_rad_immune() || ( rad_resist && !one_in( 4 ) );
        if( kept_in ) {
            // As if standing on a map tile with radiation level equal to rad_mut
            rads += rad_mut / 100.0f;
        }

        if( rad_mut_proc && !kept_in ) {
            // Irradiate a random nearby point
            // If you can't, irradiate the player instead
            tripoint rad_point = pos() + point( rng( -3, 3 ), rng( -3, 3 ) );
            // TODO: Radioactive vehicles?
            if( g->m.get_radiation( rad_point ) < rad_mut ) {
                g->m.adjust_radiation( rad_point, 1 );
            } else {
                rads += rad_mut;
            }
        }
    }

    // Used to control vomiting from radiation to make it not-annoying
    bool radiation_increasing = irradiate( rads );

    if( radiation_increasing && calendar::once_every( 3_minutes ) && has_bionic( bio_geiger ) ) {
        add_msg_if_player( m_warning,
                           _( "You feel an anomalous sensation coming from "
                              "your radiation sensors." ) );
    }

    if( calendar::once_every( 15_minutes ) ) {
        if( get_rad() < 0 ) {
            set_rad( 0 );
        } else if( get_rad() > 2000 ) {
            set_rad( 2000 );
        }
        if( get_option<bool>( "RAD_MUTATION" ) && rng( 100, 10000 ) < get_rad() ) {
            mutate();
            mod_rad( -50 );
        } else if( get_rad() > 50 && rng( 1, 3000 ) < get_rad() && ( stomach.contains() > 0_ml ||
                   radiation_increasing || !in_sleep_state() ) ) {
            vomit();
            mod_rad( -1 );
        }
    }

    const bool radiogenic = has_trait( trait_RADIOGENIC );
    if( radiogenic && calendar::once_every( 30_minutes ) && get_rad() > 0 ) {
        // At 200 irradiation, twice as fast as REGEN
        if( x_in_y( get_rad(), 200 ) ) {
            healall( 1 );
            if( rad_mut == 0 ) {
                // Don't heal radiation if we're generating it naturally
                // That would counter the main downside of radioactivity
                mod_rad( -5 );
            }
        }
    }

    if( !radiogenic && get_rad() > 0 ) {
        // Even if you heal the radiation itself, the damage is done.
        const int hmod = get_healthy_mod();
        if( hmod > 200 - get_rad() ) {
            set_healthy_mod( std::max( -200, 200 - get_rad() ) );
        }
    }

    if( get_rad() > 200 && calendar::once_every( 10_minutes ) && x_in_y( get_rad(), 1000 ) ) {
        hurtall( 1, nullptr );
        mod_rad( -5 );
    }

    if( !reactor_plut && !tank_plut && !slow_rad ) {
        return;
    }
    // Microreactor CBM and supporting bionics
    if( has_bionic( bio_reactor ) || has_bionic( bio_advreactor ) ) {
        //first do the filtering of plutonium from storage to reactor
        if( tank_plut > 0 ) {
            int plut_trans;
            if( has_active_bionic( bio_plut_filter ) ) {
                plut_trans = tank_plut * 0.025;
            } else {
                plut_trans = tank_plut * 0.005;
            }
            if( plut_trans < 1 ) {
                plut_trans = 1;
            }
            tank_plut -= plut_trans;
            reactor_plut += plut_trans;
        }
        //leaking radiation, reactor is unshielded, but still better than a simple tank
        slow_rad += ( ( tank_plut * 0.1 ) + ( reactor_plut * 0.01 ) );
        //begin power generation
        if( reactor_plut > 0 ) {
            int power_gen = 0;
            if( has_bionic( bio_advreactor ) ) {
                if( ( reactor_plut * 0.05 ) > 2000 ) {
                    power_gen = 2000;
                } else {
                    power_gen = reactor_plut * 0.05;
                    if( power_gen < 1 ) {
                        power_gen = 1;
                    }
                }
                slow_rad += ( power_gen * 3 );
                while( slow_rad >= 50 ) {
                    if( power_gen >= 1 ) {
                        slow_rad -= 50;
                        power_gen -= 1;
                        reactor_plut -= 1;
                    } else {
                        break;
                    }
                }
            } else if( has_bionic( bio_reactor ) ) {
                if( ( reactor_plut * 0.025 ) > 500 ) {
                    power_gen = 500;
                } else {
                    power_gen = reactor_plut * 0.025;
                    if( power_gen < 1 ) {
                        power_gen = 1;
                    }
                }
                slow_rad += ( power_gen * 3 );
            }
            reactor_plut -= power_gen;
            while( power_gen >= 250 ) {
                apply_damage( nullptr, bodypart_id( "torso" ), 1 );
                mod_pain( 1 );
                add_msg_if_player( m_bad,
                                   _( "Your chest burns as your power systems overload!" ) );
                mod_power_level( 50_kJ );
                power_gen -= 60; // ten units of power lost due to short-circuiting into you
            }
            mod_power_level( units::from_kilojoule( power_gen ) );
        }
    } else {
        slow_rad += ( reactor_plut + tank_plut ) * 40;
        //plutonium in body without any kind of container.  Not good at all.
        reactor_plut *= 0.6;
        tank_plut *= 0.6;
    }
    while( slow_rad >= 1000 ) {
        mod_rad( 1 );
        slow_rad -= 1000;
    }
}

void Character::suffer_from_bad_bionics()
{
    // Negative bionics effects
    if( has_bionic( bio_dis_shock ) && get_power_level() > 9_kJ && one_turn_in( 2_hours ) &&
        !has_effect( effect_narcosis ) ) {
        add_msg_if_player( m_bad, _( "You suffer a painful electrical discharge!" ) );
        mod_pain( 1 );
        moves -= 150;
        mod_power_level( -10_kJ );

        if( weapon.typeId() == "e_handcuffs" && weapon.charges > 0 ) {
            weapon.charges -= rng( 1, 3 ) * 50;
            if( weapon.charges < 1 ) {
                weapon.charges = 1;
            }

            add_msg_if_player( m_good, _( "The %s seems to be affected by the discharge." ),
                               weapon.tname() );
        }
        sfx::play_variant_sound( "bionics", "elec_discharge", 100 );
    }
    if( has_bionic( bio_dis_acid ) && one_turn_in( 150_minutes ) ) {
        add_msg_if_player( m_bad, _( "You suffer a burning acidic discharge!" ) );
        hurtall( 1, nullptr );
        sfx::play_variant_sound( "bionics", "acid_discharge", 100 );
        sfx::do_player_death_hurt( g->u, false );
    }
    if( has_bionic( bio_drain ) && get_power_level() > 24_kJ && one_turn_in( 1_hours ) ) {
        add_msg_if_player( m_bad, _( "Your batteries discharge slightly." ) );
        mod_power_level( -25_kJ );
        sfx::play_variant_sound( "bionics", "elec_crackle_low", 100 );
    }
    if( has_bionic( bio_noise ) && one_turn_in( 50_minutes ) &&
        !has_effect( effect_narcosis ) ) {
        // TODO: NPCs with said bionic
        if( !is_deaf() ) {
            add_msg( m_bad, _( "A bionic emits a crackle of noise!" ) );
            sfx::play_variant_sound( "bionics", "elec_blast", 100 );
        } else {
            add_msg( m_bad, _( "You feel your faulty bionic shuddering." ) );
            sfx::play_variant_sound( "bionics", "elec_blast_muffled", 100 );
        }
        sounds::sound( pos(), 60, sounds::sound_t::movement, _( "Crackle!" ) ); //sfx above
    }
    if( has_bionic( bio_power_weakness ) && has_max_power() &&
        get_power_level() >= get_max_power_level() * .75 ) {
        mod_str_bonus( -3 );
    }
    if( has_bionic( bio_trip ) && one_turn_in( 50_minutes ) &&
        !has_effect( effect_visuals ) &&
        !has_effect( effect_narcosis ) && !in_sleep_state() ) {
        add_msg_if_player( m_bad, _( "Your vision pixelates!" ) );
        add_effect( effect_visuals, 10_minutes );
        sfx::play_variant_sound( "bionics", "pixelated", 100 );
    }
    if( has_bionic( bio_spasm ) && one_turn_in( 5_hours ) && !has_effect( effect_downed ) &&
        !has_effect( effect_narcosis ) ) {
        add_msg_if_player( m_bad,
                           _( "Your malfunctioning bionic causes you to spasm and fall to the floor!" ) );
        mod_pain( 1 );
        add_effect( effect_stunned, 1_turns );
        add_effect( effect_downed, 1_turns, num_bp, false, 0, true );
        sfx::play_variant_sound( "bionics", "elec_crackle_high", 100 );
    }
    if( has_bionic( bio_shakes ) && get_power_level() > 24_kJ && one_turn_in( 2_hours ) ) {
        add_msg_if_player( m_bad, _( "Your bionics short-circuit, causing you to tremble and shiver." ) );
        mod_power_level( -25_kJ );
        add_effect( effect_shakes, 5_minutes );
        sfx::play_variant_sound( "bionics", "elec_crackle_med", 100 );
    }
    if( has_bionic( bio_leaky ) && one_turn_in( 6_minutes ) ) {
        mod_healthy_mod( -1, -200 );
    }
    if( has_bionic( bio_sleepy ) && one_turn_in( 50_minutes ) && !in_sleep_state() ) {
        mod_fatigue( 1 );
    }
    if( has_bionic( bio_itchy ) && one_turn_in( 50_minutes ) && !has_effect( effect_formication ) &&
        !has_effect( effect_narcosis ) ) {
        add_msg_if_player( m_bad, _( "Your malfunctioning bionic itches!" ) );
        body_part bp = random_body_part( true );
        add_effect( effect_formication, 10_minutes, bp );
    }
    if( has_bionic( bio_glowy ) && !has_effect( effect_glowy_led ) && one_turn_in( 50_minutes ) &&
        get_power_level() > 1_kJ ) {
        add_msg_if_player( m_bad, _( "Your malfunctioning bionic starts to glow!" ) );
        add_effect( effect_glowy_led, 5_minutes );
        mod_power_level( -1_kJ );
    }
}

void Character::suffer_from_artifacts()
{
    // Artifact effects
    if( has_artifact_with( AEP_ATTENTION ) ) {
        add_effect( effect_attention, 3_turns );
    }

    if( has_artifact_with( AEP_BAD_WEATHER ) && calendar::once_every( 1_minutes ) &&
        g->weather.weather != WEATHER_SNOWSTORM ) {
        g->weather.weather_override = WEATHER_SNOWSTORM;
        g->weather.set_nextweather( calendar::turn );
    }

    if( has_artifact_with( AEP_MUTAGENIC ) && one_turn_in( 48_hours ) ) {
        mutate();
    }
    if( has_artifact_with( AEP_FORCE_TELEPORT ) && one_turn_in( 1_hours ) ) {
        teleport::teleport( *this );
    }
}

void Character::suffer_from_stimulants( const int current_stim )
{
    // Stim +250 kills
    if( current_stim > 210 ) {
        if( one_turn_in( 2_minutes ) && !has_effect( effect_downed ) ) {
            add_msg_if_player( m_bad, _( "Your muscles spasm!" ) );
            if( !has_effect( effect_downed ) ) {
                add_msg_if_player( m_bad, _( "You fall to the ground!" ) );
                add_effect( effect_downed, rng( 6_turns, 20_turns ) );
            }
        }
    }
    if( current_stim > 170 ) {
        if( !has_effect( effect_winded ) && calendar::once_every( 10_minutes ) ) {
            add_msg( m_bad, _( "You feel short of breath." ) );
            add_effect( effect_winded, 10_minutes + 1_turns );
        }
    }
    if( current_stim > 110 ) {
        if( !has_effect( effect_shakes ) && calendar::once_every( 10_minutes ) ) {
            add_msg( _( "You shake uncontrollably." ) );
            add_effect( effect_shakes, 15_minutes + 1_turns );
        }
    }
    if( current_stim > 75 ) {
        if( !one_turn_in( 2_minutes ) && !has_effect( effect_nausea ) ) {
            add_msg( _( "You feel nauseous…" ) );
            add_effect( effect_nausea, 5_minutes );
        }
    }

    //stim -200 or painkillers 240 kills
    if( current_stim < -160 || get_painkiller() > 200 ) {
        if( one_turn_in( 3_minutes ) && !in_sleep_state() ) {
            add_msg_if_player( m_bad, _( "You black out!" ) );
            const time_duration dur = rng( 30_minutes, 60_minutes );
            add_effect( effect_downed, dur );
            add_effect( effect_blind, dur );
            fall_asleep( dur );
        }
    }
    if( current_stim < -120 || get_painkiller() > 160 ) {
        if( !has_effect( effect_winded ) && calendar::once_every( 10_minutes ) ) {
            add_msg( m_bad, _( "Your breathing slows down." ) );
            add_effect( effect_winded, 10_minutes + 1_turns );
        }
    }
    if( current_stim < -85 || get_painkiller() > 145 ) {
        if( one_turn_in( 15_seconds ) && !has_effect( effect_sleep ) ) {
            add_msg_if_player( m_bad, _( "You feel dizzy for a moment." ) );
            mod_moves( -rng( 10, 30 ) );
            if( one_in( 3 ) && !has_effect( effect_downed ) ) {
                add_msg_if_player( m_bad, _( "You stumble and fall over!" ) );
                add_effect( effect_downed, rng( 3_turns, 10_turns ) );
            }
        }
    }
    if( current_stim < -60 || get_painkiller() > 130 ) {
        if( calendar::once_every( 10_minutes ) ) {
            add_msg( m_warning, _( "You feel tired…" ) );
            mod_fatigue( rng( 1, 2 ) );
        }
    }
}

void Character::suffer_without_sleep( const int sleep_deprivation )
{
    // redo as a snippet?
    if( sleep_deprivation >= SLEEP_DEPRIVATION_HARMLESS ) {
        if( one_turn_in( 50_minutes ) ) {
            switch( dice( 1, 4 ) ) {
                default:
                case 1:
                    add_msg_player_or_npc( m_warning, _( "You tiredly rub your eyes." ),
                                           _( "<npcname> tiredly rubs their eyes." ) );
                    break;
                case 2:
                    add_msg_player_or_npc( m_warning, _( "You let out a small yawn." ),
                                           _( "<npcname> lets out a small yawn." ) );
                    break;
                case 3:
                    add_msg_player_or_npc( m_warning, _( "You stretch your back." ),
                                           _( "<npcname> stretches their back." ) );
                    break;
                case 4:
                    add_msg_player_or_npc( m_warning, _( "You feel mentally tired." ),
                                           _( "<npcname> lets out a huge yawn." ) );
                    break;
            }
        }
    }
    // Minor discomfort
    if( sleep_deprivation >= SLEEP_DEPRIVATION_MINOR ) {
        if( one_turn_in( 75_minutes ) ) {
            add_msg_if_player( m_warning, _( "You feel lightheaded for a moment." ) );
            moves -= 10;
        }
        if( one_turn_in( 100_minutes ) ) {
            add_msg_if_player( m_warning, _( "Your muscles spasm uncomfortably." ) );
            mod_pain( 2 );
        }
        if( !has_effect( effect_visuals ) && one_turn_in( 150_minutes ) ) {
            add_msg_if_player( m_warning, _( "Your vision blurs a little." ) );
            add_effect( effect_visuals, rng( 1_minutes, 5_minutes ) );
        }
    }
    // Slight disability
    if( sleep_deprivation >= SLEEP_DEPRIVATION_SERIOUS ) {
        if( one_turn_in( 75_minutes ) ) {
            add_msg_if_player( m_bad, _( "Your mind lapses into unawareness briefly." ) );
            moves -= rng( 20, 80 );
        }
        if( one_turn_in( 125_minutes ) ) {
            add_msg_if_player( m_bad, _( "Your muscles ache in stressfully unpredictable ways." ) );
            mod_pain( rng( 2, 10 ) );
        }
        if( one_turn_in( 5_hours ) ) {
            add_msg_if_player( m_bad, _( "You have a distractingly painful headache." ) );
            mod_pain( rng( 10, 25 ) );
        }
    }
    // Major disability, high chance of passing out also relevant
    if( sleep_deprivation >= SLEEP_DEPRIVATION_MAJOR ) {
        if( !has_effect( effect_nausea ) && one_turn_in( 500_minutes ) ) {
            add_msg_if_player( m_bad, _( "You feel heartburn and an acid taste in your mouth." ) );
            mod_pain( 5 );
            add_effect( effect_nausea, rng( 5_minutes, 30_minutes ) );
        }
        if( one_turn_in( 5_hours ) ) {
            add_msg_if_player( m_bad, _( "Your mind is so tired that you feel you can't trust "
                                         "your eyes anymore." ) );
            add_effect( effect_hallu, rng( 5_minutes, 60_minutes ) );
        }
        if( !has_effect( effect_shakes ) && one_turn_in( 425_minutes ) ) {
            add_msg_if_player( m_bad, _( "Your muscles spasm uncontrollably, and you have "
                                         "trouble keeping your balance." ) );
            add_effect( effect_shakes, 15_minutes );
        } else if( has_effect( effect_shakes ) && one_turn_in( 75_seconds ) ) {
            moves -= 10;
            add_msg_player_or_npc( m_warning, _( "Your shaking legs make you stumble." ),
                                   _( "<npcname> stumbles." ) );
            if( !has_effect( effect_downed ) && one_in( 10 ) ) {
                add_msg_player_or_npc( m_bad, _( "You fall over!" ), _( "<npcname> falls over!" ) );
                add_effect( effect_downed, rng( 3_turns, 10_turns ) );
            }
        }
    }
}

void Character::suffer_from_pain()
{
}

void Character::suffer()
{
    const int current_stim = get_stim();
    // TODO: Remove this section and encapsulate hp_cur
    for( int i = 0; i < num_hp_parts; i++ ) {
        body_part bp = hp_to_bp( static_cast<hp_part>( i ) );
        if( hp_cur[i] <= 0 ) {
            add_effect( effect_disabled, 1_turns, bp, true );
        }
    }

    for( size_t i = 0; i < get_bionics().size(); i++ ) {
        process_bionic( i );
    }

    for( std::pair<const trait_id, Character::trait_data> &mut : my_mutations ) {
        const mutation_branch &mdata = mut.first.obj();
        if( calendar::once_every( 1_minutes ) ) {
            suffer_water_damage( mdata );
        }
        Character::trait_data &tdata = mut.second;
        if( tdata.powered ) {
            suffer_mutation_power( mdata, tdata );
        }
    }

    if( underwater ) {
        suffer_while_underwater();
    }

    suffer_from_addictions();

    if( !in_sleep_state() ) {
        suffer_while_awake( current_stim );
    } // Done with while-awake-only effects

    if( has_trait( trait_ASTHMA ) ) {
        suffer_from_asthma( current_stim );
    }

    suffer_in_sunlight();
    suffer_from_other_mutations();
    suffer_from_artifacts();
    suffer_from_radiation();
    suffer_from_bad_bionics();
    suffer_from_stimulants( current_stim );
    int sleep_deprivation = in_sleep_state() ? 0 : get_sleep_deprivation();
    // Stimulants can lessen the PERCEIVED effects of sleep deprivation, but
    // they do nothing to cure it. As such, abuse is even more dangerous now.
    if( current_stim > 0 ) {
        // 100% of blacking out = 20160sd ; Max. stim modifier = 12500sd @ 250stim
        // Note: Very high stim already has its own slew of bad effects,
        // so the "useful" part of this bonus is actually lower.
        sleep_deprivation -= current_stim * 50;
    }

    suffer_without_sleep( sleep_deprivation );
    suffer_from_pain();
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
        const bool rad_resist = power_armored || worn_with_flag( "RAD_RESIST" );

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

        // Apply rads to any radiation badges.
        for( item *const it : inv_dump() ) {
            if( it->typeId() != "rad_badge" ) {
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
            if( inv.has_item( *it ) ) {
                continue;
            }

            // If the color hasn't changed, don't print anything.
            const std::string &col_before = rad_badge_color( before );
            const std::string &col_after = rad_badge_color( it->irradiation );
            if( col_before == col_after ) {
                continue;
            }

            add_msg_if_player( m_warning, _( "Your radiation badge changes from %1$s to %2$s!" ),
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
    for( int i = 0; i < num_hp_parts; i++ ) {
        if( is_limb_broken( static_cast<hp_part>( i ) ) ) {
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
        healing_factor *= addiction_scaling( 0.25f, 0.75f, addiction_level( add_type::CIG ) );
    }

    if( has_effect( effect_drunk ) ) {
        healing_factor *= 0.5;
    } else {
        healing_factor *= addiction_scaling( 0.25f, 0.75f, addiction_level( add_type::ALCOHOL ) );
    }

    if( get_rad() > 0 && !has_trait( trait_RADIOGENIC ) ) {
        healing_factor *= clamp( ( 1000.0f - get_rad() ) / 1000.0f, 0.0f, 1.0f );
    }

    // Bed rest speeds up mending
    if( has_effect( effect_sleep ) ) {
        healing_factor *= 4.0;
    } else if( get_fatigue() > DEAD_TIRED ) {
        // but being dead tired does not...
        healing_factor *= 0.75;
    } else {
        // If not dead tired, resting without sleep also helps
        healing_factor *= 1.0f + rest_quality();
    }

    // Being healthy helps.
    healing_factor *= 1.0f + get_healthy() / 200.0f;

    // Very hungry starts lowering the chance
    // square rooting the value makes the numbers drop off faster when below 1
    healing_factor *= std::sqrt( static_cast<float>( get_stored_kcal() ) / static_cast<float>
                                 ( get_healthy_kcal() ) );
    // Similar for thirst - starts at very thirsty, drops to 0 ~halfway between two last statuses
    healing_factor *= 1.0f - clamp( ( get_thirst() - 80.0f ) / 300.0f, 0.0f, 1.0f );

    // Mutagenic healing factor!
    bool needs_splint = true;

    healing_factor *= mutation_value( "mending_modifier" );

    if( has_trait( trait_REGEN_LIZ ) ) {
        needs_splint = false;
    }

    add_msg( m_debug, "Limb mend healing factor: %.2f", healing_factor );
    if( healing_factor <= 0.0f ) {
        // The section below assumes positive healing rate
        return;
    }

    for( int i = 0; i < num_hp_parts; i++ ) {
        const bool broken = is_limb_broken( static_cast<hp_part>( i ) );
        if( !broken ) {
            continue;
        }

        body_part part = hp_to_bp( static_cast<hp_part>( i ) );
        if( needs_splint && !worn_with_flag( "SPLINT", part ) ) {
            continue;
        }

        const time_duration dur_inc = 1_turns * roll_remainder( rate_multiplier * healing_factor );
        auto &eff = get_effect( effect_mending, part );
        if( eff.is_null() ) {
            add_effect( effect_mending, dur_inc, part, true );
            continue;
        }

        eff.set_duration( eff.get_duration() + dur_inc );

        if( eff.get_duration() >= eff.get_max_duration() ) {
            hp_cur[i] = 1;
            remove_effect( effect_mending, part );
            g->events().send<event_type::broken_bone_mends>( getID(), part );
            //~ %s is bodypart
            add_msg_if_player( m_good, _( "Your %s has started to mend!" ),
                               body_part_name( part ) );
        }
    }
}

void Character::sound_hallu()
{
    // Random 'dangerous' sound from a random direction
    // 1/5 chance to be a loud sound
    std::vector<std::string> dir{ "north",
                                  "northeast",
                                  "northwest",
                                  "south",
                                  "southeast",
                                  "southwest",
                                  "east",
                                  "west" };

    std::vector<std::string> dirz{ "and above you ", "and below you " };

    std::vector<std::tuple<std::string, std::string, std::string>> desc{
        std::make_tuple( "whump!", "smash_fail", "t_door_c" ),
        std::make_tuple( "crash!", "smash_success", "t_door_c" ),
        std::make_tuple( "glass breaking!", "smash_success", "t_window_domestic" ) };

    std::vector<std::tuple<std::string, std::string, std::string>> desc_big{
        std::make_tuple( "huge explosion!", "explosion", "default" ),
        std::make_tuple( "bang!", "fire_gun", "glock_19" ),
        std::make_tuple( "blam!", "fire_gun", "mossberg_500" ),
        std::make_tuple( "crash!", "smash_success", "t_wall" ),
        std::make_tuple( "SMASH!", "smash_success", "t_wall" ) };

    std::string i_dir = dir[rng( 0, dir.size() - 1 )];

    if( one_in( 10 ) ) {
        i_dir += " " + dirz[rng( 0, dirz.size() - 1 )];
    }

    std::string i_desc;
    std::pair<std::string, std::string> i_sound;
    if( one_in( 5 ) ) {
        int r_int = rng( 0, desc_big.size() - 1 );
        i_desc = std::get<0>( desc_big[r_int] );
        i_sound = std::make_pair( std::get<1>( desc_big[r_int] ), std::get<2>( desc_big[r_int] ) );
    } else {
        int r_int = rng( 0, desc.size() - 1 );
        i_desc = std::get<0>( desc[r_int] );
        i_sound = std::make_pair( std::get<1>( desc[r_int] ), std::get<2>( desc[r_int] ) );
    }

    add_msg( m_warning, _( "From the %1$s you hear %2$s" ), i_dir, i_desc );
    sfx::play_variant_sound( i_sound.first, i_sound.second, rng( 20, 80 ) );
}

void Character::drench( int saturation, const body_part_set &flags, bool ignore_waterproof )
{
    if( saturation < 1 ) {
        return;
    }

    // OK, water gets in your AEP suit or whatever.  It wasn't built to keep you dry.
    if( has_trait( trait_DEBUG_NOTEMP ) || has_active_mutation( trait_SHELL2 ) ||
        ( !ignore_waterproof && is_waterproof( flags ) ) ) {
        return;
    }

    // Make the body wet
    for( const body_part bp : all_body_parts ) {
        // Different body parts have different size, they can only store so much water
        int bp_wetness_max = 0;
        if( flags.test( bp ) ) {
            bp_wetness_max = drench_capacity[bp];
        }

        if( bp_wetness_max == 0 ) {
            continue;
        }
        // Different sources will only make the bodypart wet to a limit
        int source_wet_max = saturation * bp_wetness_max * 2 / 100;
        int wetness_increment = ignore_waterproof ? 100 : 2;
        // Respect maximums
        const int wetness_max = std::min( source_wet_max, bp_wetness_max );
        if( body_wetness[bp] < wetness_max ) {
            body_wetness[bp] = std::min( wetness_max, body_wetness[bp] + wetness_increment );
        }
    }

    if( body_wetness[bp_torso] >= drench_capacity[bp_torso] / 2.0 &&
        has_effect( effect_masked_scent ) &&
        get_value( "waterproof_scent" ).empty() ) {
        add_msg_if_player( m_info, _( "The water wash away the scent." ) );
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

void Character::apply_wetness_morale( int temperature )
{
    // First, a quick check if we have any wetness to calculate morale from
    // Faster than checking all worn items for friendliness
    if( !std::any_of( body_wetness.begin(), body_wetness.end(),
    []( const int w ) {
    return w != 0;
} ) ) {
        return;
    }

    // Normalize temperature to [-1.0,1.0]
    temperature = std::max( 0, std::min( 100, temperature ) );
    const double global_temperature_mod = -1.0 + ( 2.0 * temperature / 100.0 );

    int total_morale = 0;
    const auto wet_friendliness = exclusive_flag_coverage( "WATER_FRIENDLY" );
    for( const body_part bp : all_body_parts ) {
        // Sum of body wetness can go up to 103
        const int part_drench = body_wetness[bp];
        if( part_drench == 0 ) {
            continue;
        }

        const auto &part_arr = mut_drench[bp];
        const int part_ignored = part_arr[WT_IGNORED];
        const int part_neutral = part_arr[WT_NEUTRAL];
        const int part_good    = part_arr[WT_GOOD];

        if( part_ignored >= part_drench ) {
            continue;
        }

        int bp_morale = 0;
        const bool is_friendly = wet_friendliness.test( bp );
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

        // Clamp to [COLD,HOT] and cast to double
        const double part_temperature =
            std::min( BODYTEMP_HOT, std::max( BODYTEMP_COLD, temp_cur[bp] ) );
        // 0.0 at COLD, 1.0 at HOT
        const double part_mod = ( part_temperature - BODYTEMP_COLD ) /
                                ( BODYTEMP_HOT - BODYTEMP_COLD );
        // Average of global and part temperature modifiers, each in range [-1.0, 1.0]
        double scaled_temperature = ( global_temperature_mod + part_mod ) / 2;

        if( bp_morale < 0 ) {
            // Damp, hot clothing on hot skin feels bad
            scaled_temperature = std::fabs( scaled_temperature );
        }

        // For an unmutated human swimming in deep water, this will add up to:
        // +51 when hot in 100% water friendly clothing
        // -103 when cold/hot in 100% unfriendly clothing
        total_morale += static_cast<int>( bp_morale * ( 1.0 + scaled_temperature ) / 2.0 );
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

void Character::add_addiction( add_type type, int strength )
{
    if( type == add_type::NONE ) {
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
    for( auto &i : addictions ) {
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

        add_msg( m_debug, "Updating addiction: %d intensity, %d sated",
                 i.intensity, to_turns<int>( i.sated ) );

        return;
    }

    // Add a new addiction
    const int roll = rng( 0, 100 );
    add_msg( m_debug, "Addiction: roll %d vs strength %d", roll, strength );
    if( roll < strength ) {
        const std::string &type_name = addiction_type_name( type );
        add_msg( m_debug, "%s got addicted to %s", disp_name(), type_name );
        addictions.emplace_back( type, 1 );
        g->events().send<event_type::gains_addiction>( getID(), type );
    }
}

bool Character::has_addiction( add_type type ) const
{
    return std::any_of( addictions.begin(), addictions.end(),
    [type]( const addiction & ad ) {
        return ad.type == type && ad.intensity >= MIN_ADDICTION_LEVEL;
    } );
}

void Character::rem_addiction( add_type type )
{
    auto iter = std::find_if( addictions.begin(), addictions.end(),
    [type]( const addiction & ad ) {
        return ad.type == type;
    } );

    if( iter != addictions.end() ) {
        addictions.erase( iter );
        g->events().send<event_type::loses_addiction>( getID(), type );
    }
}

int Character::addiction_level( add_type type ) const
{
    auto iter = std::find_if( addictions.begin(), addictions.end(),
    [type]( const addiction & ad ) {
        return ad.type == type;
    } );
    return iter != addictions.end() ? iter->intensity : 0;
}

int  Character::leak_level( const std::string &flag ) const
{
    int leak_level = 0;
    leak_level = inv.leak_level( flag );
    return leak_level;
}
