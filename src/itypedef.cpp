#include "itype.h"
#include "ammo.h"
#include "game.h"
#include "monstergenerator.h"
#include "item_factory.h"
#include <fstream>

std::vector<std::string> artifact_itype_ids;
std::vector<std::string> standard_itype_ids;

std::map<std::string, itype*> itypes;


// Members of iuse struct, which is slowly morphing into a class.
bool itype::has_use() {
    return !use_methods.empty();
}

bool itype::can_use( std::string iuse_name ) {
    return std::find( use_methods.cbegin(), use_methods.cend(),
                      *item_controller->get_iuse( iuse_name ) ) != use_methods.cend();
}

int itype::invoke( player *p, item *it, bool active ) {
    int charges_to_use = 0;
    for( auto method = use_methods.begin();
         charges_to_use >= 0 && method != use_methods.end(); ++method ) {
        charges_to_use = method->call( p, it, active );
    }
    return charges_to_use;
}


void game::init_itypes ()
{


// First, the null object.  NOT REALLY AN OBJECT AT ALL.  More of a concept.
 itypes["null"]=
  new itype("null", 0, "none", "none", "", '#', c_white, "null", "null", PNULL, 0, 0, 0, 0, 0);
// Corpse - a special item
 itypes["corpse"]=
  new itype("corpse", 0, "corpse", "corpses", _("A dead body."), '%', c_white, "null", "null", PNULL, 0, 0,
            0, 0, 1);
 itypes["corpse"]->item_tags.insert("NO_UNLOAD");
// This must -always- be set, or bad mojo in map::drawsq and whereever we check 'typeId() == "corpse" instead of 'corpse != NULL' ....
 itypes["corpse"]->corpse=GetMType("mon_null");
// Fire - only appears in crafting recipes
 itypes["fire"]=
  new itype("fire", 0, "nearby fire", "none",
            "Some fire - if you are reading this it's a bug! (itypdef:fire)",
            '$', c_red, "null", "null", PNULL, 0, 0, 0, 0, 0);
// Integrated toolset - ditto
 itypes["toolset"]=
  new itype("toolset", 0, "integrated toolset", "none",
            "A fake item. If you are reading this it's a bug! (itypdef:toolset)",
            '$', c_red, "null", "null", PNULL, 0, 0, 0, 0, 0);
// For smoking drugs
 itypes["apparatus"]=
  new itype("apparatus", 0, "a smoking device and a source of flame", "none",
            "A fake item. If you are reading this it's a bug! (itypdef:apparatus)",
            '$', c_red, "null", "null", PNULL, 0, 0, 0, 0, 0);
// For CVD Forging
 itypes["cvd_machine"]=
  new itype("cvd_machine", 0, "cvd machine", "none",
            "A fake item. If you are reading this it's a bug! (itypdef:apparatus)",
            '$', c_red, "null", "null", PNULL, 0, 0, 0, 0, 0);

// SOFTWARE
#define SOFTWARE(id, name, name_plural, price, swtype, power, description) \
itypes[id]=new it_software(id, price, name, name_plural, description,\
    ' ', c_white, "null", "null", 0, 0, 0, 0, 0, swtype, power)

//Macguffins
#define MACGUFFIN(id, name, name_plural, price, sym, color, mat1, mat2, volume, wgt, dam, cut,\
                  to_hit, readable, function, description) \
itypes[id]=new it_macguffin(id, price, name, name_plural, description,\
    sym, color, mat1, mat2, volume, wgt, dam, cut, to_hit, readable,\
    function)

SOFTWARE("software_useless", "misc software", "none", 300, SW_USELESS, 0, _("\
A miscellaneous piece of hobby software. Probably useless."));

SOFTWARE("software_hacking", "hackPRO", "none", 800, SW_HACKING, 2, _("\
A piece of hacking software."));

SOFTWARE("software_medical", "MediSoft", "none", 600, SW_MEDICAL, 2, _("\
A piece of medical software."));

SOFTWARE("software_math", "MatheMAX", "none", 500, SW_SCIENCE, 3, _("\
A piece of mathematical software."));

SOFTWARE("software_blood_data", "infection data", "none", 200, SW_DATA, 5, _("\
Medical data on zombie blood."));

MACGUFFIN("note", "note", "notes", 0, '?', c_white, "paper", "null", 1, 3, 0, 0, 0,
    true, &iuse::mcg_note, _("\
A hand-written paper note."));

// Finally, add all the keys from the map to a vector of all possible items
for(std::map<std::string,itype*>::iterator iter = itypes.begin(); iter != itypes.end(); ++iter){
    if(iter->first == "null" || iter->first == "corpse" || iter->first == "toolset" || iter->first == "fire" || iter->first == "apparatus"){
        // pseudo item, do not add to standard_itype_ids
    } else {
        standard_itype_ids.push_back(iter->first);
    }
}
}

std::string ammo_name(std::string t)
{
    return ammunition_type::find_ammunition_type(t)->name();
}

itype_id default_ammo(std::string t)
{
    return ammunition_type::find_ammunition_type(t)->default_ammotype();
}
