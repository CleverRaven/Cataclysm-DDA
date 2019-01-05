#include "recipe_groups.h"

#include <string>
#include <vector>

#include "game.h" // TODO: This is a circular dependency
#include "generic_factory.h"
#include "json.h"
#include "messages.h"
#include "player.h"

// recipe_groups namespace

namespace
{

struct recipe_group_data;
using group_id = string_id<recipe_group_data>;

struct recipe_group_data {
    group_id id;
    std::string building_type = "NONE";
    std::map<std::string, std::string> cooking_recipes;
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
        std::string name_id = ordering.get_string( "id" );
        std::string desc = ordering.get_string( "description" );
        cooking_recipes[desc] = name_id;
    }

}

void recipe_group_data::check() const
{
    for( auto a : cooking_recipes ) {
        if( !recipe_id( a.second ).is_valid() ) {
            debugmsg( "%s is not a valid recipe", a.second );
        }
    }
}

std::map<std::string, std::string> recipe_group::get_recipes( std::string id )
{
    std::map<std::string, std::string> all_rec;
    if( id == "ALL" ) {
        for( auto gr : recipe_groups_data.get_all() ) {
            std::map<std::string, std::string> tmp = gr.cooking_recipes;
            all_rec.insert( tmp.begin(), tmp.end() );
        }
        return all_rec;
    } else if( id == "COOK" || id == "BASE" || id == "FARM" || id == "SMITH" ) {
        for( auto gr : recipe_groups_data.get_all() ) {
            if( gr.building_type != id ) {
                continue;
            }
            std::map<std::string, std::string> tmp = gr.cooking_recipes;
            all_rec.insert( tmp.begin(), tmp.end() );
        }
        return all_rec;
    }
    if( !recipe_groups_data.is_valid( group_id( id ) ) ) {
        return all_rec;
    }
    const recipe_group_data &group = recipe_groups_data.obj( group_id( id ) );
    return group.cooking_recipes;
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
