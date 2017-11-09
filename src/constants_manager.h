#pragma once
#ifndef CONSTANTS_MANAGER_H
#define CONSTANTS_MANAGER_H

#include "json.h"

#include <set>

/** Constansts manager. To read constant use something like:
float val = get_constant<float>( "FLOAT_CONSTANT_EXAMPLE" );
get_constant() avalible statically*/
class constants_manager
{
        friend class DynamicDataLoader;


    private:
        /** Load constants from JSON */
        static void load( JsonObject &jo );

    public:
        class game_constant
        {
            public:
                template<typename T>
                T value_as() const;
                game_constant( const std::string &id = std::string() ) : id_( id ) {}

                /** Load constant definition from JSON */
                void load( JsonObject &jo );

            private:
                const std::string id_;
                std::string description_;
                std::string stype_;
                std::string string_value_;
                float float_value_;
                int int_value_;
                bool bool_value_;


        };

        game_constant &get_constant( const std::string &id );

        std::map<std::string, game_constant> game_constants_all;
};

constants_manager &get_constants_manager();

template<typename T>
inline T get_constant( const std::string &name )
{
    return get_constants_manager().get_constant( name ).value_as<T>();
}


#endif
