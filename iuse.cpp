#include "iuse.h"
#include "game.h"
#include "mapdata.h"
#include "keypress.h"
#include "output.h"
#include "rng.h"
#include "line.h"
#include "mutation.h"
#include "player.h"
#include "vehicle.h"
#include <sstream>
#include <algorithm>

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
  if( replacement.is_drink() ) {
      it->charges++;
      return;
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

// Returns false if the inscription failed or if the player canceled the action. Otherwise, returns true.
static bool inscribe_item( game *g, player *p, std::string verb, std::string gerund, bool carveable )
{
    char ch = g->inv(verb + " on what?");
    item* cut = &(p->i_at(ch));
    if (cut->type->id == "null")
    {
        g->add_msg("You do not have that item!");
        return false;
    }
    if (!cut->made_of(SOLID))
    {
        std::string lower_verb = verb;
        std::transform(lower_verb.begin(), lower_verb.end(), lower_verb.begin(), ::tolower);
        g->add_msg("You can't %s an item that's not solid!", lower_verb.c_str());
        return false;
    }
    if(carveable && !(cut->made_of("wood") || cut->made_of("plastic") || cut->made_of("glass") ||
                      cut->made_of("chitin") || cut->made_of("iron") || cut->made_of("steel") ||
                      cut->made_of("silver"))) {
        std::string lower_verb = verb;
        std::transform(lower_verb.begin(), lower_verb.end(), lower_verb.begin(), ::tolower);
        g->add_msg("You can't %s an item made of %s!",
                   lower_verb.c_str(), cut->get_material(1).c_str());
        return false;
    }

    std::map<std::string, std::string>::iterator ent = cut->item_vars.find("item_note");
    std::string message = gerund + " on this " + cut->type->name + " is a note saying: ";
    message = string_input_popup(verb + " what?", 64, (ent != cut->item_vars.end() ?
                                                       cut->item_vars["item_note"] : message ));

    if( message.size() > 0 ) { cut->item_vars["item_note"] = message; }
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
  g->m.spawn_item(p->posx, p->posy, "wax",0, 2);
}

void iuse::royal_jelly(game *g, player *p, item *it, bool t)
{
// TODO: Add other diseases here; royal jelly is a cure-all!
 p->pkill += 5;
 std::string message;
 if (p->has_disease("fungus")) {
  message = "You feel cleansed inside!";
  p->rem_disease("fungus");
 }
 if (p->has_disease("blind")) {
  message = "Your sight returns!";
  p->rem_disease("blind");
 }
 if (p->has_disease("poison") || p->has_disease("foodpoison") ||
     p->has_disease("badpoison")) {
  message = "You feel much better!";
  p->rem_disease("poison");
  p->rem_disease("badpoison");
  p->rem_disease("foodpoison");
 }
 if (p->has_disease("asthma")) {
  message = "Your breathing clears up!";
  p->rem_disease("asthma");
 }
 if (p->has_disease("common_cold") || p->has_disease("flu")) {
  message = "You feel healthier!";
  p->rem_disease("common_cold");
  p->rem_disease("flu");
 }
 g->add_msg_if_player(p,message.c_str());
}

// returns true if we want to use the special action
bool use_healing_item(game *g, player *p, item *it, int normal_power, int head_power,
                      int torso_power, std::string item_name, std::string special_action)
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

        mvwprintz(hp_window, 1, 1, c_ltred,  "Use %s:", item_name.c_str());
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
        if(special_action != "")
        {
            mvwprintz(hp_window, 8, 1, c_ltgray, "7: %s", special_action.c_str());
        }
        mvwprintz(hp_window, 9, 1, c_ltgray, "8: Exit");
        nc_color color;
        std::string asterisks = "";
        for (int i = 0; i < num_hp_parts; i++)
        {
            if (p->hp_cur[i] < p->hp_max[i])
            {
                int current_hp = p->hp_cur[i];
                int temporary_bonus = bonus;
                if (current_hp != 0)
                {
                    switch (hp_part(i)) {
                        case hp_head:
                            current_hp += head_power;
                            temporary_bonus *=  .8;
                            break;
                        case hp_torso:
                            current_hp += torso_power;
                            temporary_bonus *= 1.5;
                            break;
                        default:
                            current_hp += normal_power;
                            break;
                    }
                    current_hp += temporary_bonus;
                    if (current_hp > p->hp_max[i])
                    {
                        current_hp = p->hp_max[i];
                    }
                    if (current_hp == p->hp_max[i])
                    {
                        color = c_green;
                        asterisks = "***";
                    }
                    else if (current_hp > p->hp_max[i] * .8)
                    {
                        color = c_ltgreen;
                        asterisks = "***";
                    }
                    else if (current_hp > p->hp_max[i] * .5)
                    {
                        color = c_yellow;
                        asterisks = "** ";
                    }
                    else if (current_hp > p->hp_max[i] * .3)
                    {
                        color = c_ltred;
                        asterisks = "** ";
                    }
                    else
                    {
                        color = c_red;
                        asterisks = "*  ";
                    }
                    if (p->has_trait(PF_SELFAWARE))
                    {
                        if (current_hp >= 100)
                        {
                            mvwprintz(hp_window, i + 2, 16, color, "%d", current_hp);
                        }
                        else if (current_hp >= 10)
                        {
                            mvwprintz(hp_window, i + 2, 17, color, "%d", current_hp);
                        }
                        else
                        {
                            mvwprintz(hp_window, i + 2, 19, color, "%d", current_hp);
                        }
                    }
                    else
                    {
                        mvwprintz(hp_window, i + 2, 16, color, asterisks.c_str());
                    }
                }
                else
                {
                    // curhp is 0; requires surgical attention
                    mvwprintz(hp_window, i + 2, 16, c_dkgray, "---");
                }
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
                    return false;
                } else {
                    healed = hp_arm_l;
                }
            } else if (ch == '4') {
                if (p->hp_cur[hp_arm_r] == 0) {
                    g->add_msg_if_player(p,"That arm is broken.  It needs surgical attention.");
                    add_or_drop_item(g, p, it);
                    return false;
                } else {
                    healed = hp_arm_r;
                }
            } else if (ch == '5') {
                if (p->hp_cur[hp_leg_l] == 0) {
                    g->add_msg_if_player(p,"That leg is broken.  It needs surgical attention.");
                    add_or_drop_item(g, p, it);
                    return false;
                } else {
                    healed = hp_leg_l;
                }
            } else if (ch == '6') {
                if (p->hp_cur[hp_leg_r] == 0) {
                    g->add_msg_if_player(p,"That leg is broken.  It needs surgical attention.");
                    add_or_drop_item(g, p, it);
                    return false;
                } else {
                    healed = hp_leg_r;
                }
            } else if (ch == '8' || special_action == "") {
                g->add_msg_if_player(p,"Never mind.");
                add_or_drop_item(g, p, it);
                return false;
            } else if (ch == '7') {
                return true;
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
        dam = head_power + bonus * .8;
    } else if (healed == hp_torso){
        dam = torso_power + bonus * 1.5;
    } else {
        dam = normal_power + bonus;
    }
    p->heal(healed, dam);
    return false;
}

void iuse::bandage(game *g, player *p, item *it, bool t)
{
    if (use_healing_item(g, p, it, 3, 1, 4, "Bandage", p->has_disease("bleed") ? "Stop Bleeding" : ""))
    {
        g->add_msg_if_player(p,"You stopped the bleeding.");
        p->rem_disease("bleed");
    }
}

void iuse::firstaid(game *g, player *p, item *it, bool t)
{
    if (use_healing_item(g, p, it, 14, 10, 18, "First Aid", p->has_disease("bite") ? "Clean Wound" : ""))
    {
        g->add_msg_if_player(p,"You clean the bite wound.");
        p->rem_disease("bite");
    }
}

void iuse::disinfectant(game *g, player *p, item *it, bool t)
{

    if (use_healing_item(g, p, it, 6, 5, 9, "Disinfectant", p->has_disease("bite") ? "Clean Wound" : ""))
    {
        g->add_msg_if_player(p,"You disinfect the bite wound.");
        p->rem_disease("bite");
    }
}

void iuse::pkill(game *g, player *p, item *it, bool t)
{
    // Aspirin
    if (it->has_flag("PKILL_1")) {
        g->add_msg_if_player(p,"You take some %s.", it->tname().c_str());
        if (!p->has_disease("pkill1")) {
            p->add_disease("pkill1", 120);
        } else {
            for (int i = 0; i < p->illness.size(); i++) {
                if (p->illness[i].type == "pkill1") {
                    p->illness[i].duration = 120;
                    i = p->illness.size();
                }
            }
        }
    // Codeine
    } else if (it->has_flag("PKILL_2")) {
        g->add_msg_if_player(p,"You take some %s.", it->tname().c_str());
        p->add_disease("pkill2", 180);

    } else if (it->has_flag("PKILL_3")) {
        g->add_msg_if_player(p,"You take some %s.", it->tname().c_str());
        p->add_disease("pkill3", 20);
        p->add_disease("pkill2", 200);

    } else if (it->has_flag("PKILL_4")) {
        g->add_msg_if_player(p,"You shoot up.");
        p->add_disease("pkill3", 80);
        p->add_disease("pkill2", 200);

    } else if (it->has_flag("PKILL_L")) {
        g->add_msg_if_player(p,"You take some %s.", it->tname().c_str());
        p->add_disease("pkill_l", rng(12, 18) * 300);
    }
}

void iuse::xanax(game *g, player *p, item *it, bool t)
{
 g->add_msg_if_player(p,"You take some %s.", it->tname().c_str());

 if (!p->has_disease("took_xanax"))
  p->add_disease("took_xanax", 900);
 else
  p->add_disease("took_xanax", 200);
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
 p->add_disease("drunk", duration);
}

void iuse::alcohol_weak(game *g, player *p, item *it, bool t)
{
 int duration = 340 - (6 * p->str_max);
 if (p->has_trait(PF_LIGHTWEIGHT))
  duration += 120;
 p->pkill += 4;
 p->add_disease("drunk", duration);
}

void iuse::cig(game *g, player *p, item *it, bool t)
{
 if (!use_fire(g, p, it)) return;
 if (it->type->id == "cig")
  g->add_msg_if_player(p,"You light a cigarette and smoke it.");
 else //cigar
  g->add_msg_if_player(p,"You take a few puffs from your cigar.");
 p->add_disease("cig", 200);
 for (int i = 0; i < p->illness.size(); i++) {
  if (p->illness[i].type == "cig" && p->illness[i].duration > 600 &&
      !p->is_npc())
   g->add_msg_if_player(p,"Ugh, too much smoke... you feel gross.");
 }
}

void iuse::antibiotic(game *g, player *p, item *it, bool t)
{
if (p->has_disease("infected")){
  g->add_msg_if_player(p,"You took some antibiotics.");
  p->rem_disease("infected");
  p->add_disease("recover", 1200);
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
 p->add_disease("high", duration);
}

void iuse::coke(game *g, player *p, item *it, bool t)
{
 g->add_msg_if_player(p,"You snort a bump.");

 int duration = 21 - p->str_cur;
 if (p->has_trait(PF_LIGHTWEIGHT))
  duration += 20;
 p->hunger -= 8;
 p->add_disease("high", duration);
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
  p->add_disease("high", duration);
}

void iuse::grack(game *g, player *p, item *it, bool t)
{
  // Grack requires a fire source AND a pipe.
  if (!use_fire(g, p, it)) return;
  g->add_msg_if_player(p,"You smoke some Grack Cocaine. Time seems to stop.");
  int duration = 1000;
  if (p->has_trait(PF_LIGHTWEIGHT))
    duration += 10;
  p->hunger -= 8;
  p->add_disease("grack", duration);
}


void iuse::meth(game *g, player *p, item *it, bool t)
{
    int duration = 10 * (40 - p->str_cur);
    if (p->has_amount("apparatus", 1) &&
        p->use_charges_if_avail("fire", 1))
    {
        g->add_msg_if_player(p,"You smoke some crystals.");
        duration *= 1.5;
    }
    else
    {
        g->add_msg_if_player(p,"You snort some crystals.");
    }
    if (!p->has_disease("meth")) {duration += 600;}
    if (duration > 0)
    {
        int hungerpen = (p->str_cur < 10 ? 20 : 30 - p->str_cur);
        p->hunger -= hungerpen;
        p->add_disease("meth", duration);
    }
}

void iuse::vitamins(game *g, player *p, item *it, bool t)
{
    g->add_msg_if_player(p,"You take some vitamins.");
    if (p->health >= 10)
    {
        return;
    }
    else if (p->health >= 0)
    {
        p->health = 10;
    }
    else
    {
        p->health += 10;
    }
}

void iuse::vaccine(game *g, player *p, item *it, bool t)
{
    g->add_msg_if_player(p,"You inject the vaccine, and feel much healthier.");
    if (p->health >= 100)
    {
        return;
    }
    else if (p->health >= 0)
    {
        p->health = 100;
    }
    else
    {
        p->health += 100;
    }
}

void iuse::poison(game *g, player *p, item *it, bool t)
{
 p->add_disease("poison", 600);
 p->add_disease("foodpoison", 1800);
}

void iuse::hallu(game *g, player *p, item *it, bool t)
{
 p->add_disease("hallu", 2400);
}

void iuse::thorazine(game *g, player *p, item *it, bool t)
{
 p->fatigue += 15;
 p->rem_disease("hallu");
 p->rem_disease("visuals");
 p->rem_disease("high");
 if (!p->has_disease("dermatik"))
  p->rem_disease("formication");
 g->add_msg_if_player(p,"You feel somewhat sedated.");
}

void iuse::prozac(game *g, player *p, item *it, bool t)
{
 if (!p->has_disease("took_prozac") && p->morale_level() < 0)
  p->add_disease("took_prozac", 7200);
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
 p->add_disease("iodine", 1200);
 g->add_msg_if_player(p,"You take an iodine tablet.");
}

void iuse::flumed(game *g, player *p, item *it, bool t)
{
 p->add_disease("took_flumed", 6000);
 g->add_msg_if_player(p,"You take some %s", it->tname().c_str());
}

void iuse::flusleep(game *g, player *p, item *it, bool t)
{
 p->add_disease("took_flumed", 7200);
 p->fatigue += 30;
 g->add_msg_if_player(p,"You feel very sleepy...");
}

void iuse::inhaler(game *g, player *p, item *it, bool t)
{
 p->rem_disease("asthma");
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
    if( it->has_flag("MUTAGEN_STRONG") )
    {
         p->mutate(g);
         if (!one_in(3))
             p->mutate(g);
         if (one_in(2))
             p->mutate(g);
    }
    else if( it->has_flag("MUTAGEN_PLANT") )
    {
        g->add_msg_if_player(p, "You feel much closer to nature.");
        p->mutate_category(g, MUTCAT_PLANT);
    }
    else if( it->has_flag("MUTAGEN_INSECT") )
    {
        g->add_msg_if_player(p, "You hear buzzing, and feel your body harden.");
        p->mutate_category(g, MUTCAT_INSECT);
    }
    else if( it->has_flag("MUTAGEN_SPIDER") )
    {
        g->add_msg_if_player(p, "You feel insidious.");
        p->mutate_category(g, MUTCAT_SPIDER);
    }
    else if( it->has_flag("MUTAGEN_SLIME") )
    {
        g->add_msg_if_player(p, "Your body loses all rigidity for a moment.");
        p->mutate_category(g, MUTCAT_SLIME);
    }
    else if( it->has_flag("MUTAGEN_FISH") )
    {
        g->add_msg_if_player(p, "You are overcome by an overwhelming longing for the ocean.");
        p->mutate_category(g, MUTCAT_FISH);
    }
    else if( it->has_flag("MUTAGEN_RAT") )
    {
        g->add_msg_if_player(p, "You feel a momentary nausea.");
        p->mutate_category(g, MUTCAT_RAT);
    }
    else if( it->has_flag("MUTAGEN_BEAST") )
    {
        g->add_msg_if_player(p, "Your heart races and you see blood for a moment.");
        p->mutate_category(g, MUTCAT_BEAST);
    }
    else if( it->has_flag("MUTAGEN_CATTLE") )
    {
        g->add_msg_if_player(p, "Your mind and body slow down. You feel peaceful.");
        p->mutate_category(g, MUTCAT_CATTLE);
    }
    else if( it->has_flag("MUTAGEN_CEPHALOPOD") )
    {
        g->add_msg_if_player(p, "Your mind is overcome by images of eldritch horrors...and then they pass.");
        p->mutate_category(g, MUTCAT_CEPHALOPOD);
    }
    else if( it->has_flag("MUTAGEN_BIRD") )
    {
        g->add_msg_if_player(p, "Your body lightens and you long for the sky.");
        p->mutate_category(g, MUTCAT_BIRD);
    }
    else if( it->has_flag("MUTAGEN_LIZARD") )
    {
        g->add_msg_if_player(p, "For a heartbeat, your body cools down.");
        p->mutate_category(g, MUTCAT_LIZARD);
    }
    else if( it->has_flag("MUTAGEN_TROGLOBITE") )
    {
        g->add_msg_if_player(p, "You yearn for a cool, dark place to hide.");
        p->mutate_category(g, MUTCAT_TROGLO);
    }
    else
    {
        if (!one_in(3))
        {
            p->mutate(g);
        }
    }
}

void iuse::purifier(game *g, player *p, item *it, bool t)
{
 std::vector<int> valid;	// Which flags the player has
 for (int i = PF_NULL+1; i < PF_MAX2; i++) {
  if (p->has_trait(pl_flag(i)) && !p->has_base_trait(pl_flag(i)))  //Looks for active mutation
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
  p->toggle_mutation(PF_MARLOSS);
 }
 if (effect == 6)
  p->radiation = 0;
}

void iuse::dogfood(game *g, player *p, item *it, bool t)
{
 int dirx, diry;
 if(!g->choose_adjacent("Put the dog food",dirx,diry))
  return;
 p->moves -= 15;
 int mon_dex = g->mon_at(dirx,diry);
 if (mon_dex != -1) {
  if (g->z[mon_dex].type->id == mon_dog) {
   g->add_msg_if_player(p,"The dog seems to like you!");
   g->z[mon_dex].friendly = -1;
  } else
   g->add_msg_if_player(p,"The %s seems quite unimpressed!",g->z[mon_dex].type->name.c_str());
 } else
  g->add_msg_if_player(p,"You spill the dogfood all over the ground.");
}


// TOOLS below this point!

bool prep_firestarter_use(game *g, player *p, item *it, int &posx, int &posy)
{
   if (!g->choose_adjacent("Light",posx,posy))
   {
     it->charges++;
     return false;
   }
   if (posx == p->posx && posy == p->posy)
   {
     g->add_msg_if_player(p, "You would set yourself on fire.");
     g->add_msg_if_player(p, "But you're already smokin' hot.");
     it->charges++;
     return false;
   }

   if (!(g->m.flammable_items_at(posx, posy)  || g->m.has_flag(flammable, posx, posy) || g->m.has_flag(flammable2, posx, posy)))
   {
     g->add_msg_if_player(p,"There's nothing to light there.");
     it->charges++;
     return false;
   }
   else
   {
     return true;
   }
}

void resolve_firestarter_use(game *g, player *p, item *it, int posx, int posy)
{
    // this should have already been checked, but double-check to make sure
	if (g->m.flammable_items_at(posx, posy) || g->m.has_flag(flammable, posx, posy) || g->m.has_flag(flammable2, posx, posy))
    {
        if (g->m.add_field(g, posx, posy, fd_fire, 1))
        {
            field &current_field = g->m.field_at(posx, posy);
            current_field.findField(fd_fire)->setFieldAge(current_field.findField(fd_fire)->getFieldAge() + 100);
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
    //some items are made from more than one material. we should try to use both items if one type of repair item is missing
    itype_id repair_item = "none";
    std::vector<std::string> plurals;
    std::vector<itype_id> repair_items;
    std::string plural = "";
    if (fix->made_of("cotton") || fix->made_of("wool"))
    {
        repair_items.push_back("rag");
        plurals.push_back("s");
    }
    if (fix->made_of("leather"))
    {
        repair_items.push_back("leather");
        plurals.push_back("");
    }
    if (fix->made_of("fur"))
    {
        plurals.push_back("");
        repair_items.push_back("fur");
    }
    if(repair_items.empty())
	{
        g->add_msg_if_player(p,"Your %s is not made of cotton, wool, leather or fur.", fix->tname().c_str());
        it->charges++;
        return;
    }

    int items_needed=(fix->damage>2||fix->damage==0)?1:0;

    // this will cause issues if/when NPCs start being able to sew.
    // but, then again, it'll cause issues when they start crafting, too.
    inventory crafting_inv = g->crafting_inventory(p);
    bool bFound = false;
    //go through all discovered repair items and see if we have any of them available
    for(unsigned int i = 0; i< repair_items.size(); i++)
    {
        if (crafting_inv.has_amount(repair_items[i], items_needed))
        {
           //we've found enough of a material, use this one
           repair_item = repair_items[i];
           bFound = true;
        }
    }
    if (!bFound)
    {
        for(unsigned int i = 0; i< repair_items.size(); i++)
        {
            g->add_msg_if_player(p,"You don't have enough %s%s to do that.", repair_items[i].c_str(), plurals[i].c_str());
        }
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
    comps.push_back(component(repair_item, items_needed));
    comps.back().available = true;


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
            g->consume_items(p, comps);
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
            if (fix->damage>=3) {g->consume_items(p, comps);}
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
            if (fix->damage>=3) {g->consume_items(p, comps);}
            fix->damage--;
        }
	    else
	    {
            g->add_msg_if_player(p,"You repair your %s completely!", fix->tname().c_str());
            if (fix->damage>=3) {g->consume_items(p, comps);}
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
    if (tool->ammo != "battery")
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
    if (!cut->made_of("cotton") && !cut->made_of("leather"))
    {
        g->add_msg("You can only slice items made of cotton or leather.");
        return;
    }

    //scrap_text is the description of worthless scraps, type is the item type,
    //pre_text is the bit before the plural on a success, post_text is the bit after the plural
    std::string scrap_text, pre_text, post_text, type;
    if (cut->made_of("cotton"))
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

    if (cut->damage>2 || cut->damage<0)
    {
        count-= cut->damage;
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
 int x, y;
 // If anyone other than the player wants to use one of these,
 // they're going to need to figure out how to aim it.
 if (!g->choose_adjacent("Spray", x, y))
  return;

 p->moves -= 140;

 field &current_field = g->m.field_at(x, y);
 if (current_field.findField(fd_fire)) {
     current_field.findField(fd_fire)->setFieldDensity(current_field.findField(fd_fire)->getFieldDensity() - rng(2, 3));
     if (current_field.findField(fd_fire)->getFieldDensity() <= 0) {
   //g->m.field_at(x, y).density = 1;
   g->m.remove_field(x, y, fd_fire);
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
  x += (x - p->posx);
  y += (y - p->posy);

  if (current_field.findField(fd_fire)) {
   current_field.findField(fd_fire)->setFieldDensity(current_field.findField(fd_fire)->getFieldDensity() - rng(0, 1) + rng(0, 1));
   if (current_field.findField(fd_fire)->getFieldDensity() <= 0) {
    //g->m.field_at(x, y).density = 1;
    g->m.remove_field(x, y,fd_fire);
   }
  }
 }
}

void iuse::hammer(game *g, player *p, item *it, bool t)
{
    g->draw();
    int x, y;
    // If anyone other than the player wants to use one of these,
    // they're going to need to figure out how to aim it.
    if (!g->choose_adjacent("Pry", x, y))
        return;

    if (x == p->posx && y == p->posy)
    {
        g->add_msg_if_player(p, "You try to hit yourself with the hammer.");
        g->add_msg_if_player(p, "But you can't touch this.");
        return;
    }

    int nails = 0, boards = 0;
    ter_id newter;
    switch (g->m.ter(x, y))
    {
        case t_fence_h:
        case t_fence_v:
        nails = 6;
        boards = 3;
        newter = t_fence_post;
        break;

        case t_window_boarded:
        nails =  8;
        boards = 4;
        newter = t_window_empty;
        break;

        case t_door_boarded:
        nails = 12;
        boards = 4;
        newter = t_door_b;
        break;

        default:
        g->add_msg_if_player(p,"Hammers can only remove boards from windows, doors and fences.");
        g->add_msg_if_player(p,"To board up a window or door, press *");
        return;
    }
    p->moves -= 500;
    g->m.spawn_item(p->posx, p->posy, "nail", 0, 0, nails);
    g->m.spawn_item(p->posx, p->posy, "2x4", 0, boards);
    g->m.ter_set(x, y, newter);
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

    else if (!p->has_disease("bite") && !p->has_disease("bleed"))
    g->add_msg_if_player(p,"You are not bleeding or bitten, there is no need to cauterize yourself.");

    else if (p->is_npc() || query_yn("Cauterize any open wounds?"))
    {
        it->charges -= 1;
        p->cauterize(g);
    }
}

void iuse::solder_weld(game *g, player *p, item *it, bool t)
{
    it->charges += (dynamic_cast<it_tool*>(it->type))->charges_per_use;
    int choice = menu(true,
    "Using soldering item:", "Cauterize wound", "Repair plastic/metal/kevlar item", "Cancel", NULL);
    switch (choice)
    {
        case 1:
        {
            iuse::cauterize_elec(g, p, it, t);
        }
        break;
        case 2:
        {
            if(it->charges <= 0)
            {
                g->add_msg_if_player(p,"You don't have enough batteries!");
                return;
            }

            char ch = g->inv_type("Repair what?", IC_ARMOR);
            item* fix = &(p->i_at(ch));
            if (fix == NULL || fix->is_null())
            {
                g->add_msg_if_player(p,"You do not have that item!");
                return;
            }
            if (!fix->is_armor())
            {
                g->add_msg_if_player(p,"That isn't clothing!");
                return;
            }
            itype_id repair_item = "none";
            std::vector<std::string> plurals;
            std::vector<std::string> repairitem_names;
            std::vector<itype_id> repair_items;
            if (fix->made_of("kevlar"))
            {
                repair_items.push_back("kevlar_plate");
                repairitem_names.push_back("kevlar plate");
                plurals.push_back("s");
            }
            if (fix->made_of("plastic"))
            {
                repair_items.push_back("plastic_chunk");
                repairitem_names.push_back("plastic chunk");
                plurals.push_back("s");
            }
            if (fix->made_of("iron") || fix->made_of("steel"))
            {
                repair_items.push_back("scrap");
                repairitem_names.push_back("scrap metal");
                plurals.push_back("");
            }
            if(repair_items.empty())
            {
                g->add_msg_if_player(p,"Your %s is not made of kevlar, plastic or metal.", fix->tname().c_str());
                return;
            }

            //repairing or modifying items requires at least 1 repair item, otherwise number is related to size of item
            int items_needed=ceil( fix->volume() * 0.25);

            // this will cause issues if/when NPCs start being able to sew.
            // but, then again, it'll cause issues when they start crafting, too.
            inventory crafting_inv = g->crafting_inventory(p);

             bool bFound = false;
            //go through all discovered repair items and see if we have any of them available
            for(unsigned int i = 0; i< repair_items.size(); i++)
            {
                if (crafting_inv.has_amount(repair_items[i], items_needed))
                {
                   //we've found enough of a material, use this one
                   repair_item = repair_items[i];
                   bFound = true;
                }
            }
            if (!bFound)
            {
                for(unsigned int i = 0; i< repair_items.size(); i++)
                {
                    g->add_msg_if_player(p,"You don't have enough %s%s to do that.", repairitem_names[i].c_str(), plurals[i].c_str());
                }
                return;
            }
            if (fix->damage < 0)
            {
                g->add_msg_if_player(p,"Your %s is already enhanced.", fix->tname().c_str());
                return;
            }

            std::vector<component> comps;
            comps.push_back(component(repair_item, items_needed));
            comps.back().available = true;


            if (fix->damage == 0)
            {
                p->moves -= 500 * p->fine_detail_vision_mod(g);
                p->practice(g->turn, "mechanics", 10);
                int rn = dice(4, 2 + p->skillLevel("mechanics"));
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
                    g->consume_items(p, comps);
                }
                else
                {
                    g->add_msg_if_player(p,"You practice your soldering.");
                }
            }
            else
            {
                p->moves -= 500 * p->fine_detail_vision_mod(g);
                p->practice(g->turn, "mechanics", 8);
                int rn = dice(4, 2 + p->skillLevel("mechanics"));
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
                    g->add_msg_if_player(p,"You don't repair your %s, and you waste lots of charge.", fix->tname().c_str());
                    int waste = rng(1, 8);
                    if (waste > it->charges)
                        {it->charges = 1;}
                    else
                        {it->charges -= waste;}
                }
                else if (rn <= 8)
                {
                    g->add_msg_if_player(p,"You repair your %s, but you waste lots of charge.", fix->tname().c_str());
                    if (fix->damage>=3) {g->consume_items(p, comps);}
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
                    if (fix->damage>=3) {g->consume_items(p, comps);}
                    fix->damage--;
                }
                else
                {
                    g->add_msg_if_player(p,"You repair your %s completely!", fix->tname().c_str());
                    if (fix->damage>=3) {g->consume_items(p, comps);}
                    fix->damage = 0;
                }
            }

            it->charges -= 1;
        }
        break;
        case 3:
        {

        }
        break;
        default:
            break;
    };
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
 if(!g->choose_adjacent("Use your pick lock", dirx, diry))
  return;
 if (dirx == p->posx && diry == p->posy) {
  g->add_msg_if_player(p, "You pick your nose and your sinuses swing open.");
  return;
 }
 ter_id type = g->m.ter(dirx, diry);
 int npcdex = g->npc_at(dirx, diry);
 if (npcdex != -1) {
  g->add_msg_if_player(p, "You can pick your friends, and you can");
  g->add_msg_if_player(p, "pick your nose, but you can't pick");
  g->add_msg_if_player(p, "your friend's nose");
  return;
 }

 int pick_quality = 1;
 if( it->typeId() == "picklock" ) {
     pick_quality = 5;
 }
 else if( it->typeId() == "crude_picklock" ) {
     pick_quality = 2;
 }


 const char *door_name;
 ter_id new_type;
 if (type == t_chaingate_l) {
   door_name = "gate";
   new_type = t_chaingate_c;
 } else if (type == t_door_locked || type == t_door_locked_alarm || type == t_door_locked_interior) {
   door_name = "door";
   new_type = t_door_c;
 } else if (type == t_door_bar_locked) {
   door_name = "door";
   new_type = t_door_bar_o;
   g->add_msg_if_player(p, "The door swings open...");
 } else {
  g->add_msg("That cannot be picked.");
  return;
 }

 p->practice(g->turn, "mechanics", 1);
 p->moves -= (1000 - (pick_quality * 100)) - (p->dex_cur + p->skillLevel("mechanics")) * 5;
 if (dice(3, 25) / pick_quality < dice(2, p->skillLevel("mechanics")) +
     dice(2, p->dex_cur) - it->damage / 2) {
  p->practice(g->turn, "mechanics", 1);
  g->add_msg_if_player(p,"With a satisfying click, the lock on the %s opens.", door_name);
  g->m.ter_set(dirx, diry, new_type);
 } else if (dice(6, 30) / pick_quality < dice(2, p->skillLevel("mechanics")) +
            dice(2, p->dex_cur) - it->damage / 2 &&
            it->damage < 100) {
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
      dice(5, 17) / pick_quality < dice(2, p->skillLevel("mechanics")) +
      dice(2, p->dex_cur) - it->damage / 2 && it->damage < 100) {
  g->sound(p->posx, p->posy, 40, "An alarm sounds!");
  if (!g->event_queued(EVENT_WANTED)) {
   g->add_event(EVENT_WANTED, int(g->turn) + 300, 0, g->levx, g->levy);
  }
 }
}

void iuse::crowbar(game *g, player *p, item *it, bool t)
{
 int dirx, diry;
 if(!g->choose_adjacent("Pry", dirx,diry))
  return;

 if (dirx == p->posx && diry == p->posy) {
    g->add_msg_if_player(p, "You attempt to pry open your wallet");
    g->add_msg_if_player(p, "but alas. You are just too miserly.");
    return;
  }
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
  } else if (type == t_door_bar_locked) {
    door_name = "door";
    action_name = "pry open";
    new_type = t_door_bar_o;
    noisy = false;
    difficulty = 10;
  } else if (type == t_manhole_cover) {
    door_name = "manhole cover";
    action_name = "lift";
    new_type = t_manhole;
    noisy = false;
    difficulty = 12;
  } else if (g->m.furn(dirx, diry) == f_crate_c) {
    door_name = "crate";
    action_name = "pop open";
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
    boards = 4;
    newter = t_window_empty;
    break;
   case t_door_boarded:
    nails = 12;
    boards = 4;
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
   g->m.spawn_item(p->posx, p->posy, "nail", 0, 0, nails);
   g->m.spawn_item(p->posx, p->posy, "2x4", 0, boards);
   g->m.ter_set(dirx, diry, newter);
   return;
  }

  p->practice(g->turn, "mechanics", 1);
  p->moves -= (difficulty * 25) - ((p->str_cur + p->skillLevel("mechanics")) * 5);
  if (dice(4, difficulty) < dice(2, p->skillLevel("mechanics")) + dice(2, p->str_cur)) {
   p->practice(g->turn, "mechanics", 1);
   g->add_msg_if_player(p,"You %s the %s.", action_name, door_name);
   if (g->m.furn(dirx, diry) == f_crate_c)
    g->m.furn_set(dirx, diry, f_crate_o);
   else
    g->m.ter_set(dirx, diry, new_type);
   if (noisy)
    g->sound(dirx, diry, 12, "crunch!");
   if ( type == t_door_locked_alarm ) {
    g->sound(p->posx, p->posy, 40, "An alarm sounds!");
    if (!g->event_queued(EVENT_WANTED)) {
     g->add_event(EVENT_WANTED, int(g->turn) + 300, 0, g->levx, g->levy);
    }
   }
  } else {
   if (type == t_window_domestic || type == t_curtains) {
    //chance of breaking the glass if pry attempt fails
    if (dice(4, difficulty) > dice(2, p->skillLevel("mechanics")) + dice(2, p->str_cur)) {
     g->add_msg_if_player(p,"You break the glass.");
     g->sound(dirx, diry, 24, "glass breaking!");
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

//TODO remove this?
void iuse::dig(game *g, player *p, item *it, bool t)
{
 g->add_msg_if_player(p,"You can dig a pit via the construction menu--hit *");
}

void iuse::siphon(game *g, player *p, item *it, bool t)
{
    int posx = 0;
    int posy = 0;
    if(!g->choose_adjacent("Siphon from", posx, posy))
      return;

    vehicle* veh = g->m.veh_at(posx, posy);
    if (veh == NULL)
    {
        g->add_msg_if_player(p,"There's no vehicle there.");
        return;
    }
    if (veh->fuel_left("gasoline") == 0)
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
 if(!g->choose_adjacent("Drill",dirx,diry))
  return;

 if (dirx == p->posx && diry == p->posy) {
  g->add_msg_if_player(p,"My god! Let's talk it over OK?");
  g->add_msg_if_player(p,"Don't do anything rash..");
  return;
 }
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
  g->add_msg_if_player(p,"Whoa buddy! You can't go cheating in items and");
  g->add_msg_if_player(p,"just expect them to work! Now put the pickaxe");
  g->add_msg_if_player(p,"down and go play the game.");
}
void iuse::set_trap(game *g, player *p, item *it, bool t)
{
 int dirx, diry;
 if(!g->choose_adjacent("Place trap",dirx,diry))
  return;

 if (dirx == p->posx && diry == p->posy) {
  g->add_msg_if_player(p,"Yeah. Place the %s at your feet.", it->tname().c_str());
  g->add_msg_if_player(p,"Real damn smart move.");
  return;
 }
 int posx = dirx;
 int posy = diry;
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
  posx = (dirx - p->posx)*2 + p->posx; //math correction for blade trap
  posy = (diry - p->posy)*2 + p->posy;
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

void iuse::arrow_flamable(game *g, player *p, item *it, bool t)
{
 if (!p->use_charges_if_avail("fire", 1))
 {
  g->add_msg_if_player(p,"You need a lighter!");
  return;
 }
 g->add_msg_if_player(p,"You light the arrow!.");
 p->moves -= 150;
 it->make(g->itypes["arrow_flamming"]);
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
 if(!g->choose_adjacent("Place the turret", dirx, diry))
  return;
 if (!g->is_empty(dirx, diry)) {
  g->add_msg_if_player(p,"You cannot place a turret there.");
  return;
 }

 p->moves -= 100;
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

void iuse::adv_UPS_off(game *g, player *p, item *it, bool t)
{
 if (it->charges == 0)
  g->add_msg_if_player(p,"The power supply has depleted the plutonium.");
 else {
  g->add_msg_if_player(p,"You turn the power supply on.");
  if (p->is_wearing("goggles_nv"))
   g->add_msg_if_player(p,"Your light amp goggles power on.");
  if (p->worn.size() && p->worn[0].type->is_power_armor())
    g->add_msg_if_player(p, "Your power armor engages.");
  it->make(g->itypes["adv_UPS_on"]);
  it->active = true;
 }
}

void iuse::adv_UPS_on(game *g, player *p, item *it, bool t)
{
 if (t) {	// Normal use
   if (p->worn.size() && p->worn[0].type->is_power_armor()) {
     it->charges -= 2;

     if (it->charges < 0) {
       it->charges = 0;
     }
   }
 } else {	// Turning it off
  g->add_msg_if_player(p,"The advanced UPS powers off with a soft hum.");
  if (p->worn.size() && p->worn[0].type->is_power_armor())
    g->add_msg_if_player(p, "Your power armor disengages.");
  it->make(g->itypes["adv_UPS_off"]);
  it->active = false;
 }
}

void iuse::tazer(game *g, player *p, item *it, bool t)
{
 int dirx, diry;
 if(!g->choose_adjacent("Shock",dirx,diry)){
  it->charges += (dynamic_cast<it_tool*>(it->type))->charges_per_use;
  return;
 }

 if (dirx == p->posx && diry == p->posy) {
  g->add_msg_if_player(p,"Umm. No.");
  it->charges += (dynamic_cast<it_tool*>(it->type))->charges_per_use;
  return;
 }
 int mondex = g->mon_at(dirx, diry);
 int npcdex = g->npc_at(dirx, diry);
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
  if (!p->has_item(it) || p->has_disease("deaf") )
   return;	// We're not carrying it, or we're deaf.
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
                      "Using knife:", "Cut up fabric", "Cut up plastic/kevlar", "Carve wood", "Cauterize", "Carve writing on item", "Cancel", NULL);
    switch (choice)
    {
        if (choice == 5)
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
           int amount = 0;
           if (cut->type->id == "null")
            {
                g->add_msg("You do not have that item!");
                return;
            }
            if(cut->made_of("plastic"))
            {
                //if we're going to cut up a bottle, make sure it isn't full of liquid
                amount = cut->volume();
                if(cut->is_container())
                {
                    if(cut->is_food_container())
                    {
                        g->add_msg("That container has liquid in it!");
                        break;
                    }
                }
                if(amount == 0)
                {
                    g->add_msg("This object is too small to salvage a meaningful quantity of plastic from!");
                    break;
                }


                g->add_msg("You cut the %s into %i plastic chunks.", cut->tname().c_str(), amount);
                int count = amount;
                item result(g->itypes["plastic_chunk"], int(g->turn), g->nextinv);
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
                    p->i_add(result);
                }
            }
            else if(cut->made_of("kevlar"))
            {
                amount = cut->volume();
                if(amount == 0)
                {
                    g->add_msg("This object is too small to salvage a meaningful quantity of kevlar from!");
                    break;
                }

                g->add_msg("You cut the %s into %i plastic chunks.", cut->tname().c_str(), amount);
                int count = amount;
                item result(g->itypes["kevlar_plate"], int(g->turn), g->nextinv);
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
                    p->i_add(result);
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
        case 4:
        {
            if (!p->has_disease("bite") && !p->has_disease("bleed"))
                g->add_msg_if_player(p,"You are not bleeding or bitten, there is no need to cauterize yourself.");
            else if (!p->use_charges_if_avail("fire", 4))
                g->add_msg_if_player(p,"You need a lighter with 4 charges before you can cauterize yourself.");
            else
                p->cauterize(g);
            break;
        }
        case 5:
        {
            inscribe_item( g, p, "Carve", "Carved", true );
            break;
        }
    }
}

void iuse::cut_log_into_planks(game *g, player *p, item *it)
{
    p->moves -= 300;
    g->add_msg("You cut the log into planks.");
    item plank(g->itypes["2x4"], int(g->turn), g->nextinv);
    item scrap(g->itypes["splinter"], int(g->turn), g->nextinv);
    bool drop = false;
    int planks = (rng(1, 3) + (p->skillLevel("carpentry") * 2));
    int scraps = 12 - planks;
    if (planks >= 12) {
        planks = 12;
    }
    if (scraps >= planks) {
        g->add_msg("You waste a lot of the wood.");
    }
    for (int i = 0; i < planks; i++) {
        int iter = 0;
        while (p->has_item(plank.invlet)) {
            plank.invlet = g->nextinv;
            g->advance_nextinv();
            iter++;
        }
        if (!drop && (iter == inv_chars.size() || p->volume_carried() >= p->volume_capacity())) {
            drop = true;
        }
        if (drop) {
            g->m.add_item(p->posx, p->posy, plank);
        } else {
            p->i_add(plank);
        }
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
     p->i_rem(ch);
     cut_log_into_planks(g, p, it);
     return;
 } else {
     g->add_msg("You can't cut that up!");
 } return;
}


void iuse::hacksaw(game *g, player *p, item *it, bool t)
{
 int dirx, diry;
 if(!g->choose_adjacent("Cut up metal", dirx, diry))
  return;

if (dirx == p->posx && diry == p->posy) {
  g->add_msg("Why would you do that?");
  g->add_msg("You're not even chained to a boiler.");
  return;
 }
 if (g->m.ter(dirx, diry) == t_chainfence_v || g->m.ter(dirx, diry) == t_chainfence_h || g->m.ter(dirx, diry) == t_chaingate_c) {
  p->moves -= 500;
  g->m.ter_set(dirx, diry, t_dirt);
  g->sound(dirx, diry, 15,"grnd grnd grnd");
  g->m.spawn_item(dirx, diry, "pipe", 0, 6);
  g->m.spawn_item(dirx, diry, "wire", 0, 20);
 if (g->m.ter(dirx, diry) == t_chainfence_posts) {
  p->moves -= 500;
  g->m.ter_set(dirx, diry, t_dirt);
  g->sound(dirx, diry, 15,"grnd grnd grnd");
  g->m.spawn_item(dirx, diry, "pipe", 0, 6);
 } else if (g->m.furn(dirx, diry) == f_rack) {
  p->moves -= 500;
  g->m.furn_set(dirx, diry, f_null);
  g->sound(dirx, diry, 15,"grnd grnd grnd");
  g->m.spawn_item(p->posx, p->posy, "pipe", 0, rng(1, 3));
  g->m.spawn_item(p->posx, p->posy, "steel_chunk", 0);
 } else if (g->m.ter(dirx, diry) == t_bars &&
            (g->m.ter(dirx + 1, diry) == t_sewage || g->m.ter(dirx, diry + 1) == t_sewage ||
             g->m.ter(dirx - 1, diry) == t_sewage || g->m.ter(dirx, diry - 1) == t_sewage)) {
  g->m.ter_set(dirx, diry, t_sewage);
  p->moves -= 1000;
  g->sound(dirx, diry, 15,"grnd grnd grnd");
  g->m.spawn_item(p->posx, p->posy, "pipe", 0, 3);
 } else if (g->m.ter(dirx, diry) == t_bars && g->m.ter(p->posx, p->posy)) {
  g->m.ter_set(dirx, diry, t_floor);
  p->moves -= 500;
  g->sound(dirx, diry, 15,"grnd grnd grnd");
  g->m.spawn_item(p->posx, p->posy, "pipe", 0, 3);
 } else {
  g->add_msg("You can't cut that.");
 }
}
}

void iuse::tent(game *g, player *p, item *it, bool t)
{
 int dirx, diry;
 if(!g->choose_adjacent("Pitch the tent", dirx, diry))
  return;

 //must place the center of the tent two spaces away from player
 //dirx and diry will be integratined with the player's position
 int posx = dirx - p->posx;
 int posy = diry - p->posy;
 if(posx == 0 && posy == 0){
  g->add_msg_if_player(p,"Invalid Direction");
  return;
 }
 posx = posx*2 + p->posx;
 posy = posy*2 + p->posy;
 for (int i = -1; i <= 1; i++)
  for (int j = -1; j <= 1; j++)
   if (!g->m.has_flag(flat, posx + i, posy + j) ||
        g->m.has_furn(posx + i, posy + j)) {
    g->add_msg("You need a 3x3 flat space to place a tent");
    return;
   }
 for (int i = -1; i <= 1; i++)
  for (int j = -1; j <= 1; j++)
    g->m.furn_set(posx + i, posy + j, f_canvas_wall);
 g->m.furn_set(posx, posy, f_groundsheet);
 g->m.furn_set(posx - (dirx - p->posx), posy - (diry - p->posy), f_canvas_door);
 it->invlet = 0;
}

void iuse::shelter(game *g, player *p, item *it, bool t)
{
 int dirx, diry;
 if(!g->choose_adjacent("Put up the shelter", dirx, diry))
  return;

 //must place the center of the tent two spaces away from player
 //dirx and diry will be integratined with the player's position
 int posx = dirx - p->posx;
 int posy = diry - p->posy;
 if(posx == 0 && posy == 0){
  g->add_msg_if_player(p,"Invalid Direction");
  return;
 }
 posx = posx*2 + p->posx;
 posy = posy*2 + p->posy;
 for (int i = -1; i <= 1; i++)
  for (int j = -1; j <= 1; j++)
   if (!g->m.has_flag(flat, posx + i, posy + j) ||
        g->m.has_furn(posx + i, posy + j)) {
    g->add_msg("You need a 3x3 flat space to place a shelter");
    return;
   }
 for (int i = -1; i <= 1; i++)
  for (int j = -1; j <= 1; j++)
    g->m.furn_set(posx + i, posy + j, f_skin_wall);
 g->m.furn_set(posx, posy, f_skin_groundsheet);
 g->m.furn_set(posx - (dirx - p->posx), posy - (diry - p->posy), f_skin_door);
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
 else if (pull->type->id == "45_acp" ||
          pull->type->id == "45_jhp") {
 casing.make(g->itypes["45_casing"]);
 primer.make(g->itypes["lgpistol_primer"]);
 gunpowder.make(g->itypes["gunpowder"]);
 gunpowder.charges = 10*multiply;
 lead.charges = 8*multiply;
 }
 else if (pull->type->id == "45_super") {
 casing.make(g->itypes["45_casing"]);
 primer.make(g->itypes["lgpistol_primer"]);
 gunpowder.make(g->itypes["gunpowder"]);
 gunpowder.charges = 12*multiply;
 lead.charges = 10*multiply;
 }
 else if (pull->type->id == "454_Casull") {
 casing.make(g->itypes["454_casing"]);
 primer.make(g->itypes["smrifle_primer"]);
 gunpowder.make(g->itypes["gunpowder"]);
 gunpowder.charges = 20*multiply;
 lead.charges = 20*multiply;
 }
 else if (pull->type->id == "500_Magnum") {
 casing.make(g->itypes["500_casing"]);
 primer.make(g->itypes["lgpistol_primer"]);
 gunpowder.make(g->itypes["gunpowder"]);
 gunpowder.charges = 24*multiply;
 lead.charges = 24*multiply;
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
 if(!g->choose_adjacent("Cut up metal",dirx,diry))
  return;

if (dirx == p->posx && diry == p->posy) {
  g->add_msg_if_player(p, "You neatly sever all of the veins");
  g->add_msg_if_player(p, "and arteries in your body. Oh wait,");
  g->add_msg_if_player(p, "Never mind.");
  return;
}
 if (g->m.ter(dirx, diry) == t_chaingate_l) {
  p->moves -= 100;
  g->m.ter_set(dirx, diry, t_chaingate_c);
  g->sound(dirx, diry, 5, "Gachunk!");
  g->m.spawn_item(p->posx, p->posy, "scrap", 0, 3);
 } else if (g->m.ter(dirx, diry) == t_chainfence_v || g->m.ter(dirx, diry) == t_chainfence_h) {
  p->moves -= 500;
  g->m.ter_set(dirx, diry, t_chainfence_posts);
  g->sound(dirx, diry, 5,"Snick, snick, gachunk!");
  g->m.spawn_item(dirx, diry, "wire", 0, 20);
 } else {
  g->add_msg("You can't cut that.");
 }
}

void iuse::mop(game *g, player *p, item *it, bool t)
{
 int dirx, diry;
 if(!g->choose_adjacent("Mop",dirx,diry))
  return;

 if (dirx == p->posx && diry == p->posy) {
   g->add_msg_if_player(p,"You mop yourself up.");
   g->add_msg_if_player(p,"The universe implodes and reforms around you.");
   return;
}
  if (g->m.moppable_items_at(dirx, diry)) {
   g->m.mop_spills(dirx, diry);
   g->add_msg("You mop up the spill");
   p->moves -= 15;
 } else {
  g->add_msg_if_player(p,"There's nothing to mop there.");
 }
}
void iuse::rag(game *g, player *p, item *it, bool t)
{
 if (p->has_disease("bleed")){
  if (one_in(2)){
   g->add_msg_if_player(p,"You managed to stop the bleeding.");
   p->rem_disease("bleed");
  } else {
   g->add_msg_if_player(p,"You couldn't stop the bleeding.");
  }
  p->use_charges("rag", 1);
  it->make(g->itypes["rag_bloody"]);
 } else {
  g->add_msg_if_player(p,"You're not bleeding enough to need your %s.", it->type->name.c_str());
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
   p->add_disease("adrenaline", rng(200, 250));
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
    g->m.add_field(g, x, y, fd_fatigue, rng(1, 2));
  } break;

  case AEA_ACIDBALL: {
   point acidball = g->look_around();
   if (acidball.x != -1 && acidball.y != -1) {
    for (int x = acidball.x - 1; x <= acidball.x + 1; x++) {
     for (int y = acidball.y - 1; y <= acidball.y + 1; y++) {
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
		  g->m.field_at(x, y).findField(fd_fire)->setFieldAge(g->m.field_at(x, y).findField(fd_fire)->getFieldAge() + 30);
     }
    }
   }
   break;

  case AEA_ATTENTION:
   g->add_msg_if_player(p,"You feel like your action has attracted attention.");
   p->add_disease("attention", 600 * rng(1, 3));
   break;

  case AEA_TELEGLOW:
   g->add_msg_if_player(p,"You feel unhinged.");
   p->add_disease("teleglow", 100 * rng(3, 12));
   break;

  case AEA_NOISE:
   g->add_msg_if_player(p,"Your %s emits a deafening boom!", it->tname().c_str());
   g->sound(p->posx, p->posy, 100, "");
   break;

  case AEA_SCREAM:
   g->add_msg_if_player(p,"Your %s screams disturbingly.", it->tname().c_str());
   g->sound(p->posx, p->posy, 40, "");
   p->add_morale(MORALE_SCREAM, -10, 0, 300, 5);
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
    // We have to access the actual it_tool class to figure out how many charges this thing uses.
    // Because the charges have already been removed in player::use, we will have to refund the charges if the user cancels.
    // This is a stupid hack, but it's the only way short of rewriting all of the iuse:: functions to draw their own charges as needed.

    it_tool *tool = dynamic_cast<it_tool*>(it->type);
    int charges_per_use = tool->charges_per_use;

    if ( it->type->id ==  "permanent_marker"  )
    {
        int ret=menu(true, "Write on what?", "The ground", "An item", "cancel", NULL );

        if (ret == 2 )
        {
            // inscribe_item returns false if the action fails or is canceled somehow.
            bool canceled_inscription = !inscribe_item( g, p, "Write", "Written", false );
            if( canceled_inscription )
            {
                //Refund the charges, because the inscription was never made.
                it->charges += charges_per_use;
            }
            return;
        }
        else if ( ret != 1) // User chose cancel or some other undefined key.
        {
            //Refund the charges, because the player canceled the action.
            it->charges += charges_per_use;
            return;
        }
    }

    std::string verb=( it->type->id     ==  "permanent_marker" ? "Write" : "Spray" );
    std::string lcverb=( it->type->id   ==  "permanent_marker" ? "write" : "spray" );

    std::string message = string_input_popup(verb + " what?");

    if(message.empty())
    {
        //Refund the charges, because the player canceled the action.
        it->charges += charges_per_use;
    }
    else
    {
        if(g->m.add_graffiti(g, p->posx, p->posy, message))
        {
            g->add_msg("You %s a message on the ground.",lcverb.c_str());
        }
        else
        {
            g->add_msg("You fail to %s a message here.",lcverb.c_str());

            // Refuned the charges, because the grafitti failed.
            it->charges += charges_per_use;
        }
    }
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
	itype_id ujfood = (it->type->id).substr(4);  // assumes "jar_" is at front of itype_id and removes it
	item ujitem(g->itypes[ujfood],0);  // temp create item to discover container
	itype_id ujcont = (dynamic_cast<it_comest*>(ujitem.type))->container;  //discovering container
	it->make(g->itypes[ujcont]);  //turning "sealed jar of xxx" into container for "xxx"
    it->contents.push_back(item(g->itypes[ujfood],0));  //shoving the "xxx" into the container
    it->contents[0].bday = g->turn + 3600 - (g->turn % 3600);
}

void iuse::devac(game *g, player *p, item *it, bool t)
{
	g->add_msg_if_player(p,"You open the vacuum pack, exposing it to the atmosphere.");
	itype_id uvfood = (it->type->id).substr(4);  // assumes "bag_" is at front of itype_id and removes it
	item uvitem(g->itypes[uvfood],0);  // temp create item to discover container
	itype_id uvcont = (dynamic_cast<it_comest*>(uvitem.type))->container;  //discovering container
	it->make(g->itypes[uvcont]);  //turning "vacuum packed xxx" into container for "xxx"
    it->contents.push_back(item(g->itypes[uvfood],0));  //shoving the "xxx" into the container
    it->contents[0].bday = g->turn + 3600 - (g->turn % 3600);
}

void iuse::rad_badge(game *g, player *p, item *it, bool t)
{
    g->add_msg_if_player(p,"You remove the badge from its wrapper, exposing it to ambient radiation.");
    it->make(g->itypes["rad_badge"]);
}

void iuse::boots(game *g, player *p, item *it, bool t)
{
 int choice = -1;
 if (it->contents.size() == 0)
  choice = menu(true, "Using boots:", "Put a knife in the boot", "Cancel", NULL);
 else if (it->contents.size() == 1)
  choice = menu(true, "Take what:", it->contents[0].tname().c_str(), "Put a knife in the boot", "Cancel", NULL);
 else
  choice = menu(true, "Take what:", it->contents[0].tname().c_str(), it->contents[1].tname().c_str(), "Cancel", NULL);

 if ((it->contents.size() > 0 && choice == 1) || // Pull 1st
     (it->contents.size() > 1 && choice == 2)) {  // Pull 2nd
  p->moves -= 15;
  item knife = it->contents[choice - 1];
  if (!p->is_armed() || p->wield(g, -3)) {
   p->i_add(knife);
   p->wield(g, knife.invlet);
   it->contents.erase(it->contents.begin() + choice - 1);
  }
 } else if ((it->contents.size() == 0 && choice == 1) || // Put 1st
            (it->contents.size() == 1 && choice == 2)) { // Put 2st
  char ch = g->inv_type("Put what?", IC_TOOL);
  item* put = &(p->i_at(ch));
  if (put == NULL || put->is_null()) {
   g->add_msg_if_player(p, "You do not have that item!");
   return;
  }
  if (put->type->use != &iuse::knife) {
   g->add_msg_if_player(p, "That isn't knife!");
   return;
  }
  if (put->type->volume > 5) {
   g->add_msg_if_player(p, "That item does not fit in your boot!");
   return;
  }
  p->moves -= 30;
  g->add_msg_if_player(p, "You put the %s in your boot.", put->tname().c_str());
  it->put_in(p->i_rem(ch));
 }
}

void iuse::towel(game *g, player *p, item *it, bool t)
{
    // check if player is wet
    if( abs(p->has_morale(MORALE_WET)) )
    {
        p->rem_morale(MORALE_WET);
        g->add_msg_if_player(p,"You use the %s to dry off!", it->name.c_str());
    }
    else
    {
        g->add_msg_if_player(p,"You are already dry, %s has no effect", it->name.c_str());
    }
}

void iuse::unfold_bicycle(game *g, player *p, item *it, bool t)
{
    vehicle *bicycle = g->m.add_vehicle( g, veh_bicycle, p->posx, p->posy, 0, 0, 0);
    if( bicycle ) {
        // Mark the vehicle as foldable.
        bicycle->tags.insert("convertible");
        // Restore HP of parts if we stashed them previously.
        if( it->item_vars.count("folding_bicycle_parts") ) {
            std::istringstream part_hps;
            part_hps.str(it->item_vars["folding_bicycle_parts"]);
            for (int p = 0; p < bicycle->parts.size(); p++)
            {
                part_hps >> bicycle->parts[p].hp;
            }
        }
        g->add_msg_if_player(p, "You painstakingly unfold the bicycle and make it ready to ride.");
        p->moves -= 500;
        it->invlet = 0;
    } else {
        g->add_msg_if_player(p, "There's no room to unfold the bicycle.");
    }
}
