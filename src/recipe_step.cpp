#include "recipe_step.h"

#include "debug.h"
#include "game_constants.h"
#include "generic_factory.h"
#include "messages.h"
#include "requirements.h"
#include "type_id.h"
#include <iostream>
// #include "item_components.h"


// class item_components;
// namespace
// {
// generic_factory<recipe_step_data> recipe_step_factory( "recipe_step" );
// } //namespace


// // Implementation of the string_id functions:
// template<>
// const recipe_step_data &string_id<recipe_step_data>::obj() const
// {
//     return recipe_step_factory.obj( *this );
// }

void recipe_proficiency::deserialize( const JsonObject &jo )
{
    load( jo );
}

void recipe_proficiency::load( const JsonObject &jo )
{
    jo.read( "proficiency", id );
    jo.read( "required", required );
    jo.read( "time_multiplier", time_multiplier );
    _skill_penalty_assigned = jo.read( "skill_penalty", skill_penalty );
    jo.read( "learning_time_multiplier", learning_time_mult );
    jo.read( "max_experience", max_experience );


    // TODO: Remove at some point
    if( jo.has_number( "fail_multiplier" ) ) {
        debugmsg( "Proficiency %s in a recipe uses 'fail_multiplier' instead of 'skill_penalty'",
                  id.c_str() );
        jo.read( "fail_multiplier", skill_penalty );
        skill_penalty -= 1;
    }
}


void recipe_step_data::load( const JsonObject &jo )
{
    // jo.read( "components", components );
    // debugmsg( "has %i steps", jo.get_array( "steps" ).size() );
    // jo.get_member( "type" );
    // mandatory( jo, was_loaded, "id", id );
    mandatory( jo, was_loaded, "description", description );

    optional( jo, was_loaded, "start_msg", start_msg );
    optional( jo, was_loaded, "end_msg", end_msg );

    optional( jo, was_loaded, "difficulty", difficulty, 0 );
    optional( jo, was_loaded, "unattended", unattended, false );
    optional( jo, was_loaded, "proficiencies", proficiencies );


    if( jo.has_string( "skill_used" ) ) {
        mandatory( jo, was_loaded, "skill_used", skill_used );

        if( jo.has_int( "difficulty" ) ) {
            difficulty = std::max( std::min( MAX_SKILL, jo.get_int( "difficulty" ) ), 0 );
        }

    }

    if( jo.has_string( "time" ) ) {
        time = to_moves<int>( read_from_json_string<time_duration>( jo.get_member( "time" ),
                              time_duration::units ) );
    }


    if( jo.has_array( "batch_time_factors" ) ) {
        JsonArray batch = jo.get_array( "batch_time_factors" );
        batch_rscale = batch.get_int( 0 ) / 100.0;
        batch_rsize  = batch.get_int( 1 );
    }

    if( jo.has_member( "skills_required" ) ) {
        JsonArray sk = jo.get_array( "skills_required" );
        required_skills.clear();

        if( sk.empty() ) {
            // clear all requirements

        } else if( sk.has_array( 0 ) ) {
            // multiple requirements
            for( JsonArray arr : sk ) {
                required_skills[skill_id( arr.get_string( 0 ) )] = arr.get_int( 1 );
            }

        } else {
            // single requirement
            required_skills[skill_id( sk.get_string( 0 ) )] = sk.get_int( 1 );
        }
    }


    const requirement_id req_id( "inline_recipe_step_" + id.str() );
    requirement_data::load_requirement( jo, req_id );
    reqs_internal.emplace_back( req_id, 1 );
}

void recipe_step_data::deserialize( const JsonObject &jo )
{
    load( jo );
}


// void recipe_step_data::load_recipe_step( const JsonObject &jo, const std::string &src )
// {
//     debugmsg( jo.has_string( "test" ) || src == "test" ? "test" : "§" );
//     // recipe_step_factory.load( jo, src );
// }

