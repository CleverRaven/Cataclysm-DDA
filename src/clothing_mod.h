#pragma once
#ifndef CATA_SRC_CLOTHING_MOD_H
#define CATA_SRC_CLOTHING_MOD_H

#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "translation.h"
#include "type_id.h"

class JsonObject;
class item;
template<typename T> struct enum_traits;

enum clothing_mod_type : int {
    clothing_mod_type_acid,
    clothing_mod_type_fire,
    clothing_mod_type_bash,
    clothing_mod_type_cut,
    clothing_mod_type_bullet,
    clothing_mod_type_encumbrance,
    clothing_mod_type_warmth,
    clothing_mod_type_invalid,
    num_clothing_mod_types
};

template<>
struct enum_traits<clothing_mod_type> {
    static constexpr clothing_mod_type last = clothing_mod_type::num_clothing_mod_types;
};

struct mod_value {
    clothing_mod_type type = clothing_mod_type::num_clothing_mod_types;
    float value = 0.0f;
    bool round_up = false;
    bool thickness_proportion = false;
    bool coverage_proportion = false;
};

struct clothing_mod {
    void load( const JsonObject &jo, std::string_view src );
    float get_mod_val( const clothing_mod_type &type, const item &it ) const;
    bool has_mod_type( const clothing_mod_type &type ) const;

    clothing_mod_id id;
    std::vector<std::pair<clothing_mod_id, mod_id>> src;
    bool was_loaded = false;

    flag_id flag;
    itype_id item_string;
    translation implement_prompt;
    translation destroy_prompt;
    std::vector< mod_value > mod_values;
    bool restricted = false;

    static size_t count();
};

namespace clothing_mods
{

constexpr std::array<clothing_mod_type, 9> all_clothing_mod_types = {{
        clothing_mod_type_acid,
        clothing_mod_type_fire,
        clothing_mod_type_bash,
        clothing_mod_type_cut,
        clothing_mod_type_bullet,
        clothing_mod_type_encumbrance,
        clothing_mod_type_warmth,
        clothing_mod_type_invalid
    }
};

void load( const JsonObject &jo, const std::string &src );
void reset();

const std::vector<clothing_mod> &get_all();
const std::vector<clothing_mod> &get_all_with( clothing_mod_type type );

std::string string_from_clothing_mod_type( clothing_mod_type type );

} // namespace clothing_mods

#endif // CATA_SRC_CLOTHING_MOD_H
