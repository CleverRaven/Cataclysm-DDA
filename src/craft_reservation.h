#pragma once
#ifndef CATA_SRC_CRAFT_RESERVATION_H
#define CATA_SRC_CRAFT_RESERVATION_H

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

class JsonObject;
class JsonOut;
class item;
class item_location;

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

// For callers whose action carries a whole subtree.
bool contains_reserved( const item &it );

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
