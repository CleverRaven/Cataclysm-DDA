#ifndef MATERIALS_H
#define MATERIALS_H

#include <string>
#include <array>

#include "game_constants.h"
#include "damage.h" // damage_type
#include "enums.h"
#include "json.h"
#include "string_id.h"
#include "vitamin.h"
#include "fire.h"

class material_type;
using material_id = string_id<material_type>;
using itype_id = std::string;

class material_type
{
    public:
        material_id id;
        bool was_loaded = false;

    private:
        std::string _name;
        itype_id _salvaged_into = itype_id( "null" ); // this material turns into this item when salvaged
        itype_id _repaired_with = itype_id( "null" ); // this material can be repaired with this item
        int _bash_resist = 0;                         // negative integers means susceptibility
        int _cut_resist = 0;
        int _acid_resist = 0;
        int _elec_resist = 0;
        int _fire_resist = 0;
        int _chip_resist = 0;                         // Resistance to physical damage of the item itself
        int _density = 1;                             // relative to "powder", which is 1
        bool _edible = false;
        bool _soft = false;

        std::string _bash_dmg_verb;
        std::string _cut_dmg_verb;
        std::vector<std::string> _dmg_adj;

        std::map<vitamin_id, double> _vitamins;

        std::array<mat_burn_data, MAX_FIELD_DENSITY> _burn_data;

    public:
        material_type();

        void load( JsonObject &jo );
        void check() const;

        int dam_resist( damage_type damtype ) const;

        material_id ident() const;
        std::string name() const;
        itype_id salvaged_into() const;
        itype_id repaired_with() const;
        int bash_resist() const;
        int cut_resist() const;
        std::string bash_dmg_verb() const;
        std::string cut_dmg_verb() const;
        std::string dmg_adj( int damage ) const;
        int acid_resist() const;
        int elec_resist() const;
        int fire_resist() const;
        int chip_resist() const;
        int density() const;
        bool edible() const;
        bool soft() const;

        double vitamin( const vitamin_id &id ) const {
            auto iter = _vitamins.find( id );
            return iter != _vitamins.end() ? iter->second : 0;
        }

        const mat_burn_data &burn_data( size_t intensity ) const;
};

namespace materials
{

void load( JsonObject &jo );
void check();
void reset();

}

#endif
