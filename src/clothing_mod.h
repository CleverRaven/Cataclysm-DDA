#pragma once
#ifndef CLOTHING_MOD_H
#define CLOTHING_MOD_H

#include <stddef.h>
#include <string>
#include <vector>

#include "type_id.h"
#include "string_id.h"
#include "ui.h"
#include "output.h"

class JsonObject;
class player;
class item;

struct mod_value {
    std::string type;
    float value;
    bool thickness_propotion = false;
    bool coverage_propotion = false;
};

struct clothing_mod {
    void load( JsonObject &jo, const std::string &src );
    float get_mod_val( const std::string type, const item &it );

    clothing_mod_id id;
    bool was_loaded = false;

    std::string flag;
    std::string item_string;
    std::string implement_prompt;
    std::string destroy_prompt;
    std::vector< mod_value > mod_values;

    static size_t count();
};

namespace clothing_mods
{

void load( JsonObject &jo, const std::string &src );
void reset();

const std::vector<clothing_mod> &get_all();

} // namespace clothing_mods

#endif
