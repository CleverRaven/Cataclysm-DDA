#include "player.h"
#include "output.h"
#include "rng.h"
#include "keypress.h"
#include "game.h"
#include <unistd.h>
#include <fstream>
#include <sstream>

// ncurses has not yet been initialized, so we need to define our line chars
#define LINE_XOXO 4194424
#define LINE_OXOX 4194417
#define LINE_XXOO 4194413
#define LINE_OXXO 4194412
#define LINE_OOXX 4194411
#define LINE_XOOX 4194410
#define LINE_XXXO 4194420
#define LINE_XXOX 4194422
#define LINE_XOXX 4194421
#define LINE_OXXX 4194423
#define LINE_XXXX 4194414

// Colors used in this file: (Most else defaults to c_ltgray)
#define COL_STAT_ACT		c_ltred    // Selected stat
#define COL_TR_GOOD		c_green    // Good trait descriptive text
#define COL_TR_GOOD_OFF		c_ltgreen  // A toggled-off good trait
#define COL_TR_GOOD_ON		c_green    // A toggled-on good trait
#define COL_TR_BAD		c_red      // Bad trait descriptive text
#define COL_TR_BAD_OFF		c_ltred    // A toggled-off bad trait
#define COL_TR_BAD_ON		c_red      // A toggled-on bad trait
#define COL_SKILL_USED		c_green    // A skill with at least one point

#define HIGH_STAT 14 // The point after which stats cost double
#define MAX_TRAIT_POINTS 12 // How many points from traits

void draw_tabs(WINDOW* w);

int set_stats(WINDOW* w, player *u, int &points);
int set_traits(WINDOW* w, player *u, int &points);
int set_skills(WINDOW* w, player *u, int &points);
int set_description(WINDOW* w, player *u, int &points);

int random_skill();

int calc_HP(int strength, bool tough);

void save_template(player *u);

bool player::create(game *g, character_type type, std::string tempname)
{
 weapon = item(g->itypes[0], 0);
 WINDOW* w = newwin(25, 80, 0, 0);
 int tab = 0, points = 38;
 if (type != PLTYPE_CUSTOM) {
  switch (type) {
   case PLTYPE_RANDOM: {
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
 
    int num_gtraits = 0, num_btraits = 0, rn, tries;
    while (points < 0 || rng(-3, 20) > points) {
     if (num_btraits < MAX_TRAIT_POINTS && one_in(3)) {
      tries = 0;
      do {
       rn = random_bad_trait();
       tries++;
      } while ((has_trait(rn) ||
              num_btraits - traits[rn].points > MAX_TRAIT_POINTS) && tries < 5);
      if (tries < 5) {
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
     switch (rng((num_gtraits < MAX_TRAIT_POINTS ? 1 : 5), 9)) {
     case 1:
     case 2:
     case 3:
     case 4:
      rn = random_good_trait();
      if (!has_trait(rn) && points >= traits[rn].points &&
          num_gtraits + traits[rn].points <= MAX_TRAIT_POINTS) {
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
      if (points >= sklevel[rn] + 1) {
       points -= sklevel[rn] + 1;
       sklevel[rn] += 2;
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
  tab = 3;
 } else
  points = 6;

 do {
  werase(w);
  draw_tabs(w);
  wrefresh(w);
  switch (tab) {
   case 0: tab += set_stats      (w, this, points); break;
   case 1: tab += set_traits     (w, this, points); break;
   case 2: tab += set_skills     (w, this, points); break;
   case 3: tab += set_description(w, this, points); break;
  }
 } while (tab >= 0 && tab < 4);
 delwin(w);

 if (tab < 0)
  return false;
 
 // Character is finalized.  Now just set up HP, &c
 for (int i = 0; i < num_hp_parts; i++) {
  hp_max[i] = calc_HP(str_max, has_trait(PF_TOUGH));
  hp_cur[i] = hp_max[i];
 }
 if (has_trait(PF_GLASSJAW)) {
  hp_max[hp_head] = int(hp_max[hp_head] * .85);
  hp_cur[hp_head] = hp_max[hp_head];
 }
 if (has_trait(PF_SMELLY))
  scent = 800;
 if (has_trait(PF_ANDROID)) {
  add_bionic(bionic_id(rng(bio_memory, max_bio_start - 1)));// Other
  if (bionics[my_bionics[0].id].power_cost > 0) {
   add_bionic(bionic_id(rng(1, bio_ethanol)));	// Power Source
   max_power_level = 10;
   power_level = 10;
  } else {
   bionic_id tmpbio;
   do
    tmpbio = bionic_id(rng(bio_ethanol + 1, bio_armor_legs));
   while (bionics[tmpbio].power_cost > 0);
   add_bionic(tmpbio);
   max_power_level = 0;
   power_level = 0;
  }

/* CHEATER'S STUFF

  add_bionic(bionic_id(rng(0, bio_ethanol)));	// Power Source
  for (int i = 0; i < 5; i++)
   add_bionic(bionic_id(rng(bio_memory, max_bio_start - 1)));// Other
  max_power_level = 80;
  power_level = 80;

End of cheatery */
 }

 if (has_trait(PF_MARTIAL_ARTS)) {
  itype_id ma_type;
  do {
   int choice = menu("Pick your style:",
                     "Karate", "Judo", "Aikido", "Tai Chi", "Taekwando", NULL);
   if (choice == 1)
    ma_type = itm_style_karate;
   if (choice == 2)
    ma_type = itm_style_judo;
   if (choice == 3)
    ma_type = itm_style_aikido;
   if (choice == 4)
    ma_type = itm_style_tai_chi;
   if (choice == 5)
    ma_type = itm_style_taekwando;
   item tmpitem = item(g->itypes[ma_type], 0);
   full_screen_popup(tmpitem.info(true).c_str());
  } while (!query_yn("Use this style?"));
  styles.push_back(ma_type);
 }
 ret_null = item(g->itypes[0], 0);
 if (!styles.empty())
  weapon = item(g->itypes[ styles[0] ], 0, ':');
 else
  weapon   = item(g->itypes[0], 0);
// Nice to start out less than naked.
 item tmp(g->itypes[itm_jeans], 0, 'a');
 worn.push_back(tmp);
 tmp = item(g->itypes[itm_tshirt], 0, 'b');
 worn.push_back(tmp);
 tmp = item(g->itypes[itm_sneakers], 0, 'c');
 worn.push_back(tmp);
// The near-sighted get to start with glasses.
 if (has_trait(PF_MYOPIC)) {
  tmp = item(g->itypes[itm_glasses_eye], 0, 'd');
  worn.push_back(tmp);
 }
// Likewise, the asthmatic start with their medication.
 if (has_trait(PF_ASTHMA)) {
  tmp = item(g->itypes[itm_inhaler], 0, 'a' + worn.size());
  inv.push_back(tmp);
 }
// make sure we have no mutations
 for (int i = 0; i < PF_MAX2; i++)
  my_mutations[i] = false;
 return true;
}

void draw_tabs(WINDOW* w)
{
 for (int i = 0; i < 16; i++) {
  if (i < 9)
   mvwputch(w, 0, i +  5, c_ltgray, LINE_OXOX);
  if (i < 10) {
   mvwputch(w, 0, i + 22, c_ltgray, LINE_OXOX);
   mvwputch(w, 0, i + 40, c_ltgray, LINE_OXOX);
  }
  mvwputch(w, 0, i + 57, c_ltgray, LINE_OXOX);
 }
 mvwputch(w, 0, 4, c_ltgray, LINE_OXXO);
 mvwputch(w, 0,21, c_ltgray, LINE_OXXO);
 mvwputch(w, 0,39, c_ltgray, LINE_OXXO);
 mvwputch(w, 0,57, c_ltgray, LINE_OXXO);

 mvwputch(w, 0,14, c_ltgray, LINE_OOXX);
 mvwputch(w, 0,32, c_ltgray, LINE_OOXX);
 mvwputch(w, 0,50, c_ltgray, LINE_OOXX);
 mvwputch(w, 0,73, c_ltgray, LINE_OOXX);

 mvwprintz(w, 1, 4, c_ltgray, "\
   STATS            TRAITS            SKILLS            DESCRIPTION   ");

 mvwputch(w, 1, 4, c_ltgray, LINE_XOXO);
 mvwputch(w, 1,14, c_ltgray, LINE_XOXO);
 mvwputch(w, 1,21, c_ltgray, LINE_XOXO);
 mvwputch(w, 1,32, c_ltgray, LINE_XOXO);
 mvwputch(w, 1,39, c_ltgray, LINE_XOXO);
 mvwputch(w, 1,50, c_ltgray, LINE_XOXO);
 mvwputch(w, 1,57, c_ltgray, LINE_XOXO);
 mvwputch(w, 1,73, c_ltgray, LINE_XOXO);
}

int set_stats(WINDOW* w, player *u, int &points)
{
 unsigned char sel = 1;
 char ch;
// Draw horizontal lines, with a gap for the active tab
 for (int i = 0; i < 80; i++) {
  if (i < 4 || i > 14)
   mvwputch(w,  2, i, c_ltgray, LINE_OXOX);
  mvwputch(w,  4, i, c_ltgray, LINE_OXOX);
  mvwputch(w, 21, i, c_ltgray, LINE_OXOX);
 }
// Attaching lines for tabs
 mvwputch(w, 1,  2, h_ltgray, '<');
 mvwputch(w, 1, 16, h_ltgray, '>');
 mvwputch(w, 2,  4, c_ltgray, LINE_XOOX);
 mvwputch(w, 2, 14, c_ltgray, LINE_XXOO);
 mvwputch(w, 2, 21, c_ltgray, LINE_XXOX);
 mvwputch(w, 2, 32, c_ltgray, LINE_XXOX);
 mvwputch(w, 2, 39, c_ltgray, LINE_XXOX);
 mvwputch(w, 2, 50, c_ltgray, LINE_XXOX);
 mvwputch(w, 2, 57, c_ltgray, LINE_XXOX);
 mvwputch(w, 2, 73, c_ltgray, LINE_XXOX);
 mvwprintz(w, 1, 5, h_ltgray, "  STATS  ");

 mvwprintz(w, 11, 0, c_ltgray, "\
   j/k, 8/2, or arrows select\n\
    a statistic.\n\
   l, 6, or right arrow\n\
    increases the statistic.\n\
   h, 4, or left arrow\n\
    decreases the statistic.\n\
\n\
   > Takes you to the next tab.\n\
   < Returns you to the main menu.");

 do {
  mvwprintz(w,  3, 2, c_ltgray, "Points left: %d  ", points);
  switch (sel) {
  case 1:
   if (u->str_max >= HIGH_STAT)
    mvwprintz(w, 3, 33, c_ltred, "Increasing Str further costs 2 points.");
   else
    mvwprintz(w, 3, 33, c_black, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
   mvwprintz(w, 6,  2, COL_STAT_ACT, "Strength:     %d  ", u->str_max);
   mvwprintz(w, 7,  2, c_ltgray,     "Dexterity:    %d  ", u->dex_max);
   mvwprintz(w, 8,  2, c_ltgray,     "Intelligence: %d  ", u->int_max);
   mvwprintz(w, 9,  2, c_ltgray,     "Perception:   %d  ", u->per_max);
   mvwprintz(w, 6, 33, COL_STAT_ACT, "Base HP: %d                                 ",
             calc_HP(u->str_max, u->has_trait(PF_TOUGH)));
   mvwprintz(w, 7, 33, COL_STAT_ACT, "Carry weight: %d lbs                        ",
             u->weight_capacity(false) / 4);
   mvwprintz(w, 8, 33, COL_STAT_ACT, "Melee damage: %d                            ",
             u->base_damage(false));
   mvwprintz(w, 9, 33, COL_STAT_ACT, "  Strength also makes you more resistant to ");
   mvwprintz(w,10, 33, COL_STAT_ACT, "many diseases and poisons, and makes actions");
   mvwprintz(w,11, 33, COL_STAT_ACT, "which require brute force more effective.   ");
   break;

  case 2:
   if (u->dex_max >= HIGH_STAT)
    mvwprintz(w, 3, 33, c_ltred, "Increasing Dex further costs 2 points.");
   else
    mvwprintz(w, 3, 33, c_black, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
   mvwprintz(w, 6,  2, c_ltgray,     "Strength:     %d  ", u->str_max);
   mvwprintz(w, 7,  2, COL_STAT_ACT, "Dexterity:    %d  ", u->dex_max);
   mvwprintz(w, 8,  2, c_ltgray,     "Intelligence: %d  ", u->int_max);
   mvwprintz(w, 9,  2, c_ltgray,     "Perception:   %d  ", u->per_max);
   mvwprintz(w, 6, 33, COL_STAT_ACT, "Melee to-hit bonus: +%d                      ",
             u->base_to_hit(false));
   mvwprintz(w, 7, 33, COL_STAT_ACT, "                                            ");
   mvwprintz(w, 7, 33, COL_STAT_ACT, "Ranged %s: %s%d",
             (u->ranged_dex_mod(false) <= 0 ? "bonus" : "penalty"),
             (u->ranged_dex_mod(false) <= 0 ? "+" : "-"),
             abs(u->ranged_dex_mod(false)));
   mvwprintz(w, 8, 33, COL_STAT_ACT, "                                            ");
   mvwprintz(w, 8, 33, COL_STAT_ACT, "Throwing %s: %s%d",
             (u->throw_dex_mod(false) <= 0 ? "bonus" : "penalty"),
             (u->throw_dex_mod(false) <= 0 ? "+" : "-"),
             abs(u->throw_dex_mod(false)));
   mvwprintz(w, 9, 33, COL_STAT_ACT, "  Dexterity also enhances many actions which");
   mvwprintz(w,10, 33, COL_STAT_ACT, "require finesse.                            ");
   mvwprintz(w,11, 33, COL_STAT_ACT, "                                            ");
   break;

  case 3:
   if (u->int_max >= HIGH_STAT)
    mvwprintz(w, 3, 33, c_ltred, "Increasing Int further costs 2 points.");
   else
    mvwprintz(w, 3, 33, c_black, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
   mvwprintz(w, 6,  2, c_ltgray,     "Strength:     %d  ", u->str_max);
   mvwprintz(w, 7,  2, c_ltgray,     "Dexterity:    %d  ", u->dex_max);
   mvwprintz(w, 8,  2, COL_STAT_ACT, "Intelligence: %d  ", u->int_max);
   mvwprintz(w, 9,  2, c_ltgray,     "Perception:   %d  ", u->per_max);
   mvwprintz(w, 6, 33, COL_STAT_ACT, "Skill comprehension: %d%%%%                     ",
             u->comprehension_percent(sk_null, false));
   mvwprintz(w, 7, 33, COL_STAT_ACT, "Read times: %d%%%%                              ",
             u->read_speed(false));
   mvwprintz(w, 8, 33, COL_STAT_ACT, "  Intelligence is also used when crafting,  ");
   mvwprintz(w, 9, 33, COL_STAT_ACT, "installing bionics, and interacting with    ");
   mvwprintz(w,10, 33, COL_STAT_ACT, "NPCs.                                       ");
   mvwprintz(w,11, 33, COL_STAT_ACT, "                                            ");
   break;

  case 4:
   if (u->per_max >= HIGH_STAT)
    mvwprintz(w, 3, 33, c_ltred, "Increasing Per further costs 2 points.");
   else
    mvwprintz(w, 3, 33, c_black, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
   mvwprintz(w, 6,  2, c_ltgray,     "Strength:     %d  ", u->str_max);
   mvwprintz(w, 7,  2, c_ltgray,     "Dexterity:    %d  ", u->dex_max);
   mvwprintz(w, 8,  2, c_ltgray,     "Intelligence: %d  ", u->int_max);
   mvwprintz(w, 9,  2, COL_STAT_ACT, "Perception:   %d  ", u->per_max);
   mvwprintz(w, 6, 33, COL_STAT_ACT, "                                            ");
   mvwprintz(w, 6, 33, COL_STAT_ACT, "Ranged %s: %s%d",
             (u->ranged_per_mod(false) <= 0 ? "bonus" : "penalty"),
             (u->ranged_per_mod(false) <= 0 ? "+" : "-"),
             abs(u->ranged_per_mod(false)));
   mvwprintz(w, 7, 33, COL_STAT_ACT, "  Perception is also used for detecting     ");
   mvwprintz(w, 8, 33, COL_STAT_ACT, "traps and other things of interest.         ");
   mvwprintz(w, 9, 33, COL_STAT_ACT, "                                            ");
   mvwprintz(w,10, 33, COL_STAT_ACT, "                                            ");
   mvwprintz(w,11, 33, COL_STAT_ACT, "                                            ");
   break;
  }
 
  wrefresh(w);
  ch = input();
  if (ch == 'j' && sel < 4)
   sel++;
  if (ch == 'k' && sel > 1)
   sel--;
  if (ch == 'h') {
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
  if (ch == 'l' && points > 0) {
   if (sel == 1 && u->str_max < 20 && (u->str_max < HIGH_STAT || points > 1)) {
    points--;
    if (u->str_max >= HIGH_STAT)
     points--;
    u->str_max++;
   } else if (sel == 2 && u->dex_max < 20 &&
              (u->dex_max < HIGH_STAT || points > 1)) {
    points--;
    if (u->dex_max >= HIGH_STAT)
     points--;
    u->dex_max++;
   } else if (sel == 3 && u->int_max < 20 &&
              (u->int_max < HIGH_STAT || points > 1)) {
    points--;
    if (u->int_max >= HIGH_STAT)
     points--;
    u->int_max++;
   } else if (sel == 4 && u->per_max < 20 &&
              (u->per_max < HIGH_STAT || points > 1)) {
    points--;
    if (u->per_max >= HIGH_STAT)
     points--;
    u->per_max++;
   }
  }
  if (ch == '<' && query_yn("Return to main menu?"))
   return -1;
  if (ch == '>')
   return 1;
 } while (true);
}

int set_traits(WINDOW* w, player *u, int &points)
{
// Track how many good / bad POINTS we have; cap both at MAX_TRAIT_POINTS
 int num_good = 0, num_bad = 0;
 for (int i = 0; i < PF_SPLIT; i++) {
  if (u->has_trait(i))
   num_good += traits[i].points;
 }
 for (int i = PF_SPLIT + 1; i < PF_MAX; i++) {
  if (u->has_trait(i))
   num_bad += abs(traits[i].points);
 }
// Draw horizontal lines, with a gap for the active tab
 for (int i = 0; i < 80; i++) {
  if (i < 21 || i > 32)
   mvwputch(w,  2, i, c_ltgray, LINE_OXOX);
  mvwputch(w,  4, i, c_ltgray, LINE_OXOX);
  mvwputch(w, 21, i, c_ltgray, LINE_OXOX);
 }
// Attaching lines for tabs
 mvwputch(w, 1,19, h_ltgray, '<');
 mvwputch(w, 1,34, h_ltgray, '>');
 mvwputch(w, 2, 4, c_ltgray, LINE_XXOX);
 mvwputch(w, 2,14, c_ltgray, LINE_XXOX);
 mvwputch(w, 2,21, c_ltgray, LINE_XOOX);
 mvwputch(w, 2,32, c_ltgray, LINE_XXOO);
 mvwputch(w, 2,39, c_ltgray, LINE_XXOX);
 mvwputch(w, 2,50, c_ltgray, LINE_XXOX);
 mvwputch(w, 2,57, c_ltgray, LINE_XXOX);
 mvwputch(w, 2,73, c_ltgray, LINE_XXOX);
 mvwprintz(w, 1,22, h_ltgray, "  TRAITS  ");

 for (int i = 0; i < 16; i++) {
  mvwprintz(w, 5 + i, 40, c_dkgray, "\
                                   ");
  mvwprintz(w, 5 + i, 40, c_dkgray, traits[PF_SPLIT + 1 + i].name.c_str());
 }
 mvwprintz(w,11,32, c_ltgray, "h   l");
 mvwprintz(w,12,32, c_ltgray, "<   >");
 mvwprintz(w,13,32, c_ltgray, "4   6");
 mvwprintz(w,15,32, c_ltgray, "Space");
 mvwprintz(w,16,31, c_ltgray,"Toggles");

 int cur_adv = 1, cur_dis = PF_SPLIT + 1, cur_trait, traitmin, traitmax, xoff;
 nc_color col_on, col_off, hi_on, hi_off;
 bool using_adv = true;	// True if we're selecting advantages, false if we're
			// selecting disadvantages

 do {
  mvwprintz(w,  3, 2, c_ltgray, "Points left: %d  ", points);
  mvwprintz(w,  3,20, c_ltgreen, "%s%d/%d", (num_good < 10 ? " " : ""),
                                 num_good, MAX_TRAIT_POINTS);
  mvwprintz(w,  3,33, c_ltred, "%s%d/%d", (num_bad < 10 ? " " : ""),
                               num_bad, MAX_TRAIT_POINTS);
// Clear the bottom of the screen.
  mvwprintz(w, 22, 0, c_ltgray, "\
                                                                             ");
  mvwprintz(w, 23, 0, c_ltgray, "\
                                                                             ");
  mvwprintz(w, 24, 0, c_ltgray, "\
                                                                             ");
  if (using_adv) {
   col_on  = COL_TR_GOOD_ON;
   col_off = COL_TR_GOOD_OFF;
   hi_on   = hilite(col_on);
   hi_off  = hilite(col_off);
   xoff = 0;
   cur_trait = cur_adv;
   traitmin = 1;
   traitmax = PF_SPLIT;
   mvwprintz(w,  3, 40, c_ltgray, "                                       ");
   mvwprintz(w,  3, 40, COL_TR_GOOD, "%s costs %d points",
             traits[cur_adv].name.c_str(), traits[cur_adv].points);
   mvwprintz(w, 22, 0, COL_TR_GOOD, "%s", traits[cur_adv].description.c_str());
  } else {
   col_on  = COL_TR_BAD_ON;
   col_off = COL_TR_BAD_OFF;
   hi_on   = hilite(col_on);
   hi_off  = hilite(col_off);
   xoff = 40;
   cur_trait = cur_dis;
   traitmin = PF_SPLIT + 1;
   traitmax = PF_MAX;
   mvwprintz(w,  3, 40, c_ltgray, "                                       ");
   mvwprintz(w,  3, 40, COL_TR_BAD, "%s earns %d points",
             traits[cur_dis].name.c_str(), traits[cur_dis].points * -1);
   mvwprintz(w, 22, 0, COL_TR_BAD, "%s", traits[cur_dis].description.c_str());
  }
  if (cur_trait <= traitmin + 7) {
   for (int i = traitmin; i < traitmin + 16; i++) {
    mvwprintz(w, 5 + i - traitmin, xoff, c_ltgray, "\
                                       ");	// Clear the line
    if (i == cur_trait) {
     if (u->has_trait(i))
      mvwprintz(w, 5 + i - traitmin, xoff, hi_on, traits[i].name.c_str());
     else
      mvwprintz(w, 5 + i - traitmin, xoff, hi_off, traits[i].name.c_str());
    } else {
     if (u->has_trait(i))
      mvwprintz(w, 5 + i - traitmin, xoff, col_on, traits[i].name.c_str());
     else
      mvwprintz(w, 5 + i - traitmin, xoff, col_off, traits[i].name.c_str());
    }
   }
  } else if (cur_trait >= traitmax - 9) {
   for (int i = traitmax - 16; i < traitmax; i++) {
    mvwprintz(w, 21 + i - traitmax, xoff, c_ltgray, "\
                                       ");	// Clear the line
    if (i == cur_trait) {
     if (u->has_trait(i))
      mvwprintz(w, 21 + i - traitmax, xoff, hi_on, traits[i].name.c_str());
     else
      mvwprintz(w, 21 + i - traitmax, xoff, hi_off, traits[i].name.c_str());
    } else {
     if (u->has_trait(i))
      mvwprintz(w, 21 + i - traitmax, xoff, col_on, traits[i].name.c_str());
     else
      mvwprintz(w, 21 + i - traitmax, xoff, col_off, traits[i].name.c_str());
    }
   }
  } else {
   for (int i = cur_trait - 7; i < cur_trait + 9; i++) {
    mvwprintz(w, 12 + i - cur_trait, xoff, c_ltgray, "\
                                      ");	// Clear the line
    if (i == cur_trait) {
     if (u->has_trait(i))
      mvwprintz(w, 12 + i - cur_trait, xoff, hi_on, traits[i].name.c_str());
     else
      mvwprintz(w, 12 + i - cur_trait, xoff, hi_off, traits[i].name.c_str());
    } else {
     if (u->has_trait(i))
      mvwprintz(w, 12 + i - cur_trait, xoff, col_on, traits[i].name.c_str());
     else
      mvwprintz(w, 12 + i - cur_trait, xoff, col_off, traits[i].name.c_str());
    }
   }
  }

  wrefresh(w);
  switch (input()) {
   case 'h':
   case 'l':
   case '\t':
    if (!using_adv) {
     for (int i = 0; i < 16; i++) {
      mvwprintz(w, 5 + i, 40, c_dkgray, "\
                                       ");
      mvwprintz(w, 5 + i, 40, c_dkgray, traits[PF_SPLIT + 1 + i].name.c_str());
     }
    } else {
     for (int i = 0; i < 16; i++) {
      mvwprintz(w, 5 + i, 0, c_dkgray, "\
                                       ");
      mvwprintz(w, 5 + i, 0, c_dkgray, traits[i + 1].name.c_str());
     }
    }
    using_adv = !using_adv;
    wrefresh(w);
    break;
   case 'k':
    if (using_adv) {
     if (cur_adv > 1)
      cur_adv--;
    } else {
     if (cur_dis > PF_SPLIT + 1)
      cur_dis--;
    }
    break;
   case 'j':
   if (using_adv) {
     if (cur_adv < PF_SPLIT - 1)
      cur_adv++;
    } else {
     if (cur_dis < PF_MAX - 1)
      cur_dis++;
    }
    break;
   case ' ':
   case '\n':
    if (u->has_trait(cur_trait)) {
     if (points + traits[cur_trait].points >= 0) {
      u->toggle_trait(cur_trait);
      points += traits[cur_trait].points;
      if (using_adv)
       num_good -= traits[cur_trait].points;
      else
       num_bad += traits[cur_trait].points;
     } else
      mvwprintz(w,  3, 2, c_red, "Points left: %d  ", points);
    } else if (using_adv && num_good + traits[cur_trait].points >
                            MAX_TRAIT_POINTS)
     popup("Sorry, but you can only take %d points of advantages.",
           MAX_TRAIT_POINTS);
    else if (!using_adv && num_bad - traits[cur_trait].points >
                           MAX_TRAIT_POINTS)
     popup("Sorry, but you can only take %d points of disadvantages.",
           MAX_TRAIT_POINTS);
    else if (points >= traits[cur_trait].points) {
     u->toggle_trait(cur_trait);
     points -= traits[cur_trait].points;
     if (using_adv)
      num_good += traits[cur_trait].points;
     else
      num_bad -= traits[cur_trait].points;
    }
    break;
   case '<':
    return -1;
   case '>':
    return 1;
  }
 } while (true);
}

int set_skills(WINDOW* w, player *u, int &points)
{
// Draw horizontal lines, with a gap for the active tab
 for (int i = 0; i < 80; i++) {
  if (i < 39 || i > 50)
   mvwputch(w,  2, i, c_ltgray, LINE_OXOX);
  mvwputch(w,  4, i, c_ltgray, LINE_OXOX);
  mvwputch(w, 21, i, c_ltgray, LINE_OXOX);
 }
// Attaching lines for tabs
 mvwputch(w, 1,37, h_ltgray, '<');
 mvwputch(w, 1,52, h_ltgray, '>');
 mvwputch(w, 2, 4, c_ltgray, LINE_XXOX);
 mvwputch(w, 2,14, c_ltgray, LINE_XXOX);
 mvwputch(w, 2,21, c_ltgray, LINE_XXOX);
 mvwputch(w, 2,32, c_ltgray, LINE_XXOX);
 mvwputch(w, 2,39, c_ltgray, LINE_XOOX);
 mvwputch(w, 2,50, c_ltgray, LINE_XXOO);
 mvwputch(w, 2,57, c_ltgray, LINE_XXOX);
 mvwputch(w, 2,73, c_ltgray, LINE_XXOX);
 mvwprintz(w,1,40, h_ltgray, "  SKILLS  ");
 
 int cur_sk = 1;

 do {
  mvwprintz(w,  3, 2, c_ltgray, "Points left: %d  ", points);
// Clear the bottom of the screen.
  mvwprintz(w, 22, 0, c_ltgray, "\
                                                                             ");
  mvwprintz(w, 23, 0, c_ltgray, "\
                                                                             ");
  mvwprintz(w, 24, 0, c_ltgray, "\
                                                                             ");
  if (points >= u->sklevel[cur_sk] + 1)
   mvwprintz(w,  3, 30, COL_SKILL_USED, "Upgrading %s costs %d points         ",
             skill_name(cur_sk).c_str(), u->sklevel[cur_sk] + 1);
  else
   mvwprintz(w,  3, 30, c_ltred, "Upgrading %s costs %d points         ",
             skill_name(cur_sk).c_str(), u->sklevel[cur_sk] + 1);
  mvwprintz(w, 22, 0, COL_SKILL_USED, skill_description(cur_sk).c_str());

  if (cur_sk <= 7) {
   for (int i = 1; i < 17; i++) {
    mvwprintz(w, 4 + i, 0, c_ltgray, "\
                                             ");	// Clear the line
    if (u->sklevel[i] == 0) {
     mvwprintz(w, 4 + i, 0, (i == cur_sk ? h_ltgray : c_ltgray),
               skill_name(i).c_str());
    } else {
     mvwprintz(w, 4 + i, 0,
               (i == cur_sk ? hilite(COL_SKILL_USED) : COL_SKILL_USED),
               "%s ", skill_name(i).c_str());
     for (int j = 0; j < u->sklevel[i]; j++)
      wprintz(w, (i == cur_sk ? hilite(COL_SKILL_USED) : COL_SKILL_USED), "*");
    }
   }
  } else if (cur_sk >= num_skill_types - 9) {
   for (int i = num_skill_types - 16; i < num_skill_types; i++) {
    mvwprintz(w, 21 + i - num_skill_types, 0, c_ltgray, "\
                                             ");	// Clear the line
    if (u->sklevel[i] == 0) {
     mvwprintz(w, 21 + i - num_skill_types, 0,
               (i == cur_sk ? h_ltgray : c_ltgray), skill_name(i).c_str());
    } else {
     mvwprintz(w, 21 + i - num_skill_types, 0,
               (i == cur_sk ? hilite(COL_SKILL_USED) : COL_SKILL_USED), "%s ",
               skill_name(i).c_str());
     for (int j = 0; j < u->sklevel[i]; j++)
      wprintz(w, (i == cur_sk ? hilite(COL_SKILL_USED) : COL_SKILL_USED), "*");
    }
   }
  } else {
   for (int i = cur_sk - 7; i < cur_sk + 9; i++) {
    mvwprintz(w, 12 + i - cur_sk, 0, c_ltgray, "\
                                             ");	// Clear the line
    if (u->sklevel[i] == 0) {
     mvwprintz(w, 12 + i - cur_sk, 0, (i == cur_sk ? h_ltgray : c_ltgray),
               skill_name(i).c_str());
    } else {
     mvwprintz(w, 12 + i - cur_sk, 0,
               (i == cur_sk ? hilite(COL_SKILL_USED) : COL_SKILL_USED),
               "%s ", skill_name(i).c_str());
     for (int j = 0; j < u->sklevel[i]; j++)
      wprintz(w, (i == cur_sk ? hilite(COL_SKILL_USED) : COL_SKILL_USED), "*");
    }
   }
  }
   
  wrefresh(w);
  switch (input()) {
   case 'j':
    if (cur_sk < num_skill_types - 1)
     cur_sk++;
    break;
   case 'k':
    if (cur_sk > 1)
     cur_sk--;
    break;
   case 'h':
    if (u->sklevel[cur_sk] > 0) {
     points += u->sklevel[cur_sk] - 1;
     u->sklevel[cur_sk] -= 2;
    }
    break;
   case 'l':
    if (points >= u->sklevel[cur_sk] + 1) {
     points -= u->sklevel[cur_sk] + 1;
     u->sklevel[cur_sk] += 2;
    }
    break;
   case '<':
    return -1;
   case '>':
    return 1;
  }
 } while (true);
}

int set_description(WINDOW* w, player *u, int &points)
{
// Draw horizontal lines, with a gap for the active tab
 for (int i = 0; i < 80; i++) {
  if (i < 57 || i > 73)
   mvwputch(w,  2, i, c_ltgray, LINE_OXOX);
  mvwputch(w,  4, i, c_ltgray, LINE_OXOX);
  mvwputch(w, 21, i, c_ltgray, LINE_OXOX);
 }
// Attaching lines for tabs
 mvwputch(w, 1,55, h_ltgray, '<');
 mvwputch(w, 1,75, h_ltgray, '>');
 mvwputch(w, 2, 4, c_ltgray, LINE_XXOX);
 mvwputch(w, 2,14, c_ltgray, LINE_XXOX);
 mvwputch(w, 2,21, c_ltgray, LINE_XXOX);
 mvwputch(w, 2,32, c_ltgray, LINE_XXOX);
 mvwputch(w, 2,39, c_ltgray, LINE_XXOX);
 mvwputch(w, 2,50, c_ltgray, LINE_XXOX);
 mvwputch(w, 2,57, c_ltgray, LINE_XOOX);
 mvwputch(w, 2,73, c_ltgray, LINE_XXOO);
 mvwprintz(w,1,58, h_ltgray, "  DESCRIPTION  ");
 mvwprintz(w,  3, 2, c_ltgray, "Points left: %d  ", points);

 mvwprintz(w, 6, 2, c_ltgray, "\
Name: ______________________________     (Press TAB to move off this line)");
 mvwprintz(w, 8, 2, c_ltgray, "\
Gender: Male Female                      (Press spacebar to toggle)");
 mvwprintz(w,10, 2, c_ltgray, "\
When your character is finished and you're ready to start playing, press >");
 mvwprintz(w,12, 2, c_ltgray, "\
To go back and review your character, press <");
 mvwprintz(w, 14, 2, c_green, "\
To pick a random name for your character, press ?.");
 mvwprintz(w, 16, 2, c_green, "\
To save this character as a template, press !.");
 
 int line = 1;
 bool noname = false;
 long ch;

 do {
  if (u->male) {
   mvwprintz(w, 8, 10, c_ltred, "Male");
   mvwprintz(w, 8, 15, c_ltgray, "Female");
  } else {
   mvwprintz(w, 8, 10, c_ltgray, "Male");
   mvwprintz(w, 8, 15, c_ltred, "Female");
  }

  if (!noname) {
   mvwprintz(w, 6, 8, c_ltgray, u->name.c_str());
   if (line == 1)
    wprintz(w, h_ltgray, "_");
  }
  if (line == 2)
   mvwprintz(w, 8, 2, h_ltgray, "Gender:");
  else
   mvwprintz(w, 8, 2, c_ltgray, "Gender:");

  wrefresh(w);
  ch = input();
  if (noname) {
   mvwprintz(w, 6, 8, c_ltgray, "______________________________");
   noname = false;
  }


  if (ch == '>') {
   if (points > 0)
    mvwprintz(w,  3, 2, c_red, "\
Points left: %d    You must use the rest of your points!", points);
   else if (u->name.size() == 0) {
    mvwprintz(w, 6, 8, h_ltgray, "______NO NAME ENTERED!!!!_____");
    noname = true;
    wrefresh(w);
    if (query_yn("Are you SURE you're finished? Your name will be randomly generated.")){
     u->pick_name();
     return 1;
    }
   } else if (query_yn("Are you SURE you're finished?"))
    return 1;
   else
    refresh();
  } else if (ch == '<') {
   return -1;
  } else if (ch == '!') {
   if (points > 0) {
    popup("You cannot save a template with unused points!");
   } else
    save_template(u);
   mvwprintz(w,12, 2, c_ltgray,"To go back and review your character, press <");
   wrefresh(w);
  } else if (ch == '?') {
   mvwprintz(w, 6, 8, c_ltgray, "______________________________");
   u->pick_name();
  } else {
   switch (line) {
    case 1:
     if (ch == KEY_BACKSPACE || ch == 127) {
      if (u->name.size() > 0) {
       mvwprintz(w, 6, 8 + u->name.size(), c_ltgray, "_");
       u->name.erase(u->name.end() - 1);
      }
     } else if (ch == '\t') {
      line = 2;
      mvwprintz(w, 6, 8 + u->name.size(), c_ltgray, "_");
     } else if (((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                  ch == ' ') && u->name.size() < 30) {
      u->name.push_back(ch);
     }
     break;
    case 2:
     if (ch == ' ')
      u->male = !u->male;
     else if (ch == 'k' || ch == '\t') {
      line = 1;
      mvwprintz(w, 8, 8, c_ltgray, ":");
     }
     break;
   }
  }
 } while (true);
}

int player::random_good_trait()
{
 return rng(1, PF_SPLIT - 1);
}

int player::random_bad_trait()
{
 return rng(PF_SPLIT + 1, PF_MAX - 1);
}

int random_skill()
{
 return rng(1, num_skill_types - 1);
}

int calc_HP(int strength, bool tough)
{
 return (60 + 3 * strength) * (tough ? 1.2 : 1);
}

void save_template(player *u)
{
 std::string name = string_input_popup("Name of template:");
 if (name.length() == 0)
  return;
 std::stringstream playerfile;
 playerfile << "data/" << name << ".template";
 std::ofstream fout;
 fout.open(playerfile.str().c_str());
 fout << u->save_info();
}
