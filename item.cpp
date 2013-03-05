#include "item.h"
#include "player.h"
#include "output.h"
#include "skill.h"
#include "game.h"
#include <sstream>
#include "cursesdef.h"

bool is_flammable(material m);

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
 mode = IF_NULL;
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
 mode = IF_NULL;
 active = false;
 curammo = NULL;
 corpse = NULL;
 owned = -1;
 mission_id = -1;
 player_id = -1;
 if (it == NULL)
  return;
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
  if (tool->max_charges == 0)
   charges = -1;
  else
   charges = tool->def_charges;
 } else if (it->is_gunmod() && it->id == itm_spare_mag) {
  charges = 0;
 } else
  charges = -1;
 if(it->is_var_veh_part()){
  it_var_veh_part* varcarpart = dynamic_cast<it_var_veh_part*>(it);
  bigness= rng( varcarpart->min_bigness, varcarpart->max_bigness);
 }
}

item::item(itype *it, unsigned int turn, char let)
{
 if(!it)
  type = nullitem();
 else
  type = it;
 bday = turn;
 name = "";
 damage = 0;
 burnt = 0;
 poison = 0;
 mode = IF_NULL;
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
 } else if (it->is_gunmod() && it->id == itm_spare_mag) {
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
}

void item::make_corpse(itype* it, mtype* mt, unsigned int turn)
{
 name = "";
 charges = -1;
 invlet = 0;
 damage = 0;
 burnt = 0;
 poison = 0;
 mode = IF_NULL;
 curammo = NULL;
 active = false;
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

bool item::is_null()
{
 return (type == NULL || type->id == 0);
}

item item::in_its_container(std::vector<itype*> *itypes)
{

 if (is_software()) {
  item ret( (*itypes)[itm_usb_drive], 0);
  ret.contents.push_back(*this);
  ret.invlet = invlet;
  return ret;
 }

  if (!is_food() || (dynamic_cast<it_comest*>(type))->container == itm_null)
  return *this;

    it_comest *food = dynamic_cast<it_comest*>(type);
    item ret((*itypes)[food->container], bday);

  if (dynamic_cast<it_comest*>(type)->container == itm_can_food)
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
 return ((invlet >= 'a' && invlet <= 'z') || (invlet >= 'A' && invlet <= 'Z'));
}

bool item::stacks_with(item rhs)
{

 bool stacks = (type   == rhs.type   && damage  == rhs.damage  &&
                active == rhs.active && charges == rhs.charges &&
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

std::string item::save_info()
{
 if (type == NULL)
  debugmsg("Tried to save an item with NULL type!");
 int ammotmp = 0;
/* TODO: This causes a segfault sometimes, even though we check to make sure
 * curammo isn't NULL.  The crashes seem to occur most frequently when saving an
 * NPC, or when saving map data containing an item an NPC has dropped.
 */
 if (curammo != NULL)
  ammotmp = curammo->id;
 if (ammotmp < 0 || ammotmp > num_items)
  ammotmp = 0; // Saves us from some bugs
 std::stringstream dump;// (std::stringstream::in | std::stringstream::out);
 dump << " " << int(invlet) << " " << int(typeId()) << " " <<  int(charges) <<
         " " << int(damage) << " " << int(burnt) << " " << poison << " " <<
         ammotmp << " " << owned << " " << int(bday);
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
 while (pos != std::string::npos)  {
  name.replace(pos, 1, "@@");
  pos = name.find_first_of("\n");
 }
 dump << " '" << name << "'";
 return dump.str();
}

void item::load_info(std::string data, game *g)
{
 std::stringstream dump;
 dump << data;
 int idtmp, ammotmp, lettmp, damtmp, burntmp, acttmp, corp;
 dump >> lettmp >> idtmp >> charges >> damtmp >> burntmp >> poison >> ammotmp >>
         owned >> bday >> acttmp >> corp >> mission_id >> player_id;
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
 burnt = burntmp;
 active = false;
 mode = IF_NULL;
 if (acttmp == 1)
  active = true;
 if (ammotmp > 0)
  curammo = dynamic_cast<it_ammo*>(g->itypes[ammotmp]);
 else
  curammo = NULL;
}

std::string item::info(bool showtext)
{
 std::vector<iteminfo> dummy;
 return info(showtext, &dummy);
}

std::string item::info(bool showtext, std::vector<iteminfo> *dump)
{
 std::stringstream temp1, temp2;

 if( !is_null() )
 {
  dump->push_back(iteminfo("BASE", " Volume: ", "", int(volume()), "", false, true));
  dump->push_back(iteminfo("BASE", "    Weight: ", "", int(weight()), "", true, true));
  dump->push_back(iteminfo("BASE", " Bash: ", "", int(type->melee_dam), "", false));
  dump->push_back(iteminfo("BASE", (has_flag(IF_SPEAR) ? "  Pierce: " : "  Cut: "), "", int(type->melee_cut), "", false));
  dump->push_back(iteminfo("BASE", "  To-hit bonus: ", ((type->m_to_hit > 0) ? "+" : ""), int(type->m_to_hit), ""));
  dump->push_back(iteminfo("BASE", " Moves per attack: ", "", int(attack_time()), "", true, true));

  /*
  dump << " Volume: " << volume() << "    Weight: " << weight() << "\n" <<
          " Bash: " << int(type->melee_dam) <<
          (has_flag(IF_SPEAR) ? "  Pierce: " : "  Cut: ") <<
          int(type->melee_cut) << "  To-hit bonus: " <<
          (type->m_to_hit > 0 ? "+" : "" ) << int(type->m_to_hit) << "\n" <<
          " Moves per attack: " << attack_time() << "\n";
  */
 }

 if (is_food()) {
  it_comest* food = dynamic_cast<it_comest*>(type);

  dump->push_back(iteminfo("FOOD", " Nutrition: ", "", int(food->nutr)));
  dump->push_back(iteminfo("FOOD", " Quench: ", "", int(food->quench)));
  dump->push_back(iteminfo("FOOD", " Enjoyability: ", "", int(food->fun)));

  /*
  dump << " Nutrition: " << int(food->nutr) << "\n Quench: " <<
          int(food->quench) << "\n Enjoyability: " << int(food->fun);
  */

 } else if (is_food_container()) {
 // added charge display for debugging
  it_comest* food = dynamic_cast<it_comest*>(contents[0].type);

  dump->push_back(iteminfo("FOOD", " Nutrition: ", "", int(food->nutr)));
  dump->push_back(iteminfo("FOOD", " Quench: ", "", int(food->quench)));
  dump->push_back(iteminfo("FOOD", " Enjoyability: ", "", int(food->fun)));
  dump->push_back(iteminfo("FOOD", " Charges: ", "", int(contents[0].charges)));

  /*
  dump << " Nutrition: " << int(food->nutr) << "\n Quench: " <<
          int(food->quench) << "\n Enjoyability: " << int(food->fun)
          << "\n Charges: " << int(contents[0].charges);
  */

 } else if (is_ammo()) {
  // added charge display for debugging
  it_ammo* ammo = dynamic_cast<it_ammo*>(type);

  dump->push_back(iteminfo("AMMO", " Type: ", ammo_name(ammo->type)));
  dump->push_back(iteminfo("AMMO", " Damage: ", "", int(ammo->damage)));
  dump->push_back(iteminfo("AMMO", " Armor-pierce: ", "", int(ammo->pierce)));
  dump->push_back(iteminfo("AMMO", " Range: ", "", int(ammo->range)));
  dump->push_back(iteminfo("AMMO", " Accuracy: ", "", int(100 - ammo->accuracy)));
  dump->push_back(iteminfo("AMMO", " Recoil: ", "", int(ammo->recoil), "", true, true));
  dump->push_back(iteminfo("AMMO", " Count: ", "", int(ammo->count)));

  /*
  dump << " Type: " << ammo_name(ammo->type) << "\n Damage: " <<
           int(ammo->damage) << "\n Armor-pierce: " << int(ammo->pierce) <<
           "\n Range: " << int(ammo->range) << "\n Accuracy: " <<
           int(100 - ammo->accuracy) << "\n Recoil: " << int(ammo->recoil)
           << "\n Count: " << int(ammo->count);
  */

 } else if (is_ammo_container()) {
  it_ammo* ammo = dynamic_cast<it_ammo*>(contents[0].type);

  dump->push_back(iteminfo("AMMO", " Type: ", ammo_name(ammo->type)));
  dump->push_back(iteminfo("AMMO", " Damage: ", "", int(ammo->damage)));
  dump->push_back(iteminfo("AMMO", " Armor-pierce: ", "", int(ammo->pierce)));
  dump->push_back(iteminfo("AMMO", " Range: ", "", int(ammo->range)));
  dump->push_back(iteminfo("AMMO", " Accuracy: ", "", int(100 - ammo->accuracy)));
  dump->push_back(iteminfo("AMMO", " Recoil: ", "", int(ammo->recoil), "", true, true));
  dump->push_back(iteminfo("AMMO", " Count: ", "", int(contents[0].charges)));

  /*
  dump << " Type: " << ammo_name(ammo->type) << "\n Damage: " <<
           int(ammo->damage) << "\n Armor-pierce: " << int(ammo->pierce) <<
           "\n Range: " << int(ammo->range) << "\n Accuracy: " <<
           int(100 - ammo->accuracy) << "\n Recoil: " << int(ammo->recoil)
           << "\n Count: " << int(contents[0].charges);
  */

 } else if (is_gun()) {
  it_gun* gun = dynamic_cast<it_gun*>(type);
  int ammo_dam = 0, ammo_recoil = 0;
  bool has_ammo = (curammo != NULL && charges > 0);
  if (has_ammo) {
   ammo_dam = curammo->damage;
   ammo_recoil = curammo->recoil;
  }

  dump->push_back(iteminfo("GUN", " Skill used: ", gun->skill_used->name()));
  dump->push_back(iteminfo("GUN", " Ammunition: ", "", int(clip_size()), " rounds of " + ammo_name(ammo_type())));

  /*
  dump << " Skill used: " << gun->skill_used->name() << "\n Ammunition: " <<
          clip_size() << " rounds of " << ammo_name(ammo_type());

  dump << "\n Damage: ";
  */

  temp1.str("");
  if (has_ammo)
   temp1 << ammo_dam; //dump << ammo_dam;

  temp1 << (gun_damage(false) >= 0 ? "+" : "" );
  //dump << (gun_damage(false) >= 0 ? "+" : "" ) << gun_damage(false);

  temp2.str("");
  if (has_ammo)
   temp2 << " = " << gun_damage(); //dump << " = " << gun_damage();

  dump->push_back(iteminfo("GUN", " Damage: ", temp1.str(), int(gun_damage(false)), temp2.str()));
  dump->push_back(iteminfo("GUN", " Accuracy: ", "", int(100 - accuracy())));
  //dump << "\n Accuracy: " << int(100 - accuracy());

  //dump << "\n Recoil: ";
  temp1.str("");
  if (has_ammo)
   temp1 << ammo_dam; //dump << ammo_recoil;

  temp1 << (recoil(false) >= 0 ? "+" : "" );
  //dump << (recoil(false) >= 0 ? "+" : "" ) << recoil(false);

  temp2.str("");
  if (has_ammo)
   temp2 << " = " << recoil(); //dump << " = " << recoil();

  dump->push_back(iteminfo("GUN"," Recoil: ", temp1.str(), int(recoil(false)), temp2.str(), true, true));

  //dump << "\n Reload time: " << int(gun->reload_time);
  //if (has_flag(IF_RELOAD_ONE))
   //dump << " per round";

  dump->push_back(iteminfo("GUN", " Reload time: ", "", int(gun->reload_time), ((has_flag(IF_RELOAD_ONE)) ? " per round" : ""), true, true));

  if (burst_size() == 0) {
   if (gun->skill_used == Skill::skill("pistol") && has_flag(IF_RELOAD_ONE))
    dump->push_back(iteminfo("GUN", " Revolver.")); //dump << "\n Revolver.";
   else
    dump->push_back(iteminfo("GUN", " Semi-automatic.")); //dump << "\n Semi-automatic.";
  } else
   dump->push_back(iteminfo("GUN", " Burst size: ", "", int(burst_size()))); //dump << "\n Burst size: " << burst_size();

  if (contents.size() > 0)
   dump->push_back(iteminfo("GUN", "\n")); //dump << "\n";

  temp1.str("");
  for (int i = 0; i < contents.size(); i++)
   temp1 << "\n+" << contents[i].tname();

  dump->push_back(iteminfo("GUN", temp1.str())); //

 } else if (is_gunmod()) {
  it_gunmod* mod = dynamic_cast<it_gunmod*>(type);

  if (mod->accuracy != 0)
   dump->push_back(iteminfo("GUNMOD", " Accuracy: ", ((mod->accuracy > 0) ? "+" : ""), int(mod->accuracy))); //dump << " Accuracy: " << (mod->accuracy > 0 ? "+" : "") << int(mod->accuracy);
  if (mod->damage != 0)
   dump->push_back(iteminfo("GUNMOD", " Damage: ", ((mod->damage > 0) ? "+" : ""), int(mod->damage))); //dump << "\n Damage: " << (mod->damage > 0 ? "+" : "") << int(mod->damage);
  if (mod->clip != 0)
   dump->push_back(iteminfo("GUNMOD", " Magazine: ", ((mod->clip > 0) ? "+" : ""), int(mod->clip), "%")); //dump << "\n Magazine: " << (mod->clip > 0 ? "+" : "") << int(mod->damage) << "%";
  if (mod->recoil != 0)
   dump->push_back(iteminfo("GUNMOD", " Recoil: ", ((mod->recoil > 0) ? "+" : ""), int(mod->recoil), "", true, true)); //dump << "\n Recoil: " << int(mod->recoil);
  if (mod->burst != 0)
   dump->push_back(iteminfo("GUNMOD", " Burst: ", (mod->burst > 0 ? "+" : ""), int(mod->burst))); //dump << "\n Burst: " << (mod->clip > 0 ? "+" : "") << int(mod->clip);

  if (mod->newtype != AT_NULL)
   dump->push_back(iteminfo("GUNMOD", " " + ammo_name(mod->newtype))); //dump << "\n " << ammo_name(mod->newtype);

  temp1.str("");
  temp1 << " Used on: ";
  if (mod->used_on_pistol)
   temp1 << "Pistols.  ";
  if (mod->used_on_shotgun)
   temp1 << "Shotguns.  ";
  if (mod->used_on_smg)
   temp1 << "SMGs.  ";
  if (mod->used_on_rifle)
   temp1 << "Rifles.";

  dump->push_back(iteminfo("GUNMOD", temp1.str()));

 } else if (is_armor()) {
  it_armor* armor = dynamic_cast<it_armor*>(type);

  temp1.str("");
  temp1 << " Covers: ";
  if (armor->covers & mfb(bp_head))
   temp1 << "The head. ";
  if (armor->covers & mfb(bp_eyes))
   temp1 << "The eyes. ";
  if (armor->covers & mfb(bp_mouth))
   temp1 << "The mouth. ";
  if (armor->covers & mfb(bp_torso))
   temp1 << "The torso. ";
  if (armor->covers & mfb(bp_arms))
   temp1 << "The arms. ";
  if (armor->covers & mfb(bp_hands))
   temp1 << "The hands. ";
  if (armor->covers & mfb(bp_legs))
   temp1 << "The legs. ";
  if (armor->covers & mfb(bp_feet))
   temp1 << "The feet. ";

  dump->push_back(iteminfo("ARMOR", temp1.str()));

  dump->push_back(iteminfo("ARMOR", " Encumberment: ", "", int(armor->encumber), "", true, true));
  dump->push_back(iteminfo("ARMOR", " Bashing protection: ", "", int(armor->dmg_resist)));
  dump->push_back(iteminfo("ARMOR", " Cut protection: ", "", int(armor->cut_resist)));
  dump->push_back(iteminfo("ARMOR", " Environmental protection: ", "", int(armor->env_resist)));
  dump->push_back(iteminfo("ARMOR", " Warmth: ", "", int(armor->warmth)));
  dump->push_back(iteminfo("ARMOR", " Storage: ", "", int(armor->storage)));

  /*
  dump << "\n Encumberment: "			<< int(armor->encumber) <<
          "\n Bashing protection: "		<< int(armor->dmg_resist) <<
          "\n Cut protection: "			<< int(armor->cut_resist) <<
          "\n Environmental protection: "	<< int(armor->env_resist) <<
          "\n Warmth: "				<< int(armor->warmth) <<
          "\n Storage: "			<< int(armor->storage);
  */

} else if (is_book()) {

  it_book* book = dynamic_cast<it_book*>(type);
  if (!book->type)
   dump->push_back(iteminfo("BOOK", " Just for fun.")); //dump << " Just for fun.\n";
  else {
    dump->push_back(iteminfo("BOOK", " Can bring your ", book->type->name() + " skill to ", int(book->level)));
    //dump << " Can bring your " << book->type->name() << " skill to " << int(book->level) << std::endl;
   if (book->req == 0)
    dump->push_back(iteminfo("BOOK", " It can be understood by beginners.")); //dump << " It can be understood by beginners.\n";
   else
    dump->push_back(iteminfo("BOOK", " Requires ", book->type->name() + " level ", int(book->req), " to understand.", true, true)); //dump << " Requires " << book->type->name() << " level " << int(book->req) << " to understand.\n";
  }

  dump->push_back(iteminfo("BOOK", " Requires intelligence of ", "", int(book->intel), " to easily read.", true, true));
  //dump << " Requires intelligence of " << int(book->intel) << " to easily read." << std::endl;
  if (book->fun != 0)
   dump->push_back(iteminfo("BOOK", " Reading this book affects your morale by ", (book->fun > 0 ? "+" : ""), int(book->fun))); //dump << " Reading this book affects your morale by " << (book->fun > 0 ? "+" : "") << int(book->fun) << std::endl;

  dump->push_back(iteminfo("BOOK", " This book takes ", "", int(book->time), " minutes to read.", true, true));
  //dump << " This book takes " << int(book->time) << " minutes to read.";

 } else if (is_tool()) {
  it_tool* tool = dynamic_cast<it_tool*>(type);
  /*
  dump << " Maximum " << tool->max_charges << " charges";
  if (tool->ammo == AT_NULL)
   dump << ".";
  else
   dump << " of " << ammo_name(tool->ammo) << ".";
  */
  dump->push_back(iteminfo("TOOL", " Maximum ", "", int(tool->max_charges), " charges" + ((tool->ammo == AT_NULL) ? "" : (" of " + ammo_name(tool->ammo))) + "."));

 } else if (is_style()) {
  it_style* style = dynamic_cast<it_style*>(type);

  //dump << "\n";
  for (int i = 0; i < style->moves.size(); i++) {
   dump->push_back(iteminfo("STYLE", default_technique_name(style->moves[i].tech), ". Requires Unarmed Skill of ", int(style->moves[i].level)));
   //dump << default_technique_name(style->moves[i].tech) << ". Requires Unarmed Skill of " << style->moves[i].level << "\n";
  }

 } else if (!is_null() && type->techniques != 0) {
  //dump << "\n";
  temp1.str("");
  for (int i = 1; i < NUM_TECHNIQUES; i++) {
   if (type->techniques & mfb(i))
    temp1 << default_technique_name( technique_id(i) ) + "; "; //dump << default_technique_name( technique_id(i) ) << "; ";
  }

  if (temp1.str() != "")
  dump->push_back(iteminfo("TECHNIQUE", temp1.str()));
 }

 if ( showtext && !is_null() ) {
  //dump << "\n\n" << type->description << "\n";
  dump->push_back(iteminfo("DESCRIPTION", type->description));
  if (contents.size() > 0) {
   if (is_gun()) {
    for (int i = 0; i < contents.size(); i++)
     dump->push_back(iteminfo("DESCRIPTION", contents[i].type->description));
     //dump << "\n " << contents[i].type->description;
   } else
    dump->push_back(iteminfo("DESCRIPTION", contents[0].type->description));
    //dump << "\n " << contents[0].type->description;
   //dump << "\n";
  }
 }

 temp1.str("");
 std::vector<iteminfo>& vecData = *dump; // vector is not copied here
 for (int i = 0; i < vecData.size(); i++) {
  if (vecData[i].sType == "DESCRIPTION")
   temp1 << "\n";

  temp1 << vecData[i].sName;
  temp1 << vecData[i].sPre;

  if (vecData[i].iValue != -999)
   temp1 << vecData[i].iValue;

  temp1 << vecData[i].sPost;
  temp1 << ((vecData[i].bNewLine) ? "\n" : "");
 }

 return temp1.str();
}

char item::symbol()
{
 if( is_null() )
  return ' ';
 return type->sym;
}

nc_color item::color(player *u)
{
 nc_color ret = c_ltgray;

 if (active) // Active items show up as yellow
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
   for (int i = 0; i < u->inv.size(); i++) {
    if (u->inv[i].is_gun() && u->inv[i].ammo_type() == amtype) {
     i = u->inv.size();
     ret = c_green;
    }
   }
  }
 } else if (is_book()) {
  it_book* tmp = dynamic_cast<it_book*>(type);
  if (tmp->type && tmp->intel <= u->int_cur + u->skillLevel(tmp->type).level() &&
      (tmp->intel == 0 || !u->has_trait(PF_ILLITERATE)) &&
      (u->skillLevel(tmp->type) >= tmp->req) &&
      (u->skillLevel(tmp->type) < tmp->level))
   ret = c_ltblue;
 }
 return ret;
}

nc_color item::color_in_inventory(player *u)
{
// Items in our inventory get colorized specially
 nc_color ret = c_white;
 if (active)
  ret = c_yellow;

 return ret;
}

std::string item::tname(game *g)
{
 std::stringstream ret;

 if (damage != 0 && !is_null()) {
  std::string damtext;
  switch (type->m1) {
   case VEGGY:
   case FLESH:
    damtext = "partially eaten ";
    break;
   case COTTON:
   case WOOL:
    if (damage == -1) damtext = "reinforced ";
    if (damage ==  1) damtext = "ripped ";
    if (damage ==  2) damtext = "torn ";
    if (damage ==  3) damtext = "shredded ";
    if (damage ==  4) damtext = "tattered ";
    break;
   case LEATHER:
    if (damage == -1) damtext = "reinforced ";
    if (damage ==  1) damtext = "scratched ";
    if (damage ==  2) damtext = "cut ";
    if (damage ==  3) damtext = "torn ";
    if (damage ==  4) damtext = "tattered ";
    break;
   case KEVLAR:
    if (damage == -1) damtext = "reinforced ";
    if (damage ==  1) damtext = "marked ";
    if (damage ==  2) damtext = "dented ";
    if (damage ==  3) damtext = "scarred ";
    if (damage ==  4) damtext = "broken ";
    break;
   case PAPER:
    if (damage ==  1) damtext = "torn ";
    if (damage >=  2) damtext = "shredded ";
    break;
   case WOOD:
    if (damage ==  1) damtext = "scratched ";
    if (damage ==  2) damtext = "chipped ";
    if (damage ==  3) damtext = "cracked ";
    if (damage ==  4) damtext = "splintered ";
    break;
   case PLASTIC:
   case GLASS:
    if (damage ==  1) damtext = "scratched ";
    if (damage ==  2) damtext = "cut ";
    if (damage ==  3) damtext = "cracked ";
    if (damage ==  4) damtext = "shattered ";
    break;
   case IRON:
    if (damage ==  1) damtext = "lightly rusted ";
    if (damage ==  2) damtext = "rusted ";
    if (damage ==  3) damtext = "very rusty ";
    if (damage ==  4) damtext = "thoroughly rusted ";
    break;
   default:
    //damtext = "damaged ";
    if (damage ==  1) damtext = "lightly damaged ";
    if (damage ==  2) damtext = "damaged ";
    if (damage ==  3) damtext = "very damaged ";
    if (damage ==  4) damtext = "thoroughly damaged ";
  }
  ret << damtext;
 }

 if (is_var_veh_part()){
  //if(is_engine()){
  if(type->bigness_aspect == BIGNESS_ENGINE_DISPLACEMENT){ //liters, e.g. "3.21-Liter V8 engine"
   ret.precision(4);
   ret << (float)bigness/100 << "-Liter ";
  }
  else if(type->bigness_aspect == BIGNESS_WHEEL_DIAMETER) { //inches, e.g. "20" wheel"
   ret << bigness << "\" ";
  }
 }

 if (volume() >= 4 && burnt >= volume() * 2)
  ret << "badly burnt ";
 else if (burnt > 0)
  ret << "burnt ";

 if (typeId() == itm_corpse) {
  ret << corpse->name << " corpse";
  if (name != "")
   ret << " of " << name;
  return ret.str();
 } else if (typeId() == itm_blood) {
  if (corpse == NULL || corpse->id == mon_null)
   ret << "human blood";
  else
   ret << corpse->name << " blood";
  return ret.str();
 }

 if (is_gun() && contents.size() > 0 ) {
  ret << type->name;
  for (int i = 0; i < contents.size(); i++)
   ret << "+";
 } else if (contents.size() == 1)
  ret << type->name << " of " << contents[0].tname();
 else if (contents.size() > 0)
  ret << type->name << ", full";
 else
  ret << type->name;

 it_comest* food = NULL;
 if (is_food())
  food = dynamic_cast<it_comest*>(type);
 else if (is_food_container())
  food = dynamic_cast<it_comest*>(contents[0].type);
 if (food != NULL && g != NULL && food->spoils != 0 &&
   int(g->turn) < (int)bday + 100)
  ret << " (hot)";
 if (food != NULL && g != NULL && food->spoils != 0 &&
   int(g->turn) - (int)bday > food->spoils * 600)
  ret << " (rotten)";

 if (owned > 0)
  ret << " (owned)";
 return ret.str();
}

nc_color item::color()
{
 if (typeId() == itm_corpse)
  return corpse->color;
 if( is_null() )
  return c_black;
 return type->color;
}

int item::price()
{
 if( is_null() )
  return 0;

 int ret = type->price;
 for (int i = 0; i < contents.size(); i++)
  ret += contents[i].price();
 return ret;
}

int item::weight()
{
 if (typeId() == itm_corpse) {
  int ret;
  switch (corpse->size) {
   case MS_TINY:   ret =    5;	break;
   case MS_SMALL:  ret =   60;	break;
   case MS_MEDIUM: ret =  520;	break;
   case MS_LARGE:  ret = 2000;	break;
   case MS_HUGE:   ret = 4000;	break;
  }
  if (made_of(VEGGY))
   ret /= 10;
  else if (made_of(IRON) || made_of(STEEL) || made_of(STONE))
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

int item::volume()
{
 if (typeId() == itm_corpse) {
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

int item::damage_cut()
{
 if (is_gun()) {
  for (int i = 0; i < contents.size(); i++) {
   if (contents[i].typeId() == itm_bayonet)
    return contents[i].type->melee_cut;
  }
 }
 if( is_null() )
  return 0;
 return type->melee_cut;
}

bool item::has_flag(item_flag f)
{
 if (is_gun()) {
  if (mode == IF_MODE_AUX) {
   item* gunmod = active_gunmod();
   if( gunmod != NULL )
    return gunmod->has_flag(f);
  } else {
   for (int i = 0; i < contents.size(); i++) {
     // Don't report flags from active gunmods for the gun.
    if (contents[i].has_flag(f) && contents[i].has_flag(IF_MODE_AUX))
     return true;
   }
  }
 }
 if( is_null() )
  return false;
 return (type->item_flags & mfb(f));
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

int item::has_gunmod(int type)
{
 if (!is_gun())
  return -1;
 for (int i = 0; i < contents.size(); i++)
  if (contents[i].is_gunmod() && contents[i].typeId() == type)
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

bool item::goes_bad()
{
 if (!is_food())
  return false;
 it_comest* food = dynamic_cast<it_comest*>(type);
 return (food->spoils != 0);
}

bool item::count_by_charges()
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
 else if (ammo_type() == AT_NULL)
  return true;

 return false;
}

int item::num_charges()
{
 if (is_gun()) {
  if (mode == IF_MODE_AUX) {
   item* gunmod = active_gunmod();
   if (gunmod != NULL)
    return gunmod->charges;
  } else {
   return charges;
  }
 }
 if (is_gunmod() && mode == IF_MODE_AUX)
  return charges;
 return 0;
}

int item::weapon_value(int skills[num_skill_types])
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
  gun_value -= int(gun->accuracy / 5);
  gun_value *= (.5 + (.3 * skills[sk_gun]));
  gun_value *= (.3 + (.7 * skills[gun->skill_used->id()])); /// TODO: FIXME
  my_value += gun_value;
 }

 my_value += int(type->melee_dam * (1   + .3 * skills[sk_bashing] +
                                          .1 * skills[sk_melee]    ));
 //debugmsg("My value: (+bash) %d", my_value);

 my_value += int(type->melee_cut * (1   + .4 * skills[sk_cutting] +
                                          .1 * skills[sk_melee]    ));
 //debugmsg("My value: (+cut) %d", my_value);

 my_value += int(type->m_to_hit  * (1.2 + .3 * skills[sk_melee]));
 //debugmsg("My value: (+hit) %d", my_value);

 return my_value;
}

int item::melee_value(int skills[num_skill_types])
{
 if( is_null() )
  return 0;

 int my_value = 0;
 my_value += int(type->melee_dam * (1   + .3 * skills[sk_bashing] +
                                          .1 * skills[sk_melee]    ));
 //debugmsg("My value: (+bash) %d", my_value);

 my_value += int(type->melee_cut * (1   + .4 * skills[sk_cutting] +
                                          .1 * skills[sk_melee]    ));
 //debugmsg("My value: (+cut) %d", my_value);

 my_value += int(type->m_to_hit  * (1.2 + .3 * skills[sk_melee]));
 //debugmsg("My value: (+hit) %d", my_value);

 if (is_style())
  my_value += 15 * skills[sk_unarmed] + 8 * skills[sk_melee];

 return my_value;
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
  if (is_gun() && (dynamic_cast<it_gun*>(type))->skill_used != Skill::skill("pistol"))
    return true;
  return (weight() > u->str_cur * 4);
}

bool item::made_of(material mat)
{
 if( is_null() )
  return false;

 if (typeId() == itm_corpse)
  return (corpse->mat == mat);

 return (type->m1 == mat || type->m2 == mat);
}

bool item::conductive()
{
 if( is_null() )
  return false;

 if ((type->m1 == IRON || type->m1 == STEEL || type->m1 == SILVER ||
      type->m1 == MNULL) &&
     (type->m2 == IRON || type->m2 == STEEL || type->m2 == SILVER ||
      type->m2 == MNULL))
  return true;
 if (type->m1 == MNULL && type->m2 == MNULL)
  return true;
 return false;
}

bool item::destroyed_at_zero_charges()
{
 return (is_ammo() || is_food());
}

bool item::is_var_veh_part()
{
 if( is_null() )
  return false;

 return type->is_var_veh_part();
}

bool item::is_gun()
{
 if( is_null() )
  return false;

 return type->is_gun();
}

bool item::is_gunmod()
{
 if( is_null() )
  return false;

 return type->is_gunmod();
}

bool item::is_bionic()
{
 if( is_null() )
  return false;

 return type->is_bionic();
}

bool item::is_ammo()
{
 if( is_null() )
  return false;

 return type->is_ammo();
}

bool item::is_food(player *u)
{
 if (!u)
  return is_food();

 if( is_null() )
  return false;

 if (type->is_food())
  return true;

 if (u->has_bionic(bio_batteries) && is_ammo() &&
     (dynamic_cast<it_ammo*>(type))->type == AT_BATT)
  return true;
 if (u->has_bionic(bio_furnace) && is_flammable(type->m1) &&
     is_flammable(type->m2) && typeId() != itm_corpse)
  return true;
 return false;
}

bool item::is_food_container(player *u)
{
 return (contents.size() >= 1 && contents[0].is_food(u));
}

bool item::is_food()
{
 if( is_null() )
  return false;

 if (type->is_food())
  return true;
 return false;
}

bool item::is_food_container()
{
 return (contents.size() >= 1 && contents[0].is_food());
}

bool item::is_ammo_container()
{
 return (contents.size() >= 1 && contents[0].is_ammo());
}

bool item::is_drink()
{
 if( is_null() )
  return false;

 return type->is_food() && type->m1 == LIQUID;
}

bool item::is_weap()
{
 if( is_null() )
  return false;

 if (is_gun() || is_food() || is_ammo() || is_food_container() || is_armor() ||
     is_book() || is_tool())
  return false;
 return (type->melee_dam > 7 || type->melee_cut > 5);
}

bool item::is_bashing_weapon()
{
 if( is_null() )
  return false;

 return (type->melee_dam >= 8);
}

bool item::is_cutting_weapon()
{
 if( is_null() )
  return false;

 return (type->melee_cut >= 8 && !has_flag(IF_SPEAR));
}

bool item::is_armor()
{
 if( is_null() )
  return false;

 return type->is_armor();
}

bool item::is_book()
{
 if( is_null() )
  return false;

/*
 if (type->is_macguffin()) {
  it_macguffin* mac = dynamic_cast<it_macguffin*>(type);
  return mac->readable;
 }
*/
 return type->is_book();
}

bool item::is_container()
{
 if( is_null() )
  return false;

 return type->is_container();
}

bool item::is_tool()
{
 if( is_null() )
  return false;

 return type->is_tool();
}

bool item::is_software()
{
 if( is_null() )
  return false;

 return type->is_software();
}

bool item::is_macguffin()
{
 if( is_null() )
  return false;

 return type->is_macguffin();
}

bool item::is_style()
{
 if( is_null() )
  return false;

 return type->is_style();
}

bool item::is_other()
{
 if( is_null() )
  return false;

 return (!is_gun() && !is_ammo() && !is_armor() && !is_food() &&
         !is_food_container() && !is_tool() && !is_gunmod() && !is_bionic() &&
         !is_book() && !is_weap());
}

bool item::is_artifact()
{
 if( is_null() )
  return false;

 return type->is_artifact();
}

int item::reload_time(player &u)
{
 int ret = 0;

 if (is_gun()) {
  it_gun* reloading = dynamic_cast<it_gun*>(type);
  ret = reloading->reload_time;
  if (charges == 0) {
   int spare_mag = has_gunmod(itm_spare_mag);
   if (spare_mag != -1 && contents[spare_mag].charges > 0)
    ret -= double(ret) * 0.9;
  }
  double skill_bonus = double(u.skillLevel(reloading->skill_used).level()) * .075;
  if (skill_bonus > .75)
   skill_bonus = .75;
  ret -= double(ret) * skill_bonus;
 } else if (is_tool())
  ret = 100 + volume() + weight();

 if (has_flag(IF_STR_RELOAD))
  ret -= u.str_cur * 20;
 if (ret < 25)
  ret = 25;
 ret += u.encumb(bp_hands) * 30;
 return ret;
}

item* item::active_gunmod()
{
 if( mode == IF_MODE_AUX )
  for (int i = 0; i < contents.size(); i++)
   if (contents[i].is_gunmod() && contents[i].mode == IF_MODE_AUX)
    return &contents[i];
 return NULL;
}

void item::next_mode()
{
 switch(mode) {
 case IF_NULL:
  if( has_flag(IF_MODE_BURST) )
   mode = IF_MODE_BURST;
  else
   // Enable the first mod with an AUX firing mode.
   for (int i = 0; i < contents.size(); i++)
    if (contents[i].is_gunmod() && contents[i].has_flag(IF_MODE_AUX)) {
     mode = IF_MODE_AUX;
     contents[i].mode = IF_MODE_AUX;
     break;
    }
  // Doesn't have another mode, just return.
  break;
 case IF_MODE_BURST:
  // Enable the first mod with an AUX firing mode.
  for (int i = 0; i < contents.size(); i++)
   if (contents[i].is_gunmod() && contents[i].has_flag(IF_MODE_AUX)) {
    mode = IF_MODE_AUX;
    contents[i].mode = IF_MODE_AUX;
    break;
   }
  if (mode == IF_MODE_BURST)
   mode = IF_NULL;
  break;
 case IF_MODE_AUX:
  {
   int i = 0;
   // Advance to next aux mode, or if there isn't one, normal mode
   for (; i < contents.size(); i++)
    if (contents[i].is_gunmod() && contents[i].mode == IF_MODE_AUX) {
     contents[i].mode = IF_NULL;
     break;
    }
   for (i++; i < contents.size(); i++)
    if (contents[i].is_gunmod() && contents[i].has_flag(IF_MODE_AUX)) {
     contents[i].mode = IF_MODE_AUX;
     break;
    }
   if (i == contents.size())
    mode = IF_NULL;
   break;
  }
 }
}

int item::clip_size()
{
 if(is_gunmod() && has_flag(IF_MODE_AUX))
  return (dynamic_cast<it_gunmod*>(type))->clip;
 if (!is_gun())
  return 0;

 it_gun* gun = dynamic_cast<it_gun*>(type);
 int ret = gun->clip;
 for (int i = 0; i < contents.size(); i++) {
  if (contents[i].is_gunmod() && !contents[i].has_flag(IF_MODE_AUX)) {
   int bonus = (ret * (dynamic_cast<it_gunmod*>(contents[i].type))->clip) / 100;
   ret = int(ret + bonus);
  }
 }
 return ret;
}

int item::accuracy()
{
 if (!is_gun())
  return 0;
 it_gun* gun = dynamic_cast<it_gun*>(type);
 int ret = gun->accuracy;
 for (int i = 0; i < contents.size(); i++) {
  if (contents[i].is_gunmod())
   ret -= (dynamic_cast<it_gunmod*>(contents[i].type))->accuracy;
 }
 ret += damage * 2;
 return ret;
}

int item::gun_damage(bool with_ammo)
{
 if (is_gunmod() && mode == IF_MODE_AUX)
  return curammo->damage;
 if (!is_gun())
  return 0;
 if(mode == IF_MODE_AUX) {
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

int item::noise()
{
 if (!is_gun())
  return 0;
 int ret = 0;
 if(mode == IF_MODE_AUX) {
  item* gunmod = active_gunmod();
  if (gunmod && gunmod->curammo)
   ret = gunmod->curammo->damage;
 } else if (curammo)
  ret = curammo->damage;
 ret *= .8;
 if (ret >= 5)
  ret += 20;
 if(mode == IF_MODE_AUX)
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
 if(mode == IF_MODE_AUX)
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
 if(mode == IF_MODE_AUX) {
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
 if(mode == IF_MODE_AUX) {
  item* gunmod = active_gunmod();
  if(gunmod && gunmod->curammo)
   return gunmod->curammo->range;
  else
   return 0;
 }

 int ret = (curammo?curammo->range:0);

 if (has_flag(IF_STR8_DRAW) && p) {
  if (p->str_cur < 4)
   return 0;
  else if (p->str_cur < 8)
   ret -= 2 * (8 - p->str_cur);
 } else if (has_flag(IF_STR10_DRAW) && p) {
  if (p->str_cur < 5)
   return 0;
  else if (p->str_cur < 10)
   ret -= 2 * (10 - p->str_cur);
 }

 return ret;
}


ammotype item::ammo_type()
{
 if (is_gun()) {
  it_gun* gun = dynamic_cast<it_gun*>(type);
  ammotype ret = gun->ammo;
  for (int i = 0; i < contents.size(); i++) {
   if (contents[i].is_gunmod() && !contents[i].has_flag(IF_MODE_AUX)) {
    it_gunmod* mod = dynamic_cast<it_gunmod*>(contents[i].type);
    if (mod->newtype != AT_NULL)
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
 return AT_NULL;
}

int item::pick_reload_ammo(player &u, bool interactive)
{
 if( is_null() )
  return false;

 if (!type->is_gun() && !type->is_tool()) {
  debugmsg("RELOADING NON-GUN NON-TOOL");
  return false;
 }
 int has_spare_mag = has_gunmod (itm_spare_mag);

 std::vector<int> am;	// List of indicies of valid ammo

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
   std::vector<int> tmpammo = u.has_ammo(ammo_type());
   for (int i = 0; i < tmpammo.size(); i++)
    if (charges <= 0 || u.inv[tmpammo[i]].typeId() == curammo->id)
      am.push_back(tmpammo[i]);
  }

  // ammo for gun attachments (shotgun attachments, grenade attachments, etc.)
  // for each attachment, find its associated ammo & append it to the ammo vector
  for (int i = 0; i < contents.size(); i++)
   if (contents[i].is_gunmod() && contents[i].has_flag(IF_MODE_AUX) &&
       contents[i].charges < (dynamic_cast<it_gunmod*>(contents[i].type))->clip) {
    std::vector<int> tmpammo = u.has_ammo((dynamic_cast<it_gunmod*>(contents[i].type))->newtype);
    for(int j = 0; j < tmpammo.size(); j++)
     if (contents[i].charges <= 0 ||
         u.inv[tmpammo[j]].typeId() == contents[i].curammo->id)
      am.push_back(tmpammo[j]);
   }
 } else { //non-gun.
  am = u.has_ammo(ammo_type());
 }

 int index = -1;

 if (am.size() > 1 && interactive) {// More than one option; list 'em and pick
   WINDOW* w_ammo = newwin(am.size() + 1, 80, 0, 0);
   char ch;
   clear();
   it_ammo* ammo_type;
   mvwprintw(w_ammo, 0, 0, "\
Choose ammo type:         Damage     Armor Pierce     Range     Accuracy");
   for (int i = 0; i < am.size(); i++) {
    ammo_type = dynamic_cast<it_ammo*>(u.inv[am[i]].type);
    mvwaddch(w_ammo, i + 1, 1, i + 'a');
    mvwprintw(w_ammo, i + 1, 3, "%s (%d)", u.inv[am[i]].tname().c_str(),
                                           u.inv[am[i]].charges);
    mvwprintw(w_ammo, i + 1, 27, "%d", ammo_type->damage);
    mvwprintw(w_ammo, i + 1, 38, "%d", ammo_type->pierce);
    mvwprintw(w_ammo, i + 1, 55, "%d", ammo_type->range);
    mvwprintw(w_ammo, i + 1, 65, "%d", 100 - ammo_type->accuracy);
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
    index = -1;
   else
    index = am[ch - 'a'];
 }
 // Either only one valid choice or chosing for a NPC, just return the first.
 else if (am.size() > 0){
  index = am[0];
 }
 return index;
}

bool item::reload(player &u, int index)
{
 bool single_load = false;
 int max_load = 1;
 item *reload_target = NULL;
 item *ammo_to_use = index != -2 ? &u.inv[index] : NULL;

 // Handle ammo in containers, currently only gasoline
 if(ammo_to_use && ammo_to_use->is_container())
   ammo_to_use = &ammo_to_use->contents[0];

 if (is_gun()) {
  // Reload using a spare magazine
  int spare_mag = has_gunmod(itm_spare_mag);
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
        contents[i].has_flag(IF_MODE_AUX) && contents[i].ammo_type() == ammo_to_use->ammo_type() &&
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
   if (reload_target->is_gunmod() && reload_target->typeId() == itm_spare_mag) {
    // Use gun numbers instead of the mod if it's a spare magazine
    max_load = (dynamic_cast<it_gun*>(type))->clip;
    single_load = has_flag(IF_RELOAD_ONE);
   } else {
    single_load = reload_target->has_flag(IF_RELOAD_ONE);
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

 if (index > -1) {
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
   if (u.inv[index].is_container())
     u.inv[index].contents.erase(u.inv[index].contents.begin());
   else
    u.i_remn(index);
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

bool is_flammable(material m)
{
 return (m == COTTON || m == WOOL || m == PAPER || m == WOOD || m == MNULL);
}

std::string default_technique_name(technique_id tech)
{
 switch (tech) {
  case TEC_SWEEP: return "Sweep attack";
  case TEC_PRECISE: return "Precision attack";
  case TEC_BRUTAL: return "Knock-back attack";
  case TEC_GRAB: return "Grab";
  case TEC_WIDE: return "Hit all adjacent monsters";
  case TEC_RAPID: return "Rapid attack";
  case TEC_FEINT: return "Feint";
  case TEC_THROW: return "Throw";
  case TEC_BLOCK: return "Block";
  case TEC_BLOCK_LEGS: return "Leg block";
  case TEC_WBLOCK_1: return "Weak block";
  case TEC_WBLOCK_2: return "Parry";
  case TEC_WBLOCK_3: return "Shield";
  case TEC_COUNTER: return "Counter-attack";
  case TEC_BREAK: return "Grab break";
  case TEC_DISARM: return "Disarm";
  case TEC_DEF_THROW: return "Defensive throw";
  case TEC_DEF_DISARM: return "Defense disarm";
  default: return "A BUG!";
 }
 return "A BUG!";
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


int item::typeId()
{
    if (!type)
        return itm_null;
    return type->id;
}
