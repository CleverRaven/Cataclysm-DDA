#include "map.h"
#include "lightmap.h"
#include "output.h"
#include "rng.h"
#include "game.h"
#include "line.h"
#include "options.h"
#include "mapbuffer.h"
#include "translations.h"
#include <cmath>
#include <stdlib.h>
#include <fstream>
#include "debug.h"
#include "item_factory.h"

#define SGN(a) (((a)<0) ? -1 : 1)
#define INBOUNDS(x, y) \
 (x >= 0 && x < SEEX * my_MAPSIZE && y >= 0 && y < SEEY * my_MAPSIZE)
#define dbg(x) dout((DebugLevel)(x),D_MAP) << __FILE__ << ":" << __LINE__ << ": "

enum astar_list {
 ASL_NONE,
 ASL_OPEN,
 ASL_CLOSED
};

map::map()
{
 nulter = t_null;
 nultrap = tr_null;
 if (is_tiny())
  my_MAPSIZE = 2;
 else
  my_MAPSIZE = MAPSIZE;
 dbg(D_INFO) << "map::map(): my_MAPSIZE: " << my_MAPSIZE;
 veh_in_active_range = true;
}

map::map(std::vector<trap*> *trptr)
{
 nulter = t_null;
 nultrap = tr_null;
 traps = trptr;
 if (is_tiny())
  my_MAPSIZE = 2;
 else
  my_MAPSIZE = MAPSIZE;
 for (int n = 0; n < my_MAPSIZE * my_MAPSIZE; n++)
  grid[n] = NULL;
 dbg(D_INFO) << "map::map( trptr["<<trptr<<"] ): my_MAPSIZE: " << my_MAPSIZE;
 veh_in_active_range = true;
 memset(veh_exists_at, 0, sizeof(veh_exists_at));
}

map::~map()
{
}

VehicleList map::get_vehicles(){
   return get_vehicles(0,0,SEEX*my_MAPSIZE, SEEY*my_MAPSIZE);
}

VehicleList map::get_vehicles(const int sx, const int sy, const int ex, const int ey)
{
 const int chunk_sx = (sx / SEEX) - 1;
 const int chunk_ex = (ex / SEEX) + 1;
 const int chunk_sy = (sy / SEEY) - 1;
 const int chunk_ey = (ey / SEEY) + 1;
 VehicleList vehs;

 for(int cx = chunk_sx; cx <= chunk_ex; ++cx) {
  for(int cy = chunk_sy; cy <= chunk_ey; ++cy) {
   const int nonant = cx + cy * my_MAPSIZE;
   if (nonant < 0 || nonant >= my_MAPSIZE * my_MAPSIZE)
    continue; // out of grid

   for(int i = 0; i < grid[nonant]->vehicles.size(); ++i) {
    wrapped_vehicle w;
    w.v = grid[nonant]->vehicles[i];
    w.x = w.v->posx + cx * SEEX;
    w.y = w.v->posy + cy * SEEY;
    w.i = cx;
    w.j = cy;
    vehs.push_back(w);
   }
  }
 }

 return vehs;
}

vehicle* map::veh_at(const int x, const int y, int &part_num)
{
 // This function is called A LOT. Move as much out of here as possible.
 if (!veh_in_active_range || !inbounds(x, y))
  return NULL;    // Out-of-bounds - null vehicle
 if(!veh_exists_at[x][y])
  return NULL;    // cache cache indicates no vehicle. This should optimize a great deal.
 std::pair<int,int> point(x,y);
 std::map< std::pair<int,int>, std::pair<vehicle*,int> >::iterator it;
 if ((it = veh_cached_parts.find(point)) != veh_cached_parts.end())
 {
  part_num = it->second.second;
  return it->second.first;
 }
 debugmsg ("vehicle part cache cache indicated vehicle not found: %d %d",x,y);
 return NULL;
}

vehicle* map::veh_at(const int x, const int y)
{
 int part = 0;
 vehicle *veh = veh_at(x, y, part);
 return veh;
}

void map::reset_vehicle_cache()
{
 clear_vehicle_cache();
 // Cache all vehicles
 veh_in_active_range = false;
 for( std::set<vehicle*>::iterator veh = vehicle_list.begin(),
   it_end = vehicle_list.end(); veh != it_end; ++veh ) {
  update_vehicle_cache(*veh, true);
 }
}

void map::update_vehicle_cache(vehicle * veh, const bool brand_new)
{
 veh_in_active_range = true;
 if(!brand_new){
 // Existing must be cleared
  std::map< std::pair<int,int>, std::pair<vehicle*,int> >::iterator it =
             veh_cached_parts.begin(), end = veh_cached_parts.end(), tmp;
  while( it != end ) {
   if( it->second.first == veh ) {
    int x = it->first.first;
    int y = it->first.second;
    if ((x > 0) && (y > 0) &&
         x < SEEX*MAPSIZE &&
         y < SEEY*MAPSIZE){
     veh_exists_at[x][y] = false;
    }
    tmp = it;
    ++it;
    veh_cached_parts.erase( tmp );
   }else
    ++it;
  }
 }
 // Get parts
 std::vector<vehicle_part> & parts = veh->parts;
 const int gx = veh->global_x();
 const int gy = veh->global_y();
 int partid = 0;
 for( std::vector<vehicle_part>::iterator it = parts.begin(),
   end = parts.end(); it != end; ++it, ++partid ) {
  const int px = gx + it->precalc_dx[0];
  const int py = gy + it->precalc_dy[0];
  veh_cached_parts.insert( std::make_pair( std::make_pair(px,py),
                                        std::make_pair(veh,partid) ));
  if ((px > 0) && (py > 0) &&
       px < SEEX*MAPSIZE &&
       py < SEEY*MAPSIZE){
   veh_exists_at[px][py] = true;
  }
 }
}

void map::clear_vehicle_cache()
{
 std::map< std::pair<int,int>, std::pair<vehicle*,int> >::iterator part;
 while( veh_cached_parts.size() ) {
  part = veh_cached_parts.begin();
  int x = part->first.first;
  int y = part->first.second;
  if ((x > 0) && (y > 0) &&
       x < SEEX*MAPSIZE &&
       y < SEEY*MAPSIZE){
   veh_exists_at[x][y] = false;
  }
  veh_cached_parts.erase(part);
 }
}

void map::update_vehicle_list(const int to) {
 // Update vehicle data
 for( std::vector<vehicle*>::iterator it = grid[to]->vehicles.begin(),
      end = grid[to]->vehicles.end(); it != end; ++it ) {
   vehicle_list.insert(*it);
 }
}

void map::board_vehicle(game *g, int x, int y, player *p)
{
 if (!p) {
  debugmsg ("map::board_vehicle: null player");
  return;
 }

 int part = 0;
 vehicle *veh = veh_at(x, y, part);
 if (!veh) {
  debugmsg ("map::board_vehicle: vehicle not found");
  return;
 }

 const int seat_part = veh->part_with_feature (part, vpf_boardable);
 if (part < 0) {
  debugmsg ("map::board_vehicle: boarding %s (not boardable)",
            veh->part_info(part).name);
  return;
 }
 if (veh->parts[seat_part].has_flag(vehicle_part::passenger_flag)) {
  player *psg = veh->get_passenger (seat_part);
  debugmsg ("map::board_vehicle: passenger (%s) is already there",
            psg ? psg->name.c_str() : "<null>");
  return;
 }
 veh->parts[seat_part].set_flag(vehicle_part::passenger_flag);
 veh->parts[seat_part].passenger_id = p->getID();

 p->posx = x;
 p->posy = y;
 p->in_vehicle = true;
 if (p == &g->u &&
     (x < SEEX * int(my_MAPSIZE / 2) || y < SEEY * int(my_MAPSIZE / 2) ||
      x >= SEEX * (1 + int(my_MAPSIZE / 2)) ||
      y >= SEEY * (1 + int(my_MAPSIZE / 2))   ))
  g->update_map(x, y);
}

void map::unboard_vehicle(game *g, const int x, const int y)
{
 int part = 0;
 vehicle *veh = veh_at(x, y, part);
 if (!veh) {
  debugmsg ("map::unboard_vehicle: vehicle not found");
  return;
 }
 const int seat_part = veh->part_with_feature (part, vpf_boardable, false);
 if (part < 0) {
  debugmsg ("map::unboard_vehicle: unboarding %s (not boardable)",
            veh->part_info(part).name);
  return;
 }
 player *psg = veh->get_passenger(seat_part);
 if (!psg) {
  debugmsg ("map::unboard_vehicle: passenger not found");
  return;
 }
 psg->in_vehicle = false;
 psg->driving_recoil = 0;
 psg->controlling_vehicle = false;
 veh->parts[seat_part].remove_flag(vehicle_part::passenger_flag);
 veh->skidding = true;
}

void map::destroy_vehicle (vehicle *veh)
{
 if (!veh) {
  debugmsg("map::destroy_vehicle was passed NULL");
  return;
 }
 const int veh_sm = veh->smx + veh->smy * my_MAPSIZE;
 for (int i = 0; i < grid[veh_sm]->vehicles.size(); i++) {
  if (grid[veh_sm]->vehicles[i] == veh) {
   vehicle_list.erase(veh);
   reset_vehicle_cache();
   grid[veh_sm]->vehicles.erase (grid[veh_sm]->vehicles.begin() + i);
   return;
  }
 }
 debugmsg ("destroy_vehicle can't find it! sm=%d", veh_sm);
}

bool map::displace_vehicle (game *g, int &x, int &y, const int dx, const int dy, bool test=false)
{
 const int x2 = x + dx;
 const int y2 = y + dy;
 int srcx = x;
 int srcy = y;
 int dstx = x2;
 int dsty = y2;

 if (!inbounds(srcx, srcy)){
  if (g->debugmon)
   debugmsg ("map::displace_vehicle: coords out of bounds %d,%d->%d,%d",
            srcx, srcy, dstx, dsty);
  return false;
 }

 const int src_na = int(srcx / SEEX) + int(srcy / SEEY) * my_MAPSIZE;
 srcx %= SEEX;
 srcy %= SEEY;

 const int dst_na = int(dstx / SEEX) + int(dsty / SEEY) * my_MAPSIZE;
 dstx %= SEEX;
 dsty %= SEEY;

 if (test)
  return src_na != dst_na;

 // first, let's find our position in current vehicles vector
 int our_i = -1;
 for (int i = 0; i < grid[src_na]->vehicles.size(); i++) {
  if (grid[src_na]->vehicles[i]->posx == srcx &&
      grid[src_na]->vehicles[i]->posy == srcy) {
   our_i = i;
   break;
  }
 }
 if (our_i < 0) {
  if (g->debugmon)
   debugmsg ("displace_vehicle our_i=%d", our_i);
  return false;
 }
 // move the vehicle
 vehicle *veh = grid[src_na]->vehicles[our_i];
 // don't let it go off grid
 if (!inbounds(x2, y2)){
  veh->stop();
   // Silent debug
  dbg(D_ERROR) << "map:displace_vehicle: Stopping vehicle, displaced dx=" << dx << ", dy=" << dy;
   // debugmon'd on screen
  if (g->debugmon) debugmsg ("stopping vehicle, displaced dx=%d, dy=%d", dx,dy);
  return false;
 }

    // record every passenger inside
 std::vector<int> psg_parts = veh->boarded_parts();
 std::vector<player *> psgs;
 for (int p = 0; p < psg_parts.size(); p++)
  psgs.push_back (veh->get_passenger (psg_parts[p]));

 const int rec = abs(veh->velocity) / 5 / 100;

 bool need_update = false;
 int upd_x, upd_y;
 // move passengers
 for (int i = 0; i < psg_parts.size(); i++) {
  player *psg = psgs[i];
  const int p = psg_parts[i];
  if (!psg) {
   debugmsg ("empty passenger part %d pcoord=%d,%d u=%d,%d?", p,
             veh->global_x() + veh->parts[p].precalc_dx[0],
             veh->global_y() + veh->parts[p].precalc_dy[0],
                      g->u.posx, g->u.posy);
   continue;
  }
  int trec = rec -psgs[i]->skillLevel("driving");
  if (trec < 0) trec = 0;
  // add recoil
  psg->driving_recoil = rec;
  // displace passenger taking in account vehicle movement (dx, dy)
  // and turning: precalc_dx/dy [0] contains previous frame direction,
  // and precalc_dx/dy[1] should contain next direction
  psg->posx += dx + veh->parts[p].precalc_dx[1] - veh->parts[p].precalc_dx[0];
  psg->posy += dy + veh->parts[p].precalc_dy[1] - veh->parts[p].precalc_dy[0];
  if (psg == &g->u) { // if passenger is you, we need to update the map
   need_update = true;
   upd_x = psg->posx;
   upd_y = psg->posy;
  }
 }
 for (int p = 0; p < veh->parts.size(); p++) {
  veh->parts[p].precalc_dx[0] = veh->parts[p].precalc_dx[1];
  veh->parts[p].precalc_dy[0] = veh->parts[p].precalc_dy[1];
 }

 veh->posx = dstx;
 veh->posy = dsty;
 if (src_na != dst_na) {
  vehicle * veh1 = veh;
  veh1->smx = int(x2 / SEEX);
  veh1->smy = int(y2 / SEEY);
  grid[dst_na]->vehicles.push_back (veh1);
  grid[src_na]->vehicles.erase (grid[src_na]->vehicles.begin() + our_i);
 }

 x += dx;
 y += dy;

 update_vehicle_cache(veh);

 bool was_update = false;
 if (need_update &&
     (upd_x < SEEX * int(my_MAPSIZE / 2) || upd_y < SEEY *int(my_MAPSIZE / 2) ||
      upd_x >= SEEX * (1+int(my_MAPSIZE / 2)) ||
      upd_y >= SEEY * (1+int(my_MAPSIZE / 2))   )) {
// map will shift, so adjust vehicle coords we've been passed
  if (upd_x < SEEX * int(my_MAPSIZE / 2))
   x += SEEX;
  else if (upd_x >= SEEX * (1+int(my_MAPSIZE / 2)))
   x -= SEEX;
  if (upd_y < SEEY * int(my_MAPSIZE / 2))
   y += SEEY;
  else if (upd_y >= SEEY * (1+int(my_MAPSIZE / 2)))
   y -= SEEY;
  g->update_map(upd_x, upd_y);
  was_update = true;
 }

 return (src_na != dst_na) || was_update;
}

void map::vehmove(game *g)
{
   // give vehicles movement points
   {
      VehicleList vehs = g->m.get_vehicles();
      for(int v = 0; v < vehs.size(); ++v) {
         vehicle* veh = vehs[v].v;
         veh->gain_moves (abs (veh->velocity));
      }
   }

   int count = 0;
   while(vehproceed(g)){
      count++;// lots of movement stuff. maybe 10 is low for collisions.
      if (count > 10)
         break;
   }
}

// find veh with the most amt of turn remaining, and move it a bit.
// proposal:
//  move it at most, a tenth of a turn, and at least one square.
bool map::vehproceed(game* g){
   VehicleList vehs = g->m.get_vehicles();
   vehicle* veh = NULL;
   float max_of_turn = 0;
   int x; int y;
   for(int v = 0; v < vehs.size(); ++v) {
      if(vehs[v].v->of_turn > max_of_turn){
         veh = vehs[v].v;
         x = vehs[v].x;
         y = vehs[v].y;
         max_of_turn = veh->of_turn;
      }
   }
   if(!veh)
      return false;

   if (!inbounds(x, y)){
    if (g->debugmon)
     debugmsg ("stopping out-of-map vehicle. (x,y)=(%d,%d)",x,y);
      veh->stop();
      veh->of_turn = 0;
      return true;
   }

   bool pl_ctrl = veh->player_in_control(&g->u);

   // k slowdown first.
   int slowdown = veh->skidding? 200 : 20; // mph lost per tile when coasting
   float kslw = (0.1 + veh->k_dynamics()) / ((0.1) + veh->k_mass());
   slowdown = (int) ceil(kslw * slowdown);
   if (abs(slowdown) > abs(veh->velocity))
      veh->stop();
   else if (veh->velocity < 0)
      veh->velocity += slowdown;
   else
      veh->velocity -= slowdown;

   if (veh->velocity && abs(veh->velocity) < 20) //low enough for bicycles to go in reverse.
      veh->stop();

   if(veh->velocity == 0) {
      veh->of_turn -= .321f;
      return true;
   }

   { // sink in water?
      int num_wheels = 0, submerged_wheels = 0;
      for (int ep = 0; ep < veh->external_parts.size(); ep++) {
         const int p = veh->external_parts[ep];
         if (veh->part_flag(p, vpf_wheel)){
            num_wheels++;
            const int px = x + veh->parts[p].precalc_dx[0];
            const int py = y + veh->parts[p].precalc_dy[0];
            if(move_cost_ter_furn(px, py) == 0) // deep water
               submerged_wheels++;
         }
      }
      // submerged wheels threshold is 2/3.
      if (num_wheels &&  (float)submerged_wheels / num_wheels > .666){
         g->add_msg(_("Your %s sank."), veh->name.c_str());
         if (pl_ctrl)
            veh->unboard_all ();
         // destroy vehicle (sank to nowhere)
         destroy_vehicle(veh);
         return true;
      }
   }
   // One-tile step take some of movement
   //  terrain cost is 1000 on roads.
   // This is stupid btw, it makes veh magically seem
   //  to accelerate when exiting rubble areas.
   float ter_turn_cost = 500.0 * move_cost_ter_furn (x,y) / abs(veh->velocity);

   //can't afford it this turn?
   if(ter_turn_cost >= veh->of_turn){
      veh->of_turn_carry = veh->of_turn;
      veh->of_turn = 0;
      return true;
   }

   veh->of_turn -= ter_turn_cost;

   // if not enough wheels, mess up the ground a bit.
   if (!veh->valid_wheel_config()) {
      veh->velocity += veh->velocity < 0 ? 2000 : -2000;
      for (int ep = 0; ep < veh->external_parts.size(); ep++) {
         const int p = veh->external_parts[ep];
         const int px = x + veh->parts[p].precalc_dx[0];
         const int py = y + veh->parts[p].precalc_dy[0];
         const ter_id &pter = ter(px, py);
         if (pter == t_dirt || pter == t_grass)
            ter_set(px, py, t_dirtmound);
      }
   }

   if (veh->skidding){
      if (one_in(4)){ // might turn uncontrollably while skidding
          veh->turn (one_in(2) ? -15 : 15);
      }
   }
   else if (pl_ctrl && rng(0, 4) > g->u.skillLevel("driving") && one_in(20)) {
      g->add_msg(_("You fumble with the %s's controls."), veh->name.c_str());
      veh->turn (one_in(2) ? -15 : 15);
   }
   // eventually send it skidding if no control
   if (!veh->boarded_parts().size() && one_in (10))
      veh->skidding = true;
   tileray mdir; // the direction we're moving
   if (veh->skidding) // if skidding, it's the move vector
      mdir = veh->move;
   else if (veh->turn_dir != veh->face.dir())
      mdir.init (veh->turn_dir); // driver turned vehicle, get turn_dir
   else
      mdir = veh->face;          // not turning, keep face.dir
   mdir.advance (veh->velocity < 0? -1 : 1);
   const int dx = mdir.dx();           // where do we go
   const int dy = mdir.dy();           // where do we go
   bool can_move = true;
   // calculate parts' mount points @ next turn (put them into precalc[1])
   veh->precalc_mounts(1, veh->skidding ? veh->turn_dir : mdir.dir());

   int imp = 0;

   std::vector<veh_collision> veh_veh_colls;

   if (veh->velocity == 0)
      can_move = false;
   // find collisions
   for (int ep = 0; ep < veh->external_parts.size() && can_move; ep++) {
      const int p = veh->external_parts[ep];
      // coords of where part will go due to movement (dx/dy)
      // and turning (precalc_dx/dy [1])
      const int dsx = x + dx + veh->parts[p].precalc_dx[1];
      const int dsy = y + dy + veh->parts[p].precalc_dy[1];
      veh_collision coll = veh->part_collision (x, y, p, dsx, dsy);
      if(coll.type == veh_coll_veh)
         veh_veh_colls.push_back(coll);
      else if (coll.type != veh_coll_nothing){ //run over someone?
         if (can_move)
            imp += coll.imp;
         if (veh->velocity == 0)
            can_move = false;
      }
   }

   if(veh_veh_colls.size()){ // we have dynamic crap!
      // effects of colliding with another vehicle:
      // transfers of momentum, skidding,
      // parts are damaged/broken on both sides,
      // remaining times are normalized,
      veh_collision c = veh_veh_colls[0];
      vehicle* veh2 = (vehicle*) c.target;
      g->add_msg(_("The %1$s's %2$s collides with the %3$s's %4$s."),
                 veh->name.c_str(),  veh->part_info(c.part).name,
                veh2->name.c_str(), veh2->part_info(c.target_part).name);

      // for reference, a cargo truck weighs ~25300, a bicycle 690,
      //  and 38mph is 3800 'velocity'
      rl_vec2d velo_veh1 = veh->velo_vec();
      rl_vec2d velo_veh2 = veh2->velo_vec();
      float m1 = veh->total_mass();
      float m2 = veh2->total_mass();

      rl_vec2d collision_axis = (velo_veh1 - velo_veh2).normalized();
      // imp? & delta? & final? reworked:
      // newvel1 =( vel1 * ( mass1 - mass2 ) + ( 2 * mass2 * vel2 ) ) / ( mass1 + mass2 )
      // as per http://en.wikipedia.org/wiki/Elastic_collision
      // impulse vectors
      rl_vec2d imp1 = collision_axis   *    collision_axis.dot_product (velo_veh1) * ( m1 ) * 2;
      rl_vec2d imp2 = (collision_axis) * (-collision_axis).dot_product (velo_veh2) * ( m2 ) * 2;
      // finally, changes in veh velocity
      // 30% is absorbed as bashing damage??

      rl_vec2d delta1 = (imp2 * .7f); // todo: get remaing kinetic energy, convert to damage
      rl_vec2d delta2 = (imp1 * .7f); // by adding imp
      rl_vec2d final1 = ( ( velo_veh1 * ( m1 - m2 ) ) + delta1 ) / ( m1 + m2 );
      rl_vec2d final2 = ( ( velo_veh2 * ( m2 - m1 ) ) + delta2 ) / ( m1 + m2 );

      veh->move.init (final1.x, final1.y);
      veh->velocity = final1.norm();
      // shrug it off if the change is less than 8mph.
      if(delta1.norm() > 800) {
         veh->skidding = 1;
      }
      veh2->move.init(final2.x, final2.y);
      veh2->velocity = final2.norm();
      if(delta2.norm() > 800) {
         veh2->skidding = 1;
      }
#ifdef testng_vehicle_collisions
      debugmsg("C(%d): %s (%.0f): %.0f => %.0f // %s (%.0f): %.0f => %.0f",
        (int)g->turn,
        veh->name.c_str(), m1, velo_veh1.norm(), final1.norm(),
        veh2->name.c_str(), m2, velo_veh2.norm(), final2.norm()
      );
#endif
      //give veh2 the initiative to proceed next before veh1
      float avg_of_turn = (veh2->of_turn + veh->of_turn) / 2;
      if(avg_of_turn < .1f)
         avg_of_turn = .1f;
      veh->of_turn = avg_of_turn * .9;
      veh2->of_turn = avg_of_turn * 1.1;
      return true;
   }

   int coll_turn = 0;
   if (imp > 0) { // imp == impulse from collisions
      // debugmsg ("collision imp=%d dam=%d-%d", imp, imp/10, imp/6);
      if (imp > 100)
         veh->damage_all(imp / 20, imp / 10, 1);// shake veh because of collision
      std::vector<int> ppl = veh->boarded_parts();
      const int vel2 = imp * k_mvel * 100 / (veh->total_mass() / 2);
      for (int ps = 0; ps < ppl.size(); ps++) {
         player *psg = veh->get_passenger (ppl[ps]);
         if (!psg) {
            debugmsg ("throw passenger: empty passenger at part %d", ppl[ps]);
            continue;
         }
         const int throw_roll = rng (vel2/100, vel2/100 * 2);
         const int psblt = veh->part_with_feature (ppl[ps], vpf_seatbelt);
         const int sb_bonus = psblt >= 0? veh->part_info(psblt).bonus : 0;
         bool throw_from_seat = throw_roll > (psg->str_cur + sb_bonus) * 3;

         if (throw_from_seat) {
            if (psg == &g->u) {
                g->add_msg(_("You are hurled from the %s's seat by the power of the impact!"), veh->name.c_str());
            } else if (psg->name.length()) {
                g->add_msg(_("%s is hurled from the %s's seat by the power of the impact!"), psg->name.c_str(), veh->name.c_str());
            }
            g->m.unboard_vehicle(g, x + veh->parts[ppl[ps]].precalc_dx[0],
                  y + veh->parts[ppl[ps]].precalc_dy[0]);
            g->fling_player_or_monster(psg, 0, mdir.dir() + rng(0, 60) - 30,
                  (vel2/100 - sb_bonus < 10 ? 10 :
                   vel2/100 - sb_bonus));
         } else if (veh->part_with_feature (ppl[ps], vpf_controls) >= 0) {
            // FIXME: should actually check if passenger is in control,
            // not just if there are controls there.
            const int lose_ctrl_roll = rng (0, imp);
            if (lose_ctrl_roll > psg->dex_cur * 2 + psg->skillLevel("driving") * 3) {
               if (psg == &g->u) {
                   g->add_msg(_("You lose control of the %s."), veh->name.c_str());
               } else if (psg->name.length()) {
                   g->add_msg(_("%s loses control of the %s."), psg->name.c_str());
               }
               int turn_amount = (rng (1, 3) * sqrt((double)vel2) / 2) / 15;
               if (turn_amount < 1)
                  turn_amount = 1;
               turn_amount *= 15;
               if (turn_amount > 120)
                  turn_amount = 120;
               //veh->skidding = true;
               //veh->turn (one_in (2)? turn_amount : -turn_amount);
               coll_turn = one_in (2)? turn_amount : -turn_amount;
            }
         }
      }
   }
   // now we're gonna handle traps we're standing on (if we're still moving).
   // this is done here before displacement because
   // after displacement veh reference would be invdalid.
   // damn references!
   if (can_move) {
      for (int ep = 0; ep < veh->external_parts.size(); ep++) {
         const int p = veh->external_parts[ep];
         if (veh->part_flag(p, vpf_wheel) && one_in(2))
            if (displace_water (x + veh->parts[p].precalc_dx[0], y + veh->parts[p].precalc_dy[0]) && pl_ctrl)
               g->add_msg(_("You hear a splash!"));
         veh->handle_trap(x + veh->parts[p].precalc_dx[0],
               y + veh->parts[p].precalc_dy[0], p);
      }
   }

   int last_turn_dec = 1;
   if (veh->last_turn < 0) {
      veh->last_turn += last_turn_dec;
      if (veh->last_turn > -last_turn_dec)
         veh->last_turn = 0;
   } else if (veh->last_turn > 0) {
      veh->last_turn -= last_turn_dec;
      if (veh->last_turn < last_turn_dec)
         veh->last_turn = 0;
   }

   if (can_move) {
      // accept new direction
      if (veh->skidding){
         veh->face.init (veh->turn_dir);
         if(pl_ctrl)
         {
             veh->possibly_recover_from_skid();
         }
      }
      else
         veh->face = mdir;
      veh->move = mdir;
      if (coll_turn) {
         veh->skidding = true;
         veh->turn (coll_turn);
      }
      // accept new position
      // if submap changed, we need to process grid from the beginning.
      displace_vehicle (g, x, y, dx, dy);
   } else { // can_move
      veh->stop();
   }
   // redraw scene
   g->draw();
   return true;
}

bool map::displace_water (const int x, const int y)
{
    if (move_cost_ter_furn(x, y) > 0 && has_flag(swimmable, x, y)) // shallow water
    { // displace it
        int dis_places = 0, sel_place = 0;
        for (int pass = 0; pass < 2; pass++)
        { // we do 2 passes.
        // first, count how many non-water places around
        // then choose one within count and fill it with water on second pass
            if (pass)
            {
                sel_place = rng (0, dis_places - 1);
                dis_places = 0;
            }
            for (int tx = -1; tx <= 1; tx++)
                for (int ty = -1; ty <= 1; ty++)
                {
                    if ((!tx && !ty) || move_cost_ter_furn(x + tx, y + ty) == 0)
                        continue;
                    ter_id ter0 = ter (x + tx, y + ty);
                    if (ter0 == t_water_sh ||
                        ter0 == t_water_dp)
                        continue;
                    if (pass && dis_places == sel_place)
                    {
                       ter_set(x + tx, y + ty, t_water_sh);
                       ter_set(x, y, t_dirt);
                        return true;
                    }
                    dis_places++;
                }
        }
    }
    return false;
}

void map::set(const int x, const int y, const ter_id new_terrain, const furn_id new_furniture)
{
 furn_set(x, y, new_furniture);
	ter_set(x, y, new_terrain);
}

std::string map::name(const int x, const int y)
{
 return has_furn(x, y) ? furnlist[furn(x, y)].name : terlist[ter(x, y)].name;
}

bool map::has_furn(const int x, const int y)
{
  return furn(x, y) != f_null;
}

furn_id map::furn(const int x, const int y)
{
 if (!INBOUNDS(x, y)) {
  return f_null;
 }

 const int nonant = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE;

 const int lx = x % SEEX;
 const int ly = y % SEEY;
 return grid[nonant]->frn[lx][ly];
}

void map::furn_set(const int x, const int y, const furn_id new_furniture)
{
 if (!INBOUNDS(x, y)) {
  return;
 }

 const int nonant = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE;

 const int lx = x % SEEX;
 const int ly = y % SEEY;
 grid[nonant]->frn[lx][ly] = new_furniture;
}

std::string map::furnname(const int x, const int y)
{
 return furnlist[furn(x, y)].name;
}

ter_id map::ter(const int x, const int y) const
{
 if (!INBOUNDS(x, y)) {
  return t_null;
 }

 const int nonant = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE;

 const int lx = x % SEEX;
 const int ly = y % SEEY;
 return grid[nonant]->ter[lx][ly];
}

void map::ter_set(const int x, const int y, const ter_id new_terrain)
{
 if (!INBOUNDS(x, y)) {
  return;
 }

 const int nonant = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE;

 const int lx = x % SEEX;
 const int ly = y % SEEY;
 grid[nonant]->ter[lx][ly] = new_terrain;
}

std::string map::tername(const int x, const int y) const
{
 return terlist[ter(x, y)].name;
}

std::string map::features(const int x, const int y)
{
// This is used in an info window that is 46 characters wide, and is expected
// to take up one line.  So, make sure it does that.
// FIXME: can't control length of localized text.
// Make the caller wrap properly, if it does not already.
 std::string ret;
 if (has_flag(bashable, x, y))
  ret += _("Smashable. ");
 if (has_flag(diggable, x, y))
  ret += _("Diggable. ");
 if (has_flag(rough, x, y))
  ret += _("Rough. ");
 if (has_flag(sharp, x, y))
  ret += _("Sharp. ");
 if (has_flag(flat, x, y))
  ret += _("Flat. ");
 return ret;
}

int map::move_cost(const int x, const int y)
{
 int vpart = -1;
 vehicle *veh = veh_at(x, y, vpart);
 if (veh) {  // moving past vehicle cost
  const int dpart = veh->part_with_feature(vpart, vpf_obstacle);
  if (dpart >= 0 && (!veh->part_flag(dpart, vpf_openable) || !veh->parts[dpart].open)) {
   return 0;
  } else {
    const int ipart = veh->part_with_feature(vpart, vpf_aisle);
    if (ipart >= 0)
      return 2;
   return 8;
  }
 }
 int cost = terlist[ter(x, y)].movecost + furnlist[furn(x, y)].movecost;
 cost+= field_at(x,y).move_cost();
 return cost > 0 ? cost : 0;
}

int map::move_cost_ter_furn(const int x, const int y)
{
 int cost = terlist[ter(x, y)].movecost + furnlist[furn(x, y)].movecost;
 return cost > 0 ? cost : 0;
}

int map::combined_movecost(const int x1, const int y1,
                           const int x2, const int y2)
{
    int cost1 = move_cost(x1, y1);
    int cost2 = move_cost(x2, y2);
    // 50 moves taken per move_cost (70.71.. diagonally)
    int mult = (trigdist && x1 != x2 && y1 != y2 ? 71 : 50);
    return (cost1 + cost2) * mult / 2;
}

bool map::trans(const int x, const int y)
{
 // Control statement is a problem. Normally returning false on an out-of-bounds
 // is how we stop rays from going on forever.  Instead we'll have to include
 // this check in the ray loop.
 int vpart = -1;
 vehicle *veh = veh_at(x, y, vpart);
 bool tertr;
 if (veh) {
  tertr = !veh->part_flag(vpart, vpf_opaque) || veh->parts[vpart].hp <= 0;
  if (!tertr) {
   const int dpart = veh->part_with_feature(vpart, vpf_openable);
   if (dpart >= 0 && veh->parts[dpart].open)
    tertr = true; // open opaque door
  }
 } else
  tertr = terlist[ter(x, y)].flags & mfb(transparent);
 if( tertr ){
  // Fields may obscure the view, too
	 field &curfield = field_at(x,y);
	 if(curfield.fieldCount() > 0){
	 field_entry *cur = NULL;
	  for(std::map<field_id, field_entry*>::iterator field_list_it = curfield.getFieldStart();
       field_list_it != curfield.getFieldEnd(); ++field_list_it){
			 cur = field_list_it->second;
			 if(cur == NULL) continue;
			 //If ANY field blocks vision, the tile does.
			 if(!fieldlist[cur->getFieldType()].transparent[cur->getFieldDensity() - 1]){
				 return false;
			 }
	  }
	 }
	 return true; //no blockers found, this is transparent
 }
 return false; //failsafe block vision
}

bool map::has_flag(const t_flag flag, const int x, const int y)
{
 if (flag == bashable) {
  int vpart;
  vehicle *veh = veh_at(x, y, vpart);
  if (veh && veh->parts[vpart].hp > 0 && // if there's a vehicle part here...
      veh->part_with_feature (vpart, vpf_obstacle) >= 0) {// & it is obstacle...
   const int p = veh->part_with_feature (vpart, vpf_openable);
   if (p < 0 || !veh->parts[p].open) // and not open door
    return true;
  }
 }
 return (terlist[ter(x, y)].flags & mfb(flag)) | (furnlist[furn(x, y)].flags & mfb(flag));
}

bool map::has_flag_ter_or_furn(const t_flag flag, const int x, const int y)
{
 return (terlist[ter(x, y)].flags & mfb(flag)) | (furnlist[furn(x, y)].flags & mfb(flag));
}

bool map::has_flag_ter_and_furn(const t_flag flag, const int x, const int y)
{
 return terlist[ter(x, y)].flags & furnlist[furn(x, y)].flags & mfb(flag);
}

bool map::is_destructable(const int x, const int y)
{
 return (has_flag(bashable, x, y) ||
         (move_cost(x, y) == 0 && !has_flag(liquid, x, y)));
}

bool map::is_destructable_ter_furn(const int x, const int y)
{
 return (has_flag_ter_or_furn(bashable, x, y) ||
         (move_cost_ter_furn(x, y) == 0 && !has_flag(liquid, x, y)));
}

bool map::is_outside(const int x, const int y)
{
 if(!INBOUNDS(x, y))
  return true;

 return outside_cache[x][y];
}

// MATERIALS-TODO: Use fire resistance
bool map::flammable_items_at(const int x, const int y)
{
 for (int i = 0; i < i_at(x, y).size(); i++) {
  item *it = &(i_at(x, y)[i]);
  int vol = it->volume();
  if (it->made_of("paper") || it->made_of("powder") ||
      it->type->id == "whiskey" || it->type->id == "vodka" ||
      it->type->id == "rum" || it->type->id == "tequila")
    return true;
  if ((it->made_of("wood") || it->made_of("veggy")) && (it->burnt < 1 || vol <= 10))
    return true;
  if (it->made_of("cotton") && (vol <= 5 || it->burnt < 1))
    return true;
  if (it->is_ammo() && it->ammo_type() != "battery" &&
      it->ammo_type() != "nail" && it->ammo_type() != "BB" &&
      it->ammo_type() != "bolt" && it->ammo_type() != "arrow" &&
      it->ammo_type() != "pebble" && it->ammo_type() != "NULL")
    return true;
 }
 return false;
}

bool map::moppable_items_at(const int x, const int y)
{
 for (int i = 0; i < i_at(x, y).size(); i++) {
  item *it = &(i_at(x, y)[i]);
  if (it->made_of(LIQUID))
   return true;
 }
 return false;
}

point map::random_outdoor_tile()
{
 std::vector<point> options;
 for (int x = 0; x < SEEX * my_MAPSIZE; x++) {
  for (int y = 0; y < SEEY * my_MAPSIZE; y++) {
   if (is_outside(x, y))
    options.push_back(point(x, y));
  }
 }
 if (options.empty()) // Nowhere is outdoors!
  return point(-1, -1);

 return options[rng(0, options.size() - 1)];
}

bool map::has_adjacent_furniture(const int x, const int y)
{
    const signed char cx[4] = { 0, -1, 0, 1};
    const signed char cy[4] = {-1,  0, 1, 0};

    for (int i = 0; i < 4; i++)
    {
        const int adj_x = x + cx[i];
        const int adj_y = y + cy[i];

        switch( furn(adj_x, adj_y) )
        {
        case f_fridge:
        case f_glass_fridge:
        case f_dresser:
        case f_rack:
        case f_bookcase:
        case f_locker:
            return true;
        }
    }

 return false;
}

void map::mop_spills(const int x, const int y) {
 for (int i = 0; i < i_at(x, y).size(); i++) {
  item *it = &(i_at(x, y)[i]);
  if (it->made_of(LIQUID)) {
    i_rem(x, y, i);
    i--;
  }
 }
}

bool map::bash(const int x, const int y, const int str, std::string &sound, int *res)
{
 sound = "";
 bool smashed_web = false;
 if (field_at(x, y).findField(fd_web)) {
  smashed_web = true;
  remove_field(x, y, fd_web);
 }

 for (int i = 0; i < i_at(x, y).size(); i++) { // Destroy glass items (maybe)
    // the check for active supresses molotovs smashing themselves with their own explosion
    if (i_at(x, y)[i].made_of("glass") && !i_at(x, y)[i].active && one_in(2)) {
        if (sound == "") {
            sound = string_format(_("A %s shatters!  "), i_at(x,y)[i].tname().c_str());
        } else {
            sound = _("Some items shatter!  ");
        }
        for (int j = 0; j < i_at(x, y)[i].contents.size(); j++) {
            i_at(x, y).push_back(i_at(x, y)[i].contents[j]);
        }
        i_rem(x, y, i);
        i--;
    }
 }

 int result = -1;
 int vpart;
 vehicle *veh = veh_at(x, y, vpart);
 if (veh) {
  veh->damage (vpart, str, 1);
  result = str;
  sound += _("crash!");
  return true;
 }

switch (furn(x, y)) {
 case f_locker:
 case f_rack:
  result = rng(0, 30);
  if (res) *res = result;
  if (str >= result) {
   sound += _("metal screeching!");
   furn_set(x, y, f_null);
   spawn_item(x, y, "scrap", 0, rng(2, 8));
   const int num_boards = rng(0, 3);
   for (int i = 0; i < num_boards; i++)
    spawn_item(x, y, "steel_chunk", 0);
   spawn_item(x, y, "pipe", 0);
   return true;
  } else {
   sound += _("clang!");
   return true;
  }
  break;

 case f_oven:
  result = rng(0, 30);
  if (res) *res = result;
  if (str >= result) {
   sound += _("metal screeching!");
   furn_set(x, y, f_null);
   spawn_item(x, y, "scrap",       0, rng(0, 6));
   spawn_item(x, y, "steel_chunk", 0, rng(0, 3));
   spawn_item(x, y, "element", 0, rng(1, 3));
   spawn_item(x, y, "sheet_metal", 0, 0, rng(2, 6));
   spawn_item(x, y, "cable", 0, 0, rng(1,3));

   return true;
  } else {
   sound += _("clang!");
   return true;
  }
 break;

 case f_sink:
 case f_toilet:
 case f_bathtub:
  result = dice(8, 4) - 8;
  if (res) *res = result;
  if (str >= result) {
   sound += _("porcelain breaking!");
   furn_set(x, y, f_null);
   return true;
  } else {
   sound += _("whunk!");
   return true;
  }
  break;

 case f_pool_table:
 case f_bulletin:
 case f_dresser:
 case f_bookcase:
 case f_counter:
 case f_table:
  result = rng(0, 45);
  if (res) *res = result;
  if (str >= result) {
   sound += _("smash!");
   furn_set(x, y, f_null);
   spawn_item(x, y, "2x4", 0, rng(2, 6));
   spawn_item(x, y, "nail", 0, 0, rng(4, 12));
   spawn_item(x, y, "splinter", 0);
   return true;
  } else {
   sound += _("whump.");
   return true;
  }
  break;

 case f_fridge:
 case f_glass_fridge:
  result = rng(0, 30);
  if (res) *res = result;
  if (str >= result) {
   sound += _("metal screeching!");
   furn_set(x, y, f_null);
   spawn_item(x, y, "scrap", 0, rng(2, 8));
   const int num_boards = rng(0, 3);
   for (int i = 0; i < num_boards; i++)
    spawn_item(x, y, "steel_chunk", 0);
   spawn_item(x, y, "hose", 0);
   return true;
  } else {
   sound += _("clang!");
   return true;
  }
  break;

 case f_bench:
 case f_chair:
 case f_desk:
 case f_cupboard:
  result = rng(0, 30);
  if (res) *res = result;
  if (str >= result) {
   sound += _("smash!");
   furn_set(x, y, f_null);
   spawn_item(x, y, "2x4", 0, rng(1, 3));
   spawn_item(x, y, "nail", 0, 0, rng(2, 6));
   spawn_item(x, y, "splinter", 0);
   return true;
  } else {
   sound += _("whump.");
   return true;
  }
  break;

 case f_crate_c:
 case f_crate_o:
  result = dice(4, 20);
  if (res) *res = result;
  if (str >= result) {
   sound += _("smash");
   furn_set(x, y, f_null);
   spawn_item(x, y, "2x4", 0, rng(1, 5));
   spawn_item(x, y, "nail", 0, 0, rng(2, 10));
   return true;
  } else {
   sound += _("wham!");
   return true;
  }
  break;

 case f_skin_wall:
 case f_skin_door:
 case f_skin_door_o:
 case f_skin_groundsheet:
 case f_canvas_wall:
 case f_canvas_door:
 case f_canvas_door_o:
 case f_groundsheet:
  result = rng(0, 6);
  if (res) *res = result;
  if (str >= result)
  {
   // Special code to collapse the tent if destroyed
   int tentx = -1, tenty = -1;
   // Find the center of the tent
   for (int i = -1; i <= 1; i++)
    for (int j = -1; j <= 1; j++)
     if (furn(x + i, y + j) == f_groundsheet ||
         furn(x + i, y + j) == f_fema_groundsheet ||
         furn(x + i, y + j) == f_skin_groundsheet)  {
       tentx = x + i;
       tenty = y + j;
       break;
     }
   // Never found tent center, bail out
   if (tentx == -1 && tenty == -1)
    break;
   // Take the tent down
   for (int i = -1; i <= 1; i++)
    for (int j = -1; j <= 1; j++) {
     if (furn(tentx + i, tenty + j) == f_groundsheet)
      spawn_item(tentx + i, tenty + j, "broketent", 0);
     if (furn(tentx + i, tenty + j) == f_skin_groundsheet)
      spawn_item(tentx + i, tenty + j, "damaged_shelter_kit", 0);
     furn_set(tentx + i, tenty + j, f_null);
    }

   sound += _("rrrrip!");
   return true;
  } else {
   sound += _("slap!");
   return true;
  }
  break;
}

switch (ter(x, y)) {
 case t_chainfence_v:
 case t_chainfence_h:
  result = rng(0, 50);
  if (res) *res = result;
  if (str >= result && str >= rng(0, 50)) {
   sound += _("clang!");
   ter_set(x, y, t_chainfence_posts);
   spawn_item(x, y, "wire", 0, rng(8, 20));
   return true;
  } else {
   sound += _("clang!");
   return true;
  }
  break;

 case t_wall_wood:
  result = rng(0, 120);
  if (res) *res = result;
  if (str >= result && str >= rng(0, 120)) {
   sound += _("crunch!");
   ter_set(x, y, t_wall_wood_chipped);
   if(one_in(2))
    spawn_item(x, y, "2x4", 0);
   spawn_item(x, y, "nail", 0, 0, 2);
   spawn_item(x, y, "splinter", 0);
   return true;
  } else {
   sound += _("whump!");
   return true;
  }
  break;

 case t_wall_wood_chipped:
  result = rng(0, 100);
  if (res) *res = result;
  if (str >= result && str >= rng(0, 100)) {
   sound += _("crunch!");
   ter_set(x, y, t_wall_wood_broken);
   spawn_item(x, y, "2x4", 0, rng(1, 4));
   spawn_item(x, y, "nail", 0, 0, rng(1, 3));
   spawn_item(x, y, "splinter", 0);
   return true;
  } else {
   sound += _("whump!");
   return true;
  }
  break;

 case t_wall_wood_broken:
  result = rng(0, 80);
  if (res) *res = result;
  if (str >= result && str >= rng(0, 80)) {
   sound += _("crash!");
   ter_set(x, y, t_dirt);
   spawn_item(x, y, "2x4", 0, rng(2, 5));
   spawn_item(x, y, "nail", 0, 0, rng(4, 10));
   spawn_item(x, y, "splinter", 0);
   return true;
  } else {
   sound += _("whump!");
   return true;
  }
  break;

case t_palisade:
case t_palisade_gate:
  result = rng(0, 120);
  if (res) *res = result;
  if (str >= result && str >= rng(0, 120)) {
   sound += _("crunch!");
   ter_set(x, y, t_pit);
   if(one_in(2))
   spawn_item(x, y, "splinter", 0, 20);
   return true;
  } else {
   sound += _("whump!");
   return true;
  }
  break;

case t_wall_log:
  result = rng(0, 120);
  if (res) *res = result;
  if (str >= result && str >= rng(0, 120)) {
   sound += _("crunch!");
   ter_set(x, y, t_wall_log_chipped);
   if(one_in(2))
   spawn_item(x, y, "splinter", 0, 3);
   return true;
  } else {
   sound += _("whump!");
   return true;
  }
  break;

 case t_wall_log_chipped:
  result = rng(0, 100);
  if (res) *res = result;
  if (str >= result && str >= rng(0, 100)) {
   sound += _("crunch!");
   ter_set(x, y, t_wall_log_broken);
   spawn_item(x, y, "splinter", 0, 5);
   return true;
  } else {
   sound += _("whump!");
   return true;
  }
  break;

 case t_wall_log_broken:
  result = rng(0, 80);
  if (res) *res = result;
  if (str >= result && str >= rng(0, 80)) {
   sound += _("crash!");
   ter_set(x, y, t_dirt);
   spawn_item(x, y, "splinter", 0, 5);
   return true;
  } else {
   sound += _("whump!");
   return true;
  }
  break;


 case t_chaingate_c:
 case t_chaingate_l:
  result = rng(0, has_adjacent_furniture(x, y) ? 80 : 100);
  if (res) *res = result;
  if (str >= result && str >= rng(0, 80)) {
   sound += _("clang!");
   ter_set(x, y, t_dirt);
   spawn_item(x, y, "wire", 0, rng(8, 20));
   spawn_item(x, y, "scrap", 0, rng(0, 12));
   return true;
  } else {
   sound += _("whump!");
   return true;
  }
  break;

  case t_fencegate_c:
  result = rng(0, has_adjacent_furniture(x, y) ? 30 : 40);
  if (res) *res = result;
  if (str >= result) {
   sound += _("crash!");
   ter_set(x, y, t_dirtfloor);
   spawn_item(x, y, "2x4", 0, rng(1, 4));
   spawn_item(x, y, "nail", 0, 0, rng(2, 12));
   spawn_item(x, y, "splinter", 0);
   return true;
  } else {
   sound += _("wham!");
   return true;
  }
  break;

 case t_door_c:
 case t_door_locked:
 case t_door_locked_alarm:
  result = rng(0, has_adjacent_furniture(x, y) ? 40 : 50);
  if (res) *res = result;
  if (str >= result) {
   sound += _("smash!");
   ter_set(x, y, t_door_b);
   return true;
  } else {
   sound += _("whump!");
   return true;
  }
  break;

 case t_door_b:
  result = rng(0, has_adjacent_furniture(x, y) ? 30 : 40);
  if (res) *res = result;
  if (str >= result) {
   sound += _("crash!");
   ter_set(x, y, t_door_frame);
   spawn_item(x, y, "2x4", 0, rng(1, 6));
   spawn_item(x, y, "nail", 0, 0, rng(2, 12));
   spawn_item(x, y, "splinter", 0);
   return true;
  } else {
   sound += _("wham!");
   return true;
  }
  break;

 case t_window_domestic:
 case t_curtains:
 case t_window_domestic_taped:
  result = rng(0, 6);
  if (res) *res = result;
  if (str >= result) {
   sound += _("glass breaking!");
   ter_set(x, y, t_window_frame);
  spawn_item(x, y, "sheet", 0, 2);
  spawn_item(x, y, "stick", 0);
  spawn_item(x, y, "string_36", 0);
   return true;
  } else {
   sound += _("whack!");
   return true;
  }
  break;

 case t_window:
 case t_window_alarm:
 case t_window_alarm_taped:
 case t_window_taped:
  result = rng(0, 6);
  if (res) *res = result;
  if (str >= result) {
   sound += _("glass breaking!");
   ter_set(x, y, t_window_frame);
   return true;
  } else {
   sound += _("whack!");
   return true;
  }
  break;

 case t_door_boarded:
  result = rng(0, has_adjacent_furniture(x, y) ? 50 : 60);
  if (res) *res = result;
  if (str >= result) {
   sound += _("crash!");
   ter_set(x, y, t_door_frame);
   spawn_item(x, y, "2x4", 0, rng(1, 6));
   spawn_item(x, y, "nail", 0, 0, rng(2, 12));
   spawn_item(x, y, "splinter", 0);
   return true;
  } else {
   sound += _("wham!");
   return true;
  }
  break;

 case t_window_boarded:
  result = rng(0, 30);
  if (res) *res = result;
  if (str >= result) {
   sound += _("crash!");
   ter_set(x, y, t_window_frame);
   const int num_boards = rng(0, 2) * rng(0, 1);
   for (int i = 0; i < num_boards; i++)
    spawn_item(x, y, "splinter", 0);
   return true;
  } else {
   sound += _("wham!");
   return true;
  }
  break;

 case t_paper:
  result = dice(1, 6) - 2;
  if (res) *res = result;
  if (str >= result) {
   sound += _("rrrrip!");
   ter_set(x, y, t_dirt);
   return true;
  } else {
   sound += _("slap!");
   return true;
  }
  break;

 case t_fence_v:
 case t_fence_h:
  result = rng(0, 10);
  if (res) *res = result;
  if (str >= result) {
   sound += _("crak");
   ter_set(x, y, t_dirt);
   spawn_item(x, y, "2x4", 0, rng(1, 3));
   spawn_item(x, y, "nail", 0, 0, rng(2, 6));
   spawn_item(x, y, "splinter", 0);
   return true;
  } else {
   sound += _("whump.");
   return true;
  }
  break;

 case t_fence_post:
  result = rng(0, 10);
  if (res) *res = result;
  if (str >= result) {
   sound += _("crak");
   ter_set(x, y, t_dirt);
   spawn_item(x, y, "pointy_stick", 0, 1);
   return true;
  } else {
   sound += _("whump.");
   return true;
  }
  break;

 case t_wall_glass_h:
 case t_wall_glass_v:
 case t_wall_glass_h_alarm:
 case t_wall_glass_v_alarm:
 case t_door_glass_c:
  result = rng(0, 20);
  if (res) *res = result;
  if (str >= result) {
   sound += _("glass breaking!");
   ter_set(x, y, t_floor);
   return true;
  } else {
   sound += _("whack!");
   return true;
  }
  break;

 case t_reinforced_glass_h:
 case t_reinforced_glass_v:
  result = rng(60, 100);
  if (res) *res = result;
  if (str >= result) {
   sound += _("glass breaking!");
   ter_set(x, y, t_floor);
   return true;
  } else {
   sound += _("whack!");
   return true;
  }
  break;

 case t_tree_young:
  result = rng(0, 50);
  if (res) *res = result;
  if (str >= result) {
   sound += _("crunch!");
   ter_set(x, y, t_underbrush);
   const int num_sticks = rng(0, 3);
   for (int i = 0; i < num_sticks; i++)
    spawn_item(x, y, "stick", 0);
   return true;
  } else {
   sound += _("whack!");
   return true;
  }
  break;

 case t_underbrush:
  result = rng(0, 30);
  if (res) *res = result;
  if (str >= result && !one_in(4)) {
   sound += _("crunch.");
   ter_set(x, y, t_dirt);
   return true;
  } else {
   sound += _("brush.");
   return true;
  }
  break;

 case t_shrub:
  result = rng(0, 30);
  if (res) *res = result;
  if (str >= result && str >= rng(0, 30) && one_in(2)){
   sound += _("crunch.");
   ter_set(x, y, t_underbrush);
   return true;
  } else {
   sound += _("brush.");
   return true;
  }
  break;

 case t_marloss:
  result = rng(0, 40);
  if (res) *res = result;
  if (str >= result) {
   sound += _("crunch!");
   ter_set(x, y, t_fungus);
   return true;
  } else {
   sound += _("whack!");
   return true;
  }
  break;

 case t_vat:
  result = dice(2, 20);
  if (res) *res = result;
  if (str >= result) {
   sound += _("ker-rash!");
   ter_set(x, y, t_floor);
   return true;
  } else {
   sound += _("plunk.");
   return true;
  }
  break;
 }
 if (res) *res = result;
 if (move_cost(x, y) <= 0) {
  sound += _("thump!");
  return true;
 }
 return smashed_web;// If we kick empty space, the action is cancelled
}

// map::destroy is only called (?) if the terrain is NOT bashable.
void map::destroy(game *g, const int x, const int y, const bool makesound)
{
 if (has_furn(x, y))
  furn_set(x, y, f_null);

 switch (ter(x, y)) {

 case t_gas_pump:
  if (makesound && one_in(3))
   g->explosion(x, y, 40, 0, true);
  else {
   for (int i = x - 2; i <= x + 2; i++) {
    for (int j = y - 2; j <= y + 2; j++) {
       if(move_cost(i, j) == 0) continue;
       if (one_in(3)) spawn_item(i, j, "gasoline", 0);
       if (one_in(6)) spawn_item(i, j, "steel_chunk", 0, 0, 3);
    }
   }
  }
  ter_set(x, y, t_rubble);
  break;

 case t_door_c:
 case t_door_b:
 case t_door_locked:
 case t_door_boarded:
  ter_set(x, y, t_door_frame);
  for (int i = x - 2; i <= x + 2; i++) {
   for (int j = y - 2; j <= y + 2; j++) {
       if(move_cost(i, j) == 0) continue;
       if (one_in(6)) spawn_item(i, j, "2x4", 0);
       if (one_in(6)) spawn_item(i, j, "nail", 0, 0, 3);
   }
  }
  break;

 case t_pavement:
 case t_pavement_y:
 case t_sidewalk:
  for (int i = x - 2; i <= x + 2; i++) {
   for (int j = y - 2; j <= y + 2; j++) {
    if (move_cost(i, j) > 0 && one_in(5))
     spawn_item(i, j, "rock", 0);
    ter_set(x, y, t_rubble);
   }
  }
  break;

 case t_floor:
 g->sound(x, y, 20, _("SMASH!!"));
  for (int i = x - 2; i <= x + 2; i++) {
   for (int j = y - 2; j <= y + 2; j++) {
       if(move_cost(i, j) == 0) continue;
       if (one_in(5)) spawn_item(i, j, "splinter", 0);
       if (one_in(6)) spawn_item(i, j, "nail", 0, 0, 3);
   }
  }
  ter_set(x, y, t_rubble);
  for (int i = x - 1; i <= x + 1; i++) {
   for (int j = y - 1; j <= y + 1; j++) {
    if (one_in(2)) {
     //debugmsg("1");
      if (!g->m.has_flag(noitem, x, y)) {
       //debugmsg("2");
       if (!(g->m.field_at(i, j).findField(fd_rubble))) {
        //debugmsg("Rubble spawned!");
        g->m.add_field(g, i, j, fd_rubble, rng(1,3));
        g->m.field_effect(i, j, g);
       }
      }
    }
   }
  }
   //TODO: Make rubble decay into smoke
  for (int i = x - 1; i <= x + 1; i++)
   for (int j = y - 1; j <= y + 1; j++) {
    if ((i == x && j == y) || !has_flag(collapses, i, j))
      continue;
     int num_supports = -1;
     for (int k = i - 1; k <= i + 1; k++)
       for (int l = j - 1; l <= j + 1; l++) {
       if (k == i && l == j)
        continue;
       if (has_flag(collapses, k, l))
        num_supports++;
       else if (has_flag(supports_roof, k, l))
        num_supports += 2;
       }
     if (one_in(num_supports))
      destroy (g, i, j, false);
   }
  break;

 case t_concrete_v:
 case t_concrete_h:
 case t_wall_v:
 case t_wall_h:
 g->sound(x, y, 20, _("SMASH!!"));
  for (int i = x - 2; i <= x + 2; i++) {
   for (int j = y - 2; j <= y + 2; j++) {
       if(move_cost(i, j) == 0) continue;
       if (one_in(5)) spawn_item(i, j, "rock", 0);
       if (one_in(4)) spawn_item(i, j, "splinter", 0);
       if (one_in(3)) spawn_item(i, j, "rebar", 0);
       if (one_in(6)) spawn_item(i, j, "nail", 0, 0, 3);
   }
  }
  ter_set(x, y, t_rubble);
  for (int i = x - 1; i <= x + 1; i++) {
   for (int j = y - 1; j <= y + 1; j++) {
    if (one_in(2)) {
     //debugmsg("1");
      if (!g->m.has_flag(noitem, x, y)) {
       //debugmsg("2");
       if (!(g->m.field_at(i, j).findField(fd_rubble))) {
        //debugmsg("Rubble spawned!");
        g->m.add_field(g, i, j, fd_rubble, rng(1,3));
        g->m.field_effect(i, j, g);
      }
      }
    }
   }
  }
  //TODO: Make rubble decay into smoke
  for (int i = x - 1; i <= x + 1; i++)
   for (int j = y - 1; j <= y + 1; j++) {
    if ((i == x && j == y) || !has_flag(supports_roof, i, j))
      continue;
     int num_supports = 0;
     for (int k = i - 1; k <= i + 1; k++)
      for (int l = j - 1; l <= j + 1; l++) {
       if (k == i && l == j)
        continue;
       if (has_flag(collapses, i, j)) {
        if (has_flag(collapses, k, l))
         num_supports++;
        else if (has_flag(supports_roof, k, l))
         num_supports += 2;
       } else if (has_flag(supports_roof, i, j))
        if (has_flag(supports_roof, k, l) && !has_flag(collapses, k, l))
         num_supports += 3;
      }
     if (one_in(num_supports))
      destroy (g, i, j, false);
   }
  break;

  case t_palisade:
  case t_palisade_gate:
      g->sound(x, y, 16, _("CRUNCH!!"));
      for (int i = x - 1; i <= x + 1; i++)
      {
          for (int j = y - 1; j <= y + 1; j++)
          {
              if(move_cost(i, j) == 0) continue;
              if (one_in(3)) spawn_item(i, j, "rope_6", 0);
              if (one_in(2)) spawn_item(i, j, "splinter", 0);
              if (one_in(3)) spawn_item(i, j, "stick", 0);
              if (one_in(6)) spawn_item(i, j, "2x4", 0);
              if (one_in(9)) spawn_item(i, j, "log", 0);
          }
      }
      ter_set(x, y, t_dirt);
      add_trap(x, y, tr_pit);
      break;

 default:
  if (makesound && has_flag(explodes, x, y) && one_in(2))
   g->explosion(x, y, 40, 0, true);
  ter_set(x, y, t_rubble);
 }

 if (makesound)
  g->sound(x, y, 40, _("SMASH!!"));
}

void map::shoot(game *g, const int x, const int y, int &dam,
                const bool hit_items, const std::set<std::string>& ammo_effects)
{
    if (dam < 0)
    {
        return;
    }

    if (has_flag(alarmed, x, y) && !g->event_queued(EVENT_WANTED))
    {
        g->sound(x, y, 30, _("An alarm sounds!"));
        g->add_event(EVENT_WANTED, int(g->turn) + 300, 0, g->levx, g->levy);
    }

    int vpart;
    vehicle *veh = veh_at(x, y, vpart);
    if (veh)
    {
        const bool inc = (ammo_effects.count("INCENDIARY") || ammo_effects.count("FLAME"));
        dam = veh->damage (vpart, dam, inc? 2 : 0, hit_items);
    }

    switch (ter(x, y))
    {

        case t_wall_wood_broken:
        case t_wall_log_broken:
        case t_door_b:
            if (hit_items || one_in(8))
            {	// 1 in 8 chance of hitting the door
                dam -= rng(20, 40);
                if (dam > 0)
                {
                    g->sound(x, y, 10, _("crash!"));
                    ter_set(x, y, t_dirt);
                }
            }
            else
                dam -= rng(0, 1);
        break;


        case t_door_c:
        case t_door_locked:
        case t_door_locked_alarm:
            dam -= rng(15, 30);
            if (dam > 0)
            {
                g->sound(x, y, 10, _("smash!"));
                ter_set(x, y, t_door_b);
            }
        break;

        case t_door_boarded:
            dam -= rng(15, 35);
            if (dam > 0)
            {
                g->sound(x, y, 10, _("crash!"));
                ter_set(x, y, t_door_b);
            }
        break;

        // Fall-through intended
        case t_window_domestic_taped:
        case t_curtains:
            if (ammo_effects.count("LASER"))
                dam -= rng(1, 5);
        case t_window_domestic:
            if (ammo_effects.count("LASER"))
                dam -= rng(0, 5);
            else
            {
                dam -= rng(1,3);
                if (dam > 0)
                {
                    g->sound(x, y, 16, _("glass breaking!"));
                    ter_set(x, y, t_window_frame);
                    spawn_item(x, y, "sheet", 0, 1);
                    spawn_item(x, y, "stick", 0);
                    spawn_item(x, y, "string_36", 0);
                }
            }
        break;

        // Fall-through intended
        case t_window_taped:
        case t_window_alarm_taped:
            if (ammo_effects.count("LASER"))
                dam -= rng(1, 5);
        case t_window:
        case t_window_alarm:
            if (ammo_effects.count("LASER"))
                dam -= rng(0, 5);
            else
            {
                dam -= rng(1,3);
                if (dam > 0)
                {
                    g->sound(x, y, 16, _("glass breaking!"));
                    ter_set(x, y, t_window_frame);
                }
            }
        break;

        case t_window_boarded:
            dam -= rng(10, 30);
            if (dam > 0)
            {
                g->sound(x, y, 16, _("glass breaking!"));
                ter_set(x, y, t_window_frame);
            }
        break;

        case t_wall_glass_h:
        case t_wall_glass_v:
        case t_wall_glass_h_alarm:
        case t_wall_glass_v_alarm:
            if (ammo_effects.count("LASER"))
                dam -= rng(0,5);
            else
            {
                dam -= rng(1,8);
                if (dam > 0)
                {
                    g->sound(x, y, 20, _("glass breaking!"));
                    ter_set(x, y, t_floor);
                }
            }
        break;


        // reinforced glass stops most bullets
        // laser beams are attenuated
        case t_reinforced_glass_v:
        case t_reinforced_glass_h:
            if (ammo_effects.count("LASER"))
            {
                dam -= rng(0, 8);
            }
            else
            {
                //Greatly weakens power of bullets
                dam -= 40;
                if (dam <= 0)
                    g->add_msg(_("The shot is stopped by the reinforced glass wall!"));
                //high powered bullets penetrate the glass, but only extremely strong
                // ones (80 before reduction) actually destroy the glass itself.
                else if (dam >= 40)
                {
                    g->sound(x, y, 20, _("glass breaking!"));
                    ter_set(x, y, t_floor);
                }
            }
        break;

        case t_paper:
            dam -= rng(4, 16);
            if (dam > 0)
            {
                g->sound(x, y, 8, _("rrrrip!"));
                ter_set(x, y, t_dirt);
            }
            if (ammo_effects.count("INCENDIARY"))
                add_field(g, x, y, fd_fire, 1);
        break;

        case t_gas_pump:
            if (hit_items || one_in(3))
            {
                if (dam > 15)
                {
                    if (ammo_effects.count("INCENDIARY") || ammo_effects.count("FLAME"))
                        g->explosion(x, y, 40, 0, true);
                    else
                    {
                        for (int i = x - 2; i <= x + 2; i++)
                        {
                            for (int j = y - 2; j <= y + 2; j++)
                            {
                                if (move_cost(i, j) > 0 && one_in(3))
                                    spawn_item(i, j, "gasoline", 0);
                            }
                        }
                        g->sound(x, y, 10, _("smash!"));
                    }
                    ter_set(x, y, t_gas_pump_smashed);
                }
                dam -= 60;
            }
        break;

        case t_vat:
            if (dam >= 10)
            {
                g->sound(x, y, 20, _("ke-rash!"));
                ter_set(x, y, t_floor);
            }
            else
                dam = 0;
        break;

        default:
            if (move_cost(x, y) == 0 && !trans(x, y))
                dam = 0;	// TODO: Bullets can go through some walls?
            else
                dam -= (rng(0, 1) * rng(0, 1) * rng(0, 1));
    }

    if (ammo_effects.count("TRAIL") && !one_in(4))
        add_field(g, x, y, fd_smoke, rng(1, 2));

    if (ammo_effects.count("LIGHTNING"))
        add_field(g, x, y, fd_electricity, rng(2, 3));

    // Set damage to 0 if it's less
    if (dam < 0)
        dam = 0;

    // Check fields?
    field_entry *fieldhit = field_at(x, y).findField(fd_web);
   // switch (fieldhit->type)
   // {
        //case fd_web:
	//Removed switch for now as web is the only relevant choice to avoid a currently redundant for loop declaration for all the field types.
	if(fieldhit){
            if (ammo_effects.count("INCENDIARY") || ammo_effects.count("FLAME"))
                add_field(g, x, y, fd_fire, fieldhit->getFieldDensity() - 1);
            else if (dam > 5 + fieldhit->getFieldDensity() * 5 &&
                     one_in(5 - fieldhit->getFieldDensity()))
            {
                dam -= rng(1, 2 + fieldhit->getFieldDensity() * 2);
                remove_field(x, y,fd_web);
            }
	}
        //break;
    //}

    // Now, destroy items on that tile.
    if ((move_cost(x, y) == 2 && !hit_items) || !INBOUNDS(x, y))
        return;	// Items on floor-type spaces won't be shot up.

    for (int i = 0; i < i_at(x, y).size(); i++)
    {
        bool destroyed = false;
        int chance = (i_at(x, y)[i].volume() > 0 ? i_at(x, y)[i].volume() : 1);   // volume dependent chance

        if (dam > i_at(x, y)[i].bash_resist() && one_in(chance))
        {
            i_at(x, y)[i].damage++;
        }
        if (i_at(x, y)[i].damage >= 5)
        {
            destroyed = true;
        }

        if (destroyed)
        {
            for (int j = 0; j < i_at(x, y)[i].contents.size(); j++)
                i_at(x, y).push_back(i_at(x, y)[i].contents[j]);
            i_rem(x, y, i);
            i--;
        }
    }
}

bool map::hit_with_acid(game *g, const int x, const int y)
{
 if (move_cost(x, y) != 0)
  return false; // Didn't hit the tile!

 switch (ter(x, y)) {
  case t_wall_glass_v:
  case t_wall_glass_h:
  case t_wall_glass_v_alarm:
  case t_wall_glass_h_alarm:
  case t_vat:
   ter_set(x, y, t_floor);
   break;

  case t_door_c:
  case t_door_locked:
  case t_door_locked_alarm:
   if (one_in(3))
    ter_set(x, y, t_door_b);
   break;

  case t_door_bar_c:
  case t_door_bar_o:
  case t_door_bar_locked:
  case t_bars:
   ter_set(x, y, t_floor);
   g->add_msg(_("The metal bars melt!"));
   break;

  case t_door_b:
   if (one_in(4))
    ter_set(x, y, t_door_frame);
   else
    return false;
   break;

  case t_window:
  case t_window_alarm:
   ter_set(x, y, t_window_empty);
   break;

  case t_wax:
   ter_set(x, y, t_floor_wax);
   break;

  case t_gas_pump:
  case t_gas_pump_smashed:
   return false;

  case t_card_science:
  case t_card_military:
   ter_set(x, y, t_card_reader_broken);
   break;
 }

 return true;
}

// returns true if terrain stops fire
bool map::hit_with_fire(game *g, const int x, const int y)
{
    if (move_cost(x, y) != 0)
        return false; // Didn't hit the tile!

    // non passable but flammable terrain, set it on fire
    if (has_flag(flammable, x, y) || has_flag(flammable2, x, y))
    {
        add_field(g, x, y, fd_fire, 3);
    }
    return true;
}

void map::marlossify(const int x, const int y)
{
 const int type = rng(1, 9);
 switch (type) {
  case 1:
  case 2:
  case 3:
  case 4: ter_set(x, y, t_fungus);      break;
  case 5:
  case 6:
  case 7: ter_set(x, y, t_marloss);     break;
  case 8: ter_set(x, y, t_tree_fungal); break;
  case 9: ter_set(x, y, t_slime);       break;
 }
}

bool map::open_door(const int x, const int y, const bool inside)
{
 if (ter(x, y) == t_door_c) {
  ter_set(x, y, t_door_o);
  return true;
 } else if (furn(x, y) == f_canvas_door) {
  furn_set(x, y, f_canvas_door_o);
  return true;
 } else if (furn(x, y) == f_skin_door) {
  furn_set(x, y, f_skin_door_o);
  return true;
 } else if (furn(x, y) == f_safe_c) {
  furn_set(x, y, f_safe_o);
  return true;
 } else if (inside && ter(x, y) == t_curtains) {
  ter_set(x, y, t_window_domestic);
  return true;
 } else if (inside && ter(x, y) == t_window_domestic) {
  ter_set(x, y, t_window_open);
  return true;
 } else if (ter(x, y) == t_chaingate_c) {
  ter_set(x, y, t_chaingate_o);
  return true;
 } else if (ter(x, y) == t_fencegate_c) {
  ter_set(x, y, t_fencegate_o);
  return true;
 } else if (ter(x, y) == t_door_metal_c) {
  ter_set(x, y, t_door_metal_o);
  return true;
 } else if (ter(x, y) == t_door_bar_c) {
  ter_set(x, y, t_door_bar_o);
  return true;
 } else if (ter(x, y) == t_door_glass_c) {
  ter_set(x, y, t_door_glass_o);
  return true;
 } else if (inside &&
            (ter(x, y) == t_door_locked || ter(x, y) == t_door_locked_alarm)) {
  ter_set(x, y, t_door_o);
  return true;
 }
 return false;
}

void map::translate(const ter_id from, const ter_id to)
{
 if (from == to) {
  debugmsg("map::translate %s => %s", terlist[from].name.c_str(),
                                      terlist[from].name.c_str());
  return;
 }
 for (int x = 0; x < SEEX * my_MAPSIZE; x++) {
  for (int y = 0; y < SEEY * my_MAPSIZE; y++) {
   if (ter(x, y) == from)
    ter_set(x, y, to);
  }
 }
}

//This function performs the translate function within a given radius of the player.
void map::translate_radius(const ter_id from, const ter_id to, float radi, int uX, int uY)
{
 if (from == to) {
  debugmsg("map::translate %s => %s", terlist[from].name.c_str(),
                                      terlist[from].name.c_str());
  return;
 }
 for (int x = 0; x < SEEX * my_MAPSIZE; x++) {
  for (int y = 0; y < SEEY * my_MAPSIZE; y++) {
   if (ter(x, y) == from){
    //float radiX = 0.0;
    float radiX = sqrt(float((uX-x)*(uX-x) + (uY-y)*(uY-y)));
    if (radiX <= radi){
      ter_set(x, y, to);}
    }
   }
  }
 }

bool map::close_door(const int x, const int y, const bool inside)
{
 if (ter(x, y) == t_door_o) {
  ter_set(x, y, t_door_c);
  return true;
 } else if (inside && ter(x, y) == t_window_domestic) {
  ter_set(x, y, t_curtains);
  return true;
 } else if (furn(x, y) == f_canvas_door_o) {
  furn_set(x, y, f_canvas_door);
  return true;
 } else if (furn(x, y) == f_skin_door_o) {
  furn_set(x, y, f_skin_door);
  return true;
 } else if (furn(x, y) == f_safe_o) {
  furn_set(x, y, f_safe_c);
  return true;
 } else if (inside && ter(x, y) == t_window_open) {
  ter_set(x, y, t_window_domestic);
  return true;
 } else if (ter(x, y) == t_chaingate_o) {
  ter_set(x, y, t_chaingate_c);
  return true;
  } else if (ter(x, y) == t_fencegate_o) {
  ter_set(x, y, t_fencegate_c);
 } else if (ter(x, y) == t_door_metal_o) {
  ter_set(x, y, t_door_metal_c);
  return true;
 } else if (ter(x, y) == t_door_bar_o) {
  ter_set(x, y, t_door_bar_locked);//jail doors lock behind you
  return true;
 } else if (ter(x, y) == t_door_glass_o) {
  ter_set(x, y, t_door_glass_c);
  return true;
 }
 return false;
}

int& map::radiation(const int x, const int y)
{
 if (!INBOUNDS(x, y)) {
  nulrad = 0;
  return nulrad;
 }
/*
 int nonant;
 cast_to_nonant(x, y, nonant);
*/
 const int nonant = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE;

 const int lx = x % SEEX;
 const int ly = y % SEEY;
 return grid[nonant]->rad[lx][ly];
}

std::vector<item>& map::i_at(const int x, const int y)
{
 if (!INBOUNDS(x, y)) {
  nulitems.clear();
  return nulitems;
 }
/*
 int nonant;
 cast_to_nonant(x, y, nonant);
*/
 const int nonant = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE;

 const int lx = x % SEEX;
 const int ly = y % SEEY;
 return grid[nonant]->itm[lx][ly];
}

item map::water_from(const int x, const int y)
{
    item ret(item_controller->find_template("water"), 0);
    if (ter(x, y) == t_water_sh && one_in(3))
        ret.poison = rng(1, 4);
    else if (ter(x, y) == t_water_dp && one_in(4))
        ret.poison = rng(1, 4);
    else if (ter(x, y) == t_sewage)
        ret.poison = rng(1, 7);
    else if (furn(x, y) == f_toilet && !one_in(3))
        ret.poison = rng(1, 3);
    else if (tr_at(x, y) == tr_funnel)
        ret.poison = (one_in(10) == true) ? 1 : 0;
    return ret;
}

item map::acid_from(const int x, const int y)
{
    item ret(item_controller->find_template("water_acid"), 0);
    return ret;
}

void map::i_rem(const int x, const int y, const int index)
{
 if (index > i_at(x, y).size() - 1)
  return;
 i_at(x, y).erase(i_at(x, y).begin() + index);
}

void map::i_clear(const int x, const int y)
{
 i_at(x, y).clear();
}

point map::find_item(const item *it)
{
 point ret;
 for (ret.x = 0; ret.x < SEEX * my_MAPSIZE; ret.x++) {
  for (ret.y = 0; ret.y < SEEY * my_MAPSIZE; ret.y++) {
   for (int i = 0; i < i_at(ret.x, ret.y).size(); i++) {
    if (it == &i_at(ret.x, ret.y)[i])
     return ret;
   }
  }
 }
 ret.x = -1;
 ret.y = -1;
 return ret;
}

void map::spawn_item(const int x, const int y, item new_item, const int birthday,
                     const int quantity, const int charges, const int damlevel)
{
    if (charges && new_item.charges > 0)
    {
        //let's fail silently if we specify charges for an item that doesn't support it
        new_item.charges = charges;
    }
    new_item = new_item.in_its_container(&(g->itypes));
    if ((new_item.made_of(LIQUID) && has_flag(swimmable, x, y)) ||
        has_flag(destroy_item, x, y))
    {
        return;
    }
    // bounds checking for damage level
    if (damlevel < -1)
    {
        new_item.damage = -1;
    }
    if (damlevel > 4)
    {
        new_item.damage = 4;
    }
    new_item.damage = damlevel;

    // clothing with variable size flag may sometimes be generated fitted
    if (new_item.is_armor() && new_item.has_flag("VARSIZE") && one_in(3))
    {
        new_item.item_tags.insert("FIT");
    }

    add_item(x, y, new_item);
}

void map::spawn_artifact(const int x, const int y, itype* type, const int bday)
{
    item newart(type, bday);
    add_item(x, y, newart);
}

//New spawn_item method, using item factory
// added argument to spawn at various damage levels
void map::spawn_item(const int x, const int y, std::string type_id, const int birthday,
                     const int quantity, const int charges, const int damlevel)
{
    item new_item = item_controller->create(type_id, birthday);
    for(int i = 1; i < quantity; i++)
    {
        spawn_item(x, y, type_id, birthday, 0, charges);
    }
    spawn_item( x, y, new_item, birthday, quantity, charges, damlevel );
}

// stub for now, could vary by ter type
int map::max_volume(const int x, const int y) {
   return MAX_VOLUME_IN_SQUARE;
}

// total volume of all the things
int map::stored_volume(const int x, const int y) {
   if(!INBOUNDS(x, y)) return 0;
   int cur_volume=0;
   for (int n = 0; n < i_at(x, y).size(); n++) {
       item* curit = &(i_at(x, y)[n]);
       cur_volume += curit->volume();
   }
   return cur_volume;
}

// free space
int map::free_volume(const int x, const int y) {
   const int maxvolume = this->max_volume(x, y);
   if(!INBOUNDS(x, y)) return 0;
   return ( maxvolume - stored_volume(x, y) );
}

// returns true if full, modified by arguments:
// (none):                            size >= max || volume >= max
// (addvolume >= 0):                  size+1 > max || volume + addvolume > max
// (addvolume >= 0, addnumber >= 0):  size + addnumber > max || volume + addvolume > max
bool map::is_full(const int x, const int y, const int addvolume, const int addnumber ) {
   const int maxitems = MAX_ITEM_IN_SQUARE; // (game.h) 1024
   const int maxvolume = this->max_volume(x, y);

   if( ! (INBOUNDS(x, y) && move_cost(x, y) > 0 && !has_flag(noitem, x, y) ) ) {
       return true;
   }

   if ( addvolume == -1 ) {
       if ( i_at(x, y).size() < maxitems ) return true;
       int cur_volume=stored_volume(x, y);
       return (cur_volume >= maxvolume ? true : false );
   } else {
       if ( i_at(x, y).size() + ( addnumber == -1 ? 1 : addnumber ) > maxitems ) return true;
       int cur_volume=stored_volume(x, y);
       return ( cur_volume + addvolume > maxvolume ? true : false );
   }

}

// adds an item to map point, or stacks charges.
// returns false if item exceeds tile's weight limits or item count. This function is expensive, and meant for
// user initiated actions, not mapgen!
// overflow_radius > 0: if x,y is full, attempt to drop item up to overflow_radius squares away, if x,y is full
bool map::add_item_or_charges(const int x, const int y, item new_item, int overflow_radius) {

    if (( new_item.is_style() || !INBOUNDS(x,y) || (new_item.made_of(LIQUID) && has_flag(swimmable, x, y)) || has_flag(destroy_item, x, y) ) ){
        debugmsg("%i,%i:is_style %i, liquid %i, destroy_item %i",x,y, new_item.is_style() ,(new_item.made_of(LIQUID) && has_flag(swimmable, x, y)) ,has_flag(destroy_item, x, y) );
        return false;
    }

    bool tryaddcharges = (new_item.charges  != -1 && (new_item.is_food() || new_item.is_ammo()));
    std::vector<point> ps = closest_points_first(overflow_radius, x, y);
    for(std::vector<point>::iterator p_it = ps.begin(); p_it != ps.end(); p_it++)
    {
        itype_id add_type = new_item.type->id; // caching this here = ~25% speed increase
        if (!INBOUNDS(p_it->x, p_it->x) || new_item.volume() > this->free_volume(p_it->x, p_it->y) ||
                has_flag(destroy_item, p_it->x, p_it->y) || has_flag(noitem, p_it->x, p_it->y)){
            continue;
        }

        if (tryaddcharges) {
            for (int i = 0; i < i_at(p_it->x,p_it->y).size(); i++)
            {
                if(i_at(p_it->x, p_it->y)[i].type->id == add_type)
                {
                    i_at(p_it->x, p_it->y)[i].charges += new_item.charges;
                    return true;
                }
            }
        }
        if (i_at(p_it->x, p_it->y).size() < MAX_ITEM_IN_SQUARE)
        {
            add_item(p_it->x, p_it->y, new_item, MAX_ITEM_IN_SQUARE);
            return true;
        }
    }
    return false;
}

// Place an item on the map, despite the parameter name, this is not necessaraly a new item.
// WARNING: does -not- check volume or stack charges. player functions (drop etc) should use
// map::add_item_or_charges
void map::add_item(const int x, const int y, item new_item, const int maxitems)
{

 if (new_item.is_style())
     return;
 if (new_item.made_of(LIQUID) && has_flag(swimmable, x, y))
     return;
 if (!INBOUNDS(x, y))
     return;
 if (has_flag(destroy_item, x, y) || (i_at(x,y).size() >= maxitems))
 {
     return;
 }

 const int nonant = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE;
 const int lx = x % SEEX;
 const int ly = y % SEEY;
 grid[nonant]->itm[lx][ly].push_back(new_item);
 if (new_item.active)
  grid[nonant]->active_item_count++;
}

void map::process_active_items(game *g)
{
 for (int gx = 0; gx < my_MAPSIZE; gx++) {
  for (int gy = 0; gy < my_MAPSIZE; gy++) {
   if (grid[gx + gy * my_MAPSIZE]->active_item_count > 0)
    process_active_items_in_submap(g, gx + gy * my_MAPSIZE);
  }
 }
}

void map::process_active_items_in_submap(game *g, const int nonant)
{
	it_tool* tmp;
	iuse use;
	for (int i = 0; i < SEEX; i++) {
		for (int j = 0; j < SEEY; j++) {
			std::vector<item> *items = &(grid[nonant]->itm[i][j]);
			for (int n = 0; n < items->size(); n++) {
				if ((*items)[n].active ||
				((*items)[n].is_container() && (*items)[n].contents.size() > 0 && (*items)[n].contents[0].active))
				{
					if ((*items)[n].is_food()) {	// food items
						if ((*items)[n].has_flag("HOT")) {
							(*items)[n].item_counter--;
							if ((*items)[n].item_counter == 0) {
								(*items)[n].item_tags.erase("HOT");
								(*items)[n].active = false;
								grid[nonant]->active_item_count--;
							}
						}
					} else if ((*items)[n].is_food_container()) {	// food in containers
						if ((*items)[n].contents[0].has_flag("HOT")) {
							(*items)[n].contents[0].item_counter--;
							if ((*items)[n].contents[0].item_counter == 0) {
								(*items)[n].contents[0].item_tags.erase("HOT");
								(*items)[n].contents[0].active = false;
								grid[nonant]->active_item_count--;
							}
						}
					} else if ((*items)[n].type->id == "corpse") { // some corpses rez over time
					    if ((*items)[n].ready_to_revive(g))
					    {
             if (rng(0,(*items)[n].volume()) > (*items)[n].burnt)
             {
                 int mapx = (nonant % my_MAPSIZE) * SEEX + i;
                 int mapy = (nonant / my_MAPSIZE) * SEEY + j;
                 if (g->u_see(mapx, mapy))
                 {
                     g->add_msg(_("A nearby corpse rises and moves towards you!"));
                 }
                 g->revive_corpse(mapx, mapy, n);
             } else {
                 (*items)[n].active = false;
             }
					    }
					} else if	(!(*items)[n].is_tool()) { // It's probably a charger gun
						(*items)[n].active = false;
						(*items)[n].charges = 0;
					} else {
						tmp = dynamic_cast<it_tool*>((*items)[n].type);
						if (tmp->use != &iuse::none)
						{
						    (use.*tmp->use)(g, &(g->u), &((*items)[n]), true);
						}
						if (tmp->turns_per_charge > 0 && int(g->turn) % tmp->turns_per_charge ==0)
						(*items)[n].charges--;
						if ((*items)[n].charges <= 0) {
						    if (tmp->use != &iuse::none)
						    {
							    (use.*tmp->use)(g, &(g->u), &((*items)[n]), false);
							}
							if (tmp->revert_to == "null" || (*items)[n].charges == -1) {
								items->erase(items->begin() + n);
								grid[nonant]->active_item_count--;
								n--;
							} else
								(*items)[n].type = g->itypes[tmp->revert_to];
						}
					}
				}
			}
		}
	}
}

std::list<item> map::use_amount(const point origin, const int range, const itype_id type, const int amount,
                     const bool use_container)
{
 std::list<item> ret;
 int quantity = amount;
 for (int radius = 0; radius <= range && quantity > 0; radius++) {
  for (int x = origin.x - radius; x <= origin.x + radius; x++) {
   for (int y = origin.y - radius; y <= origin.y + radius; y++) {
    if (rl_dist(origin.x, origin.y, x, y) >= radius) {
     for (int n = 0; n < i_at(x, y).size() && quantity > 0; n++) {
      item* curit = &(i_at(x, y)[n]);
      bool used_contents = false;
      for (int m = 0; m < curit->contents.size() && quantity > 0; m++) {
       if (curit->contents[m].type->id == type) {
        ret.push_back(curit->contents[m]);
        quantity--;
        curit->contents.erase(curit->contents.begin() + m);
        m--;
        used_contents = true;
       }
      }
      if (use_container && used_contents) {
       i_rem(x, y, n);
       n--;
      } else if (curit->type->id == type && quantity > 0) {
       ret.push_back(*curit);
       quantity--;
       i_rem(x, y, n);
       n--;
      }
     }
    }
   }
  }
 }
 return ret;
}

std::list<item> map::use_charges(const point origin, const int range, const itype_id type, const int amount)
{
 std::list<item> ret;
 int quantity = amount;
 for (int radius = 0; radius <= range && quantity > 0; radius++) {
  for (int x = origin.x - radius; x <= origin.x + radius; x++) {
   for (int y = origin.y - radius; y <= origin.y + radius; y++) {
    if (rl_dist(origin.x, origin.y, x, y) >= radius) {
      int vpart = -1;
      vehicle *veh = veh_at(x, y, vpart);

      if (veh) { // check if a vehicle part is present to provide water/power
        const int kpart = veh->part_with_feature(vpart, vpf_kitchen);
        const int weldpart = veh->part_with_feature(vpart, vpf_weldrig);

        if (kpart >= 0) { // we have a kitchen, now to see what to drain
          ammotype ftype = "NULL";

          if (type == "water_clean")
            ftype = "water";
          else if (type == "hotplate")
            ftype = "battery";

          item tmp = item_controller->create(type, 0); //TODO add a sane birthday arg
          tmp.charges = veh->drain(ftype, quantity);
          quantity -= tmp.charges;
          ret.push_back(tmp);

          if (quantity == 0)
            return ret;
        }
        
        if (weldpart >= 0) { // we have a weldrig, now to see what to drain
          ammotype ftype = "NULL";

          if (type == "welder")
            ftype = "battery";
          else if (type == "soldering_iron")
            ftype = "battery";

          item tmp = item_controller->create(type, 0); //TODO add a sane birthday arg
          tmp.charges = veh->drain(ftype, quantity);
          quantity -= tmp.charges;
          ret.push_back(tmp);

          if (quantity == 0)
            return ret;
        }
      }

     for (int n = 0; n < i_at(x, y).size(); n++) {
      item* curit = &(i_at(x, y)[n]);
// Check contents first
      for (int m = 0; m < curit->contents.size() && quantity > 0; m++) {
       if (curit->contents[m].type->id == type) {
        if (curit->contents[m].charges <= quantity) {
         ret.push_back(curit->contents[m]);
         quantity -= curit->contents[m].charges;
         if (curit->contents[m].destroyed_at_zero_charges()) {
          curit->contents.erase(curit->contents.begin() + m);
          m--;
         } else
          curit->contents[m].charges = 0;
        } else {
         item tmp = curit->contents[m];
         tmp.charges = quantity;
         ret.push_back(tmp);
         curit->contents[m].charges -= quantity;
         return ret;
        }
       }
      }
// Now check the actual item
      if (curit->type->id == type) {
       if (curit->charges <= quantity) {
        ret.push_back(*curit);
        quantity -= curit->charges;
        if (curit->destroyed_at_zero_charges()) {
         i_rem(x, y, n);
         n--;
        } else
         curit->charges = 0;
       } else {
        item tmp = *curit;
        tmp.charges = quantity;
        ret.push_back(tmp);
        curit->charges -= quantity;
        return ret;
       }
      }
     }
    }
   }
  }
 }
 return ret;
}

trap_id& map::tr_at(const int x, const int y)
{
 if (!INBOUNDS(x, y)) {
  nultrap = tr_null;
  return nultrap;	// Out-of-bounds, return our null trap
 }
/*
 int nonant;
 cast_to_nonant(x, y, nonant);
*/
 const int nonant = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE;

 const int lx = x % SEEX;
 const int ly = y % SEEY;
 if (lx < 0 || lx >= SEEX || ly < 0 || ly >= SEEY) {
  debugmsg("tr_at contained bad x:y %d:%d", lx, ly);
  nultrap = tr_null;
  return nultrap;	// Out-of-bounds, return our null trap
 }

 if (terlist[ grid[nonant]->ter[lx][ly] ].trap != tr_null) {
  nultrap = terlist[ grid[nonant]->ter[lx][ly] ].trap;
  return nultrap;
 }

 return grid[nonant]->trp[lx][ly];
}

void map::add_trap(const int x, const int y, const trap_id t)
{
 if (!INBOUNDS(x, y))
  return;
 const int nonant = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE;

 const int lx = x % SEEX;
 const int ly = y % SEEY;
 grid[nonant]->trp[lx][ly] = t;
}

void map::disarm_trap(game *g, const int x, const int y)
{
  int skillLevel = g->u.skillLevel("traps");

 if (tr_at(x, y) == tr_null) {
  debugmsg("Tried to disarm a trap where there was none (%d %d)", x, y);
  return;
 }

 const int tSkillLevel = g->u.skillLevel("traps");
 const int diff = g->traps[tr_at(x, y)]->difficulty;
 int roll = rng(tSkillLevel, 4 * tSkillLevel);

 while ((rng(5, 20) < g->u.per_cur || rng(1, 20) < g->u.dex_cur) && roll < 50)
  roll++;
 if (roll >= diff) {
  g->add_msg(_("You disarm the trap!"));
  std::vector<itype_id> comp = g->traps[tr_at(x, y)]->components;
  for (int i = 0; i < comp.size(); i++) {
   if (comp[i] != "null")
    spawn_item(x, y, comp[i], 0, 0, 1);
  }
  if( tr_at(x, y) == tr_engine ) {
      for (int i = -1; i <= 1; i++) {
          for (int j = -1; j <= 1; j++) {
              if (i != 0 || j != 0) {
                  tr_at(x + i, y + j) = tr_null;
              }
          }
      }
  }
  tr_at(x, y) = tr_null;
  if(diff > 1.25 * skillLevel) // failure might have set off trap
    g->u.practice(g->turn, "traps", 1.5*(diff - skillLevel));
 } else if (roll >= diff * .8) {
  g->add_msg(_("You fail to disarm the trap."));
  if(diff > 1.25 * skillLevel)
    g->u.practice(g->turn, "traps", 1.5*(diff - skillLevel));
 }
 else {
  g->add_msg(_("You fail to disarm the trap, and you set it off!"));
  trap* tr = g->traps[tr_at(x, y)];
  trapfunc f;
  (f.*(tr->act))(g, x, y);
  if(diff - roll <= 6)
   // Give xp for failing, but not if we failed terribly (in which
   // case the trap may not be disarmable).
   g->u.practice(g->turn, "traps", 2*diff);
 }
}

field& map::field_at(const int x, const int y)
{
 if (!INBOUNDS(x, y)) {
  nulfield = field();
  return nulfield;
 }

 const int nonant = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE;

 const int lx = x % SEEX;
 const int ly = y % SEEY;
 return grid[nonant]->fld[lx][ly];
}

bool map::add_field(game *g, const point p, const field_id t, unsigned int density, const int age)
{
	if (!INBOUNDS(p.x, p.y))
		return false;

	if (density > 3)
		density = 3;
	if (density <= 0)
		return false;
	const int nonant = int(p.x / SEEX) + int(p.y / SEEY) * my_MAPSIZE;

	const int lx = p.x % SEEX;
	const int ly = p.y % SEEY;
	if (!grid[nonant]->fld[lx][ly].findField(t)) //TODO: Update overall field_count appropriately. This is the spirit of "fd_null" that it used to be.
		grid[nonant]->field_count++; //Only adding it to the count if it doesn't exist.
	grid[nonant]->fld[lx][ly].addField(t, density, age); //This will insert and/or update the field.
	if(g != NULL && p.x == g->u.posx && p.y == g->u.posy)
		step_in_field(p.x,p.y,g); //Hit the player with the field if it spawned on top of them.
	return true;
}

bool map::add_field(game *g, const int x, const int y,
					const field_id t, const unsigned char new_density)
{
 return this->add_field(g,point(x,y),t,new_density,0);
}

void map::remove_field(const int x, const int y, const field_id field_to_remove)
{
 if (!INBOUNDS(x, y))
  return;
 const int nonant = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE;

 const int lx = x % SEEX;
 const int ly = y % SEEY;
 if (grid[nonant]->fld[lx][ly].findField(field_to_remove)) //same as checking for fd_null in the old system
  grid[nonant]->field_count--;
 grid[nonant]->fld[lx][ly].removeField(field_to_remove);
}

computer* map::computer_at(const int x, const int y)
{
 if (!INBOUNDS(x, y))
  return NULL;

 const int nonant = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE;

 if (grid[nonant]->comp.name == "")
  return NULL;
 return &(grid[nonant]->comp);
}

bool map::allow_camp(const int x, const int y, const int radius)
{
	return camp_at(x, y, radius) == NULL;
}

basecamp* map::camp_at(const int x, const int y, const int radius)
{
	// locate the nearest camp in a CAMPSIZE radius
 if (!INBOUNDS(x, y))
  return NULL;

 const int sx = std::max(0, x / SEEX - CAMPSIZE);
 const int sy = std::max(0, y / SEEY - CAMPSIZE);
 const int ex = std::min(MAPSIZE - 1, x / SEEX + CAMPSIZE);
 const int ey = std::min(MAPSIZE - 1, y / SEEY + CAMPSIZE);

 for( int ly = sy; ly < ey; ++ly )
 {
 	for( int lx = sx; lx < ex; ++lx )
 	{
 		int nonant = lx + ly * my_MAPSIZE;
 		if (grid[nonant]->camp.is_valid())
 		{
 			// we only allow on camp per size radius, kinda
 			return &(grid[nonant]->camp);
 		}
 	}
 }

 return NULL;
}

void map::add_camp(const std::string& name, const int x, const int y)
{
	if (!allow_camp(x, y)) {
		dbg(D_ERROR) << "map::add_camp: Attempting to add camp when one in local area.";
		return;
	}

	const int nonant = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE;
	grid[nonant]->camp = basecamp(name, x, y);
}

void map::debug()
{
 mvprintw(0, 0, "MAP DEBUG");
 getch();
 for (int i = 0; i <= SEEX * 2; i++) {
  for (int j = 0; j <= SEEY * 2; j++) {
   if (i_at(i, j).size() > 0) {
    mvprintw(1, 0, "%d, %d: %d items", i, j, i_at(i, j).size());
    mvprintw(2, 0, "%c, %d", i_at(i, j)[0].symbol(), i_at(i, j)[0].color());
    getch();
   }
  }
 }
 getch();
}

void map::draw(game *g, WINDOW* w, const point center)
{
 g->reset_light_level();
 const int g_light_level = (int)g->light_level();
 const int natural_sight_range = g->u.sight_range(1);
 const int light_sight_range = g->u.sight_range(g_light_level);
 const int lowlight_sight_range = std::max(g_light_level / 2, natural_sight_range);
 const int max_sight_range = g->u.unimpaired_range();
 const bool u_is_boomered = g->u.has_disease("boomered");
 const int u_clairvoyance = g->u.clairvoyance();
 const bool u_sight_impaired = g->u.sight_impaired();

 for (int i = 0; i < my_MAPSIZE * my_MAPSIZE; i++) {
  if (!grid[i])
   debugmsg("grid %d (%d, %d) is null! mapbuffer size = %d",
            i, i % my_MAPSIZE, i / my_MAPSIZE, MAPBUFFER.size());
 }

 for  (int realx = center.x - getmaxx(w)/2; realx <= center.x + getmaxx(w)/2; realx++) {
  for (int realy = center.y - getmaxy(w)/2; realy <= center.y + getmaxy(w)/2; realy++) {
   const int dist = rl_dist(g->u.posx, g->u.posy, realx, realy);
   int sight_range = light_sight_range;
   int low_sight_range = lowlight_sight_range;
   bool bRainOutside = false;
   // While viewing indoor areas use lightmap model
   if (!is_outside(realx, realy)) {
    sight_range = natural_sight_range;
   // Don't display area as shadowy if it's outside and illuminated by natural light
   } else if (dist <= light_sight_range) {
    low_sight_range = std::max(g_light_level, natural_sight_range);
    bRainOutside = true;
   }

   // I've moved this part above loops without even thinking that
   // this must stay here...
   int real_max_sight_range = light_sight_range > max_sight_range ? light_sight_range : max_sight_range;
   int distance_to_look = real_max_sight_range;
   distance_to_look = DAYLIGHT_LEVEL;

   bool can_see = pl_sees(g->u.posx, g->u.posy, realx, realy, distance_to_look);
   lit_level lit = light_at(realx, realy);

   // now we're gonna adjust real_max_sight, to cover some nearby "highlights",
   // but at the same time changing light-level depending on distance,
   // to create actual "gradual" stuff
   // Also we'll try to ALWAYS show LL_BRIGHT stuff independent of where it is...
   if (lit != LL_BRIGHT) {
       if (dist > real_max_sight_range) {
           int intLit = (int)lit - (dist - real_max_sight_range)/2;
           if (intLit < 0) intLit = LL_DARK;
           lit = (lit_level)intLit;
       }
   }
   // additional case for real_max_sight_range
   // if both light_sight_range and max_sight_range were small
   // it means we really have limited visibility (e.g. inside a pit)
   // and we shouldn't touch that
   if (lit > LL_DARK && real_max_sight_range > 1) {
       real_max_sight_range = distance_to_look;
   }

   if ((g->u.has_active_bionic("bio_night") && dist < 15 && dist > natural_sight_range) || // if bio_night active, blackout 15 tile radius around player
       dist > real_max_sight_range ||
       (dist > light_sight_range &&
         (lit == LL_DARK ||
         (u_sight_impaired && lit != LL_BRIGHT) ||
	  !can_see))) {
    if (u_is_boomered)
   	 mvwputch(w, realy+getmaxy(w)/2 - center.y, realx+getmaxx(w)/2 - center.x, c_magenta, '#');
    else
         mvwputch(w, realy+getmaxy(w)/2 - center.y, realx+getmaxx(w)/2 - center.x, c_dkgray, '#');
   } else if (dist > light_sight_range && u_sight_impaired && lit == LL_BRIGHT) {
    if (u_is_boomered)
     mvwputch(w, realy+getmaxy(w)/2 - center.y, realx+getmaxx(w)/2 - center.x, c_pink, '#');
    else
     mvwputch(w, realy+getmaxy(w)/2 - center.y, realx+getmaxx(w)/2 - center.x, c_ltgray, '#');
   } else if (dist <= u_clairvoyance || can_see) {
    if (bRainOutside && INBOUNDS(realx, realy) && is_outside(realx, realy))
     g->mapRain[realy + getmaxy(w)/2 - center.y][realx + getmaxx(w)/2 - center.x] = true;
    drawsq(w, g->u, realx, realy, false, true, center.x, center.y,
           (dist > low_sight_range && LL_LIT > lit) ||
	   (dist > sight_range && LL_LOW == lit),
           LL_BRIGHT == lit);
   } else {
    mvwputch(w, realy+getmaxy(w)/2 - center.y, realx+getmaxx(w)/2 - center.x, c_black,' ');
   }
  }
 }
 int atx = getmaxx(w)/2 + g->u.posx - center.x, aty = getmaxy(w)/2 + g->u.posy - center.y;
 if (atx >= 0 && atx < TERRAIN_WINDOW_WIDTH && aty >= 0 && aty < TERRAIN_WINDOW_HEIGHT) {
  mvwputch(w, aty, atx, g->u.color(), '@');
  g->mapRain[aty][atx] = false;
 }
}

void map::drawsq(WINDOW* w, player &u, const int x, const int y, const bool invert_arg,
                 const bool show_items_arg, const int view_center_x_arg, const int view_center_y_arg,
                 const bool low_light, const bool bright_light)
{
 bool invert = invert_arg;
 bool show_items = show_items_arg;
 int cx = view_center_x_arg;
 int cy = view_center_y_arg;
 if (!INBOUNDS(x, y))
  return;	// Out of bounds
 if (cx == -1)
  cx = u.posx;
 if (cy == -1)
  cy = u.posy;
 const int k = x + getmaxx(w)/2 - cx;
 const int j = y + getmaxy(w)/2 - cy;
 nc_color tercol;
 const ter_id curr_ter = ter(x,y);
 const furn_id curr_furn = furn(x,y);
 const trap_id curr_trap = tr_at(x, y);
 field &curr_field = field_at(x, y);
 const std::vector<item> curr_items = i_at(x, y);
 long sym;
 bool hi = false;
 bool graf = false;
 bool normal_tercol = false, drew_field = false;
 if (has_furn(x, y)) {
  sym = furnlist[curr_furn].sym;
  tercol = furnlist[curr_furn].color;
 } else {
  sym = terlist[curr_ter].sym;
  tercol = terlist[curr_ter].color;
 }
 if (u.has_disease("boomered"))
  tercol = c_magenta;
 else if ( u.has_nv() )
  tercol = (bright_light) ? c_white : c_ltgreen;
 else if (low_light)
  tercol = c_dkgray;
 else
  normal_tercol = true;
 if (move_cost(x, y) == 0 && has_flag(swimmable, x, y) && !u.underwater)
  show_items = false;	// Can only see underwater items if WE are underwater
// If there's a trap here, and we have sufficient perception, draw that instead
 if (curr_trap != tr_null &&
     u.per_cur - u.encumb(bp_eyes) >= (*traps)[curr_trap]->visibility) {
  tercol = (*traps)[curr_trap]->color;
  if ((*traps)[curr_trap]->sym == '%') {
   switch(rng(1, 5)) {
    case 1: sym = '*'; break;
    case 2: sym = '0'; break;
    case 3: sym = '8'; break;
    case 4: sym = '&'; break;
    case 5: sym = '+'; break;
   }
  } else
   sym = (*traps)[curr_trap]->sym;
 }
// If there's a field here, draw that instead (unless its symbol is %)
 if (curr_field.fieldCount() > 0 && curr_field.findField(curr_field.fieldSymbol()) &&
     fieldlist[curr_field.fieldSymbol()].sym != '&') {
		 tercol = fieldlist[curr_field.fieldSymbol()].color[curr_field.findField(curr_field.fieldSymbol())->getFieldDensity() - 1];
  drew_field = true;
  if (fieldlist[curr_field.fieldSymbol()].sym == '*') {
   switch (rng(1, 5)) {
    case 1: sym = '*'; break;
    case 2: sym = '0'; break;
    case 3: sym = '8'; break;
    case 4: sym = '&'; break;
    case 5: sym = '+'; break;
   }
  } else if (fieldlist[curr_field.fieldSymbol()].sym != '%' ||
             curr_items.size() > 0) {
   sym = fieldlist[curr_field.fieldSymbol()].sym;
   drew_field = false;
  }
 }
// If there's items here, draw those instead
 if (show_items && !has_flag(container, x, y) && curr_items.size() > 0 && !drew_field) {
  if (sym != '.')
   hi = true;
  else {
   tercol = curr_items[curr_items.size() - 1].color();
   if (curr_items.size() > 1)
    invert = !invert;
   sym = curr_items[curr_items.size() - 1].symbol();
  }
 }

 int veh_part = 0;
 vehicle *veh = veh_at(x, y, veh_part);
 if (veh) {
  sym = special_symbol (veh->face.dir_symbol(veh->part_sym(veh_part)));
  if (normal_tercol)
   tercol = veh->part_color(veh_part);
 }
 // If there's graffiti here, change background color
 if(graffiti_at(x,y).contents)
  graf = true;

 //suprise, we're not done, if it's a wall adjacent to an other, put the right glyph
 if(sym == LINE_XOXO || sym == LINE_OXOX)//vertical or horizontal
  sym = determine_wall_corner(x, y, sym);

 if (invert)
  mvwputch_inv(w, j, k, tercol, sym);
 else if (hi)
  mvwputch_hi (w, j, k, tercol, sym);
 else if (graf)
  mvwputch    (w, j, k, red_background(tercol), sym);
 else
  mvwputch    (w, j, k, tercol, sym);
}

/*
map::sees based off code by Steve Register [arns@arns.freeservers.com]
http://roguebasin.roguelikedevelopment.org/index.php?title=Simple_Line_of_Sight
*/
bool map::sees(const int Fx, const int Fy, const int Tx, const int Ty,
               const int range, int &tc)
{
 const int dx = Tx - Fx;
 const int dy = Ty - Fy;
 const int ax = abs(dx) << 1;
 const int ay = abs(dy) << 1;
 const int sx = SGN(dx);
 const int sy = SGN(dy);
 int x = Fx;
 int y = Fy;
 int t = 0;
 int st;

 if (range >= 0 && (abs(dx) > range || abs(dy) > range))
  return false;	// Out of range!
 if (ax > ay) { // Mostly-horizontal line
  st = SGN(ay - (ax >> 1));
// Doing it "backwards" prioritizes straight lines before diagonal.
// This will help avoid creating a string of zombies behind you and will
// promote "mobbing" behavior (zombies surround you to beat on you)
  for (tc = abs(ay - (ax >> 1)) * 2 + 1; tc >= -1; tc--) {
   t = tc * st;
   x = Fx;
   y = Fy;
   do {
    if (t > 0) {
     y += sy;
     t -= ax;
    }
    x += sx;
    t += ay;
    if (x == Tx && y == Ty) {
     tc *= st;
     return true;
    }
   } while ((trans(x, y)) && (INBOUNDS(x,y)));
  }
  return false;
 } else { // Same as above, for mostly-vertical lines
  st = SGN(ax - (ay >> 1));
  for (tc = abs(ax - (ay >> 1)) * 2 + 1; tc >= -1; tc--) {
  t = tc * st;
  x = Fx;
  y = Fy;
   do {
    if (t > 0) {
     x += sx;
     t -= ay;
    }
    y += sy;
    t += ax;
    if (x == Tx && y == Ty) {
     tc *= st;
     return true;
    }
   } while ((trans(x, y)) && (INBOUNDS(x,y)));
  }
  return false;
 }
 return false; // Shouldn't ever be reached, but there it is.
}

bool map::clear_path(const int Fx, const int Fy, const int Tx, const int Ty,
                     const int range, const int cost_min, const int cost_max, int &tc)
{
 const int dx = Tx - Fx;
 const int dy = Ty - Fy;
 const int ax = abs(dx) << 1;
 const int ay = abs(dy) << 1;
 const int sx = SGN(dx);
 const int sy = SGN(dy);
 int x = Fx;
 int y = Fy;
 int t = 0;
 int st;

 if (range >= 0 && (abs(dx) > range || abs(dy) > range))
  return false;	// Out of range!
 if (ax > ay) { // Mostly-horizontal line
  st = SGN(ay - (ax >> 1));
// Doing it "backwards" prioritizes straight lines before diagonal.
// This will help avoid creating a string of zombies behind you and will
// promote "mobbing" behavior (zombies surround you to beat on you)
  for (tc = abs(ay - (ax >> 1)) * 2 + 1; tc >= -1; tc--) {
   t = tc * st;
   x = Fx;
   y = Fy;
   do {
    if (t > 0) {
     y += sy;
     t -= ax;
    }
    x += sx;
    t += ay;
    if (x == Tx && y == Ty) {
     tc *= st;
     return true;
    }
   } while (move_cost(x, y) >= cost_min && move_cost(x, y) <= cost_max &&
            INBOUNDS(x, y));
  }
  return false;
 } else { // Same as above, for mostly-vertical lines
  st = SGN(ax - (ay >> 1));
  for (tc = abs(ax - (ay >> 1)) * 2 + 1; tc >= -1; tc--) {
  t = tc * st;
  x = Fx;
  y = Fy;
   do {
    if (t > 0) {
     x += sx;
     t -= ay;
    }
    y += sy;
    t += ax;
    if (x == Tx && y == Ty) {
     tc *= st;
     return true;
    }
   } while (move_cost(x, y) >= cost_min && move_cost(x, y) <= cost_max &&
            INBOUNDS(x,y));
  }
  return false;
 }
 return false; // Shouldn't ever be reached, but there it is.
}

// Bash defaults to true.
std::vector<point> map::route(const int Fx, const int Fy, const int Tx, const int Ty, const bool can_bash)
{
/* TODO: If the origin or destination is out of bound, figure out the closest
 * in-bounds point and go to that, then to the real origin/destination.
 */

 if (!INBOUNDS(Fx, Fy) || !INBOUNDS(Tx, Ty)) {
  int linet;
  if (sees(Fx, Fy, Tx, Ty, -1, linet))
   return line_to(Fx, Fy, Tx, Ty, linet);
  else {
   std::vector<point> empty;
   return empty;
  }
 }
// First, check for a simple straight line on flat ground
 int linet = 0;
 if (clear_path(Fx, Fy, Tx, Ty, -1, 2, 2, linet))
  return line_to(Fx, Fy, Tx, Ty, linet);
/*
 if (move_cost(Tx, Ty) == 0)
  debugmsg("%d:%d wanted to move to %d:%d, a %s!", Fx, Fy, Tx, Ty,
           tername(Tx, Ty).c_str());
 if (move_cost(Fx, Fy) == 0)
  debugmsg("%d:%d, a %s, wanted to move to %d:%d!", Fx, Fy,
           tername(Fx, Fy).c_str(), Tx, Ty);
*/
 std::vector<point> open;
 astar_list list[SEEX * MAPSIZE][SEEY * MAPSIZE];
 int score	[SEEX * MAPSIZE][SEEY * MAPSIZE];
 int gscore	[SEEX * MAPSIZE][SEEY * MAPSIZE];
 point parent	[SEEX * MAPSIZE][SEEY * MAPSIZE];
 int startx = Fx - 4, endx = Tx + 4, starty = Fy - 4, endy = Ty + 4;
 if (Tx < Fx) {
  startx = Tx - 4;
  endx = Fx + 4;
 }
 if (Ty < Fy) {
  starty = Ty - 4;
  endy = Fy + 4;
 }
 if (startx < 0)
  startx = 0;
 if (starty < 0)
  starty = 0;
 if (endx > SEEX * my_MAPSIZE - 1)
  endx = SEEX * my_MAPSIZE - 1;
 if (endy > SEEY * my_MAPSIZE - 1)
  endy = SEEY * my_MAPSIZE - 1;

 for (int x = startx; x <= endx; x++) {
  for (int y = starty; y <= endy; y++) {
   list  [x][y] = ASL_NONE; // Init to not being on any list
   score [x][y] = 0;        // No score!
   gscore[x][y] = 0;        // No score!
   parent[x][y] = point(-1, -1);
  }
 }
 list[Fx][Fy] = ASL_OPEN;
 open.push_back(point(Fx, Fy));

 bool done = false;

 do {
  //debugmsg("Open.size() = %d", open.size());
  int best = 9999;
  int index = -1;
  for (int i = 0; i < open.size(); i++) {
   if (i == 0 || score[open[i].x][open[i].y] < best) {
    best = score[open[i].x][open[i].y];
    index = i;
   }
  }
  for (int x = open[index].x - 1; x <= open[index].x + 1; x++) {
   for (int y = open[index].y - 1; y <= open[index].y + 1; y++) {
    if (x == open[index].x && y == open[index].y)
     y++;	// Skip the current square
    if (x == Tx && y == Ty) {
     done = true;
     parent[x][y] = open[index];
    } else if (x >= startx && x <= endx && y >= starty && y <= endy &&
               (move_cost(x, y) > 0 || (can_bash && has_flag(bashable, x, y)))) {
     if (list[x][y] == ASL_NONE) {	// Not listed, so make it open
      list[x][y] = ASL_OPEN;
      open.push_back(point(x, y));
      parent[x][y] = open[index];
      gscore[x][y] = gscore[open[index].x][open[index].y] + move_cost(x, y);
      if (ter(x, y) == t_door_c)
       gscore[x][y] += 4;	// A turn to open it and a turn to move there
      else if (move_cost(x, y) == 0 && (can_bash && has_flag(bashable, x, y)))
       gscore[x][y] += 18;	// Worst case scenario with damage penalty
      score[x][y] = gscore[x][y] + 2 * rl_dist(x, y, Tx, Ty);
     } else if (list[x][y] == ASL_OPEN) { // It's open, but make it our child
      int newg = gscore[open[index].x][open[index].y] + move_cost(x, y);
      if (ter(x, y) == t_door_c)
       newg += 4;	// A turn to open it and a turn to move there
      else if (move_cost(x, y) == 0 && (can_bash && has_flag(bashable, x, y)))
       newg += 18;	// Worst case scenario with damage penalty
      if (newg < gscore[x][y]) {
       gscore[x][y] = newg;
       parent[x][y] = open[index];
       score [x][y] = gscore[x][y] + 2 * rl_dist(x, y, Tx, Ty);
      }
     }
    }
   }
  }
  list[open[index].x][open[index].y] = ASL_CLOSED;
  open.erase(open.begin() + index);
 } while (!done && open.size() > 0);

 std::vector<point> tmp;
 std::vector<point> ret;
 if (done) {
  point cur(Tx, Ty);
  while (cur.x != Fx || cur.y != Fy) {
   //debugmsg("Retracing... (%d:%d) => [%d:%d] => (%d:%d)", Tx, Ty, cur.x, cur.y, Fx, Fy);
   tmp.push_back(cur);
   if (rl_dist(cur.x, cur.y, parent[cur.x][cur.y].x, parent[cur.x][cur.y].y)>1){
    debugmsg("Jump in our route! %d:%d->%d:%d", cur.x, cur.y,
             parent[cur.x][cur.y].x, parent[cur.x][cur.y].y);
    return ret;
   }
   cur = parent[cur.x][cur.y];
  }
  for (int i = tmp.size() - 1; i >= 0; i--)
   ret.push_back(tmp[i]);
 }
 return ret;
}

void map::save(overmap *om, unsigned const int turn, const int x, const int y, const int z)
{
 for (int gridx = 0; gridx < my_MAPSIZE; gridx++) {
  for (int gridy = 0; gridy < my_MAPSIZE; gridy++)
   saven(om, turn, x, y, z, gridx, gridy);
 }
}

void map::load(game *g, const int wx, const int wy, const int wz, const bool update_vehicle)
{
 for (int gridx = 0; gridx < my_MAPSIZE; gridx++) {
  for (int gridy = 0; gridy < my_MAPSIZE; gridy++) {
   if (!loadn(g, wx, wy, wz, gridx, gridy, update_vehicle))
    loadn(g, wx, wy, wz, gridx, gridy, update_vehicle);
  }
 }
}

void map::shift(game *g, const int wx, const int wy, const int wz, const int sx, const int sy)
{
// Special case of 0-shift; refresh the map
 if (sx == 0 && sy == 0) {
  return; // Skip this?
  for (int gridx = 0; gridx < my_MAPSIZE; gridx++) {
   for (int gridy = 0; gridy < my_MAPSIZE; gridy++) {
    if (!loadn(g, wx+sx, wy+sy, wz, gridx, gridy))
     loadn(g, wx+sx, wy+sy, wz, gridx, gridy);
   }
  }
  return;
 }

// if player is in vehicle, (s)he must be shifted with vehicle too
 if (g->u.in_vehicle && (sx !=0 || sy != 0)) {
  g->u.posx -= sx * SEEX;
  g->u.posy -= sy * SEEY;
 }

 // Clear vehicle list and rebuild after shift
 clear_vehicle_cache();
 vehicle_list.clear();
// Shift the map sx submaps to the right and sy submaps down.
// sx and sy should never be bigger than +/-1.
// wx and wy are our position in the world, for saving/loading purposes.
 if (sx >= 0) {
  for (int gridx = 0; gridx < my_MAPSIZE; gridx++) {
   if (sy >= 0) {
    for (int gridy = 0; gridy < my_MAPSIZE; gridy++) {
/*
     if (gridx < sx || gridy < sy) {
      saven(&(g->cur_om), g->turn, wx, wy, gridx, gridy);
     }
*/
     if (gridx + sx < my_MAPSIZE && gridy + sy < my_MAPSIZE) {
      copy_grid(gridx + gridy * my_MAPSIZE,
                gridx + sx + (gridy + sy) * my_MAPSIZE);
      update_vehicle_list(gridx + gridy * my_MAPSIZE);
     } else if (!loadn(g, wx + sx, wy + sy, wz, gridx, gridy))
      loadn(g, wx + sx, wy + sy, wz, gridx, gridy);
    }
   } else { // sy < 0; work through it backwards
    for (int gridy = my_MAPSIZE - 1; gridy >= 0; gridy--) {
/*
     if (gridx < sx || gridy - my_MAPSIZE >= sy) {
      saven(&(g->cur_om), g->turn, wx, wy, gridx, gridy);
     }
*/
     if (gridx + sx < my_MAPSIZE && gridy + sy >= 0) {
      copy_grid(gridx + gridy * my_MAPSIZE,
                gridx + sx + (gridy + sy) * my_MAPSIZE);
      update_vehicle_list(gridx + gridy * my_MAPSIZE);
     } else if (!loadn(g, wx + sx, wy + sy, wz, gridx, gridy))
      loadn(g, wx + sx, wy + sy, wz, gridx, gridy);
    }
   }
  }
 } else { // sx < 0; work through it backwards
  for (int gridx = my_MAPSIZE - 1; gridx >= 0; gridx--) {
   if (sy >= 0) {
    for (int gridy = 0; gridy < my_MAPSIZE; gridy++) {
/*
     if (gridx - my_MAPSIZE >= sx || gridy < sy) {
      saven(&(g->cur_om), g->turn, wx, wy, gridx, gridy);
     }
*/
     if (gridx + sx >= 0 && gridy + sy < my_MAPSIZE) {
      copy_grid(gridx + gridy * my_MAPSIZE,
                gridx + sx + (gridy + sy) * my_MAPSIZE);
      update_vehicle_list(gridx + gridy * my_MAPSIZE);
     } else if (!loadn(g, wx + sx, wy + sy, wz, gridx, gridy))
      loadn(g, wx + sx, wy + sy, wz, gridx, gridy);
    }
   } else { // sy < 0; work through it backwards
    for (int gridy = my_MAPSIZE - 1; gridy >= 0; gridy--) {
/*
     if (gridx - my_MAPSIZE >= sx || gridy - my_MAPSIZE >= sy) {
      saven(&(g->cur_om), g->turn, wx, wy, gridx, gridy);
     }
*/
     if (gridx + sx >= 0 && gridy + sy >= 0) {
      copy_grid(gridx + gridy * my_MAPSIZE,
                gridx + sx + (gridy + sy) * my_MAPSIZE);
      update_vehicle_list(gridx + gridy * my_MAPSIZE);
     } else if (!loadn(g, wx + sx, wy + sy, wz, gridx, gridy))
      loadn(g, wx + sx, wy + sy, wz, gridx, gridy);
    }
   }
  }
 }
 reset_vehicle_cache();
}

// saven saves a single nonant.  worldx and worldy are used for the file
// name and specifies where in the world this nonant is.  gridx and gridy are
// the offset from the top left nonant:
// 0,0 1,0 2,0
// 0,1 1,1 2,1
// 0,2 1,2 2,2
void map::saven(overmap *om, unsigned const int turn, const int worldx, const int worldy, const int worldz,
                const int gridx, const int gridy)
{
 dbg(D_INFO) << "map::saven(om[" << (void*)om << "], turn[" << turn <<"], worldx["<<worldx<<"], worldy["<<worldy<<"], gridx["<<gridx<<"], gridy["<<gridy<<"])";

 const int n = gridx + gridy * my_MAPSIZE;

 dbg(D_INFO) << "map::saven n: " << n;

 if ( !grid[n] || grid[n]->ter[0][0] == t_null)
 {
  dbg(D_ERROR) << "map::saven grid NULL!";
  return;
 }
 const int abs_x = om->pos().x * OMAPX * 2 + worldx + gridx,
           abs_y = om->pos().y * OMAPY * 2 + worldy + gridy;

 dbg(D_INFO) << "map::saven abs_x: " << abs_x << "  abs_y: " << abs_y;

 MAPBUFFER.add_submap(abs_x, abs_y, worldz, grid[n]);
}

// worldx & worldy specify where in the world this is;
// gridx & gridy specify which nonant:
// 0,0  1,0  2,0
// 0,1  1,1  2,1
// 0,2  1,2  2,2 etc
bool map::loadn(game *g, const int worldx, const int worldy, const int worldz, const int gridx, const int gridy,
                const bool update_vehicles)
{
 dbg(D_INFO) << "map::loadn(game[" << g << "], worldx["<<worldx<<"], worldy["<<worldy<<"], gridx["<<gridx<<"], gridy["<<gridy<<"])";

 const int absx = g->cur_om->pos().x * OMAPX * 2 + worldx + gridx,
           absy = g->cur_om->pos().y * OMAPY * 2 + worldy + gridy,
           gridn = gridx + gridy * my_MAPSIZE;

 dbg(D_INFO) << "map::loadn absx: " << absx << "  absy: " << absy
            << "  gridn: " << gridn;

 submap *tmpsub = MAPBUFFER.lookup_submap(absx, absy, worldz);
 if (tmpsub) {
  grid[gridn] = tmpsub;

  // Update vehicle data
  for( std::vector<vehicle*>::iterator it = tmpsub->vehicles.begin(),
        end = tmpsub->vehicles.end(); update_vehicles && it != end; ++it ) {

   // Only add if not tracking already.
   if( vehicle_list.find( *it ) == vehicle_list.end() ) {
    // gridx/y not correct. TODO: Fix
    (*it)->smx = gridx;
    (*it)->smy = gridy;
    vehicle_list.insert(*it);
    update_vehicle_cache(*it);
   }
  }

  // check spoiled stuff
  for(int x = 0; x < 12; x++) {
      for(int y = 0; y < 12; y++) {
          for(std::vector<item, std::allocator<item> >::iterator it = tmpsub->itm[x][y].begin();
              it != tmpsub->itm[x][y].end();) {
              if(it->goes_bad()) {
                  it_comest *food = dynamic_cast<it_comest*>(it->type);
                  int maxShelfLife = it->bday + (food->spoils * 600)*2;
                  if(g->turn >= maxShelfLife) {
                      it = tmpsub->itm[x][y].erase(it);
                  } else { ++it; }
              } else { ++it; }
          }
      }
  }

  // plantEpoch is half a season; 3 epochs pass from plant to harvest
  const int plantEpoch = 14400 * (int)OPTIONS["SEASON_LENGTH"] / 2;

  // check plants
  for (int x = 0; x < 12; x++) {
    for (int y = 0; y < 12; y++) {
      furn_id furn = tmpsub->frn[x][y];
      if (furn && (furnlist[furn].flags & mfb(plant))) {
        item seed = tmpsub->itm[x][y][0];

        while (g->turn > seed.bday + plantEpoch && furn < f_plant_harvest) {
          furn = (furn_id((int)furn + 1));
          seed.bday += plantEpoch;

          tmpsub->itm[x][y].resize(1);

          tmpsub->itm[x][y][0].bday = seed.bday;
          tmpsub->frn[x][y] = furn;
        }
      }
    }
  }

 } else { // It doesn't exist; we must generate it!
  dbg(D_INFO|D_WARNING) << "map::loadn: Missing mapbuffer data. Regenerating.";
  tinymap tmp_map(traps);
// overx, overy is where in the overmap we need to pull data from
// Each overmap square is two nonants; to prevent overlap, generate only at
//  squares divisible by 2.
  int newmapx = worldx + gridx - ((worldx + gridx) % 2);
  int newmapy = worldy + gridy - ((worldy + gridy) % 2);
  overmap* this_om = g->cur_om;

  // slightly out of bounds? to the east, south, or both?
  // cur_om is the one containing the upper-left corner of the map
  if (newmapx >= OMAPX*2){
     newmapx -= OMAPX*2;
     this_om = g->om_hori;
     if (newmapy >= OMAPY*2){
        newmapy -= OMAPY*2;
        this_om = g->om_diag;
     }
  }
  else if (newmapy >= OMAPY*2){
     newmapy -= OMAPY*2;
     this_om = g->om_vert;
  }

  if (worldx + gridx < 0)
   newmapx = worldx + gridx;
  if (worldy + gridy < 0)
   newmapy = worldy + gridy;
  tmp_map.generate(g, this_om, newmapx, newmapy, worldz, int(g->turn));
  return false;
 }
 return true;
}

void map::copy_grid(const int to, const int from)
{
 grid[to] = grid[from];
 for( std::vector<vehicle*>::iterator it = grid[to]->vehicles.begin(),
       end = grid[to]->vehicles.end(); it != end; ++it ) {
  (*it)->smx = to % my_MAPSIZE;
  (*it)->smy = to / my_MAPSIZE;
 }
}

void map::spawn_monsters(game *g)
{
 for (int gx = 0; gx < my_MAPSIZE; gx++) {
  for (int gy = 0; gy < my_MAPSIZE; gy++) {
   const int n = gx + gy * my_MAPSIZE;
   for (int i = 0; i < grid[n]->spawns.size(); i++) {
    for (int j = 0; j < grid[n]->spawns[i].count; j++) {
     int tries = 0;
     int mx = grid[n]->spawns[i].posx, my = grid[n]->spawns[i].posy;
     monster tmp(g->mtypes[grid[n]->spawns[i].type]);
     tmp.spawnmapx = g->levx + gx;
     tmp.spawnmapy = g->levy + gy;
     tmp.faction_id = grid[n]->spawns[i].faction_id;
     tmp.mission_id = grid[n]->spawns[i].mission_id;
     if (grid[n]->spawns[i].name != "NONE")
      tmp.unique_name = grid[n]->spawns[i].name;
     if (grid[n]->spawns[i].friendly)
      tmp.friendly = -1;
     int fx = mx + gx * SEEX, fy = my + gy * SEEY;

     while ((!g->is_empty(fx, fy) || !tmp.can_move_to(g, fx, fy)) &&
            tries < 10) {
      mx = (grid[n]->spawns[i].posx + rng(-3, 3)) % SEEX;
      my = (grid[n]->spawns[i].posy + rng(-3, 3)) % SEEY;
      if (mx < 0)
       mx += SEEX;
      if (my < 0)
       my += SEEY;
      fx = mx + gx * SEEX;
      fy = my + gy * SEEY;
      tries++;
     }
     if (tries != 10) {
      tmp.spawnposx = fx;
      tmp.spawnposy = fy;
      tmp.spawn(fx, fy);
      g->z.push_back(tmp);
     }
    }
   }
   grid[n]->spawns.clear();
  }
 }
}

void map::clear_spawns()
{
 for (int i = 0; i < my_MAPSIZE * my_MAPSIZE; i++)
  grid[i]->spawns.clear();
}

void map::clear_traps()
{
 for (int i = 0; i < my_MAPSIZE * my_MAPSIZE; i++) {
  for (int x = 0; x < SEEX; x++) {
   for (int y = 0; y < SEEY; y++)
    grid[i]->trp[x][y] = tr_null;
  }
 }
}


bool map::inbounds(const int x, const int y)
{
 return (x >= 0 && x < SEEX * my_MAPSIZE && y >= 0 && y < SEEY * my_MAPSIZE);
}

bool map::add_graffiti(game *g, int x, int y, std::string contents)
{
  int nx = x;
  int ny = y;
  int nonant = int(nx / SEEX) + int(ny / SEEY) * my_MAPSIZE;
  nx %= SEEX;
  ny %= SEEY;
  grid[nonant]->graf[nx][ny] = graffiti(contents);
  return true;
}

graffiti map::graffiti_at(int x, int y)
{
 if (!inbounds(x, y))
  return graffiti();
/*
 int nonant;
 cast_to_nonant(x, y, nonant);
*/
 int nonant = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE;

 x %= SEEX;
 y %= SEEY;
 return grid[nonant]->graf[x][y];
}

long map::determine_wall_corner(const int x, const int y, const long orig_sym)
{
    long sym = orig_sym;
    //LINE_NESW
    const long above = terlist[ter(x, y-1)].sym;
    const long below = terlist[ter(x, y+1)].sym;
    const long left  = terlist[ter(x-1, y)].sym;
    const long right = terlist[ter(x+1, y)].sym;

    const bool above_connects = above == sym || (above == '"' || above == '+' || above == '\'');
    const bool below_connects = below == sym || (below == '"' || below == '+' || below == '\'');
    const bool left_connects  = left  == sym || (left  == '"' || left  == '+' || left  == '\'');
    const bool right_connects = right == sym || (right == '"' || right == '+' || right == '\'');

    // -
    // |      this = - and above = | or a connectable
    if(sym == LINE_OXOX &&  (above == LINE_XOXO || above_connects))
    {
        //connects to upper
        if(left_connects)
            sym = LINE_XOOX; // ┘ left coming wall
        else if(right_connects)
            sym = LINE_XXOO;//└   right coming wall
        if(left_connects && right_connects)
            sym = LINE_XXOX; // ┴ passing by
    }

    // |
    // -      this = - and below = | or a connectable
    else if(sym == LINE_OXOX && (below == LINE_XOXO || below_connects))
    {
        //connects to lower
        if(left_connects)
            sym = LINE_OOXX; // ┐ left coming wall
        else if(right_connects)
            sym = LINE_OXXO;//┌   right coming wall
        if(left_connects && right_connects)
            sym = LINE_OXXX; // ┬ passing by
    }

    // -|       this = | and left = - or a connectable
    else if(sym == LINE_XOXO && (left == LINE_OXOX || left_connects))
    {
        //connexts to left
        if(above_connects)
            sym = LINE_XOOX; // ┘ north coming wall
        else if(below_connects )
            sym = LINE_OOXX;//┐   south coming wall
        if(above_connects && below_connects)
            sym = LINE_XOXX; // ┤ passing by
    }

    // |-       this = | and right = - or a connectable
    else if(sym == LINE_XOXO && (right == LINE_OXOX || right_connects))
    {
        //connects to right
        if(above_connects)
            sym = LINE_XXOO; // └ north coming wall
        else if(below_connects)
            sym = LINE_OXXO;// ┌   south coming wall
        if(above_connects && below_connects)
            sym = LINE_XXXO; // ├ passing by
    }

    if(above == LINE_XOXO && left == LINE_OXOX && above == below && left == right)
        sym = LINE_XXXX; // ┼ crossway

    return sym;
}

float map::light_transparency(const int x, const int y) const
{
  return transparency_cache[x][y];
}

void map::build_outside_cache(const game *g)
{
    if (g->levz < 0)
    {
        memset(outside_cache, false, sizeof(outside_cache));
        return;
    }
    memset(outside_cache, true, sizeof(outside_cache));

    for(int x = 0; x < SEEX * my_MAPSIZE; x++)
    {
        for(int y = 0; y < SEEY * my_MAPSIZE; y++)
        {
            if( has_flag_ter_or_furn(indoors, x, y))
            {
                for( int dx = -1; dx <= 1; dx++ )
                {
                    for( int dy = -1; dy <= 1; dy++ )
                    {
                        if(INBOUNDS(x + dx, y + dy))
                        {
                            outside_cache[x + dx][y + dy] = false;
                        }
                    }
                }
            }
        }
    }
}

// TODO Consider making this just clear the cache and dynamically fill it in as trans() is called
void map::build_transparency_cache()
{
 for(int x = 0; x < my_MAPSIZE * SEEX; x++) {
  for(int y = 0; y < my_MAPSIZE * SEEY; y++) {

   // Default to fully transparent.
   transparency_cache[x][y] = LIGHT_TRANSPARENCY_CLEAR;

   if (!has_flag_ter_and_furn(transparent, x, y)) {
    transparency_cache[x][y] = LIGHT_TRANSPARENCY_SOLID;
    continue;
   }

   //Quoted to see if this works!
   field &curfield = field_at(x,y);
   if(curfield.fieldCount() > 0){
	   field_entry *cur = NULL;
	   for(std::map<field_id, field_entry*>::iterator field_list_it = curfield.getFieldStart(); field_list_it != curfield.getFieldEnd(); ++field_list_it){
		   cur = field_list_it->second;
		   if(cur == NULL) continue;

		   if(!fieldlist[cur->getFieldType()].transparent[cur->getFieldDensity() - 1]) {
			   // Fields are either transparent or not, however we want some to be translucent
			   switch(cur->getFieldType()) {
			   case fd_smoke:
			   case fd_toxic_gas:
			   case fd_tear_gas:
				   if(cur->getFieldDensity() == 3)
					   transparency_cache[x][y] = LIGHT_TRANSPARENCY_SOLID;
				   if(cur->getFieldDensity() == 2)
					   transparency_cache[x][y] *= 0.5;
				   break;
			   case fd_nuke_gas:
				   transparency_cache[x][y] *= 0.5;
				   break;
			   default:
				   transparency_cache[x][y] = LIGHT_TRANSPARENCY_SOLID;
				   break;
			   }
		   }

		   // TODO: [lightmap] Have glass reduce light as well
	   }
   }
  }
 }
}

void map::build_seen_cache(game *g)
{
  memset(seen_cache, false, sizeof(seen_cache));
  const int j = (SEEX * my_MAPSIZE) - 1;
  for (int i = 0; i < SEEX * my_MAPSIZE; i++) {
    cache_seen(g->u.posx, g->u.posy, 0, i, 60);
    cache_seen(g->u.posx, g->u.posy, i, 0, 60);
    cache_seen(g->u.posx, g->u.posy, j, i, 60);
    cache_seen(g->u.posx, g->u.posy, i, j, 60);
  }
}

void map::build_map_cache(game *g)
{
 build_outside_cache(g);

 build_transparency_cache();

 // Cache all the vehicle stuff in one loop
 VehicleList vehs = get_vehicles();
 for(int v = 0; v < vehs.size(); ++v) {
  for (std::vector<int>::iterator part = vehs[v].v->external_parts.begin();
       part != vehs[v].v->external_parts.end(); ++part) {
   int px = vehs[v].x + vehs[v].v->parts[*part].precalc_dx[0];
   int py = vehs[v].y + vehs[v].v->parts[*part].precalc_dy[0];
   if(INBOUNDS(px, py)) {
    if (vehs[v].v->is_inside(*part)) {
     outside_cache[px][py] = false;
    }
    if (vehs[v].v->part_flag(*part, vpf_opaque) && vehs[v].v->parts[*part].hp > 0) {
     int dpart = vehs[v].v->part_with_feature(*part , vpf_openable);
     if (dpart < 0 || !vehs[v].v->parts[dpart].open) {
      transparency_cache[px][py] = LIGHT_TRANSPARENCY_SOLID;
     }
    }
   }
  }
 }

 build_seen_cache(g);
 generate_lightmap(g);
}

//this returns points in a spiral pattern starting at center_x/center_y until it hits the radius. clockwise fashion
//credit to Tom J Nowell; http://stackoverflow.com/a/1555236/1269969
std::vector<point> closest_points_first(int radius, int center_x, int center_y)
{
    std::vector<point> points;
    int X,Y,x,y,dx,dy;
    X = Y = (radius * 2) + 1;
    x = y = dx = 0;
    dy = -1;
    int t = std::max(X,Y);
    int maxI = t * t;
    for(int i = 0; i < maxI; i++)
    {
        if ((-X/2 <= x) && (x <= X/2) && (-Y/2 <= y) && (y <= Y/2))
        {
            points.push_back(point(x + center_x, y + center_y));
        }
        if( (x == y) || ((x < 0) && (x == -y)) || ((x > 0) && (x == 1 - y)))
        {
            t = dx;
            dx = -dy;
            dy = t;
        }
        x += dx;
        y += dy;
    }
    return points;
}

tinymap::tinymap()
{
 nulter = t_null;
 nultrap = tr_null;
}

tinymap::tinymap(std::vector<trap*> *trptr)
{
 nulter = t_null;
 nultrap = tr_null;
 traps = trptr;
 my_MAPSIZE = 2;
 for (int n = 0; n < 4; n++)
  grid[n] = NULL;
 veh_in_active_range = true;
 memset(veh_exists_at, 0, sizeof(veh_exists_at));
}

tinymap::~tinymap()
{
}
