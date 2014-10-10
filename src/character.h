#ifndef CHARACTER_H
#define CHARACTER_H

#include "creature.h"
#include "action.h"

#include <map>

class Character : public Creature
{
    public:
        virtual ~Character() override;

        field_id bloodType() const;
        field_id gibType() const;
        virtual bool is_warm() const override;
        virtual const std::string &symbol() const override;
        
        virtual void add_effect(efftype_id eff_id, int dur, body_part bp = num_bp, int intensity = 1,
                        bool permanent = false);
                        
    protected:
        Character();
        Character(const Character &) = default;
        Character(Character &&) = default;
        Character &operator=(const Character &) = default;
        Character &operator=(Character &&) = default;

        void store(JsonOut &jsout) const;
        void load(JsonObject &jsin);
};

#endif
