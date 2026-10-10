#pragma once
#ifndef CATA_SRC_NPC_ARBITRATION_H
#define CATA_SRC_NPC_ARBITRATION_H

#include <limits>
#include <string>

#include "coordinates.h"
#include "point.h"

class vehicle;

// what an option asks of the NPC's availability, judged against its obligations
struct npc_option_cost {
    // where NPC stands while doing it
    tripoint_abs_ms where;
    // false: usable from here, adjacent tiles included
    bool needs_walk = false;
    // False when the option leaves the NPC unable to react (asleep)
    bool responsive = true;
    // the route to be walked keeps to the reach policy
    bool route_in_reach = true;
    // false only for best_case()
    bool where_known = true;
    // harvest, forage: holds the NPC in place, counts like a walk; stays last for the factories
    bool occupies = false;

    static npc_option_cost in_place( const tripoint_abs_ms &where ) {
        return { where, false, true, true, true };
    }
    static npc_option_cost trip( const tripoint_abs_ms &where, bool needs_walk, bool responsive,
                                 bool route_in_reach ) {
        return { where, needs_walk, responsive, route_in_reach, true };
    }
    // no position yet: judged as a spot inside the reach, never a concrete option
    static npc_option_cost best_case( const tripoint_abs_ms &near, bool responsive ) {
        return { near, false, responsive, true, false };
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
};

// Area an anchored obligation keeps the NPC in: a radius around a center, or the
// vehicle the leader sits in. Recomputed on every call, never stored.
struct npc_reach {
    tripoint_abs_ms center;
    int radius = 0;
    const vehicle *aboard = nullptr;
    bool contains( const tripoint_abs_ms &p ) const;
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
    npc_permission_verdict verdict;
    npc_option_kind kind = npc_option_kind::none;

    // may this goal be considered at all
    bool may_consider() const {
        return kind != npc_option_kind::none && raw >= floor;
    }
    // may this concrete option be pursued now
    bool eligible() const {
        return kind == npc_option_kind::actual && raw >= floor;
    }
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
};

struct npc_challenger {
    std::string goal;
    float selection = 0.0f;
};

// need urgency above which a need preempts another whatever the margin; also thirst's
// emergency level
constexpr float need_critical_urgency = 0.75f;

bool should_interrupt( const npc_pursuit &current, const npc_challenger &challenger );

const char *permission_name( npc_permission p );
const char *obligation_name( npc_obligation o );

#endif // CATA_SRC_NPC_ARBITRATION_H
