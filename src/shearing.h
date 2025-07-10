#pragma once
#include "type_id.h"
#ifndef CATA_SRC_SHEARING_H
#define CATA_SRC_SHEARING_H

#include <vector>

class JsonObject;
class monster;

struct shearing_roll {
    itype_id result;
    int amount = 0;
};

struct shearing_entry {
    itype_id result;
    int amount          = 0;
    int random_min      = 0;
    int random_max      = 0;
    float ratio_mass    = 0.0f;
    float ratio_volume  = 0.0f;

    shearing_roll roll( const monster &mon ) const;

    bool was_loaded = false;
    void load( const JsonObject &jo );
};

class shearing_data
{
    public:
        shearing_data() = default;
        explicit shearing_data( std::vector<shearing_entry> &shearing_entries );

        bool valid() const {
            return valid_;
        }

        const std::vector<shearing_entry> &entries() const {
            return entries_;
        }

        std::vector<shearing_roll> roll_all( const monster &mon ) const;

    private:
        bool valid_ = false;
        std::vector<shearing_entry> entries_;
};

#endif // CATA_SRC_SHEARING_H
