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
    context,
    var,
    last
};

class global_variables
{
    public:
        // Methods for setting/getting misc key/value pairs.
        void set_global_value( const std::string &key, const std::string &value ) {
            global_values[ key ] = value;
        }

        void remove_global_value( const std::string &key ) {
            global_values.erase( key );
        }

        std::optional<std::string> maybe_get_global_value( const std::string &key ) const {
            auto it = global_values.find( key );
            return it == global_values.end() ? std::nullopt : std::optional<std::string> { it->second };
        }

        std::string get_global_value( const std::string &key ) const {
            return maybe_get_global_value( key ).value_or( std::string{} );
        }

        std::unordered_map<std::string, std::string> get_global_values() const {
            return global_values;
        }

        void clear_global_values() {
            global_values.clear();
        }

        void set_global_values( std::unordered_map<std::string, std::string> input ) {
            global_values = std::move( input );
        }
        void unserialize( JsonObject &jo );
        void serialize( JsonOut &jsout ) const;

        std::map<std::string, std::string> migrations; // NOLINT(cata-serialize)
        static void load_migrations( const JsonObject &jo, const std::string_view &src );

    private:
        std::unordered_map<std::string, std::string> global_values;
};
global_variables &get_globals();

#endif // CATA_SRC_GLOBAL_VARS_H
