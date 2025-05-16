#pragma once
#ifndef CATA_SRC_GLOBAL_VARS_H
#define CATA_SRC_GLOBAL_VARS_H

#include "math_parser_diag_value.h"

#include "json.h"

class global_variables
{
    public:
        using impl_t = std::unordered_map<std::string, diag_value>;

        // Methods for setting/getting misc key/value pairs.
        void set_global_value( const std::string &key, diag_value value ) {
            global_values[ key ] = std::move( value );
        }

        template <typename... Args>
        void set_global_value( const std::string &key, Args... args ) {
            set_global_value( key, diag_value{ std::forward<Args>( args )... } );
        }

        void remove_global_value( const std::string &key ) {
            global_values.erase( key );
        }

        diag_value const *maybe_get_global_value( const std::string &key ) const {
            return _common_maybe_get_value( key, global_values );
        }

        diag_value const &get_global_value( const std::string &key ) const {
            return _common_get_value( key, global_values );
        }

        static diag_value const *_common_maybe_get_value( const std::string &key, const impl_t &cont ) {
            auto it = cont.find( key );
            return it == cont.end() ? nullptr : &it->second;
        }

        static diag_value const &_common_get_value( const std::string &key, const impl_t &cont ) {
            static diag_value const null_val;
            diag_value const *ret = _common_maybe_get_value( key, cont );
            return ret ? *ret : null_val;
        }

        impl_t &get_global_values() {
            return global_values;
        }

        impl_t const &get_global_values() const {
            return global_values;
        }

        void clear_global_values() {
            global_values.clear();
        }

        void set_global_values( impl_t input ) {
            global_values = std::move( input );
        }
        void unserialize( JsonObject &jo );
        void serialize( JsonOut &jsout ) const;

        std::map<std::string, std::string> migrations; // NOLINT(cata-serialize)
        static void load_migrations( const JsonObject &jo, std::string_view src );

    private:
        impl_t global_values;
};
global_variables &get_globals();

#endif // CATA_SRC_GLOBAL_VARS_H
