#pragma once
#ifndef CATA_SRC_NPC_TRIP_H
#define CATA_SRC_NPC_TRIP_H

#include <optional>
#include <vector>

#include "calendar.h"
#include "coordinates.h"
#include "point.h"
#include "safe_reference.h"

class vehicle;

// area an obligation keeps the NPC in: a radius, or a vehicle when aboard is set.
// raw vehicle pointer: recompute, never store
struct npc_reach {
    tripoint_abs_ms center;
    int radius = 0;
    vehicle *aboard = nullptr;
    bool contains( const tripoint_abs_ms &p ) const;
};

// safe_reference keeps a destroyed vehicle from matching
struct npc_reach_snapshot {
    bool anchored = false;
    tripoint_abs_ms center;
    int radius = 0;
    bool has_vehicle = false;
    safe_reference<vehicle> aboard;

    static npc_reach_snapshot of( const std::optional<npc_reach> &reach );
    bool matches( const std::optional<npc_reach> &reach ) const;
};

// tiles a constrained walk may use: the reach, plus same-z tiles within
// approach_bound of the center for a walk started outside
struct npc_reach_policy {
    npc_reach reach;
    // -1 when started inside the reach
    int approach_bound = -1;
    bool allows( const tripoint_abs_ms &p ) const;
};

// how a self-care walk ends
enum class npc_trip_kind : int {
    // on the source itself
    stand_on,
    // in range of the source, on its z level
    within_range,
    // on nearest_passable( source )
    legacy_nearest_passable
};

struct npc_trip_spec {
    tripoint_bub_ms source;
    npc_trip_kind kind = npc_trip_kind::stand_on;
    int range = 0;
    bool no_bashing = true;

    bool operator==( const npc_trip_spec &o ) const {
        return source == o.source && kind == o.kind && range == o.range &&
               no_bashing == o.no_bashing;
    }
    static npc_trip_spec stand_on( const tripoint_bub_ms &spot, bool no_bashing ) {
        return { spot, npc_trip_kind::stand_on, 0, no_bashing };
    }
    static npc_trip_spec within_range( const tripoint_bub_ms &source, int range,
                                       bool no_bashing ) {
        return { source, npc_trip_kind::within_range, range, no_bashing };
    }
    static npc_trip_spec legacy_nearest_passable( const tripoint_bub_ms &source, bool no_bashing ) {
        return { source, npc_trip_kind::legacy_nearest_passable, 0, no_bashing };
    }
};

struct npc_trip {
    time_point turn = calendar::turn_zero;
    tripoint_bub_ms from;
    npc_trip_spec spec;
    bool feasible = false;
    // path keeps to the reach policy; true without a reach
    bool route_in_reach = true;
    // resolved under a reach policy; part of the kept path's identity
    bool constrained = false;
    int approach_bound = -1;
    // where the NPC stands to act; its own tile when it can act already
    tripoint_bub_ms dest;
    // steps after current tile, ending at dest; empty when it can act already
    std::vector<tripoint_bub_ms> path;
};

// which trip npc::path was resolved for, so the next turn reuses it instead of routing
struct npc_retained_trip {
    bool valid = false;
    npc_trip_spec spec;
    bool constrained = false;
    int approach_bound = -1;
    npc_reach_snapshot reach;
};

// p if passable, else its passable neighbor closest to closest_to
std::optional<tripoint_bub_ms> nearest_passable( const tripoint_bub_ms &p,
        const tripoint_bub_ms &closest_to );

#endif // CATA_SRC_NPC_TRIP_H
