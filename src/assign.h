#pragma once
#ifndef CATA_SRC_ASSIGN_H
#define CATA_SRC_ASSIGN_H

#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "calendar.h"
#include "color.h"
#include "damage.h"
#include "debug.h"
#include "flat_set.h"
#include "json.h"
#include "units.h"

namespace cata
{
template<typename T>
class optional;
} // namespace cata
namespace detail
{
template<typename ...T>
class is_optional_helper : public std::false_type
{
};
template<typename T>
class is_optional_helper<std::optional<T>> : public std::true_type
{
};
} // namespace detail
template<typename T>
class is_optional : public detail::is_optional_helper<typename std::decay<T>::type>
{
};

void report_strict_violation( const JsonObject &jo, const std::string &message,
                              std::string_view name );

template <typename T, typename std::enable_if<std::is_arithmetic<T>::value, int>::type = 0>
bool assign( const JsonObject &jo, std::string_view name, T &val, bool strict = false,
             T lo = std::numeric_limits<T>::lowest(), T hi = std::numeric_limits<T>::max() )
{
    T out;
    double scalar;

    // Object via which to report errors which differs for proportional/relative values
    const JsonObject *err = &jo;
    JsonObject relative = jo.get_object( "relative" );
    relative.allow_omitted_members();
    JsonObject proportional = jo.get_object( "proportional" );
    proportional.allow_omitted_members();

    // Do not require strict parsing for relative and proportional values as rules
    // such as +10% are well-formed independent of whether they affect base value
    if( relative.read( name, out ) ) {
        err = &relative;
        strict = false;
        out += val;

    } else if( proportional.read( name, scalar ) ) {
        err = &proportional;
        if( scalar <= 0 || scalar == 1 ) {
            err->throw_error_at( name, "multiplier must be a positive number other than 1" );
        }
        strict = false;
        out = val * scalar;

    } else if( !jo.read( name, out ) ) {
        return false;
    }

    if( out < lo || out > hi ) {
        err->throw_error_at( name, "value outside supported range" );
    }

    if( strict && out == val ) {
        report_strict_violation( *err,
                                 "cannot assign explicit value the same as default or inherited value",
                                 name );
    }

    val = out;

    return true;
}

// Overload assign specifically for bool to avoid warnings,
// and also to avoid potentially nonsensical interactions between relative and proportional.
bool assign( const JsonObject &jo, std::string_view name, bool &val, bool strict = false );

template <typename T, typename std::enable_if<std::is_arithmetic<T>::value, int>::type = 0>
bool assign( const JsonObject &jo, const std::string_view name, std::pair<T, T> &val,
             bool strict = false, T lo = std::numeric_limits<T>::lowest(), T hi = std::numeric_limits<T>::max() )
{
    std::pair<T, T> out;

    if( jo.has_array( name ) ) {
        JsonArray arr = jo.get_array( name );
        arr.read( 0, out.first );
        arr.read( 1, out.second );

    } else if( jo.read( name, out.first ) ) {
        out.second = out.first;

    } else {
        return false;
    }

    if( out.first > out.second ) {
        std::swap( out.first, out.second );
    }

    if( out.first < lo || out.second > hi ) {
        jo.throw_error_at( name, "value outside supported range" );
    }

    if( strict && out == val ) {
        report_strict_violation( jo, "cannot assign explicit value the same as default or inherited value",
                                 name );
    }

    val = out;

    return true;
}

// Note: is_optional excludes any types based on cata::optional, which is
// handled below in a separate function.
template < typename T, typename std::enable_if < std::is_class<T>::value &&!is_optional<T>::value,
           int >::type = 0 >
bool assign( const JsonObject &jo, std::string_view name, T &val, bool strict = false )
{
    T out;
    if( !jo.read( name, out ) ) {
        return false;
    }

    if( strict && out == val ) {
        report_strict_violation( jo, "cannot assign explicit value the same as default or inherited value",
                                 name );
    }

    val = out;

    return true;
}

namespace details
{

template <typename T, typename Set>
bool assign_set( const JsonObject &jo, const std::string_view name, Set &val )
{
    JsonObject add = jo.get_object( "extend" );
    add.allow_omitted_members();
    JsonObject del = jo.get_object( "delete" );
    del.allow_omitted_members();

    if( jo.has_string( name ) || jo.has_array( name ) ) {
        val = jo.get_tags<T, Set>( name );

        if( add.has_member( name ) || del.has_member( name ) ) {
            // ill-formed to (re)define a value and then extend/delete within same definition
            jo.throw_error_at( name, "multiple assignment of value" );
        }
        return true;
    }

    bool res = false;

    if( add.has_string( name ) || add.has_array( name ) ) {
        auto tags = add.get_tags<T>( name );
        val.insert( tags.begin(), tags.end() );
        res = true;
    }

    if( del.has_string( name ) || del.has_array( name ) ) {
        for( const auto &e : del.get_tags<T>( name ) ) {
            val.erase( e );
        }
        res = true;
    }

    return res;
}
} // namespace details

template <typename T>
typename std::enable_if<std::is_constructible<T, std::string>::value, bool>::type assign(
    const JsonObject &jo, const std::string_view name, std::set<T> &val, bool = false )
{
    return details::assign_set<T, std::set<T>>( jo, name, val );
}

template <typename T>
typename std::enable_if<std::is_constructible<T, std::string>::value, bool>::type assign(
    const JsonObject &jo, const std::string_view name, cata::flat_set<T> &val, bool = false )
{
    return details::assign_set<T, cata::flat_set<T>>( jo, name, val );
}

bool assign( const JsonObject &jo, const std::string &name, units::volume &val,
             bool strict = false,
             units::volume lo = units::volume_min,
             units::volume hi = units::volume_max );

bool assign( const JsonObject &jo, std::string_view name, units::mass &val,
             bool strict = false,
             units::mass lo = units::mass_min,
             units::mass hi = units::mass_max );

bool assign( const JsonObject &jo, std::string_view name, units::length &val,
             bool strict = false,
             units::length lo = units::length_min,
             units::length hi = units::length_max );

bool assign( const JsonObject &jo, std::string_view name, units::money &val,
             bool strict = false,
             units::money lo = units::money_min,
             units::money hi = units::money_max );

bool assign( const JsonObject &jo, std::string_view name, units::energy &val,
             bool strict = false,
             units::energy lo = units::energy_min,
             units::energy hi = units::energy_max );

bool assign( const JsonObject &jo, std::string_view name, units::power &val,
             bool strict = false,
             units::power lo = units::power_min,
             units::power hi = units::power_max );

bool assign( const JsonObject &jo, const std::string &name, nc_color &val );

class time_duration;

template<typename T>
inline typename
std::enable_if<std::is_same<typename std::decay<T>::type, time_duration>::value, bool>::type
read_with_factor( const JsonObject &jo, const std::string_view name, T &val, const T &factor )
{
    int tmp;
    if( jo.read( name, tmp, false ) ) {
        // JSON contained a raw number -> apply factor
        val = tmp * factor;
        return true;
    } else if( jo.has_string( name ) ) {
        // JSON contained a time duration string -> no factor
        val = read_from_json_string<time_duration>( jo.get_member( name ), time_duration::units );
        return true;
    }
    return false;
}

// This is a function template not a real function as that allows it to be defined
// even when time_duration is *not* defined yet. When called with anything else but
// time_duration as `val`, SFINAE (the enable_if) will disable this function and it
// will be ignored. If it is called with time_duration, it is available and the
// *caller* is responsible for including the "calendar.h" header.
template<typename T>
inline typename
std::enable_if<std::is_same<typename std::decay<T>::type, time_duration>::value, bool>::type assign(
    const JsonObject &jo, const std::string &name, T &val, bool strict, const T &factor )
{
    T out{};
    double scalar;

    // Object via which to report errors which differs for proportional/relative values
    JsonObject err = jo;
    err.allow_omitted_members();
    JsonObject relative = jo.get_object( "relative" );
    relative.allow_omitted_members();
    JsonObject proportional = jo.get_object( "proportional" );
    proportional.allow_omitted_members();

    // Do not require strict parsing for relative and proportional values as rules
    // such as +10% are well-formed independent of whether they affect base value
    if( read_with_factor( relative, name, out, factor ) ) {
        err = relative;
        strict = false;
        out = out + val;

    } else if( proportional.read( name, scalar ) ) {
        err = proportional;
        if( scalar <= 0 || scalar == 1 ) {
            err.throw_error_at( name, "multiplier must be a positive number other than 1" );
        }
        strict = false;
        out = val * scalar;

    } else if( !read_with_factor( jo, name, out, factor ) ) {
        return false;
    }

    if( strict && out == val ) {
        report_strict_violation( err, "cannot assign explicit value the same as default or inherited value",
                                 name );
    }

    val = out;

    return true;
}

template<typename T>
inline bool assign( const JsonObject &jo, const std::string_view name, std::optional<T> &val,
                    const bool strict = false )
{
    if( !jo.has_member( name ) ) {
        return false;
    }
    if( jo.has_null( name ) ) {
        val.reset();
        return true;
    }
    if( !val ) {
        val.emplace();
    }
    return assign( jo, name, *val, strict );
}

constexpr float float_max = std::numeric_limits<float>::max();

bool assign(
    const JsonObject &jo, const std::string &name, damage_instance &val, bool strict = false,
    const damage_instance &lo = damage_instance( damage_type::NONE, 0.0f, 0.0f, 0.0f, 0.0f ),
    const damage_instance &hi = damage_instance(
                                    damage_type::NONE, float_max, float_max, float_max, float_max ) );

#endif // CATA_SRC_ASSIGN_H
