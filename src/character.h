#ifndef _CHARACTER_H_
#define _CHARACTER_H_

#include "creature.h"
#include "action.h"

#include <map>

class Character : public Creature
{
    public:
        Character();
        Character(const Creature &rhs);
        Character &operator= (const Character &rhs);

        field_id bloodType() const;
        field_id gibType() const;
        virtual bool is_warm() const override;
        virtual const std::string &symbol() const override;
};

#endif
