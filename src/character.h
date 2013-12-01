#ifndef _DUDE_H_
#define _DUDE_H_

#include "creature.h"

class Character : public Creature
{
    public:
        Character();
        Character(const Creature & rhs);
};

#endif
