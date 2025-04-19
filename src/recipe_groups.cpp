#include "recipe_groups.h"

#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cata_variant.h"
#include "debug.h"
#include "enum_conversions.h"
#include "enums.h"
#include "flexbuffer_json.h"
#include "generic_factory.h"
#include "mapgendata.h"
#include "overmap.h"
#include "translation.h"
#include "type_id.h"

// recipe_groups namespace

namespace
{

struct recipe_group_data;

using group_id = string_id<recipe_group_data>;

struct omt_types_parameters {
    std::string omt;
    ot_match_type omt_type;
    std::unordered_map<std::string, std::unordered_set<std::string>> parameters;
};

struct recipe_group_data {
    group_id id;
    std::vector<std::pair<group_id, mod_id>> src;
    std::string building_type = "NONE";
    std::map<recipe_id, translation> recipes;
    std::map<recipe_id, std::vector<omt_types_parameters>> om_terrains;
    bool was_loaded = false;

    void load( const JsonObject &jo, std::string_view src );
    void check() const;
};

generic_factory<recipe_group_data> recipe_groups_data( "recipe group type" );

} // namespace

void recipe_group_data::load( const JsonObject &jo, std::string_view )
{
    building_type = jo.get_string( "building_type" );
    for( JsonObject ordering : jo.get_array( "recipes" ) ) {
        recipe_id name_id;
        ordering.read( "id", name_id );
        translation desc;
        ordering.read( "description", desc );
        recipes.emplace( name_id, desc );
        for( const JsonValue jv : ordering.get_array( "om_terrains" ) ) {
            std::string ter;
            ot_match_type ter_match_type = ot_match_type::type;
            std::unordered_map<std::string, std::unordered_set<std::string>> parameter_map;
            if( jv.test_string() ) {
                ter = jv.get_string();
            } else {
                JsonObject jo = jv.get_object();
                ter = jo.get_string( "om_terrain" );
                if( jo.has_string( "om_terrain_match_type" ) ) {
                    ter_match_type = jo.get_enum_value<ot_match_type>( "om_terrain_match_type", ter_match_type );
                }
                if( jo.has_object( "parameters" ) ) {
                    jo.read( "parameters", parameter_map );
                }
            }
            om_terrains[name_id].emplace_back( omt_types_parameters{ ter, ter_match_type, parameter_map } );
        }
    }
}

void recipe_group_data::check() const
{
    for( const auto &a : recipes ) {
        if( !a.first.is_valid() ) {
            debugmsg( "%s is not a valid recipe", a.first.c_str() );
        }
    }
}

std::map<recipe_id, translation> recipe_group::get_recipes_by_bldg( const std::string &bldg )
{
    std::map<recipe_id, translation> all_rec;
    if( bldg == "ALL" ) {
        for( const auto &gr : recipe_groups_data.get_all() ) {
            all_rec.insert( gr.recipes.cbegin(), gr.recipes.cend() );
        }
        return all_rec;
    } else {
        for( const auto &gr : recipe_groups_data.get_all() ) {
            if( gr.building_type != bldg ) {
                continue;
            }
            all_rec.insert( gr.recipes.cbegin(), gr.recipes.cend() );
        }
        return all_rec;
    }
}

std::map<recipe_id, translation> recipe_group::get_recipes_by_id( const std::string &id )
{
    std::map<recipe_id, translation> all_rec;
    if( !recipe_groups_data.is_valid( group_id( id ) ) ) {
        return all_rec;
    }
    const recipe_group_data &group = recipe_groups_data.obj( group_id( id ) );
    return group.recipes;
}

bool recipe_group::has_recipes_by_id( const std::string &id,
                                      const oter_id &omt_ter, const std::optional<mapgen_arguments> *maybe_args )
{
    return !get_recipes_by_id( id, omt_ter, maybe_args, 1 ).empty();
}

std::map<recipe_id, translation> recipe_group::get_recipes_by_id( const std::string &id,
        const oter_id &omt_ter, const std::optional<mapgen_arguments> *maybe_args, const size_t limit )
{
    std::map<recipe_id, translation> all_rec;
    if( !recipe_groups_data.is_valid( group_id( id ) ) ) {
        return all_rec;
    }
    const recipe_group_data &group = recipe_groups_data.obj( group_id( id ) );
    for( const auto &recp : group.recipes ) {
        if( limit > 0 && all_rec.size() >= limit ) {
            break;
        }
        const auto &recp_terrain_it = group.om_terrains.find( recp.first );
        if( recp_terrain_it == group.om_terrains.end() ) {
            debugmsg( "Recipe %s doesn't specify 'om_terrains', use ANY instead if intended to work anywhere",
                      recp.second );
            continue;
        }
        for( const omt_types_parameters &om_terrain : recp_terrain_it->second ) {
            if( om_terrain.omt == "ANY" ) {
                all_rec.emplace( recp );
                break;
            }
            if( !is_ot_match( om_terrain.omt, omt_ter, om_terrain.omt_type ) ) {
                continue;
            }
            if( om_terrain.parameters.empty() ) {
                all_rec.emplace( recp );
                break;
            }
            if( !!maybe_args ) {
                bool parameters_matched = true;
                for( const auto &key_value_set_pair : om_terrain.parameters ) {
                    auto map_key_it = maybe_args->value().map.find( key_value_set_pair.first );
                    if( map_key_it == maybe_args->value().map.end() ) {
                        debugmsg( "Parameter key %s in recipe %s not found for omt %s", key_value_set_pair.first, id,
                                  omt_ter.id().str() );
                        continue;
                    }
                    if( key_value_set_pair.second.find( map_key_it->second.get_string() ) ==
                        key_value_set_pair.second.end() ) {
                        parameters_matched = false;
                        break;
                    }
                }
                if( parameters_matched ) {
                    all_rec.emplace( recp );
                    break;
                }
            } else {
                debugmsg( "Parameter(s) expected for recipe %s but none found for omt %s", id, omt_ter.id().str() );
            }
        }
    }
    return all_rec;
}

std::string recipe_group::get_building_of_recipe( const std::string &recipe )
{
    for( const auto &group : recipe_groups_data.get_all() ) {
        for( const auto &rec : group.recipes ) {
            if( rec.first.str() == recipe ) {
                return group.building_type;
            }
        }
    }

    return "";
}

void recipe_group::load( const JsonObject &jo, const std::string &src )
{
    recipe_groups_data.load( jo, src );
}

void recipe_group::check()
{
    recipe_groups_data.check();
}

void recipe_group::reset()
{
    recipe_groups_data.reset();
}
