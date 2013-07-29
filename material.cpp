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
    _bash_dmg_verb = _("damages");
    _cut_dmg_verb = _("damages");
    _dmg_adj[0] = _("lightly damaged");
    _dmg_adj[1] = _("damaged");
    _dmg_adj[2] = _("very damaged");
    _dmg_adj[3] = _("thoroughly damaged");
    _acid_resist = 0;
    _elec_resist = 0;
    _fire_resist = 0;
    _density = 1;
}

material_type::material_type(unsigned int id, std::string ident, std::string name,
                             int bash_resist, int cut_resist,
                             std::string bash_dmg_verb, std::string cut_dmg_verb,
                             std::string dmg_adj[], int acid_resist, int elec_resist, int fire_resist,
                             int density)
{
    _id = id;
    _ident = ident;
    _name = name;
    _bash_resist = bash_resist;
    _cut_resist = cut_resist;
    _bash_dmg_verb = bash_dmg_verb;
    _cut_dmg_verb = bash_dmg_verb;
    _dmg_adj[0] = dmg_adj[0];
    _dmg_adj[1] = dmg_adj[1];
    _dmg_adj[2] = dmg_adj[2];
    _dmg_adj[3] = dmg_adj[3];        
    _acid_resist = acid_resist;
    _elec_resist = elec_resist;
    _fire_resist = fire_resist;
    _density = density;
}

material_type::material_type(std::string ident)
{
    material_type* mat_type = find_material(ident);
    _id = mat_type->id();
    _name = mat_type->name();
    _bash_resist = mat_type->bash_resist();
    _cut_resist = mat_type->cut_resist();
    _bash_dmg_verb = mat_type->bash_dmg_verb();
    _cut_dmg_verb = mat_type->bash_dmg_verb();
    _dmg_adj[0] = mat_type->dmg_adj(1);
    _dmg_adj[1] = mat_type->dmg_adj(2);
    _dmg_adj[2] = mat_type->dmg_adj(3);
    _dmg_adj[3] = mat_type->dmg_adj(4); 
    _acid_resist = mat_type->acid_resist();
    _elec_resist = mat_type->elec_resist();
    _fire_resist = mat_type->fire_resist();
    _density = mat_type->density();
}

material_map material_type::_all_materials;

void game::init_materials()
{
    material_type::_all_materials = material_type::load_materials();
}

material_map material_type::load_materials()
{
    material_map allMaterials;

    catajson materialsRaw("data/raw/materials.json", true);

    unsigned int id = 0;
    for (materialsRaw.set_begin(); materialsRaw.has_curr() && json_good(); materialsRaw.next())
    {
        ++id;
        catajson currMaterial = materialsRaw.curr();
        std::string ident = currMaterial.get("ident").as_string();
        std::string name = currMaterial.get("name").as_string();
        int bash_resist = currMaterial.get("bash_resist").as_int();
        int cut_resist = currMaterial.get("cut_resist").as_int();
        std::string bash_dmg_verb = currMaterial.get("bash_dmg_verb").as_string();
        std::string cut_dmg_verb = currMaterial.get("cut_dmg_verb").as_string();
        int acid_resist = currMaterial.get("acid_resist").as_int();
        int elec_resist = currMaterial.get("elec_resist").as_int();
        int fire_resist = currMaterial.get("fire_resist").as_int();
        int density = currMaterial.get("density").as_int();

        catajson adjList = currMaterial.get("dmg_adj");
        std::string dmg_adj[4];
        dmg_adj[0] = adjList.get(0).as_string();
        dmg_adj[1] = adjList.get(1).as_string();
        dmg_adj[2] = adjList.get(2).as_string();
        dmg_adj[3] = adjList.get(3).as_string();

        name = _(name.c_str());
        bash_dmg_verb = _(bash_dmg_verb.c_str());
        cut_dmg_verb = _(cut_dmg_verb.c_str());
        dmg_adj[0] = _(dmg_adj[0].c_str());
        dmg_adj[1] = _(dmg_adj[1].c_str());
        dmg_adj[2] = _(dmg_adj[2].c_str());
        dmg_adj[3] = _(dmg_adj[3].c_str());

        material_type newMaterial(id, ident, name, bash_resist, cut_resist, bash_dmg_verb,
                                  cut_dmg_verb, dmg_adj, acid_resist, elec_resist, fire_resist, density);

        allMaterials[ident] = newMaterial;
    }

    if(!json_good())
        exit(1);

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

material_type* material_type::base_material()
{
    return material_type::find_material("null");
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

bool material_type::is_null() const
{
    return (_ident == "null");
}

unsigned int material_type::id() const
{
    return _id;
}

std::string material_type::ident() const
{
    return _ident;
}

std::string material_type::name() const
{
    return _name;
}

int material_type::bash_resist() const
{
    return _bash_resist;
}

int material_type::cut_resist() const
{
    return _cut_resist;
}

std::string material_type::bash_dmg_verb() const
{
    return _bash_dmg_verb;
}

std::string material_type::cut_dmg_verb() const
{
    return _cut_dmg_verb;
}

std::string material_type::dmg_adj(int dam) const
{
    int tmpdam = dam - 1;
    // bounds check
    if (tmpdam < 0 || tmpdam >= 4)
        return "";
    
    return _dmg_adj[tmpdam];
}

int material_type::acid_resist() const
{
    return _acid_resist;
}

int material_type::elec_resist() const
{
    return _elec_resist;
}

int material_type::fire_resist() const
{
    return _fire_resist;
}

int material_type::density() const
{
    return _density;
}
