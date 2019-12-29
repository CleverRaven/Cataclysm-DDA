#pragma once
#ifndef MAP_EXTRAS_H
#define MAP_EXTRAS_H

#include <cstdint>
#include <string>
#include <unordered_map>

#include "catacharset.h"
#include "color.h"
#include "string_id.h"

class JsonObject;
class map;
struct tripoint;
template<typename T> struct enum_traits;
template<typename T> class generic_factory;

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

using map_extra_pointer = void( * )( map &, const tripoint & );

class map_extra
{
    public:
        string_id<map_extra> id = string_id<map_extra>::NULL_ID();
        std::string name;
        std::string description;
        std::string generator_id;
        map_extra_method generator_method;
        bool autonote = false;
        uint32_t symbol = UTF8_getch( "X" );
        nc_color color = c_red;

        std::string get_symbol() const {
            return utf32_to_utf8( symbol );
        }

        // Used by generic_factory
        bool was_loaded = false;
        void load( const JsonObject &jo, const std::string &src );
        void check() const;
};

namespace MapExtras
{
using FunctionMap = std::unordered_map<std::string, map_extra_pointer>;

map_extra_pointer get_function( const std::string &name );
FunctionMap all_functions();

void apply_function( const string_id<map_extra> &id, map &m, const tripoint &abs_sub );
void apply_function( const std::string &id, map &m, const tripoint &abs_sub );

void load( const JsonObject &jo, const std::string &src );
void check_consistency();

void debug_spawn_test();

/// This function provides access to all loaded map extras.
const generic_factory<map_extra> &mapExtraFactory();

} // namespace MapExtras

#endif
