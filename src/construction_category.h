#pragma once
#ifndef CATA_SRC_CONSTRUCTION_CATEGORY_H
#define CATA_SRC_CONSTRUCTION_CATEGORY_H

#include <cstddef>
#include <string>
#include <vector>

#include "type_id.h"
#include "translations.h"

class JsonObject;

struct construction_category {
        void load( const JsonObject &jo, const std::string &src );

        construction_category_id id;
        bool was_loaded = false;

        std::string name() const {
            return _name.translated();
        }
        static size_t count();

    private:
        translation _name;
};

namespace construction_categories
{

void load( const JsonObject &jo, const std::string &src );
void reset();

const std::vector<construction_category> &get_all();

} // namespace construction_categories

#endif // CATA_SRC_CONSTRUCTION_CATEGORY_H
