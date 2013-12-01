#ifndef _NEW_NONPLAYER_H_
#define _NEW_NONPLAYER_H_

#include "character.h"

class Nonplayer : public Character, public JsonSerializer, public JsonDeserializer
{
    public:
        virtual ~Nonplayer();
        Nonplayer();
        Nonplayer(const Nonplayer &rhs);
        Nonplayer& operator= (const Nonplayer & rhs);
};

#endif
