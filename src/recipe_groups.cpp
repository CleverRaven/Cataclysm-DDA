#include "recipe_groups.h"

#include <string>
#include <vector>
#include <set>
#include <utility>

#include "generic_factory.h"
#include "json.h"
#include "debug.h"
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
    std::map<std::string, std::string> recipes;
    std::map<std::string, std::set<std::string>> om_terrains;
    bool was_loaded;

    void load( JsonObject &jo, const std::string &src );
    void check() const;
};

generic_factory<recipe_group_data> recipe_groups_data( "recipe group type", "name",
        "other_handles" );

} // namespace

void recipe_group_data::load( JsonObject &jo, const std::string & )
{
    building_type = jo.get_string( "building_type" );
    JsonArray jsarr = jo.get_array( "recipes" );
    while( jsarr.has_more() ) {
        JsonObject ordering = jsarr.next_object();
        const std::string name_id = ordering.get_string( "id" );
        const std::string desc = ordering.get_string( "description" );
        recipes[desc] = name_id;
        om_terrains[name_id] = std::set<std::string>();
        JsonArray js_terr = ordering.get_array( "om_terrains" );
        while( js_terr.has_more() ) {
            const std::string ter_type = js_terr.next_string();
            om_terrains[name_id].insert( ter_type );
        }
    }
}

void recipe_group_data::check() const
{
    for( const auto &a : recipes ) {
        if( !recipe_id( a.second ).is_valid() ) {
            debugmsg( "%s is not a valid recipe", a.second );
        }
    }
}

std::map<std::string, std::string> recipe_group::get_recipes_by_bldg( const std::string &bldg )
{
    std::map<std::string, std::string> all_rec;
    if( bldg == "ALL" ) {
        for( const auto &gr : recipe_groups_data.get_all() ) {
            std::map<std::string, std::string> tmp = gr.recipes;
            all_rec.insert( tmp.begin(), tmp.end() );
        }
        return all_rec;
    } else {
        for( const auto &gr : recipe_groups_data.get_all() ) {
            if( gr.building_type != bldg ) {
                continue;
            }
            std::map<std::string, std::string> tmp = gr.recipes;
            all_rec.insert( tmp.begin(), tmp.end() );
        }
        return all_rec;
    }
}

std::map<std::string, std::string> recipe_group::get_recipes_by_id( const std::string &id,
        const std::string &om_terrain_id )
{
    std::map<std::string, std::string> all_rec;
    if( !recipe_groups_data.is_valid( group_id( id ) ) ) {
        return all_rec;
    }
    const recipe_group_data &group = recipe_groups_data.obj( group_id( id ) );
    if( om_terrain_id != "ANY" ) {
        for( const auto &recp : group.recipes ) {
            const auto &recp_terrain = group.om_terrains.find( recp.second );
            if( recp_terrain == group.om_terrains.end() ) {
                continue;
            }
            if( recp_terrain->second.find( om_terrain_id ) != recp_terrain->second.end() ) {
                all_rec[recp.first] = recp.second;
            }
        }
        return all_rec;
    }
    return group.recipes;
}

void recipe_group::load( JsonObject &jo, const std::string &src )
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
