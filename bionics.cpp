#include "player.h"
#include "game.h"
#include "rng.h"
#include "keypress.h"
#include "item.h"
#include "bionics.h"
#include "line.h"

#define BATTERY_AMOUNT 4 // How much batteries increase your power

void bionics_install_failure(game *g, player *u, int success);


// Why put this in a Big Switch?  Why not let bionics have pointers to
// functions, much like monsters and items?
//
// Well, because like diseases, which are also in a Big Switch, bionics don't
// share functions....
void player::activate_bionic(int b, game *g)
{
 bionic bio = my_bionics[b];
 int power_cost = bionics[bio.id].power_cost;
 if (weapon.type->id == itm_bio_claws && bio.id == bio_claws)
  power_cost = 0;
 if (power_level < power_cost) {
  if (my_bionics[b].powered) {
   g->add_msg("Your %s powers down.", bionics[bio.id].name.c_str());
   my_bionics[b].powered = false;
  } else
   g->add_msg("You cannot power your %s", bionics[bio.id].name.c_str());
  return;
 }

 if (my_bionics[b].powered && my_bionics[b].charge > 0) {
// Already-on units just lose a bit of charge
  my_bionics[b].charge--;
 } else {
// Not-on units, or those with zero charge, have to pay the power cost
  if (bionics[bio.id].charge_time > 0) {
   my_bionics[b].powered = true;
   my_bionics[b].charge = bionics[bio.id].charge_time;
  }
  power_level -= power_cost;
 }

 std::string junk;
 std::vector<point> traj;
 std::vector<std::string> good;
 std::vector<std::string> bad;
 WINDOW* w;
 int dirx, diry, t, l, index;
 item tmp_item;

 switch (bio.id) {

 case bio_painkiller:
  pkill += 6;
  pain -= 2;
  if (pkill > pain)
   pkill = pain;
  break;

 case bio_nanobots:
  healall(4);
  break;

 case bio_resonator:
  g->sound(posx, posy, 30, "VRRRRMP!");
  for (int i = posx - 1; i <= posx + 1; i++) {
   for (int j = posy - 1; j <= posy + 1; j++) {
    g->m.bash(i, j, 40, junk);
    g->m.bash(i, j, 40, junk);	// Multibash effect, so that doors &c will fall
    g->m.bash(i, j, 40, junk);
    if (g->m.is_destructable(i, j) && rng(1, 10) >= 4)
     g->m.ter(i, j) = t_rubble;
   }
  }
  break;

 case bio_time_freeze:
  moves += 100 * power_level;
  power_level = 0;
  g->add_msg("Your speed suddenly increases!");
  if (one_in(3)) {
   g->add_msg("Your muscles tear with the strain.");
   hurt(g, bp_arms, 0, rng(5, 10));
   hurt(g, bp_arms, 1, rng(5, 10));
   hurt(g, bp_legs, 0, rng(7, 12));
   hurt(g, bp_legs, 1, rng(7, 12));
   hurt(g, bp_torso, 0, rng(5, 15));
  }
  if (one_in(5))
   add_disease(DI_TELEGLOW, rng(50, 400), g);
  break;

 case bio_teleport:
  g->teleport();
  add_disease(DI_TELEGLOW, 300, g);
  break;

// TODO: More stuff here (and bio_blood_filter)
 case bio_blood_anal:
  w = newwin(20, 40, 3, 10);
  wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
             LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
  if (has_disease(DI_FUNGUS))
   bad.push_back("Fungal Parasite");
  if (has_disease(DI_DERMATIK))
   bad.push_back("Insect Parasite");
  if (has_disease(DI_POISON))
   bad.push_back("Poison");
  if (radiation > 0)
   bad.push_back("Irradiated");
  if (has_disease(DI_PKILL1))
   good.push_back("Minor Painkiller");
  if (has_disease(DI_PKILL2))
   good.push_back("Moderate Painkiller");
  if (has_disease(DI_PKILL3))
   good.push_back("Heavy Painkiller");
  if (has_disease(DI_PKILL_L))
   good.push_back("Slow-Release Painkiller");
  if (has_disease(DI_DRUNK))
   good.push_back("Alcohol");
  if (has_disease(DI_CIG))
   good.push_back("Nicotine");
  if (has_disease(DI_HIGH))
   good.push_back("Intoxicant: Other");
  if (has_disease(DI_TOOK_PROZAC))
   good.push_back("Prozac");
  if (has_disease(DI_TOOK_FLUMED))
   good.push_back("Antihistamines");
  if (has_disease(DI_ADRENALINE))
   good.push_back("Adrenaline Spike");
  if (good.size() == 0 && bad.size() == 0)
   mvwprintz(w, 1, 1, c_white, "No effects.");
  else {
   for (int line = 1; line < 39 && line <= good.size() + bad.size(); line++) {
    if (line <= bad.size())
     mvwprintz(w, line, 1, c_red, bad[line - 1].c_str());
    else
     mvwprintz(w, line, 1, c_green, good[line - 1 - bad.size()].c_str());
   }
  }
  wrefresh(w);
  refresh();
  getch();
  delwin(w);
  break;

 case bio_blood_filter:
  rem_disease(DI_FUNGUS);
  rem_disease(DI_POISON);
  rem_disease(DI_PKILL1);
  rem_disease(DI_PKILL2);
  rem_disease(DI_PKILL3);
  rem_disease(DI_PKILL_L);
  rem_disease(DI_DRUNK);
  rem_disease(DI_CIG);
  rem_disease(DI_HIGH);
  rem_disease(DI_TOOK_PROZAC);
  rem_disease(DI_TOOK_FLUMED);
  rem_disease(DI_ADRENALINE);
  break;

 case bio_evap:
  if (query_yn("Drink directly? Otherwise you will need a container.")) {
   tmp_item = item(g->itypes[itm_water], 0);
   thirst -= 50;
   if (has_trait(PF_GOURMAND) && thirst < -60) {
     g->add_msg("You can't finish it all!");
     thirst = -60;
   } else if (!has_trait(PF_GOURMAND) && thirst < -20) {
     g->add_msg("You can't finish it all!");
     thirst = -20;
   }
  } else {
   t = g->inv("Choose a container:");
   if (i_at(t).type == 0) {
    g->add_msg("You don't have that item!");
    power_level += bionics[bio_evap].power_cost;
   } else if (!i_at(t).is_container()) {
    g->add_msg("That %s isn't a container!", i_at(t).tname().c_str());
    power_level += bionics[bio_evap].power_cost;
   } else {
    it_container *cont = dynamic_cast<it_container*>(i_at(t).type);
    if (i_at(t).volume_contained() + 1 > cont->contains) {
     g->add_msg("There's no space left in your %s.", i_at(t).tname().c_str());
     power_level += bionics[bio_evap].power_cost;
    } else if (!(cont->flags & con_wtight)) {
     g->add_msg("Your %s isn't watertight!", i_at(t).tname().c_str());
     power_level += bionics[bio_evap].power_cost;
    } else {
     g->add_msg("You pour water into your %s.", i_at(t).tname().c_str());
     i_at(t).put_in(item(g->itypes[itm_water], 0));
    }
   }
  }
  break;

 case bio_lighter:
  g->draw();
  mvprintw(0, 0, "Torch in which direction?");
  get_direction(g, dirx, diry, input());
  if (dirx == -2) {
   g->add_msg("Invalid direction.");
   power_level += bionics[bio_lighter].power_cost;
   return;
  }
  dirx += posx;
  diry += posy;
  if (!g->m.add_field(g, dirx, diry, fd_fire, 1))	// Unsuccessful.
   g->add_msg("You can't light a fire there.");
  break;

 case bio_claws:
  if (weapon.type->id == itm_bio_claws) {
   g->add_msg("You withdraw your claws.");
   weapon = ret_null;
  } else if (weapon.type->id != 0) {
   g->add_msg("Your claws extend, forcing you to drop your %s.",
              weapon.tname().c_str());
   g->m.add_item(posx, posy, weapon);
   weapon = item(g->itypes[itm_bio_claws], 0);
   weapon.invlet = '#';
  } else {
   g->add_msg("Your claws extend!");
   weapon = item(g->itypes[itm_bio_claws], 0);
   weapon.invlet = '#';
  }
  break;

 case bio_blaster:
  tmp_item = weapon;
  weapon = item(g->itypes[itm_bio_blaster], 0);
  weapon.curammo = dynamic_cast<it_ammo*>(g->itypes[itm_bio_fusion]);
  weapon.charges = 1;
  g->refresh_all();
  g->plfire(false);
  weapon = tmp_item;
  break;

 case bio_laser:
  tmp_item = weapon;
  weapon = item(g->itypes[itm_v29], 0);
  weapon.curammo = dynamic_cast<it_ammo*>(g->itypes[itm_laser_pack]);
  weapon.charges = 1;
  g->refresh_all();
  g->plfire(false);
  weapon = tmp_item;
  break;

 case bio_emp:
  g->draw();
  mvprintw(0, 0, "Fire EMP in which direction?");
  get_direction(g, dirx, diry, input());
  if (dirx == -2) {
   g->add_msg("Invalid direction.");
   power_level += bionics[bio_emp].power_cost;
   return;
  }
  dirx += posx;
  diry += posy;
  g->emp_blast(dirx, diry);
  break;

 case bio_hydraulics:
  g->add_msg("Your muscles hiss as hydraulic strength fills them!");
  break;

 case bio_water_extractor:
  for (int i = 0; i < g->m.i_at(posx, posy).size(); i++) {
   item tmp = g->m.i_at(posx, posy)[i];
   if (tmp.type->id == itm_corpse && query_yn("Extract water from the %s",
                                              tmp.tname().c_str())) {
    i = g->m.i_at(posx, posy).size() + 1;	// Loop is finished
    t = g->inv("Choose a container:");
    if (i_at(t).type == 0) {
     g->add_msg("You don't have that item!");
     power_level += bionics[bio_water_extractor].power_cost;
    } else if (!i_at(t).is_container()) {
     g->add_msg("That %s isn't a container!", i_at(t).tname().c_str());
     power_level += bionics[bio_water_extractor].power_cost;
    } else {
     it_container *cont = dynamic_cast<it_container*>(i_at(t).type);
     if (i_at(t).volume_contained() + 1 > cont->contains) {
      g->add_msg("There's no space left in your %s.", i_at(t).tname().c_str());
      power_level += bionics[bio_water_extractor].power_cost;
     } else {
      g->add_msg("You pour water into your %s.", i_at(t).tname().c_str());
      i_at(t).put_in(item(g->itypes[itm_water], 0));
     }
    }
   }
   if (i == g->m.i_at(posx, posy).size() - 1)	// We never chose a corpse
    power_level += bionics[bio_water_extractor].power_cost;
  }
  break;

 case bio_magnet:
  for (int i = posx - 10; i <= posx + 10; i++) {
   for (int j = posy - 10; j <= posy + 10; j++) {
    if (g->m.i_at(i, j).size() > 0) {
     if (g->m.sees(i, j, posx, posy, -1, t))
      traj = line_to(i, j, posx, posy, t);
     else
      traj = line_to(i, j, posx, posy, 0);
    }
    traj.insert(traj.begin(), point(i, j));
    for (int k = 0; k < g->m.i_at(i, j).size(); k++) {
     if (g->m.i_at(i, j)[k].made_of(IRON) || g->m.i_at(i, j)[k].made_of(STEEL)){
      tmp_item = g->m.i_at(i, j)[k];
      g->m.i_rem(i, j, k);
      for (l = 0; l < traj.size(); l++) {
       index = g->mon_at(traj[l].x, traj[l].y);
       if (index != -1) {
        if (g->z[index].hurt(tmp_item.weight() * 2))
         g->kill_mon(index, true);
        g->m.add_item(traj[l].x, traj[l].y, tmp_item);
        l = traj.size() + 1;
       } else if (l > 0 && g->m.move_cost(traj[l].x, traj[l].y) == 0) {
        g->m.bash(traj[l].x, traj[l].y, tmp_item.weight() * 2, junk);
        g->sound(traj[l].x, traj[l].y, 12, junk);
        if (g->m.move_cost(traj[l].x, traj[l].y) == 0) {
         g->m.add_item(traj[l - 1].x, traj[l - 1].y, tmp_item);
         l = traj.size() + 1;
        }
       }
      }
      if (l == traj.size())
       g->m.add_item(posx, posy, tmp_item);
     }
    }
   }
  }
  break;

 case bio_lockpick:
  g->draw();
  mvprintw(0, 0, "Unlock in which direction?");
  get_direction(g, dirx, diry, input());
  if (dirx == -2) {
   g->add_msg("Invalid direction.");
   power_level += bionics[bio_lockpick].power_cost;
   return;
  }
  dirx += posx;
  diry += posy;
  if (g->m.ter(dirx, diry) == t_door_locked) {
   moves -= 40;
   g->add_msg("You unlock the door.");
   g->m.ter(dirx, diry) = t_door_c;
  } else
   g->add_msg("You can't unlock that %s.", g->m.tername(dirx, diry).c_str());
  break;

 }
}

bool player::install_bionics(game *g, it_bionic* type)
{
 if (type == NULL) {
  debugmsg("Tried to install NULL bionic");
  return false;
 }
 std::string bio_name = type->name.substr(5);	// Strip off "CBM: "
 WINDOW* w = newwin(25, 80, 0, 0);

 int pl_skill = int_cur + sklevel[sk_electronics] * 4 +
                          sklevel[sk_firstaid]    * 3 +
                          sklevel[sk_mechanics]   * 2;

 int skint = int(pl_skill / 4);
 int skdec = int((pl_skill * 10) / 4) % 10;

// Header text
 mvwprintz(w, 0,  0, c_white, "Installing bionics:");
 mvwprintz(w, 0, 20, type->color, bio_name.c_str());

// Dividing bars
 for (int i = 0; i < 80; i++) {
  mvwputch(w,  1, i, c_ltgray, LINE_OXOX);
  mvwputch(w, 21, i, c_ltgray, LINE_OXOX);
 }
// Init the list of bionics
 for (int i = 1; i < type->options.size(); i++) {
  bionic_id id = type->options[i];
  mvwprintz(w, i + 2, 0, (has_bionic(id) ? c_ltred : c_ltblue),
            bionics[id].name.c_str());
 }
// Helper text
 mvwprintz(w, 2, 40, c_white,        "Difficulty of this module: %d",
           type->difficulty);
 mvwprintz(w, 3, 40, c_white,        "Your installation skill:   %d.%d",
           skint, skdec);
 mvwprintz(w, 4, 40, c_white,       "Installation requires high intelligence,");
 mvwprintz(w, 5, 40, c_white,       "and skill in electronics, first aid, and");
 mvwprintz(w, 6, 40, c_white,       "mechanics (in that order of importance).");

 int chance_of_success = int((100 * pl_skill) /
                             (pl_skill + 4 * type->difficulty));

 mvwprintz(w, 8, 40, c_white,        "Chance of success:");

 nc_color col_suc;
 if (chance_of_success >= 95)
  col_suc = c_green;
 else if (chance_of_success >= 80)
  col_suc = c_ltgreen;
 else if (chance_of_success >= 60)
  col_suc = c_yellow;
 else if (chance_of_success >= 35)
  col_suc = c_ltred;
 else
  col_suc = c_red;

 mvwprintz(w, 8, 59, col_suc, "%d%%%%", chance_of_success);

 mvwprintz(w, 10, 40, c_white,       "Failure may result in crippling damage,");
 mvwprintz(w, 11, 40, c_white,       "loss of existing bionics, genetic damage");
 mvwprintz(w, 12, 40, c_white,       "or faulty installation.");
 wrefresh(w);

 if (type->id == itm_bionics_battery) {	// No selection list; just confirm
  mvwprintz(w,  2, 0, h_ltblue, "Battery Level +%d", BATTERY_AMOUNT);
  mvwprintz(w, 22, 0, c_ltblue, "\
Installing this bionic will increase your total battery capacity by 10.\n\
Batteries are necessary for most bionics to function.  They also require a\n\
charge mechanism, which must be installed from another CBM.");
  char ch;
  wrefresh(w);
  do
   ch = getch();
  while (ch != 'q' && ch != '\n' && ch != KEY_ESCAPE);
  if (ch == '\n') {
   practice(sk_electronics, (100 - chance_of_success) * 1.5);
   practice(sk_firstaid, (100 - chance_of_success) * 1.0);
   practice(sk_mechanics, (100 - chance_of_success) * 0.5);
   int success = chance_of_success - rng(1, 100);
   if (success > 0) {
    g->add_msg("Successfully installed batteries.");
    max_power_level += BATTERY_AMOUNT;
   } else
    bionics_install_failure(g, this, success);
   werase(w);
   delwin(w);
   g->refresh_all();
   return true;
  }
  werase(w);
  delwin(w);
  g->refresh_all();
  return false;
 }

 int selection = 0;
 char ch;

 do {

  bionic_id id = type->options[selection];
  mvwprintz(w, 2 + selection, 0, (has_bionic(id) ? h_ltred : h_ltblue),
            bionics[id].name.c_str());

// Clear the bottom three lines...
  mvwprintz(w, 22, 0, c_ltgray, "\
                                                                             \n\
                                                                             \n\
                                                                             ");
// ...and then fill them with the description of the selected bionic
  mvwprintz(w, 22, 0, c_ltblue, bionics[id].description.c_str());

  wrefresh(w);
  ch = input();
  switch (ch) {

  case 'j':
   mvwprintz(w, 2 + selection, 0, (has_bionic(id) ? c_ltred : c_ltblue),
             bionics[id].name.c_str());
   if (selection == type->options.size() - 1)
    selection = 0;
   else
    selection++;
   break;

  case 'k':
   mvwprintz(w, 2 + selection, 0, (has_bionic(id) ? c_ltred : c_ltblue),
             bionics[id].name.c_str());
   if (selection == 0)
    selection = type->options.size() - 1;
   else
    selection--;
   break;

  }
  if (ch == '\n' && has_bionic(id)) {
   popup("You already have a %s!", bionics[id].name.c_str());
   ch = 'a';
  }
 } while (ch != '\n' && ch != 'q' && ch != KEY_ESCAPE);

 if (ch == '\n') {
  practice(sk_electronics, (100 - chance_of_success) * 1.5);
  practice(sk_firstaid, (100 - chance_of_success) * 1.0);
  practice(sk_mechanics, (100 - chance_of_success) * 0.5);
  bionic_id id = type->options[selection];
  int success = chance_of_success - rng(1, 100);
  if (success > 0) {
   g->add_msg("Successfully installed %s.", bionics[id].name.c_str());
   add_bionic(id);
  } else
   bionics_install_failure(g, this, success);
  werase(w);
  delwin(w);
  g->refresh_all();
  return true;
 }
 werase(w);
 delwin(w);
 g->refresh_all();
 return false;
}

void bionics_install_failure(game *g, player *u, int success)
{
 success = abs(success) - rng(1, 10);
 int failure_level = 0;
 if (success <= 0) {
  g->add_msg("The installation fails without incident.");
  return;
 }

 while (success > 0) {
  failure_level++;
  success -= rng(1, 10);
 }

 int fail_type = rng(1, (failure_level > 5 ? 5 : failure_level));
 std::string fail_text;

 switch (rng(1, 5)) {
  case 1: fail_text = "You flub the installation";	break;
  case 2: fail_text = "You mess up the installation";	break;
  case 3: fail_text = "The installation fails";		break;
  case 4: fail_text = "The installation is a failure";	break;
  case 5: fail_text = "You screw up the installation";	break;
 }

 if (fail_type == 3 && u->my_bionics.size() == 0)
  fail_type = 2; // If we have no bionics, take damage instead of losing some

 switch (fail_type) {

 case 1:
  fail_text += ", causing great pain.";
  u->pain += rng(failure_level * 3, failure_level * 6);
  break;
 
 case 2:
  fail_text += " and your body is damaged.";
  u->hurtall(rng(failure_level, failure_level * 2));
  break;

 case 3:
  fail_text += " and ";
  fail_text += (u->my_bionics.size() <= failure_level ? "all" : "some");
  fail_text += " of your existing bionics are lost.";
  for (int i = 0; i < failure_level && u->my_bionics.size() > 0; i++) {
   int rem = rng(0, u->my_bionics.size() - 1);
   u->my_bionics.erase(u->my_bionics.begin() + rem);
  }
  break;

 case 4:
  fail_text += " and do damage to your genetics, causing mutation.";
  g->add_msg(fail_text.c_str()); // Failure text comes BEFORE mutation text
  while (failure_level > 0) {
   u->mutate(g);
   failure_level -= rng(1, failure_level + 2);
  }
  return;	// So the failure text doesn't show up twice
  break;

 case 5:
 {
  fail_text += ", causing a faulty installation.";
  std::vector<bionic_id> valid;
  for (int i = max_bio_good + 1; i < max_bio; i++) {
   bionic_id id = bionic_id(i);
   if (!u->has_bionic(id))
    valid.push_back(id);
  }
  if (valid.size() == 0) {	// We've got all the bad bionics!
   if (u->max_power_level > 0) {
    g->add_msg("You lose power capacity!");
    u->max_power_level = rng(0, u->max_power_level - 1);
   }
// TODO: What if we can't lose power capacity?  No penalty?
  } else {
   int index = rng(0, valid.size() - 1);
   u->add_bionic(valid[index]);
  }
 }
  break;
 }

 g->add_msg(fail_text.c_str());

}
