#pragma once
#ifndef CLOTHING_MOD_H
#define CLOTHING_MOD_H

#include <cstddef>
#include <string>
#include <vector>
#include <array>

#include "type_id.h"

class JsonObject;
class player;
class item;

template<typename T> struct enum_traits;

enum clothing_mod_type : int {
    clothing_mod_type_acid,
    clothing_mod_type_fire,
    clothing_mod_type_bash,
    clothing_mod_type_cut,
    clothing_mod_type_encumbrance,
    clothing_mod_type_warmth,
    clothing_mod_type_storage,
    clothing_mod_type_invalid,
    num_clothing_mod_types
};

template<>
struct enum_traits<clothing_mod_type> {
    static constexpr clothing_mod_type last = clothing_mod_type::num_clothing_mod_types;
};

struct mod_value {
    clothing_mod_type type;
    float value;
    bool round_up = false;
    bool thickness_propotion = false;
    bool coverage_propotion = false;
};

struct clothing_mod {
    void load( JsonObject &jo, const std::string &src );
    float get_mod_val( const clothing_mod_type &type, const item &it ) const;
    bool has_mod_type( const clothing_mod_type &type ) const;

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

constexpr std::array<clothing_mod_type, 8> all_clothing_mod_types = {{
        clothing_mod_type_acid,
        clothing_mod_type_fire,
        clothing_mod_type_bash,
        clothing_mod_type_cut,
        clothing_mod_type_encumbrance,
        clothing_mod_type_warmth,
        clothing_mod_type_storage,
        clothing_mod_type_invalid
    }
};

void load( JsonObject &jo, const std::string &src );
void reset();

const std::vector<clothing_mod> &get_all();
const std::vector<clothing_mod> &get_all_with( clothing_mod_type type );

std::string string_from_clothing_mod_type( clothing_mod_type type );

} // namespace clothing_mods

#endif
