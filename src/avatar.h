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
};

#endif
