#include "character.h"

Character::Character() {
    Creature::set_speed_base(100);
};

Character::Character(const Creature &) {
    Creature::set_speed_base(100);
};

Character &Character::operator= (const Character &rhs)
{
    Creature::operator=(rhs);
    return (*this);
}

field_id Character::bloodType() const {
    return fd_blood;
}
field_id Character::gibType() const {
    return fd_gibs_flesh;
}
