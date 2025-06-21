#pragma once
#ifndef CATA_SRC_OVERMAP_MAP_DATA_CACHE_H
#define CATA_SRC_OVERMAP_MAP_DATA_CACHE_H

#include <bitset>
#include <string>

#include "json.h"

struct map_data_summary;

namespace map_data_placeholders
{
void load( const JsonObject &jo, const std::string &src );
void reset();
void finalize();
} // namespace map_data_placeholders

// Summarises one OMT worth of map data for use by agents operating at overmap scale.
// E.g. hordes, fire spread, NPCs.
// overmap already has to do a ton of coordinate stuff so just exposing the data
// for the overmap to access instead of wrapping it in methods.
struct map_data_summary {
    map_data_summary() = default;
    map_data_summary( std::bitset<24 * 24> const &new_passable,
                      bool placeholder_override = false ): placeholder( placeholder_override ),
        passable( new_passable ) {}
    void load( const JsonObject &jo, const std::string &src );
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
