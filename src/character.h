#ifndef _DUDE_H_
#define _DUDE_H_

#include "creature.h"

class Character : public creature
{
    public:
        Character();
        Character(const creature & rhs);
};

#endif
