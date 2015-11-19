#ifndef NEW_NONPLAYER_H
#define NEW_NONPLAYER_H

#include "character.h"

class Nonplayer : public Character, public JsonSerializer, public JsonDeserializer
{
    public:
        virtual ~Nonplayer();
        Nonplayer();
        Nonplayer( const Nonplayer &rhs );
        Nonplayer &operator= ( const Nonplayer &rhs );
};

#endif
