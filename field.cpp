#include "rng.h"
#include "map.h"
#include "game.h"

#define INBOUNDS(x, y) \
 (x >= 0 && x < SEEX * my_MAPSIZE && y >= 0 && y < SEEY * my_MAPSIZE)

bool vector_has(std::vector <item> vec, itype_id type);

bool map::process_fields(game *g)
{
 bool found_field = false;
 for (int x = 0; x < my_MAPSIZE; x++) {
  for (int y = 0; y < my_MAPSIZE; y++) {
   if (grid[x + y * my_MAPSIZE]->field_count > 0)
    found_field |= process_fields_in_submap(g, x + y * my_MAPSIZE);
  }
 }
 return found_field;
}

bool map::process_fields_in_submap(game *g, int gridn)
{
 bool found_field = false;
 field *cur;
 field_id curtype;
 for (int locx = 0; locx < SEEX; locx++) {
  for (int locy = 0; locy < SEEY; locy++) {
   cur = &(grid[gridn]->fld[locx][locy]);
   int x = locx + SEEX * (gridn % my_MAPSIZE),
       y = locy + SEEY * int(gridn / my_MAPSIZE);
   
   curtype = cur->type;
   if (!found_field && curtype != fd_null)
    found_field = true;
   if (cur->density > 3 || cur->density < 1)
    debugmsg("Whoooooa density of %d", cur->density);

  if (cur->age == 0)	// Don't process "newborn" fields
   curtype = fd_null;

  int part;
  vehicle *veh;
  switch (curtype) {

   case fd_null:
    break;	// Do nothing, obviously.  OBVIOUSLY.

   case fd_blood:
   case fd_bile:
    if (has_flag(swimmable, x, y))	// Dissipate faster in water
     cur->age += 250;
    break;

   case fd_acid:
    if (has_flag(swimmable, x, y))	// Dissipate faster in water
     cur->age += 20;
    for (int i = 0; i < i_at(x, y).size(); i++) {
     item *melting = &(i_at(x, y)[i]);
     if (melting->made_of(LIQUID) || melting->made_of(VEGGY)   ||
         melting->made_of(FLESH)  || melting->made_of(POWDER)  ||
         melting->made_of(COTTON) || melting->made_of(WOOL)    ||
         melting->made_of(PAPER)  || melting->made_of(PLASTIC) ||
         (melting->made_of(GLASS) && !one_in(3)) || one_in(4)) {
// Acid destructable objects here
      melting->damage++;
      if (melting->damage >= 5 ||
          (melting->made_of(PAPER) && melting->damage >= 3)) {
       cur->age += melting->volume();
       for (int m = 0; m < i_at(x, y)[i].contents.size(); m++)
        i_at(x, y).push_back( i_at(x, y)[i].contents[m] );
       i_at(x, y).erase(i_at(x, y).begin() + i);
       i--;
      }
     }
    }
    break;

   case fd_sap:
    break; // It doesn't do anything.

   case fd_fire: {
// Consume items as fuel to help us grow/last longer.
    bool destroyed = false;
    int vol = 0, smoke = 0, consumed = 0;
    for (int i = 0; i < i_at(x, y).size() && consumed < cur->density * 2; i++) {
     destroyed = false;
     vol = i_at(x, y)[i].volume();
     item *it = &(i_at(x, y)[i]);

     if (it->is_ammo() && it->ammo_type() != AT_BATT &&
         it->ammo_type() != AT_NAIL && it->ammo_type() != AT_BB &&
         it->ammo_type() != AT_BOLT && it->ammo_type() != AT_ARROW) {
      cur->age /= 2;
      cur->age -= 600;
      destroyed = true;
      smoke += 6;
      consumed++;

     } else if (it->made_of(PAPER)) {
      destroyed = it->burn(cur->density * 3);
      consumed++;
      if (cur->density == 1)
       cur->age -= vol * 10;
      if (vol >= 4)
       smoke++;

     } else if ((it->made_of(WOOD) || it->made_of(VEGGY))) {
      if (vol <= cur->density * 10 || cur->density == 3) {
       cur->age -= 4;
       destroyed = it->burn(cur->density);
       smoke++;
       consumed++;
      } else if (it->burnt < cur->density) {
       destroyed = it->burn(1);
       smoke++;
      }

     } else if ((it->made_of(COTTON) || it->made_of(WOOL))) {
      if (vol <= cur->density * 5 || cur->density == 3) {
       cur->age--;
       destroyed = it->burn(cur->density);
       smoke++;
       consumed++;
      } else if (it->burnt < cur->density) {
       destroyed = it->burn(1);
       smoke++;
      }

     } else if (it->made_of(FLESH)) {
      if (vol <= cur->density * 5 || (cur->density == 3 && one_in(vol / 20))) {
       cur->age--;
       destroyed = it->burn(cur->density);
       smoke += 3;
       consumed++;
      } else if (it->burnt < cur->density * 5 || cur->density >= 2) {
       destroyed = it->burn(1);
       smoke++;
      }

     } else if (it->made_of(LIQUID)) {
      switch (it->type->id) { // TODO: Make this be not a hack.
       case itm_whiskey:
       case itm_vodka:
       case itm_rum:
       case itm_tequila:
        cur->age -= 300;
        smoke += 6;
        break;
       default:
        cur->age += rng(80 * vol, 300 * vol);
        smoke++;
      }
      destroyed = true;
      consumed++;

     } else if (it->made_of(POWDER)) {
      cur->age -= vol;
      destroyed = true;
      smoke += 2;

     } else if (it->made_of(PLASTIC)) {
      smoke += 3;
      if (it->burnt <= cur->density * 2 || (cur->density == 3 && one_in(vol))) {
       destroyed = it->burn(cur->density);
       if (one_in(vol + it->burnt))
        cur->age--;
      }
     }

     if (destroyed) {
      for (int m = 0; m < i_at(x, y)[i].contents.size(); m++)
       i_at(x, y).push_back( i_at(x, y)[i].contents[m] );
      i_at(x, y).erase(i_at(x, y).begin() + i);
      i--;
     }
    }

    veh = veh_at(x, y, part);
    if (veh)
     veh->damage (part, cur->density * 10, false);
// Consume the terrain we're on
    if (has_flag(explodes, x, y)) {
     ter(x, y) = ter_id(int(ter(x, y)) + 1);
     cur->age = 0;
     cur->density = 3;
     g->explosion(x, y, 40, 0, true);

    } else if (has_flag(flammable, x, y) && one_in(32 - cur->density * 10)) {
     cur->age -= cur->density * cur->density * 40;
     smoke += 15;
     if (cur->density == 3)
      ter(x, y) = t_rubble;

    } else if (has_flag(l_flammable, x, y) && one_in(62 - cur->density * 10)) {
     cur->age -= cur->density * cur->density * 30;
     smoke += 10;
     if (cur->density == 3)
      ter(x, y) = t_rubble;

    } else if (terlist[ter(x, y)].flags & mfb(swimmable))
     cur->age += 800;	// Flames die quickly on water

// If we consumed a lot, the flames grow higher
    while (cur->density < 3 && cur->age < 0) {
     cur->age += 300;
     cur->density++;
    }
// If the flames are in a pit, it can't spread to non-pit
    bool in_pit = (ter(x, y) == t_pit);
// If the flames are REALLY big, they contribute to adjacent flames
    if (cur->density == 3 && cur->age < 0) {
// Randomly offset our x/y shifts by 0-2, to randomly pick a square to spread to
     int starti = rng(0, 2);
     int startj = rng(0, 2);
     for (int i = 0; i < 3 && cur->age < 0; i++) {
      for (int j = 0; j < 3 && cur->age < 0; j++) {
       int fx = x + ((i + starti) % 3) - 1, fy = y + ((j + startj) % 3) - 1;
       if (field_at(fx, fy).type == fd_fire && field_at(fx, fy).density < 3 &&
           (!in_pit || ter(fx, fy) == t_pit)) {
        field_at(fx, fy).density++; 
        field_at(fx, fy).age = 0;
        cur->age = 0;
       }
      }
     }
    }
// Consume adjacent fuel / terrain / webs to spread.
// Randomly offset our x/y shifts by 0-2, to randomly pick a square to spread to
    int starti = rng(0, 2);
    int startj = rng(0, 2);
    for (int i = 0; i < 3; i++) {
     for (int j = 0; j < 3; j++) {
      int fx = x + ((i + starti) % 3) - 1, fy = y + ((j + startj) % 3) - 1;
      if (INBOUNDS(fx, fy)) {
       int spread_chance = 20 * (cur->density - 1) + 10 * smoke;
       if (field_at(fx, fy).type == fd_web)
        spread_chance = 50 + spread_chance / 2;
       if (has_flag(explodes, fx, fy) && one_in(8 - cur->density)) {
        ter(fx, fy) = ter_id(int(ter(fx, fy)) + 1);
        g->explosion(fx, fy, 40, 0, true);
       } else if ((i != 0 || j != 0) && rng(1, 100) < spread_chance &&
                  (!in_pit || ter(fx, fy) == t_pit) &&
                  ((cur->density == 3 &&
                    (has_flag(flammable, fx, fy) || one_in(20))) ||
                   (cur->density == 3 &&
                    (has_flag(l_flammable, fx, fy) && one_in(10))) ||
                   flammable_items_at(fx, fy) ||
                   field_at(fx, fy).type == fd_web)) {
        if (field_at(fx, fy).type == fd_smoke ||
            field_at(fx, fy).type == fd_web)
         field_at(fx, fy) = field(fd_fire, 1, 0);
        else
         add_field(g, fx, fy, fd_fire, 1);
       } else {
        bool nosmoke = true;
        for (int ii = -1; ii <= 1; ii++) {
         for (int jj = -1; jj <= 1; jj++) {
          if (field_at(x+ii, y+jj).type == fd_fire &&
              field_at(x+ii, y+jj).density == 3)
           smoke++;
          else if (field_at(x+ii, y+jj).type == fd_smoke)
           nosmoke = false;
         }
        }
// If we're not spreading, maybe we'll stick out some smoke, huh?
        if (move_cost(fx, fy) > 0 &&
            (!one_in(smoke) || (nosmoke && one_in(40))) && 
            rng(3, 35) < cur->density * 10 && cur->age < 1000) {
         smoke--;
         add_field(g, fx, fy, fd_smoke, rng(1, cur->density));
        }
       }
      }
     }
    }
   } break;
  
   case fd_smoke:
    for (int i = -1; i <= 1; i++) {
     for (int j = -1; j <= 1; j++)
      g->scent(x+i, y+j) = 0;
    }
    if (is_outside(x, y))
     cur->age += 50;
    if (one_in(2)) {
     std::vector <point> spread;
     for (int a = -1; a <= 1; a++) {
      for (int b = -1; b <= 1; b++) {
       if ((field_at(x+a, y+b).type == fd_smoke &&
             field_at(x+a, y+b).density < 3       ) ||
           (field_at(x+a, y+b).is_null() && move_cost(x+a, y+b) > 0))
        spread.push_back(point(x+a, y+b));
      }
     }
     if (cur->density > 0 && cur->age > 0 && spread.size() > 0) {
      point p = spread[rng(0, spread.size() - 1)];
      if (field_at(p.x, p.y).type == fd_smoke &&
          field_at(p.x, p.y).density < 3) {
        field_at(p.x, p.y).density++;
        cur->density--;
      } else if (cur->density > 0 && move_cost(p.x, p.y) > 0 &&
                 add_field(g, p.x, p.y, fd_smoke, 1)){
       cur->density--;
       field_at(p.x, p.y).age = cur->age;
      }
     }
    }
   break;

   case fd_tear_gas:
// Reset nearby scents to zero
    for (int i = -1; i <= 1; i++) {
     for (int j = -1; j <= 1; j++)
      g->scent(x+i, y+j) = 0;
    }
    if (is_outside(x, y))
     cur->age += 30;
// One in three chance that it spreads (less than smoke!)
    if (one_in(3)) {
     std::vector <point> spread;
// Pick all eligible points to spread to
     for (int a = -1; a <= 1; a++) {
      for (int b = -1; b <= 1; b++) {
       if (((field_at(x+a, y+b).type == fd_smoke ||
             field_at(x+a, y+b).type == fd_tear_gas) &&
             field_at(x+a, y+b).density < 3            )      ||
           (field_at(x+a, y+b).is_null() && move_cost(x+a, y+b) > 0))
        spread.push_back(point(x+a, y+b));
      }
     }
// Then, spread to a nearby point
     if (cur->density > 0 && cur->age > 0 && spread.size() > 0) {
      point p = spread[rng(0, spread.size() - 1)];
// Nearby teargas grows thicker
      if (field_at(p.x, p.y).type == fd_tear_gas &&
          field_at(p.x, p.y).density < 3) {
        field_at(p.x, p.y).density++;
        cur->density--;
// Nearby smoke is converted into teargas
      } else if (field_at(p.x, p.y).type == fd_smoke) {
       field_at(p.x, p.y).type = fd_tear_gas;
// Or, just create a new field.
      } else if (cur->density > 0 && move_cost(p.x, p.y) > 0 &&
                 add_field(g, p.x, p.y, fd_tear_gas, 1)) {
       cur->density--;
       field_at(p.x, p.y).age = cur->age;
      }
     }
    }
    break;

   case fd_toxic_gas:
// Reset nearby scents to zero
    for (int i = -1; i <= 1; i++) {
     for (int j = -1; j <= 1; j++)
      g->scent(x+i, y+j) = 0;
    }
    if (is_outside(x, y))
     cur->age += 40;
    if (one_in(2)) {
     std::vector <point> spread;
// Pick all eligible points to spread to
     for (int a = -1; a <= 1; a++) {
      for (int b = -1; b <= 1; b++) {
       if (((field_at(x+a, y+b).type == fd_smoke ||
             field_at(x+a, y+b).type == fd_tear_gas ||
             field_at(x+a, y+b).type == fd_toxic_gas ||
             field_at(x+a, y+b).type == fd_nuke_gas   ) &&
             field_at(x+a, y+b).density < 3            )      ||
           (field_at(x+a, y+b).is_null() && move_cost(x+a, y+b) > 0))
        spread.push_back(point(x+a, y+b));
      }
     }
// Then, spread to a nearby point
     if (cur->density > 0 && cur->age > 0 && spread.size() > 0) {
      point p = spread[rng(0, spread.size() - 1)];
// Nearby toxic gas grows thicker
      if (field_at(p.x, p.y).type == fd_toxic_gas &&
          field_at(p.x, p.y).density < 3) {
        field_at(p.x, p.y).density++;
        cur->density--;
// Nearby smoke & teargas is converted into toxic gas
      } else if (field_at(p.x, p.y).type == fd_smoke ||
                 field_at(p.x, p.y).type == fd_tear_gas) {
       field_at(p.x, p.y).type = fd_toxic_gas;
// Or, just create a new field.
      } else if (cur->density > 0 && move_cost(p.x, p.y) > 0 &&
                 add_field(g, p.x, p.y, fd_toxic_gas, 1)) {
       cur->density--;
       field_at(p.x, p.y).age = cur->age;
      }
     }
    }
    break;


   case fd_nuke_gas:
// Reset nearby scents to zero
    for (int i = -1; i <= 1; i++) {
     for (int j = -1; j <= 1; j++)
      g->scent(x+i, y+j) = 0;
    }
    if (is_outside(x, y))
     cur->age += 40;
// Increase long-term radiation in the land underneath
    radiation(x, y) += rng(0, cur->density);
    if (one_in(2)) {
     std::vector <point> spread;
// Pick all eligible points to spread to
     for (int a = -1; a <= 1; a++) {
      for (int b = -1; b <= 1; b++) {
       if (((field_at(x+a, y+b).type == fd_smoke ||
             field_at(x+a, y+b).type == fd_tear_gas ||
             field_at(x+a, y+b).type == fd_toxic_gas ||
             field_at(x+a, y+b).type == fd_nuke_gas   ) &&
             field_at(x+a, y+b).density < 3            )      ||
           (field_at(x+a, y+b).is_null() && move_cost(x+a, y+b) > 0))
        spread.push_back(point(x+a, y+b));
      }
     }
// Then, spread to a nearby point
     if (cur->density > 0 && cur->age > 0 && spread.size() > 0) {
      point p = spread[rng(0, spread.size() - 1)];
// Nearby nukegas grows thicker
      if (field_at(p.x, p.y).type == fd_nuke_gas &&
          field_at(p.x, p.y).density < 3) {
        field_at(p.x, p.y).density++;
        cur->density--;
// Nearby smoke, tear, and toxic gas is converted into nukegas
      } else if (field_at(p.x, p.y).type == fd_smoke ||
                 field_at(p.x, p.y).type == fd_toxic_gas ||
                 field_at(p.x, p.y).type == fd_tear_gas) {
       field_at(p.x, p.y).type = fd_nuke_gas;
// Or, just create a new field.
      } else if (cur->density > 0 && move_cost(p.x, p.y) > 0 &&
                 add_field(g, p.x, p.y, fd_nuke_gas, 1)) {
       cur->density--;
       field_at(p.x, p.y).age = cur->age;
      }
     }
    }
    break;

   case fd_gas_vent:
    for (int i = x - 1; i <= x + 1; i++) {
     for (int j = y - 1; j <= y + 1; j++) {
      if (field_at(i, j).type == fd_toxic_gas && field_at(i, j).density < 3)
       field_at(i, j).density++;
      else
       add_field(g, i, j, fd_toxic_gas, 3);
     }
    }
    break;

   case fd_fire_vent:
    if (cur->density > 1) {
     if (one_in(3))
      cur->density--;
    } else {
     cur->type = fd_flame_burst;
     cur->density = 3;
    }
    break;

   case fd_flame_burst:
    if (cur->density > 1)
     cur->density--;
    else {
     cur->type = fd_fire_vent;
     cur->density = 3;
    }
    break;

   case fd_electricity:
    if (!one_in(5)) {	// 4 in 5 chance to spread
     std::vector<point> valid;
     if (move_cost(x, y) == 0 && cur->density > 1) { // We're grounded
      int tries = 0;
      while (tries < 10 && cur->age < 50) {
       int cx = x + rng(-1, 1), cy = y + rng(-1, 1);
       if (move_cost(cx, cy) != 0 && field_at(cx, cy).is_null()) {
        add_field(g, cx, cy, fd_electricity, 1);
        cur->density--;
        tries = 0;
       } else
        tries++;
      }
     } else {	// We're not grounded; attempt to ground
      for (int a = -1; a <= 1; a++) {
       for (int b = -1; b <= 1; b++) {
        if (move_cost(x + a, y + b) == 0 && // Grounded tiles first
            field_at(x + a, y + b).is_null())
         valid.push_back(point(x + a, y + b));
       }
      }
      if (valid.size() == 0) {	// Spread to adjacent space, then
       int px = x + rng(-1, 1), py = y + rng(-1, 1);
       if (move_cost(px, py) > 0 && field_at(px, py).type == fd_electricity &&
           field_at(px, py).density < 3)
        field_at(px, py).density++;
       else if (move_cost(px, py) > 0)
        add_field(g, px, py, fd_electricity, 1);
       cur->density--;
      }
      while (valid.size() > 0 && cur->density > 0) {
       int index = rng(0, valid.size() - 1);
       add_field(g, valid[index].x, valid[index].y, fd_electricity, 1);
       cur->density--;
       valid.erase(valid.begin() + index);
      }
     }
    }
    break;

   case fd_fatigue:
    if (cur->density < 3 && int(g->turn) % 3600 == 0 && one_in(10))
     cur->density++;
    else if (cur->density == 3 && one_in(600)) { // Spawn nether creature!
     mon_id type = mon_id(rng(mon_flying_polyp, mon_blank));
     monster creature(g->mtypes[type]);
     creature.spawn(x + rng(-3, 3), y + rng(-3, 3));
     g->z.push_back(creature);
    }
    break;

   case fd_push_items: {
    std::vector<item> *it = &(i_at(x, y));
    for (int i = 0; i < it->size(); i++) {
     if ((*it)[i].type->id != itm_rock || (*it)[i].bday >= int(g->turn) - 1)
      i++;
     else {
      item tmp = (*it)[i];
      tmp.bday = int(g->turn);
      it->erase(it->begin() + i);
      i--;
      std::vector<point> valid;
      for (int xx = x - 1; xx <= x + 1; xx++) {
       for (int yy = y - 1; yy <= y + 1; yy++) {
        if (field_at(xx, yy).type == fd_push_items)
         valid.push_back( point(xx, yy) );
       }
      }
      if (!valid.empty()) {
       point newp = valid[rng(0, valid.size() - 1)];
       add_item(newp.x, newp.y, tmp);
       if (g->u.posx == newp.x && g->u.posy == newp.y) {
        g->add_msg("A %s hits you!", tmp.tname().c_str());
        g->u.hit(g, random_body_part(), rng(0, 1), 6, 0);
       }
       int npcdex = g->npc_at(newp.x, newp.y),
           mondex = g->mon_at(newp.x, newp.y);

       if (npcdex != -1) {
        int junk;
        npc *p = &(g->active_npc[npcdex]);
        p->hit(g, random_body_part(), rng(0, 1), 6, 0);
        if (g->u_see(newp.x, newp.y, junk))
         g->add_msg("A %s hits %s!", tmp.tname().c_str(), p->name.c_str());
       }

       if (mondex != -1) {
        int junk;
        monster *mon = &(g->z[mondex]);
        mon->hurt(6 - mon->armor_bash());
        if (g->u_see(newp.x, newp.y, junk))
         g->add_msg("A %s hits the %s!", tmp.tname().c_str(),
                                         mon->name().c_str());
       }
      }
     }
    }
   } break;

   case fd_shock_vent:
    if (cur->density > 1) {
     if (one_in(5))
      cur->density--;
    } else {
     cur->density = 3;
     int num_bolts = rng(3, 6);
     for (int i = 0; i < num_bolts; i++) {
      int xdir = 0, ydir = 0;
      while (xdir == 0 && ydir == 0) {
       xdir = rng(-1, 1);
       ydir = rng(-1, 1);
      }
      int dist = rng(4, 12);
      int boltx = x, bolty = y;
      for (int n = 0; n < dist; n++) {
       boltx += xdir;
       bolty += ydir;
       add_field(g, boltx, bolty, fd_electricity, rng(2, 3));
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
    }
    break;

   case fd_acid_vent:
    if (cur->density > 1) {
     if (cur->age >= 10) {
      cur->density--;
      cur->age = 0;
     }
    } else {
     cur->density = 3;
     for (int i = x - 5; i <= x + 5; i++) {
      for (int j = y - 5; j <= y + 5; j++) {
       if (field_at(i, j).type == fd_null || field_at(i, j).density == 0) {
        int newdens = 3 - (rl_dist(x, y, i, j) / 2) + (one_in(3) ? 1 : 0);
        if (newdens > 3)
         newdens = 3;
        if (newdens > 0)
         add_field(g, i, j, fd_acid, newdens);
       }
      }
     }
    }
    break;

   } // switch (curtype)
  
   cur->age++;
   if (fieldlist[cur->type].halflife > 0) {
    if (cur->age > 0 &&
        dice(3, cur->age) > dice(3, fieldlist[cur->type].halflife)) {
     cur->age = 0;
     cur->density--;
    }
    if (cur->density <= 0) { // Totally dissapated.
     grid[gridn]->field_count--;
     grid[gridn]->fld[locx][locy] = field();
    }
   }
  }
 }
 return found_field;
}

void map::step_in_field(int x, int y, game *g)
{
 field *cur = &field_at(x, y);
 switch (cur->type) {
  case fd_null:
  case fd_blood:	// It doesn't actually do anything
  case fd_bile:		// Ditto
   return;

  case fd_web: {
   if (!g->u.has_trait(PF_WEB_WALKER)) {
    int web = cur->density * 5 - g->u.disease_level(DI_WEBBED);
    if (web > 0)
     g->u.add_disease(DI_WEBBED, web, g);
    remove_field(x, y);
   }
  } break;

  case fd_acid:
   if (cur->density == 3) {
    g->add_msg("The acid burns your legs and feet!");
    g->u.hit(g, bp_feet, 0, 0, rng(4, 10));
    g->u.hit(g, bp_feet, 1, 0, rng(4, 10));
    g->u.hit(g, bp_legs, 0, 0, rng(2,  8));
    g->u.hit(g, bp_legs, 1, 0, rng(2,  8));
   } else {
    g->add_msg("The acid burns your feet!");
    g->u.hit(g, bp_feet, 0, 0, rng(cur->density, 4 * cur->density));
    g->u.hit(g, bp_feet, 1, 0, rng(cur->density, 4 * cur->density));
   }
   break;

 case fd_sap:
  g->add_msg("The sap sticks to you!");
  g->u.add_disease(DI_SAP, cur->density * 2, g);
  if (cur->density == 1)
   remove_field(x, y);
  else
   cur->density--;
  break;

  case fd_fire:
   if (!g->u.has_active_bionic(bio_heatsink)) {
    if (cur->density == 1) {
     g->add_msg("You burn your legs and feet!");
     g->u.hit(g, bp_feet, 0, 0, rng(2, 6));
     g->u.hit(g, bp_feet, 1, 0, rng(2, 6));
     g->u.hit(g, bp_legs, 0, 0, rng(1, 4));
     g->u.hit(g, bp_legs, 1, 0, rng(1, 4));
    } else if (cur->density == 2) {
     g->add_msg("You're burning up!");
     g->u.hit(g, bp_legs, 0, 0,  rng(2, 6));
     g->u.hit(g, bp_legs, 1, 0,  rng(2, 6));
     g->u.hit(g, bp_torso, 0, 4, rng(4, 9));
    } else if (cur->density == 3) {
     g->add_msg("You're set ablaze!");
     g->u.hit(g, bp_legs, 0, 0, rng(2, 6));
     g->u.hit(g, bp_legs, 1, 0, rng(2, 6));
     g->u.hit(g, bp_torso, 0, 4, rng(4, 9));
     g->u.add_disease(DI_ONFIRE, 5, g);
    }
    if (cur->density == 2)
     g->u.infect(DI_SMOKE, bp_mouth, 5, 20, g);
    else if (cur->density == 3)
     g->u.infect(DI_SMOKE, bp_mouth, 7, 30, g);
   }
   break;

  case fd_smoke:
   if (cur->density == 3)
    g->u.infect(DI_SMOKE, bp_mouth, 4, 15, g);
   break;

  case fd_tear_gas:
   if (cur->density > 1 || !one_in(3))
    g->u.infect(DI_TEARGAS, bp_mouth, 5, 20, g);
   if (cur->density > 1)
    g->u.infect(DI_BLIND, bp_eyes, cur->density * 2, 10, g);
   break;

  case fd_toxic_gas:
   if (cur->density == 2)
    g->u.infect(DI_POISON, bp_mouth, 5, 30, g);
   else if (cur->density == 3)
    g->u.infect(DI_BADPOISON, bp_mouth, 5, 30, g);
   break;

  case fd_nuke_gas:
   g->u.radiation += rng(0, cur->density * (cur->density + 1));
   if (cur->density == 3) {
    g->add_msg("This radioactive gas burns!");
    g->u.hurtall(rng(1, 3));
   }
   break;

  case fd_flame_burst:
   if (!g->u.has_active_bionic(bio_heatsink)) {
    g->add_msg("You're torched by flames!");
    g->u.hit(g, bp_legs, 0, 0,  rng(2, 6));
    g->u.hit(g, bp_legs, 1, 0,  rng(2, 6));
    g->u.hit(g, bp_torso, 0, 4, rng(4, 9));
   } else
    g->add_msg("These flames do not burn you.");
   break;

  case fd_electricity:
   if (g->u.has_artifact_with(AEP_RESIST_ELECTRICITY))
    g->add_msg("The electricity flows around you.");
   else {
    g->add_msg("You're electrocuted!");
    g->u.hurtall(rng(1, cur->density));
    if (one_in(8 - cur->density) && !one_in(30 - g->u.str_cur)) {
     g->add_msg("You're paralyzed!");
     g->u.moves -= rng(cur->density * 50, cur->density * 150);
    }
   }
   break;

  case fd_fatigue:
   if (rng(0, 2) < cur->density) {
    g->add_msg("You're violently teleported!");
    g->u.hurtall(cur->density);
    g->teleport();
   }
   break;

  case fd_shock_vent:
  case fd_acid_vent:
   remove_field(x, y);
   break;
 }
}
    
void map::mon_in_field(int x, int y, game *g, monster *z)
{
 if (z->has_flag(MF_DIGS))
  return;	// Digging monsters are immune to fields
 field *cur = &field_at(x, y);
 int dam = 0;
 switch (cur->type) {
  case fd_null:
  case fd_blood:	// It doesn't actually do anything
  case fd_bile:		// Ditto
   break;

  case fd_web:
   if (!z->has_flag(MF_WEBWALK)) {
    z->speed *= .8;
    remove_field(x, y);
   }
   break;

  case fd_acid:
   if (!z->has_flag(MF_DIGS) && !z->has_flag(MF_FLIES) &&
       !z->has_flag(MF_ACIDPROOF)) {
    if (cur->density == 3)
     dam = rng(4, 10) + rng(2, 8);
    else
     dam = rng(cur->density, cur->density * 4);
   }
   break;

  case fd_sap:
   z->speed -= cur->density * 5;
   if (cur->density == 1)
    remove_field(x, y);
   else
    cur->density--;
   break;

  case fd_fire:
   if (z->made_of(FLESH))
    dam = 3;
   if (z->made_of(VEGGY))
    dam = 12;
   if (z->made_of(PAPER) || z->made_of(LIQUID) || z->made_of(POWDER) ||
       z->made_of(WOOD)  || z->made_of(COTTON) || z->made_of(WOOL))
    dam = 20;
   if (z->made_of(STONE) || z->made_of(KEVLAR) || z->made_of(STEEL))
    dam = -20;
   if (z->has_flag(MF_FLIES))
    dam -= 15;

   if (cur->density == 1)
    dam += rng(2, 6);
   else if (cur->density == 2) {
    dam += rng(6, 12);
    if (!z->has_flag(MF_FLIES)) {
     z->moves -= 20;
     if (!z->made_of(LIQUID) && !z->made_of(STONE) && !z->made_of(KEVLAR) &&
         !z->made_of(STEEL) && !z->has_flag(MF_FIREY))
      z->add_effect(ME_ONFIRE, rng(3, 8));
    }
   } else if (cur->density == 3) {
    dam += rng(10, 20);
    if (!z->has_flag(MF_FLIES) || one_in(3)) {
     z->moves -= 40;
     if (!z->made_of(LIQUID) && !z->made_of(STONE) && !z->made_of(KEVLAR) &&
         !z->made_of(STEEL) && !z->has_flag(MF_FIREY))
      z->add_effect(ME_ONFIRE, rng(8, 12));
    }
   }
// Drop through to smoke

  case fd_smoke:
   if (cur->density == 3)
    z->speed -= rng(10, 20);
   if (z->made_of(VEGGY))	// Plants suffer from smoke even worse
    z->speed -= rng(1, cur->density * 12);
   break;

  case fd_tear_gas:
   if (z->made_of(FLESH) || z->made_of(VEGGY)) {
    z->add_effect(ME_BLIND, cur->density * 8);
    if (cur->density == 3) {
     z->add_effect(ME_STUNNED, rng(10, 20));
     dam = rng(4, 10);
    } else if (cur->density == 2) {
     z->add_effect(ME_STUNNED, rng(5, 10));
     dam = rng(2, 5);
    } else
     z->add_effect(ME_STUNNED, rng(1, 5));
    if (z->made_of(VEGGY)) {
     z->speed -= rng(cur->density * 5, cur->density * 12);
     dam += cur->density * rng(8, 14);
    }
   }
   break;

  case fd_toxic_gas:
   dam = cur->density;
   z->speed -= cur->density;
   break;

  case fd_nuke_gas:
   if (cur->density == 3) {
    z->speed -= rng(60, 120);
    dam = rng(30, 50);
   } else if (cur->density == 2) {
    z->speed -= rng(20, 50);
    dam = rng(10, 25);
   } else {
    z->speed -= rng(0, 15);
    dam = rng(0, 12);
   }
   if (z->made_of(VEGGY)) {
    z->speed -= rng(cur->density * 5, cur->density * 12);
    dam *= cur->density;
   }
   break;

  case fd_flame_burst:
   if (z->made_of(FLESH))
    dam = 3;
   if (z->made_of(VEGGY))
    dam = 12;
   if (z->made_of(PAPER) || z->made_of(LIQUID) || z->made_of(POWDER) ||
       z->made_of(WOOD)  || z->made_of(COTTON) || z->made_of(WOOL))
    dam = 50;
   if (z->made_of(STONE) || z->made_of(KEVLAR) || z->made_of(STEEL))
    dam = -25;
   dam += rng(0, 8);
   z->moves -= 20;
   break;

  case fd_electricity:
   dam = rng(1, cur->density);
   if (one_in(8 - cur->density))
    z->moves -= cur->density * 150;
   break;

  case fd_fatigue:
   if (rng(0, 2) < cur->density) {
    dam = cur->density;
    int tries = 0;
    int newposx, newposy;
    do {
     newposx = rng(z->posx - SEEX, z->posx + SEEX);
     newposy = rng(z->posy - SEEY, z->posy + SEEY);
     tries++;
    } while (g->m.move_cost(newposx, newposy) == 0 && tries != 10);

    if (tries == 10)
     g->explode_mon(g->mon_at(z->posx, z->posy));
    else {
     int mon_hit = g->mon_at(newposx, newposy), t;
     if (mon_hit != -1) {
      if (g->u_see(z, t))
       g->add_msg("The %s teleports into a %s, killing them both!",
                  z->name().c_str(), g->z[mon_hit].name().c_str());
      g->explode_mon(mon_hit);
     } else {
      z->posx = newposx;
      z->posy = newposy;
     }
    }
   }
   break;
     
 }
 if (dam > 0)
  z->hurt(dam);
}

bool vector_has(std::vector <item> vec, itype_id type)
{
 for (int i = 0; i < vec.size(); i++) {
  if (vec[i].type->id == type)
   return true;
 }
 return false;
}
