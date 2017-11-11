#include "constants_manager.h"

#include "debug.h"

#include <map>
#include <algorithm>
#include <unordered_map>


constants_manager &get_constants_manager()
{
    static constants_manager single_instance;
    return single_instance;
}


const constants_manager::game_constant &constants_manager::get_constant( const std::string &id )
{
    static game_constant null_constant;
    auto iter = game_constants_all.find( id );
    return iter != game_constants_all.end() ? iter->second : null_constant;
}

static const std::unordered_map< std::string, constants_manager::game_constant_type > game_constant_type_values = {
	{ "float", constants_manager::game_constant_type::FLOAT },
	{ "int", constants_manager::game_constant_type::INT },
	{ "bool", constants_manager::game_constant_type::BOOL },
	{ "string", constants_manager::game_constant_type::STRING }
};

const std::string enum_to_string(constants_manager::game_constant_type data)
{
	const auto iter = std::find_if(game_constant_type_values.begin(), game_constant_type_values.end(),
		[data](const std::pair<std::string, constants_manager::game_constant_type> &pr) {
		return pr.second == data;
	});

	if (iter == game_constant_type_values.end()) {
		throw io::InvalidEnumString{};
	}

	return iter->first;
}

template<>
std::string constants_manager::game_constant::value_as<std::string>() const
{
    if( type_ != STRING ) {
        debugmsg( "Tried to get string value from constant %s of type %s", id_.c_str(), enum_to_string(type_).c_str());
    }
    return  string_value_;
}

template<>
bool constants_manager::game_constant::value_as<bool>() const
{
    if( type_ != BOOL ) {
        debugmsg( "Tried to get bool value from constant %s of type %s",  id_.c_str(), enum_to_string(type_).c_str());
    }

    return bool_value_;
}

template<>
float constants_manager::game_constant::value_as<float>() const
{
    if( type_ != FLOAT ) {
        debugmsg( "Tried to get float value from constant %s of type %s", id_.c_str(), enum_to_string(type_).c_str());
    }

    return float_value_;
}

template<>
int constants_manager::game_constant::value_as<int>() const
{
    if( type_ != INT ) {
        debugmsg( "Tried to get int value from constant %s of type %s", id_.c_str(), enum_to_string(type_).c_str());
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
	auto stype = jo.get_string( "stype" );
	type_ = io::string_to_enum_look_up( game_constant_type_values, stype );

    if( type_ == FLOAT) {
        float_value_ = jo.get_float( "value" );
    } else if( type_ == INT ) {
        int_value_ = jo.get_int( "value" );
    } else if( type_ == BOOL ) {
        bool_value_ = jo.get_bool( "value" );
    } else if( type_ == STRING ) {
        string_value_ = jo.get_string( "value" );
    } else {
        jo.throw_error( "Unknown stype", "stype" );
    }
}


