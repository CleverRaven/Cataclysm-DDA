#include "itype.h"
#include "game.h"
#include "setvector.h"
#include "monstergenerator.h"
#include <fstream>

// Armor colors
#define C_SHOES  c_blue
#define C_PANTS  c_brown
#define C_BODY   c_yellow
#define C_TORSO  c_ltred
#define C_ARMS   c_blue
#define C_GLOVES c_ltblue
#define C_MOUTH  c_white
#define C_EYES   c_cyan
#define C_HAT    c_dkgray
#define C_STORE  c_green
#define C_DECOR  c_ltgreen

// Special function for setting melee techniques
#define TECH(id, t) itypes[id]->techniques = t

std::vector<std::string> unreal_itype_ids;
std::vector<std::string> martial_arts_itype_ids;
std::vector<std::string> artifact_itype_ids;
std::vector<std::string> standard_itype_ids;
std::vector<std::string> pseudo_itype_ids;

std::map<std::string, itype*> itypes;

// GENERAL GUIDELINES
// When adding a new item, you MUST REMEMBER to insert it in the itype_id enum
//  at the top of itype.h!
//  Additionally, you should check mapitemsdef.cpp and insert the new item in
//  any appropriate lists.
void game::init_itypes ()
{
// First, the null object.  NOT REALLY AN OBJECT AT ALL.  More of a concept.
 itypes["null"]=
  new itype("null", 0, "none", "", '#', c_white, "null", "null", PNULL, 0, 0, 0, 0, 0);
// Corpse - a special item
 itypes["corpse"]=
  new itype("corpse", 0, _("corpse"), _("A dead body."), '%', c_white, "null", "null", PNULL, 0, 0,
            0, 0, 1);
 itypes["corpse"]->item_tags.insert("NO_UNLOAD");
// This must -always- be set, or bad mojo in map::drawsq and whereever we check 'typeId() == "corpse" instead of 'corpse != NULL' ....
 itypes["corpse"]->corpse=GetMType("mon_null");
// Fire - only appears in crafting recipes
 itypes["fire"]=
  new itype("fire", 0, _("nearby fire"),
            "Some fire - if you are reading this it's a bug! (itypdef:fire)",
            '$', c_red, "null", "null", PNULL, 0, 0, 0, 0, 0);
// Integrated toolset - ditto
 itypes["toolset"]=
  new itype("toolset", 0, _("integrated toolset"),
            "A fake item. If you are reading this it's a bug! (itypdef:toolset)",
            '$', c_red, "null", "null", PNULL, 0, 0, 0, 0, 0);
// For smoking drugs
 itypes["apparatus"]=
  new itype("apparatus", 0, _("a smoking device and a source of flame"),
            "A fake item. If you are reading this it's a bug! (itypdef:apparatus)",
            '$', c_red, "null", "null", PNULL, 0, 0, 0, 0, 0);

// SOFTWARE
#define SOFTWARE(id, name, price, swtype, power, description) \
itypes[id]=new it_software(id, price, name, description,\
    ' ', c_white, "null", "null", 0, 0, 0, 0, 0, swtype, power)

//Macguffins
#define MACGUFFIN(id, name, price, sym, color, mat1, mat2, volume, wgt, dam, cut,\
                  to_hit, readable, function, description) \
itypes[id]=new it_macguffin(id, price, name, description,\
    sym, color, mat1, mat2, volume, wgt, dam, cut, to_hit, readable,\
    function)

SOFTWARE("software_useless", _("misc software"), 300, SW_USELESS, 0, _("\
A miscellaneous piece of hobby software. Probably useless."));

SOFTWARE("software_hacking", _("hackPRO"), 800, SW_HACKING, 2, _("\
A piece of hacking software."));

SOFTWARE("software_medical", _("MediSoft"), 600, SW_MEDICAL, 2, _("\
A piece of medical software."));

SOFTWARE("software_math", _("MatheMAX"), 500, SW_SCIENCE, 3, _("\
A piece of mathematical software."));

SOFTWARE("software_blood_data", _("infection data"), 200, SW_DATA, 5, _("\
Medical data on zombie blood."));

MACGUFFIN("note", _("note"), 0, '?', c_white, "paper", "null", 1, 3, 0, 0, 0,
    true, &iuse::mcg_note, _("\
A hand-written paper note."));

#define STATIONARY(id, name, price, category, description) \
itypes[id] = new it_stationary(id, price, name, description,\
',', c_white, "paper", "null", 0, 3, 0, 0, 0, category)

STATIONARY("flyer", _("flyer"), 1, "flier", _("A scrap of paper."));

// Finally, add all the keys from the map to a vector of all possible items
for(std::map<std::string,itype*>::iterator iter = itypes.begin(); iter != itypes.end(); ++iter){
    if(iter->first == "null" || iter->first == "corpse" || iter->first == "toolset" || iter->first == "fire" || iter->first == "apparatus"){
        pseudo_itype_ids.push_back(iter->first);
    } else {
        standard_itype_ids.push_back(iter->first);
    }
}
}

std::string ammo_name(ammotype t)
{
    if( t == "939mm")       return_("9x39");
    if( t == "700nx")       return _(".700 Nitro Express");
    if( t == "ammo_flintlock")  return _("paper cartidge");
    if( t == "50")          return _(".50 BMG");
    if( t == "nail")        return _("nails");
    if( t == "BB" )         return _("BBs");
    if( t == "bolt" )       return _("bolts");
    if( t == "arrow" )      return _("arrows");
    if( t == "pebble" )     return _("pebbles");
    if( t == "shot" )       return _("shot");
    if( t == "22" )         return _(".22");
    if( t == "9mm" )        return _("9mm");
    if( t == "762x25" )     return _("7.62x25mm");
    if( t == "38" )         return _(".38");
    if( t == "40" )         return _(".40");
    if( t == "44" )         return _(".44");
    if( t == "45" )         return _(".45");
    if( t == "454" )        return _(".454");
    if( t == "500" )        return _(".500");
    if( t == "57" )         return _("5.7mm");
    if( t == "46" )         return _("4.6mm");
    if( t == "762" )        return _("7.62x39mm");
    if( t == "223" )        return _(".223");
    if( t == "3006" )       return _(".30-06");
    if( t == "308" )        return _(".308");
    if( t == "40mm" )       return _("40mm grenade");
    if( t == "66mm" )       return _("High Explosive Anti Tank warhead");
    if( t == "84x246mm" )   return _("84mm recoilless projectile");
    if( t == "m235" )       return _("M235 Incendiary TPA");
    if( t == "gasoline" )   return _("gasoline");
    if( t == "thread" )     return _("thread");
    if( t == "battery" )    return _("batteries");
    if( t == "laser_capacitor")return _("charge");
    if( t == "plutonium" )  return _("plutonium");
    if( t == "muscle" )     return _("muscle");
    if( t == "fusion" )     return _("fusion cell");
    if( t == "12mm" )       return _("12mm slugs");
    if( t == "plasma" )     return _("hydrogen");
    if( t == "water" )      return _("clean water");
    if( t == "8x40mm" )     return _("8x40mm caseless");
    if( t == "20x66mm" )    return _("20x66mm caseless shotgun");
    if( t == "5x50" )       return _("5x50mm flechette");
    if( t == "signal_flare")return _("signal flare");
    if( t == "mininuke_mod")return _("modified mininuke");
    if( t == "charcoal" )   return _("charcoal");
    if( t == "metal_rail" ) return _("ferrous rail projectile");
    if( t == "UPS" )        return _("UPS");
    if( t == "thrown" )     return _("throwing weapon");
    if( t == "ampoule" )    return _("chemical ampoule");
    if( t == "components" ) return _("components");
    if( t == "RPG-7" )      return _("RPG-7");
    if( t == "dart" )       return _("dart");
    if( t == "fishspear" )  return _("speargun spear");
    return "XXX";
}

itype_id default_ammo(ammotype guntype)
{
    if( guntype == "nail" )         return "nail";
    if( guntype == "BB" )           return "bb";
    if( guntype == "bolt" )         return "bolt_wood";
    if( guntype == "arrow" )        return "arrow_wood";
    if( guntype == "pebble" )       return "pebble";
    if( guntype == "shot" )         return "shot_00";
    if( guntype == "22" )           return "22_lr";
    if( guntype == "9mm" )          return "9mm";
    if( guntype == "762x25" )       return "762_25";
    if( guntype == "38" )           return "38_special";
    if( guntype == "40" )           return "10mm";
    if( guntype == "44" )           return "44magnum";
    if( guntype == "45" )           return "45_acp";
    if( guntype == "454" )          return "454_Casull";
    if( guntype == "500" )          return "500_Magnum";
    if( guntype == "57" )           return "57mm";
    if( guntype == "46" )           return "46mm";
    if( guntype == "762" )          return "762_m43";
    if( guntype == "223" )          return "223";
    if( guntype == "308" )          return "308";
    if( guntype == "3006" )         return "270";
    if( guntype == "40mm" )         return "40mm_concussive";
    if( guntype == "66mm" )         return "66mm_HEAT";
    if( guntype == "84x246mm" )     return "84x246mm_he";
    if( guntype == "m235" )         return "m235tpa";
    if( guntype == "battery" )      return "battery";
    if( guntype == "fusion" )       return "laser_pack";
    if( guntype == "12mm" )         return "12mm";
    if( guntype == "plasma" )       return "plasma";
    if( guntype == "plutonium" )    return "plut_cell";
    if( guntype == "gasoline" )     return "gasoline";
    if( guntype == "thread" )       return "thread";
    if( guntype == "water" )        return "water_clean";
    if( guntype == "charcoal"  )    return "charcoal";
    if( guntype == "8x40mm"  )      return "8mm_caseless";
    if( guntype == "20x66mm"  )     return "20x66_shot";
    if( guntype == "5x50"  )        return "5x50dart";
    if( guntype == "signal_flare")  return "signal_flare";
    if( guntype == "mininuke_mod")  return "mininuke_mod";
    if( guntype == "metal_rail"  )  return "rebar_rail";
    if( guntype == "UPS"  )         return "UPS";
    if( guntype == "components"  )  return "components";
    if( guntype == "thrown"  )      return "thrown";
    if( guntype == "ampoule"  )     return "ampoule";
    if( guntype == "50"  )          return "50bmg";
    if( guntype == "fishspear"  )   return "fishspear";
    if( guntype == "939mm" )      return "9x39";
    return "null";
}
