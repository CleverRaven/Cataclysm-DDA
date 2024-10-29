#pragma once
#ifndef CATA_SRC_CONSTRUCTION_GROUP_H
#define CATA_SRC_CONSTRUCTION_GROUP_H

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "translation.h"
#include "type_id.h"

class JsonObject;

struct construction_group {
        void load( const JsonObject &jo, std::string_view src );

        construction_group_str_id id;
        std::vector<std::pair<construction_group_str_id, mod_id>> src;
        bool was_loaded = false;

        std::string name() const;

        static size_t count();

    private:
        translation _name;
};

namespace construction_groups
{

void load( const JsonObject &jo, const std::string &src );
void reset();

const std::vector<construction_group> &get_all();

} // namespace construction_groups

#endif // CATA_SRC_CONSTRUCTION_GROUP_H
