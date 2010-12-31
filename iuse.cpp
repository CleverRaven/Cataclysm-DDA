#include "iuse.h"
#include "game.h"
#include "mapdata.h"
#include "keypress.h"
#include "output.h"
#include "rng.h"
#include "line.h"
#include <sstream>

void iuse::royal_jelly(game *g, item *it, bool t)
{
// TODO: Add other diseases here; royal jelly is a cure-all!
 g->u.pkill += 5;
 if (g->u.has_disease(DI_FUNGUS)) {
  g->add_msg("You feel cleansed inside!");
  g->u.rem_disease(DI_FUNGUS);
 }
 if (g->u.has_disease(DI_BLIND)) {
  g->add_msg("Your sight returns!");
  g->u.rem_disease(DI_BLIND);
 }
 if (g->u.has_disease(DI_POISON) || g->u.has_disease(DI_FOODPOISON)) {
  g->add_msg("You feel much better!");
  g->u.rem_disease(DI_POISON);
  g->u.rem_disease(DI_FOODPOISON);
 }
 if (g->u.has_disease(DI_ASTHMA)) {
  g->add_msg("Your breathing clears up!");
  g->u.rem_disease(DI_ASTHMA);
 }
}

void iuse::bandage(game *g, item *it, bool t) 
{
 int bonus = g->u.sklevel[sk_firstaid];
 WINDOW* w = newwin(10, 20, 8, 1);
 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 mvwprintz(w, 1, 1, c_ltred,  "Bandage where?");
 mvwprintz(w, 2, 1, c_ltgray, "1: Head");
 mvwprintz(w, 3, 1, c_ltgray, "2: Torso");
 mvwprintz(w, 4, 1, c_ltgray, "3: Left Arm");
 mvwprintz(w, 5, 1, c_ltgray, "4: Right Arm");
 mvwprintz(w, 6, 1, c_ltgray, "5: Left Leg");
 mvwprintz(w, 7, 1, c_ltgray, "6: Right Leg");
 mvwprintz(w, 8, 1, c_ltgray, "7: Exit");
 nc_color col;
 int curhp;
 for (int i = 0; i < num_hp_parts; i++) {
  curhp = g->u.hp_cur[i];
  if (curhp != 0) {
   switch (hp_part(i)) {
    case hp_head:  curhp += 1;	bonus *=  .8;	break;
    case hp_torso: curhp += 4;	bonus *= 1.5;	break;
    default:       curhp += 3;			break;
   }
   curhp += bonus;
   if (curhp > g->u.hp_max[i])
    curhp = g->u.hp_max[i];
   if (curhp == g->u.hp_max[i])
    col = c_green;
   else if (curhp > g->u.hp_max[i] * .8)
    col = c_ltgreen;
   else if (curhp > g->u.hp_max[i] * .5)
    col = c_yellow;
   else if (curhp > g->u.hp_max[i] * .3)
    col = c_ltred;
   else
    col = c_red;
   if (g->u.has_trait(PF_HPIGNORANT))
    mvwprintz(w, i + 2, 15, col, "***");
   else {
    if (curhp >= 100)
     mvwprintz(w, i + 2, 15, col, "%d", curhp);
    else if (curhp >= 10)
     mvwprintz(w, i + 2, 16, col, "%d", curhp);
    else
     mvwprintz(w, i + 2, 17, col, "%d", curhp);
   }
  } else	// curhp is 0; requires surgical attention
   mvwprintz(w, i + 2, 15, c_dkgray, "---");
 }
 wrefresh(w);
 char ch;
 do {
  ch = getch();
         if (ch == '1') {
   g->u.heal(bp_head,  0, 1 + bonus * 0.8);
  } else if (ch == '2') {
   g->u.heal(bp_torso, 0, 4 + bonus * 1.5);
  } else if (ch == '3') {
   if (g->u.hp_cur[hp_arm_l] == 0) {
    g->add_msg("That arm is broken.  It needs surgical attention.");
    it->charges++;
   } else
    g->u.heal(bp_arms,  0, 3 + bonus);
  } else if (ch == '4') {
   if (g->u.hp_cur[hp_arm_r] == 0) {
    g->add_msg("That arm is broken.  It needs surgical attention.");
    it->charges++;
   } else
    g->u.heal(bp_arms,  1, 3 + bonus);
  } else if (ch == '5') {
   if (g->u.hp_cur[hp_leg_l] == 0) {
    g->add_msg("That leg is broken.  It needs surgical attention.");
    it->charges++;
   } else
    g->u.heal(bp_legs,  0, 3 + bonus);
  } else if (ch == '6') {
   if (g->u.hp_cur[hp_leg_r] == 0) {
    g->add_msg("That leg is broken.  It needs surgical attention.");
    it->charges++;
   } else
    g->u.heal(bp_legs,  1, 3 + bonus);
  }
 } while (ch < '1' || ch > '7');
 if (ch != '7')
  g->u.practice(sk_firstaid, 8);
 else
  it->charges++;
 werase(w);
 wrefresh(w);
 delwin(w);
 refresh();
}

void iuse::firstaid(game *g, item *it, bool t) 
{
 int bonus = g->u.sklevel[sk_firstaid] * 2;
 WINDOW* w = newwin(10, 20, 8, 1);
 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 mvwprintz(w, 1, 1, c_ltred,  "Heal where?");
 mvwprintz(w, 2, 1, c_ltgray, "1: Head");
 mvwprintz(w, 3, 1, c_ltgray, "2: Torso");
 mvwprintz(w, 4, 1, c_ltgray, "3: Left Arm");
 mvwprintz(w, 5, 1, c_ltgray, "4: Right Arm");
 mvwprintz(w, 6, 1, c_ltgray, "5: Left Leg");
 mvwprintz(w, 7, 1, c_ltgray, "6: Right Leg");
 mvwprintz(w, 8, 1, c_ltgray, "7: Exit");
 nc_color col;
 int curhp;
 for (int i = 0; i < num_hp_parts; i++) {
  curhp = g->u.hp_cur[i];
  if (curhp != 0) {
   switch (hp_part(i)) {
    case hp_head:  curhp += 10;	bonus *= 0.8;	break;
    case hp_torso: curhp += 18;	bonus *= 1.5;	break;
    default:       curhp += 14;			break;
   }
   curhp += bonus;
   if (curhp > g->u.hp_max[i])
    curhp = g->u.hp_max[i];
   if (curhp == g->u.hp_max[i])
    col = c_green;
   else if (curhp > g->u.hp_max[i] * .8)
    col = c_ltgreen;
   else if (curhp > g->u.hp_max[i] * .5)
    col = c_yellow;
   else if (curhp > g->u.hp_max[i] * .3)
    col = c_ltred;
   else
    col = c_red;
   if (g->u.has_trait(PF_HPIGNORANT)) {
    mvwprintz(w, i + 2, 15, col, "***");
   } else {
    if (curhp >= 100)
     mvwprintz(w, i + 2, 15, col, "%d", curhp);
    else if (curhp >= 10)
     mvwprintz(w, i + 2, 16, col, "%d", curhp);
    else
     mvwprintz(w, i + 2, 17, col, "%d", curhp);
   }
  } else
   mvwprintz(w, i + 2, 15, c_dkgray, "---");
 }
 wrefresh(w);
 char ch;
 do {
  ch = getch();
         if (ch == '1') {
   g->u.heal(bp_head, 0,  10 + bonus * 0.8);
  } else if (ch == '2') {
   g->u.heal(bp_torso, 0, 18 + bonus * 1.5);
  } else if (ch == '3') {
   if (g->u.hp_cur[hp_arm_l] == 0) {
    g->add_msg("That arm is broken.  It needs surgical attention.");
    it->charges++;
   } else
    g->u.heal(bp_arms, 0,  15 + bonus);
  } else if (ch == '4') {
   if (g->u.hp_cur[hp_arm_r] == 0) {
    g->add_msg("That arm is broken.  It needs surgical attention.");
    it->charges++;
   } else
    g->u.heal(bp_arms, 1,  15 + bonus);
  } else if (ch == '5') {
   if (g->u.hp_cur[hp_leg_l] == 0) {
    g->add_msg("That leg is broken.  It needs surgical attention.");
    it->charges++;
   } else
    g->u.heal(bp_legs, 0,  15 + bonus);
  } else if (ch == '6') {
   if (g->u.hp_cur[hp_leg_r] == 0) {
    g->add_msg("That leg is broken.  It needs surgical attention.");
    it->charges++;
   } else
    g->u.heal(bp_legs, 1,  15 + bonus);
  }
 } while (ch < '1' || ch > '7');
 if (ch != '7')
  g->u.practice(sk_firstaid, rng(40, 60));
 else
  it->charges++;
 werase(w);
 wrefresh(w);
 delwin(w);
 refresh();
}

// Aspirin
void iuse::pkill_1(game *g, item *it, bool t)
{
 g->add_msg("You take some %s.", it->tname().c_str());
 if (!g->u.has_disease(DI_PKILL1))
  g->u.add_disease(DI_PKILL1, 120, g);
 else {
  for (int i = 0; i < g->u.illness.size(); i++) {
   if (g->u.illness[i].type == DI_PKILL1) {
    g->u.illness[i].duration = 50;
    i = g->u.illness.size();
   }
  }
 }
}

void iuse::pkill_2(game *g, item *it, bool t)
{
 g->add_msg("You take some %s.", it->tname().c_str());
 g->u.add_disease(DI_PKILL2, 100, g);
}

void iuse::pkill_3(game *g, item *it, bool t)
{
 g->add_msg("You take some %s.", it->tname().c_str());
 g->u.add_disease(DI_PKILL2, 200, g);
}

void iuse::pkill_4(game *g, item *it, bool t)
{
 g->add_msg("You shoot up.");
 g->u.add_disease(DI_PKILL3, 80, g);
 g->u.add_disease(DI_PKILL2, 400, g);
}

void iuse::pkill_l(game *g, item *it, bool t)
{
 g->add_msg("You take some %s.", it->tname().c_str());
 g->u.add_disease(DI_PKILL_L, rng(12, 18) * 300, g);
}

void iuse::xanax(game *g, item *it, bool t)
{
 g->add_msg("You take some %s.", it->tname().c_str());
 if (!g->u.has_disease(DI_TOOK_XANAX))
  g->u.add_disease(DI_TOOK_XANAX, 900, g);
 else
  g->u.add_disease(DI_TOOK_XANAX, 200, g);
}

void iuse::caff(game *g, item *it, bool t)
{
 it_comest *food = dynamic_cast<it_comest*> (it->type);
 g->u.fatigue -= food->stim * 3;
}

void iuse::alcohol(game *g, item *it, bool t)
{
 int duration = 680 - (10 * g->u.str_max); // Weaker characters are cheap drunks
 if (g->u.has_trait(PF_LIGHTWEIGHT))
  duration += 300;
 g->u.pkill += 8;
 g->u.add_disease(DI_DRUNK, duration, g);
}

void iuse::cig(game *g, item *it, bool t)
{
 g->add_msg("You light a cigarette and smoke it.");
 g->u.add_disease(DI_CIG, 200, g);
 for (int i = 0; i < g->u.illness.size(); i++) {
  if (g->u.illness[i].type == DI_CIG && g->u.illness[i].duration > 600)
   g->add_msg("Ugh, too much smoke... you feel gross.");
 }
}

void iuse::weed(game *g, item *it, bool t)
{
 int duration = 60;
 if (g->u.has_trait(PF_LIGHTWEIGHT))
  duration = 90;
 g->add_msg("Good stuff, man!");
 g->u.hunger += 8;
 g->u.pkill += 10;
 g->u.add_disease(DI_HIGH, duration, g);
}

void iuse::coke(game *g, item *it, bool t)
{
 int duration = 21 - g->u.str_cur;
 if (g->u.has_trait(PF_LIGHTWEIGHT))
  duration += 20;
 g->add_msg("You snort a bump.");
 g->u.hunger -= 8;
 g->u.add_disease(DI_HIGH, duration, g);
}

void iuse::meth(game *g, item *it, bool t)
{
 int duration = 10 * (40 - g->u.str_cur);
 if (g->u.has_amount(itm_lighter, 1)) {
  g->add_msg("You smoke some crystals.");
  duration *= 1.5;
 } else
  g->add_msg("You snort some crystals.");
 if (!g->u.has_disease(DI_METH))
  duration += 600;
 int hungerpen = (g->u.str_cur < 10 ? 20 : 30 - g->u.str_cur);
 g->u.hunger -= hungerpen;
 g->u.add_disease(DI_METH, duration, g);
}

void iuse::poison(game *g, item *it, bool t)
{
 g->u.add_disease(DI_POISON, 100, g);
}

void iuse::hallu(game *g, item *it, bool t)
{
 g->u.add_disease(DI_HALLU, 2400, g);
}

void iuse::thorazine(game *g, item *it, bool t)
{
 g->add_msg("You feel somewhat sedated.");
 g->u.rem_disease(DI_HALLU);
 g->u.rem_disease(DI_VISUALS);
 g->u.rem_disease(DI_HIGH);
 g->u.fatigue += 15;
}

void iuse::prozac(game *g, item *it, bool t)
{
 if (!g->u.has_disease(DI_TOOK_PROZAC) && g->u.morale_level() < 0)
  g->u.add_disease(DI_TOOK_PROZAC, 7200, g);
 else
  g->u.stim += 3;
}

void iuse::sleep(game *g, item *it, bool t)
{
 g->add_msg("You feel very sleepy...");
 g->u.fatigue += 40;
}

void iuse::iodine(game *g, item *it, bool t)
{
 g->add_msg("You take an iodine tablet.");
 g->u.add_disease(DI_IODINE, 1200, g);
}

void iuse::flumed(game *g, item *it, bool t)
{
 g->add_msg("You take some %s", it->tname().c_str());
 g->u.add_disease(DI_TOOK_FLUMED, 6000, g);
}

void iuse::flusleep(game *g, item *it, bool t)
{
 g->u.add_disease(DI_TOOK_FLUMED, 7200, g);
 g->add_msg("You feel very sleepy...");
 g->u.fatigue += 30;
}

void iuse::inhaler(game *g, item *it, bool t)
{
 g->u.rem_disease(DI_ASTHMA);
 g->add_msg("You take a puff from your inhaler.");
}

void iuse::blech(game *g, item *it, bool t)
{
 g->add_msg("Blech, that burns your throat!");
// TODO: Add more effects?
}

void iuse::mutagen(game *g, item *it, bool t)
{
 g->u.mutate(g);
}

void iuse::purifier(game *g, item *it, bool t)
{
 std::vector<pl_flag> valid;	// Which flags the player has
 for (pl_flag i = pl_flag(0); i < PF_MAX2; i = pl_flag(i + 1)) {
  if (g->u.has_trait(i) && traits[i].curable)
   valid.push_back(i);
 }
 if (valid.size() == 0) {
  int stat_raised = rng(1, 4);
  std::string adj;
  switch (stat_raised) {
   case 1: adj = "stronger";		g->u.str_max++;	break;
   case 2: adj = "nimbler";		g->u.dex_max++;	break;
   case 3: adj = "smarter";		g->u.int_max++;	break;
   case 4: adj = "more perceptive";	g->u.per_max++;	break;
  }
  g->add_msg("You feel %s.", adj.c_str());
  return;
 }
 int num_cured = rng(1, valid.size());
 if (num_cured > 4)
  num_cured = 4;
 for (int i = 0; i < num_cured && valid.size() > 0; i++) {
  int index = rng(0, valid.size() - 1);
  g->u.toggle_trait(valid[index]);
 }
}

// TOOLS below this point!

void iuse::lighter(game *g, item *it, bool t)
{
 int dirx, diry;
 g->draw();
 mvprintw(0, 0, "Place where?");
 get_direction(dirx, diry, input());
 if (dirx == -2) {
  g->add_msg("Invalid direction.");
  return;
 }
 g->u.moves -= 15;
 dirx += g->u.posx;
 diry += g->u.posy;
 if (g->m.add_field(g, dirx, diry, fd_fire, 1))
  g->m.field_at(dirx, diry).age = 1400;
}

void iuse::sew(game *g, item *it, bool t)
{
 char ch = g->inv("Repair what?");
 item* fix = &(g->u.i_at(ch));
 if (fix->type->id == 0) {
  g->add_msg("You do not have that item!");
  return;
 }
 if (!fix->is_armor()) {
  g->add_msg("That isn't clothing!");
  it->charges++;
  return;
 }
 if (!fix->made_of(COTTON) && !fix->made_of(WOOL)) {
  g->add_msg("Your %s is not made of cotton or wool.", fix->tname().c_str());
  it->charges++;
  return;
 }
 if (fix->damage < 0) {
  g->add_msg("Your %s is already enhanced.", fix->tname().c_str());
  it->charges++;
  return;
 } else if (fix->damage == 0) {
  g->u.moves -= 500;
  g->u.practice(sk_tailor, 10);
  int rn = dice(4, 2 + g->u.sklevel[sk_tailor]);
  if (g->u.dex_cur < 8 && one_in(g->u.dex_cur))
   rn -= rng(2, 6);
  if (g->u.dex_cur >= 8 && (g->u.dex_cur >= 16 || one_in(16 - g->u.dex_cur)))
   rn += rng(2, 6);
  if (g->u.dex_cur > 16)
   rn += rng(0, g->u.dex_cur - 16);
  if (rn <= 4) {
   g->add_msg("You damage your %s!", fix->tname().c_str());
   fix->damage++;
  } else if (rn >= 12) {
   g->add_msg("You make your %s extra-sturdy.", fix->tname().c_str());
   fix->damage--;
  } else
   g->add_msg("You practice your sewing.");
 } else {
  g->u.moves -= 500;
  g->u.practice(sk_tailor, 8);
  int rn = dice(4, 2 + g->u.sklevel[sk_tailor]);
  rn -= rng(fix->damage, fix->damage * 2);
  if (g->u.dex_cur < 8 && one_in(g->u.dex_cur))
   rn -= rng(2, 6);
  if (g->u.dex_cur >= 8 && (g->u.dex_cur >= 16 || one_in(16 - g->u.dex_cur)))
   rn += rng(2, 6);
  if (g->u.dex_cur > 16)
   rn += rng(0, g->u.dex_cur - 16);

  if (rn <= 4) {
   g->add_msg("You damage your %s further!", fix->tname().c_str());
   fix->damage++;
   if (fix->damage >= 5) {
    g->add_msg("You destroy it!");
    g->u.i_rem(ch);
   }
  } else if (rn <= 6) {
   g->add_msg("You don't repair your %s, but you waste lots of thread.",
              fix->tname().c_str());
   int waste = rng(1, 8);
   if (waste > it->charges)
    it->charges = 0;
   else
    it->charges -= waste;
  } else if (rn <= 8) {
   g->add_msg("You repair your %s, but waste lots of thread.",
              fix->tname().c_str());
   fix->damage--;
   int waste = rng(1, 8);
   if (waste > it->charges)
    it->charges = 0;
   else
    it->charges -= waste;
  } else if (rn <= 16) {
   g->add_msg("You repair your %s!", fix->tname().c_str());
   fix->damage--;
  } else {
   g->add_msg("You repair your %s completely!", fix->tname().c_str());
   fix->damage = 0;
  }
 }
}

void iuse::scissors(game *g, item *it, bool t)
{
 char ch = g->inv("Chop up what?");
 item* cut = &(g->u.i_at(ch));
 if (cut->type->id == 0) {
  g->add_msg("You do not have that item!");
  return;
 }
 if (cut->type->id == itm_rag) {
  g->add_msg("There's no point in cutting a rag.");
  return;
 }
 if (cut->type->id == itm_string_36) {
  g->u.moves -= 150;
  g->add_msg("You cut the string into 6 smaller pieces.");
  item string(g->itypes[itm_string_6], g->turn, g->nextinv);
  g->u.i_rem(ch);
  bool drop = false;
  for (int i = 0; i < 6; i++) {
   int iter = 0;
   while (g->u.has_item(string.invlet)) {
    string.invlet = g->nextinv;
    g->advance_nextinv();
    iter++;
   }
   if (!drop && (iter == 52 || g->u.volume_carried() >= g->u.volume_capacity()))
    drop = true;
   if (drop)
    g->m.add_item(g->u.posx, g->u.posy, string);
   else
    g->u.i_add(string);
  }
 }
 if (!cut->made_of(COTTON)) {
  g->add_msg("You can only slice items made of cotton.");
  return;
 }
 g->u.moves -= 25 * cut->volume();
 int count = cut->volume();
 if (g->u.sklevel[sk_tailor] == 0)
  count = rng(0, count);
 else if (g->u.sklevel[sk_tailor] == 1 && count >= 2)
  count -= rng(0, 2);
 if (dice(3, 3) > g->u.dex_cur)
  count -= rng(1, 3);
 if (count <= 0) {
  g->add_msg("You clumsily cut the %s into useless ribbons.",
             cut->tname().c_str());
  g->u.i_rem(ch);
  return;
 }
 g->add_msg("You slice the %s into %d rag%s.", cut->tname().c_str(), count,
            (count == 1 ? "" : "s"));
 item rag(g->itypes[itm_rag], g->turn, g->nextinv);
 g->u.i_rem(ch);
 bool drop = false;
 for (int i = 0; i < count; i++) {
  int iter = 0;
  while (g->u.has_item(rag.invlet) && iter < 52) {
   rag.invlet = g->nextinv;
   g->advance_nextinv();
   iter++;
  }
  if (!drop && (iter == 52 || g->u.volume_carried() >= g->u.volume_capacity()))
   drop = true;
  if (drop)
   g->m.add_item(g->u.posx, g->u.posy, rag);
  else
   g->u.i_add(rag);
 }
}

void iuse::extinguisher(game *g, item *it, bool t)
{
 g->draw();
 mvprintz(0, 0, c_red, "Pick a direction to spray:");
 int dirx, diry;
 get_direction(dirx, diry, input());
 if (dirx == -2) {
  g->add_msg("Invalid direction!");
  it->charges++;
  return;
 }
 g->u.moves -= 140;
 int x = dirx + g->u.posx;
 int y = diry + g->u.posy;
 if (g->m.field_at(x, y).type == fd_fire) {
  g->m.field_at(x, y).density -= rng(2, 3);
  if (g->m.field_at(x, y).density <= 0) {
   g->m.field_at(x, y).density = 1;
   g->m.field_at(x, y).type = fd_null;
  }
 }
 int mondex = g->mon_at(x, y);
 if (mondex != -1) {
  int linet;
  g->z[mondex].moves -= 150;
  if (g->u_see(&(g->z[mondex]), linet))
   g->add_msg("The %s is sprayed!", g->z[mondex].name().c_str());
  if (g->z[mondex].made_of(LIQUID)) {
   if (g->u_see(&(g->z[mondex]), linet))
    g->add_msg("The %s is frozen!", g->z[mondex].name().c_str());
   if (g->z[mondex].hurt(rng(20, 60)))
    g->kill_mon(mondex);
   else
    g->z[mondex].speed /= 2;
  }
 }
 if (g->m.move_cost(x, y) != 0) {
  x += dirx;
  y += diry;
  if (g->m.field_at(x, y).type == fd_fire) {
   g->m.field_at(x, y).density -= rng(0, 1) + rng(0, 1);
   if (g->m.field_at(x, y).density <= 0) {
    g->m.field_at(x, y).density = 1;
    g->m.field_at(x, y).type = fd_null;
   }
  }
 }
}

void iuse::hammer(game *g, item *it, bool t)
{
 mvprintz(0, 0, c_red, "Pick a direction in which to construct:");
 int dirx, diry;
 get_direction(dirx, diry, input());
 if (dirx == -2) {
  g->add_msg("Invalid direction!");
  return;
 }
 dirx += g->u.posx;
 diry += g->u.posy;
 WINDOW* w = newwin(7, 40, 9, 30);
 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 int nails = 0, boards = 0;
 std::string title;
 ter_id newter;
 switch (g->m.ter(dirx, diry)) {
 case t_door_b:
  title = "Repair Door";
  nails = 12;
  boards = 3;
  newter = t_door_c;
  break;
 case t_door_c:
 case t_door_locked:
  title = "Board Up Door";
  nails = 12;
  boards = 3;
  newter = t_door_boarded;
  break;
 case t_door_frame:
  title = "Board Up Door Frame";
  nails = 24;
  boards = 6;
  newter = t_door_boarded;
  break;
 case t_window:
 case t_window_frame:
  title = "Board Up Window";
  nails = 4;
  boards = 2;
  newter = t_window_boarded;
  break;
 default:
  title = "";
  nails = 0;
  boards = 0;
 }
 if (nails == 0) {
  g->add_msg("Nothing to board up there!");
  return;
 }
 nc_color col;
 mvwprintz(w, 1, 1, c_white, title.c_str());
 col = (g->u.has_amount(itm_nail, nails) ? c_green : c_red);
 mvwprintz(w, 2, 1, col, "> %d nails", nails);
 col = (g->u.has_amount(itm_2x4, boards) ? c_green : c_red);
 mvwprintz(w, 3, 1, col, "> %d two by fours", boards);

 if (g->u.has_amount(itm_nail, nails) && g->u.has_amount(itm_2x4, boards)) {
  mvwprintz(w, 5, 1, c_yellow, "Perform action? (y/n)");
  wrefresh(w);
  char ch;
  do
   ch = getch();
  while (ch != 'y' && ch != 'Y' && ch != 'n' && ch != 'N');
  if (ch == 'y' || ch == 'Y') {
   g->u.moves -= boards * 50;
   g->u.use_up(itm_nail, nails);
   g->u.use_up(itm_2x4, boards);
   g->m.ter(dirx, diry) = newter;
  }
 } else {
  mvwprintz(w, 5, 1, c_red, "Cannot perform action.");
  wrefresh(w);
  getch();
 }
 delwin(w);
}
 
void iuse::light_off(game *g, item *it, bool t)
{
 if (it->charges == 0)
  g->add_msg("The flaslight's batteries are dead.");
 else {
  g->add_msg("You turn the flashlight on.");
  it->make(g->itypes[itm_flashlight_on]);
  it->active = true;
 }
}
 
void iuse::light_on(game *g, item *it, bool t)
{
 if (t) {	// Normal use
  if (g->turn % 20 == 0)
   it->charges--;	// Flashlights last a long long time.
 } else {	// Turning it off
  g->add_msg("The flashlight flicks off.");
  it->make(g->itypes[itm_flashlight]);
  it->active = false;
 }
}

void iuse::water_purifier(game *g, item *it, bool t)
{
 char ch = g->inv("Purify what?");
 if (!g->u.has_item(ch)) {
  g->add_msg("You do not have that idea!");
  return;
 }
 if (g->u.i_at(ch).contents.size() == 0) {
  g->add_msg("You can only purify water.");
  return;
 }
 item *pure = &(g->u.i_at(ch).contents[0]);
 if (pure->type->id != itm_water && pure->type->id != itm_water_dirty &&
     pure->type->id != itm_salt_water) {
  g->add_msg("You can only purify water.");
  return;
 }
 g->u.moves -= 150;
 pure->make(g->itypes[itm_water]);
}

void iuse::two_way_radio(game *g, item *it, bool t)
{
 WINDOW* w = newwin(6, 36, 9, 5);
 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
// TODO: More options here.  Thoughts...
//       > Respond to the SOS of an NPC
//       > Report something to a faction
//       > Call another player
 mvwprintz(w, 1, 1, c_white, "1: Radio a faction for help...");
 mvwprintz(w, 2, 1, c_white, "2: Call Acquaitance...");
 mvwprintz(w, 3, 1, c_white, "3: General S.O.S.");
 mvwprintz(w, 4, 1, c_white, "0: Cancel");
 wrefresh(w);
 char ch = getch();
 if (ch == '1') {
  g->u.moves -= 300;
  faction* fac = g->list_factions("Call for help...");
  if (fac == NULL) {
   it->charges++;
   return;
  }
  int bonus = 0;
  if (fac->goal == FACGOAL_CIVILIZATION)
   bonus += 2;
  if (fac->has_job(FACJOB_MERCENARIES))
   bonus += 4;
  if (fac->has_job(FACJOB_DOCTORS))
   bonus += 2;
  if (fac->has_value(FACVAL_CHARITABLE))
   bonus += 3;
  if (fac->has_value(FACVAL_LONERS))
   bonus -= 3;
  if (fac->has_value(FACVAL_TREACHERY))
   bonus -= rng(0, 8);
  bonus += fac->respects_u + 3 * fac->likes_u;
  if (bonus >= 25) {
   popup("They reply, \"Help is on the way!\"");
   g->add_event(EVENT_HELP, g->turn + fac->response_time(g), fac);
   fac->respects_u -= rng(0, 8);
   fac->likes_u -= rng(3, 5);
  } else if (bonus >= -5) {
   popup("They reply, \"Sorry, you're on your own!\"");
   fac->respects_u -= rng(0, 5);
  } else
   popup("They reply, \"Hah!  We hope you die!\"");

 } else if (ch == '2') {	// Call Acquaitance
// TODO: Implement me!
 } else if (ch == '3') {	// General S.O.S.
  g->u.moves -= 150;
  std::vector<npc*> in_range;
  for (int i = 0; i < g->cur_om.npcs.size(); i++) {
   if (g->cur_om.npcs[i].op_of_u.value >= 4 &&
       trig_dist(g->levx, g->levy, g->cur_om.npcs[i].mapx,
                                   g->cur_om.npcs[i].mapy) <= 30)
    in_range.push_back(&(g->cur_om.npcs[i]));
  }
  if (in_range.size() > 0) {
   npc* coming = in_range[rng(0, in_range.size() - 1)];
   popup("A reply!  %s says, \"I'm on my way; give me %d minutes!\"",
         coming->name.c_str(), coming->minutes_to_u(g));
   coming->mission = MISSION_RESCUE_U;
  } else
   popup("No-one seems to reply...");
 } else
  it->charges++;	// Canceled the call, get our charge back
 werase(w);
 wrefresh(w);
 delwin(w);
 refresh();
}
 
void iuse::radio_off(game *g, item *it, bool t)
{
 if (it->charges == 0)
  g->add_msg("It's dead.");
 else {
  g->add_msg("You turn the radio on.");
  it->make(g->itypes[itm_radio_on]);
  it->active = true;
 }
}

void iuse::radio_on(game *g, item *it, bool t)
{
 if (t) {	// Normal use
  if (g->turn % 12 == 0)
   it->charges--;	// Radios last a long long time.
  int best_signal = 0;
  std::string message = "Radio: Kssssssssssssh.";
  for (int k = 0; k < g->cur_om.radios.size(); k++) {
   int signal = g->cur_om.radios[k].strength -
                trig_dist(g->cur_om.radios[k].x, g->cur_om.radios[k].y,
                          g->levx, g->levy);
   if (signal > best_signal) {
    best_signal = signal;
    message = g->cur_om.radios[k].message;
   }
  }
  if (best_signal > 0) {
   for (int j = 0; j < message.length(); j++) {
    if (dice(10, 100) > dice(10, best_signal * 5)) {
     if (!one_in(10))
      message[j] = '#';
     else
      message[j] = char(rng('a', 'z'));
    }
   }
   message = "radio: " + message;
  }
  point p = g->find_item(it);
  g->sound(p.x, p.y, 6, message.c_str());
 } else {	// Turning it off
  g->add_msg("The radio dies.");
  it->make(g->itypes[itm_radio]);
  it->active = false;
 }
}

void iuse::crowbar(game *g, item *it, bool t)
{
 int dirx, diry;
 g->draw();
 mvprintw(0, 0, "Pry where?");
 get_direction(dirx, diry, input());
 if (dirx == -2) {
  g->add_msg("Invalid direction.");
  return;
 }
 dirx += g->u.posx;
 diry += g->u.posy;
 if (g->m.ter(dirx, diry) == t_door_c || g->m.ter(dirx, diry) == t_door_locked){
  if (dice(4, 6) < dice(4, g->u.str_cur)) {
   g->add_msg("You pry the door open.");
   g->u.moves -= (150 - (g->u.str_cur * 5));
   g->m.ter(dirx, diry) = t_door_o;
  } else {
   g->add_msg("You pry, but cannot open the door.");
   g->u.moves -= 100;
  }
 } else if (g->m.ter(dirx, diry) == t_manhole_cover) {
  if (dice(8, 8) < dice(8, g->u.str_cur)) {
   g->add_msg("You lift the manhole cover.");
   g->u.moves -= (500 - (g->u.str_cur * 5));
   g->m.ter(dirx, diry) = t_manhole;
   g->m.add_item(g->u.posx, g->u.posy, g->itypes[itm_manhole_cover], 0);
  } else {
   g->add_msg("You pry, but cannot lift the manhole cover.");
   g->u.moves -= 100;
  }
 } else
  g->add_msg("There's nothing to pry there.");
}

void iuse::makemound(game *g, item *it, bool t)
{
 if (g->m.has_flag(diggable, g->u.posx, g->u.posy)) {
  g->add_msg("You churn up the earth here.");
  g->u.moves = -300;
  g->m.ter(g->u.posx, g->u.posy) = t_dirtmound;
 } else
  g->add_msg("You can't churn up this ground.");
}

void iuse::dig(game *g, item *it, bool t)
{
 int dirx, diry;
 g->draw();
 mvprintw(0, 0, "Dig where?");
 get_direction(dirx, diry, input());
 if (dirx == -2) {
  g->add_msg("Invalid direction.");
  return;
 }
 if (g->m.has_flag(diggable, g->u.posx + dirx, g->u.posy + diry)) {
  g->u.moves -= 300;
  g->add_msg("You dig a pit.");
  g->m.ter     (g->u.posx + dirx, g->u.posy + diry) = t_pit;
  g->m.add_trap(g->u.posx + dirx, g->u.posy + diry, tr_pit);
  g->u.practice(sk_traps, 1);
 } else
  g->add_msg("You can't dig through %d!",
             g->m.tername(g->u.posx + dirx, g->u.posy + diry).c_str());
}

void iuse::chainsaw_off(game *g, item *it, bool t)
{
 g->u.moves -= 80;
 if (rng(0, 10) - it->damage > 5 && it->charges > 0) {
  g->sound(g->u.posx, g->u.posy, 20,
           "With a roar, the chainsaw leaps to life!");
  it->make(g->itypes[itm_chainsaw_on]);
  it->active = true;
 } else
  g->add_msg("You yank the cord, but nothing happens.");
}

void iuse::chainsaw_on(game *g, item *it, bool t)
{
 if (t) {	// Effects while simply on
  if (one_in(15))
   g->sound(g->u.posx, g->u.posy, 12, "Your chainsaw rumbles.");
 } else {	// Toggling
  g->add_msg("Your chainsaw dies.");
  it->make(g->itypes[itm_chainsaw_off]);
  it->active = false;
 }
}

void iuse::jackhammer(game *g, item *it, bool t)
{
 int dirx, diry;
 g->draw();
 mvprintw(0, 0, "Drill in which direction?");
 get_direction(dirx, diry, input());
 if (dirx == -2) {
  g->add_msg("Invalid direction.");
  return;
 }
 dirx += g->u.posx;
 diry += g->u.posy;
 if (g->m.is_destructable(dirx, diry)) {
  g->m.destroy(g, dirx, diry, false);
  g->u.moves -= 500;
  g->sound(dirx, diry, 45, "TATATATATATATAT!");
 } else {
  g->add_msg("You can't drill there.");
  it->charges += (dynamic_cast<it_tool*>(it->type))->charges_per_use;
 }
}

void iuse::set_trap(game *g, item *it, bool t)
{
 int dirx, diry;
 g->draw();
 mvprintw(0, 0, "Place where?");
 get_direction(dirx, diry, input());
 if (dirx == -2) {
  g->add_msg("Invalid direction.");
  return;
 }
 int posx = dirx + g->u.posx;
 int posy = diry + g->u.posy;
 if (g->m.move_cost(posx, posy) != 2) {
  g->add_msg("You can't place a %s there.", it->tname().c_str());
  return;
 }

 trap_id type = tr_null;
 bool buried = false;
 std::stringstream message;
 int practice;

 switch (it->type->id) {
 case itm_bubblewrap:
  message << "You set the bubblewrap on the ground, ready to be popped.";
  type = tr_bubblewrap;
  practice = 2;
  break;
 case itm_beartrap:
  buried = (g->u.has_amount(itm_shovel, 1) && query_yn("Bury the beartrap?"));
  type = (buried ? tr_beartrap_buried : tr_beartrap);
  message << "You " << (buried ? "bury" : "set") << " the beartrap.";
  practice = (buried ? 7 : 4); 
  break;
 case itm_board_trap:
  message << "You set the board trap on the " << g->m.tername(posx, posy) <<
             ", nails facing up.";
  type = tr_nailboard;
  practice = 2;
  break;
 case itm_tripwire:
// Must have a connection between solid squares.
  if ((g->m.move_cost(posx    , posy - 1) != 2 &&
       g->m.move_cost(posx    , posy + 1) != 2   ) ||
      (g->m.move_cost(posx + 1, posy    ) != 2 &&
       g->m.move_cost(posx - 1, posy    ) != 2   ) ||
      (g->m.move_cost(posx - 1, posy - 1) != 2 &&
       g->m.move_cost(posx + 1, posy + 1) != 2   ) ||
      (g->m.move_cost(posx + 1, posy - 1) != 2 &&
       g->m.move_cost(posx - 1, posy + 1) != 2   )) {
   message << "You string up the tripwire.";
   type= tr_tripwire;
   practice = 3;
  } else {
   g->add_msg("You must place the tripwire between two solid tiles.");
   return;
  }
  break;
 case itm_crossbow_trap:
  message << "You set the crossbow trap.";
  type = tr_crossbow;
  practice = 4;
  break;
 case itm_shotgun_trap:
  message << "You set the shotgun trap.";
  type = tr_shotgun_2;
  practice = 5;
  break;
 case itm_blade_trap:
  posx += dirx;
  posy += diry;
  for (int i = -1; i <= 1; i++) {
   for (int j = -1; j <= 1; j++) {
    if (g->m.move_cost(posx + i, posy + j) != 2) {
     g->add_msg("\
That trap needs a 3x3 space to be clear, centered two tiles from you.");
     return;
    }
   }
  }
  message << "You set the blade trap two squares away.";
  type = tr_engine;
  practice = 12;
  break;
 case itm_landmine:
  buried = true;
  message << "You bury the landmine.";
  type = tr_landmine;
  practice = 7;
  break;
 default:
  g->add_msg("Tried to set a trap.  But got confused! %s", it->tname().c_str());
  return;
 }

 if (buried) {
  if (!g->u.has_amount(itm_shovel, 1)) {
   g->add_msg("You need a shovel.");
   return;
  } else if (!g->m.has_flag(diggable, posx, posy)) {
   g->add_msg("You can't dig in that %s", g->m.tername(posx, posy).c_str());
   return;
  }
 }

 g->add_msg(message.str().c_str());
 g->u.practice(sk_traps, practice);
 g->m.add_trap(posx, posy, type);
 g->u.moves -= practice * 25;
 if (type == tr_engine) {
  for (int i = -1; i <= 1; i++) {
   for (int j = -1; j <= 1; j++) {
    if (i != 0 || j != 0)
     g->m.add_trap(posx + i, posy + j, tr_blade);
   }
  }
 }
 g->u.i_rem(it->invlet);
}

void iuse::geiger(game *g, item *it, bool t)
{
 WINDOW* w = newwin(6, 16, 8, 1);
 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 mvwprintz(w, 1, 1, c_ltred,  "Scan what?");
 mvwprintz(w, 2, 1, c_ltgray, "1: Yourself");
 mvwprintz(w, 3, 1, c_ltgray, "2: The ground");
 mvwprintz(w, 4, 1, c_ltgray, "3: Cancel");
 wrefresh(w);
 char ch;
 do {
  ch = getch();
  if (ch == '1')
   g->add_msg("Your radiation level: %d", g->u.radiation);
  else if (ch == '2')
   g->add_msg("The ground's radiation level: %d",
              g->m.radiation(g->u.posx, g->u.posy));
  else if (ch == '3')
   it->charges++;
 } while (ch < '1' || ch > '3');
}

void iuse::teleport(game *g, item *it, bool t)
{
 g->teleport();
}

void iuse::can_goo(game *g, item *it, bool t)
{
 int tries = 0, goox, gooy, junk;
 do {
  goox = g->u.posx + rng(-2, 2);
  gooy = g->u.posy + rng(-2, 2);
  tries++;
 } while (g->m.move_cost(goox, gooy) == 0 && tries < 10);
 if (tries == 10)
  return;
 int mondex = g->mon_at(goox, gooy);
 if (mondex != -1) {
  if (g->u_see(goox, gooy, junk))
   g->add_msg("Black goo emerges from the canister and envelopes a %s!",
              g->z[mondex].name().c_str());
  g->z[mondex].poly(g->mtypes[mon_blob]);
  g->z[mondex].speed -= rng(5, 25);
  g->z[mondex].hp = g->z[mondex].speed;
 } else {
  if (g->u_see(goox, gooy, junk))
   g->add_msg("Living black goo emerges from the canister!");
  monster goo(g->mtypes[mon_blob]);
  goo.spawn(goox, gooy);
  g->z.push_back(goo);
 }
 tries = 0;
 while (!one_in(5) && tries < 10) {
  tries = 0;
  do {
   goox = g->u.posx + rng(-2, 2);
   gooy = g->u.posy + rng(-2, 2);
   tries++;
  } while (g->m.move_cost(goox, gooy) == 0 &&
           g->m.tr_at(goox, gooy) != tr_null && tries < 10);
  if (g->m.tr_at(goox, gooy) == tr_null) {
   if (g->u_see(goox, gooy, junk))
    g->add_msg("A nearby splatter of goo forms into a goo pit.");
   g->m.tr_at(goox, gooy) = tr_goo;
  }
 }
}

void iuse::pipebomb(game *g, item *it, bool t)
{
 if (!g->u.has_amount(itm_lighter, 1)) {
  g->add_msg("You need a lighter!");
  return;
 }
 g->u.use_up(itm_lighter, 1);
 g->add_msg("You light the fuse on the pipe bomb.");
 it->make(g->itypes[itm_pipebomb_act]);
 it->charges = 3;
 it->active = true;
}

void iuse::pipebomb_act(game *g, item *it, bool t)
{
 int linet;
 point pos = g->find_item(it);
 if (pos.x == -999 || pos.y == -999)
  return;
 if (t) // Simple timer effects
  g->sound(pos.x, pos.y, 0, "Ssssss");	// Vol 0 = only heard if you hold it
 else {	// The timer has run down
  if (one_in(10) && g->u_see(pos.x, pos.y, linet))
   g->add_msg("The pipe bomb fizzles out.");
  else
   g->explosion(pos.x, pos.y, rng(6, 14), rng(0, 4), false);
 }
}
 
void iuse::grenade(game *g, item *it, bool t)
{
 g->add_msg("You pull the pin on the grenade.");
 it->make(g->itypes[itm_grenade_act]);
 it->charges = 5;
 it->active = true;
}

void iuse::grenade_act(game *g, item *it, bool t)
{
 point pos = g->find_item(it);
 if (pos.x == -999 || pos.y == -999)
  return;
 if (t) // Simple timer effects
  g->sound(pos.x, pos.y, 0, "Tick.");	// Vol 0 = only heard if you hold it
 else	// When that timer runs down...
  g->explosion(pos.x, pos.y, 18, 12, false);
}

void iuse::EMPbomb(game *g, item *it, bool t)
{
 g->add_msg("You pull the pin on the EMP grenade.");
 it->make(g->itypes[itm_EMPbomb_act]);
 it->charges = 3;
 it->active = true;
}

void iuse::EMPbomb_act(game *g, item *it, bool t)
{
 point pos = g->find_item(it);
 if (pos.x == -999 || pos.y == -999)
  return;
 if (t)	// Simple timer effects
  g->sound(pos.x, pos.y, 0, "Tick.");	// Vol 0 = only heard if you hold it
 else {	// When that timer runs down...
  for (int x = pos.x - 4; x <= pos.x + 4; x++) {
   for (int y = pos.y - 4; y <= pos.y + 4; y++)
    g->emp_blast(x, y);
  }
 }
}

void iuse::gasbomb(game *g, item *it, bool t)
{
 g->add_msg("You pull the pin on the teargas canister.");
 it->make(g->itypes[itm_gasbomb_act]);
 it->charges = 20;
 it->active = true;
}

void iuse::gasbomb_act(game *g, item *it, bool t)
{
 point pos = g->find_item(it);
 if (pos.x == -999 || pos.y == -999)
  return;
 if (t) {
  if (it->charges > 15)
   g->sound(pos.x, pos.y, 0, "Tick.");	// Vol 0 = only heard if you hold it
  else {
   int junk;
   for (int i = -2; i <= 2; i++) {
    for (int j = -2; j <= 2; j++) {
     if (g->m.sees(pos.x, pos.y, pos.x + i, pos.y + j, 3, junk) &&
         g->m.move_cost(pos.x + i, pos.y + j) > 0)
      g->m.add_field(g, pos.x + i, pos.y + j, fd_tear_gas, 3);
    }
   }
  }
 } else
  it->make(g->itypes[itm_canister_empty]);
}

void iuse::smokebomb(game *g, item *it, bool t)
{
 g->add_msg("You pull the pin on the smoke bomb.");
 it->make(g->itypes[itm_smokebomb_act]);
 it->charges = 20;
 it->active = true;
}

void iuse::smokebomb_act(game *g, item *it, bool t)
{
 point pos = g->find_item(it);
 if (pos.x == -999 || pos.y == -999)
  return;
 if (t) {
  if (it->charges > 17)
   g->sound(pos.x, pos.y, 0, "Tick.");	// Vol 0 = only heard if you hold it
  else {
   int junk;
   for (int i = -2; i <= 2; i++) {
    for (int j = -2; j <= 2; j++) {
     if (g->m.sees(pos.x, pos.y, pos.x + i, pos.y + j, 3, junk) &&
         g->m.move_cost(pos.x + i, pos.y + j) > 0)
      g->m.add_field(g, pos.x + i, pos.y + j, fd_smoke, rng(1, 2) + rng(0, 1));
    }
   }
  }
 } else
  it->make(g->itypes[itm_canister_empty]);
}


void iuse::molotov(game *g, item *it, bool t)
{
 if (!g->u.has_amount(itm_lighter, 1)) {
  g->add_msg("You need a lighter!");
  return;
 }
 g->u.use_up(itm_lighter, 1);
 g->add_msg("You light the molotov cocktail.");
 g->u.moves -= 150;
 it->make(g->itypes[itm_molotov_lit]);
 it->charges = 1;
 it->active = true;
}
 
void iuse::molotov_lit(game *g, item *it, bool t)
{
 if (!g->u.has_item(it)) {
  point pos = g->find_item(it);
  it->charges = 0;
  g->explosion(pos.x, pos.y, 8, 0, true);
 } else if (one_in(20)) {
  g->add_msg("Your lit molotov goes out.");
  it->make(g->itypes[itm_molotov]);
  it->charges = 0;
  it->active = false;
 }
}

void iuse::dynamite(game *g, item *it, bool t)
{
 if (!g->u.has_amount(itm_lighter, 1)) {
  g->add_msg("You need a lighter!");
  return;
 }
 g->u.use_up(itm_lighter, 1);
 g->add_msg("You light the dynamite.");
 it->make(g->itypes[itm_dynamite_act]);
 it->charges = 20;
 it->active = true;
}

void iuse::dynamite_act(game *g, item *it, bool t)
{
 point pos = g->find_item(it);
 if (pos.x == -999 || pos.y == -999)
  return;
 if (t) // Simple timer effects
  g->sound(pos.x, pos.y, 0, "ssss...");
 else	// When that timer runs down...
  g->explosion(pos.x, pos.y, 60, 0, false);
}

void iuse::mininuke(game *g, item *it, bool t)
{
 g->add_msg("You activate the mininuke.");
 it->make(g->itypes[itm_mininuke_act]);
 it->charges = 10;
 it->active = true;
}

void iuse::mininuke_act(game *g, item *it, bool t)
{
 point pos = g->find_item(it);
 if (pos.x == -999 || pos.y == -999)
  return;
 if (t) 	// Simple timer effects
  g->sound(pos.x, pos.y, 2, "Tick.");
 else {	// When that timer runs down...
  g->explosion(pos.x, pos.y, 200, 0, false);
  int junk;
  for (int i = -4; i <= 4; i++) {
   for (int j = -4; j <= 4; j++) {
    if (g->m.sees(pos.x, pos.y, pos.x + i, pos.y + j, 3, junk) &&
        g->m.move_cost(pos.x + i, pos.y + j) > 0)
     g->m.add_field(g, pos.x + i, pos.y + j, fd_nuke_gas, 3);
   }
  }
 }
}

void iuse::pheromone(game *g, item *it, bool t)
{
 point pos = g->find_item(it);
 if (pos.x == -999 || pos.y == -999)
  return;

 g->add_msg("You squeeze the pheromone ball...");
 g->u.moves -= 15;

 int converts = 0;
 for (int x = pos.x - 4; x <= pos.x + 4; x++) {
  for (int y = pos.y - 4; y <= pos.y + 4; y++) {
   int mondex = g->mon_at(x, y);
   if (mondex != -1 && g->z[mondex].symbol() == 'Z' &&
       g->z[mondex].friendly == 0 && rng(0, 500) > g->z[mondex].hp) {
    converts++;
    g->z[mondex].make_friendly();
   }
  }
 }

 if (converts == 0)
  g->add_msg("...but nothing happens.");
 else if (converts == 1)
  g->add_msg("...and a nearby zombie turns friendly!");
 else
  g->add_msg("...and several nearby zombies turn friendly!");
}
 

void iuse::portal(game *g, item *it, bool t)
{
 g->m.add_trap(g->u.posx + rng(-2, 2), g->u.posy + rng(-2, 2), tr_portal);
}

void iuse::manhack(game *g, item *it, bool t)
{
 std::vector<point> valid;	// Valid spawn locations
 for (int x = g->u.posx - 1; x <= g->u.posx + 1; x++) {
  for (int y = g->u.posy - 1; y <= g->u.posy + 1; y++) {
   if (g->is_empty(x, y))
    valid.push_back(point(x, y));
  }
 }
 if (valid.size() == 0) {	// No valid points!
  g->add_msg("There is no adjacent square to release the manhack in!");
  return;
 }
 int index = rng(0, valid.size() - 1);
 g->u.moves -= 60;
 g->u.i_rem(it->invlet);	// Remove the manhack from the player's inv
 monster manhack(g->mtypes[mon_manhack], valid[index].x, valid[index].y);
 if (rng(0, g->u.int_cur / 2) + g->u.sklevel[sk_electronics] / 2 +
     g->u.sklevel[sk_computer] < rng(0, 4))
  g->add_msg("You misprogram the manhack; it's hostile!");
 else
  manhack.friendly = -1;
 g->z.push_back(manhack);
}

void iuse::UPS_off(game *g, item *it, bool t)
{
 if (it->charges == 0)
  g->add_msg("The power supply's batteries are dead.");
 else {
  g->add_msg("You turn the power supply on.");
  if (g->u.is_wearing(itm_goggles_nv))
   g->add_msg("Your light amp goggles power on.");
  it->make(g->itypes[itm_UPS_on]);
  it->active = true;
 }
}
 
void iuse::UPS_on(game *g, item *it, bool t)
{
 if (t) {	// Normal use
	// Does nothing
 } else {	// Turning it off
  g->add_msg("The UPS powers off with a soft hum.");
  it->make(g->itypes[itm_UPS_off]);
  it->active = false;
 }
}

void iuse::tazer(game *g, item *it, bool t)
{
 int dirx, diry;
 g->draw();
 mvprintw(0, 0, "Shock in which direction?");
 get_direction(dirx, diry, input());
 if (dirx == -2) {
  g->add_msg("Invalid direction.");
  it->charges += (dynamic_cast<it_tool*>(it->type))->charges_per_use;
  return;
 }
 int sx = dirx + g->u.posx, sy = diry + g->u.posy;
 int mondex = g->mon_at(sx, sy);
 int npcdex = g->npc_at(sx, sy);
 if (mondex == -1 && npcdex == -1) {
  g->add_msg("Your tazer crackles in the air.");
  return;
 }

 int numdice = 3 + (g->u.dex_cur / 2.5) + g->u.sklevel[sk_melee] * 2;
 g->u.moves -= 100;

 if (mondex != -1) {
  monster *z = &(g->z[mondex]);
  switch (z->type->size) {
   case MS_TINY:  numdice -= 2; break;
   case MS_SMALL: numdice -= 1; break;
   case MS_LARGE: numdice += 2; break;
   case MS_HUGE:  numdice += 4; break;
  }
  int mondice = z->dodge();
  if (dice(numdice, 10) < dice(mondice, 10)) {	// A miss!
   g->add_msg("You attempt to shock the %s, but miss.", z->name().c_str());
   return;
  }
  if (z->has_flag(MF_SHOCK)) {
   g->add_msg("You shock the %s, but it seems immune!", z->name().c_str());
   return;
  }
  g->add_msg("You shock the %s!", z->name().c_str());
  int shock = rng(5, 25);
  z->moves -= shock * 100;
  if (z->hurt(shock))
   g->kill_mon(mondex);
  return;
 }
 
 if (npcdex != -1) {
  npc *foe = dynamic_cast<npc*>(&g->active_npc[npcdex]);
  if (foe->attitude != NPCATT_FLEE)
   foe->attitude = NPCATT_KILL;
  if (foe->str_max >= 17)
    numdice++;	// Minor bonus against huge people
  else if (foe->str_max <= 5)
   numdice--;	// Minor penalty against tiny people
  if (dice(numdice, 10) <= dice(foe->dodge(), 6)) {
   g->add_msg("You attempt to shock %s, but miss.", foe->name.c_str());
   return;
  }
  g->add_msg("You shock %s!", foe->name.c_str());
  int shock = rng(5, 20);
  foe->moves -= shock * 100;
  foe->hurtall(shock);
  if (foe->hp_cur[hp_head]  <= 0 || foe->hp_cur[hp_torso] <= 0) {
   foe->die(g, true);
   g->active_npc.erase(g->active_npc.begin() + npcdex);
  }
 }

}

void iuse::mp3(game *g, item *it, bool t)
{
 if (it->charges == 0)
  g->add_msg("The mp3 player's batteries are dead.");
 else {
  g->add_msg("You put in the earbuds and start listening to music.");
  it->make(g->itypes[itm_mp3_on]);
  it->active = true;
 }
}

void iuse::mp3_on(game *g, item *it, bool t)
{
 if (t) {	// Normal use
  g->u.add_morale(MOR_MUSIC, rng(0, 1), rng(2, 3));

  if (g->turn % 10 == 0) {	// Every 10 turns, describe the music
   std::string sound = "";
   switch (rng(1, 10)) {
    case 1: sound = "a sweet guitar solo!";	g->u.stim++;	break;
    case 2: sound = "a funky bassline.";			break;
    case 3: sound = "some amazing vocals.";			break;
    case 4: sound = "some pumping bass.";			break;
    case 5: sound = "dramatic classical music.";
            if (g->u.int_cur >= 10)
             g->u.add_morale(MOR_MUSIC, 1, 1);			break;
   }
   if (sound.length() > 0)
    g->add_msg("You listen to %s", sound.c_str());
  }
 } else {	// Turning it off
  g->add_msg("The mp3 player turns off.");
  it->make(g->itypes[itm_mp3]);
  it->active = false;
 }
}
