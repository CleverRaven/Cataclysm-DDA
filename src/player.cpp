#include "player.h"

#include <algorithm>
#include <array>
#include <bitset>
#include <cmath>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <unordered_map>

#include "action.h"
#include "activity_handlers.h"
#include "ammo.h"
#include "avatar.h"
#include "avatar_action.h"
#include "bionics.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "character_martial_arts.h"
#include "craft_command.h"
#include "cursesdef.h"
#include "debug.h"
#include "effect.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "faction.h"
#include "fault.h"
#include "field_type.h"
#include "game.h"
#include "gun_mode.h"
#include "handle_liquid.h"
#include "input.h"
#include "int_id.h"
#include "inventory.h"
#include "item.h"
#include "item_contents.h"
#include "item_location.h"
#include "itype.h"
#include "lightmap.h"
#include "line.h"
#include "magic.h"
#include "magic_enchantment.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "martialarts.h"
#include "math_defines.h"
#include "messages.h"
#include "monster.h"
#include "morale.h"
#include "mtype.h"
#include "mutation.h"
#include "npc.h"
#include "options.h"
#include "output.h"
#include "overmap_types.h"
#include "overmapbuffer.h"
#include "pickup.h"
#include "profession.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "requirements.h"
#include "rng.h"
#include "skill.h"
#include "stomach.h"
#include "string_formatter.h"
#include "string_id.h"
#include "translations.h"
#include "trap.h"
#include "ui.h"
#include "uistate.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_type.h"
#include "vehicle.h"
#include "visitable.h"
#include "vitamin.h"
#include "vpart_position.h"
#include "weather.h"
#include "weather_gen.h"

static const efftype_id effect_adrenaline( "adrenaline" );
static const efftype_id effect_bandaged( "bandaged" );
static const efftype_id effect_bite( "bite" );
static const efftype_id effect_blind( "blind" );
static const efftype_id effect_bloodworms( "bloodworms" );
static const efftype_id effect_boomered( "boomered" );
static const efftype_id effect_brainworms( "brainworms" );
static const efftype_id effect_darkness( "darkness" );
static const efftype_id effect_deaf( "deaf" );
static const efftype_id effect_dermatik( "dermatik" );
static const efftype_id effect_disinfected( "disinfected" );
static const efftype_id effect_downed( "downed" );
static const efftype_id effect_drunk( "drunk" );
static const efftype_id effect_earphones( "earphones" );
static const efftype_id effect_fungus( "fungus" );
static const efftype_id effect_grabbed( "grabbed" );
static const efftype_id effect_grabbing( "grabbing" );
static const efftype_id effect_infected( "infected" );
static const efftype_id effect_masked_scent( "masked_scent" );
static const efftype_id effect_mending( "mending" );
static const efftype_id effect_meth( "meth" );
static const efftype_id effect_narcosis( "narcosis" );
static const efftype_id effect_nausea( "nausea" );
static const efftype_id effect_onfire( "onfire" );
static const efftype_id effect_paincysts( "paincysts" );
static const efftype_id effect_recover( "recover" );
static const efftype_id effect_sleep( "sleep" );
static const efftype_id effect_stunned( "stunned" );
static const efftype_id effect_tapeworm( "tapeworm" );
static const efftype_id effect_weed_high( "weed_high" );

static const trait_id trait_ACIDBLOOD( "ACIDBLOOD" );
static const trait_id trait_ANTENNAE( "ANTENNAE" );
static const trait_id trait_DEBUG_NODMG( "DEBUG_NODMG" );

static const trait_id trait_CANNIBAL( "CANNIBAL" );
static const trait_id trait_CENOBITE( "CENOBITE" );
static const trait_id trait_CF_HAIR( "CF_HAIR" );
static const trait_id trait_CHITIN_FUR( "CHITIN_FUR" );
static const trait_id trait_CHITIN_FUR2( "CHITIN_FUR2" );
static const trait_id trait_CHITIN_FUR3( "CHITIN_FUR3" );
static const trait_id trait_CHLOROMORPH( "CHLOROMORPH" );
static const trait_id trait_CLUMSY( "CLUMSY" );
static const trait_id trait_COLDBLOOD4( "COLDBLOOD4" );
static const trait_id trait_DEBUG_BIONIC_POWER( "DEBUG_BIONIC_POWER" );
static const trait_id trait_DEBUG_CLOAK( "DEBUG_CLOAK" );
static const trait_id trait_DEBUG_HS( "DEBUG_HS" );
static const trait_id trait_DEFT( "DEFT" );
static const trait_id trait_EASYSLEEPER( "EASYSLEEPER" );
static const trait_id trait_EASYSLEEPER2( "EASYSLEEPER2" );
static const trait_id trait_EATHEALTH( "EATHEALTH" );
static const trait_id trait_FASTLEARNER( "FASTLEARNER" );
static const trait_id trait_FAT( "FAT" );
static const trait_id trait_FELINE_FUR( "FELINE_FUR" );
static const trait_id trait_FUR( "FUR" );
static const trait_id trait_HATES_BOOKS( "HATES_BOOKS" );
static const trait_id trait_HOOVES( "HOOVES" );
static const trait_id trait_HUGE( "HUGE" );
static const trait_id trait_HUGE_OK( "HUGE_OK" );
static const trait_id trait_INFIMMUNE( "INFIMMUNE" );
static const trait_id trait_INSOMNIA( "INSOMNIA" );
static const trait_id trait_INT_SLIME( "INT_SLIME" );
static const trait_id trait_LARGE( "LARGE" );
static const trait_id trait_LARGE_OK( "LARGE_OK" );
static const trait_id trait_LEG_TENTACLES( "LEG_TENTACLES" );
static const trait_id trait_LIGHTFUR( "LIGHTFUR" );
static const trait_id trait_LIGHTSTEP( "LIGHTSTEP" );
static const trait_id trait_LOVES_BOOKS( "LOVES_BOOKS" );
static const trait_id trait_LUPINE_FUR( "LUPINE_FUR" );
static const trait_id trait_M_IMMUNE( "M_IMMUNE" );
static const trait_id trait_M_SKIN3( "M_SKIN3" );
static const trait_id trait_MOREPAIN( "MORE_PAIN" );
static const trait_id trait_MOREPAIN2( "MORE_PAIN2" );
static const trait_id trait_MOREPAIN3( "MORE_PAIN3" );
static const trait_id trait_NAUSEA( "NAUSEA" );
static const trait_id trait_NOMAD( "NOMAD" );
static const trait_id trait_NOMAD2( "NOMAD2" );
static const trait_id trait_NOMAD3( "NOMAD3" );
static const trait_id trait_NOPAIN( "NOPAIN" );
static const trait_id trait_PACIFIST( "PACIFIST" );
static const trait_id trait_PADDED_FEET( "PADDED_FEET" );
static const trait_id trait_PAINRESIST( "PAINRESIST" );
static const trait_id trait_PAINRESIST_TROGLO( "PAINRESIST_TROGLO" );
static const trait_id trait_PARAIMMUNE( "PARAIMMUNE" );
static const trait_id trait_PARKOUR( "PARKOUR" );
static const trait_id trait_PER_SLIME( "PER_SLIME" );
static const trait_id trait_PER_SLIME_OK( "PER_SLIME_OK" );
static const trait_id trait_PROF_SKATER( "PROF_SKATER" );
static const trait_id trait_PSYCHOPATH( "PSYCHOPATH" );
static const trait_id trait_QUILLS( "QUILLS" );
static const trait_id trait_ROOTS2( "ROOTS2" );
static const trait_id trait_ROOTS3( "ROOTS3" );
static const trait_id trait_SAPIOVORE( "SAPIOVORE" );
static const trait_id trait_SAVANT( "SAVANT" );
static const trait_id trait_SHELL2( "SHELL2" );
static const trait_id trait_SLIMY( "SLIMY" );
static const trait_id trait_SLOWLEARNER( "SLOWLEARNER" );
static const trait_id trait_SPINES( "SPINES" );
static const trait_id trait_SPIRITUAL( "SPIRITUAL" );
static const trait_id trait_STRONGSTOMACH( "STRONGSTOMACH" );
static const trait_id trait_SUNLIGHT_DEPENDENT( "SUNLIGHT_DEPENDENT" );
static const trait_id trait_THORNS( "THORNS" );
static const trait_id trait_THRESH_SPIDER( "THRESH_SPIDER" );
static const trait_id trait_TOUGH_FEET( "TOUGH_FEET" );
static const trait_id trait_URSINE_FUR( "URSINE_FUR" );
static const trait_id trait_VOMITOUS( "VOMITOUS" );
static const trait_id trait_WATERSLEEP( "WATERSLEEP" );
static const trait_id trait_WEAKSTOMACH( "WEAKSTOMACH" );
static const trait_id trait_WEB_SPINNER( "WEB_SPINNER" );
static const trait_id trait_WEB_WALKER( "WEB_WALKER" );
static const trait_id trait_WEB_WEAVER( "WEB_WEAVER" );

static const std::string flag_SPLINT( "SPLINT" );

static const skill_id skill_dodge( "dodge" );
static const skill_id skill_gun( "gun" );
static const skill_id skill_swimming( "swimming" );

static const bionic_id bio_cloak( "bio_cloak" );
static const bionic_id bio_cqb( "bio_cqb" );
static const bionic_id bio_earplugs( "bio_earplugs" );
static const bionic_id bio_ears( "bio_ears" );
static const bionic_id bio_ground_sonar( "bio_ground_sonar" );
static const bionic_id bio_jointservo( "bio_jointservo" );
static const bionic_id bio_memory( "bio_memory" );
static const bionic_id bio_soporific( "bio_soporific" );
static const bionic_id bio_speed( "bio_speed" );
static const bionic_id bio_syringe( "bio_syringe" );
static const bionic_id bio_uncanny_dodge( "bio_uncanny_dodge" );

stat_mod player::get_pain_penalty() const
{
    stat_mod ret;
    int pain = get_perceived_pain();
    if( pain <= 0 ) {
        return ret;
    }

    int stat_penalty = std::floor( std::pow( pain, 0.8f ) / 10.0f );

    bool ceno = has_trait( trait_CENOBITE );
    if( !ceno ) {
        ret.strength = stat_penalty;
        ret.dexterity = stat_penalty;
    }

    if( !has_trait( trait_INT_SLIME ) ) {
        ret.intelligence = 1 + stat_penalty;
    } else {
        ret.intelligence = 1 + pain / 5;
    }

    ret.perception = stat_penalty * 2 / 3;

    ret.speed = std::pow( pain, 0.7f );
    if( ceno ) {
        ret.speed /= 2;
    }

    ret.speed = std::min( ret.speed, 50 );
    return ret;
}

player::player()
{
    str_cur = 8;
    str_max = 8;
    dex_cur = 8;
    dex_max = 8;
    int_cur = 8;
    int_max = 8;
    per_cur = 8;
    per_max = 8;
    dodges_left = 1;
    blocks_left = 1;
    set_power_level( 0_kJ );
    set_max_power_level( 0_kJ );
    cash = 0;
    scent = 500;
    male = true;
    prof = profession::has_initialized() ? profession::generic() :
           nullptr; //workaround for a potential structural limitation, see player::create

    start_location = start_location_id( "sloc_shelter" );
    moves = 100;
    movecounter = 0;
    oxygen = 0;
    in_vehicle = false;
    controlling_vehicle = false;
    grab_point = tripoint_zero;
    hauling = false;
    focus_pool = 100;
    last_item = itype_id( "null" );
    sight_max = 9999;
    last_batch = 0;
    lastconsumed = itype_id( "null" );
    next_expected_position = cata::nullopt;
    death_drops = true;

    nv_cached = false;
    volume = 0;

    set_value( "THIEF_MODE", "THIEF_ASK" );

    for( const auto &v : vitamin::all() ) {
        vitamin_levels[ v.first ] = 0;
    }

    recalc_sight_limits();
    reset_encumbrance();
}

player::~player() = default;
player::player( player && ) = default;
player &player::operator=( player && ) = default;

void player::normalize()
{
    Character::normalize();

    recalc_hp();

    temp_conv.fill( BODYTEMP_NORM );
    set_stamina( get_stamina_max() );
}

void player::process_turn()
{
    // Has to happen before reset_stats
    clear_miss_reasons();

    Character::process_turn();

    // If we're actively handling something we can't just drop it on the ground
    // in the middle of handling it
    if( activity.targets.empty() ) {
        drop_invalid_inventory();
    }
    process_items();
    // Didn't just pick something up
    last_item = itype_id( "null" );

    if( !is_npc() && has_trait( trait_DEBUG_BIONIC_POWER ) ) {
        mod_power_level( get_max_power_level() );
    }

    visit_items( [this]( item * e ) {
        e->process_artifact( this, pos() );
        e->process_relic( this );
        return VisitResponse::NEXT;
    } );

    suffer();
    // NPCs currently don't make any use of their scent, pointless to calculate it
    // TODO: make use of NPC scent.
    if( !is_npc() ) {
        if( !has_effect( effect_masked_scent ) ) {
            restore_scent();
        }
        const int mask_intensity = get_effect_int( effect_masked_scent );

        // Set our scent towards the norm
        int norm_scent = 500;
        int temp_norm_scent = INT_MIN;
        bool found_intensity = false;
        for( const trait_id &mut : get_mutations() ) {
            const cata::optional<int> &scent_intensity = mut->scent_intensity;
            if( scent_intensity ) {
                found_intensity = true;
                temp_norm_scent = std::max( temp_norm_scent, *scent_intensity );
            }
        }
        if( found_intensity ) {
            norm_scent = temp_norm_scent;
        }

        for( const trait_id &mut : get_mutations() ) {
            const cata::optional<int> &scent_mask = mut->scent_mask;
            if( scent_mask ) {
                norm_scent += *scent_mask;
            }
        }

        //mask from scent altering items;
        norm_scent += mask_intensity;

        // Scent increases fast at first, and slows down as it approaches normal levels.
        // Estimate it will take about norm_scent * 2 turns to go from 0 - norm_scent / 2
        // Without smelly trait this is about 1.5 hrs. Slows down significantly after that.
        if( scent < rng( 0, norm_scent ) ) {
            scent++;
        }

        // Unusually high scent decreases steadily until it reaches normal levels.
        if( scent > norm_scent ) {
            scent--;
        }

        for( const trait_id &mut : get_mutations() ) {
            scent *= mut.obj().scent_modifier;
        }
    }

    // We can dodge again! Assuming we can actually move...
    if( in_sleep_state() ) {
        blocks_left = 0;
        dodges_left = 0;
    } else if( moves > 0 ) {
        blocks_left = get_num_blocks();
        dodges_left = get_num_dodges();
    }

    // auto-learning. This is here because skill-increases happens all over the place:
    // SkillLevel::readBook (has no connection to the skill or the player),
    // player::read, player::practice, ...
    // Check for spontaneous discovery of martial art styles
    for( auto &style : autolearn_martialart_types() ) {
        const matype_id &ma( style );

        if( !martial_arts_data.has_martialart( ma ) && can_autolearn( ma ) ) {
            martial_arts_data.add_martialart( ma );
            add_msg_if_player( m_info, _( "You have learned a new style: %s!" ), ma.obj().name );
        }
    }

    // Update time spent conscious in this overmap tile for the Nomad traits.
    if( !is_npc() && ( has_trait( trait_NOMAD ) || has_trait( trait_NOMAD2 ) ||
                       has_trait( trait_NOMAD3 ) ) &&
        !has_effect( effect_sleep ) && !has_effect( effect_narcosis ) ) {
        const tripoint ompos = global_omt_location();
        const point pos = ompos.xy();
        if( overmap_time.find( pos ) == overmap_time.end() ) {
            overmap_time[pos] = 1_turns;
        } else {
            overmap_time[pos] += 1_turns;
        }
    }
    // Decay time spent in other overmap tiles.
    if( !is_npc() && calendar::once_every( 1_hours ) ) {
        const tripoint ompos = global_omt_location();
        const time_point now = calendar::turn;
        time_duration decay_time = 0_days;
        if( has_trait( trait_NOMAD ) ) {
            decay_time = 7_days;
        } else if( has_trait( trait_NOMAD2 ) ) {
            decay_time = 14_days;
        } else if( has_trait( trait_NOMAD3 ) ) {
            decay_time = 28_days;
        }
        auto it = overmap_time.begin();
        while( it != overmap_time.end() ) {
            if( it->first.x == ompos.x && it->first.y == ompos.y ) {
                it++;
                continue;
            }
            // Find the amount of time passed since the player touched any of the overmap tile's submaps.
            const tripoint tpt = tripoint( it->first, 0 );
            const time_point last_touched = overmap_buffer.scent_at( tpt ).creation_time;
            const time_duration since_visit = now - last_touched;
            // If the player has spent little time in this overmap tile, let it decay after just an hour instead of the usual extended decay time.
            const time_duration modified_decay_time = it->second > 5_minutes ? decay_time : 1_hours;
            if( since_visit > modified_decay_time ) {
                // Reduce the tracked time spent in this overmap tile.
                const time_duration decay_amount = std::min( since_visit - modified_decay_time, 1_hours );
                const time_duration updated_value = it->second - decay_amount;
                if( updated_value <= 0_turns ) {
                    // We can stop tracking this tile if there's no longer any time recorded there.
                    it = overmap_time.erase( it );
                    continue;
                } else {
                    it->second = updated_value;
                }
            }
            it++;
        }
    }
}

int player::kcal_speed_penalty()
{
    static const std::vector<std::pair<float, float>> starv_thresholds = { {
            std::make_pair( 0.0f, -90.0f ),
            std::make_pair( character_weight_category::emaciated, -50.f ),
            std::make_pair( character_weight_category::underweight, -25.0f ),
            std::make_pair( character_weight_category::normal, 0.0f )
        }
    };
    if( get_kcal_percent() > 0.95f ) {
        // TODO: get speed penalties for being too fat, too
        return 0;
    } else {
        return std::round( multi_lerp( starv_thresholds, get_bmi() ) );
    }
}

int player::thirst_speed_penalty( int thirst )
{
    // We die at 1200 thirst
    // Start by dropping speed really fast, but then level it off a bit
    static const std::vector<std::pair<float, float>> thirst_thresholds = {{
            std::make_pair( 40.0f, 0.0f ),
            std::make_pair( 300.0f, -25.0f ),
            std::make_pair( 600.0f, -50.0f ),
            std::make_pair( 1200.0f, -75.0f )
        }
    };
    return static_cast<int>( multi_lerp( thirst_thresholds, thirst ) );
}

void player::recalc_speed_bonus()
{
    // Minus some for weight...
    int carry_penalty = 0;
    if( weight_carried() > weight_capacity() && !has_trait( trait_id( "DEBUG_STORAGE" ) ) ) {
        carry_penalty = 25 * ( weight_carried() - weight_capacity() ) / ( weight_capacity() );
    }
    mod_speed_bonus( -carry_penalty );

    mod_speed_bonus( -get_pain_penalty().speed );

    if( get_thirst() > 40 ) {
        mod_speed_bonus( thirst_speed_penalty( get_thirst() ) );
    }
    // fat or underweight, you get slower. cumulative with hunger
    mod_speed_bonus( kcal_speed_penalty() );

    for( const auto &maps : *effects ) {
        for( auto i : maps.second ) {
            bool reduced = resists_effect( i.second );
            mod_speed_bonus( i.second.get_mod( "SPEED", reduced ) );
        }
    }

    // add martial arts speed bonus
    mod_speed_bonus( mabuff_speed_bonus() );

    // Not sure why Sunlight Dependent is here, but OK
    // Ectothermic/COLDBLOOD4 is intended to buff folks in the Summer
    // Threshold-crossing has its charms ;-)
    if( g != nullptr ) {
        if( has_trait( trait_SUNLIGHT_DEPENDENT ) && !g->is_in_sunlight( pos() ) ) {
            mod_speed_bonus( -( g->light_level( posz() ) >= 12 ? 5 : 10 ) );
        }
        const float temperature_speed_modifier = mutation_value( "temperature_speed_modifier" );
        if( temperature_speed_modifier != 0 ) {
            const auto player_local_temp = g->weather.get_temperature( pos() );
            if( has_trait( trait_COLDBLOOD4 ) || player_local_temp < 65 ) {
                mod_speed_bonus( ( player_local_temp - 65 ) * temperature_speed_modifier );
            }
        }
    }

    if( has_artifact_with( AEP_SPEED_UP ) ) {
        mod_speed_bonus( 20 );
    }
    if( has_artifact_with( AEP_SPEED_DOWN ) ) {
        mod_speed_bonus( -20 );
    }

    float speed_modifier = Character::mutation_value( "speed_modifier" );
    set_speed_bonus( static_cast<int>( get_speed() * speed_modifier ) - get_speed_base() );

    if( has_bionic( bio_speed ) ) { // multiply by 1.1
        set_speed_bonus( static_cast<int>( get_speed() * 1.1 ) - get_speed_base() );
    }

    // Speed cannot be less than 25% of base speed, so minimal speed bonus is -75% base speed.
    const int min_speed_bonus = static_cast<int>( -0.75 * get_speed_base() );
    if( get_speed_bonus() < min_speed_bonus ) {
        set_speed_bonus( min_speed_bonus );
    }
}

float player::stability_roll() const
{
    /** @EFFECT_STR improves player stability roll */

    /** @EFFECT_PER slightly improves player stability roll */

    /** @EFFECT_DEX slightly improves player stability roll */

    /** @EFFECT_MELEE improves player stability roll */
    return get_melee() + get_str() + ( get_per() / 3.0f ) + ( get_dex() / 4.0f );
}

double player::recoil_vehicle() const
{
    // TODO: vary penalty dependent upon vehicle part on which player is boarded

    if( in_vehicle ) {
        if( const optional_vpart_position vp = g->m.veh_at( pos() ) ) {
            return static_cast<double>( std::abs( vp->vehicle().velocity ) ) * 3 / 100;
        }
    }
    return 0;
}

double player::recoil_total() const
{
    return recoil + recoil_vehicle();
}

bool player::is_hallucination() const
{
    return false;
}

void player::set_underwater( bool u )
{
    if( underwater != u ) {
        underwater = u;
        recalc_sight_limits();
    }
}

nc_color player::basic_symbol_color() const
{
    if( has_effect( effect_onfire ) ) {
        return c_red;
    }
    if( has_effect( effect_stunned ) ) {
        return c_light_blue;
    }
    if( has_effect( effect_boomered ) ) {
        return c_pink;
    }
    if( has_active_mutation( trait_id( "SHELL2" ) ) ) {
        return c_magenta;
    }
    if( underwater ) {
        return c_blue;
    }
    if( has_active_bionic( bio_cloak ) || has_artifact_with( AEP_INVISIBLE ) ||
        is_wearing_active_optcloak() || has_trait( trait_DEBUG_CLOAK ) ) {
        return c_dark_gray;
    }
    if( move_mode == CMM_RUN ) {
        return c_yellow;
    }
    if( move_mode == CMM_CROUCH ) {
        return c_light_gray;
    }
    return c_white;
}

void player::mod_stat( const std::string &stat, float modifier )
{
    if( stat == "thirst" ) {
        mod_thirst( modifier );
    } else if( stat == "fatigue" ) {
        mod_fatigue( modifier );
    } else if( stat == "oxygen" ) {
        oxygen += modifier;
    } else if( stat == "stamina" ) {
        mod_stamina( modifier );
    } else {
        // Fall through to the creature method.
        Character::mod_stat( stat, modifier );
    }
}

time_duration player::estimate_effect_dur( const skill_id &relevant_skill,
        const efftype_id &target_effect, const time_duration &error_magnitude,
        int threshold, const Creature &target ) const
{
    const time_duration zero_duration = 0_turns;

    int skill_lvl = get_skill_level( relevant_skill );

    time_duration estimate = std::max( zero_duration, target.get_effect_dur( target_effect ) +
                                       rng( -1, 1 ) * error_magnitude *
                                       rng( 0, std::max( 0, threshold - skill_lvl ) ) );
    return estimate;
}

bool player::has_conflicting_trait( const trait_id &flag ) const
{
    return ( has_opposite_trait( flag ) || has_lower_trait( flag ) || has_higher_trait( flag ) ||
             has_same_type_trait( flag ) );
}

bool player::has_lower_trait( const trait_id &flag ) const
{
    for( auto &i : flag->prereqs ) {
        if( has_trait( i ) || has_lower_trait( i ) ) {
            return true;
        }
    }
    return false;
}

bool player::has_higher_trait( const trait_id &flag ) const
{
    for( auto &i : flag->replacements ) {
        if( has_trait( i ) || has_higher_trait( i ) ) {
            return true;
        }
    }
    return false;
}

bool player::has_same_type_trait( const trait_id &flag ) const
{
    for( auto &i : get_mutations_in_types( flag->types ) ) {
        if( has_trait( i ) && flag != i ) {
            return true;
        }
    }
    return false;
}

bool player::purifiable( const trait_id &flag ) const
{
    return flag->purifiable;
}

/// Returns a randomly selected dream
std::string player::get_category_dream( const std::string &cat,
                                        int strength ) const
{
    std::vector<dream> valid_dreams;
    //Pull the list of dreams
    for( auto &i : dreams ) {
        //Pick only the ones matching our desired category and strength
        if( ( i.category == cat ) && ( i.strength == strength ) ) {
            // Put the valid ones into our list
            valid_dreams.push_back( i );
        }
    }
    if( valid_dreams.empty() ) {
        return "";
    }
    const dream &selected_dream = random_entry( valid_dreams );
    return random_entry( selected_dream.messages() );
}

std::list<item *> player::get_radio_items()
{
    std::list<item *> rc_items;
    const invslice &stacks = inv.slice();
    for( auto &stack : stacks ) {
        item &stack_iter = stack->front();
        if( stack_iter.has_flag( "RADIO_ACTIVATION" ) ) {
            rc_items.push_back( &stack_iter );
        }
    }

    for( auto &elem : worn ) {
        if( elem.has_flag( "RADIO_ACTIVATION" ) ) {
            rc_items.push_back( &elem );
        }
    }

    if( is_armed() ) {
        if( weapon.has_flag( "RADIO_ACTIVATION" ) ) {
            rc_items.push_back( &weapon );
        }
    }
    return rc_items;
}

std::list<item *> player::get_artifact_items()
{
    std::list<item *> art_items;
    const invslice &stacks = inv.slice();
    for( auto &stack : stacks ) {
        item &stack_iter = stack->front();
        if( stack_iter.is_artifact() ) {
            art_items.push_back( &stack_iter );
        }
    }
    for( auto &elem : worn ) {
        if( elem.is_artifact() ) {
            art_items.push_back( &elem );
        }
    }
    if( is_armed() ) {
        if( weapon.is_artifact() ) {
            art_items.push_back( &weapon );
        }
    }
    return art_items;
}

bool player::avoid_trap( const tripoint &pos, const trap &tr ) const
{
    /** @EFFECT_DEX increases chance to avoid traps */

    /** @EFFECT_DODGE increases chance to avoid traps */
    int myroll = dice( 3, dex_cur + get_skill_level( skill_dodge ) * 1.5 );
    int traproll;
    if( tr.can_see( pos, *this ) ) {
        traproll = dice( 3, tr.get_avoidance() );
    } else {
        traproll = dice( 6, tr.get_avoidance() );
    }

    if( has_trait( trait_LIGHTSTEP ) ) {
        myroll += dice( 2, 6 );
    }

    if( has_trait( trait_CLUMSY ) ) {
        myroll -= dice( 2, 6 );
    }

    return myroll >= traproll;
}

void player::pause()
{
    moves = 0;
    recoil = MAX_RECOIL;

    // Train swimming if underwater
    if( !in_vehicle ) {
        if( underwater ) {
            practice( skill_swimming, 1 );
            drench( 100, { {
                    bp_leg_l, bp_leg_r, bp_torso, bp_arm_l,
                    bp_arm_r, bp_head, bp_eyes, bp_mouth,
                    bp_foot_l, bp_foot_r, bp_hand_l, bp_hand_r
                }
            }, true );
        } else if( g->m.has_flag( TFLAG_DEEP_WATER, pos() ) ) {
            practice( skill_swimming, 1 );
            // Same as above, except no head/eyes/mouth
            drench( 100, { {
                    bp_leg_l, bp_leg_r, bp_torso, bp_arm_l,
                    bp_arm_r, bp_foot_l, bp_foot_r, bp_hand_l,
                    bp_hand_r
                }
            }, true );
        } else if( g->m.has_flag( "SWIMMABLE", pos() ) ) {
            drench( 40, { { bp_foot_l, bp_foot_r, bp_leg_l, bp_leg_r } }, false );
        }
    }

    // Try to put out clothing/hair fire
    if( has_effect( effect_onfire ) ) {
        time_duration total_removed = 0_turns;
        time_duration total_left = 0_turns;
        bool on_ground = has_effect( effect_downed );
        for( const body_part bp : all_body_parts ) {
            effect &eff = get_effect( effect_onfire, bp );
            if( eff.is_null() ) {
                continue;
            }

            // TODO: Tools and skills
            total_left += eff.get_duration();
            // Being on the ground will smother the fire much faster because you can roll
            const time_duration dur_removed = on_ground ? eff.get_duration() / 2 + 2_turns : 1_turns;
            eff.mod_duration( -dur_removed );
            total_removed += dur_removed;
        }

        // Don't drop on the ground when the ground is on fire
        if( total_left > 1_minutes && !is_dangerous_fields( g->m.field_at( pos() ) ) ) {
            add_effect( effect_downed, 2_turns, num_bp, false, 0, true );
            add_msg_player_or_npc( m_warning,
                                   _( "You roll on the ground, trying to smother the fire!" ),
                                   _( "<npcname> rolls on the ground!" ) );
        } else if( total_removed > 0_turns ) {
            add_msg_player_or_npc( m_warning,
                                   _( "You attempt to put out the fire on you!" ),
                                   _( "<npcname> attempts to put out the fire on them!" ) );
        }
    }

    // on-pause effects for martial arts
    martial_arts_data.ma_onpause_effects( *this );

    if( is_npc() ) {
        // The stuff below doesn't apply to NPCs
        // search_surroundings should eventually do, though
        return;
    }

    if( in_vehicle && one_in( 8 ) ) {
        VehicleList vehs = g->m.get_vehicles();
        vehicle *veh = nullptr;
        for( auto &v : vehs ) {
            veh = v.v;
            if( veh && veh->is_moving() && veh->player_in_control( *this ) ) {
                double exp_temp = 1 + veh->total_mass() / 400.0_kilogram +
                                  std::abs( veh->velocity / 3200.0 );
                int experience = static_cast<int>( exp_temp );
                if( exp_temp - experience > 0 && x_in_y( exp_temp - experience, 1.0 ) ) {
                    experience++;
                }
                practice( skill_id( "driving" ), experience );
                break;
            }
        }
    }

    search_surroundings();
}

void player::search_surroundings()
{
    if( controlling_vehicle ) {
        return;
    }
    // Search for traps in a larger area than before because this is the only
    // way we can "find" traps that aren't marked as visible.
    // Detection formula takes care of likelihood of seeing within this range.
    for( const tripoint &tp : g->m.points_in_radius( pos(), 5 ) ) {
        const trap &tr = g->m.tr_at( tp );
        if( tr.is_null() || tp == pos() ) {
            continue;
        }
        if( has_active_bionic( bio_ground_sonar ) && !knows_trap( tp ) &&
            ( tr.loadid == tr_beartrap_buried ||
              tr.loadid == tr_landmine_buried || tr.loadid == tr_sinkhole ) ) {
            const std::string direction = direction_name( direction_from( pos(), tp ) );
            add_msg_if_player( m_warning, _( "Your ground sonar detected a %1$s to the %2$s!" ),
                               tr.name(), direction );
            add_known_trap( tp, tr );
        }
        if( !sees( tp ) ) {
            continue;
        }
        if( tr.is_always_invisible() || tr.can_see( tp, *this ) ) {
            // Already seen, or can never be seen
            continue;
        }
        // Chance to detect traps we haven't yet seen.
        if( tr.detect_trap( tp, *this ) ) {
            if( tr.get_visibility() > 0 ) {
                // Only bug player about traps that aren't trivial to spot.
                const std::string direction = direction_name(
                                                  direction_from( pos(), tp ) );
                add_msg_if_player( _( "You've spotted a %1$s to the %2$s!" ),
                                   tr.name(), direction );
            }
            add_known_trap( tp, tr );
        }
    }
}

int player::talk_skill() const
{
    /** @EFFECT_INT slightly increases talking skill */

    /** @EFFECT_PER slightly increases talking skill */

    /** @EFFECT_SPEECH increases talking skill */
    int ret = get_int() + get_per() + get_skill_level( skill_id( "speech" ) ) * 3;
    return ret;
}

int player::intimidation() const
{
    /** @EFFECT_STR increases intimidation factor */
    int ret = get_str() * 2;
    if( weapon.is_gun() ) {
        ret += 10;
    }
    if( weapon.damage_melee( DT_BASH ) >= 12 ||
        weapon.damage_melee( DT_CUT ) >= 12 ||
        weapon.damage_melee( DT_STAB ) >= 12 ) {
        ret += 5;
    }

    if( get_stim() > 20 ) {
        ret += 2;
    }
    if( has_effect( effect_drunk ) ) {
        ret -= 4;
    }

    return ret;
}

bool player::is_dead_state() const
{
    return hp_cur[hp_head] <= 0 || hp_cur[hp_torso] <= 0;
}

void player::on_dodge( Creature *source, float difficulty )
{
    static const matec_id tec_none( "tec_none" );

    // Each avoided hit consumes an available dodge
    // When no more available we are likely to fail player::dodge_roll
    dodges_left--;

    // dodging throws of our aim unless we are either skilled at dodging or using a small weapon
    if( is_armed() && weapon.is_gun() ) {
        recoil += std::max( weapon.volume() / 250_ml - get_skill_level( skill_dodge ), 0 ) * rng( 0, 100 );
        recoil = std::min( MAX_RECOIL, recoil );
    }

    // Even if we are not to train still call practice to prevent skill rust
    difficulty = std::max( difficulty, 0.0f );
    practice( skill_dodge, difficulty * 2, difficulty );

    martial_arts_data.ma_ondodge_effects( *this );

    // For adjacent attackers check for techniques usable upon successful dodge
    if( source && square_dist( pos(), source->pos() ) == 1 ) {
        matec_id tec = pick_technique( *source, used_weapon(), false, true, false );

        if( tec != tec_none && !is_dead_state() ) {
            if( get_stamina() < get_stamina_max() / 3 ) {
                add_msg( m_bad, _( "You try to counterattack but you are too exhausted!" ) );
            } else {
                melee_attack( *source, false, tec );
            }
        }
    }
}

void player::on_hit( Creature *source, body_part bp_hit,
                     float /*difficulty*/, dealt_projectile_attack const *const proj )
{
    check_dead_state();
    if( source == nullptr || proj != nullptr ) {
        return;
    }

    bool u_see = g->u.sees( *this );
    if( has_active_bionic( bionic_id( "bio_ods" ) ) && get_power_level() > 5_kJ ) {
        if( is_player() ) {
            add_msg( m_good, _( "Your offensive defense system shocks %s in mid-attack!" ),
                     source->disp_name() );
        } else if( u_see ) {
            add_msg( _( "%1$s's offensive defense system shocks %2$s in mid-attack!" ),
                     disp_name(),
                     source->disp_name() );
        }
        int shock = rng( 1, 4 );
        mod_power_level( units::from_kilojoule( -shock ) );
        damage_instance ods_shock_damage;
        ods_shock_damage.add_damage( DT_ELECTRIC, shock * 5 );
        // Should hit body part used for attack
        source->deal_damage( this, bp_torso, ods_shock_damage );
    }
    if( !wearing_something_on( bp_hit ) &&
        ( has_trait( trait_SPINES ) || has_trait( trait_QUILLS ) ) ) {
        int spine = rng( 1, has_trait( trait_QUILLS ) ? 20 : 8 );
        if( !is_player() ) {
            if( u_see ) {
                add_msg( _( "%1$s's %2$s puncture %3$s in mid-attack!" ), name,
                         ( has_trait( trait_QUILLS ) ? _( "quills" ) : _( "spines" ) ),
                         source->disp_name() );
            }
        } else {
            add_msg( m_good, _( "Your %1$s puncture %2$s in mid-attack!" ),
                     ( has_trait( trait_QUILLS ) ? _( "quills" ) : _( "spines" ) ),
                     source->disp_name() );
        }
        damage_instance spine_damage;
        spine_damage.add_damage( DT_STAB, spine );
        source->deal_damage( this, bp_torso, spine_damage );
    }
    if( ( !( wearing_something_on( bp_hit ) ) ) && ( has_trait( trait_THORNS ) ) &&
        ( !( source->has_weapon() ) ) ) {
        if( !is_player() ) {
            if( u_see ) {
                add_msg( _( "%1$s's %2$s scrape %3$s in mid-attack!" ), name,
                         _( "thorns" ), source->disp_name() );
            }
        } else {
            add_msg( m_good, _( "Your thorns scrape %s in mid-attack!" ), source->disp_name() );
        }
        int thorn = rng( 1, 4 );
        damage_instance thorn_damage;
        thorn_damage.add_damage( DT_CUT, thorn );
        // In general, critters don't have separate limbs
        // so safer to target the torso
        source->deal_damage( this, bp_torso, thorn_damage );
    }
    if( ( !( wearing_something_on( bp_hit ) ) ) && ( has_trait( trait_CF_HAIR ) ) ) {
        if( !is_player() ) {
            if( u_see ) {
                add_msg( _( "%1$s gets a load of %2$s's %3$s stuck in!" ), source->disp_name(),
                         name, ( _( "hair" ) ) );
            }
        } else {
            add_msg( m_good, _( "Your hairs detach into %s!" ), source->disp_name() );
        }
        source->add_effect( effect_stunned, 2_turns );
        if( one_in( 3 ) ) { // In the eyes!
            source->add_effect( effect_blind, 2_turns );
        }
    }
    if( worn_with_flag( "REQUIRES_BALANCE" ) && !has_effect( effect_downed ) ) {
        int rolls = 4;
        if( worn_with_flag( "ROLLER_ONE" ) ) {
            rolls += 2;
        }
        if( has_trait( trait_PROF_SKATER ) ) {
            rolls--;
        }
        if( has_trait( trait_DEFT ) ) {
            rolls--;
        }
        if( has_trait( trait_CLUMSY ) ) {
            rolls++;
        }

        if( stability_roll() < dice( rolls, 10 ) ) {
            if( !is_player() ) {
                if( u_see ) {
                    add_msg( _( "%1$s loses their balance while being hit!" ), name );
                }
            } else {
                add_msg( m_bad, _( "You lose your balance while being hit!" ) );
            }
            // This kind of downing is not subject to immunity.
            add_effect( effect_downed, 2_turns, num_bp, false, 0, true );
        }
    }
    Character::on_hit( source, bp_hit, 0.0f, proj );
}

int player::get_lift_assist() const
{
    int result = 0;
    const std::vector<npc *> helpers = get_crafting_helpers();
    for( const npc *np : helpers ) {
        result += np->get_str();
    }
    return result;
}

int player::get_num_crafting_helpers( int max ) const
{
    std::vector<npc *> helpers = get_crafting_helpers();
    return std::min( max, static_cast<int>( helpers.size() ) );
}

bool player::immune_to( body_part bp, damage_unit dam ) const
{
    if( has_trait( trait_DEBUG_NODMG ) || is_immune_damage( dam.type ) ) {
        return true;
    }

    passive_absorb_hit( bp, dam );

    for( const item &cloth : worn ) {
        if( cloth.get_coverage() == 100 && cloth.covers( bp ) ) {
            cloth.mitigate_damage( dam );
        }
    }

    return dam.amount <= 0;
}

void player::mod_pain( int npain )
{
    if( npain > 0 ) {
        if( has_trait( trait_NOPAIN ) || has_effect( effect_narcosis ) ) {
            return;
        }
        // always increase pain gained by one from these bad mutations
        if( has_trait( trait_MOREPAIN ) ) {
            npain += std::max( 1, roll_remainder( npain * 0.25 ) );
        } else if( has_trait( trait_MOREPAIN2 ) ) {
            npain += std::max( 1, roll_remainder( npain * 0.5 ) );
        } else if( has_trait( trait_MOREPAIN3 ) ) {
            npain += std::max( 1, roll_remainder( npain * 1.0 ) );
        }

        if( npain > 1 ) {
            // if it's 1 it'll just become 0, which is bad
            if( has_trait( trait_PAINRESIST_TROGLO ) ) {
                npain = roll_remainder( npain * 0.5 );
            } else if( has_trait( trait_PAINRESIST ) ) {
                npain = roll_remainder( npain * 0.67 );
            }
        }
    }
    Creature::mod_pain( npain );
}

void player::set_pain( int npain )
{
    const int prev_pain = get_perceived_pain();
    Creature::set_pain( npain );
    const int cur_pain = get_perceived_pain();

    if( cur_pain != prev_pain ) {
        react_to_felt_pain( cur_pain - prev_pain );
        on_stat_change( "perceived_pain", cur_pain );
    }
}

int player::get_perceived_pain() const
{
    if( get_effect_int( effect_adrenaline ) > 1 ) {
        return 0;
    }

    return std::max( get_pain() - get_painkiller(), 0 );
}

float player::fall_damage_mod() const
{
    if( has_effect_with_flag( "EFFECT_FEATHER_FALL" ) ) {
        return 0.0f;
    }
    float ret = 1.0f;

    // Ability to land properly is 2x as important as dexterity itself
    /** @EFFECT_DEX decreases damage from falling */

    /** @EFFECT_DODGE decreases damage from falling */
    float dex_dodge = dex_cur / 2.0 + get_skill_level( skill_dodge );
    // Penalize for wearing heavy stuff
    const float average_leg_encumb = ( encumb( bp_leg_l ) + encumb( bp_leg_r ) ) / 2.0;
    dex_dodge -= ( average_leg_encumb + encumb( bp_torso ) ) / 10;
    // But prevent it from increasing damage
    dex_dodge = std::max( 0.0f, dex_dodge );
    // 100% damage at 0, 75% at 10, 50% at 20 and so on
    ret *= ( 100.0f - ( dex_dodge * 4.0f ) ) / 100.0f;

    if( has_trait( trait_PARKOUR ) ) {
        ret *= 2.0f / 3.0f;
    }

    // TODO: Bonus for Judo, mutations. Penalty for heavy weight (including mutations)
    return std::max( 0.0f, ret );
}

// force is maximum damage to hp before scaling
int player::impact( const int force, const tripoint &p )
{
    // Falls over ~30m are fatal more often than not
    // But that would be quite a lot considering 21 z-levels in game
    // so let's assume 1 z-level is comparable to 30 force

    if( force <= 0 ) {
        return force;
    }

    // Damage modifier (post armor)
    float mod = 1.0f;
    int effective_force = force;
    int cut = 0;
    // Percentage armor penetration - armor won't help much here
    // TODO: Make cushioned items like bike helmets help more
    float armor_eff = 1.0f;
    // Shock Absorber CBM heavily reduces damage
    const bool shock_absorbers = has_active_bionic( bionic_id( "bio_shock_absorber" ) );

    // Being slammed against things rather than landing means we can't
    // control the impact as well
    const bool slam = p != pos();
    std::string target_name = "a swarm of bugs";
    Creature *critter = g->critter_at( p );
    if( critter != this && critter != nullptr ) {
        target_name = critter->disp_name();
        // Slamming into creatures and NPCs
        // TODO: Handle spikes/horns and hard materials
        armor_eff = 0.5f; // 2x as much as with the ground
        // TODO: Modify based on something?
        mod = 1.0f;
        effective_force = force;
    } else if( const optional_vpart_position vp = g->m.veh_at( p ) ) {
        // Slamming into vehicles
        // TODO: Integrate it with vehicle collision function somehow
        target_name = vp->vehicle().disp_name();
        if( vp.part_with_feature( "SHARP", true ) ) {
            // Now we're actually getting impaled
            cut = force; // Lots of fun
        }

        mod = slam ? 1.0f : fall_damage_mod();
        armor_eff = 0.25f; // Not much
        if( !slam && vp->part_with_feature( "ROOF", true ) ) {
            // Roof offers better landing than frame or pavement
            // TODO: Make this not happen with heavy duty/plated roof
            effective_force /= 2;
        }
    } else {
        // Slamming into terrain/furniture
        target_name = g->m.disp_name( p );
        int hard_ground = g->m.has_flag( TFLAG_DIGGABLE, p ) ? 0 : 3;
        armor_eff = 0.25f; // Not much
        // Get cut by stuff
        // This isn't impalement on metal wreckage, more like flying through a closed window
        cut = g->m.has_flag( TFLAG_SHARP, p ) ? 5 : 0;
        effective_force = force + hard_ground;
        mod = slam ? 1.0f : fall_damage_mod();
        if( g->m.has_furn( p ) ) {
            // TODO: Make furniture matter
        } else if( g->m.has_flag( TFLAG_SWIMMABLE, p ) ) {
            // TODO: Some formula of swimming
            effective_force /= 4;
        }
    }

    // Rescale for huge force
    // At >30 force, proper landing is impossible and armor helps way less
    if( effective_force > 30 ) {
        // Armor simply helps way less
        armor_eff *= 30.0f / effective_force;
        if( mod < 1.0f ) {
            // Everything past 30 damage gets a worse modifier
            const float scaled_mod = std::pow( mod, 30.0f / effective_force );
            const float scaled_damage = ( 30.0f * mod ) + scaled_mod * ( effective_force - 30.0f );
            mod = scaled_damage / effective_force;
        }
    }

    if( !slam && mod < 1.0f && mod * force < 5 ) {
        // Perfect landing, no damage (regardless of armor)
        add_msg_if_player( m_warning, _( "You land on %s." ), target_name );
        return 0;
    }

    // Shock absorbers kick in only when they need to, so if our other protections fail, fall back on them
    if( shock_absorbers ) {
        effective_force -= 15; // Provide a flat reduction to force
        if( mod > 0.25f ) {
            mod = 0.25f; // And provide a 75% reduction against that force if we don't have it already
        }
        if( effective_force < 0 ) {
            effective_force = 0;
        }
    }

    int total_dealt = 0;
    if( mod * effective_force >= 5 ) {
        for( int i = 0; i < num_hp_parts; i++ ) {
            const body_part bp = hp_to_bp( static_cast<hp_part>( i ) );
            const int bash = effective_force * rng( 60, 100 ) / 100;
            damage_instance di;
            di.add_damage( DT_BASH, bash, 0, armor_eff, mod );
            // No good way to land on sharp stuff, so here modifier == 1.0f
            di.add_damage( DT_CUT, cut, 0, armor_eff, 1.0f );
            total_dealt += deal_damage( nullptr, bp, di ).total_damage();
        }
    }

    if( total_dealt > 0 && is_player() ) {
        // "You slam against the dirt" is fine
        add_msg( m_bad, _( "You are slammed against %1$s for %2$d damage." ),
                 target_name, total_dealt );
    } else if( is_player() && shock_absorbers ) {
        add_msg( m_bad, _( "You are slammed against %s!" ),
                 target_name, total_dealt );
        add_msg( m_good, _( "…but your shock absorbers negate the damage!" ) );
    } else if( slam ) {
        // Only print this line if it is a slam and not a landing
        // Non-players should only get this one: player doesn't know how much damage was dealt
        // and landing messages for each slammed creature would be too much
        add_msg_player_or_npc( m_bad,
                               _( "You are slammed against %s." ),
                               _( "<npcname> is slammed against %s." ),
                               target_name );
    } else {
        // No landing message for NPCs
        add_msg_if_player( m_warning, _( "You land on %s." ), target_name );
    }

    if( x_in_y( mod, 1.0f ) ) {
        add_effect( effect_downed, rng( 1_turns, 1_turns + mod * 3_turns ) );
    }

    return total_dealt;
}

void player::knock_back_to( const tripoint &to )
{
    if( to == pos() ) {
        return;
    }

    // First, see if we hit a monster
    if( monster *const critter = g->critter_at<monster>( to ) ) {
        deal_damage( critter, bp_torso, damage_instance( DT_BASH, critter->type->size ) );
        add_effect( effect_stunned, 1_turns );
        /** @EFFECT_STR_MAX allows knocked back player to knock back, damage, stun some monsters */
        if( ( str_max - 6 ) / 4 > critter->type->size ) {
            critter->knock_back_from( pos() ); // Chain reaction!
            critter->apply_damage( this, bodypart_id( "torso" ), ( str_max - 6 ) / 4 );
            critter->add_effect( effect_stunned, 1_turns );
        } else if( ( str_max - 6 ) / 4 == critter->type->size ) {
            critter->apply_damage( this, bodypart_id( "torso" ), ( str_max - 6 ) / 4 );
            critter->add_effect( effect_stunned, 1_turns );
        }
        critter->check_dead_state();

        add_msg_player_or_npc( _( "You bounce off a %s!" ), _( "<npcname> bounces off a %s!" ),
                               critter->name() );
        return;
    }

    if( npc *const np = g->critter_at<npc>( to ) ) {
        deal_damage( np, bp_torso, damage_instance( DT_BASH, np->get_size() ) );
        add_effect( effect_stunned, 1_turns );
        np->deal_damage( this, bp_torso, damage_instance( DT_BASH, 3 ) );
        add_msg_player_or_npc( _( "You bounce off %s!" ), _( "<npcname> bounces off %s!" ),
                               np->name );
        np->check_dead_state();
        return;
    }

    // If we're still in the function at this point, we're actually moving a tile!
    if( g->m.has_flag( "LIQUID", to ) && g->m.has_flag( TFLAG_DEEP_WATER, to ) ) {
        if( !is_npc() ) {
            avatar_action::swim( g->m, g->u, to );
        }
        // TODO: NPCs can't swim!
    } else if( g->m.impassable( to ) ) { // Wait, it's a wall

        // It's some kind of wall.
        // TODO: who knocked us back? Maybe that creature should be the source of the damage?
        apply_damage( nullptr, bodypart_id( "torso" ), 3 );
        add_effect( effect_stunned, 2_turns );
        add_msg_player_or_npc( _( "You bounce off a %s!" ), _( "<npcname> bounces off a %s!" ),
                               g->m.obstacle_name( to ) );

    } else { // It's no wall
        setpos( to );
    }
}

int player::hp_percentage() const
{
    int total_cur = 0;
    int total_max = 0;
    // Head and torso HP are weighted 3x and 2x, respectively
    total_cur = hp_cur[hp_head] * 3 + hp_cur[hp_torso] * 2;
    total_max = hp_max[hp_head] * 3 + hp_max[hp_torso] * 2;
    for( int i = hp_arm_l; i < num_hp_parts; i++ ) {
        total_cur += hp_cur[i];
        total_max += hp_max[i];
    }

    return ( 100 * total_cur ) / total_max;
}

void player::siphon( vehicle &veh, const itype_id &desired_liquid )
{
    auto qty = veh.fuel_left( desired_liquid );
    if( qty <= 0 ) {
        add_msg( m_bad, _( "There is not enough %s left to siphon it." ),
                 item::nname( desired_liquid ) );
        return;
    }

    item liquid( desired_liquid, calendar::turn, qty );
    if( liquid.is_food() ) {
        liquid.set_item_specific_energy( veh.fuel_specific_energy( desired_liquid ) );
    }
    if( liquid_handler::handle_liquid( liquid, nullptr, 1, nullptr, &veh ) ) {
        veh.drain( desired_liquid, qty - liquid.charges );
    }
}

void player::add_pain_msg( int val, body_part bp ) const
{
    if( has_trait( trait_NOPAIN ) ) {
        return;
    }
    if( bp == num_bp ) {
        if( val > 20 ) {
            add_msg_if_player( _( "Your body is wracked with excruciating pain!" ) );
        } else if( val > 10 ) {
            add_msg_if_player( _( "Your body is wracked with terrible pain!" ) );
        } else if( val > 5 ) {
            add_msg_if_player( _( "Your body is wracked with pain!" ) );
        } else if( val > 1 ) {
            add_msg_if_player( _( "Your body pains you!" ) );
        } else {
            add_msg_if_player( _( "Your body aches." ) );
        }
    } else {
        if( val > 20 ) {
            add_msg_if_player( _( "Your %s is wracked with excruciating pain!" ),
                               body_part_name_accusative( bp ) );
        } else if( val > 10 ) {
            add_msg_if_player( _( "Your %s is wracked with terrible pain!" ),
                               body_part_name_accusative( bp ) );
        } else if( val > 5 ) {
            add_msg_if_player( _( "Your %s is wracked with pain!" ),
                               body_part_name_accusative( bp ) );
        } else if( val > 1 ) {
            add_msg_if_player( _( "Your %s pains you!" ),
                               body_part_name_accusative( bp ) );
        } else {
            add_msg_if_player( _( "Your %s aches." ),
                               body_part_name_accusative( bp ) );
        }
    }
}

void player::process_one_effect( effect &it, bool is_new )
{
    bool reduced = resists_effect( it );
    double mod = 1;
    body_part bp = it.get_bp();
    int val = 0;

    // Still hardcoded stuff, do this first since some modify their other traits
    hardcoded_effects( it );

    const auto get_effect = [&it, is_new]( const std::string & arg, bool reduced ) {
        if( is_new ) {
            return it.get_amount( arg, reduced );
        }
        return it.get_mod( arg, reduced );
    };

    // Handle miss messages
    auto msgs = it.get_miss_msgs();
    if( !msgs.empty() ) {
        for( const auto &i : msgs ) {
            add_miss_reason( _( i.first ), static_cast<unsigned>( i.second ) );
        }
    }

    // Handle health mod
    val = get_effect( "H_MOD", reduced );
    if( val != 0 ) {
        mod = 1;
        if( is_new || it.activated( calendar::turn, "H_MOD", val, reduced, mod ) ) {
            int bounded = bound_mod_to_vals(
                              get_healthy_mod(), val, it.get_max_val( "H_MOD", reduced ),
                              it.get_min_val( "H_MOD", reduced ) );
            // This already applies bounds, so we pass them through.
            mod_healthy_mod( bounded, get_healthy_mod() + bounded );
        }
    }

    // Handle health
    val = get_effect( "HEALTH", reduced );
    if( val != 0 ) {
        mod = 1;
        if( is_new || it.activated( calendar::turn, "HEALTH", val, reduced, mod ) ) {
            mod_healthy( bound_mod_to_vals( get_healthy(), val,
                                            it.get_max_val( "HEALTH", reduced ), it.get_min_val( "HEALTH", reduced ) ) );
        }
    }

    // Handle stim
    val = get_effect( "STIM", reduced );
    if( val != 0 ) {
        mod = 1;
        if( is_new || it.activated( calendar::turn, "STIM", val, reduced, mod ) ) {
            mod_stim( bound_mod_to_vals( get_stim(), val, it.get_max_val( "STIM", reduced ),
                                         it.get_min_val( "STIM", reduced ) ) );
        }
    }

    // Handle hunger
    val = get_effect( "HUNGER", reduced );
    if( val != 0 ) {
        mod = 1;
        if( is_new || it.activated( calendar::turn, "HUNGER", val, reduced, mod ) ) {
            mod_hunger( bound_mod_to_vals( get_hunger(), val, it.get_max_val( "HUNGER", reduced ),
                                           it.get_min_val( "HUNGER", reduced ) ) );
        }
    }

    // Handle thirst
    val = get_effect( "THIRST", reduced );
    if( val != 0 ) {
        mod = 1;
        if( is_new || it.activated( calendar::turn, "THIRST", val, reduced, mod ) ) {
            mod_thirst( bound_mod_to_vals( get_thirst(), val, it.get_max_val( "THIRST", reduced ),
                                           it.get_min_val( "THIRST", reduced ) ) );
        }
    }

    // Handle fatigue
    val = get_effect( "FATIGUE", reduced );
    // Prevent ongoing fatigue effects while asleep.
    // These are meant to change how fast you get tired, not how long you sleep.
    if( val != 0 && !in_sleep_state() ) {
        mod = 1;
        if( is_new || it.activated( calendar::turn, "FATIGUE", val, reduced, mod ) ) {
            mod_fatigue( bound_mod_to_vals( get_fatigue(), val, it.get_max_val( "FATIGUE", reduced ),
                                            it.get_min_val( "FATIGUE", reduced ) ) );
        }
    }

    // Handle Radiation
    val = get_effect( "RAD", reduced );
    if( val != 0 ) {
        mod = 1;
        if( is_new || it.activated( calendar::turn, "RAD", val, reduced, mod ) ) {
            mod_rad( bound_mod_to_vals( get_rad(), val, it.get_max_val( "RAD", reduced ), 0 ) );
            // Radiation can't go negative
            if( get_rad() < 0 ) {
                set_rad( 0 );
            }
        }
    }

    // Handle Pain
    val = get_effect( "PAIN", reduced );
    if( val != 0 ) {
        mod = 1;
        if( it.get_sizing( "PAIN" ) ) {
            if( has_trait( trait_FAT ) ) {
                mod *= 1.5;
            }
            if( has_trait( trait_LARGE ) || has_trait( trait_LARGE_OK ) ) {
                mod *= 2;
            }
            if( has_trait( trait_HUGE ) || has_trait( trait_HUGE_OK ) ) {
                mod *= 3;
            }
        }
        if( is_new || it.activated( calendar::turn, "PAIN", val, reduced, mod ) ) {
            int pain_inc = bound_mod_to_vals( get_pain(), val, it.get_max_val( "PAIN", reduced ), 0 );
            mod_pain( pain_inc );
            if( pain_inc > 0 ) {
                add_pain_msg( val, bp );
            }
        }
    }

    // Handle Damage
    val = get_effect( "HURT", reduced );
    if( val != 0 ) {
        mod = 1;
        if( it.get_sizing( "HURT" ) ) {
            if( has_trait( trait_FAT ) ) {
                mod *= 1.5;
            }
            if( has_trait( trait_LARGE ) || has_trait( trait_LARGE_OK ) ) {
                mod *= 2;
            }
            if( has_trait( trait_HUGE ) || has_trait( trait_HUGE_OK ) ) {
                mod *= 3;
            }
        }
        if( is_new || it.activated( calendar::turn, "HURT", val, reduced, mod ) ) {
            if( bp == num_bp ) {
                if( val > 5 ) {
                    add_msg_if_player( _( "Your %s HURTS!" ), body_part_name_accusative( bp_torso ) );
                } else {
                    add_msg_if_player( _( "Your %s hurts!" ), body_part_name_accusative( bp_torso ) );
                }
                apply_damage( nullptr, bodypart_id( "torso" ), val, true );
            } else {
                if( val > 5 ) {
                    add_msg_if_player( _( "Your %s HURTS!" ), body_part_name_accusative( bp ) );
                } else {
                    add_msg_if_player( _( "Your %s hurts!" ), body_part_name_accusative( bp ) );
                }
                apply_damage( nullptr, convert_bp( bp ).id(), val, true );
            }
        }
    }

    // Handle Sleep
    val = get_effect( "SLEEP", reduced );
    if( val != 0 ) {
        mod = 1;
        if( ( is_new || it.activated( calendar::turn, "SLEEP", val, reduced, mod ) ) &&
            !has_effect( efftype_id( "sleep" ) ) ) {
            add_msg_if_player( _( "You pass out!" ) );
            fall_asleep( time_duration::from_turns( val ) );
        }
    }

    // Handle painkillers
    val = get_effect( "PKILL", reduced );
    if( val != 0 ) {
        mod = it.get_addict_mod( "PKILL", addiction_level( add_type::PKILLER ) );
        if( is_new || it.activated( calendar::turn, "PKILL", val, reduced, mod ) ) {
            mod_painkiller( bound_mod_to_vals( get_painkiller(), val, it.get_max_val( "PKILL", reduced ), 0 ) );
        }
    }

    // Handle coughing
    mod = 1;
    val = 0;
    if( it.activated( calendar::turn, "COUGH", val, reduced, mod ) ) {
        cough( it.get_harmful_cough() );
    }

    // Handle vomiting
    mod = vomit_mod();
    val = 0;
    if( it.activated( calendar::turn, "VOMIT", val, reduced, mod ) ) {
        vomit();
    }

    // Handle stamina
    val = get_effect( "STAMINA", reduced );
    if( val != 0 ) {
        mod = 1;
        if( is_new || it.activated( calendar::turn, "STAMINA", val, reduced, mod ) ) {
            mod_stamina( bound_mod_to_vals( get_stamina(), val,
                                            it.get_max_val( "STAMINA", reduced ),
                                            it.get_min_val( "STAMINA", reduced ) ) );
        }
    }

    // Speed and stats are handled in recalc_speed_bonus and reset_stats respectively
}

void player::process_effects()
{
    //Special Removals
    if( has_effect( effect_darkness ) && g->is_in_sunlight( pos() ) ) {
        remove_effect( effect_darkness );
    }
    if( has_trait( trait_M_IMMUNE ) && has_effect( effect_fungus ) ) {
        vomit();
        remove_effect( effect_fungus );
        add_msg_if_player( m_bad,  _( "We have mistakenly colonized a local guide!  Purging now." ) );
    }
    if( has_trait( trait_PARAIMMUNE ) && ( has_effect( effect_dermatik ) ||
                                           has_effect( effect_tapeworm ) ||
                                           has_effect( effect_bloodworms ) || has_effect( effect_brainworms ) ||
                                           has_effect( effect_paincysts ) ) ) {
        remove_effect( effect_dermatik );
        remove_effect( effect_tapeworm );
        remove_effect( effect_bloodworms );
        remove_effect( effect_brainworms );
        remove_effect( effect_paincysts );
        add_msg_if_player( m_good, _( "Something writhes and inside of you as it dies." ) );
    }
    if( has_trait( trait_ACIDBLOOD ) && ( has_effect( effect_dermatik ) ||
                                          has_effect( effect_bloodworms ) ||
                                          has_effect( effect_brainworms ) ) ) {
        remove_effect( effect_dermatik );
        remove_effect( effect_bloodworms );
        remove_effect( effect_brainworms );
    }
    if( has_trait( trait_EATHEALTH ) && has_effect( effect_tapeworm ) ) {
        remove_effect( effect_tapeworm );
        add_msg_if_player( m_good, _( "Your bowels gurgle as something inside them dies." ) );
    }
    if( has_trait( trait_INFIMMUNE ) && ( has_effect( effect_bite ) || has_effect( effect_infected ) ||
                                          has_effect( effect_recover ) ) ) {
        remove_effect( effect_bite );
        remove_effect( effect_infected );
        remove_effect( effect_recover );
    }

    //Human only effects
    for( auto &elem : *effects ) {
        for( auto &_effect_it : elem.second ) {
            process_one_effect( _effect_it.second, false );
        }
    }

    Creature::process_effects();
}

double player::vomit_mod()
{
    double mod = 1;
    if( has_effect( effect_weed_high ) ) {
        mod *= .1;
    }
    if( has_trait( trait_STRONGSTOMACH ) ) {
        mod *= .5;
    }
    if( has_trait( trait_WEAKSTOMACH ) ) {
        mod *= 2;
    }
    if( has_trait( trait_NAUSEA ) ) {
        mod *= 3;
    }
    if( has_trait( trait_VOMITOUS ) ) {
        mod *= 3;
    }
    // If you're already nauseous, any food in your stomach greatly
    // increases chance of vomiting. Liquids don't provoke vomiting, though.
    if( stomach.contains() != 0_ml && has_effect( effect_nausea ) ) {
        mod *= 5 * get_effect_int( effect_nausea );
    }
    return mod;
}

void player::update_body_wetness( const w_point &weather )
{
    // Average number of turns to go from completely soaked to fully dry
    // assuming average temperature and humidity
    constexpr time_duration average_drying = 2_hours;

    // A modifier on drying time
    double delay = 1.0;
    // Weather slows down drying
    delay += ( ( weather.humidity - 66 ) - ( weather.temperature - 65 ) ) / 100;
    delay = std::max( 0.1, delay );
    // Fur/slime retains moisture
    if( has_trait( trait_LIGHTFUR ) || has_trait( trait_FUR ) || has_trait( trait_FELINE_FUR ) ||
        has_trait( trait_LUPINE_FUR ) || has_trait( trait_CHITIN_FUR ) || has_trait( trait_CHITIN_FUR2 ) ||
        has_trait( trait_CHITIN_FUR3 ) ) {
        delay = delay * 6 / 5;
    }
    if( has_trait( trait_URSINE_FUR ) || has_trait( trait_SLIMY ) ) {
        delay *= 1.5;
    }

    if( !x_in_y( 1, to_turns<int>( average_drying * delay / 100.0 ) ) ) {
        // No drying this turn
        return;
    }

    // Now per-body-part stuff
    // To make drying uniform, make just one roll and reuse it
    const int drying_roll = rng( 1, 80 );

    for( const body_part bp : all_body_parts ) {
        if( body_wetness[bp] == 0 ) {
            continue;
        }
        // This is to normalize drying times
        int drying_chance = drench_capacity[bp];
        // Body temperature affects duration of wetness
        // Note: Using temp_conv rather than temp_cur, to better approximate environment
        if( temp_conv[bp] >= BODYTEMP_SCORCHING ) {
            drying_chance *= 2;
        } else if( temp_conv[bp] >= BODYTEMP_VERY_HOT ) {
            drying_chance = drying_chance * 3 / 2;
        } else if( temp_conv[bp] >= BODYTEMP_HOT ) {
            drying_chance = drying_chance * 4 / 3;
        } else if( temp_conv[bp] > BODYTEMP_COLD ) {
            // Comfortable, doesn't need any changes
        } else {
            // Evaporation doesn't change that much at lower temp
            drying_chance = drying_chance * 3 / 4;
        }

        if( drying_chance < 1 ) {
            drying_chance = 1;
        }

        // TODO: Make evaporation reduce body heat
        if( drying_chance >= drying_roll ) {
            body_wetness[bp] -= 1;
            if( body_wetness[bp] < 0 ) {
                body_wetness[bp] = 0;
            }
        }
    }
    // TODO: Make clothing slow down drying
}

void player::on_worn_item_transform( const item &old_it, const item &new_it )
{
    morale->on_worn_item_transform( old_it, new_it );
}

void player::process_items()
{
    if( weapon.needs_processing() && weapon.process( this, pos(), false ) ) {
        weapon = item();
    }

    std::vector<item *> inv_active = inv.active_items();
    for( item *tmp_it : inv_active ) {
        if( tmp_it->process( this, pos(), false ) ) {
            inv.remove_item( tmp_it );
        }
    }

    // worn items
    remove_worn_items_with( [this]( item & itm ) {
        return itm.needs_processing() && itm.process( this, pos(), false );
    } );

    // Active item processing done, now we're recharging.
    item *cloak = nullptr;
    item *power_armor = nullptr;
    std::vector<item *> active_worn_items;
    bool weapon_active = weapon.has_flag( "USE_UPS" ) &&
                         weapon.charges < weapon.type->maximum_charges();
    std::vector<size_t> active_held_items;
    int ch_UPS = 0;
    for( size_t index = 0; index < inv.size(); index++ ) {
        item &it = inv.find_item( index );
        itype_id identifier = it.type->get_id();
        if( identifier == "UPS_off" ) {
            ch_UPS += it.ammo_remaining();
        } else if( identifier == "adv_UPS_off" ) {
            ch_UPS += it.ammo_remaining() / 0.6;
        }
        if( it.has_flag( "USE_UPS" ) && it.charges < it.type->maximum_charges() ) {
            active_held_items.push_back( index );
        }
    }
    bool update_required = get_check_encumbrance();
    for( item &w : worn ) {
        if( w.has_flag( "USE_UPS" ) &&
            w.charges < w.type->maximum_charges() ) {
            active_worn_items.push_back( &w );
        }
        if( w.active ) {
            if( cloak == nullptr && w.has_flag( "ACTIVE_CLOAKING" ) ) {
                cloak = &w;
            }
            // Only the main power armor item can be active, the other ones (hauling frame, helmet) aren't.
            if( power_armor == nullptr && w.is_power_armor() ) {
                power_armor = &w;
            }
        }
        // Necessary for UPS in Aftershock - check worn items for charge
        const itype_id &identifier = w.typeId();
        if( identifier == "UPS_off" ) {
            ch_UPS += w.ammo_remaining();
        } else if( identifier == "adv_UPS_off" ) {
            ch_UPS += w.ammo_remaining() / 0.6;
        }
        if( !update_required && w.encumbrance_update_ ) {
            update_required = true;
        }
        w.encumbrance_update_ = false;
    }
    if( update_required ) {
        reset_encumbrance();
    }
    if( has_active_bionic( bionic_id( "bio_ups" ) ) ) {
        ch_UPS += units::to_kilojoule( get_power_level() );
    }
    int ch_UPS_used = 0;
    if( cloak != nullptr ) {
        if( ch_UPS >= 20 ) {
            use_charges( "UPS", 20 );
            ch_UPS -= 20;
            if( ch_UPS < 200 && one_in( 3 ) ) {
                add_msg_if_player( m_warning, _( "Your cloaking flickers for a moment!" ) );
            }
        } else if( ch_UPS > 0 ) {
            use_charges( "UPS", ch_UPS );
            return;
        } else {
            add_msg_if_player( m_bad,
                               _( "Your cloaking flickers and becomes opaque." ) );
            // Bypass the "you deactivate the ..." message
            cloak->active = false;
            return;
        }
    }

    // For powered armor, an armor-powering bionic should always be preferred over UPS usage.
    if( power_armor != nullptr ) {
        const int power_cost = 4;
        bool bio_powered = can_interface_armor() && has_power();
        // Bionic power costs are handled elsewhere.
        if( !bio_powered ) {
            if( ch_UPS >= power_cost ) {
                use_charges( "UPS", power_cost );
                ch_UPS -= power_cost;
            } else {
                // Deactivate armor here, bypassing the usual deactivation message.
                add_msg_if_player( m_warning, _( "Your power armor disengages." ) );
                power_armor->active = false;
            }
        }
    }

    // Load all items that use the UPS to their minimal functional charge,
    // The tool is not really useful if its charges are below charges_to_use
    for( size_t index : active_held_items ) {
        if( ch_UPS_used >= ch_UPS ) {
            break;
        }
        item &it = inv.find_item( index );
        ch_UPS_used++;
        it.charges++;
    }
    if( weapon_active && ch_UPS_used < ch_UPS ) {
        ch_UPS_used++;
        weapon.charges++;
    }
    for( item *worn_item : active_worn_items ) {
        if( ch_UPS_used >= ch_UPS ) {
            break;
        }
        ch_UPS_used++;
        worn_item->charges++;
    }
    if( ch_UPS_used > 0 ) {
        use_charges( "UPS", ch_UPS_used );
    }
}

item player::reduce_charges( int position, int quantity )
{
    item &it = i_at( position );
    if( it.is_null() ) {
        debugmsg( "invalid item position %d for reduce_charges", position );
        return item();
    }
    if( it.charges <= quantity ) {
        return i_rem( position );
    }
    it.mod_charges( -quantity );
    item tmp( it );
    tmp.charges = quantity;
    return tmp;
}

item player::reduce_charges( item *it, int quantity )
{
    if( !has_item( *it ) ) {
        debugmsg( "invalid item (name %s) for reduce_charges", it->tname() );
        return item();
    }
    if( it->charges <= quantity ) {
        return i_rem( it );
    }
    it->mod_charges( -quantity );
    item result( *it );
    result.charges = quantity;
    return result;
}

bool player::can_interface_armor() const
{
    bool okay = std::any_of( my_bionics->begin(), my_bionics->end(),
    []( const bionic & b ) {
        return b.powered && b.info().armor_interface;
    } );
    return okay;
}

bool player::has_mission_item( int mission_id ) const
{
    return mission_id != -1 && has_item_with( has_mission_item_filter{ mission_id } );
}

//Returns the amount of charges that were consumed by the player
int player::drink_from_hands( item &water )
{
    int charges_consumed = 0;
    if( query_yn( _( "Drink %s from your hands?" ),
                  colorize( water.type_name(), water.color_in_inventory() ) ) ) {
        // Create a dose of water no greater than the amount of water remaining.
        item water_temp( water );
        // If player is slaked water might not get consumed.
        consume_item( water_temp );
        charges_consumed = water.charges - water_temp.charges;
        if( charges_consumed > 0 ) {
            moves -= 350;
        }
    }

    return charges_consumed;
}

// TODO: Properly split medications and food instead of hacking around
bool player::consume_med( item &target )
{
    if( !target.is_medication() ) {
        return false;
    }

    const itype_id tool_type = target.get_comestible()->tool;
    const auto req_tool = item::find_type( tool_type );
    bool tool_override = false;
    if( tool_type == "syringe" && has_bionic( bio_syringe ) ) {
        tool_override = true;
    }
    if( req_tool->tool ) {
        if( !( has_amount( tool_type, 1 ) && has_charges( tool_type, req_tool->tool->charges_per_use ) ) &&
            !tool_override ) {
            add_msg_if_player( m_info, _( "You need a %s to consume that!" ), req_tool->nname( 1 ) );
            return false;
        }
        use_charges( tool_type, req_tool->tool->charges_per_use );
    }

    int amount_used = 1;
    if( target.type->has_use() ) {
        amount_used = target.type->invoke( *this, target, pos() );
        if( amount_used <= 0 ) {
            return false;
        }
    }

    // TODO: Get the target it was used on
    // Otherwise injecting someone will give us addictions etc.
    if( target.has_flag( "NO_INGEST" ) ) {
        const auto &comest = *target.get_comestible();
        // Assume that parenteral meds don't spoil, so don't apply rot
        modify_health( comest );
        modify_stimulation( comest );
        modify_fatigue( comest );
        modify_radiation( comest );
        modify_addiction( comest );
        modify_morale( target );
    } else {
        // Take by mouth
        consume_effects( target );
    }

    mod_moves( -250 );
    target.charges -= amount_used;
    return target.charges <= 0;
}

static bool query_consume_ownership( item &target, player &p )
{
    if( !target.is_owned_by( p, true ) ) {
        bool choice = true;
        if( p.get_value( "THIEF_MODE" ) == "THIEF_ASK" ) {
            choice = Pickup::query_thief();
        }
        if( p.get_value( "THIEF_MODE" ) == "THIEF_HONEST" || !choice ) {
            return false;
        }
        std::vector<npc *> witnesses;
        for( npc &elem : g->all_npcs() ) {
            if( rl_dist( elem.pos(), p.pos() ) < MAX_VIEW_DISTANCE && elem.sees( p.pos() ) ) {
                witnesses.push_back( &elem );
            }
        }
        for( npc *elem : witnesses ) {
            elem->say( "<witnessed_thievery>", 7 );
        }
        if( !witnesses.empty() && target.is_owned_by( p, true ) ) {
            if( p.add_faction_warning( target.get_owner() ) ) {
                for( npc *elem : witnesses ) {
                    elem->make_angry();
                }
            }
        }
    }
    return true;
}

bool player::consume_item( item &target )
{
    if( target.is_null() ) {
        add_msg_if_player( m_info, _( "You do not have that item." ) );
        return false;
    }
    if( is_underwater() && !has_trait( trait_WATERSLEEP ) ) {
        add_msg_if_player( m_info, _( "You can't do that while underwater." ) );
        return false;
    }

    item &comest = get_consumable_from( target );

    if( comest.is_null() || target.is_craft() ) {
        add_msg_if_player( m_info, _( "You can't eat your %s." ), target.tname() );
        if( is_npc() ) {
            debugmsg( "%s tried to eat a %s", name, target.tname() );
        }
        return false;
    }
    if( is_player() && !query_consume_ownership( target, *this ) ) {
        return false;
    }
    if( consume_med( comest ) ||
        eat( comest ) || feed_reactor_with( comest ) || feed_furnace_with( comest ) ||
        fuel_bionic_with( comest ) ) {

        if( target.is_container() ) {
            target.on_contents_changed();
        }

        return comest.charges <= 0;
    }

    return false;
}

bool player::consume( item_location loc )
{
    item &target = *loc;
    const bool wielding = is_wielding( target );
    const bool worn = is_worn( target );
    const bool inv_item = !( wielding || worn );

    if( consume_item( target ) ) {

        const bool was_in_container = !can_consume_as_is( target );

        if( was_in_container ) {
            i_rem( &target.contents.front() );
        } else {
            i_rem( &target );
        }

        //Restack and sort so that we don't lie about target's invlet
        if( inv_item ) {
            inv.restack( *this );
        }

        if( was_in_container && wielding ) {
            add_msg_if_player( _( "You are now wielding an empty %s." ), weapon.tname() );
        } else if( was_in_container && worn ) {
            add_msg_if_player( _( "You are now wearing an empty %s." ), target.tname() );
        } else if( was_in_container && !is_npc() ) {
            bool drop_it = false;
            if( get_option<std::string>( "DROP_EMPTY" ) == "no" ) {
                drop_it = false;
            } else if( get_option<std::string>( "DROP_EMPTY" ) == "watertight" ) {
                drop_it = !target.is_watertight_container();
            } else if( get_option<std::string>( "DROP_EMPTY" ) == "all" ) {
                drop_it = true;
            }
            if( drop_it ) {
                add_msg( _( "You drop the empty %s." ), target.tname() );
                put_into_vehicle_or_drop( *this, item_drop_reason::deliberate, { inv.remove_item( &target ) } );
            } else {
                int quantity = inv.const_stack( inv.position_by_item( &target ) ).size();
                char letter = target.invlet ? target.invlet : ' ';
                add_msg( m_info, _( "%c - %d empty %s" ), letter, quantity, target.tname( quantity ) );
            }
        }
    } else if( inv_item ) {
        if( Pickup::handle_spillable_contents( *this, target, g->m ) ) {
            i_rem( &target );
        }
        inv.restack( *this );
        inv.unsort();
    }

    return true;
}

bool player::add_faction_warning( const faction_id &id )
{
    const auto it = warning_record.find( id );
    if( it != warning_record.end() ) {
        it->second.first += 1;
        if( it->second.second - calendar::turn > 5_minutes ) {
            it->second.first -= 1;
        }
        it->second.second = calendar::turn;
        if( it->second.first > 3 ) {
            return true;
        }
    } else {
        warning_record[id] = std::make_pair( 1, calendar::turn );
    }
    faction *fac = g->faction_manager_ptr->get( id );
    if( fac != nullptr && is_player() && fac->id != faction_id( "no_faction" ) ) {
        fac->likes_u -= 1;
        fac->respects_u -= 1;
    }
    return false;
}

int player::current_warnings_fac( const faction_id &id )
{
    const auto it = warning_record.find( id );
    if( it != warning_record.end() ) {
        if( it->second.second - calendar::turn > 5_minutes ) {
            it->second.first = std::max( 0,
                                         it->second.first - 1 );
        }
        return it->second.first;
    }
    return 0;
}

bool player::beyond_final_warning( const faction_id &id )
{
    const auto it = warning_record.find( id );
    if( it != warning_record.end() ) {
        if( it->second.second - calendar::turn > 5_minutes ) {
            it->second.first = std::max( 0,
                                         it->second.first - 1 );
        }
        return it->second.first > 3;
    }
    return false;
}

item::reload_option player::select_ammo( const item &base,
        std::vector<item::reload_option> opts ) const
{
    if( opts.empty() ) {
        add_msg_if_player( m_info, _( "Never mind." ) );
        return item::reload_option();
    }

    uilist menu;
    menu.text = string_format( base.is_watertight_container() ? _( "Refill %s" ) :
                               base.has_flag( "RELOAD_AND_SHOOT" ) ? _( "Select ammo for %s" ) : _( "Reload %s" ),
                               base.tname() );
    menu.w_width = -1;
    menu.w_height = -1;

    // Construct item names
    std::vector<std::string> names;
    std::transform( opts.begin(), opts.end(),
    std::back_inserter( names ), [&]( const item::reload_option & e ) {
        if( e.ammo->is_magazine() && e.ammo->ammo_data() ) {
            if( e.ammo->ammo_current() == "battery" ) {
                // This battery ammo is not a real object that can be recovered but pseudo-object that represents charge
                //~ battery storage (charges)
                return string_format( pgettext( "magazine", "%1$s (%2$d)" ), e.ammo->type_name(),
                                      e.ammo->ammo_remaining() );
            } else {
                //~ magazine with ammo (count)
                return string_format( pgettext( "magazine", "%1$s with %2$s (%3$d)" ), e.ammo->type_name(),
                                      e.ammo->ammo_data()->nname( e.ammo->ammo_remaining() ), e.ammo->ammo_remaining() );
            }
        } else if( e.ammo->is_watertight_container() ||
                   ( e.ammo->is_ammo_container() && is_worn( *e.ammo ) ) ) {
            // worn ammo containers should be named by their contents with their location also updated below
            return e.ammo->contents.front().display_name();

        } else {
            return ( ammo_location && ammo_location == e.ammo ? "* " : "" ) + e.ammo->display_name();
        }
    } );

    // Get location descriptions
    std::vector<std::string> where;
    std::transform( opts.begin(), opts.end(),
    std::back_inserter( where ), [this]( const item::reload_option & e ) {
        bool is_ammo_container = e.ammo->is_ammo_container();
        if( is_ammo_container || e.ammo->is_container() ) {
            if( is_ammo_container && is_worn( *e.ammo ) ) {
                return e.ammo->type_name();
            }
            return string_format( _( "%s, %s" ), e.ammo->type_name(), e.ammo.describe( &g->u ) );
        }
        return e.ammo.describe( &g->u );
    } );

    // Pads elements to match longest member and return length
    auto pad = []( std::vector<std::string> &vec, int n, int t ) -> int {
        for( const auto &e : vec )
        {
            n = std::max( n, utf8_width( e, true ) + t );
        }
        for( auto &e : vec )
        {
            e += std::string( n - utf8_width( e, true ), ' ' );
        }
        return n;
    };

    // Pad the first column including 4 trailing spaces
    int w = pad( names, utf8_width( menu.text, true ), 6 );
    menu.text.insert( 0, 2, ' ' ); // add space for UI hotkeys
    menu.text += std::string( w + 2 - utf8_width( menu.text, true ), ' ' );

    // Pad the location similarly (excludes leading "| " and trailing " ")
    w = pad( where, utf8_width( _( "| Location " ) ) - 3, 6 );
    menu.text += _( "| Location " );
    menu.text += std::string( w + 3 - utf8_width( _( "| Location " ) ), ' ' );

    menu.text += _( "| Amount  " );
    menu.text += _( "| Moves   " );

    // We only show ammo statistics for guns and magazines
    if( base.is_gun() || base.is_magazine() ) {
        menu.text += _( "| Damage  | Pierce  " );
    }

    auto draw_row = [&]( int idx ) {
        const auto &sel = opts[ idx ];
        std::string row = string_format( "%s| %s |", names[ idx ], where[ idx ] );
        row += string_format( ( sel.ammo->is_ammo() ||
                                sel.ammo->is_ammo_container() ) ? " %-7d |" : "         |", sel.qty() );
        row += string_format( " %-7d ", sel.moves() );

        if( base.is_gun() || base.is_magazine() ) {
            const itype *ammo = sel.ammo->is_ammo_container() ? sel.ammo->contents.front().ammo_data() :
                                sel.ammo->ammo_data();
            if( ammo ) {
                const damage_instance &dam = ammo->ammo->damage;
                row += string_format( "| %-7d | %-7d", static_cast<int>( dam.total_damage() ),
                                      static_cast<int>( dam.empty() ? 0.0f : ( *dam.begin() ).res_pen ) );
            } else {
                row += "|         |         ";
            }
        }
        return row;
    };

    itype_id last = uistate.lastreload[ ammotype( base.ammo_default() ) ];
    // We keep the last key so that pressing the key twice (for example, r-r for reload)
    // will always pick the first option on the list.
    int last_key = inp_mngr.get_previously_pressed_key();
    bool last_key_bound = false;
    // This is the entry that has out default
    int default_to = 0;

    // If last_key is RETURN, don't use that to override hotkey
    if( last_key == '\n' ) {
        last_key_bound = true;
        default_to = -1;
    }

    for( auto i = 0; i < static_cast<int>( opts.size() ); ++i ) {
        const item &ammo = opts[ i ].ammo->is_ammo_container() ? opts[ i ].ammo->contents.front() :
                           *opts[ i ].ammo;

        char hotkey = -1;
        if( has_item( ammo ) ) {
            // if ammo in player possession and either it or any container has a valid invlet use this
            if( ammo.invlet ) {
                hotkey = ammo.invlet;
            } else {
                for( const auto obj : parents( ammo ) ) {
                    if( obj->invlet ) {
                        hotkey = obj->invlet;
                        break;
                    }
                }
            }
        }
        if( last == ammo.typeId() ) {
            if( !last_key_bound && hotkey == -1 ) {
                // If this is the first occurrence of the most recently used type of ammo and the hotkey
                // was not already set above then set it to the keypress that opened this prompt
                hotkey = last_key;
                last_key_bound = true;
            }
            if( !last_key_bound ) {
                // Pressing the last key defaults to the first entry of compatible type
                default_to = i;
                last_key_bound = true;
            }
        }
        if( hotkey == last_key ) {
            last_key_bound = true;
            // Prevent the default from being used: key is bound to something already
            default_to = -1;
        }

        menu.addentry( i, true, hotkey, draw_row( i ) );
    }

    struct reload_callback : public uilist_callback {
        public:
            std::vector<item::reload_option> &opts;
            const std::function<std::string( int )> draw_row;
            int last_key;
            const int default_to;
            const bool can_partial_reload;

            reload_callback( std::vector<item::reload_option> &_opts,
                             std::function<std::string( int )> _draw_row,
                             int _last_key, int _default_to, bool _can_partial_reload ) :
                opts( _opts ), draw_row( _draw_row ),
                last_key( _last_key ), default_to( _default_to ),
                can_partial_reload( _can_partial_reload )
            {}

            bool key( const input_context &, const input_event &event, int idx, uilist *menu ) override {
                auto cur_key = event.get_first_input();
                if( default_to != -1 && cur_key == last_key ) {
                    // Select the first entry on the list
                    menu->ret = default_to;
                    return true;
                }
                if( idx < 0 || idx >= static_cast<int>( opts.size() ) ) {
                    return false;
                }
                auto &sel = opts[ idx ];
                switch( cur_key ) {
                    case KEY_LEFT:
                        if( can_partial_reload ) {
                            sel.qty( sel.qty() - 1 );
                            menu->entries[ idx ].txt = draw_row( idx );
                        }
                        return true;

                    case KEY_RIGHT:
                        if( can_partial_reload ) {
                            sel.qty( sel.qty() + 1 );
                            menu->entries[ idx ].txt = draw_row( idx );
                        }
                        return true;
                }
                return false;
            }
    } cb( opts, draw_row, last_key, default_to, !base.has_flag( "RELOAD_ONE" ) );
    menu.callback = &cb;

    menu.query();
    if( menu.ret < 0 || static_cast<size_t>( menu.ret ) >= opts.size() ) {
        add_msg_if_player( m_info, _( "Never mind." ) );
        return item::reload_option();
    }

    const item_location &sel = opts[ menu.ret ].ammo;
    uistate.lastreload[ ammotype( base.ammo_default() ) ] = sel->is_ammo_container() ?
            sel->contents.front().typeId() :
            sel->typeId();
    return opts[ menu.ret ];
}

bool player::list_ammo( const item &base, std::vector<item::reload_option> &ammo_list,
                        bool empty ) const
{
    auto opts = base.gunmods();
    opts.push_back( &base );

    if( base.magazine_current() ) {
        opts.push_back( base.magazine_current() );
    }

    for( const auto mod : base.gunmods() ) {
        if( mod->magazine_current() ) {
            opts.push_back( mod->magazine_current() );
        }
    }

    bool ammo_match_found = false;
    int ammo_search_range = is_mounted() ? -1 : 1;
    for( const auto e : opts ) {
        for( item_location &ammo : find_ammo( *e, empty, ammo_search_range ) ) {
            // don't try to unload frozen liquids
            if( ammo->is_watertight_container() && ammo->contents_made_of( SOLID ) ) {
                continue;
            }
            auto id = ( ammo->is_ammo_container() || ammo->is_container() )
                      ? ammo->contents.front().typeId()
                      : ammo->typeId();
            if( e->can_reload_with( id ) ) {
                // Speedloaders require an empty target.
                if( !ammo->has_flag( "SPEEDLOADER" ) || e->ammo_remaining() < 1 ) {
                    ammo_match_found = true;
                }
            }
            if( can_reload( *e, id ) || e->has_flag( "RELOAD_AND_SHOOT" ) ) {
                ammo_list.emplace_back( this, e, &base, std::move( ammo ) );
            }
        }
    }
    return ammo_match_found;
}

item::reload_option player::select_ammo( const item &base, bool prompt, bool empty ) const
{
    std::vector<item::reload_option> ammo_list;
    bool ammo_match_found = list_ammo( base, ammo_list, empty );

    if( ammo_list.empty() ) {
        if( !is_npc() ) {
            if( !base.is_magazine() && !base.magazine_integral() && !base.magazine_current() ) {
                add_msg_if_player( m_info, _( "You need a compatible magazine to reload the %s!" ),
                                   base.tname() );

            } else if( ammo_match_found ) {
                add_msg_if_player( m_info, _( "Nothing to reload!" ) );
            } else {
                std::string name;
                if( base.ammo_data() ) {
                    name = base.ammo_data()->nname( 1 );
                } else if( base.is_watertight_container() ) {
                    name = base.is_container_empty() ? "liquid" : base.contents.front().tname();
                } else {
                    name = enumerate_as_string( base.ammo_types().begin(),
                    base.ammo_types().end(), []( const ammotype & at ) {
                        return at->name();
                    }, enumeration_conjunction::none );
                }
                add_msg_if_player( m_info, _( "You don't have any %s to reload your %s!" ),
                                   name, base.tname() );
            }
        }
        return item::reload_option();
    }

    // sort in order of move cost (ascending), then remaining ammo (descending) with empty magazines always last
    std::stable_sort( ammo_list.begin(), ammo_list.end(), []( const item::reload_option & lhs,
    const item::reload_option & rhs ) {
        return lhs.ammo->ammo_remaining() > rhs.ammo->ammo_remaining();
    } );
    std::stable_sort( ammo_list.begin(), ammo_list.end(), []( const item::reload_option & lhs,
    const item::reload_option & rhs ) {
        return lhs.moves() < rhs.moves();
    } );
    std::stable_sort( ammo_list.begin(), ammo_list.end(), []( const item::reload_option & lhs,
    const item::reload_option & rhs ) {
        return ( lhs.ammo->ammo_remaining() != 0 ) > ( rhs.ammo->ammo_remaining() != 0 );
    } );

    if( is_npc() ) {
        return ammo_list[ 0 ];
    }

    if( !prompt && ammo_list.size() == 1 ) {
        // unconditionally suppress the prompt if there's only one option
        return ammo_list[ 0 ];
    }

    return select_ammo( base, std::move( ammo_list ) );
}

ret_val<bool> player::can_wield( const item &it ) const
{
    if( it.made_of_from_type( LIQUID ) ) {
        return ret_val<bool>::make_failure( _( "Can't wield spilt liquids." ) );
    }

    if( get_working_arm_count() <= 0 ) {
        return ret_val<bool>::make_failure(
                   _( "You need at least one arm to even consider wielding something." ) );
    }

    if( is_armed() && weapon.has_flag( "NO_UNWIELD" ) ) {
        return ret_val<bool>::make_failure( _( "The %s is preventing you from wielding the %s." ),
                                            weapname(), it.tname() );
    }

    if( it.is_two_handed( *this ) && ( !has_two_arms() || worn_with_flag( "RESTRICT_HANDS" ) ) ) {
        if( worn_with_flag( "RESTRICT_HANDS" ) ) {
            return ret_val<bool>::make_failure(
                       _( "Something you are wearing hinders the use of both hands." ) );
        } else if( it.has_flag( "ALWAYS_TWOHAND" ) ) {
            return ret_val<bool>::make_failure( _( "The %s can't be wielded with only one arm." ),
                                                it.tname() );
        } else {
            return ret_val<bool>::make_failure( _( "You are too weak to wield %s with only one arm." ),
                                                it.tname() );
        }
    }

    return ret_val<bool>::make_success();
}

bool player::unwield()
{
    if( weapon.is_null() ) {
        return true;
    }

    if( !can_unwield( weapon ).success() ) {
        return false;
    }

    const std::string query = string_format( _( "Stop wielding %s?" ), weapon.tname() );

    if( !dispose_item( item_location( *this, &weapon ), query ) ) {
        return false;
    }

    inv.unsort();

    return true;
}

// ids of martial art styles that are available with the bio_cqb bionic.
static const std::vector<matype_id> bio_cqb_styles{ {
        matype_id{ "style_aikido" },
        matype_id{ "style_biojutsu" },
        matype_id{ "style_boxing" },
        matype_id{ "style_capoeira" },
        matype_id{ "style_crane" },
        matype_id{ "style_dragon" },
        matype_id{ "style_judo" },
        matype_id{ "style_karate" },
        matype_id{ "style_krav_maga" },
        matype_id{ "style_leopard" },
        matype_id{ "style_muay_thai" },
        matype_id{ "style_ninjutsu" },
        matype_id{ "style_pankration" },
        matype_id{ "style_snake" },
        matype_id{ "style_taekwondo" },
        matype_id{ "style_tai_chi" },
        matype_id{ "style_tiger" },
        matype_id{ "style_wingchun" },
        matype_id{ "style_zui_quan" }
    }};

bool character_martial_arts::pick_style( const avatar &you ) // Style selection menu
{
    enum style_selection {
        KEEP_HANDS_FREE = 0,
        STYLE_OFFSET
    };

    // If there are style already, cursor starts there
    // if no selected styles, cursor starts from no-style

    // Any other keys quit the menu
    const std::vector<matype_id> &selectable_styles = you.has_active_bionic(
                bio_cqb ) ? bio_cqb_styles :
            ma_styles;

    input_context ctxt( "MELEE_STYLE_PICKER" );
    ctxt.register_action( "SHOW_DESCRIPTION" );

    uilist kmenu;
    kmenu.text = string_format( _( "Select a style.\n"
                                   "\n"
                                   "STR: <color_white>%d</color>, DEX: <color_white>%d</color>, "
                                   "PER: <color_white>%d</color>, INT: <color_white>%d</color>\n"
                                   "Press [<color_yellow>%s</color>] for more info.\n" ),
                                you.get_str(), you.get_dex(), you.get_per(), you.get_int(),
                                ctxt.get_desc( "SHOW_DESCRIPTION" ) );
    ma_style_callback callback( static_cast<size_t>( STYLE_OFFSET ), selectable_styles );
    kmenu.callback = &callback;
    kmenu.input_category = "MELEE_STYLE_PICKER";
    kmenu.additional_actions.emplace_back( "SHOW_DESCRIPTION", translation() );
    kmenu.desc_enabled = true;
    kmenu.addentry_desc( KEEP_HANDS_FREE, true, 'h',
                         keep_hands_free ? _( "Keep hands free (on)" ) : _( "Keep hands free (off)" ),
                         _( "When this is enabled, player won't wield things unless explicitly told to." ) );

    kmenu.selected = STYLE_OFFSET;

    for( size_t i = 0; i < selectable_styles.size(); i++ ) {
        auto &style = selectable_styles[i].obj();
        //Check if this style is currently selected
        const bool selected = selectable_styles[i] == style_selected;
        std::string entry_text = style.name.translated();
        if( selected ) {
            kmenu.selected = i + STYLE_OFFSET;
            entry_text = colorize( entry_text, c_pink );
        }
        kmenu.addentry_desc( i + STYLE_OFFSET, true, -1, entry_text, style.description.translated() );
    }

    kmenu.query();
    int selection = kmenu.ret;

    if( selection >= STYLE_OFFSET ) {
        style_selected = selectable_styles[selection - STYLE_OFFSET];
        martialart_use_message( you );
    } else if( selection == KEEP_HANDS_FREE ) {
        keep_hands_free = !keep_hands_free;
    } else {
        return false;
    }

    return true;
}

hint_rating player::rate_action_wear( const item &it ) const
{
    // TODO: flag already-worn items as hint_rating::iffy

    if( !it.is_armor() ) {
        return hint_rating::cant;
    }

    return can_wear( it ).success() ? hint_rating::good : hint_rating::iffy;
}

bool player::can_reload( const item &it, const itype_id &ammo ) const
{
    if( !it.is_reloadable_with( ammo ) ) {
        return false;
    }

    if( it.is_ammo_belt() ) {
        const auto &linkage = it.type->magazine->linkage;
        if( linkage && !has_charges( *linkage, 1 ) ) {
            return false;
        }
    }

    return true;
}

void player::mend_item( item_location &&obj, bool interactive )
{
    if( has_trait( trait_DEBUG_HS ) ) {
        uilist menu;
        menu.text = _( "Toggle which fault?" );
        std::vector<std::pair<fault_id, bool>> opts;
        for( const auto &f : obj->faults_potential() ) {
            opts.emplace_back( f, !!obj->faults.count( f ) );
            menu.addentry( -1, true, -1, string_format(
                               opts.back().second ? pgettext( "fault", "Mend: %s" ) : pgettext( "fault", "Set: %s" ),
                               f.obj().name() ) );
        }
        if( opts.empty() ) {
            add_msg( m_info, _( "The %s doesn't have any faults to toggle." ), obj->tname() );
            return;
        }
        menu.query();
        if( menu.ret >= 0 ) {
            if( opts[ menu.ret ].second ) {
                obj->faults.erase( opts[ menu.ret ].first );
            } else {
                obj->faults.insert( opts[ menu.ret ].first );
            }
        }
        return;
    }

    auto inv = crafting_inventory();

    struct mending_option {
        fault_id fault;
        std::reference_wrapper<const mending_method> method;
        bool doable;
    };

    std::vector<mending_option> mending_options;
    for( const fault_id &f : obj->faults ) {
        for( const auto &m : f->mending_methods() ) {
            mending_option opt { f, m.second, true };
            for( const auto &sk : m.second.skills ) {
                if( get_skill_level( sk.first ) < sk.second ) {
                    opt.doable = false;
                    break;
                }
            }
            opt.doable = opt.doable &&
                         m.second.requirements->can_make_with_inventory( inv, is_crafting_component );
            mending_options.emplace_back( opt );
        }
    }

    if( mending_options.empty() ) {
        if( interactive ) {
            add_msg( m_info, _( "The %s doesn't have any faults to mend." ), obj->tname() );
            if( obj->damage() > 0 ) {
                const std::set<std::string> &rep = obj->repaired_with();
                if( !rep.empty() ) {
                    const std::string repair_options =
                    enumerate_as_string( rep.begin(), rep.end(), []( const itype_id & e ) {
                        return item::nname( e );
                    }, enumeration_conjunction::or_ );

                    add_msg( m_info, _( "It is damaged, and could be repaired with %s.  "
                                        "%s to use one of those items." ),
                             repair_options, press_x( ACTION_USE ) );
                }
            }
        }
        return;
    }

    int sel = 0;
    if( interactive ) {
        uilist menu;
        menu.text = _( "Mend which fault?" );
        menu.desc_enabled = true;
        menu.desc_lines = 0; // Let uilist handle description height

        constexpr int fold_width = 80;

        for( const mending_option &opt : mending_options ) {
            const mending_method &method = opt.method;
            const nc_color col = opt.doable ? c_white : c_light_gray;

            auto reqs = method.requirements.obj();
            auto tools = reqs.get_folded_tools_list( fold_width, col, inv );
            auto comps = reqs.get_folded_components_list( fold_width, col, inv, is_crafting_component );

            std::string descr;
            if( method.turns_into ) {
                descr += string_format( _( "Turns into: <color_cyan>%s</color>\n" ),
                                        method.turns_into->obj().name() );
            }
            if( method.also_mends ) {
                descr += string_format( _( "Also mends: <color_cyan>%s</color>\n" ),
                                        method.also_mends->obj().name() );
            }
            descr += string_format( _( "Time required: <color_cyan>%s</color>\n" ),
                                    to_string_approx( method.time ) );
            if( method.skills.empty() ) {
                descr += string_format( _( "Skills: <color_cyan>none</color>\n" ) );
            } else {
                descr += string_format( _( "Skills: %s\n" ),
                                        enumerate_as_string( method.skills.begin(), method.skills.end(),
                [this]( const std::pair<skill_id, int> &sk ) -> std::string {
                    if( get_skill_level( sk.first ) >= sk.second )
                    {
                        return string_format( pgettext( "skill requirement",
                                                        //~ %1$s: skill name, %2$s: current skill level, %3$s: required skill level
                                                        "<color_cyan>%1$s</color> <color_green>(%2$d/%3$d)</color>" ),
                                              sk.first->name(), get_skill_level( sk.first ), sk.second );
                    } else
                    {
                        return string_format( pgettext( "skill requirement",
                                                        //~ %1$s: skill name, %2$s: current skill level, %3$s: required skill level
                                                        "<color_cyan>%1$s</color> <color_yellow>(%2$d/%3$d)</color>" ),
                                              sk.first->name(), get_skill_level( sk.first ), sk.second );
                    }
                } ) );
            }

            for( const std::string &line : tools ) {
                descr += line + "\n";
            }
            for( const std::string &line : comps ) {
                descr += line + "\n";
            }

            const std::string desc = method.description + "\n\n" + colorize( descr, col );
            menu.addentry_desc( -1, true, -1, method.name.translated(), desc );
        }
        menu.query();
        if( menu.ret < 0 ) {
            add_msg( _( "Never mind." ) );
            return;
        }
        sel = menu.ret;
    }

    if( sel >= 0 ) {
        const mending_option &opt = mending_options[sel];
        if( !opt.doable ) {
            if( interactive ) {
                add_msg( m_info, _( "You are currently unable to mend the %s this way." ), obj->tname() );
            }
            return;
        }

        const mending_method &method = opt.method;
        assign_activity( activity_id( "ACT_MEND_ITEM" ), to_moves<int>( method.time ) );
        activity.name = opt.fault.str();
        activity.str_values.emplace_back( method.id );
        activity.targets.push_back( std::move( obj ) );
    }
}

int player::item_reload_cost( const item &it, const item &ammo, int qty ) const
{
    if( ammo.is_ammo() || ammo.is_frozen_liquid() ) {
        qty = std::max( std::min( ammo.charges, qty ), 1 );
    } else if( ammo.is_ammo_container() || ammo.is_container() ) {
        qty = clamp( qty, ammo.contents.front().charges, 1 );
    } else if( ammo.is_magazine() ) {
        qty = 1;
    } else {
        debugmsg( "cannot determine reload cost as %s is neither ammo or magazine", ammo.tname() );
        return 0;
    }

    // If necessary create duplicate with appropriate number of charges
    item obj = ammo;
    obj = obj.split( qty );
    if( obj.is_null() ) {
        obj = ammo;
    }
    // No base cost for handling ammo - that's already included in obtain cost
    // We have the ammo in our hands right now
    int mv = item_handling_cost( obj, true, 0 );

    if( ammo.has_flag( "MAG_BULKY" ) ) {
        mv *= 1.5; // bulky magazines take longer to insert
    }

    if( !it.is_gun() && !it.is_magazine() ) {
        return mv + 100; // reload a tool or sealable container
    }

    /** @EFFECT_GUN decreases the time taken to reload a magazine */
    /** @EFFECT_PISTOL decreases time taken to reload a pistol */
    /** @EFFECT_SMG decreases time taken to reload an SMG */
    /** @EFFECT_RIFLE decreases time taken to reload a rifle */
    /** @EFFECT_SHOTGUN decreases time taken to reload a shotgun */
    /** @EFFECT_LAUNCHER decreases time taken to reload a launcher */

    int cost = ( it.is_gun() ? it.get_reload_time() : it.type->magazine->reload_time ) * qty;

    skill_id sk = it.is_gun() ? it.type->gun->skill_used : skill_gun;
    mv += cost / ( 1.0f + std::min( get_skill_level( sk ) * 0.1f, 1.0f ) );

    if( it.has_flag( "STR_RELOAD" ) ) {
        /** @EFFECT_STR reduces reload time of some weapons */
        mv -= get_str() * 20;
    }

    return std::max( mv, 25 );
}

cata::optional<std::list<item>::iterator>
player::wear( int pos, bool interactive )
{
    return wear( i_at( pos ), interactive );
}

cata::optional<std::list<item>::iterator>
player::wear( item &to_wear, bool interactive )
{
    if( is_worn( to_wear ) ) {
        if( interactive ) {
            add_msg_player_or_npc( m_info,
                                   _( "You are already wearing that." ),
                                   _( "<npcname> is already wearing that." )
                                 );
        }
        return cata::nullopt;
    }
    if( to_wear.is_null() ) {
        if( interactive ) {
            add_msg_player_or_npc( m_info,
                                   _( "You don't have that item." ),
                                   _( "<npcname> doesn't have that item." ) );
        }
        return cata::nullopt;
    }

    bool was_weapon;
    item to_wear_copy( to_wear );
    if( &to_wear == &weapon ) {
        weapon = item();
        was_weapon = true;
    } else {
        inv.remove_item( &to_wear );
        inv.restack( *this );
        was_weapon = false;
    }

    auto result = wear_item( to_wear_copy, interactive );
    if( !result ) {
        if( was_weapon ) {
            weapon = to_wear_copy;
        } else {
            inv.add_item( to_wear_copy, true );
        }
        return cata::nullopt;
    }

    return result;
}

hint_rating player::rate_action_takeoff( const item &it ) const
{
    if( !it.is_armor() ) {
        return hint_rating::cant;
    }

    if( is_worn( it ) ) {
        return hint_rating::good;
    }

    return hint_rating::iffy;
}

ret_val<bool> player::can_takeoff( const item &it, const std::list<item> *res )
{
    auto iter = std::find_if( worn.begin(), worn.end(), [ &it ]( const item & wit ) {
        return &it == &wit;
    } );

    if( iter == worn.end() ) {
        return ret_val<bool>::make_failure( !is_npc() ? _( "You are not wearing that item." ) :
                                            _( "<npcname> is not wearing that item." ) );
    }

    if( res == nullptr && !get_dependent_worn_items( it ).empty() ) {
        return ret_val<bool>::make_failure( !is_npc() ?
                                            _( "You can't take off power armor while wearing other power armor components." ) :
                                            _( "<npcname> can't take off power armor while wearing other power armor components." ) );
    }
    if( it.has_flag( "NO_TAKEOFF" ) ) {
        return ret_val<bool>::make_failure( !is_npc() ?
                                            _( "You can't take that item off." ) :
                                            _( "<npcname> can't take that item off." ) );
    }
    return ret_val<bool>::make_success();
}

bool player::takeoff( item &it, std::list<item> *res )
{
    const auto ret = can_takeoff( it, res );
    if( !ret.success() ) {
        add_msg( m_info, "%s", ret.c_str() );
        return false;
    }

    auto iter = std::find_if( worn.begin(), worn.end(), [ &it ]( const item & wit ) {
        return &it == &wit;
    } );

    if( res == nullptr ) {
        if( volume_carried() + it.volume() > volume_capacity_reduced_by( it.get_storage() ) ) {
            if( is_npc() || query_yn( _( "No room in inventory for your %s.  Drop it?" ),
                                      colorize( it.tname(), it.color_in_inventory() ) ) ) {
                item_location loc( *this, &it );
                drop( loc, pos() );
                return true; // the drop activity ends up taking off the item anyway so shouldn't try to do it again here
            } else {
                return false;
            }
        }
        iter->on_takeoff( *this );
        inv.add_item_keep_invlet( it );
    } else {
        iter->on_takeoff( *this );
        res->push_back( it );
    }

    add_msg_player_or_npc( _( "You take off your %s." ),
                           _( "<npcname> takes off their %s." ),
                           it.tname() );

    // TODO: Make this variable
    mod_moves( -250 );
    worn.erase( iter );

    recalc_sight_limits();
    reset_encumbrance();

    return true;
}

bool player::takeoff( int pos )
{
    return takeoff( i_at( pos ) );
}

bool player::add_or_drop_with_msg( item &it, const bool unloading )
{
    if( it.made_of( LIQUID ) ) {
        liquid_handler::consume_liquid( it, 1 );
        return it.charges <= 0;
    }
    it.charges = this->i_add_to_container( it, unloading );
    if( it.is_ammo() && it.charges == 0 ) {
        return true;
    } else if( !this->can_pickVolume( it ) ) {
        put_into_vehicle_or_drop( *this, item_drop_reason::too_large, { it } );
    } else if( !this->can_pickWeight( it, !get_option<bool>( "DANGEROUS_PICKUPS" ) ) ) {
        put_into_vehicle_or_drop( *this, item_drop_reason::too_heavy, { it } );
    } else {
        auto &ni = this->i_add( it );
        add_msg( _( "You put the %s in your inventory." ), ni.tname() );
        add_msg( m_info, "%c - %s", ni.invlet == 0 ? ' ' : ni.invlet, ni.tname() );
    }
    return true;
}

bool player::unload( item &it )
{
    // Unload a container consuming moves per item successfully removed
    if( it.is_container() || it.is_bandolier() ) {
        if( it.contents.empty() ) {
            add_msg( m_info, _( "The %s is already empty!" ), it.tname() );
            return false;
        }
        if( !it.can_unload_liquid() ) {
            add_msg( m_info, _( "The liquid can't be unloaded in its current state!" ) );
            return false;
        }

        bool changed = false;
        for( item *contained : it.contents.all_items_top() ) {
            int old_charges = contained->charges;
            const bool consumed = this->add_or_drop_with_msg( *contained, true );
            changed = changed || consumed || contained->charges != old_charges;
            if( consumed ) {
                this->mod_moves( -this->item_handling_cost( *contained ) );
                this->remove_item( *contained );
            }
        }

        if( changed ) {
            it.on_contents_changed();
        }
        return true;
    }

    // If item can be unloaded more than once (currently only guns) prompt user to choose
    std::vector<std::string> msgs( 1, it.tname() );
    std::vector<item *> opts( 1, &it );

    for( auto e : it.gunmods() ) {
        if( e->is_gun() && !e->has_flag( "NO_UNLOAD" ) &&
            ( e->magazine_current() || e->ammo_remaining() > 0 || e->casings_count() > 0 ) ) {
            msgs.emplace_back( e->tname() );
            opts.emplace_back( e );
        }
    }

    item *target = nullptr;
    if( opts.size() > 1 ) {
        const int ret = uilist( _( "Unload what?" ), msgs );
        if( ret >= 0 ) {
            target = opts[ret];
        }
    } else {
        target = &it;
    }

    if( target == nullptr ) {
        return false;
    }

    // Next check for any reasons why the item cannot be unloaded
    if( target->ammo_types().empty() || target->ammo_capacity() <= 0 ) {
        add_msg( m_info, _( "You can't unload a %s!" ), target->tname() );
        return false;
    }

    if( target->has_flag( "NO_UNLOAD" ) ) {
        if( target->has_flag( "RECHARGE" ) || target->has_flag( "USE_UPS" ) ) {
            add_msg( m_info, _( "You can't unload a rechargeable %s!" ), target->tname() );
        } else {
            add_msg( m_info, _( "You can't unload a %s!" ), target->tname() );
        }
        return false;
    }

    if( !target->magazine_current() && target->ammo_remaining() <= 0 && target->casings_count() <= 0 ) {
        if( target->is_tool() ) {
            add_msg( m_info, _( "Your %s isn't charged." ), target->tname() );
        } else {
            add_msg( m_info, _( "Your %s isn't loaded." ), target->tname() );
        }
        return false;
    }

    target->casings_handle( [&]( item & e ) {
        return this->i_add_or_drop( e );
    } );

    if( target->is_magazine() ) {
        player_activity unload_mag_act( activity_id( "ACT_UNLOAD_MAG" ) );
        assign_activity( unload_mag_act );
        activity.targets.emplace_back( item_location( *this, target ) );

        // Calculate the time to remove the contained ammo (consuming half as much time as required to load the magazine)
        int mv = 0;
        for( const item *content : target->contents.all_items_top() ) {
            mv += this->item_reload_cost( it, *content, content->charges ) / 2;
        }
        activity.moves_left += mv;

        // I think this means if unload is not done on ammo-belt, it takes as long as it takes to reload a mag.
        if( !it.is_ammo_belt() ) {
            activity.moves_left += mv;
        }
        activity.auto_resume = true;

        return true;

    } else if( target->magazine_current() ) {
        if( !this->add_or_drop_with_msg( *target->magazine_current(), true ) ) {
            return false;
        }
        // Eject magazine consuming half as much time as required to insert it
        this->moves -= this->item_reload_cost( *target, *target->magazine_current(), -1 ) / 2;

        target->remove_items_with( [&target]( const item & e ) {
            return target->magazine_current() == &e;
        } );

    } else if( target->ammo_remaining() ) {
        int qty = target->ammo_remaining();

        if( target->ammo_current() == "plut_cell" ) {
            qty = target->ammo_remaining() / PLUTONIUM_CHARGES;
            if( qty > 0 ) {
                add_msg( _( "You recover %i unused plutonium." ), qty );
            } else {
                add_msg( m_info, _( "You can't remove partially depleted plutonium!" ) );
                return false;
            }
        }

        // Construct a new ammo item and try to drop it
        item ammo( target->ammo_current(), calendar::turn, qty );
        if( target->is_filthy() ) {
            ammo.set_flag( "FILTHY" );
        }

        if( ammo.made_of_from_type( LIQUID ) ) {
            if( !this->add_or_drop_with_msg( ammo ) ) {
                qty -= ammo.charges; // only handled part (or none) of the liquid
            }
            if( qty <= 0 ) {
                return false; // no liquid was moved
            }

        } else if( !this->add_or_drop_with_msg( ammo, qty > 1 ) ) {
            return false;
        }

        // If successful remove appropriate qty of ammo consuming half as much time as required to load it
        this->moves -= this->item_reload_cost( *target, ammo, qty ) / 2;

        if( target->ammo_current() == "plut_cell" ) {
            qty *= PLUTONIUM_CHARGES;
        }

        target->ammo_set( target->ammo_current(), target->ammo_remaining() - qty );
    }

    // Turn off any active tools
    if( target->is_tool() && target->active && target->ammo_remaining() == 0 ) {
        target->type->invoke( *this, *target, this->pos() );
    }

    add_msg( _( "You unload your %s." ), target->tname() );
    return true;
}

void player::use_wielded()
{
    use( -1 );
}

hint_rating player::rate_action_reload( const item &it ) const
{
    hint_rating res = hint_rating::cant;

    // Guns may contain additional reloadable mods so check these first
    for( const auto mod : it.gunmods() ) {
        switch( rate_action_reload( *mod ) ) {
            case hint_rating::good:
                return hint_rating::good;

            case hint_rating::cant:
                continue;

            case hint_rating::iffy:
                res = hint_rating::iffy;
        }
    }

    if( !it.is_reloadable() ) {
        return res;
    }

    return can_reload( it ) ? hint_rating::good : hint_rating::iffy;
}

hint_rating player::rate_action_unload( const item &it ) const
{
    if( ( it.is_container() || it.is_bandolier() ) && !it.contents.empty() &&
        it.can_unload_liquid() ) {
        return hint_rating::good;
    }

    if( it.has_flag( "NO_UNLOAD" ) ) {
        return hint_rating::cant;
    }

    if( it.magazine_current() ) {
        return hint_rating::good;
    }

    for( auto e : it.gunmods() ) {
        if( e->is_gun() && !e->has_flag( "NO_UNLOAD" ) &&
            ( e->magazine_current() || e->ammo_remaining() > 0 || e->casings_count() > 0 ) ) {
            return hint_rating::good;
        }
    }

    if( it.ammo_types().empty() ) {
        return hint_rating::cant;
    }

    if( it.ammo_remaining() > 0 || it.casings_count() > 0 ) {
        return hint_rating::good;
    }

    if( it.ammo_capacity() > 0 ) {
        return hint_rating::iffy;
    }

    return hint_rating::cant;
}

hint_rating player::rate_action_mend( const item &it ) const
{
    // TODO: check also if item damage could be repaired via a tool
    if( !it.faults.empty() ) {
        return hint_rating::good;
    }
    return it.faults_potential().empty() ? hint_rating::cant : hint_rating::iffy;
}

hint_rating player::rate_action_disassemble( const item &it )
{
    if( can_disassemble( it, crafting_inventory() ).success() ) {
        return hint_rating::good; // possible

    } else if( recipe_dictionary::get_uncraft( it.typeId() ) ) {
        return hint_rating::iffy; // potentially possible but we currently lack requirements

    } else {
        return hint_rating::cant; // never possible
    }
}

hint_rating player::rate_action_use( const item &it ) const
{
    if( it.is_tool() ) {
        return it.ammo_sufficient() ? hint_rating::good : hint_rating::iffy;

    } else if( it.is_gunmod() ) {
        /** @EFFECT_GUN >0 allows rating estimates for gun modifications */
        if( get_skill_level( skill_gun ) == 0 ) {
            return hint_rating::iffy;
        } else {
            return hint_rating::good;
        }
    } else if( it.is_food() || it.is_medication() || it.is_book() || it.is_armor() ) {
        return hint_rating::iffy; //the rating is subjective, could be argued as hint_rating::cant or hint_rating::good as well
    } else if( it.type->has_use() ) {
        return hint_rating::good;
    } else if( !it.is_container_empty() ) {
        return rate_action_use( it.get_contained() );
    }

    return hint_rating::cant;
}

void player::use( int inventory_position )
{
    item &used = i_at( inventory_position );
    auto loc = item_location( *this, &used );

    use( loc );
}

void player::use( item_location loc )
{
    item &used = *loc.get_item();

    if( used.is_null() ) {
        add_msg( m_info, _( "You do not have that item." ) );
        return;
    }

    last_item = used.typeId();

    if( used.is_tool() ) {
        if( !used.type->has_use() ) {
            add_msg_if_player( _( "You can't do anything interesting with your %s." ), used.tname() );
            return;
        }
        invoke_item( &used, loc.position() );

    } else if( used.type->can_use( "DOGFOOD" ) ||
               used.type->can_use( "CATFOOD" ) ||
               used.type->can_use( "BIRDFOOD" ) ||
               used.type->can_use( "CATTLEFODDER" ) ) {
        invoke_item( &used, loc.position() );

    } else if( !used.is_craft() && ( used.is_medication() || ( !used.type->has_use() &&
                                     ( used.is_food() ||
                                       used.get_contained().is_food() ||
                                       used.get_contained().is_medication() ) ) ) ) {
        consume( loc );

    } else if( used.is_book() ) {
        // TODO: Handle this with dynamic dispatch.
        if( avatar *u = as_avatar() ) {
            u->read( used );
        }
    } else if( used.type->has_use() ) {
        invoke_item( &used, loc.position() );
    } else if( used.has_flag( flag_SPLINT ) ) {
        ret_val<bool> need_splint = can_wear( used );
        if( need_splint.success() ) {
            wear_item( used );
            loc.remove_item();
        } else {
            add_msg( m_info, need_splint.str() );
        }
    } else {
        add_msg( m_info, _( "You can't do anything interesting with your %s." ),
                 used.tname() );
    }
}

void player::reassign_item( item &it, int invlet )
{
    bool remove_old = true;
    if( invlet ) {
        item *prev = invlet_to_item( invlet );
        if( prev != nullptr ) {
            remove_old = it.typeId() != prev->typeId();
            inv.reassign_item( *prev, it.invlet, remove_old );
        }
    }

    if( !invlet || inv_chars.valid( invlet ) ) {
        const auto iter = inv.assigned_invlet.find( it.invlet );
        bool found = iter != inv.assigned_invlet.end();
        if( found ) {
            inv.assigned_invlet.erase( iter );
        }
        if( invlet && ( !found || it.invlet != invlet ) ) {
            inv.assigned_invlet[invlet] = it.typeId();
        }
        inv.reassign_item( it, invlet, remove_old );
    }
}

static bool has_mod( const item &gun, const item &mod )
{
    for( const item *toolmod : gun.gunmods() ) {
        if( &mod == toolmod ) {
            return true;
        }
    }
    return false;
}

bool player::gunmod_remove( item &gun, item &mod )
{
    if( !has_mod( gun, mod ) ) {
        debugmsg( "Cannot remove non-existent gunmod" );
        return false;
    }
    if( mod.ammo_remaining() && !g->unload( mod ) ) {
        return false;
    }

    gun.gun_set_mode( gun_mode_id( "DEFAULT" ) );
    //TODO: add activity for removing gunmods

    if( mod.typeId() == "brass_catcher" ) {
        gun.casings_handle( [&]( item & e ) {
            return i_add_or_drop( e );
        } );
    }

    const itype *modtype = mod.type;

    i_add_or_drop( mod );
    gun.remove_item( mod );

    //If the removed gunmod added mod locations, check to see if any mods are in invalid locations
    if( !modtype->gunmod->add_mod.empty() ) {
        std::map<gunmod_location, int> mod_locations = gun.get_mod_locations();
        for( const auto &slot : mod_locations ) {
            int free_slots = gun.get_free_mod_locations( slot.first );

            for( auto the_mod : gun.gunmods() ) {
                if( the_mod->type->gunmod->location == slot.first && free_slots < 0 ) {
                    gunmod_remove( gun, *the_mod );
                    free_slots++;
                } else if( mod_locations.find( the_mod->type->gunmod->location ) ==
                           mod_locations.end() ) {
                    gunmod_remove( gun, *the_mod );
                }
            }
        }
    }

    //~ %1$s - gunmod, %2$s - gun.
    add_msg_if_player( _( "You remove your %1$s from your %2$s." ), modtype->nname( 1 ),
                       gun.tname() );

    return true;
}

std::pair<int, int> player::gunmod_installation_odds( const item &gun, const item &mod ) const
{
    // Mods with INSTALL_DIFFICULT have a chance to fail, potentially damaging the gun
    if( !mod.has_flag( "INSTALL_DIFFICULT" ) || has_trait( trait_DEBUG_HS ) ) {
        return std::make_pair( 100, 0 );
    }

    int roll = 100; // chance of success (%)
    int risk = 0;   // chance of failure (%)
    int chances = 1; // start with 1 in 6 (~17% chance)

    for( const auto &e : mod.type->min_skills ) {
        // gain an additional chance for every level above the minimum requirement
        skill_id sk = e.first == "weapon" ? gun.gun_skill() : skill_id( e.first );
        chances += std::max( get_skill_level( sk ) - e.second, 0 );
    }
    // cap success from skill alone to 1 in 5 (~83% chance)
    roll = std::min( static_cast<double>( chances ), 5.0 ) / 6.0 * 100;
    // focus is either a penalty or bonus of at most +/-10%
    roll += ( std::min( std::max( focus_pool, 140 ), 60 ) - 100 ) / 4;
    // dexterity and intelligence give +/-2% for each point above or below 12
    roll += ( get_dex() - 12 ) * 2;
    roll += ( get_int() - 12 ) * 2;
    // each level of damage to the base gun reduces success by 10%
    roll -= std::max( gun.damage_level( 4 ), 0 ) * 10;
    roll = std::min( std::max( roll, 0 ), 100 );

    // risk of causing damage on failure increases with less durable guns
    risk = ( 100 - roll ) * ( ( 10.0 - std::min( gun.type->gun->durability, 9 ) ) / 10.0 );

    return std::make_pair( roll, risk );
}

void player::gunmod_add( item &gun, item &mod )
{
    if( !gun.is_gunmod_compatible( mod ).success() ) {
        debugmsg( "Tried to add incompatible gunmod" );
        return;
    }

    if( !has_item( gun ) && !has_item( mod ) ) {
        debugmsg( "Tried gunmod installation but mod/gun not in player possession" );
        return;
    }

    // first check at least the minimum requirements are met
    if( !has_trait( trait_DEBUG_HS ) && !can_use( mod, gun ) ) {
        return;
    }

    // any (optional) tool charges that are used during installation
    auto odds = gunmod_installation_odds( gun, mod );
    int roll = odds.first;
    int risk = odds.second;

    std::string tool;
    int qty = 0;

    if( mod.is_irremovable() ) {
        if( !query_yn( _( "Permanently install your %1$s in your %2$s?" ),
                       colorize( mod.tname(), mod.color_in_inventory() ),
                       colorize( gun.tname(), gun.color_in_inventory() ) ) ) {
            add_msg_if_player( _( "Never mind." ) );
            return; // player canceled installation
        }
    }

    // if chance of success <100% prompt user to continue
    if( roll < 100 ) {
        uilist prompt;
        prompt.text = string_format( _( "Attach your %1$s to your %2$s?" ), mod.tname(),
                                     gun.tname() );

        std::vector<std::function<void()>> actions;

        prompt.addentry( -1, true, 'w',
                         string_format( _( "Try without tools (%i%%) risking damage (%i%%)" ), roll, risk ) );
        actions.emplace_back( [&] {} );

        prompt.addentry( -1, has_charges( "small_repairkit", 100 ), 'f',
                         string_format( _( "Use 100 charges of firearm repair kit (%i%%)" ), std::min( roll * 2, 100 ) ) );

        actions.emplace_back( [&] {
            tool = "small_repairkit";
            qty = 100;
            roll *= 2; // firearm repair kit improves success...
            risk /= 2; // ...and reduces the risk of damage upon failure
        } );

        prompt.addentry( -1, has_charges( "large_repairkit", 25 ), 'g',
                         string_format( _( "Use 25 charges of gunsmith repair kit (%i%%)" ), std::min( roll * 3, 100 ) ) );

        actions.emplace_back( [&] {
            tool = "large_repairkit";
            qty = 25;
            roll *= 3; // gunsmith repair kit improves success markedly...
            risk = 0;  // ...and entirely prevents damage upon failure
        } );

        prompt.query();
        if( prompt.ret < 0 ) {
            add_msg_if_player( _( "Never mind." ) );
            return; // player canceled installation
        }
        actions[ prompt.ret ]();
    }

    const int moves = !has_trait( trait_DEBUG_HS ) ? mod.type->gunmod->install_time : 0;

    assign_activity( activity_id( "ACT_GUNMOD_ADD" ), moves, -1, 0, tool );
    activity.targets.push_back( item_location( *this, &gun ) );
    activity.targets.push_back( item_location( *this, &mod ) );
    activity.values.push_back( 0 ); // dummy value
    activity.values.push_back( roll ); // chance of success (%)
    activity.values.push_back( risk ); // chance of damage (%)
    activity.values.push_back( qty ); // tool charges
}

void player::toolmod_add( item_location tool, item_location mod )
{
    if( !tool && !mod ) {
        debugmsg( "Tried toolmod installation but mod/tool not available" );
        return;
    }
    // first check at least the minimum requirements are met
    if( !has_trait( trait_DEBUG_HS ) && !can_use( *mod, *tool ) ) {
        return;
    }

    if( !query_yn( _( "Permanently install your %1$s in your %2$s?" ),
                   colorize( mod->tname(), mod->color_in_inventory() ),
                   colorize( tool->tname(), tool->color_in_inventory() ) ) ) {
        add_msg_if_player( _( "Never mind." ) );
        return; // player canceled installation
    }

    assign_activity( activity_id( "ACT_TOOLMOD_ADD" ), 1, -1 );
    activity.targets.emplace_back( std::move( tool ) );
    activity.targets.emplace_back( std::move( mod ) );
}

bool player::fun_to_read( const item &book ) const
{
    // If you don't have a problem with eating humans, To Serve Man becomes rewarding
    if( ( has_trait( trait_CANNIBAL ) || has_trait( trait_PSYCHOPATH ) ||
          has_trait( trait_SAPIOVORE ) ) &&
        book.typeId() == "cookbook_human" ) {
        return true;
    } else if( has_trait( trait_SPIRITUAL ) && book.has_flag( "INSPIRATIONAL" ) ) {
        return true;
    } else {
        return book_fun_for( book, *this ) > 0;
    }
}

int player::book_fun_for( const item &book, const player &p ) const
{
    int fun_bonus = book.type->book->fun;
    if( !book.is_book() ) {
        debugmsg( "called avatar::book_fun_for with non-book" );
        return 0;
    }

    // If you don't have a problem with eating humans, To Serve Man becomes rewarding
    if( ( p.has_trait( trait_CANNIBAL ) || p.has_trait( trait_PSYCHOPATH ) ||
          p.has_trait( trait_SAPIOVORE ) ) &&
        book.typeId() == "cookbook_human" ) {
        fun_bonus = std::abs( fun_bonus );
    } else if( p.has_trait( trait_SPIRITUAL ) && book.has_flag( "INSPIRATIONAL" ) ) {
        fun_bonus = std::abs( fun_bonus * 3 );
    }

    if( has_trait( trait_LOVES_BOOKS ) ) {
        fun_bonus++;
    } else if( has_trait( trait_HATES_BOOKS ) ) {
        if( book.type->book->fun > 0 ) {
            fun_bonus = 0;
        } else {
            fun_bonus--;
        }
    }

    if( fun_bonus > 1 && book.get_chapters() > 0 && book.get_remaining_chapters( p ) == 0 ) {
        fun_bonus /= 2;
    }

    return fun_bonus;
}

bool player::studied_all_recipes( const itype &book ) const
{
    if( !book.book ) {
        return true;
    }
    for( auto &elem : book.book->recipes ) {
        if( !knows_recipe( elem.recipe ) ) {
            return false;
        }
    }
    return true;
}

const recipe_subset &player::get_learned_recipes() const
{
    // Cache validity check
    if( *_skills != *valid_autolearn_skills ) {
        for( const auto &r : recipe_dict.all_autolearn() ) {
            if( meets_skill_requirements( r->autolearn_requirements ) ) {
                learned_recipes->include( r );
            }
        }
        *valid_autolearn_skills = *_skills; // Reassign the validity stamp
    }

    return *learned_recipes;
}

recipe_subset player::get_recipes_from_books( const inventory &crafting_inv ) const
{
    recipe_subset res;

    for( const auto &stack : crafting_inv.const_slice() ) {
        const item &candidate = stack->front();

        for( std::pair<const recipe *, int> recipe_entry :
             candidate.get_available_recipes( *this ) ) {
            res.include( recipe_entry.first, recipe_entry.second );
        }
    }

    return res;
}

std::set<itype_id> player::get_books_for_recipe( const inventory &crafting_inv,
        const recipe *r ) const
{
    std::set<itype_id> book_ids;
    const int skill_level = get_skill_level( r->skill_used );
    for( auto &book_lvl : r->booksets ) {
        itype_id book_id = book_lvl.first;
        int required_skill_level = book_lvl.second;
        // NPCs don't need to identify books
        if( !has_identified( book_id ) ) {
            continue;
        }

        if( skill_level >= required_skill_level && crafting_inv.amount_of( book_id ) > 0 ) {
            book_ids.insert( book_id );
        }
    }
    return book_ids;
}

recipe_subset player::get_available_recipes( const inventory &crafting_inv,
        const std::vector<npc *> *helpers ) const
{
    recipe_subset res( get_learned_recipes() );

    res.include( get_recipes_from_books( crafting_inv ) );

    if( helpers != nullptr ) {
        for( npc *np : *helpers ) {
            // Directly form the helper's inventory
            res.include( get_recipes_from_books( np->inv ) );
            // Being told what to do
            res.include_if( np->get_learned_recipes(), [ this ]( const recipe & r ) {
                return get_skill_level( r.skill_used ) >= static_cast<int>( r.difficulty *
                        0.8f ); // Skilled enough to understand
            } );
        }
    }

    return res;
}

void player::try_to_sleep( const time_duration &dur )
{
    const optional_vpart_position vp = g->m.veh_at( pos() );
    const trap &trap_at_pos = g->m.tr_at( pos() );
    const ter_id ter_at_pos = g->m.ter( pos() );
    const furn_id furn_at_pos = g->m.furn( pos() );
    bool plantsleep = false;
    bool fungaloid_cosplay = false;
    bool websleep = false;
    bool webforce = false;
    bool websleeping = false;
    bool in_shell = false;
    bool watersleep = false;
    if( has_trait( trait_CHLOROMORPH ) ) {
        plantsleep = true;
        if( ( ter_at_pos == t_dirt || ter_at_pos == t_pit ||
              ter_at_pos == t_dirtmound || ter_at_pos == t_pit_shallow ||
              ter_at_pos == t_grass ) && !vp &&
            furn_at_pos == f_null ) {
            add_msg_if_player( m_good, _( "You relax as your roots embrace the soil." ) );
        } else if( vp ) {
            add_msg_if_player( m_bad, _( "It's impossible to sleep in this wheeled pot!" ) );
        } else if( furn_at_pos != f_null ) {
            add_msg_if_player( m_bad,
                               _( "The humans' furniture blocks your roots.  You can't get comfortable." ) );
        } else { // Floor problems
            add_msg_if_player( m_bad, _( "Your roots scrabble ineffectively at the unyielding surface." ) );
        }
    } else if( has_trait( trait_M_SKIN3 ) ) {
        fungaloid_cosplay = true;
        if( g->m.has_flag_ter_or_furn( "FUNGUS", pos() ) ) {
            add_msg_if_player( m_good,
                               _( "Our fibers meld with the ground beneath us.  The gills on our neck begin to seed the air with spores as our awareness fades." ) );
        }
    }
    if( has_trait( trait_WEB_WALKER ) ) {
        websleep = true;
    }
    // Not sure how one would get Arachnid w/o web-making, but Just In Case
    if( has_trait( trait_THRESH_SPIDER ) && ( has_trait( trait_WEB_SPINNER ) ||
            ( has_trait( trait_WEB_WEAVER ) ) ) ) {
        webforce = true;
    }
    if( websleep || webforce ) {
        int web = g->m.get_field_intensity( pos(), fd_web );
        if( !webforce ) {
            // At this point, it's kinda weird, but surprisingly comfy...
            if( web >= 3 ) {
                add_msg_if_player( m_good,
                                   _( "These thick webs support your weight, and are strangely comfortable…" ) );
                websleeping = true;
            } else if( web > 0 ) {
                add_msg_if_player( m_info,
                                   _( "You try to sleep, but the webs get in the way.  You brush them aside." ) );
                g->m.remove_field( pos(), fd_web );
            }
        } else {
            // Here, you're just not comfortable outside a nice thick web.
            if( web >= 3 ) {
                add_msg_if_player( m_good, _( "You relax into your web." ) );
                websleeping = true;
            } else {
                add_msg_if_player( m_bad,
                                   _( "You try to sleep, but you feel exposed and your spinnerets keep twitching." ) );
                add_msg_if_player( m_info, _( "Maybe a nice thick web would help you sleep." ) );
            }
        }
    }
    if( has_active_mutation( trait_SHELL2 ) ) {
        // Your shell's interior is a comfortable place to sleep.
        in_shell = true;
    }
    if( has_trait( trait_WATERSLEEP ) ) {
        if( underwater ) {
            add_msg_if_player( m_good,
                               _( "You lay beneath the waves' embrace, gazing up through the water's surface…" ) );
            watersleep = true;
        } else if( g->m.has_flag_ter( "SWIMMABLE", pos() ) ) {
            add_msg_if_player( m_good, _( "You settle into the water and begin to drowse…" ) );
            watersleep = true;
        }
    }
    if( !plantsleep && ( furn_at_pos.obj().comfort > static_cast<int>( comfort_level::neutral ) ||
                         ter_at_pos == t_improvised_shelter ||
                         trap_at_pos.comfort > static_cast<int>( comfort_level::neutral ) ||
                         in_shell || websleeping || watersleep ||
                         vp.part_with_feature( "SEAT", true ) ||
                         vp.part_with_feature( "BED", true ) ) ) {
        add_msg_if_player( m_good, _( "This is a comfortable place to sleep." ) );
    } else if( !plantsleep && !fungaloid_cosplay && !watersleep ) {
        if( !vp && ter_at_pos != t_floor ) {
            add_msg_if_player( ter_at_pos.obj().movecost <= 2 ?
                               _( "It's a little hard to get to sleep on this %s." ) :
                               _( "It's hard to get to sleep on this %s." ),
                               ter_at_pos.obj().name() );
        } else if( vp ) {
            if( vp->part_with_feature( VPFLAG_AISLE, true ) ) {
                add_msg_if_player(
                    //~ %1$s: vehicle name, %2$s: vehicle part name
                    _( "It's a little hard to get to sleep on this %2$s in %1$s." ),
                    vp->vehicle().disp_name(),
                    vp->part_with_feature( VPFLAG_AISLE, true )->part().name( false ) );
            } else {
                add_msg_if_player(
                    //~ %1$s: vehicle name
                    _( "It's hard to get to sleep in %1$s." ),
                    vp->vehicle().disp_name() );
            }
        }
    }
    add_msg_if_player( _( "You start trying to fall asleep." ) );
    if( has_active_bionic( bio_soporific ) ) {
        bio_soporific_powered_at_last_sleep_check = has_power();
        if( bio_soporific_powered_at_last_sleep_check ) {
            // The actual bonus is applied in sleep_spot( p ).
            add_msg_if_player( m_good, _( "Your soporific inducer starts working its magic." ) );
        } else {
            add_msg_if_player( m_bad, _( "Your soporific inducer doesn't have enough power to operate." ) );
        }
    }
    assign_activity( activity_id( "ACT_TRY_SLEEP" ), to_moves<int>( dur ) );
}

int player::sleep_spot( const tripoint &p ) const
{
    const int current_stim = get_stim();
    const comfort_response_t comfort_info = base_comfort_value( p );
    if( comfort_info.aid != nullptr ) {
        add_msg_if_player( m_info, _( "You use your %s for comfort." ), comfort_info.aid->tname() );
    }

    int sleepy = static_cast<int>( comfort_info.level );
    bool watersleep = has_trait( trait_WATERSLEEP );

    if( has_addiction( add_type::SLEEP ) ) {
        sleepy -= 4;
    }
    if( has_trait( trait_INSOMNIA ) ) {
        // 12.5 points is the difference between "tired" and "dead tired"
        sleepy -= 12;
    }
    if( has_trait( trait_EASYSLEEPER ) ) {
        // Low fatigue (being rested) has a much stronger effect than high fatigue
        // so it's OK for the value to be that much higher
        sleepy += 24;
    }
    if( has_active_bionic( bio_soporific ) ) {
        sleepy += 30;
    }
    if( has_trait( trait_EASYSLEEPER2 ) ) {
        // Mousefolk can sleep just about anywhere.
        sleepy += 40;
    }
    if( watersleep && g->m.has_flag_ter( "SWIMMABLE", pos() ) ) {
        sleepy += 10; //comfy water!
    }

    if( get_fatigue() < TIRED + 1 ) {
        sleepy -= static_cast<int>( ( TIRED + 1 - get_fatigue() ) / 4 );
    } else {
        sleepy += static_cast<int>( ( get_fatigue() - TIRED + 1 ) / 16 );
    }

    if( current_stim > 0 || !has_trait( trait_INSOMNIA ) ) {
        sleepy -= 2 * current_stim;
    } else {
        // Make it harder for insomniac to get around the trait
        sleepy -= current_stim;
    }

    return sleepy;
}

bool player::can_sleep()
{
    if( has_effect( effect_meth ) ) {
        // Sleep ain't happening until that meth wears off completely.
        return false;
    }

    // Since there's a bit of randomness to falling asleep, we want to
    // prevent exploiting this if can_sleep() gets called over and over.
    // Only actually check if we can fall asleep no more frequently than
    // every 30 minutes.  We're assuming that if we return true, we'll
    // immediately be falling asleep after that.
    //
    // Also if player debug menu'd time backwards this breaks, just do the
    // check anyway, this will reset the timer if 'dur' is negative.
    const time_point now = calendar::turn;
    const time_duration dur = now - last_sleep_check;
    if( dur >= 0_turns && dur < 30_minutes ) {
        return false;
    }
    last_sleep_check = now;

    int sleepy = sleep_spot( pos() );
    sleepy += rng( -8, 8 );
    bool result = sleepy > 0;

    if( has_active_bionic( bio_soporific ) ) {
        if( bio_soporific_powered_at_last_sleep_check && !has_power() ) {
            add_msg_if_player( m_bad, _( "Your soporific inducer runs out of power!" ) );
        } else if( !bio_soporific_powered_at_last_sleep_check && has_power() ) {
            add_msg_if_player( m_good, _( "Your soporific inducer starts back up." ) );
        }
        bio_soporific_powered_at_last_sleep_check = has_power();
    }

    return result;
}

// Returned values range from 1.0 (unimpeded vision) to 11.0 (totally blind).
//  1.0 is LIGHT_AMBIENT_LIT or brighter
//  4.0 is a dark clear night, barely bright enough for reading and crafting
//  6.0 is LIGHT_AMBIENT_DIM
//  7.3 is LIGHT_AMBIENT_MINIMAL, a dark cloudy night, unlit indoors
// 11.0 is zero light or blindness
float player::fine_detail_vision_mod( const tripoint &p ) const
{
    // PER_SLIME_OK implies you can get enough eyes around the bile
    // that you can generally see.  There still will be the haze, but
    // it's annoying rather than limiting.
    if( is_blind() ||
        ( ( has_effect( effect_boomered ) || has_effect( effect_darkness ) ) &&
          !has_trait( trait_PER_SLIME_OK ) ) ) {
        return 11.0;
    }
    // Scale linearly as light level approaches LIGHT_AMBIENT_LIT.
    // If we're actually a source of light, assume we can direct it where we need it.
    // Therefore give a hefty bonus relative to ambient light.
    float own_light = std::max( 1.0, LIGHT_AMBIENT_LIT - active_light() - 2 );

    // Same calculation as above, but with a result 3 lower.
    float ambient_light = std::max( 1.0,
                                    LIGHT_AMBIENT_LIT - g->m.ambient_light_at( p == tripoint_zero ? pos() : p ) + 1.0 );

    return std::min( own_light, ambient_light );
}

void player::practice( const skill_id &id, int amount, int cap, bool suppress_warning )
{
    SkillLevel &level = get_skill_level_object( id );
    const Skill &skill = id.obj();
    std::string skill_name = skill.name();

    if( !level.can_train() && !in_sleep_state() ) {
        // If leveling is disabled, don't train, don't drain focus, don't print anything
        // Leaving as a skill method rather than global for possible future skill cap setting
        return;
    }

    const auto highest_skill = [&]() {
        std::pair<skill_id, int> result( skill_id::NULL_ID(), -1 );
        for( const auto &pair : *_skills ) {
            const SkillLevel &lobj = pair.second;
            if( lobj.level() > result.second ) {
                result = std::make_pair( pair.first, lobj.level() );
            }
        }
        return result.first;
    };

    const bool isSavant = has_trait( trait_SAVANT );
    const skill_id savantSkill = isSavant ? highest_skill() : skill_id::NULL_ID();

    amount = adjust_for_focus( amount );

    if( has_trait( trait_PACIFIST ) && skill.is_combat_skill() ) {
        if( !one_in( 3 ) ) {
            amount = 0;
        }
    }
    if( has_trait_flag( "PRED2" ) && skill.is_combat_skill() ) {
        if( one_in( 3 ) ) {
            amount *= 2;
        }
    }
    if( has_trait_flag( "PRED3" ) && skill.is_combat_skill() ) {
        amount *= 2;
    }

    if( has_trait_flag( "PRED4" ) && skill.is_combat_skill() ) {
        amount *= 3;
    }

    if( isSavant && id != savantSkill ) {
        amount /= 2;
    }

    if( amount > 0 && get_skill_level( id ) > cap ) { //blunt grinding cap implementation for crafting
        amount = 0;
        if( !suppress_warning ) {
            handle_skill_warning( id, false );
        }
    }
    if( amount > 0 && level.isTraining() ) {
        int oldLevel = get_skill_level( id );
        get_skill_level_object( id ).train( amount );
        int newLevel = get_skill_level( id );
        if( is_player() && newLevel > oldLevel ) {
            add_msg( m_good, _( "Your skill in %s has increased to %d!" ), skill_name, newLevel );
        }
        if( is_player() && newLevel > cap ) {
            //inform player immediately that the current recipe can't be used to train further
            add_msg( m_info, _( "You feel that %s tasks of this level are becoming trivial." ),
                     skill_name );
        }

        int chance_to_drop = focus_pool;
        focus_pool -= chance_to_drop / 100;
        // Apex Predators don't think about much other than killing.
        // They don't lose Focus when practicing combat skills.
        if( ( rng( 1, 100 ) <= ( chance_to_drop % 100 ) ) && ( !( has_trait_flag( "PRED4" ) &&
                skill.is_combat_skill() ) ) ) {
            focus_pool--;
        }
    }

    get_skill_level_object( id ).practice();
}

void player::handle_skill_warning( const skill_id &id, bool force_warning )
{
    //remind the player intermittently that no skill gain takes place
    if( is_player() && ( force_warning || one_in( 5 ) ) ) {
        SkillLevel &level = get_skill_level_object( id );

        const Skill &skill = id.obj();
        std::string skill_name = skill.name();
        int curLevel = level.level();

        add_msg( m_info, _( "This task is too simple to train your %s beyond %d." ),
                 skill_name, curLevel );
    }
}

int player::exceeds_recipe_requirements( const recipe &rec ) const
{
    return get_all_skills().exceeds_recipe_requirements( rec );
}

bool player::has_recipe_requirements( const recipe &rec ) const
{
    return get_all_skills().has_recipe_requirements( rec );
}

bool player::can_decomp_learn( const recipe &rec ) const
{
    return !rec.learn_by_disassembly.empty() &&
           meets_skill_requirements( rec.learn_by_disassembly );
}

bool player::knows_recipe( const recipe *rec ) const
{
    return get_learned_recipes().contains( rec );
}

int player::has_recipe( const recipe *r, const inventory &crafting_inv,
                        const std::vector<npc *> &helpers ) const
{
    if( !r->skill_used ) {
        return 0;
    }

    if( knows_recipe( r ) ) {
        return r->difficulty;
    }

    const auto available = get_available_recipes( crafting_inv, &helpers );
    return available.contains( r ) ? available.get_custom_difficulty( r ) : -1;
}

void player::learn_recipe( const recipe *const rec )
{
    if( rec->never_learn ) {
        return;
    }
    learned_recipes->include( rec );
}

bool player::has_gun_for_ammo( const ammotype &at ) const
{
    return has_item_with( [at]( const item & it ) {
        // item::ammo_type considers the active gunmod.
        return it.is_gun() && it.ammo_types().count( at );
    } );
}

bool player::has_magazine_for_ammo( const ammotype &at ) const
{
    return has_item_with( [&at]( const item & it ) {
        return !it.has_flag( "NO_RELOAD" ) &&
               ( ( it.is_magazine() && it.ammo_types().count( at ) ) ||
                 ( it.is_gun() && it.magazine_integral() && it.ammo_types().count( at ) ) ||
                 ( it.is_gun() && it.magazine_current() != nullptr &&
                   it.magazine_current()->ammo_types().count( at ) ) );
    } );
}

// mytest return weapon name to display in sidebar
std::string player::weapname( unsigned int truncate ) const
{
    if( weapon.is_gun() ) {
        std::string str = string_format( "(%s) %s", weapon.gun_current_mode().tname(), weapon.type_name() );

        // Is either the base item or at least one auxiliary gunmod loaded (includes empty magazines)
        bool base = weapon.ammo_capacity() > 0 && !weapon.has_flag( "RELOAD_AND_SHOOT" );

        const auto mods = weapon.gunmods();
        bool aux = std::any_of( mods.begin(), mods.end(), [&]( const item * e ) {
            return e->is_gun() && e->ammo_capacity() > 0 && !e->has_flag( "RELOAD_AND_SHOOT" );
        } );

        if( base || aux ) {
            str += " (";
            if( base ) {
                str += std::to_string( weapon.ammo_remaining() );
                if( weapon.magazine_integral() ) {
                    str += "/" + std::to_string( weapon.ammo_capacity() );
                }
            } else {
                str += "---";
            }
            str += ")";

            for( auto e : mods ) {
                if( e->is_gun() && e->ammo_capacity() > 0 && !e->has_flag( "RELOAD_AND_SHOOT" ) ) {
                    str += " (" + std::to_string( e->ammo_remaining() );
                    if( e->magazine_integral() ) {
                        str += "/" + std::to_string( e->ammo_capacity() );
                    }
                    str += ")";
                }
            }
        }
        return str;

    } else if( weapon.is_container() && weapon.contents.num_item_stacks() == 1 ) {
        return string_format( "%s (%d)", weapon.tname(),
                              weapon.contents.front().charges );

    } else if( !is_armed() ) {
        return _( "fists" );

    } else {
        return weapon.tname( 1, true, truncate );
    }
}

bool player::wield_contents( item &container, item *internal_item, bool penalties, int base_cost )
{
    // if index not specified and container has multiple items then ask the player to choose one
    if( internal_item == nullptr ) {
        std::vector<std::string> opts;
        std::list<item *> container_contents = container.contents.all_items_top();
        std::transform( container_contents.begin(), container_contents.end(),
        std::back_inserter( opts ), []( const item * elem ) {
            return elem->display_name();
        } );
        if( opts.size() > 1 ) {
            int pos = uilist( _( "Wield what?" ), opts );
            if( pos < 0 ) {
                return false;
            }
            internal_item = *std::next( container_contents.begin(), pos );
        } else {
            internal_item = &container.contents.front();
        }
    }

    if( !container.has_item( *internal_item ) ) {
        debugmsg( "Tried to wield non-existent item from container (player::wield_contents)" );
        return false;
    }

    const ret_val<bool> ret = can_wield( *internal_item );
    if( !ret.success() ) {
        add_msg_if_player( m_info, "%s", ret.c_str() );
        return false;
    }

    int mv = 0;

    if( is_armed() ) {
        if( !unwield() ) {
            return false;
        }
        inv.unsort();
    }

    weapon = std::move( *internal_item );
    container.remove_item( *internal_item );
    container.on_contents_changed();

    inv.update_invlet( weapon );
    inv.update_cache_with_item( weapon );
    last_item = weapon.typeId();

    /**
     * @EFFECT_PISTOL decreases time taken to draw pistols from holsters
     * @EFFECT_SMG decreases time taken to draw smgs from holsters
     * @EFFECT_RIFLE decreases time taken to draw rifles from holsters
     * @EFFECT_SHOTGUN decreases time taken to draw shotguns from holsters
     * @EFFECT_LAUNCHER decreases time taken to draw launchers from holsters
     * @EFFECT_STABBING decreases time taken to draw stabbing weapons from sheathes
     * @EFFECT_CUTTING decreases time taken to draw cutting weapons from scabbards
     * @EFFECT_BASHING decreases time taken to draw bashing weapons from holsters
     */
    int lvl = get_skill_level( weapon.is_gun() ? weapon.gun_skill() : weapon.melee_skill() );
    mv += item_handling_cost( weapon, penalties, base_cost ) / ( ( lvl + 10.0f ) / 10.0f );

    moves -= mv;

    weapon.on_wield( *this, mv );

    return true;
}

void player::store( item &container, item &put, bool penalties, int base_cost )
{
    moves -= item_store_cost( put, container, penalties, base_cost );
    container.put_in( i_rem( &put ) );
    reset_encumbrance();
}

nc_color encumb_color( int level )
{
    if( level < 0 ) {
        return c_green;
    }
    if( level < 10 ) {
        return c_light_gray;
    }
    if( level < 40 ) {
        return c_yellow;
    }
    if( level < 70 ) {
        return c_light_red;
    }
    return c_red;
}

float player::get_melee() const
{
    return get_skill_level( skill_id( "melee" ) );
}

bool player::uncanny_dodge()
{
    bool is_u = this == &g->u;
    bool seen = g->u.sees( *this );
    if( this->get_power_level() < 74_kJ || !this->has_active_bionic( bio_uncanny_dodge ) ) {
        return false;
    }
    tripoint adjacent = adjacent_tile();
    mod_power_level( -75_kJ );
    if( adjacent.x != posx() || adjacent.y != posy() ) {
        position.x = adjacent.x;
        position.y = adjacent.y;
        if( is_u ) {
            add_msg( _( "Time seems to slow down and you instinctively dodge!" ) );
        } else if( seen ) {
            add_msg( _( "%s dodges… so fast!" ), this->disp_name() );

        }
        return true;
    }
    if( is_u ) {
        add_msg( _( "You try to dodge but there's no room!" ) );
    } else if( seen ) {
        add_msg( _( "%s tries to dodge but there's no room!" ), this->disp_name() );
    }
    return false;
}

int player::climbing_cost( const tripoint &from, const tripoint &to ) const
{
    if( !g->m.valid_move( from, to, false, true ) ) {
        return 0;
    }

    const int diff = g->m.climb_difficulty( from );

    if( diff > 5 ) {
        return 0;
    }

    return 50 + diff * 100;
    // TODO: All sorts of mutations, equipment weight etc.
}

void player::environmental_revert_effect()
{
    addictions.clear();
    morale->clear();

    for( int part = 0; part < num_hp_parts; part++ ) {
        hp_cur[part] = hp_max[part];
    }
    set_hunger( 0 );
    set_thirst( 0 );
    set_fatigue( 0 );
    set_healthy( 0 );
    set_healthy_mod( 0 );
    set_stim( 0 );
    set_pain( 0 );
    set_painkiller( 0 );
    set_rad( 0 );

    recalc_sight_limits();
    reset_encumbrance();
}

void player::set_destination( const std::vector<tripoint> &route,
                              const player_activity &new_destination_activity )
{
    auto_move_route = route;
    set_destination_activity( new_destination_activity );
    destination_point.emplace( g->m.getabs( route.back() ) );
}

void player::clear_destination()
{
    auto_move_route.clear();
    clear_destination_activity();
    destination_point = cata::nullopt;
    next_expected_position = cata::nullopt;
}

bool player::has_distant_destination() const
{
    return has_destination() && !get_destination_activity().is_null() &&
           get_destination_activity().id() == "ACT_TRAVELLING" && !omt_path.empty();
}

bool player::is_auto_moving() const
{
    return destination_point.has_value();
}

bool player::has_destination() const
{
    return !auto_move_route.empty();
}

bool player::has_destination_activity() const
{
    return !get_destination_activity().is_null() && destination_point &&
           position == g->m.getlocal( *destination_point );
}

void player::start_destination_activity()
{
    if( !has_destination_activity() ) {
        debugmsg( "Tried to start invalid destination activity" );
        return;
    }

    assign_activity( get_destination_activity() );
    clear_destination();
}

std::vector<tripoint> &player::get_auto_move_route()
{
    return auto_move_route;
}

action_id player::get_next_auto_move_direction()
{
    if( !has_destination() ) {
        return ACTION_NULL;
    }

    if( next_expected_position ) {
        if( pos() != *next_expected_position ) {
            // We're off course, possibly stumbling or stuck, cancel auto move
            return ACTION_NULL;
        }
    }

    next_expected_position.emplace( auto_move_route.front() );
    auto_move_route.erase( auto_move_route.begin() );

    tripoint dp = *next_expected_position - pos();

    // Make sure the direction is just one step and that
    // all diagonal moves have 0 z component
    if( std::abs( dp.x ) > 1 || std::abs( dp.y ) > 1 || std::abs( dp.z ) > 1 ||
        ( std::abs( dp.z ) != 0 && ( std::abs( dp.x ) != 0 || std::abs( dp.y ) != 0 ) ) ) {
        // Should never happen, but check just in case
        return ACTION_NULL;
    }
    return get_movement_action_from_delta( dp, iso_rotate::yes );
}

bool player::defer_move( const tripoint &next )
{
    // next must be adjacent to current pos
    if( square_dist( next, pos() ) != 1 ) {
        return false;
    }
    // next must be adjacent to subsequent move in any preexisting automove route
    if( has_destination() && square_dist( auto_move_route.front(), next ) != 1 ) {
        return false;
    }
    auto_move_route.insert( auto_move_route.begin(), next );
    next_expected_position = pos();
    return true;
}

void player::shift_destination( const point &shift )
{
    if( next_expected_position ) {
        *next_expected_position += shift;
    }

    for( auto &elem : auto_move_route ) {
        elem += shift;
    }
}

bool player::has_weapon() const
{
    return !unarmed_attack();
}

int player::get_hp() const
{
    return get_hp( num_hp_parts );
}

int player::get_hp( hp_part bp ) const
{
    if( bp < num_hp_parts ) {
        return hp_cur[bp];
    }
    int hp_total = 0;
    for( int i = 0; i < num_hp_parts; ++i ) {
        hp_total += hp_cur[i];
    }
    return hp_total;
}

int player::get_hp_max() const
{
    return get_hp_max( num_hp_parts );
}

int player::get_hp_max( hp_part bp ) const
{
    if( bp < num_hp_parts ) {
        return hp_max[bp];
    }
    int hp_total = 0;
    for( int i = 0; i < num_hp_parts; ++i ) {
        hp_total += hp_max[i];
    }
    return hp_total;
}

Creature::Attitude player::attitude_to( const Creature &other ) const
{
    const auto m = dynamic_cast<const monster *>( &other );
    if( m != nullptr ) {
        if( m->friendly != 0 ) {
            return A_FRIENDLY;
        }
        switch( m->attitude( const_cast<player *>( this ) ) ) {
            // player probably does not want to harm them, but doesn't care much at all.
            case MATT_FOLLOW:
            case MATT_FPASSIVE:
            case MATT_IGNORE:
            case MATT_FLEE:
                return A_NEUTRAL;
            // player does not want to harm those.
            case MATT_FRIEND:
            case MATT_ZLAVE:
                // Don't want to harm your zlave!
                return A_FRIENDLY;
            case MATT_ATTACK:
                return A_HOSTILE;
            case MATT_NULL:
            case NUM_MONSTER_ATTITUDES:
                break;
        }

        return A_NEUTRAL;
    }

    const auto p = dynamic_cast<const npc *>( &other );
    if( p != nullptr ) {
        if( p->is_enemy() ) {
            return A_HOSTILE;
        } else if( p->is_player_ally() ) {
            return A_FRIENDLY;
        } else {
            return A_NEUTRAL;
        }
    } else if( &other == this ) {
        return A_FRIENDLY;
    }

    return A_NEUTRAL;
}

bool player::sees( const tripoint &t, bool, int ) const
{
    static const bionic_id str_bio_night( "bio_night" );
    const int wanted_range = rl_dist( pos(), t );
    bool can_see = is_player() ? g->m.pl_sees( t, wanted_range ) :
                   Creature::sees( t );
    // Clairvoyance is now pretty cheap, so we can check it early
    if( wanted_range < MAX_CLAIRVOYANCE && wanted_range < clairvoyance() ) {
        return true;
    }
    // Only check if we need to override if we already came to the opposite conclusion.
    if( can_see && wanted_range < 15 && wanted_range > sight_range( 1 ) &&
        has_active_bionic( str_bio_night ) ) {
        can_see = false;
    }
    if( can_see && wanted_range > unimpaired_range() ) {
        can_see = false;
    }

    return can_see;
}

bool player::sees( const Creature &critter ) const
{
    // This handles only the player/npc specific stuff (monsters don't have traits or bionics).
    const int dist = rl_dist( pos(), critter.pos() );
    if( dist <= 3 && has_active_mutation( trait_ANTENNAE ) ) {
        return true;
    }

    return Creature::sees( critter );
}

nc_color player::bodytemp_color( int bp ) const
{
    nc_color color = c_light_gray; // default
    if( bp == bp_eyes ) {
        color = c_light_gray;    // Eyes don't count towards warmth
    } else if( temp_conv[bp]  > BODYTEMP_SCORCHING ) {
        color = c_red;
    } else if( temp_conv[bp]  > BODYTEMP_VERY_HOT ) {
        color = c_light_red;
    } else if( temp_conv[bp]  > BODYTEMP_HOT ) {
        color = c_yellow;
    } else if( temp_conv[bp]  > BODYTEMP_COLD ) {
        color = c_green;
    } else if( temp_conv[bp]  > BODYTEMP_VERY_COLD ) {
        color = c_light_blue;
    } else if( temp_conv[bp]  > BODYTEMP_FREEZING ) {
        color = c_cyan;
    } else if( temp_conv[bp] <= BODYTEMP_FREEZING ) {
        color = c_blue;
    }
    return color;
}

//message related stuff
void player::add_msg_if_player( const std::string &msg ) const
{
    Messages::add_msg( msg );
}

void player::add_msg_player_or_npc( const std::string &player_msg,
                                    const std::string &/*npc_msg*/ ) const
{
    Messages::add_msg( player_msg );
}

void player::add_msg_if_player( const game_message_params &params, const std::string &msg ) const
{
    Messages::add_msg( params, msg );
}

void player::add_msg_player_or_npc( const game_message_params &params,
                                    const std::string &player_msg,
                                    const std::string &/*npc_msg*/ ) const
{
    Messages::add_msg( params, player_msg );
}

void player::add_msg_player_or_say( const std::string &player_msg,
                                    const std::string &/*npc_speech*/ ) const
{
    Messages::add_msg( player_msg );
}

void player::add_msg_player_or_say( const game_message_params &params,
                                    const std::string &player_msg,
                                    const std::string &/*npc_speech*/ ) const
{
    Messages::add_msg( params, player_msg );
}

bool player::knows_trap( const tripoint &pos ) const
{
    const tripoint p = g->m.getabs( pos );
    return known_traps.count( p ) > 0;
}

void player::add_known_trap( const tripoint &pos, const trap &t )
{
    const tripoint p = g->m.getabs( pos );
    if( t.is_null() ) {
        known_traps.erase( p );
    } else {
        // TODO: known_traps should map to a trap_str_id
        known_traps[p] = t.id.str();
    }
}

bool player::can_hear( const tripoint &source, const int volume ) const
{
    if( is_deaf() ) {
        return false;
    }

    // source is in-ear and at our square, we can hear it
    if( source == pos() && volume == 0 ) {
        return true;
    }
    const int dist = rl_dist( source, pos() );
    const float volume_multiplier = hearing_ability();
    return ( volume - weather::sound_attn( g->weather.weather ) ) * volume_multiplier >= dist;
}

float player::hearing_ability() const
{
    float volume_multiplier = 1.0;

    // Mutation/Bionic volume modifiers
    if( has_active_bionic( bio_ears ) && !has_active_bionic( bio_earplugs ) ) {
        volume_multiplier *= 3.5;
    }
    if( has_trait( trait_PER_SLIME ) ) {
        // Random hearing :-/
        // (when it's working at all, see player.cpp)
        // changed from 0.5 to fix Mac compiling error
        volume_multiplier *= ( rng( 1, 2 ) );
    }

    volume_multiplier *= Character::mutation_value( "hearing_modifier" );

    if( has_effect( effect_deaf ) ) {
        // Scale linearly up to 30 minutes
        volume_multiplier *= ( 30_minutes - get_effect_dur( effect_deaf ) ) / 30_minutes;
    }

    if( has_effect( effect_earphones ) ) {
        volume_multiplier *= .25;
    }

    return volume_multiplier;
}

std::vector<std::string> player::short_description_parts() const
{
    std::vector<std::string> result;

    if( is_armed() ) {
        result.push_back( _( "Wielding: " ) + weapon.tname() );
    }
    const std::string worn_str = enumerate_as_string( worn.begin(), worn.end(),
    []( const item & it ) {
        return it.tname();
    } );
    if( !worn_str.empty() ) {
        result.push_back( _( "Wearing: " ) + worn_str );
    }
    const int visibility_cap = 0; // no cap
    const auto trait_str = visible_mutations( visibility_cap );
    if( !trait_str.empty() ) {
        result.push_back( _( "Traits: " ) + trait_str );
    }
    return result;
}

std::string player::short_description() const
{
    return join( short_description_parts(), ";   " );
}

int player::print_info( const catacurses::window &w, int vStart, int, int column ) const
{
    mvwprintw( w, point( column, vStart++ ), _( "You (%s)" ), name );
    return vStart;
}

bool player::query_yn( const std::string &mes ) const
{
    return ::query_yn( mes );
}

