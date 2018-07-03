#pragma once
#ifndef H_ASSIGN
#define H_ASSIGN

#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>

#include "json.h"
#include "debug.h"
#include "units.h"
#include "color.h"

inline void report_strict_violation( JsonObject &jo, const std::string &message,
                                     const std::string &name )
{
    try {
        // Let the json class do the formatting, it includes the context of the JSON data.
        jo.throw_error( message, name );
    } catch( const JsonError &err ) {
        // And catch the exception so the loading continues like normal.
        debugmsg( "%s", err.what() );
    }
}

template <typename T, typename std::enable_if<std::is_arithmetic<T>::value, int>::type = 0>
bool assign( JsonObject &jo, const std::string &name, T &val, bool strict = false,
             T lo = std::numeric_limits<T>::min(), T hi = std::numeric_limits<T>::max() )
{
    T out;
    double scalar;

    // Object via which to report errors which differs for proportional/relative values
    JsonObject err = jo;

    // Do not require strict parsing for relative and proportional values as rules
    // such as +10% are well-formed independent of whether they affect base value
    if( jo.get_object( "relative" ).read( name, out ) ) {
        err = jo.get_object( "relative" );
        strict = false;
        out += val;

    } else if( jo.get_object( "proportional" ).read( name, scalar ) ) {
        err = jo.get_object( "proportional" );
        if( scalar <= 0 || scalar == 1 ) {
            err.throw_error( "invalid proportional scalar", name );
        }
        strict = false;
        out = val * scalar;

    } else if( !jo.read( name, out ) ) {

        return false;
    }

    if( out < lo || out > hi ) {
        err.throw_error( "value outside supported range", name );
    }

    if( strict && out == val ) {
        report_strict_violation( err, "assignment does not update value", name );
    }

    val = out;

    return true;
}

// Overload assign specifically for bool to avoid warnings,
// and also to avoid potentially nonsensical interactions between relative and proportional.
inline bool assign( JsonObject &jo, const std::string &name, bool &val, bool strict = false )
{
    bool out;

    // Object via which to report errors which differs for proportional/relative values
    JsonObject err = jo;

    if( !jo.read( name, out ) ) {
        return false;
    }

    if( strict && out == val ) {
        report_strict_violation( err, "assignment does not update value", name );
    }

    val = out;

    return true;
}

template <typename T, typename std::enable_if<std::is_arithmetic<T>::value, int>::type = 0>
bool assign( JsonObject &jo, const std::string &name, std::pair<T, T> &val,
             bool strict = false, T lo = std::numeric_limits<T>::min(), T hi = std::numeric_limits<T>::max() )
{
    std::pair<T, T> out;

    if( jo.has_array( name ) ) {
        auto arr = jo.get_array( name );
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
        jo.throw_error( "value outside supported range", name );
    }

    if( strict && out == val ) {
        report_strict_violation( jo, "assignment does not update value", name );
    }

    val = out;

    return true;
}

template <typename T, typename std::enable_if<std::is_class<T>::value, int>::type = 0>
bool assign( JsonObject &jo, const std::string &name, T &val, bool strict = false )
{
    T out;
    if( !jo.read( name, out ) ) {
        return false;
    }

    if( strict && out == val ) {
        report_strict_violation( jo, "assignment does not update value", name );
    }

    val = out;

    return true;
}

template <typename T>
typename std::enable_if<std::is_constructible<T, std::string>::value, bool>::type assign(
    JsonObject &jo, const std::string &name, std::set<T> &val, bool = false )
{
    auto add = jo.get_object( "extend" );
    auto del = jo.get_object( "delete" );

    if( jo.has_string( name ) || jo.has_array( name ) ) {
        val = jo.get_tags<T>( name );

        if( add.has_member( name ) || del.has_member( name ) ) {
            // ill-formed to (re)define a value and then extend/delete within same definition
            jo.throw_error( "multiple assignment of value", name );
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

inline bool assign( JsonObject &jo, const std::string &name, units::volume &val,
                    bool strict = false,
                    const units::volume lo = units::volume_min,
                    const units::volume hi = units::volume_max )
{
    auto parse = [&name]( JsonObject & obj, units::volume & out ) {
        if( obj.has_int( name ) ) {
            out = obj.get_int( name ) * units::legacy_volume_factor;
            return true;
        }

        if( obj.has_string( name ) ) {
            units::volume::value_type tmp;
            std::string suffix;
            std::istringstream str( obj.get_string( name ) );
            str >> tmp >> suffix;
            if( str.peek() != std::istringstream::traits_type::eof() ) {
                obj.throw_error( "syntax error when specifying volume", name );
            }
            if( suffix == "ml" ) {
                out = units::from_milliliter( tmp );
            } else if( suffix == "L" ) {
                out = units::from_milliliter( tmp * 1000 );
            } else {
                obj.throw_error( "unrecognized volumetric unit", name );
            }
            return true;
        }

        return false;
    };

    units::volume out;

    // Object via which to report errors which differs for proportional/relative values
    JsonObject err = jo;

    // Do not require strict parsing for relative and proportional values as rules
    // such as +10% are well-formed independent of whether they affect base value
    if( jo.get_object( "relative" ).has_member( name ) ) {
        units::volume tmp;
        err = jo.get_object( "relative" );
        if( !parse( err, tmp ) ) {
            err.throw_error( "invalid relative value specified", name );
        }
        strict = false;
        out = val + tmp;

    } else if( jo.get_object( "proportional" ).has_member( name ) ) {
        double scalar;
        err = jo.get_object( "proportional" );
        if( !err.read( name, scalar ) || scalar <= 0 || scalar == 1 ) {
            err.throw_error( "invalid proportional scalar", name );
        }
        strict = false;
        out = val * scalar;

    } else if( !parse( jo, out ) ) {
        return false;
    }

    if( out < lo || out > hi ) {
        err.throw_error( "value outside supported range", name );
    }

    if( strict && out == val ) {
        report_strict_violation( err, "assignment does not update value", name );
    }

    val = out;

    return true;
}

inline bool assign( JsonObject &jo, const std::string &name, units::mass &val,
                    bool strict = false,
                    const units::mass lo = units::mass_min,
                    const units::mass hi = units::mass_max )
{
    auto tmp = val.value();
    if( !assign( jo, name, tmp, strict, lo.value(), hi.value() ) ) {
        return false;
    }
    val = units::mass{ tmp, units::mass::unit_type{} };
    return true;
}

inline bool assign( JsonObject &jo, const std::string &name, nc_color &val,
                    const bool strict = false )
{
    if( !jo.has_member( name ) ) {
        return false;
    }
    const nc_color out = color_from_string( jo.get_string( name ) );
    if( out == c_unset ) {
        jo.throw_error( "invalid color name", name );
    }
    if( strict && out == val ) {
        report_strict_violation( jo, "assignment does not update value", name );
    }
    val = out;
    return true;
}

class time_duration;
// This is a function template not a real function as that allows it to be defined
// even when time_duration is *not* defined yet. When called with anything else but
// time_duration as `val`, SFINAE (the enable_if) will disable this function and it
// will be ignored. If it is called with time_duration, it is available and the
// *caller* is responsible for including the "calendar.h" header.
template<typename T>
inline typename
std::enable_if<std::is_same<typename std::decay<T>::type, time_duration>::value, bool>::type assign(
    JsonObject &jo, const std::string &name, T &val, bool strict, const T &factor )
{
    T out = 0;
    double scalar;
    int tmp;

    // Object via which to report errors which differs for proportional/relative values
    JsonObject err = jo;

    // Do not require strict parsing for relative and proportional values as rules
    // such as +10% are well-formed independent of whether they affect base value
    if( jo.get_object( "relative" ).read( name, tmp ) ) {
        err = jo.get_object( "relative" );
        strict = false;
        out = tmp * factor + val;

    } else if( jo.get_object( "proportional" ).read( name, scalar ) ) {
        err = jo.get_object( "proportional" );
        if( scalar <= 0 || scalar == 1 ) {
            err.throw_error( "invalid proportional scalar", name );
        }
        strict = false;
        out = val * scalar;

    } else if( jo.read( name, tmp ) ) {
        out = tmp * factor;

    } else {
        return false;
    }

    if( strict && out == val ) {
        report_strict_violation( err, "assignment does not update value", name );
    }

    val = out;

    return true;
}

#endif
