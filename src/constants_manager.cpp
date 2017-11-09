#include "constants_manager.h"

#include "debug.h"

#include <map>
#include <algorithm>


constants_manager &get_constants()
{
    static constants_manager single_instance;
    return single_instance;
}


constants_manager::game_constant &constants_manager::get_constant( const std::string &id )
{
    static game_constant null_constant;
    auto iter = game_constants_all.find(id);
    return iter != game_constants_all.end() ? iter->second : null_constant;
}

template<>
std::string constants_manager::game_constant::value_as<std::string>() const
{
    if( stype_ != "string" ) {
        debugmsg( "%s tried to get string value from constant of type %s" , id_.c_str(), stype_.c_str() );
    }
    return  string_value_;
}

template<>
bool constants_manager::game_constant::value_as<bool>() const
{
    if( stype_ != "bool" ) {
        debugmsg("%s tried to get bool value from constant of type %s", id_.c_str(), stype_.c_str());
    }
    
    return bool_value_;
}

template<>
float constants_manager::game_constant::value_as<float>() const
{
    if( stype_ != "float" ) {
        debugmsg("%s tried to get float value from constant of type %s", id_.c_str(), stype_.c_str());
    }

    return float_value_;
}

template<>
int constants_manager::game_constant::value_as<int>() const
{
    if( stype_ != "int" ) {
        debugmsg("%s tried to get int value from constant of type %s", id_.c_str(), stype_.c_str());
    }
    
    return int_value_;
}


void constants_manager::load(JsonObject &jo, const std::string &src)
{
    auto id = jo.get_string("id");
    auto &f = get_constants().game_constants_all.emplace(id, game_constant(id)).first->second;
    f.load( jo, src );
}


void constants_manager::game_constant::load(JsonObject &jo, const std::string &src)
{
    jo.read( "description", description_ );
    jo.read( "stype", stype_ );
    if ( stype_ == "float" )
        jo.read( "value", float_value_ );
    else if ( stype_ == "int" )
        jo.read( "value", int_value_ );
    else if ( stype_ == "bool" )
        jo.read( "value", bool_value_ );
    else if ( stype_ == "string" )
        jo.read( "value", string_value_ );
    else
        debugmsg( "Unknown stype for %s.", id_.c_str() );
}
