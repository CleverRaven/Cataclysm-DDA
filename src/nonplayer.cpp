#include "nonplayer.h"

Nonplayer::~Nonplayer() { }
Nonplayer::Nonplayer() : Character()
{
}

Nonplayer::Nonplayer(const Nonplayer &rhs) : Character(), JsonSerializer(), JsonDeserializer()
{
    *this = rhs;
}


Nonplayer &Nonplayer::operator= (const Nonplayer &rhs)
{
    Character::operator=(rhs);
    return (*this);
}
