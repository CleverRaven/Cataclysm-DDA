#pragma once
#ifndef CATA_SRC_MAP_ACCESSORIES_H
#define CATA_SRC_MAP_ACCESSORIES_H

#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "type_id.h"

template<typename T>
class generic_factory;

class JsonObject;

class bash_damage_profile
{
        friend class generic_factory<bash_damage_profile>;
        friend struct mod_tracker;

        bool was_loaded = false;
        bash_damage_profile_id id;
        std::vector<std::pair<bash_damage_profile_id, mod_id>> src;

        std::map<damage_type_id, double> profile;

    public:
        int damage_from( const std::map<damage_type_id, int> &str, int armor ) const;

        void load( const JsonObject &jo, std::string_view );
        void finalize();
        void check() const;

        static void load_all( const JsonObject &jo, const std::string &src );
        static void reset();
        static void finalize_all();
        static void check_all();
};

#endif // CATA_SRC_MAP_ACCESSORIES_H
