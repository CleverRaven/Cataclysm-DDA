#include "recipe_step.h"

#include "generic_factory.h"
#include "requirements.h"
#include "type_id.h"
#include <iostream>
// #include "item_components.h"


// class item_components;
namespace
{
generic_factory<recipe_step_data> recipe_step_factory( "recipe_step" );
} //namespace


// Implementation of the string_id functions:
template<>
const recipe_step_data &string_id<recipe_step_data>::obj() const
{
    return recipe_step_factory.obj( *this );
}


void recipe_step_data::load( const JsonObject &jo, const std::string &src )
{
    if( src == "dda" ) {
        std::cout << "test";
    }
    // jo.read( "components", components );
    jo.read( "skill_used", skill_used );
    jo.read( "id", id );

    const requirement_id req_id( "inline_recipe_step_" + id.str() );
    requirement_data::load_requirement( jo, req_id );
    reqs_internal.emplace_back( req_id, 1 );
}

void recipe_step_data::load_recipe_step( const JsonObject &jo, const std::string &src )
{
    recipe_step_factory.load( jo, src );
}

