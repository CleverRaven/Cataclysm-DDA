#pragma once
#ifndef CATA_SRC_OVERMAP_LOCATION_H
#define CATA_SRC_OVERMAP_LOCATION_H

#include <algorithm>
#include <string>
#include <vector>

#include "int_id.h"
#include "string_id.h"

class JsonObject;
struct oter_t;
struct oter_type_t;

using oter_type_id = int_id<oter_type_t>;
using oter_type_str_id = string_id<oter_type_t>;

struct overmap_location {
    public:
        void load( const JsonObject &jo, const std::string &src );
        void check() const;
        void finalize();

        // Test if oter meets the terrain restrictions.
        bool test( const int_id<oter_t> &oter ) const;
        std::vector<oter_type_id> get_all_terrains() const;
        oter_type_id get_random_terrain() const;

    public:
        // Used by generic_factory
        string_id<overmap_location> id;
        bool was_loaded = false;

    private:
        std::vector<oter_type_str_id> terrains;
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
