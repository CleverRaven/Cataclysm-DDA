#include "item.h"
#include "player.h"
#include "output.h"
#include "skill.h"
#include "game.h"
#include <sstream>
#include <algorithm>
#include "cursesdef.h"
#include "text_snippets.h"
#include "material.h"
#include "item_factory.h"
#include "options.h"

// mfb(n) converts a flag to its appropriate position in covers's bitfield
#ifndef mfb
#define mfb(n) long(1 << (n))
#endif

std::string default_technique_name(technique_id tech);

item::item()
{
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
}

item::item(itype* it, unsigned int turn)
{
 if(!it)
  type = nullitem();
 else
  type = it;
 bday = turn;
 name = "";
 invlet = 0;
 damage = 0;
 burnt = 0;
 poison = 0;
 mode = "NULL";
 item_counter = 0;
 active = false;
 curammo = NULL;
 corpse = NULL;
 owned = -1;
 mission_id = -1;
 player_id = -1;
 if (it == NULL)
  return;
 name = it->name;
 if (it->is_gun())
  charges = 0;
 else if (it->is_ammo()) {
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
 } else if ((it->is_gunmod() && it->id == "spare_mag") || it->item_tags.count("MODE_AUX")) {
  charges = 0;
 } else
  charges = -1;
 if(it->is_var_veh_part()){
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
 if(!it) {
  type = nullitem();
  debugmsg("Instantiating an item from itype, with NULL itype!");
 } else {
  type = it;
 }
 bday = turn;
 name = it->name;
 damage = 0;
 burnt = 0;
 poison = 0;
 mode = "NULL";
 item_counter = 0;
 active = false;
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
  else
   charges = tool->def_charges;
 } else if (it->is_gunmod() && it->id == "spare_mag") {
  charges = 0;
 } else {
  charges = -1;
 }
 if(it->is_var_veh_part()){
  it_var_veh_part* engine = dynamic_cast<it_var_veh_part*>(it);
  bigness= rng( engine->min_bigness, engine->max_bigness);
 }
 curammo = NULL;
 corpse = NULL;
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

void item::make_corpse(itype* it, mtype* mt, unsigned int turn)
{
 name = "";
 charges = -1;
 invlet = 0;
 damage = 0;
 burnt = 0;
 poison = 0;
 mode = "NULL";
 item_counter = 0;
 curammo = NULL;
 active = mt->species == species_zombie ? true : false;
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

item::~item()
{
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

std::string item::save_info() const
{
 if (type == NULL){
  debugmsg("Tried to save an item with NULL type!");
 }
 itype_id ammotmp = "null";
/* TODO: This causes a segfault sometimes, even though we check to make sure
 * curammo isn't NULL.  The crashes seem to occur most frequently when saving an
 * NPC, or when saving map data containing an item an NPC has dropped.
 */
 if (curammo != NULL){
  ammotmp = curammo->id;
 }
 if( std::find(unreal_itype_ids.begin(), unreal_itype_ids.end(),
     ammotmp) != unreal_itype_ids.end()  &&
     std::find(artifact_itype_ids.begin(), artifact_itype_ids.end(),
     ammotmp) != artifact_itype_ids.end()
     ) {
  ammotmp = "null"; //Saves us from some bugs, apparently?
 }
 std::stringstream dump;
 dump << " " << int(invlet) << " " << typeId() << " " <<  int(charges) <<
     " " << int(damage) << " ";
/////
 int stags=item_tags.size() + item_vars.size();
/////
 dump << stags << " ";
 for( std::set<std::string>::const_iterator it = item_tags.begin();
      it != item_tags.end(); ++it )
 {
     dump << *it << " ";
 }
/////
 for( std::map<std::string, std::string>::const_iterator it = item_vars.begin(); it != item_vars.end(); ++it ) {
    std::string itstr="";
    std::string itval="";
    dump << ivaresc << it->first << "=";
    for(std::string::const_iterator sit = it->second.begin(); sit != it->second.end(); ++sit ) {
       switch(*sit) {
           case '\n': dump << ivaresc << "0A"; break;
           case '\r': dump << ivaresc << "0D"; break;
           case '\t': dump << ivaresc << "09"; break;
           case ' ': dump << ivaresc << "20"; break;
           default:  dump << *sit; break;
       }
    }
    dump << " ";
 }
////

 dump << burnt << " " << poison << " " << ammotmp <<
        " " << owned << " " << int(bday) << " " << mode;
 if (active)
  dump << " 1";
 else
  dump << " 0";
 if (corpse != NULL)
  dump << " " << corpse->id;
 else
  dump << " -1";
 dump << " " << mission_id << " " << player_id;
 size_t pos = name.find_first_of("\n");
 std::string temp_name = name;
 while (pos != std::string::npos)  {
  temp_name.replace(pos, 1, "@@");
  pos = temp_name.find_first_of("\n");
 }
 dump << " '" << temp_name << "'";
 return dump.str();
}

bool itag2ivar( std::string &item_tag, std::map<std::string, std::string> &item_vars ) {
   if(item_tag.at(0) == ivaresc && item_tag.find('=') != -1 && item_tag.find('=') >= 2 ) {
     std::string var_name, val_decoded;
     int svarlen, svarsep;
     svarsep=item_tag.find('=');
     svarlen=item_tag.size();
     var_name="";
     val_decoded="";
     var_name=item_tag.substr(1,svarsep-1); // will assume sanity here for now
     for(int s = svarsep+1; s < svarlen; s++ ) { // cheap and temporary, afaik stringstream IFS = [\r\n\t ];
         if(item_tag[s] == ivaresc && s < svarlen-2 ){
             if ( item_tag[s+1] == '0' && item_tag[s+2] == 'A' ) {
                 s+=2; val_decoded.append(1, '\n');
             } else if ( item_tag[s+1] == '0' && item_tag[s+2] == 'D' ) {
                 s+=2; val_decoded.append(1, '\r');
             } else if ( item_tag[s+1] == '0' && item_tag[s+2] == '6' ) {
                 s+=2; val_decoded.append(1, '\t');
             } else if ( item_tag[s+1] == '2' && item_tag[s+2] == '0' ) {
                 s+=2; val_decoded.append(1, ' ');
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
 clear();
 std::stringstream dump;
 dump << data;
 std::string idtmp, ammotmp, item_tag;
 int lettmp, damtmp, acttmp, corp, tag_count;
 dump >> lettmp >> idtmp >> charges >> damtmp >> tag_count;

 for( int i = 0; i < tag_count; ++i )
 {
     dump >> item_tag;
   if( itag2ivar(item_tag, item_vars ) == false ) {
     item_tags.insert( item_tag );
   }
 }

 dump >> burnt >> poison >> ammotmp >> owned >> bday >>
        mode >> acttmp >> corp >> mission_id >> player_id;
 if (corp != -1)
  corpse = g->mtypes[corp];
 else
  corpse = NULL;
 getline(dump, name);
 if (name == " ''")
  name = "";
 else {
  size_t pos = name.find_first_of("@@");
  while (pos != std::string::npos)  {
   name.replace(pos, 2, "\n");
   pos = name.find_first_of("@@");
  }
  name = name.substr(2, name.size() - 3); // s/^ '(.*)'$/\1/
 }
 make(g->itypes[idtmp]);
 invlet = char(lettmp);
 damage = damtmp;
 active = false;
 if (acttmp == 1)
  active = true;
 if (ammotmp != "null")
  curammo = dynamic_cast<it_ammo*>(g->itypes[ammotmp]);
 else
  curammo = NULL;
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
  dump->push_back(iteminfo("BASE", _("Volume: "), "", int(volume()), "", false, true));
  dump->push_back(iteminfo("BASE", _("   Weight: "), "", int(weight()), "", true, true));
  dump->push_back(iteminfo("BASE", _("Bash: "), "", int(type->melee_dam), "", false));
  dump->push_back(iteminfo("BASE", (has_flag("SPEAR") ? _(" Pierce: ") : _(" Cut: ")), "", int(type->melee_cut), "", false));
  dump->push_back(iteminfo("BASE", _(" To-hit bonus: "), "", int(type->m_to_hit), ((type->m_to_hit > 0) ? "+" : "")));
  dump->push_back(iteminfo("BASE", _("Moves per attack: "), "", int(attack_time()), "", true, true));
  if ( debug == true ) {
    if( g != NULL ) {
      dump->push_back(iteminfo("BASE", _("age: "), "",  (int(g->turn) - bday) / (10 * 60), "", true, true));
    }
    dump->push_back(iteminfo("BASE", _("burn: "), "",  burnt, "", true, true));
  }
  if (type->techniques != 0)
  for (int i = 1; i < NUM_TECHNIQUES; i++)
   if (type->techniques & mfb(i))
    dump->push_back(iteminfo("TECHNIQUE", "+",default_technique_name( technique_id(i) )));
 }

 if (is_food()) {
  it_comest* food = dynamic_cast<it_comest*>(type);

  dump->push_back(iteminfo("FOOD", _("Nutrition: "), "", int(food->nutr)));
  dump->push_back(iteminfo("FOOD", _("Quench: "), "", int(food->quench)));
  dump->push_back(iteminfo("FOOD", _("Enjoyability: "), "", int(food->fun)));
  if (corpse != NULL &&
    ( debug == true ||
      ( g != NULL &&
        ( g->u.has_bionic("bio_scent_vision") || g->u.has_trait(PF_CARNIVORE) || g->u.has_artifact_with(AEP_SUPER_CLAIRVOYANCE) )
      )
    )
  ) {
    dump->push_back(iteminfo("FOOD", _("Smells like: ") + corpse->name));
  }
 } else if (is_food_container()) {
 // added charge display for debugging
  it_comest* food = dynamic_cast<it_comest*>(contents[0].type);

  dump->push_back(iteminfo("FOOD", _("Nutrition: "), "", int(food->nutr)));
  dump->push_back(iteminfo("FOOD", _("Quench: "), "", int(food->quench)));
  dump->push_back(iteminfo("FOOD", _("Enjoyability: "), "", int(food->fun)));
  dump->push_back(iteminfo("FOOD", _("Portions: "), "", abs(int(contents[0].charges))));

 } else if (is_ammo()) {
  // added charge display for debugging
  it_ammo* ammo = dynamic_cast<it_ammo*>(type);

  dump->push_back(iteminfo("AMMO", _("Type: "), ammo_name(ammo->type)));
  dump->push_back(iteminfo("AMMO", _("Damage: "), "", int(ammo->damage)));
  dump->push_back(iteminfo("AMMO", _("Armor-pierce: "), "", int(ammo->pierce)));
  dump->push_back(iteminfo("AMMO", _("Range: "), "", int(ammo->range)));
  dump->push_back(iteminfo("AMMO", _("Dispersion: "), "", int(ammo->dispersion)));
  dump->push_back(iteminfo("AMMO", _("Recoil: "), "", int(ammo->recoil), "", true, true));
  dump->push_back(iteminfo("AMMO", _("Count: "), "", int(ammo->count)));

 } else if (is_ammo_container()) {
  it_ammo* ammo = dynamic_cast<it_ammo*>(contents[0].type);

  dump->push_back(iteminfo("AMMO", _("Type: "), ammo_name(ammo->type)));
  dump->push_back(iteminfo("AMMO", _("Damage: "), "", int(ammo->damage)));
  dump->push_back(iteminfo("AMMO", _("Armor-pierce: "), "", int(ammo->pierce)));
  dump->push_back(iteminfo("AMMO", _("Range: "), "", int(ammo->range)));
  dump->push_back(iteminfo("AMMO", _("Dispersion: "), "", int(ammo->dispersion)));
  dump->push_back(iteminfo("AMMO", _("Recoil: "), "", int(ammo->recoil), "", true, true));
  dump->push_back(iteminfo("AMMO", _("Count: "), "", int(contents[0].charges)));

 } else if (is_gun()) {
  it_gun* gun = dynamic_cast<it_gun*>(type);
  int ammo_dam = 0, ammo_range = 0, ammo_recoil = 0;
  bool has_ammo = (curammo != NULL && charges > 0);
  if (has_ammo) {
   ammo_dam = curammo->damage;
   ammo_range = curammo->range;
   ammo_recoil = curammo->recoil;
  }

  dump->push_back(iteminfo("GUN", _("Skill used: "), gun->skill_used->name()));
  dump->push_back(iteminfo("GUN", _("Ammunition: "), string_format(_("<num> rounds of %s"), ammo_name(ammo_type()).c_str()), int(clip_size())));

  temp1.str("");
  if (has_ammo)
   temp1 << ammo_dam;

  temp1 << (gun_damage(false) >= 0 ? "+" : "" );

  temp2.str("");
  if (has_ammo)
   temp2 << string_format(_("<num> = %d"), gun_damage());

  dump->push_back(iteminfo("GUN", _("Damage: "), temp2.str(), int(gun_damage(false)), temp1.str()));

  temp1.str("");
  if (has_ammo) {
   temp1 << ammo_range;
  }
  temp1 << (range(NULL) >= 0 ? "+" : "");

  temp2.str("");
  if (has_ammo) {
   temp2 << string_format(_("<num> = %d"), range(NULL));
  }

  dump->push_back(iteminfo("GUN", _("Range: "), temp2.str(), int(gun->range), temp1.str()));

  dump->push_back(iteminfo("GUN", _("Dispersion: "), "%d", int(dispersion())));


  temp1.str("");
  if (has_ammo)
   temp1 << ammo_recoil;

  temp1 << (recoil(false) >= 0 ? "+" : "" );

  temp2.str("");
  if (has_ammo)
   temp2 << string_format(_("<num> = %d"), recoil());

  dump->push_back(iteminfo("GUN",_("Recoil: "), temp2.str(), int(recoil(false)), temp1.str(), true, true));

  dump->push_back(iteminfo("GUN", _("Reload time: "), ((has_flag("RELOAD_ONE")) ? _("<num> per round") : ""), int(gun->reload_time), "", true, true));

  if (burst_size() == 0) {
   if (gun->skill_used == Skill::skill("pistol") && has_flag("RELOAD_ONE"))
    dump->push_back(iteminfo("GUN", _("Revolver.")));
   else
    dump->push_back(iteminfo("GUN", _("Semi-automatic.")));
  } else
   dump->push_back(iteminfo("GUN", _("Burst size: "), "", int(burst_size())));

  if (contents.size() > 0)
   dump->push_back(iteminfo("GUN", "\n"));

  temp1.str("");
  for (int i = 0; i < contents.size(); i++)
   temp1 << "\n+" << contents[i].tname();

  dump->push_back(iteminfo("GUN", temp1.str()));

 } else if (is_gunmod()) {
  it_gunmod* mod = dynamic_cast<it_gunmod*>(type);

  if (mod->dispersion != 0)
   dump->push_back(iteminfo("GUNMOD", _("Dispersion: "), "", int(mod->dispersion), ((mod->dispersion > 0) ? "+" : "")));
  if (mod->damage != 0)
   dump->push_back(iteminfo("GUNMOD", _("Damage: "), "", int(mod->damage), ((mod->damage > 0) ? "+" : "")));
  if (mod->clip != 0)
   dump->push_back(iteminfo("GUNMOD", _("Magazine: "), "<num>%%", int(mod->clip), ((mod->clip > 0) ? "+" : "")));
  if (mod->recoil != 0)
   dump->push_back(iteminfo("GUNMOD", _("Recoil: "), "", int(mod->recoil), ((mod->recoil > 0) ? "+" : ""), true, true));
  if (mod->burst != 0)
   dump->push_back(iteminfo("GUNMOD", _("Burst: "), "", int(mod->burst), (mod->burst > 0 ? "+" : "")));

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
  dump->push_back(iteminfo("ARMOR", _("Coverage: "), _("<num> percent"), int(armor->coverage)));
    if (has_flag("FIT"))
    {
        dump->push_back(iteminfo("ARMOR", _("Encumberment: "), "<num> (fits)", int(armor->encumber) - 1, "", true, true));
    }
    else
    {
        dump->push_back(iteminfo("ARMOR", _("Encumberment: "), "", int(armor->encumber), "", true, true));
    }
  dump->push_back(iteminfo("ARMOR", _("Protection: Bash: "), "", int(bash_resist()), "", false));
  dump->push_back(iteminfo("ARMOR", _("   Cut: "), "", int(cut_resist()), "", true));
  dump->push_back(iteminfo("ARMOR", _("Environmental protection: "), "", int(armor->env_resist)));
  dump->push_back(iteminfo("ARMOR", _("Warmth: "), "", int(armor->warmth)));
  dump->push_back(iteminfo("ARMOR", _("Storage: "), "", int(armor->storage)));

} else if (is_book()) {

  it_book* book = dynamic_cast<it_book*>(type);
  if (!book->type)
   dump->push_back(iteminfo("BOOK", _("Just for fun.")));
  else {
    dump->push_back(iteminfo("BOOK", "", string_format(_("Can bring your %s skill to <num>"), book->type->name().c_str()), int(book->level)));

   if (book->req == 0)
    dump->push_back(iteminfo("BOOK", _("It can be understood by beginners.")));
   else
    dump->push_back(iteminfo("BOOK", "", string_format(_("Requires %s level <num> to understand."), book->type->name().c_str()), int(book->req), "", true, true));
  }

  dump->push_back(iteminfo("BOOK", "", _("Requires intelligence of <num> to easily read."), int(book->intel), "", true, true));
  if (book->fun != 0)
   dump->push_back(iteminfo("BOOK", "", _("Reading this book affects your morale by <num>"), int(book->fun), (book->fun > 0 ? "+" : "")));

  dump->push_back(iteminfo("BOOK", "", _("This book takes <num> minutes to read."), int(book->time), "", true, true));

  if (book->recipes.size() > 0) {
   dump->push_back(iteminfo("BOOK", "", _("This book contains <num> crafting recipes."), book->recipes.size(), "", true, true));
  }

 } else if (is_tool()) {
  it_tool* tool = dynamic_cast<it_tool*>(type);

  if ((tool->max_charges)!=0) {
   if (has_flag("DOUBLE_AMMO")) {
    dump->push_back(iteminfo("TOOL", "", ((tool->ammo == "NULL")?_("Maximum <num> charges (doubled)."):string_format(_("Maximum <num> charges (doubled) of %s."), ammo_name(tool->ammo).c_str())), int(tool->max_charges*2)));
   } else {
    dump->push_back(iteminfo("TOOL", "", ((tool->ammo == "NULL")?_("Maximum <num> charges."):string_format(_("Maximum <num> charges of %s."), ammo_name(tool->ammo).c_str())), int(tool->max_charges)));
   }
  }

 } else if (is_style()) {
  it_style* style = dynamic_cast<it_style*>(type);
  dump->push_back(iteminfo("STYLE", ""));
  for (int i = 0; i < style->moves.size(); i++) {
   dump->push_back(iteminfo("STYLE", default_technique_name(style->moves[i].tech), _(". Requires Unarmed Skill of <num>"), int(style->moves[i].level)));
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
        dump->push_back(iteminfo("DESCRIPTION", "This piece of clothing fits you perfectly."));
    }
    if (is_armor() && has_flag("POCKETS"))
    {
        dump->push_back(iteminfo("DESCRIPTION", "\n\n"));
        dump->push_back(iteminfo("DESCRIPTION", "This piece of clothing has pockets to warm your hands."));
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
        dump->push_back(iteminfo("DESCRIPTION", "The film strip on the badge is " +
                                 rad_threshold_colors[i - 1] + "."));
    }
    if (is_tool() && has_flag("DOUBLE_AMMO"))
    {
        dump->push_back(iteminfo("DESCRIPTION", "\n\n"));
        dump->push_back(iteminfo("DESCRIPTION", "This tool has double the normal maximum charges."));
    }
    std::map<std::string, std::string>::iterator item_note = item_vars.find("item_note");
    if ( item_note != item_vars.end() ) {
        dump->push_back(iteminfo("DESCRIPTION", "\n" ));
        dump->push_back(iteminfo("DESCRIPTION", item_note->second ));
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
  if (vecData[i].iValue != -999)
      temp1 << vecData[i].iValue;
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
      (tmp->intel == 0 || !u->has_trait(PF_ILLITERATE)) &&
      (u->skillLevel(tmp->type) >= (int)tmp->req) &&
      (u->skillLevel(tmp->type) < (int)tmp->level))
   ret = c_ltblue;
 }
 return ret;
}

nc_color item::color_in_inventory(player *u)
{
// Items in our inventory get colorized specially
 nc_color ret = c_white;
 if (active && !is_food() && !is_food_container())
  ret = c_yellow;

 return ret;
}

std::string item::tname(game *g)
{
 std::stringstream ret;

// MATERIALS-TODO: put this in json
 std::string damtext = "";
 if (damage != 0 && !is_null()) {
  if (damage == -1) {
    damtext = _("<dam_adj>reinforced ");
  } else {
   if (type->id == "corpse") {
    if (damage == 1) damtext = _("<dam_adj>bruised ");
    if (damage == 2) damtext = _("<dam_adj>damaged ");
    if (damage == 3) damtext = _("<dam_adj>mangled ");
    if (damage == 4) damtext = _("<dam_adj>pulped ");
   } else {
    damtext = string_format("<dam_adj>%s ", type->dmg_adj(damage).c_str());
   }
  }
  damtext  = damtext.substr(9);
 }
 
 std::string vehtext = "";
 if (is_var_veh_part()){
  if(type->bigness_aspect == BIGNESS_ENGINE_DISPLACEMENT){ //liters, e.g. "3.21-Liter V8 engine"
   ret.str("");
   ret.precision(4);
   ret << (float)bigness/100;
   vehtext = string_format(_("<veh_adj>%s-Liter "), ret.str().c_str());
  }
  else if(type->bigness_aspect == BIGNESS_WHEEL_DIAMETER) { //inches, e.g. "20" wheel"
   vehtext = string_format(_("<veh_adj>%d\" "), bigness);
  }
 }
 if(vehtext.length()>9){
     vehtext = vehtext.substr(9);
 }
 
 std::string burntext = "";
 if (volume() >= 4 && burnt >= volume() * 2)
  burntext = std::string(_("<burnt_adj>badly burnt "));
 else if (burnt > 0)
  burntext = std::string(_("<burnt_adj>burnt "));
 if(burntext.length()>11){
     burntext = burntext.substr(9);
 }

 std::string maintext = "";
 if (typeId() == "corpse") {
  if (name != "")
   maintext = string_format(_("<item_name>%s corpse of %s"), corpse->name.c_str(), name.c_str());
  else maintext = string_format(_("<item_name>%s corpse"), corpse->name.c_str());
 } else if (typeId() == "blood") {
  if (corpse == NULL || corpse->id == mon_null)
   maintext = _("<item_name>human blood");
  else
   maintext = string_format("<item_name>%s blood", corpse->name.c_str());
 }
 else if (is_gun() && contents.size() > 0 ) {
  ret.str();
  ret << "<item_name>" << type->name;
  for (int i = 0; i < contents.size(); i++)
   ret << "+";
  maintext = ret.str();
 } else if (contents.size() == 1) {
  maintext = string_format(
      contents[0].made_of(LIQUID)?
      _("<item_name>%s of %s"):
      _("<item_name>%s with %s"),
      type->name.c_str(), contents[0].tname().c_str()
  );
 }
 else if (contents.size() > 0) {
  maintext = string_format("<item_name>%s, full", type->name.c_str());
 } else {
  maintext = "<item_name>" + type->name;
 }
 if(maintext.length()>11){
     maintext = maintext.substr(11);
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
   int(g->turn) - (int)(food->bday) > food_type->spoils * 600)
   ret << _(" (rotten)");

 if (has_flag("FIT")){
     ret << " (fits)";
 }

 if (owned > 0)
  ret << _(" (owned)");

 tagtext = ret.str();

 ret.str("");

 ret << damtext << vehtext << burntext << maintext << tagtext;

 return ret.str();
}

nc_color item::color() const
{
 if (typeId() == "corpse")
  return corpse->color;
 if( is_null() )
  return c_black;
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
 if (typeId() == "corpse") {
  int ret;
  switch (corpse->size) {
   case MS_TINY:   ret =    5;	break;
   case MS_SMALL:  ret =   60;	break;
   case MS_MEDIUM: ret =  520;	break;
   case MS_LARGE:  ret = 2000;	break;
   case MS_HUGE:   ret = 4000;	break;
  }
  if (made_of("veggy"))
   ret /= 10;
  else if (made_of("iron") || made_of("steel") || made_of("stone"))
   ret *= 5;
  return ret;
 }

 if( is_null() )
  return 0;

 int ret = type->weight;

 if (count_by_charges() && !made_of(LIQUID)) {
 ret *= charges;
 ret /= 100;
 }

 for (int i = 0; i < contents.size(); i++)
  if (contents[i].made_of(LIQUID))
  {
    if (contents[i].type->is_food())
      {
        it_comest* tmp_comest = dynamic_cast<it_comest*>(contents[i].type);
        ret += contents[i].weight() * (contents[i].charges / tmp_comest->charges);
      }
      else if (contents[i].type->is_ammo())
      {
        it_ammo* tmp_ammo = dynamic_cast<it_ammo*>(contents[i].type);
        ret += contents[i].weight() * (contents[i].charges / tmp_ammo->count);
      }
      else
        ret += contents[i].weight();
  }
  else
  ret += contents[i].weight();

 return ret;
}

int item::volume() const
{
 if (typeId() == "corpse") {
  switch (corpse->size) {
   case MS_TINY:   return   2;
   case MS_SMALL:  return  40;
   case MS_MEDIUM: return  75;
   case MS_LARGE:  return 160;
   case MS_HUGE:   return 600;
  }
 }

 if( is_null() )
  return 0;

 int ret = type->volume;

 if (count_by_charges()) {
 ret *= charges;
 ret /= 100;
 }

 if (is_gun()) {
  for (int i = 0; i < contents.size(); i++)
   ret += contents[i].volume();
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
 int ret = 65 + 4 * volume() + 2 * weight();
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
   if (contents[i].typeId() == "bayonet")
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
             if (contents[i].has_flag(f) && contents[i].has_flag("MODE_AUX")) {
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

bool item::has_technique(technique_id tech, player *p)
{
 if (is_style()) {
  it_style *style = dynamic_cast<it_style*>(type);
  for (int i = 0; i < style->moves.size(); i++) {
   if (style->moves[i].tech == tech &&
       (!p || p->skillLevel("unarmed") >= style->moves[i].level))
    return true;
  }
 }
 if( is_null() )
  return false;
 return (type->techniques & mfb(tech));
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

std::vector<technique_id> item::techniques()
{
 std::vector<technique_id> ret;
 for (int i = 0; i < NUM_TECHNIQUES; i++) {
  if (has_technique( technique_id(i) ))
   ret.push_back( technique_id(i) );
 }
 return ret;
}

bool item::rotten(game *g)
{
 if (!is_food() || g == NULL)
  return false;
 it_comest* food = dynamic_cast<it_comest*>(type);
 return (food->spoils != 0 && int(g->turn) - (int)bday > food->spoils * 600);
}

bool item::ready_to_revive(game *g)
{
    if (OPTIONS[OPT_REVIVE_ZOMBIES]) {
        if (type->id != "corpse" || corpse->species != species_zombie || damage >= 4)
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

 if (is_style())
  my_value += 15 * p->skillLevel("unarmed") + 8 * p->skillLevel("melee");

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

style_move item::style_data(technique_id tech)
{
 style_move ret;

 if (!is_style())
  return ret;

 it_style* style = dynamic_cast<it_style*>(type);

 for (int i = 0; i < style->moves.size(); i++) {
  if (style->moves[i].tech == tech)
   return style->moves[i];
 }

 return ret;
}

bool item::is_two_handed(player *u)
{
    if (has_flag("ALWAYS_TWOHAND"))
    {
        return true;
    }
    return (weight() > u->str_cur * 4);
}

bool item::made_of(std::string mat_ident) const
{
 if( is_null() )
  return false;

 if (typeId() == "corpse")
  return (corpse->mat == mat_ident);

    return (type->m1 == mat_ident || type->m2 == mat_ident);
}

std::string item::get_material(int m) const
{
    if (typeId() == "corpse")
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
   curammo->type == "pebble"  // sling[shot]
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

bool item::is_style() const
{
 if( is_null() )
  return false;

 return type->is_style();
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
  ret = 100 + volume() + weight();

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

 int ret = (curammo ? dynamic_cast<it_gun*>(type)->range + curammo->range : 0);

 if (has_flag("STR8_DRAW") && p) {
  if (p->str_cur < 4)
   return 0;
  else if (p->str_cur < 8)
   ret -= 2 * (8 - p->str_cur);
 } else if (has_flag("STR10_DRAW") && p) {
  if (p->str_cur < 5)
   return 0;
  else if (p->str_cur < 10)
   ret -= 2 * (10 - p->str_cur);
 } else if (has_flag("STR12_DRAW") && p) {
   if (p->str_cur < 6)
     return 0;
   if (p->str_cur < 12)
     ret -= 2 * (12 - p->str_cur);
 }

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

 std::vector<item*> am;	// List of valid ammo

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

 char am_invlet = 0;

 if (am.size() > 1 && interactive) {// More than one option; list 'em and pick
   WINDOW* w_ammo = newwin(am.size() + 1, FULL_SCREEN_WIDTH, VIEW_OFFSET_Y, VIEW_OFFSET_X);
   char ch;
   clear();
   it_ammo* ammo_def;
   mvwprintw(w_ammo, 0, 0, _("\
Choose ammo type:         Damage     Armor Pierce     Range     Accuracy"));
   for (int i = 0; i < am.size(); i++) {
    ammo_def = dynamic_cast<it_ammo*>(am[i]->type);
    mvwaddch(w_ammo, i + 1, 1, i + 'a');
    mvwprintw(w_ammo, i + 1, 3, "%s (%d)", am[i]->tname().c_str(),
                                           am[i]->charges);
    mvwprintw(w_ammo, i + 1, 27, "%d", ammo_def->damage);
    mvwprintw(w_ammo, i + 1, 38, "%d", ammo_def->pierce);
    mvwprintw(w_ammo, i + 1, 55, "%d", ammo_def->range);
    mvwprintw(w_ammo, i + 1, 65, "%d", 100 - ammo_def->dispersion);
   }
   refresh();
   wrefresh(w_ammo);
   do
    ch = getch();
   while ((ch < 'a' || ch - 'a' > am.size() - 1) && ch != ' ' && ch != 27);
   werase(w_ammo);
   delwin(w_ammo);
   erase();
   if (ch == ' ' || ch == 27)
    am_invlet = 0;
   else
    am_invlet = am[ch - 'a']->invlet;
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
  if (single_load || max_load == 1) {	// Only insert one cartridge!
   reload_target->charges++;
   ammo_to_use->charges--;
  }
  else if (reload_target->typeId() == "adv_UPS_off" || reload_target->typeId() == "adv_UPS_on") {
      int charges_per_plut = 500;
      int max_plut = std::floor( (max_load - reload_target->charges) / charges_per_plut );
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
      else
      {
          u.i_remn(ammo_invlet);
      }
  }
  return true;
 } else
  return false;
}

void item::use(player &u)
{
 if (charges > 0)
  charges--;
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

    return ((cur_mat1->fire_resist() + cur_mat2->elec_resist()) <= 0);
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

item item::clone(){
    return item(type, bday);
}
