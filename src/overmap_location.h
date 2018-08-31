#pragma once
#ifndef OVERMAP_LOCATION_H
#define OVERMAP_LOCATION_H

#include "int_id.h"
#include "string_id.h"

#include <string>
#include <vector>

class JsonObject;

struct oter_t;
struct oter_type_t;

using oter_type_id = int_id<oter_type_t>;
using oter_type_str_id = string_id<oter_type_t>;

struct overmap_location {
    public:
        void load( JsonObject &jo, const std::string &src );
        void check() const;

        // Test if oter meets the terrain restrictions.
        bool test( const int_id<oter_t> &oter ) const;

        oter_type_id get_random_terrain() const;

    public:
        // Used by generic_factory
        string_id<overmap_location> id;
        bool was_loaded = false;

    private:
        std::vector<oter_type_str_id> terrains;
};

namespace overmap_locations
{

void load( JsonObject &jo, const std::string &src );
void check_consistency();
void reset();

}

#endif // OVERMAP_LOCATION_H
