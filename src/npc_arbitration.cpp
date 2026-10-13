#include "npc_arbitration.h"

#include <algorithm>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "behavior.h"
#include "calendar.h"
#include "character.h"
#include "character_oracle.h"
#include "debug.h"
#include "coordinates.h"
#include "map.h"
#include "messages.h"
#include "npc.h"
#include "npc_class.h"
#include "npc_decision_category.h"
#include "npc_trip.h"
#include "translations.h"
#include "type_id.h"
#include "vehicle.h"
#include "vpart_position.h"

class Creature;

static const activity_id ACT_CRAFT( "ACT_CRAFT" );
static const activity_id ACT_CRAFT_WAIT( "ACT_CRAFT_WAIT" );

static const efftype_id effect_lying_down( "lying_down" );
static const efftype_id effect_npc_suspend( "npc_suspend" );
static const efftype_id effect_sleep( "sleep" );

static const string_id<behavior::node_t> behavior_node_t_npc_needs( "npc_needs" );

namespace
{
// a guard can walk this far from the post for something quick
constexpr int post_reach = 4;
constexpr float preempt_margin = 0.15f;
// how far someone lying down notices trouble walking up
constexpr int rest_watch_radius = 6;
// failed sleep search holds sleep back this long unless what it was judged on changes
constexpr time_duration rest_cooldown_duration = 1_minutes;
// NPC may drift this far from where search failed without ending cooldown
constexpr int rest_cooldown_drift = 2;
// a leader staying in one place this long starts to count as settling in
constexpr time_duration leader_settle_delay = 2_minutes;
// and this long as settled
constexpr time_duration leader_settled_after = 15_minutes;

struct need_levels {
    // where the need's own predicate fires; a discouraged floor relaxes down to it
    float act;
    float insist;
    float emergency;
};

enum class post_duty : int { none, guard, shift };
} // namespace

const char *permission_name( const npc_permission p )
{
    switch( p ) {
        case npc_permission::allowed:
            return "allowed";
        case npc_permission::discouraged:
            return "discouraged";
        case npc_permission::emergency_only:
            return "emergency_only";
        case npc_permission::blocked:
            return "blocked";
    }
    return "allowed";
}

const char *obligation_name( const npc_obligation o )
{
    switch( o ) {
        case npc_obligation::none:
            return "none";
        case npc_obligation::accompany:
            return "accompany";
        case npc_obligation::post:
            return "post";
        case npc_obligation::goto_order:
            return "goto_order";
        case npc_obligation::stay_awake_rule:
            return "stay_awake_rule";
    }
    return "none";
}

// raw urgency at which a need overrides a discouraged or emergency-only option
static need_levels levels_for( const npc::need_goal_id id )
{
    switch( id ) {
        case npc::need_goal_id::drink_water:
            // 520 (thirsty predicate) and 800 on the 1200 dehydration scale, emergency at critical (900)
            return { 520.0f / 1200.0f, 800.0f / 1200.0f, need_critical_urgency };
        case npc::need_goal_id::go_to_sleep:
            return { static_cast<float>( sleepiness_levels::TIRED ) / 1000.0f,
                     static_cast<float>( sleepiness_levels::DEAD_TIRED ) / 1000.0f,
                     static_cast<float>( sleepiness_levels::EXHAUSTED ) / 1000.0f };
        case npc::need_goal_id::eat_food:
        case npc::need_goal_id::seek_warmth:
            break;
    }
    // no option costs declared: overrides wait for the critical level
    return { need_critical_urgency, need_critical_urgency, need_critical_urgency };
}

float npc_need_floor( const npc::need_goal_id id, const npc_permission_verdict &v )
{
    const need_levels levels = levels_for( id );
    switch( v.permission ) {
        case npc_permission::allowed:
            return 0.0f;
        case npc_permission::discouraged:
            // relaxes from insist toward the need's own level as ease grows
            return levels.act + ( levels.insist - levels.act ) * ( 1.0f - v.ease );
        case npc_permission::emergency_only:
            return levels.emergency;
        case npc_permission::blocked:
            break;
    }
    return std::numeric_limits<float>::infinity();
}

// 0 while the leader travels, rising to 1 as it stays in one place
static float leader_stillness_ease()
{
    const time_duration still = get_avatar().time_since_relocated();
    if( still <= leader_settle_delay ) {
        return 0.0f;
    }
    return std::min( 1.0f, to_turns<float>( still - leader_settle_delay ) /
                     to_turns<float>( leader_settled_after - leader_settle_delay ) );
}

static bool npc_vehicle_bed_at( const tripoint_abs_ms &p, const vehicle &veh )
{
    map &here = get_map();
    const optional_vpart_position vp = here.veh_at( here.get_bub( p ) );
    return vp && &vp->vehicle() == &veh &&
           ( vp->part_with_feature( "SEATBELT", true ) || vp->part_with_feature( "BED", true ) );
}

// can_use_floor_warmth() misses crafting
static bool leader_is_resting( const Character &leader )
{
    return leader.can_use_floor_warmth() || leader.has_activity( ACT_CRAFT ) ||
           leader.has_activity( ACT_CRAFT_WAIT );
}

// ignores follow_distance()'s shrink (leader out of sight, stairs) so the reach
// doesn't flicker and a pursuit inside it finishes before the follower closes in
static int follow_spacing( const npc &guy )
{
    if( !guy.rules.has_flag( ally_rule::follow_close ) ) {
        return 6;
    }
    return guy.rules.has_flag( ally_rule::follow_distance_2 ) ? 2 : 4;
}

static post_duty post_duty_of( const npc &guy )
{
    if( !guy.is_on_shift() ) {
        return post_duty::none;
    }
    const std::pair<int, int> &hours = guy.myclass.obj().get_work_hours();
    if( hours.first != 0 || hours.second != 24 ) {
        return post_duty::shift;
    }
    return guy.is_guarding() && !guy.is_patrolling() ? post_duty::guard : post_duty::none;
}

bool npc::is_on_shift() const
{
    if( !guard_pos || !myclass.is_valid() || is_walking_with() ) {
        return false;
    }
    const std::pair<int, int> &hours = myclass.obj().get_work_hours();
    return is_within_work_hours( to_hours<int>( time_past_midnight( calendar::turn ) ),
                                 hours.first, hours.second );
}

std::optional<npc_reach> npc::obligation_reach() const
{
    if( is_walking_with() ) {
        const Character &leader = get_player_character();
        npc_reach reach{ leader.pos_abs(), follow_spacing( *this ), nullptr };
        if( leader.in_vehicle ) {
            // vehicle is the reach when parked or when the NPC rides it too; outside a
            // moving one the radius stays: it can't follow until the vehicle stops
            map &here = get_map();
            const optional_vpart_position vp = here.veh_at( leader.pos_bub() );
            if( vp ) {
                const optional_vpart_position mine = here.veh_at( pos_bub() );
                const bool aboard_same = in_vehicle && mine && &mine->vehicle() == &vp->vehicle();
                if( vp->vehicle().velocity == 0 || aboard_same ) {
                    reach.aboard = &vp->vehicle();
                }
            }
        }
        return reach;
    }
    if( post_duty_of( *this ) != post_duty::none ) {
        return npc_reach{ *guard_pos, post_reach, nullptr };
    }
    return std::nullopt;
}

bool npc::goal_is_idle( const std::string &goal ) const
{
    // once aboard, moving to a better seat isn't work worth giving up sleep for
    return goal == "idle" || goal == "free_time" || ( goal == "follow_embarked" && in_vehicle );
}

bool npc::in_vehicle_bed() const
{
    if( !in_vehicle ) {
        return false;
    }
    const optional_vpart_position theirs = get_map().veh_at( get_player_character().pos_bub() );
    return theirs && npc_vehicle_bed_at( pos_abs(), theirs->vehicle() );
}

npc_permission_verdict npc::judge_option( const npc_option_cost &cost ) const
{
    npc_permission_verdict verdict;
    const auto consider = [&verdict]( const npc_permission_verdict & v ) {
        // stricter: higher permission, or same one relaxed less; ease only moves a
        // discouraged floor between act and insist, so this keeps the largest floor
        if( v.permission > verdict.permission ||
            ( v.permission == verdict.permission && v.ease < verdict.ease ) ) {
            verdict = v;
        }
    };
    const Character &leader = get_player_character();
    const std::optional<npc_reach> reach = obligation_reach();
    const bool holds_up = cost.needs_walk || cost.occupies;
    const bool beyond = cost.where_known && reach &&
                        ( !reach->contains( cost.where ) || !cost.route_in_reach );
    if( is_walking_with() ) {
        npc_permission_verdict v{ npc_permission::allowed, npc_obligation::accompany, 0.0f };
        if( !cost.responsive ) {
            // strapped in or bedded down aboard the group's vehicle is riding along, not
            // dropping out; the best case assumes such a spot exists
            const bool on_a_bed = reach && reach->aboard != nullptr && in_vehicle &&
                                  ( !cost.where_known || npc_vehicle_bed_at( cost.where, *reach->aboard ) );
            if( beyond ) {
                v.permission = npc_permission::emergency_only;
            } else if( !leader_is_resting( leader ) && !on_a_bed ) {
                const float ease = leader_stillness_ease();
                if( ease < 1.0f ) {
                    v.permission = npc_permission::discouraged;
                    v.ease = ease;
                }
            }
        } else if( holds_up && beyond ) {
            v.permission = npc_permission::discouraged;
        }
        consider( v );
    }
    const post_duty duty = post_duty_of( *this );
    if( duty != post_duty::none ) {
        npc_permission p = npc_permission::allowed;
        if( !cost.responsive ) {
            // shift workers sleep off shift; a round-the-clock guard only when exhausted
            p = duty == post_duty::shift ? npc_permission::blocked : npc_permission::emergency_only;
        } else if( holds_up && beyond ) {
            p = npc_permission::emergency_only;
        }
        consider( { p, npc_obligation::post, 0.0f } );
    }
    if( goto_to_this_pos && pos_abs() != *goto_to_this_pos && ( !cost.responsive || holds_up ) ) {
        consider( { npc_permission::emergency_only, npc_obligation::goto_order, 0.0f } );
    }
    if( !cost.responsive && is_player_ally() && !rules.has_flag( ally_rule::allow_sleep ) ) {
        // "Stay awake as long as possible": dead tired once the leader sleeps, else exhausted.
        consider( { leader.in_sleep_state() ? npc_permission::discouraged :
                    npc_permission::emergency_only, npc_obligation::stay_awake_rule, 0.0f } );
    }
    return verdict;
}

npc_need_view npc::need_view( const need_goal_id id, const npc_option_cost &cost ) const
{
    npc_need_view view;
    view.kind = cost.where_known ? npc_option_kind::actual : npc_option_kind::best_case;
    view.raw = current_need_urgency( id );
    view.verdict = judge_option( cost );
    view.floor = npc_need_floor( id, view.verdict );
    view.strict_floor = npc_need_floor( id, { view.verdict.permission, view.verdict.by, 0.0f } );
    view.selection = view.raw >= view.floor ? view.raw : 0.0f;
    return view;
}

npc_need_view npc::need_view( const need_goal_id id,
                              const std::optional<npc_option_cost> &cost ) const
{
    if( !cost ) {
        npc_need_view none;
        none.raw = current_need_urgency( id );
        return none;
    }
    return need_view( id, *cost );
}

bool should_interrupt( const npc_pursuit &current, const npc_challenger &challenger )
{
    if( current.cost == npc_interrupt_cost::wake_only ) {
        return false;
    }
    // obligation changed (leader left, order given, shift began): pursuit ends
    if( !current.permitted ) {
        return true;
    }
    if( current.yields && !challenger.idle && challenger.goal != current.goal ) {
        return true;
    }
    const decision_category cur_cat = bt_goal_to_category( current.goal );
    const decision_category new_cat = bt_goal_to_category( challenger.goal );
    if( new_cat != cur_cat ) {
        return new_cat < cur_cat;
    }
    if( new_cat != decision_category::needs || challenger.goal == current.goal ) {
        return false;
    }
    // critical applies at every cost: a cheaper pursuit is never harder to leave
    const bool critical = challenger.selection > need_critical_urgency &&
                          challenger.selection > current.selection;
    switch( current.cost ) {
        case npc_interrupt_cost::cheap:
            return challenger.selection > current.selection + preempt_margin || critical;
        case npc_interrupt_cost::stalled:
            return challenger.selection > current.selection + preempt_margin * 2 || critical;
        case npc_interrupt_cost::costly:
            return challenger.selection > current.selection + preempt_margin * 3 || critical;
        case npc_interrupt_cost::wake_only:
            break;
    }
    return false;
}

void npc::record_selected_option( const need_goal_id id, const npc_option_cost &cost )
{
    npc_intention &in = ai_cache.intention;
    if( goal_id_for( in.goal ) != id ) {
        // not a commitment (an address_needs() pick): nothing to record
        return;
    }
    const npc_need_view view = need_view( id, cost );
    in.verdict = view.verdict;
    if( view.overrides() && view.verdict.permission > in.explained ) {
        add_msg_debug( debugmode::DF_NPC_NEEDS,
                       "NPC %s: %s overrides a %s verdict from %s, raw %.2f, floor %.2f",
                       get_name(), in.goal, permission_name( view.verdict.permission ),
                       obligation_name( view.verdict.by ), view.raw, view.floor );
        in.explained = view.verdict.permission;
        explain_override( id );
    }
}

void npc::adopt_goal( const std::string &goal )
{
    if( goal.empty() || goal == "idle" ) {
        ai_cache.intention = {};
        return;
    }
    if( ai_cache.intention.goal == goal ) {
        return;
    }
    ai_cache.intention = { goal, calendar::turn, {}, npc_permission::allowed };
    add_msg_debug( debugmode::DF_NPC_NEEDS, "NPC %s: adopts %s", get_name(), goal );
}

void npc::explain_override( const need_goal_id id )
{
    std::string issue;
    std::string speech;
    switch( id ) {
        case need_goal_id::drink_water:
            issue = "need_override_water";
            speech = _( "I really need some water.  I'll be right back." );
            break;
        case need_goal_id::go_to_sleep:
            issue = "need_override_sleep";
            speech = _( "I can't stay awake any longer." );
            break;
        case need_goal_id::eat_food:
        case need_goal_id::seek_warmth:
            return;
    }
    // said even with complaints turned off, unless it would wake the player
    complain_about( issue, 30_minutes, speech, !get_player_character().in_sleep_state() );
}

npc_need_view npc::best_need_view( const need_goal_id id )
{
    const npc_option_cost in_place = npc_option_cost::in_place( pos_abs() );
    if( id == need_goal_id::drink_water ) {
        behavior::character_oracle_t oracle( this );
        if( oracle.has_water( "" ) == behavior::status_t::running ) {
            return need_view( id, in_place );
        }
        npc_need_view held;
        held.raw = current_need_urgency( id );
        const std::set<tripoint_abs_ms> &failed = failed_targets_for( id );
        for( const need_candidate &c : find_water_candidates() ) {
            if( failed.count( c.target ) != 0 ) {
                continue;
            }
            const npc_need_view v = need_view( id, water_option_cost( c ) );
            if( v.eligible() ) {
                return v;
            }
            if( held.kind == npc_option_kind::none ) {
                held = v;
            }
        }
        return held;
    }
    if( id == need_goal_id::go_to_sleep ) {
        if( sleep_on_hold() ) {
            npc_need_view none;
            none.raw = current_need_urgency( id );
            return none;
        }
        // the executor judges the spot it finds
        return need_view( id, rest_option_best_case() );
    }
    return need_view( id, in_place );
}

void npc::arm_rest_cooldown()
{
    const Character &leader = get_player_character();
    const float raw = current_need_urgency( need_goal_id::go_to_sleep );
    const need_levels levels = levels_for( need_goal_id::go_to_sleep );
    npc_short_term_cache::rest_cooldown &cd = ai_cache.no_rest_option;
    cd.until = calendar::turn + rest_cooldown_duration;
    cd.leader_asleep = leader.in_sleep_state();
    cd.leader_resting = leader_is_resting( leader );
    cd.reach = npc_reach_snapshot::of( obligation_reach() );
    cd.goto_target = goto_to_this_pos;
    cd.pos = pos_abs();
    // next level where a stricter verdict gives way; none above emergency
    cd.next_floor = raw < levels.insist ? levels.insist : raw < levels.emergency ?
                    levels.emergency : std::numeric_limits<float>::infinity();
}

bool npc::rest_cooldown_active() const
{
    const npc_short_term_cache::rest_cooldown &cd = ai_cache.no_rest_option;
    if( calendar::turn >= cd.until ) {
        return false;
    }
    const Character &leader = get_player_character();
    return leader.in_sleep_state() == cd.leader_asleep &&
           leader_is_resting( leader ) == cd.leader_resting &&
           cd.reach.matches( obligation_reach() ) &&
           goto_to_this_pos == cd.goto_target &&
           rl_dist( pos_abs(), cd.pos ) <= rest_cooldown_drift &&
           current_need_urgency( need_goal_id::go_to_sleep ) < cd.next_floor;
}

bool npc::may_walk_to( const npc_trip_spec &spec ) const
{
    const std::optional<npc_option_cost> cost = trip_cost( spec, true );
    return cost && judge_option( *cost ).permission == npc_permission::allowed;
}

npc_pursuit npc::committed_pursuit() const
{
    npc_pursuit current;
    current.goal = ai_cache.intention.goal;
    if( const std::optional<need_goal_id> gid = goal_id_for( current.goal ) ) {
        current.cost = interrupt_cost( *gid );
        const npc_need_view view = need_view( *gid, pursued_option_cost( *gid ) );
        current.selection = view.selection;
        current.permitted = view.may_consider();
        current.yields = *gid == need_goal_id::go_to_sleep && sleep_stalled();
    }
    return current;
}

std::optional<time_point> npc::last_complaint( const std::string &issue ) const
{
    const auto it = complaints.find( issue );
    if( it == complaints.end() ) {
        return std::nullopt;
    }
    return it->second;
}

bool npc::complaint_due( const std::string &issue, const time_duration &dur ) const
{
    const std::optional<time_point> last = last_complaint( issue );
    return !last || *last < calendar::turn - dur;
}

bool npc::sees_hostile_within( const int radius ) const
{
    for( const Creature *critter : get_visible_creatures( radius ) ) {
        if( attitude_to( *critter ) == Attitude::HOSTILE ) {
            return true;
        }
    }
    return false;
}

std::optional<npc_challenger> npc::rest_interruption() const
{
    if( is_dead() || !has_effect( effect_lying_down ) || has_effect( effect_sleep ) ||
        has_effect( effect_npc_suspend ) ) {
        return std::nullopt;
    }
    npc_short_term_cache::rest_review_memo &memo = ai_cache.rest_review;
    if( memo.turn == calendar::turn ) {
        return memo.interruption;
    }
    memo.turn = calendar::turn;
    memo.interruption = std::nullopt;

    npc_pursuit current;
    current.goal = "go_to_sleep";
    current.cost = interrupt_cost( need_goal_id::go_to_sleep );
    const npc_need_view rest = need_view( need_goal_id::go_to_sleep, rest_option_cost( pos_abs() ) );
    current.selection = rest.selection;
    current.permitted = rest.eligible();

    std::vector<npc_challenger> challengers;
    if( sees_hostile_within( rest_watch_radius ) ) {
        challengers.push_back( { "fight", 1.0f } );
    }
    behavior::character_oracle_t oracle( this );
    behavior::tree needs;
    needs.add( &behavior_node_t_npc_needs.obj() );
    const std::pair<std::string, float> picked = needs.tick_full( &oracle );
    if( picked.first != "idle" ) {
        challengers.push_back( { picked.first, picked.second } );
    }
    // wins only once resting here is no longer allowed
    challengers.push_back( { "idle", 0.0f, true } );
    for( const npc_challenger &c : challengers ) {
        if( should_interrupt( current, c ) ) {
            add_msg_debug( debugmode::DF_NPC_NEEDS, "NPC %s: gets up for %s, rest %s by %s",
                           get_name(), c.goal, permission_name( rest.verdict.permission ),
                           obligation_name( rest.verdict.by ) );
            memo.interruption = c;
            break;
        }
    }
    return memo.interruption;
}

void npc::review_rest()
{
    if( rest_interruption() ) {
        stop_settling();
        return;
    }
    // still settling, maybe now as an override: say so like a walk would; move() is
    // skipped while lying down, so no executor records it
    if( !is_dead() && has_effect( effect_lying_down ) && !has_effect( effect_sleep ) &&
        !has_effect( effect_npc_suspend ) ) {
        if( const std::optional<npc_option_cost> rest = rest_option_cost( pos_abs() ) ) {
            record_selected_option( need_goal_id::go_to_sleep, *rest );
        }
    }
}

void npc::review_self_care_activity()
{
    if( const std::optional<need_goal_id> gid = goal_id_for( ai_cache.intention.goal ) ) {
        const std::optional<npc_option_cost> cost = pursued_option_cost( *gid );
        const npc_need_view view = need_view( *gid, cost );
        if( !view.may_consider() ) {
            clear_committed_goal();
        } else if( cost ) {
            // may now be an override: announce it as a walk would
            record_selected_option( *gid, *cost );
        }
        return;
    }
    // no pursuit on record (address_needs() or a load): runs only while allowed outright
    // TODO: the pursuit isn't saved, so a load cancels an emergency harvest that
    // overrode an obligation; the activity would need to keep its need
    if( !may_occupy_here() ) {
        cancel_activity();
    }
}

bool npc::may_occupy_here() const
{
    npc_option_cost here = npc_option_cost::in_place( pos_abs() );
    here.occupies = true;
    return judge_option( here ).permission == npc_permission::allowed;
}
