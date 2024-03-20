#pragma once
#ifndef CATA_SRC_MAP_EXTRAS_H
#define CATA_SRC_MAP_EXTRAS_H

#include <cstdint>
#include <iosfwd>
#include <string>
#include <unordered_map>

#include "catacharset.h"
#include "color.h"
#include "coordinates.h"
#include "flat_set.h"
#include "string_id.h"
#include "translations.h"
#include "type_id.h"

class JsonObject;
class map;
class mapgendata;
class tinymap;
struct tripoint;
template<typename T> class generic_factory;
template<typename T> struct enum_traits;

enum class map_extra_method : int {
    null = 0,
    map_extra_function,
    mapgen,
    update_mapgen,
    num_map_extra_methods
};

template<>
struct enum_traits<map_extra_method> {
    static constexpr map_extra_method last = map_extra_method::num_map_extra_methods;
};

using map_extra_pointer = bool( * )( map &, const tripoint & );

class map_extra
{
    public:
        map_extra_id id = string_id<map_extra>::NULL_ID();
        std::vector<std::pair<map_extra_id, mod_id>> src;
        std::string generator_id;
        map_extra_method generator_method = map_extra_method::null;
        bool autonote = false;
        uint32_t symbol = UTF8_getch( "X" );
        nc_color color = c_red;

        bool is_valid_for( const mapgendata & ) const;

        std::string get_symbol() const {
            return utf32_to_utf8( symbol );
        }
        std::string name() const {
            return name_.translated();
        }
        std::string description() const {
            return description_.translated();
        }
        bool has_flag( const std::string &flag ) const {
            return flags_.count( flag );
        }
        const cata::flat_set<std::string> &get_flags() const {
            return flags_;
        }

        // Used by generic_factory
        bool was_loaded = false;
        void load( const JsonObject &jo, std::string_view src );
        void check() const;
    private:
        translation name_;
        translation description_;
        std::optional<std::pair<int, int>> min_max_zlevel_;
        cata::flat_set<std::string> flags_;
};

namespace MapExtras
{
using FunctionMap = std::unordered_map<map_extra_id, map_extra_pointer>;

map_extra_pointer get_function( const map_extra_id &name );
FunctionMap all_functions();
std::vector<map_extra_id> get_all_function_names();

void apply_function( const map_extra_id &id, map &m, const tripoint_abs_sm &abs_sub );
void apply_function( const map_extra_id &id, tinymap &m,
                     const tripoint_abs_omt &abs_omt );

void load( const JsonObject &jo, const std::string &src );
void check_consistency();

void debug_spawn_test();

void clear();

/// This function provides access to all loaded map extras.
const generic_factory<map_extra> &mapExtraFactory();

} // namespace MapExtras

#endif // CATA_SRC_MAP_EXTRAS_H
