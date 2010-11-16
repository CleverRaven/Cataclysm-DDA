#include "player.h"
#include "bionics.h"
#include "game.h"
#include "disease.h"
#include "addiction.h"
#include "keypress.h"
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
 str_cur = str_max;
 dex_cur = dex_max;
 int_cur = int_max;
 per_cur = per_max;
 can_dodge = true;
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
 if (has_trait(PF_CHITIN))
  dex_cur -= 3;
 if (pain > pkill) {
  str_cur  -= int((pain - pkill) / 15);
  dex_cur  -= int((pain - pkill) / 15);
  per_cur  -= int((pain - pkill) / 20);
  int_cur  -= 1 + int((pain - pkill) / 25);
 }
 if (abs(morale_level()) >= 10) {
  str_cur  += int(morale_level() / 18);
  dex_cur  += int(morale_level() / 20);
  per_cur  += int(morale_level() / 14);
  int_cur  += int(morale_level() / 10);
 }
 if (radiation > 0) {
  str_cur  -= int(radiation / 12);
  dex_cur  -= int(radiation / 18);
  per_cur  -= int(radiation / 20);
  int_cur  -= int(radiation / 24);
 }
 dex_cur += int(stim / 10);
 per_cur += int(stim /  7);
 int_cur += int(stim /  6);
 if (stim >= 30) { 
  dex_cur -= int(abs(stim - 15) /  8);
  per_cur -= int(abs(stim - 15) / 12);
  int_cur -= int(abs(stim - 15) / 14);
 }

 moves += current_speed();
 if (dex_cur < 0)
  dex_cur = 0;
 if (str_cur < 0)
  str_cur = 0;
 if (per_cur < 0)
  per_cur = 0;
 if (int_cur < 0)
  int_cur = 0;
 
 if (has_bionic(bio_metabolics) && power_level < max_power_level &&
     hunger < 100) {
  hunger += 2;
  power_level++;
 }

 update_morale();
}

void player::update_morale()
{
 for (int i = 0; i < morale.size(); i++) {
  morale[i].duration--;
  if (morale[i].duration <= 0) {
   morale.erase(morale.begin() + i);
   i--;
  }
 }
}

int player::current_speed()
{
 int newmoves = 100; // Start with 12 movement points...
// Minus some for weight...
 if (weight_carried() > weight_capacity())
  newmoves = 1;
 else if (weight_carried() >= weight_capacity() * .25)
  newmoves = int((120 * (weight_capacity() - weight_carried())) /
                         weight_capacity());
// Minus some amount for pain & painkiller wooziness...
 if (pain > pkill)
  newmoves -= int((pain - pkill) * .7);
 if (pkill >= 3)
  newmoves -= int(pkill * .4);
// Plus or minus some for considerable morale...
 if (abs(morale_level()) >= 10) {
  int morale_bonus = int(morale_level() / 2);
  if (morale_bonus < -25)
   morale_bonus = -25;
  else if (morale_bonus > 15)
   morale_bonus = 15;
  newmoves += morale_bonus;
 }
 if (radiation > 0)
  newmoves -= int(radiation * .4);
 if (thirst > 40)
  newmoves -= int((thirst - 40) / 10);
 if (hunger > 100)
  newmoves -= int((hunger - 100) / 10);
 newmoves += stim;
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
  ret -= 100;
 ret += (50 - sklevel[sk_swimming] * 2) * abs(encumb(bp_legs));
 ret += (80 - sklevel[sk_swimming] * 3) * abs(encumb(bp_torso));
 for (int i = 0; i < worn.size(); i++)
  ret += worn[i].volume() * (20 - sklevel[sk_swimming]);
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

void player::load_info(std::string data)
{
 std::stringstream dump;
 dump << data;
 dump >> posx >> posy >> str_cur >> str_max >> dex_cur >> dex_max >>
         int_cur >> int_max >> per_cur >> per_max >> power_level >>
         max_power_level >> hunger >> thirst >> fatigue >> stim >>
         pain >> pkill >> radiation >> cash >> recoil >> scent >> moves >>
         underwater >> can_dodge >> oxygen;

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
}

std::string player::save_info()
{
 std::stringstream dump;
 dump << posx    << " " << posy << " " << str_cur << " " << str_max << " " <<
         dex_cur << " " << dex_max << " " << int_cur << power_level << " " <<
         max_power_level << " " << int_max << " " << per_cur << " " <<
         per_max << " " << hunger << " " << thirst << " " << fatigue << " " <<
         " " << stim << " " << pain << " " << pkill << " " << radiation <<
         " " << cash << " " << recoil << " " << scent << " " << moves << " " <<
         underwater << " " << can_dodge << " " << oxygen << " ";

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
          int(my_bionics[i].powered) << " " << my_bionics[i].charge << " ";

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
 if (abs(morale_level()) >= 10) {
  bool pos = (morale_level() > 0);
  effect_name.push_back(pos ? "Elated" : "Depressed");
  std::stringstream morale_text;
  if (abs(morale_level()) >= 20)
   morale_text << "Dexterity" << (pos ? " +" : " ") <<
                   int(morale_level() / 20) << "   ";
  if (abs(morale_level()) >= 18)
   morale_text << "Strength" << (pos ? " +" : " ") <<
                  int(morale_level() / 18) << "   ";
  if (abs(morale_level()) >= 14)
   morale_text << "Perception" << (pos ? " +" : " ") <<
                  int(morale_level() / 14) << "   ";
  morale_text << "Intelligence" << (pos ? " +" : " ") <<
                 int(morale_level() / 10) << "   ";
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
               (intbonus > 0 ? "+" : "") << intbonus << "   Perception " <<
               (perbonus > 0 ? "+" : "") << perbonus << "   Dexterity "  <<
               (dexbonus > 0 ? "+" : "") << dexbonus;
  effect_text.push_back(stim_text.str());
 } else if (stim < 0) {
  effect_name.push_back("Depressants");
  std::stringstream stim_text;
  int dexpen = int(stim / 10);
  int perpen = int(stim /  7);
  int intpen = int(stim /  6);
  stim_text << "Speed " << stim << "   Intelligence " << intpen <<
               "   Perception " << perpen << "   Dexterity " << dexpen;
  effect_text.push_back(stim_text.str());
 }

 if (has_trait(PF_TROGLO) && is_in_sunlight(g)) {
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
    if (skexercise[i] >= 100) {
     mvwprintz(w_skills, line,  1, c_pink, "%s:", skill_name(skill(i)).c_str());
     mvwprintz(w_skills, line, 19, c_pink, "%d", sklevel[i]);
    } else {
     mvwprintz(w_skills, line, 1, c_ltblue, "%s:",
               skill_name(skill(i)).c_str());
     mvwprintz(w_skills, line,19, c_ltblue, "%d", sklevel[i]);
    }
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
 pen = int(morale_level() / 2);
 if (abs(pen) >= 5) {
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
 if (pkill >= 3) {
  pen = int(pkill * .4);
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
Eye encumberance is in the form of something distorting your vision slightly,\n\
such as goggles.  It creates a slight decrease in ranged accuracy.");
   } else if (line == 2) {
    mvwprintz(w_encumb, 4, 2, h_ltgray, "Mouth");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Jaw encumberance means that something is covering your mouth and possibly\n\
making it harder to breathe comfortably.  It makes running slightly harder.");
   } else if (line == 3) {
    mvwprintz(w_encumb, 5, 2, h_ltgray, "Torso");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Torso encumberance means that clothing is hindering the movement of your\n\
arms or torso in general.  It interferes with melee and ranged combat as well\n\
as some other actions.");
   } else if (line == 4) {
    mvwprintz(w_encumb, 6, 2, h_ltgray, "Hands");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Hand encumberance limits your fine motor skills, resulting in a minor penalty\n\
to melee and ranged combat alike.");
   } else if (line == 5) {
    mvwprintz(w_encumb, 7, 2, h_ltgray, "Legs");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Leg encumberance comes from baggy pants, or wearing too many layers of pants.\n\
It makes running and dodging more difficult.");
   } else if (line == 6) {
    mvwprintz(w_encumb, 8, 2, h_ltgray, "Feet");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Some footwear will cause minor encumberance.  This will make running a bit\n\
slower.");
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
     curtab ++;
     break;
    case 'q':
    case KEY_ESCAPE:
     done = true;
   }
   if (line >= 0 && line < illness.size())
    mvwprintz(w_effects, 2 + line, 1, c_ltgray, effect_name[line].c_str());
   break;

  case 5:	// Skills tab
   mvwprintz(w_skills, 0, 0, h_ltgray, "         SKILLS           ");
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
     if (skexercise[skillslist[i]] >= 100)
      status = c_pink;
     else
      status = c_ltblue;
    }
    mvwprintz(w_skills, 2 + i - min, 1, c_ltgray, "                         ");
    if (skexercise[i] >= 100) {
     mvwprintz(w_skills, 2 + i - min, 1, status, "%s:",
               skill_name(skillslist[i]).c_str());
     mvwprintz(w_skills, 2 + i - min,19, status, "%d", sklevel[skillslist[i]]);
    } else {
     mvwprintz(w_skills, 2 + i - min, 1, status, "%s:",
               skill_name(skillslist[i]).c_str());
     mvwprintz(w_skills, 2 + i - min,19, status, "%d", sklevel[skillslist[i]]);
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
     mvwprintz(w_skills, 0, 0, c_ltgray, "         SKILLS           ");
     for (int i = 0; i < skillslist.size() && i < 7; i++) {
      if (skexercise[skillslist[i]] >= 100)
       status = c_pink;
      else
       status = c_ltblue;
      mvwprintz(w_skills, i + 2,  1, status, "%s:",
                skill_name(skillslist[i]).c_str());
      mvwprintz(w_skills, i + 2, 19, status, "%d", sklevel[skillslist[i]]);
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
 mvwprintz(w, 2, 20, c_ltgray, "Value       Duration");

 for (int i = 0; i < morale.size(); i++) {
  int b = morale[i].bonus;
  int mins = int(morale[i].duration / 10);
  int hours = int(mins / 60);
  mins %= 60;

  int bpos = 24;
  if (abs(b) >= 10)
   bpos--;
  if (b < 0)
   bpos--;

  int dpos = 32;
  if (hours >= 10)
   dpos--;

  mvwprintz(w, i + 3,  1, (b < 0 ? c_red : c_green), morale[i].name().c_str());
  mvwprintz(w, i + 3, bpos, (b < 0 ? c_red : c_green), "%d", b);
  mvwprintz(w, i + 3, dpos, c_blue, "%dh%s%dm", hours, (mins < 10 ? "0" : ""),
            mins);
 }

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
   mvwprintz(w, 1, 35, c_red,    "Recoil");
  else if (recoil >= 20)
   mvwprintz(w, 1, 35, c_ltred,  "Recoil");
  else if (recoil >= 4)
   mvwprintz(w, 1, 35, c_yellow, "Recoil");
  else if (recoil > 0)
   mvwprintz(w, 1, 35, c_ltgray, "Recoil");
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
  mvwprintz(w, 2, 14, c_ltred,  "Parched");
 else if (thirst > 240)
  mvwprintz(w, 2, 14, c_ltred,  "Dehydrated");
 else if (thirst > 80)
  mvwprintz(w, 2, 14, c_yellow, "Very thirsty");
 else if (thirst > 40)
  mvwprintz(w, 2, 14, c_yellow, "Thirsty");
 else if (thirst < 0)
  mvwprintz(w, 2, 14, c_green,  "Slaked");

      if (fatigue > 575)
  mvwprintz(w, 2, 27, c_red,    "Exhausted");
 else if (fatigue > 383)
  mvwprintz(w, 2, 27, c_ltred,  "Dead tired");
 else if (fatigue > 191)
  mvwprintz(w, 2, 27, c_yellow, "Tired");

 if (str_cur > str_max)
  mvwprintz(w, 2, 37, c_green, "Str");
 if (str_cur < str_max)
  mvwprintz(w, 2, 37, c_red,   "Str");
 if (dex_cur > dex_max)
  mvwprintz(w, 2, 41, c_green, "Dex");
 if (dex_cur < dex_max)
  mvwprintz(w, 2, 41, c_red,   "Dex");
 if (int_cur > int_max)
  mvwprintz(w, 2, 45, c_green, "Int");
 if (int_cur < int_max)
  mvwprintz(w, 2, 45, c_red,   "Int");
 if (per_cur > per_max)
  mvwprintz(w, 2, 49, c_green, "Per");
 if (per_cur < per_max)
  mvwprintz(w, 2, 49, c_red,   "Per");

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
 else if (pain - pkill >  0)
  mvwprintz(w, 3, 0, c_yellow, "Minor pain");

      if (pkill >= 100)
  mvwprintz(w, 3, 19, c_red,    "Passing out");
 else if (pkill >= 75)
  mvwprintz(w, 3, 19, c_ltred,  "Dizzy");
 else if (pkill >= 50)
  mvwprintz(w, 3, 19, c_ltred,  "Woozy");
 else if (pkill >= 25)
  mvwprintz(w, 3, 19, c_yellow, "Light-headed");
 else if (pkill >= 10)
  mvwprintz(w, 3, 19, c_yellow, "Buzzed");
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
  if (my_bionics[i].id == b) {
   if (my_bionics[i].powered)
    return true;
   else
    return false;
  }
 }
 return false;
}

void player::add_bionic(bionic_id b)
{
 for (int i = 0; i < my_bionics.size(); i++) {
  if (my_bionics[i].id == b)
   return;	// No dupes!
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
You can not activate %s!  To read a description of\
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
//  case it is a 7 in 17 chance).
  if (rng(1, 7) >= 6 || (has_trait(PF_ROBUST) && one_in(3))) {
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
      per_max -= rng(0, 2);	// Tough break!
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
     g->add_msg("Your deformations melt away%s.",
                (has_trait(PF_DEFORMED2) ? " somewhat" : ""));
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
    } else if (!has_trait(PF_REGEN) & one_in(5)) {
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
   case 22:
    if (!has_trait(PF_TALONS)) {
     g->add_msg("Your index fingers grow into huge talons!");
     toggle_trait(PF_TALONS);
     return;
    }
   case 23:
    if (!has_trait(PF_RADIOGENIC) && one_in(2)) {
     g->add_msg("You crave irradiation!");
     toggle_trait(PF_RADIOGENIC);
     return;
    }
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
    if (!has_trait(PF_DEFORMED)) {
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
     if (is_in_sunlight(g))
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
       g->add_msg("Your %s are pushed off!", worn[i].tname().c_str());
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
     g->add_msg("Your mouth extends into a beak!");
     toggle_trait(PF_BEAK);
     if (has_trait(PF_FANGS)) {
      g->add_msg("Your fangs fade away.");
      toggle_trait(PF_FANGS);
     }
// Force off and damage any mouthwear
     for (int i = 0; i < worn.size(); i++) {
      if ((dynamic_cast<it_armor*>(worn[i].type))->covers & mfb(bp_mouth)) {
       worn[i].damage += 2;
       if (worn[i].damage >= 5)
        g->add_msg("Your %s is destroyed!", worn[i].tname().c_str());
       else {
        g->add_msg("Your %s is damaged and pushed off!",
                   worn[i].tname().c_str());
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
   case 21:
    if (!has_trait(PF_RADIOACTIVE)) {
     g->add_msg("You feel radioactive.");
     toggle_trait(PF_RADIOACTIVE);
     return;
    }
   case 22:
    if (!has_trait(PF_SLIMY)) {
     g->add_msg("You start to ooze a thick green slime.");
     toggle_trait(PF_SLIMY);
     return;
    }
   case 23:
    if (!has_trait(PF_HERBIVORE) && !has_trait(PF_CARNIVORE)) {
     if (!has_trait(PF_VEGETARIAN))
      g->add_msg("The thought of eating meat suddenly disgusts you.");
     toggle_trait(PF_HERBIVORE);
     return;
    }
   case 24:
    if (!has_trait(PF_CARNIVORE) && !has_trait(PF_HERBIVORE)) {
     if (has_trait(PF_VEGETARIAN))
      g->add_msg("Despite your convictions, you crave meat.");
     else
      g->add_msg("You crave meat.");
     toggle_trait(PF_CARNIVORE);
     return;
    }
   }
  }
 }
}

int player::sight_range(int light_level)
{
 int ret = light_level;
 if ((is_wearing(itm_goggles_nv) ||has_active_bionic(bio_night_vision)) &&
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

bool player::is_in_sunlight(game *g)
{
 if (g->light_level() >= 40 && g->m.ter(posx, posy) != t_floor)
  return true;
 return false;
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

int player::hit_mon(game *g, monster *z)
{
 bool is_u = (this == &(g->u));	// Affects how we'll display messages
 int j;
 bool can_see = (is_u || g->u_see(posx, posy, j));
 std::string You  = (is_u ? "You"  : name);
 std::string Your = (is_u ? "Your" : name + "'s");
 std::string your = (is_u ? "your" : (male ? "his" : "her"));

// Types of combat (may overlap!)
 bool unarmed = false, bashing = false, cutting = false;
// Calculate our hit roll
 int numdice = 1 + weapon.type->m_to_hit + (dex_cur / 2.5) + sklevel[sk_melee];
// Are we unarmed?
 if (!is_armed() || weapon.type->id == itm_bio_claws) {
  unarmed = true;
  numdice += sklevel[sk_unarmed];
  if (sklevel[sk_unarmed] > 4)
   numdice += sklevel[sk_unarmed] - 2;	// Extra bonus for high levels
  if (has_trait(PF_DRUNKEN) && has_disease(DI_DRUNK))
   numdice += rng(0, 1) + int(disease_level(DI_DRUNK) / 300);
 } else if (has_trait(PF_DRUNKEN) && has_disease(DI_DRUNK))
  numdice += int(disease_level(DI_DRUNK) / 500);
// Using a bashing weapon?
 if (weapon.is_bashing_weapon()) {
  bashing = true;
  numdice += int(sklevel[sk_bashing] / 3);
 }
// Using a cutting weapon?
 if (weapon.is_cutting_weapon()) {
  cutting = true;
  numdice += int(sklevel[sk_cutting] / 1.5);
 }

// Recoil penalty
 if (recoil <= 30)
  recoil += 6;
// Movement cost
 moves -= (80 + 4 * weapon.volume() + 2 * weapon.weight() + encumb(bp_torso));
// Different sizes affect your chance to hit
 switch (z->type->size) {
  case MS_TINY:  numdice -= 4; break;
  case MS_SMALL: numdice -= 2; break;
  case MS_LARGE: numdice += 2; break;
  case MS_HUGE:  numdice += 4; break;
 }
 if (numdice < 1)
  numdice = 1;
 int mondice = z->type->sk_dodge;
 if (dice(numdice, 10) < dice(mondice, 10)) {	// A miss!
// Movement penalty for missing & stumbling
  int stumble_pen = weapon.volume() + int(weapon.weight() / 2);
  if (has_trait(PF_DEFT))
   stumble_pen = int(stumble_pen * .3) - 10;
  if (stumble_pen < 0)
   stumble_pen = 0;
  if (stumble_pen > 0 && (one_in(16 - str_cur) || one_in(22 - dex_cur)))
   stumble_pen = rng(0, stumble_pen);
  if (is_u) {	// Only display messages if this is the player
   if (stumble_pen >= 30)
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
  g->add_msg("The %s's electric body shocks you!", z->name().c_str());
  hurtall(rng(1, 3));
 }
// For very high hit rolls, we crit!
 bool critical_hit = (dice(numdice, 10) > dice(mondice + 4, 30));
// Basic damage = 0 to (strength / 2)
 int dam = rng(0, int(str_cur / 2));
// Bonus for strong characters
 if (str_cur > 10)
  dam += int((str_cur - 10) / 2);
// Bonus for very strong characters
 if (str_cur > 20)
  dam += str_cur - 20;
 if (has_trait(PF_DRUNKEN) && has_disease(DI_DRUNK)) {
  if (unarmed)
   dam += disease_level(DI_DRUNK) / 250;
  else
   dam += disease_level(DI_DRUNK) / 400;
 }
 if (unarmed) {// Unarmed bonuses
  dam += rng(0, sklevel[sk_unarmed]);
  if (has_trait(PF_TALONS) && z->type->armor - sklevel[sk_unarmed] < 10) {
   int z_armor = (z->type->armor - sklevel[sk_unarmed]);
   if (z_armor < 0)
    z_armor = 0;
   dam += 10 - z_armor;
  }
 } else if (rng(1, 65 - 2 * dex_cur) < sklevel[sk_unarmed]) {
// If we're not unarmed, there's still a possibility of getting in a bonus
// unarmed attack.
  switch (rng(1, 4)) {	// TODO: More unarmed bonus attacks
  case 1:
   g->add_msg("You kick the %s!", z->name().c_str());
   break;
  case 2:
   g->add_msg("You headbutt the %s!", z->name().c_str());
   break;
  case 3:
   g->add_msg("You elbow the %s!", z->name().c_str());
   break;
  case 4:
   g->add_msg("You knee the %s!", z->name().c_str());
   break;
  }
  dam += rng(1, sklevel[sk_unarmed]);
  practice(sk_unarmed, 3);
 }
// Melee skill bonus
 dam += int(pow(1.3, sklevel[sk_melee]));
// Bashing damage bonus
 int melee_dam = weapon.type->melee_dam;
 if (melee_dam > 5 + str_cur + sklevel[sk_bashing])// Cap for weak characters
  melee_dam = (5 + str_cur + melee_dam + sklevel[sk_bashing]) / 2;
 dam += rng(melee_dam / 2, melee_dam);
 if (bashing)
  dam += rng(0, sklevel[sk_bashing]) * sqrt(str_cur);
// Cutting damage bonus
 if (weapon.type->melee_cut > z->type->armor - int(sklevel[sk_cutting] / 2)) {
  int z_armor = z->type->armor - int(sklevel[sk_cutting] / 2);
  if (z_armor < 0)
   z_armor = 0;
  dam += weapon.type->melee_cut - z_armor;
 }

 bool shock_them = (has_bionic(bio_shock) && power_level >= 2 && unarmed && 
                    one_in(3));
 bool drain_them = (has_bionic(bio_heat_absorb) && power_level >= 1 &&
                    !is_armed() && z->has_flag(MF_WARM));
 bool  bite_them = (has_trait(PF_FANGS) && z->armor() < 18 &&
                    one_in(20 - dex_cur - sklevel[sk_unarmed]));
 bool  peck_them = (has_trait(PF_BEAK)  && z->armor() < 16 &&
                    one_in(15 - dex_cur - sklevel[sk_unarmed]));
 if (drain_them)
  power_level--;
 drain_them &= one_in(2);	// Only works half the time

 if (critical_hit) {
  bool headshot = (!z->has_flag(MF_NOHEAD) && !one_in(numdice));
// Second chance for shock_them, drain_them, bite_them and peck_them
  shock_them = (shock_them || (has_bionic(bio_shock) && power_level >= 2 &&
                               unarmed && !one_in(3)));
  drain_them = (drain_them || (has_bionic(bio_heat_absorb) && !is_armed() &&
                               z->has_flag(MF_WARM) && !one_in(3)));
   bite_them = ( bite_them || (has_trait(PF_FANGS) && z->armor() < 18 &&
                               one_in(5)));
   peck_them = ( peck_them || (has_trait(PF_BEAK)  && z->armor() < 16 &&
                               one_in(4)));
  if (unarmed) {
   dam += rng(2, 6) * sklevel[sk_unarmed];
   if (sklevel[sk_unarmed] > 5)
    dam += 4 * (sklevel[sk_unarmed - 3]);
   z->moves -= dam;	// Stunning blow
   if (weapon.type->id == itm_bio_claws) {
    if (sklevel[sk_cutting] >= 3)
     dam += 5;
    headshot &= z->hp < dam;
    if (headshot && can_see)
     g->add_msg("%s claws pierce the %s's skull!", Your.c_str(),
                z->name().c_str());
    else if (can_see)
     g->add_msg("%s claws stab straight through the %s!", Your.c_str(),
                z->name().c_str());
   } else if (has_trait(PF_TALONS)) {
    dam += 2;
    headshot &= z->hp < dam;
    if (headshot && can_see)
     g->add_msg("%s talons tear the %s's head open!", Your.c_str(),
                z->name().c_str());
    else if (can_see)
     g->add_msg("%s bur%s %s talons into the %s!", You.c_str(),(is_u?"y":"ies"),
                your.c_str(), z->name().c_str());
   } else {
    headshot &= z->hp < dam;
    if (headshot && can_see)
     g->add_msg("%s crush%s the %s's skull in a single blow!", 
                You.c_str(), (is_u ? "" : "es"), z->name().c_str());
    else if (can_see)
     g->add_msg("%s deliver%s a crushing punch!",You.c_str(),(is_u ? "" : "s"));
   }
  } else {	// If not unarmed...
   if (bashing) {
    dam += 2 * sklevel[sk_bashing];
    dam += 8 + (str_cur / 2);
    z->moves -= dam;	// Stunning blow
   }
   if (cutting) {
    dam += 3 * sklevel[sk_cutting];
    headshot &= z->hp < dam;
    if (headshot && can_see)
     g->add_msg("%s %s slices the %s's head off!", Your.c_str(),
                weapon.tname().c_str(), z->name().c_str());
    else if (can_see)
     g->add_msg("%s stab %s %s through the %s!", You.c_str(), your.c_str(),
                weapon.tname().c_str(), z->name().c_str());
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

 g->sound(posx, posy, 6, "");

// Glass weapons shatter sometimes
 if (weapon.made_of(GLASS) &&
     rng(0, weapon.volume() + 8) < weapon.volume() + str_cur) {
  if (can_see)
   g->add_msg("%s %s shatters!", Your.c_str(), weapon.tname().c_str());
  g->sound(posx, posy, 16, "");
// Dump its contents on the ground
  for (int i = 0; i < weapon.contents.size(); i++)
   g->m.add_item(posx, posy, weapon.contents[i]);
  hit(g, bp_arms, 1, 0, rng(0, weapon.volume() * 2));// Take damage
  if (weapon.is_two_handed(this))// Hurt left arm too, if it was big
   hit(g, bp_arms, 0, 0, rng(0, weapon.volume()));
  dam += rng(0, int(weapon.volume() * 1.5));	// Hurt the monster extra
  remove_weapon();
 }

 if (dam <= 0) {
  if (is_u)
   g->add_msg("You hit the %s, but do no damage.", z->name().c_str());
  else if (can_see)
   g->add_msg("%s's %s hits the %s, but does no damage.", You.c_str(),
              weapon.tname().c_str(), z->name().c_str());
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
             (weapon.type->id == 0 ? "fists" : weapon.tname().c_str()));
 practice(sk_melee, rng(5, 10));
 if (unarmed)
  practice(sk_unarmed, rng(5, 10));
 if (bashing)
  practice(sk_bashing, rng(5, 10));
 if (cutting)
  practice(sk_cutting, rng(5, 10));

 return dam;
}

bool player::hit_player(player &p, body_part &bp, int &hitdam, int &hitcut)
{
 if (p.is_npc()) {
  npc *foe = dynamic_cast<npc*>(&p);
  if (foe->attitude != NPCATT_FLEE)
   foe->attitude = NPCATT_KILL;
 }
 bool unarmed = false, bashing = false, cutting = false;
 int numdice = weapon.type->m_to_hit + (dex_cur / 2.5) + sklevel[sk_melee];
 if (!is_armed() || weapon.type->id == itm_bio_claws) {	// Unarmed
  unarmed = true;
  numdice += sklevel[sk_unarmed];
  if (has_trait(PF_DRUNKEN) && has_disease(DI_DRUNK))
   numdice += rng(0, 1) + int(disease_level(DI_DRUNK) / 300);
 } else if (has_trait(PF_DRUNKEN) && has_disease(DI_DRUNK))
  numdice += int(disease_level(DI_DRUNK) / 500);
 if (weapon.is_bashing_weapon()) {
  bashing = true;
  numdice += int(sklevel[sk_bashing] / 3);
 }
 if (weapon.is_cutting_weapon()) {
  cutting = true;
  numdice += int(sklevel[sk_cutting] / 1.5);
 }
 if (p.str_max >= 17)
  numdice++;	// Minor bonus against huge people
 else if (p.str_max <= 5)
  numdice--;	// Minor penalty against tiny people
 int hitit = dice(numdice, 10) - dice(p.dodge(), 6);
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
 
 // Unarmed: 0 to (strength / 2)
 hitdam = rng(0, int(str_cur / 2));
 if (unarmed) {// Unarmed bonuses
  hitdam += rng(0, sklevel[sk_unarmed]);
  if (sklevel[sk_unarmed] >= 5)
   hitdam += rng(sklevel[sk_unarmed], 3 * sklevel[sk_unarmed]);
  if (has_trait(PF_TALONS))
   hitcut += 10;
  if (sklevel[sk_unarmed] >= 8 && (one_in(3) ||
                                   rng(5, 20) < sklevel[sk_unarmed]))
   hitdam *= rng(2, 3);
 }
 // Weapon adds (melee_dam / 2) to (melee_dam)
 hitdam += rng(weapon.type->melee_dam / 2, weapon.type->melee_dam);
 if (bashing)
  hitdam += rng(0, sklevel[sk_bashing]) * sqrt(str_cur);

 hitdam += int(pow(1.5, sklevel[sk_melee]));
 hitcut = weapon.type->melee_cut;
 if (hitcut > 0)
  hitcut += int(sklevel[sk_cutting] / 3);
 if (hitdam < 0) hitdam = 0;
 if (hitdam > 0 || hitcut > 0) {	// Practicing
  practice(sk_melee, rng(5, 10));
  if (unarmed)
   practice(sk_unarmed, rng(5, 10));
  if (bashing)
   practice(sk_bashing, rng(5, 10));
  if (cutting)
   practice(sk_cutting, rng(5, 10));
 } else {
  practice(sk_melee, rng(2, 5));
  if (!is_armed())
   practice(sk_unarmed, 2);
  if (weapon.is_bashing_weapon())
   practice(sk_bashing, 2);
  if (weapon.is_cutting_weapon())
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
 practice(sk_dodge, 9);
 int ret = 4 + (dex_cur / 2);
 if (moves < 0)	// We did something else, and thus cannot dodge quite as well!
  ret = 0;
 ret += sklevel[sk_dodge];
 ret -= (encumb(bp_legs) / 2) + encumb(bp_torso);
 if (!can_dodge)	// We already dodged
  ret = rng(0, ret);
 can_dodge = false;
 return ret;
}

int player::throw_range(char ch)
{
 item tmp;
 if (weapon.invlet == ch)
  tmp = weapon;
 else {
  int i;
  for (i = 0; i < inv.size(); i++) {
   if (inv[i].invlet == ch) {
    tmp = inv[i];
    i = inv.size() + 1;
   }
  }
  if (i == inv.size())
   return -1;
 }

 if (tmp.weight() > int(str_cur * 15))
  return 0;
 int ret = int((str_cur * 8) / (tmp.weight() > 0 ? tmp.weight() : 10));
 ret -= int(tmp.volume() / 10);
 if (ret < 1)
  return 1;
 if (ret > str_cur * 2)
  return str_cur * 2;
 return ret;
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
     (hp_cur[hp_head] < 25 || hp_cur[hp_torso] < 15 || pain > 50))
  add_disease(DI_ADRENALINE, 600, g);
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
     (hp_cur[hp_head] < 25 || hp_cur[hp_torso] < 15 || pain > 50))
  add_disease(DI_ADRENALINE, 600, g);
}

void player::heal(body_part bpheal, int side, int dam)
{
 hp_part healpart;
 switch (bpheal) {
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
  if (side == 0 || side == 3)
   healpart = hp_arm_l;
  if (side == 1 || side == 3)
   healpart = hp_arm_r;
 break;
 case bp_feet:
// Shouldn't happen, but fall through to legs
  debugmsg("Heal against feet!");
 case bp_legs:
  if (side == 0 || side == 3)
   healpart = hp_leg_l;
  if (side == 1 || side == 3)
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
 activity.type = ACT_NULL;
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
 int strength2 = strength;
 int timer = 1200;
 if (has_trait(PF_ADDICTIVE)) {
  strength2 = int(strength * 1.5);
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
   if ((rng(0, strength2) > rng(0, addictions[i].intensity * 5) ||
       rng(0, 500) < strength2) && addictions[i].intensity < 20)
    addictions[i].intensity++;
   return;
  }
 }
 if (rng(0, 100) < strength2) {
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
     add_morale(MOR_FEEL_GOOD);
    else
     add_morale(MOR_FEEL_BAD);
   }
  }
  if (has_trait(PF_SCHIZOPHRENIC) && one_in(2400)) { // Every 4 hours or so
   monster phantasm;
   int i;
   switch(rng(0, 10)) {
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
     add_morale(MOR_FEEL_BAD);
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
    add_morale(MOR_MOODSWING_BAD);
   else			// 45% chance
    add_morale(MOR_MOODSWING_GOOD);
  }
  if (has_trait(PF_VOMITOUS) && (one_in(4200) ||
                                 (has_trait(PF_WEAKSTOMACH) && one_in(4200))))
   vomit(g);
 }	// Done with while-awake-only effects
 if (has_trait(PF_ASTHMA) && one_in(3600 - stim * 50)) {
  add_disease(DI_ASTHMA, 50 * rng(1, 4), g);
  if (underwater)
   oxygen = int(oxygen / 2);
  if (has_disease(DI_SLEEP)) {
   rem_disease(DI_SLEEP);
   g->add_msg("Your asthma wakes you up!");
  }
 }
 if (has_trait(PF_ALBINO) && is_in_sunlight(g) && one_in(20)) {
  g->add_msg("The sunlight burns your skin!");
  if (has_disease(DI_SLEEP)) {
   rem_disease(DI_SLEEP);
   g->add_msg("You wake up!");
  }
  hurtall(1);
 }
 if (has_trait(PF_TROGLO) && is_in_sunlight(g)) {
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
  if (g->m.radiation(posx, posy) < 20)
   g->m.radiation(posx, posy)++;
  if (radiation < 50)
   radiation++;
 }
 if (has_trait(PF_UNSTABLE) && one_in(14400))	// Average once a day
  mutate(g);
 radiation += rng(0, g->m.radiation(posx, posy) / 10);
 if (rng(1, 1000) < radiation && rng(1, 1000) < radiation) {
  mutate(g);
  if (radiation > 2000)
   radiation = 2000;
  radiation /= 2;
  radiation -= 5;
  if (radiation < 0)
   radiation = 0;
 }
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
 hunger += 50;
 thirst += 60;
 for (int i = 0; i < illness.size(); i++) {
  if (illness[i].type == DI_FOODPOISON)
   illness[i].duration -= 600;
  else if (illness[i].type == DI_DRUNK)
   illness[i].duration -= rng(0, 5) * 100;
  if (illness[i].duration < 0)
   rem_disease(DI_FOODPOISON);
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
 for (int i = 0; i < worn.size(); i++) {
  ret += worn[i].weight();
 }
 for (int i = 0; i < inv.size(); i++) {
  ret += inv[i].weight();
 }
 return ret;
}

int player::volume_carried()
{
 int ret = 0;
 for (int i = 0; i < inv.size(); i++)
  ret += inv[i].volume();
 return ret;
}

int player::weight_capacity()
{
 int ret = 400 + str_cur * 35;
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
  if (pen > 30)
   pen = 30;
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
  ret = int(ret / 2);

 return ret;
}

void player::add_morale(morale_type type, int bonus, int duration)
{
 bool placed = false;
 if (bonus == -1)
  bonus = morale_data[type].top_bonus;
 if (duration == -1)
  duration = morale_data[type].top_duration;

 for (int i = 0; i < morale.size() && !placed; i++) {
  if (morale[i].type == type) {
   morale[i].up_bonus(bonus);
   morale[i].up_duration(duration);
   placed = true;
  }
 }
 if (!placed) { // Didn't increase an existing point, so add a new one
  morale_point tmp(type, bonus, duration);
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
 } else
  inv.push_back(it);
}

bool player::has_active_item(itype_id id)
{
 for (int i = 0; i < inv.size(); i++) {
  if (inv[i].type->id == id && inv[i].active)
   return true;
 }
 return false;
}

void player::process_active_items(game *g)
{
 it_tool* tmp;
 iuse use;
 if (weapon.active) {
  tmp = dynamic_cast<it_tool*>(weapon.type);
  (use.*tmp->use)(g, &weapon, true);
  weapon.charges -= tmp->charges_per_sec;
  if (weapon.charges <= 0) {
   (use.*tmp->use)(g, &weapon, false);
   if (tmp->revert_to == itm_null)
    weapon = ret_null;
   else
    weapon.type = g->itypes[tmp->revert_to];
  }
 }
 for (int i = 0; i < inv.size(); i++) {
  if (inv[i].active) {
   tmp = dynamic_cast<it_tool*>(inv[i].type);
   (use.*tmp->use)(g, &inv[i], true);
   inv[i].charges -= tmp->charges_per_sec;
   if (inv[i].charges <= 0) {
    (use.*tmp->use)(g, &inv[i], false);
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
 int cur_item = 0;
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

 while (used < quantity && cur_item < inv.size()) {
  if (inv[cur_item].type->id == it) { 
   if (inv[cur_item].is_ammo() || inv[cur_item].is_tool()) {
    if (quantity < inv[cur_item].charges) {
     used = quantity;
     inv[cur_item].charges -= quantity;
    } else {
     used += inv[cur_item].charges;
     inv[cur_item].charges = 0;
    }
    if (inv[cur_item].charges <= 0) {
     used += inv[cur_item].charges;
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
     used = quantity;
     inv[cur_item].contents[0].charges -= quantity;
    } else {
     used += inv[cur_item].contents[0].charges;
     inv[cur_item].contents[0].charges = 0;
    }
    if (inv[cur_item].contents[0].charges <= 0) {
     used += inv[cur_item].contents[0].charges;
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
  if (inv[i].type->id == it) { 
   used += inv[i].charges;
   if (used > quantity)
    inv[i].charges = used - quantity;
   else
    inv[i].charges = 0;
  } else if (inv[i].contents.size() > 0 &&
             inv[i].contents[0].type->id == it) {
   used += inv[i].contents[0].charges;
   if (used > quantity)
    inv[i].contents[0].charges = used - quantity;
   else
    inv[i].contents[0].charges = 0;
  }
 }
}

int player::butcher_factor()
{
 int lowest_factor = 999;
 for (int i = 0; i < inv.size(); i++) {
  if (inv[i].type->melee_cut >= 10) {
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
  if (weapon.is_ammo() || weapon.is_tool())
   i_have += weapon.charges;
  else
   i_have++;
 } else if (weapon.contents.size() > 0 && weapon.contents[0].type->id == it) {
  if (weapon.contents[0].is_ammo() || weapon.contents[0].is_tool())
   i_have += weapon.contents[0].charges;
  else
   i_have++;
 }
 if (i_have >= quantity)
  return true;
 for (int i = 0; i < inv.size(); i++) {
  if (inv[i].type->id == it) {
   if (inv[i].is_ammo())
    i_have += inv[i].charges;
   else
    i_have++;
  } else if (inv[i].contents.size() > 0 && inv[i].contents[0].type->id == it) {
   if (inv[i].contents[0].is_ammo() || inv[i].contents[0].is_tool())
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

bool player::eat(game *g, char let)
{
 it_comest *tmp = NULL;
 item *eaten = NULL;
 int which = -3; // Helps us know how to delete the item which got eaten
 if (weapon.invlet == let && weapon.is_food(this)) {
  if (weapon.is_food())
   tmp = dynamic_cast<it_comest*>(weapon.type);
  eaten = &weapon;
  which = -1;
 } else if (weapon.invlet == let && weapon.is_food_container(this)) {
  if (weapon.contents[0].is_food())
   tmp = dynamic_cast<it_comest*>(weapon.contents[0].type);
  eaten = &weapon.contents[0];
  which = -2;
 } else {
  for (int i = 0; i < inv.size(); i++) {
   if (inv[i].invlet == let && inv[i].is_food(this)) {
    if (inv[i].is_food())
     tmp = dynamic_cast<it_comest*>(inv[i].type);
    eaten = &inv[i];
    which = i;
   } else if (inv[i].invlet == let && inv[i].is_food_container(this)) {
    if (inv[i].contents[0].is_food())
     tmp = dynamic_cast<it_comest*>(inv[i].contents[0].type);
    eaten = &(inv[i].contents[0]);
    which = i + inv.size();
   }
  }
 }
 if (eaten == NULL)
  return false;
 if (eaten->is_ammo()) {
  charge_power(eaten->charges / 20);
  eaten->charges = 0;
  return true;
 } else if (!eaten->type->is_food() && !eaten->is_food_container(this)) {
  int charge = (eaten->volume() + eaten->weight()) / 2;
  if (eaten->type->m1 == LEATHER || eaten->type->m2 == LEATHER)
   charge /= 4;
  if (eaten->type->m1 == WOOD    || eaten->type->m2 == WOOD)
   charge /= 2;
  charge_power(charge);
  return true;
 } else {
  if (tmp == NULL) {
   debugmsg("player::eat a %s; tmp is NULL!", eaten->tname().c_str());
   return false;
  }
  if (tmp != NULL && tmp->tool != itm_null && !has_amount(tmp->tool, 1)) {
   g->add_msg("You need a %s to consume that!",
              g->itypes[tmp->tool]->name.c_str());
   return false;
  }
  if (hunger < 0 && tmp->nutr > 0 && !has_trait(PF_GOURMAND) &&
      !query_yn("You're full.  Force yourself to eat?"))
   return false;
  else if (thirst < 0 && tmp->quench > 0 && !has_trait(PF_GOURMAND) &&
           !query_yn("You're full.  Force yourself to drink?"))
   return false;
  bool hunger_danger = (!has_trait(PF_GOURMAND) && hunger < 0);
  if (has_trait(PF_CARNIVORE) && eaten->made_of(VEGGY)) {
   g->add_msg("You can only eat meat!");
   return false;
  }
  if (has_trait(PF_VEGETARIAN) && eaten->made_of(FLESH) &&
      !query_yn("Really eat that meat? (The poor animals!)"))
   return false;
  if (tmp->spoils != 0 && (g->turn - eaten->bday) / 600 > tmp->spoils) {
// We're only warned if we're a supertaster, OR the food is very old
   if ((!has_trait(PF_SUPERTASTER) &&
        (g->turn - eaten->bday) / 600 < tmp->spoils * 1.5) ||
       query_yn("This %s smells awful!  Eat it?", eaten->tname().c_str())) {
    g->add_msg("Ick, this %s doesn't taste so good...",eaten->tname().c_str());
    if (!has_bionic(bio_digestion) || one_in(3))
     add_disease(DI_FOODPOISON, rng(60, (tmp->quench + tmp->nutr + 1) * 60), g);
    hunger -= rng(0, tmp->nutr);
    if (!has_bionic(bio_digestion))
     health -= 3;
   } else
    return false;
  } else {
   hunger -= tmp->nutr;
   thirst -= tmp->quench;
   if (has_bionic(bio_digestion))
    hunger -= rng(0, tmp->nutr);
   else if (!has_trait(PF_GOURMAND)) {
    if ((hunger_danger && rng(-200, 0) > hunger))
     vomit(g);
   }
   health += tmp->healthy;
  }
  if (eaten->made_of(LIQUID))
   g->add_msg("You drink your %s.", eaten->tname().c_str());
  else if (tmp->nutr >= 5)
   g->add_msg("You eat your %s.", eaten->tname().c_str());
/*
  else
   g->add_msg("You consume your %s.", eaten->tname().c_str());
*/
  if (g->itypes[tmp->tool]->is_tool())
   use_charges(tmp->tool, 1); // Tools like lighters get used
  if (tmp->stim > 0 && stim < tmp->stim * 3)
   stim += tmp->stim;
 
  iuse use;
  (use.*tmp->use)(g, eaten, false);
  add_addiction(tmp->add, tmp->addict);
  if (has_bionic(bio_ethanol) && tmp->use == &iuse::alcohol)
   charge_power(rng(20, 30));

  if (has_trait(PF_VEGETARIAN) && eaten->made_of(FLESH)) {
   g->add_msg("You feel bad about eating this meat...");
   add_morale(MOR_FOOD_VEGETARIAN);
  }
  if (has_trait(PF_HERBIVORE) && eaten->made_of(FLESH)) {
   if (!one_in(3))
    vomit(g);
   if (tmp->quench >= 2)
    thirst += int(tmp->quench / 2);
   if (tmp->nutr >= 2)
    hunger += int(tmp->nutr * .75);
  }
  if (has_trait(PF_GOURMAND)) {
   if (tmp->fun < -2)
    add_morale(MOR_FOOD_BAD, tmp->fun);
   else if (tmp->fun > 0)
    add_morale(MOR_FOOD_GOOD, tmp->fun * 1.5);
   if (hunger < -60 || thirst < -60)
    g->add_msg("You can't finish it all!");
   if (hunger < -60)
    hunger = -60;
   if (thirst < -60)
    thirst = -60;
  } else {
   if (tmp->fun < 0)
    add_morale(MOR_FOOD_BAD, tmp->fun);
   else if (tmp->fun > 0)
    add_morale(MOR_FOOD_GOOD, tmp->fun);
   if (hunger < -20 || thirst < -20)
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
   g->add_msg("You are now wielding an empty %s.", weapon.tname().c_str());
  } else if (which >= 0 && which < inv.size())
   i_remn(which);
  else if (which >= inv.size()) {
   which -= inv.size();
   inv[which].contents.erase(inv[which].contents.begin());
   g->add_msg("%c - an empty %s", inv[which].invlet,
                                  inv[which].tname().c_str());
   inv_sorted = false;
  }
 }
 return true;
}

bool player::wield(game *g, char let)
{
 if (weapon.type->id == itm_bio_claws) {
  g->add_msg("You cannot unwield your claws!  Withdraw them with 'p'.");
  return false;
 }
 if (let == '-') {
  if (!is_armed()) {
   g->add_msg("You are already wielding nothing.");
   return false;
  } else if (volume_carried() + weapon.volume() < volume_capacity()) {
   inv.push_back(remove_weapon());
   inv_sorted = false;
   return true;
  } else if (query_yn("No space in inventory for your %s.  Drop it?",
                      weapon.tname().c_str())) {
    g->m.add_item(posx, posy, remove_weapon());
    return true;
  } else {
    return false;
  }
 }
 for (int i = 0; i < inv.size(); i++) {
  if (inv[i].invlet == let) {
   if (inv[i].is_two_handed(this) && !has_two_arms()) {
    g->add_msg("You cannot wield a %s with only one arm.",
               inv[i].tname().c_str());
    return false;
   }
   if (!is_armed()) {
    weapon = inv[i];
    i_remn(i);
    return true;
   } else if (volume_carried() + weapon.volume() - inv[i].volume() <
              volume_capacity()) {
    inv.push_back(remove_weapon());
    weapon = inv[i];
    i_remn(i);
    inv_sorted = false;
    return true;
   } else if (query_yn("No space in inventory for your %s.  Drop it?",
                       weapon.tname().c_str())) {
    g->m.add_item(posx, posy, remove_weapon());
    weapon = inv[i];
    i_remn(i);
    inv_sorted = false;
    return true;
   } else {
    return false;
   }
  }
 }
 if (weapon.invlet == let)
  g->add_msg("You are already wielding your %s.", weapon.tname().c_str());
 else
  g->add_msg("You do not have that item.");
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
  g->add_msg("Putting on a %s would be tricky.", to_wear->tname().c_str());
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
  g->add_msg("You cannot put %s over your webbed hands.", armor->name.c_str());
  return false;
 }
 if (armor->covers & mfb(bp_feet) && wearing_something_on(bp_feet)) {
  g->add_msg("You're already wearing footwear!");
  return false;
 }
 g->add_msg("You put on your %s.", to_wear->tname().c_str());
 worn.push_back(*to_wear);
 if (index == -2)
  weapon = ret_null;
 else
  inv.erase(inv.begin() + index);
 for (body_part i = bp_head; i < num_bp; i = body_part(i + 1)) {
  std::string verb = "are";
  if (i == bp_head || i == bp_torso)
   verb = "is";
  if (encumb(i) >= 4)
   g->add_msg("Your %s %s very encumbered!",
              body_part_name(body_part(i), 2).c_str(),
              (i == bp_head || i == bp_torso ? "is" : "are"));
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
                       worn[i].tname().c_str())) {
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
   (use.*tool->use)(g, used, false);
   used->charges -= tool->charges_per_use;
  } else
   g->add_msg("Your %s doesn't have enough charges.", used->tname().c_str());
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
   g->add_msg("That %s is not a gun.", gun->tname().c_str());
   return;
  }
  it_gun* guntype = dynamic_cast<it_gun*>(gun->type);
  if (guntype->skill_used == sk_pistol && !mod->used_on_pistol) {
   g->add_msg("That %s cannot be attached to a handgun.",used->tname().c_str());
   return;
  } else if (guntype->skill_used == sk_shotgun && !mod->used_on_shotgun) {
   g->add_msg("That %s cannot be attached to a shotgun.",used->tname().c_str());
   return;
  } else if (guntype->skill_used == sk_smg && !mod->used_on_smg) {
   g->add_msg("That %s cannot be attached to a submachine gun.",
              used->tname().c_str());
   return;
  } else if (guntype->skill_used == sk_rifle && !mod->used_on_rifle) {
   g->add_msg("That %s cannot be attached to a rifle.", used->tname().c_str());
   return;
  } else if (mod->acceptible_ammo_types != 0 &&
             !(mfb(guntype->ammo) & mod->acceptible_ammo_types)) {
   g->add_msg("That %s cannot be used on a %s gun.", used->tname().c_str(),
              ammo_name(guntype->ammo).c_str());
   return;
  } else if (gun->contents.size() >= 4) {
   g->add_msg("Your %s already has 4 mods installed!  To remove the mods,\
press 'U' while wielding the unloaded gun.", gun->tname().c_str());
   return;
  }
  for (int i = 0; i < gun->contents.size(); i++) {
   if (gun->contents[i].type->id == used->type->id) {
    g->add_msg("Your %s already has a %s.", gun->tname().c_str(),
               used->tname().c_str());
    return;
   } else if (mod->newtype != AT_NULL &&
        (dynamic_cast<it_gunmod*>(gun->contents[i].type))->newtype != AT_NULL) {
    g->add_msg("Your %s's caliber has already been modified.",
               gun->tname().c_str());
    return;
   } else if ((mod->id == itm_barrel_big || mod->id == itm_barrel_small) &&
              (gun->contents[i].type->id == itm_barrel_big ||
               gun->contents[i].type->id == itm_barrel_small)) {
    g->add_msg("Your %s already has a barrel replacement",
               gun->tname().c_str());
    return;
   } else if ((mod->id == itm_clip || mod->id == itm_clip2) &&
              gun->clip_size() <= 2) {
    g->add_msg("You can not extend the ammo capacity of your %s",
               gun->tname().c_str());
    return;
   }
  }
  g->add_msg("You attach the %s to your %s.", used->tname().c_str(),
             gun->tname().c_str());
  gun->contents.push_back(i_rem(let));
 } else if (used->is_food() || used->is_food_container())
  eat(g, let);
 else if (used->is_book())
  read(g, let);
 else if (used->is_armor())
  wear(g, let);
 else
  g->add_msg("You can't do anything interesting with your %s.",
             used->tname().c_str());
  return;
}

void player::read(game *g, char ch)
{
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
 if (g->light_level() <= 2) {
  g->add_msg("It's too dark to read!");
  return;
 } else if (index == -1) {
  g->add_msg("You do not have that item.");
  return;
 } else if ((index >=  0 && !inv[index].is_book()) ||
            (index == -2 && !weapon.is_book())) {
  g->add_msg("Your %s is not good reading material.",
           (index == -2 ? weapon.tname().c_str() : inv[index].tname().c_str()));
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
 } else if (tmp->intel > int_cur + sklevel[tmp->type]) {
  g->add_msg("This book is way too complex for you to understand.");
  return;
 } else if (tmp->req > sklevel[tmp->type]) {
  g->add_msg("The %s-related jargon flies over your head!",
             skill_name(tmp->type).c_str());
  return;
 }
 int time = tmp->time * 1000;	// tmp->time is in minutes; 1000 move points
				// is one minute.
 time -= int_cur * 250;
 if (has_trait(PF_FASTREADER))
  time /= 2;
 if (time < 1000)
  time = 1000;
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
  sleepy -= 4;
 if (g->m.ter(posx, posy) == t_bed)
  sleepy += 5;
 else if (g->m.ter(posx, posy) == t_floor)
  sleepy += 1;
 else if (g->m.ter(posx, posy) != t_grass)
  sleepy -= g->m.move_cost(posx, posy);
 sleepy += (fatigue - 192) / 12;
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
  armor = dynamic_cast<it_armor*>(worn[i].type);
  if (armor->covers & mfb(bp) ||
      (bp == bp_torso && (armor->covers & mfb(bp_arms)))) {
   ret += armor->encumber;
   if (armor->encumber > 0 || bp != bp_torso)
    layers++;
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
        worn[i].made_of(COTTON) || worn[i].made_of(GLASS) ||
        worn[i].made_of(WOOD)) &&
       rng(0, tmp->cut_resist * 2) < cut && !one_in(cut))
    worn[i].damage++;
// Kevlar, plastic, iron, steel, and silver may be damaged by BASHING damage
   if ((worn[i].made_of(KEVLAR) || worn[i].made_of(PLASTIC) ||
        worn[i].made_of(IRON)   || worn[i].made_of(STEEL) ||
        worn[i].made_of(SILVER) || worn[i].made_of(STONE)) &&
       rng(0, tmp->dmg_resist * 2) < dam && !one_in(dam))
    worn[i].damage++;
   if (worn[i].damage >= 5) {
    g->add_msg("Your %s is completely destroyed!", worn[i].tname().c_str());
    worn.erase(worn.begin() + i);
   }
  }
 }
 dam -= arm_bash;
 cut -= arm_cut;
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
  if ((dynamic_cast<it_armor*>(worn[i].type))->covers & mfb(bp))
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
 for (int i = 0; i < amount; i++) {
  if ((savant == sk_null || savant == s || !one_in(2)) &&
      (has_trait(PF_FASTLEARNER) || one_in((2 + sklevel[s]) / 2)) &&
      (one_in((2 + sklevel[s]) / 2) || rng(0, 50) < int_cur) &&
      rng(8, 16) > sklevel[s])
   skexercise[s]++;
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

