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
class is_optional_helper<cata::optional<T>> : public std::true_type
{
};
} // namespace detail
template<typename T>
class is_optional : public detail::is_optional_helper<typename std::decay<T>::type>
{
};

inline void report_strict_violation( const JsonObject &jo, const std::string &message,
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
bool assign( const JsonObject &jo, const std::string &name, T &val, bool strict = false,
             T lo = std::numeric_limits<T>::lowest(), T hi = std::numeric_limits<T>::max() )
{
    T out;
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
    if( relative.read( name, out ) ) {
        err = relative;
        strict = false;
        out += val;

    } else if( proportional.read( name, scalar ) ) {
        err = proportional;
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
inline bool assign( const JsonObject &jo, const std::string &name, bool &val, bool strict = false )
{
    bool out;

    if( !jo.read( name, out ) ) {
        return false;
    }

    if( strict && out == val ) {
        report_strict_violation( jo, "assignment does not update value", name );
    }

    val = out;

    return true;
}

template <typename T, typename std::enable_if<std::is_arithmetic<T>::value, int>::type = 0>
bool assign( const JsonObject &jo, const std::string &name, std::pair<T, T> &val,
             bool strict = false, T lo = std::numeric_limits<T>::lowest(), T hi = std::numeric_limits<T>::max() )
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

// Note: is_optional excludes any types based on cata::optional, which is
// handled below in a separate function.
template < typename T, typename std::enable_if < std::is_class<T>::value &&!is_optional<T>::value,
           int >::type = 0 >
bool assign( const JsonObject &jo, const std::string &name, T &val, bool strict = false )
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
    const JsonObject &jo, const std::string &name, std::set<T> &val, bool = false )
{
    JsonObject add = jo.get_object( "extend" );
    add.allow_omitted_members();
    JsonObject del = jo.get_object( "delete" );
    del.allow_omitted_members();

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

inline bool assign( const JsonObject &jo, const std::string &name, units::volume &val,
                    bool strict = false,
                    const units::volume lo = units::volume_min,
                    const units::volume hi = units::volume_max )
{
    const auto parse = [&name]( const JsonObject & obj, units::volume & out ) {
        if( obj.has_int( name ) ) {
            out = obj.get_int( name ) * units::legacy_volume_factor;
            return true;
        }

        if( obj.has_string( name ) ) {
            units::volume::value_type tmp;
            std::string suffix;
            std::istringstream str( obj.get_string( name ) );
            str.imbue( std::locale::classic() );
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
    err.allow_omitted_members();
    JsonObject relative = jo.get_object( "relative" );
    relative.allow_omitted_members();
    JsonObject proportional = jo.get_object( "proportional" );
    proportional.allow_omitted_members();

    // Do not require strict parsing for relative and proportional values as rules
    // such as +10% are well-formed independent of whether they affect base value
    if( relative.has_member( name ) ) {
        units::volume tmp;
        err = relative;
        if( !parse( err, tmp ) ) {
            err.throw_error( "invalid relative value specified", name );
        }
        strict = false;
        out = val + tmp;

    } else if( proportional.has_member( name ) ) {
        double scalar;
        err = proportional;
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

inline bool assign( const JsonObject &jo, const std::string &name, units::mass &val,
                    bool strict = false,
                    const units::mass lo = units::mass_min,
                    const units::mass hi = units::mass_max )
{
    const auto parse = [&name]( const JsonObject & obj, units::mass & out ) {
        if( obj.has_int( name ) ) {
            out = units::from_gram<std::int64_t>( obj.get_int( name ) );
            return true;
        }
        if( obj.has_string( name ) ) {

            out = read_from_json_string<units::mass> ( *obj.get_raw( name ), units::mass_units );
            return true;
        }
        return false;
    };

    units::mass out;

    // Object via which to report errors which differs for proportional/relative values
    JsonObject err = jo;
    err.allow_omitted_members();
    JsonObject relative = jo.get_object( "relative" );
    relative.allow_omitted_members();
    JsonObject proportional = jo.get_object( "proportional" );
    proportional.allow_omitted_members();

    // Do not require strict parsing for relative and proportional values as rules
    // such as +10% are well-formed independent of whether they affect base value
    if( relative.has_member( name ) ) {
        units::mass tmp;
        err = relative;
        if( !parse( err, tmp ) ) {
            err.throw_error( "invalid relative value specified", name );
        }
        strict = false;
        out = val + tmp;

    } else if( proportional.has_member( name ) ) {
        double scalar;
        err = proportional;
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

inline bool assign( const JsonObject &jo, const std::string &name, units::money &val,
                    bool strict = false,
                    const units::money lo = units::money_min,
                    const units::money hi = units::money_max )
{
    const auto parse = [&name]( const JsonObject & obj, units::money & out ) {
        if( obj.has_int( name ) ) {
            out = units::from_cent( obj.get_int( name ) );
            return true;
        }
        if( obj.has_string( name ) ) {

            out = read_from_json_string<units::money>( *obj.get_raw( name ), units::money_units );
            return true;
        }
        return false;
    };

    units::money out;

    // Object via which to report errors which differs for proportional/relative values
    JsonObject err = jo;
    err.allow_omitted_members();
    JsonObject relative = jo.get_object( "relative" );
    relative.allow_omitted_members();
    JsonObject proportional = jo.get_object( "proportional" );
    proportional.allow_omitted_members();

    // Do not require strict parsing for relative and proportional values as rules
    // such as +10% are well-formed independent of whether they affect base value
    if( relative.has_member( name ) ) {
        units::money tmp;
        err = relative;
        if( !parse( err, tmp ) ) {
            err.throw_error( "invalid relative value specified", name );
        }
        strict = false;
        out = val + tmp;

    } else if( proportional.has_member( name ) ) {
        double scalar;
        err = proportional;
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

inline bool assign( const JsonObject &jo, const std::string &name, units::energy &val,
                    bool strict = false,
                    const units::energy lo = units::energy_min,
                    const units::energy hi = units::energy_max )
{
    const auto parse = [&name]( const JsonObject & obj, units::energy & out ) {
        if( obj.has_int( name ) ) {
            const std::int64_t tmp = obj.get_int( name );
            if( tmp > units::to_kilojoule( units::energy_max ) ) {
                out = units::energy_max;
            } else {
                out = units::from_kilojoule( tmp );
            }
            return true;
        }
        if( obj.has_string( name ) ) {

            out = read_from_json_string<units::energy>( *obj.get_raw( name ), units::energy_units );
            return true;
        }
        return false;
    };

    units::energy out;

    // Object via which to report errors which differs for proportional/relative values
    JsonObject err = jo;
    err.allow_omitted_members();
    JsonObject relative = jo.get_object( "relative" );
    relative.allow_omitted_members();
    JsonObject proportional = jo.get_object( "proportional" );
    proportional.allow_omitted_members();

    // Do not require strict parsing for relative and proportional values as rules
    // such as +10% are well-formed independent of whether they affect base value
    if( relative.has_member( name ) ) {
        units::energy tmp;
        err = relative;
        if( !parse( err, tmp ) ) {
            err.throw_error( "invalid relative value specified", name );
        }
        strict = false;
        out = val + tmp;

    } else if( proportional.has_member( name ) ) {
        double scalar;
        err = proportional;
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

inline bool assign( const JsonObject &jo, const std::string &name, nc_color &val,
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

template<typename T>
inline typename
std::enable_if<std::is_same<typename std::decay<T>::type, time_duration>::value, bool>::type
read_with_factor( const JsonObject &jo, const std::string &name, T &val, const T &factor )
{
    int tmp;
    if( jo.read( name, tmp, false ) ) {
        // JSON contained a raw number -> apply factor
        val = tmp * factor;
        return true;
    } else if( jo.has_string( name ) ) {
        // JSON contained a time duration string -> no factor
        val = read_from_json_string<time_duration>( *jo.get_raw( name ), time_duration::units );
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
            err.throw_error( "invalid proportional scalar", name );
        }
        strict = false;
        out = val * scalar;

    } else if( !read_with_factor( jo, name, out, factor ) ) {
        return false;
    }

    if( strict && out == val ) {
        report_strict_violation( err, "assignment does not update value", name );
    }

    val = out;

    return true;
}

template<typename T>
inline bool assign( const JsonObject &jo, const std::string &name, cata::optional<T> &val,
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

static const float float_max = std::numeric_limits<float>::max();

static void assign_dmg_relative( damage_instance &out, const damage_instance &val,
                                 damage_instance relative, bool &strict )
{
    for( const damage_unit &val_dmg : val.damage_units ) {
        for( damage_unit &tmp : relative.damage_units ) {
            if( tmp.type != val_dmg.type ) {
                continue;
            }

            // Do not require strict parsing for relative and proportional values as rules
            // such as +10% are well-formed independent of whether they affect base value
            strict = false;

            // res_mult is set to 1 if it's not specified. Set it to zero so we don't accidentally add to it
            if( tmp.res_mult == 1.0f ) {
                tmp.res_mult = 0;
            }
            // Same for damage_multiplier
            if( tmp.damage_multiplier == 1.0f ) {
                tmp.damage_multiplier = 0;
            }

            // As well as the unconditional versions
            if( tmp.unconditional_res_mult == 1.0f ) {
                tmp.unconditional_res_mult = 0;
            }

            if( tmp.unconditional_damage_mult == 1.0f ) {
                tmp.unconditional_damage_mult = 0;
            }

            damage_unit out_dmg( tmp.type, 0.0f );

            out_dmg.amount = tmp.amount + val_dmg.amount;
            out_dmg.res_pen = tmp.res_pen + val_dmg.res_pen;

            out_dmg.res_mult = tmp.res_mult + val_dmg.res_mult;
            out_dmg.damage_multiplier = tmp.damage_multiplier + val_dmg.damage_multiplier;

            out_dmg.unconditional_res_mult = tmp.unconditional_res_mult + val_dmg.unconditional_res_mult;
            out_dmg.unconditional_damage_mult = tmp.unconditional_damage_mult +
                                                val_dmg.unconditional_damage_mult;

            out.add( out_dmg );
        }
    }
}

static void assign_dmg_proportional( const JsonObject &jo, const std::string &name,
                                     damage_instance &out,
                                     const damage_instance &val,
                                     damage_instance proportional, bool &strict )
{
    for( const damage_unit &val_dmg : val.damage_units ) {
        for( damage_unit &scalar : proportional.damage_units ) {
            if( scalar.type != val_dmg.type ) {
                continue;
            }

            // Do not require strict parsing for relative and proportional values as rules
            // such as +10% are well-formed independent of whether they affect base value
            strict = false;

            // Can't have negative percent, and 100% is pointless
            // If it's 0, it wasn't loaded
            if( scalar.amount == 1 || scalar.amount < 0 ) {
                jo.throw_error( "Proportional damage amount is not a valid scalar", name );
            }

            // If it's 0, it wasn't loaded
            if( scalar.res_pen < 0 || scalar.res_pen == 1 ) {
                jo.throw_error( "Proportional armor penetration is not a valid scalar", name );
            }

            // It wasn't loaded, so set it 100%
            if( scalar.res_pen == 0 ) {
                scalar.res_pen = 1.0f;
            }

            // Ditto
            if( scalar.amount == 0 ) {
                scalar.amount = 1.0f;
            }

            // If it's 1, it wasn't loaded (or was loaded as 1)
            if( scalar.res_mult <= 0 ) {
                jo.throw_error( "Proportional armor penetration multiplier is not a valid scalar", name );
            }

            // If it's 1, it wasn't loaded (or was loaded as 1)
            if( scalar.damage_multiplier <= 0 ) {
                jo.throw_error( "Proportional damage multipler is not a valid scalar", name );
            }

            // If it's 1, it wasn't loaded (or was loaded as 1)
            if( scalar.unconditional_res_mult <= 0 ) {
                jo.throw_error( "Proportional unconditional armor penetration multiplier is not a valid scalar",
                                name );
            }

            // It's it's 1, it wasn't loaded (or was loaded as 1)
            if( scalar.unconditional_damage_mult <= 0 ) {
                jo.throw_error( "Proportional unconditional damage multiplier is not a valid scalar", name );
            }

            damage_unit out_dmg( scalar.type, 0.0f );

            out_dmg.amount = val_dmg.amount * scalar.amount;
            out_dmg.res_pen = val_dmg.res_pen * scalar.res_pen;

            out_dmg.res_mult = val_dmg.res_mult * scalar.res_mult;
            out_dmg.damage_multiplier = val_dmg.damage_multiplier * scalar.damage_multiplier;

            out_dmg.unconditional_res_mult = val_dmg.unconditional_res_mult * scalar.unconditional_res_mult;
            out_dmg.unconditional_damage_mult = val_dmg.unconditional_damage_mult *
                                                scalar.unconditional_damage_mult;

            out.add( out_dmg );
        }
    }
}

static void check_assigned_dmg( const JsonObject &err, const std::string &name,
                                const damage_instance &out, const damage_instance &lo_inst, const damage_instance &hi_inst )
{
    for( const damage_unit &out_dmg : out.damage_units ) {
        auto lo_iter = std::find_if( lo_inst.damage_units.begin(),
        lo_inst.damage_units.end(), [&out_dmg]( const damage_unit & du ) {
            return du.type == out_dmg.type || du.type == DT_NULL;
        } );

        auto hi_iter = std::find_if( hi_inst.damage_units.begin(),
        hi_inst.damage_units.end(), [&out_dmg]( const damage_unit & du ) {
            return du.type == out_dmg.type || du.type == DT_NULL;
        } );

        if( lo_iter == lo_inst.damage_units.end() ) {
            err.throw_error( "Min damage type used in assign does not match damage type assigned", name );
        }
        if( hi_iter == hi_inst.damage_units.end() ) {
            err.throw_error( "Max damage type used in assign does not match damage type assigned", name );
        }

        const damage_unit &hi_dmg = *hi_iter;
        const damage_unit &lo_dmg = *lo_iter;

        if( out_dmg.amount < lo_dmg.amount || out_dmg.amount > hi_dmg.amount ) {
            err.throw_error( "value for damage outside supported range", name );
        }
        if( out_dmg.res_pen < lo_dmg.res_pen || out_dmg.res_pen > hi_dmg.res_pen ) {
            err.throw_error( "value for armor penetration outside supported range", name );
        }
        if( out_dmg.res_mult < lo_dmg.res_mult || out_dmg.res_mult > hi_dmg.res_mult ) {
            err.throw_error( "value for armor penetration multiplier outside supported range", name );
        }
        if( out_dmg.damage_multiplier < lo_dmg.damage_multiplier ||
            out_dmg.damage_multiplier > hi_dmg.damage_multiplier ) {
            err.throw_error( "value for damage multiplier outside supported range", name );
        }
    }
}

inline bool assign( const JsonObject &jo, const std::string &name, damage_instance &val,
                    bool strict = false,
                    const damage_instance &lo = damage_instance( DT_NULL, 0.0f, 0.0f, 0.0f, 0.0f ),
                    const damage_instance &hi = damage_instance( DT_NULL, float_max, float_max, float_max,
                            float_max ) )
{
    // What we'll eventually be returning for the damage instance
    damage_instance out;

    if( jo.has_array( name ) ) {
        out = load_damage_instance_inherit( jo.get_array( name ), val );
    } else if( jo.has_object( name ) ) {
        out = load_damage_instance_inherit( jo.get_object( name ), val );
    } else {
        // Legacy: remove after 0.F
        float amount = 0.0f;
        float arpen = 0.0f;
        float unc_dmg_mult = 1.0f;

        // There will always be either a prop_damage or damage (name)
        if( jo.has_member( name ) ) {
            amount = jo.get_float( name );
        } else if( jo.has_member( "prop_damage" ) ) {
            unc_dmg_mult = jo.get_float( "prop_damage" );
        }
        // And there may or may not be armor penetration
        if( jo.has_member( "pierce" ) ) {
            arpen = jo.get_float( "pierce" );
        }

        out.add_damage( DT_STAB, amount, arpen, 1.0f, 1.0f, 1.0f, unc_dmg_mult );
    }

    // Object via which to report errors which differs for proportional/relative values
    const JsonObject &err = jo;
    JsonObject relative = jo.get_object( "relative" );
    relative.allow_omitted_members();
    JsonObject proportional = jo.get_object( "proportional" );
    proportional.allow_omitted_members();

    // Currently, we load only either relative or proportional when loading damage
    // There's no good reason for this, but it's simple for now
    if( relative.has_object( name ) ) {
        assign_dmg_relative( out, val, load_damage_instance( relative.get_object( name ) ), strict );
    } else if( relative.has_array( name ) ) {
        assign_dmg_relative( out, val, load_damage_instance( relative.get_array( name ) ), strict );
    } else if( proportional.has_object( name ) ) {
        assign_dmg_proportional( proportional, name, out, val,
                                 load_damage_instance( proportional.get_object( name ) ),
                                 strict );
    } else if( proportional.has_array( name ) ) {
        assign_dmg_proportional( proportional, name, out, val,
                                 load_damage_instance( proportional.get_array( name ) ),
                                 strict );
    } else if( relative.has_member( name ) || relative.has_member( "pierce" ) ||
               relative.has_member( "prop_damage" ) ) {
        // Legacy: Remove after 0.F
        // It is valid for relative to adjust any of pierce, prop_damage, or damage
        // So check for what it's modifying, and modify that
        float amt = 0.0f;
        float arpen = 0.0f;
        float unc_dmg_mul = 1.0f;

        if( relative.has_member( name ) ) {
            amt = relative.get_float( name );
        }
        if( relative.has_member( "pierce" ) ) {
            arpen = relative.get_float( "pierce" );
        }
        if( relative.has_member( "prop_damage" ) ) {
            unc_dmg_mul = relative.get_float( "prop_damage" );
        }

        assign_dmg_relative( out, val, damage_instance( DT_STAB, amt, arpen, 1.0f, 1.0f, 1.0f,
                             unc_dmg_mul ), strict );
    } else if( proportional.has_member( name ) || proportional.has_member( "pierce" ) ||
               proportional.has_member( "prop_damage" ) ) {
        // Legacy: Remove after 0.F
        // It is valid for proportional to adjust any of pierce, prop_damage, or damage
        // So check if it's modifying any of the things before going on to modify it
        float amt = 0.0f;
        float arpen = 0.0f;
        float unc_dmg_mul = 1.0f;

        if( proportional.has_member( name ) ) {
            amt = proportional.get_float( name );
        }
        if( proportional.has_member( "pierce" ) ) {
            arpen = proportional.get_float( "pierce" );
        }
        if( proportional.has_member( "prop_damage" ) ) {
            unc_dmg_mul = proportional.get_float( "prop_damage" );
        }

        assign_dmg_proportional( proportional, name, out, val, damage_instance( DT_STAB, amt, arpen, 1.0f,
                                 1.0f, 1.0f, unc_dmg_mul ), strict );
    } else if( !jo.has_member( name ) && !jo.has_member( "prop_damage" ) ) {
        // Straight copy-from, not modified by proportional or relative
        out = val;
        strict = false;
    }

    check_assigned_dmg( err, name, out, lo, hi );

    if( strict && out == val ) {
        report_strict_violation( err, "assignment does not update value", name );
    }

    if( out.damage_units.empty() ) {
        out = damage_instance( DT_STAB, 0.0f );
    }

    // Now that we've verified everything in out is all good, set val to it
    val = out;

    return true;
}
#endif // CATA_SRC_ASSIGN_H
