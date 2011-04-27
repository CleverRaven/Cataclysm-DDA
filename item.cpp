#include "item.h"
#include "player.h"
#include "output.h"
#include "skill.h"
#include "game.h"
#include <sstream>
#include <curses.h>

bool is_flammable(material m);


item::item()
{
 name = "";
 charges = -1;
 bday = 0;
 invlet = 0;
 damage = 0;
 curammo = NULL;
 corpse = NULL;
 active = false;
 owned = false;
}

item::item(itype* it, unsigned int turn)
{
 type = it;
 bday = turn;
 name = "";
 invlet = 0;
 damage = 0;
 active = false;
 if (it->is_gun()) {
  charges = 0;
 } else if (it->is_ammo()) {
  it_ammo* ammo = dynamic_cast<it_ammo*>(it);
  charges = ammo->count;
 } else if (it->is_food()) {
  it_comest* comest = dynamic_cast<it_comest*>(it);
  if (comest->charges == 1)
   charges = -1;
  else
   charges = comest->charges;
 } else if (it->is_tool()) {
  it_tool* tool = dynamic_cast<it_tool*>(it);
  if (tool->max_charges == 0)
   charges = -1;
  else
   charges = tool->def_charges;
 } else {
  charges = -1;
 }
 curammo = NULL;
 corpse = NULL;
 owned = false;
}

item::item(itype *it, unsigned int turn, char let)
{
 type = it;
 bday = turn;
 name = "";
 damage = 0;
 active = false;
 if (it->is_gun()) {
  charges = 0;
 } else if (it->is_ammo()) {
  it_ammo* ammo = dynamic_cast<it_ammo*>(it);
  charges = ammo->count;
 } else if (it->is_food()) {
  it_comest* comest = dynamic_cast<it_comest*>(it);
  if (comest->charges == 1)
   charges = -1;
  else
   charges = comest->charges;
 } else if (it->is_tool()) {
  it_tool* tool = dynamic_cast<it_tool*>(it);
  if (tool->max_charges == 0)
   charges = -1;
  else
   charges = tool->def_charges;
 } else {
  charges = -1;
 }
 curammo = NULL;
 corpse = NULL;
 owned = false;
 invlet = let;
}

void item::make_corpse(itype* it, mtype* mt, unsigned int turn)
{
 name = "";
 charges = -1;
 invlet = 0;
 damage = 0;
 curammo = NULL;
 active = false;
 type = it;
 corpse = mt;
 bday = turn;
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
 type = it;
 contents.clear();
}

item item::in_its_container(std::vector<itype*> *itypes)
{
 if (!is_food() || (dynamic_cast<it_comest*>(type))->container == itm_null)
  return *this;
 it_comest *food = dynamic_cast<it_comest*>(type);
 item ret((*itypes)[food->container], bday);
 ret.contents.push_back(*this);
 ret.invlet = invlet;
 return ret;
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
 if (is_gun() && curammo != NULL)
  ammotmp = curammo->id;
 std::stringstream dump;// (std::stringstream::in | std::stringstream::out);
 dump << " " << int(invlet) << " " << int(type->id) << " " <<  int(charges) <<
         " " << int(damage) << " " << ammotmp << " " << int(bday);
 if (active)
  dump << " 1";
 else
  dump << " 0";
 if (owned)
  dump << " 1";
 else
  dump << " 0";
 if (corpse != NULL)
  dump << " " << corpse->id;
 else
  dump << " -1";
 dump << " '" << name << "'";
 return dump.str();
}

void item::load_info(std::string data, game *g)
{
 std::stringstream dump;
 dump << data;
 int idtmp, ammotmp, lettmp, damtmp, acttmp, owntmp, corp;
 dump >> lettmp >> idtmp >> charges >> damtmp >> ammotmp >> bday >> acttmp >>
         owntmp >> corp;
 if (corp != -1)
  corpse = g->mtypes[corp];
 else
  corpse = NULL;
 getline(dump, name);
 if (name == " ''")
  name = "";
 else
  name = name.substr(2, name.size() - 3); // s/^ '(.*)'$/\1/
 make(g->itypes[idtmp]);
 invlet = char(lettmp);
 damage = damtmp;
 active = false;
 if (acttmp == 1)
  active = true;
 owned = false;
 if (owntmp == 1)
  owned = true;
 if (is_gun() && ammotmp > 0)
  curammo = dynamic_cast<it_ammo*>(g->itypes[ammotmp]);
}
 
std::string item::info(bool showtext)
{
 std::stringstream dump;
 dump << " Volume: " << volume() << "    Weight: " << weight() << "\n" <<
         " Bash: " << int(type->melee_dam) << "  Cut: " <<
         int(type->melee_cut) << "  To-hit bonus: " <<
         (type->m_to_hit > 0 ? "+" : "" ) << int(type->m_to_hit) << "\n" <<
         " Moves per attack: " << attack_time() << "\n";

 if (is_food()) {

  it_comest* food = dynamic_cast<it_comest*>(type);
  dump << " Nutrituion: " << int(food->nutr) << "\n Quench: " <<
          int(food->quench) << "\n Enjoyability: " << int(food->fun);

 } else if (is_food_container()) {

  it_comest* food = dynamic_cast<it_comest*>(contents[0].type);
  dump << " Nutrituion: " << int(food->nutr) << "\n Quench: " <<
          int(food->quench) << "\n Enjoyability: " << int(food->fun);

 } else if (is_ammo()) {

  it_ammo* ammo = dynamic_cast<it_ammo*>(type);
  dump << " Type: " << ammo_name(ammo->type) << "\n Damage: " <<
           int(ammo->damage) << "\n Armor-pierce: " << int(ammo->pierce) <<
           "\n Range: " << int(ammo->range) << "\n Accuracy: " <<
           int(100 - ammo->accuracy) << "\n Recoil: " << int(ammo->recoil);

 } else if (is_gun()) {

  it_gun* gun = dynamic_cast<it_gun*>(type);
  int ammo_dam = 0, ammo_recoil = 0;
  bool has_ammo = (curammo != NULL && charges > 0);
  if (has_ammo) {
   ammo_dam = curammo->damage;
   ammo_recoil = curammo->recoil;
  }
   
  dump << " Skill used: " << skill_name(gun->skill_used) << "\n Ammunition: " <<
          clip_size() << " rounds of " << ammo_name(ammo());

  dump << "\n Damage: ";
  if (has_ammo)
   dump << ammo_dam;
  dump << (gun_damage(false) >= 0 ? "+" : "" ) << gun_damage(false);
  if (has_ammo)
   dump << " = " << gun_damage();

  dump << "\n Accuracy: " << int(100 - accuracy());

  dump << "\n Recoil: ";
  if (has_ammo)
   dump << ammo_recoil;
  dump << (recoil(false) >= 0 ? "+" : "" ) << recoil(false);
  if (has_ammo)
   dump << " = " << recoil();

  if (burst_size() == 0)
   dump << "\n Semi-automatic.";
  else
   dump << "\n Burst size: " << burst_size();
  if (contents.size() > 0)
   dump << "\n";
  for (int i = 0; i < contents.size(); i++)
   dump << "\n+" << contents[i].tname();

 } else if (is_gunmod()) {

  it_gunmod* mod = dynamic_cast<it_gunmod*>(type);
  if (mod->accuracy != 0)
   dump << " Accuracy: " << (mod->accuracy > 0 ? "+" : "") <<
           int(mod->accuracy);
  if (mod->damage != 0)
   dump << "\n Damage: " << (mod->damage > 0 ? "+" : "") << int(mod->damage);
  if (mod->clip != 0)
   dump << "\n Clip: " << (mod->clip > 0 ? "+" : "") << int(mod->damage) << "%";
  if (mod->recoil != 0)
   dump << "\n Recoil: " << int(mod->recoil);
  if (mod->burst != 0)
   dump << "\n Burst: " << (mod->clip > 0 ? "+" : "") << int(mod->clip);
  if (mod->newtype != AT_NULL)
   dump << "\n " << ammo_name(mod->newtype);
  dump << "\n Used on: ";
  if (mod->used_on_pistol)
   dump << "Pistols.  ";
  if (mod->used_on_shotgun)
   dump << "Shotguns.  ";
  if (mod->used_on_smg)
   dump << "SMGs.  ";
  if (mod->used_on_rifle)
   dump << "Rifles.";

 } else if (is_armor()) {

  it_armor* armor = dynamic_cast<it_armor*>(type);
  dump << " Covers: ";
  if (armor->covers & mfb(bp_head))
   dump << "The head. ";
  if (armor->covers & mfb(bp_eyes))
   dump << "The eyes. ";
  if (armor->covers & mfb(bp_mouth))
   dump << "The mouth. ";
  if (armor->covers & mfb(bp_torso))
   dump << "The torso. ";
  if (armor->covers & mfb(bp_hands))
   dump << "The hands. ";
  if (armor->covers & mfb(bp_legs))
   dump << "The legs. ";
  if (armor->covers & mfb(bp_feet))
   dump << "The feet. ";
  dump << "\n Encumberment: "			<< int(armor->encumber) <<
          "\n Bashing protection: "		<< int(armor->dmg_resist) <<
          "\n Cut protection: "			<< int(armor->cut_resist) <<
          "\n Environmental protection: "	<< int(armor->env_resist) <<
          "\n Warmth: "				<< int(armor->warmth) <<
          "\n Storage: "			<< int(armor->storage);

 } else if (is_book()) {

  it_book* book = dynamic_cast<it_book*>(type);
  if (book->type == sk_null)
   dump << " Just for fun.\n";
  else {
   dump << " Can bring your " << skill_name(book->type) << " skill to " <<
           int(book->level) << std::endl;
   if (book->req == 0)
    dump << " It can be understood by beginners.\n";
   else
    dump << " Requires " << skill_name(book->type) << " level " <<
            int(book->req) << " to understand.\n";
  }
  dump << " Requires intelligence of " << int(book->intel) << std::endl;
  if (book->fun != 0)
   dump << " Reading this book affects your morale by " <<
           (book->fun > 0 ? "+" : "") << int(book->fun) << std::endl;
  dump << " This book takes " << int(book->time) << " minutes to read.";

 } else if (is_tool()) {

  it_tool* tool = dynamic_cast<it_tool*>(type);
  dump << " Maximum " << tool->max_charges << " charges";
  if (tool->ammo == AT_NULL)
   dump << ".";
  else
   dump << " of " << ammo_name(tool->ammo) << ".";
 }

 if (showtext) {
  dump << "\n\n " << type->description << "\n";
  if (contents.size() > 0) {
   if (is_gun()) {
    for (int i = 0; i < contents.size(); i++)
     dump << "\n " << contents[i].type->description;
   } else
    dump << "\n " << contents[0].type->description;
   dump << "\n";
  }
 }
 return dump.str();
}

char item::symbol()
{
 return type->sym;
}

nc_color item::color(player *u)
{
 nc_color ret = c_ltgray;

 if (is_gun()) { // Guns are green if you are carrying ammo for them
  ammotype amtype = ammo();
  if (u->has_ammo(amtype).size() > 0)
   ret = c_green;
 } else if (is_ammo()) { // Likewise, ammo is green if you have guns that use it
  ammotype amtype = ammo();
  if (u->weapon.is_gun() && u->weapon.ammo() == amtype)
   ret = c_green;
  else {
   for (int i = 0; i < u->inv.size(); i++) {
    if (u->inv[i].is_gun() && u->inv[i].ammo() == amtype) {
     i = u->inv.size();
     ret = c_green;
    }
   }
  }
 } else if (is_book()) {
  it_book* tmp = dynamic_cast<it_book*>(type);
  if (tmp->type !=sk_null && tmp->intel <= u->int_cur + u->sklevel[tmp->type] &&
      (tmp->intel == 0 || !u->has_trait(PF_ILLITERATE)) &&
      tmp->req <= u->sklevel[tmp->type] && tmp->level > u->sklevel[tmp->type])
   ret = c_ltblue;
 }
 return ret;
}

std::string item::tname(game *g)
{
 std::stringstream ret;

 if (damage != 0) {
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
    damtext = "damaged ";
  }
  ret << damtext;
 }

 if (type->id == itm_corpse) {
  ret << corpse->name << " corpse";
  if (name != "")
   ret << " of " << name;
  return ret.str();
 }

 if (is_gun() && contents.size() > 0) {
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
     g->turn - bday > food->spoils * 600)
  ret << " (rotten)";


 if (owned)
  ret << " (owned)";
 return ret.str();
}

nc_color item::color()
{
 if (type->id == itm_corpse)
  return corpse->color;
 return type->color;
}

int item::price()
{
 int ret = type->price;
 for (int i = 0; i < contents.size(); i++)
  ret += contents[i].price();
 return ret;
}

int item::weight()
{
 if (type->id == itm_corpse) {
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
 int ret = type->weight;
 if (is_ammo()) { 
  ret *= charges;
  ret /= 100;
 }
 for (int i = 0; i < contents.size(); i++)
  ret += contents[i].weight();
 return ret;
}

int item::volume()
{
 if (type->id == itm_corpse) {
  switch (corpse->size) {
   case MS_TINY:   return   2;
   case MS_SMALL:  return  40;
   case MS_MEDIUM: return  75;
   case MS_LARGE:  return 160;
   case MS_HUGE:   return 600;
  }
 }
 int ret = type->volume;
 if (is_gun()) {
  for (int i = 0; i < contents.size(); i++)
   ret += contents[i].volume();
 }
 return type->volume;
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
 return 80 + 4 * volume() + 2 * weight();
}

int item::weapon_value(int skills[num_skill_types])
{
 int my_value = 0;
 if (is_gun()) {
  int gun_value = 14;
  it_gun* gun = dynamic_cast<it_gun*>(type);
  gun_value += gun->dmg_bonus;
  gun_value += int(gun->burst / 2);
  gun_value += int(gun->clip / 3);
  gun_value -= int(gun->accuracy / 5);
  gun_value *= (.5 + (.3 * skills[sk_gun]));
  gun_value *= (.3 + (.7 * skills[gun->skill_used]));
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
 int my_value = 0;
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
 
bool item::is_two_handed(player *u)
{
 if (is_gun() && (dynamic_cast<it_gun*>(type))->skill_used != sk_pistol)
  return true;
 return (volume() > 10 || weight() > u->str_cur * 4);
}

bool item::made_of(material mat)
{
 if (type->id == itm_corpse)
  return (corpse->mat == mat);
 return (type->m1 == mat || type->m2 == mat);
}

bool item::conductive()
{
 if ((type->m1 == IRON || type->m1 == STEEL || type->m1 == SILVER) &&
     (type->m2 == IRON || type->m2 == STEEL || type->m2 == SILVER)   )
  return true;
 return false;
}

bool item::is_gun()
{
 return type->is_gun();
}

bool item::is_gunmod()
{
 return type->is_gunmod();
}

bool item::is_bionic()
{
 return type->is_bionic();
}

bool item::is_ammo()
{
 return type->is_ammo();
}

bool item::is_food(player *u)
{
 if (type->is_food())
  return true;
 if (u->has_bionic(bio_batteries) && is_ammo() &&
     (dynamic_cast<it_ammo*>(type))->type == AT_BATT)
  return true;
 if (u->has_bionic(bio_furnace) && is_flammable(type->m1) &&
     is_flammable(type->m2))
  return true;
 return false;
}

bool item::is_food_container(player *u)
{
 return (contents.size() >= 1 && contents[0].is_food(u));
}

bool item::is_food()
{
 if (type->is_food())
  return true;
 return false;
}

bool item::is_food_container()
{
 return (contents.size() >= 1 && contents[0].is_food());
}

bool item::is_drink()
{
 return type->is_food() && type->m1 == LIQUID;
}

bool item::is_weap()
{
 if (is_gun() || is_food() || is_ammo() || is_food_container() || is_armor() ||
     is_book() || is_tool())
  return false;
 return (type->melee_dam > 7 || type->melee_cut > 5);
}

bool item::is_bashing_weapon()
{
 return (type->melee_dam > 7);
}

bool item::is_cutting_weapon()
{
 return (type->melee_cut > 5);
}

bool item::is_armor()
{
 return type->is_armor();
}

bool item::is_book()
{
 return type->is_book();
}

bool item::is_container()
{
 return type->is_container();
}

bool item::is_tool()
{
 return type->is_tool();
}

int item::reload_time(player &u)
{
 int ret = 0;
 if (is_gun()) {
  it_gun* reloading = dynamic_cast<it_gun*>(type);
  switch (reloading->skill_used) {
  case (sk_pistol):
   if (u.sklevel[sk_pistol] > 8)
    ret = 30;
   else
    ret = (350 - 40 * u.sklevel[sk_pistol]);
  break;
  case (sk_shotgun):
   if (u.sklevel[sk_shotgun] > 10)
    ret = 20;
   else
    ret = (120 - 10 * u.sklevel[sk_shotgun]);
  break;
  case (sk_smg):
   if (u.sklevel[sk_smg] > 8)
    ret = 50;
   else
    ret = (450 - 50 * u.sklevel[sk_smg]);
   break;
  case (sk_rifle):
   if (u.sklevel[sk_rifle] > 8)
    ret = 60;
   else
    ret = (500 - 55 * u.sklevel[sk_rifle]);
  break;
  default:
   debugmsg("Why is reloading %s using %s skill?", (reloading->name).c_str(),
 		skill_name(reloading->skill_used).c_str());
  }
  if (ret <= 0)
   debugmsg("Reloading %s takes %d moves!", (reloading->name).c_str(), ret);
 } else if (is_tool())
  ret = 100 + volume() + weight();
 if (type->id == itm_crossbow)	// Crossbows are special, they take longer
  ret += 150 - u.str_cur * 10;
 ret += u.encumb(bp_hands) * 30;
 return ret;
}

int item::clip_size()
{
 if (!is_gun())
  return 0;
 it_gun* gun = dynamic_cast<it_gun*>(type);
 int ret = gun->clip;
 for (int i = 0; i < contents.size(); i++) {
  if (contents[i].is_gunmod()) {
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
 if (!is_gun())
  return 0;
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
 if (curammo != NULL)
  ret = curammo->damage * .8;
 if (ret >= 5)
  ret += 20;
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
 it_gun* gun = dynamic_cast<it_gun*>(type);
 int ret = gun->recoil;
 if (with_ammo && curammo != NULL)
  ret += curammo->recoil;
 for (int i = 0; i < contents.size(); i++) {
  if (contents[i].is_gunmod())
   ret += (dynamic_cast<it_gunmod*>(contents[i].type))->recoil;
 }
 return ret;
}

ammotype item::ammo()
{
 if (is_gun()) {
  it_gun* gun = dynamic_cast<it_gun*>(type);
  ammotype ret = gun->ammo;
  for (int i = 0; i < contents.size(); i++) {
   if (contents[i].is_gunmod()) {
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
 }
 return AT_NULL;
}
 
int item::pick_reload_ammo(player &u, bool interactive)
{
 if (!type->is_gun() && !type->is_tool()) {
  debugmsg("RELOADING NON-GUN NON-TOOL");
  return false;
 }
 bool single_load = false;
 int max_load = 0;
 std::vector<int> am;	// List of indicies of valid ammo

 if (type->is_gun()) {
  if (charges > 0) {
   itype_id aid = itype_id(curammo->id);
   for (int i = 0; i < u.inv.size(); i++) {
    if (u.inv[i].type->id == aid)
     am.push_back(i);
   }
  } else {
  it_gun* tmp = dynamic_cast<it_gun*>(type);
   am = u.has_ammo(ammo());
   max_load = tmp->clip;
   if (tmp->skill_used == sk_shotgun)
    single_load = true;
  }
 } else {
  it_tool* tmp = dynamic_cast<it_tool*>(type);
  am = u.has_ammo(ammo());
  max_load = tmp->max_charges;
 }

 int index = -1;

 if (am.size() > 1 && interactive) {// More than one option; list 'em and pick
  WINDOW* w_ammo = newwin(am.size() + 1, 80, 0, 0);
  if (charges == 0) {
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
  } else {
   int smallest = 500;
   for (int i = 0; i < am.size(); i++) {
    //if (u.inv[am[i]].type->id == curammo->id &&
        if (u.inv[am[i]].charges < smallest) {
     smallest = u.inv[am[i]].charges;
     index = am[i];
    }
   }
  }
 } else if (am.size() == 1 || !interactive)
  index = am[0];
 return index;
}

bool item::reload(player &u, int index)
{
 bool single_load;
 int max_load;
 if (is_gun()) {
  it_gun* reloading = dynamic_cast<it_gun*>(type);
  single_load = (reloading->skill_used == sk_shotgun);
  max_load = clip_size();
 } else if (is_tool()) {
  it_tool* tool = dynamic_cast<it_tool*>(type);
  single_load = false;
  max_load = tool->max_charges;
 }
 if (index > -1) {
// If the gun is currently loaded with a different type of ammo, reloading fails
  if (is_gun() && charges > 0 && curammo->id != u.inv[index].type->id)
   return false;
  if (is_gun())
   curammo = dynamic_cast<it_ammo*>((u.inv[index].type));
  if (single_load) {	// Only insert one cartridge!
   charges++;
   u.inv[index].charges--;
  } else {
   charges += u.inv[index].charges;
   u.inv[index].charges = 0;
   if (charges > max_load) {
 // More bullets than the clip holds, put some back
    u.inv[index].charges += charges - max_load;
    charges = max_load;
   }
  }
  if (u.inv[index].charges == 0) {
   u.i_remn(index);
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

/*
bool item::stack_with(item &it)
{
 if (type->id != it.type->id || damage != it.damage ||
     charges != it.charges || owned != it.owned || active != it.active ||
     ((is_food() || is_food_container()) && bday != it.bday))
  return false;

 if (contents.size() > 0) {
  if (contents.size() != it.contents.size())
   return false;
  std::vector<int> tmp_contents; // Duplicate it to check if it matches
  for (int i = 0; i < it.contents.size(); i++)
   tmp_contents.push_back(it.contents[i].type->id);
  for (int i = 0; i < contents.size(); i++) {
   for (int j = 0; j < tmp_contents.size(); j++) {
    if (tmp_contents[j] == contents[i].type->id) {
    tmp_contents.erase(tmp_contents.begin() + j);
     j = tmp_contents.size();
    }
   }
  }
// At this point, tmp_contents should be empty if it's a perfect match
  if (!tmp_contents.empty())
   return false;
 }

 count++;
 return true;
}
*/

bool is_flammable(material m)
{
 if (m == VEGGY   || m == FLESH || m == POWDER || m == COTTON || m == WOOL ||
     m == LEATHER || m == PAPER || m == WOOD   || m == MNULL)
  return true;
 return false;
}
