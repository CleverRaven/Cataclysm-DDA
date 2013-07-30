#include <vector>
#include <string>
#include "game.h"
#include "keypress.h"
#include "output.h"
#include "line.h"
#include "skill.h"
#include "rng.h"
#include "item.h"
#include "options.h"

int time_to_fire(player &p, it_gun* firing);
int recoil_add(player &p);
void make_gun_sound_effect(game *g, player &p, bool burst, item* weapon);
int calculate_range(player &p, int tarx, int tary);
double calculate_missed_by(player &p, int trange, item* weapon);
void shoot_monster(game *g, player &p, monster &mon, int &dam, double goodhit, item* weapon);
void shoot_player(game *g, player &p, player *h, int &dam, double goodhit);

void splatter(game *g, std::vector<point> trajectory, int dam,
              monster* mon = NULL);

void ammo_effects(game *g, int x, int y, const std::set<std::string> &effects);

void game::fire(player &p, int tarx, int tary, std::vector<point> &trajectory,
                bool burst)
{
 item ammotmp;
 item* gunmod = p.weapon.active_gunmod();
 it_ammo *curammo = NULL;
 item *weapon = NULL;

 if (p.weapon.has_flag("CHARGE")) { // It's a charger gun, so make up a type
// Charges maxes out at 8.
  int charges = p.weapon.num_charges();
  it_ammo *tmpammo = dynamic_cast<it_ammo*>(itypes["charge_shot"]);

  tmpammo->damage = charges * charges;
  tmpammo->pierce = (charges >= 4 ? (charges - 3) * 2.5 : 0);
  tmpammo->range = 5 + charges * 5;
  if (charges <= 4)
   tmpammo->dispersion = 14 - charges * 2;
  else // 5, 12, 21, 32
   tmpammo->dispersion = charges * (charges - 4);
  tmpammo->recoil = tmpammo->dispersion * .8;
  if (charges == 8) { tmpammo->ammo_effects.insert("EXPLOSIVE_BIG"); }
  else if (charges >= 6) { tmpammo->ammo_effects.insert("EXPLOSIVE"); }

  if (charges >= 5){ tmpammo->ammo_effects.insert("FLAME"); }
  else if (charges >= 4) { tmpammo->ammo_effects.insert("INCENDIARY"); }

  if (gunmod != NULL) {
   weapon = gunmod;
  } else {
   weapon = &p.weapon;
  }
  curammo = tmpammo;
  weapon->curammo = tmpammo;
  weapon->active = false;
  weapon->charges = 0;
 } else if (gunmod != NULL) {
  weapon = gunmod;
  curammo = weapon->curammo;
 } else {// Just a normal gun. If we're here, we know curammo is valid.
  curammo = p.weapon.curammo;
  weapon = &p.weapon;
 }

 ammotmp = item(curammo, 0);
 ammotmp.charges = 1;

 if (!weapon->is_gun() && !weapon->is_gunmod()) {
  debugmsg("%s tried to fire a non-gun (%s).", p.name.c_str(),
                                               weapon->tname().c_str());
  return;
 }

 bool is_bolt = false;
 std::set<std::string> *effects = &curammo->ammo_effects;
 // Bolts and arrows are silent
 if (curammo->type == "bolt" || curammo->type == "arrow")
  is_bolt = true;

 int x = p.posx, y = p.posy;
 // Have to use the gun, gunmods don't have a type
 it_gun* firing = dynamic_cast<it_gun*>(p.weapon.type);
 if (p.has_trait(PF_TRIGGERHAPPY) && one_in(30))
  burst = true;
 if (burst && weapon->burst_size() < 2)
  burst = false; // Can't burst fire a semi-auto

// Use different amounts of time depending on the type of gun and our skill
 if (!effects->count("BOUNCE")) {
     p.moves -= time_to_fire(p, firing);
 }
// Decide how many shots to fire
 int num_shots = 1;
 if (burst)
  num_shots = weapon->burst_size();
 if (num_shots > weapon->num_charges() && !weapon->has_flag("CHARGE"))
  num_shots = weapon->num_charges();

 if (num_shots == 0)
  debugmsg("game::fire() - num_shots = 0!");

// Set up a timespec for use in the nanosleep function below
 timespec ts;
 ts.tv_sec = 0;
 ts.tv_nsec = BULLET_SPEED;

 // Use up some ammunition
int trange = rl_dist(p.posx, p.posy, tarx, tary);

 if (trange < int(firing->volume / 3) && firing->ammo != "shot")
  trange = int(firing->volume / 3);
 else if (p.has_bionic("bio_targeting")) {
  if (trange > LONG_RANGE)
   trange = int(trange * .65);
  else
   trange = int(trange * .8);
 }
 if (firing->skill_used == Skill::skill("rifle") && trange > LONG_RANGE)
  trange = LONG_RANGE + .6 * (trange - LONG_RANGE);
 std::string message = "";

 bool missed = false;
 int tart;

 const bool debug_retarget = false;  // this will inevitably be needed
 const bool wildly_spraying = false; // stub for now. later, rng based on stress/skill/etc at the start,
 int weaponrange = p.weapon.range(); // this is expensive, let's cache. todo: figure out if we need p.weapon.range(&p);

 for (int curshot = 0; curshot < num_shots; curshot++) {
 // Burst-fire weapons allow us to pick a new target after killing the first
     if ( curshot > 0 && (mon_at(tarx, tary) == -1 || z[mon_at(tarx, tary)].hp <= 0) ) {
       std::vector<point> new_targets;
       new_targets.clear();

       if ( debug_retarget == true ) {
          mvprintz(curshot,5,c_red,"[%d] %s: retarget: mon_at(%d,%d)",curshot,p.name.c_str(),tarx,tary);
          if(mon_at(tarx, tary) == -1) {
            printz(c_red, " = -1");
          } else {
            printz(c_red, ".hp=%d",
              z[mon_at(tarx, tary)].hp
            );
          }
       }

       for (
         int radius = 0;                        /* range from last target, not shooter! */
         radius <= 2 + p.skillLevel("gun") &&   /* more skill: wider burst area? */
         radius <= weaponrange &&               /* this seems redundant */
         ( new_targets.empty() ||               /* got target? stop looking. However this breaks random selection, aka, wildly spraying, so: */
            wildly_spraying == true );          /* lets set this based on rng && stress or whatever elsewhere */
         radius++
       ) {                                      /* iterate from last target's position: makes sense for burst fire.*/

           for (std::vector<monster>::iterator it = z.begin(); it != z.end(); it++) {
               int nt_range_to_me = rl_dist(p.posx, p.posy, it->posx, it->posy);
               int dummy;
               if (nt_range_to_me == 0 || nt_range_to_me > weaponrange ||
                   !pl_sees(&p, &(*it), dummy)) {
                   /* reject out of range and unseen targets as well as MY FACE */
                   continue;
               }

               int nt_range_to_lt = rl_dist(tarx,tary,it->posx,it->posy);
               /* debug*/ if ( debug_retarget && nt_range_to_lt <= 5 ) printz(c_red, " r:%d/l:%d/m:%d ..", radius, nt_range_to_lt, nt_range_to_me );
               if (nt_range_to_lt != radius) {
                   continue;                    /* we're spiralling outward, catch you next iteration (maybe) */
               }
               if (it->hp >0 && it->friendly == 0) {
                   new_targets.push_back(point(it->posx, it->posy)); /* oh you're not dead and I don't like you. Hello! */
               }
           }
       }
       if ( new_targets.empty() == false ) {    /* new victim! or last victim moved */
          int target_picked = rng(0, new_targets.size() - 1); /* 1 victim list unless wildly spraying */
          tarx = new_targets[target_picked].x;
          tary = new_targets[target_picked].y;
          if (m.sees(p.posx, p.posy, tarx, tary, 0, tart)) {
              trajectory = line_to(p.posx, p.posy, tarx, tary, tart);
          } else {
              trajectory = line_to(p.posx, p.posy, tarx, tary, 0);
          }

          /* debug */ if (debug_retarget) printz(c_ltgreen, " NEW:(%d:%d,%d) %d,%d (%s)[%d] hp: %d",
              target_picked, new_targets[target_picked].x, new_targets[target_picked].y,
              tarx, tary, z[mon_at(tarx, tary)].name().c_str(), mon_at(tarx, tary), z[mon_at(tarx, tary)].hp);

       } else if (
          (
             !p.has_trait(PF_TRIGGERHAPPY) ||   /* double tap. TRIPLE TAP! wait, no... */
             one_in(3)                          /* on second though...everyone double-taps at times. */
          ) && (
             p.skillLevel("gun") >= 7 ||        /* unless trained */
             one_in(7 - p.skillLevel("gun"))    /* ...sometimes */
          ) ) {
          return;                               // No targets, so return
       } else if (debug_retarget) {
          printz(c_red, " new targets.empty()!");
       }
  } else if (debug_retarget) {
    mvprintz(curshot,5,c_red,"[%d] %s: target == mon_at(%d,%d)[%d] %s hp %d",curshot, p.name.c_str(), tarx ,tary,
       mon_at(tarx, tary),
       z[mon_at(tarx, tary)].name().c_str(),
       z[mon_at(tarx, tary)].hp);
  }

  // Drop a shell casing if appropriate.
  itype_id casing_type = "null";
  if( curammo->type == "shot" ) casing_type = "shot_hull";
  else if( curammo->type == "9mm" ) casing_type = "9mm_casing";
  else if( curammo->type == "22" ) casing_type = "22_casing";
  else if( curammo->type == "38" ) casing_type = "38_casing";
  else if( curammo->type == "40" ) casing_type = "40_casing";
  else if( curammo->type == "44" ) casing_type = "44_casing";
  else if( curammo->type == "45" ) casing_type = "45_casing";
  else if( curammo->type == "454" ) casing_type = "454_casing";
  else if( curammo->type == "500" ) casing_type = "500_casing";
  else if( curammo->type == "57" ) casing_type = "57mm_casing";
  else if( curammo->type == "46" ) casing_type = "46mm_casing";
  else if( curammo->type == "762" ) casing_type = "762_casing";
  else if( curammo->type == "223" ) casing_type = "223_casing";
  else if( curammo->type == "3006" ) casing_type = "3006_casing";
  else if( curammo->type == "308" ) casing_type = "308_casing";
  else if( curammo->type == "40mm" ) casing_type = "40mm_casing";

  if (casing_type != "null") {
   item casing;
   casing.make(itypes[casing_type]);
   // Casing needs a charges of 1 to stack properly with other casings.
   casing.charges = 1;
    if( weapon->has_gunmod("brass_catcher") != -1 ) {
        p.i_add( casing );
    } else {
       int x = p.posx - 1 + rng(0, 2);
       int y = p.posy - 1 + rng(0, 2);
       m.add_item_or_charges(x, y, casing);
    }
   }

  // Use up a round (or 100)
  if (weapon->has_flag("FIRE_100"))
   weapon->charges -= 100;
  else
   weapon->charges--;

  if (firing->skill_used != Skill::skill("archery") &&
      firing->skill_used != Skill::skill("throw"))
  {
      // Current guns have a durability between 5 and 9.
      // Misfire chance is between 1/64 and 1/1024.
      if (one_in(2 << firing->durability)) {
          add_msg_player_or_npc( &p, _("Your weapon misfires!"),
                                 _("<npcname>'s weapon misfires!") );
          return;
      }
  }

  make_gun_sound_effect(this, p, burst, weapon);
  int trange = calculate_range(p, tarx, tary);
  double missed_by = calculate_missed_by(p, trange, weapon);
// Calculate a penalty based on the monster's speed
  double monster_speed_penalty = 1.;
  int target_index = mon_at(tarx, tary);
  if (target_index != -1) {
   monster_speed_penalty = double(z[target_index].speed) / 80.;
   if (monster_speed_penalty < 1.)
    monster_speed_penalty = 1.;
  }

  if (curshot > 0) {
   if (recoil_add(p) % 2 == 1)
    p.recoil++;
   p.recoil += recoil_add(p) / 2;
  } else
   p.recoil += recoil_add(p);

  if (missed_by >= 1.) {
// We missed D:
// Shoot a random nearby space?
   int mtarx = tarx + rng(0 - int(sqrt(double(missed_by))), int(sqrt(double(missed_by))));
   int mtary = tary + rng(0 - int(sqrt(double(missed_by))), int(sqrt(double(missed_by))));
   if (m.sees(p.posx, p.posy, x, y, -1, tart))
    trajectory = line_to(p.posx, p.posy, mtarx, mtary, tart);
   else
    trajectory = line_to(p.posx, p.posy, mtarx, mtary, 0);
   missed = true;
   if (!burst) {
       add_msg_player_or_npc( &p, _("You miss!"), _("<npcname> misses!") );
   }
  } else if (missed_by >= .7 / monster_speed_penalty) {
// Hit the space, but not necessarily the monster there
   missed = true;
   if (!burst) {
       add_msg_player_or_npc( &p, _("You barely miss!"), _("<npcname> barely misses!") );
   }
  }

  int dam = weapon->gun_damage();
  int tx = trajectory[0].x;
  int ty = trajectory[0].y;
  int px = trajectory[0].x;
  int py = trajectory[0].y;
  for (int i = 0; i < trajectory.size() &&
         (dam > 0 || (effects->count("FLAME"))); i++) {
      px = tx;
      py = ty;
      tx = trajectory[i].x;
      ty = trajectory[i].y;
// Drawing the bullet uses player u, and not player p, because it's drawn
// relative to YOUR position, which may not be the gunman's position.
   if (u_see(tx, ty)) {
    if (i > 0)
    {
        m.drawsq(w_terrain, u, trajectory[i-1].x, trajectory[i-1].y, false,
                 true, u.posx + u.view_offset_x, u.posy + u.view_offset_y);
    }
    char bullet = '*';
    if (effects->count("FLAME"))
     bullet = '#';
    mvwputch(w_terrain, ty + VIEWY - u.posy - u.view_offset_y,
             tx + VIEWX - u.posx - u.view_offset_x, c_red, bullet);
    wrefresh(w_terrain);
    if (&p == &u)
     nanosleep(&ts, NULL);
   }

   if (dam <= 0 && !(effects->count("FLAME"))) { // Ran out of momentum.
    ammo_effects(this, tx, ty, *effects);
    if (is_bolt && !(effects->count("IGNITE")) &&
        !(effects->count("EXPLOSIVE")) &&
        ((curammo->m1 == "wood" && !one_in(4)) ||
         (curammo->m1 != "wood" && !one_in(15))))
     m.add_item_or_charges(tx, ty, ammotmp);
    if (weapon->num_charges() == 0)
     weapon->curammo = NULL;
    return;
   }

// If there's a monster in the path of our bullet, and either our aim was true,
//  OR it's not the monster we were aiming at and we were lucky enough to hit it
   int mondex = mon_at(tx, ty);
// If we shot us a monster...
   if (mondex != -1 && (!z[mondex].has_flag(MF_DIGS) ||
       rl_dist(p.posx, p.posy, z[mondex].posx, z[mondex].posy) <= 1) &&
       ((!missed && i == trajectory.size() - 1) ||
        one_in((5 - int(z[mondex].type->size))))) {

    double goodhit = missed_by;
    if (i < trajectory.size() - 1) // Unintentional hit
     goodhit = double(rand() / (RAND_MAX + 1.0)) / 2;

// Penalize for the monster's speed
    if (z[mondex].speed > 80)
     goodhit *= double( double(z[mondex].speed) / 80.);

    std::vector<point> blood_traj = trajectory;
    blood_traj.insert(blood_traj.begin(), point(p.posx, p.posy));
    splatter(this, blood_traj, dam, &z[mondex]);
    shoot_monster(this, p, z[mondex], dam, goodhit, weapon);

   } else if ((!missed || one_in(3)) &&
              (npc_at(tx, ty) != -1 || (u.posx == tx && u.posy == ty)))  {
    double goodhit = missed_by;
    if (i < trajectory.size() - 1) // Unintentional hit
     goodhit = double(rand() / (RAND_MAX + 1.0)) / 2;
    player *h;
    if (u.posx == tx && u.posy == ty)
     h = &u;
    else
     h = active_npc[npc_at(tx, ty)];
    if (h->power_level >= 10 && h->uncanny_dodge()) {
     h->power_level -= 7; // dodging bullets costs extra
    }
    else {
     std::vector<point> blood_traj = trajectory;
     blood_traj.insert(blood_traj.begin(), point(p.posx, p.posy));
     splatter(this, blood_traj, dam);
     shoot_player(this, p, h, dam, goodhit);
    }
   } else
    m.shoot(this, tx, ty, dam, i == trajectory.size() - 1, *effects);
  } // Done with the trajectory!

  ammo_effects(this, tx, ty, *effects);
  if (effects->count("BOUNCE"))
  {
    for (int i = 0; i < z.size(); i++)
    {
        // search for monsters in radius 4 around impact site
        if (rl_dist(z[i].posx, z[i].posy, tx, ty) <= 4)
        {
            // don't hit targets that have already been hit
            if (!z[i].has_effect(ME_BOUNCED) && !z[i].dead)
            {
                add_msg(_("The attack bounced to %s!"), z[i].name().c_str());
                trajectory = line_to(tx, ty, z[i].posx, z[i].posy, 0);
                if (weapon->charges > 0)
                    fire(p, z[i].posx, z[i].posy, trajectory, false);
                break;
            }
        }
    }
  }

  if (m.move_cost(tx, ty) == 0) {
      tx = px;
      ty = py;
  }
  if (is_bolt && !(effects->count("IGNITE")) &&
      !(effects->count("EXPLOSIVE")) &&
      ((curammo->m1 == "wood" && !one_in(5)) ||
       (curammo->m1 != "wood" && !one_in(15))  ))
    m.add_item_or_charges(tx, ty, ammotmp);
 }

 if (weapon->num_charges() == 0)
  weapon->curammo = NULL;
}


void game::throw_item(player &p, int tarx, int tary, item &thrown,
                      std::vector<point> &trajectory)
{
    int deviation = 0;
    int trange = 1.5 * rl_dist(p.posx, p.posy, tarx, tary);
    std::set<std::string> no_effects;

    // Throwing attempts below "Basic Competency" level are extra-bad
    int skillLevel = p.skillLevel("throw");

    if (skillLevel < 3)
        deviation += rng(0, 8 - skillLevel);

    if (skillLevel < 8)
        deviation += rng(0, 8 - skillLevel);
    else
        deviation -= skillLevel - 6;

    deviation += p.throw_dex_mod();

    if (p.per_cur < 6)
        deviation += rng(0, 8 - p.per_cur);
    else if (p.per_cur > 8)
        deviation -= p.per_cur - 8;

    deviation += rng(0, p.encumb(bp_hands) * 2 + p.encumb(bp_eyes) + 1);
    if (thrown.volume() > 5)
        deviation += rng(0, 1 + (thrown.volume() - 5) / 4);
    if (thrown.volume() == 0)
        deviation += rng(0, 3);

    deviation += rng(0, 1 + abs(p.str_cur - thrown.weight()));

    double missed_by = .01 * deviation * trange;
    bool missed = false;
    int tart;

    if (missed_by >= 1)
    {
        // We missed D:
        // Shoot a random nearby space?
        if (missed_by > 9)
            missed_by = 9;
        tarx += rng(0 - int(sqrt(double(missed_by))), int(sqrt(double(missed_by))));
        tary += rng(0 - int(sqrt(double(missed_by))), int(sqrt(double(missed_by))));
        if (m.sees(p.posx, p.posy, tarx, tary, -1, tart))
            trajectory = line_to(p.posx, p.posy, tarx, tary, tart);
        else
            trajectory = line_to(p.posx, p.posy, tarx, tary, 0);
        missed = true;
        add_msg_if_player(&p,_("You miss!"));
    }
    else if (missed_by >= .6)
    {
        // Hit the space, but not necessarily the monster there
        missed = true;
        add_msg_if_player(&p,_("You barely miss!"));
    }

    std::string message;
    int real_dam = (thrown.weight() / 4 + thrown.type->melee_dam / 2 + p.str_cur / 2) /
               double(2 + double(thrown.volume() / 4));
    if (real_dam > thrown.weight() * 3)
        real_dam = thrown.weight() * 3;
    if (p.has_active_bionic("bio_railgun") && (thrown.made_of("iron") || thrown.made_of("steel")))
    {
        real_dam *= 2;
    }
    int dam = real_dam;

    int i = 0, tx = 0, ty = 0;
    for (i = 0; i < trajectory.size() && dam >= 0; i++)
    {
        message = "";
        double goodhit = missed_by;
        tx = trajectory[i].x;
        ty = trajectory[i].y;

        // If there's a monster in the path of our item, and either our aim was true,
        //  OR it's not the monster we were aiming at and we were lucky enough to hit it
        if (mon_at(tx, ty) != -1 &&
           (!missed || one_in(7 - int(z[mon_at(tx, ty)].type->size))))
        {
            if (rng(0, 100) < 20 + skillLevel * 12 &&
                thrown.type->melee_cut > 0)
            {
                if (!p.is_npc())
                {
                    char buf[128];
                    sprintf(buf, _(" You cut the %s!"), z[mon_at(tx, ty)].name().c_str());
                    message += buf;
                }
                if (thrown.type->melee_cut > z[mon_at(tx, ty)].armor_cut())
                    dam += (thrown.type->melee_cut - z[mon_at(tx, ty)].armor_cut());
            }
            if (thrown.made_of("glass") && !thrown.active && // active = molotov, etc.
                rng(0, thrown.volume() + 8) - rng(0, p.str_cur) < thrown.volume())
            {
                if (u_see(tx, ty))
                    add_msg(_("The %s shatters!"), thrown.tname().c_str());
                for (int i = 0; i < thrown.contents.size(); i++)
                    m.add_item_or_charges(tx, ty, thrown.contents[i]);
                    sound(tx, ty, 16, _("glass breaking!"));
                    int glassdam = rng(0, thrown.volume() * 2);
                    if (glassdam > z[mon_at(tx, ty)].armor_cut())
                        dam += (glassdam - z[mon_at(tx, ty)].armor_cut());
            }
            else
                m.add_item_or_charges(tx, ty, thrown);
            if (i < trajectory.size() - 1)
                goodhit = double(double(rand() / RAND_MAX) / 2);
            if (goodhit < .1 && !z[mon_at(tx, ty)].has_flag(MF_NOHEAD))
            {
                message = _("Headshot!");
                dam = rng(dam, dam * 3);
                p.practice(turn, "throw", 5);
            }
            else if (goodhit < .2)
            {
                message = _("Critical!");
                dam = rng(dam, dam * 2);
                p.practice(turn, "throw", 2);
            }
            else if (goodhit < .4)
                dam = rng(int(dam / 2), int(dam * 1.5));
            else if (goodhit < .5)
            {
                message = _("Grazing hit.");
                dam = rng(0, dam);
            }
            if (u_see(tx, ty)) {
                g->add_msg_player_or_npc(&p,
                    _("%s You hit the %s for %d damage."),
                    _("%s <npcname> hits the %s for %d damage."),
                    message.c_str(), z[mon_at(tx, ty)].name().c_str(), dam);
            }
            if (z[mon_at(tx, ty)].hurt(dam, real_dam))
                kill_mon(mon_at(tx, ty), !p.is_npc());
            return;
        }
        else // No monster hit, but the terrain might be.
        {
            m.shoot(this, tx, ty, dam, false, no_effects);
        }
        if (m.move_cost(tx, ty) == 0)
        {
            if (i > 0)
            {
                tx = trajectory[i - 1].x;
                ty = trajectory[i - 1].y;
            }
            else
            {
                tx = u.posx;
                ty = u.posy;
            }
            i = trajectory.size();
        }
        if (p.has_active_bionic("bio_railgun") && (thrown.made_of("iron") || thrown.made_of("steel")))
        {
            m.add_field(this, tx, ty, fd_electricity, rng(2,3));
        }
    }
    if (m.move_cost(tx, ty) == 0)
    {
        if (i > 1)
        {
            tx = trajectory[i - 2].x;
            ty = trajectory[i - 2].y;
        }
        else
        {
            tx = u.posx;
            ty = u.posy;
        }
    }
    if (thrown.made_of("glass") && !thrown.active && // active means molotov, etc
        rng(0, thrown.volume() + 8) - rng(0, p.str_cur) < thrown.volume())
    {
        if (u_see(tx, ty))
            add_msg(_("The %s shatters!"), thrown.tname().c_str());
        for (int i = 0; i < thrown.contents.size(); i++)
            m.add_item_or_charges(tx, ty, thrown.contents[i]);
        sound(tx, ty, 16, _("glass breaking!"));
    }
    else
    {
        sound(tx, ty, 8, _("thud."));
        m.add_item_or_charges(tx, ty, thrown);
    }
}

std::vector<point> game::target(int &x, int &y, int lowx, int lowy, int hix,
                                int hiy, std::vector <monster> t, int &target,
                                item *relevent)
{
 std::vector<point> ret;
 int tarx, tary, junk;
 int range=(hix-u.posx);
// First, decide on a target among the monsters, if there are any in range
 if (t.size() > 0) {
// Check for previous target
  if (target == -1) {
// If no previous target, target the closest there is
   double closest = -1;
   double dist;
   for (int i = 0; i < t.size(); i++) {
    dist = rl_dist(t[i].posx, t[i].posy, u.posx, u.posy);
    if (closest < 0 || dist < closest) {
     closest = dist;
     target = i;
    }
   }
  }
  x = t[target].posx;
  y = t[target].posy;
 } else
  target = -1;	// No monsters in range, don't use target, reset to -1

 WINDOW* w_target = newwin(13, 48, VIEW_OFFSET_Y + MINIMAP_HEIGHT, TERRAIN_WINDOW_WIDTH + 7 + VIEW_OFFSET_X);
 wborder(w_target, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                 LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 mvwprintz(w_target, 0, 2, c_white, "< ");
 if (!relevent) { // currently targetting vehicle to refill with fuel
   wprintz(w_target, c_red, _("Select a vehicle"));
 } else {
   if (relevent == &u.weapon && relevent->is_gun()) {
     wprintz(w_target, c_red, _("Firing %s (%d)"), // - %s (%d)",
            u.weapon.tname().c_str(),// u.weapon.curammo->name.c_str(),
            u.weapon.charges);
   } else {
     wprintz(w_target, c_red, _("Throwing %s"), relevent->tname().c_str());
   }
 }
 wprintz(w_target, c_white, " >");
/* Annoying clutter @ 2 3 4. */
 mvwprintz(w_target, 9, 1, c_white,
           _("Move cursor to target with directional keys."));
 if (relevent) {
  mvwprintz(w_target, 10, 1, c_white,
            _("'<' '>' Cycle targets; 'f' or '.' to fire."));
  mvwprintz(w_target, 11, 1, c_white,
            _("'0' target self; '*' toggle snap-to-target"));
 }

 wrefresh(w_target);
 char ch;
 bool snap_to_target = OPTIONS[OPT_SNAP_TO_TARGET];
 do {
  ret = line_to(u.posx, u.posy, x, y,0);

  if(trigdist && trig_dist(u.posx,u.posy, x,y) > range) {
    bool cont=true;
    int cx=x;
    int cy=y;
    for (int i = 0; i < ret.size() && cont; i++) {
      if(trig_dist(u.posx,u.posy, ret[i].x, ret[i].y) > range) {
        ret.resize(i);
        cont=false;
      } else {
        cx=0+ret[i].x; cy=0+ret[i].y;
      }
    }
    x=cx;y=cy;
  }
  point center;
  if (snap_to_target)
   center = point(x, y);
  else
   center = point(u.posx + u.view_offset_x, u.posy + u.view_offset_y);
  // Clear the target window.
  for (int i = 1; i < 8; i++) {
   for (int j = 1; j < 46; j++)
    mvwputch(w_target, i, j, c_white, ' ');
  }
  m.build_map_cache(this);
  m.draw(this, w_terrain, center);
  // Draw the Monsters
  for (int i = 0; i < z.size(); i++) {
   if (u_see(&(z[i]))) {
    z[i].draw(w_terrain, center.x, center.y, false);
   }
  }
  // Draw the NPCs
  for (int i = 0; i < active_npc.size(); i++) {
   if (u_see(active_npc[i]->posx, active_npc[i]->posy))
    active_npc[i]->draw(w_terrain, center.x, center.y, false);
  }
  if (x != u.posx || y != u.posy) {

   // Draw the player
   int atx = VIEWX + u.posx - center.x, aty = VIEWY + u.posy - center.y;
   if (atx >= 0 && atx < TERRAIN_WINDOW_WIDTH && aty >= 0 && aty < TERRAIN_WINDOW_HEIGHT)
    mvwputch(w_terrain, aty, atx, u.color(), '@');

   // Only draw a highlighted trajectory if we can see the endpoint.
   // Provides feedback to the player, and avoids leaking information about tiles they can't see.
   if (u_see( x, y)) {
    for (int i = 0; i < ret.size(); i++) {
      int mondex = mon_at(ret[i].x, ret[i].y),
          npcdex = npc_at(ret[i].x, ret[i].y);
      // NPCs and monsters get drawn with inverted colors
      if (mondex != -1 && u_see(&(z[mondex])))
       z[mondex].draw(w_terrain, center.x, center.y, true);
      else if (npcdex != -1)
       active_npc[npcdex]->draw(w_terrain, center.x, center.y, true);
      else
       m.drawsq(w_terrain, u, ret[i].x, ret[i].y, true,true,center.x, center.y);
    }
   }

   if (!relevent) { // currently targetting vehicle to refill with fuel
    vehicle *veh = m.veh_at(x, y);
    if (veh)
     mvwprintw(w_target, 1, 1, _("There is a %s"), veh->name.c_str());
   } else
    mvwprintw(w_target, 1, 1, _("Range: %d"), rl_dist(u.posx, u.posy, x, y));

   if (mon_at(x, y) == -1) {
    if (snap_to_target)
     mvwputch(w_terrain, VIEWY, VIEWX, c_red, '*');
    else
     mvwputch(w_terrain, VIEWY + y - center.y, VIEWX + x - center.x, c_red, '*');
   } else if (u_see(&(z[mon_at(x, y)]))) {
    z[mon_at(x, y)].print_info(this, w_target,2);
   }
  }
  wrefresh(w_target);
  wrefresh(w_terrain);
  wrefresh(w_status);
  refresh();
  ch = input();
  get_direction(this, tarx, tary, ch);
  if (tarx != -2 && tary != -2 && ch != '.') {	// Direction character pressed
   int mondex = mon_at(x, y), npcdex = npc_at(x, y);
   if (mondex != -1 && u_see(&(z[mondex])))
    z[mondex].draw(w_terrain, center.x, center.y, false);
   else if (npcdex != -1)
    active_npc[npcdex]->draw(w_terrain, center.x, center.y, false);
   else if (m.sees(u.posx, u.posy, x, y, -1, junk))
    m.drawsq(w_terrain, u, x, y, false, true, center.x, center.y);
   else
    mvwputch(w_terrain, VIEWY, VIEWX, c_black, 'X');
   x += tarx;
   y += tary;
   if (x < lowx)
    x = lowx;
   else if (x > hix)
    x = hix;
   if (y < lowy)
    y = lowy;
   else if (y > hiy)
    y = hiy;
  } else if ((ch == '<') && (target != -1)) {
   target--;
   if (target == -1) target = t.size() - 1;
   x = t[target].posx;
   y = t[target].posy;
  } else if ((ch == '>') && (target != -1)) {
   target++;
   if (target == t.size()) target = 0;
   x = t[target].posx;
   y = t[target].posy;
  } else if (ch == '.' || ch == 'f' || ch == 'F' || ch == '\n') {
   for (int i = 0; i < t.size(); i++) {
    if (t[i].posx == x && t[i].posy == y)
     target = i;
   }
   if (u.posx == x && u.posy == y)
       ret.clear();
   return ret;
  } else if (ch == '0') {
   x = u.posx;
   y = u.posy;
   ret.clear();
  } else if (ch == '*')
   snap_to_target = !snap_to_target;
  else if (ch == KEY_ESCAPE || ch == 'q') { // return empty vector (cancel)
   ret.clear();
   return ret;
  }
 } while (true);
}

// MATERIALS-TODO: use fire resistance
void game::hit_monster_with_flags(monster &z, const std::set<std::string> &effects)
{
 if (effects.count("FLAME")) {

  if (z.made_of("veggy") || z.made_of("cotton") || z.made_of("wool") ||
      z.made_of("paper") || z.made_of("wood"))
   z.add_effect(ME_ONFIRE, rng(8, 20));
  else if (z.made_of("flesh"))
   z.add_effect(ME_ONFIRE, rng(5, 10));
 } else if (effects.count("INCENDIARY")) {

  if (z.made_of("veggy") || z.made_of("cotton") || z.made_of("wool") ||
      z.made_of("paper") || z.made_of("wood"))
   z.add_effect(ME_ONFIRE, rng(2, 6));
  else if (z.made_of("flesh") && one_in(4))
   z.add_effect(ME_ONFIRE, rng(1, 4));

 } else if (effects.count("IGNITE")) {

   if (z.made_of("veggy") || z.made_of("cotton") || z.made_of("wool") ||
      z.made_of("paper") || z.made_of("wood"))
      z.add_effect(ME_ONFIRE, rng(6, 6));
   else if (z.made_of("flesh"))
   z.add_effect(ME_ONFIRE, rng(10, 10));

 }
 if (effects.count("BOUNCE")) {
     z.add_effect(ME_BOUNCED, 1);
 }
 int stun_strength = 0;
 if (effects.count("BEANBAG")) {
     stun_strength = 4;
 }
 if (effects.count("LARGE_BEANBAG")) {
     stun_strength = 16;
 }
 if( stun_strength > 0 ) {
     switch( z.type->size )
     {
     case MS_TINY:
         stun_strength *= 4;
         break;
     case MS_SMALL:
         stun_strength *= 2;
         break;
     case MS_MEDIUM:
     default:
         break;
     case MS_LARGE:
         stun_strength /= 2;
         break;
     case MS_HUGE:
         stun_strength /= 4;
         break;
     }
     z.add_effect( ME_STUNNED, rng(stun_strength / 2, stun_strength) );
 }
}

int time_to_fire(player &p, it_gun* firing)
{
 int time = 0;
 if (firing->skill_used == Skill::skill("pistol")) {
   if (p.skillLevel("pistol") > 6)
     time = 10;
   else
     time = (80 - 10 * p.skillLevel("pistol"));
 } else if (firing->skill_used == Skill::skill("shotgun")) {
   if (p.skillLevel("shotgun") > 3)
     time = 70;
   else
     time = (150 - 25 * p.skillLevel("shotgun"));
 } else if (firing->skill_used == Skill::skill("smg")) {
   if (p.skillLevel("smg") > 5)
     time = 20;
   else
     time = (80 - 10 * p.skillLevel("smg"));
 } else if (firing->skill_used == Skill::skill("rifle")) {
   if (p.skillLevel("rifle") > 8)
     time = 30;
   else
     time = (150 - 15 * p.skillLevel("rifle"));
 } else if (firing->skill_used == Skill::skill("archery")) {
   if (p.skillLevel("archery") > 8)
     time = 20;
   else
     time = (220 - 25 * p.skillLevel("archery"));
 } else if (firing->skill_used == Skill::skill("throw")) {
   if (p.skillLevel("throw") > 6){
     time = 50;
   }else{
     time = (220 - 25 * p.skillLevel("throw"));
   }
 } else if (firing->skill_used == Skill::skill("launcher")) {
   if (p.skillLevel("launcher") > 8)
     time = 30;
   else
     time = (200 - 20 * p.skillLevel("launcher"));
 }
  else {
   debugmsg("Why is shooting %s using %s skill?", (firing->name).c_str(), firing->skill_used->name().c_str());
   time =  0;
 }

 return time;
}

void make_gun_sound_effect(game *g, player &p, bool burst, item* weapon)
{
 std::string gunsound;
 // noise() doesn't suport gunmods, but it does return the right value
 int noise = p.weapon.noise();
 if (noise < 5) {
  if (burst)
   gunsound = _("Brrrip!");
  else
   gunsound = _("plink!");
 } else if (noise < 25) {
  if (burst)
   gunsound = _("Brrrap!");
  else
   gunsound = _("bang!");
 } else if (noise < 60) {
  if (burst)
   gunsound = _("P-p-p-pow!");
  else
   gunsound = _("blam!");
 } else {
  if (burst)
   gunsound = _("Kaboom!!");
  else
   gunsound = _("kerblam!");
 }
 if (weapon->curammo->type == "fusion" || weapon->curammo->type == "battery" ||
     weapon->curammo->type == "plutonium")
  g->sound(p.posx, p.posy, 8, _("Fzzt!"));
 else if (weapon->curammo->type == "40mm")
  g->sound(p.posx, p.posy, 8, _("Thunk!"));
 else if (weapon->curammo->type == "gasoline" || weapon->curammo->type == "66mm")
  g->sound(p.posx, p.posy, 4, _("Fwoosh!"));
 else if (weapon->curammo->type != "bolt" &&
          weapon->curammo->type != "arrow" &&
          weapon->curammo->type != "pebble")
  g->sound(p.posx, p.posy, noise, gunsound);
}

int calculate_range(player &p, int tarx, int tary)
{
 int trange = rl_dist(p.posx, p.posy, tarx, tary);
 it_gun* firing = dynamic_cast<it_gun*>(p.weapon.type);
 if (trange < int(firing->volume / 3) && firing->ammo != "shot")
  trange = int(firing->volume / 3);
 else if (p.has_bionic("bio_targeting")) {
  if (trange > LONG_RANGE)
   trange = int(trange * .65);
  else
   trange = int(trange * .8);
 }

 if (firing->skill_used == Skill::skill("rifle") && trange > LONG_RANGE)
  trange = LONG_RANGE + .6 * (trange - LONG_RANGE);

 return trange;
}

double calculate_missed_by(player &p, int trange, item* weapon)
{
    // No type for gunmods,so use player weapon.
    it_gun* firing = dynamic_cast<it_gun*>(p.weapon.type);
    // Calculate deviation from intended target (assuming we shoot for the head)
    double deviation = 0.; // Measured in quarter-degrees.
    // Up to 0.75 degrees for each skill point < 8.
    if (p.skillLevel(firing->skill_used) < 8) {
        deviation += rng(0, 3 * (8 - p.skillLevel(firing->skill_used)));
    }

    // Up to 0.25 deg per each skill point < 9.
    if (p.skillLevel("gun") < 9) { deviation += rng(0, 9 - p.skillLevel("gun")); }

    deviation += rng(0, p.ranged_dex_mod());
    deviation += rng(0, p.ranged_per_mod());

    deviation += rng(0, 2 * p.encumb(bp_arms)) + rng(0, 4 * p.encumb(bp_eyes));

    deviation += rng(0, weapon->curammo->dispersion);
    // item::dispersion() doesn't support gunmods.
    deviation += rng(0, p.weapon.dispersion());
    int adj_recoil = p.recoil + p.driving_recoil;
    deviation += rng(int(adj_recoil / 4), adj_recoil);

    if (deviation < 0) { return 0; }
    // .013 * trange is a computationally cheap version of finding the tangent.
    // (note that .00325 * 4 = .013; .00325 is used because deviation is a number
    //  of quarter-degrees)
    // It's also generous; missed_by will be rather short.
    return (.00325 * deviation * trange);
}

int recoil_add(player &p)
{
 // Gunmods don't have atype,so use guns.
 it_gun* firing = dynamic_cast<it_gun*>(p.weapon.type);
 // item::recoil() doesn't suport gunmods, so call it on player gun.
 int ret = p.weapon.recoil();
 ret -= rng(p.str_cur / 2, p.str_cur);
 ret -= rng(0, p.skillLevel(firing->skill_used) / 2);
 if (ret > 0)
  return ret;
 return 0;
}

void shoot_monster(game *g, player &p, monster &mon, int &dam, double goodhit, item* weapon)
{
 // Gunmods don't have a type, so use the player weapon type.
 it_gun* firing = dynamic_cast<it_gun*>(p.weapon.type);
 std::string message;
 bool u_see_mon = g->u_see(&(mon));
 int adjusted_damage = dam;
 if (mon.has_flag(MF_HARDTOSHOOT) && !one_in(4) &&
     weapon->curammo->phase != LIQUID &&
     weapon->curammo->dispersion >= 4) { // Buckshot hits anyway
  if (u_see_mon)
   g->add_msg(_("The shot passes through the %s without hitting."),
           mon.name().c_str());
  goodhit = 1;
 } else { // Not HARDTOSHOOT
// Armor blocks BEFORE any critical effects.
  int zarm = mon.armor_cut();
  zarm -= weapon->curammo->pierce;
  if (weapon->curammo->phase == LIQUID)
   zarm = 0;
  else if (weapon->curammo->dispersion < 4) // Shot doesn't penetrate armor well
   zarm *= rng(2, 4);
  if (zarm > 0)
   adjusted_damage -= zarm;
  if (adjusted_damage <= 0) {
   if (u_see_mon)
    g->add_msg(_("The shot reflects off the %s!"),
            mon.name_with_armor().c_str());
   adjusted_damage = 0;
   goodhit = 1;
  }
  if (goodhit < .1 && !mon.has_flag(MF_NOHEAD)) {
   message = _("Headshot!");
   adjusted_damage = rng(5 * adjusted_damage, 8 * adjusted_damage);
   p.practice(g->turn, firing->skill_used, 5);
  } else if (goodhit < .2) {
   message = _("Critical!");
   adjusted_damage = rng(adjusted_damage * 2, adjusted_damage * 3);
   p.practice(g->turn, firing->skill_used, 2);
  } else if (goodhit < .4) {
   adjusted_damage = rng(int(adjusted_damage * .9), int(adjusted_damage * 1.5));
   p.practice(g->turn, firing->skill_used, rng(0, 2));
  } else if (goodhit <= .7) {
   message = _("Grazing hit.");
   adjusted_damage = rng(0, adjusted_damage);
  } else
   adjusted_damage = 0;

  if(item(weapon->curammo, 0).has_flag("NOGIB"))
  {
      adjusted_damage = std::min(adjusted_damage, mon.hp+10);
  }

// Find the zombie at (x, y) and hurt them, MAYBE kill them!
  if (adjusted_damage > 0) {
   mon.moves -= adjusted_damage * 5;
   if (&p == &(g->u) && u_see_mon)
    g->add_msg(_("%s You hit the %s for %d damage."), message.c_str(), mon.name().c_str(), adjusted_damage);
   else if (u_see_mon)
    g->add_msg(_("%s %s shoots the %s."), message.c_str(), p.name.c_str(), mon.name().c_str());
   bool bMonDead = mon.hurt(adjusted_damage, dam);
   if( u_see_mon ) {
       hit_animation(mon.posx - g->u.posx + VIEWX - g->u.view_offset_x,
                     mon.posy - g->u.posy + VIEWY - g->u.view_offset_y,
                     red_background(mon.type->color), (bMonDead) ? '%' : mon.symbol());
   }

   if (bMonDead)
    g->kill_mon(g->mon_at(mon.posx, mon.posy), (&p == &(g->u)));
   else if (!weapon->curammo->ammo_effects.empty())
    g->hit_monster_with_flags(mon, weapon->curammo->ammo_effects);

   adjusted_damage = 0;
  }
 }
 dam = adjusted_damage;
}

void shoot_player(game *g, player &p, player *h, int &dam, double goodhit)
{
 int npcdex = g->npc_at(h->posx, h->posy);
 // Gunmods don't have a type, so use the player gun type.
 it_gun* firing = dynamic_cast<it_gun*>(p.weapon.type);
 body_part hit;
 int side = rng(0, 1);
 if (goodhit < .003) {
  hit = bp_eyes;
  dam = rng(3 * dam, 5 * dam);
  p.practice(g->turn, firing->skill_used, 5);
 } else if (goodhit < .066) {
  if (one_in(25))
   hit = bp_eyes;
  else if (one_in(15))
   hit = bp_mouth;
  else
   hit = bp_head;
  dam = rng(2 * dam, 5 * dam);
  p.practice(g->turn, firing->skill_used, 5);
 } else if (goodhit < .2) {
  hit = bp_torso;
  dam = rng(dam, 2 * dam);
  p.practice(g->turn, firing->skill_used, 2);
 } else if (goodhit < .4) {
  if (one_in(3))
   hit = bp_torso;
  else if (one_in(2))
   hit = bp_arms;
  else
   hit = bp_legs;
  dam = rng(int(dam * .9), int(dam * 1.5));
  p.practice(g->turn, firing->skill_used, rng(0, 1));
 } else if (goodhit < .5) {
  if (one_in(2))
   hit = bp_arms;
  else
   hit = bp_legs;
  dam = rng(dam / 2, dam);
 } else {
  dam = 0;
 }
 if (dam > 0) {
  h->moves -= rng(0, dam);
  if (h == &(g->u))
   g->add_msg(_("%s shoots your %s for %d damage!"), p.name.c_str(),
              body_part_name(hit, side).c_str(), dam);
  else {
   if (&p == &(g->u)) {
    g->add_msg(_("You shoot %s's %s."), h->name.c_str(),
               body_part_name(hit, side).c_str());
                g->active_npc[npcdex]->make_angry();
 } else if (g->u_see(h->posx, h->posy))
    g->add_msg(_("%s shoots %s's %s."),
               (g->u_see(p.posx, p.posy) ? p.name.c_str() : _("Someone")),
               h->name.c_str(), body_part_name(hit, side).c_str());
  }
  h->hit(g, hit, side, 0, dam);
 }
}

void splatter(game *g, std::vector<point> trajectory, int dam, monster* mon)
{
 field_id blood = fd_blood;
 if (mon != NULL) {
  if (!mon->made_of("flesh"))
   return;
  if (mon->type->dies == &mdeath::boomer)
   blood = fd_bile;
  else if (mon->type->dies == &mdeath::acid)
   blood = fd_acid;
 }

 int distance = 1;
 if (dam > 50)
  distance = 3;
 else if (dam > 20)
  distance = 2;

 std::vector<point> spurt = continue_line(trajectory, distance);

 for (int i = 0; i < spurt.size(); i++) {
  int tarx = spurt[i].x, tary = spurt[i].y;
  field &bloody_field = g->m.field_at(tarx, tary);
  if (bloody_field.findField(fd_blood) &&
      bloody_field.findField(fd_blood)->getFieldDensity() < 3)
      bloody_field.findField(fd_blood)->setFieldDensity(bloody_field.findField(fd_blood)->getFieldDensity() + 1);
  else
   g->m.add_field(g, tarx, tary, blood, 1);
 }
}

void ammo_effects(game *g, int x, int y, const std::set<std::string> &effects)
{
  if (effects.count("EXPLOSIVE"))
    g->explosion(x, y, 24, 0, false);

  if (effects.count("FRAG"))
    g->explosion(x, y, 12, 28, false);

  if (effects.count("NAPALM"))
    g->explosion(x, y, 18, 0, true);

  if (effects.count("ACIDBOMB")) {
    for (int i = x - 1; i <= x + 1; i++) {
      for (int j = y - 1; j <= y + 1; j++) {
        g->m.add_field(g, i, j, fd_acid, 3);
      }
    }
  }

  if (effects.count("EXPLOSIVE_BIG"))
    g->explosion(x, y, 40, 0, false);

  if (effects.count("TEARGAS")) {
    for (int i = -2; i <= 2; i++) {
      for (int j = -2; j <= 2; j++)
        g->m.add_field(g, x + i, y + j, fd_tear_gas, 3);
    }
  }

  if (effects.count("SMOKE")) {
    for (int i = -1; i <= 1; i++) {
      for (int j = -1; j <= 1; j++)
        g->m.add_field(g, x + i, y + j, fd_smoke, 3);
    }
  }

  if (effects.count("FLASHBANG"))
    g->flashbang(x, y);

  if (effects.count("FLAME"))
    g->explosion(x, y, 4, 0, true);

  if (effects.count("LIGHTNING")) {
    for (int i = x - 1; i <= x + 1; i++) {
      for (int j = y - 1; j <= y + 1; j++) {
        g->m.add_field(g, i, j, fd_electricity, 3);
      }
    }
  }
}
