#include "assign.h"

void report_strict_violation( const JsonObject &jo, const std::string &message,
                              const std::string_view name )
{
    try {
        // Let the json class do the formatting, it includes the context of the JSON data.
        jo.throw_error_at( name, message );
    } catch( const JsonError &err ) {
        // And catch the exception so the loading continues like normal.
        debugmsg( "(json-error)\n%s", err.what() );
    }
}

bool assign( const JsonObject &jo, const std::string_view name, bool &val, bool strict )
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

bool assign( const JsonObject &jo, const std::string &name, units::volume &val, bool strict,
             const units::volume lo, const units::volume hi )
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
                obj.throw_error_at( name, "syntax error when specifying volume" );
            }
            if( suffix == "ml" ) {
                out = units::from_milliliter( tmp );
            } else if( suffix == "L" ) {
                out = units::from_milliliter( tmp * 1000 );
            } else {
                obj.throw_error_at( name, "unrecognized volumetric unit" );
            }
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

bool assign( const JsonObject &jo, const std::string_view name, units::mass &val, bool strict,
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

bool assign( const JsonObject &jo, const std::string_view name, units::length &val, bool strict,
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

bool assign( const JsonObject &jo, const std::string_view name, units::money &val, bool strict,
             const units::money lo, const units::money hi )
{
    const auto parse = [&name]( const JsonObject & obj, units::money & out ) {
        if( obj.has_int( name ) ) {
            out = units::from_cent( obj.get_int( name ) );
            return true;
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

bool assign( const JsonObject &jo, const std::string_view name, units::energy &val, bool strict,
             const units::energy lo, const units::energy hi )
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

bool assign( const JsonObject &jo, const std::string_view name, units::power &val, bool strict,
             const units::power lo, const units::power hi )
{
    const auto parse = [&name]( const JsonObject & obj, units::power & out ) {
        if( obj.has_int( name ) ) {
            const std::int64_t tmp = obj.get_int( name );
            if( tmp > units::to_kilowatt( units::power_max ) ) {
                out = units::power_max;
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

bool assign( const JsonObject &jo, const std::string &name, nc_color &val )
{
    if( !jo.has_member( name ) ) {
        return false;
    }
    const nc_color out = color_from_string( jo.get_string( name ) );
    if( out == c_unset ) {
        jo.throw_error_at( name, "invalid color name" );
    }
    val = out;
    return true;
}

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

            for( const barrel_desc &bd : val_dmg.barrels ) {
                out_dmg.barrels.emplace_back( bd.barrel_length, bd.amount + tmp.amount );
            }

            out.add( out_dmg );
        }
    }
}

static void assign_dmg_proportional( const JsonObject &jo, const std::string_view name,
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
                jo.throw_error_at( name, "Proportional damage multiplier must be a positive number other than 1" );
            }

            // If it's 0, it wasn't loaded
            if( scalar.res_pen < 0 || scalar.res_pen == 1 ) {
                jo.throw_error_at( name,
                                   "Proportional armor penetration multiplier must be a positive number other than 1" );
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
                jo.throw_error_at( name, "Proportional armor penetration multiplier must be a positive number" );
            }

            // If it's 1, it wasn't loaded (or was loaded as 1)
            if( scalar.damage_multiplier <= 0 ) {
                jo.throw_error_at( name, "Proportional damage multiplier must be a positive number" );
            }

            // If it's 1, it wasn't loaded (or was loaded as 1)
            if( scalar.unconditional_res_mult <= 0 ) {
                jo.throw_error_at( name,
                                   "Proportional unconditional armor penetration multiplier must be a positive number" );
            }

            // It's it's 1, it wasn't loaded (or was loaded as 1)
            if( scalar.unconditional_damage_mult <= 0 ) {
                jo.throw_error_at( name, "Proportional unconditional damage multiplier must be a positive number" );
            }

            damage_unit out_dmg( scalar.type, 0.0f );

            out_dmg.amount = val_dmg.amount * scalar.amount;
            out_dmg.res_pen = val_dmg.res_pen * scalar.res_pen;

            out_dmg.res_mult = val_dmg.res_mult * scalar.res_mult;
            out_dmg.damage_multiplier = val_dmg.damage_multiplier * scalar.damage_multiplier;

            out_dmg.unconditional_res_mult = val_dmg.unconditional_res_mult * scalar.unconditional_res_mult;
            out_dmg.unconditional_damage_mult = val_dmg.unconditional_damage_mult *
                                                scalar.unconditional_damage_mult;

            for( const barrel_desc &bd : val_dmg.barrels ) {
                out_dmg.barrels.emplace_back( bd.barrel_length, bd.amount * scalar.amount );
            }

            out.add( out_dmg );
        }
    }
}

static void check_assigned_dmg( const JsonObject &err, const std::string_view name,
                                const damage_instance &out, const damage_instance &lo_inst, const damage_instance &hi_inst )
{
    for( const damage_unit &out_dmg : out.damage_units ) {
        auto lo_iter = std::find_if( lo_inst.damage_units.begin(),
        lo_inst.damage_units.end(), [&out_dmg]( const damage_unit & du ) {
            return du.type == out_dmg.type || du.type == damage_type::NONE;
        } );

        auto hi_iter = std::find_if( hi_inst.damage_units.begin(),
        hi_inst.damage_units.end(), [&out_dmg]( const damage_unit & du ) {
            return du.type == out_dmg.type || du.type == damage_type::NONE;
        } );

        if( lo_iter == lo_inst.damage_units.end() ) {
            err.throw_error_at( name, "Min damage type used in assign does not match damage type assigned" );
        }
        if( hi_iter == hi_inst.damage_units.end() ) {
            err.throw_error_at( name, "Max damage type used in assign does not match damage type assigned" );
        }

        const damage_unit &hi_dmg = *hi_iter;
        const damage_unit &lo_dmg = *lo_iter;

        if( out_dmg.amount < lo_dmg.amount || out_dmg.amount > hi_dmg.amount ) {
            err.throw_error_at( name, "value for damage outside supported range" );
        }
        if( out_dmg.res_pen < lo_dmg.res_pen || out_dmg.res_pen > hi_dmg.res_pen ) {
            err.throw_error_at( name, "value for armor penetration outside supported range" );
        }
        if( out_dmg.res_mult < lo_dmg.res_mult || out_dmg.res_mult > hi_dmg.res_mult ) {
            err.throw_error_at( name, "value for armor penetration multiplier outside supported range" );
        }
        if( out_dmg.damage_multiplier < lo_dmg.damage_multiplier ||
            out_dmg.damage_multiplier > hi_dmg.damage_multiplier ) {
            err.throw_error_at( name, "value for damage multiplier outside supported range" );
        }
    }
}

bool assign( const JsonObject &jo, const std::string &name, damage_instance &val, bool strict,
             const damage_instance &lo, const damage_instance &hi )
{
    // What we'll eventually be returning for the damage instance
    damage_instance out;

    std::string id_err = "no id found";
    // Grab the id for good error reporting
    if( jo.has_string( "id" ) ) {
        id_err = jo.get_string( "id" );
    }

    bool assigned = false;

    if( jo.has_array( name ) ) {
        out = load_damage_instance_inherit( jo.get_array( name ), val );
        assigned = true;
    } else if( jo.has_object( name ) ) {
        out = load_damage_instance_inherit( jo.get_object( name ), val );
        assigned = true;
    } else {
        // Legacy: remove after 0.F
        float amount = 0.0f;
        float arpen = 0.0f;
        float unc_dmg_mult = 1.0f;
        bool with_legacy = false;

        // There will always be either a prop_damage or damage (name)
        if( jo.has_member( name ) ) {
            with_legacy = true;
            amount = jo.get_float( name );
        } else if( jo.has_member( "prop_damage" ) ) {
            with_legacy = true;
            unc_dmg_mult = jo.get_float( "prop_damage" );
        }
        // And there may or may not be armor penetration
        if( jo.has_member( "pierce" ) ) {
            with_legacy = true;
            arpen = jo.get_float( "pierce" );
        }

        if( with_legacy ) {
            // Give a load warning, it's likely anything loading damage this way
            // is a gun, and as such is using the wrong damage type
            debugmsg( "Warning: %s loads damage using legacy methods - damage type may be wrong", id_err );
            out.add_damage( damage_type::STAB, amount, arpen, 1.0f, 1.0f, 1.0f, unc_dmg_mult );
            assigned = true;
        }
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
        assigned = true;
    } else if( relative.has_array( name ) ) {
        assign_dmg_relative( out, val, load_damage_instance( relative.get_array( name ) ), strict );
        assigned = true;
    } else if( proportional.has_object( name ) ) {
        assign_dmg_proportional( proportional, name, out, val,
                                 load_damage_instance( proportional.get_object( name ) ),
                                 strict );
        assigned = true;
    } else if( proportional.has_array( name ) ) {
        assign_dmg_proportional( proportional, name, out, val,
                                 load_damage_instance( proportional.get_array( name ) ),
                                 strict );
        assigned = true;
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

        // Give a load warning, it's likely anything loading damage this way
        // is a gun, and as such is using the wrong damage type
        debugmsg( "Warning: %s loads damage using legacy methods - damage type may be wrong", id_err );

        assign_dmg_relative( out, val, damage_instance( damage_type::STAB, amt, arpen, 1.0f, 1.0f, 1.0f,
                             unc_dmg_mul ), strict );
        assigned = true;
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

        // Give a load warning, it's likely anything loading damage this way
        // is a gun, and as such is using the wrong damage type
        debugmsg( "Warning: %s loads damage using legacy methods - damage type may be wrong", id_err );

        assign_dmg_proportional( proportional, name, out, val, damage_instance( damage_type::STAB, amt,
                                 arpen, 1.0f,
                                 1.0f, 1.0f, unc_dmg_mul ), strict );
        assigned = true;
    }
    if( !assigned ) {
        // Straight copy-from, not modified by proportional or relative
        out = val;
    }

    check_assigned_dmg( err, name, out, lo, hi );

    if( assigned && strict && out == val ) {
        report_strict_violation( err,
                                 "cannot assign explicit damage value the same as default or inherited value", name );
    }

    // Now that we've verified everything in out is all good, set val to it
    val = out;

    return true;
}
