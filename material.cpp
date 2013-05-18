#include <iostream>
#include <fstream>
#include <sstream>

#include "catajson.h"
#include "material.h"
#include "enums.h"
#include "game.h"

material_type::material_type()
{
    _id = 0;
    _ident = "null";
    _name = "null";
    _bash_resist = 0;
    _cut_resist = 0;
    _acid_resist = 0;
    _elec_resist = 0;
    _fire_resist = 0;
}

material_type::material_type(unsigned int id, std::string ident, std::string name, int bash_resist, int cut_resist, int acid_resist, int elec_resist, int fire_resist)
{
    _id = id;
    _ident = ident;
    _name = name;
    _bash_resist = bash_resist;
    _cut_resist = cut_resist;
    _acid_resist = acid_resist;
    _elec_resist = elec_resist;
    _fire_resist = fire_resist;
}

void game::init_materials()
{
    catajson materialsRaw("data/raw/materials.json");

    unsigned int id = 0;
    for (materialsRaw.set_begin(); materialsRaw.has_curr(); materialsRaw.next())
    {
        ++id;
        catajson currMaterial = materialsRaw.curr();
        std::string ident = currMaterial.get("ident").as_string();        
        std::string name = currMaterial.get("name").as_string();
        int bash_resist = currMaterial.get("bash_resist").as_int();
        int cut_resist = currMaterial.get("cut_resist").as_int();
        int acid_resist = currMaterial.get("acid_resist").as_int();
        int elec_resist = currMaterial.get("elec_resist").as_int();
        int fire_resist = currMaterial.get("fire_resist").as_int();

        material_type newMaterial(id, ident, name, bash_resist, cut_resist, acid_resist, elec_resist, fire_resist);

        materials[ident] = newMaterial;
    }
}


unsigned int material_type::id() const
{
    return _id;
}

int material_type::dam_resist(damage_type damtype) const
{
    switch (damtype)
    {
        case BASH:
            return _bash_resist;
            break;
        case CUT:
            return _cut_resist;
            break;
        case ACID:
            return _acid_resist;
            break;
        case ELECTRICITY:
            return _elec_resist;
            break;
        case FIRE:
            return _fire_resist;
            break;
        default:
            return 0;
            break;
    }
}
