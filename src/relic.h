#pragma once
#ifndef CATA_SRC_RELIC_H
#define CATA_SRC_RELIC_H

#include <string>
#include <vector>

#include "magic.h"
#include "magic_enchantment.h"
#include "translations.h"

class Creature;
class JsonIn;
class JsonObject;
class JsonOut;
struct tripoint;

class relic
{
    private:
        std::vector<fake_spell> active_effects;
        std::vector<enchantment> passive_effects;

        // the item's name will be replaced with this if the string is not empty
        translation item_name_override;

        int charges_per_activation;
        // activating an artifact overrides all spell casting costs
        int moves;
    public:
        std::string name() const;
        // returns number of charges that should be consumed
        int activate( Creature &caster, const tripoint &target ) const;

        void load( const JsonObject &jo );

        void serialize( JsonOut &jsout ) const;
        void deserialize( JsonIn &jsin );

        void add_passive_effect( const enchantment &ench );
        void add_active_effect( const fake_spell &sp );

        std::vector<enchantment> get_enchantments() const;

        int modify_value( enchantment::mod value_type, int value ) const;
};

#endif // CATA_SRC_RELIC_H
