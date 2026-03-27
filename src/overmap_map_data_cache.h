#pragma once
#ifndef CATA_SRC_OVERMAP_MAP_DATA_CACHE_H
#define CATA_SRC_OVERMAP_MAP_DATA_CACHE_H

#include <bitset>
#include <memory>
#include <string>
#include <string_view>

#include "string_id.h"

class JsonObject;
struct map_data_summary;

namespace map_data_placeholders
{
void load( const JsonObject &jo, const std::string &src );
void reset();
std::shared_ptr<const map_data_summary> get_ptr( string_id<map_data_summary> id );
void finalize();
} // namespace map_data_placeholders

// Summarises one OMT worth of map data for use by agents operating at overmap scale.
// E.g. hordes, fire spread, NPCs.
// overmap already has to do a ton of coordinate stuff so just exposing the data
// for the overmap to access instead of wrapping it in methods.
// These come in two different flavors. "Placeholder" entries are loaded from json and live in
// the placeholder_map_data generic_factory.  We pass "non-owning" references to these to
// overmap::layer[].map_cache to use until the underlying data is edited such that we need precise
// data instead, in which case we create an "actual" map_data_summary that is then kept in sync
// with the underlying OMT data and serialized with the overmap.
struct map_data_summary {
    map_data_summary() = default;
    explicit map_data_summary( std::bitset<24 * 24> const &new_passable,
                               bool placeholder_override = false ): placeholder( placeholder_override ),
        passable( new_passable ) {}
    void load( const JsonObject &jo, const std::string_view &src );
    // Only used for placeholder summaries, not used by "real" map summaries.
    string_id<map_data_summary> id;
    bool was_loaded = false;
    // Set to indicate that this is a placeholder instead of actual map data.
    // If this is true and something writes to this data structure it should be
    // replaced by a copy of itself (with this field set to false) and then written to.
    bool placeholder = false;
    // It's absolutely critical that we have a passability value for every single
    // map space because otherwise we can't compute that an area is completely enclosed
    // in obstacles in any reasonable way.
    std::bitset<24 * 24> passable;
};

#endif // CATA_SRC_OVERMAP_MAP_DATA_CACHE_H
