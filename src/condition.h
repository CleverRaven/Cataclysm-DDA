#pragma once
#ifndef CATA_SRC_CONDITION_H
#define CATA_SRC_CONDITION_H

#include <functional>
#include <iosfwd>
#include <string>
#include <unordered_set>

#include "dialogue.h"
#include "dialogue_helpers.h"
#include "mission.h"

class JsonObject;
namespace dialogue_data
{
const std::unordered_set<std::string> &simple_string_conds();
const std::unordered_set<std::string> &complex_conds();
} // namespace dialogue_data

enum class jarg {
    member = 1,
    object = 1 << 1,
    string = 1 << 2,
    array = 1 << 3
};

template<>
struct enum_traits<jarg> {
    static constexpr bool is_flag_enum = true;
};

str_or_var get_str_or_var( const JsonValue &jv, std::string_view member, bool required = true,
                           std::string_view default_val = "" );
translation_or_var get_translation_or_var( const JsonValue &jv, std::string_view member,
        bool required = true, const translation &default_val = {} );
dbl_or_var get_dbl_or_var( const JsonObject &jo, std::string_view member, bool required = true,
                           double default_val = 0.0 );
dbl_or_var_part get_dbl_or_var_part( const JsonValue &jv, std::string_view member,
                                     bool required = true,
                                     double default_val = 0.0 );
duration_or_var get_duration_or_var( const JsonObject &jo, const std::string_view &member,
                                     bool required = true,
                                     time_duration default_val = 0_seconds );
duration_or_var_part get_duration_or_var_part( const JsonValue &jv, const std::string_view &member,
        bool required = true,
        time_duration default_val = 0_seconds );
tripoint_abs_ms get_tripoint_from_var( std::optional<var_info> var, dialogue const &d );
var_info read_var_info( const JsonObject &jo );
void write_var_value( var_type type, const std::string &name, talker *talk, dialogue *d,
                      const std::string &value );
void write_var_value( var_type type, const std::string &name, talker *talk, dialogue *d,
                      double value );
std::string get_talk_varname( const JsonObject &jo, std::string_view member,
                              bool check_value, dbl_or_var &default_val );
std::string get_talk_var_basename( const JsonObject &jo, std::string_view member,
                                   bool check_value );
// the truly awful declaration for the conditional_t loading helper_function
void read_condition( const JsonObject &jo, const std::string &member_name,
                     std::function<bool( dialogue & )> &condition, bool default_val );

void finalize_conditions();

/**
 * A condition for a response spoken by the player.
 * This struct only adds the constructors which will load the data from json
 * into a lambda, stored in the std::function object.
 * Invoking the function operator with a dialog reference (so the function can access the NPC)
 * returns whether the response is allowed.
 */
struct conditional_t {
    public:
        using func = std::function<bool( dialogue & )>;

        conditional_t() = default;
        explicit conditional_t( std::string_view type );
        explicit conditional_t( const JsonObject &jo );

        static std::function<std::string( const dialogue & )> get_get_string( const JsonObject &jo );
        static std::function<translation( const dialogue & )> get_get_translation( const JsonObject &jo );
        static std::function<double( dialogue & )> get_get_dbl( std::string_view checked_value,
                char scope );
        std::function<void( dialogue &, double )>
        static get_set_dbl( std::string_view checked_value, char scope );
        bool operator()( dialogue &d ) const {
            if( !condition ) {
                return false;
            }
            return condition( d );
        }

    private:
        func condition;
};

#endif // CATA_SRC_CONDITION_H
