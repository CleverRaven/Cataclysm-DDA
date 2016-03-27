#ifndef MATERIALS_H
#define MATERIALS_H

#include <string>
#include <map>

#include "game_constants.h"
#include "damage.h" // damage_type
#include "enums.h"
#include "json.h"
#include "string_id.h"

class material_type;
using material_id = string_id<material_type>;

typedef std::map<material_id, material_type> material_map;

class material_type
{
    private:
        material_id _ident;
        std::string _name;
        std::string _salvage_id; // this material turns into this item when salvaged
        float _salvage_multiplier; // multiplier when salvaging.
        int _bash_resist;       // negative integers means susceptibility
        int _cut_resist;
        std::string _bash_dmg_verb;
        std::string _cut_dmg_verb;
        std::string _dmg_adj[MAX_ITEM_DAMAGE];
        int _acid_resist;
        int _elec_resist;
        int _fire_resist;
        int _chip_resist;       // Resistance to physical damage of the item itself
        int _density;   // relative to "powder", which is 1

        static material_map _all_materials;

    public:
        material_type();
        material_type( material_id ident );
        static void load_material( JsonObject &jsobj );

        // functions
        static material_type *find_material( material_id ident );
        //  static material_type* find_material_from_tag(material mat);
        static material_type *base_material();  // null material
        static bool has_material( const material_id &ident );
        // clear material map, every material pointer becames invalid!
        static void reset();

        int dam_resist( damage_type damtype ) const;

        bool is_null() const;
        material_id ident() const;
        std::string name() const;
        std::string salvage_id() const;
        float salvage_multiplier() const;
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
};

#endif
