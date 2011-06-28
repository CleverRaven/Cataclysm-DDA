#include "player.h"
#include "bionics.h"
#include "mission.h"
#include "game.h"
#include "disease.h"
#include "addiction.h"
#include "keypress.h"
#include "moraledata.h"
#include <sstream>
#include <curses.h>
#include <stdlib.h>

nc_color encumb_color(int level);

player::player()
{
 str_cur = 8;
 str_max = 8;
 dex_cur = 8;
 dex_max = 8;
 int_cur = 8;
 int_max = 8;
 per_cur = 8;
 per_max = 8;
 underwater = false;
 can_dodge = true;
 power_level = 0;
 max_power_level = 0;
 hunger = 0;
 thirst = 0;
 fatigue = 0;
 stim = 0;
 pain = 0;
 pkill = 0;
 radiation = 0;
 cash = 8000;
 recoil = 0;
 scent = 500;
 name = "";
 male = true;
 inv_sorted = true;
 moves = 100;
 oxygen = 0;
 active_mission = -1;
 xp_pool = 0;
 for (int i = 0; i < num_skill_types; i++) {
  sklevel[i] = 0;
  skexercise[i] = 0;
 }
 for (int i = 0; i < PF_MAX2; i++)
  my_traits[i] = false;
}

player::~player()
{
}

void player::normalize(game *g)
{
 ret_null = item(g->itypes[0], 0);
 weapon   = item(g->itypes[0], 0);
// Nice to start out less than naked.
 worn.push_back(item(g->itypes[itm_jeans],    0, 'a'));
 worn.push_back(item(g->itypes[itm_tshirt],   0, 'b'));
 worn.push_back(item(g->itypes[itm_sneakers], 0, 'c'));
 worn.push_back(item(g->itypes[itm_holster],  0, 'd'));
 for (int i = 0; i < num_hp_parts; i++) {
  hp_max[i] = 60 + str_max * 3;
  if (has_trait(PF_TOUGH))
   hp_max[i] = int(hp_max[i] * 1.2);
  hp_cur[i] = hp_max[i];
 }
}
 
void player::reset()
{
// Reset our stats to normal levels
// Any persistent buffs/debuffs will take place in disease.h,
// player::suffer(), etc.
 str_cur = str_max;
 dex_cur = dex_max;
 int_cur = int_max;
 per_cur = per_max;
// We can dodge again!
 can_dodge = true;
// Bionic buffs
 if (has_active_bionic(bio_hydraulics))
  str_cur += 20;
 if (has_bionic(bio_eye_enhancer))
  per_cur += 2;
 if (has_bionic(bio_carbon))
  dex_cur -= 2;
 if (has_bionic(bio_armor_head))
  per_cur--;
 if (has_bionic(bio_armor_arms))
  dex_cur -= 2;
 if (has_bionic(bio_armor_torso))
  dex_cur--;
 if (has_bionic(bio_metabolics) && power_level < max_power_level &&
     hunger < 100) {
  hunger += 2;
  power_level++;
 }

// Trait / mutation buffs
 if (has_trait(PF_CHITIN))
  dex_cur -= 3;
// Pain
 if (pain > pkill) {
  str_cur  -=     int((pain - pkill) / 15);
  dex_cur  -=     int((pain - pkill) / 15);
  per_cur  -=     int((pain - pkill) / 20);
  int_cur  -= 1 + int((pain - pkill) / 25);
 }
// Morale
 if (abs(morale_level()) >= 100) {
  str_cur  += int(morale_level() / 180);
  dex_cur  += int(morale_level() / 200);
  per_cur  += int(morale_level() / 125);
  int_cur  += int(morale_level() / 100);
 }
// Radiation
 if (radiation > 0) {
  str_cur  -= int(radiation / 24);
  dex_cur  -= int(radiation / 36);
  per_cur  -= int(radiation / 40);
  int_cur  -= int(radiation / 48);
 }
// Stimulants
 dex_cur += int(stim / 10);
 per_cur += int(stim /  7);
 int_cur += int(stim /  6);
 if (stim >= 30) { 
  dex_cur -= int(abs(stim - 15) /  8);
  per_cur -= int(abs(stim - 15) / 12);
  int_cur -= int(abs(stim - 15) / 14);
 }

// Give us our movement points for the turn.
 moves += current_speed();

// Floor for our stats.  No stat changes should occur after this!
 if (dex_cur < 0)
  dex_cur = 0;
 if (str_cur < 0)
  str_cur = 0;
 if (per_cur < 0)
  per_cur = 0;
 if (int_cur < 0)
  int_cur = 0;
 
 int mor = morale_level();
 if      (mor >= 300)
  xp_pool += 4;
 else if (mor >= 200)
  xp_pool += 3;
 else if (mor >= 100)
  xp_pool += 2;
 else if (mor >=  25)
  xp_pool += 1;
 else if (mor >=   0)
  xp_pool += rng(0, 1) * rng(0, 1) * rng(0, 1);
}

void player::update_morale()
{
 for (int i = 0; i < morale.size(); i++) {
  if (morale[i].bonus < 0)
   morale[i].bonus++;
  else if (morale[i].bonus > 0)
   morale[i].bonus--;

  if (morale[i].bonus == 0) {
   morale.erase(morale.begin() + i);
   i--;
  }
 }
}

int player::current_speed()
{
 int newmoves = 100; // Start with 100 movement points...
// Minus some for weight...
 if (weight_carried() > weight_capacity())
  newmoves = 1;
 else if (weight_carried() > weight_capacity() * .25)
  newmoves = int((120 * (weight_capacity() - weight_carried())) /
                         weight_capacity());

 if (pain > pkill)
  newmoves -= int((pain - pkill) * .7);
 if (pkill >= 10)
  newmoves -= int(pkill * .1);

 if (abs(morale_level()) >= 100) {
  int morale_bonus = int(morale_level() / 25);
  if (morale_bonus < -10)
   morale_bonus = -10;
  else if (morale_bonus > 10)
   morale_bonus = 10;
  newmoves += morale_bonus;
 }

 if (radiation > 10) {
  int rad_penalty = radiation / 10;
  if (rad_penalty > 20)
   rad_penalty = 20;
  newmoves -= rad_penalty;
 }

 if (thirst > 40)
  newmoves -= int((thirst - 40) / 10);
 if (hunger > 100)
  newmoves -= int((hunger - 100) / 10);

 newmoves += (stim > 40 ? 40 : stim);

 if (has_trait(PF_QUICK))
  newmoves = int(newmoves * 1.10);

 if (newmoves < 1)
  newmoves = 1;

 return newmoves;
}

int player::swim_speed()
{
 int ret = 440 + 2 * weight_carried() - 50 * sklevel[sk_swimming];
 if (has_trait(PF_WEBBED))
  ret -= 100 + str_cur * 10;
 ret += (50 - sklevel[sk_swimming] * 2) * abs(encumb(bp_legs));
 ret += (80 - sklevel[sk_swimming] * 3) * abs(encumb(bp_torso));
 if (sklevel[sk_swimming] < 10) {
  for (int i = 0; i < worn.size(); i++)
   ret += (worn[i].volume() * (10 - sklevel[sk_swimming])) / 2;
 }
 ret -= str_cur * 6 + dex_cur * 4;
// If (ret > 500), we can not swim; so do not apply the underwater bonus.
 if (underwater && ret < 500)
  ret -= 50;
 if (ret < 30)
  ret = 30;
 return ret;
}

nc_color player::color()
{
 if (has_disease(DI_ONFIRE))
  return c_red;
 if (has_disease(DI_BOOMERED))
  return c_pink;
 if (underwater)
  return c_blue;
 if (has_active_bionic(bio_cloak))
  return c_dkgray;
 return c_white;
}

void player::load_info(game *g, std::string data)
{
 std::stringstream dump;
 dump << data;
 dump >> posx >> posy >> str_cur >> str_max >> dex_cur >> dex_max >>
         int_cur >> int_max >> per_cur >> per_max >> power_level >>
         max_power_level >> hunger >> thirst >> fatigue >> stim >>
         pain >> pkill >> radiation >> cash >> recoil >> scent >> moves >>
         underwater >> can_dodge >> oxygen >> active_mission >> xp_pool;

 for (int i = 0; i < PF_MAX2; i++)
  dump >> my_traits[i];

 for (int i = 0; i < num_hp_parts; i++)
  dump >> hp_cur[i] >> hp_max[i];
 for (int i = 0; i < num_skill_types; i++)
  dump >> sklevel[i] >> skexercise[i];

 int numill;
 int typetmp;
 disease illtmp;
 dump >> numill;
 for (int i = 0; i < numill; i++) {
  dump >> typetmp >> illtmp.duration;
  illtmp.type = dis_type(typetmp);
  illness.push_back(illtmp);
 }

 int numadd;
 addiction addtmp;
 dump >> numadd;
 for (int i = 0; i < numadd; i++) {
  dump >> typetmp >> addtmp.intensity >> addtmp.sated;
  addtmp.type = add_type(typetmp);
  addictions.push_back(addtmp);
 }

 int numbio;
 bionic biotmp;
 dump >> numbio;
 for (int i = 0; i < numbio; i++) {
  dump >> typetmp >> biotmp.invlet >> biotmp.powered >> biotmp.charge;
  biotmp.id = bionic_id(typetmp);
  my_bionics.push_back(biotmp);
 }

 int nummor;
 morale_point mortmp;
 dump >> nummor;
 for (int i = 0; i < nummor; i++) {
  int mortype;
  int item_id;
  dump >> mortmp.bonus >> mortype >> item_id;
  mortmp.type = morale_type(mortype);
  if (item_id == 0)
   mortmp.item_type = NULL;
  else
   mortmp.item_type = g->itypes[item_id];
  morale.push_back(mortmp);
 }
}

std::string player::save_info()
{
 std::stringstream dump;
 dump << posx    << " " << posy    << " " << str_cur << " " << str_max << " " <<
         dex_cur << " " << dex_max << " " << int_cur << " " << int_max << " " <<
         per_cur << " " << per_max << " " << power_level << " " <<
         max_power_level << " " << hunger << " " << thirst << " " << fatigue <<
         " " << stim << " " << pain << " " << pkill << " " << radiation <<
         " " << cash << " " << recoil << " " << scent << " " << moves << " " <<
         underwater << " " << can_dodge << " " << oxygen << " " <<
         active_mission << " " << xp_pool << " ";

 for (int i = 0; i < PF_MAX2; i++)
  dump << my_traits[i] << " ";
 for (int i = 0; i < num_hp_parts; i++)
  dump << hp_cur[i] << " " << hp_max[i] << " ";
 for (int i = 0; i < num_skill_types; i++)
  dump << int(sklevel[i]) << " " << skexercise[i] << " ";

 dump << illness.size() << " ";
 for (int i = 0; i < illness.size();  i++)
  dump << int(illness[i].type) << " " << illness[i].duration << " ";

 dump << addictions.size() << " ";
 for (int i = 0; i < addictions.size(); i++)
  dump << int(addictions[i].type) << " " << addictions[i].intensity << " " <<
          addictions[i].sated << " ";

 dump << my_bionics.size() << " ";
 for (int i = 0; i < my_bionics.size(); i++)
  dump << int(my_bionics[i].id) << " " << my_bionics[i].invlet << " " <<
          my_bionics[i].powered << " " << my_bionics[i].charge << " ";

 dump << morale.size() << " ";
 for (int i = 0; i < morale.size(); i++) {
  dump << morale[i].bonus << " " << morale[i].type << " ";
  if (morale[i].item_type == NULL)
   dump << "0";
  else
   dump << morale[i].item_type->id;
  dump << " ";
 }
 dump << std::endl;

 for (int i = 0; i < inv.size(); i++) {
  dump << "I " << inv[i].save_info() << std::endl;
  for (int j = 0; j < inv[i].contents.size(); j++)
   dump << "C " << inv[i].contents[j].save_info() << std::endl;
 }
 for (int i = 0; i < worn.size(); i++)
  dump << "W " << worn[i].save_info() << std::endl;
 dump << "w " << weapon.save_info() << std::endl;
 for (int j = 0; j < weapon.contents.size(); j++)
  dump << "c " << weapon.contents[j].save_info() << std::endl;

 return dump.str();
}

void player::disp_info(game *g)
{
 int line;
 std::vector<std::string> effect_name;
 std::vector<std::string> effect_text;
 for (int i = 0; i < illness.size(); i++) {
  if (dis_name(illness[i]).size() > 0) {
   effect_name.push_back(dis_name(illness[i]));
   effect_text.push_back(dis_description(illness[i]));
  }
 }
 if (abs(morale_level()) >= 100) {
  bool pos = (morale_level() > 0);
  effect_name.push_back(pos ? "Elated" : "Depressed");
  std::stringstream morale_text;
  if (abs(morale_level()) >= 200)
   morale_text << "Dexterity" << (pos ? " +" : " ") <<
                   int(morale_level() / 200) << "   ";
  if (abs(morale_level()) >= 180)
   morale_text << "Strength" << (pos ? " +" : " ") <<
                  int(morale_level() / 180) << "   ";
  if (abs(morale_level()) >= 125)
   morale_text << "Perception" << (pos ? " +" : " ") <<
                  int(morale_level() / 125) << "   ";
  morale_text << "Intelligence" << (pos ? " +" : " ") <<
                 int(morale_level() / 100) << "   ";
  effect_text.push_back(morale_text.str());
 }
 if (pain - pkill > 0) {
  effect_name.push_back("Pain");
  std::stringstream pain_text;
  if (pain - pkill >= 15)
   pain_text << "Strength -" << int((pain - pkill) / 15) << "   Dexterity -" <<
                int((pain - pkill) / 15) << "   ";
  if (pain - pkill >= 20)
   pain_text << "Perception -" << int((pain - pkill) / 15) << "   ";
  pain_text << "Intelligence -" << 1 + int((pain - pkill) / 25);
  effect_text.push_back(pain_text.str());
 }
 if (stim > 0) {
  int dexbonus = int(stim / 10);
  int perbonus = int(stim /  7);
  int intbonus = int(stim /  6);
  if (abs(stim) >= 30) { 
   dexbonus -= int(abs(stim - 15) /  8);
   perbonus -= int(abs(stim - 15) / 12);
   intbonus -= int(abs(stim - 15) / 14);
  }
  
  if (dexbonus < 0)
   effect_name.push_back("Stimulant Overdose");
  else
   effect_name.push_back("Stimulant");
  std::stringstream stim_text;
  stim_text << "Speed +" << stim << "   Intelligence " <<
               (intbonus > 0 ? "+ " : "") << intbonus << "   Perception " <<
               (perbonus > 0 ? "+ " : "") << perbonus << "   Dexterity "  <<
               (dexbonus > 0 ? "+ " : "") << dexbonus;
  effect_text.push_back(stim_text.str());
 } else if (stim < 0) {
  effect_name.push_back("Depressants");
  std::stringstream stim_text;
  int dexpen = int(stim / 10);
  int perpen = int(stim /  7);
  int intpen = int(stim /  6);
// Since dexpen etc. are always less than 0, no need for + signs
  stim_text << "Speed " << stim << "   Intelligence " << intpen <<
               "   Perception " << perpen << "   Dexterity " << dexpen;
  effect_text.push_back(stim_text.str());
 }

 if (has_trait(PF_TROGLO) && g->is_in_sunlight(posx, posy)) {
  effect_name.push_back("In Sunlight");
  effect_text.push_back("The sunlight irritates you terribly.\n\
Strength - 4;    Dexterity - 4;    Intelligence - 4;    Dexterity - 4");
 }

 for (int i = 0; i < addictions.size(); i++) {
  if (addictions[i].sated < 0) {
   effect_name.push_back(addiction_name(addictions[i]));
   effect_text.push_back(addiction_text(addictions[i]));
  }
 }

 WINDOW* w_grid    = newwin(25, 80,  0,  0);
 WINDOW* w_stats   = newwin( 9, 26,  2,  0);
 WINDOW* w_encumb  = newwin( 9, 26, 12,  0);
 WINDOW* w_traits  = newwin( 9, 26,  2, 27);
 WINDOW* w_effects = newwin( 9, 26, 12, 27);
 WINDOW* w_skills  = newwin( 9, 26,  2, 54);
 WINDOW* w_speed   = newwin( 9, 26, 12, 54);
 WINDOW* w_info    = newwin( 3, 80, 22,  0);
// Print name and header
 mvwprintw(w_grid, 0, 0, "%s - %s", name.c_str(), (male ? "Male" : "Female"));
 mvwprintz(w_grid, 0, 39, c_ltred, "| Press TAB to cycle, ESC or q to return.");
// Main line grid
 for (int i = 0; i < 80; i++) {
  mvwputch(w_grid,  1, i, c_ltgray, LINE_OXOX);
  mvwputch(w_grid, 21, i, c_ltgray, LINE_OXOX);
  mvwputch(w_grid, 11, i, c_ltgray, LINE_OXOX);
  if (i > 1 && i < 21) {
   mvwputch(w_grid, i, 26, c_ltgray, LINE_XOXO);
   mvwputch(w_grid, i, 53, c_ltgray, LINE_XOXO);
  }
 }
 mvwputch(w_grid,  1, 26, c_ltgray, LINE_OXXX);
 mvwputch(w_grid,  1, 53, c_ltgray, LINE_OXXX);
 mvwputch(w_grid, 21, 26, c_ltgray, LINE_XXOX);
 mvwputch(w_grid, 21, 53, c_ltgray, LINE_XXOX);
 mvwputch(w_grid, 11, 26, c_ltgray, LINE_XXXX);
 mvwputch(w_grid, 11, 53, c_ltgray, LINE_XXXX);
 wrefresh(w_grid);	// w_grid should stay static.

// First!  Default STATS screen.
 mvwprintz(w_stats, 0, 10, c_ltgray, "STATS");
 mvwprintz(w_stats, 2,  2, c_ltgray, "Strength:%s(%d)",
           (str_max < 10 ? "         " : "        "), str_max);
 mvwprintz(w_stats, 3,  2, c_ltgray, "Dexterity:%s(%d)",
           (dex_max < 10 ? "        "  : "       "),  dex_max);
 mvwprintz(w_stats, 4,  2, c_ltgray, "Intelligence:%s(%d)",
           (int_max < 10 ? "     "     : "    "),     int_max);
 mvwprintz(w_stats, 5,  2, c_ltgray, "Perception:%s(%d)",
           (per_max < 10 ? "       "   : "      "),   per_max);

 nc_color status = c_white;

 if (str_cur <= 0)
  status = c_dkgray;
 else if (str_cur < str_max / 2)
  status = c_red;
 else if (str_cur < str_max)
  status = c_ltred;
 else if (str_cur == str_max)
  status = c_white;
 else if (str_cur < str_max * 1.5)
  status = c_ltgreen;
 else
  status = c_green;
 mvwprintz(w_stats,  2, (str_cur < 10 ? 17 : 16), status, "%d", str_cur);

 if (dex_cur <= 0)
  status = c_dkgray;
 else if (dex_cur < dex_max / 2)
  status = c_red;
 else if (dex_cur < dex_max)
  status = c_ltred;
 else if (dex_cur == dex_max)
  status = c_white;
 else if (dex_cur < dex_max * 1.5)
  status = c_ltgreen;
 else
  status = c_green;
 mvwprintz(w_stats,  3, (dex_cur < 10 ? 17 : 16), status, "%d", dex_cur);

 if (int_cur <= 0)
  status = c_dkgray;
 else if (int_cur < int_max / 2)
  status = c_red;
 else if (int_cur < int_max)
  status = c_ltred;
 else if (int_cur == int_max)
  status = c_white;
 else if (int_cur < int_max * 1.5)
  status = c_ltgreen;
 else
  status = c_green;
 mvwprintz(w_stats,  4, (int_cur < 10 ? 17 : 16), status, "%d", int_cur);

 if (per_cur <= 0)
  status = c_dkgray;
 else if (per_cur < per_max / 2)
  status = c_red;
 else if (per_cur < per_max)
  status = c_ltred;
 else if (per_cur == per_max)
  status = c_white;
 else if (per_cur < per_max * 1.5)
  status = c_ltgreen;
 else
  status = c_green;
 mvwprintz(w_stats,  5, (per_cur < 10 ? 17 : 16), status, "%d", per_cur);

 wrefresh(w_stats);

// Next, draw encumberment.
 mvwprintz(w_encumb, 0, 6, c_ltgray, "ENCUMBERANCE");
 mvwprintz(w_encumb, 2, 2, c_ltgray, "Head................");
 mvwprintz(w_encumb, 2,(encumb(bp_head) >= 0 && encumb(bp_head) < 10 ? 21 : 20),
           encumb_color(encumb(bp_head)), "%d", encumb(bp_head));
 mvwprintz(w_encumb, 3, 2, c_ltgray, "Eyes................");
 mvwprintz(w_encumb, 3,(encumb(bp_eyes) >= 0 && encumb(bp_eyes) < 10 ? 21 : 20),
           encumb_color(encumb(bp_eyes)), "%d", encumb(bp_eyes));
 mvwprintz(w_encumb, 4, 2, c_ltgray, "Mouth...............");
 mvwprintz(w_encumb, 4,(encumb(bp_mouth)>=0 && encumb(bp_mouth) < 10 ? 21 : 20),
           encumb_color(encumb(bp_mouth)), "%d", encumb(bp_mouth));
 mvwprintz(w_encumb, 5, 2, c_ltgray, "Torso...............");
 mvwprintz(w_encumb, 5,(encumb(bp_torso)>=0 && encumb(bp_torso) < 10 ? 21 : 20),
           encumb_color(encumb(bp_torso)), "%d", encumb(bp_torso));
 mvwprintz(w_encumb, 6, 2, c_ltgray, "Hands...............");
 mvwprintz(w_encumb, 6,(encumb(bp_hands)>=0 && encumb(bp_hands) < 10 ? 21 : 20),
           encumb_color(encumb(bp_hands)), "%d", encumb(bp_hands));
 mvwprintz(w_encumb, 7, 2, c_ltgray, "Legs................");
 mvwprintz(w_encumb, 7,(encumb(bp_legs) >= 0 && encumb(bp_legs) < 10 ? 21 : 20),
           encumb_color(encumb(bp_legs)), "%d", encumb(bp_legs));
 mvwprintz(w_encumb, 8, 2, c_ltgray, "Feet................");
 mvwprintz(w_encumb, 8,(encumb(bp_feet) >= 0 && encumb(bp_feet) < 10 ? 21 : 20),
           encumb_color(encumb(bp_feet)), "%d", encumb(bp_feet));
 wrefresh(w_encumb);

// Next, draw traits.
 line = 2;
 std::vector <pl_flag> traitslist;
 mvwprintz(w_traits, 0, 9, c_ltgray, "TRAITS");
 for (int i = 0; i < PF_MAX2; i++) {
  if (my_traits[i]) {
   traitslist.push_back(pl_flag(i));
   if (i > PF_MAX2)
    status = c_ltblue;
   else if (traits[i].points > 0)
    status = c_ltgreen;
   else
    status = c_ltred;
   if (line < 9) {
    mvwprintz(w_traits, line, 1, status, traits[i].name.c_str());
    line++;
   }
  }
 }
 wrefresh(w_traits);

// Next, draw effects.
 line = 2;
 mvwprintz(w_effects, 0, 8, c_ltgray, "EFFECTS");
 for (int i = 0; i < effect_name.size() && line < 9; i++) {
  mvwprintz(w_effects, line, 1, c_ltgray, effect_name[i].c_str());
  line++;
 }
 wrefresh(w_effects);

// Next, draw skills.
 line = 2;
 std::vector <skill> skillslist;
 mvwprintz(w_skills, 0, 11, c_ltgray, "SKILLS");
 for (int i = 1; i < num_skill_types; i++) {
  if (sklevel[i] > 0) {
   skillslist.push_back(skill(i));
   if (line < 9) {
    mvwprintz(w_skills, line, 1, c_ltblue, "%s:",
              skill_name(skill(i)).c_str());
    mvwprintz(w_skills, line,19, c_ltblue, "%d (%s%d%%%%)", sklevel[i],
              (skexercise[i] < 10 && skexercise[i] >= 0 ? " " : ""),
              (skexercise[i] <  0 ? 0 : skexercise[i]));
    line++;
   }
  }
 }
 wrefresh(w_skills);

// Finally, draw speed.
 mvwprintz(w_speed, 0, 11, c_ltgray, "SPEED");
 mvwprintz(w_speed, 2,  1, c_ltgray, "Current Speed:");
 int newmoves = current_speed();
 int pen = 0;
 line = 3;
 if (weight_carried() > weight_capacity()) {
  mvwprintz(w_speed, line, 1, c_red, "OVERBURDENED! Move 1!!!");
  line++;
 } else if (weight_carried() > weight_capacity() * .25) {
  pen = 100 - int((120 * (weight_capacity() - weight_carried())) /
                         weight_capacity());
  mvwprintz(w_speed, line, 1, c_red, "Overburdened        -%s%d%%%%",
            (pen < 10 ? " " : ""), pen);
  line++;
 }
 pen = int(morale_level() / 25);
 if (abs(pen) >= 4) {
  if (pen > 10)
   pen = 10;
  else if (pen < -10)
   pen = -10;
  if (pen > 0)
   mvwprintz(w_speed, line, 1, c_green, "Good mood           +%s%d%%%%",
             (pen < 10 ? " " : ""), pen);
  else
   mvwprintz(w_speed, line, 1, c_red, "Depressed           -%s%d%%%%",
             (abs(pen) < 10 ? " " : ""), abs(pen));
  line++;
 }
 pen = int((pain - pkill) * .7);
 if (pen >= 1) {
  mvwprintz(w_speed, line, 1, c_red, "Pain                -%s%d%%%%",
            (pen < 10 ? " " : ""), pen);
  line++;
 }
 if (pkill >= 10) {
  pen = int(pkill * .1);
  mvwprintz(w_speed, line, 1, c_red, "Painkillers         -%s%d%%%%",
            (pen < 10 ? " " : ""), pen);
  line++;
 }
 if (stim != 0) {
  pen = stim;
  if (pen > 0)
   mvwprintz(w_speed, line, 1, c_green, "Stimulants          +%s%d%%%%",
            (pen < 10 ? " " : ""), pen);
  else
   mvwprintz(w_speed, line, 1, c_red, "Depressants         -%s%d%%%%",
            (abs(pen) < 10 ? " " : ""), abs(pen));
  line++;
 }
 if (thirst > 40) {
  pen = int((thirst - 40) / 10);
  mvwprintz(w_speed, line, 1, c_red, "Thirst              -%s%d%%%%",
            (pen < 10 ? " " : ""), pen);
  line++;
 }
 if (hunger > 100) {
  pen = int((hunger - 100) / 10);
  mvwprintz(w_speed, line, 1, c_red, "Hunger              -%s%d%%%%",
            (pen < 10 ? " " : ""), pen);
  line++;
 }
 if (has_trait(PF_QUICK)) {
  pen = int(newmoves * .1);
  mvwprintz(w_speed, line, 1, c_green, "Quick               +%s%d%%%%",
            (pen < 10 ? " " : ""), pen);
 }
 mvwprintw(w_speed, 2, (newmoves >= 100 ? 21 : (newmoves < 10 ? 23 : 22)),
           "%d%%", newmoves);
 wrefresh(w_speed);

 refresh();
 int curtab = 1;
 int min, max;
 line = 0;
 bool done = false;

// Initial printing is DONE.  Now we give the player a chance to scroll around
// and "hover" over different items for more info.
 do {
  werase(w_info);
  switch (curtab) {
  case 1:	// Stats tab
   mvwprintz(w_stats, 0, 0, h_ltgray, "          STATS           ");
   if (line == 0) {
    mvwprintz(w_stats, 2, 2, h_ltgray, "Strength:");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Strength affects your melee damage, the amount of weight you can carry, your\n\
total HP, your resistance to many diseases, and the effectiveness of actions\n\
which require brute force.");
   } else if (line == 1) {
    mvwprintz(w_stats, 3, 2, h_ltgray, "Dexterity:");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Dexterity affects your chance to hit in melee combat, helps you steady your\n\
gun for ranged combat, and enhances many actions that require finesse."); 
   } else if (line == 2) {
    mvwprintz(w_stats, 4, 2, h_ltgray, "Intelligence:");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Intelligence is less important in most situations, but it is vital for more\n\
complex tasks like electronics crafting. It also affects how much skill you\n\
can pick up from reading a book.");
   } else if (line == 3) {
    mvwprintz(w_stats, 5, 2, h_ltgray, "Perception:");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Perception is the most important stat for ranged combat. It's also used for\n\
detecting traps and other things of interest.");
   }
   wrefresh(w_stats);
   wrefresh(w_info);
   switch (input()) {
    case 'j':
     line++;
     if (line == 4)
      line = 0;
     break;
    case 'k':
     line--;
     if (line == -1)
      line = 3;
     break;
    case '\t':
     mvwprintz(w_stats, 0, 0, c_ltgray, "          STATS           ");
     wrefresh(w_stats);
     line = 0;
     curtab++;
     break;
    case 'q':
    case KEY_ESCAPE:
     done = true;
   }
   mvwprintz(w_stats, 2, 2, c_ltgray, "Strength:");
   mvwprintz(w_stats, 3, 2, c_ltgray, "Dexterity:");
   mvwprintz(w_stats, 4, 2, c_ltgray, "Intelligence:");
   mvwprintz(w_stats, 5, 2, c_ltgray, "Perception:");
   wrefresh(w_stats);
   break;
  case 2:	// Encumberment tab
   mvwprintz(w_encumb, 0, 0, h_ltgray, "      ENCUMBERANCE        ");
   if (line == 0) {
    mvwprintz(w_encumb, 2, 2, h_ltgray, "Head");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Head encumberance has no effect; it simply limits how much you can put on.");
   } else if (line == 1) {
    mvwprintz(w_encumb, 3, 2, h_ltgray, "Eyes");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Perception -%d when checking traps or firing ranged weapons;\n\
Perception -%.1f when throwing items", encumb(bp_eyes),
double(double(encumb(bp_eyes)) / 2));
   } else if (line == 2) {
    mvwprintz(w_encumb, 4, 2, h_ltgray, "Mouth");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Running costs +%d movement points", encumb(bp_mouth) * 5);
   } else if (line == 3) {
    mvwprintz(w_encumb, 5, 2, h_ltgray, "Torso");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Melee skill -%d;      Dodge skill -%d;\n\
Swimming costs +%d movement points;\n\
Melee attacks cost +%d movement points", encumb(bp_torso), encumb(bp_torso),
encumb(bp_torso) * (80 - sklevel[sk_swimming] * 3), encumb(bp_torso) * 20);
   } else if (line == 4) {
    mvwprintz(w_encumb, 6, 2, h_ltgray, "Hands");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Reloading costs +%d movement points;\n\
Dexterity -%d when throwing items", encumb(bp_hands) * 30, encumb(bp_hands));
   } else if (line == 5) {
    mvwprintz(w_encumb, 7, 2, h_ltgray, "Legs");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Running costs +%d movement points;  Swimming costs +%d movement points;\n\
Dodge skill -%.1f", encumb(bp_legs) * 3,
encumb(bp_legs) * (50 - sklevel[sk_swimming]),
double(double(encumb(bp_legs)) / 2));
   } else if (line == 6) {
    mvwprintz(w_encumb, 8, 2, h_ltgray, "Feet");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Running costs %s%d movement points", (encumb(bp_feet) >= 0 ? "+" : ""),
encumb(bp_feet) * 5);
   }
   wrefresh(w_encumb);
   wrefresh(w_info);
   switch (input()) {
    case 'j':
     line++;
     if (line == 7)
      line = 0;
     break;
    case 'k':
     line--;
     if (line == -1)
      line = 6;
     break;
    case '\t':
     mvwprintz(w_encumb, 0, 0, c_ltgray, "      ENCUMBERANCE        ");
     wrefresh(w_encumb);
     line = 0;
     curtab++;
     break;
    case 'q':
    case KEY_ESCAPE:
     done = true;
   }
   mvwprintz(w_encumb, 2, 2, c_ltgray, "Head");
   mvwprintz(w_encumb, 3, 2, c_ltgray, "Eyes");
   mvwprintz(w_encumb, 4, 2, c_ltgray, "Mouth");
   mvwprintz(w_encumb, 5, 2, c_ltgray, "Torso");
   mvwprintz(w_encumb, 6, 2, c_ltgray, "Hands");
   mvwprintz(w_encumb, 7, 2, c_ltgray, "Legs");
   mvwprintz(w_encumb, 8, 2, c_ltgray, "Feet");
   wrefresh(w_encumb);
   break;
  case 3:	// Traits tab
   mvwprintz(w_traits, 0, 0, h_ltgray, "         TRAITS           ");
   if (line <= 2) {
    min = 0;
    max = 7;
    if (traitslist.size() < max)
     max = traitslist.size();
   } else if (line >= traitslist.size() - 3) {
    min = (traitslist.size() < 8 ? 0 : traitslist.size() - 7);
    max = traitslist.size();
   } else {
    min = line - 3;
    max = line + 4;
    if (traitslist.size() < max)
     max = traitslist.size();
   }
   for (int i = min; i < max; i++) {
    mvwprintz(w_traits, 2 + i - min, 1, c_ltgray, "                         ");
    if (traitslist[i] > PF_MAX2)
     status = c_ltblue;
    else if (traits[traitslist[i]].points > 0)
     status = c_ltgreen;
    else
     status = c_ltred;
    if (i == line)
     mvwprintz(w_traits, 2 + i - min, 1, hilite(status),
               traits[traitslist[i]].name.c_str());
    else
     mvwprintz(w_traits, 2 + i - min, 1, status,
               traits[traitslist[i]].name.c_str());
   }
   if (line >= 0 && line < traitslist.size())
    mvwprintz(w_info, 0, 0, c_magenta, "%s",
              traits[traitslist[line]].description.c_str());
   wrefresh(w_traits);
   wrefresh(w_info);
   switch (input()) {
    case 'j':
     if (line < traitslist.size() - 1)
      line++;
     break;
    case 'k':
     if (line > 0)
      line--;
     break;
    case '\t':
     mvwprintz(w_traits, 0, 0, c_ltgray, "         TRAITS           ");
     for (int i = 0; i < traitslist.size() && i < 7; i++) {
      mvwprintz(w_traits, i + 2, 1, c_black, "xxxxxxxxxxxxxxxxxxxxxxxxx");
      if (traitslist[i] > PF_MAX2)
       status = c_ltblue;
      else if (traits[traitslist[i]].points > 0)
       status = c_ltgreen;
      else
       status = c_ltred;
      mvwprintz(w_traits, i + 2, 1, status, traits[traitslist[i]].name.c_str());
     }
     wrefresh(w_traits);
     line = 0;
     curtab++;
     break;
    case 'q':
    case KEY_ESCAPE:
     done = true;
   }
   break;

  case 4:	// Effects tab
   mvwprintz(w_effects, 0, 0, h_ltgray, "        EFFECTS           ");
   if (line <= 2) {
    min = 0;
    max = 7;
    if (effect_name.size() < max)
     max = effect_name.size();
   } else if (line >= effect_name.size() - 3) {
    min = (effect_name.size() < 8 ? 0 : effect_name.size() - 7);
    max = effect_name.size();
   } else {
    min = line - 2;
    max = line + 4;
    if (effect_name.size() < max)
     max = effect_name.size();
   }
   for (int i = min; i < max; i++) {
    if (i == line)
     mvwprintz(w_effects, 2 + i - min, 1, h_ltgray, effect_name[i].c_str());
    else
     mvwprintz(w_effects, 2 + i - min, 1, c_ltgray, effect_name[i].c_str());
   }
   if (line >= 0 && line < effect_text.size())
    mvwprintz(w_info, 0, 0, c_magenta, effect_text[line].c_str());
   wrefresh(w_effects);
   wrefresh(w_info);
   switch (input()) {
    case 'j':
     if (line < effect_name.size() - 1)
      line++;
     break;
    case 'k':
     if (line > 0)
      line--;
     break;
    case '\t':
     mvwprintz(w_effects, 0, 0, c_ltgray, "        EFFECTS           ");
     for (int i = 0; i < effect_name.size() && i < 7; i++)
      mvwprintz(w_effects, i + 2, 1, c_ltgray, effect_name[i].c_str());
     wrefresh(w_effects);
     line = 0;
     curtab++;
     break;
    case 'q':
    case KEY_ESCAPE:
     done = true;
   }
   break;

  case 5:	// Skills tab
   mvwprintz(w_skills, 0, 0, h_ltgray, "           SKILLS         ");
   if (line <= 2) {
    min = 0;
    max = 7;
    if (skillslist.size() < max)
     max = skillslist.size();
   } else if (line >= skillslist.size() - 3) {
    min = (skillslist.size() < 8 ? 0 : skillslist.size() - 7);
    max = skillslist.size();
   } else {
    min = line - 3;
    max = line + 4;
    if (skillslist.size() < max)
     max = skillslist.size();
    if (min < 0)
     min = 0;
   }
   for (int i = min; i < max; i++) {
    if (i == line) {
     if (skexercise[skillslist[i]] >= 100)
      status = h_pink;
     else
      status = h_ltblue;
    } else {
     if (skexercise[skillslist[i]] < 0)
      status = c_ltred;
     else
      status = c_ltblue;
    }
    mvwprintz(w_skills, 2 + i - min, 1, c_ltgray, "                         ");
    if (skexercise[i] >= 100) {
     mvwprintz(w_skills, 2 + i - min, 1, status, "%s:",
               skill_name(skillslist[i]).c_str());
     mvwprintz(w_skills, 2 + i - min,19, status, "%d (%s%d%%%%)",
               sklevel[skillslist[i]],
               (skexercise[skillslist[i]] < 10 &&
                skexercise[skillslist[i]] >= 0 ? " " : ""),
               (skexercise[skillslist[i]] <  0 ? 0 :
                skexercise[skillslist[i]]));
    } else {
     mvwprintz(w_skills, 2 + i - min, 1, status, "%s:",
               skill_name(skillslist[i]).c_str());
     mvwprintz(w_skills, 2 + i - min,19, status, "%d (%s%d%%%%)", 
               sklevel[skillslist[i]],
               (skexercise[skillslist[i]] < 10 &&
                skexercise[skillslist[i]] >= 0 ? " " : ""),
               (skexercise[skillslist[i]] <  0 ? 0 :
                skexercise[skillslist[i]]));
    }
   }
   werase(w_info);
   if (line >= 0 && line < skillslist.size())
    mvwprintz(w_info, 0, 0, c_magenta,
              skill_description(skillslist[line]).c_str());
   wrefresh(w_skills);
   wrefresh(w_info);
   switch (input()) {
    case 'j':
     if (line < skillslist.size() - 1)
      line++;
     break;
    case 'k':
     if (line > 0)
      line--;
     break;
    case '\t':
     mvwprintz(w_skills, 0, 0, c_ltgray, "           SKILLS         ");
     for (int i = 0; i < skillslist.size() && i < 7; i++) {
      if (skexercise[skillslist[i]] < 0)
       status = c_ltred;
      else
       status = c_ltblue;
      mvwprintz(w_skills, i + 2,  1, status, "%s:",
                skill_name(skillslist[i]).c_str());
      mvwprintz(w_skills, i + 2, 19, status, "%d (%s%d%%%%)",
                sklevel[skillslist[i]],
                (skexercise[skillslist[i]] < 10 &&
                 skexercise[skillslist[i]] >= 0 ? " " : ""),
                (skexercise[skillslist[i]] <  0 ? 0 :
                 skexercise[skillslist[i]]));
     }
     wrefresh(w_skills);
     line = 0;
     curtab = 1;
     break;
    case 'q':
    case 'Q':
    case KEY_ESCAPE:
     done = true;
   }
  }
 } while (!done);
 
 werase(w_info);
 werase(w_grid);
 werase(w_stats);
 werase(w_encumb);
 werase(w_traits);
 werase(w_effects);
 werase(w_skills);
 werase(w_speed);
 werase(w_info);

 delwin(w_info);
 delwin(w_grid);
 delwin(w_stats);
 delwin(w_encumb);
 delwin(w_traits);
 delwin(w_effects);
 delwin(w_skills);
 delwin(w_speed);
 delwin(w_info);
 erase();
}

void player::disp_morale()
{
 WINDOW *w = newwin(25, 80, 0, 0);
 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 mvwprintz(w, 1,  1, c_white, "Morale Modifiers:");
 mvwprintz(w, 2,  1, c_ltgray, "Name");
 mvwprintz(w, 2, 20, c_ltgray, "Value");

 for (int i = 0; i < morale.size(); i++) {
  int b = morale[i].bonus;

  int bpos = 24;
  if (abs(b) >= 10)
   bpos--;
  if (abs(b) >= 100)
   bpos--;
  if (b < 0)
   bpos--;

  mvwprintz(w, i + 3,  1, (b < 0 ? c_red : c_green),
            morale[i].name(morale_data).c_str());
  mvwprintz(w, i + 3, bpos, (b < 0 ? c_red : c_green), "%d", b);
 }

 int mor = morale_level();
 int bpos = 24;
  if (abs(mor) >= 10)
   bpos--;
  if (abs(mor) >= 100)
   bpos--;
  if (mor < 0)
   bpos--;
 mvwprintz(w, 20, 1, (mor < 0 ? c_red : c_green), "Total:");
 mvwprintz(w, 20, bpos, (mor < 0 ? c_red : c_green), "%d", mor);

 wrefresh(w);
 getch();
 werase(w);
 delwin(w);
}
 

void player::disp_status(WINDOW *w)
{
 mvwprintz(w, 1, 0, c_ltgray, "Weapon: %s", weapname().c_str());
 if (weapon.is_gun()) {
       if (recoil >= 36)
   mvwprintz(w, 1, 30, c_red,    "Recoil");
  else if (recoil >= 20)
   mvwprintz(w, 1, 30, c_ltred,  "Recoil");
  else if (recoil >= 4)
   mvwprintz(w, 1, 30, c_yellow, "Recoil");
  else if (recoil > 0)
   mvwprintz(w, 1, 30, c_ltgray, "Recoil");
 }

      if (hunger > 2800)
  mvwprintz(w, 2, 0, c_red,    "Starving!");
 else if (hunger > 1400)
  mvwprintz(w, 2, 0, c_ltred,  "Near starving");
 else if (hunger > 300)
  mvwprintz(w, 2, 0, c_ltred,  "Famished");
 else if (hunger > 100)
  mvwprintz(w, 2, 0, c_yellow, "Very hungry");
 else if (hunger > 40)
  mvwprintz(w, 2, 0, c_yellow, "Hungry");
 else if (hunger < 0)
  mvwprintz(w, 2, 0, c_green,  "Full");

      if (thirst > 520)
  mvwprintz(w, 2, 15, c_ltred,  "Parched");
 else if (thirst > 240)
  mvwprintz(w, 2, 15, c_ltred,  "Dehydrated");
 else if (thirst > 80)
  mvwprintz(w, 2, 15, c_yellow, "Very thirsty");
 else if (thirst > 40)
  mvwprintz(w, 2, 15, c_yellow, "Thirsty");
 else if (thirst < 0)
  mvwprintz(w, 2, 15, c_green,  "Slaked");

      if (fatigue > 575)
  mvwprintz(w, 2, 30, c_red,    "Exhausted");
 else if (fatigue > 383)
  mvwprintz(w, 2, 30, c_ltred,  "Dead tired");
 else if (fatigue > 191)
  mvwprintz(w, 2, 30, c_yellow, "Tired");

 mvwprintz(w, 2, 41, c_white, "XP: ");
 nc_color col_xp = c_dkgray;
 if (xp_pool >= 100)
  col_xp = c_white;
 else if (xp_pool >  0)
  col_xp = c_ltgray;
 mvwprintz(w, 2, 45, col_xp, "%d", xp_pool);

      if (pain - pkill >= 50)
  mvwprintz(w, 3, 0, c_red,    "Excrutiating pain!");
 else if (pain - pkill >= 40)
  mvwprintz(w, 3, 0, c_ltred,  "Extreme pain");
 else if (pain - pkill >= 30)
  mvwprintz(w, 3, 0, c_ltred,  "Intense pain");
 else if (pain - pkill >= 20)
  mvwprintz(w, 3, 0, c_yellow, "Heavy pain");
 else if (pain - pkill >= 10)
  mvwprintz(w, 3, 0, c_yellow, "Moderate pain");
 else if (pain - pkill >   0)
  mvwprintz(w, 3, 0, c_yellow, "Minor pain");

 nc_color col_str = c_white, col_dex = c_white, col_int = c_white,
          col_per = c_white, col_spd = c_white;
 if (str_cur < str_max)
  col_str = c_red;
 if (str_cur > str_max)
  col_str = c_green;
 if (dex_cur < dex_max)
  col_dex = c_red;
 if (dex_cur > dex_max)
  col_dex = c_green;
 if (int_cur < int_max)
  col_int = c_red;
 if (int_cur > int_max)
  col_int = c_green;
 if (per_cur < per_max)
  col_per = c_red;
 if (per_cur > per_max)
  col_per = c_green;
 int spd_cur = current_speed();
 if (current_speed() < 100)
  col_spd = c_red;
 if (current_speed() > 100)
  col_spd = c_green;

 mvwprintz(w, 3, 20, col_str, "Str %s%d", str_cur >= 10 ? "" : " ", str_cur);
 mvwprintz(w, 3, 27, col_dex, "Dex %s%d", dex_cur >= 10 ? "" : " ", dex_cur);
 mvwprintz(w, 3, 34, col_int, "Int %s%d", int_cur >= 10 ? "" : " ", int_cur);
 mvwprintz(w, 3, 41, col_per, "Per %s%d", per_cur >= 10 ? "" : " ", per_cur);
 mvwprintz(w, 3, 48, col_spd, "Spd %s%d", spd_cur >= 10 ? "" : " ", spd_cur);

}

bool player::has_trait(int flag)
{
 return my_traits[flag];
}

void player::toggle_trait(int flag)
{
 my_traits[flag] = !my_traits[flag];
}

bool player::has_bionic(bionic_id b)
{
 for (int i = 0; i < my_bionics.size(); i++) {
  if (my_bionics[i].id == b)
   return true;
 }
 return false;
}

bool player::has_active_bionic(bionic_id b)
{
 for (int i = 0; i < my_bionics.size(); i++) {
  if (my_bionics[i].id == b)
   return (my_bionics[i].powered);
 }
 return false;
}

void player::add_bionic(bionic_id b)
{
 for (int i = 0; i < my_bionics.size(); i++) {
  if (my_bionics[i].id == b)
   return;	// No duplicates!
 }
 char newinv;
 if (my_bionics.size() == 0)
  newinv = 'a';
 else
  newinv = my_bionics[my_bionics.size() - 1].invlet + 1;
 my_bionics.push_back(bionic(b, newinv));
}

void player::charge_power(int amount)
{
 power_level += amount;
 if (power_level > max_power_level)
  power_level = max_power_level;
 if (power_level < 0)
  power_level = 0;
}

void player::power_bionics(game *g)
{
 WINDOW *wBio = newwin(25, 80, 0, 0);
 werase(wBio);
 std::vector <bionic> passive;
 std::vector <bionic> active;
 mvwprintz(wBio, 0, 0, c_blue, "BIONICS -");
 mvwprintz(wBio, 0,10, c_white,
           "Activating.  Press '!' to examine your implants.");

 for (int i = 0; i < 80; i++) {
  mvwputch(wBio,  1, i, c_ltgray, LINE_OXOX);
  mvwputch(wBio, 21, i, c_ltgray, LINE_OXOX);
 }
 for (int i = 0; i < my_bionics.size(); i++) {
  if ( bionics[my_bionics[i].id].power_source ||
      !bionics[my_bionics[i].id].activated      )
   passive.push_back(my_bionics[i]);
  else
   active.push_back(my_bionics[i]);
 }
 nc_color type;
 if (passive.size() > 0) {
  mvwprintz(wBio, 2, 0, c_ltblue, "Passive:");
  for (int i = 0; i < passive.size(); i++) {
   if (bionics[passive[i].id].power_source)
    type = c_ltcyan;
   else
    type = c_cyan;
   mvwputch(wBio, 3 + i, 0, type, passive[i].invlet);
   mvwprintz(wBio, 3 + i, 2, type, bionics[passive[i].id].name.c_str());
  }
 }
 if (active.size() > 0) {
  mvwprintz(wBio, 2, 32, c_ltblue, "Active:");
  for (int i = 0; i < active.size(); i++) {
   if (active[i].powered)
    type = c_red;
   else
    type = c_ltred;
   mvwputch(wBio, 3 + i, 32, type, active[i].invlet);
   mvwprintz(wBio, 3 + i, 34, type,
             (active[i].powered ? "%s - ON" : "%s - %d PU / %d trns"),
             bionics[active[i].id].name.c_str(),
             bionics[active[i].id].power_cost,
             bionics[active[i].id].charge_time);
  }
 }

 wrefresh(wBio);
 char ch;
 bool activating = true;
 bionic *tmp;
 int b;
 do {
  ch = getch();
  if (ch == '!') {
   activating = !activating;
   if (activating)
    mvwprintz(wBio, 0, 10, c_white,
              "Activating.  Press '!' to examine your implants.");
   else
    mvwprintz(wBio, 0, 10, c_white,
              "Examining.  Press '!' to activate your implants.");
  } else if (ch == ' ')
   ch = KEY_ESCAPE;
  else if (ch != KEY_ESCAPE) {
   for (int i = 0; i < my_bionics.size(); i++) {
    if (ch == my_bionics[i].invlet) {
     tmp = &my_bionics[i];
     b = i;
     ch = KEY_ESCAPE;
    }
   }
   if (ch == KEY_ESCAPE) {
    if (activating) {
     if (bionics[tmp->id].activated) {
      if (tmp->powered) {
       tmp->powered = false;
       g->add_msg("%s powered off.", bionics[tmp->id].name.c_str());
      } else if (power_level >= bionics[tmp->id].power_cost)
       activate_bionic(b, g);
     } else
      mvwprintz(wBio, 22, 0, c_ltred, "\
You can not activate %s!  To read a description of \
%s, press '!', then '%c'.", bionics[tmp->id].name.c_str(),
                            bionics[tmp->id].name.c_str(), tmp->invlet);
    } else {	// Describing bionics, not activating them!
// Clear the lines first
     ch = 0;
     mvwprintz(wBio, 22, 0, c_ltgray, "\
                                                                               \
                                                                               \
                                                                             ");
     mvwprintz(wBio, 22, 0, c_ltblue,
               bionics[tmp->id].description.c_str());
    }
   }
  }
  wrefresh(wBio);
 } while (ch != KEY_ESCAPE);
 werase(wBio);
 wrefresh(wBio);
 delwin(wBio);
 erase();
}

void player::mutate(game *g)
{
 while (true) {	// Infinite loop until a mutation forms.
// GOOD mutations are a 2 in 7 chance, unless you have robust genetics (in which
//  case it is a 9 in 14 chance).
  if (rng(1, 7) >= 6 || (has_trait(PF_ROBUST) && one_in(2))) {
   switch (rng(1, 24)) {
   case 1:
    g->add_msg("Your muscles ripple and grow.");
    str_max += 1;
    return;
   case 2:
    g->add_msg("Your muscles feel more nimble.");
    dex_max += 1;
    return;
   case 3:
    g->add_msg("Your mind feels more clear.");
    int_max += 1;
    return;
   case 4:
    g->add_msg("You feel more aware of the world around you.");
    per_max += 1;
    return;
   case 5:
    if (!has_trait(PF_QUICK)) {
     g->add_msg("You feel quicker!");
     toggle_trait(PF_QUICK);
     return;
    }
    break;
   case 6:
    if (!has_trait(PF_LIGHTEATER)) {
     g->add_msg("You feel less hungry.");
     toggle_trait(PF_LIGHTEATER);
     return;
    }
    break;
   case 7:
    if (!has_trait(PF_PAINRESIST)) {
     if (pain > 0)
      g->add_msg("Your pain feels dulled.");
     else {
      per_max -= rng(0, 1);	// Tough break!
      g->add_msg("Your senses feel dulled.");
     }
     toggle_trait(PF_PAINRESIST);
     return;
    }
    break;
   case 8:
    if (!has_trait(PF_NIGHTVISION) && !has_trait(PF_NIGHTVISION2)) {
     if (g->light_level() <= 2)
      g->add_msg("The darkness seems clearer.");
     toggle_trait(PF_NIGHTVISION);
     return;
    } else if (!has_trait(PF_NIGHTVISION2)) {
     if (g->light_level() <= 4)
      g->add_msg("The darkness seems clearer.");
     toggle_trait(PF_NIGHTVISION);
     toggle_trait(PF_NIGHTVISION2);
     return;
    }
    break;
   case 9:
    if (!has_trait(PF_THICKSKIN)) {
     g->add_msg("Your skin thickens.");
     toggle_trait(PF_THICKSKIN);
     return;
    }
    break;
   case 10:
    if (!has_trait(PF_SUPERTASTER)) {
     g->add_msg("Your sense of taste feels acute.");
     toggle_trait(PF_SUPERTASTER);
     return;
    }
    break;
   case 11:
    if (has_trait(PF_MYOPIC)) {
     g->add_msg("Your vision feels sharper.");
     toggle_trait(PF_MYOPIC);
     return;
    }
    break;
   case 12:
    if (has_trait(PF_ASTHMA)) {
     if (has_disease(DI_ASTHMA)) {
      g->add_msg("Your asthma clears up!");
      rem_disease(DI_ASTHMA);
     }
     toggle_trait(PF_ASTHMA);
     return;
    }
    break;
   case 13:
    if (has_trait(PF_DEFORMED2) || has_trait(PF_DEFORMED)) {
     g->add_msg("Your deformations %smelt away.",// lol smelt
                (has_trait(PF_DEFORMED2) ? "partially " : ""));
     if (has_trait(PF_DEFORMED2))
      toggle_trait(PF_DEFORMED2);
     toggle_trait(PF_DEFORMED);
     return;
    }
    break;
   case 14:
    if (!has_trait(PF_INFRARED)) {
     g->add_msg("You can suddenly see heat.");
     toggle_trait(PF_INFRARED);
     return;
    }
    break;
   case 15:
    if (has_trait(PF_ROT)) {
     g->add_msg("Your body stops disintegrating.");
     toggle_trait(PF_ROT);
     return;
// Regeneration is really good, so give it a 4 in 5 chance to fail
    } else if (!has_trait(PF_REGEN) && one_in(5)) {
     g->add_msg("You feel your flesh rebuilding itself!");
     toggle_trait(PF_REGEN);
     return;
    }
    break;
   case 16:
    if (!has_trait(PF_FANGS)) {
     if (has_trait(PF_BEAK)) {
      g->add_msg("Your beak shrinks back into a normal mouth!");
      toggle_trait(PF_BEAK);
     } else {
      g->add_msg("Your teeth lengthen into huge fangs!");
      toggle_trait(PF_FANGS);
     }
     return;
    }
    break;
   case 17:
    if (!has_trait(PF_MEMBRANE)) {
     g->add_msg("You grow a set of clear eyelids.");
     toggle_trait(PF_MEMBRANE);
     return;
    }
   case 18:
    if (!has_trait(PF_GILLS)) {
     g->add_msg("A set of gills forms in your neck!");
     toggle_trait(PF_GILLS);
     return;
    }
    break;
   case 19:
    if (!has_trait(PF_SCALES)) {
     g->add_msg("Thick green scales grow to cover your body!");
     toggle_trait(PF_SCALES);
     return;
    }
    break;
   case 20:
    if (!has_trait(PF_FUR)) {
     g->add_msg("Thick black fur grows to cover your body!");
     toggle_trait(PF_FUR);
     return;
    }
    break;
   case 21:
    if (!has_trait(PF_CHITIN)) {
     g->add_msg("You develop a chitinous exoskeleton!");
     toggle_trait(PF_CHITIN);
     return;
    }
    break;
   case 22:
    if (!has_trait(PF_TALONS)) {
     g->add_msg("Your index fingers grow into huge talons!");
     toggle_trait(PF_TALONS);
     return;
    }
    break;
   case 23:
    if (!has_trait(PF_RADIOGENIC) && one_in(2)) {
     g->add_msg("You crave irradiation!");
     toggle_trait(PF_RADIOGENIC);
     return;
    }
    break;
   case 24:
    if (!has_trait(PF_SPINES) && !has_trait(PF_QUILLS)) {
     g->add_msg("Fine spines grow over your body.");
     toggle_trait(PF_SPINES);
     return;
    } else if (!has_trait(PF_QUILLS)) {
     g->add_msg("Your spines grow into large quills.");
     toggle_trait(PF_SPINES);
     toggle_trait(PF_QUILLS);
     return;
    }
    break;
   case 25:
    if (has_trait(PF_HERBIVORE)) {
     g->add_msg("You feel like you could stomach some meat.");
     toggle_trait(PF_HERBIVORE);
     return;
    } else if (has_trait(PF_CARNIVORE)) {
     g->add_msg("You feel like you could stomach some vegetables.");
     toggle_trait(PF_CARNIVORE);
     return;
    }
    break;
   }
  } else {	// Bad mutations!
   switch (rng(1, 24)) {
   case 1:
    g->add_msg("You feel weak!");
    str_max -= 1;
    return;
   case 2:
    g->add_msg("You feel clumsy!");
    dex_max -= 1;
    return;
   case 3:
    g->add_msg("You feel stupid!");
    int_max -= 1;
    return;
   case 4:
    g->add_msg("You feel less perceptive!");
    per_max -= 1;
    return;
   case 5:
    if (!has_trait(PF_MYOPIC)) {
     g->add_msg("Your vision blurs.");
     toggle_trait(PF_MYOPIC);
     return;
    }
    break;
   case 6:
    if (!has_trait(PF_BADHEARING)) {
     g->add_msg("Your hearing seems dampened.");
     toggle_trait(PF_BADHEARING);
     return;
    }
    break;
   case 7:
    if (!has_trait(PF_FORGETFUL)) {
     g->add_msg("Your memory seems hazy.");
     toggle_trait(PF_FORGETFUL);
     return;
    }
    break;
   case 8:
    if (!has_trait(PF_CHEMIMBALANCE)) {
     g->add_msg("You feel weird inside...");
     toggle_trait(PF_CHEMIMBALANCE);
     return;
    }
    break;
   case 9:
    if (!has_trait(PF_SCHIZOPHRENIC)) {
     if (one_in(2)) {
      g->add_msg("You feel your sanity slipping away.");
      toggle_trait(PF_SCHIZOPHRENIC);
     } else
      g->add_msg("You feel a little unhinged.");	// Lucked out!
     return;
    }
    break;
   case 10:
    if (!has_trait(PF_DEFORMED) && !has_trait(PF_DEFORMED2)) {
     g->add_msg("Your flesh twists and deforms.");
     toggle_trait(PF_DEFORMED);
     return;
    } else if (!has_trait(PF_DEFORMED2)) {
     g->add_msg("Your flesh deforms further; you're hideous!");
     toggle_trait(PF_DEFORMED);
     toggle_trait(PF_DEFORMED2);
     return;
    }
    break;
   case 11:
    if (has_trait(PF_LIGHTEATER)) {
     g->add_msg("You suddenly feel hungrier.");
     hunger += 20;
     toggle_trait(PF_LIGHTEATER);
     return;
    } else if (!has_trait(PF_HUNGER)) {
     g->add_msg("You suddenly feel incredibly hungry.");
     hunger += 50;
     toggle_trait(PF_HUNGER);
     return;
    }
    break;
   case 12:
    if (!has_trait(PF_THIRST)) {
     g->add_msg("Your skin crackles and dries out.");
     thirst += 50;
     toggle_trait(PF_THIRST);
     return;
    }
    break;
   case 13:
    if (has_trait(PF_REGEN)) {
     g->add_msg("Your flesh stops regenerating.");
     toggle_trait(PF_REGEN);
     return;
    } else if (!has_trait(PF_ROT) && one_in(5)) {
     g->add_msg("You can feel your body disintegrating!");
     toggle_trait(PF_ROT);
     return;
    }
    break;
   case 14:
    if (!has_trait(PF_ALBINO)) {
     g->add_msg("Your skin pigment fades away.");
     toggle_trait(PF_ALBINO);
     return;
    }
    break;
   case 15:
    if (!has_trait(PF_SORES)) {
     g->add_msg("Painful boils appear all over your body!");
     toggle_trait(PF_SORES);
     return;
    }
    break;
   case 16:
    if (!has_trait(PF_TROGLO)) {
     if (g->is_in_sunlight(posx, posy))
      g->add_msg("The sunlight starts to irritate you!");
     else
      g->add_msg("You feel at home here, out of the sunlight.");
     toggle_trait(PF_TROGLO);
     return;
    }
    break;
   case 17:
    if (!has_trait(PF_WEBBED)) {
     g->add_msg("Your hands and feet develop a thick webbing!");
// Force off any gloves
     for (int i = 0; i < worn.size(); i++) {
      if ((dynamic_cast<it_armor*>(worn[i].type))->covers & mfb(bp_hands)) {
       g->add_msg("Your %s are pushed off!", worn[i].tname(g).c_str());
       g->m.add_item(posx, posy, worn[i]);
       worn.erase(worn.begin() + i);
       i--;
      }
     }
     toggle_trait(PF_WEBBED);
     return;
    }
    break;
   case 18:
    if (!has_trait(PF_BEAK)) {
     if (has_trait(PF_FANGS)) {
      g->add_msg("Your fangs fade away.");
      toggle_trait(PF_FANGS);
      return;
     }
     g->add_msg("Your mouth extends into a beak!");
     toggle_trait(PF_BEAK);
// Force off and damage any mouthwear
     for (int i = 0; i < worn.size(); i++) {
      if ((dynamic_cast<it_armor*>(worn[i].type))->covers & mfb(bp_mouth)) {
       if (worn[i].damage >= 3)
        g->add_msg("Your %s is destroyed!", worn[i].tname(g).c_str());
       else {
        worn[i].damage += 2;
        g->add_msg("Your %s is damaged and pushed off!",
                   worn[i].tname(g).c_str());
        g->m.add_item(posx, posy, worn[i]);
       }
       worn.erase(worn.begin() + i);
       i--;
      }
     }
     return;
    }
    break;
   case 19:
    if (!has_trait(PF_UNSTABLE)) {
     g->add_msg("You feel genetically unstable.");
     toggle_trait(PF_UNSTABLE);
     return;
    }
    break;
   case 20:
    if (!has_trait(PF_VOMITOUS)) {
     g->add_msg("Your stomach churns.");
     toggle_trait(PF_VOMITOUS);
     return;
    }
    break;
   case 21:
    if (!has_trait(PF_RADIOACTIVE) && one_in(2)) {
     g->add_msg("You feel radioactive.");
     toggle_trait(PF_RADIOACTIVE);
     return;
    }
    break;
   case 22:
    if (!has_trait(PF_SLIMY)) {
     g->add_msg("You start to ooze a thick green slime.");
     toggle_trait(PF_SLIMY);
     return;
    }
    break;
   case 23:
    if (!has_trait(PF_HERBIVORE) && !has_trait(PF_CARNIVORE)) {
     if (!has_trait(PF_VEGETARIAN))
      g->add_msg("The thought of eating meat suddenly disgusts you.");
     toggle_trait(PF_HERBIVORE);
     return;
    }
    break;
   case 24:
    if (!has_trait(PF_CARNIVORE) && !has_trait(PF_HERBIVORE)) {
     if (has_trait(PF_VEGETARIAN))
      g->add_msg("Despite your convictions, you crave meat.");
     else
      g->add_msg("You crave meat.");
     toggle_trait(PF_CARNIVORE);
     return;
    }
    break;
   }
  }
 }
}

int player::sight_range(int light_level)
{
 int ret = light_level;
 if (((is_wearing(itm_goggles_nv) && has_active_item(itm_UPS_on)) ||
     has_active_bionic(bio_night_vision)) &&
     ret < 12)
  ret = 12;
 if (has_trait(PF_NIGHTVISION) && ret == 1)
  ret = 2;
 if (has_trait(PF_NIGHTVISION2) && ret < 4)
  ret = 4;
 if (underwater && !has_bionic(bio_membrane) && !has_trait(PF_MEMBRANE) &&
     !is_wearing(itm_goggles_swim))
  ret = 1;
 if (has_disease(DI_BOOMERED))
  ret = 1;
 if (has_disease(DI_IN_PIT))
  ret = 1;
 if (has_disease(DI_BLIND))
  ret = 0;
 if (ret > 4 && has_trait(PF_MYOPIC) && !is_wearing(itm_glasses_eye))
  ret = 4;
 return ret;
}

bool player::has_two_arms()
{
 if (has_bionic(bio_blaster) || hp_cur[hp_arm_l] < 10 || hp_cur[hp_arm_r] < 10)
  return false;
 return true;
}

bool player::is_armed()
{
 return (weapon.type->id != 0);
}

bool player::unarmed_attack()
{
 return (weapon.type->id == 0 || weapon.type->id == itm_bio_claws);
}

bool player::avoid_trap(trap* tr)
{
 int myroll = dice(3, dex_cur + sklevel[sk_dodge] * 1.5);
 int traproll;
 if (per_cur - encumb(bp_eyes) >= tr->visibility)
  traproll = dice(3, tr->avoidance);
 else
  traproll = dice(6, tr->avoidance);
 if (has_trait(PF_LIGHTSTEP))
  myroll += dice(2, 6);
 if (myroll >= traproll)
  return true;
 return false;
}

void player::pause()
{
 moves = 0;
 if (recoil > 0) {
  if (str_cur + 2 * sklevel[sk_gun] >= recoil)
   recoil = 0;
  else {
   recoil -= str_cur + 2 * sklevel[sk_gun];
   recoil = int(recoil / 2);
  }
 }
}

int player::hit_roll()
{
 int numdice = base_to_hit() + weapon.type->m_to_hit;
 bool unarmed = false, bashing = false, cutting = false;
// Are we unarmed?
 if (unarmed_attack()) {
  unarmed = true;
  numdice += sklevel[sk_unarmed];
  if (sklevel[sk_unarmed] > 4)
   numdice += sklevel[sk_unarmed] - 2;	// Extra bonus for high levels
  if (has_trait(PF_DRUNKEN) && has_disease(DI_DRUNK))
   numdice += rng(0, 1) + int(disease_level(DI_DRUNK) / 300);
 } else if (has_trait(PF_DRUNKEN) && has_disease(DI_DRUNK))
  numdice += int(disease_level(DI_DRUNK) / 400);
// Using a bashing weapon?
 if (weapon.is_bashing_weapon()) {
  bashing = true;
  numdice += int(sklevel[sk_bashing] / 3);
 }
// Using a cutting weapon?
 if (weapon.is_cutting_weapon()) {
  cutting = true;
  numdice += int(sklevel[sk_cutting] / 2);
 }
// Using a spear?
 if (weapon.has_weapon_flag(WF_SPEAR) || weapon.has_weapon_flag(WF_STAB))
  numdice += int(sklevel[sk_stabbing]);

 int sides = 10;
 if (numdice < 1) {
  numdice = 1;
  sides = 8;
 }
 sides -= encumb(bp_torso);
 if (sides < 2)
  sides = 2;

 return dice(numdice, sides);
}

int player::hit_mon(game *g, monster *z)
{
 bool is_u = (this == &(g->u));	// Affects how we'll display messages
 int j;
 bool can_see = (is_u || g->u_see(posx, posy, j));
 std::string You  = (is_u ? "You"  : name);
 std::string Your = (is_u ? "Your" : name + "'s");
 std::string your = (is_u ? "your" : (male ? "his" : "her"));

// Types of combat (may overlap!)
 bool unarmed  = unarmed_attack(), bashing = weapon.is_bashing_weapon(),
      cutting  = weapon.is_cutting_weapon(),
      stabbing = (weapon.has_weapon_flag(WF_SPEAR) ||
                  weapon.has_weapon_flag(WF_STAB));

// Recoil penalty
 if (recoil <= 30)
  recoil += 6;
// Movement cost
 moves -= weapon.attack_time() + 20 * encumb(bp_torso);
// Different sizes affect your chance to hit
 if (hit_roll() < z->dodge_roll()) {// A miss!
// Movement penalty for missing & stumbling
  int stumble_pen = 2 * weapon.volume() + weapon.weight();
  if (has_trait(PF_DEFT))
   stumble_pen = int(stumble_pen * .3) - 10;
  if (stumble_pen < 0)
   stumble_pen = 0;
  if (stumble_pen > 0 && (one_in(16 - str_cur) || one_in(22 - dex_cur)))
   stumble_pen = rng(0, stumble_pen);
  if (is_u) {	// Only display messages if this is the player
   if (stumble_pen >= 60)
    g->add_msg("You miss and stumble with the momentum.");
   else if (stumble_pen >= 10)
    g->add_msg("You swing wildly and miss.");
   else
    g->add_msg("You miss.");
  }
  moves -= stumble_pen;
  return 0;
 }
 if (z->has_flag(MF_SHOCK) && !wearing_something_on(bp_hands) &&
     (unarmed || weapon.conductive())) {
  if (is_u)
   g->add_msg("The %s's electric body shocks you!", z->name().c_str());
  hurtall(rng(1, 3));
 }
// For very high hit rolls, we crit!
 bool critical_hit = (hit_roll() >= 50 + 10 * z->dodge_roll());
 int dam = base_damage(true);
 int cutting_penalty = 0; // Moves lost from getting a cutting weapon stuck

// Drunken Master bonuses
 if (has_trait(PF_DRUNKEN) && has_disease(DI_DRUNK)) {
// Remember, a single drink gives 600 levels of DI_DRUNK
  if (unarmed)
   dam += disease_level(DI_DRUNK) / 250;
  else
   dam += disease_level(DI_DRUNK) / 400;
 }

 if (unarmed) { // Unarmed bonuses
  dam += rng(0, sklevel[sk_unarmed]);
  if (has_trait(PF_TALONS) && z->type->armor - sklevel[sk_unarmed] < 10) {
   int z_armor = (z->type->armor - sklevel[sk_unarmed]);
   if (z_armor < 0)
    z_armor = 0;
   dam += 10 - z_armor;
  }
 } else if (rng(1, 45 - dex_cur) < 2 * sklevel[sk_unarmed] &&
            rng(1, 65 - dex_cur) < 2 * sklevel[sk_unarmed]   ) {
// If we're not unarmed, there's still a possibility of getting in a bonus
// unarmed attack.
  if (is_u || can_see) {
   switch (rng(1, 4)) {
    case 1: g->add_msg("%s kick%s the %s!", You.c_str(), (is_u ? "" : "s"),
                       z->name().c_str()); break;
    case 2: g->add_msg("%s headbutt%s the %s!", You.c_str(), (is_u ? "" : "s"),
                       z->name().c_str()); break;
    case 3: g->add_msg("%s elbow%s the %s!", You.c_str(), (is_u ? "" : "s"),
                       z->name().c_str()); break;
    case 4: g->add_msg("%s knee%s the %s!", You.c_str(), (is_u ? "" : "s"),
                       z->name().c_str()); break;
   }
  }
  dam += rng(1, sklevel[sk_unarmed]);
  practice(sk_unarmed, 2);
 }
// Melee skill bonus
 dam += rng(0, sklevel[sk_melee]);
// Bashing damage bonus
 int bash_dam = weapon.type->melee_dam,
     bash_cap = 5 + str_cur + sklevel[sk_bashing];
 if (bash_dam > bash_cap)// Cap for weak characters
  bash_dam = (bash_cap * 3 + bash_dam) / 4;
 if (bashing)
  bash_dam += rng(0, sklevel[sk_bashing]) * sqrt(str_cur);
 int bash_min = bash_dam / 4;
 if (bash_min < sklevel[sk_bashing] * 2)
  bash_min = sklevel[sk_bashing] * 2;
 dam += rng(bash_dam / 4, bash_dam);
// Take some moves away from the target; at this point it's skill & bash damage
 z->moves -= rng(0, dam * 2);
// Spears treat cutting damage specially.
 if (weapon.has_weapon_flag(WF_SPEAR) &&
     weapon.type->melee_cut > z->type->armor - int(sklevel[sk_stabbing])) {
  int z_armor = z->type->armor - int(sklevel[sk_stabbing]);
  dam += int(weapon.type->melee_cut / 5);
  int minstab = sklevel[sk_stabbing] *  8 + weapon.type->melee_cut * 2,
      maxstab = sklevel[sk_stabbing] * 20 + weapon.type->melee_cut * 4;
  int monster_penalty = rng(minstab, maxstab);
  if (monster_penalty >= 150)
   g->add_msg("You force the %s to the ground!", z->name().c_str());
  else if (monster_penalty >= 80)
   g->add_msg("The %s is skewered and flinches!", z->name().c_str());
  z->moves -= monster_penalty;
  cutting_penalty = weapon.type->melee_cut * 4 + z_armor * 8 -
                    dice(sklevel[sk_stabbing], 10);
  practice(sk_stabbing, 5);
// Cutting damage bonus
 } else if (weapon.type->melee_cut >
            z->type->armor - int(sklevel[sk_cutting] / 2)) {
  int z_armor = z->type->armor - int(sklevel[sk_cutting] / 2);
  if (z_armor < 0)
   z_armor = 0;
  dam += weapon.type->melee_cut - z_armor;
  cutting_penalty = weapon.type->melee_cut * 3 + z_armor * 8 -
                    dice(sklevel[sk_cutting], 10);
 }
 if (weapon.has_weapon_flag(WF_MESSY)) { // e.g. chainsaws
  cutting_penalty /= 6; // Harder to get stuck
  for (int x = z->posx - 1; x <= z->posx + 1; x++) {
   for (int y = z->posy - 1; y <= z->posy + 1; y++) {
    if (!one_in(3)) {
     if (g->m.field_at(x, y).type == fd_blood &&
         g->m.field_at(x, y).density < 3)
      g->m.field_at(x, y).density++;
     else
      g->m.add_field(g, x, y, fd_blood, 1);
    }
   }
  }
 }

// Bonus attacks!

 bool shock_them = (!z->has_flag(MF_SHOCK) && has_bionic(bio_shock) &&
                    power_level >= 2 && unarmed && one_in(3));
 bool drain_them = (has_bionic(bio_heat_absorb) && power_level >= 1 &&
                    !is_armed() && z->has_flag(MF_WARM));
 bool  bite_them = (has_trait(PF_FANGS) && z->armor() < 18 &&
                    one_in(20 - dex_cur - sklevel[sk_unarmed]));
 bool  peck_them = (has_trait(PF_BEAK)  && z->armor() < 16 &&
                    one_in(15 - dex_cur - sklevel[sk_unarmed]));
 if (drain_them)
  power_level--;
 drain_them &= one_in(2);	// Only works half the time

// Critical hit effects
 if (critical_hit) {
  bool headshot = (!z->has_flag(MF_NOHEAD) && !one_in(3));
// Second chance for shock_them, drain_them, bite_them and peck_them
  shock_them = (shock_them || (!z->has_flag(MF_SHOCK) && has_bionic(bio_shock)&&
                               power_level >= 2 && unarmed && !one_in(3)));
  drain_them = (drain_them || (has_bionic(bio_heat_absorb) && !is_armed() &&
                               power_level >= 1 && z->has_flag(MF_WARM) &&
                               !one_in(3)));
  bite_them  = ( bite_them || (has_trait(PF_FANGS) && z->armor() < 18 &&
                               one_in(5)));
  peck_them  = ( peck_them || (has_trait(PF_BEAK)  && z->armor() < 16 &&
                               one_in(4)));

  if (weapon.has_weapon_flag(WF_SPEAR) || weapon.has_weapon_flag(WF_STAB)) {
   dam += weapon.type->melee_cut;
   dam += rng(1, 10) * sklevel[sk_stabbing];
   practice(sk_stabbing, 5);
  }

  if (unarmed) {
   dam += rng(2, 6) * sklevel[sk_unarmed];
   if (sklevel[sk_unarmed] > 5)
    dam += 4 * (sklevel[sk_unarmed - 3]);
   z->moves -= dam;	// Stunning blow
   if (weapon.type->id == itm_bio_claws) {
    if (sklevel[sk_cutting] >= 3)
     dam += 5;
    headshot &= z->hp < dam && one_in(2);
    if (headshot && can_see)
     g->add_msg("%s claws pierce the %s's skull!", Your.c_str(),
                z->name().c_str());
    else if (can_see)
     g->add_msg("%s claws stab straight through the %s!", Your.c_str(),
                z->name().c_str());
   } else if (has_trait(PF_TALONS)) {
    dam += 2;
    headshot &= z->hp < dam && one_in(2);
    if (headshot && can_see)
     g->add_msg("%s talons tear the %s's head open!", Your.c_str(),
                z->name().c_str());
    else if (can_see)
     g->add_msg("%s bur%s %s talons into the %s!", You.c_str(),(is_u?"y":"ies"),
                your.c_str(), z->name().c_str());
   } else {
    headshot &= z->hp < dam && one_in(2);
    if (headshot && can_see)
     g->add_msg("%s crush%s the %s's skull in a single blow!", 
                You.c_str(), (is_u ? "" : "es"), z->name().c_str());
    else if (can_see)
     g->add_msg("%s deliver%s a crushing punch!",You.c_str(),(is_u ? "" : "s"));
   }
   if (z->hp > 0 && rng(1, 5) < sklevel[sk_unarmed])
    z->add_effect(ME_STUNNED, 1 + sklevel[sk_unarmed]);
  } else {	// Not unarmed
   if (bashing) {
    dam += 2 * sklevel[sk_bashing];
    dam += 8 + (str_cur / 2);
    z->add_effect(ME_STUNNED, int(dam / 20) + sklevel[sk_bashing]);
    z->moves -= rng(dam, dam * 2);	// Stunning blow
   }
   if (cutting) {
    dam += 3 * sklevel[sk_cutting];
    headshot &= z->hp < dam;
    if (headshot && can_see)
     g->add_msg("%s %s slices the %s's head off!", Your.c_str(),
                weapon.tname(g).c_str(), z->name().c_str());
    else if (can_see)
     g->add_msg("%s stab %s %s through the %s!", You.c_str(), your.c_str(),
                weapon.tname(g).c_str(), z->name().c_str());
   } else {	// Not cutting, probably bashing
    headshot &= z->hp < dam;
    if (headshot && can_see)
     g->add_msg("%s crush%s the %s's skull!", You.c_str(), (is_u ? "" : "es"),
                z->name().c_str());
    else if (can_see)
     g->add_msg("%s crush%s the %s's body!", You.c_str(), (is_u ? "" : "es"),
                z->name().c_str());
   }
  }	// End of not-unarmed
 }	// End of critical hit

 if (shock_them) {
  power_level -= 2;
  if (can_see)
   g->add_msg("%s shock%s the %s!", You.c_str(), (is_u ? "" : "s"),
              z->name().c_str());
  int shock = rng(2, 5);
  dam += shock * rng(1, 3);
  z->moves -= shock * 180;
 }
 if (drain_them) {
  charge_power(rng(0, 4));
  if (can_see)
   g->add_msg("%s drain%s the %s's body heat!", You.c_str(), (is_u ? "" : "s"),
              z->name().c_str());
  dam += rng(4, 10);
  z->moves -= rng(80, 120);
 }
 if (bite_them) {
  if (can_see)
   g->add_msg("%s sink %s fangs into the %s!", You.c_str(), your.c_str(),
              z->name().c_str());
  dam += 18 - z->armor();
 }
 if (peck_them) {
  if (can_see)
   g->add_msg("%s peck%s the %s viciously!", You.c_str(), (is_u ? "" : "s"),
              z->name().c_str());
  dam += 16 - z->armor();
 }

// Make a rather quiet sound, to alert any nearby monsters
 g->sound(posx, posy, 6, "");

// Glass weapons shatter sometimes
 if (weapon.made_of(GLASS) &&
     rng(0, weapon.volume() + 8) < weapon.volume() + str_cur) {
  if (can_see)
   g->add_msg("%s %s shatters!", Your.c_str(), weapon.tname(g).c_str());
  g->sound(posx, posy, 16, "");
// Dump its contents on the ground
  for (int i = 0; i < weapon.contents.size(); i++)
   g->m.add_item(posx, posy, weapon.contents[i]);
  hit(g, bp_arms, 1, 0, rng(0, weapon.volume() * 2));// Take damage
  if (weapon.is_two_handed(this))// Hurt left arm too, if it was big
   hit(g, bp_arms, 0, 0, rng(0, weapon.volume()));
  dam += rng(0, 5 + int(weapon.volume() * 1.5));// Hurt the monster extra
  remove_weapon();
 }

 if (dam <= 0) {
  if (is_u)
   g->add_msg("You hit the %s, but do no damage.", z->name().c_str());
  else if (can_see)
   g->add_msg("%s's %s hits the %s, but does no damage.", You.c_str(),
              weapon.tname(g).c_str(), z->name().c_str());
  practice(sk_melee, rng(2, 5));
  if (unarmed)
   practice(sk_unarmed, 2);
  if (bashing)
   practice(sk_bashing, 2);
  if (cutting)
   practice(sk_cutting, 3);
  return 0;
 }
 if (is_u)
  g->add_msg("You hit the %s for %d damage.", z->name().c_str(), dam);
 else if (can_see)
  g->add_msg("%s hits the %s with %s %s.", You.c_str(), z->name().c_str(),
             (male ? "his" : "her"),
             (weapon.type->id == 0 ? "fists" : weapon.tname(g).c_str()));
 practice(sk_melee, rng(5, 10));
 if (unarmed)
  practice(sk_unarmed, rng(5, 10));
 if (bashing)
  practice(sk_bashing, rng(5, 10));
 if (cutting)
  practice(sk_cutting, rng(5, 10));

// Penalize the player if their cutting weapon got stuck
 if (!unarmed && dam < z->hp && cutting_penalty > dice(str_cur, 20)) {
  if (is_u)
   g->add_msg("Your %s gets stuck in the %s, pulling it out of your hands!",
              weapon.tname().c_str(), z->type->name.c_str());
  z->add_item(remove_weapon());
  if (weapon.has_weapon_flag(WF_SPEAR) || weapon.has_weapon_flag(WF_STAB))
   z->speed *= .7;
  else
   z->speed *= .85;
 } else {
  if (dam >= z->hp) {
   cutting_penalty /= 2;
   cutting_penalty -= rng(sklevel[sk_cutting], sklevel[sk_cutting] * 2 + 2);
  }
  if (cutting_penalty > 0)
   moves -= cutting_penalty;
  if (cutting_penalty >= 50 && is_u)
   g->add_msg("Your %s gets stuck in the %s, but you yank it free.",
              weapon.tname().c_str(), z->type->name.c_str());
  if (weapon.has_weapon_flag(WF_SPEAR) || weapon.has_weapon_flag(WF_STAB))
   z->speed *= .9;
 }

 return dam;
}

bool player::hit_player(player &p, body_part &bp, int &hitdam, int &hitcut)
{
// TODO: Add bionics and other bonus (e.g. heat drain, shock, etc)
 if (!is_npc() && p.is_npc()) {
  npc *foe = dynamic_cast<npc*>(&p);
  foe->make_angry();
 }
 bool unarmed = unarmed_attack(), bashing = weapon.is_bashing_weapon(),
      cutting = weapon.is_cutting_weapon();
 int hitit = hit_roll() - p.dodge_roll();
 if (hitit < 0) {	// They dodged
  practice(sk_melee, rng(2, 4));
  if (unarmed)
   practice(sk_unarmed, 3);
  if (bashing)
   practice(sk_bashing, 1);
  if (cutting)
   practice(sk_cutting, 2);
  return false;
 }

 if (hitit >= 15)
  bp = bp_eyes;
 else if (hitit >= 12)
  bp = bp_mouth;
 else if (hitit >= 10)
  bp = bp_head;
 else if (hitit >= 6)
  bp = bp_torso;
 else if (hitit >= 2)
  bp = bp_arms;
 else
  bp = bp_legs;
 
 hitdam = base_damage();

 if (unarmed) {// Unarmed bonuses
  hitdam += rng(0, sklevel[sk_unarmed]);
  if (sklevel[sk_unarmed] >= 5)
   hitdam += rng(sklevel[sk_unarmed], 3 * sklevel[sk_unarmed]);
  if (has_trait(PF_TALONS))
   hitcut += 10;
  if (sklevel[sk_unarmed] >= 8 &&
      (one_in(3) || rng(5, 20) < sklevel[sk_unarmed]))
   hitdam *= rng(2, 3);
 }
// Weapon adds (melee_dam / 4) to (melee_dam)
 hitdam += rng(weapon.type->melee_dam / 4, weapon.type->melee_dam);
 if (bashing)
  hitdam += rng(0, sklevel[sk_bashing]) * sqrt(str_cur);

 hitdam += int(pow(1.5, sklevel[sk_melee]));
 hitcut = weapon.type->melee_cut;
 if (hitcut > 0)
  hitcut += int(sklevel[sk_cutting] / 3);
 if (hitdam < 0) hitdam = 0;
 if (hitdam > 0 || hitcut > 0) { // Practicing
  practice(sk_melee, rng(5, 10));
  if (unarmed)
   practice(sk_unarmed, rng(5, 10));
  if (bashing)
   practice(sk_bashing, rng(5, 10));
  if (cutting)
   practice(sk_cutting, rng(5, 10));
 } else { // Less practice if we missed
  practice(sk_melee, rng(2, 5));
  if (unarmed)
   practice(sk_unarmed, 2);
  if (bashing)
   practice(sk_bashing, 2);
  if (cutting)
   practice(sk_cutting, 3);
 }
 return true;
}

int player::dodge()
{
// If this function is called, it should be assumed that we're exercising
//  the dodge skill.
 if (has_disease(DI_SLEEP) || has_disease(DI_LYING_DOWN))
  return 0;
 practice(sk_dodge, 5);
 int ret = 4 + (dex_cur / 2);
 ret += sklevel[sk_dodge];
 ret -= (encumb(bp_legs) / 2) + encumb(bp_torso);
 ret += int(current_speed() / 150);
 if (str_max >= 16)
  ret--; // Penalty if we're hyuuge
 else if (str_max <= 5)
  ret++; // Bonus if we're small
 if (!can_dodge) // We already dodged this turn
  ret = rng(0, ret);
 can_dodge = false;
 return ret;
}

int player::dodge_roll()
{
 return dice(dodge(), 6);
}

int player::throw_range(int index)
{
 item tmp;
 if (index == -1)
  tmp = weapon;
 else if (index == -2)
  return -1;
 else
  tmp = inv[index];

 if (tmp.weight() > int(str_cur * 15))
  return 0;
 int ret = int((str_cur * 8) / (tmp.weight() > 0 ? tmp.weight() : 10));
 ret -= int(tmp.volume() / 10);
 if (ret < 1)
  return 1;
// Cap at double our strength
 if (ret > str_cur * 2)
  return str_cur * 2;
 return ret;
}
 
int player::base_damage(bool real_life)
{
 int str = (real_life ? str_cur : str_max);
 int dam = (real_life ? rng(0, str / 2) : str / 2);
// Bonus for strong characters
 if (str > 10)
  dam += int((str - 9) / 2);
// Big bonus for super-human characters
 if (str > 20)
  dam += int((str - 20) * 1.5);

 return dam;
}

int player::base_to_hit(bool real_life)
{
 int dex = (real_life ? dex_cur : dex_max);
 return 1 + int(dex / 2) + sklevel[sk_melee];
}

int player::ranged_dex_mod(bool real_life)
{
 int dex = (real_life ? dex_cur : dex_max);
 if (dex == 8)
  return 0;
 if (dex > 8)
  return (real_life ? (0 - rng(0, dex - 8)) : (8 - dex));

 int deviation = 0;
 if (dex < 6)
  deviation = 2 * (8 - dex);
 else
  deviation = 1.5 * (8 - dex);

 return (real_life ? rng(0, deviation) : deviation);
}

int player::ranged_per_mod(bool real_life)
{
 int per = (real_life ? per_cur : per_max);
 if (per == 8)
  return 0;
 int deviation = 0;

 if (per < 6) {
  deviation = 2.5 * (8 - per);
  if (real_life)
   deviation = rng(0, deviation);
 } else if (per < 8) {
  deviation = 2 * (8 - per);
  if (real_life)
   deviation = rng(0, deviation);
 } else {
  deviation = 3 * (0 - (per > 16 ? 8 : per - 8));
  if (real_life && one_in(per))
   deviation = 0 - rng(0, abs(deviation));
 }
 return deviation;
}

int player::throw_dex_mod(bool real_life)
{
 int dex = (real_life ? dex_cur : dex_max);
 if (dex == 8 || dex == 9)
  return 0;
 if (dex >= 10)
  return (real_life ? 0 - rng(0, dex - 9) : 9 - dex);
 
 int deviation = 0;
 if (dex < 6)
  deviation = 3 * (8 - dex);
 else
  deviation = 2 * (8 - dex);

 return (real_life ? rng(0, deviation) : deviation);
}

int player::comprehension_percent(skill s, bool real_life)
{
 double intel = (double)(real_life ? int_cur : int_max);
 double percent = 80.; // double temporarily, since we divide a lot
 int learned = (real_life ? sklevel[s] : 4);
 if (learned > intel / 2)
  percent /= 1 + ((learned - intel / 2) / (intel / 3));
 else if (!real_life && intel > 8)
  percent += 125 - 1000 / intel;

 if (has_trait(PF_FASTLEARNER))
  percent = (100 + percent) / 2;
 return (int)(percent);
}

int player::read_speed(bool real_life)
{
 int intel = (real_life ? int_cur : int_max);
 int ret = 1000 - 50 * (intel - 8);
 if (has_trait(PF_FASTREADER))
  ret /= 3;
 if (ret < 100)
  ret = 100;
 return (real_life ? ret : ret / 10);
}

int player::convince_score()
{
 return int_cur + sklevel[sk_speech] * 3;
}

void player::hit(game *g, body_part bphurt, int side, int dam, int cut)
{
 int painadd = 0;
 if (has_disease(DI_SLEEP)) {
  g->add_msg("You wake up!");
  rem_disease(DI_SLEEP);
 } else if (has_disease(DI_LYING_DOWN))
  rem_disease(DI_LYING_DOWN);

 absorb(g, bphurt, dam, cut);

 dam += cut;
 if (dam <= 0)
  return;

 if (has_trait(PF_PAINRESIST))
  painadd = (sqrt(cut) + dam + cut) / (rng(4, 6) + rng(0, 4));
 else
  painadd = (sqrt(cut) + dam + cut) / 4;
 pain += painadd;

 switch (bphurt) {
 case bp_eyes:
  pain++;
  if (dam > 5 || cut > 0) {
   int minblind = int((dam + cut) / 10);
   if (minblind < 1)
    minblind = 1;
   int maxblind = int((dam + cut) /  4);
   if (maxblind > 5)
    maxblind = 5;
   add_disease(DI_BLIND, rng(minblind, maxblind), g);
  }
// Fall through to head damage
 case bp_mouth:
// Fall through to head damage
 case bp_head:
  pain++;
  hp_cur[hp_head] -= dam;
  if (hp_cur[hp_head] < 0)
   hp_cur[hp_head] = 0;
 break;
 case bp_torso:
  recoil += int(dam / 5);
  hp_cur[hp_torso] -= dam;
  if (hp_cur[hp_torso] < 0)
   hp_cur[hp_torso] = 0;
 break;
 case bp_hands:
// Fall through to arms
 case bp_arms:
  if (side == 1 || side == 3 || weapon.is_two_handed(this))
   recoil += int(dam / 3);
  if (side == 0 || side == 3) {
   hp_cur[hp_arm_l] -= dam;
   if (hp_cur[hp_arm_l] < 0)
    hp_cur[hp_arm_l] = 0;
  }
  if (side == 1 || side == 3) {
   hp_cur[hp_arm_r] -= dam;
   if (hp_cur[hp_arm_r] < 0)
    hp_cur[hp_arm_r] = 0;
  }
 break;
 case bp_feet:
// Fall through to legs
 case bp_legs:
  if (side == 0 || side == 3) {
   hp_cur[hp_leg_l] -= dam;
   if (hp_cur[hp_leg_l] < 0)
    hp_cur[hp_leg_l] = 0;
  }
  if (side == 1 || side == 3) {
   hp_cur[hp_leg_r] -= dam;
   if (hp_cur[hp_leg_r] < 0)
    hp_cur[hp_leg_r] = 0;
  }
 break;
 default:
  debugmsg("Wacky body part hit!");
 }
 if (has_trait(PF_ADRENALINE) && !has_disease(DI_ADRENALINE) &&
     (hp_cur[hp_head] < 25 || hp_cur[hp_torso] < 15))
  add_disease(DI_ADRENALINE, 200, g);
}

void player::hurt(game *g, body_part bphurt, int side, int dam)
{
 int painadd = 0;
 if (has_disease(DI_SLEEP) && rng(0, dam) > 2) {
  g->add_msg("You wake up!");
  rem_disease(DI_SLEEP);
 } else if (has_disease(DI_LYING_DOWN))
  rem_disease(DI_LYING_DOWN);

 if (dam <= 0)
  return;

 if (has_trait(PF_PAINRESIST))
  painadd = dam / 3;
 else
  painadd = dam / 2;
 pain += painadd;

 switch (bphurt) {
 case bp_eyes:	// Fall through to head damage
 case bp_mouth:	// Fall through to head damage
 case bp_head:
  pain++;
  hp_cur[hp_head] -= dam;
  if (hp_cur[hp_head] < 0)
   hp_cur[hp_head] = 0;
 break;
 case bp_torso:
  hp_cur[hp_torso] -= dam;
  if (hp_cur[hp_torso] < 0)
   hp_cur[hp_torso] = 0;
 break;
 case bp_hands:	// Fall through to arms
 case bp_arms:
  if (side == 0 || side == 3) {
   hp_cur[hp_arm_l] -= dam;
   if (hp_cur[hp_arm_l] < 0)
    hp_cur[hp_arm_l] = 0;
  }
  if (side == 1 || side == 3) {
   hp_cur[hp_arm_r] -= dam;
   if (hp_cur[hp_arm_r] < 0)
    hp_cur[hp_arm_r] = 0;
  }
 break;
 case bp_feet:	// Fall through to legs
 case bp_legs:
  if (side == 0 || side == 3) {
   hp_cur[hp_leg_l] -= dam;
   if (hp_cur[hp_leg_l] < 0)
    hp_cur[hp_leg_l] = 0;
  }
  if (side == 1 || side == 3) {
   hp_cur[hp_leg_r] -= dam;
   if (hp_cur[hp_leg_r] < 0)
    hp_cur[hp_leg_r] = 0;
  }
 break;
 default:
  debugmsg("Wacky body part hurt!");
 }
 if (has_trait(PF_ADRENALINE) && !has_disease(DI_ADRENALINE) &&
     (hp_cur[hp_head] < 25 || hp_cur[hp_torso] < 15))
  add_disease(DI_ADRENALINE, 200, g);
}

void player::heal(body_part healed, int side, int dam)
{
 hp_part healpart;
 switch (healed) {
 case bp_eyes:	// Fall through to head damage
 case bp_mouth:	// Fall through to head damage
 case bp_head:
  healpart = hp_head;
 break;
 case bp_torso:
  healpart = hp_torso;
 break;
 case bp_hands:
// Shouldn't happen, but fall through to arms
  debugmsg("Heal against hands!");
 case bp_arms:
  if (side == 0)
   healpart = hp_arm_l;
  else
   healpart = hp_arm_r;
 break;
 case bp_feet:
// Shouldn't happen, but fall through to legs
  debugmsg("Heal against feet!");
 case bp_legs:
  if (side == 0)
   healpart = hp_leg_l;
  else
   healpart = hp_leg_r;
 break;
 default:
  debugmsg("Wacky body part healed!");
  healpart = hp_torso;
 }
 hp_cur[healpart] += dam;
 if (hp_cur[healpart] > hp_max[healpart])
  hp_cur[healpart] = hp_max[healpart];
}

void player::heal(hp_part healed, int dam)
{
 hp_cur[healed] += dam;
 if (hp_cur[healed] > hp_max[healed])
  hp_cur[healed] = hp_max[healed];
}

void player::healall(int dam)
{
 for (int i = 0; i < num_hp_parts; i++) {
  hp_cur[i] += dam;
  if (hp_cur[i] > hp_max[i])
   hp_cur[i] = hp_max[i];
 }
}

void player::hurtall(int dam)
{
 for (int i = 0; i < num_hp_parts; i++) {
  int painadd = 0;
  hp_cur[i] -= dam;
   if (hp_cur[i] < 0)
     hp_cur[i] = 0;
  if (has_trait(PF_PAINRESIST))
   painadd = dam / 3;
  else
   painadd = dam / 2;
  pain += painadd;
 }
}

int player::hp_percentage()
{
 int total_cur = 0, total_max = 0;
// Head and torso HP are weighted 3x and 2x, respectively
 total_cur = hp_cur[hp_head] * 3 + hp_cur[hp_torso] * 2;
 total_max = hp_max[hp_head] * 3 + hp_max[hp_torso] * 2;
 for (int i = hp_arm_l; i < num_hp_parts; i++) {
  total_cur += hp_cur[i];
  total_max += hp_max[i];
 }
 return (100 * total_cur) / total_max;
}

void player::get_sick(game *g)
{
 if (one_in(3))
  health -= rng(0, 2);
 if (g->debugmon)
  debugmsg("Health: %d", health);
}

void player::infect(dis_type type, body_part vector, int strength,
                    int duration, game *g)
{
 if (dice(strength, 3) > dice(resist(vector), 3))
  add_disease(type, duration, g);
}

void player::add_disease(dis_type type, int duration, game *g)
{
 bool found = false;
 int i = 0;
 while ((i < illness.size()) && !found) {
  if (illness[i].type == type) {
   illness[i].duration += duration;
   found = true;
  }
  i++;
 }
 if (!found) {   
  if (!is_npc())
   dis_msg(g, type);
  disease tmp(type, duration);
  illness.push_back(tmp);
 }
// activity.type = ACT_NULL;
}

void player::rem_disease(dis_type type)
{
 for (int i = 0; i < illness.size(); i++) {
  if (illness[i].type == type)
   illness.erase(illness.begin() + i);
 }
}

bool player::has_disease(dis_type type)
{
 for (int i = 0; i < illness.size(); i++) {
  if (illness[i].type == type)
   return true;
 }
 return false;
}

int player::disease_level(dis_type type)
{
 for (int i = 0; i < illness.size(); i++) {
  if (illness[i].type == type)
   return illness[i].duration;
 }
 return 0;
}

void player::add_addiction(add_type type, int strength)
{
 if (type == ADD_NULL)
  return;
 int timer = 1200;
 if (has_trait(PF_ADDICTIVE)) {
  strength = int(strength * 1.5);
  timer = 800;
 }
 for (int i = 0; i < addictions.size(); i++) {
  if (addictions[i].type == type) {
        if (addictions[i].sated <   0)
    addictions[i].sated = timer;
   else if (addictions[i].sated < 600)
    addictions[i].sated += timer;	// TODO: Make this variable?
   else
    addictions[i].sated += int((3000 - addictions[i].sated) / 2);
   if ((rng(0, strength) > rng(0, addictions[i].intensity * 5) ||
       rng(0, 500) < strength) && addictions[i].intensity < 20)
    addictions[i].intensity++;
   return;
  }
 }
 if (rng(0, 100) < strength) {
  addiction tmp(type, 1);
  addictions.push_back(tmp);
 }
}

bool player::has_addiction(add_type type)
{
 for (int i = 0; i < addictions.size(); i++) {
  if (addictions[i].type == type)
   return true;
 }
 return false;
}

void player::rem_addiction(add_type type)
{
 for (int i = 0; i < addictions.size(); i++) {
  if (addictions[i].type == type) {
   addictions.erase(addictions.begin() + i);
   return;
  }
 }
}

int player::addiction_level(add_type type)
{
 for (int i = 0; i < addictions.size(); i++) {
  if (addictions[i].type == type)
   return addictions[i].intensity;
 }
 return 0;
}

void player::suffer(game *g)
{
 for (int i = 0; i < my_bionics.size(); i++) {
  if (my_bionics[i].powered)
   activate_bionic(i, g);
 }
 if (underwater) {
  if (!has_trait(PF_GILLS))
   oxygen--;
  if (oxygen <= 5)
   g->add_msg("You're almost out of air!  Press '<' to surface.");
  else if (oxygen < 0) {
   if (has_bionic(bio_gills) && power_level > 0) {
    oxygen += 5;
    power_level--;
   } else {
    g->add_msg("You're drowning!");
    hurt(g, bp_torso, 0, rng(2, 5));
   }
  }
 }
 for (int i = 0; i < illness.size(); i++) {
  dis_effect(g, *this, illness[i]);
  illness[i].duration--;
  if (illness[i].duration < -43200)	// Cap permanent disease age @ 3 days
   illness[i].duration = -43200;
  if (illness[i].duration == 0) {
   illness.erase(illness.begin() + i);
   i--;
  }
 }
 if (!has_disease(DI_SLEEP)) {
  int timer = -3600;
  if (has_trait(PF_ADDICTIVE))
   timer = -4000;
  for (int i = 0; i < addictions.size(); i++) {
   if (addictions[i].sated <= 0)
    addict_effect(g, addictions[i]);
   addictions[i].sated--;
   if (!one_in(addictions[i].intensity - 2) && addictions[i].sated > 0)
    addictions[i].sated -= 1;
   if (addictions[i].sated < timer - (100 * addictions[i].intensity)) {
    if (addictions[i].intensity <= 2) {
     addictions.erase(addictions.begin() + i);
     i--;
    } else {
     addictions[i].intensity = int(addictions[i].intensity / 2);
     addictions[i].intensity--;
     addictions[i].sated = 0;
    }
   }
  }
  if (has_trait(PF_CHEMIMBALANCE)) {
   if (one_in(3600)) {
    g->add_msg("You suddenly feel sharp pain for no reason.");
    pain += 3 * rng(1, 3);
   }
   if (one_in(3600)) {
    int pkilladd = 5 * rng(-1, 2);
    if (pkilladd > 0)
     g->add_msg("You suddenly feel numb.");
    else if (pkilladd < 0)
     g->add_msg("You suddenly ache.");
    pkill += pkilladd;
   }
   if (one_in(3600)) {
    g->add_msg("You feel dizzy for a moment.");
    moves -= rng(10, 30);
   }
   if (one_in(3600)) {
    int hungadd = 5 * rng(-1, 3);
    if (hungadd > 0)
     g->add_msg("You suddenly feel hungry.");
    else
     g->add_msg("You suddenly feel a little full.");
    hunger += hungadd;
   }
   if (one_in(3600)) {
    g->add_msg("You suddenly feel thirsty.");
    thirst += 5 * rng(1, 3);
   }
   if (one_in(3600)) {
    g->add_msg("You feel fatigued all of a sudden.");
    fatigue += 10 * rng(2, 4);
   }
   if (one_in(4800)) {
    if (one_in(3))
     add_morale(MORALE_FEELING_GOOD, 20, 100);
    else
     add_morale(MORALE_FEELING_BAD, -20, -100);
   }
  }
  if (has_trait(PF_SCHIZOPHRENIC) && one_in(2400)) { // Every 4 hours or so
   monster phantasm;
   int i;
   switch(rng(0, 11)) {
    case 0:
     add_disease(DI_HALLU, 3600, g);
     break;
    case 1:
     add_disease(DI_VISUALS, rng(15, 60), g);
     break;
    case 2:
     g->add_msg("From the south you hear glass breaking.");
     break;
    case 3:
     g->add_msg("YOU SHOULD QUIT THE GAME IMMEDIATELY.");
     add_morale(MORALE_FEELING_BAD, -50, -150);
     break;
    case 4:
     for (i = 0; i < 10; i++) {
      g->add_msg("XXXXXXXXXXXXXXXXXXXXXXXXXXX");
     }
     break;
    case 5:
     g->add_msg("You suddenly feel so numb...");
     pkill += 25;
     break;
    case 6:
     g->add_msg("You start to shake uncontrollably.");
     add_disease(DI_SHAKES, 10 * rng(2, 5), g);
     break;
    case 7:
     for (i = 0; i < 10; i++) {
      phantasm = monster(g->mtypes[mon_hallu_zom + rng(0, 3)]);
      phantasm.spawn(posx + rng(-10, 10), posy + rng(-10, 10));
      if (g->mon_at(phantasm.posx, phantasm.posy) == -1)
       g->z.push_back(phantasm);
     }
     break;
    case 8:
     g->add_msg("It's a good time to lie down and sleep.");
     add_disease(DI_LYING_DOWN, 200, g);
     break;
    case 9:
     g->add_msg("You have the sudden urge to SCREAM!");
     g->sound(posx, posy, 10 + 2 * str_cur, "AHHHHHHH!");
     break;
    case 10:
     g->add_msg(std::string(name + name + name + name + name + name + name +
                            name + name + name + name + name + name + name +
                            name + name + name + name + name + name).c_str());
     break;
    case 11:
     add_disease(DI_FORMICATION, 600, g);
     break;
   }
  }
  if (has_trait(PF_JITTERY) && !has_disease(DI_SHAKES)) {
   if (stim > 0 && one_in(300 - 10 * stim))
    add_disease(DI_SHAKES, 300 + 10 * stim, g);
   else if (hunger > 50 && one_in(400 - hunger))
    add_disease(DI_SHAKES, 400, g);
  }
  if (has_trait(PF_MOODSWINGS) && one_in(3600)) {
   if (rng(1, 20) > 9)	// 55% chance
    add_morale(MORALE_MOODSWING, -100, -500);
   else			// 45% chance
    add_morale(MORALE_MOODSWING, 100, 500);
  }
  if (has_trait(PF_VOMITOUS) && (one_in(4200) ||
                                 (has_trait(PF_WEAKSTOMACH) && one_in(4200))))
   vomit(g);
 }	// Done with while-awake-only effects
 if (has_trait(PF_ASTHMA) && one_in(3600 - stim * 50)) {
  bool auto_use = has_charges(itm_inhaler, 1);
  if (underwater) {
   oxygen = int(oxygen / 2);
   auto_use = false;
  }
  if (has_disease(DI_SLEEP)) {
   rem_disease(DI_SLEEP);
   g->add_msg("Your asthma wakes you up!");
   auto_use = false;
  }
  if (auto_use)
   use_charges(itm_inhaler, 1);
  else
   add_disease(DI_ASTHMA, 50 * rng(1, 4), g);
 }
 if (has_trait(PF_ALBINO) && g->is_in_sunlight(posx, posy) && one_in(20)) {
  g->add_msg("The sunlight burns your skin!");
  if (has_disease(DI_SLEEP)) {
   rem_disease(DI_SLEEP);
   g->add_msg("You wake up!");
  }
  hurtall(1);
 }
 if (has_trait(PF_TROGLO) && g->is_in_sunlight(posx, posy)) {
  str_cur -= 4;
  dex_cur -= 4;
  int_cur -= 4;
  per_cur -= 4;
 }
 if (has_trait(PF_SORES)) {
  for (int i = bp_head; i < num_bp; i++) {
   if (pain < 5 + 4 * abs(encumb(body_part(i))))
    pain = 5 + 4 * abs(encumb(body_part(i)));
  }
 }
 if (has_trait(PF_SLIMY)) {
  if (g->m.field_at(posx, posy).type == fd_null)
   g->m.add_field(g, posx, posy, fd_slime, 1);
  else if (g->m.field_at(posx, posy).type == fd_slime &&
           g->m.field_at(posx, posy).density < 3)
   g->m.field_at(posx, posy).density++;
 }
 if (has_trait(PF_RADIOGENIC) && g->turn % 50 == 0 && radiation >= 10) {
  radiation -= 10;
  healall(1);
 }
 if (has_trait(PF_RADIOACTIVE)) {
  if (g->m.radiation(posx, posy) < 20 && one_in(5))
   g->m.radiation(posx, posy)++;
 }
 if (has_trait(PF_UNSTABLE) && one_in(28800))	// Average once per 2 days
  mutate(g);
 radiation += rng(0, g->m.radiation(posx, posy) / 4);
 if (rng(1, 1000) < radiation && g->turn % 600 == 0) {
  mutate(g);
  if (radiation > 2000)
   radiation = 2000;
  radiation /= 2;
  radiation -= 5;
  if (radiation < 0)
   radiation = 0;
 }

// Negative bionics effects
 if (has_bionic(bio_dis_shock) && one_in(1200)) {
  g->add_msg("You suffer a painful electrical discharge!");
  pain++;
  moves -= 150;
 }
 if (has_bionic(bio_dis_acid) && one_in(1500)) {
  g->add_msg("You suffer a buurning acided discharge!");
  hurtall(1);
 }
 if (has_bionic(bio_drain) && power_level > 0 && one_in(600)) {
  g->add_msg("Your batteries discharge slightly.");
  power_level--;
 }
 if (has_bionic(bio_noise) && one_in(500)) {
  g->add_msg("A bionic emits a crackle of noise!");
  g->sound(posx, posy, 60, "");
 }
 if (has_bionic(bio_power_weakness) && max_power_level > 0 &&
     power_level >= max_power_level * .75)
  str_cur -= 3;

 if (dex_cur < 0)
  dex_cur = 0;
 if (str_cur < 0)
  str_cur = 0;
 if (per_cur < 0)
  per_cur = 0;
 if (int_cur < 0)
  int_cur = 0;
}

void player::vomit(game *g)
{
 g->add_msg("You throw up heavily!");
 hunger += 30;
 thirst += 30;
 for (int i = 0; i < illness.size(); i++) {
  if (illness[i].type == DI_FOODPOISON)
   illness[i].duration -= 600;
  else if (illness[i].type == DI_DRUNK)
   illness[i].duration -= rng(0, 5) * 100;
  if (illness[i].duration < 0)
   rem_disease(illness[i].type);
 }
 rem_disease(DI_PKILL1);
 rem_disease(DI_PKILL2);
 rem_disease(DI_PKILL3);
 if (has_disease(DI_SLEEP) && one_in(4)) { // Pulled a Hendrix!
  g->add_msg("You choke on your vomit and die...");
  hurtall(500);
 }
 rem_disease(DI_SLEEP);
}

int player::weight_carried()
{
 int ret = 0;
 ret += weapon.weight();
 for (int i = 0; i < worn.size(); i++)
  ret += worn[i].weight();
 for (int i = 0; i < inv.size(); i++)
  ret += inv[i].weight();
 return ret;
}

int player::volume_carried()
{
 int ret = 0;
 for (int i = 0; i < inv.size(); i++)
  ret += inv[i].volume();
 return ret;
}

int player::weight_capacity(bool real_life)
{
 int str = (real_life ? str_cur : str_max);
 int ret = 400 + str * 35;
 if (has_trait(PF_BADBACK))
  ret = int(ret * .65);
 return ret;
}

int player::volume_capacity()
{
 int ret = 2;	// A small bonus (the overflow)
 it_armor *armor;
 for (int i = 0; i < worn.size(); i++) {
  armor = dynamic_cast<it_armor*>(worn[i].type);
  ret += armor->storage;
 }
 if (has_bionic(bio_storage))
  ret += 6;
 if (has_trait(PF_PACKMULE))
  ret = int(ret * 1.4);
 return ret;
}

int player::morale_level()
{
 int ret = 0;
 for (int i = 0; i < morale.size(); i++)
  ret += morale[i].bonus;

 if (has_trait(PF_HOARDER)) {
  int pen = int((volume_capacity()-volume_carried()) / 2);
  if (pen > 70)
   pen = 70;
  if (has_disease(DI_TOOK_XANAX))
   pen = int(pen / 7);
  else if (has_disease(DI_TOOK_PROZAC))
   pen = int(pen / 2);
  ret -= pen;
 }

 if (has_trait(PF_MASOCHIST)) {
  int bonus = pain / 2.5;
  if (bonus > 25)
   bonus = 25;
  if (has_disease(DI_TOOK_PROZAC))
   bonus = int(bonus / 3);
  ret += bonus;
 }

 if (has_trait(PF_OPTIMISTIC)) {
  if (ret < 0) {	// Up to -15 is canceled out
   ret += 15;
   if (ret > 0)
    ret = 0;
  } else		// Otherwise, we're just extra-happy
   ret += 6;
 }

 if (has_disease(DI_TOOK_PROZAC) && ret < 0)
  ret = int(ret / 4);

 return ret;
}

void player::add_morale(morale_type type, int bonus, int max_bonus,
                        itype* item_type)
{
 bool placed = false;

 for (int i = 0; i < morale.size() && !placed; i++) {
  if (morale[i].type == type && morale[i].item_type == item_type) {
   placed = true;
   if (abs(morale[i].bonus) < abs(max_bonus) || max_bonus == 0) {
    morale[i].bonus += bonus;
    if (abs(morale[i].bonus) > abs(max_bonus) && max_bonus != 0)
     morale[i].bonus = max_bonus;
   }
  }
 }
 if (!placed) { // Didn't increase an existing point, so add a new one
  morale_point tmp(type, item_type, bonus);
  morale.push_back(tmp);
 }
}
 
void player::sort_inv()
{
 std::vector<item> types[8]; // guns ammo weaps armor food tools books other
 item tmp;
 for (int i = 0; i < inv.size(); i++) {
  tmp = inv[i];
       if (tmp.is_gun())
   types[0].push_back(tmp);
  else if (tmp.is_ammo())
   types[1].push_back(tmp);
  else if (tmp.is_armor())
   types[3].push_back(tmp);
  else if (tmp.is_tool() || tmp.is_gunmod())
   types[5].push_back(tmp);
  else if (tmp.is_food() || tmp.is_food_container())
   types[4].push_back(tmp);
  else if (tmp.is_book())
   types[6].push_back(tmp);
  else if (tmp.is_weap())
   types[2].push_back(tmp);
  else
   types[7].push_back(tmp);
 }
 inv.clear();
 for (int i = 0; i < 8; i++) {
  for (int j = 0; j < types[i].size(); j++)
   inv.push_back(types[i][j]);
 }
 inv_sorted = true;
}

void player::i_add(item it)
{
 if (it.is_food() || it.is_ammo() || it.is_gun()  || it.is_armor() || 
     it.is_book() || it.is_tool() || it.is_weap() || it.is_food_container())
  inv_sorted = false;
 if (it.is_ammo()) {	// Possibly combine with other ammo
  for (int i = 0; i < inv.size(); i++) {
   if (inv[i].type->id == it.type->id) {
    it_ammo* ammo = dynamic_cast<it_ammo*>(inv[i].type);
    if (inv[i].charges < ammo->count) {
     inv[i].charges += it.charges;
     if (inv[i].charges > ammo->count) {
      it.charges = inv[i].charges - ammo->count;
      inv[i].charges = ammo->count;
     } else
      it.charges = 0;
    }
   }
  }
  if (it.charges > 0)
   inv.push_back(it);
  return;
 }
/*
 bool combined = false;
 for (int i = 0; i < inv.size() && !combined; i++)
  combined = inv[i].stack_with(it);
 if (!combined)
*/
  inv.push_back(it);
}

bool player::has_active_item(itype_id id)
{
 if (weapon.type->id == id && weapon.active)
  return true;
 for (int i = 0; i < inv.size(); i++) {
  if (inv[i].type->id == id && inv[i].active)
   return true;
 }
 return false;
}

int player::active_item_charges(itype_id id)
{
 int max = 0;
 if (weapon.type->id == id && weapon.active)
  max = weapon.charges;
 for (int i = 0; i < inv.size(); i++) {
  if (inv[i].type->id == id && inv[i].active && inv[i].charges > max)
   max = inv[i].charges;
 }
 return max;
}

void player::process_active_items(game *g)
{
 it_tool* tmp;
 iuse use;
 if (weapon.active) {
  tmp = dynamic_cast<it_tool*>(weapon.type);
  (use.*tmp->use)(g, this, &weapon, true);
  if (g->turn % tmp->turns_per_charge == 0)
   weapon.charges--;
  if (weapon.charges <= 0) {
   (use.*tmp->use)(g, this, &weapon, false);
   if (tmp->revert_to == itm_null)
    weapon = ret_null;
   else
    weapon.type = g->itypes[tmp->revert_to];
  }
 }
 for (int i = 0; i < inv.size(); i++) {
  if (inv[i].active) {
   tmp = dynamic_cast<it_tool*>(inv[i].type);
   (use.*tmp->use)(g, this, &inv[i], true);
   if (tmp->turns_per_charge > 0 && g->turn % tmp->turns_per_charge == 0)
    inv[i].charges--;
   if (inv[i].charges <= 0) {
    (use.*tmp->use)(g, this, &inv[i], false);
    if (tmp->revert_to == itm_null) {
     inv.erase(inv.begin() + i);
     i--;
    } else
     inv[i].type = g->itypes[tmp->revert_to];
   }
  }
 }
}

item player::remove_weapon()
{
 item tmp = weapon;
 weapon = ret_null;
 return tmp;
}

item player::i_rem(char let)
{
 item tmp;
 if (weapon.invlet == let) {
  if (weapon.type->id > num_items)
   return ret_null;
  tmp = weapon;
  weapon = ret_null;
  return tmp;
 }
 for (int i = 0; i < worn.size(); i++) {
  if (worn[i].invlet == let) {
   tmp = worn[i];
   worn.erase(worn.begin() + i);
   return tmp;
  }
 }
 for (int i = 0; i < inv.size(); i++) {
  if (inv[i].invlet == let) {
   tmp = inv[i];
   inv.erase(inv.begin() + i);
   return tmp;
  }
 }
 return ret_null;
}

item player::i_rem(itype_id type)
{
 item ret;
 if (weapon.type->id == type)
  return remove_weapon();
 for (int i = 0; i < inv.size(); i++) {
  if (inv[i].type->id == type) {
   ret = inv[i];
   i_remn(i);
   return ret;
  }
 }
 return ret_null;
}

item& player::i_at(char let)
{
 if (weapon.invlet == let)
  return weapon;
 for (int i = 0; i < worn.size(); i++) {
  if (worn[i].invlet == let)
   return worn[i];
 }
 for (int i = 0; i < inv.size(); i++) {
  if (inv[i].invlet == let)
   return inv[i];
 }
 return ret_null;
}

item& player::i_of_type(itype_id type)
{
 if (weapon.type->id == type)
  return weapon;
 for (int i = 0; i < worn.size(); i++) {
  if (worn[i].type->id == type)
   return worn[i];
 }
 for (int i = 0; i < inv.size(); i++) {
  if (inv[i].type->id == type)
   return inv[i];
 }
 return ret_null;
}

std::vector<item> player::inv_dump()
{
 std::vector<item> ret;
 if (weapon.type->id != 0 && weapon.type->id < num_items)
  ret.push_back(weapon);
 for (int i = 0; i < worn.size(); i++)
  ret.push_back(worn[i]);
 for(int i = 0; i < inv.size(); i++)
  ret.push_back(inv[i]);
 return ret;
}

item player::i_remn(int index)
{
 if (index > inv.size() || index < 0)
  return ret_null;
 item ret = inv[index];
 inv.erase(inv.begin() + index);
 return ret;
}

void player::use_up(itype_id it, int quantity)
{
 int used = 0;
 if (weapon.type->id == it) {
  if (weapon.is_ammo() || weapon.is_tool()) {
   if (quantity < weapon.charges) {
    used = quantity;
    weapon.charges -= quantity;
   } else {
    used += weapon.charges;
    weapon.charges = 0;
   }
   if (weapon.charges <= 0) {
    used += weapon.charges;
    if (!weapon.is_tool())
     remove_weapon();
   }
  } else {
   used++;
   remove_weapon();
  }
 } else if (weapon.contents.size() > 0 && weapon.contents[0].type->id == it) {
  if (weapon.contents[0].is_ammo() || weapon.contents[0].is_tool()) {
   if (quantity < weapon.contents[0].charges) {
    used = quantity;
    weapon.contents[0].charges -= quantity;
   } else {
    used += weapon.contents[0].charges;
    weapon.contents[0].charges = 0;
   }
   if (weapon.contents[0].charges <= 0) {
    used += weapon.contents[0].charges;
    if (!weapon.contents[0].is_tool())
     weapon.contents.erase(weapon.contents.begin());
   }
  } else {
   used++;
   weapon.contents.erase(weapon.contents.begin());
  }
 }

 int cur_item = 0;
 while (used < quantity && cur_item < inv.size()) {
  int local_used = 0;
  if (inv[cur_item].type->id == it) { 
   if (inv[cur_item].is_ammo() || inv[cur_item].is_tool()) {
    if (quantity < inv[cur_item].charges) {
     local_used = quantity;
     inv[cur_item].charges -= quantity;
    } else {
     local_used += inv[cur_item].charges;
     inv[cur_item].charges = 0;
    }
    if (inv[cur_item].charges <= 0) {
     local_used += inv[cur_item].charges;
     if (!inv[cur_item].is_tool()) {
      i_remn(cur_item);
      cur_item--;
     }
    }
   } else {
    used++;
    i_remn(cur_item);
    cur_item--;
   }
  } else if (inv[cur_item].contents.size() > 0 &&
             inv[cur_item].contents[0].type->id == it) {
   if (inv[cur_item].contents[0].is_ammo() ||
       inv[cur_item].contents[0].is_tool()) {
    if (quantity < inv[cur_item].contents[0].charges) {
     local_used = quantity;
     inv[cur_item].contents[0].charges -= quantity;
    } else {
     local_used += inv[cur_item].contents[0].charges;
     inv[cur_item].contents[0].charges = 0;
    }
    if (inv[cur_item].contents[0].charges <= 0) {
     local_used += inv[cur_item].contents[0].charges;
     if (!inv[cur_item].contents[0].is_tool()) {
      i_remn(cur_item);
      cur_item--;
     }
    }
   } else {
    used++;
    i_remn(cur_item);
    cur_item--;
   }
  }
  used += local_used;
  cur_item++;
 }
}

void player::use_amount(itype_id it, int quantity)
{
 int cur_item = 0;
 int used = 0;
 if (weapon.type->id == it) {
  used++;
  remove_weapon();
 } else if (weapon.contents.size() > 0 && weapon.contents[0].type->id == it) {
  used++;
  weapon.contents.erase(weapon.contents.begin());
 }

 while (used < quantity && cur_item < inv.size()) {
  if (inv[cur_item].type->id == it) { 
   used++;
   i_remn(cur_item);
   cur_item--;
  } else if (inv[cur_item].contents.size() > 0 &&
             inv[cur_item].contents[0].type->id == it) {
   used++;
   i_remn(cur_item);
   cur_item--;
  }
 cur_item++;
 }
}

void player::use_charges(itype_id it, int quantity)
{
 int used = 0;
 if (weapon.type->id == it) {
  used += weapon.charges;
  if (used > quantity)
   weapon.charges = used - quantity;
  else
   weapon.charges = 0;
 } else if (weapon.contents.size() > 0 && weapon.contents[0].type->id == it) {
  used += weapon.contents[0].charges;
  if (used > quantity)
   weapon.contents[0].charges = used - quantity;
  else
   weapon.contents[0].charges = 0;
 }

 for (int i = 0; i < inv.size(); i++) {
  int local_used = 0;
  if (inv[i].type->id == it) { 
   local_used = inv[i].charges;
   if (local_used > quantity)
    inv[i].charges = local_used - quantity;
   else
    inv[i].charges = 0;
  } else if (inv[i].contents.size() > 0 &&
             inv[i].contents[0].type->id == it) {
   local_used = inv[i].contents[0].charges;
   if (local_used > quantity)
    inv[i].contents[0].charges = local_used - quantity;
   else
    inv[i].contents[0].charges = 0;
  }
  used += local_used;
 }
}

int player::butcher_factor()
{
 int lowest_factor = 999;
 for (int i = 0; i < inv.size(); i++) {
  if (inv[i].type->melee_cut >= 10 && !inv[i].has_weapon_flag(WF_SPEAR)) {
   int factor = inv[i].volume() * 5 - inv[i].weight() * 1.5 -
                inv[i].type->melee_cut;
   if (inv[i].type->melee_cut < 20)
    factor *= 2;
   if (factor < lowest_factor)
    lowest_factor = factor;
  }
 }
 if (weapon.type->melee_cut >= 10) {
  int factor = weapon.volume() * 5 - weapon.weight() * 1.5 -
               weapon.type->melee_cut;
  if (weapon.type->melee_cut < 20)
   factor *= 2;
  if (factor < lowest_factor)
   lowest_factor = factor;
 }
 return lowest_factor;
}

bool player::is_wearing(itype_id it)
{
 for (int i = 0; i < worn.size(); i++) {
  if (worn[i].type->id == it)
   return true;
 }
 return false;
}

bool player::has_amount(itype_id it, int quantity)
{
 int i_have = 0;
 if (weapon.type->id == it) {
  if ((weapon.is_ammo() || weapon.is_food()) && weapon.charges > 0)
   i_have += weapon.charges;
  else
   i_have++;
 } else if (weapon.contents.size() > 0 && weapon.contents[0].type->id == it) {
  if ((weapon.contents[0].is_ammo() || weapon.contents[0].is_food()) &&
      weapon.contents[0].charges > 0)
   i_have += weapon.contents[0].charges;
  else
   i_have++;
 }
 if (i_have >= quantity)
  return true;
 for (int i = 0; i < inv.size(); i++) {
  if (inv[i].type->id == it) {
   if ((inv[i].is_ammo() || inv[i].is_food()) && inv[i].charges > 0)
    i_have += inv[i].charges;
   else
    i_have++;
  } else if (inv[i].contents.size() > 0 && inv[i].contents[0].type->id == it) {
   if ((inv[i].contents[0].is_ammo() || inv[i].contents[0].is_food()) &&
       inv[i].contents[0].charges > 0)
    i_have += inv[i].contents[0].charges;
   else
    i_have++;
  }
  if (i_have >= quantity)
   return true;
 }
 return false;
}

bool player::has_number(itype_id it, int quantity)
{
 int i_have = 0;
 if (weapon.type->id == it)
  i_have += (weapon.is_ammo() ? weapon.charges : 1);
 else if (weapon.contents.size() > 0 && weapon.contents[0].type->id == it)
  i_have += (weapon.is_ammo() ? weapon.charges : 1);
 if (i_have >= quantity)
  return true;
 for (int i = 0; i < inv.size(); i++) {
  if (inv[i].type->id == it)
   i_have += (inv[i].is_ammo() ? inv[i].charges : 1);
  else if (inv[i].contents.size() > 0 && inv[i].contents[0].type->id == it)
   i_have += (inv[i].is_ammo() ? inv[i].charges : 1);
  if (i_have >= quantity)
   return true;
 }
 return false;
}

bool player::has_charges(itype_id it, int quantity)
{
 int i_have = 0;
 if (weapon.type->id == it)
  i_have += weapon.charges;
 else if (weapon.contents.size() > 0 && weapon.contents[0].type->id == it)
  i_have += weapon.contents[0].charges;
 if (i_have >= quantity)
  return true;
 for (int i = 0; i < inv.size(); i++) {
  if (inv[i].type->id == it)
   i_have += inv[i].charges;
  else if (inv[i].contents.size() > 0 && inv[i].contents[0].type->id == it) 
   i_have += inv[i].contents[0].charges;
  if (i_have >= quantity)
   return true;
 }
 return false;
}

bool player::has_item(char let)
{
 if (weapon.invlet == let)
  return true;
 for (int i = 0; i < worn.size(); i++) {
  if (worn[i].invlet == let)
   return true;
 }
 for (int i = 0; i < inv.size(); i++) {
  if (inv[i].invlet == let)
   return true;
 }
 return false;
}

bool player::has_item(item *it)
{
 if (it == &weapon)
  return true;
 for (int i = 0; i < inv.size(); i++) {
  if (it == &(inv[i]))
   return true;
 }
 for (int i = 0; i < worn.size(); i++) {
  if (it == &(worn[i]))
   return true;
 }
 return false;
}

int player::lookup_item(char let)
{
 if (weapon.invlet == let)
  return -1;

 for (int i = 0; i < inv.size(); i++) {
  if (inv[i].invlet == let)
   return i;
 }

 return -2; // -2 is for "item not found"
}

bool player::eat(game *g, int index)
{
 it_comest *comest = NULL;
 item *eaten = NULL;
 int which = -3; // Helps us know how to delete the item which got eaten
 int linet;
 if (index == -1) {
  if (weapon.is_food_container(this)) {
   eaten = &weapon.contents[0];
   which = -2;
   if (weapon.contents[0].is_food())
    comest = dynamic_cast<it_comest*>(weapon.contents[0].type);
  } else if (weapon.is_food(this)) {
   eaten = &weapon;
   which = -1;
   if (weapon.is_food())
    comest = dynamic_cast<it_comest*>(weapon.type);
  } else {
   if (!is_npc())
    g->add_msg("You can't eat your %s.", weapon.tname(g).c_str());
   else
    debugmsg("%s tried to eat a %s", name.c_str(), weapon.tname(g).c_str());
   return false;
  }
 } else {
  if (inv[index].is_food_container(this)) {
   eaten = &(inv[index].contents[0]);
   which = index + inv.size();
   if (inv[index].contents[0].is_food())
    comest = dynamic_cast<it_comest*>(inv[index].contents[0].type);
  } else if (inv[index].is_food(this)) {
   eaten = &inv[index];
   which = index;
   if (inv[index].is_food())
    comest = dynamic_cast<it_comest*>(inv[index].type);
  } else {
   if (!is_npc())
    g->add_msg("You can't eat your %s.", inv[index].tname(g).c_str());
   else
    debugmsg("%s tried to eat a %s", name.c_str(), inv[index].tname(g).c_str());
   return false;
  }
 }
 if (eaten == NULL)
  return false;

 if (eaten->is_ammo()) { // For when bionics let you eat fuel
  charge_power(eaten->charges / 20);
  eaten->charges = 0;
 } else if (!eaten->type->is_food() && !eaten->is_food_container(this)) {
// For when bionics let you burn organic materials
  int charge = (eaten->volume() + eaten->weight()) / 2;
  if (eaten->type->m1 == LEATHER || eaten->type->m2 == LEATHER)
   charge /= 4;
  if (eaten->type->m1 == WOOD    || eaten->type->m2 == WOOD)
   charge /= 2;
  charge_power(charge);
 } else { // It's real food!  i.e. an it_comest
// Remember, comest points to the it_comest data
  if (comest == NULL) {
   debugmsg("player::eat(%s); comest is NULL!", eaten->tname(g).c_str());
   return false;
  }
  if (comest != NULL && comest->tool != itm_null && !has_amount(comest->tool, 1)) {
   if (!is_npc())
    g->add_msg("You need a %s to consume that!",
               g->itypes[comest->tool]->name.c_str());
   return false;
  }
  bool overeating = (!has_trait(PF_GOURMAND) && hunger < 0 && comest->nutr >= 15);
  bool spoiled = (comest->spoils != 0 &&
                  (g->turn - eaten->bday) / 600 > comest->spoils);
  bool very_spoiled = (spoiled &&
                       (g->turn - eaten->bday / 600) > comest->spoils * 1.2);

  if (overeating && !is_npc() &&
      !query_yn("You're full.  Force yourself to eat?"))
   return false;

  if (has_trait(PF_CARNIVORE) && eaten->made_of(VEGGY)) {
   if (!is_npc())
    g->add_msg("You can only eat meat!");
   else
    g->add_msg("Carnivore %s tried to eat meat!", name.c_str());
   return false;
  }

  if (has_trait(PF_VEGETARIAN) && eaten->made_of(FLESH) && !is_npc() &&
      !query_yn("Really eat that meat? (The poor animals!)"))
   return false;

  if (spoiled) {
// We're only warned if we're a supertaster, OR the food is very old
   if ((!has_trait(PF_SUPERTASTER) && very_spoiled) || 
       (!is_npc() && query_yn("This %s smells awful!  Eat it?",
                              eaten->tname(g).c_str()))) {
    if (is_npc())
     return false;
    g->add_msg("Ick, this %s doesn't taste so good...",eaten->tname(g).c_str());
    if (!has_bionic(bio_digestion) || one_in(3))
     add_disease(DI_FOODPOISON, rng(60, (comest->nutr + 1) * 60), g);
    hunger -= rng(0, comest->nutr);
    if (!has_bionic(bio_digestion))
     health -= 3;
   } else
    return false;
  } else {
   hunger -= comest->nutr;
   thirst -= comest->quench;
   if (has_bionic(bio_digestion))
    hunger -= rng(0, comest->nutr);
   else if (!has_trait(PF_GOURMAND)) {
    if ((overeating && rng(-200, 0) > hunger))
     vomit(g);
   }
   health += comest->healthy;
  }
// Descriptive text
  if (!is_npc()) {
   if (eaten->made_of(LIQUID))
    g->add_msg("You drink your %s.", eaten->tname(g).c_str());
   else if (comest->nutr >= 5)
    g->add_msg("You eat your %s.", eaten->tname(g).c_str());
  } else if (g->u_see(posx, posy, linet))
   g->add_msg("%s eats a %s.", name.c_str(), eaten->tname(g).c_str());

  if (g->itypes[comest->tool]->is_tool())
   use_charges(comest->tool, 1); // Tools like lighters get used
  if (comest->stim > 0 && stim < comest->stim * 3)
   stim += comest->stim;
 
  iuse use;
  (use.*comest->use)(g, this, eaten, false);
  add_addiction(comest->add, comest->addict);
  if (has_bionic(bio_ethanol) && comest->use == &iuse::alcohol)
   charge_power(rng(20, 30));

  if (has_trait(PF_VEGETARIAN) && eaten->made_of(FLESH)) {
   if (!is_npc())
    g->add_msg("You feel bad about eating this meat...");
   add_morale(MORALE_VEGETARIAN, -75, -400);
  }
  if (has_trait(PF_HERBIVORE) && eaten->made_of(FLESH)) {
   if (!one_in(3))
    vomit(g);
   if (comest->quench >= 2)
    thirst += int(comest->quench / 2);
   if (comest->nutr >= 2)
    hunger += int(comest->nutr * .75);
  }
  std::stringstream morale_text;
  if (has_trait(PF_GOURMAND)) {
   if (comest->fun < -2)
    add_morale(MORALE_FOOD_BAD, comest->fun * 2, comest->fun * 15, comest);
   else if (comest->fun > 0)
    add_morale(MORALE_FOOD_GOOD, comest->fun * 3, comest->fun * 20, comest);
   if (!is_npc() && (hunger < -60 || thirst < -60))
    g->add_msg("You can't finish it all!");
   if (hunger < -60)
    hunger = -60;
   if (thirst < -60)
    thirst = -60;
  } else {
   if (comest->fun < 0)
    add_morale(MORALE_FOOD_BAD, comest->fun * 2, comest->fun * 15, comest);
   else if (comest->fun > 0)
    add_morale(MORALE_FOOD_GOOD, comest->fun * 2, comest->fun * 15, comest);
   if (!is_npc() && (hunger < -20 || thirst < -20))
    g->add_msg("You can't finish it all!");
   if (hunger < -20)
    hunger = -20;
   if (thirst < -20)
    thirst = -20;
  }
 }
 
 eaten->charges--;
 if (eaten->charges <= 0) {
  if (which == -1)
   weapon = ret_null;
  else if (which == -2) {
   weapon.contents.erase(weapon.contents.begin());
   if (!is_npc())
    g->add_msg("You are now wielding an empty %s.", weapon.tname(g).c_str());
  } else if (which >= 0 && which < inv.size())
   i_remn(which);
  else if (which >= inv.size()) {
   which -= inv.size();
   inv[which].contents.erase(inv[which].contents.begin());
   if (!is_npc())
    g->add_msg("%c - an empty %s", inv[which].invlet,
                                   inv[which].tname(g).c_str());
   inv_sorted = false;
  }
 }
 return true;
}

bool player::wield(game *g, int index)
{
 if (weapon.type->id == itm_bio_claws) {
  g->add_msg("You cannot unwield your claws!  Withdraw them with 'p'.");
  return false;
 }
 if (index == -3) {
  if (!is_armed()) {
   g->add_msg("You are already wielding nothing.");
   return false;
  } else if (volume_carried() + weapon.volume() < volume_capacity()) {
   inv.push_back(remove_weapon());
   inv_sorted = false;
   moves -= 20;
   recoil = 0;
   return true;
  } else if (query_yn("No space in inventory for your %s.  Drop it?",
                      weapon.tname(g).c_str())) {
   g->m.add_item(posx, posy, remove_weapon());
   recoil = 0;
   return true;
  } else
   return false;
 }
 if (index == -1) {
  g->add_msg("You're already wielding that!");
  return false;
 } else if (index == -2) {
  g->add_msg("You don't have that item.");
  return false;
 }

 if (inv[index].is_two_handed(this) && !has_two_arms()) {
  g->add_msg("You cannot wield a %s with only one arm.",
             inv[index].tname(g).c_str());
  return false;
 }
 if (!is_armed()) {
  weapon = inv[index];
  i_remn(index);
  moves -= 30;
  return true;
 } else if (volume_carried() + weapon.volume() - inv[index].volume() <
            volume_capacity()) {
  inv.push_back(remove_weapon());
  weapon = inv[index];
  i_remn(index);
  inv_sorted = false;
  moves -= 45;
  return true;
 } else if (query_yn("No space in inventory for your %s.  Drop it?",
                     weapon.tname(g).c_str())) {
  g->m.add_item(posx, posy, remove_weapon());
  weapon = inv[index];
  i_remn(index);
  inv_sorted = false;
  moves -= 30;
  return true;
 }

 return false;

}

bool player::wear(game *g, char let)
{
 item* to_wear = NULL;
 int index = -1;
 if (weapon.invlet == let) {
  to_wear = &weapon;
  index = -2;
 } else {
  for (int i = 0; i < inv.size(); i++) {
   if (inv[i].invlet == let) {
    to_wear = &(inv[i]);
    index = i;
    i = inv.size();
   }
  }
 }

 if (to_wear == NULL) {
  g->add_msg("You don't have item '%c'.", let);
  return false;
 }

 it_armor* armor = NULL;
 if (to_wear->is_armor())
  armor = dynamic_cast<it_armor*>(to_wear->type);
 else {
  g->add_msg("Putting on a %s would be tricky.", to_wear->tname(g).c_str());
  return false;
 }

 if (has_trait(PF_WOOLALLERGY) && to_wear->made_of(WOOL)) {
  g->add_msg("You can't wear that, it's made of wool!");
  return false;
 }
 if (armor->covers & mfb(bp_head) && encumb(bp_head) != 0) {
  g->add_msg("You can't wear a%s helmet!",
             wearing_something_on(bp_head) ? "nother" : "");
  return false;
 }
 if (armor->covers & mfb(bp_hands) && has_trait(PF_WEBBED)) {
  g->add_msg("You cannot put a %s over your webbed hands.", armor->name.c_str());
  return false;
 }
 if (armor->covers & mfb(bp_mouth) && has_trait(PF_BEAK)) {
  g->add_msg("You cannot put a %s over your beak.", armor->name.c_str());
  return false;
 }
 if (armor->covers & mfb(bp_feet) && wearing_something_on(bp_feet)) {
  g->add_msg("You're already wearing footwear!");
  return false;
 }
 g->add_msg("You put on your %s.", to_wear->tname(g).c_str());
 worn.push_back(*to_wear);
 if (index == -2)
  weapon = ret_null;
 else
  inv.erase(inv.begin() + index);
 for (body_part i = bp_head; i < num_bp; i = body_part(i + 1)) {
  if (encumb(i) >= 4)
   g->add_msg("Your %s %s very encumbered! %s",
              body_part_name(body_part(i), 2).c_str(),
              (i == bp_head || i == bp_torso || i == bp_mouth ? "is" : "are"),
              encumb_text(body_part(i)).c_str());
 }
 return true;
}

bool player::takeoff(game *g, char let)
{
 for (int i = 0; i < worn.size(); i++) {
  if (worn[i].invlet == let) {
   if (volume_capacity() - (dynamic_cast<it_armor*>(worn[i].type))->storage >
       volume_carried() + worn[i].type->volume) {
    inv.push_back(worn[i]);
    worn.erase(worn.begin() + i);
    inv_sorted = false;
    return true;
   } else if (query_yn("No room in inventory for your %s.  Drop it?",
                       worn[i].tname(g).c_str())) {
    g->m.add_item(posx, posy, worn[i]);
    worn.erase(worn.begin() + i);
    return true;
   } else
    return false;
  }
 }
 g->add_msg("You are not wearing that item.");
 return false;
}

void player::use(game *g, char let)
{
 item* used = &(i_at(let));
 if (used->type->id == 0) {
  g->add_msg("You do not have that item.");
  return;
 }

 if (used->is_tool()) {

  it_tool *tool = dynamic_cast<it_tool*>(used->type);
  if (tool->charges_per_use == 0 || used->charges >= tool->charges_per_use) {
   iuse use;
   (use.*tool->use)(g, this, used, false);
   used->charges -= tool->charges_per_use;
  } else
   g->add_msg("Your %s doesn't have enough charges.", used->tname(g).c_str());

 } else if (used->is_gunmod()) {

  if (sklevel[sk_gun] == 0) {
   g->add_msg("You need to be at least level 1 in the firearms skill before you\
 can modify guns.");
   return;
  }
  char gunlet = g->inv("Select gun to modify:");
  it_gunmod *mod = dynamic_cast<it_gunmod*>(used->type);
  item* gun = &(i_at(gunlet));
  if (gun->type->id == 0) {
   g->add_msg("You do not have that item.");
   return;
  } else if (!gun->is_gun()) {
   g->add_msg("That %s is not a gun.", gun->tname(g).c_str());
   return;
  }
  it_gun* guntype = dynamic_cast<it_gun*>(gun->type);
  if (guntype->skill_used == sk_pistol && !mod->used_on_pistol) {
   g->add_msg("That %s cannot be attached to a handgun.",used->tname(g).c_str());
   return;
  } else if (guntype->skill_used == sk_shotgun && !mod->used_on_shotgun) {
   g->add_msg("That %s cannot be attached to a shotgun.",used->tname(g).c_str());
   return;
  } else if (guntype->skill_used == sk_smg && !mod->used_on_smg) {
   g->add_msg("That %s cannot be attached to a submachine gun.",
              used->tname(g).c_str());
   return;
  } else if (guntype->skill_used == sk_rifle && !mod->used_on_rifle) {
   g->add_msg("That %s cannot be attached to a rifle.", used->tname(g).c_str());
   return;
  } else if (mod->acceptible_ammo_types != 0 &&
             !(mfb(guntype->ammo) & mod->acceptible_ammo_types)) {
   g->add_msg("That %s cannot be used on a %s gun.", used->tname(g).c_str(),
              ammo_name(guntype->ammo).c_str());
   return;
  } else if (gun->contents.size() >= 4) {
   g->add_msg("Your %s already has 4 mods installed!  To remove the mods,\
press 'U' while wielding the unloaded gun.", gun->tname(g).c_str());
   return;
  }
  for (int i = 0; i < gun->contents.size(); i++) {
   if (gun->contents[i].type->id == used->type->id) {
    g->add_msg("Your %s already has a %s.", gun->tname(g).c_str(),
               used->tname(g).c_str());
    return;
   } else if (mod->newtype != AT_NULL &&
        (dynamic_cast<it_gunmod*>(gun->contents[i].type))->newtype != AT_NULL) {
    g->add_msg("Your %s's caliber has already been modified.",
               gun->tname(g).c_str());
    return;
   } else if ((mod->id == itm_barrel_big || mod->id == itm_barrel_small) &&
              (gun->contents[i].type->id == itm_barrel_big ||
               gun->contents[i].type->id == itm_barrel_small)) {
    g->add_msg("Your %s already has a barrel replacement.",
               gun->tname(g).c_str());
    return;
   } else if ((mod->id == itm_clip || mod->id == itm_clip2) &&
              gun->clip_size() <= 2) {
    g->add_msg("You can not extend the ammo capacity of your %s.",
               gun->tname(g).c_str());
    return;
   }
  }
  g->add_msg("You attach the %s to your %s.", used->tname(g).c_str(),
             gun->tname(g).c_str());
  gun->contents.push_back(i_rem(let));

 } else if (used->is_bionic()) {
  it_bionic* tmp = dynamic_cast<it_bionic*>(used->type);
  if (install_bionics(g, tmp))
   i_rem(let);
 } else if (used->is_food() || used->is_food_container())
  eat(g, lookup_item(let));
 else if (used->is_book())
  read(g, let);
 else if (used->is_armor())
  wear(g, let);
 else
  g->add_msg("You can't do anything interesting with your %s.",
             used->tname(g).c_str());
}

void player::read(game *g, char ch)
{
 if (morale_level() < MIN_MORALE_READ) {	// See morale.h
  g->add_msg("What's the point of reading?  (Your morale is too low!)");
  return;
 }

// Find the object
 int index = -1;
 if (weapon.invlet == ch)
  index = -2;
 else {
  for (int i = 0; i < inv.size(); i++) {
   if (inv[i].invlet == ch) {
    index = i;
    i = inv.size();
   }
  }
 }

// Check if reading is okay
 if (g->light_level() <= 2) {
  g->add_msg("It's too dark to read!");
  return;
 } else if (index == -1) {
  g->add_msg("You do not have that item.");
  return;
 }

// Some macguffins can be read, but they aren't treated like books.
 it_macguffin* mac = NULL;
 item *used;
 if (index == -2 && weapon.is_macguffin()) {
  mac = dynamic_cast<it_macguffin*>(weapon.type);
  used = &weapon;
 } else if (index >= 0 && inv[index].is_macguffin()) {
  mac = dynamic_cast<it_macguffin*>(inv[index].type);
  used = &(inv[index]);
 }
 if (mac != NULL) {
  iuse use;
  (use.*mac->use)(g, this, used, false);
  return;
 }

 if ((index >=  0 && !inv[index].is_book()) ||
            (index == -2 && !weapon.is_book())) {
  g->add_msg("Your %s is not good reading material.",
           (index == -2 ? weapon.tname(g).c_str() : inv[index].tname(g).c_str()));
  return;
 }

 it_book* tmp;
 if (index == -2)
  tmp = dynamic_cast<it_book*>(weapon.type);
 else
  tmp = dynamic_cast<it_book*>(inv[index].type);
 if (tmp->intel > 0 && has_trait(PF_ILLITERATE)) {
  g->add_msg("You're illiterate!");
  return;
 } else if (tmp->intel > int_cur) {
  g->add_msg("This book is way too complex for you to understand.");
  return;
 } else if (tmp->req > sklevel[tmp->type]) {
  g->add_msg("The %s-related jargon flies over your head!",
             skill_name(tmp->type).c_str());
  return;
 } else if (tmp->level <= sklevel[tmp->type] && tmp->fun <= 0 &&
            !query_yn("Your %s skill won't be improved.  Read anyway?",
                      skill_name(tmp->type).c_str()))
  return;

// Base read_speed() is 1000 move points (1 minute per tmp->time)
 int time = tmp->time * read_speed();
 activity = player_activity(ACT_READ, time, index);
 moves = 0;
}
 
void player::try_to_sleep(game *g)
{
 add_disease(DI_LYING_DOWN, 300, g);
}

bool player::can_sleep(game *g)
{
 int sleepy = 0;
 if (has_addiction(ADD_SLEEP))
  sleepy -= 3;
 if (has_trait(PF_INSOMNIA))
  sleepy -= 8;
 if (g->m.ter(posx, posy) == t_bed)
  sleepy += 5;
 else if (g->m.ter(posx, posy) == t_floor)
  sleepy += 1;
 else
  sleepy -= g->m.move_cost(posx, posy);
 sleepy += int((fatigue - 192) / 16);
 sleepy += rng(-8, 8);
 sleepy -= 2 * stim;
 if (sleepy > 0)
  return true;
 return false;
}

int player::warmth(body_part bp)
{
 int ret = 0;
 for (int i = 0; i < worn.size(); i++) {
  if ((dynamic_cast<it_armor*>(worn[i].type))->covers & mfb(bp))
   ret += (dynamic_cast<it_armor*>(worn[i].type))->warmth;
 }
 return ret;
}

int player::encumb(body_part bp)
{
 int ret = 0;
 int layers = 0;
 it_armor* armor;
 for (int i = 0; i < worn.size(); i++) {
  if (!worn[i].is_armor())
   debugmsg("%s::encumb hit a non-armor item at worn[%d] (%s)", name.c_str(),
            i, worn[i].tname().c_str());
  if (worn[i].type->id != itm_glasses_eye) {
   armor = dynamic_cast<it_armor*>(worn[i].type);
   if (armor->covers & mfb(bp) ||
       (bp == bp_torso && (armor->covers & mfb(bp_arms)))) {
    ret += armor->encumber;
    if (armor->encumber >= 0 || bp != bp_torso)
     layers++;
   }
  }
 }
 if (layers > 1)
  ret += (layers - 1) * (bp == bp_torso ? .5 : 2);// Easier to layer on torso
 if ((bp == bp_head  && has_bionic(bio_armor_head))  ||
     (bp == bp_torso && has_bionic(bio_armor_torso)) ||
     (bp == bp_legs  && has_bionic(bio_armor_legs)))
  ret += 2;
 if (volume_carried() > volume_capacity() - 2 && bp != bp_head)
  ret += 3;
 if (has_bionic(bio_stiff) && bp != bp_head && bp != bp_mouth)
  ret += 1;
 return ret;
}

int player::armor_bash(body_part bp)
{
 int ret = 0;
 it_armor* armor;
 for (int i = 0; i < worn.size(); i++) {
  armor = dynamic_cast<it_armor*>(worn[i].type);
  if (armor->covers & mfb(bp))
   ret += armor->dmg_resist;
 }
 if (has_bionic(bio_carbon))
  ret += 2;
 if (bp == bp_head && has_bionic(bio_armor_head))
  ret += 3;
 else if (bp == bp_arms && has_bionic(bio_armor_arms))
  ret += 3;
 else if (bp == bp_torso && has_bionic(bio_armor_torso))
  ret += 3;
 else if (bp == bp_legs && has_bionic(bio_armor_legs))
  ret += 3;
 if (has_trait(PF_FUR))
  ret++;
 if (has_trait(PF_CHITIN))
  ret += 2;
 return ret;
}

int player::armor_cut(body_part bp)
{
 int ret = 0;
 it_armor* armor;
 for (int i = 0; i < worn.size(); i++) {
  armor = dynamic_cast<it_armor*>(worn[i].type);
  if (armor->covers & mfb(bp))
   ret += armor->cut_resist;
 }
 if (has_bionic(bio_carbon))
  ret += 4;
 if (bp == bp_head && has_bionic(bio_armor_head))
  ret += 3;
 else if (bp == bp_arms && has_bionic(bio_armor_arms))
  ret += 3;
 else if (bp == bp_torso && has_bionic(bio_armor_torso))
  ret += 3;
 else if (bp == bp_legs && has_bionic(bio_armor_legs))
  ret += 3;
 if (has_trait(PF_THICKSKIN))
  ret++;
 if (has_trait(PF_SCALES))
  ret += 3;
 if (has_trait(PF_CHITIN))
  ret += 10;
 return ret;
}

void player::absorb(game *g, body_part bp, int &dam, int &cut)
{
 it_armor* tmp;
 int arm_bash = 0, arm_cut = 0;
 if (has_active_bionic(bio_ads)) {
  if (dam > 0 && power_level > 1) {
   dam -= rng(1, 8);
   power_level--;
  }
  if (cut > 0 && power_level > 1) {
   cut -= rng(0, 4);
   power_level--;
  }
  if (dam < 0)
   dam = 0;
  if (cut < 0)
   cut = 0;
 }
// See, we do it backwards, which assumes the player put on their jacket after
//  their T shirt, for example.  TODO: don't assume! ASS out of U & ME, etc.
 for (int i = worn.size() - 1; i >= 0; i--) {
  tmp = dynamic_cast<it_armor*>(worn[i].type);
  if ((tmp->covers & mfb(bp)) && tmp->storage < 20) {
   arm_bash = tmp->dmg_resist;
   arm_cut  = tmp->cut_resist;
   switch (worn[i].damage) {
   case 1:
    arm_bash *= .8;
    arm_cut  *= .9;
    break;
   case 2:
    arm_bash *= .7;
    arm_cut  *= .7;
    break;
   case 3:
    arm_bash *= .5;
    arm_cut  *= .4;
    break;
   case 4:
    arm_bash *= .2;
    arm_cut  *= .1;
    break;
   }
// Wool, leather, and cotton clothing may be damaged by CUTTING damage
   if ((worn[i].made_of(WOOL)   || worn[i].made_of(LEATHER) ||
        worn[i].made_of(COTTON) || worn[i].made_of(GLASS)   ||
        worn[i].made_of(WOOD)   || worn[i].made_of(KEVLAR)) &&
       rng(0, tmp->cut_resist * 2) < cut && !one_in(cut))
    worn[i].damage++;
// Kevlar, plastic, iron, steel, and silver may be damaged by BASHING damage
   if ((worn[i].made_of(PLASTIC) || worn[i].made_of(IRON)   ||
        worn[i].made_of(STEEL)   || worn[i].made_of(SILVER) ||
        worn[i].made_of(STONE))  &&
       rng(0, tmp->dmg_resist * 2) < dam && !one_in(dam))
    worn[i].damage++;
   if (worn[i].damage >= 5) {
    g->add_msg("Your %s is completely destroyed!", worn[i].tname(g).c_str());
    worn.erase(worn.begin() + i);
   }
  }
  dam -= arm_bash;
  cut -= arm_cut;
 }
 if (has_bionic(bio_carbon)) {
  dam -= 2;
  cut -= 4;
 }
 if (bp == bp_head && has_bionic(bio_armor_head)) {
  dam -= 3;
  cut -= 3;
 } else if (bp == bp_arms && has_bionic(bio_armor_arms)) {
  dam -= 3;
  cut -= 3;
 } else if (bp == bp_torso && has_bionic(bio_armor_torso)) {
  dam -= 3;
  cut -= 3;
 } else if (bp == bp_legs && has_bionic(bio_armor_legs)) {
  dam -= 3;
  cut -= 3;
 }
 if (has_trait(PF_THICKSKIN))
  cut--;
 if (has_trait(PF_SCALES))
  cut -= 3;
 if (has_trait(PF_FUR))
  dam--;
 if (has_trait(PF_CHITIN)) {
  dam -= 2;
  cut -= 10;
 }
 if (dam < 0)
  dam = 0;
 if (cut < 0)
  cut = 0;
}
  
int player::resist(body_part bp)
{
 int ret = 0;
 for (int i = 0; i < worn.size(); i++) {
  if ((dynamic_cast<it_armor*>(worn[i].type))->covers & mfb(bp) ||
      (bp == bp_eyes && // Head protection works on eyes too (e.g. baseball cap)
           (dynamic_cast<it_armor*>(worn[i].type))->covers & mfb(bp_head)))
   ret += (dynamic_cast<it_armor*>(worn[i].type))->env_resist;
 }
 if (bp == bp_mouth && has_bionic(bio_purifier) && ret < 5) {
  ret += 2;
  if (ret == 6)
   ret = 5;
 }
 return ret;
}

bool player::wearing_something_on(body_part bp)
{
 for (int i = 0; i < worn.size(); i++) {
  if ((dynamic_cast<it_armor*>(worn[i].type))->covers & mfb(bp))
    return true;
 }
 return false;
}

void player::practice(skill s, int amount)
{
 skill savant = sk_null;
 int savant_level = 0, savant_exercise = 0;
 if (has_trait(PF_SAVANT)) {
// Find our best skill
  for (int i = 1; i < num_skill_types; i++) {
   if (sklevel[i] >= savant_level) {
    savant = skill(i);
    savant_level = sklevel[i];
    savant_exercise = skexercise[i];
   } else if (sklevel[i] == savant_level && skexercise[i] > savant_exercise) {
    savant = skill(i);
    savant_exercise = skexercise[i];
   }
  }
 }
 while (amount > 0 && xp_pool >= 1) {
  amount--;
  if ((savant == sk_null || savant == s || !one_in(2)) &&
      rng(0, 100) < comprehension_percent(s)) {
   xp_pool--;
   skexercise[s]++;
  }
 }
}

std::vector<int> player::has_ammo(ammotype at)
{
 std::vector<int> ret;
 it_ammo* tmp;
 bool newtype = true;
 for (int a = 0; a < inv.size(); a++) {
  if (inv[a].is_ammo() && dynamic_cast<it_ammo*>(inv[a].type)->type == at) {
   newtype = true;
   tmp = dynamic_cast<it_ammo*>(inv[a].type);
   for (int i = 0; i < ret.size(); i++) {
    if (tmp->id == inv[ret[i]].type->id &&
        inv[a].charges == inv[ret[i]].charges) {
// They're effectively the same; don't add it to the list
// TODO: Bullets may become rusted, etc., so this if statement may change
     newtype = false;
     i = ret.size();
    }
   }
   if (newtype)
    ret.push_back(a);
  }
 }
 return ret;
}

std::string player::weapname(bool charges)
{
 if (!(weapon.is_tool() &&
       dynamic_cast<it_tool*>(weapon.type)->max_charges <= 0) &&
     weapon.charges >= 0 && charges) {
  std::stringstream dump;
  dump << weapon.tname().c_str() << " (" << weapon.charges << ")";
  return dump.str();
 } else if (weapon.type->id == 0)
  return "fists";
 else
  return weapon.tname();
}

nc_color encumb_color(int level)
{
 if (level < 0)
  return c_green;
 if (level == 0)
  return c_ltgray;
 if (level < 4)
  return c_yellow;
 if (level < 7)
  return c_ltred;
 return c_red;
}

