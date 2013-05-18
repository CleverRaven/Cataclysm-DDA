#ifndef _MATERIALS_H_
#define _MATERIALS_H_

#include <string>
#include <map>

#include "enums.h"

class material_type;

typedef std::map<std::string, material_type> material_map;

class material_type
{
private:
    unsigned int _id;
    std::string _ident;    
    std::string _name;
    int _bash_resist;
    int _cut_resist;
    int _acid_resist;
    int _elec_resist;
    int _fire_resist;
    
public:
    material_type();
    material_type(unsigned int id, std::string ident, std::string name, int bash_resist, int cut_resist, int acid_resist, int elec_resist, int fire_resist);
    static material_map _all_materials;
    static material_map load_materials();

    // functions
    material_type* find_material(std::string ident);
    std::string material_type_from_tag(material mat);   
    int dam_resist(damage_type damtype) const;
};


#endif // _MATERIALS_H_
