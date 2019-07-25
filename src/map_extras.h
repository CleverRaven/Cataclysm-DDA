#pragma once
#ifndef MAP_EXTRAS_H
#define MAP_EXTRAS_H

#include <stdint.h>
#include <string>
#include <unordered_map>

#include "catacharset.h"
#include "color.h"
#include "string_id.h"

class JsonObject;
class map;
struct tripoint;

using map_extra_pointer = void( * )( map &, const tripoint & );

class map_extra
{
    public:
        string_id<map_extra> id = string_id<map_extra>::NULL_ID();
        std::string name;
        std::string description;
        std::string function;
        map_extra_pointer function_pointer;
        bool autonote = false;
        uint32_t symbol = UTF8_getch( "X" );
        nc_color color = c_red;

        std::string get_symbol() const {
            return utf32_to_utf8( symbol );
        }

        // Used by generic_factory
        bool was_loaded = false;
        void load( JsonObject &jo, const std::string &src );
};

namespace MapExtras
{
using FunctionMap = std::unordered_map<std::string, map_extra_pointer>;

map_extra_pointer get_function( const std::string &name );
FunctionMap all_functions();

void apply_function( const string_id<map_extra> &id, map &m, const tripoint &abs_sub );
void apply_function( const std::string &id, map &m, const tripoint &abs_sub );

void load( JsonObject &jo, const std::string &src );

} // namespace MapExtras

#endif
