#pragma once
#ifndef OVERMAP_BIOME_H
#define OVERMAP_BIOME_H

#include <vector>
#include <string>

#include "int_id.h"
#include "string_id.h"

class JsonObject;

struct overmap_biome {
    public:
        void load(const JsonObject &jo, const std::string &src);
        void check() const;
        void finalize();

    public:
        // Used by generic_factory
        string_id<overmap_biome> id;

        //Biome weight on OM
        int weight = 100;

    
};

namespace overmap_biomes
{
    void load(const JsonObject &jo, const std::string &src);
    void check_consistency();
    void reset();
    void finalize();

} // namespace overmap_biomes

#endif // OVERMAP_BIOME_H
