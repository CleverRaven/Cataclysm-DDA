#pragma once
#ifndef CATA_SRC_BUTCHERY_REQUIREMENTS_H
#define CATA_SRC_BUTCHERY_REQUIREMENTS_H

#include <map>
#include <string>
#include <vector>

#include "optional.h"
#include "string_id.h"
#include "type_id.h"

class inventory;
class JsonObject;

enum class butcher_type : int;
enum class creature_size : int;

/**
 * Contains several requirements, each with a speed modifier.
 * The goal is to iterate from highest speed modifier to lowest
 * to find the first requirement satisfied to butcher a corpse.
 */
class butchery_requirements
{
    public:
        bool was_loaded = false;
        string_id<butchery_requirements> id;

        // tries to find the requirement with the highest speed bonus. if it fails it returns cata::nullopt
        std::pair<float, requirement_id> get_fastest_requirements(
            const inventory &crafting_inv, creature_size size, butcher_type butcher ) const;

        static void load_butchery_req( const JsonObject &jo, const std::string &src );
        void load( const JsonObject &jo, const std::string & );
        static const std::vector<butchery_requirements> &get_all();
        static void check_consistency();
        static void reset_all();
        bool is_valid() const;
    private:
        // int is speed bonus
        std::map<float, std::map<creature_size, std::map<butcher_type, requirement_id>>> requirements;
};

#endif // CATA_SRC_BUTCHERY_REQUIREMENTS_H
