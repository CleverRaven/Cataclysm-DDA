#include "craft_reservation.h"

#include <algorithm>
#include <functional>
#include <iterator>
#include <utility>

#include "flexbuffer_json.h"
#include "item.h"
#include "item_location.h"
#include "item_uid.h"
#include "json.h"
#include "visitable.h"

namespace craft_reservation
{

void binding::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "root", root );
    jsout.member( "group_index", group_index );
    jsout.member( "alternative_index", alternative_index );
    jsout.member( "req", static_cast<int>( req ) );
    jsout.member( "qual", qual );
    jsout.member( "level", level );
    jsout.member( "tool_type", tool_type );
    jsout.member( "group_count", group_count );
    jsout.member( "kind", static_cast<int>( kind ) );
    jsout.member( "provider_uid", provider_uid );
    jsout.member( "tile", tile );
    jsout.member( "furn", furn );
    jsout.member( "ter", ter );
    jsout.member( "pseudo_type", pseudo_type );
    jsout.member( "intrinsic_owner", intrinsic_owner );
    jsout.member( "occurrence_slot", occurrence_slot );
    jsout.end_object();
}

void binding::deserialize( const JsonObject &data )
{
    data.read( "root", root );
    data.read( "group_index", group_index );
    data.read( "alternative_index", alternative_index );
    int req_raw = static_cast<int>( requirement_kind::last );
    data.read( "req", req_raw );
    req = req_raw >= 0 && req_raw < static_cast<int>( requirement_kind::last )
          ? static_cast<requirement_kind>( req_raw )
          : requirement_kind::last;
    data.read( "qual", qual );
    data.read( "level", level );
    data.read( "tool_type", tool_type );
    data.read( "group_count", group_count );
    int kind_raw = static_cast<int>( provider_kind::last );
    data.read( "kind", kind_raw );
    kind = kind_raw >= 0 && kind_raw < static_cast<int>( provider_kind::last )
           ? static_cast<provider_kind>( kind_raw )
           : provider_kind::last;
    data.read( "provider_uid", provider_uid );
    data.read( "tile", tile );
    data.read( "furn", furn );
    data.read( "ter", ter );
    data.read( "pseudo_type", pseudo_type );
    data.read( "intrinsic_owner", intrinsic_owner );
    data.read( "occurrence_slot", occurrence_slot );
}

bool contains_reserved( const item &it )
{
    const craft_reservation_index &idx = get_craft_reservations();
    // The common state is an empty index, and this runs per item in every inventory
    // formation, charge walk and automation scan; the walk must cost nothing then.
    if( !idx.any_item_claims() ) {
        return false;
    }
    bool found = false;
    it.visit_items( [&idx, &found]( const item * node, const item * ) {
        if( idx.is_reserved_uid( node->uid().get_value() ) ) {
            found = true;
            return VisitResponse::ABORT;
        }
        return VisitResponse::NEXT;
    } );
    return found;
}

} // namespace craft_reservation

bool craft_reservation_index::any_item_claims() const
{
    return !item_uid_to_owner_.empty() || !overlay_items_.empty();
}

bool craft_reservation_index::incumbent_is_live( const int64_t owner_token ) const
{
    const auto it = by_uid_.find( owner_token );
    return it != by_uid_.end() && it->second.live( calendar::turn );
}

void craft_reservation_index::unindex( const record &rec )
{
    // Conditional on still owning the entry, so an expired craft's release cannot erase
    // whoever took its provider meanwhile.
    for( const int64_t uid : rec.provider_item_uids ) {
        const auto it = item_uid_to_owner_.find( uid );
        if( it != item_uid_to_owner_.end() && it->second == rec.craft_uid ) {
            item_uid_to_owner_.erase( it );
            ++generation_;
        }
    }
    for( const int64_t uid : rec.provider_part_uids ) {
        const auto it = part_uid_to_owner_.find( uid );
        if( it != part_uid_to_owner_.end() && it->second == rec.craft_uid ) {
            part_uid_to_owner_.erase( it );
            ++generation_;
        }
    }
    for( const tripoint_abs_ms &tile : rec.provider_tiles ) {
        const auto it = provider_tiles_.find( tile );
        if( it != provider_tiles_.end() && it->second.erase( rec.craft_uid ) > 0 ) {
            ++generation_;
            if( it->second.empty() ) {
                provider_tiles_.erase( it );
            }
        }
    }
    if( rec.craft_tile ) {
        const auto it = craft_sites_.find( *rec.craft_tile );
        if( it != craft_sites_.end() && it->second.erase( rec.craft_uid ) > 0 ) {
            ++generation_;
            if( it->second.empty() ) {
                craft_sites_.erase( it );
            }
        }
    }
}

void craft_reservation_index::index( const record &rec )
{
    // Stored but never indexed, so load order cannot decide ownership.
    if( !rec.live( calendar::turn ) ) {
        return;
    }

    for( const int64_t uid : rec.provider_item_uids ) {
        const auto it = item_uid_to_owner_.find( uid );
        // An expired incumbent is no incumbent, or nothing could be bound until the
        // sweep runs.
        if( it == item_uid_to_owner_.end() || !incumbent_is_live( it->second ) ) {
            item_uid_to_owner_[uid] = rec.craft_uid;
            ++generation_;
        }
    }
    for( const int64_t uid : rec.provider_part_uids ) {
        const auto it = part_uid_to_owner_.find( uid );
        if( it == part_uid_to_owner_.end() || !incumbent_is_live( it->second ) ) {
            part_uid_to_owner_[uid] = rec.craft_uid;
            ++generation_;
        }
    }
    for( const tripoint_abs_ms &tile : rec.provider_tiles ) {
        std::set<int64_t> &owners = provider_tiles_[tile];
        for( auto it = owners.begin(); it != owners.end(); ) {
            it = incumbent_is_live( *it ) ? std::next( it ) : owners.erase( it );
        }
        if( owners.insert( rec.craft_uid ).second ) {
            ++generation_;
        }
    }
    if( rec.craft_tile ) {
        std::set<int64_t> &owners = craft_sites_[*rec.craft_tile];
        for( auto it = owners.begin(); it != owners.end(); ) {
            it = incumbent_is_live( *it ) ? std::next( it ) : owners.erase( it );
        }
        if( owners.insert( rec.craft_uid ).second ) {
            ++generation_;
        }
    }
}

static bool same_membership( const craft_reservation_index::record &a,
                             const craft_reservation_index::record &b )
{
    return a.craft_tile == b.craft_tile &&
           a.provider_tiles == b.provider_tiles &&
           a.provider_item_uids == b.provider_item_uids &&
           a.provider_part_uids == b.provider_part_uids;
}

void craft_reservation_index::set( const record &rec )
{
    const auto existing = by_uid_.find( rec.craft_uid );
    if( existing == by_uid_.end() ) {
        by_uid_[rec.craft_uid] = rec;
        index( by_uid_[rec.craft_uid] );
        return;
    }

    const bool was_live = existing->second.live( calendar::turn );
    const bool now_live = rec.live( calendar::turn );

    // Validation runs once per craft per minute; bumping on a refresh would invalidate
    // every character's crafting inventory that often.
    if( was_live == now_live && same_membership( existing->second, rec ) ) {
        existing->second = rec;
        // Idempotent, and it retakes a claim this record lost to an incumbent that has
        // since gone; an unchanged live record indexes to nothing and bumps nothing.
        index( existing->second );
        return;
    }

    unindex( existing->second );
    existing->second = rec;
    index( existing->second );
}

void craft_reservation_index::erase( const int64_t owner_token )
{
    const auto it = by_uid_.find( owner_token );
    if( it == by_uid_.end() ) {
        return;
    }
    unindex( it->second );
    by_uid_.erase( it );
}

const craft_reservation_index::record *craft_reservation_index::find(
    const int64_t owner_token ) const
{
    const auto it = by_uid_.find( owner_token );
    return it == by_uid_.end() ? nullptr : &it->second;
}

bool craft_reservation_index::is_reserved_uid( const int64_t item_uid ) const
{
    return overlay_items_.count( item_uid ) > 0 || record_for_item_uid( item_uid ) != nullptr;
}

const craft_reservation_index::record *craft_reservation_index::record_for_item_uid(
    const int64_t item_uid ) const
{
    const auto it = item_uid_to_owner_.find( item_uid );
    if( it == item_uid_to_owner_.end() ) {
        return nullptr;
    }
    const record *rec = find( it->second );
    // An expired owner reads as unreserved even while its mapping survives.
    return rec != nullptr && rec->live( calendar::turn ) ? rec : nullptr;
}

bool craft_reservation_index::craft_site_reserved( const tripoint_abs_ms &tile ) const
{
    const auto it = craft_sites_.find( tile );
    if( it == craft_sites_.end() ) {
        return false;
    }
    return std::any_of( it->second.begin(), it->second.end(),
    [this]( const int64_t owner ) {
        return incumbent_is_live( owner );
    } );
}

bool craft_reservation_index::provider_tile_reserved( const tripoint_abs_ms &tile ) const
{
    if( overlay_tiles_.count( tile ) > 0 ) {
        return true;
    }
    const auto it = provider_tiles_.find( tile );
    if( it == provider_tiles_.end() ) {
        return false;
    }
    return std::any_of( it->second.begin(), it->second.end(),
    [this]( const int64_t owner ) {
        return incumbent_is_live( owner );
    } );
}

bool craft_reservation_index::vehicle_part_reserved( const int64_t part_base_uid ) const
{
    if( overlay_parts_.count( part_base_uid ) > 0 ) {
        return true;
    }
    const auto it = part_uid_to_owner_.find( part_base_uid );
    if( it == part_uid_to_owner_.end() ) {
        return false;
    }
    return incumbent_is_live( it->second );
}

bool craft_reservation_index::item_claimed_by_other( const int64_t item_uid,
        const int64_t owner_token ) const
{
    const auto it = item_uid_to_owner_.find( item_uid );
    return it != item_uid_to_owner_.end() && it->second != owner_token &&
           incumbent_is_live( it->second );
}

bool craft_reservation_index::part_claimed_by_other( const int64_t part_base_uid,
        const int64_t owner_token ) const
{
    const auto it = part_uid_to_owner_.find( part_base_uid );
    return it != part_uid_to_owner_.end() && it->second != owner_token &&
           incumbent_is_live( it->second );
}

bool craft_reservation_index::provider_tile_claimed_by_other( const tripoint_abs_ms &tile,
        const int64_t owner_token ) const
{
    const auto it = provider_tiles_.find( tile );
    if( it == provider_tiles_.end() ) {
        return false;
    }
    return std::any_of( it->second.begin(), it->second.end(),
    [this, owner_token]( const int64_t owner ) {
        return owner != owner_token && incumbent_is_live( owner );
    } );
}

void craft_reservation_index::rebuild_for_craft( const item_location &loc )
{
    if( !loc || !loc->is_craft() ) {
        return;
    }
    const item &craft = *loc;
    const int64_t token = craft.peek_reservation_owner_token();
    if( token == 0 ) {
        return;
    }

    record rec;
    rec.craft_uid = token;
    rec.crafter = craft.get_crafter_id();
    rec.craft_tile = craft.get_reserved_tile();
    // Copied, never minted: a craft whose lease lapsed before the save comes back
    // expired rather than reclaiming its provider.
    rec.expires_at = craft.get_reservation_expiry();

    for( const craft_reservation::binding &b : craft.get_reservations() ) {
        switch( b.kind ) {
            case craft_reservation::provider_kind::item:
                rec.provider_item_uids.push_back( b.provider_uid );
                break;
            case craft_reservation::provider_kind::vehicle_part:
                rec.provider_part_uids.push_back( b.provider_uid );
                break;
            case craft_reservation::provider_kind::furniture:
            case craft_reservation::provider_kind::terrain:
                if( b.tile ) {
                    rec.provider_tiles.push_back( *b.tile );
                }
                break;
            // Shared providers and the unestablished sentinel index nothing.
            case craft_reservation::provider_kind::intrinsic:
            case craft_reservation::provider_kind::environment:
            case craft_reservation::provider_kind::last:
                break;
        }
    }

    // One provider backing several bindings is one entry; release is reference counted.
    auto dedup = []( std::vector<int64_t> &v ) {
        std::sort( v.begin(), v.end() );
        v.erase( std::unique( v.begin(), v.end() ), v.end() );
    };
    dedup( rec.provider_item_uids );
    dedup( rec.provider_part_uids );
    std::sort( rec.provider_tiles.begin(), rec.provider_tiles.end() );
    rec.provider_tiles.erase( std::unique( rec.provider_tiles.begin(), rec.provider_tiles.end() ),
                              rec.provider_tiles.end() );

    set( rec );
}

void craft_reservation_index::clear()
{
    by_uid_.clear();
    item_uid_to_owner_.clear();
    part_uid_to_owner_.clear();
    craft_sites_.clear();
    provider_tiles_.clear();
    clear_overlay();
    ++generation_;
}

void craft_reservation_index::set_overlay( std::set<int64_t> items, std::set<int64_t> parts,
        std::set<tripoint_abs_ms> tiles )
{
    const bool changes = !items.empty() || !parts.empty() || !tiles.empty();
    overlay_items_ = std::move( items );
    overlay_parts_ = std::move( parts );
    overlay_tiles_ = std::move( tiles );
    // An inventory built before this is keyed on the generation alone, and its entries
    // are copies whose uids no longer match, so only a rebuild can drop them.
    if( changes ) {
        ++generation_;
    }
}

void craft_reservation_index::clear_overlay()
{
    const bool changes = !overlay_items_.empty() || !overlay_parts_.empty() ||
                         !overlay_tiles_.empty();
    overlay_items_.clear();
    overlay_parts_.clear();
    overlay_tiles_.clear();
    if( changes ) {
        ++generation_;
    }
}

void craft_reservation_index::sweep_expired_records()
{
    // The release became visible at the deadline, not here, so the generation must not
    // move.
    const uint64_t saved_generation = generation_;
    for( auto it = by_uid_.begin(); it != by_uid_.end(); ) {
        if( it->second.live( calendar::turn ) ) {
            ++it;
            continue;
        }
        unindex( it->second );
        it = by_uid_.erase( it );
    }
    generation_ = saved_generation;
}

uint64_t craft_reservation_index::generation() const
{
    return generation_;
}

craft_reservation_index &get_craft_reservations()
{
    static craft_reservation_index index;
    return index;
}
