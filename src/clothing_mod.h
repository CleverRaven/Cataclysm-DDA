#pragma once
#ifndef CLOTHING_MOD_H
#define CLOTHING_MOD_H

#include <cstddef>
#include <string>
#include <vector>
#include <array>

#include "type_id.h"
#include "calendar.h"
#include "bodypart.h"

class JsonObject;
class JsonArray;
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
    clothing_mod_type_coverage,
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
    bool thickness_propotion = false;
    bool coverage_propotion = false;
};

struct clothing_mod {
    void load( const JsonObject &jo, const std::string &src );
    float get_mod_val( const clothing_mod_type &type, const item &it ) const;
    bool has_mod_type( const clothing_mod_type &type ) const;

    /**
     * Check whether the mod applies a given flag.
     *
     * @param f a flag string to check
     * @return bool true if the mod would apply the flag
     */
    bool applies_flag( const std::string &f ) const;

    /**
     * Check whether the mod suppresses a given flag.
     *
     * @param f a flag string to check
     * @return bool true if the mod would suppress the flag
     */
    bool suppresses_flag( const std::string &f ) const;

    /**
     * Whether the mod applies any flags when used.
     *
     * @return bool true if the mod applies flags
     */
    bool applies_flags() const;

    /**
     * Whether the mod suppresses any flags when used.
     *
     * @return bool true if the mod applies flags
     */
    bool suppresses_flags() const;

    /**
     * Checks if the mod is compatible with an item.
     *
     * Flags are incompatible if an item has one of the flags this mod applies,
     * or if this mod excludes one of its flags.
     *
     * Body parts are compatible if the item covers at least one of the valid_parts
     * and does not cover any of the invalid_parts.
     *
     * @param it an item
     * @return bool true if the mod is compatible, else false
     */
    bool is_compatible( const item &it ) const;

    clothing_mod_id id;
    bool was_loaded = false;

    std::string flag;
    std::string item_string;
    std::string implement_prompt;
    std::string destroy_prompt;
    /** (optional) base time to complete the mod (default 30s) **/
    time_duration time_base = 30_seconds;
    /** (optional) difficulty, default 1 **/
    int difficulty = 1;
    /** (optional) additional flags that the mod applies to the clothing **/
    std::vector< std::string > apply_flags;
    /** (optional) flags the mod suppresses from the original item when applied **/
    std::vector< std::string > suppress_flags;
    /** (optional) if present at least one of the listed flags are required for the mod to be compatible **/
    std::vector< std::string > require_flags;
    /** (optional) flags the mod is incompatible with, regardless of other issues **/
    std::vector< std::string > exclude_flags;
    /** (optional) body parts the mod can be applied to - at least one of the parts in the list must be covered by the item **/
    std::vector< body_part > valid_parts;
    /** (optional) body parts the mod cannot be applied to - none of the locations in the list may be covered **/
    std::vector< body_part > invalid_parts;
    /** a list of statistics the clothing mod effects, can be bash, cut, fire, environment, coverage, storage @see mod_value **/
    std::vector< mod_value > mod_values;
    /** (optional) if set, the mod requires the item to have this coverage level at minimum to be applied **/
    int min_coverage = 0;
    /** (optional) if set, the mod requires the item to have this coverage level at maximum to be applied **/
    int max_coverage = 100;
    /** (optional) if true, the mod will not scale the required item amount **/
    bool no_material_scaling = false;
    /** (optional) input material quantity for the mod **/
    int item_quantity = 1;
    /** (optional) must the target item opt-in to using this mod in its valid_mods property **/
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
        clothing_mod_type_encumbrance,
        clothing_mod_type_warmth,
        clothing_mod_type_storage,
        clothing_mod_type_coverage,
        clothing_mod_type_invalid
    }
};

void load( const JsonObject &jo, const std::string &src );
void reset();

/**
 * Helper function to parse body parts from clothing mod json.
 *
 * @param jo a json array containing body parts
 * @returns vector of body_part tokens
 */
std::vector<body_part> parse_json_body_parts( const JsonArray &jo );
const std::vector<clothing_mod> &get_all();
const std::vector<clothing_mod> &get_all_with( clothing_mod_type type );

std::string string_from_clothing_mod_type( clothing_mod_type type );

} // namespace clothing_mods

#endif
