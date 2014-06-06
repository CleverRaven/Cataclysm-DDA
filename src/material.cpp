#include "material.h"

#include "output.h" // debugmsg
#include "damage.h" // damage_type
#include "json.h"
#include "translations.h"

#include <string>

material_type::material_type()
{
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

material_type::material_type(std::string ident, std::string name,
                             int bash_resist, int cut_resist,
                             std::string bash_dmg_verb, std::string cut_dmg_verb,
                             std::string dmg_adj[], int acid_resist, int elec_resist, int fire_resist,
                             int density)
{
    _ident = ident;
    _name = name;
    _bash_resist = bash_resist;
    _cut_resist = cut_resist;
    _bash_dmg_verb = bash_dmg_verb;
    _cut_dmg_verb = cut_dmg_verb;
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
    _ident = ident;
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

// load a material object from incoming JSON
void material_type::load_material(JsonObject &jsobj)
{
    material_type mat;

    mat._ident = jsobj.get_string("ident");
    mat._name = _(jsobj.get_string("name").c_str());
    mat._bash_resist = jsobj.get_int("bash_resist");
    mat._cut_resist = jsobj.get_int("cut_resist");
    mat._bash_dmg_verb = _(jsobj.get_string("bash_dmg_verb").c_str());
    mat._cut_dmg_verb = _(jsobj.get_string("cut_dmg_verb").c_str());
    mat._acid_resist = jsobj.get_int("acid_resist");
    mat._elec_resist = jsobj.get_int("elec_resist");
    mat._fire_resist = jsobj.get_int("fire_resist");
    mat._density = jsobj.get_int("density");

    JsonArray jsarr = jsobj.get_array("dmg_adj");
    mat._dmg_adj[0] = _(jsarr.next_string().c_str());
    mat._dmg_adj[1] = _(jsarr.next_string().c_str());
    mat._dmg_adj[2] = _(jsarr.next_string().c_str());
    mat._dmg_adj[3] = _(jsarr.next_string().c_str());

    _all_materials[mat._ident] = mat;
    //dout(D_INFO) << "Loaded material: " << mat._name;
}

material_type* material_type::find_material(std::string ident)
{
    material_map::iterator found = _all_materials.find(ident);
    if(found != _all_materials.end()){
        return &(found->second);
    } else {
        debugmsg("Tried to get invalid material: %s", ident.c_str());
        static material_type null_material;
        return &null_material;
    }
}

void material_type::reset()
{
    _all_materials.clear();
}

bool material_type::has_material(const std::string &ident)
{
    return _all_materials.count(ident) > 0;
}

material_type* material_type::base_material()
{
    return material_type::find_material("null");
}

int material_type::dam_resist(damage_type damtype) const
{
    switch (damtype)
    {
        case DT_BASH:
            return _bash_resist;
            break;
        case DT_CUT:
            return _cut_resist;
            break;
        case DT_ACID:
            return _acid_resist;
            break;
        case DT_ELECTRIC:
            return _elec_resist;
            break;
        case DT_HEAT:
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
