#ifndef _CHARACTER_H_
#define _CHARACTER_H_

#include "creature.h"
#include "action.h"
#include "bodypart.h"
#include "wound.h"

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
        
        void add_wound(body_part target, int nside, int sev = 1,
                std::vector<wefftype_id> wound_effs = std::vector<wefftype_id>());
        void remove_wounds(body_part target, int nside, int sev = 0,
                std::vector<wefftype_id> wound_effs = std::vector<wefftype_id>());
                
         /** Draws the UI and handles player input for the healing window */
         void treatment();
    
    protected:
        std::vector<wound> wounds;
};

#endif
