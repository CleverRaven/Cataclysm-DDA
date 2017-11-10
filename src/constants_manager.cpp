#include "constants_manager.h"

#include "debug.h"

#include <map>
#include <algorithm>


constants_manager &get_constants_manager()
{
    static constants_manager single_instance;
    return single_instance;
}


constants_manager::game_constant &constants_manager::get_constant( const std::string &id )
{
    static game_constant null_constant;
    auto iter = game_constants_all.find( id );
    return iter != game_constants_all.end() ? iter->second : null_constant;
}

template<>
std::string constants_manager::game_constant::value_as<std::string>() const
{
    if( stype_ != "string" ) {
        debugmsg( "%s tried to get string value from constant of type %s", id_.c_str(), stype_.c_str() );
    }
    return  string_value_;
}

template<>
bool constants_manager::game_constant::value_as<bool>() const
{
    if( stype_ != "bool" ) {
        debugmsg( "%s tried to get bool value from constant of type %s", id_.c_str(), stype_.c_str() );
    }

    return bool_value_;
}

template<>
float constants_manager::game_constant::value_as<float>() const
{
    if( stype_ != "float" ) {
        debugmsg( "%s tried to get float value from constant of type %s", id_.c_str(), stype_.c_str() );
    }

    return float_value_;
}

template<>
int constants_manager::game_constant::value_as<int>() const
{
    if( stype_ != "int" ) {
        debugmsg( "%s tried to get int value from constant of type %s", id_.c_str(), stype_.c_str() );
    }

    return int_value_;
}


void constants_manager::load( JsonObject &jo )
{
    auto id = jo.get_string( "id" );
    auto &f = get_constants_manager().game_constants_all.emplace( id,
              game_constant( id ) ).first->second;
    f.load( jo );
}


void constants_manager::game_constant::load( JsonObject &jo )
{
    jo.read( "description", description_ );
    stype_ = jo.get_string( "stype" );
    if( stype_ == "float" ) {
        float_value_ = jo.get_float( "value" );
    } else if( stype_ == "int" ) {
        int_value_ = jo.get_int( "value" );
    } else if( stype_ == "bool" ) {
        bool_value_ = jo.get_bool( "value" );
    } else if( stype_ == "string" ) {
        string_value_ = jo.get_string( "value" );
    } else {
        debugmsg( "Unknown stype for %s.", id_.c_str() );
    }
}
