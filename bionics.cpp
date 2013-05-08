#include "player.h"
#include "game.h"
#include "rng.h"
#include "input.h"
#include "keypress.h"
#include "item.h"
#include "bionics.h"
#include "line.h"

#define BATTERY_AMOUNT 4 // How much batteries increase your power

std::map<bionic_id, bionic_data*> bionics;
std::vector<bionic_id> faulty_bionics;
std::vector<bionic_id> power_source_bionics;
std::vector<bionic_id> unpowered_bionics;

void bionics_install_failure(game *g, player *u, int success);

bionic_data::bionic_data(std::string new_name, bool new_power_source, bool new_activated,
                          int new_power_cost, int new_charge_time, std::string new_description, bool new_faulty){
   name = new_name;
   power_source = new_power_source;
   activated = new_activated;
   power_cost = new_power_cost;
   charge_time = new_charge_time;
   description = new_description;
   faulty = new_faulty;
}

bionic_id game::random_good_bionic() const
{
    std::map<std::string,bionic_data*>::const_iterator random_bionic;
    do
    {
        random_bionic = bionics.begin();
        std::advance(random_bionic,rng(0,bionics.size()-1));
    } while (random_bionic->first == "bio_null" || random_bionic->second->faulty);
    return random_bionic->first;
}

// helper function for power_bionics
void show_power_level_in_titlebar(WINDOW* window, player* p)
{
    mvwprintz(window, 1, 62, c_white, "Power: %d/%d", p->power_level, p->max_power_level);
}

void player::power_bionics(game *g)
{
 WINDOW* wBio = newwin(25, 80, (TERMY > 25) ? (TERMY-25)/2 : 0, (TERMX > 80) ? (TERMX-80)/2 : 0);
 WINDOW* w_description = newwin(3, 78, 21 + getbegy(wBio), 1 + getbegx(wBio));

 werase(wBio);
 wborder(wBio, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
               LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );

 std::vector <bionic> passive;
 std::vector <bionic> active;
 mvwprintz(wBio, 1,  1, c_blue, "BIONICS -");
 mvwprintz(wBio, 1, 11, c_white, "Activating.  Press '!' to examine your implants.");
 show_power_level_in_titlebar(wBio, this);

 for (int i = 1; i < 79; i++) {
  mvwputch(wBio,  2, i, c_ltgray, LINE_OXOX);
  mvwputch(wBio, 20, i, c_ltgray, LINE_OXOX);
 }

 mvwputch(wBio, 2,  0, c_ltgray, LINE_XXXO); // |-
 mvwputch(wBio, 2, 79, c_ltgray, LINE_XOXX); // -|

 mvwputch(wBio, 20,  0, c_ltgray, LINE_XXXO); // |-
 mvwputch(wBio, 20, 79, c_ltgray, LINE_XOXX); // -|

 for (int i = 0; i < my_bionics.size(); i++) {
  if ( bionics[my_bionics[i].id]->power_source ||
      !bionics[my_bionics[i].id]->activated      )
   passive.push_back(my_bionics[i]);
  else
   active.push_back(my_bionics[i]);
 }
 nc_color type;
 if (passive.size() > 0) {
  mvwprintz(wBio, 3, 1, c_ltblue, "Passive:");
  for (int i = 0; i < passive.size(); i++) {
   if (bionics[passive[i].id]->power_source)
    type = c_ltcyan;
   else
    type = c_cyan;
   mvwputch(wBio, 4 + i, 1, type, passive[i].invlet);
   mvwprintz(wBio, 4 + i, 3, type, bionics[passive[i].id]->name.c_str());
  }
 }
 if (active.size() > 0) {
  mvwprintz(wBio, 3, 33, c_ltblue, "Active:");
  for (int i = 0; i < active.size(); i++) {
   if (active[i].powered)
    type = c_red;
   else
    type = c_ltred;
   mvwputch(wBio, 4 + i, 33, type, active[i].invlet);
   mvwprintz(wBio, 4 + i, 35, type,
             (active[i].powered ? "%s - ON" : "%s - %d PU / %d trns"),
             bionics[active[i].id]->name.c_str(),
             bionics[active[i].id]->power_cost,
             bionics[active[i].id]->charge_time);
  }
 }
 wrefresh(wBio);
 char ch;
 bool activating = true;
 bionic *tmp = NULL;
 int b = -1;
 do {
  ch = getch();
  if (ch == '!') {
   activating = !activating;
   if (activating)
    mvwprintz(wBio, 1, 11, c_white, "Activating.  Press '!' to examine your implants.");
   else
    mvwprintz(wBio, 1, 11, c_white, "Examining.  Press '!' to activate your implants.");
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
   if (ch == KEY_ESCAPE && tmp != NULL) {
    if (activating) {
     if (bionics[tmp->id]->activated) {
      show_power_level_in_titlebar(wBio, this);
      itype_id weapon_id = weapon.type->id;
      if (tmp->powered) {
       tmp->powered = false;
       g->add_msg("%s powered off.", bionics[tmp->id]->name.c_str());
      } else if (b >= 0 && (power_level >= bionics[tmp->id]->power_cost ||
                 (weapon_id == "bio_claws_weapon" && tmp->id == "bio_claws_weapon")))
       activate_bionic(b, g);
     } else
      mvwprintz(wBio, 21, 1, c_ltred, "\
You can not activate %s!  To read a description of \
%s, press '!', then '%c'.", bionics[tmp->id]->name.c_str(),
                            bionics[tmp->id]->name.c_str(), tmp->invlet);
    } else {	// Describing bionics, not activating them!
// Clear the lines first
     ch = 0;
     werase(w_description);
     mvwprintz(w_description, 0, 0, c_ltblue, bionics[tmp->id]->description.c_str());
    }
   }
  }
  wrefresh(w_description);
  wrefresh(wBio);
 } while (ch != KEY_ESCAPE);
 werase(wBio);
 wrefresh(wBio);
 delwin(wBio);
 erase();
}

// TODO: clean up liquid handling
void pour_water_from_bionics(game *g, player *p, int power_cost)
{
    char invlet = g->inv("Choose a container:");
    if (p->i_at(invlet).type == 0)
    {
        g->add_msg("You don't have that item!");
        p->power_level += power_cost;
    }
    else if (!p->i_at(invlet).is_container())
    {
        g->add_msg("That %s isn't a container!", p->i_at(invlet).tname().c_str());
        p->power_level += power_cost;
    }
    else
    {
        it_container *cont = dynamic_cast<it_container*>(p->i_at(invlet).type);
        if (p->i_at(invlet).volume_contained() + 1 > cont->contains)
        {
            g->add_msg("There's no space left in your %s.", p->i_at(invlet).tname().c_str());
            p->power_level += power_cost;
        }
        else if (!(cont->flags & con_wtight))
        {
            g->add_msg("Your %s isn't watertight!", p->i_at(invlet).tname().c_str());
            p->power_level += power_cost;
        }
        else
        {
            g->add_msg("You pour water into your %s.", p->i_at(invlet).tname().c_str());
            p->i_at(invlet).put_in(item(g->itypes["water_clean"], 0));
        }
    }
}

void bio_activate_painkiller(player *p)
{
    p->pkill += 6;
    p->pain -= 2;
    if (p->pkill > p->pain)
    {
       p->pkill = p->pain;
    }
}

void bio_activate_resonator(game *g, player *p)
{
    std::string junk;
    g->sound(p->posx, p->posy, 30, "VRRRRMP!");
    for (int i = p->posx - 1; i <= p->posx + 1; i++)
    {
       for (int j = p->posy - 1; j <= p->posy + 1; j++)
       {
           g->m.bash(i, j, 40, junk);
           g->m.bash(i, j, 40, junk); // Multibash effect, so that doors &c will fall
           g->m.bash(i, j, 40, junk);
           if (g->m.is_destructable(i, j) && rng(1, 10) >= 4)
           {
               g->m.ter_set(i, j, t_rubble);
           }
       }
    }
}

void bio_activate_time_freeze(game *g, player *p)
{
    p->moves += 100 * p->power_level;
    p->power_level = 0;
    g->add_msg("Your speed suddenly increases!");
    if (one_in(3))
    {
        g->add_msg("Your muscles tear with the strain.");
        p->hurt(g, bp_arms, 0, rng(5, 10));
        p->hurt(g, bp_arms, 1, rng(5, 10));
        p->hurt(g, bp_legs, 0, rng(7, 12));
        p->hurt(g, bp_legs, 1, rng(7, 12));
        p->hurt(g, bp_torso, 0, rng(5, 15));
    }
    if (one_in(5))
    {
       p->add_disease(DI_TELEGLOW, rng(50, 400), g);
    }
}

// TODO: make sure that blood analysis and blood filter are up-to-date
void bio_activate_blood_anal(game *g, player *p)
{
    std::vector<std::string> good;
    std::vector<std::string> bad;
    WINDOW *w = newwin(20, 40, 3 + ((TERMY > 25) ? (TERMY-25)/2 : 0), 10+((TERMX > 80) ? (TERMX-80)/2 : 0));
    wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
               LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
    // Soron: okay, maybe strict allman has issues. So. Many. Braces.
    // then again, we could also handle this with a map or something...
    if (p->has_disease(DI_FUNGUS))
    {
        bad.push_back("Fungal Parasite");
    }
    if (p->has_disease(DI_DERMATIK))
    {
        bad.push_back("Insect Parasite");
    }
    if (p->has_disease(DI_POISON))
    {
        bad.push_back("Poison");
    }
    if (p->radiation > 0)
    {
        bad.push_back("Irradiated");
    }
    if (p->has_disease(DI_PKILL1))
    {
        good.push_back("Minor Painkiller");
    }
    if (p->has_disease(DI_PKILL2))
    {
        good.push_back("Moderate Painkiller");
    }
    if (p->has_disease(DI_PKILL3))
    {
        good.push_back("Heavy Painkiller");
    }
    if (p->has_disease(DI_PKILL_L))
    {
        good.push_back("Slow-Release Painkiller");
    }
    if (p->has_disease(DI_DRUNK))
    {
        good.push_back("Alcohol");
    }
    if (p->has_disease(DI_CIG))
    {
        good.push_back("Nicotine");
    }
    if (p->has_disease(DI_HIGH))
    {
        good.push_back("Intoxicant: Other");
    }
    if (p->has_disease(DI_TOOK_PROZAC))
    {
        good.push_back("Prozac");
    }
    if (p->has_disease(DI_TOOK_FLUMED))
    {
        good.push_back("Antihistamines");
    }
    if (p->has_disease(DI_ADRENALINE))
    {
        good.push_back("Adrenaline Spike");
    }

    if (good.size() == 0 && bad.size() == 0)
    {
        mvwprintz(w, 1, 1, c_white, "No effects.");
    }
    else
    {
        for (int line = 1; line < 39 && line <= good.size() + bad.size(); line++)
        {
            if (line <= bad.size())
            {
                mvwprintz(w, line, 1, c_red, bad[line - 1].c_str());
            }
            else
            {
                mvwprintz(w, line, 1, c_green, good[line - 1 - bad.size()].c_str());
            }
        }
    }
    wrefresh(w);
    refresh();
    getch();
    delwin(w);
}

void bio_activate_blood_filter(player *p)
{
    p->rem_disease(DI_FUNGUS);
    p->rem_disease(DI_POISON);
    p->rem_disease(DI_PKILL1);
    p->rem_disease(DI_PKILL2);
    p->rem_disease(DI_PKILL3);
    p->rem_disease(DI_PKILL_L);
    p->rem_disease(DI_DRUNK);
    p->rem_disease(DI_CIG);
    p->rem_disease(DI_HIGH);
    p->rem_disease(DI_TOOK_PROZAC);
    p->rem_disease(DI_TOOK_FLUMED);
    p->rem_disease(DI_ADRENALINE);
}

void bio_activate_evaporator(game *g, player *p)
{
    item tmp_item;
    if (query_yn("Drink directly? Otherwise you will need a container."))
    {
        tmp_item = item(g->itypes["water_clean"], 0);
        p->thirst -= 50;
        if (p->has_trait(PF_GOURMAND) && p->thirst < -60)
        {
            g->add_msg("You can't finish it all!");
            p->thirst = -60;
        }
        else if (!p->has_trait(PF_GOURMAND) && p->thirst < -20)
        {
            g->add_msg("You can't finish it all!");
            p->thirst = -20;
        }
    }
    else
    {
        pour_water_from_bionics(g, p, bionics["bio_evap"]->power_cost);
    }
}

void bio_activate_lighter(game *g, player *p)
{
    int dirx, diry;
    g->draw();
    mvprintw(0, 0, "Torch in which direction?");
    InputEvent input = get_input();
    get_direction(dirx, diry, input);
    if (dirx == -2)
    {
        g->add_msg("Invalid direction.");
        p->power_level += bionics["bio_lighter"]->power_cost;
        return;
    }
    dirx += p->posx;
    diry += p->posy;
    if (!g->m.add_field(g, dirx, diry, fd_fire, 1))	// Unsuccessful.
    {
        g->add_msg("You can't light a fire there.");
        p->power_level += bionics["bio_lighter"]->power_cost;
    }
}

void bio_activate_claws(game *g, player *p)
{
    if (p->weapon.type->id == "bio_claws_weapon")
    {
        g->add_msg("You withdraw your claws.");
        p->weapon = p->ret_null;
    }
    else if (p->weapon.type->id != "null")
    {
        g->add_msg("Your claws extend, forcing you to drop your %s.",
                   p->weapon.tname().c_str());
        g->m.add_item(p->posx, p->posy, p->weapon);
        p->weapon = item(g->itypes["bio_claws_weapon"], 0);
        p->weapon.invlet = '#';
    }
    else
    {
        g->add_msg("Your claws extend!");
        p->weapon = item(g->itypes["bio_claws_weapon"], 0);
        p->weapon.invlet = '#';
    }
}

void bio_activate_shoot(game *g, player *p, itype_id gun_id, itype_id ammo_id)
{
    item tmp_item = p->weapon;
    p->weapon = item(g->itypes[gun_id], 0);
    p->weapon.curammo = dynamic_cast<it_ammo*>(g->itypes[ammo_id]);
    p->weapon.charges = 1;
    g->refresh_all();
    g->plfire(false);
    p->weapon = tmp_item;
}

void bio_activate_emp(game *g, player *p)
{
    int dirx, diry;
    g->draw();
    mvprintw(0, 0, "Fire EMP in which direction?");
    InputEvent input = get_input();
    get_direction(dirx, diry, input);
    if (dirx == -2)
    {
        g->add_msg("Invalid direction.");
        p->power_level += bionics["bio_emp"]->power_cost;
        return;
    }
    dirx += p->posx;
    diry += p->posy;
    g->emp_blast(dirx, diry);
}

void bio_activate_water_extractor(game *g, player *p)
{
    for (int i = 0; i < g->m.i_at(p->posx, p->posy).size(); ++i )
    {
        item tmp = g->m.i_at(p->posx, p->posy)[i];
        if (tmp.type->id == "corpse" && query_yn("Extract water from the %s",
                                                  tmp.tname().c_str()))
        {
            i = g->m.i_at(p->posx, p->posy).size() + 1;	// Loop is finished
            pour_water_from_bionics(g, p, bionics["bio_water_extractor"]->power_cost);
        }
        if (i == g->m.i_at(p->posx, p->posy).size() - 1)	// We never chose a corpse
        {
            p->power_level += bionics["bio_water_extractor"]->power_cost;
        }
    }
}

void bio_activate_magnet(game *g, player *p)
{
    std::string junk;
    std::vector<point> traj;
    int t;
    for (int i = p->posx - 10; i <= p->posx + 10; i++)
    {
        for (int j = p->posy - 10; j <= p->posy + 10; j++)
        {
            if (g->m.i_at(i, j).size() > 0)
            {
                if (g->m.sees(i, j, p->posx, p->posy, -1, t))
                {
                    traj = line_to(i, j, p->posx, p->posy, t);
                }
                else
                {
                    traj = line_to(i, j, p->posx, p->posy, 0);
                }
            }
            traj.insert(traj.begin(), point(i, j));
            for (int k = 0; k < g->m.i_at(i, j).size(); k++)
            {
                if (g->m.i_at(i, j)[k].made_of(IRON) || g->m.i_at(i, j)[k].made_of(STEEL)){
                    item tmp_item = g->m.i_at(i, j)[k];
                    g->m.i_rem(i, j, k);
	                int l;
                    for (l = 0; l < traj.size(); l++)
                    {
                        int index = g->mon_at(traj[l].x, traj[l].y);
                        if (index != -1)
                        {
                            if (g->z[index].hurt(tmp_item.weight() * 2))
                            {
                                g->kill_mon(index, true);
                            }
                            g->m.add_item(traj[l].x, traj[l].y, tmp_item);
                            l = traj.size() + 1;
                        }
                        else if (l > 0 && g->m.move_cost(traj[l].x, traj[l].y) == 0)
                        {
                            g->m.bash(traj[l].x, traj[l].y, tmp_item.weight() * 2, junk);
                            g->sound(traj[l].x, traj[l].y, 12, junk);
                            if (g->m.move_cost(traj[l].x, traj[l].y) == 0)
                            {
                                g->m.add_item(traj[l - 1].x, traj[l - 1].y, tmp_item);
                                l = traj.size() + 1;
                            }
                        }
                    }
                    if (l == traj.size())
                    {
                        g->m.add_item(p->posx, p->posy, tmp_item);
                    }
                }
            }
        }
    }
}

void bio_activate_lockpick(game *g, player *p)
{
    int dirx, diry;
    g->draw();
    mvprintw(0, 0, "Unlock in which direction?");
    InputEvent input = get_input();
    get_direction(dirx, diry, input);
    if (dirx == -2)
    {
        g->add_msg("Invalid direction.");
        p->power_level += bionics["bio_lockpick"]->power_cost;
        return;
    }
    dirx += p->posx;
    diry += p->posy;
    if (g->m.ter(dirx, diry) == t_door_locked)
    {
        p->moves -= 40;
        g->add_msg("You unlock the door.");
        g->m.ter_set(dirx, diry, t_door_c);
    }
    else
    {
        g->add_msg("You can't unlock that %s.", g->m.tername(dirx, diry).c_str());
        p->power_level += bionics["bio_lockpick"]->power_cost;
    }
}

// Why put this in a Big Switch?  Why not let bionics have pointers to
// functions, much like monsters and items?
//
// Well, because like diseases, which are also in a Big Switch, bionics don't
// share functions....
void player::activate_bionic(int b, game *g)
{
 bionic bio = my_bionics[b];
 int power_cost = bionics[bio.id]->power_cost;
 if (weapon.type->id == "bio_claws_weapon" && bio.id == "bio_claws_weapon")
  power_cost = 0;
 if (power_level < power_cost) {
  if (my_bionics[b].powered) {
   g->add_msg("Your %s powers down.", bionics[bio.id]->name.c_str());
   my_bionics[b].powered = false;
  } else
   g->add_msg("You cannot power your %s", bionics[bio.id]->name.c_str());
  return;
 }

 if (my_bionics[b].powered && my_bionics[b].charge > 0) {
// Already-on units just lose a bit of charge
  my_bionics[b].charge--;
 } else {
// Not-on units, or those with zero charge, have to pay the power cost
  if (bionics[bio.id]->charge_time > 0) {
   my_bionics[b].powered = true;
   my_bionics[b].charge = bionics[bio.id]->charge_time;
  }
  power_level -= power_cost;
 }

 if(bio.id == "bio_painkiller"){
  bio_activate_painkiller(this);
 } else if (bio.id == "bio_nanobots"){
  healall(4);
 } else if (bio.id == "bio_resonator"){
  bio_activate_resonator(g, this);
 } else if (bio.id == "bio_time_freeze"){
  bio_activate_time_freeze(g, this);
 } else if (bio.id == "bio_teleport"){
  g->teleport();
  add_disease(DI_TELEGLOW, 300, g);
 }
 else if(bio.id == "bio_blood_anal"){
  bio_activate_blood_anal(g, this);
 } else if(bio.id == "bio_blood_filter"){
  bio_activate_blood_filter(this);
 } else if(bio.id == "bio_evap"){
  bio_activate_evaporator(g, this);
 } else if(bio.id == "bio_lighter"){
  bio_activate_lighter(g, this);
 } else if(bio.id == "bio_claws"){
  bio_activate_claws(g, this);
 } else if(bio.id == "bio_blaster"){
  bio_activate_shoot(g, this, "bio_blaster_gun", "bio_fusion_ammo");
 } else if (bio.id == "bio_laser"){
  bio_activate_shoot(g, this, "v29", "laser_pack");
 } else if (bio.id == "bio_emp"){
  bio_activate_emp(g, this);
 } else if (bio.id == "bio_hydraulics"){
  g->add_msg("Your muscles hiss as hydraulic strength fills them!"); // does this do anything?
 } else if (bio.id == "bio_water_extractor"){
  bio_activate_water_extractor(g, this);
 } else if(bio.id == "bio_magnet"){
  bio_activate_magnet(g, this);
 } else if(bio.id == "bio_lockpick"){
  bio_activate_lockpick(g, this);
 }
}

bool player::install_bionics(game *g, it_bionic* type)
{
 if (type == NULL) {
  debugmsg("Tried to install NULL bionic");
  return false;
 }
 std::string bio_name = type->name.substr(5);	// Strip off "CBM: "

 WINDOW* w = newwin(25, 80, (TERMY > 25) ? (TERMY-25)/2 : 0, (TERMX > 80) ? (TERMX-80)/2 : 0);
 WINDOW* w_description = newwin(3, 78, 21 + getbegy(w), 1 + getbegx(w));

 werase(w);
 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );

 int pl_skill = int_cur +
   skillLevel("electronics") * 4 +
   skillLevel("firstaid")    * 3 +
   skillLevel("mechanics")   * 2;

 int skint = int(pl_skill / 4);
 int skdec = int((pl_skill * 10) / 4) % 10;

// Header text
 mvwprintz(w, 1,  1, c_white, "Installing bionics:");
 mvwprintz(w, 1, 21, type->color, bio_name.c_str());

// Dividing bars
 for (int i = 1; i < 79; i++) {
  mvwputch(w,  2, i, c_ltgray, LINE_OXOX);
  mvwputch(w, 20, i, c_ltgray, LINE_OXOX);
 }

 mvwputch(w, 2,  0, c_ltgray, LINE_XXXO); // |-
 mvwputch(w, 2, 79, c_ltgray, LINE_XOXX); // -|

 mvwputch(w, 20,  0, c_ltgray, LINE_XXXO); // |-
 mvwputch(w, 20, 79, c_ltgray, LINE_XOXX); // -|

// Init the list of bionics
 for (int i = 1; i < type->options.size(); i++) {
  bionic_id bio_id = type->options[i];
  mvwprintz(w, i + 3, 1, (has_bionic(bio_id) ? c_ltred : c_ltblue),
            bionics[bio_id]->name.c_str());
 }
// Helper text
 mvwprintz(w, 3, 39, c_white,        "Difficulty of this module: %d",
           type->difficulty);
 mvwprintz(w, 4, 39, c_white,        "Your installation skill:   %d.%d",
           skint, skdec);
 mvwprintz(w, 5, 39, c_white,       "Installation requires high intelligence,");
 mvwprintz(w, 6, 39, c_white,       "and skill in electronics, first aid, and");
 mvwprintz(w, 7, 39, c_white,       "mechanics (in that order of importance).");

 int chance_of_success = int((100 * pl_skill) /
                             (pl_skill + 4 * type->difficulty));

 mvwprintz(w, 9, 39, c_white,        "Chance of success:");

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

 mvwprintz(w, 9, 59, col_suc, "%d%%%%", chance_of_success);

 mvwprintz(w, 11, 39, c_white,       "Failure may result in crippling damage,");
 mvwprintz(w, 12, 39, c_white,       "loss of existing bionics, genetic damage");
 mvwprintz(w, 13, 39, c_white,       "or faulty installation.");
 wrefresh(w);

 if (type->id == "bio_power_storage" || type->id == "bio_power_storage_mkII") { // No selection list; just confirm
   int pow_up = BATTERY_AMOUNT;

   if (type->id == "bio_power_storage_mkII") {
     pow_up = 10;
   }

  mvwprintz(w, 3, 1, h_ltblue, "Power Storage +%d", pow_up);
  mvwprintz(w_description, 0, 0, c_ltblue, "\
Installing this bionic will increase your total power storage by %d.\n\
Power is necessary for most bionics to function. You also require a\n\
charge mechanism, which must be installed from another CBM.", pow_up);

  InputEvent input;
  wrefresh(w_description);
  wrefresh(w);
  do
   input = get_input();
  while (input != Confirm && input != Cancel);
  if (input == Confirm) {
   practice(g->turn, "electronics", (100 - chance_of_success) * 1.5);
   practice(g->turn, "firstaid", (100 - chance_of_success) * 1.0);
   practice(g->turn, "mechanics", (100 - chance_of_success) * 0.5);
   int success = chance_of_success - rng(1, 100);
   if (success > 0) {
    g->add_msg("Successfully installed batteries.");
    max_power_level += pow_up;
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
 InputEvent input;

 do {

  bionic_id bio_id = type->options[selection];
  mvwprintz(w, 3 + selection, 1, (has_bionic(bio_id) ? h_ltred : h_ltblue),
            bionics[bio_id]->name.c_str());

// Clear the bottom three lines...
  werase(w_description);
// ...and then fill them with the description of the selected bionic
  mvwprintz(w_description, 0, 0, c_ltblue, bionics[bio_id]->description.c_str());

  wrefresh(w_description);
  wrefresh(w);
  input = get_input();
  switch (input) {

  case DirectionS:
   mvwprintz(w, 2 + selection, 0, (has_bionic(bio_id) ? c_ltred : c_ltblue),
             bionics[bio_id]->name.c_str());
   if (selection == type->options.size() - 1)
    selection = 0;
   else
    selection++;
   break;

  case DirectionN:
   mvwprintz(w, 2 + selection, 0, (has_bionic(bio_id) ? c_ltred : c_ltblue),
             bionics[bio_id]->name.c_str());
   if (selection == 0)
    selection = type->options.size() - 1;
   else
    selection--;
   break;

  }
  if (input == Confirm && has_bionic(bio_id)) {
   popup("You already have a %s!", bionics[bio_id]->name.c_str());
   input = Nothing;
  }
 } while (input != Cancel && input != Confirm);

 if (input == Confirm) {
   practice(g->turn, "electronics", (100 - chance_of_success) * 1.5);
   practice(g->turn, "firstaid", (100 - chance_of_success) * 1.0);
   practice(g->turn, "mechanics", (100 - chance_of_success) * 0.5);
  bionic_id bio_id = type->options[selection];
  int success = chance_of_success - rng(1, 100);
  if (success > 0) {
   g->add_msg("Successfully installed %s.", bionics[bio_id]->name.c_str());
   add_bionic(bio_id);
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
  // no need for a break, because we've returned already

 case 5:
 {
  fail_text += ", causing a faulty installation.";
  std::vector<bionic_id> valid;
  for (std::vector<std::string>::iterator it = faulty_bionics.begin() ; it != faulty_bionics.end(); ++it){
   if (!u->has_bionic(*it)){
    valid.push_back(*it);
   }
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

 // not required in the case of mutation, because we emitted the message already
 g->add_msg(fail_text.c_str());

}

void game::init_bionics(){
    bionics["bio_null"] = new bionic_data("NULL bionics", false, false, 0, 0, "\
If you're seeing this, it's a bug. (game::init_bionics)");
    // NAME          ,PW_SRC, ACT ,COST, TIME,
    bionics["bio_batteries"] = new bionic_data("Battery System", true, false, 0, 0, "\
You have a battery draining attachment, and thus can make use of the energy\n\
contained in normal, everyday batteries.  Use 'E' to consume batteries.");
    power_source_bionics.push_back("bio_batteries");
    bionics["bio_metabolics"] = new bionic_data("Metabolic Interchange", true, false, 0, 0, "\
Your digestive system and power supply are interconnected.  Any drain on\n\
energy instead increases your hunger.");
    power_source_bionics.push_back("bio_metabolics");
    bionics["bio_solar"] = new bionic_data("Solar Panels", true, false, 0, 0, "\
You have a few solar panels installed.  While in direct sunlight, your power\n\
level will slowly recharge.");
    power_source_bionics.push_back("bio_solar");
    bionics["bio_torsionratchet"] = new bionic_data("Joint Torsion Ratchet", true, false, 0, 0, "\
Your joints have been equipped with torsion ratchets that generate power slowly\n\
when you move.");
    power_source_bionics.push_back("bio_torsionratchet");
    bionics["bio_furnace"] = new bionic_data("Internal Furnace", true, false, 0, 0, "\
You can burn nearly any organic material as fuel (use 'E'), recharging your\n\
power level.  Some materials will burn better than others.");
    power_source_bionics.push_back("bio_furnace");
    bionics["bio_ethanol"] = new bionic_data("Ethanol Burner", true, false, 0, 0, "\
You burn alcohol as fuel in an extremely efficient reaction.  However, you\n\
will still suffer the inebriating effects of the substance.");
    power_source_bionics.push_back("bio_ethanol");
    bionics["bio_memory"] = new bionic_data("Enhanced Memory Banks", false, false, 1, 0, "\
Your memory has been enhanced with small quantum storage drives.  Any time\n\
you start to forget a skill, you have a chance at retaining all knowledge, at\n\
the cost of a small amount of poweron.");
    unpowered_bionics.push_back("bio_memory");
    bionics["bio_ears"] = new bionic_data("Enhanced Hearing", false, false, 0, 0, "\
Your hearing has been drastically improved, allowing you to hear ten times\n\
better than the average person.  Additionally, high-intensity sounds will be\n\
automatically dampened before they can damage your hearing.");
    unpowered_bionics.push_back("bio_ears");

    bionics["bio_eye_enhancer"] = new bionic_data("Diamond Cornea", false, false, 0, 0, "\
Your vision is greatly enhanced, giving you a +2 bonus to perception.");
    unpowered_bionics.push_back("bio_eye_enhancer");
    bionics["bio_membrane"] = new bionic_data("Nictating Membrane", false, false, 0, 0, "\
Your eyes have a thin membrane that closes over your eyes while underwater,\n\
negating any vision penalties.");
    unpowered_bionics.push_back("bio_membrane");
    bionics["bio_targeting"] = new bionic_data("Targeting System", false, false, 0, 0, "\
Your eyes are equipped with range finders, and their movement is synced with\n\
that of your arms, to a degree.  Shots you fire will be much more accurate,\n\
particularly at long range.");
    unpowered_bionics.push_back("bio_targeting");
    bionics["bio_gills"] = new bionic_data("Membrane Oxygenator", false, false, 1, 0, "\
An oxygen interchange system automatically switches on while underwater,\n\
slowly draining your energy reserves but providing oxygen.");
    unpowered_bionics.push_back("bio_gills");
    bionics["bio_purifier"] = new bionic_data("Air Filtration System", false, false, 1, 0, "\
Implanted in your trachea is an advanced filtration system.  If toxins find\n\
their way into your windpipe, the filter will attempt to remove them.");
    unpowered_bionics.push_back("bio_purifier");
    bionics["bio_climate"] = new bionic_data("Internal Climate Control", false, true, 1, 30, "\
Throughout your body lies a network of thermal piping which eases the effects\n\
of high and low ambient temperatures.");
    unpowered_bionics.push_back("bio_climate");

    bionics["bio_storage"] = new bionic_data("Internal Storage", false, false, 0, 0, "\
Space inside your chest cavity has been converted into a storage area.  You\n\
may carry an extra 8 units of volume.");
    unpowered_bionics.push_back("bio_storage");
    bionics["bio_recycler"] = new bionic_data("Recycler Unit", false, false, 0, 0, "\
Your digestive system has been outfitted with a series of filters and\n\
processors, allowing you to reclaim waste liquid and, to a lesser degree,\n\
nutrients.  The net effect is a greatly reduced need to eat and drink.");
    unpowered_bionics.push_back("bio_recycler");
    bionics["bio_digestion"] = new bionic_data("Expanded Digestive System", false, false, 0, 0, "\
You have been outfitted with three synthetic stomachs and industrial-grade\n\
intestines.  Not only can you extract much more nutrition from food, but you\n\
are highly resistant to foodborne illness, and can sometimes eat rotten food.");
    unpowered_bionics.push_back("bio_digestion");
    bionics["bio_tools"] = new bionic_data("Integrated Toolset", false, false, 0, 0, "\
Implanted in your hands and fingers is a complete tool set - screwdriver,\n\
hammer, wrench, and heating elements.  You can use this in place of many\n\
tools when crafting.");
    unpowered_bionics.push_back("bio_tools");
    bionics["bio_shock"] = new bionic_data("Electroshock Unit", false, false, 1, 0, "\
While fighting unarmed, or with a weapon that conducts electricity, there is\n\
a chance that a successful hit will shock your opponent, inflicting extra\n\
damage and disabling them temporarily at the cost of some energy.");
    unpowered_bionics.push_back("bio_shock");
    bionics["bio_heat_absorb"] = new bionic_data("Heat Drain", false, false, 1, 0, "\
While fighting unarmed against a warm-blooded opponent, there is a chance\n\
that a successful hit will drain body heat, inflicting a small amount of\n\
extra damage, and increasing your power reserves slightly.");
    unpowered_bionics.push_back("bio_heat_absorb");
    bionics["bio_carbon"] = new bionic_data("Subdermal Carbon Filament", false, false, 0, 0, "\
Lying just beneath your skin is a thin armor made of carbon nanotubes. This\n\
reduces bashing damage by 2 and cutting damage by 4.");
    unpowered_bionics.push_back("bio_carbon");
    bionics["bio_armor_head"] = new bionic_data("Alloy Plating - Head", false, false, 0, 0, "\
The flesh on your head has been replaced by a strong armor, protecting both\n\
your head and jaw regions.");
    unpowered_bionics.push_back("bio_armor_head");
    bionics["bio_armor_torso"] = new bionic_data("Alloy Plating - Torso", false, false, 0, 0, "\
The flesh on your torso has been replaced by a strong armor, protecting you\n\
greatly.");
    unpowered_bionics.push_back("bio_armor_torso");
    bionics["bio_armor_arms"] = new bionic_data("Alloy Plating - Arms", false, false, 0, 0, "\
The flesh on your arms has been replaced by a strong armor, protecting you\n\
greatly.");
    unpowered_bionics.push_back("bio_armor_arms");
    bionics["bio_armor_legs"] = new bionic_data("Alloy Plating - Legs", false, false, 0, 0, "\
The flesh on your legs has been replaced by a strong armor, protecting you\n\
greatly.");
    unpowered_bionics.push_back("bio_armor_legs");

    bionics["bio_flashlight"] = new bionic_data("Cranial Flashlight", false, true, 1, 30, "\
Mounted between your eyes is a small but powerful LED flashlight.");
    bionics["bio_night_vision"] = new bionic_data("Implanted Night Vision", false, true, 1, 20, "\
Your eyes have been modified to amplify existing light, allowing you to see\n\
in the dark.");
    bionics["bio_infrared"] = new bionic_data("Infrared Vision", false, true, 1, 4, "\
Your range of vision extends into the infrared, allowing you to see warm-\n\
blooded creatures in the dark, and even through walls.");
    bionics["bio_face_mask"] = new bionic_data("Facial Distortion", false, true, 1, 10, "\
Your face is actually made of a compound which may be molded by electrical\n\
impulses, making you impossible to recognize.  While not powered, however,\n\
the compound reverts to its default shape.");
    bionics["bio_ads"] = new bionic_data("Active Defense System", false, true, 1, 7, "\
A thin forcefield surrounds your body, continually draining power.  Anything\n\
attempting to penetrate this field has a chance of being deflected at the\n\
cost of more energy.  Melee attacks will be stopped more often than bullets.");
    bionics["bio_ods"] = new bionic_data("Offensive Defense System", false, true, 1, 6, "\
A thin forcefield surrounds your body, continually draining power.  This\n\
field does not deflect penetration, but rather delivers a very strong shock,\n\
damaging unarmed attackers and those with a conductive weapon.");
    bionics["bio_scent_mask"] = new bionic_data("Olfactory Mask", false, true, 1, 8, "\
While this system is powered, your body will produce very little odor, making\n\
it nearly impossible for creatures to track you by scent.");

    bionics["bio_scent_vision"] = new bionic_data("Scent Vision", false, true, 1, 30, "\
While this system is powered, you're able to visually sense your own scent,\n\
making it possible for you to recognize your surroundings even if you can't\n\
see it.");
    bionics["bio_cloak"] = new bionic_data("Cloaking System", false, true, 2, 1, "\
This high-power system uses a set of cameras and LEDs to make you blend into\n\
your background, rendering you fully invisible to normal vision.  However,\n\
you may be detected by infrared, sonar, etc.");
    bionics["bio_painkiller"] = new bionic_data("Sensory Dulling", false, true, 2, 0, "\
Your nervous system is wired to allow you to inhibit the signals of pain,\n\
allowing you to dull your senses at will.  However, the use of this system\n\
may cause delayed reaction time and drowsiness.");
    bionics["bio_nanobots"] = new bionic_data("Repair Nanobots", false, true, 5, 0, "\
Inside your body is a fleet of tiny dormant robots.  Once charged from your\n\
energy banks, they will flit about your body, repairing any damage.");
    bionics["bio_heatsink"] = new bionic_data("Thermal Dissapation", false, true, 1, 6, "\
Powerful heatsinks supermaterials are woven into your flesh.  While powered,\n\
this system will prevent heat damage up to 2000 degrees fahrenheit.  Note\n\
that this does not affect your internal temperature.");
    bionics["bio_resonator"] = new bionic_data("Sonic Resonator", false, true, 4, 0, "\
Your entire body may resonate at very high power, creating a short-range\n\
shockwave.  While it will not to much damage to flexible creatures, stiff\n\
items such as walls, doors, and even robots will be severely damaged.");

    bionics["bio_time_freeze"] = new bionic_data("Time Dilation", false, true, 3, 0, "\
At an immense cost of power, you may increase your body speed and reactions\n\
dramatically, essentially freezing time.  You are still delicate, however,\n\
and violent or rapid movements may damage you due to friction.");
    bionics["bio_teleport"] = new bionic_data("Teleportation Unit", false, true, 10, 0, "\
This highly experimental unit folds space over short distances, instantly\n\
transporting your body up to 25 feet at the cost of much power.  Note that\n\
prolonged or frequent use may have dangerous side effects.");
    bionics["bio_blood_anal"] = new bionic_data("Blood Analysis", false, true, 1, 0, "\
Small sensors have been implanted in your heart, allowing you to analyse your\n\
blood.  This will detect many illnesses, drugs, and other conditions.");
    bionics["bio_blood_filter"] = new bionic_data("Blood Filter", false, true, 3, 0, "\
A filtration system in your heart allows you to actively filter out chemical\n\
impurities, primarily drugs.  It will have limited impact on viruses.  Note\n\
that it is not a targeted filter; ALL drugs in your system will be affected.");
    bionics["bio_alarm"] = new bionic_data("Alarm System", false, true, 1, 400, "\
A motion-detecting alarm system will notice almost all movement within a\n\
fifteen-foot radius, and will silently alert you.  This is very useful during\n\
sleep, or if you suspect a cloaked pursuer.");
    bionics["bio_evap"] = new bionic_data("Aero-Evaporator", false, true, 8, 0, "\
This unit draws moisture from the surrounding air, which then is poured from\n\
a fingertip in the form of water.  It may fail in very dry environments.");
    bionics["bio_lighter"] = new bionic_data("Mini-Flamethrower", false, true, 3, 0, "\
The index fingers of both hands have powerful fire starters which extend from\n\
the tip.");
    bionics["bio_claws"] = new bionic_data("Adamantium Claws", false, true, 3, 0, "\
Your fingers can withdraw into your hands, allowing a set of vicious claws to\n\
extend.  These do considerable cutting damage, but prevent you from holding\n\
anything else.");

    bionics["bio_blaster"] = new bionic_data("Fusion Blaster Arm", false, true, 2, 0, "\
Your left arm has been replaced by a heavy-duty fusion blaster!  You may use\n\
your energy banks to fire a damaging heat ray; however, you are unable to use\n\
or carry two-handed items, and may only fire handguns.");
    bionics["bio_laser"] = new bionic_data("Finger-Mounted Laser", false, true, 2, 0, "\
One of your fingers has a small high-powered laser embedded in it.  This long\n\
range weapon is not incredibly damaging, but is very accurate, and has the\n\
potential to start fires.");
    bionics["bio_emp"] = new bionic_data("Directional EMP", false, true, 4, 0, "\
Mounted in the palms of your hand are small parabolic EMP field generators.\n\
You may use power to fire a short-ranged blast which will disable electronics\n\
and robots.");
    bionics["bio_hydraulics"] = new bionic_data("Hydraulic Muscles", false, true, 1, 3, "\
While activated, the muscles in your arms will be greatly enchanced,\n\
increasing your strength by 20.");
    bionics["bio_water_extractor"] = new bionic_data("Water Extraction Unit", false, true, 2, 0, "\
Nanotubes embedded in the palm of your hand will pump any available fluid out\n\
of a dead body, cleanse it of impurities and convert it into drinkable water.\n\
You must, however, have a container to store the water in.");
    bionics["bio_magnet"] = new bionic_data("Electromagnetic Unit", false, true, 2, 0, "\
Embedded in your hand is a powerful electromagnet, allowing you to pull items\n\
made of iron over short distances.");
    bionics["bio_fingerhack"] = new bionic_data("Fingerhack", false, true, 1, 0, "\
One of your fingers has an electrohack embedded in it; an all-purpose hacking\n\
unit used to override control panels and the like (but not computers).  Skill\n\
in computers is important, and a failed use may damage your circuits.");
    bionics["bio_lockpick"] = new bionic_data("Fingerpick", false, true, 1, 0, "\
One of your fingers has an electronic lockpick embedded in it.  This auto-\n\
matic system will quickly unlock all but the most advanced key locks without\n\
any skill required on the part of the user.");

    bionics["bio_ground_sonar"] = new bionic_data("Terranian Sonar", false, true, 1, 5, "\
Your feet are equipped with precision sonar equipment, allowing you to detect\n\
the movements of creatures below the ground.");

    bionics["bio_power_armor_interface"] = new bionic_data("Power Armor Interface", false, true, 1, 10, "\
Interfaces your power system with the internal charging port on suits of power armor.");

    //Fault Bionics from here on out.
    bionics["bio_dis_shock"] = new bionic_data("Electrical Discharge", false, false, 0, 0, "\
A malfunctioning bionic which occasionally discharges electricity through\n\
your body, causing pain and brief paralysis but no damage.");
    faulty_bionics.push_back("bio_dis_shock");

    bionics["bio_dis_acid"] = new bionic_data("Acidic Discharge", false, false, 0, 0, "\
A malfunctioning bionic which occasionally discharges acid into your muscles,\n\
causing sharp pain and minor damage.");
    faulty_bionics.push_back("bio_dis_shock");

    bionics["bio_drain"] = new bionic_data("Electrical Drain", false, false, 0, 0, "\
A malfunctioning bionic.  It doesn't perform any useful function, but will\n\
occasionally draw power from your batteries.");
    faulty_bionics.push_back("bio_drain");

    bionics["bio_noise"] = new bionic_data("Noisemaker", false, false, 0, 0, "\
A malfunctioning bionic.  It will occasionally emit a loud burst of noise.");
    faulty_bionics.push_back("bio_noise");
    bionics["bio_power_weakness"] = new bionic_data("Power Overload", false, false, 0, 0, "\
Damaged power circuits cause short-circuiting inside your muscles when your\n\
batteries are above 75%%%% capacity, causing greatly reduced strength.  This\n\
has no effect if you have no internal batteries.");
    faulty_bionics.push_back("bio_power_weakness");

    bionics["bio_stiff"] = new bionic_data("Wire-induced Stiffness", false, false, 0, 0, "\
Improperly installed wires cause a physical stiffness in most of your body,\n\
causing increased encumberance.");
    faulty_bionics.push_back("bio_stiff");
}

