#pragma once
#ifndef CATA_SRC_MORALE_TYPES_H
#define CATA_SRC_MORALE_TYPES_H

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "translation.h"
#include "type_id.h"

class JsonObject;
struct itype;

class morale_type_data
{
    private:
        bool permanent = false;
        // May contain '%s' format string
        translation text;
    public:
        morale_type id;
        std::vector<std::pair<morale_type, mod_id>> src;
        bool was_loaded = false;

        /** Describes this morale type, with item type to replace wildcard with. */
        std::string describe( const itype *it = nullptr ) const;
        bool is_permanent() const {
            return permanent;
        }

        void load( const JsonObject &jo, std::string_view src );
        void check() const;

        static void load_type( const JsonObject &jo, const std::string &src );
        static void check_all();
        static void reset();
};

#endif // CATA_SRC_MORALE_TYPES_H
