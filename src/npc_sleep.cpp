#include "npc_sleep.h"

#include <algorithm>
#include <optional>
#include <set>
#include <string>

#include "calendar.h"
#include "character.h"
#include "coordinates.h"
#include "debug.h"
#include "messages.h"
#include "npc.h"
#include "npc_arbitration.h"
#include "point.h"
#include "rng.h"
#include "type_id.h"

static const efftype_id effect_lying_down( "lying_down" );
static const efftype_id effect_narcosis( "narcosis" );
static const efftype_id effect_npc_suspend( "npc_suspend" );
static const efftype_id effect_sleep( "sleep" );

static const json_character_flag json_flag_CANNOT_SLEEP( "CANNOT_SLEEP" );

namespace
{
// a settling NPC rolls to fall asleep this often
constexpr time_duration settle_attempt_interval = 3_minutes;
// and gives a spot up after this long
constexpr time_duration settle_timeout = 30_minutes;
// patience stops growing after this many attempts
constexpr int settle_patience_cap = 10;
// this many wakes inside restless_window stall the sleep
constexpr int restless_wake_limit = 3;
constexpr time_duration restless_window = 30_minutes;
// stalled sleep steps aside for other work this long
constexpr time_duration stall_duration = 30_minutes;
// a woken NPC that hasn't lain back down forgets the attempt after this
constexpr time_duration stale_wake_timeout = 1_minutes;
// a second noise wake holds sleep back this long, doubling with each further wake up to
// noise_hold_doublings times
constexpr time_duration noise_hold_base = 1_minutes;
constexpr int noise_hold_doublings = 4;
// heard this many turns in a row, a noise counts as steady
constexpr int steady_noise_turns = 2;
// woken by the player, it stays up this long unless it gets this much more tired
constexpr time_duration player_wake_grace = 1_hours;
constexpr int player_wake_grace_margin = 100;
} // namespace

void npc_sleep_session::begin_settling()
{
    phase = npc_sleep_phase::settling;
    settle_started = calendar::turn;
    next_attempt = calendar::turn + settle_attempt_interval;
}

void npc_sleep_session::note_wake()
{
    if( phase != npc_sleep_phase::settling && phase != npc_sleep_phase::asleep ) {
        return;
    }
    if( calendar::turn - wake_window_start > restless_window ) {
        wake_window_start = calendar::turn;
        wakes = 0;
    }
    wakes++;
    woken_at = calendar::turn;
    phase = npc_sleep_phase::woken;
    if( wakes >= restless_wake_limit ) {
        stall();
    }
}

void npc_sleep_session::stall()
{
    stalled_until = calendar::turn + stall_duration;
}

void npc_sleep_session::end()
{
    npc_sleep_session holds = *this;
    *this = npc_sleep_session();
    stalled_until = holds.stalled_until;
    stay_awake_until = holds.stay_awake_until;
    stay_awake_floor = holds.stay_awake_floor;
    noise_hold_until = holds.noise_hold_until;
    noise_hold_at = holds.noise_hold_at;
    noise_hold_deaf = holds.noise_hold_deaf;
    noise_wakes = holds.noise_wakes;
    last_noise_wake = holds.last_noise_wake;
}

bool npc_sleep_session::in_grace( const int sleepiness ) const
{
    return calendar::turn < stay_awake_until && sleepiness < stay_awake_floor;
}

bool npc_sleep_session::stalled() const
{
    return calendar::turn < stalled_until;
}

bool npc_sleep_session::wants_resume( const int sleepiness ) const
{
    return phase == npc_sleep_phase::woken &&
           sleepiness >= static_cast<int>( sleepiness_levels::TIRED ) / 2;
}

int npc_sleep_session::patience() const
{
    return std::min( settle_patience_cap,
                     static_cast<int>( ( calendar::turn - settle_started ) / settle_attempt_interval ) );
}

bool npc::try_fall_asleep_now()
{
    if( has_flag( json_flag_CANNOT_SLEEP ) ) {
        return false;
    }
    if( sleep_readiness() + sleep_session.patience() + rng( -8, 8 ) <= 0 ) {
        return false;
    }
    fall_asleep();
    sleep_session.phase = npc_sleep_phase::asleep;
    // a wake lies back down where the NPC is, never walks back to this tile
    ai_cache.sleep_plan.clear();
    return true;
}

npc::need_result npc::settle_here()
{
    need_plan &plan = ai_cache.sleep_plan;
    if( !plan.active() ) {
        plan.goal = "go_to_sleep";
        plan.source_kind = need_source::sleep_spot;
        plan.target = pos_abs();
    }
    sleep_session.begin_settling();
    if( try_fall_asleep_now() ) {
        return need_result::progressed;
    }
    add_effect( effect_lying_down, settle_timeout + 1_minutes, false, 1 );
    // adding the effect already asked rest_interruption(); this turn's review mustn't
    // reuse that answer
    ai_cache.rest_review = {};
    plan.last_result = need_result::holding;
    return need_result::holding;
}

bool npc::sleep_settle_step()
{
    npc_sleep_session &ses = sleep_session;
    if( has_flag( json_flag_CANNOT_SLEEP ) ) {
        // can't fall asleep at all now: stop lying here, the spot isn't to blame
        ai_cache.sleep_plan.clear();
        ses.phase = npc_sleep_phase::none;
        return true;
    }
    if( ses.phase != npc_sleep_phase::settling ) {
        // lying down from a save or a script: settle from now on
        ses.begin_settling();
        return false;
    }
    if( calendar::turn < ses.next_attempt ) {
        return false;
    }
    if( try_fall_asleep_now() ) {
        return true;
    }
    if( calendar::turn - ses.settle_started >= settle_timeout ) {
        // can't sleep here: the executor looks elsewhere
        ai_cache.sleep_failed_targets.insert( pos_abs() );
        ai_cache.sleep_plan.clear();
        ses.phase = npc_sleep_phase::none;
        return true;
    }
    ses.next_attempt = calendar::turn + settle_attempt_interval;
    return false;
}

void npc::reconcile_sleep_session()
{
    npc_sleep_session &ses = sleep_session;
    if( ( ses.phase == npc_sleep_phase::settling || ses.phase == npc_sleep_phase::asleep ) &&
        !in_sleep_state() ) {
        // sleep effects gone without wake_up(): debug menu, a script
        ses.note_wake();
    } else if( ses.phase == npc_sleep_phase::woken && get_committed_goal() != "go_to_sleep" &&
               !ai_cache.sleep_plan.active() && calendar::turn - ses.woken_at > stale_wake_timeout ) {
        // woken and no sleep goal or plan took it up: forget the attempt
        ses.end();
    }
}

bool npc::sleep_stalled() const
{
    return sleep_session.stalled();
}

void npc::wake_up()
{
    const bool was_resting = has_effect( effect_sleep ) || has_effect( effect_lying_down );
    Character::wake_up();
    // narcosis makes Character::wake_up() return early
    if( was_resting && !has_effect( effect_narcosis ) ) {
        sleep_session.note_wake();
    }
}

bool npc::wake_by_player()
{
    if( has_effect( effect_narcosis ) ) {
        return false;
    }
    wake_up();
    // outside effect processing, so the removal wake_up() defers can happen now
    remove_effect( effect_sleep );
    remove_effect( effect_npc_suspend );
    stop_settling();
    grant_stay_awake( player_wake_grace, get_sleepiness() + player_wake_grace_margin );
    return true;
}

void npc::grant_stay_awake( const time_duration &dur, const int sleepiness_floor )
{
    npc_sleep_session &ses = sleep_session;
    const bool running = calendar::turn < ses.stay_awake_until;
    ses.stay_awake_until = std::max( ses.stay_awake_until, calendar::turn + dur );
    ses.stay_awake_floor = running ? std::max( ses.stay_awake_floor, sleepiness_floor ) :
                           sleepiness_floor;
}

bool npc::sleep_on_hold() const
{
    return rest_cooldown_active() || sleep_session.in_grace( get_sleepiness() ) ||
           noise_hold_active();
}

void npc::note_noise( const bool woke )
{
    npc_short_term_cache::noise_memo &heard = ai_cache.noise_heard;
    if( heard.last != calendar::turn ) {
        heard.streak = heard.last == calendar::turn - 1_turns ? heard.streak + 1 : 1;
        heard.last = calendar::turn;
    }
    if( !woke ) {
        return;
    }
    npc_sleep_session &ses = sleep_session;
    if( calendar::turn - ses.last_noise_wake > restless_window ) {
        ses.noise_wakes = 0;
    }
    ses.noise_wakes++;
    ses.last_noise_wake = calendar::turn;
    if( ses.noise_wakes < 2 ) {
        return;
    }
    const int doublings = std::min( ses.noise_wakes - 2, noise_hold_doublings );
    ses.noise_hold_until = calendar::turn + noise_hold_base * ( 1 << doublings );
    ses.noise_hold_at = pos_abs();
    ses.noise_hold_deaf = is_deaf();
    add_msg_debug( debugmode::DF_NPC_NEEDS, "NPC %s: woken by noise %d times, waits %s to rest",
                   get_name(), ses.noise_wakes, to_string_writable( ses.noise_hold_until - calendar::turn ) );
}

bool npc::noise_hold_active() const
{
    const npc_sleep_session &ses = sleep_session;
    const npc_short_term_cache::noise_memo &heard = ai_cache.noise_heard;
    // heard on steady_noise_turns turns in a row, the latest this turn or the last
    const bool steady = heard.streak >= steady_noise_turns &&
                        calendar::turn - heard.last <= 1_turns;
    return steady && calendar::turn < ses.noise_hold_until && pos_abs() == ses.noise_hold_at &&
           is_deaf() == ses.noise_hold_deaf;
}
