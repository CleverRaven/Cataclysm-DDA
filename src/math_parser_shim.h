#ifndef CATA_SRC_MATH_PARSER_SHIM_H
#define CATA_SRC_MATH_PARSER_SHIM_H

#include <map>
#include <optional>
#include <string>
#include <string_view>

#include "json.h"

// temporary shim that pretends to be a JsonObject for the purpose of reusing code between the new
// "math" and the old "arithmetic"/"compare_num"/"u_val"
class kwargs_shim
{
    public:
        explicit kwargs_shim( std::vector<std::string> const &tokens, char scope );

        std::string get_string( std::string_view key ) const;
        double get_float( std::string_view key, double def = 0 ) const;
        int get_int( std::string_view key, int def = 0 ) const;

        bool has_string( std::string_view key ) const {
            return _get_string( key ).has_value();
        }
        bool has_int( std::string_view key ) const {
            constexpr int min_int = std::numeric_limits<int>::min();
            return get_int( key, min_int ) != min_int;
        }
        bool has_member( std::string_view key ) const {
            return has_string( key );
        }

        JsonValue get_member( std::string_view key ) const;
        static bool has_object( std::string_view /* key */ ) {
            return false;
        }
        static JsonObject get_object( std::string_view /* key */ ) {
            return {};
        }
        static bool has_array( std::string_view /* key */ ) {
            return false;
        }
        static JsonArray get_array( std::string_view /* key */ ) {
            return {};
        }

        static void throw_error( std::string_view message ) {
            debugmsg( message.data() );
        }
        std::string str() const;
        void allow_omitted_members() const {}

    private:
        std::map<std::string, std::string_view, std::less<>> kwargs;
        std::optional<std::string> _get_string( std::string_view key ) const;
};

#endif // CATA_SRC_MATH_PARSER_SHIM_H
