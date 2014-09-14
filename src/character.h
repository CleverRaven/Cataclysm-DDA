#ifndef _CHARACTER_H_
#define _CHARACTER_H_

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
