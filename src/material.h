#pragma once
#ifndef MATERIAL_H
#define MATERIAL_H

#include <array>
#include <map>
#include <string>
#include <vector>

#include "fire.h"
#include "game_constants.h"
#include "optional.h"
#include "string_id.h"

enum damage_type : int;
class material_type;
using material_id = string_id<material_type>;
using itype_id = std::string;
class JsonObject;
class vitamin;
using vitamin_id = string_id<vitamin>;
using mat_burn_products = std::vector<std::pair<itype_id, float>>;
using mat_compacts_into = std::vector<itype_id>;
using material_list = std::vector<material_type>;
using material_id_list = std::vector<material_id>;

class material_type
{
    public:
        material_id id;
        bool was_loaded = false;

    private:
        std::string _name;
        cata::optional<itype_id> _salvaged_into; // this material turns into this item when salvaged
        itype_id _repaired_with = itype_id( "null" ); // this material can be repaired with this item
        int _bash_resist = 0;                         // negative integers means susceptibility
        int _cut_resist = 0;
        int _acid_resist = 0;
        int _elec_resist = 0;
        int _fire_resist = 0;
        int _chip_resist = 0;                         // Resistance to physical damage of the item itself
        int _density = 1;                             // relative to "powder", which is 1
        float _specific_heat_liquid = 4.186;
        float _specific_heat_solid = 2.108;
        float _latent_heat = 334;
        bool _edible = false;
        bool _soft = false;

        std::string _bash_dmg_verb;
        std::string _cut_dmg_verb;
        std::vector<std::string> _dmg_adj;

        std::map<vitamin_id, double> _vitamins;

        std::array<mat_burn_data, MAX_FIELD_DENSITY> _burn_data;

        //Burn products defined in JSON as "burn_products": [ [ "X", float efficiency ], [ "Y", float efficiency ] ]
        mat_burn_products _burn_products;

        material_id_list _compact_accepts;
        mat_compacts_into _compacts_into;

    public:
        material_type();

        void load( JsonObject &jsobj, const std::string &src );
        void check() const;

        int dam_resist( damage_type damtype ) const;

        material_id ident() const;
        std::string name() const;
        /**
         * @returns An empty optional if this material can not be
         * salvaged into any items (e.g. for powder, liquids).
         * Or a valid id of the item type that this can be salvaged
         * into (e.g. clothes made of material leather can be salvaged
         * into lather patches).
         */
        cata::optional<itype_id> salvaged_into() const;
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
        float specific_heat_liquid() const;
        float specific_heat_solid() const;
        float latent_heat() const;
        int density() const;
        bool edible() const;
        bool soft() const;

        double vitamin( const vitamin_id &id ) const {
            const auto iter = _vitamins.find( id );
            return iter != _vitamins.end() ? iter->second : 0;
        }

        const mat_burn_data &burn_data( size_t intensity ) const;
        const mat_burn_products &burn_products() const;
        const material_id_list &compact_accepts() const;
        const mat_compacts_into &compacts_into() const;
};

namespace materials
{

void load( JsonObject &jo, const std::string &src );
void check();
void reset();

material_list get_all();
material_list get_compactable();

}

#endif
