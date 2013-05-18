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

material_map material_type::_all_materials(material_type::load_materials());

material_map material_type::load_materials()
{
    material_map allMaterials;
    
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

        allMaterials[ident] = newMaterial;
    }
    return allMaterials;
}

material_type* material_type::find_material(std::string ident)
{
    material_map::iterator found = _all_materials.find(ident);
    if(found != _all_materials.end()){
        return &(found->second);
    }
    else
    {
        debugmsg("Tried to get invalid material: %s", ident.c_str());
        return NULL;
    }   
}

std::string material_type_from_tag(material mat)
{
    switch(mat)
    {
        case MNULL:     return "null";  break;
        case VEGGY:     return "veggy"; break;
        case FLESH:     return "flesh"; break;
        case POWDER:    return "powder"; break;
        case HFLESH:    return "hflesh"; break;
        case COTTON:    return "cotton"; break;
        case WOOL:      return "wool"; break;
        case LEATHER:   return "leather"; break;
        case KEVLAR:    return "kevlar"; break;
        case FUR:       return "fur"; break;
        case CHITIN:    return "chitin"; break;
        case STONE:     return "stone"; break;
        case PAPER:     return "paper"; break;
        case WOOD:      return "wood"; break;
        case PLASTIC:   return "plastic"; break;
        case GLASS:     return "glass"; break;
        case IRON:      return "iron"; break;
        case STEEL:     return "steel"; break;
        case SILVER:    return "silver"; break;
        default:        return "null"; break;
    }
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
