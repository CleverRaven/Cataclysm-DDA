#ifndef _MATERIALS_H_
#define _MATERIALS_H_

#include <string>
#include <map>

#include "enums.h"
#include "json.h"

class material_type;

typedef std::map<std::string, material_type> material_map;

class material_type
{
private:
    std::string _ident;
    std::string _name;
    int _bash_resist;       // negative integers means susceptibility
    int _cut_resist;
    std::string _bash_dmg_verb;
    std::string _cut_dmg_verb;
    std::string _dmg_adj[4];
    int _acid_resist;
    int _elec_resist;
    int _fire_resist;
    int _density;   // relative to "powder", which is 1

public:
    material_type();
    material_type(std::string ident, std::string name,
                  int bash_resist, int cut_resist,
                  std::string bash_dmg_verb, std::string cut_dmg_verb,
                  std::string dmg_adj[], int acid_resist, int elec_resist, int fire_resist,
                  int density);
    material_type(std::string ident);
    static material_map _all_materials;
    static void load_material(JsonObject &jsobj);

    // functions
    static material_type* find_material(std::string ident);
//  static material_type* find_material_from_tag(material mat);
    static material_type* base_material();  // null material

    int dam_resist(damage_type damtype) const;

    bool is_null() const;
    std::string ident() const;
    std::string name() const;
    int bash_resist() const;
    int cut_resist() const;
    std::string bash_dmg_verb() const;
    std::string cut_dmg_verb() const;
    std::string dmg_adj(int dam) const;
    int acid_resist() const;
    int elec_resist() const;
    int fire_resist() const;
    int density() const;
};


#endif // _MATERIALS_H_
