#include "player.h"
#include "profession.h"
#include "item_factory.h"
#include "input.h"
#include "output.h"
#include "rng.h"
#include "keypress.h"
#include "game.h"
#include "options.h"
#include "catacharset.h"
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <fstream>
#include <sstream>

// Colors used in this file: (Most else defaults to c_ltgray)
#define COL_STAT_ACT		c_ltred    // Selected stat
#define COL_TR_GOOD		c_green    // Good trait descriptive text
#define COL_TR_GOOD_OFF_ACT		c_ltgray  // A toggled-off good trait
#define COL_TR_GOOD_ON_ACT		c_ltgreen    // A toggled-on good trait
#define COL_TR_GOOD_OFF_PAS		c_dkgray  // A toggled-off good trait
#define COL_TR_GOOD_ON_PAS		c_green    // A toggled-on good trait
#define COL_TR_BAD		c_red      // Bad trait descriptive text
#define COL_TR_BAD_OFF_ACT		c_ltgray    // A toggled-off bad trait
#define COL_TR_BAD_ON_ACT		c_ltred      // A toggled-on bad trait
#define COL_TR_BAD_OFF_PAS		c_dkgray    // A toggled-off bad trait
#define COL_TR_BAD_ON_PAS		c_red      // A toggled-on bad trait
#define COL_SKILL_USED		c_green    // A skill with at least one point

#define HIGH_STAT 14 // The point after which stats cost double

#define NEWCHAR_TAB_MAX 4 // The ID of the rightmost tab

void draw_tabs(WINDOW* w, std::string sTab);

int set_stats(WINDOW* w, game* g, player *u, character_type type, int &points);
int set_traits(WINDOW* w, game* g, player *u, character_type type, int &points, int max_trait_points);
int set_profession(WINDOW* w, game* g, player *u, character_type type, int &points);
int set_skills(WINDOW* w, game* g, player *u, character_type type, int &points);
int set_description(WINDOW* w, game* g, player *u, character_type type, int &points);

bool is_char_allowed(char ch);
int random_skill();

int calc_HP(int strength, bool tough);

void save_template(player *u);

bool player::create(game *g, character_type type, std::string tempname)
{
 weapon = item(g->itypes["null"], 0);

 g->u.prof = profession::generic();

 WINDOW* w = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                    (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY-FULL_SCREEN_HEIGHT)/2 : 0,
                    (TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0);

 int tab = 0, points = 38, max_trait_points = 12;
 if (type != PLTYPE_CUSTOM) {
  switch (type) {
   case PLTYPE_NOW:
    g->u.male = (rng(1,100)>50);
    g->u.pick_name();
   case PLTYPE_RANDOM:
   {
    str_max = rng(6, 12);
    dex_max = rng(6, 12);
    int_max = rng(6, 12);
    per_max = rng(6, 12);
    points = points - str_max - dex_max - int_max - per_max;
    if (str_max > HIGH_STAT)
     points -= (str_max - HIGH_STAT);
    if (dex_max > HIGH_STAT)
     points -= (dex_max - HIGH_STAT);
    if (int_max > HIGH_STAT)
     points -= (int_max - HIGH_STAT);
    if (per_max > HIGH_STAT)
     points -= (per_max - HIGH_STAT);

    int num_gtraits = 0, num_btraits = 0, tries = 0;
    std::string rn = "";

    while (points < 0 || rng(-3, 20) > points) {
     if (num_btraits < max_trait_points && one_in(3)) {
      tries = 0;
      do {
       rn = random_bad_trait();
       tries++;
      } while ((has_trait(rn) || num_btraits - traits[rn].points > max_trait_points) && tries < 5);

      if (tries < 5 && !has_conflicting_trait(rn)) {
       toggle_trait(rn);
       points -= traits[rn].points;
       num_btraits -= traits[rn].points;
      }
     } else {
      switch (rng(1, 4)) {
       case 1: if (str_max > 5) { str_max--; points++; } break;
       case 2: if (dex_max > 5) { dex_max--; points++; } break;
       case 3: if (int_max > 5) { int_max--; points++; } break;
       case 4: if (per_max > 5) { per_max--; points++; } break;
      }
     }
    }
    while (points > 0) {
     switch (rng((num_gtraits < max_trait_points ? 1 : 5), 9)) {
     case 1:
     case 2:
     case 3:
     case 4:
      rn = random_good_trait();
      if (!has_trait(rn) && points >= traits[rn].points &&
          num_gtraits + traits[rn].points <= max_trait_points && !has_conflicting_trait(rn)) {
       toggle_trait(rn);
       points -= traits[rn].points;
       num_gtraits += traits[rn].points;
      }
      break;
     case 5:
      switch (rng(1, 4)) {
       case 1: if (str_max < HIGH_STAT) { str_max++; points--; } break;
       case 2: if (dex_max < HIGH_STAT) { dex_max++; points--; } break;
       case 3: if (int_max < HIGH_STAT) { int_max++; points--; } break;
       case 4: if (per_max < HIGH_STAT) { per_max++; points--; } break;
      }
      break;
     case 6:
     case 7:
     case 8:
     case 9:
      rn = random_skill();

      Skill *aSkill = Skill::skill(rn);
      int level = skillLevel(aSkill);

      if (level < points) {
        points -= level + 1;
        skillLevel(aSkill).level(level + 2);
      }
      break;
     }
    }
   } break;
   case PLTYPE_TEMPLATE: {
    std::ifstream fin;
    std::stringstream filename;
    filename << "data/" << tempname << ".template";
    fin.open(filename.str().c_str());
    if (!fin.is_open()) {
     debugmsg("Couldn't open %s!", filename.str().c_str());
     return false;
    }
    std::string(data);
    getline(fin, data);
    load_info(g, data);
    points = 0;
   } break;
  }
  tab = NEWCHAR_TAB_MAX;
 } else
  points = OPTIONS["INITIAL_POINTS"];
  max_trait_points = OPTIONS["MAX_TRAIT_POINTS"];

 do {
  werase(w);
  wrefresh(w);
  switch (tab) {
   case 0: tab += set_stats      (w, g, this, type, points); break;
   case 1: tab += set_traits     (w, g, this, type, points, max_trait_points); break;
   case 2: tab += set_profession (w, g, this, type, points); break;
   case 3: tab += set_skills     (w, g, this, type, points); break;
   case 4: tab += set_description(w, g, this, type, points); break;
  }
 } while (tab >= 0 && tab <= NEWCHAR_TAB_MAX);
 delwin(w);

 if (tab < 0)
  return false;

 // Character is finalized.  Now just set up HP, &c
 for (int i = 0; i < num_hp_parts; i++) {
  hp_max[i] = calc_HP(str_max, has_trait("TOUGH"));
  hp_cur[i] = hp_max[i];
 }
 if (has_trait("HARDCORE")) {
  for (int i = 0; i < num_hp_parts; i++) {
   hp_max[i] = int(hp_max[i] * .25);
   hp_cur[i] = hp_max[i];
  }
 } if (has_trait("GLASSJAW")) {
  hp_max[hp_head] = int(hp_max[hp_head] * .80);
  hp_cur[hp_head] = hp_max[hp_head];
 }
 if (has_trait("SMELLY"))
  scent = 800;
 if (has_trait("ANDROID")) {
  bionic_id first_bio;
  do {
   first_bio = g->random_good_bionic();
  } while (bionics[first_bio]->power_cost > 10);
  add_bionic(first_bio);
  add_bionic(bionic_id(power_source_bionics[rng(0,power_source_bionics.size()-1)]));	// Power Source
  max_power_level = 10;
  power_level = 10;
 }

 if (has_trait("MARTIAL_ARTS")) {
  itype_id ma_type;
  do {
   int choice = (PLTYPE_NOW==type)? rng(1, 5) : menu(false, _("Pick your style:"),
                     _("Karate"), _("Judo"), _("Aikido"), _("Tai Chi"),
                     _("Taekwondo"), NULL);
   if (choice == 1)
    ma_type = "style_karate";
   if (choice == 2)
    ma_type = "style_judo";
   if (choice == 3)
    ma_type = "style_aikido";
   if (choice == 4)
    ma_type = "style_tai_chi";
   if (choice == 5)
    ma_type = "style_taekwondo";
   item tmpitem = item(g->itypes[ma_type], 0);
   if(PLTYPE_NOW!=type) {
       full_screen_popup(tmpitem.info(true).c_str());
   }
  } while (PLTYPE_NOW!=type && !query_yn(_("Use this style?")));
  styles.push_back(ma_type);
  style_selected=ma_type;
 }

    if (has_trait("MARTIAL_ARTS2")) {
  itype_id ma_type;
  do {
   int choice = (PLTYPE_NOW==type)? rng(1, 5) : menu(false, _("Pick your style:"),
                     _("Capoeira"), _("Krav Maga"), _("Muay Thai"),
                     _("Ninjutsu"), _("Zui Quan"), NULL);
   if (choice == 1)
    ma_type = "style_capoeira";
   if (choice == 2)
    ma_type = "style_krav_maga";
   if (choice == 3)
    ma_type = "style_muay_thai";
   if (choice == 4)
    ma_type = "style_ninjutsu";
   if (choice == 5)
    ma_type = "style_zui_quan";
   item tmpitem = item(g->itypes[ma_type], 0);
   if(PLTYPE_NOW!=type) {
     full_screen_popup(tmpitem.info(true).c_str());
   }
  } while (PLTYPE_NOW!=type && !query_yn(_("Use this style?")));
  styles.push_back(ma_type);
  style_selected=ma_type;
 }
 if (has_trait("MARTIAL_ARTS3")) {
  itype_id ma_type;
  do {
   int choice = (PLTYPE_NOW==type)? rng(1, 5) : menu(false, _("Pick your style:"),
                     _("Tiger"), _("Crane"), _("Leopard"), _("Snake"),
                     _("Dragon"), NULL);
   if (choice == 1)
    ma_type = "style_tiger";
   if (choice == 2)
    ma_type = "style_crane";
   if (choice == 3)
    ma_type = "style_leopard";
   if (choice == 4)
    ma_type = "style_snake";
   if (choice == 5)
    ma_type = "style_dragon";
   item tmpitem = item(g->itypes[ma_type], 0);
   if(PLTYPE_NOW!=type) {
     full_screen_popup(tmpitem.info(true).c_str());
   }
  } while (PLTYPE_NOW!=type && !query_yn(_("Use this style?")));
  styles.push_back(ma_type);
  style_selected=ma_type;
 }
 if (has_trait("MARTIAL_ARTS4")) {
  itype_id ma_type;
  do {
   int choice = (PLTYPE_NOW==type)? rng(1, 5) : menu(false, _("Pick your style:"),
                     _("Centipede"), _("Viper"), _("Scorpion"), _("Lizard"),
                     _("Toad"), NULL);
   if (choice == 1)
    ma_type = "style_centipede";
   if (choice == 2)
    ma_type = "style_venom_snake";
   if (choice == 3)
    ma_type = "style_scorpion";
   if (choice == 4)
    ma_type = "style_lizard";
   if (choice == 5)
    ma_type = "style_toad";
   item tmpitem = item(g->itypes[ma_type], 0);
   if(PLTYPE_NOW!=type) {
     full_screen_popup(tmpitem.info(true).c_str());
   }
  } while (PLTYPE_NOW!=type && !query_yn(_("Use this style?")));
  styles.push_back(ma_type);
  style_selected=ma_type;
 }
 ret_null = item(g->itypes["null"], 0);
 weapon = get_combat_style();


 item tmp; //gets used several times

 std::vector<std::string> prof_items = g->u.prof->items();
 for (std::vector<std::string>::const_iterator iter = prof_items.begin();
      iter != prof_items.end(); ++iter)
 {
    tmp = item(item_controller->find_template(*iter), 0);
    tmp = tmp.in_its_container(&(g->itypes));
    inv.push_back(tmp);
 }

 std::vector<addiction> prof_addictions = g->u.prof->addictions();
 for (std::vector<addiction>::const_iterator iter = prof_addictions.begin();
      iter != prof_addictions.end(); ++iter)
 {
     g->u.addictions.push_back(*iter);
 }

 // Grab the skills from the profession, if there are any
 profession::StartingSkillList prof_skills = g->u.prof->skills();
 for (profession::StartingSkillList::const_iterator iter = prof_skills.begin();
      iter != prof_skills.end(); ++iter)
 {
     assert(Skill::skill(iter->first));
     if (Skill::skill(iter->first))
     {
        g->u.boost_skill_level(iter->first, iter->second);
     }
 }

 // Those who are both near-sighted and far-sighted start with bifocal glasses.
 if (has_trait("HYPEROPIC") && has_trait("MYOPIC"))
 {
    tmp = item(g->itypes["glasses_bifocal"], 0);
    inv.push_back(tmp);
 }
 // The near-sighted start with eyeglasses.
 else if (has_trait("MYOPIC"))
 {
    tmp = item(g->itypes["glasses_eye"], 0);
    inv.push_back(tmp);
 }
 // The far-sighted start with reading glasses.
 else if (has_trait("HYPEROPIC"))
 {
    tmp = item(g->itypes["glasses_reading"], 0);
    inv.push_back(tmp);
 }

// Likewise, the asthmatic start with their medication.
 if (has_trait("ASTHMA")) {
  tmp = item(g->itypes["inhaler"], 0);
  inv.push_back(tmp);
 }
// Basic starter gear, added independently of profession.
 tmp = item(g->itypes["pockknife"], 0);
  inv.push_back(tmp);
 tmp = item(g->itypes["matches"], 0);
  inv.push_back(tmp);
// make sure we have no mutations
 for (std::map<std::string, trait>::iterator iter = traits.begin(); iter != traits.end(); ++iter)
  if (!has_base_trait(iter->first))
	my_mutations[iter->first] = false;

	// Equip any armor from our inventory.
 // If we are unable to wear some of it due to encumberance, it will silently fail.
    std::vector<item*> tmp_inv;
    inv.dump(tmp_inv);

    for(std::vector<item*>::iterator i = tmp_inv.begin(); i != tmp_inv.end(); ++i)
    {
        if( (*i)->is_armor())
        {
            if( (*i)->has_flag("VARSIZE"))
            {
                (*i)->item_tags.insert("FIT");
            }
            // It might be more elegant to use player::wear_item,
            // but then we have to implement our own inventory removal.
            wear(g, (*i)->invlet, false);
        }
    }

 // Ensure that persistent morale effects (e.g. Optimist) are present at the start.
 apply_persistent_morale();
 return true;
}

void draw_tabs(WINDOW* w, std::string sTab)
{
 for (int i = 1; i < 79; i++) {
  mvwputch(w, 2, i, c_ltgray, LINE_OXOX);
  mvwputch(w, 4, i, c_ltgray, LINE_OXOX);
  mvwputch(w, 24, i, c_ltgray, LINE_OXOX);

  if (i > 2 && i < 24) {
   mvwputch(w, i, 0, c_ltgray, LINE_XOXO);
   mvwputch(w, i, 79, c_ltgray, LINE_XOXO);
  }
 }

 int x = 2;
 // TODO: align prettily, find how much space will be free and distribute
 draw_tab(w, x, _("STATS"), (sTab == "STATS") ? true : false);
 x += utf8_width(_("STATS")) + 5;
 draw_tab(w, x, _("TRAITS"), (sTab == "TRAITS") ? true : false);
 x += utf8_width(_("TRAITS")) + 5;
 draw_tab(w, x, _("PROFESSION"), (sTab == "PROFESSION") ? true : false);
 x += utf8_width(_("PROFESSION")) + 5;
 draw_tab(w, x, _("SKILLS"), (sTab == "SKILLS") ? true : false);
 x += utf8_width(_("SKILLS")) + 5;
 draw_tab(w, x, _("DESCRIPTION"), (sTab == "DESCRIPTION") ? true : false);

 mvwputch(w, 2,  0, c_ltgray, LINE_OXXO); // |^
 mvwputch(w, 2, 79, c_ltgray, LINE_OOXX); // ^|

 mvwputch(w, 4, 0, c_ltgray, LINE_XXXO); // |-
 mvwputch(w, 4, 79, c_ltgray, LINE_XOXX); // -|

 mvwputch(w, 24, 0, c_ltgray, LINE_XXOO); // |_
 mvwputch(w, 24, 79, c_ltgray, LINE_XOOX); // _|
}

int set_stats(WINDOW* w, game* g, player *u, character_type type, int &points)
{
    unsigned char sel = 1;
    char ch;

    draw_tabs(w, "STATS");

    mvwprintz(w, 11, 2, c_ltgray, _("j/k, 8/2, or arrows"));
    mvwprintz(w, 12, 2, c_ltgray, _(" select a statistic."));
    mvwprintz(w, 13, 2, c_ltgray, _("l, 6, or right arrow"));
    mvwprintz(w, 14, 2, c_ltgray, _(" increase the statistic."));
    mvwprintz(w, 15, 2, c_ltgray, _("h, 4, or left arrow"));
    mvwprintz(w, 16, 2, c_ltgray, _(" decrease the statistic."));
    mvwprintz(w, 18, 2, c_ltgray, _("> Takes you to the next tab."));
    mvwprintz(w, 19, 2, c_ltgray, _("< Returns you to the main menu."));

    const char clear[] = "                                              ";

    do {
        mvwprintz(w, 3, 2, c_ltgray, _("Points left:%3d"), points);
        mvwprintz(w, 3, 33, c_black, clear);
        for (int i = 6; i < 15; i++) {
            mvwprintz(w, i, 33, c_black, clear);
        }
        mvwprintz(w, 6,  2, c_ltgray, _("Strength:"));
        mvwprintz(w, 6,  16, c_ltgray, "%2d", u->str_max);
        mvwprintz(w, 7,  2, c_ltgray, _("Dexterity:"));
        mvwprintz(w, 7,  16, c_ltgray, "%2d", u->dex_max);
        mvwprintz(w, 8,  2, c_ltgray, _("Intelligence:"));
        mvwprintz(w, 8,  16, c_ltgray, "%2d", u->int_max);
        mvwprintz(w, 9,  2, c_ltgray, _("Perception:"));
        mvwprintz(w, 9,  16, c_ltgray, "%2d", u->per_max);

        switch (sel) {
        case 1:
            mvwprintz(w, 6,  2, COL_STAT_ACT, _("Strength:"));
            mvwprintz(w, 6,  16, COL_STAT_ACT, "%2d", u->str_max);
            if (u->str_max >= HIGH_STAT) {
                mvwprintz(w, 3, 33, c_ltred, _("Increasing Str further costs 2 points."));
            }
            mvwprintz(w, 6, 33, COL_STAT_ACT, _("Base HP: %d"),
                      calc_HP(u->str_max, u->has_trait("TOUGH")));
            mvwprintz(w, 7, 33, COL_STAT_ACT, _("Carry weight: %.1f %s"),
                      u->convert_weight(u->weight_capacity(false)),
                      OPTIONS["USE_METRIC_WEIGHTS"] == "kg"?"kg":"lbs");
            mvwprintz(w, 8, 33, COL_STAT_ACT, _("Melee damage: %d"),
                      u->base_damage(false));
            fold_and_print(w, 10, 33, 45, COL_STAT_ACT, _("Strength also makes you more resistant to many diseases and poisons, and makes actions which require brute force more effective."));
            break;

        case 2:
            mvwprintz(w, 7,  2, COL_STAT_ACT, _("Dexterity:"));
            mvwprintz(w, 7,  16, COL_STAT_ACT, "%2d", u->dex_max);
            if (u->dex_max >= HIGH_STAT) {
                mvwprintz(w, 3, 33, c_ltred, _("Increasing Dex further costs 2 points."));
            }
            mvwprintz(w, 6, 33, COL_STAT_ACT, _("Melee to-hit bonus: +%d"),
                      u->base_to_hit(false));
            mvwprintz(w, 7, 33, COL_STAT_ACT, _("Ranged penalty: -%d"),
                      abs(u->ranged_dex_mod(false)));
            if (u->throw_dex_mod(false) <= 0) {
                mvwprintz(w, 8, 33, COL_STAT_ACT, _("Throwing bonus: +%d"),
                          abs(u->throw_dex_mod(false)));
            } else {
                mvwprintz(w, 8, 33, COL_STAT_ACT, _("Throwing penalty: -%d"),
                          abs(u->throw_dex_mod(false)));
            }
            fold_and_print(w, 10, 33, 45, COL_STAT_ACT, _("Dexterity also enhances many actions which require finesse."));
            break;

        case 3:
            mvwprintz(w, 8,  2, COL_STAT_ACT, _("Intelligence:"));
            mvwprintz(w, 8,  16, COL_STAT_ACT, "%2d", u->int_max);
            if (u->int_max >= HIGH_STAT) {
                mvwprintz(w, 3, 33, c_ltred, _("Increasing Int further costs 2 points."));
            }
            mvwprintz(w, 6, 33, COL_STAT_ACT, _("Read times: %d%%%%"),
                      u->read_speed(false));
            mvwprintz(w, 7, 33, COL_STAT_ACT, _("Skill rust: %d%%%%"),
                      u->rust_rate(false));
            fold_and_print(w, 9, 33, 45, COL_STAT_ACT, _("Intelligence is also used when crafting, installing bionics, and interacting with NPCs."));
            break;

        case 4:
            mvwprintz(w, 9,  2, COL_STAT_ACT, _("Perception:"));
            mvwprintz(w, 9,  16, COL_STAT_ACT, "%2d", u->per_max);
            if (u->per_max >= HIGH_STAT) {
                mvwprintz(w, 3, 33, c_ltred, _("Increasing Per further costs 2 points."));
            }
                mvwprintz(w, 6, 33, COL_STAT_ACT, _("Ranged penalty: -%d"),
                      abs(u->ranged_per_mod(false)));
            fold_and_print(w, 8, 33, 45, COL_STAT_ACT, _("Perception is also used for detecting traps and other things of interest."));
            break;
        }

  wrefresh(w);
  ch = input();
  if ((ch == 'j' || ch == '2') && sel < 4)
   sel++;
  if ((ch == 'k' || ch == '8') && sel > 1)
   sel--;
  if (ch == 'h' || ch == '4'){
   if (sel == 1 && u->str_max > 4) {
    if (u->str_max > HIGH_STAT)
     points++;
    u->str_max--;
    points++;
   } else if (sel == 2 && u->dex_max > 4) {
    if (u->dex_max > HIGH_STAT)
     points++;
    u->dex_max--;
    points++;
   } else if (sel == 3 && u->int_max > 4) {
    if (u->int_max > HIGH_STAT)
     points++;
    u->int_max--;
    points++;
   } else if (sel == 4 && u->per_max > 4) {
    if (u->per_max > HIGH_STAT)
     points++;
    u->per_max--;
    points++;
   }
  }
  if ((ch == 'l' || ch == '6')) {
   if (sel == 1 && u->str_max < 20) {
    points--;
    if (u->str_max >= HIGH_STAT)
     points--;
    u->str_max++;
   } else if (sel == 2 && u->dex_max < 20) {
    points--;
    if (u->dex_max >= HIGH_STAT)
     points--;
    u->dex_max++;
   } else if (sel == 3 && u->int_max < 20) {
    points--;
    if (u->int_max >= HIGH_STAT)
     points--;
    u->int_max++;
   } else if (sel == 4 && u->per_max < 20) {
    points--;
    if (u->per_max >= HIGH_STAT)
     points--;
    u->per_max++;
   }
  }
  if (ch == '<' && query_yn(_("Return to main menu?")))
   return -1;
  if (ch == '>')
   return 1;
 } while (true);
}

int set_traits(WINDOW* w, game* g, player *u, character_type type, int &points, int max_trait_points)
{
    draw_tabs(w, "TRAITS");

    WINDOW* w_description = newwin(3, 78, 21 + getbegy(w), 1 + getbegx(w));
    // Track how many good / bad POINTS we have; cap both at MAX_TRAIT_POINTS
    int num_good = 0, num_bad = 0;

    std::vector<std::string> vStartingTraits[2];

    for (std::map<std::string, trait>::iterator iter = traits.begin(); iter != traits.end(); ++iter) {
        if (iter->second.startingtrait) {
            if (iter->second.points >= 0) {
                vStartingTraits[0].push_back(iter->first);

                if (u->has_trait(iter->first)) {
                    num_good += iter->second.points;
                }
            } else {
                vStartingTraits[1].push_back(iter->first);

                if (u->has_trait(iter->first)) {
                    num_bad += iter->second.points;
                }
            }
        }
    }

    nc_color col_on_act, col_off_act, col_on_pas, col_off_pas, hi_on, hi_off, col_tr;

    int iStartPos = 0;
    int iContentHeight = 16;
    int iCurWorkingPage = 0;

    int iCurrentLine[2];
    iCurrentLine[0] = 0;
    iCurrentLine[1] = 0;

    do {
        mvwprintz(w,  3, 2, c_ltgray, _("Points left:%3d"), points);
        mvwprintz(w,  3,18, c_ltgreen, "%2d/%d", num_good, max_trait_points);
        mvwprintz(w,  3,25, c_ltred, "%2d/%d", num_bad, max_trait_points);

        // Clear the bottom of the screen.
        werase(w_description);

        for (int iCurrentPage = 0; iCurrentPage < 2; iCurrentPage++) { //Good/Bad
            if (iCurrentPage == 0) {
                col_on_act  = COL_TR_GOOD_ON_ACT;
                col_off_act = COL_TR_GOOD_OFF_ACT;
                col_on_pas  = COL_TR_GOOD_ON_PAS;
                col_off_pas = COL_TR_GOOD_OFF_PAS;
                col_tr = COL_TR_GOOD;
                hi_on   = hilite(col_on_act);
                hi_off  = hilite(col_off_act);
            } else {
                col_on_act  = COL_TR_BAD_ON_ACT;
                col_off_act = COL_TR_BAD_OFF_ACT;
                col_on_pas  = COL_TR_BAD_ON_PAS;
                col_off_pas = COL_TR_BAD_OFF_PAS;
                col_tr = COL_TR_BAD;
                hi_on   = hilite(col_on_act);
                hi_off  = hilite(col_off_act);
            }

            if (vStartingTraits[iCurrentPage].size() > iContentHeight) {
                iStartPos = iCurrentLine[iCurrentPage] - (iContentHeight - 1) / 2;

                if (iStartPos < 0) {
                    iStartPos = 0;
                } else if (iStartPos + iContentHeight > vStartingTraits[iCurrentPage].size()) {
                    iStartPos = vStartingTraits[iCurrentPage].size() - iContentHeight;
                }
            }

            //Draw Traits
            for (int i = iStartPos; i < vStartingTraits[iCurrentPage].size(); i++) {
                if (i >= iStartPos && i < iStartPos +
                    ((iContentHeight > vStartingTraits[iCurrentPage].size()) ?
                     vStartingTraits[iCurrentPage].size() : iContentHeight)) {
                    if (iCurrentLine[iCurrentPage] == i && iCurrentPage == iCurWorkingPage) {
                        mvwprintz(w,  3, 33, c_ltgray, "                                              ");
                        mvwprintz(w,  3, 33, col_tr, _("%s earns %d points"),
                                  _(traits[vStartingTraits[iCurrentPage][i]].name.c_str()),
                                  traits[vStartingTraits[iCurrentPage][i]].points * -1);
                        fold_and_print(w_description, 0, 0, 78, col_tr, "%s",
                                       _(traits[vStartingTraits[iCurrentPage][i]].description.c_str()));
                    }

                    nc_color cLine = col_off_pas;
                    if (iCurWorkingPage == iCurrentPage) {
                        cLine = col_off_act;
                        if (iCurrentLine[iCurrentPage] == i) {
                            cLine = hi_off;

                            if (u->has_conflicting_trait(vStartingTraits[iCurrentPage][i])) {
                                cLine = hilite(c_dkgray);
                            } else if (u->has_trait(vStartingTraits[iCurrentPage][i])) {
                                cLine = hi_on;
                            }
                        } else {
                             if (u->has_conflicting_trait(vStartingTraits[iCurrentPage][i])) {
                                cLine = c_dkgray;

                            } else if (u->has_trait(vStartingTraits[iCurrentPage][i])) {
                                cLine = col_on_act;
                            }
                        }
                    } else if (u->has_trait(vStartingTraits[iCurrentPage][i])) {
                        cLine = col_on_pas;

                    } else if (u->has_conflicting_trait(vStartingTraits[iCurrentPage][i])) {
                        cLine = c_ltgray;
                    }

                    mvwprintz(w, 5 + i - iStartPos, (iCurrentPage == 0) ? 2 : 40, c_ltgray, "\
                                  ");	// Clear the line
                    mvwprintz(w, 5 + i - iStartPos, (iCurrentPage == 0) ? 2 : 40, cLine,
                              traits[vStartingTraits[iCurrentPage][i]].name.c_str());
                }
            }
        }

        wrefresh(w);
        wrefresh(w_description);
        switch (input()) {
            case 'h':
            case '4':
            case 'l':
            case '6':
            case '\t':
                if (iCurWorkingPage == 0) {
                    iCurWorkingPage = 1;
                } else {
                    iCurWorkingPage = 0;
                }
                wrefresh(w);
                break;
            case 'k':
            case '8':
                iCurrentLine[iCurWorkingPage]--;
                if (iCurrentLine[iCurWorkingPage] < 0) {
                    iCurrentLine[iCurWorkingPage] = vStartingTraits[iCurWorkingPage].size() - 1;
                }
                break;
            case 'j':
            case '2':
                iCurrentLine[iCurWorkingPage]++;
                if (iCurrentLine[iCurWorkingPage] >= vStartingTraits[iCurWorkingPage].size()) {
                    iCurrentLine[iCurWorkingPage] = 0;
                }
                break;
            case ' ':
            case '\n':
            case '5':
            {
                std::string cur_trait = vStartingTraits[iCurWorkingPage][iCurrentLine[iCurWorkingPage]];
                if (u->has_trait(cur_trait)) {

                    u->toggle_trait(cur_trait);

                    // If turning off the trait violates a profession condition,
                    // turn it back on.
                    if(u->prof->can_pick(u, 0) != "YES") {
                        u->toggle_trait(cur_trait);
                        popup(_("Your profession of %s prevents you from removing this trait."),
                              u->prof->name().c_str());

                    } else {
                        points += traits[cur_trait].points;
                        if (iCurWorkingPage == 0) {
                            num_good -= traits[cur_trait].points;
                        } else {
                            num_bad -= traits[cur_trait].points;
                        }
                    }

                } else if(u->has_conflicting_trait(cur_trait)) {
                    popup(_("You already picked a conflicting trait!"));

                } else if (iCurWorkingPage == 0 && num_good + traits[cur_trait].points >
                           max_trait_points) {
                    popup(_("Sorry, but you can only take %d points of advantages."),
                          max_trait_points);

                } else if (iCurWorkingPage != 0 && num_bad + traits[cur_trait].points <
                           -max_trait_points) {
                    popup(_("Sorry, but you can only take %d points of disadvantages."),
                          max_trait_points);

                } else {
                    u->toggle_trait(cur_trait);

                    // If turning on the trait violates a profession condition,
                    // turn it back off.
                    if(u->prof->can_pick(u, 0) != "YES") {
                        u->toggle_trait(cur_trait);
                        popup(_("Your profession of %s prevents you from taking this trait."),
                              u->prof->name().c_str());

                    } else {
                        points -= traits[cur_trait].points;
                        if (iCurWorkingPage == 0) {
                            num_good += traits[cur_trait].points;
                        } else {
                            num_bad += traits[cur_trait].points;
                        }
                    }
                }
                break;
            }
            case '<':
                return -1;
            case '>':
                return 1;
        }
    } while (true);

    return 1;
}

int set_profession(WINDOW* w, game* g, player *u, character_type type, int &points)
{
    draw_tabs(w, "PROFESSION");

    WINDOW* w_description = newwin(3, 78, 21 + getbegy(w), 1 + getbegx(w));

    int cur_id = 1;
    int retval = 0;

    //may as well stick that +1 on for convenience
    profession const** sorted_profs = new profession const*[profession::count()+1];
    for (profmap::const_iterator iter = profession::begin(); iter != profession::end(); ++iter)
    {
        profession const* prof = &(iter->second);
        sorted_profs[prof->id()] = prof;
    }

    do
    {
        int netPointCost = sorted_profs[cur_id]->point_cost() - u->prof->point_cost();
        std::string can_pick = sorted_profs[cur_id]->can_pick(u, points);

        mvwprintz(w,  3, 2, c_ltgray, _("Points left:%3d"), points);
        // Clear the bottom of the screen.
        werase(w_description);
        mvwprintz(w,  3, 40, c_ltgray, "                                      ");
        if (can_pick == "YES")
        {
            mvwprintz(w,  3, 20, c_green, _("Profession %1$s costs %2$d points (net: %3$d)"),
                      sorted_profs[cur_id]->name().c_str(), sorted_profs[cur_id]->point_cost(),
                      netPointCost);
        }
        else if(can_pick == "INSUFFICIENT_POINTS")
        {
            mvwprintz(w,  3, 20, c_ltred, _("Profession %1$s costs %2$d points (net: %3$d)"),
                      sorted_profs[cur_id]->name().c_str(), sorted_profs[cur_id]->point_cost(),
                      netPointCost);
        }
        fold_and_print(w_description, 0, 0, 78, c_green, sorted_profs[cur_id]->description().c_str());

        for (int i = 1; i < 17; ++i)
        {
            mvwprintz(w, 4 + i, 2, c_ltgray, "\
                                             ");	// Clear the line
            int id = i;
            if ((cur_id < 7) || (profession::count() < 16))
            {
                //do nothing
            }
            else if (cur_id >= profession::count() - 9)
            {
                id = profession::count() - 16 + i;
            }
            else
            {
                id += cur_id - 7;
            }

            if (id > profession::count())
            {
                break;
            }

            if (u->prof != sorted_profs[id])
            {
                mvwprintz(w, 4 + i, 2, (sorted_profs[id] == sorted_profs[cur_id] ? h_ltgray : c_ltgray),
                          sorted_profs[id]->name().c_str());
            }
            else
            {
                mvwprintz(w, 4 + i, 2,
                          (sorted_profs[id] == sorted_profs[cur_id] ?
                           hilite(COL_SKILL_USED) : COL_SKILL_USED),
                          sorted_profs[id]->name().c_str());
            }
        }

        wrefresh(w);
        wrefresh(w_description);
        switch (input())
        {
            case 'j':
            case '2':
                if (cur_id < profession::count())
                cur_id++;
            break;

            case 'k':
            case '8':
                if (cur_id > 1)
                cur_id--;
            break;

            case '\n':
            case '5':
                u->prof = profession::prof(sorted_profs[cur_id]->ident()); // we've got a const*
                points -= netPointCost;
            break;

            case '<':
                retval = -1;
            break;

            case '>':
                retval = 1;
            break;
        }
    } while (retval == 0);

    delete[] sorted_profs;
    return retval;
}

int set_skills(WINDOW* w, game* g, player *u, character_type type, int &points)
{
 draw_tabs(w, "SKILLS");

 WINDOW* w_description = newwin(3, 78, 21 + getbegy(w), 1 + getbegx(w));

 int cur_sk = 0;
 Skill *currentSkill = Skill::skill(cur_sk);

 do {
  mvwprintz(w,  3, 2, c_ltgray, _("Points left:%3d"), points);
  // Clear the bottom of the screen.
  werase(w_description);
  mvwprintz(w,  3, 40, c_ltgray, "                                    ");
  if (points >= u->skillLevel(currentSkill) + 1)
   mvwprintz(w,  3, 30, COL_SKILL_USED, _("Upgrading %s costs %d points"),
             currentSkill->name().c_str(), u->skillLevel(currentSkill) + 1);
  else
   mvwprintz(w,  3, 30, c_ltred, _("Upgrading %s costs %d points"),
             currentSkill->name().c_str(), u->skillLevel(currentSkill) + 1);
  fold_and_print(w_description, 0, 0, 78, COL_SKILL_USED, currentSkill->description().c_str());

  if (cur_sk <= 7) {
   for (int i = 0; i < 17; i++) {
     Skill *thisSkill = Skill::skill(i);

    mvwprintz(w, 5 + i, 2, c_ltgray, "\
                                             ");	// Clear the line
    if (u->skillLevel(thisSkill) == 0) {
     mvwprintz(w, 5 + i, 2, (i == cur_sk ? h_ltgray : c_ltgray),
               thisSkill->name().c_str());
    } else {
     mvwprintz(w, 5 + i, 2,
               (i == cur_sk ? hilite(COL_SKILL_USED) : COL_SKILL_USED),
               "%s ", thisSkill->name().c_str());
     for (int j = 0; j < u->skillLevel(thisSkill); j++)
      wprintz(w, (i == cur_sk ? hilite(COL_SKILL_USED) : COL_SKILL_USED), "*");
    }
   }
  } else if (cur_sk >= Skill::skills.size() - 9) {
   for (int i = Skill::skills.size() - 16; i < Skill::skills.size(); i++) {
     Skill *thisSkill = Skill::skill(i);
    mvwprintz(w, 21 + i - Skill::skills.size(), 2, c_ltgray, "\
                                             ");	// Clear the line
    if (u->skillLevel(thisSkill) == 0) {
     mvwprintz(w, 21 + i - Skill::skills.size(), 2,
               (i == cur_sk ? h_ltgray : c_ltgray), thisSkill->name().c_str());
    } else {
     mvwprintz(w, 21 + i - Skill::skills.size(), 2,
               (i == cur_sk ? hilite(COL_SKILL_USED) : COL_SKILL_USED), "%s ",
               thisSkill->name().c_str());
     for (int j = 0; j < u->skillLevel(thisSkill); j++)
      wprintz(w, (i == cur_sk ? hilite(COL_SKILL_USED) : COL_SKILL_USED), "*");
    }
   }
  } else {
   for (int i = cur_sk - 7; i < cur_sk + 9; i++) {
     Skill *thisSkill = Skill::skill(i);
    mvwprintz(w, 12 + i - cur_sk, 2, c_ltgray, "\
                                             ");	// Clear the line
    if (u->skillLevel(thisSkill) == 0) {
     mvwprintz(w, 12 + i - cur_sk, 2, (i == cur_sk ? h_ltgray : c_ltgray),
               thisSkill->name().c_str());
    } else {
     mvwprintz(w, 12 + i - cur_sk, 2,
               (i == cur_sk ? hilite(COL_SKILL_USED) : COL_SKILL_USED),
               "%s ", thisSkill->name().c_str());
     for (int j = 0; j < u->skillLevel(thisSkill); j++)
      wprintz(w, (i == cur_sk ? hilite(COL_SKILL_USED) : COL_SKILL_USED), "*");
    }
   }
  }

  wrefresh(w);
  wrefresh(w_description);
  switch (input()) {
    case 'j':
    case '2':
     if (cur_sk < Skill::skills.size() - 1)
      cur_sk++;
    currentSkill = Skill::skill(cur_sk);
    break;
    case 'k':
    case '8':
    if (cur_sk > 0)
     cur_sk--;
    currentSkill = Skill::skill(cur_sk);
    break;
    case 'h':
    case '4':
     if (u->skillLevel(currentSkill)) {
      u->skillLevel(currentSkill).level(u->skillLevel(currentSkill) - 2);
      points += u->skillLevel(currentSkill) + 1;
    }
    break;
    case 'l':
    case '6':
     if (u->skillLevel(currentSkill) <= 19) { 
      points -= u->skillLevel(currentSkill) + 1;
      u->skillLevel(currentSkill).level(u->skillLevel(currentSkill) + 2);
     }
    break;
   case '<':
    return -1;
   case '>':
    return 1;
  }
 } while (true);
}

int set_description(WINDOW* w, game* g, player *u, character_type type, int &points)
{
 if(PLTYPE_NOW==type) return 1;

 draw_tabs(w, "DESCRIPTION");

 mvwprintz(w,  3, 2, c_ltgray, _("Points left:%3d"), points);

 unsigned namebar_pos, male_pos, female_pos;
 mvwprintz(w, 6, 2, c_ltgray, _("Name:"));
 namebar_pos = 3 + utf8_width(_("Name:"));
 mvwprintz(w, 6, namebar_pos, c_ltgray, "______________________________");
 mvwprintz(w, 6, namebar_pos + 31, c_ltgray, _("(Press TAB to move off this line)"));
 mvwprintz(w, 8, 2, c_ltgray, _("Gender:"));
 male_pos = 3 + utf8_width(_("Gender:"));
 mvwprintz(w, 8, male_pos, c_ltgray, _("Male"));
 female_pos = 1 + male_pos + utf8_width(_("Male"));
 mvwprintz(w, 8, female_pos, c_ltgray, _("Female"));
 mvwprintz(w, 8, namebar_pos + 31, c_ltgray, _("(Press spacebar to toggle)"));
 fold_and_print(w, 10, 2, 76, c_ltgray,
                _("When your character is finished and you're ready to start playing, press >"));
 fold_and_print(w, 12, 2, 76, c_ltgray, _("To go back and review your character, press <"));
 fold_and_print(w, 14, 2, 76, c_green, _("To pick a random name for your character, press ?."));
 fold_and_print(w, 16, 2, 76, c_green, _("To save this character as a template, press !."));

 int line = 1;
 bool noname = false;
 long ch;

 do {
  if (u->male) {
   mvwprintz(w, 8, male_pos, c_ltred, _("Male"));
   mvwprintz(w, 8, female_pos, c_ltgray, _("Female"));
  } else {
   mvwprintz(w, 8, male_pos, c_ltgray, _("Male"));
   mvwprintz(w, 8, female_pos, c_ltred, _("Female"));
  }

  if (!noname) {
   mvwprintz(w, 6, 8, c_ltgray, "%s", u->name.c_str());
   if (line == 1)
    wprintz(w, h_ltgray, "_");
  }
  if (line == 2)
   mvwprintz(w, 8, 2, h_ltgray, _("Gender:"));
  else
   mvwprintz(w, 8, 2, c_ltgray, _("Gender:"));

  wrefresh(w);
  ch = input();
  if (noname) {
   mvwprintz(w, 6, namebar_pos, c_ltgray, "______________________________");
   noname = false;
  }

  if (ch == '>') {
   if (points < 0) {
    popup(_("Too many points allocated, change some features and try again."));
	continue;
   } else if (points > 0 &&
              !query_yn(_("Remaining points will be discarded, are you sure you want to proceed?"))) {
    continue;
   } else if (u->name.size() == 0) {
    mvwprintz(w, 6, namebar_pos, h_ltgray, _("______NO NAME ENTERED!!!!_____"));
    noname = true;
    wrefresh(w);
    if (!query_yn(_("Are you SURE you're finished? Your name will be randomly generated."))) {
     continue;
    } else {
     u->pick_name();
     return 1;
    }
   } else if (query_yn(_("Are you SURE you're finished?"))) {
    return 1;
   } else {
    continue;
   }
  } else if (ch == '<') {
   return -1;
  } else if (ch == '!') {
   if (points > 0) {
    popup(_("You cannot save a template with unused points!"));
   } else
    save_template(u);
   mvwprintz(w,12, 2, c_ltgray, _("To go back and review your character, press <"));
   wrefresh(w);
  } else if (ch == '?') {
   mvwprintz(w, 6, namebar_pos, c_ltgray, "______________________________");
   u->pick_name();
  } else {
   switch (line) {
    case 1:
     if (ch == KEY_BACKSPACE || ch == 127) {
      if (u->name.size() > 0) {
       //erease utf8 character TODO: make a function
       while(u->name.size()>0 && ((unsigned char)u->name[u->name.size()-1])>=128 &&
                                 ((unsigned char)u->name[(int)u->name.size()-1])<=191) {
           u->name.erase(u->name.size()-1);
       }
       u->name.erase(u->name.size()-1);
       mvwprintz(w, 6, namebar_pos, c_ltgray, "______________________________");
       mvwprintz(w, 6, namebar_pos, c_ltgray, "%s", u->name.c_str());
       wprintz(w, h_ltgray, "_");
      }
     } else if (ch == '\t') {
      line = 2;
      mvwprintz(w, 6, namebar_pos +  utf8_width(u->name.c_str()), c_ltgray, "_");
     } else if (is_char_allowed(ch) &&  utf8_width(u->name.c_str()) < 30) {
      u->name.push_back(ch);
     }
     else if(ch==KEY_F(2)) {
         std::string tmp = get_input_string_from_file();
         int tmplen = utf8_width(tmp.c_str());
         if(tmplen>0 && tmplen+utf8_width(u->name.c_str())<30) {
            u->name.append(tmp);
         }
     }
     //experimental unicode input
     else if(ch>127) {
         std::string tmp = utf32_to_utf8(ch);
         int tmplen = utf8_width(tmp.c_str());
         if(tmplen>0 && tmplen+utf8_width(u->name.c_str())<30) {
            u->name.append(tmp);
         }
     }
     break;
    case 2:
     if (ch == ' ')
      u->male = !u->male;
     else if (ch == 'k' || ch == '\t') {
      line = 1;
     }
     break;
   }
  }
 } while (true);
}

/**
 * Returns whether or not the given (ASCII) character is usable in a (player)
 * character's name. Only printable symbols not reserved by the filesystem are
 * permitted.
 * @param ch The char to check.
 * @return true if the char is allowed in a name, false if not.
 */
bool is_char_allowed(char ch) {

  //Allow everything EXCEPT the following reserved characters:
  return (ch > 31 //0-31 are control characters
          && ch < 127 //DEL character
          && ch != '/' && ch != '\\' //Path separators
          && ch != '?' && ch != '*' && ch != '%' //Wildcards
          && ch != ':' //Mount point/drive marker
          && ch != '|' //Output pipe
          && ch != '"' //Filename (with spaces) marker
          && ch != '>' && ch != '<'); //Input/output redirection

}

std::string player::random_good_trait()
{
    std::vector<std::string> vTraitsGood;

    for (std::map<std::string, trait>::iterator iter = traits.begin(); iter != traits.end(); ++iter) {
        if (iter->second.startingtrait && iter->second.points >= 0) {
            vTraitsGood.push_back(iter->first);
        }
    }

    return vTraitsGood[rng(0, vTraitsGood.size() - 1)];
}

std::string player::random_bad_trait()
{
    std::vector<std::string> vTraitsBad;

    for (std::map<std::string, trait>::iterator iter = traits.begin(); iter != traits.end(); ++iter) {
        if (iter->second.startingtrait && iter->second.points < 0) {
            vTraitsBad.push_back(iter->first);
        }
    }

    return vTraitsBad[rng(0, vTraitsBad.size() - 1)];
}

int random_skill()
{
  return rng(1, Skill::skills.size() - 1);
}

int calc_HP(int strength, bool tough)
{
 return (60 + 3 * strength) * (tough ? 1.2 : 1);
}

void save_template(player *u)
{
 std::string name = string_input_popup(_("Name of template:"));
 if (name.length() == 0)
  return;
 std::stringstream playerfile;
 playerfile << "data/" << name << ".template";
 std::ofstream fout;
 fout.open(playerfile.str().c_str());
 fout << u->save_info();
}
