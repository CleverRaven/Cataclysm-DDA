#include "item_factory.h"
#include "mondeath.h"
#include "monster.h"
#include "game.h"
#include "rng.h"
#include "line.h"
#include "monstergenerator.h"
#include <sstream>

void mdeath::normal(game *g, monster *z)
{
 if (g->u_see(z)) {
  g->add_msg(_("The %s dies!"), z->name().c_str());
 }
 if(z->type->difficulty >= 30) {
   g->u.add_memorial_log(_("Killed a %s."), z->name().c_str());
 }
 if (z->made_of("flesh") && z->has_flag(MF_WARM)) {
   g->m.add_field(g, z->posx(), z->posy(), fd_blood, 1);
 }

    int gib_amount = 0;
    bool leave_corpse = false;
    int corpse_damage = 0;
// determine how much of a mess is left, for flesh and veggy creatures
    if (z->made_of("flesh") || z->made_of("veggy") || z->made_of("hflesh"))
    {
        if (z->hp >= 0 - 2 * z->type->hp) // less than 2x monster hp overkill, leave corpse
        {
            leave_corpse = true;
            corpse_damage = 0;
        }
        if (z->hp >= 0 - 2 * z->type->hp - 20 && z->hp < 0 - 2 * z->type->hp) // corpse with damage 1
        {
            leave_corpse = true;
            corpse_damage = 1;
            gib_amount = rng(1,3);
        }
        if (z->hp >= 0 - 2 * z->type->hp - 40 && z->hp < 0 - 2 * z->type->hp - 20) // corpse with damage 2
        {
            leave_corpse = true;
            corpse_damage = 2;
            gib_amount = rng(2,5);
        }
        if (z->hp >= 0 - 2 * z->type->hp - 60 && z->hp < 0 - 2 * z->type->hp - 40) // corpse with damage 3
        {
            leave_corpse = true;
            corpse_damage = 3;
            gib_amount = rng(3,9);
        }
        if (z->hp >= 0 - 2 * z->type->hp - 80 && z->hp < 0 - 2 * z->type->hp - 60) // corpse with damage 4
        {
            leave_corpse = true;
            corpse_damage = 4;
            gib_amount = rng(4,12);
        }
        if (z->hp <= 0 - 2 * z->type->hp - 80) // no corpse if MS_TINY or MS_SMALL, gib_amount fields only
        {
            if (z->type->size == MS_MEDIUM || z->type->size == MS_LARGE || z->type->size == MS_HUGE)
            {
                leave_corpse = true;
                corpse_damage = 4;
            }
            else
            {
                leave_corpse = false;
            }
            gib_amount = rng(5,15);
        }
    }

    // leave a corpse, if any
    if (leave_corpse)
    {
        item tmp;
        tmp.make_corpse(g->itypes["corpse"], z->type, g->turn);
        tmp.damage = corpse_damage;
        g->m.add_item_or_charges(z->posx(), z->posy(), tmp);
    }

    // leave gibs
    for (int i = 0; i < gib_amount; i++)
    {
        const int rand_posx = z->posx() + rng(1,5) - 3;
        const int rand_posy = z->posy() + rng(1,5) - 3;
        const int rand_density = rng(1, 3);

        if (z->made_of("flesh") || z->made_of("hflesh"))
        {
            g->m.add_field(g, rand_posx, rand_posy, fd_gibs_flesh, rand_density);
        }
        else if (z->made_of("veggy"))
        {
            g->m.add_field(g, rand_posx, rand_posy, fd_gibs_veggy, rand_density);
        }
    }
}

void mdeath::acid(game *g, monster *z)
{
 if (g->u_see(z))
  g->add_msg(_("The %s's corpse melts into a pool of acid."), z->name().c_str());
 g->m.add_field(g, z->posx(), z->posy(), fd_acid, 3);
}

void mdeath::boomer(game *g, monster *z)
{
 std::string tmp;
 g->sound(z->posx(), z->posy(), 24, _("a boomer explodes!"));
 for (int i = -1; i <= 1; i++) {
  for (int j = -1; j <= 1; j++) {
   g->m.bash(z->posx() + i, z->posy() + j, 10, tmp);
    g->m.add_field(g, z->posx() + i, z->posy() + j, fd_bile, 1);
   int mondex = g->mon_at(z->posx() + i, z->posy() +j);
   if (mondex != -1) {
    g->zombie(mondex).stumble(g, false);
    g->zombie(mondex).moves -= 250;
   }
  }
 }
 if (rl_dist(z->posx(), z->posy(), g->u.posx, g->u.posy) == 1)
  g->u.infect("boomered", bp_eyes, 2, 24, false, 1, 1);
}

void mdeath::kill_vines(game *g, monster *z)
{
 std::vector<int> vines;
 std::vector<int> hubs;
 for (int i = 0; i < g->num_zombies(); i++) {
  if (g->zombie(i).type->id == "mon_creeper_hub" &&
      (g->zombie(i).posx() != z->posx() || g->zombie(i).posy() != z->posy()))
   hubs.push_back(i);
  if (g->zombie(i).type->id == "mon_creeper_vine")
   vines.push_back(i);
 }

 for (int i = 0; i < vines.size(); i++) {
  monster *vine = &(g->zombie( vines[i] ) );
  int dist = rl_dist(vine->posx(), vine->posy(), z->posx(), z->posy());
  bool closer_hub = false;
  for (int j = 0; j < hubs.size() && !closer_hub; j++) {
   if (rl_dist(vine->posx(), vine->posy(),
               g->zombie( hubs[j] ).posx(), g->zombie( hubs[j] ).posy()) < dist)
    closer_hub = true;
  }
  if (!closer_hub)
   vine->hp = 0;
 }
}

void mdeath::vine_cut(game *g, monster *z)
{
 std::vector<int> vines;
 for (int x = z->posx() - 1; x <= z->posx() + 1; x++) {
  for (int y = z->posy() - 1; y <= z->posy() + 1; y++) {
   if (x == z->posx() && y == z->posy())
    y++; // Skip ourselves
   int mondex = g->mon_at(x, y);
   if (mondex != -1 && g->zombie(mondex).type->id == "mon_creeper_vine")
    vines.push_back(mondex);
  }
 }

 for (int i = 0; i < vines.size(); i++) {
  bool found_neighbor = false;
  monster *vine = &(g->zombie( vines[i] ));
  for (int x = vine->posx() - 1; x <= vine->posx() + 1 && !found_neighbor; x++) {
   for (int y = vine->posy() - 1; y <= vine->posy() + 1 && !found_neighbor; y++) {
    if (x != z->posx() || y != z->posy()) { // Not the dying vine
     int mondex = g->mon_at(x, y);
     if (mondex != -1 && (g->zombie(mondex).type->id == "mon_creeper_hub" ||
                          g->zombie(mondex).type->id == "mon_creeper_vine"  ))
      found_neighbor = true;
    }
   }
  }
  if (!found_neighbor)
   vine->hp = 0;
 }
}

void mdeath::triffid_heart(game *g, monster *z)
{
 g->add_msg(_("The root walls begin to crumble around you."));
 g->add_event(EVENT_ROOTS_DIE, int(g->turn) + 100);
}

void mdeath::fungus(game *g, monster *z)
{
    mdeath::normal(g, z);
    monster spore(GetMType("mon_spore"));
    int sporex, sporey;
    int mondex;
    //~ the sound of a fungus dying
    g->sound(z->posx(), z->posy(), 10, _("Pouf!"));
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            sporex = z->posx() + i;
            sporey = z->posy() + j;
            mondex = g->mon_at(sporex, sporey);
            if (g->m.move_cost(sporex, sporey) > 0) {
                if (mondex != -1) { // Spores hit a monster
                    if (g->u_see(sporex, sporey) &&
                          !g->zombie(mondex).type->in_species("FUNGUS")) {
                        g->add_msg(_("The %s is covered in tiny spores!"),
                                        g->zombie(mondex).name().c_str());
                    }
                    if (!g->zombie(mondex).make_fungus(g)) {
                        g->kill_mon(mondex, (z->friendly != 0));
                    }
                } else if (g->u.posx == sporex && g->u.posy == sporey) {
                    // Spores hit the player
                    bool hit = false;
                    if (one_in(4) && g->u.infect("spores", bp_head, 3, 90, false, 1, 3, 120, 1, true)) {
                        hit = true;
                    }
                    if (one_in(2) && g->u.infect("spores", bp_torso, 3, 90, false, 1, 3, 120, 1, true)) {
                        hit = true;
                    }
                    if (one_in(4) && g->u.infect("spores", bp_arms, 3, 90, false, 1, 3, 120, 1, true, 1)) {
                        hit = true;
                    }
                    if (one_in(4) && g->u.infect("spores", bp_arms, 3, 90, false, 1, 3, 120, 1, true, 0)) {
                        hit = true;
                    }
                    if (one_in(4) && g->u.infect("spores", bp_legs, 3, 90, false, 1, 3, 120, 1, true, 1)) {
                        hit = true;
                    }
                    if (one_in(4) && g->u.infect("spores", bp_legs, 3, 90, false, 1, 3, 120, 1, true, 0)) {
                        hit = true;
                    }
                    if (hit) {
                        g->add_msg(_("You're covered in tiny spores!"));
                    }
                } else if (one_in(2) && g->num_zombies() <= 1000) { // Spawn a spore
                    spore.spawn(sporex, sporey);
                    g->add_zombie(spore);
                }
            }
        }
    }
}

void mdeath::disintegrate(game *g, monster *z)
{
 if (g->u_see(z))
  g->add_msg(_("It disintegrates!"));
}

void mdeath::worm(game *g, monster *z)
{
    if (g->u_see(z))
        g->add_msg(_("The %s splits in two!"), z->name().c_str());

    std::vector <point> wormspots;
    int wormx, wormy;
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            wormx = z->posx() + i;
            wormy = z->posy() + j;
            if (g->m.has_flag("DIGGABLE", wormx, wormy) && 
                    !(g->u.posx == wormx && g->u.posy == wormy)) {
                wormspots.push_back(point(wormx, wormy));
            }
        }
    }
    int worms = 0;
    while(worms < 2 && wormspots.size() > 0) {
        monster worm(GetMType("mon_halfworm"));
        int rn = rng(0, wormspots.size() - 1);
        if(-1 == g->mon_at(wormspots[rn])) {
            worm.spawn(wormspots[rn].x, wormspots[rn].y);
            g->add_zombie(worm);
            worms++;
        }
        wormspots.erase(wormspots.begin() + rn);
    }
}

void mdeath::disappear(game *g, monster *z)
{
 g->add_msg(_("The %s disappears!  Was it in your head?"), z->name().c_str());
}

void mdeath::guilt(game *g, monster *z)
{
 if (g->u.has_trait("CANNIBAL"))
  return; // We don't give a shit!
 if (rl_dist(z->posx(), z->posy(), g->u.posx, g->u.posy) > 5)
  return; // Too far away, we can deal with it
 if (z->hp >= 0)
  return; // It probably didn't die from damage
 g->add_msg(_("You feel terrible for killing %s!"), z->name().c_str());
 if(z->type->id == "mon_hallu_mom")
 {
 g->u.add_morale(MORALE_KILLED_MONSTER, -50, -250, 300, 30);
 }
 else if(z->type->id == "mon_zombie_child")
 {
 g->u.add_morale(MORALE_KILLED_MONSTER, -5, -250, 300, 30);
 }
 else
 {
 return;
 }
}
void mdeath::blobsplit(game *g, monster *z)
{
 int speed = z->speed - rng(30, 50);
 if (speed <= 0) {
  if (g->u_see(z))
   g->add_msg(_("The %s splatters into tiny, dead pieces."), z->name().c_str());
  return;
 }
 monster blob(GetMType((speed < 50 ? "mon_blob_small" : "mon_blob")));
 blob.speed = speed;
 blob.friendly = z->friendly; // If we're tame, our kids are too
 if (g->u_see(z))
  g->add_msg(_("The %s splits!"), z->name().c_str());
 blob.hp = blob.speed;
 std::vector <point> valid;

 for (int i = -1; i <= 1; i++) {
  for (int j = -1; j <= 1; j++) {
   if (g->m.move_cost(z->posx()+i, z->posy()+j) > 0 &&
       g->mon_at(z->posx()+i, z->posy()+j) == -1 &&
       (g->u.posx != z->posx()+i || g->u.posy != z->posy() + j))
    valid.push_back(point(z->posx()+i, z->posy()+j));
  }
 }

 int rn;
 for (int s = 0; s < 2 && valid.size() > 0; s++) {
  rn = rng(0, valid.size() - 1);
  blob.spawn(valid[rn].x, valid[rn].y);
  g->add_zombie(blob);
  valid.erase(valid.begin() + rn);
 }
}

void mdeath::melt(game *g, monster *z)
{
 if (g->u_see(z))
  g->add_msg(_("The %s melts away!"), z->name().c_str());
}

void mdeath::amigara(game *g, monster *z)
{
 if (g->u.has_disease("amigara")) {
  int count = 0;
  for (int i = 0; i < g->num_zombies(); i++) {
   if (g->zombie(i).type->id == "mon_amigara_horror")
    count++;
  }
  if (count <= 1) { // We're the last!
   g->u.rem_disease("amigara");
   g->add_msg(_("Your obsession with the fault fades away..."));
   item art(g->new_artifact(), g->turn);
   g->m.add_item_or_charges(z->posx(), z->posy(), art);
  }
 }
 normal(g, z);
}

void mdeath::thing(game *g, monster *z)
{
 monster thing(GetMType("mon_thing"));
 thing.spawn(z->posx(), z->posy());
 g->add_zombie(thing);
}

void mdeath::explode(game *g, monster *z)
{
 int size = 0;
 switch (z->type->size) {
  case MS_TINY:   size =  4; break;
  case MS_SMALL:  size =  8; break;
  case MS_MEDIUM: size = 14; break;
  case MS_LARGE:  size = 20; break;
  case MS_HUGE:   size = 26; break;
 }
 g->explosion(z->posx(), z->posy(), size, 0, false);
}

void mdeath::ratking(game *g, monster *z)
{
    g->u.rem_disease("rat");
    if (g->u_see(z)) {
        g->add_msg(_("Rats swarm from nowhere to avenge the %s."), z->name().c_str());
    }

    std::vector <point> ratspots;
    int ratx, raty;
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            ratx = z->posx() + i;
            raty = z->posy() + i;
            if (g->m.move_cost(ratx, raty) > 0 && g->mon_at(ratx, raty) == -1 &&
                  !(g->u.posx == ratx && g->u.posy == raty)) {
                ratspots.push_back(point(ratx, raty));
            }
        }
    }
    int rn;
    monster rat(GetMType("mon_sewer_rat"));
    for (int rats = 0; rats < 7 && ratspots.size() > 0; rats++) {
        rn = rng(0, ratspots.size() - 1);
        rat.spawn(ratspots[rn].x, ratspots[rn].y);
        g->add_zombie(rat);
        ratspots.erase(ratspots.begin() + rn);
 }
}

void mdeath::smokeburst(game *g, monster *z)
{
  std::string tmp;
  g->sound(z->posx(), z->posy(), 24, _("a smoker explodes!"));
  for (int i = -1; i <= 1; i++) {
    for (int j = -1; j <= 1; j++) {
      g->m.add_field(g, z->posx() + i, z->posy() + j, fd_smoke, 3);
      int mondex = g->mon_at(z->posx() + i, z->posy() +j);
      if (mondex != -1) {
        g->zombie(mondex).stumble(g, false);
        g->zombie(mondex).moves -= 250;
      }
    }
  }
}

// this function generates clothing for zombies
void mdeath::zombie(game *g, monster *z)
{
    // normal death function first
    mdeath::normal(g, z);

    // skip clothing generation if the zombie was rezzed rather than spawned
    if (z->no_extra_death_drops)
    {
        return;
    }

    // now generate appropriate clothing
    char dropset = -1;
    std::string zid = z->type->id;
    if (zid == "mon_zombie_cop"){ dropset = 0;}
    else if (zid == "mon_zombie_swimmer"){ dropset = 1;}
    else if (zid == "mon_zombie_scientist"){ dropset = 2;}
    else if (zid == "mon_zombie_soldier"){ dropset = 3;}
    else if (zid == "mon_zombie_hulk"){ dropset = 4;}
    switch(dropset)
    {
        case 0: // mon_zombie_cop
            g->m.put_items_from("cop_shoes", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            g->m.put_items_from("cop_torso", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            g->m.put_items_from("cop_pants", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
        break;

        case 1: // mon_zombie_swimmer
            if (one_in(10)) {
              //Wetsuit zombie
              g->m.put_items_from("swimmer_wetsuit", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1, 4));
            } else {
              if (!one_in(4)) {
                  g->m.put_items_from("swimmer_head", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1, 4));
              }
              if (one_in(3)) {
                  g->m.put_items_from("swimmer_torso", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1, 4));
              }
              g->m.put_items_from("swimmer_pants", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1, 4));
              if (one_in(4)) {
                  g->m.put_items_from("swimmer_shoes", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1, 4));
              }
            }
        break;

        case 2: // mon_zombie_scientist
            g->m.put_items_from("lab_shoes", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            g->m.put_items_from("lab_torso", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            g->m.put_items_from("lab_pants", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
        break;

        case 3: // mon_zombie_soldier
            g->m.put_items_from("cop_shoes", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            g->m.put_items_from("mil_armor_torso", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            g->m.put_items_from("mil_armor_pants", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            if (one_in(4))
            {
                g->m.put_items_from("mil_armor_helmet", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            }
        break;

        case 4: // mon_zombie_hulk
            g->m.spawn_item(z->posx(), z->posy(), "rag", g->turn, 0, 0, rng(5,10));
            g->m.put_items_from("pants", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            break;

        default:
            g->m.put_items_from("pants", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            g->m.put_items_from("shirts", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            if (one_in(6))
            {
                g->m.put_items_from("jackets", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            }
            if (one_in(15))
            {
                g->m.put_items_from("bags", 1, z->posx(), z->posy(), g->turn, 0, 0, rng(1,4));
            }
        break;
    }
}

void mdeath::gameover(game *g, monster *z)
{
 g->add_msg(_("Your %s is destroyed!  GAME OVER!"), z->name().c_str());
 g->u.hp_cur[hp_torso] = 0;
}

void mdeath::kill_breathers(game *g, monster *z)
{
 for (int i = 0; i < g->num_zombies(); i++) {
  if (g->zombie(i).type->id == "mon_breather_hub "|| g->zombie(i).type->id == "mon_breather")
   g->zombie(i).dead = true;
 }
}
