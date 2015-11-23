#ifndef MATERIALS_H
#define MATERIALS_H

#include <string>
#include <map>

#include "damage.h" // damage_type
#include "enums.h"
#include "json.h"

class material_type;

typedef std::map<std::string, material_type> material_map;

class material_type
{
    private:
        std::string _ident;
        std::string _name;
        std::string _salvage_id; // this material turns into this item when salvaged
        float _salvage_multiplier; // multiplier when salvaging.
        int _bash_resist;       // negative integers means susceptibility
        int _cut_resist;
        std::string _bash_dmg_verb;
        std::string _cut_dmg_verb;
        std::string _dmg_adj[4];
        int _acid_resist;
        int _elec_resist;
        int _fire_resist;
        int _chip_resist;       // Resistance to physical damage of the item itself
        int _density;   // relative to "powder", which is 1

        static material_map _all_materials;

    public:
        material_type();
        material_type( std::string ident, std::string name,
                       std::string salvage_id, float salvage_multiplier,
                       int bash_resist, int cut_resist,
                       std::string bash_dmg_verb, std::string cut_dmg_verb,
                       std::string dmg_adj[],
                       int acid_resist, int elec_resist, int fire_resist,
                       int chip_resist, int density );
        material_type( std::string ident );
        static void load_material( JsonObject &jsobj );

        // functions
        static material_type *find_material( std::string ident );
        //  static material_type* find_material_from_tag(material mat);
        static material_type *base_material();  // null material
        static bool has_material( const std::string &ident );
        // clear material map, every material pointer becames invalid!
        static void reset();

        int dam_resist( damage_type damtype ) const;

        bool is_null() const;
        std::string ident() const;
        std::string name() const;
        std::string salvage_id() const;
        float salvage_multiplier() const;
        int bash_resist() const;
        int cut_resist() const;
        std::string bash_dmg_verb() const;
        std::string cut_dmg_verb() const;
        std::string dmg_adj( int dam ) const;
        int acid_resist() const;
        int elec_resist() const;
        int fire_resist() const;
        int chip_resist() const;
        int density() const;
};

#endif
