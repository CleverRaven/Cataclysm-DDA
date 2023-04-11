#include "activity_type.h"

#include <functional>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>

#include "activity_actor.h"
#include "activity_handlers.h"
#include "assign.h"
#include "debug.h"
#include "enum_conversions.h"
#include "generic_factory.h"
#include "json.h"
#include "sounds.h"
#include "string_formatter.h"
#include "translations.h"
#include "type_id.h"

// activity_type functions
static std::map< activity_id, activity_type > activity_type_all;

/** @relates string_id */
template<>
const activity_type &string_id<activity_type>::obj() const
{
    const auto found = activity_type_all.find( *this );
    if( found == activity_type_all.end() ) {
        debugmsg( "Tried to get invalid activity_type: %s", c_str() );
        static const activity_type null_activity_type {};
        return null_activity_type;
    }
    return found->second;
}

static const std::unordered_map< std::string, based_on_type > based_on_type_values = {
    { "time", based_on_type::TIME },
    { "speed", based_on_type::SPEED },
    { "neither", based_on_type::NEITHER }
};

void activity_type::load( const JsonObject &jo )
{
    activity_type result;

    result.id_ = activity_id( jo.get_string( "id" ) );
    assign( jo, "rooted", result.rooted_, true );
    assign( jo, "verb", result.verb_, true );
    assign( jo, "interruptable", result.interruptable_, true );
    assign( jo, "interruptable_with_kb", result.interruptable_with_kb_, true );
    assign( jo, "suspendable", result.suspendable_, true );
    assign( jo, "no_resume", result.no_resume_, true );
    assign( jo, "multi_activity", result.multi_activity_, false );
    assign( jo, "refuel_fires", result.refuel_fires, false );
    assign( jo, "auto_needs", result.auto_needs, false );
    optional( jo, false, "completion_eoc", result.completion_EOC );
    optional( jo, false, "do_turn_eoc", result.do_turn_EOC );

    std::string activity_level = jo.get_string( "activity_level", "" );
    if( activity_level.empty() ) {
        debugmsg( "Warning.  %s has undefined activity level.  defaulting to LIGHT_EXERCISE",
                  result.id().c_str() );
        activity_level = "LIGHT_EXERCISE";
    }
    result.activity_level = activity_levels_map.find( activity_level )->second;

    result.based_on_ = io::string_to_enum_look_up( based_on_type_values, jo.get_string( "based_on" ) );

    if( activity_type_all.find( result.id_ ) != activity_type_all.end() ) {
        debugmsg( "Redefinition of %s", result.id_.c_str() );
    } else {
        activity_type_all.insert( { result.id_, result } );
    }
}

void activity_type::check_consistency()
{
    for( const auto &pair : activity_type_all ) {
        if( pair.second.verb_.empty() ) {
            debugmsg( "%s doesn't have a verb", pair.first.c_str() );
        }
        const bool has_actor = activity_actors::deserialize_functions.find( pair.second.id_ ) !=
                               activity_actors::deserialize_functions.end();
        const bool has_turn_func = activity_handlers::do_turn_functions.find( pair.second.id_ ) !=
                                   activity_handlers::do_turn_functions.end();

        if( pair.second.based_on_ == based_on_type::NEITHER && !( has_turn_func || has_actor ) ) {
            debugmsg( "%s needs a do_turn function or activity actor if it's not based on time or speed.",
                      pair.second.id_.c_str() );
        }
    }

    for( const auto &pair : activity_handlers::do_turn_functions ) {
        if( activity_type_all.find( pair.first ) == activity_type_all.end() ) {
            debugmsg( "The do_turn function %s doesn't correspond to a valid activity_type.",
                      pair.first.c_str() );
        }
    }

    for( const auto &pair : activity_handlers::finish_functions ) {
        if( activity_type_all.find( pair.first ) == activity_type_all.end() ) {
            debugmsg( "The finish_function %s doesn't correspond to a valid activity_type",
                      pair.first.c_str() );
        }
    }
}

void activity_type::call_do_turn( player_activity *act, Character *you ) const
{
    const auto &pair = activity_handlers::do_turn_functions.find( id_ );
    if( pair != activity_handlers::do_turn_functions.end() ) {
        pair->second( act, you );
    }
}

bool activity_type::call_finish( player_activity *act, Character *you ) const
{
    const auto &pair = activity_handlers::finish_functions.find( id_ );
    if( pair != activity_handlers::finish_functions.end() ) {
        pair->second( act, you );
        // kill activity sounds at finish
        sfx::end_activity_sounds();
        return true;
    }

    return false;
}

void activity_type::reset()
{
    activity_type_all.clear();
}

std::string activity_type::stop_phrase() const
{
    return string_format( _( "Stop %s?" ), verb_ );
}
