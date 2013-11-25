#include "item.h"
#include "player.h"
#include "output.h"
#include "skill.h"
#include "game.h"
#include "cursesdef.h"
#include "text_snippets.h"
#include "material.h"
#include "item_factory.h"
#include "options.h"
#include "uistate.h"

#include <cmath> // floor
#include <sstream>
#include <algorithm>

// mfb(n) converts a flag to its appropriate position in covers's bitfield
#ifndef mfb
#define mfb(n) static_cast <unsigned long> (1 << (n))
#endif

std::string default_technique_name(technique_id tech);

light_emission nolight={0,0,0};

item::item()
{
    init();
}

item::item(itype* it, unsigned int turn)
{
    init();
    if (it == NULL)
        return;
    type = it;
    bday = turn;
    corpse = it->corpse;
    name = it->name;
    if (it->is_gun()) {
        charges = 0;
    } else if (it->is_ammo()) {
        it_ammo* ammo = dynamic_cast<it_ammo*>(it);
        charges = ammo->count;
    } else if (it->is_food()) {
        it_comest* comest = dynamic_cast<it_comest*>(it);
        if (comest->charges == 1 && !made_of(LIQUID))
            charges = -1;
        else
            charges = comest->charges;
    } else if (it->is_tool()) {
        it_tool* tool = dynamic_cast<it_tool*>(it);
        if (tool->max_charges == 0) {
            charges = -1;
        } else {
            charges = tool->def_charges;
            if (tool->ammo != "NULL") {
                curammo = dynamic_cast<it_ammo*>(item_controller->find_template(default_ammo(tool->ammo)));
            }
        }
    } else if (it->is_book()) {
        it_book* book = dynamic_cast<it_book*>(it);
        charges = book->chapters;
    } else if ((it->is_gunmod() && it->id == "spare_mag") || it->item_tags.count("MODE_AUX")) {
        charges = 0;
    } else
        charges = -1;
    if(it->is_var_veh_part()) {
        it_var_veh_part* varcarpart = dynamic_cast<it_var_veh_part*>(it);
        bigness= rng( varcarpart->min_bigness, varcarpart->max_bigness);
    }
// Should be a flag, but we're out at the moment
    if( it->is_stationary() )
    {
        note = SNIPPET.assign( (dynamic_cast<it_stationary*>(it))->category );
    }
}

item::item(itype *it, unsigned int turn, char let)
{
    init();
    if(!it) {
        type = nullitem();
        debugmsg("Instantiating an item from itype, with NULL itype! Returning null item");
    } else {
        type = it;
        bday = turn;
        name = it->name;
        if (it->is_gun()) {
            charges = 0;
        } else if (it->is_ammo()) {
            it_ammo* ammo = dynamic_cast<it_ammo*>(it);
            charges = ammo->count;
        } else if (it->is_food()) {
            it_comest* comest = dynamic_cast<it_comest*>(it);
            if (comest->charges == 1 && !made_of(LIQUID))
                charges = -1;
            else
                charges = comest->charges;
        } else if (it->is_tool()) {
            it_tool* tool = dynamic_cast<it_tool*>(it);
            if (tool->max_charges == 0)
                charges = -1;
            else {
                charges = tool->def_charges;
                if (tool->ammo != "NULL") {
                    curammo = dynamic_cast<it_ammo*>(item_controller->find_template(default_ammo(tool->ammo)));
                }
            }
        } else if (it->is_gunmod() && it->id == "spare_mag") {
            charges = 0;
        } else {
            charges = -1;
        }
        if(it->is_var_veh_part()) {
            it_var_veh_part* engine = dynamic_cast<it_var_veh_part*>(it);
            bigness= rng( engine->min_bigness, engine->max_bigness);
        }
        curammo = NULL;
        corpse = it->corpse;
        owned = -1;
        invlet = let;
        mission_id = -1;
        player_id = -1;
        // Should be a flag, but we're out at the moment
        if( it->is_stationary() )
        {
            note = SNIPPET.assign( (dynamic_cast<it_stationary*>(it))->category );
        }
    }
}

void item::make_corpse(itype* it, mtype* mt, unsigned int turn)
{
    init();
    active = mt->has_flag(MF_REVIVES)? true : false;
    if(!it)
        type = nullitem();
    else
        type = it;
    corpse = mt;
    bday = turn;
}

itype * item::nullitem_m = new itype();
itype * item::nullitem()
{
    return nullitem_m;
}

item::item(std::string itemdata, game *g)
{
 load_info(itemdata, g);
}

item::item(JsonObject &jo)
{
    deserialize(jo);
}

item::~item()
{
}

void item::init() {
    name = "";
    charges = -1;
    bday = 0;
    invlet = 0;
    damage = 0;
    burnt = 0;
    poison = 0;
    mode = "NULL";
    item_counter = 0;
    type = nullitem();
    curammo = NULL;
    corpse = NULL;
    active = false;
    owned = -1;
    mission_id = -1;
    player_id = -1;
    light = nolight;
    fridge = 0;
}

void item::make(itype* it)
{
    if(!it)
        type = nullitem();
    else
        type = it;
    contents.clear();
}

void item::clear()
{
    // should we be clearing contents, as well?
    // Seems risky to - there aren't any reported content-clearing bugs
    // init(); // this should not go here either, or make() should not use it...
    item_tags.clear();
    item_vars.clear();
}

bool item::is_null() const
{
    return (type == NULL || type->id == "null");
}

item item::in_its_container(std::map<std::string, itype*> *itypes)
{
    if (is_software()) {
        item ret( (*itypes)["usb_drive"], 0);
        ret.contents.push_back(*this);
        ret.invlet = invlet;
        return ret;
    }
    if (!is_food() || (dynamic_cast<it_comest*>(type))->container == "null")
        return *this;

    it_comest *food = dynamic_cast<it_comest*>(type);
    item ret((*itypes)[food->container], bday);

    if (dynamic_cast<it_comest*>(type)->container == "can_food")
        food->spoils = 0;

    if (made_of(LIQUID))
    {
        it_container* container = dynamic_cast<it_container*>(ret.type);
        charges = container->contains * food->charges;
    }
    ret.contents.push_back(*this);
    ret.invlet = invlet;
    return ret;
}

bool item::invlet_is_okay()
{
    return (inv_chars.find(invlet) != std::string::npos);
}

bool item::stacks_with(item rhs)
{
    bool stacks = (type   == rhs.type   && damage  == rhs.damage  &&
    active == rhs.active && charges == rhs.charges &&
    item_tags == rhs.item_tags &&
    item_vars == rhs.item_vars &&
    contents.size() == rhs.contents.size() &&
    (!goes_bad() || bday == rhs.bday));

    if ((corpse == NULL && rhs.corpse != NULL) ||
    (corpse != NULL && rhs.corpse == NULL)   )
        return false;

    if (corpse != NULL && rhs.corpse != NULL &&
    corpse->id != rhs.corpse->id)
        return false;

    if (contents.size() != rhs.contents.size())
        return false;

    if(is_var_veh_part())
        if(bigness != rhs.bigness)
            return false;

    for (int i = 0; i < contents.size() && stacks; i++)
        stacks &= contents[i].stacks_with(rhs.contents[i]);

    return stacks;
}

void item::put_in(item payload)
{
    contents.push_back(payload);
}
const char ivaresc=001;


/*
 * Old 1 line string output retained for mapbuffer
 */
std::string item::save_info() const
{
    // doing this manually so as not to recurse
    std::stringstream s;
    JsonOut jsout(&s);
    serialize(jsout, false);
    return s.str();
}

bool itag2ivar( std::string &item_tag, std::map<std::string, std::string> &item_vars ) {
    if(item_tag.at(0) == ivaresc && item_tag.find('=') != -1 && item_tag.find('=') >= 2 ) {
        std::string var_name, val_decoded;
        int svarlen, svarsep;
        svarsep=item_tag.find('=');
        svarlen=item_tag.size();
        val_decoded="";
        var_name=item_tag.substr(1,svarsep-1); // will assume sanity here for now
        for(int s = svarsep+1; s < svarlen; s++ ) { // cheap and temporary, afaik stringstream IFS = [\r\n\t ];
            if(item_tag[s] == ivaresc && s < svarlen-2 ) {
                if ( item_tag[s+1] == '0' && item_tag[s+2] == 'A' ) {
                    s+=2;
                    val_decoded.append(1, '\n');
                } else if ( item_tag[s+1] == '0' && item_tag[s+2] == 'D' ) {
                    s+=2;
                    val_decoded.append(1, '\r');
                } else if ( item_tag[s+1] == '0' && item_tag[s+2] == '6' ) {
                    s+=2;
                    val_decoded.append(1, '\t');
                } else if ( item_tag[s+1] == '2' && item_tag[s+2] == '0' ) {
                    s+=2;
                    val_decoded.append(1, ' ');
                } else {
                    val_decoded.append(1, item_tag[s]); // hhrrrmmmmm should be passing \a?
                }
            } else {
                val_decoded.append(1, item_tag[s]);
            }
        }
        item_vars[var_name]=val_decoded;
        return true;
    } else {
        return false;
    }
}


void item::load_info(std::string data, game *g)
{
    std::stringstream dump;
    dump << data;
    char check=dump.peek();
    if ( check == ' ' ) {
        // sigh..
        check=data[1];
    }
    if ( check == '{' ) {
        JsonIn jsin(&dump);
        try {
            deserialize(jsin);
        } catch (std::string jsonerr) {
            debugmsg("Bad item json\n%s", jsonerr.c_str() );
        }
        return;
    } else {
        load_legacy(g, dump);
    }
}


std::string item::info(bool showtext)
{
    std::vector<iteminfo> dummy;
    return info(showtext, &dummy);
}

std::string item::info(bool showtext, std::vector<iteminfo> *dump, game *g, bool debug)
{
 std::stringstream temp1, temp2;
 if ( g != NULL && debug == false &&
   ( g->debugmon == true || g->u.has_artifact_with(AEP_SUPER_CLAIRVOYANCE) )
 ) debug=true;
 if( !is_null() )
 {
  dump->push_back(iteminfo("BASE", _("Volume: "), "", volume(), true, "", false, true));
  dump->push_back(iteminfo("BASE", _("   Weight: "), "", g->u.convert_weight(weight()), false, "", true, true));
  dump->push_back(iteminfo("BASE", _("Bash: "), "", damage_bash(), true, "", false));
  dump->push_back(iteminfo("BASE", (has_flag("SPEAR") || has_flag("STAB") ? _(" Pierce: ") : _(" Cut: ")), "", damage_cut(), true, "", false));
  dump->push_back(iteminfo("BASE", _(" To-hit bonus: "), ((type->m_to_hit > 0) ? "+" : ""), type->m_to_hit, true, ""));
  dump->push_back(iteminfo("BASE", _("Moves per attack: "), "", attack_time(), true, "", true, true));
  if ( debug == true ) {
    if( g != NULL ) {
      dump->push_back(iteminfo("BASE", _("age: "), "",  (int(g->turn) - bday) / (10 * 60), true, "", true, true));
    }
    dump->push_back(iteminfo("BASE", _("burn: "), "",  burnt, true, "", true, true));
  }
 }

 if (is_food()) {
  it_comest* food = dynamic_cast<it_comest*>(type);

  dump->push_back(iteminfo("FOOD", _("Nutrition: "), "", food->nutr));
  dump->push_back(iteminfo("FOOD", _("Quench: "), "", food->quench));
  dump->push_back(iteminfo("FOOD", _("Enjoyability: "), "", food->fun));
  if (corpse != NULL &&
    ( debug == true ||
      ( g != NULL &&
        ( g->u.has_bionic("bio_scent_vision") || g->u.has_trait("CARNIVORE") || g->u.has_artifact_with(AEP_SUPER_CLAIRVOYANCE) )
      )
    )
  ) {
    dump->push_back(iteminfo("FOOD", _("Smells like: ") + corpse->name));
  }
 } else if (is_food_container()) {
 // added charge display for debugging
  it_comest* food = dynamic_cast<it_comest*>(contents[0].type);

  dump->push_back(iteminfo("FOOD", _("Nutrition: "), "", food->nutr));
  dump->push_back(iteminfo("FOOD", _("Quench: "), "", food->quench));
  dump->push_back(iteminfo("FOOD", _("Enjoyability: "), "", food->fun));
  dump->push_back(iteminfo("FOOD", _("Portions: "), "", abs(int(contents[0].charges))));

 } else if (is_ammo()) {
  // added charge display for debugging
  it_ammo* ammo = dynamic_cast<it_ammo*>(type);

  dump->push_back(iteminfo("AMMO", _("Type: "), ammo_name(ammo->type)));
  dump->push_back(iteminfo("AMMO", _("Damage: "), "", ammo->damage));
  dump->push_back(iteminfo("AMMO", _("Armor-pierce: "), "", ammo->pierce));
  dump->push_back(iteminfo("AMMO", _("Range: "), "", ammo->range));
  dump->push_back(iteminfo("AMMO", _("Dispersion: "), "", ammo->dispersion, true, "", true, true));
  dump->push_back(iteminfo("AMMO", _("Recoil: "), "", ammo->recoil, true, "", true, true));
  dump->push_back(iteminfo("AMMO", _("Count: "), "", ammo->count));

 } else if (is_ammo_container()) {
  it_ammo* ammo = dynamic_cast<it_ammo*>(contents[0].type);

  dump->push_back(iteminfo("AMMO", _("Type: "), ammo_name(ammo->type)));
  dump->push_back(iteminfo("AMMO", _("Damage: "), "", ammo->damage));
  dump->push_back(iteminfo("AMMO", _("Armor-pierce: "), "", ammo->pierce));
  dump->push_back(iteminfo("AMMO", _("Range: "), "", ammo->range));
  dump->push_back(iteminfo("AMMO", _("Dispersion: "), "", ammo->dispersion, true, "", true, true));
  dump->push_back(iteminfo("AMMO", _("Recoil: "), "", ammo->recoil, true, "", true, true));
  dump->push_back(iteminfo("AMMO", _("Count: "), "", contents[0].charges));

 } else if (is_gun()) {
  it_gun* gun = dynamic_cast<it_gun*>(type);
  int ammo_dam = 0, ammo_range = 0, ammo_recoil = 0, ammo_pierce = 0;
  bool has_ammo = (curammo != NULL && charges > 0);
  if (has_ammo) {
   ammo_dam = curammo->damage;
   ammo_range = curammo->range;
   ammo_recoil = curammo->recoil;
   ammo_pierce = curammo->pierce;
  }

  dump->push_back(iteminfo("GUN", _("Skill used: "), gun->skill_used->name()));
  dump->push_back(iteminfo("GUN", _("Ammunition: "), string_format(_("<num> rounds of %s"), ammo_name(ammo_type()).c_str()), clip_size(), true));

  temp1.str("");
  if (has_ammo)
   temp1 << ammo_dam;

  temp1 << (gun_damage(false) >= 0 ? "+" : "" );

  temp2.str("");
  if (has_ammo)
   temp2 << string_format(_("<num> = %d"), gun_damage());

  dump->push_back(iteminfo("GUN", _("Damage: "), temp2.str(), gun_damage(false), true, temp1.str(), true, false));

  temp1.str("");
  if (has_ammo)
   temp1 << ammo_pierce;

  temp1 << (gun_pierce(false) >= 0 ? "+" : "" );

  temp2.str("");
  if (has_ammo)
   temp2 << string_format(_("<num> = %d"), gun_pierce());

  dump->push_back(iteminfo("GUN", _("Armor-pierce: "), temp2.str(), gun_pierce(false), true, temp1.str(), true, false));

  temp1.str("");
  if (has_ammo) {
   temp1 << ammo_range;
  }
  temp1 << (range(NULL) >= 0 ? "+" : "");

  temp2.str("");
  if (has_ammo) {
   temp2 << string_format(_("<num> = %d"), range(NULL));
  }

  dump->push_back(iteminfo("GUN", _("Range: "), temp2.str(), gun->range, true, temp1.str(), true, false));

  dump->push_back(iteminfo("GUN", _("Dispersion: "), "", dispersion(), true, "", true, true));


  temp1.str("");
  if (has_ammo)
   temp1 << ammo_recoil;

  temp1 << (recoil(false) >= 0 ? "+" : "" );

  temp2.str("");
  if (has_ammo)
   temp2 << string_format(_("<num> = %d"), recoil());

  dump->push_back(iteminfo("GUN",_("Recoil: "), temp2.str(), recoil(false), true, temp1.str(), true, true));

  dump->push_back(iteminfo("GUN", _("Reload time: "), ((has_flag("RELOAD_ONE")) ? _("<num> per round") : ""), gun->reload_time, true, "", true, true));

  if (burst_size() == 0) {
   if (gun->skill_used == Skill::skill("pistol") && has_flag("RELOAD_ONE"))
    dump->push_back(iteminfo("GUN", _("Revolver.")));
   else
    dump->push_back(iteminfo("GUN", _("Semi-automatic.")));
  } else
   dump->push_back(iteminfo("GUN", _("Burst size: "), "", burst_size()));

  temp1.str("");
  temp1 << "Mods: ";
  for (int i = 0; i < contents.size(); i++) {
     if (i == contents.size() - 1) {
        if (!(i % 2) && i > 0)
          temp1 << "\n+       ";
        temp1 << contents[i].tname() + ".";
     }
     else {
        if (!(i % 2) && i > 0)
          temp1 << "\n+       ";
        temp1 << contents[i].tname() + ", ";
     }
   }

  dump->push_back(iteminfo("GUN", temp1.str()));

 } else if (is_gunmod()) {
  it_gunmod* mod = dynamic_cast<it_gunmod*>(type);

  if (mod->dispersion != 0)
   dump->push_back(iteminfo("GUNMOD", _("Dispersion: "), "", mod->dispersion, true, ((mod->dispersion > 0) ? "+" : "")));
  if (mod->damage != 0)
   dump->push_back(iteminfo("GUNMOD", _("Damage: "), "", mod->damage, true, ((mod->damage > 0) ? "+" : "")));
  if (mod->clip != 0)
   dump->push_back(iteminfo("GUNMOD", _("Magazine: "), "<num>%%", mod->clip, true, ((mod->clip > 0) ? "+" : "")));
  if (mod->recoil != 0)
   dump->push_back(iteminfo("GUNMOD", _("Recoil: "), "", mod->recoil, true, ((mod->recoil > 0) ? "+" : ""), true, true));
  if (mod->burst != 0)
   dump->push_back(iteminfo("GUNMOD", _("Burst: "), "", mod->burst, true, (mod->burst > 0 ? "+" : "")));

  if (mod->newtype != "NULL")
   dump->push_back(iteminfo("GUNMOD", "" + ammo_name(mod->newtype)));

  temp1.str("");
  temp1 << _("Used on: ");
  if (mod->used_on_pistol)
   temp1 << _("Pistols.  ");
  if (mod->used_on_shotgun)
   temp1 << _("Shotguns.  ");
  if (mod->used_on_smg)
   temp1 << _("SMGs.  ");
  if (mod->used_on_rifle)
   temp1 << _("Rifles.");

  dump->push_back(iteminfo("GUNMOD", temp1.str()));

 } else if (is_armor()) {
  it_armor* armor = dynamic_cast<it_armor*>(type);

  temp1.str("");
  temp1 << _("Covers: ");
  if (armor->covers & mfb(bp_head))
   temp1 << _("The head. ");
  if (armor->covers & mfb(bp_eyes))
   temp1 << _("The eyes. ");
  if (armor->covers & mfb(bp_mouth))
   temp1 << _("The mouth. ");
  if (armor->covers & mfb(bp_torso))
   temp1 << _("The torso. ");
  if (armor->covers & mfb(bp_arms))
   temp1 << _("The arms. ");
  if (armor->covers & mfb(bp_hands))
   temp1 << _("The hands. ");
  if (armor->covers & mfb(bp_legs))
   temp1 << _("The legs. ");
  if (armor->covers & mfb(bp_feet))
   temp1 << _("The feet. ");

  dump->push_back(iteminfo("ARMOR", temp1.str()));
  dump->push_back(iteminfo("ARMOR", _("Coverage: "), _("<num> percent"), armor->coverage));
    if (has_flag("FIT"))
    {
        dump->push_back(iteminfo("ARMOR", _("Encumberment: "), _("<num> (fits)"),
                                 std::max(0, armor->encumber - 1), true, "", true, true));
    }
    else
    {
        dump->push_back(iteminfo("ARMOR", _("Encumberment: "), "", armor->encumber, true, "", true, true));
    }
  dump->push_back(iteminfo("ARMOR", _("Protection: Bash: "), "", bash_resist(), true, "", false));
  dump->push_back(iteminfo("ARMOR", _("   Cut: "), "", cut_resist(), true, "", true));
  dump->push_back(iteminfo("ARMOR", _("Environmental protection: "), "", armor->env_resist));
  dump->push_back(iteminfo("ARMOR", _("Warmth: "), "", armor->warmth));
  dump->push_back(iteminfo("ARMOR", _("Storage: "), "", armor->storage));

} else if (is_book()) {

  it_book* book = dynamic_cast<it_book*>(type);
  if (!book->type)
   dump->push_back(iteminfo("BOOK", _("Just for fun.")));
  else {
    dump->push_back(iteminfo("BOOK", "", string_format(_("Can bring your %s skill to <num>"), book->type->name().c_str()), book->level));

   if (book->req == 0)
    dump->push_back(iteminfo("BOOK", _("It can be understood by beginners.")));
   else
    dump->push_back(iteminfo("BOOK", "", string_format(_("Requires %s level <num> to understand."), book->type->name().c_str()), book->req, true, "", true, true));
  }

  dump->push_back(iteminfo("BOOK", "", _("Requires intelligence of <num> to easily read."), book->intel, true, "", true, true));
  if (book->fun != 0)
   dump->push_back(iteminfo("BOOK", "", _("Reading this book affects your morale by <num>"), book->fun, true, (book->fun > 0 ? "+" : "")));

  dump->push_back(iteminfo("BOOK", "", _("This book takes <num> minutes to read."), book->time, true, "", true, true));

  if (!(book->recipes.empty())) {
   dump->push_back(iteminfo("BOOK", "", _("This book contains <num> crafting recipes."), book->recipes.size(), true, "", true, true));
  }

 } else if (is_tool()) {
  it_tool* tool = dynamic_cast<it_tool*>(type);

  if ((tool->max_charges)!=0) {
   if (has_flag("DOUBLE_AMMO")) {

    dump->push_back(iteminfo("TOOL", "", ((tool->ammo == "NULL")?_("Maximum <num> charges (doubled)."):string_format(_("Maximum <num> charges (doubled) of %s."), ammo_name(tool->ammo).c_str())), tool->max_charges*2));
   } else {
    dump->push_back(iteminfo("TOOL", "", ((tool->ammo == "NULL")?_("Maximum <num> charges."):string_format(_("Maximum <num> charges of %s."), ammo_name(tool->ammo).c_str())), tool->max_charges));
   }
  }
 }

 if ( type->qualities.size() > 0){
    for(std::map<std::string, int>::const_iterator quality = type->qualities.begin(); quality!=type->qualities.end();++quality){
        dump->push_back( iteminfo("QUALITIES", "", string_format(_("Has %s of level %d."),qualities[quality->first].name.c_str(),quality->second) ));
    }
 }

 if ( showtext && !is_null() ) {
    if (is_stationary()) {
       // Just use the dynamic description
        dump->push_back( iteminfo("DESCRIPTION", SNIPPET.get(note)) );
    } else {
       dump->push_back(iteminfo("DESCRIPTION", type->description));
    }

    if (is_armor() && has_flag("FIT"))
    {
        dump->push_back(iteminfo("DESCRIPTION", "\n\n"));
        dump->push_back(iteminfo("DESCRIPTION", _("This piece of clothing fits you perfectly.")));
    }
    if (is_armor() && has_flag("SKINTIGHT"))
    {
        dump->push_back(iteminfo("DESCRIPTION", "\n\n"));
        dump->push_back(iteminfo("DESCRIPTION", _("This piece of clothing lies close to the skin and layers easily.")));
    }
    if (is_armor() && has_flag("POCKETS"))
    {
        dump->push_back(iteminfo("DESCRIPTION", "\n\n"));
        dump->push_back(iteminfo("DESCRIPTION", _("This piece of clothing has pockets to warm your hands.")));
    }
    if (is_armor() && has_flag("HOOD"))
    {
        dump->push_back(iteminfo("DESCRIPTION", "\n\n"));
        dump->push_back(iteminfo("DESCRIPTION", _("This piece of clothing has a hood to keep your head warm.")));
    }
    if (is_armor() && has_flag("RAINPROOF"))
    {
        dump->push_back(iteminfo("DESCRIPTION", "\n\n"));
        dump->push_back(iteminfo("DESCRIPTION", _("This piece of clothing is designed to keep you dry in the rain.")));
    }
    if (is_armor() && has_flag("STURDY"))
    {
        dump->push_back(iteminfo("DESCRIPTION", "\n\n"));
        dump->push_back(iteminfo("DESCRIPTION", _("This piece of clothing is designed to protect you from harm and withstand a lot of abuse.")));
    }
    if (is_armor() && type->id == "rad_badge")
    {
        int i;
        for( i = 1; i < sizeof(rad_dosage_thresholds)/sizeof(rad_dosage_thresholds[0]); i++ )
        {
            if( irridation < rad_dosage_thresholds[i] )
            {
                break;
            }
        }
        dump->push_back(iteminfo("DESCRIPTION", "\n\n"));
        dump->push_back(iteminfo("DESCRIPTION", string_format(_("The film strip on the badge is %s."), rad_threshold_colors[i - 1].c_str())));
    }
    if (is_tool() && has_flag("DOUBLE_AMMO"))
    {
        dump->push_back(iteminfo("DESCRIPTION", "\n\n"));
        dump->push_back(iteminfo("DESCRIPTION", _("This tool has double the normal maximum charges.")));
    }
    std::map<std::string, std::string>::const_iterator item_note = item_vars.find("item_note");
    std::map<std::string, std::string>::const_iterator item_note_type = item_vars.find("item_note_type");

    if ( item_note != item_vars.end() ) {
        dump->push_back(iteminfo("DESCRIPTION", "\n" ));
        std::string ntext = "";
        if ( item_note_type != item_vars.end() ) {
            ntext += string_format(_("%1$s on this %2$s is a note saying: "),
                item_note_type->second.c_str(), type->name.c_str()
            );
        } else {
            ntext += _("Note: ");
        }
        dump->push_back(iteminfo("DESCRIPTION", ntext + item_note->second ));
    }
  if (contents.size() > 0) {
   if (is_gun()) {
    for (int i = 0; i < contents.size(); i++)
     dump->push_back(iteminfo("DESCRIPTION", contents[i].type->description));
   } else
    dump->push_back(iteminfo("DESCRIPTION", contents[0].type->description));
  }
 }

 temp1.str("");
 std::vector<iteminfo>& vecData = *dump; // vector is not copied here
 for (int i = 0; i < vecData.size(); i++) {
  if (vecData[i].sType == "DESCRIPTION")
   temp1 << "\n";

  temp1 << vecData[i].sName;
  size_t pos = vecData[i].sFmt.find("<num>");
  std::string sPost = "";
  if(pos != std::string::npos)
  {
      temp1 << vecData[i].sFmt.substr(0, pos);
      sPost = vecData[i].sFmt.substr(pos+5);
  }
  else
  {
      temp1 << vecData[i].sFmt.c_str(); //string_format(vecData[i].sFmt.c_str(), vecData[i].iValue)
  }
  if (vecData[i].sValue != "-999")
      temp1 << vecData[i].sValue;
  temp1 << sPost;
  temp1 << ((vecData[i].bNewLine) ? "\n" : "");
 }

 return temp1.str();
}

char item::symbol() const
{
    if( is_null() )
        return ' ';
    return type->sym;
}

nc_color item::color(player *u) const
{
    nc_color ret = c_ltgray;

    if (active && !is_food() && !is_food_container()) // Active items show up as yellow
        ret = c_yellow;
    else if (is_gun()) { // Guns are green if you are carrying ammo for them
        ammotype amtype = ammo_type();
        if (u->has_ammo(amtype).size() > 0)
            ret = c_green;
    } else if (is_ammo()) { // Likewise, ammo is green if you have guns that use it
        ammotype amtype = ammo_type();
        if (u->weapon.is_gun() && u->weapon.ammo_type() == amtype)
            ret = c_green;
        else {
            if (u->inv.has_gun_for_ammo(amtype)) {
                ret = c_green;
            }
        }
    } else if (is_book()) {
        it_book* tmp = dynamic_cast<it_book*>(type);
        if (tmp->type && tmp->intel <= u->int_cur + u->skillLevel(tmp->type) &&
                (tmp->intel == 0 || !u->has_trait("ILLITERATE")) &&
                (u->skillLevel(tmp->type) >= (int)tmp->req) &&
                (u->skillLevel(tmp->type) < (int)tmp->level))
            ret = c_ltblue;
    }
    return ret;
}

nc_color item::color_in_inventory()
{
    // Items in our inventory get colorized specially
    if (active && !is_food() && !is_food_container()) {
        return c_yellow;
    }
    return c_white;
}

std::string item::tname(game *g)
{
    std::stringstream ret;

// MATERIALS-TODO: put this in json
    std::string damtext = "";
    if (damage != 0 && !is_null()) {
        if (damage == -1) {
            damtext = rm_prefix(_("<dam_adj>reinforced "));
        } else {
            if (type->id == "corpse") {
                if (damage == 1) damtext = rm_prefix(_("<dam_adj>bruised "));
                if (damage == 2) damtext = rm_prefix(_("<dam_adj>damaged "));
                if (damage == 3) damtext = rm_prefix(_("<dam_adj>mangled "));
                if (damage == 4) damtext = rm_prefix(_("<dam_adj>pulped "));
            } else {
                damtext = rmp_format("%s ", type->dmg_adj(damage).c_str());
            }
        }
    }

    std::string vehtext = "";
    if (is_var_veh_part()) {
        if(type->bigness_aspect == BIGNESS_ENGINE_DISPLACEMENT) {
            ret.str("");
            ret.precision(4);
            ret << (float)bigness/100;
            //~ liters, e.g. 3.21-Liter V8 engine
            vehtext = rmp_format(_("<veh_adj>%s-Liter "), ret.str().c_str());
        }
        else if(type->bigness_aspect == BIGNESS_WHEEL_DIAMETER) {
            //~ inches, e.g. 20" wheel
            vehtext = rmp_format(_("<veh_adj>%d\" "), bigness);
        }
    }

    std::string burntext = "";
    if (volume() >= 4 && burnt >= volume() * 2)
        burntext = rm_prefix(_("<burnt_adj>badly burnt "));
    else if (burnt > 0)
        burntext = rm_prefix(_("<burnt_adj>burnt "));

    std::string maintext = "";
    if (corpse != NULL && typeId() == "corpse" ) {
        if (name != "")
            maintext = rmp_format(_("<item_name>%s corpse of %s"), corpse->name.c_str(), name.c_str());
        else maintext = rmp_format(_("<item_name>%s corpse"), corpse->name.c_str());
    } else if (typeId() == "blood") {
        if (corpse == NULL || corpse->id == "mon_null")
            maintext = rm_prefix(_("<item_name>human blood"));
        else
            maintext = rmp_format(_("<item_name>%s blood"), corpse->name.c_str());
    }
    else if (is_gun() && contents.size() > 0 ) {
        ret.str("");
        ret << type->name;
        for (int i = 0; i < contents.size(); i++)
            ret << "+";
        maintext = ret.str();
    } else if (contents.size() == 1) {
        maintext = rmp_format(
                       contents[0].made_of(LIQUID)?
                       _("<item_name>%s of %s"):
                       _("<item_name>%s with %s"),
                       type->name.c_str(), contents[0].tname().c_str()
                   );
    }
    else if (contents.size() > 0) {
        maintext = rmp_format(_("<item_name>%s, full"), type->name.c_str());
    } else {
        maintext = type->name;
    }

    item* food = NULL;
    it_comest* food_type = NULL;
    std::string tagtext = "";
    ret.str("");
    if (is_food())
    {
        food = this;
        food_type = dynamic_cast<it_comest*>(type);
    }
    else if (is_food_container())
    {
        food = &contents[0];
        food_type = dynamic_cast<it_comest*>(contents[0].type);
    }
    if (food != NULL && g != NULL && food_type->spoils != 0 &&
    int(g->turn) < (int)(food->bday + 100))
        ret << _(" (fresh)");
    if (food != NULL && g != NULL && food->has_flag("HOT"))
        ret << _(" (hot)");
    if (food != NULL && g != NULL && food_type->spoils != 0 &&
        rotten(g))
        ret << _(" (rotten)");

    if (has_flag("FIT")) {
        ret << _(" (fits)");
    }

    if (owned > 0)
        ret << _(" (owned)");

    tagtext = ret.str();

    ret.str("");

    ret << damtext << vehtext << burntext << maintext << tagtext;

    if (!item_vars.empty()) {
        return "*" + ret.str() + "*";
    } else {
        return ret.str();
    }
}

nc_color item::color() const
{
    if( is_null() )
        return c_black;
    if ( corpse != NULL && typeId() == "corpse" ) {
        return corpse->color;
    }
    return type->color;
}

int item::price() const
{
    if( is_null() )
        return 0;

    int ret = type->price;
    for (int i = 0; i < contents.size(); i++)
        ret += contents[i].price();
    return ret;
}

// MATERIALS-TODO: add a density field to materials.json
int item::weight() const
{
    if (corpse != NULL && typeId() == "corpse" ) {
        int ret = 0;
        switch (corpse->size) {
            case MS_TINY:   ret =   1000;  break;
            case MS_SMALL:  ret =  40750;  break;
            case MS_MEDIUM: ret =  81500;  break;
            case MS_LARGE:  ret = 120000;  break;
            case MS_HUGE:   ret = 200000;  break;
        }
        if (made_of("veggy")) {
            ret /= 3;
        } else if (made_of("iron") || made_of("steel") || made_of("stone")) {
            ret *= 7;
        }
        return ret;
    }

    if( is_null() ) {
        return 0;
    }

    int ret = type->weight;

    if (count_by_charges()) {
        ret *= charges;
    } else if (type->is_gun() && charges >= 1) {
        ret += curammo->weight * charges;
    } else if (type->is_tool() && charges >= 1 && ammo_type() != "NULL") {
        if (typeId() == "adv_UPS_off" || typeId() == "adv_UPS_on") {
            ret += item_controller->find_template(default_ammo(this->ammo_type()))->weight * charges / 500;
        } else {
            ret += item_controller->find_template(default_ammo(this->ammo_type()))->weight * charges;
        }
    }
    for (int i = 0; i < contents.size(); i++) {
        if (contents[i].is_gunmod() && contents[i].charges >= 1) {
            ret += contents[i].weight();
            ret += contents[i].curammo->weight * contents[i].charges;
        } else if (contents[i].charges <= 0) {
            ret += contents[i].weight();
        } else {
            if (contents[i].count_by_charges()) {
                ret += contents[i].weight();
            } else {
                ret += contents[i].weight() * contents[i].charges;
            }
        }
    }
    return ret;
}

/*
 * precise_unit_volume: Returns the volume, multiplied by 1000.
 * 1: -except- ammo, since the game treats the volume of count_by_charge items as 1/stack_size of the volume defined in .json
 * 2: Ammo is also not totalled.
 * 3: gun mods -are- added to the total, since a modded gun is not a splittable thing, in an inventory sense
 * This allows one to obtain the volume of something consistent with game rules, with a precision that is lost
 * when a 2 volume bullet is divided by ???? and returned as an int.
 */
int item::precise_unit_volume() const
{
   return volume(true, true);
}

/*
 * note, the game currently has an undefined number of different scales of volume: items that are count_by_charges, and
 * everything else:
 *    everything else: volume = type->volume
 *   count_by_charges: volume = type->volume / stack_size
 * Also, this function will multiply count_by_charges items by the amount of charges before dividing by ???.
 * If you need more precision, precise_value = true will return a multiple of 1000
 * If you want to handle counting up charges elsewhere, unit value = true will skip that part,
 *   except for guns.
 * Default values are unit_value=false, precise_value=false
 */
int item::volume(bool unit_value, bool precise_value ) const
{
    int ret = 0;
    if (corpse != NULL && typeId() == "corpse" ) {
        switch (corpse->size) {
            case MS_TINY:
                ret = 2;
                break;
            case MS_SMALL:
                ret = 40;
                break;
            case MS_MEDIUM:
                ret = 75;
                break;
            case MS_LARGE:
                ret = 160;
                break;
            case MS_HUGE:
                ret = 600;
                break;
        }
        if ( precise_value == true ) {
            ret *= 1000;
        }
        return ret;
    }

    if( is_null()) {
        return 0;
    }

    ret = type->volume;

    if ( precise_value == true ) {
        ret *= 1000;
    }

    if (count_by_charges()) {
        if ( unit_value == false ) {
            ret *= charges;
        }
        ret /= max_charges();
    }

    if (is_gun()) {
        for (int i = 0; i < contents.size(); i++) {
            ret += contents[i].volume( false, precise_value );
        }
    }
    return ret;
}

int item::volume_contained()
{
    int ret = 0;
    for (int i = 0; i < contents.size(); i++)
        ret += contents[i].volume();
    return ret;
}

int item::attack_time()
{
    int ret = 65 + 4 * volume() + weight() / 60;
    return ret;
}

int item::damage_bash()
{
    if( is_null() )
        return 0;
    return type->melee_dam;
}

int item::damage_cut() const
{
    if (is_gun()) {
        for (int i = 0; i < contents.size(); i++) {
            if (contents[i].typeId() == "bayonet" || "pistol_bayonet"|| "sword_bayonet")
                return contents[i].type->melee_cut;
        }
    }
    if( is_null() )
        return 0;
    return type->melee_cut;
}

bool item::has_flag(std::string f) const
{
    bool ret = false;

    // first check for flags specific to item type
    // gun flags
    if (is_gun()) {
        if (mode == "MODE_AUX") {
            item const* gunmod = inspect_active_gunmod();
            if( gunmod != NULL )
                ret = gunmod->has_flag(f);
            if (ret) return ret;
        } else {
            for (int i = 0; i < contents.size(); i++) {
                // Don't report flags from active gunmods for the gun.
                if (contents[i].has_flag(f) && !contents[i].has_flag("MODE_AUX")) {
                    ret = true;
                    return ret;
                }
            }
        }
    }
    // other item type flags
    ret = type->item_tags.count(f);
    if (ret) return ret;

    // now check for item specific flags
    ret = item_tags.count(f);
    return ret;
}

bool item::has_quality(std::string quality_id) const {
    return has_quality(quality_id, 1);
}

bool item::has_quality(std::string quality_id, int quality_value) const {
    // TODO: actually implement this >:(
    (void)quality_id; (void)quality_value; //unused grrr
    bool ret = false;

    if(type->qualities.size() > 0){
      ret = true;
    }
    return ret;
}

bool item::has_technique(matec_id tech)
{
    return type->techniques.count(tech);
}

int item::has_gunmod(itype_id mod_type)
{
    if (!is_gun())
        return -1;
    for (int i = 0; i < contents.size(); i++)
        if (contents[i].is_gunmod() && contents[i].typeId() == mod_type)
            return i;
    return -1;
}

bool item::rotten(game *g)
{
    int expiry;
    if (!is_food() || g == NULL)
        return false;
    it_comest* food = dynamic_cast<it_comest*>(type);
    if (food->spoils != 0) {
      it_comest* food = dynamic_cast<it_comest*>(type);
      if (fridge > 0) {
        // Add the number of turns we should get from refrigeration
        bday += ((int)g->turn - fridge) * 0.8;
        fridge = 0;
      }
      expiry = (int)g->turn - bday;
      return (expiry > food->spoils * 600);
    }
    else {
      return false;
    }
}

bool item::ready_to_revive(game *g)
{
    if ( corpse == NULL || !corpse->has_flag(MF_REVIVES) || damage >= 4)
    {
        return false;
    }
    int age_in_hours = (int(g->turn) - bday) / (10 * 60);
    age_in_hours -= ((float)burnt/volume()) * 24;
    if (damage > 0)
    {
        age_in_hours /= (damage + 1);
    }
    int rez_factor = 48 - age_in_hours;
    if (age_in_hours > 6 && (rez_factor <= 0 || one_in(rez_factor)))
    {
        return true;
    }
    return false;
}

bool item::goes_bad()
{
    if (!is_food())
        return false;
    it_comest* food = dynamic_cast<it_comest*>(type);
    return (food->spoils != 0);
}

bool item::count_by_charges() const
{
    if (is_ammo())
        return true;
    if (is_food()) {
        it_comest* food = dynamic_cast<it_comest*>(type);
        return (food->charges > 1);
    }
    return false;
}

int item::max_charges() const
{
    if(count_by_charges()) {
        return type->stack_size;
    } else {
        return 1;
    }
}

bool item::craft_has_charges()
{
    if (count_by_charges())
        return true;
    else if (ammo_type() == "NULL")
        return true;

    return false;
}

int item::num_charges()
{
    if (is_gun()) {
        if (mode == "MODE_AUX") {
            item* gunmod = active_gunmod();
            if (gunmod != NULL)
                return gunmod->charges;
        } else {
            return charges;
        }
    }
    if (is_gunmod() && mode == "MODE_AUX")
        return charges;
    return 0;
}

int item::weapon_value(player *p) const
{
 if( is_null() )
  return 0;

 int my_value = 0;
 if (is_gun()) {
  int gun_value = 14;
  it_gun* gun = dynamic_cast<it_gun*>(type);
  gun_value += gun->dmg_bonus;
  gun_value += int(gun->burst / 2);
  gun_value += int(gun->clip / 3);
  gun_value -= int(gun->dispersion / 5);
  gun_value *= (.5 + (.3 * p->skillLevel("gun")));
  gun_value *= (.3 + (.7 * p->skillLevel(gun->skill_used)));
  my_value += gun_value;
 }

 my_value += int(type->melee_dam * (1   + .3 * p->skillLevel("bashing") +
                                          .1 * p->skillLevel("melee")    ));

 my_value += int(type->melee_cut * (1   + .4 * p->skillLevel("cutting") +
                                          .1 * p->skillLevel("melee")    ));

 my_value += int(type->m_to_hit  * (1.2 + .3 * p->skillLevel("melee")));

 return my_value;
}

int item::melee_value(player *p)
{
 if( is_null() )
  return 0;

 int my_value = 0;
 my_value += int(type->melee_dam * (1   + .3 * p->skillLevel("bashing") +
                                          .1 * p->skillLevel("melee")    ));

 my_value += int(type->melee_cut * (1   + .4 * p->skillLevel("cutting") +
                                          .1 * p->skillLevel("melee")    ));

 my_value += int(type->m_to_hit  * (1.2 + .3 * p->skillLevel("melee")));

 return my_value;
}

int item::bash_resist() const
{
    int ret = 0;

    if (is_null())
        return 0;

    if (is_armor())
    {
        // base resistance
        it_armor* tmp = dynamic_cast<it_armor*>(type);
        material_type* cur_mat1 = material_type::find_material(tmp->m1);
        material_type* cur_mat2 = material_type::find_material(tmp->m2);
        int eff_thickness = ((tmp->thickness - damage <= 0) ? 1 : (tmp->thickness - damage));

        // assumes weighted sum of materials for items with 2 materials, 66% material 1 and 33% material 2
        if (cur_mat2->is_null())
        {
            ret = eff_thickness * (3 * cur_mat1->bash_resist());
        }
        else
        {
            ret = eff_thickness * (cur_mat1->bash_resist() + cur_mat1->bash_resist() + cur_mat2->bash_resist());
        }
    }
    else // for non-armor, just bash_resist
    {
        material_type* cur_mat1 = material_type::find_material(type->m1);
        material_type* cur_mat2 = material_type::find_material(type->m2);
        if (cur_mat2->is_null())
        {
            ret = 3 * cur_mat1->bash_resist();
        }
        else
        {
            ret = cur_mat1->bash_resist() + cur_mat1->bash_resist() + cur_mat2->bash_resist();
        }
    }

    return ret;
}

int item::cut_resist() const
{
    int ret = 0;

    if (is_null())
        return 0;

    if (is_armor())
        {
        it_armor* tmp = dynamic_cast<it_armor*>(type);
        material_type* cur_mat1 = material_type::find_material(tmp->m1);
        material_type* cur_mat2 = material_type::find_material(tmp->m2);
        int eff_thickness = ((tmp->thickness - damage <= 0) ? 1 : (tmp->thickness - damage));

        // assumes weighted sum of materials for items with 2 materials, 66% material 1 and 33% material 2
        if (cur_mat2->is_null())
        {
            ret = eff_thickness * (3 * cur_mat1->cut_resist());

        }
        else
        {
            ret = eff_thickness * (cur_mat1->cut_resist() + cur_mat1->cut_resist() + cur_mat2->cut_resist());
        }
    }
    else // for non-armor
    {
        material_type* cur_mat1 = material_type::find_material(type->m1);
        material_type* cur_mat2 = material_type::find_material(type->m2);
        if (cur_mat2->is_null())
        {
            ret = 3 * cur_mat1->cut_resist();
        }
        else
        {
            ret = cur_mat1->cut_resist() + cur_mat1->cut_resist() + cur_mat2->cut_resist();
        }
    }

    return ret;
}

int item::acid_resist() const
{
    int ret = 0;

    if (is_null())
        return 0;

    // similar weighted sum of acid resistances
    material_type* cur_mat1 = material_type::find_material(type->m1);
    material_type* cur_mat2 = material_type::find_material(type->m2);
    if (cur_mat2->is_null())
    {
        ret = 3 * cur_mat1->acid_resist();
    }
    else
    {
        ret = cur_mat1->acid_resist() + cur_mat1->acid_resist() + cur_mat2->acid_resist();
    }

    return ret;
}

bool item::is_two_handed(player *u)
{
    if (has_flag("ALWAYS_TWOHAND"))
    {
        return true;
    }
    return ((weight() / 113) > u->str_cur * 4);
}

bool item::made_of(std::string mat_ident) const
{
    if( is_null() )
        return false;

    if (corpse != NULL && typeId() == "corpse" )
        return (corpse->mat == mat_ident);

    return (type->m1 == mat_ident || type->m2 == mat_ident);
}

std::string item::get_material(int m) const
{
    if (corpse != NULL && typeId() == "corpse" )
        return corpse->mat;

    return (m==2)?type->m2:type->m1;
}

bool item::made_of(phase_id phase) const
{
    if( is_null() )
        return false;

    return (type->phase == phase);
}

bool item::conductive() const
{
    if( is_null() )
        return false;

    material_type* cur_mat1 = material_type::find_material(type->m1);
    material_type* cur_mat2 = material_type::find_material(type->m2);

    return (cur_mat1->elec_resist() <= 0 || cur_mat2->elec_resist() <= 0);
}

bool item::destroyed_at_zero_charges()
{
    return (is_ammo() || is_food());
}

bool item::is_var_veh_part() const
{
    if( is_null() )
        return false;

    return type->is_var_veh_part();
}

bool item::is_gun() const
{
    if( is_null() )
        return false;

    return type->is_gun();
}

bool item::is_silent() const
{
 if ( is_null() )
  return false;

 // So far only gun code uses this check
 return type->is_gun() && (
   noise() < 5 ||              // almost silent
   curammo->type == "bolt" || // crossbows
   curammo->type == "arrow" ||// bows
   curammo->type == "pebble" ||// sling[shot]
   curammo->type == "dart"     // blowguns and such
 );
}

bool item::is_gunmod() const
{
    if( is_null() )
        return false;

    return type->is_gunmod();
}

bool item::is_bionic() const
{
    if( is_null() )
        return false;

    return type->is_bionic();
}

bool item::is_ammo() const
{
    if( is_null() )
        return false;

    return type->is_ammo();
}

bool item::is_food(player const*u) const
{
    if (!u)
        return is_food();

    if( is_null() )
        return false;

    if (type->is_food())
        return true;

    if (u->has_bionic("bio_batteries") && is_ammo() &&
            (dynamic_cast<it_ammo*>(type))->type == "battery")
        return true;
    if (u->has_bionic("bio_furnace") && flammable() && typeId() != "corpse")
        return true;
    return false;
}

bool item::is_food_container(player const*u) const
{
    return (contents.size() >= 1 && contents[0].is_food(u));
}

bool is_edible(item i, player const*u)
{
    return (i.is_food(u) || i.is_food_container(u));
}

bool item::is_food() const
{
    if( is_null() )
        return false;

    if (type->is_food())
        return true;
    return false;
}

bool item::is_food_container() const
{
    return (contents.size() >= 1 && contents[0].is_food());
}

bool item::is_ammo_container() const
{
    return (contents.size() >= 1 && contents[0].is_ammo());
}

bool item::is_drink() const
{
    if( is_null() )
        return false;

    return type->is_food() && type->phase == LIQUID;
}

bool item::is_weap() const
{
    if( is_null() )
        return false;

    if (is_gun() || is_food() || is_ammo() || is_food_container() || is_armor() ||
            is_book() || is_tool() || is_bionic() || is_gunmod())
        return false;
    return (type->melee_dam > 7 || type->melee_cut > 5);
}

bool item::is_bashing_weapon() const
{
    if( is_null() )
        return false;

    return (type->melee_dam >= 8);
}

bool item::is_cutting_weapon() const
{
    if( is_null() )
        return false;

    return (type->melee_cut >= 8 && !has_flag("SPEAR"));
}

bool item::is_armor() const
{
    if( is_null() )
        return false;

    return type->is_armor();
}

bool item::is_book() const
{
    if( is_null() )
        return false;

    return type->is_book();
}

bool item::is_container() const
{
    if( is_null() )
        return false;

    return type->is_container();
}

bool item::is_watertight_container() const
{
    return ( is_container() != false && has_flag("WATERTIGHT") && has_flag("SEALS") );
}

int item::is_funnel_container(int bigger_than) const
{
    if ( ! is_container() ) {
        return 0;
    }
    it_container *ct = static_cast<it_container *>(type);
    // todo; consider linking funnel to item or -making- it an active item
    if ( (int)ct->contains <= bigger_than ) {
        return 0; // skip contents check, performance
    }
    if (
        contents.empty() ||
        contents[0].typeId() == "water" ||
        contents[0].typeId() == "water_acid" ||
        contents[0].typeId() == "water_acid_weak") {
        return (int)ct->contains;
    }
    return 0;
}

bool item::is_tool() const
{
    if( is_null() )
        return false;

    return type->is_tool();
}

bool item::is_software() const
{
    if( is_null() )
        return false;

    return type->is_software();
}

bool item::is_macguffin() const
{
    if( is_null() )
        return false;

    return type->is_macguffin();
}

bool item::is_stationary() const
{
    if( is_null() )
        return false;

    return type->is_stationary();
}

bool item::is_other() const
{
    if( is_null() )
        return false;

    return (!is_gun() && !is_ammo() && !is_armor() && !is_food() &&
            !is_food_container() && !is_tool() && !is_gunmod() && !is_bionic() &&
            !is_book() && !is_weap());
}

bool item::is_artifact() const
{
    if( is_null() )
        return false;

    return type->is_artifact();
}

int item::sort_rank() const
{
    // guns ammo weaps tools armor food med books mods other
    if (is_gun())
    {
        return 0;
    }
    else if (is_ammo())
    {
        return 1;
    }
    else if (is_weap()) // is_weap calls a lot of other stuff, so possible optimization candidate
    {
        return 2;
    }
    else if (is_tool())
    {
        return 3;
    }
    else if (is_armor())
    {
        return 4;
    }
    else if (is_food_container())
    {
        return 5;
    }
    else if (is_food())
    {
        it_comest* comest = dynamic_cast<it_comest*>(type);
        if (comest->comesttype != "MED")
        {
            return 5;
        }
        else
        {
            return 6;
        }
    }
    else if (is_book())
    {
        return 7;
    }
    else if (is_gunmod() || is_bionic())
    {
        return 8;
    }

    // "other" case
    return 9;
}

bool item::operator<(const item& other) const
{
    int my_rank = sort_rank();
    int other_rank = other.sort_rank();
    if (my_rank == other_rank)
    {
        const item *me = is_container() && contents.size() > 0 ? &contents[0] : this;
        const item *rhs = other.is_container() && other.contents.size() > 0 ? &other.contents[0] : &other;

        if (me->type->id == rhs->type->id)
        {
            return me->charges < rhs->charges;
        }
        else
        {
            return me->type->id < rhs->type->id;
        }
    }
    else
    {
        return sort_rank() < other.sort_rank();
    }
}

int item::reload_time(player &u)
{
    int ret = 0;

    if (is_gun()) {
        it_gun* reloading = dynamic_cast<it_gun*>(type);
        ret = reloading->reload_time;
        if (charges == 0) {
            int spare_mag = has_gunmod("spare_mag");
            if (spare_mag != -1 && contents[spare_mag].charges > 0)
                ret -= double(ret) * 0.9;
        }
        double skill_bonus = double(u.skillLevel(reloading->skill_used)) * .075;
        if (skill_bonus > .75)
            skill_bonus = .75;
        ret -= double(ret) * skill_bonus;
    } else if (is_tool())
        ret = 100 + volume() + (weight() / 113);

    if (has_flag("STR_RELOAD"))
        ret -= u.str_cur * 20;
    if (ret < 25)
        ret = 25;
    ret += u.encumb(bp_hands) * 30;
    return ret;
}

item* item::active_gunmod()
{
    if( mode == "MODE_AUX" )
        for (int i = 0; i < contents.size(); i++)
            if (contents[i].is_gunmod() && contents[i].mode == "MODE_AUX")
                return &contents[i];
    return NULL;
}

item const* item::inspect_active_gunmod() const
{
    if (mode == "MODE_AUX")
    {
        for (int i = 0; i < contents.size(); ++i)
        {
            if (contents[i].is_gunmod() && contents[i].mode == "MODE_AUX")
            {
                return &contents[i];
            }
        }
    }
    return NULL;
}

void item::next_mode()
{
    if( mode == "NULL" )
    {
        if( has_flag("MODE_BURST") )
        {
            mode = "MODE_BURST";
        }
        else
        {
            // Enable the first mod with an AUX firing mode.
            for (int i = 0; i < contents.size(); i++)
            {
                if (contents[i].is_gunmod() && contents[i].has_flag("MODE_AUX"))
                {
                    mode = "MODE_AUX";
                    contents[i].mode = "MODE_AUX";
                    break;
                }
            }
        }
        // Doesn't have another mode.
    }
    else if( mode ==  "MODE_BURST" )
    {
        // Enable the first mod with an AUX firing mode.
        for (int i = 0; i < contents.size(); i++)
        {
            if (contents[i].is_gunmod() && contents[i].has_flag("MODE_AUX"))
            {
                mode = "MODE_AUX";
                contents[i].mode = "MODE_AUX";
                break;
            }
        }
        if (mode == "MODE_BURST")
            mode = "NULL";
    }
    else if( mode == "MODE_AUX")
    {
        int i = 0;
        // Advance to next aux mode, or if there isn't one, normal mode
        for (; i < contents.size(); i++)
        {
            if (contents[i].is_gunmod() && contents[i].mode == "MODE_AUX")
            {
                contents[i].mode = "NULL";
                break;
            }
        }
        for (i++; i < contents.size(); i++)
        {
            if (contents[i].is_gunmod() && contents[i].has_flag("MODE_AUX"))
            {
                contents[i].mode = "MODE_AUX";
                break;
            }
        }
        if (i == contents.size())
        {
            mode = "NULL";
        }
    }
}

int item::clip_size()
{
    if(is_gunmod() && has_flag("MODE_AUX"))
        return (dynamic_cast<it_gunmod*>(type))->clip;
    if (!is_gun())
        return 0;

    it_gun* gun = dynamic_cast<it_gun*>(type);
    int ret = gun->clip;
    for (int i = 0; i < contents.size(); i++) {
        if (contents[i].is_gunmod() && !contents[i].has_flag("MODE_AUX")) {
            int bonus = (ret * (dynamic_cast<it_gunmod*>(contents[i].type))->clip) / 100;
            ret = int(ret + bonus);
        }
    }
    return ret;
}

int item::dispersion()
{
    if (!is_gun())
        return 0;
    it_gun* gun = dynamic_cast<it_gun*>(type);
    int ret = gun->dispersion;
    for (int i = 0; i < contents.size(); i++) {
        if (contents[i].is_gunmod())
            ret += (dynamic_cast<it_gunmod*>(contents[i].type))->dispersion;
    }
    ret += damage * 2;
    if (ret < 0) ret = 0;
    return ret;
}

int item::gun_damage(bool with_ammo)
{
    if (is_gunmod() && mode == "MODE_AUX")
        return curammo->damage;
    if (!is_gun())
        return 0;
    if(mode == "MODE_AUX") {
        item* gunmod = active_gunmod();
        if(gunmod != NULL && gunmod->curammo != NULL)
            return gunmod->curammo->damage;
        else
            return 0;
    }
    it_gun* gun = dynamic_cast<it_gun*>(type);
    int ret = gun->dmg_bonus;
    if (with_ammo && curammo != NULL)
        ret += curammo->damage;
    for (int i = 0; i < contents.size(); i++) {
        if (contents[i].is_gunmod())
            ret += (dynamic_cast<it_gunmod*>(contents[i].type))->damage;
    }
    ret -= damage * 2;
    return ret;
}

int item::gun_pierce(bool with_ammo)
{
    if (is_gunmod() && mode == "MODE_AUX")
        return curammo->pierce;
    if (!is_gun())
        return 0;
    if(mode == "MODE_AUX") {
        item* gunmod = active_gunmod();
        if(gunmod != NULL && gunmod->curammo != NULL)
            return gunmod->curammo->pierce;
        else
            return 0;
    }
    it_gun* gun = dynamic_cast<it_gun*>(type);
    int ret = gun->pierce;
    if (with_ammo && curammo != NULL)
        ret += curammo->pierce;
    return ret;
}

int item::noise() const
{
    if (!is_gun())
        return 0;
    int ret = 0;
    if(mode == "MODE_AUX") {
        item const* gunmod = inspect_active_gunmod();
        if (gunmod && gunmod->curammo)
            ret = gunmod->curammo->damage;
    } else if (curammo)
        ret = curammo->damage;
    ret *= .8;
    if (ret >= 5)
        ret += 20;
    if(mode == "MODE_AUX")
        return ret;
    for (int i = 0; i < contents.size(); i++) {
        if (contents[i].is_gunmod())
            ret += (dynamic_cast<it_gunmod*>(contents[i].type))->loudness;
    }
    return ret;
}

int item::burst_size()
{
    if (!is_gun())
        return 0;
// No burst fire for gunmods right now.
    if(mode == "MODE_AUX")
        return 1;
    it_gun* gun = dynamic_cast<it_gun*>(type);
    int ret = gun->burst;
    for (int i = 0; i < contents.size(); i++) {
        if (contents[i].is_gunmod())
            ret += (dynamic_cast<it_gunmod*>(contents[i].type))->burst;
    }
    if (ret < 0)
        return 0;
    return ret;
}

int item::recoil(bool with_ammo)
{
    if (!is_gun())
        return 0;
// Just use the raw ammo recoil for now.
    if(mode == "MODE_AUX") {
        item* gunmod = active_gunmod();
        if (gunmod && gunmod->curammo)
            return gunmod->curammo->recoil;
        else
            return 0;
    }
    it_gun* gun = dynamic_cast<it_gun*>(type);
    int ret = gun->recoil;
    if (with_ammo && curammo)
        ret += curammo->recoil;
    for (int i = 0; i < contents.size(); i++) {
        if (contents[i].is_gunmod())
            ret += (dynamic_cast<it_gunmod*>(contents[i].type))->recoil;
    }
    return ret;
}

int item::range(player *p)
{
    if (!is_gun())
        return 0;
    // Just use the raw ammo range for now.
    // we do NOT want to use the parent gun's range.
    if(mode == "MODE_AUX") {
        item* gunmod = active_gunmod();
        if(gunmod && gunmod->curammo)
            return gunmod->curammo->range;
        else
            return 0;
    }

    // Ammoless weapons use weapon's range only
    if (has_flag("NO_AMMO") && !curammo) {
        return dynamic_cast<it_gun*>(type)->range;
    }

    int ret = (curammo ? dynamic_cast<it_gun*>(type)->range + curammo->range : 0);

    if (has_flag("CHARGE")) {
        ret = dynamic_cast<it_gun*>(type)->range + 5 + charges * 5;
    }

    if (has_flag("STR8_DRAW") && p) {
        if (p->str_cur < 4) { return 0; }
        if (p->str_cur < 8) {
            ret -= 2 * (8 - p->str_cur);
        }
    } else if (has_flag("STR10_DRAW") && p) {
        if (p->str_cur < 5) { return 0; }
        if (p->str_cur < 10) {
            ret -= 2 * (10 - p->str_cur);
        }
    } else if (has_flag("STR12_DRAW") && p) {
        if (p->str_cur < 6) { return 0; }
        if (p->str_cur < 12) {
            ret -= 2 * (12 - p->str_cur);
        }
    }

    if(ret < 0) { ret = 0; }
    return ret;
}


ammotype item::ammo_type() const
{
    if (is_gun()) {
        it_gun* gun = dynamic_cast<it_gun*>(type);
        ammotype ret = gun->ammo;
        for (int i = 0; i < contents.size(); i++) {
            if (contents[i].is_gunmod() && !contents[i].has_flag("MODE_AUX")) {
                it_gunmod* mod = dynamic_cast<it_gunmod*>(contents[i].type);
                if (mod->newtype != "NULL")
                    ret = mod->newtype;
            }
        }
        return ret;
    } else if (is_tool()) {
        it_tool* tool = dynamic_cast<it_tool*>(type);
        return tool->ammo;
    } else if (is_ammo()) {
        it_ammo* amm = dynamic_cast<it_ammo*>(type);
        return amm->type;
    } else if (is_gunmod()) {
        it_gunmod* mod = dynamic_cast<it_gunmod*>(type);
        return mod->newtype;
    }
    return "NULL";
}

char item::pick_reload_ammo(player &u, bool interactive)
{
 if( is_null() )
  return false;

 if (!type->is_gun() && !type->is_tool()) {
  debugmsg("RELOADING NON-GUN NON-TOOL");
  return false;
 }
 int has_spare_mag = has_gunmod ("spare_mag");

 std::vector<item*> am; // List of valid ammo

 if (type->is_gun()) {
  if(charges <= 0 && has_spare_mag != -1 && contents[has_spare_mag].charges > 0) {
   // Special return to use magazine for reloading.
   return -2;
  }
  it_gun* tmp = dynamic_cast<it_gun*>(type);

  // If there's room to load more ammo into the gun or a spare mag, stash the ammo.
  // If the gun is partially loaded make sure the ammo matches.
  // If the gun is empty, either the spre mag is empty too and anything goes,
  // or the spare mag is loaded and we're doing a tactical reload.
  if (charges < clip_size() ||
      (has_spare_mag != -1 && contents[has_spare_mag].charges < tmp->clip)) {
   std::vector<item*> tmpammo = u.has_ammo(ammo_type());
   for (int i = 0; i < tmpammo.size(); i++)
    if (charges <= 0 || tmpammo[i]->typeId() == curammo->id)
      am.push_back(tmpammo[i]);
  }

  // ammo for gun attachments (shotgun attachments, grenade attachments, etc.)
  // for each attachment, find its associated ammo & append it to the ammo vector
  for (int i = 0; i < contents.size(); i++)
   if (contents[i].is_gunmod() && contents[i].has_flag("MODE_AUX") &&
       contents[i].charges < (dynamic_cast<it_gunmod*>(contents[i].type))->clip) {
    std::vector<item*> tmpammo = u.has_ammo((dynamic_cast<it_gunmod*>(contents[i].type))->newtype);
    for(int j = 0; j < tmpammo.size(); j++)
     if (contents[i].charges <= 0 ||
         tmpammo[j]->typeId() == contents[i].curammo->id)
      am.push_back(tmpammo[j]);
   }
 } else { //non-gun.
  am = u.has_ammo(ammo_type());
 }

 // Check if the player is wielding ammo
 if (g->u.is_armed() && g->u.weapon.is_ammo()){
     // if it is compatible then include it.
     it_ammo* w_ammo = dynamic_cast<it_ammo*>(u.weapon.type);
     if (w_ammo->type == ammo_type())
         am.push_back(&u.weapon);
 }


 char am_invlet = 0;

 if (am.size() > 1 && interactive) {// More than one option; list 'em and pick
     uimenu amenu;
     amenu.return_invalid = true;
     amenu.w_y = 0;
     amenu.w_x = 0;
     amenu.w_width = TERMX;
     int namelen=TERMX-2-40-3;
     std::string lastreload = "";

     if ( uistate.lastreload.find( ammo_type() ) != uistate.lastreload.end() ) {
         lastreload = uistate.lastreload[ ammo_type() ];
     }

     amenu.text=string_format("Choose ammo type:"+std::string(namelen,' ')).substr(0,namelen) +
         "   Damage    Pierce    Range     Accuracy";
     it_ammo* ammo_def;
     for (int i = 0; i < am.size(); i++) {
         ammo_def = dynamic_cast<it_ammo*>(am[i]->type);
         amenu.addentry(i,true,i + 'a',"%s | %-7d | %-7d | %-7d | %-7d",
             std::string(
                string_format("%s (%d)", am[i]->tname().c_str(), am[i]->charges ) +
                std::string(namelen,' ')
             ).substr(0,namelen).c_str(),
             ammo_def->damage, ammo_def->pierce, ammo_def->range,
             100 - ammo_def->dispersion
         );
         if ( lastreload == am[i]->typeId() ) {
             amenu.selected = i;
         }
     }
     amenu.query();
     if ( amenu.ret >= 0 ) {
        am_invlet = am[ amenu.ret ]->invlet;
        uistate.lastreload[ ammo_type() ] = am[ amenu.ret ]->typeId();
     }
 }
 // Either only one valid choice or chosing for a NPC, just return the first.
 else if (am.size() > 0){
  am_invlet = am[0]->invlet;
 }
 return am_invlet;
}

bool item::reload(player &u, char ammo_invlet)
{
 bool single_load = false;
 int max_load = 1;
 item *reload_target = NULL;
 item *ammo_to_use = (ammo_invlet != 0 ? &u.inv.item_by_letter(ammo_invlet) : NULL);

 // also check if wielding ammo
 if (ammo_to_use->is_null()) {
     if (u.is_armed() && u.weapon.is_ammo() && u.weapon.invlet == ammo_invlet)
         ammo_to_use = &u.weapon;
 }

 // Handle ammo in containers, currently only gasoline
 if(ammo_to_use && ammo_to_use->is_container())
   ammo_to_use = &ammo_to_use->contents[0];

 if (is_gun()) {
  // Reload using a spare magazine
  int spare_mag = has_gunmod("spare_mag");
  if (charges <= 0 && spare_mag != -1 &&
      u.weapon.contents[spare_mag].charges > 0) {
   charges = u.weapon.contents[spare_mag].charges;
   curammo = u.weapon.contents[spare_mag].curammo;
   u.weapon.contents[spare_mag].charges = 0;
   u.weapon.contents[spare_mag].curammo = NULL;
   return true;
  }

  // Determine what we're reloading, the gun, a spare magazine, or another gunmod.
  // Prefer the active gunmod if there is one
  item* gunmod = active_gunmod();
  if (gunmod && gunmod->ammo_type() == ammo_to_use->ammo_type() &&
      (gunmod->charges <= 0 || gunmod->curammo->id == ammo_to_use->typeId())) {
   reload_target = gunmod;
  // Then prefer the gun itself
  } else if (charges < clip_size() &&
             ammo_type() == ammo_to_use->ammo_type() &&
             (charges <= 0 || curammo->id == ammo_to_use->typeId())) {
   reload_target = this;
  // Then prefer a spare mag if present
  } else if (spare_mag != -1 &&
             ammo_type() == ammo_to_use->ammo_type() &&
             contents[spare_mag].charges != (dynamic_cast<it_gun*>(type))->clip &&
             (charges <= 0 || curammo->id == ammo_to_use->typeId())) {
   reload_target = &contents[spare_mag];
  // Finally consider other gunmods
  } else {
   for (int i = 0; i < contents.size(); i++) {
    if (&contents[i] != gunmod && i != spare_mag && contents[i].is_gunmod() &&
        contents[i].has_flag("MODE_AUX") && contents[i].ammo_type() == ammo_to_use->ammo_type() &&
        (contents[i].charges <= (dynamic_cast<it_gunmod*>(contents[i].type))->clip ||
        (contents[i].charges <= 0 ||  contents[i].curammo->id == ammo_to_use->typeId()))) {
     reload_target = &contents[i];
     break;
    }
   }
  }

  if (reload_target == NULL)
   return false;

  if (reload_target->is_gun() || reload_target->is_gunmod()) {
   if (reload_target->is_gunmod() && reload_target->typeId() == "spare_mag") {
    // Use gun numbers instead of the mod if it's a spare magazine
    max_load = (dynamic_cast<it_gun*>(type))->clip;
    single_load = has_flag("RELOAD_ONE");
   } else {
    single_load = reload_target->has_flag("RELOAD_ONE");
    max_load = reload_target->clip_size();
   }
  }
 } else if (is_tool()) {
  it_tool* tool = dynamic_cast<it_tool*>(type);
  reload_target = this;
  single_load = false;
  max_load = tool->max_charges;
 } else
  return false;

 if (has_flag("DOUBLE_AMMO")) {
  max_load *= 2;
 }

 if (ammo_invlet > 0) {
  // If the gun is currently loaded with a different type of ammo, reloading fails
  if ((reload_target->is_gun() || reload_target->is_gunmod()) &&
      reload_target->charges > 0 &&
      reload_target->curammo->id != ammo_to_use->typeId())
   return false;
  if (reload_target->is_gun() || reload_target->is_gunmod()) {
   if (!ammo_to_use->is_ammo()) {
    debugmsg("Tried to reload %s with %s!", tname().c_str(),
             ammo_to_use->tname().c_str());
    return false;
   }
   reload_target->curammo = dynamic_cast<it_ammo*>((ammo_to_use->type));
  }
  if (single_load || max_load == 1) { // Only insert one cartridge!
   reload_target->charges++;
   ammo_to_use->charges--;
  }
  else if (reload_target->typeId() == "adv_UPS_off" || reload_target->typeId() == "adv_UPS_on") {
      int charges_per_plut = 500;
      int max_plut = floor( static_cast<float>((max_load - reload_target->charges) / charges_per_plut) );
      int charges_used = std::min(ammo_to_use->charges, max_plut);
      reload_target->charges += (charges_used * charges_per_plut);
      ammo_to_use->charges -= charges_used;
  } else {
   reload_target->charges += ammo_to_use->charges;
   ammo_to_use->charges = 0;
   if (reload_target->charges > max_load) {
    // More rounds than the clip holds, put some back
    ammo_to_use->charges += reload_target->charges - max_load;
    reload_target->charges = max_load;
   }
  }
  if (ammo_to_use->charges == 0)
  {
      if (ammo_to_use->is_container())
      {
          ammo_to_use->contents.erase(ammo_to_use->contents.begin());
      }
      else if (u.weapon.invlet == ammo_to_use->invlet) {
          u.remove_weapon();
      }
      else
      {
          u.i_remn(ammo_invlet);
      }
  }
  return true;
 } else
  return false;
}

void item::use()
{
    if (charges > 0) {
        charges--;
    }
}

bool item::burn(int amount)
{
    burnt += amount;
    return (burnt >= volume() * 3);
}

bool item::flammable() const
{
    material_type* cur_mat1 = material_type::find_material(type->m1);
    material_type* cur_mat2 = material_type::find_material(type->m2);

    return ((cur_mat1->fire_resist() + cur_mat2->fire_resist()) <= 0);
}

std::string default_technique_name(technique_id tech)
{
 switch (tech) {
  case TEC_SWEEP: return _("Sweep attack");
  case TEC_PRECISE: return _("Precision attack");
  case TEC_BRUTAL: return _("Knock-back attack");
  case TEC_GRAB: return _("Grab");
  case TEC_WIDE: return _("Hit all adjacent monsters");
  case TEC_RAPID: return _("Rapid attack");
  case TEC_FEINT: return _("Feint");
  case TEC_THROW: return _("Throw");
  case TEC_BLOCK: return _("Block");
  case TEC_BLOCK_LEGS: return _("Leg block");
  case TEC_WBLOCK_1: return _("Weak block");
  case TEC_WBLOCK_2: return _("Parry");
  case TEC_WBLOCK_3: return _("Shield");
  case TEC_COUNTER: return _("Counter-attack");
  case TEC_BREAK: return _("Grab break");
  case TEC_DISARM: return _("Disarm");
  case TEC_DEF_THROW: return _("Defensive throw");
  case TEC_DEF_DISARM: return _("Defense disarm");
  case TEC_FLAMING: return    _("FLAMING");
  default: return "A BUG! (item.cpp:default_technique_name (default))";
 }
 return "A BUG! (item.cpp:default_technique_name)";
}

std::ostream & operator<<(std::ostream & out, const item * it)
{
    out << "item(";
    if(!it)
    {
        out << "NULL)";
        return out;
    }
    out << it->name << ")";
    return out;
}

std::ostream & operator<<(std::ostream & out, const item & it)
{
    out << (&it);
    return out;
}


itype_id item::typeId() const
{
    if (!type)
        return "null";
    return type->id;
}

item item::clone() {
    return item(type, bday);
}

bool item::getlight(float & luminance, int & width, int & direction, bool calculate_dimming ) const {
    luminance = 0;
    width = 0;
    direction = 0;
    if ( light.luminance > 0 ) {
        luminance = (float)light.luminance;
        if ( light.width > 0 ) { // width > 0 is a light arc
            width = light.width;
            direction = light.direction;
        }
        return true;
    } else {
        const int lumint = getlight_emit( calculate_dimming );
        if ( lumint > 0 ) {
            luminance = (float)lumint;
            return true;
        }
    }
    return false;
}

/*
 * Returns just the integer
 */
int item::getlight_emit(bool calculate_dimming) const {
    const int mult = 10; // woo intmath
    const int chargedrop = 5 * mult; // start dimming at 1/5th charge.

    int lumint = type->light_emission * mult;

    if ( lumint == 0 ) {
        return 0;
    }
    if ( calculate_dimming && has_flag("CHARGEDIM") && is_tool()) {
        it_tool * tool = dynamic_cast<it_tool *>(type);
        int maxcharge = tool->max_charges;
        if ( maxcharge > 0 ) {
            lumint = ( type->light_emission * chargedrop * charges ) / maxcharge;
        }
    }
    if ( lumint > 4 && lumint < 10 ) {
        lumint = 10;
    }
    return lumint / 10;
}

// How much more of this liquid can be put in this container
int item::get_remaining_capacity_for_liquid(const item &liquid, LIQUID_FILL_ERROR &error) const
{
    error = L_ERR_NONE;

    if (liquid.is_ammo() && (is_tool() || is_gun())) {
        // for filling up chainsaws, jackhammers and flamethrowers
        ammotype ammo = "NULL";
        int max = 0;

        if (is_tool()) {
            it_tool *tool = dynamic_cast<it_tool *>(type);
            ammo = tool->ammo;
            max = tool->max_charges;
        } else {
            it_gun *gun = dynamic_cast<it_gun *>(type);
            ammo = gun->ammo;
            max = gun->clip;
        }

        ammotype liquid_type = liquid.ammo_type();

        if (ammo != liquid_type) {
            error = L_ERR_NOT_CONTAINER;
            return 0;
        }

        if (max <= 0 || charges >= max) {
            error = L_ERR_FULL;
            return 0;
        }

        if (charges > 0 && curammo != NULL && curammo->id != liquid.type->id) {
            error = L_ERR_NO_MIX;
            return 0;
        }
        return max - charges;
    }

    if (!is_container()) {
        error = L_ERR_NOT_CONTAINER;
        return 0;
    }

    if (contents.empty()) {
        if (!has_flag("WATERTIGHT")) { // invalid container types
            error = L_ERR_NOT_WATERTIGHT;
            return 0;
        } else if (!has_flag("SEALS")) {
            error = L_ERR_NOT_SEALED;
            return 0;
        }
    } else { // Not empty
        if (contents[0].type->id != liquid.type->id) {
            error = L_ERR_NO_MIX;
            return 0;
        }
    }

    it_container *container = dynamic_cast<it_container *>(type);
    int total_capacity = container->contains;

    if (liquid.is_food()) {
        it_comest *tmp_comest = dynamic_cast<it_comest *>(liquid.type);
        total_capacity = container->contains * tmp_comest->charges;
    } else if (liquid.is_ammo()) {
        it_ammo *tmp_ammo = dynamic_cast<it_ammo *>(liquid.type);
        total_capacity = container->contains * tmp_ammo->count;
    }

    int remaining_capacity = total_capacity;
    if (!contents.empty()) {
        remaining_capacity -= contents[0].charges;
    }

    if (remaining_capacity <= 0) {
        error = L_ERR_FULL;
        return 0;
    }

    return remaining_capacity;
}
