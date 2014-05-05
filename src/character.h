#ifndef _CHARACTER_H_
#define _CHARACTER_H_

#include "creature.h"
#include "action.h"
#include "bodypart.h"

#include <map>

class Character : public Creature
{
    public:
        Character();
        Character(const Creature &rhs);
        Character &operator= (const Character &rhs);

        field_id bloodType();
        field_id gibType();
        
        /** Disease/effect functions */
        virtual void cough(bool harmful = false);
        virtual bool will_vomit(int chance = 1000);
        
        bool hibernating();
        virtual void manage_sleep();
        
        void add_wound(body_part bp, int side, int duration, int severity = 1, int heal_mod = 0);
        void remove_wound(body_part bp, int side,
    
    protected:
        std::vector<wound> wounds;
};

#endif
