#include "recipe_groups.h"

#include <string>
#include <vector>
#include <set>
#include <utility>

#include "generic_factory.h"
#include "json.h"
#include "debug.h"
#include "overmap.h"
#include "string_id.h"
#include "type_id.h"

// recipe_groups namespace

namespace
{

struct recipe_group_data;

using group_id = string_id<recipe_group_data>;

struct recipe_group_data {
    group_id id;
    std::string building_type = "NONE";
    std::map<recipe_id, translation> recipes;
    std::map<recipe_id, std::set<std::string>> om_terrains;
    bool was_loaded;

    void load( const JsonObject &jo, const std::string &src );
    void check() const;
};

generic_factory<recipe_group_data> recipe_groups_data( "recipe group type", "name",
        "other_handles" );

} // namespace

void recipe_group_data::load( const JsonObject &jo, const std::string & )
{
    building_type = jo.get_string( "building_type" );
    for( JsonObject ordering : jo.get_array( "recipes" ) ) {
        recipe_id name_id;
        ordering.read( "id", name_id );
        translation desc;
        ordering.read( "description", desc );
        recipes.emplace( name_id, desc );
        om_terrains[name_id] = std::set<std::string>();
        for( const std::string ter_type : ordering.get_array( "om_terrains" ) ) {
            om_terrains[name_id].insert( ter_type );
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

std::map<recipe_id, translation> recipe_group::get_recipes_by_id( const std::string &id,
        const std::string &om_terrain_id )
{
    std::map<recipe_id, translation> all_rec;
    if( !recipe_groups_data.is_valid( group_id( id ) ) ) {
        return all_rec;
    }
    const recipe_group_data &group = recipe_groups_data.obj( group_id( id ) );
    if( om_terrain_id != "ANY" ) {
        std::string base_om_ter_id = oter_no_dir( oter_id( om_terrain_id ) );

        for( const auto &recp : group.recipes ) {
            const auto &recp_terrain = group.om_terrains.find( recp.first );
            if( recp_terrain == group.om_terrains.end() ) {
                continue;
            }
            if( recp_terrain->second.find( base_om_ter_id ) != recp_terrain->second.end() ) {
                all_rec.emplace( recp );
            }
        }
        return all_rec;
    }
    return group.recipes;
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
