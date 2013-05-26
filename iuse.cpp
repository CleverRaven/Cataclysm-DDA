#include "iuse.h"
#include "game.h"
#include "mapdata.h"
#include "keypress.h"
#include "output.h"
#include "rng.h"
#include "line.h"
#include "player.h"
#include <sstream>

#define RADIO_PER_TURN 25 // how many characters per turn of radio

// mfb(n) converts a flag to its appropriate position in covers's bitfield
#ifndef mfb
#define mfb(n) long(1 << (n))
#endif

static void add_or_drop_item(game *g, player *p, item *it)
{
  item replacement(g->itypes[it->type->id], int(g->turn), g->nextinv);
  bool drop = false;
  int iter = 0;
  // Should this vary based on how many charges get consumed by default?
  if (replacement.charges >= 0)
  {
      replacement.charges = 1;
  }
  while (p->has_item(replacement.invlet)) {
    replacement.invlet = g->nextinv;
    g->advance_nextinv();
    iter++;
  }
  if (!drop && (iter == inv_chars.size() || p->volume_carried() >= p->volume_capacity()))
    drop = true;
  if (drop)
    g->m.add_item(p->posx, p->posy, replacement);
  else
    p->i_add(replacement, g);
}

static bool use_fire(game *g, player *p, item *it)
{
    if (!p->use_charges_if_avail("fire", 1))
    {
        add_or_drop_item(g, p, it);
        g->add_msg_if_player(p, "You need a lighter!");
        return false;
    }
    return true;
}

void iuse::none(game *g, player *p, item *it, bool t)
{
  g->add_msg("You can't do anything interesting with your %s.",
             it->tname(g).c_str());
}
/* To mark an item as "removed from inventory", set its invlet to 0
   This is useful for traps (placed on ground), inactive bots, etc
 */
void iuse::sewage(game *g, player *p, item *it, bool t)
{
 p->vomit(g);
 if (one_in(4))
  p->mutate(g);
}

void iuse::honeycomb(game *g, player *p, item *it, bool t)
{
  g->m.spawn_item(p->posx, p->posy, g->itypes["wax"],0, 2);
}

void iuse::royal_jelly(game *g, player *p, item *it, bool t)
{
// TODO: Add other diseases here; royal jelly is a cure-all!
 p->pkill += 5;
 std::string message;
 if (p->has_disease(DI_FUNGUS)) {
  message = "You feel cleansed inside!";
  p->rem_disease(DI_FUNGUS);
 }
 if (p->has_disease(DI_BLIND)) {
  message = "Your sight returns!";
  p->rem_disease(DI_BLIND);
 }
 if (p->has_disease(DI_POISON) || p->has_disease(DI_FOODPOISON) ||
     p->has_disease(DI_BADPOISON)) {
  message = "You feel much better!";
  p->rem_disease(DI_POISON);
  p->rem_disease(DI_BADPOISON);
  p->rem_disease(DI_FOODPOISON);
 }
 if (p->has_disease(DI_ASTHMA)) {
  message = "Your breathing clears up!";
  p->rem_disease(DI_ASTHMA);
 }
 if (p->has_disease(DI_COMMON_COLD) || p->has_disease(DI_FLU)) {
  message = "You feel healthier!";
  p->rem_disease(DI_COMMON_COLD);
  p->rem_disease(DI_FLU);
 }
 g->add_msg_if_player(p,message.c_str());
}

void iuse::bandage(game *g, player *p, item *it, bool t)
{
    int bonus = p->skillLevel("firstaid");
    hp_part healed;

    if (p->is_npc()) { // NPCs heal whichever has sustained the most damage
        int highest_damage = 0;
        for (int i = 0; i < num_hp_parts; i++) {
            int damage = p->hp_max[i] - p->hp_cur[i];
            if (i == hp_head)
                damage *= 1.5;
            if (i == hp_torso)
                damage *= 1.2;
            if (damage > highest_damage) {
                highest_damage = damage;
                healed = hp_part(i);
            }
        }
    } else { // Player--present a menu
        WINDOW* hp_window = newwin(11, 22, (TERMY-10)/2, (TERMX-22)/2);
        wborder(hp_window, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                           LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );

        mvwprintz(hp_window, 1, 1, c_ltred,  "Use Bandage:");
        if(p->hp_cur[hp_head] < p->hp_max[hp_head])
        {
            mvwprintz(hp_window, 2, 1, c_ltgray, "1: Head");
        }
        if(p->hp_cur[hp_torso] < p->hp_max[hp_torso])
        {
            mvwprintz(hp_window, 3, 1, c_ltgray, "2: Torso");
        }
        if(p->hp_cur[hp_arm_l] < p->hp_max[hp_arm_l])
        {
            mvwprintz(hp_window, 4, 1, c_ltgray, "3: Left Arm");
        }
        if(p->hp_cur[hp_arm_r] < p->hp_max[hp_arm_r])
        {
            mvwprintz(hp_window, 5, 1, c_ltgray, "4: Right Arm");
        }
        if(p->hp_cur[hp_leg_l] < p->hp_max[hp_leg_l])
        {
            mvwprintz(hp_window, 6, 1, c_ltgray, "5: Left Leg");
        }
        if(p->hp_cur[hp_leg_r] < p->hp_max[hp_leg_r])
        {
            mvwprintz(hp_window, 7, 1, c_ltgray, "6: Right Leg");
        }
        if(p->has_disease(DI_BLEED))
        {
            mvwprintz(hp_window, 8, 1, c_ltgray, "7: Stop Bleeding");
        }
        mvwprintz(hp_window, 9, 1, c_ltgray, "8: Exit");
        nc_color color;
        std::string asterisks = "";
        int current_hp;
        for (int i = 0; i < num_hp_parts; i++) {
            current_hp = p->hp_cur[i];
            int temporary_bonus = bonus;
            if (current_hp != 0) {
                switch (hp_part(i)) {
                    case hp_head:
                        current_hp += 1;
                        temporary_bonus *=  .8;
                        break;
                    case hp_torso:
                        current_hp += 4;
                        temporary_bonus *= 1.5;
                        break;
                    default:
                        current_hp += 3;
                        break;
                }
                current_hp += temporary_bonus;
                if (current_hp > p->hp_max[i]){
                  current_hp = p->hp_max[i];
                }
                if (current_hp == p->hp_max[i]){
                  color = c_green;
                  asterisks = "***";
                } else if (current_hp > p->hp_max[i] * .8) {
                  color = c_ltgreen;
                  asterisks = "***";
                } else if (current_hp > p->hp_max[i] * .5) {
                  color = c_yellow;
                  asterisks = "** ";
                } else if (current_hp > p->hp_max[i] * .3) {
                  color = c_ltred;
                  asterisks = "** ";
                } else {
                  color = c_red;
                  asterisks = "*  ";
                }
                if (p->has_trait(PF_SELFAWARE)) {
                    if (current_hp >= 100){
                        mvwprintz(hp_window, i * 2 + 15, 0, color, "%d", current_hp);
                    } else if (current_hp >= 10) {
                        mvwprintz(hp_window, i * 2 + 16, 0, color, "%d", current_hp);
                    } else {
                        mvwprintz(hp_window, i * 2 + 17, 0, color, "%d", current_hp);
                    }
                } else {
                    mvwprintz(hp_window, i * 2 + 15, 0, color, asterisks.c_str());
                }
            } else { // curhp is 0; requires surgical attention
                mvwprintz(hp_window, i + 2, 15, c_dkgray, "---");
            }
        }
        wrefresh(hp_window);
        char ch;
        do {
            ch = getch();
            if (ch == '1'){
                healed = hp_head;
            } else if (ch == '2'){
                healed = hp_torso;
            } else if (ch == '3') {
                if (p->hp_cur[hp_arm_l] == 0) {
                    g->add_msg_if_player(p,"That arm is broken.  It needs surgical attention.");
                    add_or_drop_item(g, p, it);
                    return;
                } else {
                    healed = hp_arm_l;
                }
            } else if (ch == '4') {
                if (p->hp_cur[hp_arm_r] == 0) {
                    g->add_msg_if_player(p,"That arm is broken.  It needs surgical attention.");
                    add_or_drop_item(g, p, it);
                    return;
                } else {
                    healed = hp_arm_r;
                }
            } else if (ch == '5') {
                if (p->hp_cur[hp_leg_l] == 0) {
                    g->add_msg_if_player(p,"That leg is broken.  It needs surgical attention.");
                    add_or_drop_item(g, p, it);
                    return;
                } else {
                    healed = hp_leg_l;
                }
            } else if (ch == '6') {
                if (p->hp_cur[hp_leg_r] == 0) {
                    g->add_msg_if_player(p,"That leg is broken.  It needs surgical attention.");
                    add_or_drop_item(g, p, it);
                    return;
                } else {
                    healed = hp_leg_r;
                }
            } else if (ch == '8') {
                g->add_msg_if_player(p,"Never mind.");
                add_or_drop_item(g, p, it);
                return;
            } else if (ch == '7') {
                    g->add_msg_if_player(p,"You stopped the bleeding.");
                    p->rem_disease(DI_BLEED);
                    return;
            }
        } while (ch < '1' || ch > '8');
        werase(hp_window);
        wrefresh(hp_window);
        delwin(hp_window);
        refresh();
    }

    p->practice(g->turn, "firstaid", 8);
    int dam = 0;
    if (healed == hp_head){
        dam = 1 + bonus * .8;
    } else if (healed == hp_torso){
        dam = 4 + bonus * 1.5;
    } else {
        dam = 3 + bonus;
    }
    p->heal(healed, dam);
}

void iuse::firstaid(game *g, player *p, item *it, bool t)
{
    int bonus = p->skillLevel("firstaid");
    hp_part healed;

    if (p->is_npc()) { // NPCs heal whichever has sustained the most damage
        int highest_damage = 0;
        for (int i = 0; i < num_hp_parts; i++) {
            int damage = p->hp_max[i] - p->hp_cur[i];
            if (i == hp_head)
                damage *= 1.5;
            if (i == hp_torso)
                damage *= 1.2;
            if (damage > highest_damage) {
                highest_damage = damage;
                healed = hp_part(i);
            }
        }
    } else { // Player--present a menu
        WINDOW* hp_window = newwin(11, 22, (TERMY-10)/2, (TERMX-22)/2);
        wborder(hp_window, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                           LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
        mvwprintz(hp_window, 1, 1, c_ltred,  "Use First Aid:");
        if(p->hp_cur[hp_head] < p->hp_max[hp_head])
        {
            mvwprintz(hp_window, 2, 1, c_ltgray, "1: Head");
        }
        if(p->hp_cur[hp_torso] < p->hp_max[hp_torso])
        {
            mvwprintz(hp_window, 3, 1, c_ltgray, "2: Torso");
        }
        if(p->hp_cur[hp_arm_l] < p->hp_max[hp_arm_l])
        {
            mvwprintz(hp_window, 4, 1, c_ltgray, "3: Left Arm");
        }
        if(p->hp_cur[hp_arm_r] < p->hp_max[hp_arm_r])
        {
            mvwprintz(hp_window, 5, 1, c_ltgray, "4: Right Arm");
        }
        if(p->hp_cur[hp_leg_l] < p->hp_max[hp_leg_l])
        {
            mvwprintz(hp_window, 6, 1, c_ltgray, "5: Left Leg");
        }
        if(p->hp_cur[hp_leg_r] < p->hp_max[hp_leg_r])
        {
            mvwprintz(hp_window, 7, 1, c_ltgray, "6: Right Leg");
        }
        if(p->has_disease(DI_BITE))
        {
            mvwprintz(hp_window, 8, 1, c_ltgray, "7: Clean Wound");
        }
        mvwprintz(hp_window, 9, 1, c_ltgray, "8: Exit");
        nc_color color;
        std::string asterisks = "";
        int current_hp;
        for (int i = 0; i < num_hp_parts; i++) {
            current_hp = p->hp_cur[i];
            int temporary_bonus = bonus;
            if (current_hp != 0) {
                switch (hp_part(i)) {
                    case hp_head:
                        current_hp += 10;
                        temporary_bonus *=  .8;
                        break;
                    case hp_torso:
                        current_hp += 18;
                        temporary_bonus *= 1.5;
                        break;
                    default:
                        current_hp += 14;
                        break;
                }
                current_hp += temporary_bonus;
                if (current_hp > p->hp_max[i]){
                  current_hp = p->hp_max[i];
                }
                if (current_hp == p->hp_max[i]){
                  color = c_green;
                  asterisks = "***";
                } else if (current_hp > p->hp_max[i] * .8) {
                  color = c_ltgreen;
                  asterisks = "***";
                } else if (current_hp > p->hp_max[i] * .5) {
                  color = c_yellow;
                  asterisks = "** ";
                } else if (current_hp > p->hp_max[i] * .3) {
                  color = c_ltred;
                  asterisks = "** ";
                } else {
                  color = c_red;
                  asterisks = "*  ";
                }
                if (p->has_trait(PF_SELFAWARE)) {
                    if (current_hp >= 100){
                        mvwprintz(hp_window, i * 2 + 15, 0, color, "%d", current_hp);
                    } else if (current_hp >= 10) {
                        mvwprintz(hp_window, i * 2 + 16, 0, color, "%d", current_hp);
                    } else {
                        mvwprintz(hp_window, i * 2 + 17, 0, color, "%d", current_hp);
                    }
                } else {
                    mvwprintz(hp_window, i * 2 + 15, 0, color, asterisks.c_str());
                }
            } else { // curhp is 0; requires surgical attention
                mvwprintz(hp_window, i + 2, 15, c_dkgray, "---");
            }
        }
        wrefresh(hp_window);
        char ch;
        do {
            ch = getch();
            if (ch == '1'){
                healed = hp_head;
            } else if (ch == '2'){
                healed = hp_torso;
            } else if (ch == '3') {
                if (p->hp_cur[hp_arm_l] == 0) {
                    g->add_msg_if_player(p,"That arm is broken.  It needs surgical attention.");
                    add_or_drop_item(g, p, it);
                    return;
                } else {
                    healed = hp_arm_l;
                }
            } else if (ch == '4') {
                if (p->hp_cur[hp_arm_r] == 0) {
                    g->add_msg_if_player(p,"That arm is broken.  It needs surgical attention.");
                    add_or_drop_item(g, p, it);
                    return;
                } else {
                    healed = hp_arm_r;
                }
            } else if (ch == '5') {
                if (p->hp_cur[hp_leg_l] == 0) {
                    g->add_msg_if_player(p,"That leg is broken.  It needs surgical attention.");
                    add_or_drop_item(g, p, it);
                    return;
                } else {
                    healed = hp_leg_l;
                }
            } else if (ch == '6') {
                if (p->hp_cur[hp_leg_r] == 0) {
                    g->add_msg_if_player(p,"That leg is broken.  It needs surgical attention.");
                    add_or_drop_item(g, p, it);
                    return;
                } else {
                    healed = hp_leg_r;
                }
            } else if (ch == '8') {
                g->add_msg_if_player(p,"Never mind.");
                add_or_drop_item(g, p, it);
                return;
            } else if (ch == '7') {
                    g->add_msg_if_player(p,"You clean the bite wound.");
                    p->rem_disease(DI_BITE);
                    return;
            }
        } while (ch < '1' || ch > '8');
        werase(hp_window);
        wrefresh(hp_window);
        delwin(hp_window);
        refresh();
    }

    p->practice(g->turn, "firstaid", 8);
    int dam = 0;
    if (healed == hp_head){
        dam = 10 + bonus * .8;
    } else if (healed == hp_torso){
        dam = 18 + bonus * 1.5;
    } else {
        dam = 14 + bonus;
    }
    p->heal(healed, dam);
}

// Aspirin
void iuse::pkill_1(game *g, player *p, item *it, bool t)
{
 g->add_msg_if_player(p,"You take some %s.", it->tname().c_str());

 if (!p->has_disease(DI_PKILL1))
  p->add_disease(DI_PKILL1, 120, g);
 else {
  for (int i = 0; i < p->illness.size(); i++) {
   if (p->illness[i].type == DI_PKILL1) {
    p->illness[i].duration = 120;
    i = p->illness.size();
   }
  }
 }
}

// Codeine
void iuse::pkill_2(game *g, player *p, item *it, bool t)
{
 g->add_msg_if_player(p,"You take some %s.", it->tname().c_str());

 p->add_disease(DI_PKILL2, 180, g);
}

void iuse::pkill_3(game *g, player *p, item *it, bool t)
{
 g->add_msg_if_player(p,"You take some %s.", it->tname().c_str());

 p->add_disease(DI_PKILL3, 20, g);
 p->add_disease(DI_PKILL2, 200, g);
}

void iuse::pkill_4(game *g, player *p, item *it, bool t)
{
 g->add_msg_if_player(p,"You shoot up.");

 p->add_disease(DI_PKILL3, 80, g);
 p->add_disease(DI_PKILL2, 200, g);
}

void iuse::pkill_l(game *g, player *p, item *it, bool t)
{
 g->add_msg_if_player(p,"You take some %s.", it->tname().c_str());

 p->add_disease(DI_PKILL_L, rng(12, 18) * 300, g);
}

void iuse::xanax(game *g, player *p, item *it, bool t)
{
 g->add_msg_if_player(p,"You take some %s.", it->tname().c_str());

 if (!p->has_disease(DI_TOOK_XANAX))
  p->add_disease(DI_TOOK_XANAX, 900, g);
 else
  p->add_disease(DI_TOOK_XANAX, 200, g);
}

void iuse::caff(game *g, player *p, item *it, bool t)
{
 it_comest *food = dynamic_cast<it_comest*> (it->type);
 p->fatigue -= food->stim * 3;
}

void iuse::alcohol(game *g, player *p, item *it, bool t)
{
 int duration = 680 - (10 * p->str_max); // Weaker characters are cheap drunks
 if (p->has_trait(PF_LIGHTWEIGHT))
  duration += 300;
 p->pkill += 8;
 p->add_disease(DI_DRUNK, duration, g);
}

void iuse::alcohol_weak(game *g, player *p, item *it, bool t)
{
 int duration = 340 - (6 * p->str_max);
 if (p->has_trait(PF_LIGHTWEIGHT))
  duration += 120;
 p->pkill += 4;
 p->add_disease(DI_DRUNK, duration, g);
}

void iuse::cig(game *g, player *p, item *it, bool t)
{
 if (!use_fire(g, p, it)) return;
 if (it->type->id == "cig")
  g->add_msg_if_player(p,"You light a cigarette and smoke it.");
 else //cigar
  g->add_msg_if_player(p,"You take a few puffs from your cigar.");
 p->add_disease(DI_CIG, 200, g);
 for (int i = 0; i < p->illness.size(); i++) {
  if (p->illness[i].type == DI_CIG && p->illness[i].duration > 600 &&
      !p->is_npc())
   g->add_msg_if_player(p,"Ugh, too much smoke... you feel gross.");
 }
}

void iuse::antibiotic(game *g, player *p, item *it, bool t)
{
if (p->has_disease(DI_INFECTED)){
  g->add_msg_if_player(p,"You took some antibiotics.");
  p->rem_disease(DI_INFECTED);
  p->add_disease(DI_RECOVER, 1200, g);
  }
   else {
 g->add_msg_if_player(p,"You took some antibiotics.");
 }
}

void iuse::weed(game *g, player *p, item *it, bool t)
{
 if (!use_fire(g, p, it)) return;
 g->add_msg_if_player(p,"Good stuff, man!");

 int duration = 60;
 if (p->has_trait(PF_LIGHTWEIGHT))
  duration = 90;
 p->hunger += 8;
 if (p->pkill < 15)
  p->pkill += 5;
 p->add_disease(DI_HIGH, duration, g);
}

void iuse::coke(game *g, player *p, item *it, bool t)
{
 g->add_msg_if_player(p,"You snort a bump.");

 int duration = 21 - p->str_cur;
 if (p->has_trait(PF_LIGHTWEIGHT))
  duration += 20;
 p->hunger -= 8;
 p->add_disease(DI_HIGH, duration, g);
}

void iuse::crack(game *g, player *p, item *it, bool t)
{
  // Crack requires a fire source AND a pipe.
  if (!use_fire(g, p, it)) return;
  g->add_msg_if_player(p,"You smoke some rocks.");
  int duration = 10;
  if (p->has_trait(PF_LIGHTWEIGHT))
  {
    duration += 10;
  }
  p->hunger -= 8;
  p->add_disease(DI_HIGH, duration, g);
}

void iuse::grack(game *g, player *p, item *it, bool t)
{
  // Grack requires a fire source AND a pipe.
  if (!use_fire(g, p, it)) return;
  g->add_msg_if_player(p,"You smoke some Grack Cocaine, time seems to stop.");
  int duration = 1000;
  if (p->has_trait(PF_LIGHTWEIGHT))
    duration += 10;
  p->hunger -= 8;
  p->add_disease(DI_GRACK, duration, g);
}


void iuse::meth(game *g, player *p, item *it, bool t)
{
 int duration = 10 * (40 - p->str_cur);
 if (p->has_amount("apparatus", 1) &&
     p->use_charges_if_avail("fire", 1)) {
  g->add_msg_if_player(p,"You smoke some crystals.");
  duration *= 1.5;
 } else
  g->add_msg_if_player(p,"You snort some crystals.");
 if (!p->has_disease(DI_METH))
  duration += 600;
 int hungerpen = (p->str_cur < 10 ? 20 : 30 - p->str_cur);
 p->hunger -= hungerpen;
 p->add_disease(DI_METH, duration, g);
}

void iuse::poison(game *g, player *p, item *it, bool t)
{
 p->add_disease(DI_POISON, 600, g);
 p->add_disease(DI_FOODPOISON, 1800, g);
}

void iuse::hallu(game *g, player *p, item *it, bool t)
{
 p->add_disease(DI_HALLU, 2400, g);
}

void iuse::thorazine(game *g, player *p, item *it, bool t)
{
 p->fatigue += 15;
 p->rem_disease(DI_HALLU);
 p->rem_disease(DI_VISUALS);
 p->rem_disease(DI_HIGH);
 if (!p->has_disease(DI_DERMATIK))
  p->rem_disease(DI_FORMICATION);
 g->add_msg_if_player(p,"You feel somewhat sedated.");
}

void iuse::prozac(game *g, player *p, item *it, bool t)
{
 if (!p->has_disease(DI_TOOK_PROZAC) && p->morale_level() < 0)
  p->add_disease(DI_TOOK_PROZAC, 7200, g);
 else
  p->stim += 3;
}

void iuse::sleep(game *g, player *p, item *it, bool t)
{
 p->fatigue += 40;
 g->add_msg_if_player(p,"You feel very sleepy...");
}

void iuse::iodine(game *g, player *p, item *it, bool t)
{
 p->add_disease(DI_IODINE, 1200, g);
 g->add_msg_if_player(p,"You take an iodine tablet.");
}

void iuse::flumed(game *g, player *p, item *it, bool t)
{
 p->add_disease(DI_TOOK_FLUMED, 6000, g);
 g->add_msg_if_player(p,"You take some %s", it->tname().c_str());
}

void iuse::flusleep(game *g, player *p, item *it, bool t)
{
 p->add_disease(DI_TOOK_FLUMED, 7200, g);
 p->fatigue += 30;
 g->add_msg_if_player(p,"You feel very sleepy...");
}

void iuse::inhaler(game *g, player *p, item *it, bool t)
{
 p->rem_disease(DI_ASTHMA);
 g->add_msg_if_player(p,"You take a puff from your inhaler.");
}

void iuse::blech(game *g, player *p, item *it, bool t)
{
// TODO: Add more effects?
 g->add_msg_if_player(p,"Blech, that burns your throat!");
 p->vomit(g);
}

void iuse::mutagen(game *g, player *p, item *it, bool t)
{
 if (!one_in(3))
  p->mutate(g);
}

void iuse::mutagen_3(game *g, player *p, item *it, bool t)
{
 p->mutate(g);
 if (!one_in(3))
  p->mutate(g);
 if (one_in(2))
  p->mutate(g);
}

void iuse::purifier(game *g, player *p, item *it, bool t)
{
 std::vector<int> valid;	// Which flags the player has
 for (int i = PF_MAX+1; i < PF_MAX2; i++) {
  if (p->has_trait(pl_flag(i)) && p->has_mutation(pl_flag(i)))
   valid.push_back(i);
 }
 if (valid.size() == 0) {
  g->add_msg_if_player(p,"You feel cleansed.");
  return;
 }
 int num_cured = rng(1, valid.size());
 if (num_cured > 4)
  num_cured = 4;
 for (int i = 0; i < num_cured && valid.size() > 0; i++) {
  int index = rng(0, valid.size() - 1);
  p->remove_mutation(g, pl_flag(valid[index]) );
  valid.erase(valid.begin() + index);
 }
}

void iuse::marloss(game *g, player *p, item *it, bool t)
{
 if (p->is_npc())
  return;
// If we have the marloss in our veins, we are a "breeder" and will spread
// alien lifeforms.
 if (p->has_trait(PF_MARLOSS)) {
  g->add_msg_if_player(p,"As you eat the berry, you have a near-religious experience, feeling at one with your surroundings...");
  p->add_morale(MORALE_MARLOSS, 100, 1000);
  p->hunger = -100;
  monster goo(g->mtypes[mon_blob]);
  goo.friendly = -1;
  int goo_spawned = 0;
  for (int x = p->posx - 4; x <= p->posx + 4; x++) {
   for (int y = p->posy - 4; y <= p->posy + 4; y++) {
    if (rng(0, 10) > trig_dist(x, y, p->posx, p->posy) &&
        rng(0, 10) > trig_dist(x, y, p->posx, p->posy)   )
     g->m.marlossify(x, y);
    if (one_in(10 + 5 * trig_dist(x, y, p->posx, p->posy)) &&
        (goo_spawned == 0 || one_in(goo_spawned * 2))) {
     goo.spawn(x, y);
     g->z.push_back(goo);
     goo_spawned++;
    }
   }
  }
  return;
 }
/* If we're not already carriers of Marloss, roll for a random effect:
 * 1 - Mutate
 * 2 - Mutate
 * 3 - Mutate
 * 4 - Purify
 * 5 - Purify
 * 6 - Cleanse radiation + Purify
 * 7 - Fully satiate
 * 8 - Vomit
 * 9 - Give Marloss mutation
 */
 int effect = rng(1, 9);
 if (effect <= 3) {
  g->add_msg_if_player(p,"This berry tastes extremely strange!");
  p->mutate(g);
 } else if (effect <= 6) { // Radiation cleanse is below
  g->add_msg_if_player(p,"This berry makes you feel better all over.");
  p->pkill += 30;
  this->purifier(g, p, it, t);
 } else if (effect == 7) {
  g->add_msg_if_player(p,"This berry is delicious, and very filling!");
  p->hunger = -100;
 } else if (effect == 8) {
  g->add_msg_if_player(p,"You take one bite, and immediately vomit!");
  p->vomit(g);
 } else if (!p->has_trait(PF_MARLOSS)) {
  g->add_msg_if_player(p,"You feel a strange warmth spreading throughout your body...");
  p->toggle_trait(PF_MARLOSS);
 }
 if (effect == 6)
  p->radiation = 0;
}

void iuse::dogfood(game *g, player *p, item *it, bool t)
{
 int dirx, diry;
 g->draw();
 mvprintw(0, 0, "Which direction?");
 get_direction(g, dirx, diry, input());
 if (dirx == -2) {
  g->add_msg_if_player(p,"Invalid direction.");
  return;
 }
 p->moves -= 15;
 dirx += p->posx;
 diry += p->posy;
 int mon_dex = g->mon_at(dirx,diry);
 if (mon_dex != -1) {
  if (g->z[mon_dex].type->id == mon_dog) {
   g->add_msg_if_player(p,"The dog seems to like you!");
   g->z[mon_dex].friendly = -1;
  } else
   g->add_msg_if_player(p,"The %s seems quit unimpressed!",g->z[mon_dex].type->name.c_str());
 } else
  g->add_msg_if_player(p,"You spill the dogfood all over the ground.");

}



// TOOLS below this point!

bool prep_firestarter_use(game *g, player *p, item *it, int &posx, int &posy)
{
    g->draw();
    mvprintw(0, 0, "Light where?");
    get_direction(g, posx, posy, input());
    if (posx == -2)
    {
        g->add_msg_if_player(p,"Invalid direction.");
        it->charges++;
        return false;
    }
    if (posx == 0 && posy == 0)
    {
        g->add_msg_if_player(p, "You would set yourself on fire.");
        g->add_msg_if_player(p, "But you're already smokin' hot.");
        it->charges++;
        return false;
    }
    posx += p->posx;
    posy += p->posy;
    if (!g->m.flammable_items_at(posx, posy))
    {
       g->add_msg_if_player(p,"There's nothing to light there.");
       it->charges++;
       return false;
    }
    return true;
}

void resolve_firestarter_use(game *g, player *p, item *it, int posx, int posy)
{
    // this should have already been checked, but double-check to make sure
    if (g->m.flammable_items_at(posx, posy))
    {
        if (g->m.add_field(g, posx, posy, fd_fire, 1))
        {
            g->m.field_at(posx, posy).age = 30;
            g->add_msg_if_player(p, "You successfully light a fire.");
        }
    }
    else
    {
        debugmsg("Flammable items disappeared while lighting a fire!");
    }
}

void iuse::lighter(game *g, player *p, item *it, bool t)
{
    int dirx, diry;
    if (prep_firestarter_use(g, p, it, dirx, diry))
    {
        p->moves -= 15;
        resolve_firestarter_use(g, p, it, dirx, diry);
    }
}

void iuse::primitive_fire(game *g, player *p, item *it, bool t)
{
    int posx, posy;
    if (prep_firestarter_use(g, p, it, posx, posy))
    {
        p->moves -= 500;
        const int skillLevel = p->skillLevel("survival");
        const int sides = 10;
        const int base_dice = 3;
        // aiming for ~50% success at skill level 3, and possible but unheard of at level 0
        const int difficulty = (base_dice + 3) * sides / 2;
        if (dice(skillLevel+base_dice, 10) >= difficulty)
        {
            resolve_firestarter_use(g, p, it, posx, posy);
        }
        else
        {
            g->add_msg_if_player(p, "You try to light a fire, but fail.");
        }
        p->practice(g->turn, "survival", 10);
    }
}

void iuse::sew(game *g, player *p, item *it, bool t)
{
    if(p->fine_detail_vision_mod(g) > 2.5)
	{
        g->add_msg("You can't see to sew!");
        it->charges++;
        return;
    }
    char ch = g->inv_type("Repair what?", IC_ARMOR);
    item* fix = &(p->i_at(ch));
    if (fix == NULL || fix->is_null())
	{
        g->add_msg_if_player(p,"You do not have that item!");
        it->charges++;
        return;
    }
    if (!fix->is_armor())
	{
        g->add_msg_if_player(p,"That isn't clothing!");
        it->charges++;
        return;
    }

    itype_id repair_item = "none";
    std::string plural = "";
    if (fix->made_of(COTTON) || fix->made_of(WOOL))
    {
        repair_item = "rag";
        plural = "s";
    }
    else if (fix->made_of(LEATHER))
    {
        repair_item = "leather";
    }
    else
	{
        g->add_msg_if_player(p,"Your %s is not made of cotton, wool or leather.", fix->tname().c_str());
        it->charges++;
        return;
    }

    // this will cause issues if/when NPCs start being able to sew.
    // but, then again, it'll cause issues when they start crafting, too.
    inventory crafting_inv = g->crafting_inventory();
    if (!crafting_inv.has_amount(repair_item, 1))
    {
        g->add_msg_if_player(p,"You don't have enough %s%s to do that.", repair_item.c_str(), plural.c_str());
        it->charges++;
        return;
    }
    if (fix->damage < 0)
	{
        g->add_msg_if_player(p,"Your %s is already enhanced.", fix->tname().c_str());
        it->charges++;
        return;
    }

    std::vector<component> comps;
    comps.push_back(component(repair_item, 1));
    comps.back().available = true;
    g->consume_items(comps);

    if (fix->damage == 0)
    {
        p->moves -= 500 * p->fine_detail_vision_mod(g);
        p->practice(g->turn, "tailor", 10);
        int rn = dice(4, 2 + p->skillLevel("tailor"));
        if (p->dex_cur < 8 && one_in(p->dex_cur))
            {rn -= rng(2, 6);}
        if (p->dex_cur >= 16 || (p->dex_cur > 8 && one_in(16 - p->dex_cur)))
            {rn += rng(2, 6);}
        if (p->dex_cur > 16)
            {rn += rng(0, p->dex_cur - 16);}
        if (rn <= 4)
	    {
            g->add_msg_if_player(p,"You damage your %s!", fix->tname().c_str());
            fix->damage++;
        } 
        else if (rn >= 12 && p->i_at(ch).has_flag("VARSIZE") && !p->i_at(ch).has_flag("FIT"))
	    {
            g->add_msg_if_player(p,"You take your %s in, improving the fit.", fix->tname().c_str());
            (p->i_at(ch).item_tags.insert("FIT"));
        }
        else if (rn >= 12 && (p->i_at(ch).has_flag("FIT") || !p->i_at(ch).has_flag("VARSIZE")))
	    {
            g->add_msg_if_player(p, "You make your %s extra sturdy.", fix->tname().c_str());
            fix->damage--;
        }
        else
		{
            g->add_msg_if_player(p,"You practice your sewing.");
		}
    }
    else
	{
        p->moves -= 500 * p->fine_detail_vision_mod(g);
        p->practice(g->turn, "tailor", 8);
        int rn = dice(4, 2 + p->skillLevel("tailor"));
        rn -= rng(fix->damage, fix->damage * 2);
        if (p->dex_cur < 8 && one_in(p->dex_cur))
            {rn -= rng(2, 6);}
        if (p->dex_cur >= 8 && (p->dex_cur >= 16 || one_in(16 - p->dex_cur)))
            {rn += rng(2, 6);}
        if (p->dex_cur > 16)
            {rn += rng(0, p->dex_cur - 16);}
        if (rn <= 4)
	    {
            g->add_msg_if_player(p,"You damage your %s further!", fix->tname().c_str());
            fix->damage++;
            if (fix->damage >= 5)
		    {
                g->add_msg_if_player(p,"You destroy it!");
                p->i_rem(ch);
            }
        }
	    else if (rn <= 6)
	    {
            g->add_msg_if_player(p,"You don't repair your %s, but you waste lots of thread.", fix->tname().c_str());
            int waste = rng(1, 8);
            if (waste > it->charges)
                {it->charges = 1;}
            else
                {it->charges -= waste;}
        }
        else if (rn <= 8)
	    {
            g->add_msg_if_player(p,"You repair your %s, but waste lots of thread.", fix->tname().c_str());
            fix->damage--;
            int waste = rng(1, 8);
        if (waste > it->charges)
            {it->charges = 1;}
        else
            {it->charges -= waste;}
        }
	    else if (rn <= 16)
	    {
            g->add_msg_if_player(p,"You repair your %s!", fix->tname().c_str());
            fix->damage--;
        }
	    else
	    {
            g->add_msg_if_player(p,"You repair your %s completely!", fix->tname().c_str());
            fix->damage = 0;
        }
    }
    
    //iuse::sew uses up 1 charge when called, if less than 1, set to 1, and use that one up.
    if (it->charges < 1)
        {it->charges = 1;}
}

void iuse::extra_battery(game *g, player *p, item *it, bool t)
{
    char ch = g->inv_type("Modify what?", IC_TOOL);
    item* modded = &(p->i_at(ch));

    if (modded == NULL || modded->is_null())
    {
        g->add_msg_if_player(p,"You do not have that item!");
        return;
    }
    if (!modded->is_tool())
    {
        g->add_msg_if_player(p,"You can only mod tools with this battery mod.");
        return;
    }

    it_tool *tool = dynamic_cast<it_tool*>(modded->type);
    if (tool->ammo != AT_BATT)
    {
        g->add_msg_if_player(p,"That item does not use batteries!");
        return;
    }

    if (modded->has_flag("DOUBLE_AMMO"))
    {
        g->add_msg_if_player(p,"That item has already had its battery capacity doubled.");
        return;
    }

    modded->item_tags.insert("DOUBLE_AMMO");
    g->add_msg_if_player(p,"You double the battery capacity of your %s!", tool->name.c_str());
    it->invlet = 0;
}

void iuse::scissors(game *g, player *p, item *it, bool t)
{
    char ch = g->inv("Chop up what?");
    item* cut = &(p->i_at(ch));
    if (cut->type->id == "null")
    {
        g->add_msg_if_player(p,"You do not have that item!");
        return;
    }
    if (cut->type->id == "string_6" || cut->type->id == "string_36" || cut->type->id == "rope_30" || cut->type->id == "rope_6")
    {
        g->add_msg("You cannot cut that, you must disassemble it using the disassemble key");
        return;
    }
    if (cut->type->id == "rag" || cut->type->id == "rag_bloody" || cut->type->id == "leather")
    {
        g->add_msg_if_player(p, "There's no point in cutting a %s.", cut->type->name.c_str());
        return;
    }
    if (!cut->made_of(COTTON) && !cut->made_of(LEATHER))
    {
        g->add_msg("You can only slice items made of cotton or leather.");
        return;
    }

    //scrap_text is the description of worthless scraps, type is the item type,
    //pre_text is the bit before the plural on a success, post_text is the bit after the plural
    std::string scrap_text, pre_text, post_text, type;
    if (cut->made_of(COTTON))
    {
        scrap_text = "ribbons";
        pre_text = "rag";
        type = "rag";
    }
    else
    {
        scrap_text = "scraps";
        pre_text = "piece";
        post_text = " of leather";
        type = "leather";
    }

    p->moves -= 25 * cut->volume();
    int count = cut->volume();
    if (p->skillLevel("tailor") == 0)
    {
        count = rng(0, count);
    }
    else if (p->skillLevel("tailor") == 1 && count >= 2)
    {
        count -= rng(0, 2);
    }

    if (dice(3, 3) > p->dex_cur)
    {
        count -= rng(1, 3);
    }

    if (count <= 0)
    {
        g->add_msg_if_player(p,"You clumsily cut the %s into useless %s.",
                             cut->tname().c_str(), scrap_text.c_str());
        p->i_rem(ch);
        return;
    }
    g->add_msg_if_player(p,"You slice the %s into %d %s%s%s.", cut->tname().c_str(), count, pre_text.c_str(),
                         (count == 1 ? "" : "s"), post_text.c_str());
    item result(g->itypes[type], int(g->turn), g->nextinv);
    p->i_rem(ch);
    bool drop = false;
    for (int i = 0; i < count; i++)
    {
        int iter = 0;
        while (p->has_item(result.invlet) && iter < inv_chars.size())
        {
            result.invlet = g->nextinv;
            g->advance_nextinv();
            iter++;
        }
        if (!drop && (iter == inv_chars.size() || p->volume_carried() >= p->volume_capacity()))
            drop = true;
        if (drop)
            g->m.add_item(p->posx, p->posy, result);
        else
            p->i_add(result, g);
    }
    return;
}

void iuse::extinguisher(game *g, player *p, item *it, bool t)
{
 g->draw();
 mvprintz(0, 0, c_red, "Pick a direction to spray:");
 int dirx, diry;
 get_direction(g, dirx, diry, input());
 if (dirx == -2) {
  g->add_msg_if_player(p,"Invalid direction!");
  it->charges++;
  return;
 }
 p->moves -= 140;
 int x = dirx + p->posx;
 int y = diry + p->posy;
 if (g->m.field_at(x, y).type == fd_fire) {
  g->m.field_at(x, y).density -= rng(2, 3);
  if (g->m.field_at(x, y).density <= 0) {
   g->m.field_at(x, y).density = 1;
   g->m.remove_field(x, y);
  }
 }
 int mondex = g->mon_at(x, y);
 if (mondex != -1) {
  g->z[mondex].moves -= 150;
  if (g->u_see(&(g->z[mondex])))
   g->add_msg_if_player(p,"The %s is sprayed!", g->z[mondex].name().c_str());
  if (g->z[mondex].made_of(LIQUID)) {
   if (g->u_see(&(g->z[mondex])))
    g->add_msg_if_player(p,"The %s is frozen!", g->z[mondex].name().c_str());
   if (g->z[mondex].hurt(rng(20, 60)))
    g->kill_mon(mondex, (p == &(g->u)));
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
    g->m.remove_field(x, y);
   }
  }
 }
}

void iuse::hammer(game *g, player *p, item *it, bool t)
{
    g->draw();
    mvprintz(0, 0, c_red, "Pick a direction in which to pry:");
    int dirx, diry;
    get_direction(g, dirx, diry, input());
    if (dirx == -2)
    {
        g->add_msg_if_player(p,"Invalid direction!");
        return;
    }
    if (dirx == 0 && diry == 0)
    {
        g->add_msg_if_player(p, "You try to hit yourself with the hammer.");
        g->add_msg_if_player(p, "But you can't touch this.");
        return;
    }
    dirx += p->posx;
    diry += p->posy;
    int nails = 0, boards = 0;
    ter_id newter;
    switch (g->m.ter(dirx, diry))
    {
        case t_fence_h:
        case t_fence_v:
        nails = 6;
        boards = 3;
        newter = t_fence_post;
        break;

        case t_window_boarded:
        nails =  8;
        boards = 3;
        newter = t_window_empty;
        break;

        case t_door_boarded:
        nails = 12;
        boards = 3;
        newter = t_door_b;
        break;

        default:
        g->add_msg_if_player(p,"Hammers can only remove boards from windows, doors and fences.");
        g->add_msg_if_player(p,"To board up a window or door, press *");
        return;
    }
    p->moves -= 500;
    g->m.spawn_item(p->posx, p->posy, g->itypes["nail"], 0, 0, nails);
    g->m.spawn_item(p->posx, p->posy, g->itypes["2x4"], 0, boards);
    g->m.ter_set(dirx, diry, newter);
}

void iuse::gasoline_lantern_off(game *g, player *p, item *it, bool t)
{
    if (it->charges == 0)
    {
        g->add_msg_if_player(p,"The lantern is empty.");
    }
    else if(!p->use_charges_if_avail("fire", 1))
    {
        g->add_msg_if_player(p,"You need a lighter!");
    }
    else
    {
        g->add_msg_if_player(p,"You turn the lantern on.");
        it->make(g->itypes["gasoline_lantern_on"]);
        it->active = true;
        it->charges --;
    }
}

void iuse::gasoline_lantern_on(game *g, player *p, item *it, bool t)
{
    if (t)  	// Normal use
    {
// Do nothing... player::active_light and the lightmap::generate deal with this
    }
    else  	// Turning it off
    {
        g->add_msg_if_player(p,"The lantern is extinguished.");
        it->make(g->itypes["gasoline_lantern"]);
        it->active = false;
    }
}

void iuse::light_off(game *g, player *p, item *it, bool t)
{
 if (it->charges == 0)
  g->add_msg_if_player(p,"The flashlight's batteries are dead.");
 else {
  g->add_msg_if_player(p,"You turn the flashlight on.");
  it->make(g->itypes["flashlight_on"]);
  it->active = true;
  it->charges --;
 }
}

void iuse::light_on(game *g, player *p, item *it, bool t)
{
 if (t) {	// Normal use
// Do nothing... player::active_light and the lightmap::generate deal with this
 } else {	// Turning it off
  g->add_msg_if_player(p,"The flashlight flicks off.");
  it->make(g->itypes["flashlight"]);
  it->active = false;
 }
}

// this function only exists because we need to set it->active = true
// otherwise crafting would just give you the active version directly
void iuse::lightstrip(game *g, player *p, item *it, bool t)
{
    g->add_msg_if_player(p,"You irreversibly activate the lightstrip.");
    it->make(g->itypes["lightstrip"]);
    it->active = true;
}

void iuse::lightstrip_active(game *g, player *p, item *it, bool t)
{
    if (t)
    {	// Normal use
        // Do nothing... player::active_light and the lightmap::generate deal with this
    }
    else
    {	// Turning it off
        g->add_msg_if_player(p,"The lightstrip dies.");
        it->make(g->itypes["lightstrip_dead"]);
        it->active = false;
    }
}

void iuse::glowstick(game *g, player *p, item *it, bool t)
{
    g->add_msg_if_player(p,"You activate the glowstick.");
    it->make(g->itypes["glowstick_lit"]);
    it->active = true;
}

void iuse::glowstick_active(game *g, player *p, item *it, bool t)
{
    if (t)
    {	// Normal use
        // Do nothing... player::active_light and the lightmap::generate deal with this
    }
    else
    {
        if (it->charges > 0)
        {
            g->add_msg_if_player(p,"You can't turn off a glowstick.");
        }
        else
        {
            g->add_msg_if_player(p,"The glowstick fades out.");
            it->active = false;
        }
    }
}
void iuse::cauterize_elec(game *g, player *p, item *it, bool t)
{
    if (it->charges == 0)
    g->add_msg_if_player(p,"You need batteries to cauterize wounds.");

    else if (!p->has_disease(DI_BITE) && !p->has_disease(DI_BLEED))
    g->add_msg_if_player(p,"You are not bleeding or bitten, there is no need to cauterize yourself.");

    else if (p->is_npc() || query_yn("Cauterize any open wounds?"))
    {
        it->charges -= 1;
        p->cauterize(g);
    }
}


void iuse::water_purifier(game *g, player *p, item *it, bool t)
{
  it->charges++;
 char ch = g->inv_type("Purify what?", IC_COMESTIBLE);
 if (!p->has_item(ch)) {
  g->add_msg_if_player(p,"You do not have that item!");
  return;
 }
 if (p->i_at(ch).contents.size() == 0) {
  g->add_msg_if_player(p,"You can only purify water.");
  return;
 }
 item *pure = &(p->i_at(ch).contents[0]);
 if (pure->type->id != "water" && pure->type->id != "salt_water") {
  g->add_msg_if_player(p,"You can only purify water.");
  return;
 }
 if (pure->charges > it->charges)
 {
  g->add_msg_if_player(p,"You don't have enough battery power to purify all the water.");
  return;
 }
 it->charges -= pure->charges;
 p->moves -= 150;
 pure->make(g->itypes["water_clean"]);
 pure->poison = 0;
}

void iuse::two_way_radio(game *g, player *p, item *it, bool t)
{
 WINDOW* w = newwin(6, 36, (TERMY-6)/2, (TERMX-36)/2);
 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
// TODO: More options here.  Thoughts...
//       > Respond to the SOS of an NPC
//       > Report something to a faction
//       > Call another player
 mvwprintz(w, 1, 1, c_white, "1: Radio a faction for help...");
 mvwprintz(w, 2, 1, c_white, "2: Call Acquaintance...");
 mvwprintz(w, 3, 1, c_white, "3: General S.O.S.");
 mvwprintz(w, 4, 1, c_white, "0: Cancel");
 wrefresh(w);
 char ch = getch();
 if (ch == '1') {
  p->moves -= 300;
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
   g->add_event(EVENT_HELP, int(g->turn) + fac->response_time(g), fac->id, -1, -1);
   fac->respects_u -= rng(0, 8);
   fac->likes_u -= rng(3, 5);
  } else if (bonus >= -5) {
   popup("They reply, \"Sorry, you're on your own!\"");
   fac->respects_u -= rng(0, 5);
  } else {
   popup("They reply, \"Hah!  We hope you die!\"");
   fac->respects_u -= rng(1, 8);
  }

 } else if (ch == '2') {	// Call Acquaitance
// TODO: Implement me!
 } else if (ch == '3') {	// General S.O.S.
  p->moves -= 150;
  std::vector<npc*> in_range;
  for (int i = 0; i < g->cur_om->npcs.size(); i++) {
   if (g->cur_om->npcs[i]->op_of_u.value >= 4 &&
       rl_dist(g->levx, g->levy, g->cur_om->npcs[i]->mapx,
                                   g->cur_om->npcs[i]->mapy) <= 30)
    in_range.push_back((g->cur_om->npcs[i]));
  }
  if (in_range.size() > 0) {
   npc* coming = in_range[rng(0, in_range.size() - 1)];
   popup("A reply!  %s says, \"I'm on my way; give me %d minutes!\"",
         coming->name.c_str(), coming->minutes_to_u(g));
   coming->mission = NPC_MISSION_RESCUE_U;
  } else
   popup("No-one seems to reply...");
 } else
  it->charges++;	// Canceled the call, get our charge back
 werase(w);
 wrefresh(w);
 delwin(w);
 refresh();
}

void iuse::radio_off(game *g, player *p, item *it, bool t)
{
 if (it->charges == 0)
  g->add_msg_if_player(p,"It's dead.");
 else {
  g->add_msg_if_player(p,"You turn the radio on.");
  it->make(g->itypes["radio_on"]);
  it->active = true;
 }
}

static radio_tower *find_radio_station( game *g, int frequency )
{
    radio_tower *tower = NULL;
    for (int k = 0; k < g->cur_om->radios.size(); k++)
    {
        tower = &g->cur_om->radios[k];
        if( 0 < tower->strength - rl_dist(tower->x, tower->y, g->levx, g->levy) &&
            tower->frequency == frequency )
        {
            return tower;
        }
    }
    return NULL;
}

void iuse::directional_antenna(game *g, player *p, item *it, bool t)
{
    // Find out if we have an active radio
    item radio = p->i_of_type("radio_on");
    if( radio.typeId() != "radio_on" )
    {
        g->add_msg( "Must have an active radio to check for signal sirection." );
        return;
    }
    // Find the radio station its tuned to (if any)
    radio_tower *tower = find_radio_station( g, radio.frequency );
    if( tower == NULL )
    {
        g->add_msg( "You can't find the direction if your radio isn't tuned." );
        return;
    }
    // Report direction.
    direction angle = direction_from( g->levx, g->levy, tower->x, tower->y );
    g->add_msg( ("The signal seems strongest to the " + direction_name(angle) + ".").c_str() );

}

void iuse::radio_on(game *g, player *p, item *it, bool t)
{
    if (t)
    {	// Normal use
        std::string message = "Radio: Kssssssssssssh.";
        radio_tower *selected_tower = find_radio_station( g, it->frequency );
        if( selected_tower != NULL )
        {
            if( selected_tower->type == MESSAGE_BROADCAST )
            {
                message = selected_tower->message;
            }
            else if (selected_tower->type == WEATHER_RADIO)
            {
                message = weather_forecast(g, *selected_tower);
            }

            int signal_strength = selected_tower->strength -
                rl_dist(selected_tower->x, selected_tower->y, g->levx, g->levy);

            for (int j = 0; j < message.length(); j++)
            {
                if (dice(10, 100) > dice(10, signal_strength * 3))
                {
                    if (!one_in(10))
                    {
                        message[j] = '#';
                    }
                    else
                    {
                        message[j] = char(rng('a', 'z'));
                    }
                }
            }

            std::vector<std::string> segments;
            while (message.length() > RADIO_PER_TURN)
            {
                int spot = message.find_last_of(' ', RADIO_PER_TURN);
                if (spot == std::string::npos)
                {
                    spot = RADIO_PER_TURN;
                }
                segments.push_back( message.substr(0, spot) );
                message = message.substr(spot + 1);
            }
            segments.push_back(message);
            int index = g->turn % (segments.size());
            std::stringstream messtream;
            messtream << "radio: " << segments[index];
            message = messtream.str();
        }
        point pos = g->find_item(it);
        g->sound(pos.x, pos.y, 6, message.c_str());
    } else {	// Activated
        int ch = menu( true, "Radio:", "Scan", "Turn off", NULL );
        switch (ch)
        {
        case 1:
        {
            int old_frequency = it->frequency;
            radio_tower *tower = NULL;
            radio_tower *lowest_tower = NULL;
            radio_tower *lowest_larger_tower = NULL;

            for (int k = 0; k < g->cur_om->radios.size(); k++)
            {
                tower = &g->cur_om->radios[k];

                if( 0 < tower->strength - rl_dist(tower->x, tower->y, g->levx, g->levy) &&
                    tower->frequency != old_frequency )
                {
                    if( tower->frequency > old_frequency &&
                        (lowest_larger_tower == NULL ||
                         tower->frequency < lowest_larger_tower->frequency) )
                    {
                        lowest_larger_tower = tower;
                    }
                    else if( lowest_tower == NULL ||
                             tower->frequency < lowest_tower->frequency )
                    {
                        lowest_tower = tower;
                    }
                }
            }
            if( lowest_larger_tower != NULL )
            {
                it->frequency = lowest_larger_tower->frequency;
            }
            else if( lowest_tower != NULL )
            {
                it->frequency = lowest_tower->frequency;
            }
        }
        break;
        case 2:
            g->add_msg_if_player(p,"The radio dies.");
            it->make(g->itypes["radio"]);
            it->active = false;
            break;
        case 3: break;
        }
    }
}

void iuse::noise_emitter_off(game *g, player *p, item *it, bool t)
{
    if (it->charges == 0)
    {
        g->add_msg_if_player(p,"It's dead.");
    }
    else
    {
        g->add_msg_if_player(p,"You turn the noise emitter on.");
        it->make(g->itypes["noise_emitter_on"]);
        it->active = true;
    }
}

void iuse::noise_emitter_on(game *g, player *p, item *it, bool t)
{
    if (t) // Normal use
    {
        point pos = g->find_item(it);
        g->sound(pos.x, pos.y, 30, "KXSHHHHRRCRKLKKK!");
    }
    else // Turning it off
    {
        g->add_msg_if_player(p,"The infernal racket dies as you turn off the noise emitter.");
        it->make(g->itypes["noise_emitter"]);
        it->active = false;
    }
}


void iuse::roadmap(game *g, player *p, item *it, bool t)
{
 if (it->charges < 1) {
  g->add_msg_if_player(p, "There isn't anything new on the map.");
  return;
 }
  // Show roads
 roadmap_targets(g, p, it, t, (int)ot_hiway_ns, 2, 0, 0);
 roadmap_targets(g, p, it, t, (int)ot_road_ns, 12, 0, 0);
 roadmap_targets(g, p, it, t, (int)ot_bridge_ns, 2, 0, 0);

  // Show evac shelters
 roadmap_targets(g, p, it, t, (int)ot_shelter, 1, 0, 0);
  // Show hospital(s)
 roadmap_targets(g, p, it, t, (int)ot_hospital_entrance, 2, 0, 0);
  // Show megastores
 roadmap_targets(g, p, it, t, (int)ot_megastore_entrance, 2, 0, 0);
  // Show police stations
 roadmap_targets(g, p, it, t, (int)ot_police_north, 4, 0, 0);
  // Show pharmacies
 roadmap_targets(g, p, it, t, (int)ot_s_pharm_north, 4, 0, 0);

 g->add_msg_if_player(p, "You add roads and points of interest to your map.");

 it->charges = 0;
}

void iuse::roadmap_a_target(game *g, player *p, item *it, bool t, int target)
{
 int dist = 0;
 oter_t oter_target = oterlist[target];
 point place = g->cur_om->find_closest(g->om_location(), (oter_id)target, 1, dist,
                                      false);

 int pomx = (g->levx + int(MAPSIZE / 2)) / 2; //overmap loc
 int pomy = (g->levy + int(MAPSIZE / 2)) / 2; //overmap loc

 if (g->debugmon) debugmsg("Map: %s at %d,%d found! You @ %d %d",oter_target.name.c_str(), place.x, place.y, pomx,pomy);

 if (place.x >= 0 && place.y >= 0) {
  for (int x = place.x - 3; x <= place.x + 3; x++) {
   for (int y = place.y - 3; y <= place.y + 3; y++)
    g->cur_om->seen(x, y, g->levz) = true;
  }

  direction to_hospital = direction_from(pomx,pomy, place.x, place.y);
  int distance = trig_dist(pomx,pomy, place.x, place.y);

  g->add_msg_if_player(p, "You add a %s location to your map.", oterlist[target].name.c_str());
  g->add_msg_if_player(p, "It's %d squares to the %s", distance,  direction_name(to_hospital).c_str());
 } else {
  g->add_msg_if_player(p, "You can't find a hospital near your location.");
 }
}

void iuse::roadmap_targets(game *g, player *p, item *it, bool t, int target, int target_range, int distance, int reveal_distance)
{
 oter_t oter_target = oterlist[target];
 point place;
 point origin = g->om_location();
 std::vector<point> places = g->cur_om->find_all(tripoint(origin.x, origin.y, g->levz),
                                                (oter_id)target, target_range, distance, false);

 for (std::vector<point>::iterator iter = places.begin(); iter != places.end(); ++iter) {
  place = *iter;
  if (place.x >= 0 && place.y >= 0) {
  if (reveal_distance == 0) {
    g->cur_om->seen(place.x,place.y,g->levz) = true;
  } else {
   for (int x = place.x - reveal_distance; x <= place.x + reveal_distance; x++) {
    for (int y = place.y - reveal_distance; y <= place.y + reveal_distance; y++)
      g->cur_om->seen(x, y,g->levz) = true;
   }
  }
  }
 }
}

void iuse::picklock(game *g, player *p, item *it, bool t)
{
 int dirx, diry;
 g->draw();
 mvprintw(0, 0, "Pick which lock?");
 get_direction(g, dirx, diry, input());
 if (dirx == -2) {
  g->add_msg_if_player(p,"Invalid direction.");
  return;
 } else if (dirx == 0 && diry == 0) {
  g->add_msg_if_player(p, "You pick your nose and your sinuses swing open.");
  return;
 }
 dirx += p->posx;
 diry += p->posy;
 ter_id type = g->m.ter(dirx, diry);
 int npcdex = g->npc_at(dirx, diry);
 if (npcdex != -1) {
  g->add_msg_if_player(p, "You can pick your friends, and you can");
  g->add_msg_if_player(p, "pick your nose, but you can't pick");
  g->add_msg_if_player(p, "your friend's nose");
  return;
 }

 const char *door_name;
 ter_id new_type;
 if (type == t_chaingate_l) {
   door_name = "gate";
   new_type = t_chaingate_c;
 } else if (type == t_door_locked || type == t_door_locked_alarm || type == t_door_locked_interior) {
   door_name = "door";
   new_type = t_door_c;
 } else {
  g->add_msg("That cannot be picked.");
  return;
 }

 p->practice(g->turn, "mechanics", 1);
 p->moves -= 500 - (p->dex_cur + p->skillLevel("mechanics")) * 5;
 if (dice(4, 6) < dice(2, p->skillLevel("mechanics")) + dice(2, p->dex_cur) - it->damage / 2) {
  p->practice(g->turn, "mechanics", 1);
  g->add_msg_if_player(p,"With a satisfying click, the lock on the %s opens.", door_name);
  g->m.ter_set(dirx, diry, new_type);
 } else if (dice(4, 4) < dice(2, p->skillLevel("mechanics")) +
                         dice(2, p->dex_cur) - it->damage / 2 && it->damage < 100) {
  it->damage++;

  std::string sStatus = "damage";
  if (it->damage >= 5) {
   sStatus = "destroy";
   it->invlet = 0; // no copy to inventory in player.cpp:4472 ->
  }

  g->add_msg_if_player(p,("The lock stumps your efforts to pick it, and you " + sStatus + " your tool.").c_str());
 } else {
  g->add_msg_if_player(p,"The lock stumps your efforts to pick it.");
 }
 if ( type == t_door_locked_alarm &&
      dice(4, 7) <  dice(2, p->skillLevel("mechanics")) +
      dice(2, p->dex_cur) - it->damage / 2 && it->damage < 100) {
  g->sound(p->posx, p->posy, 30, "An alarm sounds!");
  if (!g->event_queued(EVENT_WANTED)) {
   g->add_event(EVENT_WANTED, int(g->turn) + 300, 0, g->levx, g->levy);
  }
 }
}
void iuse::crowbar(game *g, player *p, item *it, bool t)
{
 int dirx, diry;
 g->draw();
 mvprintw(0, 0, "Pry where?");
 get_direction(g, dirx, diry, input());
 if (dirx == -2) {
  g->add_msg_if_player(p,"Invalid direction.");
  return;
 }
if (dirx == 0 && diry == 0) {
   g->add_msg_if_player(p, "You attempt to pry open your wallet");
   g->add_msg_if_player(p, "but alas. You are just too miserly.");
   return;
 }
 dirx += p->posx;
 diry += p->posy;
 ter_id type = g->m.ter(dirx, diry);
 const char *door_name;
 const char *action_name;
 ter_id new_type;
 bool noisy;
 int difficulty;

 if (type == t_door_c || type == t_door_locked || type == t_door_locked_alarm || type == t_door_locked_interior) {
   door_name = "door";
   action_name = "pry open";
   new_type = t_door_o;
   noisy = true;
   difficulty = 6;
 } else if (type == t_manhole_cover) {
   door_name = "manhole cover";
   action_name = "lift";
   new_type = t_manhole;
   noisy = false;
   difficulty = 12;
 } else if (g->m.ter(dirx, diry) == t_crate_c) {
   door_name = "crate";
   action_name = "pop open";
   new_type = t_crate_o;
   noisy = true;
   difficulty = 6;
 } else if (type == t_window_domestic || type == t_curtains) {
   door_name = "window";
   action_name = "pry open";
   new_type = t_window_open;
   noisy = true;
   difficulty = 6;
 } else {
  int nails = 0, boards = 0;
  ter_id newter;
  switch (g->m.ter(dirx, diry)) {
  case t_window_boarded:
   nails =  8;
   boards = 3;
   newter = t_window_empty;
   break;
  case t_door_boarded:
   nails = 12;
   boards = 3;
   newter = t_door_b;
   break;
  case t_fence_h:
   nails = 6;
   boards = 3;
   newter = t_fence_post;
   break;
  case t_fence_v:
   nails = 6;
   boards = 3;
   newter = t_fence_post;
   break;
  default:
   g->add_msg_if_player(p,"There's nothing to pry there.");
   return;
  }
  if(p->skillLevel("carpentry") < 1)
   p->practice(g->turn, "carpentry", 1);
  p->moves -= 500;
  g->m.spawn_item(p->posx, p->posy, g->itypes["nail"], 0, 0, nails);
  g->m.spawn_item(p->posx, p->posy, g->itypes["2x4"], 0, boards);
  g->m.ter_set(dirx, diry, newter);
  return;
 }

 p->practice(g->turn, "mechanics", 1);
 p->moves -= (difficulty * 25) - ((p->str_cur + p->skillLevel("mechanics")) * 5);
 if (dice(4, difficulty) < dice(2, p->skillLevel("mechanics")) + dice(2, p->str_cur)) {
  p->practice(g->turn, "mechanics", 1);
  g->add_msg_if_player(p,"You %s the %s.", action_name, door_name);
  g->m.ter_set(dirx, diry, new_type);
  if (noisy)
   g->sound(dirx, diry, 8, "crunch!");
  if ( type == t_door_locked_alarm ) {
   g->sound(p->posx, p->posy, 30, "An alarm sounds!");
   if (!g->event_queued(EVENT_WANTED)) {
    g->add_event(EVENT_WANTED, int(g->turn) + 300, 0, g->levx, g->levy);
   }
  }
 } else {
  if (type == t_window_domestic || type == t_curtains) {
   //chance of breaking the glass if pry attempt fails
   if (dice(4, difficulty) > dice(2, p->skillLevel("mechanics")) + dice(2, p->str_cur)) {
    g->add_msg_if_player(p,"You break the glass.");
    g->sound(dirx, diry, 16, "glass breaking!");
    g->m.ter_set(dirx, diry, t_window_frame);
    return;
   }
  }
  g->add_msg_if_player(p,"You pry, but cannot %s the %s.", action_name, door_name);
 }
}

void iuse::makemound(game *g, player *p, item *it, bool t)
{
 if (g->m.has_flag(diggable, p->posx, p->posy)) {
  g->add_msg_if_player(p,"You churn up the earth here.");
  p->moves = -300;
  g->m.ter_set(p->posx, p->posy, t_dirtmound);
 } else
  g->add_msg_if_player(p,"You can't churn up this ground.");
}

void iuse::dig(game *g, player *p, item *it, bool t)
{
 g->add_msg_if_player(p,"You can dig a pit via the construction menu--hit *");
/*
 int dirx, diry;
 g->draw();
 mvprintw(0, 0, "Dig where?");
 get_direction(g, dirx, diry, input());
 if (dirx == -2) {
  g->add_msg_if_player(p,"Invalid direction.");
  return;
 }
 if (g->m.has_flag(diggable, p->posx + dirx, p->posy + diry)) {
  p->moves -= 300;
  g->add_msg_if_player(p,"You dig a pit.");
  g->m.ter     (p->posx + dirx, p->posy + diry) = t_pit;
  g->m.add_trap(p->posx + dirx, p->posy + diry, tr_pit);
  p->practice(g->turn, "traps", 1);
 } else
  g->add_msg_if_player(p,"You can't dig through %s!",
             g->m.tername(p->posx + dirx, p->posy + diry).c_str());
*/
}

void iuse::siphon(game *g, player *p, item *it, bool t)
{
    int posx = 0;
    int posy = 0;
    g->draw();
    mvprintw(0, 0, "Siphon where?");
    get_direction(g, posx, posy, input());
    if (posx == -2)
    {
        g->add_msg_if_player(p,"Invalid direction.");
        return;
    }
    // there's no self-tile check because the player could be in a vehicle
    posx += p->posx;
    posy += p->posy;
    
    vehicle* veh = g->m.veh_at(posx, posy);
    if (veh == NULL)
    {
        g->add_msg_if_player(p,"There's no vehicle there.");
        return;
    }
    if (veh->fuel_left(AT_GAS) == 0)
    {
        g->add_msg_if_player(p, "That vehicle has no fuel to siphon.");
        return;
    }
    p->siphon_gas(g, veh);
    p->moves -= 200;
    return;
}

void iuse::chainsaw_off(game *g, player *p, item *it, bool t)
{
 p->moves -= 80;
 if (rng(0, 10) - it->damage > 5 && it->charges > 0) {
  g->sound(p->posx, p->posy, 20,
           "With a roar, the chainsaw leaps to life!");
  it->make(g->itypes["chainsaw_on"]);
  it->active = true;
 } else
  g->add_msg_if_player(p,"You yank the cord, but nothing happens.");
}

void iuse::chainsaw_on(game *g, player *p, item *it, bool t)
{
 if (t) {	// Effects while simply on
  if (one_in(15))
   g->sound(p->posx, p->posy, 12, "Your chainsaw rumbles.");
 } else {	// Toggling
  g->add_msg_if_player(p,"Your chainsaw dies.");
  it->make(g->itypes["chainsaw_off"]);
  it->active = false;
 }
}

void iuse::jackhammer(game *g, player *p, item *it, bool t)
{
 int dirx, diry;
 g->draw();
 mvprintw(0, 0, "Drill in which direction?");
 get_direction(g, dirx, diry, input());
 if (dirx == -2) {
  g->add_msg_if_player(p,"Invalid direction.");
  return;
 }
 if (dirx == 0 && diry == 0) {
  g->add_msg_if_player(p,"My god! Let's talk it over OK?");
  g->add_msg_if_player(p,"Don't do anything rash..");
  return;
 }
 dirx += p->posx;
 diry += p->posy;
 if (g->m.is_destructable(dirx, diry) && g->m.has_flag(supports_roof, dirx, diry) &&
     g->m.ter(dirx, diry) != t_tree) {
  g->m.destroy(g, dirx, diry, false);
  p->moves -= 500;
  g->sound(dirx, diry, 45, "TATATATATATATAT!");
 } else if (g->m.move_cost(dirx, diry) == 2 && g->levz != -1 &&
            g->m.ter(dirx, diry) != t_dirt && g->m.ter(dirx, diry) != t_grass) {
  g->m.destroy(g, dirx, diry, false);
  p->moves -= 500;
  g->sound(dirx, diry, 45, "TATATATATATATAT!");
 } else {
  g->add_msg_if_player(p,"You can't drill there.");
  it->charges += (dynamic_cast<it_tool*>(it->type))->charges_per_use;
 }
}

void iuse::jacqueshammer(game *g, player *p, item *it, bool t)
{
 int dirx, diry;
 g->draw();
 mvprintw(0, 0, "Percer dans quelle direction?");
 get_direction(g, dirx, diry, input());
 if (dirx == -2) {
  g->add_msg_if_player(p,"Direction invalide");
  return;
 }
 if (dirx == 0 && diry == 0) {
  g->add_msg_if_player(p,"Mon dieu! Nous allons en parler OK?");
  g->add_msg_if_player(p,"Ne pas faire eruption rien..");
  return;
 }
 dirx += p->posx;
 diry += p->posy;
 if (g->m.is_destructable(dirx, diry) && g->m.has_flag(supports_roof, dirx, diry) &&
     g->m.ter(dirx, diry) != t_tree) {
  g->m.destroy(g, dirx, diry, false);
  p->moves -= 500;
  g->sound(dirx, diry, 45, "OHOHOHOHOHOHOHOHO!");
 } else if (g->m.move_cost(dirx, diry) == 2 && g->levz != -1 &&
            g->m.ter(dirx, diry) != t_dirt && g->m.ter(dirx, diry) != t_grass) {
  g->m.destroy(g, dirx, diry, false);
  p->moves -= 500;
  g->sound(dirx, diry, 45, "OHOHOHOHOHOHOHOHO!");
 } else {
  g->add_msg_if_player(p,"Vous ne pouvez pas percer la-bas..");
  it->charges += (dynamic_cast<it_tool*>(it->type))->charges_per_use;
 }
}

void iuse::pickaxe(game *g, player *p, item *it, bool t)
{
/* int dirx, diry;
 g->draw();
 mvprintw(0, 0, "Drill in which direction?");
 get_direction(g, dirx, diry, input());
 if (dirx == -2) {
  g->add_msg_if_player(p,"Invalid direction.");
  return;
 }
 dirx += p->posx;
 diry += p->posy;
 if (g->m.is_destructable(dirx, diry) && g->m.has_flag(supports_roof, dirx, diry) &&
     g->m.ter(dirx, diry) != t_tree) {
  g->m.destroy(g, dirx, diry, false);
  p->moves -= 500;
  g->sound(dirx, diry, 12, "CHNK! CHNK! CHNK!");
 } else if (g->m.move_cost(dirx, diry) == 2 && g->levz != -1 &&
            g->m.ter(dirx, diry) != t_dirt && g->m.ter(dirx, diry) != t_grass) {
  g->m.destroy(g, dirx, diry, false);
  p->moves -= 500;
  g->sound(dirx, diry, 12, CHNK! CHNK! CHNK!");
 } else {
  g->add_msg_if_player(p,"You can't mine there.");
*/
  g->add_msg_if_player(p,"Whoa buddy! You can't go cheating in items and");
  g->add_msg_if_player(p,"just expect them to work! Now put the pickaxe");
  g->add_msg_if_player(p,"down and go play the game.");
}
void iuse::set_trap(game *g, player *p, item *it, bool t)
{
 int dirx, diry;
 g->draw();
 mvprintw(0, 0, "Place where?");
 get_direction(g, dirx, diry, input());
 if (dirx == -2) {
  g->add_msg_if_player(p,"Invalid direction.");
  return;
 }
 if (dirx == 0 && diry == 0) {
  g->add_msg_if_player(p,"Yeah. Place the %s at your feet.", it->tname().c_str());
  g->add_msg_if_player(p,"Real damn smart move.");
  return;
 }
 int posx = dirx + p->posx;
 int posy = diry + p->posy;
 if (g->m.move_cost(posx, posy) != 2) {
  g->add_msg_if_player(p,"You can't place a %s there.", it->tname().c_str());
  return;
 }
  if (g->m.tr_at(posx, posy) != tr_null) {
  g->add_msg_if_player(p, "You can't place a %s there. It contains a trap already.", it->tname().c_str());
  return;
 }



 trap_id type = tr_null;
 ter_id ter;
 bool buried = false;
 bool set = false;
 std::stringstream message;
 int practice;

if(it->type->id == "cot"){
  message << "You unfold the cot and place it on the ground.";
  type = tr_cot;
  practice = 0;
 } else if(it->type->id == "rollmat"){
  message << "You unroll the mat and lay it on the ground.";
  type = tr_rollmat;
  practice = 0;
 } else if(it->type->id == "brazier"){
  message << "You place the brazier securely.";
  type = tr_brazier;
  practice = 0;
 } else if(it->type->id == "boobytrap"){
  message << "You set the boobytrap up and activate the grenade.";
  type = tr_boobytrap;
  practice = 4;
 } else if(it->type->id == "bubblewrap"){
  message << "You set the bubblewrap on the ground, ready to be popped.";
  type = tr_bubblewrap;
  practice = 2;
 } else if(it->type->id == "beartrap"){
  buried = (p->has_amount("shovel", 1) &&
            g->m.has_flag(diggable, posx, posy) &&
            query_yn("Bury the beartrap?"));
  type = (buried ? tr_beartrap_buried : tr_beartrap);
  message << "You " << (buried ? "bury" : "set") << " the beartrap.";
  practice = (buried ? 7 : 4);
 } else if(it->type->id == "board_trap"){
  message << "You set the board trap on the " << g->m.tername(posx, posy) <<
             ", nails facing up.";
  type = tr_nailboard;
  practice = 2;
  } else if(it->type->id == "funnel"){
  message << "You place the funnel, waiting to collect rain.";
  type = tr_funnel;
  practice = 0;
 } else if(it->type->id == "tripwire"){
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
   g->add_msg_if_player(p,"You must place the tripwire between two solid tiles.");
   return;
  }
 } else if(it->type->id == "crossbow_trap"){
  message << "You set the crossbow trap.";
  type = tr_crossbow;
  practice = 4;
 } else if(it->type->id == "shotgun_trap"){
  message << "You set the shotgun trap.";
  type = tr_shotgun_2;
  practice = 5;
 } else if(it->type->id == "blade_trap"){
  posx += dirx;
  posy += diry;
  for (int i = -1; i <= 1; i++) {
   for (int j = -1; j <= 1; j++) {
    if (g->m.move_cost(posx + i, posy + j) != 2) {
     g->add_msg_if_player(p,"\
That trap needs a 3x3 space to be clear, centered two tiles from you.");
     return;
    }
   }
  }
  message << "You set the blade trap two squares away.";
  type = tr_engine;
  practice = 12;
 } else if(it->type->id == "light_snare_kit"){
  for(int i = -1; i <= 1; i++) {
    for(int j = -1; j <= 1; j++){
      ter = g->m.ter(posx+j, posy+i);
      if(ter == t_tree_young && !set) {
        message << "You set the snare trap.";
        type = tr_light_snare;
        practice = 2;
        set = true;
      }
    }
  }
  if(!set) {
    g->add_msg_if_player(p, "Invalid Placement.");
    return;
  }
 } else if(it->type->id == "heavy_snare_kit"){
  for(int i = -1; i <= 1; i++) {
    for(int j = -1; j <= 1; j++){
      ter = g->m.ter(posx+j, posy+i);
      if(ter == t_tree && !set) {
        message << "You set the snare trap.";
        type = tr_heavy_snare;
        practice = 4;
        set = true;
      }
    }
  }
  if(!set) {
    g->add_msg_if_player(p, "Invalid Placement.");
    return;
  }
 } else if(it->type->id == "landmine"){
  buried = (p->has_amount("shovel", 1) &&
            g->m.has_flag(diggable, posx, posy) &&
            query_yn("Bury the landmine?"));
  type = (buried ? tr_landmine_buried : tr_landmine);
  message << "You " << (buried ? "bury" : "set") << " the landmine.";
  practice = (buried ? 7 : 4);
 } else {
  g->add_msg_if_player(p,"Tried to set a trap.  But got confused! %s", it->tname().c_str());
 }

 if (buried) {
  if (!p->has_amount("shovel", 1)) {
   g->add_msg_if_player(p,"You need a shovel.");
   return;
  } else if (!g->m.has_flag(diggable, posx, posy)) {
   g->add_msg_if_player(p,"You can't dig in that %s", g->m.tername(posx, posy).c_str());
   return;
  }
 }

 g->add_msg_if_player(p,message.str().c_str());
 p->practice(g->turn, "traps", practice);
 g->m.add_trap(posx, posy, type);
 p->moves -= 100 + practice * 25;
 if (type == tr_engine) {
  for (int i = -1; i <= 1; i++) {
   for (int j = -1; j <= 1; j++) {
    if (i != 0 || j != 0)
     g->m.add_trap(posx + i, posy + j, tr_blade);
   }
  }
 }
 it->invlet = 0; // Remove the trap from the player's inv
}

void iuse::geiger(game *g, player *p, item *it, bool t)
{
 if (t) { // Every-turn use when it's on
  int rads = g->m.radiation(p->posx, p->posy);
  if (rads == 0)
   return;
  g->sound(p->posx, p->posy, 6, "");
  if (rads > 50)
   g->add_msg("The geiger counter buzzes intensely.");
  else if (rads > 35)
   g->add_msg("The geiger counter clicks wildly.");
  else if (rads > 25)
   g->add_msg("The geiger counter clicks rapidly.");
  else if (rads > 15)
   g->add_msg("The geiger counter clicks steadily.");
  else if (rads > 8)
   g->add_msg("The geiger counter clicks slowly.");
  else if (rads > 4)
   g->add_msg("The geiger counter clicks intermittantly.");
  else
   g->add_msg("The geiger counter clicks once.");
  return;
 }
// Otherwise, we're activating the geiger counter
 it_tool *type = dynamic_cast<it_tool*>(it->type);
 bool is_on = (type->id == "geiger_on");
 if (is_on) {
  g->add_msg("The geiger counter's SCANNING LED flicks off.");
  it->make(g->itypes["geiger_off"]);
  it->active = false;
  return;
 }
 std::string toggle_text = "Turn continuous scan ";
 toggle_text += (is_on ? "off" : "on");
 int ch = menu(true, "Geiger counter:", "Scan yourself", "Scan the ground",
               toggle_text.c_str(), "Cancel", NULL);
 switch (ch) {
  case 1: g->add_msg_if_player(p,"Your radiation level: %d", p->radiation); break;
  case 2: g->add_msg_if_player(p,"The ground's radiation level: %d",
                     g->m.radiation(p->posx, p->posy));		break;
  case 3:
   g->add_msg_if_player(p,"The geiger counter's scan LED flicks on.");
   it->make(g->itypes["geiger_on"]);
   it->active = true;
   break;
  case 4:
   it->charges++;
   break;
 }
}

void iuse::teleport(game *g, player *p, item *it, bool t)
{
 p->moves -= 100;
 g->teleport(p);
}

void iuse::can_goo(game *g, player *p, item *it, bool t)
{
 it->make(g->itypes["canister_empty"]);
 int tries = 0, goox, gooy;
 do {
  goox = p->posx + rng(-2, 2);
  gooy = p->posy + rng(-2, 2);
  tries++;
 } while (g->m.move_cost(goox, gooy) == 0 && tries < 10);
 if (tries == 10)
  return;
 int mondex = g->mon_at(goox, gooy);
 if (mondex != -1) {
  if (g->u_see(goox, gooy))
   g->add_msg("Black goo emerges from the canister and envelopes a %s!",
              g->z[mondex].name().c_str());
  g->z[mondex].poly(g->mtypes[mon_blob]);
  g->z[mondex].speed -= rng(5, 25);
  g->z[mondex].hp = g->z[mondex].speed;
 } else {
  if (g->u_see(goox, gooy))
   g->add_msg("Living black goo emerges from the canister!");
  monster goo(g->mtypes[mon_blob]);
  goo.friendly = -1;
  goo.spawn(goox, gooy);
  g->z.push_back(goo);
 }
 tries = 0;
 while (!one_in(4) && tries < 10) {
  tries = 0;
  do {
   goox = p->posx + rng(-2, 2);
   gooy = p->posy + rng(-2, 2);
   tries++;
  } while (g->m.move_cost(goox, gooy) == 0 &&
           g->m.tr_at(goox, gooy) == tr_null && tries < 10);
  if (tries < 10) {
   if (g->u_see(goox, gooy))
    g->add_msg("A nearby splatter of goo forms into a goo pit.");
   g->m.tr_at(goox, gooy) = tr_goo;
  }
 }
}


void iuse::pipebomb(game *g, player *p, item *it, bool t)
{
 if (!p->use_charges_if_avail("fire", 1)) {
  g->add_msg_if_player(p,"You need a lighter!");
  return;
 }
 g->add_msg_if_player(p,"You light the fuse on the pipe bomb.");
 it->make(g->itypes["pipebomb_act"]);
 it->charges = 3;
 it->active = true;
}

void iuse::pipebomb_act(game *g, player *p, item *it, bool t)
{
 point pos = g->find_item(it);
 if (pos.x == -999 || pos.y == -999)
  return;
 if (t) // Simple timer effects
  g->sound(pos.x, pos.y, 0, "Ssssss");	// Vol 0 = only heard if you hold it
 else {	// The timer has run down
  if (one_in(10) && g->u_see(pos.x, pos.y))
   g->add_msg("The pipe bomb fizzles out.");
  else
   g->explosion(pos.x, pos.y, rng(6, 14), rng(0, 4), false);
 }
}

void iuse::grenade(game *g, player *p, item *it, bool t)
{
 g->add_msg_if_player(p,"You pull the pin on the grenade.");
 it->make(g->itypes["grenade_act"]);
 it->charges = 5;
 it->active = true;
}

void iuse::grenade_act(game *g, player *p, item *it, bool t)
{
 point pos = g->find_item(it);
 if (pos.x == -999 || pos.y == -999)
  return;
 if (t) // Simple timer effects
  g->sound(pos.x, pos.y, 0, "Tick.");	// Vol 0 = only heard if you hold it
 else	// When that timer runs down...
  g->explosion(pos.x, pos.y, 12, 28, false);
}

void iuse::flashbang(game *g, player *p, item *it, bool t)
{
 g->add_msg_if_player(p,"You pull the pin on the flashbang.");
 it->make(g->itypes["flashbang_act"]);
 it->charges = 5;
 it->active = true;
}

void iuse::flashbang_act(game *g, player *p, item *it, bool t)
{
 point pos = g->find_item(it);
 if (pos.x == -999 || pos.y == -999)
  return;
 if (t) // Simple timer effects
  g->sound(pos.x, pos.y, 0, "Tick.");	// Vol 0 = only heard if you hold it
 else	// When that timer runs down...
  g->flashbang(pos.x, pos.y);
}

void iuse::c4(game *g, player *p, item *it, bool t)
{
 int time = query_int("Set the timer to (0 to cancel)?");
 if (time == 0) {
  g->add_msg_if_player(p,"Never mind.");
  return;
 }
 g->add_msg_if_player(p,"You set the timer to %d.", time);
 it->make(g->itypes["c4armed"]);
 it->charges = time;
 it->active = true;
}

void iuse::c4armed(game *g, player *p, item *it, bool t)
{
 point pos = g->find_item(it);
 if (pos.x == -999 || pos.y == -999)
  return;
 if (t) // Simple timer effects
  g->sound(pos.x, pos.y, 0, "Tick.");	// Vol 0 = only heard if you hold it
 else	// When that timer runs down...
  g->explosion(pos.x, pos.y, 40, 3, false);
}

void iuse::EMPbomb(game *g, player *p, item *it, bool t)
{
 g->add_msg_if_player(p,"You pull the pin on the EMP grenade.");
 it->make(g->itypes["EMPbomb_act"]);
 it->charges = 3;
 it->active = true;
}

void iuse::EMPbomb_act(game *g, player *p, item *it, bool t)
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

void iuse::scrambler(game *g, player *p, item *it, bool t)
{
 g->add_msg_if_player(p,"You pull the pin on the scrambler grenade.");
 it->make(g->itypes["scrambler_act"]);
 it->charges = 3;
 it->active = true;
}

void iuse::scrambler_act(game *g, player *p, item *it, bool t)
{
 point pos = g->find_item(it);
 if (pos.x == -999 || pos.y == -999)
  return;
 if (t)	// Simple timer effects
  g->sound(pos.x, pos.y, 0, "Tick.");	// Vol 0 = only heard if you hold it
 else {	// When that timer runs down...
  for (int x = pos.x - 4; x <= pos.x + 4; x++) {
   for (int y = pos.y - 4; y <= pos.y + 4; y++)
    g->scrambler_blast(x, y);
  }
 }
}

void iuse::gasbomb(game *g, player *p, item *it, bool t)
{
 g->add_msg_if_player(p,"You pull the pin on the teargas canister.");
 it->make(g->itypes["gasbomb_act"]);
 it->charges = 20;
 it->active = true;
}

void iuse::gasbomb_act(game *g, player *p, item *it, bool t)
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
  it->make(g->itypes["canister_empty"]);
}

void iuse::smokebomb(game *g, player *p, item *it, bool t)
{
 g->add_msg_if_player(p,"You pull the pin on the smoke bomb.");
 it->make(g->itypes["smokebomb_act"]);
 it->charges = 20;
 it->active = true;
}

void iuse::smokebomb_act(game *g, player *p, item *it, bool t)
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
  it->make(g->itypes["canister_empty"]);
}

void iuse::acidbomb(game *g, player *p, item *it, bool t)
{
 g->add_msg_if_player(p,"You remove the divider, and the chemicals mix.");
 p->moves -= 150;
 it->make(g->itypes["acidbomb_act"]);
 it->charges = 1;
 it->bday = int(g->turn);
 it->active = true;
}

void iuse::acidbomb_act(game *g, player *p, item *it, bool t)
{
 if (!p->has_item(it)) {
  point pos = g->find_item(it);
  if (pos.x == -999)
   pos = point(p->posx, p->posy);
  it->charges = 0;
  for (int x = pos.x - 1; x <= pos.x + 1; x++) {
   for (int y = pos.y - 1; y <= pos.y + 1; y++)
    g->m.add_field(g, x, y, fd_acid, 3);
  }
 }
}

void iuse::molotov(game *g, player *p, item *it, bool t)
{
 if (!p->use_charges_if_avail("fire", 1))
 {
  g->add_msg_if_player(p,"You need a lighter!");
  return;
 }
 g->add_msg_if_player(p,"You light the molotov cocktail.");
 p->moves -= 150;
 it->make(g->itypes["molotov_lit"]);
 it->charges = 1;
 it->bday = int(g->turn);
 it->active = true;
}

void iuse::molotov_lit(game *g, player *p, item *it, bool t)
{
 int age = int(g->turn) - it->bday;
 if (!p->has_item(it)) {
  point pos = g->find_item(it);
  it->charges = -1;
  g->explosion(pos.x, pos.y, 8, 0, true);
 } else if (age >= 5) { // More than 5 turns old = chance of going out
  if (rng(1, 50) < age) {
   g->add_msg_if_player(p,"Your lit molotov goes out.");
   it->make(g->itypes["molotov"]);
   it->charges = 0;
   it->active = false;
  }
 }
}

void iuse::dynamite(game *g, player *p, item *it, bool t)
{
 if (!p->use_charges_if_avail("fire", 1))
 {
  it->charges++;
  g->add_msg_if_player(p,"You need a lighter!");
  return;
 }
 g->add_msg_if_player(p,"You light the dynamite.");
 it->make(g->itypes["dynamite_act"]);
 it->charges = 20;
 it->active = true;
}

void iuse::dynamite_act(game *g, player *p, item *it, bool t)
{
 point pos = g->find_item(it);
 if (pos.x == -999 || pos.y == -999)
  return;
 if (t) // Simple timer effects
  g->sound(pos.x, pos.y, 0, "ssss...");
 else	// When that timer runs down...
  g->explosion(pos.x, pos.y, 60, 0, false);
}

void iuse::firecracker_pack(game *g, player *p, item *it, bool t)
{
 if (!p->use_charges_if_avail("fire", 1))
 {
  g->add_msg_if_player(p,"You need a lighter!");
  return;
 }
 WINDOW* w = newwin(5, 41, (TERMY-5)/2, (TERMX-41)/2);
 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
              LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 int mid_x = getmaxx(w) / 2;
 mvwprintz(w, 1, 2, c_white,  "How many do you want to light? (1-%d)", it->charges);
 mvwprintz(w, 2, mid_x, c_white, "1");
 mvwprintz(w, 3, 5, c_ltred, "I");
 mvwprintz(w, 3, 6, c_white, "ncrease");
 mvwprintz(w, 3, 14, c_ltred, "D");
 mvwprintz(w, 3, 15, c_white, "ecrease");
 mvwprintz(w, 3, 23, c_ltred, "A");
 mvwprintz(w, 3, 24, c_white, "ccept");
 mvwprintz(w, 3, 30, c_ltred, "C");
 mvwprintz(w, 3, 31, c_white, "ancel");
 wrefresh(w);
 bool close = false;
 int charges = 1;
 char ch = getch();
 while(!close) {
  if(ch == 'I') {
   charges++;
   if(charges > it->charges) {
    charges = it->charges;
   }
   mvwprintz(w, 2, mid_x, c_white, "%d", charges);
   wrefresh(w);
  } else if(ch == 'D') {
   charges--;
   if(charges < 1) {
    charges = 1;
   }
   mvwprintz(w, 2, mid_x, c_white, "%d ", charges); //Trailing space clears the second digit when decreasing from 10 to 9
   wrefresh(w);
  } else if(ch == 'A') {
   if(charges == it->charges) {
    g->add_msg_if_player(p,"You light the pack of firecrackers.");
    it->make(g->itypes["firecracker_pack_act"]);
    it->charges = charges;
    it->bday = g->turn;
    it->active = true;
   } else {
    if(charges == 1) {
     g->add_msg_if_player(p,"You light one firecracker.");
     item new_it = item(g->itypes["firecracker_act"], int(g->turn));
     new_it.charges = 2;
     new_it.active = true;
     p->i_add(new_it, g);
     it->charges -= 1;
    } else {
     g->add_msg_if_player(p,"You light a string of %d firecrackers.", charges);
     item new_it = item(g->itypes["firecracker_pack_act"], int(g->turn));
     new_it.charges = charges;
     new_it.bday = g->turn;
     new_it.active = true;
     p->i_add(new_it, g);
     it->charges -= charges;
    }
    if(it->charges == 1) {
     it->make(g->itypes["firecracker"]);
    }
   }
   close = true;
  } else if(ch == 'C') {
   close = true;
  }
  if(!close) {
   ch = getch();
  }
 }
}

void iuse::firecracker_pack_act(game *g, player *p, item *it, bool t)
{
 point pos = g->find_item(it);
 int current_turn = g->turn;
 int timer = current_turn - it->bday;
 if(timer < 2) {
  g->sound(pos.x, pos.y, 0, "ssss...");
  it->damage += 1;
 } else if(it->charges > 0) {
  int ex = rng(3,5);
  int i = 0;
  if(ex > it->charges) {
    ex = it->charges;
  }
  for(i = 0; i < ex; i++) {
   g->sound(pos.x, pos.y, 20, "Bang!");
  }
  it->charges -= ex;
 }
}

void iuse::firecracker(game *g, player *p, item *it, bool t)
{
 if (!p->use_charges_if_avail("fire", 1))
 {
  g->add_msg_if_player(p,"You need a lighter!");
  return;
 }
 g->add_msg_if_player(p,"You light the firecracker.");
 it->make(g->itypes["firecracker_act"]);
 it->charges = 2;
 it->active = true;
}

void iuse::firecracker_act(game *g, player *p, item *it, bool t)
{
 point pos = g->find_item(it);
 if (pos.x == -999 || pos.y == -999)
  return;
 if (t) {// Simple timer effects
  g->sound(pos.x, pos.y, 0, "ssss...");
 } else {  // When that timer runs down...
  g->sound(pos.x, pos.y, 20, "Bang!");
 }
}

void iuse::mininuke(game *g, player *p, item *it, bool t)
{
 g->add_msg_if_player(p,"You activate the mininuke.");
 it->make(g->itypes["mininuke_act"]);
 it->charges = 10;
 it->active = true;
}

void iuse::mininuke_act(game *g, player *p, item *it, bool t)
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

void iuse::pheromone(game *g, player *p, item *it, bool t)
{
 point pos(p->posx, p->posy);

 bool is_u = !p->is_npc(), can_see = (is_u || g->u_see(p->posx, p->posy));
 if (pos.x == -999 || pos.y == -999)
  return;

 if (is_u)
  g->add_msg("You squeeze the pheromone ball...");
 else if (can_see)
  g->add_msg("%s squeezes a pheromone ball...", p->name.c_str());
 p->moves -= 15;

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

 if (can_see) {
  if (converts == 0)
   g->add_msg("...but nothing happens.");
  else if (converts == 1)
   g->add_msg("...and a nearby zombie turns friendly!");
  else
   g->add_msg("...and several nearby zombies turn friendly!");
 }
}


void iuse::portal(game *g, player *p, item *it, bool t)
{
 g->m.add_trap(p->posx + rng(-2, 2), p->posy + rng(-2, 2), tr_portal);
}

void iuse::manhack(game *g, player *p, item *it, bool t)
{
 std::vector<point> valid;	// Valid spawn locations
 for (int x = p->posx - 1; x <= p->posx + 1; x++) {
  for (int y = p->posy - 1; y <= p->posy + 1; y++) {
   if (g->is_empty(x, y))
    valid.push_back(point(x, y));
  }
 }
 if (valid.size() == 0) {	// No valid points!
  g->add_msg_if_player(p,"There is no adjacent square to release the manhack in!");
  return;
 }
 int index = rng(0, valid.size() - 1);
 p->moves -= 60;
 it->invlet = 0; // Remove the manhack from the player's inv
 monster m_manhack(g->mtypes[mon_manhack], valid[index].x, valid[index].y);
 if (rng(0, p->int_cur / 2) + p->skillLevel("electronics") / 2 +
     p->skillLevel("computer") < rng(0, 4))
  g->add_msg_if_player(p,"You misprogram the manhack; it's hostile!");
 else
  m_manhack.friendly = -1;
 g->z.push_back(m_manhack);
}

void iuse::turret(game *g, player *p, item *it, bool t)
{
 int dirx, diry;
 g->draw();
 mvprintw(0, 0, "Place where?");
 get_direction(g, dirx, diry, input());
 if (dirx == -2) {
  g->add_msg_if_player(p,"Invalid direction.");
  return;
 }
 p->moves -= 100;
 dirx += p->posx;
 diry += p->posy;
 if (!g->is_empty(dirx, diry)) {
  g->add_msg_if_player(p,"You cannot place a turret there.");
  return;
 }
 it->invlet = 0; // Remove the turret from the player's inv
 monster mturret(g->mtypes[mon_turret], dirx, diry);
 if (rng(0, p->int_cur / 2) + p->skillLevel("electronics") / 2 +
     p->skillLevel("computer") < rng(0, 6))
  g->add_msg_if_player(p,"You misprogram the turret; it's hostile!");
 else
  mturret.friendly = -1;
 g->z.push_back(mturret);
}

void iuse::UPS_off(game *g, player *p, item *it, bool t)
{
 if (it->charges == 0)
  g->add_msg_if_player(p,"The power supply's batteries are dead.");
 else {
  g->add_msg_if_player(p,"You turn the power supply on.");
  if (p->is_wearing("goggles_nv"))
   g->add_msg_if_player(p,"Your light amp goggles power on.");
  if (p->worn.size() && p->worn[0].type->is_power_armor())
    g->add_msg_if_player(p, "Your power armor engages.");
  it->make(g->itypes["UPS_on"]);
  it->active = true;
 }
}

void iuse::UPS_on(game *g, player *p, item *it, bool t)
{
 if (t) {	// Normal use
   if (p->worn.size() && p->worn[0].type->is_power_armor()) {
     it->charges -= 4;

     if (it->charges < 0) {
       it->charges = 0;
     }
   }
 } else {	// Turning it off
  g->add_msg_if_player(p,"The UPS powers off with a soft hum.");
  if (p->worn.size() && p->worn[0].type->is_power_armor())
    g->add_msg_if_player(p, "Your power armor disengages.");
  it->make(g->itypes["UPS_off"]);
  it->active = false;
 }
}

void iuse::tazer(game *g, player *p, item *it, bool t)
{
 int dirx, diry;
 g->draw();
 mvprintw(0, 0, "Shock in which direction?");
 get_direction(g, dirx, diry, input());
 if (dirx == -2) {
  g->add_msg_if_player(p,"Invalid direction.");
  it->charges += (dynamic_cast<it_tool*>(it->type))->charges_per_use;
  return;
 }
 else if (dirx == 0 && diry == 0) {
  g->add_msg_if_player(p,"Umm. No.");
  it->charges += (dynamic_cast<it_tool*>(it->type))->charges_per_use;
  return;
 }
 int sx = dirx + p->posx, sy = diry + p->posy;
 int mondex = g->mon_at(sx, sy);
 int npcdex = g->npc_at(sx, sy);
 if (mondex == -1 && npcdex == -1) {
  g->add_msg_if_player(p,"Your tazer crackles in the air.");
  return;
 }

 int numdice = 3 + (p->dex_cur / 2.5) + p->skillLevel("melee") * 2;
 p->moves -= 100;

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
   g->add_msg_if_player(p,"You attempt to shock the %s, but miss.", z->name().c_str());
   return;
  }
  g->add_msg_if_player(p,"You shock the %s!", z->name().c_str());
  int shock = rng(5, 25);
  z->moves -= shock * 100;
  if (z->hurt(shock))
   g->kill_mon(mondex, (p == &(g->u)));
  return;
 }

 if (npcdex != -1) {
  npc *foe = g->active_npc[npcdex];
  if (foe->attitude != NPCATT_FLEE)
   foe->attitude = NPCATT_KILL;
  if (foe->str_max >= 17)
    numdice++;	// Minor bonus against huge people
  else if (foe->str_max <= 5)
   numdice--;	// Minor penalty against tiny people
  if (dice(numdice, 10) <= dice(foe->dodge(g), 6)) {
   g->add_msg_if_player(p,"You attempt to shock %s, but miss.", foe->name.c_str());
   return;
  }
  g->add_msg_if_player(p,"You shock %s!", foe->name.c_str());
  int shock = rng(5, 20);
  foe->moves -= shock * 100;
  foe->hurtall(shock);
  if (foe->hp_cur[hp_head]  <= 0 || foe->hp_cur[hp_torso] <= 0) {
   foe->die(g, true);
   g->active_npc.erase(g->active_npc.begin() + npcdex);
  }
 }

}

void iuse::mp3(game *g, player *p, item *it, bool t)
{
 if (it->charges == 0)
  g->add_msg_if_player(p,"The mp3 player's batteries are dead.");
 else if (p->has_active_item("mp3_on"))
  g->add_msg_if_player(p,"You are already listening to an mp3 player!");
 else {
  g->add_msg_if_player(p,"You put in the earbuds and start listening to music.");
  it->make(g->itypes["mp3_on"]);
  it->active = true;
 }
}

void iuse::mp3_on(game *g, player *p, item *it, bool t)
{
 if (t) {	// Normal use
  if (!p->has_item(it))
   return;	// We're not carrying it!
  p->add_morale(MORALE_MUSIC, 1, 50);

  if (int(g->turn) % 10 == 0) {	// Every 10 turns, describe the music
   std::string sound = "";
   if (one_in(50))
     sound = "some bass-heavy post-glam speed polka";
   switch (rng(1, 10)) {
    case 1: sound = "a sweet guitar solo!";	p->stim++;	break;
    case 2: sound = "a funky bassline.";			break;
    case 3: sound = "some amazing vocals.";			break;
    case 4: sound = "some pumping bass.";			break;
    case 5: sound = "dramatic classical music.";
            if (p->int_cur >= 10)
             p->add_morale(MORALE_MUSIC, 1, 100);		break;
   }
   if (sound.length() > 0)
    g->add_msg_if_player(p,"You listen to %s", sound.c_str());
  }
 } else {	// Turning it off
  g->add_msg_if_player(p,"The mp3 player turns off.");
  it->make(g->itypes["mp3"]);
  it->active = false;
 }
}

void iuse::vortex(game *g, player *p, item *it, bool t)
{
 std::vector<point> spawn;
 for (int i = -3; i <= 3; i++) {
  if (g->is_empty(p->posx - 3, p->posy + i))
   spawn.push_back( point(p->posx - 3, p->posy + i) );
  if (g->is_empty(p->posx + 3, p->posy + i))
   spawn.push_back( point(p->posx + 3, p->posy + i) );
  if (g->is_empty(p->posx + i, p->posy - 3))
   spawn.push_back( point(p->posx + i, p->posy - 3) );
  if (g->is_empty(p->posx + i, p->posy + 3))
   spawn.push_back( point(p->posx + i, p->posy + 3) );
 }
 if (spawn.empty()) {
  g->add_msg_if_player(p,"Air swirls around you for a moment.");
  it->make(g->itypes["spiral_stone"]);
  return;
 }

 g->add_msg_if_player(p,"Air swirls all over...");
 int index = rng(0, spawn.size() - 1);
 p->moves -= 100;
 it->make(g->itypes["spiral_stone"]);
 monster mvortex(g->mtypes[mon_vortex], spawn[index].x, spawn[index].y);
 mvortex.friendly = -1;
 g->z.push_back(mvortex);
}

void iuse::dog_whistle(game *g, player *p, item *it, bool t)
{
 g->add_msg_if_player(p,"You blow your dog whistle.");
 for (int i = 0; i < g->z.size(); i++) {
  if (g->z[i].friendly != 0 && g->z[i].type->id == mon_dog) {
   bool u_see = g->u_see(&(g->z[i]));
   if (g->z[i].has_effect(ME_DOCILE)) {
    if (u_see)
     g->add_msg_if_player(p,"Your %s looks ready to attack.", g->z[i].name().c_str());
    g->z[i].rem_effect(ME_DOCILE);
   } else {
    if (u_see)
     g->add_msg_if_player(p,"Your %s goes docile.", g->z[i].name().c_str());
    g->z[i].add_effect(ME_DOCILE, -1);
   }
  }
 }
}

void iuse::vacutainer(game *g, player *p, item *it, bool t)
{
 if (p->is_npc())
  return; // No NPCs for now!

 if (!it->contents.empty()) {
  g->add_msg_if_player(p,"That %s is full!", it->tname().c_str());
  return;
 }

 item blood(g->itypes["blood"], g->turn);
 bool drew_blood = false;
 for (int i = 0; i < g->m.i_at(p->posx, p->posy).size() && !drew_blood; i++) {
  item *map_it = &(g->m.i_at(p->posx, p->posy)[i]);
  if (map_it->type->id == "corpse" &&
      query_yn("Draw blood from %s?", map_it->tname().c_str())) {
   blood.corpse = map_it->corpse;
   drew_blood = true;
  }
 }

 if (!drew_blood && query_yn("Draw your own blood?"))
  drew_blood = true;

 if (!drew_blood)
  return;

 it->put_in(blood);
}

void iuse::knife(game *g, player *p, item *it, bool t)
{
    int choice = menu(true,
    "Using knife:", "Cut up fabric", "Carve wood", "Cauterize", "Cancel", NULL);
    switch (choice)
    {
        if (choice == 4)
        break;
        case 1:
        {
            iuse::scissors(g, p, it, t);
        }
        break;
        case 2:
        {
            char ch = g->inv("Chop up what?");
            item* cut = &(p->i_at(ch));
            if (cut->type->id == "null")
            {
                g->add_msg("You do not have that item!");
                return;
            }
            if (cut->type->id == "stick" || cut->type->id == "2x4")
            {
                g->add_msg("You carve several skewers from the %s.", cut->tname().c_str());
                int count = 12;
                item skewer(g->itypes["skewer"], int(g->turn), g->nextinv);
                p->i_rem(ch);
                bool drop = false;
                for (int i = 0; i < count; i++)
                {
                    int iter = 0;
                    while (p->has_item(skewer.invlet) && iter < inv_chars.size())
                    {
                        skewer.invlet = g->nextinv;
                        g->advance_nextinv();
                        iter++;
                    }
                    if (!drop && (iter == inv_chars.size() || p->volume_carried() >= p->volume_capacity()))
                    drop = true;
                    if (drop)
                    g->m.add_item(p->posx, p->posy, skewer);
                    else
                    p->i_add(skewer);
                }
            }
            else
            {
                g->add_msg("You can't carve that up!");
            }
        }
        break;
        case 3:
        {
            if (!p->has_disease(DI_BITE) && !p->has_disease(DI_BLEED))
                g->add_msg_if_player(p,"You are not bleeding or bitten, there is no need to cauterize yourself.");
            else if (!p->use_charges_if_avail("fire", 4))
                g->add_msg_if_player(p,"You need a lighter with 4 charges before you can cauterize yourself.");
            else
                p->cauterize(g);
            break;
        }
    }
}

void iuse::lumber(game *g, player *p, item *it, bool t)
{
 char ch = g->inv("Cut up what?");
 item* cut = &(p->i_at(ch));
 if (cut->type->id == "null") {
  g->add_msg("You do not have that item!");
  return;
 }
 if (cut->type->id == "log") {
  p->moves -= 300;
  g->add_msg("You cut the log into planks.");
  item plank(g->itypes["2x4"], int(g->turn), g->nextinv);
  item scrap(g->itypes["splinter"], int(g->turn), g->nextinv);
  p->i_rem(ch);
  bool drop = false;
  int planks = (rng(1, 3) + (p->skillLevel("carpentry") * 2));
  int scraps = 12 - planks;
   if (planks >= 12)
    planks = 12;
  if (scraps >= planks)
   g->add_msg("You waste a lot of the wood.");
  for (int i = 0; i < planks; i++) {
   int iter = 0;
   while (p->has_item(plank.invlet)) {
    plank.invlet = g->nextinv;
    g->advance_nextinv();
    iter++;
   }
   if (!drop && (iter == inv_chars.size() || p->volume_carried() >= p->volume_capacity()))
    drop = true;
   if (drop)
    g->m.add_item(p->posx, p->posy, plank);
   else
    p->i_add(plank);
  }
 for (int i = 0; i < scraps; i++) {
   int iter = 0;
   while (p->has_item(scrap.invlet)) {
    scrap.invlet = g->nextinv;
    g->advance_nextinv();
    iter++;
   }
   if (!drop && (iter == inv_chars.size() || p->volume_carried() >= p->volume_capacity()))
    drop = true;
   if (drop)
    g->m.add_item(p->posx, p->posy, scrap);
   else
    p->i_add(scrap);
  }
  return;
  } else { g->add_msg("You can't cut that up!");
 } return;
}


void iuse::hacksaw(game *g, player *p, item *it, bool t)
{
 int dirx, diry;
 g->draw();
 mvprintw(0, 0, "Cut up metal where?");
 get_direction(g, dirx, diry, input());
 if (dirx == -2) {
  g->add_msg("Invalid direction.");
  return;
 }
if (dirx == 0 && diry == 0) {
  g->add_msg("Why would you do that?");
  g->add_msg("You're not even chained to a boiler.");
  return;
 }
 dirx += p->posx;
 diry += p->posy;
 if (g->m.ter(dirx, diry) == t_chainfence_v || g->m.ter(dirx, diry) == t_chainfence_h || g->m.ter(dirx, diry) == t_chaingate_c) {
  p->moves -= 500;
  g->m.ter_set(dirx, diry, t_dirt);
  g->sound(dirx, diry, 15,"grnd grnd grnd");
  g->m.spawn_item(dirx, diry, g->itypes["pipe"], 0, 6);
  g->m.spawn_item(dirx, diry, g->itypes["wire"], 0, 20);
 if (g->m.ter(dirx, diry) == t_chainfence_posts) {
  p->moves -= 500;
  g->m.ter_set(dirx, diry, t_dirt);
  g->sound(dirx, diry, 15,"grnd grnd grnd");
  g->m.spawn_item(dirx, diry, g->itypes["pipe"], 0, 6);
 } else if (g->m.ter(dirx, diry) == t_rack) {
  p->moves -= 500;
  g->m.ter_set(dirx, diry, t_floor);
  g->sound(dirx, diry, 15,"grnd grnd grnd");
  g->m.spawn_item(p->posx, p->posy, g->itypes["pipe"], 0, rng(1, 3));
  g->m.spawn_item(p->posx, p->posy, g->itypes["steel_chunk"], 0);
 } else if (g->m.ter(dirx, diry) == t_bars &&
            (g->m.ter(dirx + 1, diry) == t_sewage || g->m.ter(dirx, diry + 1) == t_sewage ||
             g->m.ter(dirx - 1, diry) == t_sewage || g->m.ter(dirx, diry - 1) == t_sewage)) {
  g->m.ter_set(dirx, diry, t_sewage);
  p->moves -= 1000;
  g->sound(dirx, diry, 15,"grnd grnd grnd");
  g->m.spawn_item(p->posx, p->posy, g->itypes["pipe"], 0, 3);
 } else if (g->m.ter(dirx, diry) == t_bars && g->m.ter(p->posx, p->posy)) {
  g->m.ter_set(dirx, diry, t_floor);
  p->moves -= 500;
  g->sound(dirx, diry, 15,"grnd grnd grnd");
  g->m.spawn_item(p->posx, p->posy, g->itypes["pipe"], 0, 3);
 } else {
  g->add_msg("You can't cut that.");
 }
}
}

void iuse::tent(game *g, player *p, item *it, bool t)
{
 int dirx, diry;
 g->draw();
 mvprintw(0, 0, "Put up tent where?");
 get_direction(g, dirx, diry, input());
 if (dirx == -2 || (dirx == 0 && diry == 0)) {
  g->add_msg_if_player(p,"Invalid direction.");
  return;
 }
 int posx = dirx + p->posx;
 int posy = diry + p->posy;
 posx += dirx;
 posy += diry;
 for (int i = -1; i <= 1; i++)
  for (int j = -1; j <= 1; j++)
   if (!g->m.has_flag(tentable, posx + i, posy + j)) {
    g->add_msg("You need a 3x3 diggable space to place a tent");
    return;
   }
 for (int i = -1; i <= 1; i++)
  for (int j = -1; j <= 1; j++)
    g->m.ter_set(posx + i, posy + j, t_canvas_wall);
 g->m.ter_set(posx, posy, t_groundsheet);
 g->m.ter_set(posx - dirx, posy - diry, t_canvas_door);
 it->invlet = 0;
}

void iuse::shelter(game *g, player *p, item *it, bool t)
{
 int dirx, diry;
 g->draw();
 mvprintw(0, 0, "Put up shelter where?");
 get_direction(g, dirx, diry, input());
 if (dirx == -2 || (dirx == 0 && diry == 0)) {
  g->add_msg_if_player(p,"Invalid direction.");
  return;
 }
 int posx = dirx + p->posx;
 int posy = diry + p->posy;
 posx += dirx;
 posy += diry;
 for (int i = -1; i <= 1; i++)
  for (int j = -1; j <= 1; j++)
   if (!g->m.has_flag(tentable, posx + i, posy + j)) {
    g->add_msg("You need a 3x3 diggable space to place a shelter");
    return;
   }
 for (int i = -1; i <= 1; i++)
  for (int j = -1; j <= 1; j++)
    g->m.ter_set(posx + i, posy + j, t_skin_wall);
 g->m.ter_set(posx, posy, t_skin_groundsheet);
 g->m.ter_set(posx - dirx, posy - diry, t_skin_door);
 it->invlet = 0;
}

void iuse::torch(game *g, player *p, item *it, bool t)
{
    if (!p->use_charges_if_avail("fire", 1))
    {
        g->add_msg_if_player(p,"You need a lighter or fire to light this.");
    }
    else
    {
        g->add_msg_if_player(p,"You light the torch.");
        it->make(g->itypes["torch_lit"]);
        it->active = true;
    }
}


void iuse::torch_lit(game *g, player *p, item *it, bool t)
{
 if (t) {	// Normal use
// Do nothing... player::active_light and the lightmap::generate deal with this
 } else {	// Turning it off
  g->add_msg_if_player(p,"The torch is extinguished");
  it->charges -= 1;
  it->make(g->itypes["torch"]);
  it->active = false;
 }
}


void iuse::candle(game *g, player *p, item *it, bool t)
{
    if (!p->use_charges_if_avail("fire", 1))
    {
        g->add_msg_if_player(p, "You need a lighter to light this.");
    }
    else
    {
        g->add_msg_if_player(p, "You light the candle.");
        it->make(g->itypes["candle_lit"]);
        it->active = true;
    }
}

void iuse::candle_lit(game *g, player *p, item *it, bool t)
{
 if (t) {	// Normal use
// Do nothing... player::active_light and the lightmap::generate deal with this
 } else {	// Turning it off
  g->add_msg_if_player(p,"The candle winks out");
  it->make(g->itypes["candle"]);
  it->active = false;
 }
}


void iuse::bullet_puller(game *g, player *p, item *it, bool t)
{
 char ch = g->inv("Disassemble what?");
 item* pull = &(p->i_at(ch));
 if (pull->type->id == "null") {
  g->add_msg("You do not have that item!");
  return;
 }
 if (p->skillLevel("gun") < 2) {
  g->add_msg("You need to be at least level 2 in the firearms skill before you\
  can disassemble ammunition.");
  return;}
 int multiply = pull->charges;
 if (multiply > 20)
 multiply = 20;
 item casing;
 item primer;
 item gunpowder;
 item lead;
 if (pull->type->id == "556_incendiary" || pull->type->id == "3006_incendiary" ||
     pull->type->id == "762_51_incendiary")
 lead.make(g->itypes["incendiary"]);
 else
 lead.make(g->itypes["lead"]);
 if (pull->type->id == "shot_bird") {
 casing.make(g->itypes["shot_hull"]);
 primer.make(g->itypes["shotgun_primer"]);
 gunpowder.make(g->itypes["gunpowder"]);
 gunpowder.charges = 12*multiply;
 lead.charges = 16*multiply;
 }
 else if (pull->type->id == "shot_00" || pull->type->id == "shot_slug") {
 casing.make(g->itypes["shot_hull"]);
 primer.make(g->itypes["shotgun_primer"]);
 gunpowder.make(g->itypes["gunpowder"]);
 gunpowder.charges = 20*multiply;
 lead.charges = 16*multiply;
 }
 else if (pull->type->id == "22_lr" || pull->type->id == "22_ratshot") {
 casing.make(g->itypes["22_casing"]);
 primer.make(g->itypes["smrifle_primer"]);
 gunpowder.make(g->itypes["gunpowder"]);
 gunpowder.charges = 2*multiply;
 lead.charges = 2*multiply;
 }
 else if (pull->type->id == "22_cb") {
 casing.make(g->itypes["22_casing"]);
 primer.make(g->itypes["smrifle_primer"]);
 gunpowder.make(g->itypes["gunpowder"]);
 gunpowder.charges = 1*multiply;
 lead.charges = 2*multiply;
 }
 else if (pull->type->id == "9mm") {
 casing.make(g->itypes["9mm_casing"]);
 primer.make(g->itypes["smpistol_primer"]);
 gunpowder.make(g->itypes["gunpowder"]);
 gunpowder.charges = 4*multiply;
 lead.charges = 4*multiply;
 }
 else if (pull->type->id == "9mmP") {
 casing.make(g->itypes["9mm_casing"]);
 primer.make(g->itypes["smpistol_primer"]);
 gunpowder.make(g->itypes["gunpowder"]);
 gunpowder.charges = 5*multiply;
 lead.charges = 4*multiply;
 }
 else if (pull->type->id == "9mmP2") {
 casing.make(g->itypes["9mm_casing"]);
 primer.make(g->itypes["smpistol_primer"]);
 gunpowder.make(g->itypes["gunpowder"]);
 gunpowder.charges = 6*multiply;
 lead.charges = 4*multiply;
 }
 else if (pull->type->id == "38_special") {
 casing.make(g->itypes["38_casing"]);
 primer.make(g->itypes["smpistol_primer"]);
 gunpowder.make(g->itypes["gunpowder"]);
 gunpowder.charges = 5*multiply;
 lead.charges = 5*multiply;
 }
 else if (pull->type->id == "38_super") {
 casing.make(g->itypes["38_casing"]);
 primer.make(g->itypes["smpistol_primer"]);
 gunpowder.make(g->itypes["gunpowder"]);
 gunpowder.charges = 7*multiply;
 lead.charges = 5*multiply;
 }
 else if (pull->type->id == "10mm") {
 casing.make(g->itypes["40_casing"]);
 primer.make(g->itypes["lgpistol_primer"]);
 gunpowder.make(g->itypes["gunpowder"]);
 gunpowder.charges = 8*multiply;
 lead.charges = 8*multiply;
 }
 else if (pull->type->id == "40sw") {
 casing.make(g->itypes["40_casing"]);
 primer.make(g->itypes["smpistol_primer"]);
 gunpowder.make(g->itypes["gunpowder"]);
 gunpowder.charges = 6*multiply;
 lead.charges = 6*multiply;
 }
 else if (pull->type->id == "44magnum") {
 casing.make(g->itypes["44_casing"]);
 primer.make(g->itypes["lgpistol_primer"]);
 gunpowder.make(g->itypes["gunpowder"]);
 gunpowder.charges = 10*multiply;
 lead.charges = 10*multiply;
 }
 else if (pull->type->id == "45_acp" || pull->type->id == "45_jhp") {
 casing.make(g->itypes["45_casing"]);
 primer.make(g->itypes["lgpistol_primer"]);
 gunpowder.make(g->itypes["gunpowder"]);
 gunpowder.charges = 10*multiply;
 lead.charges = 8*multiply;
 }
// else if (pull->type->id == "45_jhp") {
// casing.make(g->itypes["45_casing"]);
// primer.make(g->itypes["lgpistol_primer"]);
// gunpowder.make(g->itypes["gunpowder"]);
// gunpowder.charges = 10*multiply;
// lead.charges = 8*multiply;
// }
 else if (pull->type->id == "45_super") {
 casing.make(g->itypes["45_casing"]);
 primer.make(g->itypes["lgpistol_primer"]);
 gunpowder.make(g->itypes["gunpowder"]);
 gunpowder.charges = 12*multiply;
 lead.charges = 10*multiply;
 }
 else if (pull->type->id == "57mm") {
 casing.make(g->itypes["57mm_casing"]);
 primer.make(g->itypes["smrifle_primer"]);
 gunpowder.make(g->itypes["gunpowder"]);
 gunpowder.charges = 4*multiply;
 lead.charges = 2*multiply;
 }
 else if (pull->type->id == "46mm") {
 casing.make(g->itypes["46mm_casing"]);
 primer.make(g->itypes["smpistol_primer"]);
 gunpowder.make(g->itypes["gunpowder"]);
 gunpowder.charges = 4*multiply;
 lead.charges = 2*multiply;
 }
 else if (pull->type->id == "762_m43") {
 casing.make(g->itypes["762_casing"]);
 primer.make(g->itypes["lgrifle_primer"]);
 gunpowder.make(g->itypes["gunpowder"]);
 gunpowder.charges = 7*multiply;
 lead.charges = 5*multiply;
 }
 else if (pull->type->id == "762_m87") {
 casing.make(g->itypes["762_casing"]);
 primer.make(g->itypes["lgrifle_primer"]);
 gunpowder.make(g->itypes["gunpowder"]);
 gunpowder.charges = 8*multiply;
 lead.charges = 5*multiply;
 }
 else if (pull->type->id == "223") {
 casing.make(g->itypes["223_casing"]);
 primer.make(g->itypes["smrifle_primer"]);
 gunpowder.make(g->itypes["gunpowder"]);
 gunpowder.charges = 4*multiply;
 lead.charges = 2*multiply;
 }
 else if (pull->type->id == "556" || pull->type->id == "556_incendiary") {
 casing.make(g->itypes["223_casing"]);
 primer.make(g->itypes["smrifle_primer"]);
 gunpowder.make(g->itypes["gunpowder"]);
 gunpowder.charges = 6*multiply;
 lead.charges = 2*multiply;
 }
 else if (pull->type->id == "270") {
 casing.make(g->itypes["3006_casing"]);
 primer.make(g->itypes["lgrifle_primer"]);
 gunpowder.make(g->itypes["gunpowder"]);
 gunpowder.charges = 10*multiply;
 lead.charges = 5*multiply;
 }
 else if (pull->type->id == "3006" || pull->type->id == "3006_incendiary") {
 casing.make(g->itypes["3006_casing"]);
 primer.make(g->itypes["lgrifle_primer"]);
 gunpowder.make(g->itypes["gunpowder"]);
 gunpowder.charges = 8*multiply;
 lead.charges = 6*multiply;
 }
 else if (pull->type->id == "308") {
 casing.make(g->itypes["308_casing"]);
 primer.make(g->itypes["lgrifle_primer"]);
 gunpowder.make(g->itypes["gunpowder"]);
 gunpowder.charges = 10*multiply;
 lead.charges = 6*multiply;
 }
 else if (pull->type->id == "762_51" || pull->type->id == "762_51_incendiary") {
 casing.make(g->itypes["308_casing"]);
 primer.make(g->itypes["lgrifle_primer"]);
 gunpowder.make(g->itypes["gunpowder"]);
 gunpowder.charges = 10*multiply;
 lead.charges = 6*multiply;
 }
 else {
 g->add_msg("You cannot disassemble that.");
  return;
 }
 pull->charges = pull->charges - multiply;
 if (pull->charges == 0)
 p->i_rem(ch);
 g->add_msg("You take apart the ammunition.");
 p->moves -= 500;
 if (casing.type->id != "null"){
 casing.charges = multiply;
 int iter = 0;
   while ((casing.invlet == 0 || p->has_item(casing.invlet)) && iter < inv_chars.size()) {
    casing.invlet = g->nextinv;
    g->advance_nextinv();
    iter++;}
    if (p->weight_carried() + casing.weight() < p->weight_capacity() &&
      p->volume_carried() + casing.volume() < p->volume_capacity() && iter < inv_chars.size()) {
    p->i_add(casing);}
    else
   g->m.add_item(p->posx, p->posy, casing);}
 if (primer.type->id != "null"){
 primer.charges = multiply;
 int iter = 0;
   while ((primer.invlet == 0 || p->has_item(primer.invlet)) && iter < inv_chars.size()) {
    primer.invlet = g->nextinv;
    g->advance_nextinv();
    iter++;}
    if (p->weight_carried() + primer.weight() < p->weight_capacity() &&
      p->volume_carried() + primer.volume() < p->volume_capacity() && iter < inv_chars.size()) {
    p->i_add(primer);}
    else
   g->m.add_item(p->posx, p->posy, primer);}
 int iter = 0;
   while ((gunpowder.invlet == 0 || p->has_item(gunpowder.invlet)) && iter < inv_chars.size()) {
    gunpowder.invlet = g->nextinv;
    g->advance_nextinv();
    iter++;}
    if (p->weight_carried() + gunpowder.weight() < p->weight_capacity() &&
      p->volume_carried() + gunpowder.volume() < p->volume_capacity() && iter < inv_chars.size()) {
    p->i_add(gunpowder);}
    else
   g->m.add_item(p->posx, p->posy, gunpowder);
 iter = 0;
   while ((lead.invlet == 0 || p->has_item(lead.invlet)) && iter < inv_chars.size()) {
    lead.invlet = g->nextinv;
    g->advance_nextinv();
    iter++;}
    if (p->weight_carried() + lead.weight() < p->weight_capacity() &&
      p->volume_carried() + lead.volume() < p->volume_capacity() && iter < inv_chars.size()) {
    p->i_add(lead);}
    else
   g->m.add_item(p->posx, p->posy, lead);

  p->practice(g->turn, "gun", rng(1, multiply / 5 + 1));
}

void iuse::boltcutters(game *g, player *p, item *it, bool t)
{
 int dirx, diry;
 g->draw();
 mvprintw(0, 0, "Cut up metal where?");
 get_direction(g, dirx, diry, input());
 if (dirx == -2) {
  g->add_msg("Invalid direction.");
  return;
 }
if (dirx == 0 && diry == 0) {
  g->add_msg_if_player(p, "You neatly sever all of the veins");
  g->add_msg_if_player(p, "and arteries in your body. Oh wait,");
  g->add_msg_if_player(p, "Never mind.");
  return;
}
 dirx += p->posx;
 diry += p->posy;

 if (g->m.ter(dirx, diry) == t_chaingate_l) {
  p->moves -= 100;
  g->m.ter_set(dirx, diry, t_chaingate_c);
  g->sound(dirx, diry, 5, "Gachunk!");
  g->m.spawn_item(p->posx, p->posy, g->itypes["scrap"], 0, 3);
 } else if (g->m.ter(dirx, diry) == t_chainfence_v || g->m.ter(dirx, diry) == t_chainfence_h) {
  p->moves -= 500;
  g->m.ter_set(dirx, diry, t_chainfence_posts);
  g->sound(dirx, diry, 5,"Snick, snick, gachunk!");
  g->m.spawn_item(dirx, diry, g->itypes["wire"], 0, 20);
 } else {
  g->add_msg("You can't cut that.");
 }
}

void iuse::mop(game *g, player *p, item *it, bool t)
{
 int dirx, diry;
 g->draw();
 mvprintw(0, 0, "Mop where?");
 get_direction(g, dirx, diry, input());
 if (dirx == -2) {
  g->add_msg_if_player(p,"Invalid direction.");
  return;
 }
 if (dirx == 0 && diry == 0) {
   g->add_msg_if_player(p,"You mop yourself up.");
   g->add_msg_if_player(p,"The universe implodes and reforms around you.");
   return;
}
 p->moves -= 15;
 dirx += p->posx;
 diry += p->posy;
  if (g->m.moppable_items_at(dirx, diry)) {
   g->m.mop_spills(dirx, diry);
   g->add_msg("You mop up the spill");
 } else {
  g->add_msg_if_player(p,"There's nothing to mop there.");
 }
}
void iuse::rag(game *g, player *p, item *it, bool t)
{
 if (p->has_disease(DI_BLEED)){
  if (one_in(2)){
   g->add_msg_if_player(p,"You managed to stop the bleeding.");
   p->rem_disease(DI_BLEED);
  } else {
   g->add_msg_if_player(p,"You couldnt stop the bleeding.");
  }
  p->use_charges("rag", 1);
  it->make(g->itypes["rag_bloody"]);
 } else {
  g->add_msg_if_player(p,"Nothing to use the rag for.");
 }

}

void iuse::pda(game *g, player *p, item *it, bool t)
{
 if (it->charges == 0)
  g->add_msg_if_player(p,"The PDA's batteries are dead.");
 else {
  g->add_msg_if_player(p,"You activate the flashlight app.");
  it->make(g->itypes["pda_flashlight"]);
  it->active = true;
  it->charges --;
 }
}

void iuse::pda_flashlight(game *g, player *p, item *it, bool t)
{
 if (t) {	// Normal use
// Do nothing... player::active_light and the lightmap::generate deal with this
 } else {	// Turning it off
  g->add_msg_if_player(p,"The PDA screen goes blank.");
  it->make(g->itypes["pda"]);
  it->active = false;
 }
}

void iuse::LAW(game *g, player *p, item *it, bool t)
{
 g->add_msg_if_player(p,"You pull the activating lever, readying the LAW to fire.");
 it->make(g->itypes["LAW"]);
 it->charges++;
 // When converting a tool to a gun, you need to set the current ammo type, this is usually done when a gun is reloaded.
 it->curammo = dynamic_cast<it_ammo*>(g->itypes["66mm_HEAT"]);
}

/* MACGUFFIN FUNCTIONS
 * These functions should refer to it->associated_mission for the particulars
 */
void iuse::mcg_note(game *g, player *p, item *it, bool t)
{
 std::stringstream message;
 message << "Dear " << it->name << ":\n";
/*
 faction* fac = NULL;
 direction dir = NORTH;
// Pick an associated faction
 switch (it->associated_mission) {
 case MISSION_FIND_FAMILY_FACTION:
  fac = &(g->factions[rng(0, g->factions.size() - 1)]);
  break;
 case MISSION_FIND_FAMILY_KIDNAPPER:
  fac = g->random_evil_faction();
  break;
 }
// Calculate where that faction is
 if (fac != NULL) {
  int omx = g->cur_om->posx, omy = g->cur_om->posy;
  if (fac->omx != g->cur_om->posx || fac->omx != g->cur_om->posy)
   dir = direction_from(omx, omy, fac->omx, fac->omy);
  else
   dir = direction_from(g->levx, g->levy, fac->mapx, fac->mapy);
 }
// Produce the note and generate the next mission
 switch (it->associated_mission) {
 case MISSION_FIND_FAMILY_FACTION:
  if (fac->name == "The army")
   message << "\
I've been rescued by an army patrol.  They're taking me\n\
to their outpost to the " << direction_name(dir) << ".\n\
Please meet me there.  I need to know you're alright.";
  else
   message << "\
This group came through, looking for survivors.  They\n\
said they were members of this group calling itself\n" << fac->name << ".\n\
They've got a settlement to the " << direction_name(dir) << ", so\n\
I guess I'm heading there.  Meet me there as soon as\n\
you can, I need to know you're alright.";
  break;


  popup(message.str().c_str());
*/
}

void iuse::artifact(game *g, player *p, item *it, bool t)
{
 if (!it->is_artifact()) {
  debugmsg("iuse::artifact called on a non-artifact item! %s",
           it->tname().c_str());
  return;
 } else if (!it->is_tool()) {
  debugmsg("iuse::artifact called on a non-tool artifact! %s",
           it->tname().c_str());
  return;
 }
 it_artifact_tool *art = dynamic_cast<it_artifact_tool*>(it->type);
 int num_used = rng(1, art->effects_activated.size());
 if (num_used < art->effects_activated.size())
  num_used += rng(1, art->effects_activated.size() - num_used);

 std::vector<art_effect_active> effects = art->effects_activated;
 for (int i = 0; i < num_used; i++) {
  int index = rng(0, effects.size() - 1);
  art_effect_active used = effects[index];
  effects.erase(effects.begin() + index);

  switch (used) {
  case AEA_STORM: {
   g->sound(p->posx, p->posy, 10, "Ka-BOOM!");
   int num_bolts = rng(2, 4);
   for (int j = 0; j < num_bolts; j++) {
    int xdir = 0, ydir = 0;
    while (xdir == 0 && ydir == 0) {
     xdir = rng(-1, 1);
     ydir = rng(-1, 1);
    }
    int dist = rng(4, 12);
    int boltx = p->posx, bolty = p->posy;
    for (int n = 0; n < dist; n++) {
     boltx += xdir;
     bolty += ydir;
     g->m.add_field(g, boltx, bolty, fd_electricity, rng(2, 3));
     if (one_in(4)) {
      if (xdir == 0)
       xdir = rng(0, 1) * 2 - 1;
      else
       xdir = 0;
     }
     if (one_in(4)) {
      if (ydir == 0)
       ydir = rng(0, 1) * 2 - 1;
      else
       ydir = 0;
     }
    }
   }
  } break;

  case AEA_FIREBALL: {
   point fireball = g->look_around();
   if (fireball.x != -1 && fireball.y != -1)
    g->explosion(fireball.x, fireball.y, 8, 0, true);
  } break;

  case AEA_ADRENALINE:
   g->add_msg_if_player(p,"You're filled with a roaring energy!");
   p->add_disease(DI_ADRENALINE, rng(200, 250), g);
   break;

  case AEA_MAP: {
   bool new_map = false;
   for (int x = int(g->levx / 2) - 20; x <= int(g->levx / 2) + 20; x++) {
    for (int y = int(g->levy / 2) - 20; y <= int(g->levy / 2) + 20; y++) {
     if (!g->cur_om->seen(x, y, g->levz)) {
      new_map = true;
      g->cur_om->seen(x, y, g->levz) = true;
     }
    }
   }
   if (new_map) {
    g->add_msg_if_player(p,"You have a vision of the surrounding area...");
    p->moves -= 100;
   }
  } break;

  case AEA_BLOOD: {
   bool blood = false;
   for (int x = p->posx - 4; x <= p->posx + 4; x++) {
    for (int y = p->posy - 4; y <= p->posy + 4; y++) {
     if (!one_in(4) && g->m.add_field(g, x, y, fd_blood, 3) &&
         (blood || g->u_see(x, y)))
      blood = true;
    }
   }
   if (blood)
    g->add_msg_if_player(p,"Blood soaks out of the ground and walls.");
  } break;

  case AEA_FATIGUE: {
   g->add_msg_if_player(p,"The fabric of space seems to decay.");
   int x = rng(p->posx - 3, p->posx + 3), y = rng(p->posy - 3, p->posy + 3);
   if (g->m.field_at(x, y).type == fd_fatigue &&
       g->m.field_at(x, y).density < 3)
    g->m.field_at(x, y).density++;
   else
    g->m.add_field(g, x, y, fd_fatigue, rng(1, 2));
  } break;

  case AEA_ACIDBALL: {
   point acidball = g->look_around();
   if (acidball.x != -1 && acidball.y != -1) {
    for (int x = acidball.x - 1; x <= acidball.x + 1; x++) {
     for (int y = acidball.y - 1; y <= acidball.y + 1; y++) {
      if (g->m.field_at(x, y).type == fd_acid &&
          g->m.field_at(x, y).density < 3)
       g->m.field_at(x, y).density++;
      else
       g->m.add_field(g, x, y, fd_acid, rng(2, 3));
     }
    }
   }
  } break;

  case AEA_PULSE:
   g->sound(p->posx, p->posy, 30, "The earth shakes!");
   for (int x = p->posx - 2; x <= p->posx + 2; x++) {
    for (int y = p->posy - 2; y <= p->posy + 2; y++) {
     std::string junk;
     g->m.bash(x, y, 40, junk);
     g->m.bash(x, y, 40, junk);  // Multibash effect, so that doors &c will fall
     g->m.bash(x, y, 40, junk);
     if (g->m.is_destructable(x, y) && rng(1, 10) >= 3)
      g->m.ter_set(x, y, t_rubble);
    }
   }
   break;

  case AEA_HEAL:
   g->add_msg_if_player(p,"You feel healed.");
   p->healall(2);
   break;

  case AEA_CONFUSED:
   for (int x = p->posx - 8; x <= p->posx + 8; x++) {
    for (int y = p->posy - 8; y <= p->posy + 8; y++) {
     int mondex = g->mon_at(x, y);
     if (mondex != -1)
      g->z[mondex].add_effect(ME_STUNNED, rng(5, 15));
    }
   }

  case AEA_ENTRANCE:
   for (int x = p->posx - 8; x <= p->posx + 8; x++) {
    for (int y = p->posy - 8; y <= p->posy + 8; y++) {
     int mondex = g->mon_at(x, y);
     if (mondex != -1 &&  g->z[mondex].friendly == 0 &&
         rng(0, 600) > g->z[mondex].hp)
      g->z[mondex].make_friendly();
    }
   }
   break;

  case AEA_BUGS: {
   int roll = rng(1, 10);
   mon_id bug = mon_null;
   int num = 0;
   std::vector<point> empty;
   for (int x = p->posx - 1; x <= p->posx + 1; x++) {
    for (int y = p->posy - 1; y <= p->posy + 1; y++) {
     if (g->is_empty(x, y))
      empty.push_back( point(x, y) );
    }
   }
   if (empty.empty() || roll <= 4)
    g->add_msg_if_player(p,"Flies buzz around you.");
   else if (roll <= 7) {
    g->add_msg_if_player(p,"Giant flies appear!");
    bug = mon_fly;
    num = rng(2, 4);
   } else if (roll <= 9) {
    g->add_msg_if_player(p,"Giant bees appear!");
    bug = mon_bee;
    num = rng(1, 3);
   } else {
    g->add_msg_if_player(p,"Giant wasps appear!");
    bug = mon_wasp;
    num = rng(1, 2);
   }
   if (bug != mon_null) {
    monster spawned(g->mtypes[bug]);
    spawned.friendly = -1;
    for (int j = 0; j < num && !empty.empty(); j++) {
     int index_inner = rng(0, empty.size() - 1);
     point spawnp = empty[index_inner];
     empty.erase(empty.begin() + index_inner);
     spawned.spawn(spawnp.x, spawnp.y);
     g->z.push_back(spawned);
    }
   }
  } break;

  case AEA_TELEPORT:
   g->teleport(p);
   break;

  case AEA_LIGHT:
   g->add_msg_if_player(p,"The %s glows brightly!", it->tname().c_str());
   g->add_event(EVENT_ARTIFACT_LIGHT, int(g->turn) + 30);
   break;

  case AEA_GROWTH: {
   monster tmptriffid(g->mtypes[0], p->posx, p->posy);
   mattack tmpattack;
   tmpattack.growplants(g, &tmptriffid);
  } break;

  case AEA_HURTALL:
   for (int j = 0; j < g->z.size(); j++)
    g->z[j].hurt(rng(0, 5));
   break;

  case AEA_RADIATION:
   g->add_msg("Horrible gasses are emitted!");
   for (int x = p->posx - 1; x <= p->posx + 1; x++) {
    for (int y = p->posy - 1; y <= p->posy + 1; y++)
     g->m.add_field(g, x, y, fd_nuke_gas, rng(2, 3));
   }
   break;

  case AEA_PAIN:
   g->add_msg_if_player(p,"You're wracked with pain!");
   p->pain += rng(5, 15);
   break;

  case AEA_MUTATE:
   if (!one_in(3))
    p->mutate(g);
   break;

  case AEA_PARALYZE:
   g->add_msg_if_player(p,"You're paralyzed!");
   p->moves -= rng(50, 200);
   break;

  case AEA_FIRESTORM:
   g->add_msg_if_player(p,"Fire rains down around you!");
   for (int x = p->posx - 3; x <= p->posx + 3; x++) {
    for (int y = p->posy - 3; y <= p->posy + 3; y++) {
     if (!one_in(3)) {
      if (g->m.add_field(g, x, y, fd_fire, 1 + rng(0, 1) * rng(0, 1)))
       g->m.field_at(x, y).age = 30;
     }
    }
   }
   break;

  case AEA_ATTENTION:
   g->add_msg_if_player(p,"You feel like your action has attracted attention.");
   p->add_disease(DI_ATTENTION, 600 * rng(1, 3), g);
   break;

  case AEA_TELEGLOW:
   g->add_msg_if_player(p,"You feel unhinged.");
   p->add_disease(DI_TELEGLOW, 100 * rng(3, 12), g);
   break;

  case AEA_NOISE:
   g->add_msg_if_player(p,"Your %s emits a deafening boom!", it->tname().c_str());
   g->sound(p->posx, p->posy, 100, "");
   break;

  case AEA_SCREAM:
   g->add_msg_if_player(p,"Your %s screams disturbingly.", it->tname().c_str());
   g->sound(p->posx, p->posy, 40, "");
   p->add_morale(MORALE_SCREAM, -10);
   break;

  case AEA_DIM:
   g->add_msg_if_player(p,"The sky starts to dim.");
   g->add_event(EVENT_DIM, int(g->turn) + 50);
   break;

  case AEA_FLASH:
   g->add_msg_if_player(p,"The %s flashes brightly!", it->tname().c_str());
   g->flashbang(p->posx, p->posy);
   break;

  case AEA_VOMIT:
   g->add_msg_if_player(p,"A wave of nausea passes through you!");
   p->vomit(g);
   break;

  case AEA_SHADOWS: {
   int num_shadows = rng(4, 8);
   monster spawned(g->mtypes[mon_shadow]);
   int num_spawned = 0;
   for (int j = 0; j < num_shadows; j++) {
    int tries = 0, monx, mony, junk;
    do {
     if (one_in(2)) {
      monx = rng(p->posx - 5, p->posx + 5);
      mony = (one_in(2) ? p->posy - 5 : p->posy + 5);
     } else {
      monx = (one_in(2) ? p->posx - 5 : p->posx + 5);
      mony = rng(p->posy - 5, p->posy + 5);
     }
    } while (tries < 5 && !g->is_empty(monx, mony) &&
             !g->m.sees(monx, mony, p->posx, p->posy, 10, junk));
    if (tries < 5) {
     num_spawned++;
     spawned.sp_timeout = rng(8, 20);
     spawned.spawn(monx, mony);
     g->z.push_back(spawned);
    }
   }
   if (num_spawned > 1)
    g->add_msg_if_player(p,"Shadows form around you.");
   else if (num_spawned == 1)
    g->add_msg_if_player(p,"A shadow forms nearby.");
  } break;

  }
 }
}

void iuse::spray_can(game *g, player *p, item *it, bool t)
{
 std::string message = string_input_popup("Spray what?");
 if(g->m.add_graffiti(g, p->posx, p->posy, message))
  g->add_msg("You spray a message on the ground.");
 else
  g->add_msg("You fail to spray a message here.");
}

void iuse::heatpack(game *g, player *p, item *it, bool t)
{
	char ch = g->inv("Heat up what?");
	item* heat = &(p->i_at(ch));
	if (heat->type->id == "null") {
		g->add_msg("You do not have that item!");
		return;
	}
	if (heat->type->is_food()) {
		p->moves -= 300;
		g->add_msg("You heat up the food.");
		heat->item_tags.insert("HOT");
		heat->active = true;
		heat->item_counter = 600;		// sets the hot food flag for 60 minutes
		it->make(g->itypes["heatpack_used"]);
		return;
  } else 	if (heat->is_food_container()) {
		p->moves -= 300;
		g->add_msg("You heat up the food.");
		heat->contents[0].item_tags.insert("HOT");
		heat->contents[0].active = true;
		heat->contents[0].item_counter = 600;		// sets the hot food flag for 60 minutes
		it->make(g->itypes["heatpack_used"]);
		return;
	}
  { g->add_msg("You can't heat that up!");
 } return;
}

void iuse::dejar(game *g, player *p, item *it, bool t)
{
	g->add_msg_if_player(p,"You open the jar, exposing it to the atmosphere.");
	itype_id ujfood = (it->type->id).substr(4,-1);  // assumes "jar_" is at front of itype_id and removes it
	item ujitem(g->itypes[ujfood],0);  // temp create item to discover container
	itype_id ujcont = (dynamic_cast<it_comest*>(ujitem.type))->container;  //discovering container
	it->make(g->itypes[ujcont]);  //turning "sealed jar of xxx" into container for "xxx"
    it->contents.push_back(item(g->itypes[ujfood],0));  //shoving the "xxx" into the container
    it->contents[0].bday = g->turn + 3600 - (g->turn % 3600);
}

void iuse::devac(game *g, player *p, item *it, bool t)
{
	g->add_msg_if_player(p,"You open the vacuum pack, exposing it to the atmosphere.");
	itype_id uvfood = (it->type->id).substr(4,-1);  // assumes "bag_" is at front of itype_id and removes it
	item uvitem(g->itypes[uvfood],0);  // temp create item to discover container
	itype_id uvcont = (dynamic_cast<it_comest*>(uvitem.type))->container;  //discovering container
	it->make(g->itypes[uvcont]);  //turning "vacuum packed xxx" into container for "xxx"
    it->contents.push_back(item(g->itypes[uvfood],0));  //shoving the "xxx" into the container
    it->contents[0].bday = g->turn + 3600 - (g->turn % 3600);
}
