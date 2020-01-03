#pragma once
#ifndef CONSTRUCTION_CATEGORY_H
#define CONSTRUCTION_CATEGORY_H

#include <cstddef>
#include <string>
#include <vector>

#include "type_id.h"
#include "string_id.h"

class JsonObject;

struct construction_category {
    void load( const JsonObject &jo, const std::string &src );

    construction_category_id id;
    bool was_loaded = false;

    std::string name;

    static size_t count();
};

namespace construction_categories
{

void load( const JsonObject &jo, const std::string &src );
void reset();

const std::vector<construction_category> &get_all();

} // namespace construction_categories

#endif
