#include "clothing_layer.h"

#include "json.h"

#include <map>

std::map<std::string, clothing_layer> clothing_layers_all;
std::map<layer_level, clothing_layer*> clothing_layers_levels;

const clothing_layer & clothing_layer::get( const std::string & id )
{
	static clothing_layer null_layer;
	auto iter = clothing_layers_all.find( id );
	return iter != clothing_layers_all.end() ? iter->second : null_layer;
}

const clothing_layer & clothing_layer::get( const layer_level & level )
{
	static clothing_layer null_layer;
	auto iter = clothing_layers_levels.find( level );
	return iter != clothing_layers_levels.end() ? *(iter->second) : null_layer;
}

void clothing_layer::load( JsonObject & jo )
{
	auto id = jo.get_string( "id" );
	auto &l = clothing_layers_all.emplace( id, clothing_layer( id ) ).first->second;

	l._level = jo.get_enum_value<layer_level>( "layer" );
	l._color = color_from_string( jo.get_string( "color" ) );
	l._symbol = jo.get_string( "symbol" )[0];
	clothing_layers_levels.emplace(l._level, &l);
}

void clothing_layer::reset()
{
	clothing_layers_all.clear();
	clothing_layers_levels.clear();
}

void clothing_layer::check_consistency() {
	for (int i = 0; i < static_cast<int>( layer_level::MAX_CLOTHING_LAYER ); i++) {
        if (clothing_layers_levels.find(static_cast<layer_level>(i)) == clothing_layers_levels.end())
			debugmsg( "layer %i has no clothing layer properties defined", i );		
	}

	for (auto it : clothing_layers_all) {
		if (it.second._color == 0) {
			debugmsg( "clothing layer %s has an unrecognized color", it.first );
		}
	}
}

const nc_color& clothing_layer::color() const {
	return _color;
}

const char& clothing_layer::symbol() const {
	return _symbol;
}

bool clothing_layer::operator==( const clothing_layer & o ) const
{
    return this->_id == o._id;
}
