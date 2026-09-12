#pragma once
#ifndef CATA_SRC_CRAFT_RESERVATION_H
#define CATA_SRC_CRAFT_RESERVATION_H

#include <algorithm>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <unordered_map>
#include <vector>

#include "calendar.h"
#include "character_id.h"
#include "coordinates.h"
#include "type_id.h"

class Creature;
class JsonObject;
class JsonOut;
class item;
class item_location;
class map;

namespace craft_reservation
{

enum class provider_kind : uint8_t {
    item,         // a concrete item, possibly nested; identity is its item_uid
    furniture,    // furn_t::crafting_pseudo_item on a tile
    terrain,      // ter_t pseudo tool on a tile
    vehicle_part, // a part's pseudo tool; identity is the part's base item uid
    intrinsic,    // character-derived pseudo item, bionic or mutation
    environment,  // map-derived generated source: fire, terrain water, keg, tank
    last
};

enum class requirement_kind : uint8_t { quality, presence_tool, last };

// Persisted on the craft.
struct binding {
    bool root = false;
    int group_index = -1;
    int alternative_index = -1;

    // Full demand as recorded, so a recipe edit that keeps the indices is caught on load.
    requirement_kind req = requirement_kind::last;
    quality_id qual;
    int level = 0;
    itype_id tool_type = itype_id::NULL_ID();
    int group_count = 0;

    provider_kind kind = provider_kind::last;
    int64_t provider_uid = 0;                // item, and vehicle_part's base item
    std::optional<tripoint_abs_ms> tile;     // furniture / terrain
    furn_id furn;
    ter_id ter;
    // Null for an intrinsic quality binding, which is keyed by capability.
    itype_id pseudo_type = itype_id::NULL_ID();
    character_id intrinsic_owner;
    // Occurrence within an abstract provider's multiset.  Bindings reusing one source
    // across groups share a slot; those within one group take distinct slots.  -1 for
    // kinds with real per-instance identity.
    int occurrence_slot = -1;

    bool established() const {
        return kind != provider_kind::last;
    }

    void serialize( JsonOut &jsout ) const;
    void deserialize( const JsonObject &data );
};

// True when a crafting inventory would fold these two into one entry, so a bound liquid
// and an equivalent free one are one provider to the gate rather than two.
bool merge_equivalent( const item &lhs, const item &rhs );

// Enforcement has no single funnel: every automation-side selector applies one of the
// predicates below itself, and one that applies none silently takes reserved providers.
// A new selector needs one, chosen by the unit its action takes: per-item where the
// action is node-local, the ancestry form where a whole subtree moves, burns or is
// re-keyed by a copy.  Planning and execution of one activity must use the same one, or
// planning keeps offering what execution refuses.

// For callers whose action carries a whole subtree.
bool contains_reserved( const item &it );

// Per-item, for callers whose action touches only the item it names.
bool usable_by_automation( const item &it );

// True when `it` or anything under it is a craft with a live passive step.  Automation
// that copies a whole subtree re-keys every craft in it, and a craft holding no bindings
// is invisible to contains_reserved.
bool contains_live_craft( const item &it );

// Both of the above in one walk, for the automation scans that ask both of every item.
bool contains_reserved_or_live_craft( const item &it );

// True when reaching `p` would require breaking something a live craft has claimed.
// map::bash destroys tile locks and the tile's items alike, hence the union.
bool bashing_would_break_reservation( map &here, const Creature &who,
                                      const tripoint_bub_ms &p );

// One expansion is one evaluation of the charged pool prune.  Test-visible so the
// budget can be asserted without measuring wall time.
uint64_t search_expansions_total();
void note_search_expansion();
void reset_search_expansions();

// Sized so the worst tick stays inside a few tens of milliseconds; the search runs on
// the main thread.
constexpr uint64_t search_budget_initial = 4096;
constexpr uint8_t search_escalation_cap = 4;

// Doubling per undetermined attempt, so a search needing modestly more finishes on the
// next tick rather than crawling up.  The largest budget an attempt can actually draw is
// the one below the cap, since a craft at the cap defers or resets rather than searching.
constexpr uint64_t search_budget_for_attempt( uint8_t attempts )
{
    return search_budget_initial << std::min( attempts, search_escalation_cap );
}

// Bump when the fingerprint's component list changes; a mismatch forces a fresh search.
constexpr uint8_t pool_fingerprint_version = 7;

// Reads a craft's own bindings as reserved for as long as it is alive, whatever the
// index says.  A lease lapses on its own schedule, and a step that keeps running past
// that must still not drain or destroy what its bindings name.
class scoped_own_claims
{
    public:
        explicit scoped_own_claims( const item &craft );
        ~scoped_own_claims();
        scoped_own_claims( const scoped_own_claims & ) = delete;
        scoped_own_claims &operator=( const scoped_own_claims & ) = delete;
};

} // namespace craft_reservation

class craft_reservation_index
{
    public:
        struct record {
            int64_t craft_uid = 0;      // the owner TOKEN, not the craft's item_uid
            character_id crafter;
            std::optional<tripoint_abs_ms> craft_tile;
            std::vector<tripoint_abs_ms> provider_tiles;
            std::vector<int64_t> provider_item_uids;
            std::vector<int64_t> provider_part_uids;
            // Copied from the craft, never minted here.
            time_point expires_at = calendar::before_time_starts;

            bool live( time_point now ) const {
                return expires_at > now;
            }

            bool empty() const {
                return !craft_tile && provider_tiles.empty() && provider_item_uids.empty() &&
                       provider_part_uids.empty();
            }
        };

        void set( const record &rec );
        void erase( int64_t owner_token );
        const record *find( int64_t owner_token ) const;

        // No requester argument: reserved is reserved, for everyone.  The reverse maps
        // carry the owner token so expiry can be checked in one hop.
        bool is_reserved_uid( int64_t item_uid ) const;

        // True while any item claim is even physically present.  Expired-but-unswept
        // mappings keep this true, so a false answer is exact: no walk can find anything.
        bool any_item_claims() const;

        // The same, widened to tiles and parts, for the guards that ask about all three.
        bool any_claims() const;

        const record *record_for_item_uid( int64_t item_uid ) const;

        // Enforcement predicates: callers outside the owning craft only need to know
        // that something is claimed.
        bool craft_site_reserved( const tripoint_abs_ms &tile ) const;
        bool provider_tile_reserved( const tripoint_abs_ms &tile ) const;
        bool vehicle_part_reserved( int64_t part_base_uid ) const;

        // For validation: a boolean cannot tell this craft's own live claim from
        // another's.  All three ignore expired records.
        bool item_claimed_by_other( int64_t item_uid, int64_t owner_token ) const;
        bool part_claimed_by_other( int64_t part_base_uid, int64_t owner_token ) const;
        bool provider_tile_claimed_by_other( const tripoint_abs_ms &tile,
                                             int64_t owner_token ) const;

        void rebuild_for_craft( const item_location &loc );
        void clear();
        // Bookkeeping only: these records were already invisible, so no generation bump.
        void sweep_expired_records();

        // Bumps only on observable membership change, never on a refresh or a sweep.
        uint64_t generation() const;

        // Claims that read as reserved for the duration of one operation, on top of the
        // records.  The enforcement predicates honour them; the owner-aware forms do
        // not, since an overlay is the owner's own claim rather than another's.
        void set_overlay( std::set<int64_t> items, std::set<int64_t> parts,
                          std::set<tripoint_abs_ms> tiles );
        void clear_overlay();

    private:

        bool incumbent_is_live( int64_t owner_token ) const;
        void unindex( const record &rec );
        void index( const record &rec );

        std::unordered_map<int64_t, record> by_uid_;
        std::unordered_map<int64_t, int64_t> item_uid_to_owner_;
        std::unordered_map<int64_t, int64_t> part_uid_to_owner_;
        // std::map: point.h specializes std::hash for tripoint, not tripoint_abs_ms.
        std::map<tripoint_abs_ms, std::set<int64_t>> craft_sites_;
        std::map<tripoint_abs_ms, std::set<int64_t>> provider_tiles_;
        std::set<int64_t> overlay_items_;
        std::set<int64_t> overlay_parts_;
        std::set<tripoint_abs_ms> overlay_tiles_;
        uint64_t generation_ = 0;
};

craft_reservation_index &get_craft_reservations();

#endif // CATA_SRC_CRAFT_RESERVATION_H
