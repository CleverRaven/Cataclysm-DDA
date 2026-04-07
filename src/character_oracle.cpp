#include "character_oracle.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include "behavior.h"
#include "bodypart.h"
#include "calendar.h"
#include "character.h"
#include "coordinates.h"
#include "item.h"
#include "itype.h"
#include "map.h"
#include "mapdata.h"
#include "npc.h"
#include "npc_class.h"
#include "point.h"
#include "ret_val.h"
#include "stomach.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"
#include "weather.h"

static const efftype_id effect_meth( "meth" );
static const efftype_id effect_npc_run_away( "npc_run_away" );
static const json_character_flag json_flag_CANNOT_MOVE( "CANNOT_MOVE" );
static const trait_id trait_IGNORE_SOUND( "IGNORE_SOUND" );

namespace behavior
{

// To avoid a local minima when the character has access to warmth in a shelter but gets cold
// when they go outside, this method needs to only alert when travel time to known shelter
// approaches time to freeze.
status_t character_oracle_t::needs_warmth_badly( std::string_view ) const
{
    // Use player::temp_conv to predict whether the Character is "in trouble".
    for( const bodypart_id &bp : subject->get_all_body_parts() ) {
        if( subject->get_part_temp_conv( bp ) <= BODYTEMP_VERY_COLD ) {
            return status_t::running;
        }
    }
    return status_t::success;
}

status_t character_oracle_t::needs_water_badly( std::string_view ) const
{
    // Check thirst threshold.
    if( subject->get_thirst() > 520 ) {
        return status_t::running;
    }
    return status_t::success;
}

status_t character_oracle_t::needs_food_badly( std::string_view ) const
{
    // Short-term: stomach empty and actively starving.
    if( subject->get_hunger() >= 300 && subject->get_starvation() > base_metabolic_rate ) {
        return status_t::running;
    }
    // Long-term: severe caloric deficit (same threshold as address_needs extreme path).
    if( subject->get_stored_kcal() + subject->stomach.get_calories() <
        subject->get_healthy_kcal() * 3 / 4 ) {
        return status_t::running;
    }
    return status_t::success;
}

status_t character_oracle_t::can_wear_warmer_clothes( std::string_view ) const
{
    // Check inventory for wearable warmer clothes, greedily.
    // Don't consider swapping clothes yet, just evaluate adding clothes.
    bool found_clothes = subject->has_item_with( [this]( const item & candidate ) {
        return candidate.get_warmth() > 0 && subject->can_wear( candidate ).success() &&
               !subject->is_worn( candidate );
    } );
    return found_clothes ? status_t::running : status_t::failure;
}

status_t character_oracle_t::can_make_fire( std::string_view ) const
{
    // Delegate to npc::find_fire_spot() so the predicate and executor
    // share the same viability policy (tool quality, tinder scope,
    // fuel type, tile suitability).
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n ) {
        return status_t::failure;
    }
    return const_cast<npc *>( n )->find_fire_spot().has_value()
           ? status_t::running : status_t::failure;
}

status_t character_oracle_t::can_take_shelter( std::string_view ) const
{
    // Delegate to npc::find_nearby_shelters() so oracle and action share
    // the same detection logic (radius, LOS, passability, creature check).
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n ) {
        return status_t::failure;
    }
    return n->find_nearby_shelters().empty() ? status_t::failure : status_t::running;
}

status_t character_oracle_t::has_water( std::string_view ) const
{
    // Check if we know about water somewhere
    bool found_water = subject->has_item_with( []( const item & cand ) {
        return cand.is_food() && cand.get_comestible()->quench > 0;
    } );
    return found_water ? status_t::running : status_t::failure;
}

status_t character_oracle_t::has_food( std::string_view ) const
{
    // Check if we know about food somewhere
    bool found_food = subject->has_item_with( []( const item & cand ) {
        return cand.is_food() && cand.get_comestible()->has_calories();
    } );
    return found_food ? status_t::running : status_t::failure;
}

status_t character_oracle_t::can_obtain_food( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n ) {
        return status_t::failure;
    }
    // TODO: const_cast because will_eat/rate_food are not const-qualified.
    // Safe today (they do not mutate), but fragile if they gain side effects.
    // Fix by making the candidate query const once those methods are.
    // Camp food is included in the candidate list as need_source::camp_food.
    return const_cast<npc *>( n )->find_food_candidates().empty()
           ? status_t::failure : status_t::running;
}

status_t character_oracle_t::can_obtain_water( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n ) {
        return status_t::failure;
    }
    // Camp water is included in the candidate list as need_source::camp_water.
    return const_cast<npc *>( n )->find_water_candidates().empty()
           ? status_t::failure : status_t::running;
}

status_t character_oracle_t::can_obtain_warmth( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n ) {
        return status_t::failure;
    }
    // Inventory wearable warmth: the executor handles this in step 1,
    // but the predicate must detect it now that legacy
    // can_wear_warmer_clothes is no longer a separate BT goal.
    if( subject->has_item_with( [this]( const item & it ) {
    return it.get_warmth() > 0 && subject->can_wear( it ).success() &&
               !subject->is_worn( it );
    } ) ) {
        return status_t::running;
    }
    // TODO: same const_cast caveat as can_obtain_food above.
    // find_nearby_warm_clothing uses can_wear which is not const-qualified.
    if( !const_cast<npc *>( n )->find_warmth_candidates().empty() ) {
        return status_t::running;
    }
    // Already indoors with no other warmth sources: the executor will
    // hold position until warmth recovers. After the indoor hold
    // timeout (60 turns without progress), stop returning running so
    // the BT can assign follow/duty instead of holding forever.
    if( get_map().has_flag( ter_furn_flag::TFLAG_INDOORS, n->pos_bub() ) &&
        n->get_warmth_indoor_hold_turns() < 60 ) {
        return status_t::running;
    }
    return status_t::failure;
}

status_t character_oracle_t::needs_sleep_badly( std::string_view ) const
{
    // TIRED (191): low enough that off-shift NPCs go to bed early.
    // On-shift duty (0.45) easily beats sleep urgency at this level (0.191).
    if( subject->get_sleepiness() >= static_cast<int>( sleepiness_levels::TIRED ) ) {
        return status_t::running;
    }
    return status_t::success;
}

float character_oracle_t::thirst_urgency( std::string_view ) const
{
    // 0 = hydrated, 1 = dehydration death (threshold 1200, character_health.cpp).
    static constexpr float death_threshold = 1200.0f;
    return std::clamp( subject->get_thirst() / death_threshold, 0.0f, 1.0f );
}

float character_oracle_t::hunger_urgency( std::string_view ) const
{
    // 0 = healthy weight, 1 = starvation death (stored_kcal <= 0, character_health.cpp).
    const int healthy = subject->get_healthy_kcal();
    if( healthy <= 0 ) {
        return 0.0f;
    }
    const float kcal_frac = static_cast<float>( subject->get_stored_kcal() ) / healthy;
    return std::clamp( 1.0f - kcal_frac, 0.0f, 1.0f );
}

float character_oracle_t::warmth_urgency( std::string_view ) const
{
    // 0 = all bodyparts at BODYTEMP_NORM (37C),
    // 1 = coldest bodypart at BODYTEMP_FREEZING (28C).
    const float norm = units::to_kelvin( BODYTEMP_NORM );
    const float freeze = units::to_kelvin( BODYTEMP_FREEZING );
    const float range = norm - freeze;
    if( range <= 0.0f ) {
        return 0.0f;
    }
    float coldest = norm;
    for( const bodypart_id &bp : subject->get_all_body_parts() ) {
        const float temp = units::to_kelvin( subject->get_part_temp_conv( bp ) );
        coldest = std::min( coldest, temp );
    }
    return std::clamp( ( norm - coldest ) / range, 0.0f, 1.0f );
}

status_t character_oracle_t::can_sleep( std::string_view ) const
{
    // Meth is the only hard blocker in Character::can_sleep().
    // Stim, comfort, insomnia, and rng are soft score modifiers whose
    // net effect depends on location and luck -- the oracle can't
    // evaluate those without knowing where the NPC will sleep.
    if( subject->has_effect( effect_meth ) ) {
        return status_t::failure;
    }
    if( subject->get_sleepiness() >= static_cast<int>( sleepiness_levels::TIRED ) ) {
        return status_t::running;
    }
    return status_t::failure;
}

float character_oracle_t::sleepiness_urgency( std::string_view ) const
{
    // 0 = rested, 1 = forced unconsciousness (MASSIVE_SLEEPINESS = 1000).
    static constexpr float cap = 1000.0f;
    return std::clamp( subject->get_sleepiness() / cap, 0.0f, 1.0f );
}

// Top-level decision predicates. These need NPC-specific state (ai_cache,
// attitude) so they dynamic_cast from Character to npc.

status_t character_oracle_t::in_danger( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n ) {
        return status_t::failure;
    }
    return n->get_ai_danger() > 0 ? status_t::running : status_t::failure;
}

status_t character_oracle_t::should_flee( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n ) {
        return status_t::failure;
    }
    if( n->has_effect( effect_npc_run_away ) ) {
        return status_t::running;
    }
    if( n->get_attitude() == NPCATT_FLEE_TEMP ) {
        return status_t::running;
    }
    return status_t::failure;
}

status_t character_oracle_t::has_target( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n ) {
        return status_t::failure;
    }
    return n->get_ai_target().lock() ? status_t::running : status_t::failure;
}

status_t character_oracle_t::has_sound_alerts( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n ) {
        return status_t::failure;
    }
    // Mirror the live cascade gates in npc::move() sound investigation:
    // companions don't investigate, immobile NPCs can't investigate,
    // IGNORE_SOUND NPCs treat sounds as non-actionable.
    if( n->is_walking_with() || n->has_flag( json_flag_CANNOT_MOVE ) ||
        n->has_trait( trait_IGNORE_SOUND ) ) {
        return status_t::failure;
    }
    return n->has_ai_sound_alerts() ? status_t::running : status_t::failure;
}

status_t character_oracle_t::displaced_from_post( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n ) {
        return status_t::failure;
    }
    if( n->has_flag( json_flag_CANNOT_MOVE ) ) {
        return status_t::failure;
    }
    std::optional<tripoint_abs_ms> gp = n->get_guard_post();
    if( !gp ) {
        return status_t::failure;
    }
    return n->pos_abs() != *gp ? status_t::running : status_t::failure;
}

status_t character_oracle_t::on_shift( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n || !n->get_guard_post() || !n->myclass.is_valid() ) {
        return status_t::failure;
    }
    const auto &[start, end] = n->myclass.obj().get_work_hours();
    const int hour = to_hours<int>( time_past_midnight( calendar::turn ) );
    return is_within_work_hours( hour, start, end ) ? status_t::running : status_t::failure;
}

float character_oracle_t::duty_urgency( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n ) {
        return 0.0f;
    }
    std::optional<tripoint_abs_ms> gp = n->get_guard_post();
    if( !gp || !n->myclass.is_valid() ) {
        return 0.0f;
    }
    const auto &[start, end] = n->myclass.obj().get_work_hours();
    const int hour = to_hours<int>( time_past_midnight( calendar::turn ) );
    const bool on = is_within_work_hours( hour, start, end );

    if( n->pos_abs() == *gp ) {
        // At post: on-shift returns a baseline that resists moderate
        // tiredness. Off-shift returns 0 so needs (sleep) can win.
        return on ? 0.45f : 0.0f;
    }
    // Off-shift: no duty pull at all. Guard stays where they are
    // (bed, shelter, wherever) until their shift starts.
    if( !on ) {
        return 0.0f;
    }
    // Displaced on-shift: distance-based with a floor so the NPC
    // strongly prefers returning even when close to post.
    const int dist = rl_dist( n->pos_abs(), *gp );
    return std::max( 0.45f, std::min( 0.5f, dist * 0.05f ) );
}

status_t character_oracle_t::npc_is_following( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n || !n->should_follow_close() ) {
        return status_t::failure;
    }
    if( n->get_guard_post() ) {
        return status_t::failure;
    }
    const Character &player = get_player_character();
    const int dist = rl_dist( n->pos_abs(), player.pos_abs() );
    if( dist <= n->follow_distance() && n->posz() == player.posz() ) {
        return status_t::success;
    }
    return status_t::running;
}

float character_oracle_t::npc_following_urgency( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n || !n->should_follow_close() ) {
        return 0.0f;
    }
    const Character &player = get_player_character();
    if( n->posz() != player.posz() ) {
        return 0.6f;
    }
    const int dist = rl_dist( n->pos_abs(), player.pos_abs() );
    if( dist <= n->follow_distance() ) {
        return 0.0f;
    }
    return std::clamp( 0.3f + ( dist - n->follow_distance() ) * 0.015f, 0.3f, 0.6f );
}

status_t character_oracle_t::npc_has_goto_order( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n || !n->goto_to_this_pos || n->has_flag( json_flag_CANNOT_MOVE ) ) {
        return status_t::failure;
    }
    if( n->pos_abs() == *n->goto_to_this_pos ) {
        return status_t::success;
    }
    return status_t::running;
}

float character_oracle_t::npc_goto_order_urgency( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n || !n->goto_to_this_pos ) {
        return 0.0f;
    }
    if( n->pos_abs() == *n->goto_to_this_pos ) {
        return 0.0f;
    }
    // Player-directed order. Beats generic follow (capped at 0.6)
    // but loses to life-threatening needs (thirst at 0.75+).
    return 0.65f;
}

status_t character_oracle_t::has_camp_job( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n || !n->assigned_camp || n->mission != NPC_MISSION_CAMP_RESIDENT ) {
        return status_t::failure;
    }
    if( n->pos_abs_omt() != *n->assigned_camp ) {
        return status_t::failure;
    }
    if( !n->has_job() ) {
        return status_t::failure;
    }
    if( calendar::turn - n->last_job_scan < 10_minutes ) {
        return status_t::failure;
    }
    return status_t::running;
}

status_t character_oracle_t::is_away_from_camp( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n || !n->assigned_camp || n->mission != NPC_MISSION_CAMP_RESIDENT ) {
        return status_t::failure;
    }
    return n->pos_abs_omt() != *n->assigned_camp
           ? status_t::running : status_t::failure;
}

status_t character_oracle_t::is_camp_idle( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n || !n->assigned_camp || n->mission != NPC_MISSION_CAMP_RESIDENT ) {
        return status_t::failure;
    }
    if( n->get_attitude() == NPCATT_ACTIVITY ) {
        return status_t::failure;
    }
    if( n->pos_abs_omt() != *n->assigned_camp ) {
        return status_t::failure;
    }
    return status_t::running;
}

float character_oracle_t::camp_work_urgency( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n || !n->assigned_camp || n->mission != NPC_MISSION_CAMP_RESIDENT
        || n->pos_abs_omt() != *n->assigned_camp || !n->has_job() ) {
        return 0.0f;
    }
    return 0.4f;
}

float character_oracle_t::return_to_camp_urgency( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n || !n->assigned_camp || n->mission != NPC_MISSION_CAMP_RESIDENT
        || n->pos_abs_omt() == *n->assigned_camp ) {
        return 0.0f;
    }
    // Below follow max (0.6), above duty (0.45).
    return 0.5f;
}

float character_oracle_t::free_time_urgency( std::string_view ) const
{
    const npc *n = dynamic_cast<const npc *>( subject );
    if( !n || !n->assigned_camp || n->mission != NPC_MISSION_CAMP_RESIDENT
        || n->get_attitude() == NPCATT_ACTIVITY
        || n->pos_abs_omt() != *n->assigned_camp ) {
        return 0.0f;
    }
    return 0.35f;
}

} // namespace behavior
