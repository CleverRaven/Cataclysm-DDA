#pragma once
#ifndef AVATAR_H
#define AVATAR_H

#include "player.h"

class avatar : public player
{
    public:
        avatar() = default;

        void store( JsonOut &json ) const;
        void load( JsonObject &data );
        void serialize( JsonOut &josn ) const;
        void deserialize( JsonIn &json );

        /** Prints out the player's memorial file */
        void memorial( std::ostream &memorial_file, const std::string &epitaph );

        // newcharacter.cpp
        bool create( character_type type, const std::string &tempname = "" );
        void randomize( bool random_scenario, points_left &points, bool play_now = false );
        bool load_template( const std::string &template_name, points_left &points );
};

#endif
