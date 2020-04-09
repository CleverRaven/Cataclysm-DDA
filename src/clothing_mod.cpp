#include "clothing_mod.h"

#include <cmath>
#include <string>

#include "generic_factory.h"
#include "item.h"
#include "itype.h"
#include "debug.h"
#include "calendar.h"

namespace
{

generic_factory<clothing_mod> all_clothing_mods( "clothing mods" );

} // namespace

static std::map<clothing_mod_type, std::vector<clothing_mod>> clothing_mods_by_type;

/** @relates string_id */
template<>
bool string_id<clothing_mod>::is_valid() const
{
    return all_clothing_mods.is_valid( *this );
}

/** @relates string_id */
template<>
const clothing_mod &string_id<clothing_mod>::obj() const
{
    return all_clothing_mods.obj( *this );
}

namespace io
{

template<>
std::string enum_to_string<clothing_mod_type>( clothing_mod_type data )
{
    switch( data ) {
        // *INDENT-OFF*
        case clothing_mod_type_acid: return "acid";
        case clothing_mod_type_fire: return "fire";
        case clothing_mod_type_bash: return "bash";
        case clothing_mod_type_cut: return "cut";
        case clothing_mod_type_encumbrance: return "encumbrance";
        case clothing_mod_type_warmth: return "warmth";
        case clothing_mod_type_storage: return "storage";
        case clothing_mod_type_coverage: return "coverage";
        case clothing_mod_type_invalid: return "invalid";
        // *INDENT-ON*
        case num_clothing_mod_types:
            break;
    }
    debugmsg( "Invalid mod type value '%d'.", data );
    return "invalid";
}

} // namespace io

void clothing_mod::load( const JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "flag", flag );
    mandatory( jo, was_loaded, "item", item_string );
    mandatory( jo, was_loaded, "implement_prompt", implement_prompt );
    mandatory( jo, was_loaded, "destroy_prompt", destroy_prompt );
    optional( jo, was_loaded, "item_quantity", item_quantity, 1 );
    optional( jo, was_loaded, "restricted", restricted, false );
    optional( jo, was_loaded, "difficulty", difficulty, 1 );
    optional( jo, was_loaded, "min_coverage", min_coverage, 0 );
    optional( jo, was_loaded, "max_coverage", max_coverage, 100 );
    optional( jo, was_loaded, "apply_flags", apply_flags, auto_flags_reader<> {} );
    optional( jo, was_loaded, "exclude_flags", exclude_flags, auto_flags_reader<> {} );
    optional( jo, was_loaded, "require_flags", require_flags, auto_flags_reader<> {} );
    optional( jo, was_loaded, "suppress_flags", suppress_flags, auto_flags_reader<> {} );
    optional( jo, was_loaded, "no_material_scaling", no_material_scaling, false );

    if( jo.has_string( "time_base" ) ) {
        time_base = read_from_json_string<time_duration>( *jo.get_raw( "time_base" ),
                    time_duration::units );
    }

    if( jo.has_array( "valid_parts" ) ) {
        valid_parts = clothing_mods::parse_json_body_parts( jo.get_array( "valid_parts" ) );
    }

    if( jo.has_array( "invalid_parts" ) ) {
        valid_parts = clothing_mods::parse_json_body_parts( jo.get_array( "valid_parts" ) );
    }

    for( const JsonObject mv_jo : jo.get_array( "mod_value" ) ) {
        mod_value mv;
        std::string temp_str;
        mandatory( mv_jo, was_loaded, "type", temp_str );
        mv.type = io::string_to_enum<clothing_mod_type>( temp_str );
        mandatory( mv_jo, was_loaded, "value", mv.value );
        optional( mv_jo, was_loaded, "round_up", mv.round_up );
        for( const JsonValue entry : mv_jo.get_array( "proportion" ) ) {
            const std::string &str = entry.get_string();
            if( str == "thickness" ) {
                mv.thickness_propotion = true;
            } else if( str == "coverage" ) {
                mv.coverage_propotion = true;
            } else {
                entry.throw_error( R"(Invalid value, valid are: "coverage" and "thickness")" );
            }
        }
        mod_values.push_back( mv );
    }
}

float clothing_mod::get_mod_val( const clothing_mod_type &type, const item &it ) const
{
    const int thickness = it.get_thickness();
    const int coverage = it.get_coverage();
    float result = 0.0f;
    for( const mod_value &mv : mod_values ) {
        if( mv.type == type ) {
            float tmp = mv.value;
            if( mv.thickness_propotion ) {
                tmp *= thickness;
            }
            if( mv.coverage_propotion ) {
                tmp *= coverage / 100.0f;
            }
            if( mv.round_up ) {
                tmp = std::ceil( tmp );
            }
            result += tmp;
        }
    }
    return result;
}

bool clothing_mod::has_mod_type( const clothing_mod_type &type ) const
{
    for( auto &mv : mod_values ) {
        if( mv.type == type ) {
            return true;
        }
    }
    return false;
}

bool clothing_mod::applies_flag( const std::string &f ) const
{
    return std::find( apply_flags.begin(), apply_flags.end(), f ) != apply_flags.end();
}

bool clothing_mod::suppresses_flag( const std::string &f ) const
{
    return std::find( suppress_flags.begin(), suppress_flags.end(), f ) != suppress_flags.end();
}

bool clothing_mod::applies_flags() const
{
    return !apply_flags.empty();
}

bool clothing_mod::suppresses_flags() const
{
    return !suppress_flags.empty();
}

bool clothing_mod::is_compatible( const item &it ) const
{
    const auto valid_mods = it.find_armor_data()->valid_mods;

    return ( !it.has_any_flag( exclude_flags ) &&
             !it.has_any_flag( apply_flags ) &&
             ( require_flags.size() == 0  || it.has_any_flag( require_flags ) ) &&
             it.get_coverage() <= max_coverage &&
             it.get_coverage() > min_coverage &&
             !( restricted && std::find( valid_mods.begin(), valid_mods.end(), flag ) == valid_mods.end() ) &&
             ( valid_parts.empty() || it.covers_any( valid_parts ) ) &&
             ( invalid_parts.empty() || !it.covers_any( invalid_parts ) ) );
}

size_t clothing_mod::count()
{
    return all_clothing_mods.size();
}

void clothing_mods::load( const JsonObject &jo, const std::string &src )
{
    all_clothing_mods.load( jo, src );
}

void clothing_mods::reset()
{
    all_clothing_mods.reset();
    clothing_mods_by_type.clear();
}

const std::vector<clothing_mod> &clothing_mods::get_all()
{
    return all_clothing_mods.get_all();
}

const std::vector<clothing_mod> &clothing_mods::get_all_with( clothing_mod_type type )
{
    const auto iter = clothing_mods_by_type.find( type );
    if( iter != clothing_mods_by_type.end() ) {
        return iter->second;
    } else {
        // Build cache
        std::vector<clothing_mod> list;
        for( auto &cm : get_all() ) {
            if( cm.has_mod_type( type ) ) {
                list.push_back( cm );
            }
        }
        clothing_mods_by_type.emplace( type, std::move( list ) );
        return clothing_mods_by_type[type];
    }
}

std::string clothing_mods::string_from_clothing_mod_type( clothing_mod_type type )
{
    return io::enum_to_string<clothing_mod_type>( type );
}

std::vector<body_part> clothing_mods::parse_json_body_parts( const JsonArray &jo )
{
    std::vector<body_part> parts;
    for( std::string val : jo ) {
        // TODO borrowed from item_factory.cpp, should make an auto parser for this someday
        if( val == "ARMS" || val == "ARM_EITHER" ) {
            parts.push_back( bp_arm_l );
            parts.push_back( bp_arm_r );
        } else if( val == "HANDS" || val == "HAND_EITHER" ) {
            parts.push_back( bp_hand_l );
            parts.push_back( bp_hand_r );
        } else if( val == "LEGS" || val == "LEG_EITHER" ) {
            parts.push_back( bp_leg_l );
            parts.push_back( bp_leg_r );
        } else if( val == "FEET" || val == "FOOT_EITHER" ) {
            parts.push_back( bp_foot_l );
            parts.push_back( bp_foot_r );
        } else {
            parts.push_back( get_body_part_token( val ) );
        }
    }

    return parts;
}
