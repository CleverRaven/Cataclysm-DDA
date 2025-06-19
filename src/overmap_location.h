#pragma once
#ifndef CATA_SRC_OVERMAP_LOCATION_H
#define CATA_SRC_OVERMAP_LOCATION_H

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "flat_set.h"
#include "type_id.h"

class JsonObject;
struct oter_t;

struct overmap_location {
    public:
        using TerrColType = cata::flat_set<oter_type_str_id>;

        void load( const JsonObject &jo, std::string_view src );
        void check() const;
        void finalize();

        // Test if oter meets the terrain restrictions.
        bool test( const int_id<oter_t> &oter ) const;
        const TerrColType &get_all_terrains() const;
        oter_type_id get_random_terrain() const;

        // Used by generic_factory
        string_id<overmap_location> id;
        std::vector<std::pair<string_id<overmap_location>, mod_id>> src;
        bool was_loaded = false;

    private:
        TerrColType terrains;
        std::vector<std::string> flags;
};

namespace overmap_locations
{

void load( const JsonObject &jo, const std::string &src );
void check_consistency();
void reset();
void finalize();

} // namespace overmap_locations

#endif // CATA_SRC_OVERMAP_LOCATION_H
