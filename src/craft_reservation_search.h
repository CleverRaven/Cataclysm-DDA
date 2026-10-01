#pragma once
#ifndef CATA_SRC_CRAFT_RESERVATION_SEARCH_H
#define CATA_SRC_CRAFT_RESERVATION_SEARCH_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "character_id.h"
#include "coordinates.h"
#include "craft_reservation.h"
#include "game_constants.h"
#include "item.h"
#include "point.h"
#include "type_id.h"

class Character;
class map;
class vehicle;

// The candidate model and the search that assigns providers to a step's requirement
// groups.  Discovery stays with crafting, because it mirrors each inventory source's own
// admission rules; everything here works on the candidates it produced.

// Where a step's tools and charges are sourced.  origin is the craft tile;
// present_char is the holder (held craft) or the crafter when standing within
// radius (map craft), else null when only the map at the craft is reachable.
struct step_source_context {
    tripoint_bub_ms origin;
    int radius = PICKUP_RANGE;
    Character *present_char = nullptr;
};

struct provider_candidate {
    craft_reservation::provider_kind kind = craft_reservation::provider_kind::last;
    const item *it = nullptr;                 // item candidates
    int64_t provider_uid = 0;
    std::optional<tripoint_abs_ms> tile;      // furniture / terrain
    furn_id furn;
    ter_id ter;
    itype_id pseudo_type = itype_id::NULL_ID();
    // Non-item providers are materialised the way their source materialises them, so a
    // charged quality is measured against the charges the provider actually has.
    std::optional<item> synthesized;
    character_id intrinsic_owner;
    // Binding a nested item hides its whole root, so scoring prefers a loose provider.
    bool nested = false;
    // Top-level item this candidate sits under, which is the unit hiding it removes
    // from the pool.  Equal to provider_uid for a loose item.
    int64_t root_uid = 0;
    // Held by the step's character rather than reachable on the map, which is the
    // distinction usage_from draws.
    bool carried = false;
};

// Everything hiding `selected` removes from the pool: an item's whole root, a part,
// or a provider tile.
struct hidden_units {
    std::set<int64_t> item_roots;
    std::set<int64_t> parts;
    std::set<tripoint_abs_ms> tiles;

    bool covers( const provider_candidate &cand ) const;
};

// Candidates that satisfy the same requests and hide the same things are
// interchangeable, so the search branches over classes rather than over items.  Members
// are grouped by exclusion unit and a unit is drained before the next one opens, which
// keeps the accumulated hidden effect a union rather than a sum.
struct candidate_class {
    std::string key;
    std::vector<const provider_candidate *> members;
    // Per request, the score, or nullopt where this class cannot serve it.
    std::vector<std::optional<int>> scores;
    bool abstract = false;
    // Leading members this craft already holds.  They sit at the front so the search
    // reaches them before it opens one that would hide anything further.
    int held = 0;
};

// One node of the depth-first assignment.
struct search_state {
    std::vector<int> usage;
    std::vector<craft_reservation::binding> selected;
    int next_abstract_slot = 0;
};

// Everything the recursion needs that does not change between nodes.
struct selection_context {
    const item *craft = nullptr;
    const step_source_context *src = nullptr;
    const std::vector<provider_candidate> *available = nullptr;
    // Merge classes collapsed away, so abstract capacity counts entries rather than
    // sources; the full pool above still answers what the debit can drain.
    const std::vector<provider_candidate> *bindable = nullptr;
    const std::vector<candidate_class> *classes = nullptr;
    const std::vector<craft_reservation::binding> *requests = nullptr;
    // (group index, providers still owed), most constrained first.
    std::vector<std::pair<int, int>> groups;
    std::set<std::string> failed;
    uint64_t expansions_left = 0;
    // Whether the charged tools of the step can still be paid for once `selected` is
    // hidden.  Supplied by the caller because it models the debit rather than the
    // search, and consulted once per expansion.
    std::function<bool( const std::vector<craft_reservation::binding> & )> pool_feasible;
};

enum class selection_result : uint8_t {
    feasible,      // an assignment was found and verified
    infeasible,    // the space was searched out; nothing present can satisfy it
    undetermined,  // the budget ran out first, so nothing is known either way
    last
};

using provider_key = std::tuple<int, int64_t, tripoint_abs_ms>;

bool unreserved_filter( const item &it );
item furniture_pseudo_item( map &m, const tripoint_bub_ms &p, const furn_id &f );
bool furniture_pseudo_available( map &m, const tripoint_bub_ms &p, const furn_id &f );
int intrinsic_exposure( const Character &who, const itype_id &type );
int provider_pseudo_quality( const provider_candidate &cand, const quality_id &qual,
                             const Character *who );
const item *candidate_item( const provider_candidate &cand );
bool candidate_merges_with( const provider_candidate &lhs, const provider_candidate &rhs );
const vehicle *linked_power_source( map &here, const item &it );
hidden_units units_hidden_by( const std::vector<craft_reservation::binding> &selected,
                              const std::vector<provider_candidate> &pool );
std::vector<provider_candidate> collapse_merge_classes(
    const std::vector<provider_candidate> &available,
    const std::vector<craft_reservation::binding> &held,
    const std::vector<craft_reservation::binding> &requests, const Character *who );
std::vector<candidate_class> build_candidate_classes(
    const std::vector<provider_candidate> &available,
    const std::vector<provider_candidate> &pool,
    const std::vector<craft_reservation::binding> &requests,
    const std::vector<craft_reservation::binding> &held, const Character *who );
std::vector<size_t> classes_for_request( const std::vector<candidate_class> &classes,
        size_t request_index );
uint64_t pool_fingerprint( const std::vector<candidate_class> &classes,
                           const std::vector<provider_candidate> &available, Character *who );
selection_result assign_from( selection_context &ctx, size_t depth, int remaining,
                              search_state &state );

#endif // CATA_SRC_CRAFT_RESERVATION_SEARCH_H
