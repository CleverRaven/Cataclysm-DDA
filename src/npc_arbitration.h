#pragma once
#ifndef CATA_SRC_NPC_ARBITRATION_H
#define CATA_SRC_NPC_ARBITRATION_H

#include <limits>
#include <string>

#include "calendar.h"
#include "coordinates.h"
#include "point.h"

// what keeps the NPC from answering an obligation once at the option's spot
enum class npc_option_hold : int {
    // usable from there, adjacent tiles too, left at once
    none,
    // harvest, forage, warming there: stays put for a while, counts like a walk
    occupy
};

// what judging the option's spot rests on
enum class npc_option_basis : int {
    // no position yet: spot assumed inside reach, never a concrete option
    assumed,
    // NPC's own tile
    here,
    // destination alone: no reach, so no route searched, or the route search failed; a
    // trip judged this way may have no way there
    destination,
    // route found and judged against reach
    routed
};

// what an option asks of the NPC's availability, judged against its obligations
struct npc_option_cost {
    // where NPC stands while doing it
    tripoint_abs_ms where;
    npc_option_hold hold = npc_option_hold::none;
    // walks a route first
    bool walks = false;
    // false when the option leaves the NPC unable to react (asleep)
    bool responsive = true;
    // the route to be walked keeps to the reach policy
    bool route_in_reach = true;
    npc_option_basis basis = npc_option_basis::here;

    bool holds_up() const {
        return walks || hold != npc_option_hold::none;
    }
    bool where_known() const {
        return basis != npc_option_basis::assumed;
    }

    static npc_option_cost in_place( const tripoint_abs_ms &where ) {
        return located( where, npc_option_basis::here );
    }
    // activity on the NPC's own tile, keeps it there until done or dropped
    static npc_option_cost stay( const tripoint_abs_ms &where ) {
        npc_option_cost cost = in_place( where );
        cost.hold = npc_option_hold::occupy;
        return cost;
    }
    // walk with no route found, judged as one out of reach
    static npc_option_cost unrouted( const tripoint_abs_ms &where ) {
        npc_option_cost cost = located( where, npc_option_basis::destination );
        cost.walks = true;
        cost.route_in_reach = false;
        return cost;
    }
    // concrete option at a known spot; every other field keeps its default
    static npc_option_cost located( const tripoint_abs_ms &where, const npc_option_basis basis ) {
        npc_option_cost cost;
        cost.where = where;
        cost.basis = basis;
        return cost;
    }
    static npc_option_cost best_case( const tripoint_abs_ms &spot, const bool responsive ) {
        npc_option_cost cost = located( spot, npc_option_basis::assumed );
        cost.responsive = responsive;
        return cost;
    }
};

// how strongly an obligation objects to an option, most permissive first
enum class npc_permission : int {
    // act at the need's own threshold
    allowed,
    // act when the need reaches insist
    discouraged,
    // act when the need reaches emergency
    emergency_only,
    // never, whatever the need
    blocked
};

// obligation behind a verdict
enum class npc_obligation : int {
    none,
    // walking with the player: stay within the follow spacing
    accompany,
    // guard / shift worker on shift: stay near the post
    post,
    // told to move, not there yet
    goto_order,
    // follower rule "sleep when tired" is off
    stay_awake_rule
};

struct npc_permission_verdict {
    npc_permission permission = npc_permission::allowed;
    npc_obligation by = npc_obligation::none;
    // how far a discouraged floor has relaxed toward the need's own level, 0 to 1; only the
    // accompany obligation sets it
    float ease = 0.0f;
};

enum class npc_option_kind : int {
    none,
    best_case,
    // a concrete option an executor can pursue
    actual
};

struct npc_need_view {
    // oracle urgency 0..1, before any obligation applies
    float raw = 0.0f;
    // raw level needed before the NPC acts on this option
    float floor = std::numeric_limits<float>::infinity();
    // raw once it reaches floor, else 0
    float selection = 0.0f;
    // the floor before any relaxation
    float strict_floor = std::numeric_limits<float>::infinity();
    npc_permission_verdict verdict;
    npc_option_kind kind = npc_option_kind::none;

    // acting past an obligation, not past one that relaxed
    bool overrides() const {
        return verdict.permission > npc_permission::allowed && raw >= strict_floor;
    }

    // may this goal be considered at all
    bool may_consider() const {
        return kind != npc_option_kind::none && raw >= floor;
    }
    // may this concrete option be pursued now
    bool eligible() const {
        return kind == npc_option_kind::actual && raw >= floor;
    }
};

// committed goal and its verdict; plans stay with the executors, and
// address_needs() acts without recording here
struct npc_intention {
    std::string goal;
    time_point since = calendar::turn_zero;
    // verdict of the option pursued now; not allowed while a need overrides an obligation
    npc_permission_verdict verdict;
    // strictest override already explained during this pursuit
    npc_permission explained = npc_permission::allowed;
};

// how an executor's current state may be interrupted
enum class npc_interrupt_cost : int {
    // holding position
    cheap,
    // blocked or deferred
    stalled,
    // making progress
    costly,
    // asleep: only the body's own wakers (noise, pain, light, schedule)
    wake_only
};

struct npc_pursuit {
    std::string goal;
    npc_interrupt_cost cost = npc_interrupt_cost::cheap;
    float selection = 0.0f;
    // obligations still allow it at this need level
    bool permitted = true;
    // stalled: any work on offer takes over, doing nothing doesn't
    bool yields = false;
};

struct npc_challenger {
    std::string goal;
    float selection = 0.0f;
    // picking it means doing nothing
    bool idle = false;
};

// need urgency above which a need preempts another whatever the margin; also thirst's
// emergency level
constexpr float need_critical_urgency = 0.75f;
// how far an NPC that's resting or drinking notices trouble walking up
constexpr int rest_watch_radius = 6;

bool should_interrupt( const npc_pursuit &current, const npc_challenger &challenger );

const char *permission_name( npc_permission p );
const char *obligation_name( npc_obligation o );

#endif // CATA_SRC_NPC_ARBITRATION_H
