#pragma once
#ifndef CLOTHING_LAYER_H
#define CLOTHING_LAYER_H

#include "color.h"
#include "item.h"

#include <string>

class JsonObject;

using itype_id = std::string;

class clothing_layer
{
        friend class DynamicDataLoader;
    public:

		static const clothing_layer& get( const std::string &id );
		static const clothing_layer& get( const layer_level &leval );

		const nc_color& color() const;
		const char& symbol() const;

		bool operator ==( const clothing_layer &o ) const;
    private:
        const std::string _id;
        char _symbol;
        nc_color _color;
        layer_level _level;

        clothing_layer( const std::string &id = std::string() ) : _id( id ) {}

        static void load( JsonObject &jo );
		static void check_consistency();
        static void reset();

};

#endif
