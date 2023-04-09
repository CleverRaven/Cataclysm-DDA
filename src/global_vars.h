#pragma once
#ifndef CATA_SRC_GLOBAL_VARS_H
#define CATA_SRC_GLOBAL_VARS_H
#include <utility>

#include "json.h"

enum class var_type : int {
    u,
    npc,
    global,
    faction,
    party,
    last
};

class global_variables
{
    public:
        // Methods for setting/getting misc key/value pairs.
        void set_global_value( const std::string &key, const std::string &value ) {
            global_values[ key ] = value;
        }

        void set_queued_point( const std::string &key, const std::string &value ) {
            queued_points[ key ] = value;
        }

        void remove_global_value( const std::string &key ) {
            global_values.erase( key );
        }

        std::string get_global_value( const std::string &key ) const {
            auto it = global_values.find( key );
            return ( it == global_values.end() ) ? "" : it->second;
        }

        std::unordered_map<std::string, std::string> get_global_values() const {
            return global_values;
        }

        std::unordered_map<std::string, std::string> get_queued_points() const {
            return queued_points;
        }

        void clear_global_values() {
            global_values.clear();
            queued_points.clear();
        }

        void clear_queued_points() {
            queued_points.clear();
        }

        void set_global_values( std::unordered_map<std::string, std::string> input ) {
            global_values = std::move( input );
        }
        void unserialize( JsonObject &jo );
        void serialize( JsonOut &jsout ) const;
    private:
        std::unordered_map<std::string, std::string> global_values;
        std::unordered_map<std::string, std::string> queued_points;
};
global_variables &get_globals();

#endif // CATA_SRC_GLOBAL_VARS_H
