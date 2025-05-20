#include "event_field_transformations.h"

#include <cstdlib>
#include <optional>
#include <set>
#include <string>
#include <utility>

#include "coordinates.h"
#include "itype.h"
#include "mapdata.h"
#include "mtype.h"
#include "omdata.h"
#include "overmapbuffer.h"
#include "point.h"
#include "type_id.h"

static std::vector<cata_variant> flags_of_itype( const cata_variant &v )
{
    const itype::FlagsSetType &flags = v.get<itype_id>()->get_flags();
    std::vector<cata_variant> result;
    result.reserve( flags.size() );
    for( const flag_id &s : flags ) {
        result.push_back( cata_variant::make<cata_variant_type::flag_id>( s ) );
    }
    return result;
}

static std::vector<cata_variant> flags_of_terrain( const cata_variant &v )
{
    const std::set<std::string> &flags = v.get<ter_id>()->get_flags();
    std::vector<cata_variant> result;
    result.reserve( flags.size() );
    for( const std::string &s : flags ) {
        result.push_back( cata_variant::make<cata_variant_type::string>( s ) );
    }
    return result;
}

static std::vector<cata_variant> is_mounted( const cata_variant &v )
{
    const mtype_id mount = v.get<mtype_id>();
    std::vector<cata_variant> result = { cata_variant( !mount.is_empty() ) };
    return result;
}

static std::vector<cata_variant> is_swimming_terrain( const cata_variant &v )
{
    const ter_id &ter = v.get<ter_id>();
    const bool swimming = ter->has_flag( ter_furn_flag::TFLAG_DEEP_WATER ) &&
                          ter->has_flag( ter_furn_flag::TFLAG_SWIMMABLE );
    std::vector<cata_variant> result = { cata_variant( swimming ) };
    return result;
}

static std::vector<cata_variant> math_abs( const cata_variant &v )
{
    std::vector<cata_variant> result = { cata_variant( std::abs( v.get<int>() ) ) };
    return result;
}

static std::vector<cata_variant> oter_type_of_oter( const cata_variant &v )
{
    const oter_id oter = v.get<oter_id>();
    std::vector<cata_variant> result = { cata_variant( oter->get_type_id() ) };
    return result;
}

static std::vector<cata_variant> overmap_special_at( const cata_variant &v )
{
    const tripoint_abs_omt p( v.get<tripoint>() );
    std::optional<overmap_special_id> special = overmap_buffer.overmap_special_at( p );
    if( special ) {
        return { cata_variant( *special ) };
    } else {
        return {};
    }
}

static std::vector<cata_variant> species_of_monster( const cata_variant &v )
{
    const std::set<species_id> &species = v.get<mtype_id>()->species;
    std::vector<cata_variant> result;
    result.reserve( species.size() );
    for( const species_id &s : species ) {
        result.emplace_back( s );
    }
    return result;
}

const std::unordered_map<std::string, event_field_transformation> event_field_transformations = {
    {
        "flags_of_itype",
        { flags_of_itype, cata_variant_type::flag_id, { cata_variant_type::itype_id } }
    },
    {
        "flags_of_terrain",
        { flags_of_terrain, cata_variant_type::string, { cata_variant_type::ter_id } }
    },
    {
        "is_mounted",
        { is_mounted, cata_variant_type::bool_, { cata_variant_type::mtype_id } }
    },
    {
        "is_swimming_terrain",
        { is_swimming_terrain, cata_variant_type::bool_, { cata_variant_type::ter_id } }
    },
    {
        "math_abs",
        { math_abs, cata_variant_type::int_, { cata_variant_type::int_ } }
    },
    {
        "oter_type_of_oter",
        { oter_type_of_oter, cata_variant_type::oter_type_str_id, { cata_variant_type::oter_id } }
    },
    {
        "overmap_special_at",
        { overmap_special_at, cata_variant_type::overmap_special_id, { cata_variant_type::tripoint } }
    },
    {
        "species_of_monster",
        { species_of_monster, cata_variant_type::species_id, { cata_variant_type::mtype_id } }
    },
};
