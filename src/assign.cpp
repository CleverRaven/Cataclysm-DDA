#include "assign.h"

#include <cstdint>
#include <vector>

#include "color.h"
#include "debug.h"

void report_strict_violation( const JsonObject &jo, const std::string &message,
                              std::string_view name )
{
    try {
        // Let the json class do the formatting, it includes the context of the JSON data.
        jo.throw_error_at( name, message );
    } catch( const JsonError &err ) {
        // And catch the exception so the loading continues like normal.
        debugmsg( "(json-error)\n%s", err.what() );
    }
}

bool assign( const JsonObject &jo, std::string_view name, bool &val, bool strict )
{
    bool out;

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

bool assign( const JsonObject &jo, std::string_view name, units::volume &val, bool strict,
             const units::volume lo, const units::volume hi )
{
    const auto parse = [name]( const JsonObject & obj, units::volume & out ) {
        if( obj.has_int( name ) ) {
            out = obj.get_int( name ) * 250_ml;
            return true;
        }

        if( obj.has_string( name ) ) {
            out = read_from_json_string<units::volume>( obj.get_member( name ), units::volume_units );
            return true;
        }

        return false;
    };

    units::volume out;

    // Object via which to report errors which differs for proportional/relative values
    const JsonObject *err = &jo;
    JsonObject relative = jo.get_object( "relative" );
    relative.allow_omitted_members();
    JsonObject proportional = jo.get_object( "proportional" );
    proportional.allow_omitted_members();

    // Do not require strict parsing for relative and proportional values as rules
    // such as +10% are well-formed independent of whether they affect base value
    if( relative.has_member( name ) ) {
        units::volume tmp;
        err = &relative;
        if( !parse( *err, tmp ) ) {
            err->throw_error_at( name, "invalid relative value specified" );
        }
        strict = false;
        out = val + tmp;

    } else if( proportional.has_member( name ) ) {
        double scalar;
        err = &proportional;
        if( !err->read( name, scalar ) || scalar <= 0 || scalar == 1 ) {
            err->throw_error_at( name, "multiplier must be a positive number other than 1" );
        }
        strict = false;
        out = val * scalar;

    } else if( !parse( jo, out ) ) {
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

bool assign( const JsonObject &jo, std::string_view name, units::mass &val, bool strict,
             const units::mass lo, const units::mass hi )
{
    const auto parse = [&name]( const JsonObject & obj, units::mass & out ) {
        if( obj.has_int( name ) ) {
            out = units::from_gram<std::int64_t>( obj.get_int( name ) );
            return true;
        }
        if( obj.has_string( name ) ) {

            out = read_from_json_string<units::mass> ( obj.get_member( name ), units::mass_units );
            return true;
        }
        return false;
    };

    units::mass out;

    // Object via which to report errors which differs for proportional/relative values
    const JsonObject *err = &jo;
    JsonObject relative = jo.get_object( "relative" );
    relative.allow_omitted_members();
    JsonObject proportional = jo.get_object( "proportional" );
    proportional.allow_omitted_members();

    // Do not require strict parsing for relative and proportional values as rules
    // such as +10% are well-formed independent of whether they affect base value
    if( relative.has_member( name ) ) {
        units::mass tmp;
        err = &relative;
        if( !parse( *err, tmp ) ) {
            err->throw_error_at( name, "invalid relative value specified" );
        }
        strict = false;
        out = val + tmp;

    } else if( proportional.has_member( name ) ) {
        double scalar;
        err = &proportional;
        if( !err->read( name, scalar ) || scalar <= 0 || scalar == 1 ) {
            err->throw_error_at( name, "multiplier must be a positive number other than 1" );
        }
        strict = false;
        out = val * scalar;

    } else if( !parse( jo, out ) ) {
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

bool assign( const JsonObject &jo, std::string_view name, units::length &val, bool strict,
             const units::length lo, const units::length hi )
{
    const auto parse = [&name]( const JsonObject & obj, units::length & out ) {
        if( obj.has_int( name ) ) {
            out = units::from_millimeter<std::int64_t>( obj.get_int( name ) );
            return true;
        }
        if( obj.has_string( name ) ) {

            out = read_from_json_string<units::length>( obj.get_member( name ), units::length_units );
            return true;
        }
        return false;
    };

    units::length out;

    // Object via which to report errors which differs for proportional/relative values
    const JsonObject *err = &jo;
    JsonObject relative = jo.get_object( "relative" );
    relative.allow_omitted_members();
    JsonObject proportional = jo.get_object( "proportional" );
    proportional.allow_omitted_members();

    // Do not require strict parsing for relative and proportional values as rules
    // such as +10% are well-formed independent of whether they affect base value
    if( relative.has_member( name ) ) {
        units::length tmp;
        err = &relative;
        if( !parse( *err, tmp ) ) {
            err->throw_error_at( name, "invalid relative value specified" );
        }
        strict = false;
        out = val + tmp;

    } else if( proportional.has_member( name ) ) {
        double scalar;
        err = &proportional;
        if( !err->read( name, scalar ) || scalar <= 0 || scalar == 1 ) {
            err->throw_error_at( name, "multiplier must be a positive number other than 1" );
        }
        strict = false;
        out = val * scalar;

    } else if( !parse( jo, out ) ) {
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

bool assign( const JsonObject &jo, std::string_view name, units::money &val, bool strict,
             const units::money lo, const units::money hi )
{
    const auto parse = [&name]( const JsonObject & obj, units::money & out ) {
        // Remove after 0.I, int reading should throw a specific error until at least then
        if( obj.has_int( name ) ) {
            std::string error_msg = "Must use denomination for pricing.  Valid units are:";
            for( const std::pair<std::string, units::money> &denomination : units::money_units ) {
                error_msg += "\n";
                error_msg += denomination.first;
            }
            obj.throw_error_at( name, error_msg );
            return false;
        }
        if( obj.has_string( name ) ) {

            out = read_from_json_string<units::money>( obj.get_member( name ), units::money_units );
            return true;
        }
        return false;
    };

    units::money out;

    // Object via which to report errors which differs for proportional/relative values
    const JsonObject *err = &jo;
    JsonObject relative = jo.get_object( "relative" );
    relative.allow_omitted_members();
    JsonObject proportional = jo.get_object( "proportional" );
    proportional.allow_omitted_members();

    // Do not require strict parsing for relative and proportional values as rules
    // such as +10% are well-formed independent of whether they affect base value
    if( relative.has_member( name ) ) {
        units::money tmp;
        err = &relative;
        if( !parse( *err, tmp ) ) {
            err->throw_error_at( name, "invalid relative value specified" );
        }
        strict = false;
        out = val + tmp;

    } else if( proportional.has_member( name ) ) {
        double scalar;
        err = &proportional;
        if( !err->read( name, scalar ) || scalar <= 0 || scalar == 1 ) {
            err->throw_error_at( name, "multiplier must be a positive number other than 1" );
        }
        strict = false;
        out = val * scalar;

    } else if( !parse( jo, out ) ) {
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

bool assign( const JsonObject &jo, std::string_view name, units::energy &val, bool strict,
             const units::energy lo, const units::energy hi )
{
    const auto parse = [&name]( const JsonObject & obj, units::energy & out ) {
        if( obj.has_int( name ) ) {
            const std::int64_t tmp = obj.get_int( name );
            if( tmp > units::to_kilojoule( units::energy::max() ) ) {
                out = units::energy::max();
            } else {
                out = units::from_kilojoule( tmp );
            }
            return true;
        }
        if( obj.has_string( name ) ) {

            out = read_from_json_string<units::energy>( obj.get_member( name ), units::energy_units );
            return true;
        }
        return false;
    };

    units::energy out;

    // Object via which to report errors which differs for proportional/relative values
    const JsonObject *err = &jo;
    JsonObject relative = jo.get_object( "relative" );
    relative.allow_omitted_members();
    JsonObject proportional = jo.get_object( "proportional" );
    proportional.allow_omitted_members();

    // Do not require strict parsing for relative and proportional values as rules
    // such as +10% are well-formed independent of whether they affect base value
    if( relative.has_member( name ) ) {
        units::energy tmp;
        err = &relative;
        if( !parse( *err, tmp ) ) {
            err->throw_error_at( name, "invalid relative value specified" );
        }
        strict = false;
        out = val + tmp;

    } else if( proportional.has_member( name ) ) {
        double scalar;
        err = &proportional;
        if( !err->read( name, scalar ) || scalar <= 0 || scalar == 1 ) {
            err->throw_error_at( name, "multiplier must be a positive number other than 1" );
        }
        strict = false;
        out = val * scalar;

    } else if( !parse( jo, out ) ) {
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

bool assign( const JsonObject &jo, std::string_view name, units::power &val, bool strict,
             const units::power lo, const units::power hi )
{
    const auto parse = [&name]( const JsonObject & obj, units::power & out ) {
        if( obj.has_int( name ) ) {
            const std::int64_t tmp = obj.get_int( name );
            if( tmp > units::to_kilowatt( units::power::max() ) ) {
                out = units::power::max();
            } else {
                out = units::from_kilowatt( tmp );
            }
            return true;
        }
        if( obj.has_string( name ) ) {

            out = read_from_json_string<units::power>( obj.get_member( name ), units::power_units );
            return true;
        }
        return false;
    };

    units::power out;

    // Object via which to report errors which differs for proportional/relative values
    const JsonObject *err = &jo;
    JsonObject relative = jo.get_object( "relative" );
    relative.allow_omitted_members();
    JsonObject proportional = jo.get_object( "proportional" );
    proportional.allow_omitted_members();

    // Do not require strict parsing for relative and proportional values as rules
    // such as +10% are well-formed independent of whether they affect base value
    if( relative.has_member( name ) ) {
        units::power tmp;
        err = &relative;
        if( !parse( *err, tmp ) ) {
            err->throw_error_at( name, "invalid relative value specified" );
        }
        strict = false;
        out = val + tmp;

    } else if( proportional.has_member( name ) ) {
        double scalar;
        err = &proportional;
        if( !err->read( name, scalar ) || scalar <= 0 || scalar == 1 ) {
            err->throw_error_at( name, "multiplier must be a positive number other than 1" );
        }
        strict = false;
        out = val * scalar;

    } else if( !parse( jo, out ) ) {
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

bool assign( const JsonObject &jo, const std::string &name, nc_color &val, bool strict )
{
    if( !jo.has_member( name ) ) {
        return false;
    }
    const nc_color out = color_from_string( jo.get_string( name ) );
    if( out == c_unset ) {
        jo.throw_error_at( name, "invalid color name" );
    }

    if( strict && out == val ) {
        report_strict_violation( jo,
                                 "cannot assign explicit value the same as default or inherited value",
                                 name );
    }

    val = out;
    return true;
}
